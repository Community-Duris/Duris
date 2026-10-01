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
| Craft submit API | Refuse before reserving output UIDs | Schema2 source/admission and exact retained result |
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
python3 scripts/migration_runner.py preflight
python3 scripts/migration_runner.py run
```

For a newly bootstrapped disposable database, first adopt the bootstrap with
`adopt --kind fresh_bootstrap`. Existing deployments retain their history and
must use their existing adoption status. Do not re-adopt an existing authority.

Migration0051 was applied and replayed through this normal workflow on disposable
MySQL8.0.46 and MariaDB10.11.14 databases. Runtime metadata fingerprints were
measured from each resulting schema and sealed in the runtime manifest/header.
No configured game database or production migration was run.

## Qualification

Both maintained server backends and the focused craft, runtime-state, ownership,
reason/version, item load, publication-retention, account identity/membership,
Redis copyover handoff and NPC ability fixtures pass. The NPC cadence fixture
measures 33.33-33.36% of the reference caster across 60 scenarios.

The master delivery has live gameplay proof on flat-file and MySQL/Redis.
Separate accounting branch SQL/backend and live qualification results are
recorded in the delivery PR; master evidence alone is not active accounting proof.
