#!/usr/bin/env python3
"""Typed mixed-stream projections, followed by optional disposable SQL proof."""
from __future__ import annotations

from copy import deepcopy
from datetime import date
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))
from rollup_definitions import RollupTarget, UTC_UNKNOWN, UNKNOWN_DAY, report_catalog, report_definition
from rollup_engine import build_page_contributions, SemanticError, BoundsExceeded, RollupEngine, RollupBounds
import observation_semantics as obs
import telemetry_rollup_review_semantics as _review


def progression(**updates):
    row = _review.ReviewSemanticsTest.interval(record_kind=6, at_monotonic_usec=100, at_utc_usec=100,
        connection_boot_id=0, connection_process_id=0, connection_seq=0,
        progression_kind=1, progression_source=3, progression_reason=1,
        progression_observation_status=1, progression_modifier_flags=0,
        progression_requested_xp=50, progression_computed_xp=50, progression_applied_xp=50,
        progression_before_exp=100, progression_after_exp=150,
        progression_before_level=20, progression_after_level=20, progression_threshold_xp=0)
    row.update(updates)
    return row


def encounter(event=1, revision=1, **updates):
    row = {"ingest_id": revision, "boot_id": 100, "process_id": 200, "record_seq": revision,
           "schema_version": 1, "record_kind": 7, "occurrence_utc_usec": 100 + revision,
           "at_monotonic_usec": 100 + revision, "at_utc_usec": 100 + revision,
           "encounter_boot_id": 100, "encounter_process_id": 200, "encounter_seq": 50,
           "encounter_event": event, "encounter_revision": revision, "encounter_mode": 1,
           "encounter_outcome": 0 if event in (1, 2) else 1,
           "encounter_environment_id": 8, "encounter_season_id": 7, "encounter_config_id": 99,
           "encounter_classifier_version": 1, "encounter_policy_version": 1,
           "encounter_zone_vnum": 123, "encounter_group_key": 0,
           "encounter_participant_subject_id": 9001 if event in (2, 3, 5) else 0,
           "encounter_participant_pid": 42 if event in (2, 3, 5) else 0,
           "encounter_start_monotonic_usec": 100, "encounter_start_utc_usec": 100,
           "elapsed_usec": revision if event == 4 else 0,
           "participant_usec": revision if event in (3, 5) else 0,
           "participant_count": 1, "expected_credit_count": 2, "encounter_quality_flags": 0}
    row.update(updates)
    return row


def combat(**updates):
    row = {"ingest_id": 10, "boot_id": 100, "process_id": 200, "record_seq": 10,
           "schema_version": 1, "record_kind": 8, "occurrence_utc_usec": 110}
    values = dict(encounter_boot_id=100, encounter_process_id=200, encounter_seq=50,
                  mode=1, outcome=1, revision=10, environment_id=8, season_id=7, config_id=99,
                  classifier_version=1, policy_version=1, zone_vnum=123, group_key=0,
                  actor_kind=1, actor_id=9001, actor_pid=42, owner_subject_id=9001,
                  start_monotonic_usec=100, end_monotonic_usec=110, start_utc_usec=100, end_utc_usec=110,
                  unique_player_count=1, participant_count=2, dropped_participant_count=0,
                  power_band=20, opponent_power_band=22, opponent_count=1, modifier_flags=1, quality_flags=0)
    values.update({name: 0 for name in obs.COMBAT_METRICS})
    values.update(damage_dealt=50, damage_taken=40, healing_attempted=20, effective_healing=12, overhealing=8,
                  casting_attempts=3, casting_completions=2, casting_aborts=1)
    row.update({"combat_" + name: value for name, value in values.items()})
    row.update(updates)
    return row


TARGET = RollupTarget(2, 1, 8, 7)


def page(rows):
    return build_page_contributions(rows, TARGET).observations


