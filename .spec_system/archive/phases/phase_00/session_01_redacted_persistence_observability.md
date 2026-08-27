# Session 01: Redacted Persistence Observability

**Session ID**: `phase00-session01-redacted-persistence-observability`
**Status**: Not Started
**Work Window**: One telemetry and privacy boundary spanning query execution, persistence
alerts, and save-state age, verified by log-content and metrics regressions.

---

## Objective

Make persistence diagnostics safe and operationally useful before later remediation
depends on them: identify call sites and durations without logging SQL or bound values,
remove unconditional ad hoc traces, and expose truthful dirty and deferred-save age.

---

## Scope

### In Scope (MVP)

- Replace raw SQL and SQL-prefix failure logs with stable site IDs, operation metadata,
  duration, and database error codes.
- Remove or explicitly disable unconditional `/tmp/garp-item-trace.log` and save traces
  that disclose player IDs, names, pointer values, or bound data.
- Record query duration by stable call site and distinguish main-thread execution from
  worker execution without copying query text into metrics.
- Expose dirty-player, deferred-save, oldest-save-age, retry, and failure state through
  existing operator diagnostics or a narrow new diagnostics surface.
- Add focused tests proving representative secrets and private values cannot appear in
  failure logs while required diagnostic fields remain available.

### Out of Scope

- A complete external dashboard, production alert routing, or the final 200-player load
  test.
- Converting every direct SQL call site to prepared statements.
- Phase 01 revision-gap and journal-age metrics that depend on the new save pipeline.

---

## Prerequisites

- [ ] Phase 00 PRD and source-review site inventory are available.
- [ ] Tests run without reading or printing `.env` contents.

---

## Deliverables

1. Redacted query and persistence diagnostic helpers in `src/sql.c`, worker persistence
   code, and the nearest shared headers.
2. Removed or safely gated ad hoc trace writers in `src/sql_player.c`, `src/account.c`,
   `src/actoth.c`, and related persistence paths.
3. Dirty-state and save-age reporting integrated with the existing persistence
   diagnostics.
4. Focused source-contract and runtime regressions under `tests/async/`.

---

## Success Criteria

- [ ] No failure path logs raw SQL, SQL prefixes, bound values, password hashes,
      confirmation values, descriptions, IP addresses, or pointer values.
- [ ] Query failures still report a stable site ID, duration, error code, and execution
      context sufficient for diagnosis.
- [ ] Operators can observe pending dirty/deferred work and its oldest age truthfully.
- [ ] Removed trace paths no longer perform synchronous append work during normal save
      or account operations.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
