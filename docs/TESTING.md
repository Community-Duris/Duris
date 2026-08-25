# Testing

The project uses a focused regression-test harness in `tests/async/`, plus
schema/migration checks at `tests/` root. There is no single test runner;
convention is to run the smallest relevant test directly.

## Layout

```
tests/
├── async/                       # focused regression + source-contract tests
│   ├── test_*.py                # ~130 test scripts (plain python3, no framework)
│   ├── run_*.sh                 # thin wrappers: exec the matching test_*.py
│   └── run_*_mysql.sh           # MySQL-backed schema-contract tests (need a live DB)
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
# Single test (preferred during development):
python3 tests/async/test_wear_all_regression.py
# or via its wrapper:
tests/async/run_sql_pool_shutdown.sh

# Schema test against dev DB:
tests/async/run_persistence_contract_mysql.sh
```

Wrappers self-anchor to the repo root, so they work from any directory.
Exit code 0 = pass; failures print which check failed and exit nonzero.

## Conventions for new tests

- One concern per file; name it after the feature/regression
  (`test_<feature>.py`), and add a matching `run_<feature>.sh` wrapper that
  just invokes it.
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
