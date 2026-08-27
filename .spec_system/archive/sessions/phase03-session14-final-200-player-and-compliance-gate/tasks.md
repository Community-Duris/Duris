# Task Checklist

**Session ID**: `phase03-session14-final-200-player-and-compliance-gate`
**Total Tasks**: 17
**Work Window**: One final integrated qualification, workload, fault, reconciliation,
privacy, repair, and evidence boundary for the binding 200-player readiness gate.
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (3 tasks)

- [x] T001 [S0314] Trace all prior phase gates, target thresholds, runtime metrics,
  reconciliation tools, lifecycle guards, schema verifiers, and exact master acceptance
  obligations into implementation evidence (`tests/async/`, `scripts/`, `src/`,
  `.spec_system/specs/phase03-session14-final-200-player-and-compliance-gate/implementation-notes.md`)
- [x] T002 [S0314] Define the complete stable-ID manifest for eight profiles, four
  ramps, duration floors, metrics, faults, reconciliations, privacy, migration, restore,
  teardown, and evidence (`tests/async/session14_gate_manifest.json`)
- [x] T003 [S0314] Add strict target/configuration qualification using a separately
  named environment file, aggregate clone thresholds, 200 sanitized identities,
  backup/restore proof, approved RPO/policy IDs, isolated ports, and production
  unreachability before any mutation (`scripts/session14_gate.py`)

---

## Foundation (4 tasks)

- [x] T004 [S0314] Implement schema-validated result state, monotonic duration,
  sanitizer, evidence checksum, resume invalidation, fail-closed decision, and
  permission-restricted ignored-output handling (`scripts/session14_gate.py`)
- [x] T005 [S0314] Implement the protocol-aware load client with bounded sockets,
  authenticated pseudonymous identities, deterministic profile schedules, explicit
  login/empty/error/disconnect outcomes, and cleanup on scope exit
  (`tests/async/session14_load_client.py`)
- [x] T006 [S0314] [P] Implement the argv-only fault-adapter protocol with allow-listed
  action IDs, schema-validated input/output, deadlines, reversible setup/teardown, and
  explicit error mapping (`tests/async/session14_fault_adapter.py`)
- [x] T007 [S0314] [P] Implement aggregate-only durable-domain reconciliation with
  bounded queries, deterministic ordering, transport deadlines, and no row/private
  value output (`tests/async/session14_reconcile.py`)

---

## Implementation (5 tasks)

- [x] T008 [S0314] Orchestrate 25/50/100/200 ramps and separate 30-minute minimum holds
  for all eight profiles, rejecting partial, shortened, skipped, or mixed-identity
  evidence (`scripts/session14_gate.py`, `tests/async/session14_gate_manifest.json`)
- [x] T009 [S0314] Orchestrate every database, Redis, worker, game, disk, terminal,
  revision, replay, archive, migration, and restore fault with checkpoints,
  reconciliation, timeout handling, and teardown compensation
  (`scripts/session14_gate.py`, `tests/async/session14_fault_adapter.py`)
- [x] T010 [S0314] Collect pulse/event/query/resource/login telemetry by stable ID and
  enforce p99, event-debt, main-thread-I/O, command-age, RPO, queue/journal/outbox,
  maintenance, retry, lock, revision, and checkpoint bounds (`scripts/session14_gate.py`)
- [x] T011 [S0314] Exercise synthetic expired-row retention, export isolation/secret
  exclusion, erasure across the approved manifest, replay/restore tombstones, migration
  drift, and pre-write boot rejection on isolated targets
  (`scripts/session14_gate.py`, `tests/async/session14_reconcile.py`)
- [x] T012 [S0314] Publish only aggregate sanitized case outcomes, checksums, exact
  limitations, operator commands, teardown, repair, and rerun rules
  (`.spec_system/specs/phase03-session14-final-200-player-and-compliance-gate/readiness-report.md`,
  `docs/PHASE03_READINESS.md`, `docs/TESTING.md`, `docs/RUNBOOK.md`)

---

## Testing And Evidence (5 tasks)

- [x] T013 [S0314] [P] Add focused tests for manifest completeness, unsafe/default
  target refusal, adapter injection rejection, duration floors, sanitizer failures,
  evidence tampering, cleanup, incomplete cases, and repair invalidation
  (`tests/async/test_session14_gate.py`)
- [x] T014 [S0314] Run every prior phase precursor, focused lifecycle/privacy,
  migration/runtime, repository, disposable MySQL, and dual-engine gate without using
  the configured development database (`tests/async/`, `make test-all`, `make test-db`)
- [x] T015 [S0314] Run the user-authorized local development integration boundary:
  backed-up database migration and replay, immutable/runtime verification, configured
  Redis invalidation, local game boot, and authenticated character smoke. Preserve the
  complete 200-player gate for its explicitly deferred future run without claiming it
  passed (`scripts/session14_gate.py`, `tmp/session14-gate/`)
- [x] T016 [S0314] Repair each demonstrated failure narrowly with a focused regression,
  then rerun the affected checkpoint and invalidate stale evidence before the complete
  acceptance rerun (`src/`, `migrations/`, `scripts/`, `tests/async/`)
- [x] T017 [S0314] Verify final coverage/checksums, server build, formatting, full tests,
  ASCII/LF, diff integrity, raw-output ignore rules, secret/private-value absence, and
  zero Phase 04 artifacts (`.spec_system/specs/phase03-session14-final-200-player-and-compliance-gate/`,
  `docs/PHASE03_READINESS.md`, `git diff --check`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] `implementation-notes.md` records exact local evidence and deferred limitations
- [x] Raw clone data, credentials, logs, plans, exports, and generated load output remain ignored
- [x] No Phase 04 artifact exists
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence)

---

## Next Steps

Session 14 and Phase 03 are complete. Enter the Phase Transition at `audit`; do not
start, plan, or scaffold Phase 04.
