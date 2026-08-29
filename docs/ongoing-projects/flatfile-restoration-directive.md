# Flat-file restoration project directive

**Effective date:** 2026-08-28
**Status:** authoritative direction for all remaining flat-file work

## Completed work

Completed implementation history is maintained in
[`flatfile-changelog.md`](./flatfile-changelog.md).

## Current progress

As of 2026-08-29, checkpoint 99 restores the six historical follower-raising paths in
flat-primary mode: ordinary undead, titans, dracoliches, golems, avatars, and greater
dracoliches. Each path now stops after constructing its off-world follower but before
publishing the follower, cloning a backup corpse, moving contents, or extracting the
corpse. One final `raise_follower` lifecycle action atomically removes the corpse, moves
its exact nested item graph and artifacts to the caster's existing player authority,
adds corpse money to the caster wallet, and records restart materialization. Only its
durable acknowledgment publishes the follower, moves the live contents, applies the
historical control or hostility result, and checkpoints the caster and pets. MariaDB
modes and NPC-caster behavior retain the synchronous historical path.

The action reuses the existing world-item, player-item, wallet, artifact,
materialization, and operation-ledger after-images under the shared authority lock. It
does not add pet identity storage, a new catalog, or a general spell callback system.
Forced interruption recovers and replays the same result exactly once. A crash after
the commit but before live follower publication safely materializes the items for the
caster on restart, while nested money objects are discarded from the live item graph
because their value was already credited atomically.

The remaining audited corpse path is decay of a corpse nested inside another object,
which must retain the containing topology. Historical mixed-format siege-state
compatibility also remains to be restored. The earlier siege/kingdom removal memo
remains research only and does not amend this directive's explicit siege requirement.

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

## Mandatory work order

The order of work is part of this directive:

1. Inventory the behavior and coverage of the historical flat-file implementation at
   the reference revision.
2. Restore every required historical flat-file path and prove that the live server uses
   it.
3. Only after that restoration is complete, fill the database-only gaps required for
   full no-database operation.

A database-only gap must not displace unresolved historical restoration work unless the
owner explicitly changes that priority.

## Persistence terms and authority

"Flat-file-only" means that the selected server build and runtime require no
MySQL/MariaDB server, schema, client library, headers, connection, or SQL-backed
persistence service. Redis and local handoff journals are not substitutes for complete
flat-file authority.

"Fallback" means a complete and current flat-file authority capable of taking over the
whole server at a known recovery point. It does not mean writing an isolated pfile after
an individual SQL save fails. Backend selection and authority transfer must be explicit
and observable; one gameplay operation must not mix MariaDB and flat-file authority or
create divergent histories.

In flat-file-primary mode, every required durable read and write uses the flat backend.
It is not a reduced-feature mode. MariaDB-backed mode must retain its existing behavior.

## Historical reference boundary

The behavioral reference is `97a4166c3fa10448b778a35e16854ad5b3e5e294`, the final
revision before the player-pfile migration began; its direct child `35f66dfc` started
that migration. SQL existed before this boundary, so the reference is the historical
player/account flat-file era, not a fully database-free release.

Historically file-backed behavior included accounts, player state and objects, locker
contents, corpses, saved world items, shopkeepers, guild core state, towns, siege state,
ships, crafting recipes, shapechange, mail, boards, and administrative state. Important
areas already dependent on SQL included alliances, outposts, nexus stones, ship cargo
markets, locker/private-chest access metadata, artifacts, and other economy records.
Those two groups define the restoration-first versus later gap-filling distinction.

Known historical defects are evidence, not behavior to reproduce. In particular, the
old pet path was already incomplete, native-layout pfiles were platform-dependent,
multi-file writes lacked atomicity, and terminal character saves could destroy live
inventory before durable publication. Historical readers may be retained as bounded
import or salvage paths, but the unsafe historical writers are not the required design.

## Scope authority

This directive supersedes conflicting scope, design, implementation-order, and "next
action" statements in the changelog and other project notes. The
[`flatfile-changelog.md`](./flatfile-changelog.md) may be used as implementation history
and technical evidence, but it does not authorize additional scope.

The historical flat-file implementation at the comparison point identified above
(`97a4166c3fa10448b778a35e16854ad5b3e5e294`) is the behavioral reference.
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

Do not undertake any of the following:

- redesigning the persistence system again;
- adding speculative architecture, generalized frameworks, or future-proofing;
- creating new requirements merely because the current architecture makes them
  possible;
- broad refactors that are not necessary for a specific restoration or safety gap;
- replacing known historical behavior with a theoretically cleaner product design;
- turning each remaining gap into a new subsystem, transaction framework, catalog, or
  multi-checkpoint project when a narrow repair is sufficient;
- continuing work solely because a historical note or changelog entry lists it as a
  planned phase or next action.

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

## Enduring safety and correctness requirements

- The client-free build must compile and link without MySQL/MariaDB headers or client
  libraries, and flat-file-primary runtime must not consult MariaDB.
- A required no-database path must never report success for a discarded write or return
  fabricated empty state that changes gameplay. Missing implementations remain explicit
  boot blockers.
- Authoritative records must have fixed bounds and explicit versions, validate their
  complete content before publication, use private owned paths, and reject unsafe files,
  permissions, and symlinks. Corrupt authority must fail closed and must not be
  overwritten by an ordinary mutation.
- Durable replacement must use the repository's safe same-directory publication
  primitives, including complete writes, file synchronization, atomic rename, and
  parent-directory synchronization where applicable.
- Durable identities and revisions must be stable. Operations spanning multiple owners
  or domains must retain the existing atomicity and idempotency semantics so a crash or
  retry cannot lose or duplicate items, currency, identity, or membership.
- Terminal or destructive live mutation must occur only after the corresponding durable
  publication succeeds.
- Flat backups must cover the complete selected authority, including indexes,
  allocators, operations, and every domain, and must be restorable into an empty state
  root. Backend selection—not an unrelated service flag—controls backup behavior.

These are constraints on required work, not authorization to add another generalized
framework. Reuse the safe primitives already present whenever they are sufficient.

## Completion validation

Completion requires focused tests for each changed path plus end-to-end evidence that:

- a client-free build can boot, shut down, copy over, and restart with MariaDB absent;
- account and character lifecycle, complete player state, items, currency, and required
  world/domain state survive restart in flat-file-primary mode;
- required cross-owner operations survive retries and forced interruption without loss
  or duplication;
- corrupt, truncated, oversized, stale, permission-unsafe, symlinked, and failed-write
  authority is rejected safely where applicable;
- a complete flat backup restores into an empty root and cold-boots consistently; and
- the same affected gameplay still works in MariaDB-backed mode.

Where a historical importer remains supported, its accepted versions and corruption
rejection must have focused compatibility tests.

## Completion standard

The project is complete when the required previous flat-file behavior is restored, every
database-only system needed for normal server operation has a working flat-file
counterpart, the server can operate fully in both database-backed and flat-file-only
modes, and the affected paths have focused validation. Flat-file-only operation must not
silently disable required gameplay or persistence behavior merely to boot without a
database. Completion does not require an idealized persistence platform, exhaustive
architectural abstraction, or implementation of speculative future capabilities.
