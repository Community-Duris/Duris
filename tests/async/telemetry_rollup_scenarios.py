"""Synthetic admitted-fact scenarios supplementing the frozen #268 golden cases."""
from collections import defaultdict
from copy import deepcopy
from statistics import median

from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows

DAY_USEC = 86_400_000_000


def non_additive_membership():
    """Concurrent sessions, repeated membership, another day/cohort, late input.

The test-only account relationship is deliberately NOT injected into the raw
schema. A report must explicitly decline account metrics, not infer them from
these subjects. Character/session contribution sums remain valid observations.
"""
    _, base = golden_rows(FIXTURE_DIR / "normal_interval.json")
    interval = next(r for r in base if r["record_kind"] == 1)
    specs = [(501, 1, 0, 100, 0), (502, 2, 0, 100, 0),
             (501, 1, 100, 100, 0), (503, 3, 0, 1000, 0),
             (501, 1, DAY_USEC, 50, 0), (501, 1, 200, 20, 1)]
    rows = []
    for index, (subject, session, utc_start, duration, zone_offset) in enumerate(specs, 1):
        row = deepcopy(interval)
        row.update(boot_id=9268, process_id=1, record_seq=index, ingest_id=index * 3 + 1,
                   session_boot_id=9268, session_process_id=1, session_seq=session,
                   connection_boot_id=9268, connection_process_id=1, connection_seq=session,
                   subject_id=subject, pid=subject, start_monotonic_usec=index * 2000,
                   end_monotonic_usec=index * 2000 + duration, duration_usec=duration,
                   start_utc_usec=utc_start, end_utc_usec=utc_start + duration,
                   occurrence_utc_usec=utc_start + duration,
                   ingested_utc_usec=2 * DAY_USEC + index, quality_flags=0)
        row["zone_vnum"] += zone_offset
        rows.append(row)
    first_day_main = defaultdict(int)
    for row in rows:
        if row["start_utc_usec"] < DAY_USEC and row["zone_vnum"] == interval["zone_vnum"]:
            first_day_main[row["subject_id"]] += row["duration_usec"]
    values = sorted(first_day_main.values())
    expected = {
        "unique_subjects_all_days_and_cohorts": len({r["subject_id"] for r in rows}),
        "first_day_main_member_durations": values,
        "first_day_main_median_usec": median(values),
        "first_day_main_mean_usec": sum(values) / len(values),
        "interval_total_usec": sum(r["duration_usec"] for r in rows),
        "test_only_accounts": {501: 77, 502: 77, 503: 88},
        "account_metrics_available": False,
        "late_ingest_id": rows[-1]["ingest_id"],
    }
    assert expected["first_day_main_median_usec"] != expected["first_day_main_mean_usec"]
    return rows, expected


if __name__ == "__main__":
    rows, expected = non_additive_membership()
    for row in rows:
        assert row["duration_usec"] == row["end_monotonic_usec"] - row["start_monotonic_usec"]
        assert row["duration_usec"] == row["end_utc_usec"] - row["start_utc_usec"]
    assert len(rows) == 6 and len({r["ingest_id"] for r in rows}) == 6
    print("ISSUE268_NON_ADDITIVE_SCENARIO_VALID", expected)
