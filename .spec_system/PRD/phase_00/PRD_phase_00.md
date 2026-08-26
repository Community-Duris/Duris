# PRD Phase 00: Correctness and Immediate Lag Removal

**Status**: In Progress
**Sessions**: 10 (initial estimate)
**Estimated Duration**: Adaptive; each session continues through its verification boundary

**Progress**: 2/10 sessions (20%)

---

## Overview

Phase 00 removes confirmed persistence correctness defects, immediate simulation-thread
latency hazards, and high-risk failure behavior before the revisioned persistence
pipeline is introduced in Phase 01. The work keeps gameplay semantics stable while
making save failure retryable, Redis failure bounded, shared-bank writes delta-only,
and persistence diagnostics safe enough to use during later load and fault testing.

The phase also closes security findings explicitly routed to P00: sensitive SQL and
trace logging, fail-open connection and certificate behavior, private-chest password
hashing, and the placeholder security/dependency baseline. Atomic epic, wallet, and
item-ownership transactions remain Phase 02 work; retention and data-subject workflows
remain Phase 03 work.

---

## Progress Tracker

| Session | Name | Status | Work Window | Validated |
|---------|------|--------|-------------|-----------|
| 01 | Redacted Persistence Observability | Complete | Safe call-site timing, trace removal, and dirty/save age metrics | 2026-08-27 |
| 02 | In-Memory Epic Bonus Hot Path | Complete | Epic-bonus hydration, mutation updates, expiry, and hot-path regressions | 2026-08-27 |
| 03 | Save Failure Retry and Terminal Safety | Complete | Deferred-save state plus all destructive terminal save boundaries | 2026-08-27 |
| 04 | Player Replacement State Cleanup | Complete | Transactional delete-and-replace semantics for removable player rows | 2026-08-27 |
| 05 | Combat and Artifact Persistence Correctness | Complete | Post-mutation frag publication and deterministic bind lookup outputs | 2026-08-27 |
| 06 | Redis Failure and Recovery Containment | Complete | Redis deadlines, dirty-set recovery, child bounds, and floor-delta ACK rules | 2026-08-27 |
| 07 | Account Bank Delta Safety | Complete | Checked delta-only shared-bank behavior and multi-character regressions | 2026-08-27 |
| 08 | Runtime Connection Trust Boundaries | Not Started | Fail-closed DB/TLS configuration and uniform connection invariants | - |
| 09 | Private Chest Password Hardening | Not Started | Adaptive salted hashes with compatible legacy upgrade behavior | - |
| 10 | Security Policy and Dependency Baseline | Not Started | Actionable policy, dependency inventory, SBOM, and CI security checks | - |

---

## Completed Sessions

- Session 01: Redacted Persistence Observability (completed 2026-08-27)
- Session 02: In-Memory Epic Bonus Hot Path (completed 2026-08-27)

---

## Upcoming Sessions

- Session 03: Save Failure Retry and Terminal Safety

---

## Objectives

1. Replace raw SQL and ad hoc persistence traces with redacted call-site timing and
   expose truthful dirty-state and save-age metrics.
2. Hydrate and maintain epic-bonus state in memory so regeneration and XP callbacks do
   not query MySQL or Redis.
3. Retry failed deferred saves with bounded backoff and prevent terminal transitions
   from destroying live state before durable success.
4. Delete obsolete timer, undead-slot, forged-item, and granted-command rows before
   inserting the current replacement set.
5. Persist victim frags only after the in-memory loss and initialize artifact-bind
   outputs deterministically on every result and failure path.
6. Bound Redis connect and command work, remove synchronous full-save fallbacks, recover
   inflight dirty state, bound temporary fork children, and retain floor deltas until
   matching world-snapshot success.
7. Remove absolute account-bank saves and make every temporary delta operation checked
   while the Phase 02 idempotent bank/wallet ledger is prepared.
8. Fail closed on unsafe database targets, credentials, transport, session invariants,
   and network TLS certificate fallback.
9. Replace unsalted private-chest SHA-256 values with versioned adaptive salted hashes
   and a safe legacy upgrade or reset path.
10. Replace placeholder security and dependency automation with an actionable baseline
    that can be reproduced in CI.

---

## Prerequisites

- The initialized Apex Spec state and master PRD are present.
- The August 26, 2026 database integration review remains the engineering baseline.
- Database migrations and write tests use only a backed-up development clone and
  non-production ports.
- C/C++ changes use the repository C++20 build and changed-line formatting checks.

---

## Planning Assumptions And Resolutions

### Working Assumptions

- Phase 00 is the first executable phase: the master PRD explicitly defines it, the
  analyzer reports Phase 00 with zero sessions, and the existing Phase 00 file directs
  phasebuild to replace its placeholder. Replacing that file in place is therefore the
  safe first-run behavior; Phase 01 is not advanced early.
