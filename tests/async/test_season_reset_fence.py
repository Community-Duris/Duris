#!/usr/bin/env python3
"""Source contracts for the durable, irreversible season reset boundary."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SQL = (ROOT / "src/sql.c").read_text()
ACTWIZ = (ROOT / "src/actwiz.c").read_text()
REDIS = (ROOT / "src/redis.c").read_text()
MIGRATION = (ROOT / "migrations/immutable/0003_season_reset_state.sql").read_text()
VERIFY = (ROOT / "migrations/immutable/0003_season_reset_state.sh").read_text()


def section(source: str, start: str, end: str) -> str:
    return source[source.index(start):source.index(end, source.index(start))]


for token in (
    "season_epoch BIGINT UNSIGNED NOT NULL",
    "reset_status ENUM('active', 'resetting')",
    "PRIMARY KEY (state_id)",
    "ENGINE=InnoDB",
    "VALUES (1, 1, 'active', NULL, NULL)",
):
    assert token in MIGRATION
assert "season reset state verified" in VERIFY

boot_start = SQL.rindex("int initialize_mysql()")
boot = SQL[boot_start:SQL.index("/* Handle a query", boot_start)]
assert boot.index("sql_verify_boot_database()") < boot.index(
    "sql_load_active_season_state()"
) < boot.index("sql_populate_lookup_tables()")
load = section(SQL, "static bool sql_load_active_season_state", "static bool sql_begin_pwipe_epoch")
assert "reset_status" in load and 'strcmp(row[1], "active")' in load

begin = section(SQL, "static bool sql_begin_pwipe_epoch", "static bool sql_complete_pwipe_epoch")
assert begin.index('"START TRANSACTION"') < begin.index("FOR UPDATE")
assert begin.index("reset_status='resetting'") < begin.index(
    "pwipe_crossed_boundary = true"
) < begin.index('"COMMIT"')
assert '"ROLLBACK"' in begin

wipe = SQL[SQL.index("bool sql_pwipe(int code_verify)", SQL.index("#else")):]
first_mutation = wipe.index("sql_clear_zone_trophy()")
assert wipe.index("redis_validate_pwipe_state()") < wipe.index(
    "sql_begin_pwipe_epoch()"
) < first_mutation
assert wipe.index("redis_clear_pwipe_state()") < wipe.index("sql_complete_pwipe_epoch()")

validate = section(REDIS, "bool redis_validate_pwipe_state", "void redis_cleanup")
assert "redis_connection_open(redis_maintenance_settings)" in validate
assert 'redis_command(context, "PING")' in validate

pwipe_case = section(ACTWIZ, "case TimedShutdownData::PWIPE:", "default:")
failure = pwipe_case[pwipe_case.index("if (!sql_pwipe(1723699))"):]
assert failure.index("sql_pwipe_crossed_boundary()") < failure.index(
    "shutdownflag = _pwipe = 0"
)
assert "forcing fenced shutdown" in failure

print("durable season reset boundary contracts passed")
