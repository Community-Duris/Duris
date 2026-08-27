# Session Specification

**Session ID**: `phase03-session14-final-200-player-and-compliance-gate`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `7db76d7348cad8a24380e2e75ae934802a0ebd0d`
**Work Window**: One final integrated evidence boundary sharing target qualification,
eight workload profiles, fault control, reconciliation, lifecycle/privacy verification,
sanitized reporting, and repair/rerun decisions; the session ends only at the exact
200-player acceptance gate.

---

## 1. Session Overview

Session 14 is the final Phase 03 session. It converts the earlier focused, synthetic,
and source-contract evidence into one reproducible release gate that qualifies an
isolated representative clone, drives every required client ramp and hold, injects the
complete failure matrix, and reconciles all durable domains before making any readiness
claim. During implementation, the user explicitly authorized the configured local
development database, approved a five-minute checkpoint RPO, and deferred creation of
200 test accounts and the four-hour live capacity run. That decision closes the Phase
03 engineering work without making a 200-player readiness claim; the complete gate and
its thresholds remain intact for a later explicitly authorized execution.

The gate is deliberately fail closed. The repository currently contains precursor
harnesses and conservative representative-cardinality thresholds, but no checked-in
full-game workload driver, sanitized integrated report, approved lifecycle policy, or
representative-clone evidence. This session builds the missing safe tooling first, then
runs the exact gate only when its external target, identity, policy, RPO, fault-control,
and backup qualifications all pass.

---

## 2. Objectives

1. Qualify a backed-up, disposable, production-unreachable representative target and
   record only sanitized build, schema, policy, configuration, and aggregate identities.
2. Run all eight workload profiles through 25, 50, 100, and 200 clients and hold each
   200-client profile for at least 30 minutes while collecting exact latency/resource
   acceptance metrics.
3. Inject every required database, Redis, worker, process, disk, terminal, revision,
   replay, archive, migration, and restore fault with deterministic before/after state.
4. Reconcile revisions, balances, ownership, ledgers, inbox/results, outbox, archives,
   migration history, and lifecycle/privacy outcomes after every workload and fault run.
5. Repair any discovered defect narrowly, rerun every affected case plus the full gate,
   and publish a sanitized readiness decision without weakening requirements.

---

## 3. Prerequisites

### Required Sessions

- [x] `phase03-session01-consistent-player-load-transaction` through
  `phase03-session13-documentation-and-operator-contract` - provide the integrated
  runtime, schema, lifecycle, operator, and precursor evidence boundaries.

### Required Tools Or Knowledge

- Existing Phase 01/02 capacity and crash gates, Phase 03 query-plan thresholds,
  lifecycle tools, migration/runtime verifiers, persistence telemetry, and operator
  procedures.
- Python 3, the C++20 server build, Docker-backed disposable database tests, a
  non-production game runtime, and deployment-owned fault/backup adapters.

### Environment Requirements

- A separate Session 14 environment file naming a loopback or otherwise explicitly
  isolated non-production target; the repository `.env` is never an implicit gate
  target.
- A backed-up representative clone meeting every aggregate threshold in
  `tests/async/query_plan_manifest.json`, at least 200 sanitized load identities, and
  non-production game/Redis/database ports.
- Approved checkpoint RPO and lifecycle policy identities, a disposable Redis domain,
  reversible fault controls, and a backup restore target that cannot reach production.

---

## 4. Scope

### In Scope (MVP)

- Operators can run one manifest-driven gate whose qualification step refuses unsafe,
  under-sized, shared, policy-pending, or incompletely backed-up targets before mutation.
- Developers can reproduce the eight exact workloads, all client ramps/holds, complete
  fault matrix, telemetry capture, reconciliation, login, lifecycle/privacy, migration,
  and restore checks without committing raw evidence or private values.
- Reviewers can map every readiness statement to a sanitized run ID, threshold,
  aggregate metric, reconciliation result, and evidence checksum.
- Defects found by the integrated gate receive narrow source changes, focused
  regressions, affected-case reruns, and the complete repository acceptance suite.

### Outside This Work Window

- Production migrations, load, faults, lifecycle actions, or restores - prohibited by
  the PRD and repository safety rules.
- Fabricated representative history or shortened 200-client holds - neither can replace
  the binding clone and duration requirements.
- Controller policy, gameplay RPO, deployment topology, credentials, or fault authority
  invention - these remain external inputs and must be supplied explicitly.
- Phase 04 planning, scaffolding, or implementation - the user boundary ends after the
  Phase 03 transition artifacts are ready for manual review.

---

## 5. Technical Approach

### Architecture

Use a versioned JSON manifest as the complete gate contract and a Python orchestrator
that treats target qualification as a prerequisite state machine. The runner invokes
load clients, metrics capture, fault control, and read-only reconciliation through
argv-only subprocess boundaries with deadlines, schema-validated JSON exchange, and
explicit teardown. Raw outputs remain under ignored `tmp/session14-gate/`; the tracked
report contains only aggregate values, stable IDs, checksums, and pass/unqualified/fail
states.