class ObservationSemantics(unittest.TestCase):
    def test_mixed_stream_preserves_legacy_totals_and_validates_all_known_families(self):
        rows = [progression(), encounter(revision=2), combat()]
        source = deepcopy(rows)
        old = build_page_contributions(rows, RollupTarget(1, 1, 8, 7))
        self.assertEqual(old.page_last_ingest_id, 10)
        self.assertEqual(old.output_fanout, 0)
        self.assertEqual(rows, source)
        current = build_page_contributions(rows, TARGET)
        self.assertEqual(current.output_fanout, 3)
        self.assertEqual(current.sessions, {})
        for row in (progression(progression_applied_xp=51), encounter(encounter_revision=0), combat(combat_effective_healing=21)):
            with self.assertRaises(SemanticError):
                build_page_contributions([row], RollupTarget(1, 1, 8, 7))

    def test_xp_sources_are_separate_and_thresholds_are_not_rewards(self):
        rows = [progression(), progression(ingest_id=2, record_seq=2, progression_source=4,
                    progression_reason=2, progression_requested_xp=-10, progression_computed_xp=-10,
                    progression_applied_xp=-10, progression_after_exp=90),
                progression(ingest_id=3, record_seq=3, progression_kind=2, progression_reason=4,
                    progression_requested_xp=0, progression_computed_xp=0, progression_applied_xp=0,
                    progression_before_exp=0, progression_after_exp=0, progression_after_level=21,
                    progression_threshold_xp=1_000_000, progression_modifier_flags=128)]
        result = page(rows)
        earned, lost = sorted(result.progression.values(), key=lambda r: r["reason"])
        self.assertEqual(earned["earned_positive_xp"], 50)
        self.assertEqual(lost["death_loss_xp"], 10)
        self.assertEqual(sum(r["applied_xp"] for r in result.progression.values()), 40)
        self.assertEqual(next(iter(result.levels.values()))["threshold_xp"], 1_000_000)
        self.assertEqual(next(iter(result.levels.values()))["modifier_flags"], 128)
        self.assertEqual(next(iter(result.levels.values()))["observation_status"], 1)

    def test_zero_and_administrative_changes_keep_their_own_cells(self):
        result = page([progression(progression_applied_xp=0, progression_after_exp=100),
                       progression(ingest_id=2, record_seq=2, progression_source=11, progression_reason=5)])
        cells = sorted(result.progression.values(), key=lambda r: r["source"])
        self.assertEqual(cells[0]["zero_applied_observations"], 1)
        self.assertEqual(cells[0]["computed_xp"], 50)
        self.assertEqual(cells[1]["applied_xp"], 50)
        self.assertEqual(cells[1]["earned_positive_xp"], 0)

    def test_joins_and_unclosed_encounters_have_unknown_measured_effort(self):
        result = page([encounter(), encounter(event=2, revision=2)])
        episode = obs.merge_episode(None, next(iter(result.episodes.values())))
        participant = obs.merge_participant(None, next(iter(result.participants.values())))
        self.assertEqual(episode["start_seen"], 1)
        self.assertEqual(episode["close_seen"], 0)
        self.assertIsNone(episode["elapsed_usec"])
        self.assertIsNone(participant["participant_usec"])
        self.assertEqual(participant["effort_revision"], 0)

    def test_leave_rejoin_summary_retains_absolute_effort_and_mode_history(self):
        result = page([encounter(), encounter(event=2, revision=2),
                       encounter(event=3, revision=3, participant_usec=20, encounter_outcome=4),
                       encounter(event=2, revision=4, encounter_mode=3),
                       encounter(event=4, revision=5, encounter_mode=3),
                       encounter(event=5, revision=6, participant_usec=40, encounter_mode=3)])
        episode = obs.merge_episode(None, next(iter(result.episodes.values())))
        participant = obs.merge_participant(None, next(iter(result.participants.values())))
        self.assertEqual(episode["mode_mask"], (1 << 1) | (1 << 3))
        self.assertEqual(episode["elapsed_usec"], 5)
        self.assertEqual(episode["source_events"], 6)
        self.assertEqual(participant["participant_usec"], 40)
        self.assertEqual(participant["effort_revision"], 6)
        self.assertEqual((participant["join_seen"], participant["leave_seen"], participant["summary_seen"]), (1, 1, 1))

    def test_close_without_start_does_not_invent_start_observation(self):
        result = page([encounter(event=4, revision=4)])
        episode = obs.merge_episode(None, next(iter(result.episodes.values())))
        self.assertEqual((episode["start_seen"], episode["close_seen"]), (0, 1))
        self.assertEqual(episode["elapsed_usec"], 4)

    def test_late_participant_event_does_not_regress_latest_effort(self):
        newer = page([encounter(event=5, revision=5, participant_usec=40)])
        existing = obs.merge_participant(None, next(iter(newer.participants.values())))
        older = page([encounter(event=3, revision=3, ingest_id=6, record_seq=6, participant_usec=20)])
        result = obs.merge_participant(existing, next(iter(older.participants.values())))
        self.assertEqual(result["participant_usec"], 40)
        self.assertEqual(result["latest_revision"], 5)
        self.assertEqual(result["leave_seen"], 1)
        bad = deepcopy(next(iter(older.participants.values())))
        bad[0]["participant_usec"] = 50
        with self.assertRaises(obs.ObservationError):
            obs.merge_participant(existing, bad)

    def test_cumulative_actor_snapshots_are_not_summed_and_ownership_is_immutable(self):
        old = obs.merge_actor(None, next(iter(page([combat()]).actors.values())))
        newer = page([combat(ingest_id=11, record_seq=11, combat_revision=11, combat_damage_dealt=60)])
        result = obs.merge_actor(old, next(iter(newer.actors.values())))
        self.assertEqual(result["damage_dealt"], 60)
        older = page([combat(ingest_id=12, record_seq=12)])
        self.assertEqual(obs.merge_actor(result, next(iter(older.actors.values())))["damage_dealt"], 60)
        pet = page([combat(combat_actor_kind=2, combat_actor_pid=-1, combat_actor_id=88)])
        petrow = obs.merge_actor(None, next(iter(pet.actors.values())))
        self.assertEqual(petrow["owner_subject_id"], 9001)
        changed = page([combat(combat_actor_kind=2, combat_actor_pid=-1, combat_actor_id=88, combat_owner_subject_id=9002)])
        with self.assertRaises(obs.ObservationError):
            obs.merge_actor(petrow, next(iter(changed.actors.values())))
        npc = page([combat(combat_actor_kind=3, combat_actor_pid=-1, combat_owner_subject_id=0)])
        self.assertEqual(obs.merge_actor(None, next(iter(npc.actors.values())))["owner_subject_id"], 0)

    def test_unknown_utc_and_cardinality_loss_survive(self):
        result = build_page_contributions([progression(at_utc_usec=UTC_UNKNOWN, occurrence_utc_usec=UTC_UNKNOWN,
                                                      quality_flags=1 << 9)], TARGET)
        xp = next(iter(result.observations.progression.values()))
        self.assertEqual(xp["utc_day"], UNKNOWN_DAY)
        self.assertTrue(xp["quality_flags"] & (1 << 9))
        self.assertTrue(xp["quality_flags"] & (1 << 16))
        self.assertIsNone(result.coverage_start_utc_usec)

    def test_scope_prefix_and_union_selection_do_not_use_inactive_fields(self):
        row = encounter(environment_id=999, season_id=999, quality_flags=1 << 31, category=99)
        self.assertEqual(len(page([row]).episodes), 1)
        row["encounter_environment_id"] = 9
        result = build_page_contributions([row], TARGET)
        self.assertEqual(result.output_fanout, 0)
        self.assertEqual(result.state_quality_flags, 0)

    def test_bounds_overflow_conflicts_and_future_families_fail_before_cursor(self):
        with self.assertRaises(BoundsExceeded):
            build_page_contributions([encounter(event=2, revision=2)], TARGET, max_output_fanout=1)
        with self.assertRaises(SemanticError):
            build_page_contributions([combat(record_kind=9)], TARGET)
        for row in (progression(connection_boot_id=True), progression(schema_version=True), combat(combat_actor_pid=0, combat_actor_kind=2),
                    progression(quality_flags=1 << 10)):
            with self.assertRaises(SemanticError):
                build_page_contributions([row], TARGET)
        result = page([progression(progression_applied_xp=(1 << 63)-1, progression_after_exp=(1 << 63)-1,
                                  progression_before_exp=0)])
        delta = next(iter(result.progression.values()))
        with self.assertRaises(obs.ObservationError):
            obs.merge_progression(delta, delta)
        actor = obs.merge_actor(None, next(iter(page([combat()]).actors.values())))
        conflicting = page([combat(combat_damage_dealt=51)])
        with self.assertRaises(obs.ObservationError):
            obs.merge_actor(actor, next(iter(conflicting.actors.values())))

    def test_versioned_catalog_keeps_existing_definitions_and_explains_limits(self):
        self.assertEqual({r["name"] for r in report_catalog()}, {"session_playtime", "cohort_activity"})
        self.assertEqual(len(report_catalog(2)), 7)
        for item in report_catalog(2):
            self.assertEqual(item["definition_version"], 2)
            self.assertFalse(item["account_metrics_available"])
        with self.assertRaises(ValueError):
            report_definition("combat_contributions", 1)
        self.assertIn("level proxy", report_definition("combat_contributions", 2).distinct_semantics)


