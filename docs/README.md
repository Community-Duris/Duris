# Documentation Index

Documentation for the DurisMUD server. Setup instructions (installing dependencies,
creating databases, first boot) live in the root [README.md](../README.md); this
directory covers architecture, operations, and reference material.

## Guides

| Document | Purpose |
|----------|---------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | How the server works: process model, boot sequence, game loop, event wheel, persistence, networking. |
| [CODEBASE.md](CODEBASE.md) | Module map of `src/` — where to find what, and the key files for each subsystem. |
| [BUILDING.md](BUILDING.md) | Build system details: flags, targets, area-file generation, sanitizer builds. |
| [DATABASE.md](DATABASE.md) | Database layer: connection handling, async persistence, schema, migrations. |
| [RUNBOOK.md](RUNBOOK.md) | Day-to-day operations: starting/stopping, restart codes, logs, backups, crash recovery. |
| [TESTING.md](TESTING.md) | The regression/source-contract test harness in `tests/async/` and how to run it. |
| [valgrind.md](valgrind.md) | Running the server under Valgrind: `scripts/valgrind_mud.sh`, suppressions, what to expect. |
| [formatting.md](formatting.md) | The `.clang-format` style and the changed-lines and full-tree workflows in `scripts/format.sh`. |
| [HELP_SYSTEM.md](HELP_SYSTEM.md) | How the in-game help pipeline works, from source files to the `pages` table. |

## Existing topic docs

- [STUDIOPROC.md](STUDIOPROC.md) — design rationale for the studio-proc/trigger system
  (`areas/world.trg`). Builder-facing grammar reference: [`src/howto_trg.txt`](src/howto_trg.txt).
- [`src/howto_add.txt`](src/howto_add.txt), [`src/howto_trg.txt`](src/howto_trg.txt),
  [`src/howto_sql_win.txt`](src/howto_sql_win.txt) — legacy builder references.
- [help/HELP_STYLE_GUIDE.md](help/HELP_STYLE_GUIDE.md) — style rules for help entries.
- [`classes_and_races.txt`](classes_and_races.txt) — historical class/race notes.
- `ongoing-projects/` — post-mortems and investigation write-ups (crash analyses,
  event-loop hotspot work).

## Diagrams

Self-contained HTML figures under [`diagrams/`](diagrams/):

- [Server architecture](diagrams/duris-server-architecture.html) — process zones,
  game loop, persistence pipeline, durable state.
- [Database model](diagrams/duris-database-model.html) — core MySQL tables and
  their relationships, verified against
  `migrations/bootstrap_multithread_safe.sql`.

## Historical/archival material

`areas/`, `attic/`, and `lib/misc/` under this directory hold legacy documentation
moved out of the live tree (old DE editor docs, licenses, revision notes). They are
retained for history; content may be badly outdated.
