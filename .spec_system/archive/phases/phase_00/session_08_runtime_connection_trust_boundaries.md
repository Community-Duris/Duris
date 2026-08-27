# Session 08: Runtime Connection Trust Boundaries

**Session ID**: `phase00-session08-runtime-connection-trust-boundaries`
**Status**: Complete
**Work Window**: One fail-closed deployment boundary across environment selection,
MySQL connection construction, session invariants, transport, and listener certificate
fallback.

---

## Objective

Prevent missing or unsafe runtime configuration from silently selecting shared database
credentials, an unsafe target, unprotected non-local transport, or a publicly tracked
listener key, while keeping explicit loopback development usable.

---

## Scope

### In Scope (MVP)

- Require explicit database credentials and environment role instead of compiled
  shared defaults, with an allow-listed target and existing production-port safety.
- Validate `.env` ownership and mode without printing its contents; reject or clearly
  fail a secret file broader than 0600 and document the local correction path.
- Add bounded MySQL connect deadlines and require TLS or a protected local socket or
  tunnel for non-local hosts.
- Set and verify `utf8mb4`, time zone, isolation level, and SQL mode on the main, pool,
  child, and legacy persistence connections.
- Restrict `certs/localhost.key` fallback to explicit local mode bound to loopback and
  fail closed for network listeners without an operator-owned certificate.
- Add focused configuration and source-contract regressions without loading or logging
  real credential values.

### Out of Scope

- Phase 03 complete migration ledger, schema checksum, and lookup-table boot ordering.
- Deployment-provider-specific secret managers or certificate issuance.
- Editing or committing `.env`, production certificates, private keys, or credentials.

---

## Prerequisites

- [x] Session 01 ensures connection failures are logged without SQL or private values.
- [x] Tests can construct isolated environment fixtures without reading the repository
      `.env` contents.

---

## Deliverables

1. Fail-closed configuration and connection setup in `src/sql.h`, `src/sql.c`,
   `src/sql_pool.c`, child connection code, and related helpers.
2. Explicit safe-local listener certificate behavior in `src/ssl.c` and configuration.
3. Updated setup guidance in the repository documentation without credential examples
   that can be mistaken for deployable defaults.
4. Focused regressions under `tests/async/` for target, permission, transport, timeout,
   session-invariant, and certificate fallback rules.

---

## Success Criteria

- [x] Missing credentials or an unapproved database target fails before any boot write.
- [x] Secret-file permission checks identify mode 0644 as unsafe without exposing file
      contents, and safe local operation requires mode 0600 or stricter.
- [x] Every MySQL connection has a bounded connect deadline and verifies the same
      charset, time zone, isolation, and SQL-mode contract.
- [x] Every non-local database connection requires verified protected transport.
- [x] The tracked localhost key can be used only by explicit loopback development and
      never as a network deployment fallback.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
