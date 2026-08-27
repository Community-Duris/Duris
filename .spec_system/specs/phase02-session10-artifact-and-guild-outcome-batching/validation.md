# Validation

## Focused Gate

- `python3 tests/async/test_artifact_guild_transactional_cutover.py` - pass, 4/4;
  deterministic child identity, bounded codecs, exact lock reconstruction, early
  immutable capture, schema parity, and generic-save protection are covered.
- `tests/async/run_artifact_guild_schema_mysql.sh` - pass against the local development
  database; atomic prestige/construction threshold apply, artifact feed, replay, stale
  rejection, immutable ledgers, result, and outbox behavior are covered.
- `tests/async/run_combat_outcome_schema_mysql.sh` and the epic/combat source contracts
  pass after composing the artifact/guild child behind each parent player key.
- `migrations/verify_artifact_guild_outcome_schema.sh` - pass.
- `migrations/reconcile_artifact_guild_outcomes.sh` - pass with zero artifact-ledger,
  compatibility-state, or latest-guild mismatches.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- Every `tests/async/test_*.py` script passes independently, 194/194.
- `make test-all` - pass; server/tools build, 194/194 Python regressions, and signal
  handlers.

Schema apply, baseline, verification, reconciliation, and MySQL harness mutations used
only the `.env`-identified local development database. Harness fixtures were isolated
and cleaned. No production migration, wipe, Redis flush, player export, credential
change, or live service mutation was performed.
