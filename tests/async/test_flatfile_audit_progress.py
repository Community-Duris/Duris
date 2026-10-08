#!/usr/bin/env python3
"""Timestamp refusals preserve read-only flatfile audit checkpoints."""
import copy
from pathlib import Path
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import flatfile_economic_audit as audit


class FlatfileAuditProgressTests(unittest.TestCase):
    def test_timestamp_bounds_and_order_refuse_invalid_progress(self):
        source = "a" * 64
        value = audit.new_progress(source, 100.0)
        self.assertIs(audit.validate(value, source, 100.0), value)
        for field in ("started_at", "last_page_at"):
            for invalid in (True, "100", -1, float("nan"), float("inf"), 100.001):
                with self.subTest(field=field, invalid=invalid):
                    changed = copy.deepcopy(value)
                    changed[field] = invalid
                    with self.assertRaises(audit.AuditError):
                        audit.validate(changed, source, 100.0)
        reversed_times = dict(value, started_at=100.0, last_page_at=99.0)
        with self.assertRaises(audit.AuditError):
            audit.validate(reversed_times, source, 101.0)

    def test_backward_clock_refuses_before_native_page_or_checkpoint_change(self):
        source = "a" * 64
        for authority_links in (False, True):
            with self.subTest(authority_links=authority_links):
                previous = audit.new_progress(source, 100.0, authority_links)
                original = copy.deepcopy(previous)
                with mock.patch.object(audit, "source_digest", return_value=source), \
                     mock.patch.object(audit.time, "time", return_value=99.0), \
                     mock.patch.object(audit.subprocess, "run") as native:
                    with self.assertRaises(audit.AuditError):
                        audit.scan(Path("/private/fixture"), Path("/private/qualifier"),
                                   previous, authority_links=authority_links)
                native.assert_not_called()
                self.assertEqual(previous, original)


if __name__ == "__main__":
    unittest.main()
