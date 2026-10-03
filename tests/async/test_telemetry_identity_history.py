#!/usr/bin/env python3
"""Reviewed association history, restricted SQL registration and observed effort."""
from __future__ import annotations

from copy import deepcopy
import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import replace
from pathlib import Path
import json
import hashlib
import os
import re
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

    def test_retained_storage_roundtrip_refuses_missing_or_changed_evidence(self):
        meta, rows = identity.storage_rows(self.registry, "approved-reviewer@%", 2_000)
        self.assertEqual(identity.registry_from_storage(meta, rows), self.registry)
        for changed_meta, changed_rows in ((dict(meta, association_count=2), rows),
            (meta, [dict(rows[0], controller_token=501)]),
            (dict(meta, database_principal=""), rows),
            (dict(meta, reviewer_token=10_000_000), rows),
            (dict(meta, registered_at_utc_usec=999), rows),
            (meta, [dict(rows[0], status=True)]),
            (meta, [dict(rows[0], evidence_digest=b"\x00" * 32)])):
            with self.subTest(meta=changed_meta["association_count"]), self.assertRaises(identity.IdentityError):
                identity.registry_from_storage(changed_meta, changed_rows)

    def test_generation_reservations_do_not_imply_published_or_complete_reports(self):
        scope = (3, 7, 8, 7)
        result = identity.public_generation(identity.generation_row(scope, self.registry))
        self.assertEqual(result["registry_digest"], self.registry.packet_digest)
        self.assertFalse(result["balance_report_published"])
        self.assertFalse(result["complete_identity_coverage_implied"])
        self.assertNotIn("reviewer_token", result)
        self.assertNotIn("associations", result)
        unknown = identity.generation_row(scope, None)
        self.assertEqual(identity.public_generation(unknown)["status"], "reserved_unknown_identity")
        for invalid in ((1, 7, 8, 7), (2, 7, 8, 7), (3, 0, 8, 7), (True, 7, 8, 7),
                        (1 << 32, 7, 8, 7), (3, 7, 8, 7, 1)):
            with self.subTest(scope=invalid), self.assertRaises(identity.IdentityError):
                identity.generation_row(invalid, None)
        for values in (dict(unknown, association_count=1), dict(unknown, registry_digest=b"a" * 32),
                       dict(unknown, reviewed_from_utc_usec=0)):
            with self.assertRaises(identity.IdentityError):
                identity.public_generation(values)

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


