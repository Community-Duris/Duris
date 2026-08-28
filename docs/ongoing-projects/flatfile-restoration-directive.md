# Flat-file restoration project directive

**Effective date:** 2026-08-28  
**Status:** authoritative direction for all remaining flat-file work

## Progress ledger

### 2026-08-29 - database-independent global timers

- **Concrete gap:** under `__NO_MYSQL__`, `set_timer` discarded every write and
  `get_timer` always returned zero. The database-backed `timers` table is the functional
  reference for these named date values; current callers use them for cargo maintenance,
  trophy reduction, and epic-zone timing.
- **Restoration:** flat-file-only mode now persists each named timer beneath the existing
  flat-file metadata authority. Records use the existing atomic file publication path,
  owner-only metadata validation, a bounded name, an explicit version, and a checksum.
  Database-backed mode remains unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_timers.py` covers missing,
  create, replace, negative-value compatibility, corruption, symlink, unsafe-name, and
  private-permission behavior. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent in-game help

- **Concrete gap:** `wiki_help` returned only a disabled message under `__NO_MYSQL__`,
  even though the help pages imported into the database are generated from tracked flat
  files in this repository.
- **Restoration:** flat-file-only mode now loads those existing sources directly and
  caches a bounded catalog. It follows the importer's existing precedence across the
  individual information files, `help_index`, and `duris_help_parsed.hlp`, then provides
  the established case-insensitive exact lookup, substring search, sorted results,
  result limit, missing-topic response, and redirects. Database-backed help remains
  unchanged.
- **Focused evidence:** `python3 tests/async/test_flatfile_help_catalog.py` loads more
  than 1,500 real topics, verifies source precedence and search behavior, and directly
  invokes `wiki_help` in a client-free runtime harness to prove that real content—not the
  former disabled stub—is returned. The test is included in the client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`, `./scripts/format.sh --check`,
  and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

### 2026-08-29 - database-independent launcher and pre-boot backup

- **Concrete gap:** `cycle_mud.sh` required database credentials and ran migrations,
  schema verification, and MySQL shutdown logging in every mode. Its pre-boot backup
  selected `mysqldump` from the unrelated `REDIS` switch, so a flat-file-only launch
  could not be operationally independent of database tools.
- **Restoration:** launcher validation and database operations now follow the selected
  persistence mode. `flatfile-primary` requires only its absolute state root, avoids
  migration/schema/MySQL paths, and snapshots that complete state root to an explicitly
  separate owner-only backup before boot. A failed or unsafe snapshot stops the launch.
  Existing database-backed and fallback paths retain their database requirements.
- **Focused evidence:** `python3 tests/async/test_flatfile_launcher.py` validates all
  three mode contracts and runs an isolated supervised flat-file cycle with `REDIS=1`.
  It proves the selected state is backed up, no database-only command path is entered,
  and an unsafe nested backup destination fails closed. The test is included in the
  client-free CI job.
- **Build evidence:** `make -C src -j2`,
  `python3 tests/async/test_flatfile_boot_preflight.py`,
  `bash -n scripts/cycle_mud.sh scripts/backup_pfiles.sh`,
  `./scripts/format.sh --check`, and `git diff --check` pass.
- **Overall state:** the full objective is not complete. The global incomplete-domain
  boot fence remains in place while other concrete DB-free gaps are restored.

## Owner intent

The purpose of this project is to restore the previous flat-file persistence system,
while making that restored system safer, more reliable, and better-quality where needed.
After restoration, the work may fill concrete functional gaps required for the flat-file
system to operate correctly.

The required end state is that the server can run fully in either of two independently
usable modes:

1. **Database-backed mode**, retaining the existing database-backed behavior.
2. **Flat-file-only mode**, requiring no database server, database client library,
   database connection, or database-backed persistence service at build time or runtime.

Some server systems were always database-backed and therefore have no historical
flat-file implementation to restore. Those systems are part of the required gap-filling
work: they must receive safe, focused flat-file persistence implementations sufficient
for the complete server to operate in flat-file-only mode.

The implementation already built on the `flatfiles` branch will be used as the starting
point. It is not an instruction to discard all current work and begin again. It is also
not a mandate to continue expanding the architecture that has been built.

## Scope authority

This directive supersedes conflicting scope, design, implementation-order, and "next
action" statements in
[`flatfile-persistence-assessment.md`](./flatfile-persistence-assessment.md) and other
ongoing project notes. Those documents may be used as implementation history and
technical evidence, but they do not authorize additional scope.

The historical flat-file implementation, including the comparison point identified in
the assessment (`97a4166c3fa10448b778a35e16854ad5b3e5e294`), is the behavioral reference.
Its behavior should be recovered deliberately rather than replaced with a newly imagined
persistence product. For a system that was historically database-only, its existing
database-backed behavior is the functional reference for the required flat-file
counterpart; this does not authorize unrelated redesign.

## Required remaining work

Remaining work must be limited to:

1. Reconstructing concrete behavior and coverage from the previous flat-file system.
2. Connecting the implementation already built to the actual server paths needed for
   that restored behavior.
3. Correcting safety, data-integrity, corruption-handling, bounds, error-handling, and
   code-quality problems that affect the restored behavior.
4. Filling specific, demonstrated gaps that prevent functional parity or correct
   operation.
5. Implementing focused flat-file counterparts for historically database-only systems
   that would otherwise prevent complete no-database operation.
6. Preserving working database-backed behavior and backend selection while adding the
   flat-file path.
7. Adding focused regression tests for restored or corrected behavior in both relevant
   modes.

Existing new code may be reused when it directly serves these requirements. It may be
simplified, corrected, or bypassed when that is the smallest safe way to restore the
required behavior.

## Explicitly out of scope

Do not undertake any of the following without the owner's explicit approval:

- redesigning the persistence system again;
- adding speculative architecture, generalized frameworks, or future-proofing;
- creating new requirements merely because the current architecture makes them
  possible;
- broad refactors that are not necessary for a specific restoration or safety gap;
- replacing known historical behavior with a theoretically cleaner product design;
- turning each remaining gap into a new subsystem, transaction framework, catalog, or
  multi-checkpoint project when a narrow repair is sufficient;
- continuing work solely because an earlier assessment lists it as a planned phase or
  next action.

## Working rule for every change

Before implementation, identify the exact historical behavior or concrete missing
server behavior being restored. For a historically database-only system, identify the
existing database behavior that the flat-file path must provide. Then make the smallest
safe change that provides it, preserve compatible code already built, and verify it with
the narrowest useful test.

If a proposed change cannot be tied to historical restoration, a demonstrated gap, or a
required safety correction, it is outside this project's scope. If the smallest safe
solution would materially expand the design, stop and obtain explicit owner approval
before proceeding.

## Completion standard

The project is complete when the required previous flat-file behavior is restored, every
database-only system needed for normal server operation has a working flat-file
counterpart, the server can operate fully in both database-backed and flat-file-only
modes, and the affected paths have focused validation. Flat-file-only operation must not
silently disable required gameplay or persistence behavior merely to boot without a
database. Completion does not require an idealized persistence platform, exhaustive
architectural abstraction, or implementation of speculative future capabilities.
