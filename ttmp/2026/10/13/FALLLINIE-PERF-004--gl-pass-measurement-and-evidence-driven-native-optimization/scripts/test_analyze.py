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
        self.reset_fixture()

    def reset_fixture(self):
        self.p.write_text('frame,interval_ms,player_ms,stream_ms\n60,20,1,0.2\n')
        self.write('.passes.csv', 'frame,view,pass,parent,cpu_ms\n60,0,view,none,10\n60,0,scene,view,7\n60,0,dynamic_prep,scene,1\n60,0,post,view,2\n60,0,output,view,1\n')
        self.write('.stats.csv', 'mode,cadence,supported,issued,valid,disjoint,unavailable_polls,skipped_pool,unsupported_scopes,misuse,disjoint_polls,pending,cpu_dropped,schema_version,cpu_samples\npasses,60,1,2,1,1,0,0,0,0,1,0,0,2,5\n')
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n60,0,scene,valid,2\n60,0,post,disjoint,\n')

    def write(self, suffix, text):
        Path(str(self.p)+suffix).write_text(text)

    def test_nested_cpu_not_summed_and_disjoint_excluded(self):
        result = analyze(self.p)
        self.assertEqual(result['gpu_scopes_no_cross_mode_sum']['0/scene']['mean_ms'], 2)
        self.assertEqual(len(result['gpu_scopes_no_cross_mode_sum']), 1)
        self.assertEqual(len(result['cpu_scopes_nested_not_additive']), 5)
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

    def remove_scope(self, scope):
        p = Path(str(self.p)+'.passes.csv')
        p.write_text(''.join(line for line in p.read_text().splitlines(keepends=True)
                             if not line.startswith(f'60,0,{scope},')))

    def test_header_only_scopes_rejected(self):
        self.write('.passes.csv', 'frame,view,pass,parent,cpu_ms\n')
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_truncated_cpu_leaf_rejected(self):
        self.remove_scope('dynamic_prep')  # no GPU counterpart; count must catch it
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_orphan_cpu_parent_rejected_even_with_reconciled_count(self):
        for scope in ('view', 'scene'):
            with self.subTest(scope=scope):
                self.reset_fixture()
                self.remove_scope(scope)
                p = Path(str(self.p)+'.stats.csv')
                p.write_text(p.read_text().replace(',2,5\n', ',2,4\n'))
                with self.assertRaisesRegex(ValueError, 'parent'): analyze(self.p)

    def test_every_gpu_status_requires_cpu_counterpart(self):
        for status in ('valid', 'disjoint', 'pending', 'pending_disjoint'):
            with self.subTest(status=status):
                self.reset_fixture()
                self.remove_scope('post')
                p = Path(str(self.p)+'.stats.csv')
                p.write_text(p.read_text().replace(',2,5\n', ',2,4\n'))
                self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n'
                           + f"60,0,post,{status},{'1' if status == 'valid' else ''}\n")
                with self.assertRaisesRegex(ValueError, 'CPU counterpart'): analyze(self.p)

    def test_unsupported_gpu_with_complete_cpu_accepted(self):
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n')
        p = Path(str(self.p)+'.stats.csv')
        p.write_text(p.read_text().splitlines()[0]+'\npasses,60,0,0,0,0,0,0,3,0,0,0,0,2,5\n')
        self.assertEqual(analyze(self.p)['gpu_scopes_no_cross_mode_sum'], {})
        self.assertEqual(analyze(self.p)['cpu_scope_integrity']['records'], 5)
        self.write('.passes.csv', 'frame,view,pass,parent,cpu_ms\n')
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_queries_off_missing_cpu_rejected(self):
        self.write('.gpu.csv', 'frame,view,pass,status,gpu_ms\n')
        p = Path(str(self.p)+'.stats.csv')
        p.write_text(p.read_text().splitlines()[0]+'\noff,60,0,0,0,0,0,0,0,0,0,0,0,2,5\n')
        self.assertEqual(analyze(self.p)['gpu_mode'], 'off')
        self.remove_scope('output')
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_legacy_stats_explicitly_versioned_not_silently_accepted(self):
        p = Path(str(self.p)+'.stats.csv')
        p.write_text('\n'.join(','.join(line.split(',')[:-2]) for line in p.read_text().splitlines())+'\n')
        with self.assertRaisesRegex(ValueError, 'legacy schema 1'): analyze(self.p)

    def test_missing_scope_file_rejected(self):
        Path(str(self.p)+'.passes.csv').unlink()
        with self.assertRaises(FileNotFoundError): analyze(self.p)

    def test_header_only_frame_file_rejected(self):
        self.p.write_text('frame,interval_ms,player_ms,stream_ms\n')
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_false_zero_count_rejected(self):
        self.write('.passes.csv', 'frame,view,pass,parent,cpu_ms\n')
        p = Path(str(self.p)+'.stats.csv')
        p.write_text(p.read_text().replace(',2,5\n', ',2,0\n'))
        with self.assertRaisesRegex(ValueError, 'CPU'): analyze(self.p)

    def test_warmup(self):
        result = analyze(self.p, 61)
        self.assertEqual(result['gpu_scopes_no_cross_mode_sum'], {})
        self.assertEqual(result['cpu_scopes_nested_not_additive'], {})


if __name__ == '__main__': unittest.main()
