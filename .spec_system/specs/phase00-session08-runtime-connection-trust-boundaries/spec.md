# Session Specification

**Session ID**: `phase00-session08-runtime-connection-trust-boundaries`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `767c7909`
**Work Window**: Fail-closed environment, secret-file, database transport/session, and listener-certificate trust boundaries.

---

## 1. Session Overview

Database access currently falls back to compiled shared credentials, accepts any named target, has no connect timeout, permits cleartext remote transport, and configures four connection constructors differently. `.env` mode is unchecked. The TLS listener silently falls back to a publicly tracked localhost key even when bound to every interface.

## 2. Objectives

1. Require an explicit environment role, credentials, and host/database allow-list before connecting or writing.
2. Reject unsafe `.env` ownership/type/mode without reading or logging its values.
3. Centralize bounded MySQL construction, protected remote transport, session setup, and verification for every connection type.
4. Bind explicit local development listeners to loopback and permit the tracked certificate only in that mode.
5. Fail network TLS startup without an operator-owned certificate/key.

## 3. Scope

### In Scope

- Runtime configuration and MySQL construction in `sql.h`, `sql.c`, `sql_pool.c`, and `sql_player.c`.
- Listener address application in telnet/TLS and WebSocket socket setup.
- Certificate fallback and private-key permission checks in `ssl.c`.
- `.env.example`, setup/configuration guidance, and focused fixture/source contracts.

### Outside This Work Window

- Secret-manager integrations, certificate issuance, schema ledger/checksum work, production migrations, or credential/key changes.

## 4. Technical Approach

Remove credential defaults and validate `ENVIRONMENT`, all required DB fields, strict port syntax, and an explicit comma-separated `host/database` allow-list before any connection. Treat loopback TCP or an explicit local socket as protected; otherwise require TLS enforcement, CA verification, and a negotiated cipher. Route main, pool, child, and legacy connections through one constructor with connect/read/write timeouts and a verified `utf8mb4`, UTC, READ COMMITTED, strict SQL-mode session contract. Apply a numeric `LISTEN_ADDRESS` to all listener sockets. Allow the tracked localhost certificate only for `ENVIRONMENT=local` plus an exact loopback listener; otherwise require an owner-controlled root key with restrictive mode.

## 5. Deliverables

| File | Change |
|------|--------|
| `src/sql.h`, `src/sql.c` | Explicit configuration validation and canonical connection constructor |
| `src/sql_pool.c`, `src/sql_player.c` | Use canonical construction for pool, child, and legacy connections |
| `src/comm.c`, `src/websocket.c`, `src/ssl.c` | Listener binding and fail-closed certificate selection |
| `.env.example`, `README.md`, `docs/CONFIGURATION.md` | Non-deployable placeholders and correction/deployment guidance |
| `tests/async/test_runtime_connection_trust.py` | Permission, target, transport, invariant, and certificate contracts |

## 6. Success Criteria

- [x] Missing role, credentials, allow-list, or approved target aborts before connection or boot writes.
- [x] `.env` must be a regular owner-controlled file with mode 0600 or stricter.
- [x] All four MySQL connection paths share bounded construction and verified session invariants.
- [x] Non-loopback/non-socket MySQL requires enforced verified TLS and a negotiated cipher.
- [x] Tracked localhost credentials require explicit local role and actual loopback binding.
- [x] Focused tests, formatting, C++20 build, and full regression suite pass.

## 7. Risks And Resolutions

- **Local boot becomes intentionally stricter**: document `chmod 600 .env`, explicit fields, allow-list, and loopback address.
- **Connector differences**: use capabilities provided by the repository's MariaDB-compatible client and verify the negotiated result after connection.
- **Session drift**: fail and close a connection unless every SET and verification query succeeds.
- **Secret disclosure**: diagnostics identify only field/category, role, and local/remote transport, never values.

## Next Steps

Session complete. Continue with `phase00-session09-private-chest-password-hardening`.
