#!/usr/bin/env python3
"""Focused pure evidence tests for canonical artifact reconciliation."""

from __future__ import annotations

import importlib.util
from pathlib import Path
from typing import Any
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "player_death_restitution_reconciliation",
    ROOT / "scripts" / "player_death_restitution_reconciliation.py",
)
assert SPEC is not None and SPEC.loader is not None
rules = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(rules)


class ArtifactReconciliationTests(unittest.TestCase):
    TEST_PID = 77
    TEST_VNUM = 91042
    TEST_UID = 7001

    def item(self, *, flags: int = 0, name: str = "ordinary gloves") -> dict[str, Any]:
        return {
            "object_uid": self.TEST_UID,
            "vnum": self.TEST_VNUM,
            "extra_flags": flags,
            "name_hex": name.encode().hex(),
        }

    def current(self, *, uid: int | None = None, vnum: int | None = None) -> dict[str, Any]:
        return {
            "item_uid": self.TEST_UID if uid is None else uid,
            "vnum": self.TEST_VNUM if vnum is None else vnum,
            "owner_type": 1,
            "owner_id": self.TEST_PID,
            "owner_context_id": 0,
            "item_revision": 3,
            "state": 3,
        }

    def artifacts(self, *, location: int = -2, domain: dict[str, Any] | None = None,
                  bind: dict[str, Any] | None = None) -> dict[str, Any]:
        legacy = {
            "vnum": self.TEST_VNUM,
            "owned": "Y",
            "loc_type": 3,
            "location": location,
            "timer": 1_600_000_000,
            "artifact_type": 2,
        }
        key = str(self.TEST_VNUM)
        result: dict[str, Any] = {
            "domain": {} if domain is None else {key: domain},
            "domain_table_present": True,
            "baseline": {},
            "baseline_table_present": True,
            "bind": {} if bind is None else {key: bind},
            "bind_table_present": True,
            "mortal": {key: dict(legacy)},
            "god": {key: dict(legacy)},
            "competitors": {},
        }
        return result

    def test_native_flag_and_unique_name_are_distinct(self) -> None:
        ordinary_named_unique = self.item(name="some unique gloves")
        self.assertEqual(rules.classify_item(ordinary_named_unique, {})["kind"], "unique")
        real_artifact = self.item(flags=rules.REAL_ARTIFACT_FLAG)
        self.assertEqual(rules.classify_item(real_artifact, {})["kind"], "artifact")

    def test_native_bit_29_is_one_based(self) -> None:
        # Independent native value from defines.h; do not derive both sides
        # from the Python constant and accidentally test an off-by-one twice.
        self.assertEqual(rules.REAL_ARTIFACT_FLAG, 268435456)
        self.assertTrue(rules.classify_item(self.item(flags=268435456), {})["authority_required"])
        self.assertFalse(rules.classify_item(self.item(flags=536870912), {})["authority_required"])

    def test_missing_domain_native_unknown_is_recoverable_with_explicit_reconciliation(self) -> None:
        artifacts = self.artifacts(
            bind={"vnum": self.TEST_VNUM, "owner_pid": self.TEST_PID, "timer": 654321},
        )
        decision = rules.reconcile_artifact_authority(
            self.item(name="some unique gloves"), self.TEST_PID, self.current(), artifacts, []
        )
        self.assertTrue(decision["ok"])
        self.assertTrue(decision["reconciliation_required"])
        self.assertEqual(decision["mode"], "missing_domain")
        self.assertTrue(decision["legacy_unknown"])
        self.assertEqual(decision["domain_seed"]["item_uid"], 7001)
        self.assertEqual(decision["domain_seed"]["timer_epoch"], 1_600_000_000)
        self.assertEqual(decision["domain_seed"]["bind_owner_pid"], self.TEST_PID)
        self.assertEqual(decision["domain_seed"]["bind_timer_epoch"], 654321)
        self.assertEqual(decision["baseline_seed"]["opening_revision"], 0)

    def test_competing_current_uid_is_a_no_write_refusal(self) -> None:
        artifacts = self.artifacts()
        decision = rules.reconcile_artifact_authority(
            self.item(), self.TEST_PID, self.current(), artifacts,
            [{"item_uid": 9001, "vnum": self.TEST_VNUM, "state": 3}],
        )
        self.assertFalse(decision["ok"])
        self.assertEqual(decision["classification"], "artifact_competing_instance")

    def test_legacy_tables_must_agree_and_unknown_npc_marker_is_not_accepted(self) -> None:
        artifacts = self.artifacts(location=-2)
        key = str(self.TEST_VNUM)
        artifacts["god"][key]["timer"] = 1_600_000_001
        decision = rules.reconcile_artifact_authority(self.item(), self.TEST_PID, self.current(), artifacts, [])
        self.assertFalse(decision["ok"])
        self.assertEqual(decision["classification"], "artifact_legacy_conflict")

        artifacts = self.artifacts(location=-2)
        artifacts["mortal"][key]["loc_type"] = 2
        decision = rules.reconcile_artifact_authority(self.item(), self.TEST_PID, self.current(), artifacts, [])
        self.assertFalse(decision["ok"])
        self.assertEqual(decision["classification"], "artifact_legacy_conflict")

    def test_existing_domain_and_baseline_preserve_identity_and_metadata(self) -> None:
        domain = {
            "vnum": self.TEST_VNUM,
            "owned": 1,
            "loc_type": 3,
            "location": -2,
            "timer_epoch": 1_600_000_000,
            "artifact_type": 2,
            "bind_owner_pid": self.TEST_PID,
            "bind_timer_epoch": 654321,
            "item_uid": 7001,
            "item_revision": 3,
            "revision": 8,
        }
        artifacts = self.artifacts(
            domain=domain,
            bind={"vnum": self.TEST_VNUM, "owner_pid": self.TEST_PID, "timer": 654321},
        )
        key = str(self.TEST_VNUM)
        artifacts["baseline"][key] = {
            "vnum": self.TEST_VNUM,
            "opening_timer_epoch": 1_600_000_000,
            "opening_bind_owner_pid": self.TEST_PID,
            "opening_bind_timer_epoch": 654321,
            "opening_revision": 0,
        }
        decision = rules.reconcile_artifact_authority(
            self.item(name="some unique gloves"), self.TEST_PID, self.current(), artifacts, [], 1_599_999_000
        )
        self.assertTrue(decision["ok"])
        self.assertFalse(decision["reconciliation_required"])
        candidate = {
            "item_uid": self.TEST_UID,
            "vnum": self.TEST_VNUM,
            "artifact_before": decision["domain_before"],
            "artifact_reconciliation": decision,
            "artifact_timing": {
                "loss_epoch": decision["loss_epoch"],
                "source_timer_epoch": decision["source_timer_epoch"],
                "usable_lifetime_seconds": decision["usable_lifetime_seconds"],
                "basis": "historical_loss_remainder",
                "compensation_reference": "",
            },
            "expected_current": {"item_revision": 3},
        }
        expected = rules.final_domain_expectation(candidate, self.TEST_PID)
        self.assertIsNone(expected["timer_epoch"])
        self.assertEqual(expected["timer_epoch_mode"], "delivery_epoch_plus_usable_lifetime")
        self.assertEqual(expected["usable_lifetime_seconds"], 1_000)
        self.assertEqual(expected["bind_timer_epoch"], 654321)
        self.assertEqual(expected["revision"], 9)
        self.assertEqual(expected["item_revision"], 4)
    def test_absolute_poof_epoch_becomes_remaining_lifetime_at_loss(self) -> None:
        artifacts = self.artifacts(
            domain={
                "vnum": self.TEST_VNUM,
                "owned": 1,
                "loc_type": 3,
                "location": -2,
                "timer_epoch": 1_600_000_000,
                "artifact_type": 2,
                "bind_owner_pid": self.TEST_PID,
                "bind_timer_epoch": 654321,
                "item_uid": self.TEST_UID,
                "item_revision": 3,
                "revision": 8,
            },
            bind={"vnum": self.TEST_VNUM, "owner_pid": self.TEST_PID, "timer": 654321},
        )
        decision = rules.reconcile_artifact_authority(
            self.item(), self.TEST_PID, self.current(), artifacts, [], 1_599_999_100
        )
        self.assertTrue(decision["ok"])
        self.assertEqual(decision["timing_status"], "historical")
        self.assertEqual(decision["usable_lifetime_seconds"], 900)
        self.assertNotEqual(decision["usable_lifetime_seconds"], decision["source_timer_epoch"])

    def test_missing_or_expired_timing_is_not_fabricated(self) -> None:
        artifacts = self.artifacts()
        missing = rules.reconcile_artifact_authority(
            self.item(), self.TEST_PID, self.current(), artifacts, [], 1_599_999_100
        )
        self.assertEqual(missing["timing_status"], "historical")
        artifacts["mortal"][str(self.TEST_VNUM)]["timer"] = 0
        artifacts["god"][str(self.TEST_VNUM)]["timer"] = 0
        missing = rules.reconcile_artifact_authority(
            self.item(), self.TEST_PID, self.current(), artifacts, [], 1_599_999_100
        )
        self.assertEqual(missing["timing_status"], "missing")
        expired = rules.reconcile_artifact_authority(
            self.item(), self.TEST_PID, self.current(), self.artifacts(), [], 1_600_000_000
        )
        self.assertEqual(expired["timing_status"], "expired_at_loss")
        self.assertEqual(expired["usable_lifetime_seconds"], 0)


if __name__ == "__main__":
    unittest.main()
