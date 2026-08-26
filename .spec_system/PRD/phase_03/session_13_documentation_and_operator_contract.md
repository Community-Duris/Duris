# Session 13: Documentation and Operator Contract

**Session ID**: `phase03-session13-documentation-and-operator-contract`
**Status**: Not Started
**Work Window**: One source-traced documentation boundary covering final architecture,
database routes, configuration, migration, maintenance, lifecycle, recovery, testing,
privacy operations, diagrams, commands, and cross-link verification.

---

## Objective

Make the repository documentation describe the implemented Phase 00 through Phase 03
system accurately enough for developers and operators to deploy, diagnose, recover,
archive, export, and erase data without relying on obsolete worker or configuration
claims.

---

## Scope

### In Scope (MVP)

- Trace final code, migrations, manifests, scripts, tests, environment handling, worker
  topology, journal/inbox/outbox, load, ownership, lifecycle, and recovery behavior
  before editing prose or diagrams.
- Correct README architecture/configuration guidance and `docs/DATABASE.md`,
  `docs/ARCHITECTURE.md`, `docs/CONFIGURATION.md`, `docs/RUNBOOK.md`, and
  `docs/TESTING.md` to the implemented routes, guarantees, bounds, deadlines, and
  degraded modes.
- Document async consistent login, Phase 01 snapshot/journal recovery, Phase 02
  critical transactions and reconciliation, current item ownership, Phase 03 query
  evidence, maintenance scheduling, archive operations, and schema compatibility.
- Document the immutable migration workflow, baseline adoption, clone-only validation,
  boot preflight, lookup versioning, and exact operator commands without exposing local
  credentials or suggesting production experimentation.
- Add or update lifecycle/privacy guidance for the approved inventory, retention,
  archive, export, erasure, tombstone, restore, exception, and evidence workflows while
  distinguishing engineering posture from legal advice.
- Update repository-native architecture/database diagrams and navigation only when
  their represented topology changed; preserve generated artifacts in their established
  source/output model.
- Verify documented paths, anchors, commands, configuration names/defaults, warnings,
  and source references with focused documentation/source-contract checks.

### Out of Scope

- Claiming 200-player readiness before Session 14 passes.
- Inventing deployment-specific production credentials, transport, archive, legal, or
  alert-routing decisions.
- Broad rewriting of unrelated gameplay, builder, lore, or historical documentation.

---

## Prerequisites

- [ ] Sessions 01 through 12 are completed and their final implementation and operator
      behavior can be traced.
- [ ] Generated plan/load/export artifacts remain in ignored redacted locations.
- [ ] The lifecycle manifest distinguishes approved facts from unknown legal decisions.

---

## Deliverables

1. Corrected root README and architecture, database, configuration, testing, and runbook
   guides matching the final persistence and load system.
2. Migration, query-plan, maintenance, retention/archive, personal-data export, account
   erasure, tombstone, restore, and reconciliation operator guidance.
3. Updated diagrams/index links where the implemented topology or data flow changed.
4. Focused documentation link, command, environment, stale-claim, and source-reference
   verification.

---

## Success Criteria

- [ ] Documentation no longer claims that full player/object/ship persistence flows
      through the legacy three raw SQL event workers or that Redis owns player dirty
      durability.
- [ ] Login, snapshot, critical command, current-owner, journal, inbox/outbox, archive,
      migration, and boot boundaries match traced implementation and named tests.
- [ ] Configuration docs state actual precedence, required role/target/transport/session
      checks, defaults, deadlines, and safe local behavior.
- [ ] Every destructive or schema operation names clone/backup/dry-run safeguards and
      never directs development validation at production.
- [ ] Privacy/lifecycle docs describe purpose approval, export, erasure, exceptions,
      backup propagation, and remaining compliance limits without unsupported claims.
- [ ] Documented commands, paths, links, anchors, and source references pass focused
      checks, and no credential or private player/account value is included.
- [ ] Relevant documentation and repository validation commands pass.
