#!/usr/bin/env python3
"""Validate TRACE005 paired clock JSONL v1; no PERF004 schema changes."""
import argparse
import json
from pathlib import Path

IDENTITY = ("schema", "pid", "tid", "run_start_ns")
PASSES = {"view", "scene", "post", "output", "dynamic_prep", "dynamic_upload"}
STAGES = {"render", "events", "physics", "player", "context", "swap", "pacing"}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def number(row, key, minimum=0):
    value = row[key]
    require(type(value) is int and value >= minimum, f"invalid {key}: {value!r}")
    return value


def duration(row, clock):
    a, b = row[f"{clock}_start_ns"], row[f"{clock}_end_ns"]
    for value in (a, b):
        require(value is None or (type(value) is int and value >= 0), "invalid timestamp")
    expected = ("both_failed" if a is None and b is None else
                "start_failed" if a is None else "end_failed" if b is None else
                "regressed" if b < a else "valid")
    require(row[f"{clock}_status"] == expected, f"inconsistent {clock} status")
    return b - a if expected == "valid" else None


def analyze(path):
    meta = final = None
    sequences, scopes, groups = set(), set(), {}
    invalid = {"wall": 0, "cpu": 0}
    rows = paired = negative = 0
    with Path(path).open() as source:
        for line_number, line in enumerate(source, 1):
            require(line.endswith("\n"), f"unterminated line {line_number}")
            row = json.loads(line)
            require(type(row) is dict, "record must be an object")
            require(final is None, "data after final record")
            require(number(row, "schema") == 1, "unsupported schema")
            number(row, "pid", 1); number(row, "tid", 1); number(row, "run_start_ns")
            kind = row["record"]
            if meta is None:
                require(kind == "meta", "first record must be meta")
                require(1 <= number(row, "cadence") <= 3600, "invalid cadence")
                require(number(row, "capacity") == 64, "invalid v1 capacity")
                require(row["wall_clock"] == "CLOCK_MONOTONIC" and
                        row["cpu_clock"] == "CLOCK_THREAD_CPUTIME_ID", "unexpected clocks")
                meta = row
                continue
            require(all(row[k] == meta[k] for k in IDENTITY), "run/PID/TID identity mismatch")
            if kind == "final":
                final = row
                continue
            require(kind == "sample", "unexpected record kind")
            seq = number(row, "sequence", 1)
            require(seq not in sequences, "duplicate sequence")
            sequences.add(seq)
            frame = number(row, "frame")
            require(frame % meta["cadence"] == 0, "frame violates cadence")
            stage, view, render_pass = row["stage"], row["view"], row["pass"]
            require(stage in STAGES and type(view) is int, "invalid stage/view")
            require((stage == "render" and view in (0, 1) and render_pass in PASSES) or
                    (stage in ("context", "swap") and view in (0, 1) and render_pass == "none") or
                    (stage in ("events", "physics", "player", "pacing") and view == -1 and render_pass == "none"),
                    "invalid frame/view/pass/stage identity")
            scope = (frame, view, render_pass, stage)
            require(scope not in scopes, "duplicate scope identity")
            scopes.add(scope)
            wall, cpu = duration(row, "wall"), duration(row, "cpu")
            rows += 1
            invalid["wall"] += wall is None
            invalid["cpu"] += cpu is None
            key = f"{stage}/view={view}/{render_pass}"
            group = groups.setdefault(key, dict(samples=0, paired_samples=0, wall_ns=0, thread_cpu_ns=0, nonrunning_estimate_ns=0, negative_estimates=0))
            group["samples"] += 1
            if wall is not None and cpu is not None:
                paired += 1
                negative += wall < cpu
                group["paired_samples"] += 1
                group["wall_ns"] += wall
                group["thread_cpu_ns"] += cpu
                group["nonrunning_estimate_ns"] += wall - cpu
                group["negative_estimates"] += wall < cpu
    require(meta is not None and final is not None, "missing meta/final: truncated collection")
    for key in ("attempted", "written", "dropped", "invalid_wall", "invalid_cpu", "active"):
        number(final, key)
    require(final["active"] == 0, "unfinished scopes")
    require(final["written"] == rows, "final written count mismatch")
    require(final["attempted"] == rows + final["dropped"], "attempted/written/dropped mismatch")
    require(not sequences or max(sequences) <= final["attempted"], "sequence exceeds attempted count")
    for clock in invalid:
        require(invalid[clock] <= final[f"invalid_{clock}"] <= invalid[clock] + final["dropped"],
                f"invalid {clock} count mismatch")
    for group in groups.values():
        n = group["paired_samples"]
        for field in ("wall_ns", "thread_cpu_ns", "nonrunning_estimate_ns"):
            group["mean_" + field] = group.pop(field) / n if n else None
    return dict(schema=1, identity={k: meta[k] for k in IDENTITY}, cadence=meta["cadence"],
                rows=rows, paired_samples=paired, negative_estimates=negative,
                final=final, transport_complete=True,
                all_attempted_samples_valid=not(final["dropped"] or final["invalid_wall"] or final["invalid_cpu"]),
                groups=groups,
                interpretation="Wall minus thread CPU is a non-running estimate with clock/accounting residual, NOT GPU wait. Nested scopes are nonadditive. Means use only same-row valid pairs.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sidecar", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(analyze(args.sidecar), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"invalid paired collection: {error}\n")


if __name__ == "__main__":
    main()
