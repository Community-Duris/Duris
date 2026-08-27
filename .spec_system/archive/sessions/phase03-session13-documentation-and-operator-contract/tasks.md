# Task Checklist

**Session ID**: `phase03-session13-documentation-and-operator-contract`
**Total Tasks**: 11
**Work Window**: One source-traced documentation, diagram, operator-procedure, and
automated contract boundary before the final integrated readiness gate.
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0313] Trace authoritative Phase 00-03 routes, limits, degraded modes, and verification commands into the session evidence notes (`src/`, `migrations/`, `scripts/`, `tests/async/`, `.spec_system/specs/phase03-session13-documentation-and-operator-contract/implementation-notes.md`)
- [x] T002 [S0313] Inventory current-guide links, anchors, command paths, configuration names, stale claims, and diagram topology before edits (`README.md`, `docs/`, `docs/diagrams/`)

---

## Documentation Implementation (6 tasks)

- [x] T003 [S0313] Correct the project overview, setup safety, architecture summary, and integrated documentation navigation without a readiness claim (`README.md`, `docs/README.md`)
- [x] T004 [S0313] Rewrite the implemented boot, game-thread, typed persistence, recovery, maintenance, and degraded-mode topology from traced source (`docs/ARCHITECTURE.md`)
- [x] T005 [S0313] Correct connection, consistent-read, snapshot, critical transaction, ownership, lifecycle, migration, lookup, and schema compatibility contracts (`docs/DATABASE.md`)
- [x] T006 [S0313] Reconcile environment precedence, required role/target/transport/session checks, actual deadlines, Redis scope, listener safety, and local behavior (`docs/CONFIGURATION.md`)
- [x] T007 [S0313] Add copyable operator diagnosis, migration, recovery, reconciliation, maintenance, archive, export, erasure, restore, and rollback procedures with explicit clone/backup/dry-run/production guards (`docs/RUNBOOK.md`)
- [x] T008 [S0313] Map focused, disposable database, workload, fault, lifecycle/privacy, and full validation gates to exact commands and evidence boundaries (`docs/TESTING.md`)

---

## Diagrams And Verification (3 tasks)

- [x] T009 [S0313] [P] Replace stale raw-worker and Redis-dirty-save server topology with accessible integrated typed-pipeline, MySQL-authority, Redis-cache, recovery, and lifecycle flow (`docs/diagrams/duris-server-architecture.html`)
- [x] T010 [S0313] [P] Replace the legacy event-table database model with accessible revision, operation, inbox/outbox, ownership, migration, and lifecycle authority groups (`docs/diagrams/duris-database-model.html`)
- [x] T011 [S0313] Add and run documentation link, anchor, path, command, configuration, stale-claim, diagram-accessibility, safety-language, ASCII/LF, focused-contract, and full repository verification (`tests/async/test_documentation_contract.py`, `Makefile`, `make test-all`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All added content is ASCII with LF line endings
- [x] `implementation-notes.md` records the traced evidence and remaining limitations
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence)

---

## Next Steps

Run the `implement` workflow step.