Each workload/fault case starts from a named checkpoint, records preconditions, runs the
required ramp/hold or injected edge, stops mutation, and reconciles before the next case.
No failed or absent case is averaged away. Repairs restart from the earliest affected
checkpoint and the final readiness decision requires every manifest case to pass.

### Design Patterns

- **Qualification before mutation**: Refuse production-reachable, under-sized,
  unbacked, policy-pending, RPO-unknown, or partially configured targets.
- **Manifest completeness**: Stable IDs enumerate every profile, ramp, metric, fault,
  reconciliation, privacy, migration, and restore obligation.
- **Adapter isolation**: Deployment fault controls receive validated action IDs over a
  narrow JSON protocol; the gate never evaluates shell text.
- **Evidence chaining**: Sanitize first, then hash each case result into the final
  report so omissions and post-run edits are detectable.
- **Exact rerun semantics**: Any repair invalidates the affected evidence and requires
  affected plus full acceptance reruns.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `tests/async/session14_gate_manifest.json` | Version the complete workload, threshold, fault, reconciliation, and privacy contract. | ~320 |
| `scripts/session14_gate.py` | Qualify targets, orchestrate cases, enforce duration/coverage, sanitize evidence, and decide readiness. | ~650 |
| `tests/async/session14_load_client.py` | Drive authenticated sanitized clients through the eight profile command schedules. | ~450 |
| `tests/async/session14_fault_adapter.py` | Define and validate the argv-only deployment fault-control protocol. | ~220 |
| `tests/async/session14_reconcile.py` | Run bounded aggregate-only durable-domain and lifecycle reconciliation checks. | ~400 |
| `tests/async/test_session14_gate.py` | Exercise manifest completeness, unsafe-target refusal, evidence integrity, and failure/rerun behavior. | ~420 |
| `.spec_system/specs/phase03-session14-final-200-player-and-compliance-gate/readiness-report.md` | Publish the sanitized integrated outcome and remaining limitations. | ~220 |
| `docs/PHASE03_READINESS.md` | Provide the final operator checklist, exact commands, evidence boundaries, and readiness decision. | ~180 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `docs/TESTING.md` | Replace the future Session 14 placeholder with exact gate commands and evidence interpretation. | ~35 |
| `docs/RUNBOOK.md` | Add qualified execution, teardown, failed-case repair, rerun, and restore procedures. | ~45 |
| Nearest affected files under `src/`, `migrations/`, `scripts/`, and `tests/async/` | Apply only repairs demonstrated by a gate failure, with focused regressions. | Evidence-dependent |

---

## 7. Success Criteria

### Functional Requirements

- [x] Qualification rejects production reachability, shared/default targets, missing
  backup/restore proof, fewer than 200 sanitized identities, any representative table
  below its declared threshold, unknown RPO, pending policy, or incomplete adapters.
- [x] The manifest defines all eight profiles, 25/50/100/200 ramps, separate 30-minute
  minimum 200-client holds, and refuses shortened, skipped, or failed evidence.
- [x] Every required fault edge has reversible setup/teardown, compensation checks,
  and a reconciliation contract for lost, duplicate, resurrected, or stale effects.
- [x] Login, queue/journal/inbox/outbox, maintenance, migration, archive, export,
  erasure, and restored-tombstone invariants are represented by stable gate cases.
- [x] The tracked report contains no secret or row value and links every claim to a
  complete checksummed aggregate result.

### Testing Requirements

- [x] Manifest/runner focused tests and every earlier phase gate pass.
- [x] Both disposable MySQL 8.0 and MariaDB 10.11 schema/runtime variants pass.
- [x] `make -C src`, `./scripts/format.sh --check`, and `make test-all` pass after all
  repairs and evidence changes.

### Non-Functional Requirements

- [x] The gate enforces p99 game pulse below 250 ms, p99 new-event processing within
  25 ms, and rejects sustained event debt or main-thread external I/O evidence.
- [x] The gate enforces normal oldest critical command below 1 second, the explicitly
  approved five-minute checkpoint RPO, and configured resource bounds.
- [x] Raw evidence stays ignored and permission-restricted; tracked output is ASCII/LF,
  sanitized, aggregate-only, reproducible, and free of private target identifiers.

### Session Completion Gates

- [x] Complete gate tooling, stable IDs, sanitization, refusal, and evidence tests exist.
- [x] The authorized local database is backed up and the 141-step upgrade replays cleanly.
- [x] MySQL 8.0 and MariaDB 10.11 schema/runtime variants pass.
- [x] The current server builds, boots against the upgraded local database, and accepts
  an authenticated test-character session.
- [x] The deferred 200-account/four-hour run is reported as not run and supports no
  capacity claim.

### Deferred Capacity Acceptance

These are future release-readiness claims, not Session 14 or Phase 03 engineering
completion criteria. The user explicitly postponed this execution.

- [ ] At least 200 sanitized identities and a representative clone are available.
- [ ] All eight 200-client profiles hold for 30 minutes and the full fault/privacy gate
  passes.

### Quality Gates

