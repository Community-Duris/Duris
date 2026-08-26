# Testing

The project uses focused regression tests in `tests/async/`, plus
schema/migration checks at `tests/` root. The root `Makefile` provides a single
developer and CI gate while retaining fast commands for focused work.

## Layout

```
tests/
├── async/                       # focused regression + source-contract tests
│   ├── test_*.py                # plain python3 regressions; no framework
│   ├── run_*.sh                 # special-purpose and legacy thin wrappers
│   └── run_*_mysql.sh           # MySQL-backed schema-contract tests (need a live DB)
├── run_regression_tests.py      # discovery, bounded parallelism, failure summary
├── compare_bootstrap_mud_schema.sh   # diff live schema vs bootstrap baseline
├── test_migration_replay_safety.sh   # migration re-run safety
└── test_run_migration_persistence_schema.sh
```

## Test styles

**Source-contract tests** — read the C sources as text and assert structural
invariants (a guard exists, a call site was not reintroduced, an ordering
holds). Example (`tests/async/test_sql_pool_shutdown.py`): slices out
`sql_pool_acquire`/`sql_pool_shutdown` from `src/sql_pool.c` and asserts the
closing-pool checks are present. These need no database and no build.

**Behavioral tests** — boot or exercise server logic where feasible; most
regression coverage for past crashes is contract-style because full boots are
expensive.

**Schema tests** — verify migrations/persistence contracts against a real
MySQL instance (development database only).

## Running

```bash
# Complete safe developer/CI gate: builds all maintained binaries and tools,
# generates world data, then runs Python and native regression tests.
make test-all

# Regression tests without rebuilding the server or area editor:
make test

# Limit concurrency, filter by filename, or inspect discovery:
make test TEST_JOBS=1
make test-python TEST_MATCH=wear
make test-list

# Single test (preferred while iterating):
python3 tests/async/test_wear_all_regression.py
# or via its wrapper:
tests/async/run_sql_pool_shutdown.sh

# Isolated Docker/MySQL schema suites (Docker is an optional prerequisite):
make test-db
```

`TEST_JOBS=0` is the default and selects up to eight workers based on available
CPUs. Test output is buffered per process so parallel failures remain readable.
The runner executes every discovered `test_*.py` in a separate process and
returns nonzero if any test fails.

`make test-all` deliberately excludes Docker and externally provisioned
database checks. `make test-db` creates and destroys isolated MySQL containers;
the three scripts at the root of `tests/` require explicitly named disposable
or read-only databases and are manual migration-verification tools. Never point
them at production.

## Conventions for new tests

- One concern per file; name it after the feature/regression
  (`test_<feature>.py`). The root runner discovers it automatically. Add a
  `run_<feature>.sh` wrapper only when the test needs special environment setup
  or is useful as a standalone workflow.
- Keep them fast and deterministic; prefer source contracts over full boots
  when the invariant is structural.
- When you change behavior, add or update the focused regression test next to
  it — this is a stated repo convention (see `AGENTS.md`).
- Schema-related changes should extend or add a `_schema_mysql` variant so the
  contract is checked against a real database on a clone.

## What exists today (samples)

| Area | Tests |
|------|-------|
| Persistence | `run_persistence_contract_mysql.sh`, dirty-flush retry, SQL pool shutdown |
| Crash regressions | wear-all, relic pickup, stuck command gate |
| Saves | copyover save guards, ship save guards/dedup, epic save guards |
| Help files | class/race helpfile completeness contracts |
| Event loop | hotspot budget regression |

Historical context for several of these is documented under
`docs/ongoing-projects/`.
