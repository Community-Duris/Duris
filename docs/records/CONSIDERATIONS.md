# Considerations

> Institutional memory for AI assistants. Updated between phases via carryforward.
> **Line budget**: 600 max | **Last updated**: Phase 03 (2026-08-27)

---

## Active Concerns

Items that still constrain any future work or release claim.

### Technical Debt

- [P03] **Representative capacity proof is deferred**: The strict eight-profile gate is
  implemented, but the 200-account/four-hour run was postponed. Do not claim 200-player
  readiness until that exact run passes on a qualified non-production clone.
- [P03] **Lifecycle activation is policy-blocked**: Archive, export, and erasure
  mechanisms are validated with synthetic data, but canonical mutation remains disabled
  until controller-approved purposes, disclosure rules, retention periods, and actions exist.
- [P03] **Query plans remain unqualified**: The available fixture missed every
  representative-size threshold, so no index candidate was accepted. Re-run the plan
  gate on a backed-up representative development clone before tuning.

### External Dependencies

- [P03] **MySQL and MariaDB are both supported authorities**: Preserve the normalized
  runtime fingerprints and run compatibility checks on both engines after schema changes.
- [P03] **Production topology is not repository-defined**: The health endpoint is locally
  proven, but hosting, platform probes, WAF, external backup storage, and deployment
  triggers require the actual deployment owner and target.
- [P03] **Controller decisions are external**: Engineering must not invent legal basis,
  disclosure, retention, or erasure exceptions. Missing approval must continue to fail closed.

### Performance / Security

- [P03] **Five-minute recovery objective is approved but capacity-unproven**: Local and
  synthetic recovery contracts pass; only the deferred integrated run can prove the
  objective under representative 200-player load and injected faults.
- [P03] **Health is a readiness signal, not full diagnostics**: `GET /health` checks the
  live process and selected persistence authority without blocking the game thread or
  backing store, or exposing configuration, player, or database values. MariaDB mode
  reads its in-memory pool state.

### Architecture

- [P03] **The game thread remains the publication owner**: Database workers return bounded
  pointer-free values; player, inventory, pet, and follower graphs publish only after the
  complete snapshot validates.
- [P03] **Schema history and boot compatibility are one boundary**: Keep immutable
  migration checksums, lifecycle coverage, runtime fingerprints, and compiled lookup
  identity synchronized; drift must abort before any boot write or listener opens.
- [P03] **Lifecycle policy is data, not ad hoc SQL**: All archive, export, erasure, season,
  restore, and backup behavior must consume the shared manifest and preserve protected
  reconciliation and audit records.

---

## Lessons Learned

### What Worked

- [P03] **One consistent load snapshot**: A worker-owned repeatable-read transaction plus
  exact request/account identity prevents mixed revisions and partial login publication.
- [P03] **Validate, stage, then publish**: Indexed graph validation before relationship
  linking makes inventory and pet failures clean, linear, and exactly reclaimable.
- [P03] **Retain recovery checkpoints on read**: Durable rows should not be consumed before
  an in-memory publication that cannot share their database transaction.
- [P03] **Use last-good in-memory catalogs**: Set-based refresh plus validated publication
  removes callback SQL and random database sorts without turning refresh failure into bad state.
- [P03] **Make evidence gates fail closed**: Under-sized fixtures, missing cases, shortened
  holds, unsafe targets, malformed metrics, and pending policy correctly produce no claim.
- [P03] **Separate engineering completion from capacity evidence**: Complete tooling can
  be reviewed and shipped while the report honestly records that the representative run
  has not happened.
- [P03] **Normalize cross-engine metadata deliberately**: MySQL 8 and MariaDB expose
  equivalent schema through different declarations and metadata formatting; public client
  headers and normalized fingerprints keep clean builds portable.
- [P03] **Check live lookup rows on no-op boot**: A stored version/checksum alone cannot
  prove the authoritative table still matches the compiled dataset.
- [P03] **Document command side effects from source**: Legacy scripts may not honor
  conventional flags; executable contracts are more reliable than assumed CLI behavior.
- [P03] **Keep operational evidence aggregate-only**: Stable IDs, counts, timing, checksums,
  and outcomes support diagnosis without logging player values, SQL, credentials, or targets.

### What to Avoid

- [P03] **Do not tune from a tiny fixture**: Correctness fixtures cannot justify indexes,
  write amplification, lock cost, or a capacity claim.
- [P03] **Do not delete recovery rows on load**: Cross-system publication cannot make
  delete-on-read atomic and can lose retryable state.
- [P03] **Do not perform a blocking database ping in health handling**: Use bounded
  in-memory pool state so a probe cannot stall the simulation thread.
- [P03] **Do not fabricate migration history**: Adopt one verified legacy baseline and
  record only future immutable steps whose apply and verifier both succeed.
- [P03] **Do not enable policy-dependent mutation from technical completeness alone**:
  Synthetic archive/export/erasure proof is not controller approval or legal compliance.
- [P03] **Do not infer script help or dry-run support**: Inspect argument handling before
  invoking operational scripts, even on development databases.

### Tool/Library Notes

- [P03] **Public MySQL declarations are the portability boundary**: Repository headers
  include `mysql/mysql.h`; statement null-indicator types derive from `MYSQL_BIND` rather
  than naming MariaDB-only `my_bool`.
- [P03] **Strict clean builds expose local dependency assumptions**: Keep the GitHub code
  quality workflow and the compiler warning contract aligned with a fresh Ubuntu build.
- [P03] **Local WebSocket ports are configurable**: `DURIS_WEBSOCKET_PORT` allows isolated
  development probes; production defaults to 4050 unless explicitly configured.
- [P03] **Spec state is settled**: All four defined phases are complete and no
  session is active. The `.spec_system/` tracking tree was retired after Phase 03
  closeout; its contents remain in git history at commit `212592e3`.

---

## Resolved

Recently closed items from the persistence program.

| Phase | Item | Resolution |
|-------|------|------------|
| P03 | Partial and N+1 player loads | One bounded consistent snapshot now stages and atomically publishes player, item, pet, PvP, and epic-task read state. |
| P03 | Runtime/schema drift | Immutable migration history and dual-engine compatibility abort before boot mutation or service publication. |
| P03 | Aligned unbounded maintenance | Eleven typed jobs use deterministic staggering, budgets, cursors, retries, and durable completion state. |
| P03 | Missing lifecycle inventory | Every declared database and non-database store is covered by one fail-closed manifest. |
| P02 | Split critical-domain writes | Operation-keyed transactions now couple authoritative rows, ledgers, results, and outbox state. |
| P01 | Forked persistence and stale saves | Revisioned workers, exact acknowledgements, typed journals, and bounded recovery replaced the unsafe fork paths. |
| P00 | Sensitive persistence logging | Stable metadata-only diagnostics and log-hygiene regressions removed raw SQL and ad hoc private traces. |

---

*Auto-generated by carryforward. Direct edits allowed but may be overwritten.*
