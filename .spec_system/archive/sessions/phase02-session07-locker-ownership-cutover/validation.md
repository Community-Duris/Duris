# Validation

## Focused Gate

- `python3 tests/async/test_locker_ownership_cutover.py` - pass, 6/6; numeric identity,
  ACK-gated routing, synthetic topology exclusion, terminal ordering, exact restore, and
  guarded migration contracts are covered.
- All locker, critical-command, critical-transaction, and live-item focused source
  contracts pass.
- `tests/async/run_item_transfer_schema_mysql.sh` - pass against its isolated database.
- `migrations/apply_locker_ownership_cutover.sh` - pass twice under local/dev guards.
- `migrations/reconcile_item_ownership.sh` - pass with zero missing baselines, item
  revision mismatches, owner revision mismatches, or latest-owner mismatches.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- `make test-all` - pass; build, 191/191 Python regressions, and signal handlers.

All migration and MySQL mutation checks targeted the `.env`-identified local
development database or an isolated harness database. No production migration, wipe,
Redis flush, player export, credential change, or live service mutation was performed.
