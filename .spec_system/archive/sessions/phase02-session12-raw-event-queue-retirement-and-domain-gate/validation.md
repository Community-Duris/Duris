# Validation

## Focused Gate

- `python3 tests/async/test_phase02_raw_queue_retirement.py` - pass, 6/6.
- `python3 tests/async/test_phase02_capacity_and_crash_gate.py` - pass, 3/3,
  including bounded 25/50/100/200-client capture.
- Updated fallback, boot-log, item-parser, copyover, and pwipe source contracts pass.
- `tests/async/run_session_audit_schema_mysql.sh` - schema and exact duplicate replay pass.
- `tests/async/run_critical_command_schema_mysql.sh` - generic critical transaction pass.
- `migrations/reconcile_phase02_domains.sh` - all authoritative domains report zero
  mismatches against the local development database.

## Repository Gate

- `./scripts/format.sh --check` and `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `make security-check` - pass.
- `make test-all` - pass; server/tools build, 197/197 Python regressions, and signal
  handlers.

No production migration, wipe, Redis flush, credential change, export, or live-service
mutation was performed. Five proven-orphan rows from earlier isolated local auction
harness executions were deleted from `duris_dev`; they had no auction, current owner,
or ownership-ledger record.
