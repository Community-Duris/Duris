# Validation

## Focused Gate

- `python3 tests/async/test_auction_transactional_cutover.py` - pass, 4/4; bounded
  codec/result capacity, exact entity fences, ACK-only live routes, durable outbox
  publication, schema, and migration contracts are covered.
- `tests/async/run_auction_transaction_schema_mysql.sh` - pass; atomic listing, early
  finalize rejection, stale bid rejection, replay, concurrent money claim, normal bid,
  buy-now settlement, refund, item claim, no-bid expiry, and seller return are covered.
- Currency, item ownership, epic, and generic critical inbox/outbox MySQL harnesses pass
  with the auction repository linked into the shared dispatcher.
- `migrations/apply_auction_transactional_cutover.sh` - pass twice under local/dev guards.
- `migrations/reconcile_auction_transactions.sh` - zero authoritative missing custody,
  owner mismatch, custody revision mismatch, or open quarantine rows.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- `make test-all` - pass; build, 192/192 Python regressions, and signal handlers.

All migration and MySQL mutation checks targeted the `.env`-identified local development
database and cleaned their named fixtures. No production migration, wipe, Redis flush,
player export, credential change, or live service mutation was performed.