- [x] All added content is ASCII with Unix LF line endings.
- [x] Code follows project conventions and external calls have deadlines and cleanup.
- [x] No production endpoint is used; the configured local development database and
  configured non-default Redis endpoint are used under explicit user authorization.
- [x] No Phase 04 plan, scaffold, session, or implementation artifact is created.

---

## 8. Implementation Notes

### Working Assumptions

- Representative qualification uses the conservative aggregate minimums already
  checked into `tests/async/query_plan_manifest.json`; Session 05 explicitly rejected
  the local fixture and fabricated capacity data, so Session 14 cannot substitute it.
- The checked-in lifecycle manifest remains `pending_controller_decision`, and the PRD
  leaves checkpoint RPO open. The runner therefore treats approved policy and RPO IDs
  as mandatory external inputs and cannot issue a readiness pass without them.
- At least four hours of 200-client holds are binding (eight profiles times 30 minutes),
  excluding ramps, faults, repairs, and reruns. Tests may simulate time to verify runner
  logic, but only monotonic wall-clock evidence from the qualified run satisfies the gate.
- Planning can proceed without user arbitration because missing external execution
  inputs are modeled as explicit fail-closed qualification results, never invented
  defaults or weakened acceptance criteria.

### Conflict Resolutions

- Earlier Phase 01/02 tests call 25/50/100/200 logical-client codec waves a gate, while
  the master PRD requires eight full-game profiles and long holds. The master Session 14
  boundary wins; earlier tests are prerequisites only.
- The goal asks to finish Phase 03, while current evidence says the configured fixture
  is unrepresentative and policy/RPO decisions are pending. Completion requires real
  qualifying inputs; a sanitized `UNQUALIFIED` report is honest progress but not a pass.
- The PRD permits representative cloned data but prohibits production operations. The
  runner accepts only a separately named isolated target with proof that production is
  unreachable; it never reads the repository `.env` implicitly.

### Key Considerations

- Preserve exact thresholds and case coverage even when execution is lengthy.
- Treat every raw artifact as sensitive until sanitization proves otherwise.
- Stop/teardown safely on the first qualification, adapter, privacy, or reconciliation
  violation while retaining enough aggregate evidence to reproduce the failure.

### Potential Challenges

- **No existing full-game load driver**: Build one protocol-aware client with stable
  profile schedules, bounded sockets, deterministic seeds, and clean disconnects.
- **Deployment-specific fault controls**: Use a narrow validated adapter protocol and
  require reversible preflight/teardown evidence for every action.
- **Large evidence volume**: Stream raw cases to ignored storage, aggregate by stable
  IDs, redact at ingestion, and checksum only sanitized records.
- **Long reruns**: Preserve per-case checkpoints but never reuse evidence invalidated by
  a repair or target/configuration identity change.

### Relevant Considerations

- [P00] **Capacity evidence is not representative**: Exact aggregate qualification is a
  hard precondition, not a warning.
- [P00] **Operational choices remain open**: RPO and lifecycle policy identities must
  be explicit inputs rather than defaults.
- [P00] **Database tests require isolation**: All integration work uses disposable
  Docker targets or the separately qualified clone.
- [P00] **Do not tune from tiny local plans**: No capacity or readiness conclusion may
  use the configured fixture.
- [P00] **Do not equate large buffers with resilience**: Resource-age/byte bounds and
  outage recovery are measured directly.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:

- Unsafe target or fault-adapter input reaching a mutating subprocess.
- Partial/failed/short workload evidence being accepted as a complete readiness run.
- Secrets, player values, raw SQL, hostnames, or IPs entering tracked reports.
- Cleanup failure leaving a fault active, clients connected, or a target unrestored.

---

## 9. Testing Strategy

### Unit Tests

- Validate manifest completeness, unique stable IDs, exact eight profiles/four ramps,
  duration floors, thresholds, fault edges, reconciliation coverage, and report schema.
- Exercise target classification, argv-only adapter validation, timeout/cleanup,
  redaction, checksum chaining, resume invalidation, and pass-decision logic.

### Integration Tests

- Run safe simulated adapters to prove refusal and orchestration semantics without a
  configured database, then run all existing focused, full, disposable DB, and
  dual-engine gates.
- On the qualified target, run every workload/fault/lifecycle case and read-only
  reconciliation between cases, with raw evidence confined to ignored storage.

### Runtime Verification

- Execute the complete minimum-duration gate, verify teardown and target restoration,
  render the sanitized report, then independently recompute coverage and evidence hashes.

### Edge Cases

- Missing/duplicate client identity, early disconnect, partial ramp, clock rollback,
  stale evidence after repair, adapter timeout, teardown failure, ambiguous commit,
  incomplete restore, policy/version change, and sanitizer rejection all fail closed.

---

## 10. Dependencies

### Other Sessions

- Depends on: all Phase 00-02 sessions and Phase 03 Sessions 01-13.
- Depended by: Phase 03 audit and transition only; no Phase 04 session is in scope.

---

## Next Steps

Run the `implement` workflow step to begin implementation.
