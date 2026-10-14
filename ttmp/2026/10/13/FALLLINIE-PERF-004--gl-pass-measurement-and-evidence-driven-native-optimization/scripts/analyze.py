#!/usr/bin/env python3
"""Summarize one run; never add nested CPU intervals or combine GPU modes."""
import argparse
import collections
import csv
import json
import math
from pathlib import Path


def number(text):
    value = float(text)
    if not math.isfinite(value) or value < 0:
        raise ValueError(f"invalid timing: {text}")
    return value


def distribution(values):
    values = sorted(values)
    if not values:
        return {"n": 0, "mean_ms": None, "p50_ms": None, "p95_ms": None, "p99_ms": None}
    return dict(n=len(values), mean_ms=sum(values)/len(values),
                **{f"p{p}_ms": values[math.ceil(len(values)*p/100)-1] for p in (50, 95, 99)})


def rows(path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def analyze(path, warmup=60):
    path = Path(path)
    stats_rows = rows(Path(str(path)+".stats.csv"))
    if len(stats_rows) != 1:
        raise ValueError("missing/ambiguous final stats (run incomplete?)")
    stats = stats_rows[0]
    # Schema 1 did not attest CPU record completeness. Fail closed rather than
    # certifying old/missing records; retain raw receipts and rerun with schema 2.
    schema = stats.get("schema_version", "1")
    if schema == "1":
        raise ValueError("legacy schema 1 lacks CPU completeness count; retain raw receipts and regenerate with schema 2")
    if schema != "2":
        raise ValueError("unsupported profile stats schema")
    cpu_samples = int(stats["cpu_samples"])
    cadence = int(stats["cadence"])
    if cpu_samples < 0 or not 1 <= cadence <= 3600:
        raise ValueError("invalid CPU sample count or cadence")
    mode = stats["mode"]
    if mode not in ("off", "view", "passes"):
        raise ValueError("unknown GPU mode")
    if int(stats["misuse"]) or int(stats["cpu_dropped"]):
        raise ValueError("instrumentation misuse or CPU sample overflow")
    cpu = collections.defaultdict(list)
    cpu_seen = set()
    for row in rows(path):
        frame = int(row["frame"])
        if frame < 0 or frame in cpu_seen:
            raise ValueError("invalid/duplicate CPU frame")
        cpu_seen.add(frame)
        for key, text in row.items():
            if key != "frame":
                value = number(text)
                if frame >= warmup:
                    cpu[key].append(value)
    if not cpu_seen:
        raise ValueError("missing completed CPU frames")
    passes = collections.defaultdict(list)
    cpu_scopes = set()
    parents = {"view": "none", "scene": "view", "post": "view", "output": "view",
               "dynamic_prep": "scene", "dynamic_upload": "scene"}
    for row in rows(Path(str(path)+".passes.csv")):
        frame, view, scope = int(row["frame"]), int(row["view"]), row["pass"]
        key = (frame, view, scope)
        if frame not in cpu_seen or frame % cadence or key in cpu_scopes or view not in (0, 1) or parents.get(scope) != row["parent"]:
            raise ValueError("invalid/duplicate CPU scope or parent")
        cpu_scopes.add(key)
        value = number(row["cpu_ms"])
        if frame >= warmup:
            passes[f"{view}/{scope} (parent={row['parent']})"].append(value)
    if len(cpu_scopes) != cpu_samples:
        raise ValueError("CPU sample count differs from scope records")
    for frame, view, scope in cpu_scopes:
        parent = parents[scope]
        if parent != "none" and (frame, view, parent) not in cpu_scopes:
            raise ValueError("CPU scope is missing its parent record")
    for frame in cpu_seen:
        if frame % cadence == 0 and (frame, 0, "view") not in cpu_scopes:
            raise ValueError("sampled CPU frame lacks top view record")
    gpu = collections.defaultdict(list)
    status_counts = collections.Counter()
    all_status_counts = collections.Counter()
    seen = set()
    for row in rows(Path(str(path)+".gpu.csv")):
        frame, view, scope = int(row["frame"]), int(row["view"]), row["pass"]
        allowed = {"view"} if mode == "view" else {"scene", "post", "output"} if mode == "passes" else set()
        key = (frame, view, scope)
        if frame not in cpu_seen or scope not in allowed or view not in (0, 1) or key in seen:
            raise ValueError("invalid, duplicate or nested GPU scope")
        if key not in cpu_scopes:
            raise ValueError("issued GPU scope lacks CPU counterpart")
        seen.add(key)
        status = row["status"]
        if status not in ("valid", "disjoint", "pending", "pending_disjoint"):
            raise ValueError("unknown GPU status")
        value = number(row["gpu_ms"]) if status == "valid" else None
        if status != "valid" and row["gpu_ms"]:
            raise ValueError("invalid GPU sample carries numeric timing")
        all_status_counts[status] += 1
        if frame >= warmup:
            status_counts[status] += 1
            if value is not None:
                gpu[f"{view}/{scope}"].append(value)
    if len(seen) != int(stats["issued"]):
        raise ValueError("issued GPU count differs from terminal records")
    if (all_status_counts['valid'] != int(stats['valid']) or
        all_status_counts['disjoint'] != int(stats['disjoint']) or
        all_status_counts['pending']+all_status_counts['pending_disjoint'] != int(stats['pending'])):
        raise ValueError("GPU status counters differ from terminal records")
    receipt_path = path.parent/'receipt.json'
    measurement = json.loads(receipt_path.read_text()).get('measurement') if receipt_path.exists() else None
    if measurement and measurement.get('gpu_mode') != mode:
        raise ValueError("receipt/profile GPU modes differ")
    return {"profile": str(path), "warmup_frames": warmup, "gpu_mode": mode,
            "measurement_receipt": measurement,
            "cpu_scope_integrity": {"schema_version": 2, "records": len(cpu_scopes),
                                    "count_reconciled": True, "parent_closure": True,
                                    "gpu_counterparts": True},
            "cpu_intervals_independent_not_additive": {k: distribution(v) for k, v in cpu.items()},
            "cpu_scopes_nested_not_additive": {k: distribution(v) for k, v in passes.items()},
            "gpu_scopes_no_cross_mode_sum": {k: distribution(v) for k, v in gpu.items()},
            "postwarmup_gpu_status": dict(status_counts), "whole_run_query_stats": stats,
            "limits": "CPU submit includes driver/backpressure; GPU elapsed is not utilization. Empty GPU data is unavailable, never zero. Static terrain lacks dynamic/streaming/audio coverage. Compare repetitions, not pooled divergent live trajectories."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", type=Path)
    parser.add_argument("--warmup", type=int, default=60)
    args = parser.parse_args()
    if args.warmup < 0:
        parser.error("warmup must be nonnegative")
    print(json.dumps(analyze(args.profile, args.warmup), indent=2, allow_nan=False))
