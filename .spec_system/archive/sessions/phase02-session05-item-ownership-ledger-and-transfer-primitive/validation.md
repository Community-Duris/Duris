# Validation

## Focused Gate

- `python3 tests/async/test_item_ownership_contract.py` - pass, 6/6; schema wiring,
  typed bounds, repository atomicity, UID boot authority, snapshot isolation, and the
  synthetic coordinator adapter are covered.
- `tests/async/run_item_transfer_schema_mysql.sh` - pass against the guarded local
  development database; exact 6-table/55-column/18-index/2-foreign-key schema plus
  allocation, creation, subtree, stale, incomplete, overflow, replay, transfer,
  destruction, duplicate-ID, ledger, inbox, and outbox behavior pass.
- `migrations/baseline_item_ownership.sh` - pass and re-runnable; 113 unambiguous legacy
  player items captured and 13 duplicate-UID rows retained in quarantine.
- `migrations/reconcile_item_ownership.sh` - pass with zero missing baselines, item
  revision mismatches, owner revision mismatches, or latest-owner mismatches.
- Critical coordinator, boot preflight, currency, migration invocation, and snapshot
  focused regressions pass.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `make security-check` - pass.
- `make test-all` - pass; build, 189/189 Python regressions, and signal handlers.

No production database, credentials, retained player exports, Redis flush, wipe, or
destructive migration was used. The running service targets another workspace and was
left untouched, so no new-binary live login is claimed.