def sql_qualification():
    import pymysql
    from db_access import (AmbiguousCommit, BoundsExceeded, ConnectionSettings, DatabaseAccessError,
                           PyMySQLConnectionFactory, PyMySQLRollupDatabase)
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    environment, command, name = prepare_sql_fixture()
    admin = None
    databases, users = [], []
    try:
        engine = environment["TELEMETRY_REPOSITORY_DB_IMAGE"]
        runtime = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        key = "mariadb10_11" if "mariadb" in engine else "mysql8"
        def measured_fingerprint():
            measured = subprocess.run(["bash", "migrations/verify_runtime_compatibility.sh", "--schema-only"],
                                      env=environment, capture_output=True, text=True, timeout=45)
            output = measured.stdout + measured.stderr
            match = re.search(r"normalized metadata fingerprint mismatch: expected=[0-9a-f]{64} actual=([0-9a-f]{64})", output)
            if match:
                assert measured.returncode != 0 and output.count("FAILED:") == 1, output
                return match[1]
            assert measured.returncode == 0, output
            return runtime["normalized_metadata_fingerprints"][key]
        fresh_fingerprint = measured_fingerprint()
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"], database=name,
            autocommit=True, cursorclass=pymysql.cursors.DictCursor, connect_timeout=3,
            read_timeout=10, write_timeout=10)
        def query(statement, values=()):
            with admin.cursor() as cursor:
                cursor.execute(statement, values)
                return list(cursor.fetchall()) if cursor.description else []
        token = hashlib.sha256(name.encode()).hexdigest()[:10]
        password = "synthetic-local-identity-review-" + token
        grants = {
            "review": {"telemetry_identity_reviewer": "SELECT", "telemetry_identity_registry": "SELECT,INSERT",
                       "telemetry_identity_association": "SELECT,INSERT"},
            "rollup": {"telemetry_identity_registry": "SELECT", "telemetry_identity_association": "SELECT",
                        "telemetry_generation_identity": "SELECT,INSERT"},
            "report": {"telemetry_generation_identity": "SELECT"},
            "writer": {"telemetry_interval": "SELECT,INSERT"},
        }
        for role, tables in grants.items():
            user = "tir_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password)); users.append(user)
            for table, permissions in tables.items():
                query(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'", (user,))
        review_user = "tir_review_" + token
        query(f"GRANT SELECT (environment_id,season_id,account_token) ON `{name}`.telemetry_account_token TO %s@'%%'", (review_user,))
        def settings(role):
            return ConnectionSettings(host="127.0.0.1", port=int(environment["DB_PORT"]), database=name,
                                      user="tir_" + role + "_" + token, password=password)
        def adapter(role):
            database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings(role)))
            databases.append(database)
            return database
        review, rollup, report = adapter("review"), adapter("rollup"), adapter("report")
        def expect_identity(action, reason):
            try: action()
            except identity.IdentityError as error: assert str(error) == reason, str(error)
            else: raise AssertionError("identity refusal missing: " + reason)
        def counts():
            return tuple(query("SELECT (SELECT COUNT(*) FROM telemetry_identity_registry) AS registries,"
                               "(SELECT COUNT(*) FROM telemetry_identity_association) AS associations")[0].values())
        p1 = packet(reviewed_from_utc_usec=-1)
        expect_identity(lambda: review.register_identity_packet(p1), "reviewer_not_authorized")
        principal = review._execute("SELECT CURRENT_USER() AS principal")[0][0]["principal"]
        # Binary principal comparison uses the authenticated SQL account, never
        # a caller-supplied display name or packet token as authentication.
        query("INSERT INTO telemetry_identity_reviewer VALUES (8,7,%s,%s,1)", (principal.upper(), bytes.fromhex(p1["reviewer_token"])))
        expect_identity(lambda: review.register_identity_packet(p1), "reviewer_not_authorized")
        query("DELETE FROM telemetry_identity_reviewer")
        query("INSERT INTO telemetry_identity_reviewer VALUES (8,7,%s,%s,0)", (principal, bytes.fromhex(p1["reviewer_token"])))
        expect_identity(lambda: review.register_identity_packet(p1), "reviewer_not_authorized")
        query("UPDATE telemetry_identity_reviewer SET enabled=1")
        expect_identity(lambda: review.register_identity_packet(dict(p1, reviewer_token="c" * 64)), "reviewer_not_authorized")
        expect_identity(lambda: review.register_identity_packet(dict(p1, season_id=8)), "reviewer_not_authorized")
        expect_identity(lambda: review.register_identity_packet(p1), "account_token_not_issued_in_scope")
        assert counts() == (0, 0)
        query("INSERT INTO telemetry_account_lifetime VALUES (1001,NULL),(1002,NULL),(1003,NULL)")
        query("INSERT INTO telemetry_account_token VALUES (8,7,100,1001),(8,7,101,1002),(8,8,777,1003)")
        foreign = packet([association(account_token=777)])
        expect_identity(lambda: review.register_identity_packet(foreign), "account_token_not_issued_in_scope")
        now = query("SELECT CAST(UNIX_TIMESTAMP(UTC_TIMESTAMP(6))*1000000 AS SIGNED) AS utc")[0]["utc"]
        future = packet(reviewed_at_utc_usec=now + 60_000_000)
        expect_identity(lambda: review.register_identity_packet(future), "unreviewed_future_registration")
        assert review.register_identity_packet(p1)["status"] == "registered"
        assert review.register_identity_packet(p1)["status"] == "already_registered"
        assert counts() == (1, 1)
        expect_identity(lambda: review.register_identity_packet(packet([association(controller_token=501)])), "registry_version_conflict")
        first = rollup.reserve_identity_generation((3, 7, 8, 7), 1)
        assert report.read_generation_identity((3, 7, 8, 7)) == first
        assert not report._connection.server_status & pymysql.constants.SERVER_STATUS.SERVER_STATUS_IN_TRANS
        p2 = packet([association(last=500), association(2,101,501,first=500)], registry_version=2,
                    previous_registry_version=1, previous_packet_digest=identity.Registry.from_packet(p1).packet_digest)
        removed = deepcopy(p2); removed["associations"] = []
        expect_identity(lambda: review.register_identity_packet(removed), "association_removed_without_withdrawal")
        changed = deepcopy(p2); changed["associations"][0]["account_token"] = 101
        expect_identity(lambda: review.register_identity_packet(changed), "association_account_changed")
        expect_identity(lambda: review.register_identity_packet(dict(p2, previous_packet_digest="f" * 64)), "registry_history_conflict")
        assert review.register_identity_packet(p2)["status"] == "registered"
        assert review.register_identity_packet(p1)["status"] == "already_registered"
        expect_identity(lambda: rollup.reserve_identity_generation((3, 7, 8, 7), 2), "generation_identity_conflict")
        assert rollup.reserve_identity_generation((3, 7, 8, 7), 1) == first
        assert report.read_generation_identity((3, 7, 8, 7)) == first
        second = rollup.reserve_identity_generation((3, 8, 8, 7), 2)
        assert second["registry_version"] == 2 and second["registry_digest"] != first["registry_digest"]
        with tempfile.TemporaryDirectory(dir=ROOT / "bin") as directory:
            path = Path(directory) / "private-synthetic-identity.json"
            path.write_text(json.dumps(p2), encoding="utf-8")
            def cli(role, *arguments):
                prefix = "TELEMETRY_IDENTITY_REVIEW_DB_" if role == "review" else "TELEMETRY_IDENTITY_ROLLUP_DB_"
                values = dict(HOST="127.0.0.1", PORT=environment["DB_PORT"], DATABASE=name,
                              USER="tir_" + role + "_" + token, PASSWORD=password)
                process = subprocess.run([sys.executable, str(ROOT / "scripts/telemetry/identity_history.py"), str(path), *arguments],
                    env={**environment, **{prefix+key:value for key,value in values.items()}},
                    capture_output=True, text=True, timeout=15)
                assert process.returncode == 0, process.stderr + process.stdout
                assert "private-synthetic-identity" not in process.stdout + process.stderr
                assert principal not in process.stdout + process.stderr
                assert p2["reviewer_token"] not in process.stdout + process.stderr
                return json.loads(process.stdout)
            assert cli("review", "--register")["status"] == "already_registered"
            cli_reservation = cli("rollup", "--reserve-generation", "14", "--definition-version", "3")
            assert cli_reservation == report.read_generation_identity((3,14,8,7))
        unknown = rollup.reserve_identity_generation((3, 9, 8, 7), None)
        assert unknown["status"] == "reserved_unknown_identity"
        expect_identity(lambda: rollup.reserve_identity_generation((3, 9, 8, 7), 2), "generation_identity_conflict")
        expect_identity(lambda: rollup.reserve_identity_generation((3, 10, 8, 7), 3), "registry_not_registered")
        expect_identity(lambda: rollup.reserve_identity_generation((3, 10, 8, 7), 2, expected_digest="f" * 64), "registry_version_conflict")
        p3 = deepcopy(p2)
        p3.update(registry_version=3, previous_registry_version=2, previous_packet_digest=identity.Registry.from_packet(p2).packet_digest)
        p3["associations"][0]["status"] = "withdrawn"
        original_commit = review._commit
        def lost_review_reply():
            original_commit()
            raise AmbiguousCommit("synthetic lost identity review reply")
        review._commit = lost_review_reply
        try: review.register_identity_packet(p3)
        except AmbiguousCommit: assert not review.connected
        else: raise AssertionError("review commit ambiguity missing")
        review._commit = original_commit
        assert review.register_identity_packet(p3)["status"] == "already_registered"
        original_commit = rollup._commit
        def lost_reservation_reply():
            original_commit()
            raise AmbiguousCommit("synthetic lost identity reservation reply")
        rollup._commit = lost_reservation_reply
        try: rollup.reserve_identity_generation((3, 11, 8, 7), 3)
        except AmbiguousCommit: assert not rollup.connected
        else: raise AssertionError("reservation commit ambiguity missing")
        rollup._commit = original_commit
        third = rollup.reserve_identity_generation((3, 11, 8, 7), 3)
        assert report.read_generation_identity((3, 11, 8, 7)) == third
        # Retained inventories are revalidated; later staff mistakes cannot
        # silently change a digest or poison an earlier generation reservation.
        query("UPDATE telemetry_identity_association SET controller_token=502 WHERE registry_version=2 AND association_id=2")
        expect_identity(lambda: review.register_identity_packet(p2), "stored_registry_digest_mismatch")
        expect_identity(lambda: rollup.reserve_identity_generation((3, 12, 8, 7), 2), "stored_registry_digest_mismatch")
        assert not query("SELECT * FROM telemetry_generation_identity WHERE generation=12")
        assert report.read_generation_identity((3, 7, 8, 7)) == first
        query("UPDATE telemetry_identity_association SET controller_token=501 WHERE registry_version=2 AND association_id=2")
        full = packet([association(index+1, 101 if index==1 else 100, first=index, last=index+1)
                       for index in range(identity.MAX_ASSOCIATIONS)], registry_version=4, previous_registry_version=3,
                      previous_packet_digest=identity.Registry.from_packet(p3).packet_digest,
                      reviewed_through_utc_usec=2_000, reviewed_at_utc_usec=2_000)
        execute = review._execute
        before = counts()
        def fail_association_insert(statement, parameters=()):
            if statement.startswith("INSERT INTO telemetry_identity_association"):
                raise DatabaseAccessError("synthetic association insert refusal")
            return execute(statement, parameters)
        review._execute = fail_association_insert
        try: review.register_identity_packet(full)
        except DatabaseAccessError: pass
        else: raise AssertionError("review insert failure missing")
        finally: review._execute = execute
        assert counts() == before
        assert review.register_identity_packet(full)["association_count"] == identity.MAX_ASSOCIATIONS
        assert counts() == (4, 1+2+2+identity.MAX_ASSOCIATIONS)
        rollup.reserve_identity_generation((3, 13, 8, 7), 4)
        overflow = deepcopy(full); overflow["associations"].append(association(1025, first=1024, last=1025))
        expect_identity(lambda: review.register_identity_packet(overflow), "association_capacity")
        # Separate native connections use the existing shared advisory lock.
        def concurrent_retry(_):
            database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings("review")))
            try: return database.register_identity_packet(full)["status"]
            finally: database.close()
        with ThreadPoolExecutor(max_workers=2) as pool:
            assert list(pool.map(concurrent_retry, range(2))) == ["already_registered"] * 2
        for role, statements in {
            "review": ["SELECT lifetime_id FROM telemetry_account_token LIMIT 0", "SELECT * FROM telemetry_account_lifetime LIMIT 0",
                       "SELECT * FROM accounts LIMIT 0", "UPDATE telemetry_identity_reviewer SET enabled=1 WHERE 0",
                       "UPDATE telemetry_identity_registry SET association_count=0 WHERE 0", "DELETE FROM telemetry_identity_association WHERE 0",
                       "INSERT INTO telemetry_generation_identity SELECT * FROM telemetry_generation_identity WHERE 0"],
            "rollup": ["SELECT * FROM telemetry_identity_reviewer LIMIT 0", "SELECT * FROM telemetry_account_token LIMIT 0",
                       "INSERT INTO telemetry_identity_registry SELECT * FROM telemetry_identity_registry WHERE 0",
                       "UPDATE telemetry_generation_identity SET registry_version=1 WHERE 0"],
            "report": ["SELECT * FROM telemetry_identity_registry LIMIT 0", "SELECT * FROM telemetry_identity_association LIMIT 0",
                       "SELECT * FROM telemetry_interval LIMIT 0", "UPDATE telemetry_generation_identity SET registry_version=1 WHERE 0"],
            "writer": ["SELECT * FROM telemetry_identity_registry LIMIT 0", "SELECT * FROM telemetry_identity_reviewer LIMIT 0",
                       "SELECT * FROM telemetry_account_token LIMIT 0", "SELECT * FROM telemetry_generation_identity LIMIT 0"],
        }.items():
            factory = PyMySQLConnectionFactory(settings(role)); connection = factory.connect()
            try:
                for statement in statements:
                    with connection.cursor() as cursor:
                        try: cursor.execute(statement)
                        except pymysql.MySQLError as error: assert error.args[0] in (1142,1143), error
                        else: raise AssertionError("role permission unexpectedly allowed")
            finally: factory.close(connection)
        try: report.read_generation_identity((3,7,8,7), max_bytes=1)
        except BoundsExceeded: pass
        else: raise AssertionError("identity read byte bound missing")
        query("UPDATE telemetry_identity_reviewer SET enabled=0")
        expect_identity(lambda: review.register_identity_packet(full), "reviewer_not_authorized")
        assert report.read_generation_identity((3,7,8,7)) == first  # Revocation is not historical erasure.
        query("UPDATE telemetry_identity_reviewer SET enabled=1")
        for statement in (
            "UPDATE telemetry_identity_association SET controller_token=0 WHERE registry_version=1",
            "UPDATE telemetry_identity_association SET status=2 WHERE registry_version=1",
            "UPDATE telemetry_identity_association SET account_token=999 WHERE registry_version=1",
            "UPDATE telemetry_identity_association SET valid_through_utc_usec=valid_from_utc_usec WHERE registry_version=1",
            "UPDATE telemetry_identity_association SET valid_from_utc_usec=-9223372036854775808 WHERE registry_version=1",
            "UPDATE telemetry_identity_registry SET association_count=1025 WHERE registry_version=1",
            "UPDATE telemetry_identity_registry SET reviewed_from_utc_usec=-9223372036854775808 WHERE registry_version=1",
            "UPDATE telemetry_identity_registry SET reviewed_at_utc_usec=registered_at_utc_usec+1 WHERE registry_version=1",
            "UPDATE telemetry_identity_registry SET database_principal='' WHERE registry_version=1",
            "UPDATE telemetry_identity_registry SET reviewer_token=UNHEX(REPEAT('00',32)) WHERE registry_version=1",
            "UPDATE telemetry_generation_identity SET definition_version=2 WHERE generation=7",
            "UPDATE telemetry_generation_identity SET registry_digest=UNHEX(REPEAT('ff',32)) WHERE generation=7",
            "UPDATE telemetry_generation_identity SET association_count=1 WHERE generation=9",
            "UPDATE telemetry_generation_identity SET reviewed_from_utc_usec=-9223372036854775808 WHERE generation=7",
        ):
            try: query(statement)
            except pymysql.MySQLError as error: assert error.args[0] in (1451,1452,3819,4025), error
            else: raise AssertionError("identity constraint unexpectedly accepted")
        # Guarded DDL reruns retain both histories and fixed generation rows.
        before = counts()
        subprocess.run(command + [name], input=(ROOT / "migrations/immutable/0058_telemetry_identity_review.sql").read_bytes(),
                       env=environment, check=True, capture_output=True)
        assert counts() == before and report.read_generation_identity((3,7,8,7)) == first
        verifier = ["bash", "migrations/immutable/0058_telemetry_identity_review.sh"]
        verified = subprocess.run(verifier, env=environment, capture_output=True, text=True, timeout=45)
        assert verified.returncode == 0, verified.stderr
        drop = "DROP CONSTRAINT" if "mariadb" in engine else "DROP CHECK"
        query(f"ALTER TABLE telemetry_identity_registry {drop} chk_identity_registry_evidence")
        query("ALTER TABLE telemetry_identity_registry ADD CONSTRAINT chk_identity_registry_evidence CHECK (association_count <= 1024)")
        refused = subprocess.run(verifier, env=environment, capture_output=True, text=True, timeout=45)
        assert refused.returncode != 0 and "checks differ" in refused.stderr, refused.stderr
        query(f"ALTER TABLE telemetry_identity_registry {drop} chk_identity_registry_evidence")
        query("ALTER TABLE telemetry_identity_registry ADD CONSTRAINT chk_identity_registry_evidence CHECK "
              "(association_count <= 1024 AND reviewer_token <> REPEAT(CHAR(0),32) AND review_evidence_digest <> REPEAT(CHAR(0),32) AND packet_digest <> REPEAT(CHAR(0),32))")
        restored = subprocess.run(verifier, env=environment, capture_output=True, text=True, timeout=45)
        assert restored.returncode == 0, restored.stderr
        fingerprint = measured_fingerprint()
        assert fingerprint == fresh_fingerprint, "drift restoration changed the fresh migrated schema fingerprint"
        artifact = dict(status="passed", engine=engine, migration_head=runtime["migration_head"]["id"],
            normalized_metadata_fingerprint=fingerprint,
            migration_apply_checksum=runtime["migration_head"]["apply_checksum"],
            migration_verify_checksum=runtime["migration_head"]["verify_checksum"],
            read_transaction_released=True, cli_qualification=True, fresh_and_restored_fingerprints_match=True,
            qualification="authenticated scoped reviewer, issued tokens, retained corrections, reservations, CLI, released read transaction, rollback, lost commit reply, capacity, simultaneous retries, permissions, guarded rerun, constraints and exact restored verifier/fingerprint")
        suffix = "mariadb" if key == "mariadb10_11" else "mysql"
        (ROOT / f"bin/telemetry-identity-review-{suffix}.json").write_text(json.dumps(artifact, indent=2)+"\n")
        print(json.dumps(artifact), flush=True)
    finally:
        for database in databases: database.close()
        if admin is not None:
            with admin.cursor() as cursor:
                for user in users: cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            admin.close()
        drop_sql_fixture(environment, command, name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(IdentityHistoryTest))
    if not result.wasSuccessful(): raise SystemExit(1)
    if args.sql_fixture: sql_qualification()
