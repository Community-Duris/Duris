# Session Specification

**Session ID**: `phase03-session13-documentation-and-operator-contract`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `8cb0684abf4f327192ccf213fd7b9bfe12bbc090`
**Work Window**: One source-traced documentation boundary spanning the integrated
persistence topology, safe operator procedures, native diagrams, navigation, and one
automated contract gate before the final readiness session.

---

## 1. Session Overview

Session 13 reconciles developer and operator documentation with the implementation
completed across Phases 00 through 03 Session 12. It is next because all runtime,
transaction, lifecycle, migration, and boot contracts now exist, while the final
200-player gate must run against documentation that names their actual controls and
limitations.

The work traces source, manifests, scripts, and focused tests before changing prose.
It updates the main guides and the two repository-native HTML diagrams, then adds a
focused documentation contract that detects broken links, invalid commands and paths,
missing configuration names, unsafe operational guidance, and known stale topology
claims. No application behavior or database schema is changed.

---

## 2. Objectives

1. Describe the implemented login, snapshot, journal, critical-command, ownership,
   maintenance, lifecycle, migration, compatibility, and recovery boundaries exactly.
2. Publish safe, executable operator procedures with actual configuration precedence,
   deadlines, degraded modes, clone-only protections, and fail-closed outcomes.
3. Replace stale architecture and database diagrams with the integrated topology and
   authoritative data relationships implemented through Session 12.
4. Enforce the documentation contract with automated link, path, command,
   configuration, safety-language, and stale-claim checks.

---

## 3. Prerequisites

### Required Sessions

- [x] `phase03-session01-consistent-player-load-transaction` through
  `phase03-session12-boot-schema-and-lookup-compatibility` - provide the final traced
  implementation and operator surfaces to document.

### Required Tools Or Knowledge

- Repository source, manifests, migrations, scripts, focused tests, and prior session
  validation reports are the authoritative evidence.
- Markdown, shell-command, relative-link, anchor, and standalone HTML/SVG inspection.

### Environment Requirements

- Read-only source tracing requires no configured database or runtime credentials.
- Any command verification that exercises schema behavior must use existing disposable
  MySQL/MariaDB wrappers and never the configured database.

---

## 4. Scope

### In Scope (MVP)

- Developers and operators can trace the complete persistence topology from README and
  architecture/database guides to source, tests, and specialized runbooks.
- Operators can configure, migrate, verify, start, diagnose, reconcile, archive,
  export, erase, restore, and test through exact repository commands with explicit
  target qualification and disabled-policy boundaries.
- Maintainers can inspect current server and database diagrams that distinguish the
  game thread, typed workers, journal/inbox/outbox, MySQL authority, optional Redis
  cache, recovery, lifecycle, and compatibility boundaries.
- CI and developers can run one focused source-contract test for documentation links,
  paths, command entry points, configuration names, stale claims, and safety warnings.

### Outside This Work Window

- Running the 200-player workload or making a readiness claim - Session 14 owns the
  integrated evidence gate.
- Runtime, schema, lifecycle-policy, or infrastructure changes - this session documents
  implemented behavior and records remaining limitations without inventing decisions.
- Broad gameplay, builder, lore, and historical-documentation rewrites - unrelated to
  the Phase 03 operator contract.

---

## 5. Technical Approach

### Architecture

Build a claim matrix from source and executable artifacts, then edit the top-level
guides as an integrated navigation layer over specialized documents. Keep normative
commands in fenced blocks, name whether they are read-only, disposable-only, or
mutation-capable, and state the required guard before every destructive or schema
operation. Update the standalone HTML/SVG diagrams in their established source/output
files and validate their accessible descriptions as part of the documentation gate.

### Design Patterns

- **Evidence before prose**: Every guarantee, limit, and degraded mode maps to code,
  manifest, script, or named test evidence.
- **Layered operator documentation**: Main guides explain topology and route readers to
  specialized migration, recovery, lifecycle, export, and erasure contracts.
- **Fail-closed examples**: Commands distinguish read-only verification from guarded
  mutation and never normalize production experimentation.
- **Source-contract testing**: Stable path, link, token, and prohibited-claim checks
  make high-risk documentation drift visible in the normal test suite.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `tests/async/test_documentation_contract.py` | Verify links, paths, commands, configuration, safe-operation language, diagrams, and stale-claim exclusions. | ~220 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `README.md` | Correct architecture, setup, safety, and operator navigation summary. | ~70 |
| `docs/README.md` | Index all integrated persistence and operator contracts. | ~20 |
| `docs/ARCHITECTURE.md` | Trace boot, game-thread, typed worker, journal, recovery, and lifecycle topology. | ~150 |
| `docs/DATABASE.md` | Correct connection, read/write, transaction, ownership, migration, and compatibility routes. | ~180 |
| `docs/CONFIGURATION.md` | State actual precedence, required variables, deadlines, transport, and safe roles. | ~80 |
| `docs/RUNBOOK.md` | Add exact diagnose, recovery, migration, reconciliation, lifecycle, and rollback procedures. | ~180 |
| `docs/TESTING.md` | Map focused, disposable DB, workload, fault, privacy, and full gates to commands. | ~90 |
| `docs/diagrams/duris-server-architecture.html` | Replace legacy raw-worker/Redis-dirty-save topology with the integrated server flow. | ~210 |
| `docs/diagrams/duris-database-model.html` | Show authoritative revisions, operations, ownership, migration, and lifecycle data groups. | ~220 |

