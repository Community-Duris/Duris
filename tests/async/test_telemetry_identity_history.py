#!/usr/bin/env python3
"""Offline reviewed association history and exact effort acceptance examples."""
from __future__ import annotations

from copy import deepcopy
from dataclasses import replace
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))
import identity_history as identity


def association(association_id=1, account_token=100, controller_token=500, first=0, last=None, **updates):
    row = {"association_id": association_id, "account_token": account_token,
           "controller_token": controller_token, "valid_from_utc_usec": first,
           "valid_through_utc_usec": last, "status": "confirmed", "provenance": "staff_review",
           "evidence_digest": "e" * 64}
    row.update(updates)
    return row


def packet(rows=None, **updates):
    result = {"registry_schema_version": 1, "environment_id": 8, "season_id": 7,
        "registry_version": 1, "previous_registry_version": 0, "previous_packet_digest": None,
        "reviewed_from_utc_usec": 0, "reviewed_through_utc_usec": 1_000,
        "reviewed_at_utc_usec": 1_000, "reviewer_token": "b" * 64,
        "review_evidence_digest": "a" * 64, "associations": [association()] if rows is None else rows}
    result.update(updates)
    return result


def interval(subject=9001, seq=None, first=10, last=110, **updates):
    values = dict(scope=(8, 7), session=(100, 200, subject), replay=(300, 400, subject + 100 if seq is None else seq),
        subject_id=subject, pid=subject, config_id=9, category="active",
        start_monotonic_usec=first, end_monotonic_usec=last,
        start_utc_usec=first - 10, end_utc_usec=last - 10)
    values.update(updates)
    return identity.ObservedInterval(**values)


def ownership(i, token=100, at=0, seq=None, **updates):
    values = dict(scope=i.scope, session=i.session, replay=(*i.replay[:2], i.subject_id if seq is None else seq),
        subject_id=i.subject_id, pid=i.pid, at_monotonic_usec=at, account_token=token,
        source="unavailable" if token is None else "authenticated_login")
    values.update(updates)
    return identity.OwnershipObservation(**values)


def cell(rows, basis, token):
    return next(row for row in rows if row.basis == basis and row.token == token)


