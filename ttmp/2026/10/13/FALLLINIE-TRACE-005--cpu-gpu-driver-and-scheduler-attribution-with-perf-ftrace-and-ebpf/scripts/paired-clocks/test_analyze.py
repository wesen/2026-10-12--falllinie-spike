#!/usr/bin/env python3
"""Use C++ emitted fixtures, then corrupt copies in a temporary directory."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

from analyze import analyze

ROOT = Path(sys.argv.pop(1))


class PairedAnalyzerTests(unittest.TestCase):
    def test_timings_and_nested_counts(self):
        report = analyze(ROOT / "valid.jsonl")
        self.assertEqual(report["rows"], 4)
        self.assertEqual(report["paired_samples"], 4)
        event = report["groups"]["events/view=-1/none"]
        self.assertEqual(event["mean_wall_ns"], 100)
        self.assertEqual(event["mean_thread_cpu_ns"], 20)
        self.assertEqual(event["mean_nonrunning_estimate_ns"], 80)
        self.assertTrue(report["all_attempted_samples_valid"])
        self.assertEqual(analyze(ROOT / "integrated.csv.paired.jsonl")["rows"], 3)

    def test_unavailable_is_not_zero(self):
        report = analyze(ROOT / "failures.jsonl")
        self.assertEqual(report["rows"], 7)
        self.assertEqual(report["paired_samples"], 0)
        self.assertFalse(report["all_attempted_samples_valid"])
        self.assertIsNone(report["groups"]["events/view=-1/none"]["mean_thread_cpu_ns"])

    def test_drops_are_reconciled(self):
        report = analyze(ROOT / "overflow.jsonl")
        self.assertEqual(report["rows"], 65)
        self.assertEqual(report["final"]["dropped"], 6)
        self.assertTrue(report["transport_complete"])
        self.assertFalse(report["all_attempted_samples_valid"])

    def check_rejected(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.jsonl"
            path.write_text(text)
            with self.assertRaises((ValueError, KeyError, TypeError)):
                analyze(path)

    def test_truncation_and_corruption(self):
        text = (ROOT / "valid.jsonl").read_text()
        rows = [json.loads(line) for line in text.splitlines()]
        self.check_rejected("\n".join(text.splitlines()[:-1]) + "\n")
        self.check_rejected(text[:-1])
        self.check_rejected(text[:-15] + "\n")
        for index, key, value in [(1, "pid", rows[1]["pid"] + 1),
                                  (1, "tid", rows[1]["tid"] + 1),
                                  (1, "run_start_ns", 999),
                                  (1, "view", 9), (1, "frame", 1),
                                  (1, "cpu_status", "end_failed"),
                                  (1, "cpu_start_ns", -1),
                                  (1, "sequence", 999),
                                  (-1, "written", 0), (-1, "attempted", 0),
                                  (-1, "invalid_cpu", 1), (-1, "active", 1),
                                  (0, "schema", 2)]:
            changed = copy.deepcopy(rows)
            changed[index][key] = value
            with self.subTest(index=index, key=key):
                self.check_rejected("".join(json.dumps(r) + "\n" for r in changed))
        self.check_rejected(text + json.dumps(rows[1]) + "\n")
        self.check_rejected("".join(json.dumps(r) + "\n" for r in rows[:2] + rows[1:]))

    def test_negative_residual_retained(self):
        rows = [json.loads(line) for line in (ROOT / "valid.jsonl").read_text().splitlines()]
        rows[1]["cpu_end_ns"] = rows[1]["cpu_start_ns"] + 200
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "negative.jsonl"
            path.write_text("".join(json.dumps(r) + "\n" for r in rows))
            report = analyze(path)
            self.assertEqual(report["negative_estimates"], 1)
            self.assertEqual(report["groups"]["events/view=-1/none"]["mean_nonrunning_estimate_ns"], -100)


if __name__ == "__main__":
    unittest.main()