- P00-tagged security findings belong in this phase when their remediation has a clear
  implementation and verification boundary. P00-S05 is limited here to temporary
  account-bank delta safety because the full economy and ownership transaction model is
  explicitly Phase 02. P00-S08 remains Phase 03 retention and data-rights work.
- The source tree still exhibits the Phase 00 findings: direct epic-bonus SQL, stranded
  deferred-save slots, destructive extraction after failed saves, missing replacement
  deletes, unbounded Redis calls and synchronous fallbacks, absolute account-bank
  writes, raw SQL logging, unsalted chest hashes, and placeholder security automation.
  State tracking is therefore correct that remediation has not started.

### Conflict Resolutions

- The master PRD previously imposed 2-4 hour and 12-25 task session limits, while Apex
  Spec 2.2.20 defines sessions by coherent outcome, working set, risk, capacity, and
  verification boundary with no fixed counters. The current Apex rule governs this
  phase, and the stale master PRD wording is updated in this phasebuild run.
- The master PRD described `.env` as mode 0600, while SECURITY-COMPLIANCE.md recorded
  mode 0644 and a read-only filesystem check confirmed 0644. The measured value is
  authoritative; the master PRD is corrected and Session 08 carries the remediation.

---

## Technical Considerations

### Architecture

The game thread remains the sole owner of mutable `P_char` and `P_obj` state. Phase 00
may add in-memory cache and retry state, but workers and forked children must not gain
new access to live mutable graphs. Temporary fork safeguards are containment only;
Phase 01 removes both fork-based snapshot paths.

Each behavior change must preserve truthful failure state. Dirty work, live characters,
inventory, fallback records, and floor deltas remain available until the exact durable
operation they protect has succeeded.

### Technologies

- C++20 server sources with legacy `.c` filenames
- MySQL or MariaDB with InnoDB
- hiredis and current Redis recovery paths
- Additive, guarded SQL migrations where schema changes are required
- Python and shell source-contract regressions under `tests/async/`
- Make, g++, clang-format, and repository CI workflows

### Risks

- Broad persistence fixes can change gameplay semantics: keep changes narrow and prove
  failure behavior with focused regressions.
- Retry or fallback changes can duplicate writes: preserve exact state identity and do
  not report success from failed or stale work.
- Redis containment can accidentally discard dirty or floor state: merge or retain
  state until explicit success and test every failure edge.
- Security hardening can make local development unusable: support explicit local mode
  without allowing production or network deployments to inherit local fallbacks.
- Password migration can lock users out: distinguish legacy hashes safely and upgrade
  only after successful verification or an explicit reset.

### Relevant Considerations

- [P00] **Save failures can become data loss**: Deferred retries and terminal failure
  handling are one durable-save contract in Session 03.
- [P00] **Per-pulse epic lookups exceed the event budget**: Session 02 removes database
  access from regeneration and XP calculations.
- [P00] **External I/O can stop the simulation**: Sessions 01 and 06 make the remaining
  exposure observable and bound Redis failure behavior.
- [P00] **The game thread owns mutable objects**: Every session preserves this ownership
  boundary; temporary fork work is containment rather than target architecture.
- [P00] **Do not clear or destroy before an exact durable ACK**: Save, Redis, and floor
  recovery acceptance checks retain state until verified success.
- [P00] **Use focused source-contract regressions**: Each implementation session extends
  the nearest test before the repository-wide phase gate.

---

## Success Criteria

Phase complete when:
- [ ] All 10 sessions completed and validated
- [ ] Regeneration and XP epic-bonus paths perform no database or Redis operation
- [ ] Failed deferred and terminal saves retain live, retryable state and report truth
- [ ] Cleared replacement-subtable values do not return after save and reload
- [ ] Victim frag and artifact-bind state is deterministic on success and failure
- [ ] Redis operations have bounded deadlines and no mutation path performs a
      synchronous full-player-save fallback
- [ ] Dirty inflight state and floor deltas survive the defined child and Redis failures
- [ ] Account-bank writes are checked and delta-only, with no stale absolute overwrite
- [ ] Persistence failure logs contain no SQL text or bound private values and expose
      call-site duration plus dirty/save age
- [ ] Database and TLS runtime configuration fails closed outside explicit safe local
      operation and every connection verifies required session invariants
- [ ] New or reset private-chest passwords use adaptive salted hashes and legacy values
      have a tested transition path
- [ ] Security policy and dependency checks are actionable and reproducible
- [ ] Relevant focused tests, formatting checks, and `make -C src` pass for all C/C++
      changes

---

## Dependencies

### Depends On

- Completed database integration and 200-player scalability investigation
- Initialized spec-system state and master PRD

### Enables

- Phase 01: Replace Forked Full Saves
- Safe instrumentation and failure baselines for later 200-player load testing
