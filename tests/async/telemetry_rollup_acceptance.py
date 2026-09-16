"""Independent SQL acceptance assertions for #268's actual rollup output.

These use frozen fixture expectations and SQL state, never engine helper output
as an expected value. Call after processing one fixture into an empty generation.
This module only reads the supplied test connection.
"""
from datetime import date, timedelta

from telemetry_rollup_fixtures import golden_rows, FIXTURE_DIR

COUNTERS = ("connected_usec", "active_usec", "idle_usec", "unknown_usec",
            "resident_usec", "linkdead_usec")
SCOPE = ("definition_version", "generation", "environment_id", "season_id")
COHORT = ("utc_day", "level_band", "class_id", "race_id", "faction_id",
          "zone_vnum", "config_id", "category")
UNKNOWN_DAY = date(1000, 1, 1)


def select_scope(connection, table, scope):
    allowed = {"telemetry_rollup_session", "telemetry_player_day",
               "telemetry_cohort_day", "telemetry_cohort_member", "telemetry_rollup_state"}
    if table not in allowed:
        raise ValueError("test query table is not allowlisted")
    with connection.cursor() as cursor:
        cursor.execute("SELECT * FROM " + table + " WHERE " +
                       " AND ".join(name + "=%s" for name in SCOPE), tuple(scope))
        return cursor.fetchall()


def assert_golden_storage(test, connection, fixture, raw_rows, scope):
    """Check one completed generation independently of its publication status."""
    metrics = fixture["expected"]["metrics"]
    sessions = select_scope(connection, "telemetry_rollup_session", scope)
    player_days = select_scope(connection, "telemetry_player_day", scope)
    cohorts = select_scope(connection, "telemetry_cohort_day", scope)
    members = select_scope(connection, "telemetry_cohort_member", scope)
    expected_sessions = {(r["session_boot_id"], r["session_process_id"], r["session_seq"])
                         for r in raw_rows if r.get("session_seq")}
    test.assertEqual({(r["session_boot_id"], r["session_process_id"], r["session_seq"])
                      for r in sessions}, expected_sessions)
    conservation = metrics["conservation"]
    for counter in COUNTERS:
        test.assertEqual(sum(int(r[counter]) for r in player_days), conservation[counter],
                         f"{fixture.get('name', 'fixture')}: interval daily {counter}")
        test.assertEqual(sum(int(r["covered_" + counter]) for r in sessions),
                         conservation[counter], f"session sealed coverage: {counter}")
    test.assertEqual(sum(int(r["observed_intervals"]) for r in sessions),
                     sum(r["record_kind"] == 1 for r in raw_rows))
    test.assertEqual(sum(int(r["duration_usec"]) for r in cohorts),
                     conservation["interval_total_usec"])

    # The durable fixture store already excludes rejected conflicts and retains
    # stale revisions. Frozen expected cumulative values independently cross-check
    # the selected latest record for the dedicated checkpoint/drop/copyover cases.
    for session in sessions:
        key = tuple(session[k] for k in ("session_boot_id", "session_process_id", "session_seq"))
        checkpoints = [r for r in raw_rows if r["record_kind"] == 3 and
                       tuple(r[k] for k in ("session_boot_id", "session_process_id", "session_seq")) == key]
        if checkpoints:
            latest = max(checkpoints, key=lambda r: r["checkpoint_revision"])
            test.assertEqual(session["latest_checkpoint_revision"], latest["checkpoint_revision"])
            for counter in COUNTERS:
                test.assertEqual(session[counter], latest[counter])
        else:
            test.assertEqual(session["latest_checkpoint_revision"], 0)
    expected_cumulative = (metrics.get("checkpoint", {}).get("latest_cumulative") or
                           metrics.get("coverage", {}).get("recovered_cumulative") or
                           metrics.get("copyover", {}).get("post_copyover_checkpoint_cumulative"))
    if expected_cumulative:
        test.assertEqual(len(sessions), 1)
        for counter in COUNTERS:
            test.assertEqual(sessions[0][counter], expected_cumulative[counter])

    if "time" in metrics:
        days = {}
        for row in player_days:
            days[row["utc_day"]] = days.get(row["utc_day"], 0) + int(row["resident_usec"])
        expected_days = {date(1970, 1, 1) + timedelta(days=int(day)): duration
                         for day, duration in metrics["time"]["utc_day_duration_usec"].items()}
        expected_days[UNKNOWN_DAY] = metrics["time"]["ambiguous_clock_jump_usec"]
        test.assertEqual(days, expected_days)
    if "coverage" in metrics:
        test.assertEqual(sum(r["attributable_usec"] for r in player_days),
                         metrics["coverage"]["known_context_usec"])
        test.assertEqual(sessions[0]["connected_usec"] - sessions[0]["covered_connected_usec"],
                         metrics["coverage"]["unattributed_connected_usec"])
    if "tail" in metrics:
        test.assertTrue(all(not row["exited"] and row["provisional"] for row in sessions))
        # The fixture records a process-global gap, not a session-scoped fact.
        state = select_scope(connection, "telemetry_rollup_state", scope)[0]
        test.assertTrue(state["quality_flags"] & 64)

    # Counts are membership cardinalities, not sums of per-page DISTINCT results.
    # Both membership grains must conserve the cohort duration; empty or missing
    # membership cannot falsely pass merely because its counter is also zero.
    for cohort in cohorts:
        key = tuple(cohort[k] for k in COHORT)
        for kind, count in ((1, "subject_count"), (2, "session_count")):
            group = [r for r in members if r["membership_kind"] == kind and
                     tuple(r[k] for k in COHORT) == key]
            test.assertEqual(cohort[count], len(group))
            test.assertEqual(cohort["duration_usec"], sum(r["duration_usec"] for r in group))
            test.assertEqual(cohort["attributable_usec"], sum(r["attributable_usec"] for r in group))
            for row in group:
                identity = tuple(row[k] for k in ("session_boot_id", "session_process_id", "session_seq"))
                if kind == 1:
                    test.assertEqual(identity, (0, 0, 0))
                else:
                    test.assertIn(identity, expected_sessions)
    states = select_scope(connection, "telemetry_rollup_state", scope)
    test.assertEqual(len(states), 1)
    test.assertTrue(states[0]["provisional"], "an ingest watermark never proves crash-tail completeness")
    return {"sessions": len(sessions), "days": len(player_days),
            "cohorts": len(cohorts), "memberships": len(members)}
