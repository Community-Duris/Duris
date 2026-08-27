# Validation

## Focused Gate

- `python3 tests/async/test_boon_reward_zone_transactional_cutover.py` - pass, 5/5.
- `tests/async/run_boon_reward_zone_schema_mysql.sh` - pass against the `.env`-identified
  local development database; boon progress/shop and immutable two-player zone-touch
  apply/replay behavior are covered.
- Account-bound reward source, exact-item/cooldown, recovery, and player UX contracts
  pass unchanged.
- `migrations/verify_boon_reward_zone_schema.sh` - pass.
- `migrations/reconcile_boon_reward_zone.sh` - pass with zero entry-count, participant,
  or committed-inbox mismatches.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- `tests/async/run_critical_command_schema_mysql.sh` - pass.
- `make test-all` - pass; server/tools build, 195/195 Python regressions, and signal
  handlers.

All schema writes and isolated harness mutations targeted only the local `duris_dev`
database and cleaned their fixtures. No production migration, wipe, Redis flush,
credential change, export, or live-service mutation was performed.
