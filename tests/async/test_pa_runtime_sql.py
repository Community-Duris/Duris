#!/usr/bin/env python3
"""Source-contract checks for the disposable real-SQL runtime authority test."""

# Exact source hashes captured from the untouched S01 worktree before recovery:
# pa_runtime_sql_harness.cpp: b0e768e70991df331304892c52ac5d528eab87d8b08293d7f7f42a475dd24662
# run_pa_runtime_sql.sh: 88dd07b68a7cff7c2332f3dbbb5e5d027f7392c3618209ac5a38968beabb3df5
# test_pa_runtime_sql.py: 0f6317dc89b6f22e7f391d96a92d8a9ae2978a86d58a44d2a25056d17c5a89e1

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[opening + 1:index]
    raise AssertionError(f"unterminated function body: {signature}")


def main() -> None:
    runtime = (ROOT / "src/sql/sql_economic_runtime.c").read_text()
    guard = (ROOT / "src/sql/sql_exclusion_guard.h").read_text()
    sql = (ROOT / "src/sql/sql.c").read_text()
    runner = (ROOT / "tests/async/run_pa_runtime_sql.sh").read_text()

    start = function_body(runtime, "bool sql_economic_runtime_start() noexcept")
    phases = [
        "sql_open_configured_connection(0)",
        "economic_sql_lifecycle_guard::acquire_runtime",
        "duris_sql_exclusion_guard_bind_economic_runtime",
        "runtime.connection = std::move(connection)",
        "runtime.authority = std::move(authority)",
        "runtime.process = getpid()",
    ]
    positions = [start.index(token) for token in phases]
    assert positions == sorted(positions), "runtime owner publishes before real SQL binding"
    assert "return false;" in start and "catch (...)" in start

    bind = function_body(guard, "static inline bool duris_sql_exclusion_guard_bind_economic_runtime")
    assert "mysql_thread_id(control)" in bind
    assert "state.economic_connection_id = session" in bind
    assert "duris_sql_exclusion_guard_validate(control)" in bind
    assert "status == duris_sql_exclusion_guard_status::inconclusive" in bind
    validate = function_body(
        guard,
        "static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_validate(MYSQL *probe)",
    )
    assert "IS_USED_LOCK(%s)" in validate
    assert "IS_USED_LOCK('%s')" in validate
    assert "state.economic_connection_id" in validate
    assert "duris_sql_exclusion_guard_status::inconclusive" in validate
    assert 'if (!strcmp(value, "0"))' in validate
    assert "state.lost = true" in validate
    assert validate.index('if (!strcmp(value, "0"))') < validate.index("state.lost = true")
    assert validate.index("duris_sql_exclusion_guard_scalar") < validate.index(
        'if (!strcmp(value, "0"))'
    )

    allows = function_body(guard, "static inline bool duris_sql_exclusion_guard_allows(MYSQL *probe)")
    check = function_body(
        guard,
        "static inline duris_sql_exclusion_guard_status duris_sql_exclusion_guard_check(MYSQL *probe)",
    )
    acquire = function_body(guard, "static inline bool duris_sql_exclusion_guard_acquire(MYSQL *connection)")
    assert "duris_sql_exclusion_guard_check(probe)" in allows
    assert "state.lost" in check and "duris_sql_exclusion_guard_validate(probe)" in check
    assert "state.connection || state.lost" in acquire
    assert acquire.index("state.connection || state.lost") < acquire.index("state.lost = false")
    assert "status == duris_sql_exclusion_guard_status::owner_lost" in acquire

    query = function_body(sql, "bool sql_trace_exec_at(struct persistence_query_site source_site")
    assert query.index("duris_sql_exclusion_guard_allows(DB)") < query.index(
        "sql_observed_execute_at(DB")
    assert "!strncasecmp(sql, \"ROLLBACK\", 8)" in query
    assert "sql[8] == ';'" in query
    assert "if (!rollback_only)" in query and "return false;" in query

    assert "test-slot-0.lock" in runner and "docker-heavy.lock" in runner
    assert "PA_RUNTIME_SQL_SLOT_LOCK" in runner and "PA_RUNTIME_SQL_HEAVY_LOCK" in runner
    network = (ROOT / "tests/async/_sql_fixture_network.sh").read_text()
    assert "--pull=never" in runner
    assert 'source "$ROOT/tests/async/_sql_fixture_network.sh"' in runner
    assert "sql_fixture_network" in runner and "127.0.0.1::3306" in network
    assert "from disposable_sql_fixture import private_network" in network
    provider = (ROOT / "tests/async/disposable_sql_fixture.py").read_text()
    assert "DURIS_TEST_CONTAINER" in provider
    assert 'metadata["Id"].startswith(socket.gethostname())' in provider
    assert "duris.task=pa-runtime-sql" in runner and "duris.run_id" in runner
    assert "mariadb:10.11" in runner and "mysql:8.0" in runner
    assert "pa_runtime_sql_test_" in runner
    for required_input in ("--binary", "--base-build", "--expected-head",
                           "--expected-binary-sha256"):
        assert required_input in runner
    for provenance_check in ("binary_sha256", "source_manifest", "rev-parse",
                             "ls-files", "--porcelain"):
        assert provenance_check in runner
    assert "_PASS" in runner and "item[\"exit\"] != 0" in runner
    assert "docker_bounded" in runner and "docker_probe" in runner
    assert "compile_bounded" in runner and "240s" in runner
    assert "cleanup_fixture_container" in runner and "no such container" in runner
    assert 'exit "$status"' in runner and 'set -euo pipefail' in runner
    assert ".env" not in runner

    print("PASS production runtime-owner, dual-lock binding, loss latch, query fence, fixture contract")


if __name__ == "__main__":
    main()