def sql_qualification():
    """Load the authoritative chain, then exercise actual commits with minimal roles."""
    import pymysql
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    from db_access import (ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase,
                           AmbiguousCommit, GenerationConflict, RAW_COLUMNS, REPORT_TABLES)
    environment, command, name = prepare_sql_fixture()
    admin = None
    users, databases = [], []
    tables = [REPORT_TABLES[r][0] for r in ("progression_observations", "level_observations", "encounter_observations",
                                          "encounter_participants", "combat_contributions")]
    try:
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]), user=environment["DB_USER"],
                                password=environment["DB_PASSWD"], database=name, autocommit=True,
                                cursorclass=pymysql.cursors.DictCursor)
        token = hashlib.sha256(name.encode()).hexdigest()[:10]
        password = "synthetic-observation-fixture-" + token
        grants = {
            "rollup": {t: "SELECT,INSERT,UPDATE" for t in (*tables, "telemetry_rollup_state", "telemetry_rollup_session",
                        "telemetry_cohort_day", "telemetry_cohort_member", "telemetry_player_day")},
            "report": {t: "SELECT" for t in (*tables, "telemetry_rollup_state", "telemetry_rollup_session",
                       "telemetry_cohort_day", "telemetry_rollup_incident_coverage", "telemetry_rollup_incident")},
        }
        grants["rollup"].update({t: "SELECT" for t in ("telemetry_interval", "telemetry_incident_registry", "telemetry_incident")})
        grants["rollup"].update({t: "SELECT,INSERT" for t in ("telemetry_rollup_incident_coverage", "telemetry_rollup_incident")})
        with admin.cursor() as cursor:
            for role, permissions in grants.items():
                user = "to_" + role + "_" + token
                cursor.execute("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password)); users.append(user)
                for table, privilege in permissions.items():
                    cursor.execute(f"GRANT {privilege} ON `{name}`.`{table}` TO %s@'%%'", (user,))
        def database(role):
            settings = ConnectionSettings(host="127.0.0.1", port=int(environment["DB_PORT"]), database=name,
                                          user="to_" + role + "_" + token, password=password)
            db = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings)); databases.append(db)
            return db
        rollup, report = database("rollup"), database("report")
        rows = [progression(), progression(progression_applied_xp=10, progression_after_exp=110),
                _review.ReviewSemanticsTest.interval(),
                progression(progression_source=4, progression_reason=2, progression_requested_xp=-10,
                            progression_computed_xp=-10, progression_applied_xp=-10, progression_after_exp=90),
                progression(progression_kind=2, progression_reason=4, progression_requested_xp=0,
                            progression_computed_xp=0, progression_applied_xp=0, progression_before_exp=0,
                            progression_after_exp=0, progression_after_level=21, progression_threshold_xp=1_000_000,
                            progression_modifier_flags=128),
                encounter(), encounter(event=2, revision=2), encounter(event=3, revision=3, participant_usec=20),
                encounter(event=2, revision=4, encounter_mode=3), encounter(event=4, revision=5, encounter_mode=3),
                encounter(event=5, revision=6, participant_usec=40, encounter_mode=3),
                encounter(encounter_seq=51), encounter(event=2, revision=2, encounter_seq=51),
                combat(), combat(combat_revision=11, combat_damage_dealt=60),
                combat(combat_actor_kind=2, combat_actor_pid=-1, combat_actor_id=88),
                combat(combat_actor_kind=3, combat_actor_pid=-1, combat_actor_id=89, combat_owner_subject_id=0),
                progression(at_utc_usec=UTC_UNKNOWN, occurrence_utc_usec=UTC_UNKNOWN, quality_flags=1 << 9,
                            progression_source=6, progression_reason=3)]
        def insert(row, ingest):
            row = dict(row, ingest_id=ingest, record_seq=ingest, ingested_utc_usec=1_000)
            assert set(row) <= set(RAW_COLUMNS)
            with admin.cursor() as cursor:
                cursor.execute("INSERT INTO telemetry_interval (" + ",".join(row) + ") VALUES (" +
                               ",".join(["%s"] * len(row)) + ")", tuple(row.values()))
        for number, row in enumerate(rows, 1): insert(row, number)
        bounds = RollupBounds(page_size=3, max_rows=100, max_runtime_s=60)
        target = RollupTarget(2, 1, 8, 7)
        result = RollupEngine(rollup).run(target, bounds=bounds)
        assert result.complete and result.final_cursor == len(rows) and result.pages == 6
        try: report.read_report(target, "progression_observations")
        except GenerationConflict: pass
        else: raise AssertionError("building observations visible as published")
        RollupEngine(rollup).publish(target)
        reports = {r: report.read_report(target, r) for r in ("progression_observations", "level_observations",
                                                           "encounter_observations", "encounter_participants", "combat_contributions")}
        xp = reports["progression_observations"].rows
        earned = next(r for r in xp if r["reason"] == 1)
        assert earned["applied_xp"] == 60 and earned["observations"] == 2
        assert next(r for r in xp if r["reason"] == 2)["death_loss_xp"] == 10
        unknown = next(r for r in xp if r["reason"] == 3)
        assert unknown["utc_day"] is None and unknown["bucket_kind"] == "unknown" and unknown["quality_flags"] & (1 << 9)
        assert reports["level_observations"].rows[0]["threshold_xp"] == 1_000_000
        assert reports["level_observations"].rows[0]["modifier_flags"] == 128
        episodes = reports["encounter_observations"].rows
        assert len(episodes) == 2 and episodes[0]["source_events"] == 6 and episodes[0]["mode_mask"] == 10
        assert episodes[1]["elapsed_usec"] is None and episodes[1]["close_seen"] == 0
        participants = reports["encounter_participants"].rows
        assert participants[0]["participant_usec"] == 40 and participants[1]["participant_usec"] is None
        actors = reports["combat_contributions"].rows
        assert len(actors) == 3 and actors[0]["damage_dealt"] == 60
        assert (actors[1]["actor_pid"], actors[1]["owner_subject_id"]) == (-1, 9001)
        assert actors[2]["owner_subject_id"] == 0
        assert all(r.coverage.incident_coverage["status"] == "not_registered" for r in reports.values())
        # Lost acknowledgement after a real commit reuses the cursor on a new
        # connection; no XP or cumulative source count is applied a second time.
        original_commit, lost = rollup._commit, False
        def lose_once():
            nonlocal lost
            original_commit()
            if not lost:
                lost = True; raise AmbiguousCommit("synthetic lost observation commit reply")
        rollup._commit = lose_once
        retry_target = RollupTarget(2, 2, 8, 7)
        assert RollupEngine(rollup).run(retry_target, bounds=bounds).complete
        rollup._commit = original_commit
        RollupEngine(rollup).publish(retry_target)
        def comparable(snapshot):
            return [dict((k, v) for k, v in r.items() if k != "generation") for r in snapshot.rows]
        for report_name, prior in reports.items():
            assert comparable(prior) == comparable(report.read_report(retry_target, report_name))
        assert RollupEngine(rollup).run(retry_target, bounds=bounds).fetched_rows == 0
        legacy = RollupTarget(1, 1, 8, 7)
        assert RollupEngine(rollup).run(legacy, bounds=bounds).complete
        RollupEngine(rollup).publish(legacy)
        assert report.read_report(legacy, "session_playtime").rows[0]["covered_active_usec"] == 100
        assert report.read_report(retry_target, "session_playtime").rows[0]["covered_active_usec"] == 100
        try: report.read_report(legacy, "combat_contributions")
        except ValueError: pass
        else: raise AssertionError("definition 1 accepted a version 2 report")
        # Output/read byte and statement budgets roll back before cursor advance.
        for generation, budget in ((3, RollupBounds(page_size=3, max_rows=100, max_page_bytes=32, max_total_bytes=32)),
                                   (4, RollupBounds(page_size=3, max_rows=100, max_output_fanout=1)),
                                   (5, RollupBounds(page_size=3, max_rows=100, max_transaction_statements=8))):
            try: RollupEngine(rollup).run(RollupTarget(2, generation, 8, 7), bounds=budget)
            except BoundsExceeded: pass
            else: raise AssertionError("projection exceeded explicit budget")
            with admin.cursor() as cursor:
                cursor.execute("SELECT COUNT(*) AS n FROM telemetry_rollup_state WHERE definition_version=2 AND generation=%s", (generation,))
                assert cursor.fetchone()["n"] == 0
        try: report.read_report(retry_target, "combat_contributions", max_bytes=4096)
        except BoundsExceeded: pass
        else: raise AssertionError("actor read exceeded fixed row byte reservation")
        assert report.read_report(retry_target, "combat_contributions", max_rows=1).truncated
        # An earlier XP write in a conflicting page rolls back with all five
        # families and the cursor, even when the conflict is in the actor family.
        conflict_target = RollupTarget(2, 6, 8, 7)
        assert RollupEngine(rollup).run(conflict_target, bounds=bounds).complete
        insert(progression(), len(rows) + 1)
        try: insert(combat(combat_revision=11, combat_damage_dealt=51), len(rows) + 2)
        except pymysql.err.IntegrityError as error: assert error.args[0] == 1062
        else: raise AssertionError("raw same actor revision conflict was not rejected")
        insert(combat(combat_revision=12, combat_damage_dealt=51), len(rows) + 2)
        try: RollupEngine(rollup).run(conflict_target, bounds=bounds)
        except SemanticError: pass
        else: raise AssertionError("regressing cumulative actor revision was accepted")
        assert rollup.read_state(conflict_target)["input_watermark"] == len(rows)
        with admin.cursor() as cursor:
            cursor.execute("SELECT applied_xp FROM telemetry_rollup_progression_day WHERE definition_version=2 AND generation=6 AND reason=1")
            assert cursor.fetchone()["applied_xp"] == 60
            # SQL constraints protect even direct fixture writes, and nullable
            # effort/closure shapes cannot be passed through SQL UNKNOWN.
            for table, assignment, predicate in ((tables[0], "source=11", "reason=1"),
                (tables[1], "after_level=before_level", "kind=2"), (tables[2], "close_seen=0", "close_seen=1"),
                (tables[3], "participant_usec=NULL", "effort_revision>0"), (tables[4], "actor_pid=0", "actor_kind=1")):
                try: cursor.execute("UPDATE " + table + " SET " + assignment + " WHERE definition_version=2 AND generation=1 AND " + predicate + " LIMIT 1")
                except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                    assert error.args[0] in (3819, 4025)
                else: raise AssertionError("invalid direct observation accepted: " + table)
        for role, statements in {
            "report": ("SELECT ingest_id FROM telemetry_interval LIMIT 0", "SELECT pid FROM player_data LIMIT 0",
                       "UPDATE telemetry_rollup_combat_actor SET damage_dealt=0 WHERE 0"),
            "rollup": ("UPDATE telemetry_interval SET record_kind=1 WHERE 0", "DELETE FROM telemetry_rollup_progression_day WHERE 0",
                       "UPDATE player_data SET pid=pid WHERE 0"),
        }.items():
            db = report if role == "report" else rollup
            for statement in statements:
                try:
                    with db._ensure_connection().cursor() as cursor:
                        cursor.execute(statement)
                except pymysql.err.OperationalError as error: assert error.args[0] in (1142, 1143)
                else: raise AssertionError("restricted role unexpectedly permitted operation")
        verified = subprocess.run(["bash", "migrations/immutable/0055_telemetry_observation_projections.sh"],
                                  env=environment, capture_output=True, text=True, timeout=30)
        assert verified.returncode == 0, verified.stderr
        with admin.cursor() as cursor:
            cursor.execute("ALTER TABLE telemetry_rollup_level_event MODIFY threshold_xp BIGINT NULL")
        refused = subprocess.run(["bash", "migrations/immutable/0055_telemetry_observation_projections.sh"],
                                 env=environment, capture_output=True, text=True, timeout=30)
        assert refused.returncode != 0 and "columns differ" in refused.stderr
        with admin.cursor() as cursor:
            cursor.execute("ALTER TABLE telemetry_rollup_level_event MODIFY threshold_xp BIGINT UNSIGNED NOT NULL")
        measured = subprocess.run(["bash", "migrations/verify_runtime_compatibility.sh", "--schema-only"],
                                  env=environment, capture_output=True, text=True, timeout=45)
        output = measured.stdout + measured.stderr
        match = re.search(r"normalized metadata fingerprint mismatch: expected=[0-9a-f]{64} actual=([0-9a-f]{64})", output)
        runtime = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        key = "mariadb10_11" if "mariadb" in os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"] else "mysql8"
        if match:
            assert measured.returncode != 0 and output.count("FAILED:") == 1, output
            fingerprint = match[1]
        else:
            assert measured.returncode == 0, output
            fingerprint = runtime["normalized_metadata_fingerprints"][key]
        artifact = dict(status="passed", engine=os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"],
                        migration_head=runtime["migration_head"]["id"], normalized_metadata_fingerprint=fingerprint)
        suffix = "mariadb" if key == "mariadb10_11" else "mysql"
        (ROOT / f"bin/telemetry-observations-{suffix}.json").write_text(json.dumps(artifact, indent=2) + "\n")
        print(json.dumps(artifact), flush=True)
    finally:
        for db in databases: db.close()
        if admin is not None:
            with admin.cursor() as cursor:
                for user in users: cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            admin.close()
        drop_sql_fixture(environment, command, name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ObservationSemantics))
    if not result.wasSuccessful(): sys.exit(1)
    if args.sql_fixture: sql_qualification()
