# Documentation Index

Setup and first boot live in the root [README](../README.md); its Quick start is the
onboarding path. This directory holds the verified development, architecture,
operations, database, and builder references.

```
docs/
  reference/     how the server works          persistence/  durability and data lifecycle
  guides/        daily development             operations/   running and operating it
  content/       builders and world content    gates/        release gates
  records/       standing records              adr/          decision records
  diagrams/      architecture diagrams         assets/       images
  ongoing-projects/  in-flight notes           legacy/       inherited upstream text
  lib/           runtime game data (not documentation)
```

## reference/ - how the server works

| Document | Purpose |
|----------|---------|
| [ARCHITECTURE.md](reference/ARCHITECTURE.md) | Process model, boot gate, game loop, typed persistence, recovery, and networking. |
| [CODEBASE.md](reference/CODEBASE.md) | Module-by-module map of the server sources. |
| [DATABASE.md](reference/DATABASE.md) | Database authority, typed reads/writes, schema, reconciliation, and migrations. |
| [EVENTS.md](reference/EVENTS.md) | The `nevent` deferred-work scheduler: the timer wheel, scheduling, cancellation, the per-pulse budget, and catch-up. |
| [api/health.md](reference/api/health.md) | The health endpoint contract. |
| [api/durisweb.md](reference/api/durisweb.md) | DurisWeb transport, challenge authentication, authorization, and privacy contract. |

## persistence/ - durability, lifecycle, and privacy

| Document | Purpose |
|----------|---------|
| [PLAYER_SAVE_PIPELINE.md](persistence/PLAYER_SAVE_PIPELINE.md) | Revisioned checkpoint coordinator and completion boundary. |
| [PLAYER_SAVE_JOURNAL.md](persistence/PLAYER_SAVE_JOURNAL.md) | Journal permissions, bounds, replay, and diagnostics. |
| [WORLD_RECOVERY_PIPELINE.md](persistence/WORLD_RECOVERY_PIPELINE.md) | Immutable world generations and exact acknowledgement. |
| [CRITICAL_COMMAND_PIPELINE.md](persistence/CRITICAL_COMMAND_PIPELINE.md) | Operation identity, transaction, journal, outbox, replay, and fences. |
| [IMMUTABLE_MIGRATIONS.md](persistence/IMMUTABLE_MIGRATIONS.md) | Honest baseline adoption and checksummed ordered migration history. |
| [RUNTIME_COMPATIBILITY.md](persistence/RUNTIME_COMPATIBILITY.md) | Pre-write schema verification and atomic lookup publication. |
| [DATA_LIFECYCLE.md](persistence/DATA_LIFECYCLE.md) | Complete store inventory and pending-policy boundary. |
| [LIFECYCLE_ARCHIVE.md](persistence/LIFECYCLE_ARCHIVE.md) | Bounded archive state machine and disabled canonical scheduler. |
| [PERSONAL_DATA_EXPORT.md](persistence/PERSONAL_DATA_EXPORT.md) | Authenticated package contract and pending activation. |
| [ACCOUNT_ERASURE.md](persistence/ACCOUNT_ERASURE.md) | Erasure/tombstone contract and restore-time no-resurrection gate. |

## guides/ - daily development

| Document | Purpose |
|----------|---------|
| [BUILDING.md](guides/BUILDING.md) | Build entry points, compile flags, warning profile, sanitizers, and area generation. |
| [TESTING.md](guides/TESTING.md) | Focused, full, isolated-database, workload, fault, and privacy evidence boundaries. |
| [CONVENTIONS.md](guides/CONVENTIONS.md) | Repository conventions and their precedence against `AGENTS.md`. |
| [formatting.md](guides/formatting.md) | Style, changed-line formatting, and editor setup. |
| [MEMORY_CHECKING.md](guides/MEMORY_CHECKING.md) | Sanitizer and leak-checking workflow. |
| [valgrind.md](guides/valgrind.md) | Valgrind invocation, suppressions, and interpretation. |
| [VERSIONING.md](guides/VERSIONING.md) | Semantic versioning and the canonical version marker. |

## operations/ - running and operating it

| Document | Purpose |
|----------|---------|
| [RUNBOOK.md](operations/RUNBOOK.md) | Safe startup, migration, backup, restore, recovery, reconciliation, and the release boundary. |
| [CONFIGURATION.md](operations/CONFIGURATION.md) | Runtime variables, Redis, listeners, proxy handling, and diagnostics. |
| [SECURITY_BASELINE.md](operations/SECURITY_BASELINE.md) | Generated dependency baseline and its validation. |
| [incident-response.md](operations/incident-response.md) | Incident handling procedure. |

## content/ - builders and world content

| Document | Purpose |
|----------|---------|
| [HELP_SYSTEM.md](content/HELP_SYSTEM.md) | Help sources, database import, and rendering. |
| [HELP_STYLE_GUIDE.md](content/HELP_STYLE_GUIDE.md) | House style for help entries. |
| [AREA_OBJECT_FORMAT.md](content/AREA_OBJECT_FORMAT.md) | Area object file format and bitvector compatibility. |
| [STUDIOPROC.md](content/STUDIOPROC.md) | Studio proc design and the reasoning behind it. |
| [classes_and_races.txt](content/classes_and_races.txt) | Class and race reference table. |

## gates/ - release gates

Executable gates and required failure behavior. These are enforced by tests and
cited by the runbook; they are contracts, not historical evidence.

| Document | Purpose |
|----------|---------|
| [PHASE03_READINESS.md](gates/PHASE03_READINESS.md) | Strict integrated capacity/fault gate and explicit deferred-run non-claim. |
| [PHASE02_DOMAIN_GATE.md](gates/PHASE02_DOMAIN_GATE.md) | Transactional domain gate for bounded, schema-versioned critical commands. |
| [PHASE02_CRASH_MATRIX.md](gates/PHASE02_CRASH_MATRIX.md) | Required crash and replay behavior for every critical gameplay domain. |

## records/ - standing records

Kept when the `.spec_system/` tracking tree was retired.

| Document | Purpose |
|----------|---------|
| [CONSIDERATIONS.md](records/CONSIDERATIONS.md) | Institutional memory carried forward between phases. |
| [SECURITY-COMPLIANCE.md](records/SECURITY-COMPLIANCE.md) | Cumulative security posture and GDPR compliance record. |
| [readiness-report.md](records/readiness-report.md) | Phase 03 final readiness result and the deferred capacity gate. |

## Decisions and diagrams

- [Architecture decision template](adr/0000-template.md)
- [Server architecture diagram](diagrams/duris-server-architecture.html) and
  [database model](diagrams/duris-database-model.html)

## Unmaintained and non-documentation trees

- [ongoing-projects/](ongoing-projects/) holds in-flight investigation notes. They are
  not contract documentation and may be ahead of or behind the code.
- `legacy/` is inherited upstream reference text (`legacy/areas/`, `legacy/src/`) and
  may be stale.
- `lib/` is **not documentation**. `lib/information/` is read by the server at runtime
  (`src/wikihelp.c`, `src/nanny.c`) and by `scripts/import_help_to_prod.sh`; moving it
  breaks the running game.
