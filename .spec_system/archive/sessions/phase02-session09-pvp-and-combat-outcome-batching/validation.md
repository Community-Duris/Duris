# Validation

## Focused Gate

- `python3 tests/async/test_combat_outcome_transactional_cutover.py` - pass, 4/4;
  bounded codecs, exact fence reconstruction, immutable capture, ACK-only publication,
  schema, and migration contracts are covered.
- `tests/async/run_combat_outcome_schema_mysql.sh` - pass; one atomic group outcome,
  immutable participant rows, frag/epic/currency ledgers, replay, stale rejection, and
  isolated cleanup are covered against the local development database.
- Epic and currency transaction contract tests pass with combat completion publishing
  through their centralized authoritative ACK APIs.
- `migrations/verify_combat_outcome_schema.sh` - pass after applying the additive schema
  to the `.env`-identified local development database.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- Every `tests/async/test_*.py` script passes independently, 193/193.
- `make test-all` - pass; server/tools build, 193/193 Python regressions, and signal
  handlers.

All migration and MySQL mutation checks targeted the `.env`-identified local development
database and cleaned their high-numbered named fixtures. No production migration, wipe,
Redis flush, player export, credential change, or live service mutation was performed.
