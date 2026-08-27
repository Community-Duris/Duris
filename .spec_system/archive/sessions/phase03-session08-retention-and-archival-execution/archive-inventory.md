# Archive Candidate and Execution Inventory

The canonical lifecycle policy has **zero approved destructive rules**. Every potential
history class remains `retain` with pending controller/retention decisions, so there is
no executable due-row selector and no archive/purge target.

Potentially growing non-protected histories include operational logs, offline messages,
PvP/statistics/progression rows, chest activity, auction histories, and legacy
persistence events. Protected Phase 02 ledgers, ownership/current rows, inbox/outbox,
outcomes, audit, quarantine, recovery, and the new lifecycle job/batch/evidence records
cannot receive an ordinary cleanup default. Session 05 did not approve new retention
indexes on its unqualified local fixture.

Before any future enablement, each approved store must add a sargable time/season due
predicate, stable source key, captured upper bound, serialization/checksum contract,
and named Phase 02 reconciliation hook. Parent-first archive copy and reverse dependency
finalization come from the lifecycle manifest.

The Session 06 scheduler now exposes `lifecycle_archive` with a fixed 64-row/25-ms
definition but `enabled=false`. Diagnostics display the disabled state, and durable
eleven-job v2 scheduler files load into the twelve-slot v3 format without scheduling
the new job.
