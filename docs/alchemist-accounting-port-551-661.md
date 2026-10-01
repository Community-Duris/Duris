# Alchemy delivery on experimental-accounting

This is the deliberate branch port of master PR573 and PR663. It preserves the
accounting branch's existing authority contracts and implements the retained
craft conservation and virtual NPC combat behavior while accounting is inactive.

## Persisted identities

| Contract | Accounting branch value |
| --- | --- |
| Craft reason | 34; existing wear29 through quest turn-in33 remain stable |
| Item payload | Version9; existing source8 and collector7 layouts preserved |
| Runtime-state migration | 0051_player_item_runtime_state |
| Canonical migration head | 51 |
| Staging0045 history head | 51; earlier staging history remains unchanged |
| Registered runtime tables | 224 |

Only the unpublished version7 same-player craft shape using draft reason27 or
master reason29 normalizes to craft34. Version9 wear29 retains its existing
meaning. Compatibility tests exercise both old draft reasons and refuse invalid
modern shapes.

SQL item load keeps canonical item_properties, equipment slot and account item
references authoritative while restoring the newly persisted rich item state.
The existing accounted materialization and continuation fields remain intact.
The moved combat scheduler uses attack_cadence.c for the post-callback actor and
victim room check.

## Active authority behavior

| Route | Behavior while accounting is active | Requirement before enabling |
| --- | --- | --- |
| Assassin poison mixing | Refuse at gameplay entry before wait, recipe RNG or allocation | Native craft root with exact ingredient/output references |
| Encrust | Refuse at gameplay entry before RNG or allocation | Native craft root, including zero-output failure |
| Harvester shard exchange | Retain branch's existing early refusal | Native exchange root and selected shard/output references |
| Craft submit API | Refuse before capturing/submitting output identities | Schema2 source/admission and exact retained result |
| Automatic NPC vial | Refuse before RNG, decision marker or allocation | Durable zone reset-generation/spawn source owns choice and exact UID |
| Virtual NPC mixture combat | Enabled through existing spell lifecycle | Existing lifecycle restrictions continue to apply |

The once-per-spawn runtime marker is sufficient for inactive gameplay and
recovery hooks; it is not a durable accounting source. This delivery does not
invent a source from a process-local mobile ID. The registry retires old potion
stock and player potion writers, retains shared poison/Encrust/Harvester routes,
and records the new active refusals. Accounting activation remains blocked by
the wider branch's unfinished writer qualification.

## Migration workflow

Use the normal migration runner with the deployment's local configuration:

```sh
python3 scripts/migration_runner.py inspect
python3 scripts/migration_runner.py run
```

For a newly bootstrapped disposable database, first adopt the bootstrap with
`adopt --kind fresh_bootstrap`. Existing deployments retain their history and
must use their existing adoption status. Do not re-adopt an existing authority.

Migration0051 was applied and replayed through this normal workflow on disposable
MySQL8.0.46 and MariaDB10.11.14 databases. Runtime metadata fingerprints were
measured from each resulting schema and sealed in the runtime manifest/header.
No configured game database or production migration was run.

The configured local database listener at 127.0.0.1:13312 was unreachable during
qualification. Applying 0051 to that deployment remains a prerequisite before
booting its upgraded SQL server; the disposable database results do not imply
that the configured deployment has been migrated.

## Qualification

Both maintained server backends and the focused craft, runtime-state, ownership,
reason/version, item load, publication-retention, account identity/membership,
Redis copyover handoff and NPC ability fixtures pass. The NPC cadence fixture
measures 33.33-33.36% of the reference caster across 60 scenarios.

The accounting branch's native SQL harness passes on MySQL8.0.46 and
MariaDB10.11.14, exercising craft commit/refusal/replay, rich-state save/load and
the branch's existing active accounting cases. The real-server flat-file journey
passes theft, poison mixing, corpse loot, virtual mixture combat, Encrust,
Harvester, three copyovers and cold player reload. The real-server MySQL/Redis
journey also passes all of those operations, unchanged craft runtime payloads
and recovery of the depleted NPC without another vial roll. These journeys run
with accounting inactive;
they do not certify the intentionally refused active craft or vial routes.

The SQL journey exposed a player-load query budget that allowed only one quest
reward read although the repository performs separate obligation and XP
entitlement reads. The budget now explicitly allows those two bounded queries;
the native fixture verifies 31 queries for PID lookup and 32 for name lookup.
Snapshot validity and recovery checks remain required.

Concurrent staff grant/save qualification exposed inherited defect #664: a
newly admitted item can be absent from an older sealed snapshot, whose safe SQL
refusal quarantines the PID before recapture. The gameplay fixture awaits each
staff setup grant's checkpoint. Its serialized result does not qualify that
overlap; #664 owns a durable grant/save ordering repair. The existing custody and
quarantine guards remain intact.

The executable active-refusal fixture verifies the real poison/Encrust entry
guards and the Harvester/craft allocation boundaries. The NPC fixture separately
verifies active vial refusal before RNG, marking or allocation. Writer inventory
validation covers 864 routes and maps every one of the 2758 unique lexical
mutation sites; that census retains the branch's incomplete-coverage release
gate and does not certify the wider accounting program.
