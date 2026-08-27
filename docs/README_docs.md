# Documentation Index

Setup and first boot live in the root [README](../README.md). This directory contains
the verified development, architecture, operations, database, and builder references.

## Start Here

| Document | Purpose |
|----------|---------|
| [onboarding.md](onboarding.md) | Setup and first verification checklist. |
| [development.md](development.md) | Daily build, format, test, security, and start commands. |
| [environments.md](environments.md) | Local and network-deployment trust boundaries. |
| [deployment.md](deployment.md) | Repository-owned CI, local probes, and external release boundary. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Process model, boot gate, game loop, typed persistence, recovery, and networking. |
| [EVENTS.md](EVENTS.md) | The `nevent` deferred-work scheduler: the timer wheel, scheduling, cancellation, the per-pulse budget, and catch-up. |
| [DATABASE.md](DATABASE.md) | Database authority, typed reads/writes, schema, reconciliation, and migrations. |
| [CONFIGURATION.md](CONFIGURATION.md) | Runtime variables, Redis, listeners, proxy handling, and diagnostics. |
| [RUNBOOK.md](RUNBOOK.md) | Safe startup, migration, backup, restore, recovery, and reconciliation procedures. |
| [TESTING.md](TESTING.md) | Focused, full, isolated-database, workload, fault, and privacy evidence boundaries. |

## Persistence and Lifecycle

| Document | Purpose |
|----------|---------|
| [PLAYER_SAVE_PIPELINE.md](PLAYER_SAVE_PIPELINE.md) | Revisioned checkpoint coordinator and completion boundary. |
| [PLAYER_SAVE_JOURNAL.md](PLAYER_SAVE_JOURNAL.md) | Journal permissions, bounds, replay, and diagnostics. |
| [WORLD_RECOVERY_PIPELINE.md](WORLD_RECOVERY_PIPELINE.md) | Immutable world generations and exact acknowledgement. |
| [CRITICAL_COMMAND_PIPELINE.md](CRITICAL_COMMAND_PIPELINE.md) | Operation identity, transaction, journal, outbox, replay, and fences. |
| [IMMUTABLE_MIGRATIONS.md](IMMUTABLE_MIGRATIONS.md) | Honest baseline adoption and checksummed ordered migration history. |
| [RUNTIME_COMPATIBILITY.md](RUNTIME_COMPATIBILITY.md) | Pre-write schema verification and atomic lookup publication. |
| [DATA_LIFECYCLE.md](DATA_LIFECYCLE.md) | Complete store inventory and pending-policy boundary. |
| [LIFECYCLE_ARCHIVE.md](LIFECYCLE_ARCHIVE.md) | Bounded archive state machine and disabled canonical scheduler. |
| [PERSONAL_DATA_EXPORT.md](PERSONAL_DATA_EXPORT.md) | Authenticated package contract and pending activation. |
| [ACCOUNT_ERASURE.md](ACCOUNT_ERASURE.md) | Erasure/tombstone contract and restore-time no-resurrection gate. |
| [PHASE03_READINESS.md](PHASE03_READINESS.md) | Strict integrated capacity/fault gate and explicit deferred-run non-claim. |

## APIs, Decisions, and Incidents

- [Health endpoint](api/health.md)
- [Architecture decision template](adr/0000-template.md)
- [Incident response](runbooks/incident-response.md)
- [Security baseline](SECURITY_BASELINE.md)

## Build and Content References

- [Building](BUILDING.md), [Codebase](CODEBASE.md), [Formatting](formatting.md),
  [Memory checking](MEMORY_CHECKING.md), and [Valgrind](valgrind.md)
- [Help system](HELP_SYSTEM.md), [help style](help/HELP_STYLE_GUIDE.md),
  [area object format](AREA_OBJECT_FORMAT.md), and [studio procs](STUDIOPROC.md)
- [Server architecture diagram](diagrams/duris-server-architecture.html) and
  [database model](diagrams/duris-database-model.html)

## Project Records

Standing references and Phase 00-03 evidence, moved here when the `.spec_system/`
tracking tree was retired.

| Document | Purpose |
|----------|---------|
| [CONVENTIONS.md](CONVENTIONS.md) | Repository conventions and their precedence against `AGENTS.md`. |
| [CONSIDERATIONS.md](CONSIDERATIONS.md) | Institutional memory carried forward between phases. |
| [SECURITY-COMPLIANCE.md](SECURITY-COMPLIANCE.md) | Cumulative security posture and GDPR compliance record. |
| [readiness-report.md](readiness-report.md) | Phase 03 final readiness result and the deferred capacity gate. |
| [query-plan-gate-report.md](query-plan-gate-report.md) | Query plan and index gate run against a non-production clone. |
| [maintenance-inventory.md](maintenance-inventory.md) | Recurring maintenance activities and their bounded-worker disposition. |
| [archive-inventory.md](archive-inventory.md) | Archive candidates and why no destructive rule is approved. |
| [docs-audit.md](docs-audit.md) | Phase 03 documentation audit, its gaps, and its evidence ledger. |

Material under `attic/`, `areas/`, and `lib/misc/` is historical and may be stale.
