#!/usr/bin/env python3
"""Comparator regressions: semantic mismatches cannot hide behind PASS prose."""
import importlib.util,pathlib,unittest
p=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('probe_compare',p/'04-compare.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
class TestOriginalFixtureComparison(unittest.TestCase):
 def test_float_tolerance(self):self.assertEqual(mod.compare(1.0,1.0+1e-10)[0],1)
 def test_float_drift_rejected(self):
  with self.assertRaises(AssertionError):mod.compare(1.0,1.01)
 def test_integer_not_float(self):
  with self.assertRaises(AssertionError):mod.compare(10260,10260.0)
 def test_unicode_label_exact(self):
  with self.assertRaises(AssertionError):mod.compare('FS Rodeo 360 · Indy','FS Rodeo 360 - Indy')
 def test_nonfinite_rejected(self):
  for x in [float('nan'),float('inf')]:
   with self.assertRaises(AssertionError):mod.compare(1.0,x)
 def test_missing_schema_rejected(self):
  with self.assertRaises(AssertionError):mod.compare({'pts':1,'name':'x'},{'pts':1})
 def test_count(self):self.assertEqual(mod.compare({'a':[1,2.0,'label']},{'a':[1,2.0,'label']})[0],3)
if __name__=='__main__':unittest.main()
