#!/usr/bin/env python3
import tempfile
import unittest
from pathlib import Path
from analyze import analyze


class AnalyzeTests(unittest.TestCase):
    def setUp(self):
        work = Path(__file__).parent / '.work'
        work.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=work)
        self.addCleanup(self.temp.cleanup)
        self.p = Path(self.temp.name)/'frames.csv'
        self.p.write_text('frame,interval_ms,player_ms,stream_ms\n60,20,1,0.2\n')
        self.write('.passes.csv', 'frame,view,pass,parent,cpu_ms\n60,0,view,none,10\n60,0,scene,view,7\n60,0,dynamic_prep,scene,1\n')
        self.write('.stats.csv', 'mode,cadence,supported,issued,valid,disjoint,unavailable_polls,skipped_pool,unsupported_scopes,misuse,disjoint_polls,pending,cpu_dropped\npasses,60,1,2,1,1,0,0,0,0,1,0,0\n')
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n60,0,scene,valid,2\n60,0,post,disjoint,\n')

    def write(self, suffix, text):
        Path(str(self.p)+suffix).write_text(text)

    def test_nested_cpu_not_summed_and_disjoint_excluded(self):
        result = analyze(self.p)
        self.assertEqual(result['gpu_scopes_no_cross_mode_sum']['0/scene']['mean_ms'], 2)
        self.assertEqual(len(result['gpu_scopes_no_cross_mode_sum']), 1)
        self.assertEqual(len(result['cpu_scopes_nested_not_additive']), 3)
        self.assertNotIn('total', result)

    def test_disjoint_numeric_rejected(self):
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n60,0,post,disjoint,0\n')
        with self.assertRaises(ValueError): analyze(self.p)

    def test_invalid_timings_rejected(self):
        for value in ('nan', 'inf', '-1', ''):
            self.write('.gpu.csv', f'frame,view,pass,status,gpu_ms\n60,0,scene,valid,{value}\n')
            with self.assertRaises(ValueError): analyze(self.p)

    def test_nested_gpu_rejected(self):
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n60,0,view,valid,3\n60,0,scene,valid,2\n')
        with self.assertRaises(ValueError): analyze(self.p)

    def test_pending_unavailable_not_zero(self):
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n60,0,scene,pending,\n60,0,post,pending_disjoint,\n')
        stats = Path(str(self.p)+'.stats.csv')
        stats.write_text(stats.read_text().replace('passes,60,1,2,1,1,0,0,0,0,1,0,0', 'passes,60,1,2,0,0,0,0,0,0,1,2,0'))
        self.assertEqual(analyze(self.p)['gpu_scopes_no_cross_mode_sum'], {})

    def test_incomplete_rejected(self):
        self.write('.stats.csv', '')
        with self.assertRaises(ValueError): analyze(self.p)

    def test_counter_mismatch_rejected(self):
        stats = Path(str(self.p)+'.stats.csv')
        stats.write_text(stats.read_text().replace('passes,60,1,2,1,1', 'passes,60,1,2,2,0'))
        with self.assertRaises(ValueError): analyze(self.p)

    def test_receipt_mode_mismatch_rejected(self):
        (self.p.parent/'receipt.json').write_text('{"measurement":{"fixture":"static-terrain","gpu_mode":"view"}}')
        with self.assertRaises(ValueError): analyze(self.p)

    def test_warmup(self):
        result = analyze(self.p, 61)
        self.assertEqual(result['gpu_scopes_no_cross_mode_sum'], {})
        self.assertEqual(result['cpu_scopes_nested_not_additive'], {})


if __name__ == '__main__': unittest.main()