class IdentityHistoryTest(unittest.TestCase):
    def setUp(self):
        self.registry = identity.Registry.from_packet(packet())

    def test_complete_packet_deterministic_order_and_private_fields(self):
        a, b = association(), association(2, 101)
        first = identity.Registry.from_packet(packet([a, b]))
        second = identity.Registry.from_packet(packet([b, a]))
        self.assertEqual(first.packet_digest, second.packet_digest)
        self.assertEqual(first.linkage_at(101, 10), (500, 2, "confirmed"))
        for name in ("account_name", "ip", "email", "device", "character_name"):
            bad = packet(**{name: "private-value"})
            with self.subTest(name=name), self.assertRaises(identity.IdentityError):
                identity.Registry.from_packet(bad)

    def test_typed_packet_negative_boundaries(self):
        failures = [packet(environment_id=True), packet(season_id=0), packet(registry_schema_version=2),
            packet(registry_version=3), packet(previous_packet_digest="f" * 64),
            packet(reviewed_from_utc_usec=None), packet(reviewed_at_utc_usec=999),
            packet(review_evidence_digest="0" * 64), packet(associations={}),
            packet([association(account_token=0)]), packet([association(controller_token=None)]),
            packet([association(status="unknown")]), packet([association(status={})]),
            packet([association(provenance="ip_similarity")]), packet([association(first=None)]),
            packet([association(last=0)]), packet([association(first=1_000)]),
            packet([association(last=1_001)]), packet([association(), association()])]
        for value in failures:
            with self.subTest(value=value), self.assertRaises(identity.IdentityError):
                identity.Registry.from_packet(value)

    def test_same_account_overlap_refused_different_accounts_allowed(self):
        for rows in ([association(last=50), association(2, first=49)],
                     [association(), association(2, first=50)],
                     [association(last=50), association(2, controller_token=None, first=49, status="unknown")]):
            with self.assertRaises(identity.IdentityError):
                identity.Registry.from_packet(packet(rows))
        registry = identity.Registry.from_packet(packet([association(last=50), association(2, first=50), association(3, 101)]))
        self.assertEqual(registry.linkage_at(100, 50), (500, 2, "confirmed"))

    def test_review_window_clips_open_ends_and_missing_accounts(self):
        registry = identity.Registry.from_packet(packet(reviewed_from_utc_usec=20, reviewed_through_utc_usec=80))
        self.assertEqual(registry.linkage_at(100, 19)[2], "outside_reviewed_window")
        self.assertEqual(registry.linkage_at(100, 20)[0], 500)
        self.assertEqual(registry.linkage_at(100, 79)[0], 500)
        self.assertEqual(registry.linkage_at(100, 80)[2], "outside_reviewed_window")
        self.assertEqual(registry.linkage_at(999, 40)[2], "no_reviewed_mapping")
        self.assertEqual(registry.linkage_at(None, 40)[2], "unknown_account")
        self.assertEqual(registry.linkage_at(100, None)[2], "clock_unknown")

    def test_confirmed_unknown_and_withdrawn_remain_distinct(self):
        registry = identity.Registry.from_packet(packet([association(last=30),
            association(2, controller_token=None, first=30, last=60, status="unknown"),
            association(3, first=60, status="withdrawn")]))
        self.assertEqual(registry.linkage_at(100, 29), (500, 1, "confirmed"))
        self.assertEqual(registry.linkage_at(100, 30), (None, 2, "unknown"))
        self.assertEqual(registry.linkage_at(100, 60), (None, None, "no_reviewed_mapping"))

    def test_append_correction_preserves_original_version(self):
        corrected = packet([association(controller_token=501, evidence_digest="d" * 64)],
            registry_version=2, previous_registry_version=1, previous_packet_digest=self.registry.packet_digest)
        second = identity.Registry.from_packet(corrected)
        self.assertEqual(identity.validate_successor(self.registry, second), "new_version")
        self.assertEqual(self.registry.linkage_at(100, 20)[0], 500)
        self.assertEqual(second.linkage_at(100, 20)[0], 501)
        self.assertEqual(identity.validate_successor(self.registry, deepcopy(self.registry)), "already_registered")
        with self.assertRaises(identity.IdentityError):
            identity.validate_successor(self.registry, identity.Registry.from_packet(packet(reviewer_token="c" * 64)))

    def test_append_requires_scope_digest_ids_and_immutable_account(self):
        for rows, updates in (([], {}), ([association(account_token=101)], {}),
             ([association()], {"previous_packet_digest": "f" * 64}),
             ([association()], {"season_id": 9}),
             ([association()], {"reviewed_through_utc_usec": 800, "reviewed_at_utc_usec": 900})):
            p = packet(rows, registry_version=2, previous_registry_version=1, previous_packet_digest=self.registry.packet_digest)
            p.update(updates)
            second = identity.Registry.from_packet(p)
            with self.subTest(updates=updates), self.assertRaises(identity.IdentityError):
                identity.validate_successor(self.registry, second)
        withdrawal = identity.Registry.from_packet(packet([association(status="withdrawn")],
            registry_version=2, previous_registry_version=1, previous_packet_digest=self.registry.packet_digest))
        self.assertEqual(identity.validate_successor(self.registry, withdrawal), "new_version")
        with self.assertRaises(identity.IdentityError):
            identity.validate_successor(None, withdrawal)

    def test_packet_capacity_decode_duplicate_nan_and_cli_privacy(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "bin") as directory:
            path = Path(directory) / "private-account-name.json"
            path.write_text(json.dumps(packet()), encoding="utf-8")
            self.assertEqual(identity.load_registry(path).packet_digest, self.registry.packet_digest)
            valid = subprocess.run([sys.executable, str(ROOT / "scripts/telemetry/identity_history.py"), str(path)],
                                   text=True, capture_output=True)
            self.assertEqual(valid.returncode, 0, valid.stderr)
            self.assertNotIn("reviewer_token", valid.stdout)
            for data in ('{"environment_id":8,"environment_id":9}', '{"x":NaN}', '{"x":1e9999}', '{',
                         '{"environment_id":' + '9' * 10_000 + '}',
                         'x' * (identity.MAX_PACKET_BYTES + 1)):
                path.write_text(data, encoding="utf-8")
                with self.assertRaises(identity.IdentityError):
                    identity.load_registry(path)
                refused = subprocess.run([sys.executable, str(ROOT / "scripts/telemetry/identity_history.py"), str(path)],
                                         text=True, capture_output=True)
                self.assertEqual(refused.returncode, 1)
                self.assertNotIn("private-account-name", refused.stdout + refused.stderr)
            path.write_text(json.dumps(packet()), encoding="utf-8")
            successor = Path(directory) / "v2.json"
            successor.write_text(json.dumps(packet(registry_version=2, previous_registry_version=1,
                previous_packet_digest=self.registry.packet_digest)), encoding="utf-8")
            self.assertEqual(subprocess.run([sys.executable, str(ROOT / "scripts/telemetry/identity_history.py"),
                str(successor), "--previous", str(path)], capture_output=True).returncode, 0)

    def test_attribution_never_backfills_before_first_observed_owner(self):
        i = interval()
        slices = identity.attribute_interval(i, [ownership(i, at=30)], self.registry)
        self.assertEqual([(r.duration_usec, r.account_token) for r in slices], [(20, None), (80, 100)])
        self.assertEqual(sum(r.duration_usec for r in slices), 100)
        self.assertEqual(slices[0].linkage_status, "unknown_account")

    def test_transfer_and_dated_controller_change_conserve_time(self):
        registry = identity.Registry.from_packet(packet([association(last=50),
            association(2, controller_token=501, first=50), association(3, 101, 502)]))
        i = interval()
        slices = identity.attribute_interval(i, [ownership(i), ownership(i, token=101, at=80, seq=2,
            source="authenticated_change")], registry)
        self.assertEqual([(r.duration_usec, r.account_token, r.controller_token) for r in slices],
                         [(50, 100, 500), (20, 100, 501), (30, 101, 502)])
        self.assertEqual(sum(r.duration_usec for r in slices), 100)

    def test_copyover_uses_new_producer_clock_and_requires_fresh_anchor(self):
        old = interval()
        resumed = replace(old, replay=(301, 401, 100), start_monotonic_usec=1, end_monotonic_usec=101)
        # Original session continuity cannot make the old monotonic clock comparable.
        slices = identity.attribute_interval(resumed, [ownership(old)], self.registry)
        self.assertEqual(slices[0].account_token, None)
        slices = identity.attribute_interval(resumed, [ownership(old), ownership(resumed, at=21, seq=3,
            source="authenticated_copyover")], self.registry)
        self.assertEqual([r.duration_usec for r in slices], [20, 80])
        self.assertEqual(slices[1].controller_token, 500)

    def test_same_scope_and_session_required_before_attribution(self):
        i = interval()
        for observed in (replace(ownership(i), scope=(8, 9)), replace(ownership(i), session=(100, 200, 9002)),
                         replace(ownership(i), subject_id=9002), replace(ownership(i), pid=9002)):
            with self.assertRaises(identity.IdentityError):
                identity.attribute_interval(i, [observed], self.registry)
        with self.assertRaises(identity.IdentityError):
            identity.attribute_interval(replace(i, scope=(9, 7)), [], self.registry)

    def test_identical_ownership_replay_and_ambiguous_boundary(self):
        i = interval()
        observation = ownership(i)
        self.assertEqual(identity.attribute_interval(i, [observation, observation], self.registry),
                         identity.attribute_interval(i, [observation], self.registry))
        for other in (replace(observation, account_token=101), replace(observation, account_token=101, replay=(300, 400, 2))):
            with self.assertRaises(identity.IdentityError):
                identity.attribute_interval(i, [observation, other], self.registry)

    def test_unknown_ownership_boundary_removes_attribution(self):
        i = interval()
        slices = identity.attribute_interval(i, [ownership(i), ownership(i, None, at=60, seq=2)], self.registry)
        self.assertEqual([(r.duration_usec, r.controller_token) for r in slices], [(50, 500), (50, None)])

    def test_unknown_mismatched_or_discontinuous_utc_retains_account_effort(self):
        for i in (interval(start_utc_usec=None, end_utc_usec=None), interval(end_utc_usec=101),
                  interval(quality_flags=identity.CLOCK_DISCONTINUITY), interval(quality_flags=1 << 16)):
            slices = identity.attribute_interval(i, [ownership(i)], self.registry)
            self.assertTrue(all(r.controller_token is None and r.linkage_status == "clock_unknown" for r in slices))
            totals = identity.union_effort(slices)
            account = cell(totals, "account", 100)
            self.assertEqual(account.character_usec, 100)
            self.assertEqual(account.unknown_clock_character_usec, 100)
            self.assertNotEqual(account.quality_flags & identity.UTC_ATTRIBUTION_FLAGS, 0)
            self.assertIsNone(account.union_usec)
            self.assertIsNone(cell(totals, "unknown_controller", None).covered_union_usec)

    def test_supported_late_and_incident_quality_retained_reserved_bits_refused(self):
        i = interval(quality_flags=(1 << 9) | (1 << 25) | (1 << 27))
        rows = identity.attribute_interval(i, [ownership(i)], self.registry)
        self.assertEqual(cell(identity.union_effort(rows), "controller", 500).quality_flags, i.quality_flags)
        with self.assertRaises(identity.IdentityError):
            interval(quality_flags=1 << 10)

    def test_six_overlapping_characters_are_six_effort_units_and_one_union(self):
        registry = identity.Registry.from_packet(packet([association(index, 99 + index) for index in range(1, 7)]))
        slices = []
        for index in range(6):
            i = interval(subject=9001 + index)
            slices.extend(identity.attribute_interval(i, [ownership(i, 100 + index)], registry))
        totals = identity.union_effort(slices)
        controller = cell(totals, "controller", 500)
        self.assertEqual((controller.character_usec, controller.union_usec), (600, 100))
        self.assertEqual((controller.distinct_characters, controller.distinct_accounts), (6, 6))
        self.assertEqual(len([row for row in totals if row.basis == "account"]), 6)

    def test_sequential_rotation_and_nested_intervals_use_exact_union(self):
        registry = identity.Registry.from_packet(packet([association(), association(2, 101)]))
        slices = []
        for subject, first, last, token in ((9001, 10, 110, 100), (9002, 110, 210, 101),
                                          (9003, 30, 50, 101), (9004, 50, 150, 100)):
            i = interval(subject=subject, first=first, last=last)
            slices.extend(identity.attribute_interval(i, [ownership(i, token)], registry))
        controller = cell(identity.union_effort(slices), "controller", 500)
        self.assertEqual((controller.character_usec, controller.union_usec), (320, 200))

    def test_unknown_controllers_cannot_be_combined_into_one_person(self):
        registry = identity.Registry.from_packet(packet([]))
        slices = []
        for subject, token in ((9001, 100), (9002, 101)):
            i = interval(subject=subject)
            slices.extend(identity.attribute_interval(i, [ownership(i, token)], registry))
        totals = identity.union_effort(slices)
        unknown = cell(totals, "unknown_controller", None)
        self.assertEqual(unknown.character_usec, 200)
        self.assertIsNone(unknown.union_usec)
        self.assertIsNone(unknown.covered_union_usec)
        self.assertEqual([row for row in totals if row.basis == "controller"], [])

    def test_active_idle_and_presence_and_configs_stay_separate(self):
        slices = []
        for index, category in enumerate(("active", "idle", "presence")):
            i = interval(subject=9001 + index, category=category)
            slices.extend(identity.attribute_interval(i, [ownership(i)], self.registry))
        other_config = interval(subject=9004, config_id=10)
        slices.extend(identity.attribute_interval(other_config, [ownership(other_config)], self.registry))
        active = [r for r in identity.union_effort(slices) if r.basis == "controller"]
        self.assertEqual([(r.config_id, r.character_usec) for r in active], [(9, 100), (10, 100)])
        self.assertEqual(cell(identity.union_effort(slices, category="presence"), "controller", 500).category, "presence")

    def test_presence_can_overlap_activity_without_becoming_active_time(self):
        active = interval()
        presence = replace(active, replay=(300, 400, 20000), category="presence")
        rows = identity.attribute_interval(active, [ownership(active)], self.registry) + identity.attribute_interval(
            presence, [ownership(presence)], self.registry)
        self.assertEqual(cell(identity.union_effort(rows), "controller", 500).character_usec, 100)
        self.assertEqual(cell(identity.union_effort(rows, category="presence"), "controller", 500).character_usec, 100)

    def test_review_clips_unknown_gaps_without_extending_controller_time(self):
        registry = identity.Registry.from_packet(packet([association(last=40), association(2, first=60)],
            reviewed_from_utc_usec=20, reviewed_through_utc_usec=80))
        i = interval()
        rows = identity.attribute_interval(i, [ownership(i)], registry)
        self.assertEqual([(r.duration_usec, r.linkage_status) for r in rows],
            [(20, "outside_reviewed_window"), (20, "confirmed"), (20, "no_reviewed_mapping"),
             (20, "confirmed"), (20, "outside_reviewed_window")])
        totals = identity.union_effort(rows)
        self.assertEqual(cell(totals, "controller", 500).union_usec, 40)
        self.assertEqual(cell(totals, "unknown_controller", None).character_usec, 60)

    def test_effort_replay_and_overlapping_source_intervals_fail_closed(self):
        i = interval()
        rows = identity.attribute_interval(i, [ownership(i)], self.registry)
        self.assertEqual(identity.union_effort(rows + rows), identity.union_effort(rows))
        contradictory = replace(rows[0], controller_token=501)
        with self.assertRaises(identity.IdentityError):
            identity.union_effort(rows + (contradictory,))
        overlapping = replace(i, replay=(300, 400, 101), start_monotonic_usec=50, end_monotonic_usec=150,
                              start_utc_usec=40, end_utc_usec=140)
        with self.assertRaises(identity.IdentityError):
            identity.union_effort(rows + identity.attribute_interval(overlapping, [ownership(overlapping)], self.registry))

    def test_registry_generations_cannot_be_mixed_for_effort(self):
        i = interval()
        rows = identity.attribute_interval(i, [ownership(i)], self.registry)
        with self.assertRaises(identity.IdentityError):
            identity.union_effort(rows + (replace(rows[0], registry_version=2),))

    def test_bounds_overflow_and_invalid_slice_types(self):
        i = interval()
        with self.assertRaises(identity.IdentityError):
            identity.attribute_interval(i, [ownership(i)] * (identity.MAX_OWNERSHIP_OBSERVATIONS + 1), self.registry)
        rows = identity.attribute_interval(i, [ownership(i)], self.registry)
        with self.assertRaises(identity.IdentityError):
            identity.union_effort(rows * (identity.MAX_EFFORT_SLICES + 1))
        for updates in ({"start_monotonic_usec": True}, {"start_utc_usec": 1},
                        {"association_id": None}, {"linkage_status": "unknown_account"}):
            with self.subTest(updates=updates), self.assertRaises(identity.IdentityError):
                replace(rows[0], **updates)
        with self.assertRaises(identity.IdentityError):
            identity.Registry.from_packet(packet([association(index + 1, index + 100) for index in range(identity.MAX_ASSOCIATIONS + 1)]))
        enormous = identity.ObservedInterval((8, 7), (1, 2, 3), (4, 5, 6), 3, 3, 9, "active",
            0, identity.UINT64_MAX, None, None)
        enormous_rows = identity.attribute_interval(enormous, [ownership(enormous)], self.registry)
        second = replace(enormous, session=(1, 2, 4), replay=(4, 5, 7), subject_id=4, pid=4)
        with self.assertRaises(identity.IdentityError):
            identity.union_effort(enormous_rows + identity.attribute_interval(second, [ownership(second)], self.registry))


if __name__ == "__main__":
    unittest.main(verbosity=2)