---

## 7. Success Criteria

### Functional Requirements

- [ ] No main guide or diagram claims full player/object/ship persistence flows through
  the legacy three raw SQL event workers or that Redis owns dirty-player durability.
- [ ] Login, checkpoint, journal, critical command, current-owner, inbox/outbox,
  maintenance, archive, migration, compatibility, and recovery boundaries match source
  and name their focused verification entry points.
- [ ] Configuration guidance states actual environment precedence, required role and
  target checks, transport/session invariants, timeouts, and safe local behavior.
- [ ] Every schema or destructive procedure states backup/clone/dry-run/qualification
  safeguards and does not direct validation at production.
- [ ] Lifecycle and privacy guidance distinguishes implemented engineering controls,
  disabled pending-policy mutation, controller decisions, and legal limitations.
- [ ] Server and database diagrams accurately represent the integrated Phase 03
  topology and have accessible text descriptions.

### Testing Requirements

- [ ] `python3 tests/async/test_documentation_contract.py` passes.
- [ ] Existing documentation-adjacent focused tests and `make test-all` pass.

### Non-Functional Requirements

- [ ] Documentation includes no credential, private key, IP address, player/account
  record, generated load artifact, or unsupported 200-player readiness claim.
- [ ] Documented commands and repository-relative paths exist and are copyable under
  their stated safety prerequisites.

### Quality Gates

- [ ] All added content is ASCII with Unix LF line endings.
- [ ] Markdown links and local anchors resolve.
- [ ] HTML diagrams remain standalone, accessible, and free of stale topology claims.
- [ ] Changes follow repository documentation and safety conventions.

---

## 8. Implementation Notes

### Working Assumptions

- The repository-native diagram source is the checked-in standalone HTML/SVG file:
  `docs/diagrams/` contains no separate generator, and the documentation index links
  directly to those files, so they should be edited in place and validated as source.
- Controller approval remains absent where the lifecycle manifest says `pending`:
  Sessions 07 through 10 deliberately keep canonical archive/export/erasure mutation
  disabled, so documentation must explain the usable inspect/evidence paths without
  presenting pending policy as approved.

### Conflict Resolutions

- `docs/DATABASE.md` broadly says player/object/ship saves use the three workers, while
  implemented Phase 01/02 code and specialized docs use typed snapshot and critical
  command pipelines. The traced implementation wins; raw event workers are documented
  only for their remaining bounded compatibility roles.
- The legacy diagrams label Redis as owning dirty saves and show one raw persistence
  queue as the primary route. The implemented journal, typed coordinators, MySQL
  current rows/ledgers, and optional Redis cache/recovery roles replace that topology.

### Key Considerations

- Preserve explicit pending-policy and unqualified-capacity limitations.
- Prefer links to specialized docs over duplicating long procedures inconsistently.
- Do not execute configured database commands merely to prove that documentation names
  them; source-contract tests and existing disposable wrappers are the safe evidence.

### Potential Challenges

- **Wide implementation surface**: Use source/tests/manifests as a claim matrix and
  fail the new contract test when a guide names an absent or stale route.
- **Command safety context**: Test that mutation-capable examples sit beside explicit
  development-clone, backup, dry-run, or production-prohibition language.
- **Diagram density**: Group typed domains semantically while keeping accessibility
  descriptions precise enough to convey omitted detail.

### Relevant Considerations

- [P00] **Trace code before trusting architecture prose**: The known worker mismatch is
  the primary correction target.
- [P00] **Capacity evidence is not representative**: Documentation must defer readiness
  until Session 14 qualifies representative data and all workload/fault profiles.
- [P00] **Operational choices remain open**: Do not invent RPO, production transport,
  archive, alert-routing, or legal-policy decisions.
- [P00] **MySQL/MariaDB is the durable authority**: Redis is documented only for
  reconstructible cache and bounded recovery roles.

---

## 9. Testing Strategy

### Unit Tests

- Parse the maintained Markdown and HTML set for required and prohibited contract
  language, environment variables, command entry points, and accessible diagram text.
- Resolve repository-relative paths, Markdown links, and local anchors without network
  access or private configuration.

### Integration Tests

- Run existing source-contract tests for connection trust, runtime compatibility,
  immutable migrations, maintenance, lifecycle, export, erasure, load, and critical
  command routes referenced by the updated guides.

### Runtime Verification

- Run `make test-all`; do not start the configured server or access a configured
  database because this session changes documentation only.

### Edge Cases

- Links containing fragments, spaces, parent-directory traversal, and standalone HTML
  targets resolve correctly.
- Command examples using placeholders are not mistaken for executable literal paths.
- Historical/attic documents are excluded from current-contract stale-claim checks.
- Pending policy is not phrased as approved or enabled canonical mutation.

---

## 10. Dependencies

### Other Sessions

- Depends on: Phase 03 Sessions 01 through 12 and all completed earlier phases.
- Depended by: `phase03-session14-final-200-player-and-compliance-gate`.

---

## Next Steps

Run the `implement` workflow step to begin implementation.
