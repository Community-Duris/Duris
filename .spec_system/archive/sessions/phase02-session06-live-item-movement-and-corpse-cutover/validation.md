# Validation

## Focused Gate

- `python3 tests/async/test_live_item_movement_contract.py` - pass, 7/7; authoritative
  hydration, pointer-free pending state, ACK-gated routes, corpse sequencing, recovery
  authority, reconnect replay, and same-owner topology are covered.
- `tests/async/run_item_transfer_schema_mysql.sh` - pass against the guarded isolated
  database; creation, complete subtree movement, same-owner attach, nested detach,
  stale/incomplete/overflow rejection, replay, destruction, ledger, and outbox pass.
- `migrations/apply_live_item_movement_cutover.sh` - pass under its local/dev guards.
- `migrations/reconcile_item_ownership.sh` - pass with zero missing baselines, item
  revision mismatches, owner revision mismatches, or latest-owner mismatches.
- Account reward corpse lifecycle and immutable world recovery focused contracts pass.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- `make test-all` - pass; build, 190/190 Python regressions, and signal handlers.

All migrations and MySQL mutation tests targeted the `.env`-identified local
development database or disposable harness database. No production migration, wipe,
Redis flush, player export, credential change, or live service mutation was performed.
