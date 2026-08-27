# Code Review: Runtime Connection Trust Boundaries

**Reviewed**: 2026-08-27
**Base commit**: `767c7909`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 08 diff: environment loading, explicit database configuration, target resolution and allow-listing, all server connection constructors, transport and session invariants, listener binding, certificate selection, restart-launcher database access, documentation, tests, and session records.

## Findings

### Critical / High

None.

### Medium - resolved

1. `.env` was initially checked with `lstat()` and then reopened by path, leaving a check/open race. It is now opened once with `O_NOFOLLOW`, validated with `fstat()`, and read through that validated descriptor. The launcher also rejects symbolic links and non-regular files.
2. The restart launcher initially validated the requested database name while the server can resolve a production-like name to `duris_dev` on a development port. The launcher now derives and allow-lists the same effective target, validates role and port, and uses that target for its shutdown record.
3. The launcher's auxiliary remote MySQL call initially lacked the server's transport boundary. It now uses a bounded connection and requires a CA plus server-certificate verification for non-loopback TCP.

### Low - resolved

1. The database guide described `DB_PORT` as required while the implementation deliberately permits the client default; the guide now matches the validated behavior.
2. Source contracts that assumed unchecked environment loading or a single raw query site were updated to assert secure loading and confinement of raw MySQL execution to `sql.c`.

## Behavioral Review

- Configuration validation precedes the first database connection and lookup-table write.
- Main, pool, fork-child, and legacy persistence connections use one constructor with the same deadlines, TLS decision, charset, UTC, isolation, and SQL-mode verification.
- Remote TCP cannot connect without enforced CA verification and a negotiated cipher; local sockets are restricted to explicit local loopback mode.
- Telnet, TLS telnet, and WebSocket listeners share the numeric bind address. The tracked key is accepted only for explicit local loopback operation.
- Diagnostics identify missing fields and failure categories without printing credentials, targets, SQL, or certificate contents.

## Verification

- Focused runtime trust, secret configuration, persistence path, log hygiene, and boot-log contracts: PASS.
- C++20 warning-as-error build, shell syntax, changed-line formatting, and whitespace checks: PASS.
- Full suite: PASS, 175/175 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
