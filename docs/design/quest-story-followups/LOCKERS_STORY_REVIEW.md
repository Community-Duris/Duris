# Adventurer's Vaults (lockers): source and storage-system review

**Scope:** source review of the 99 WLD room placeholders, zone header, empty
mobile/object files, active command bindings, compiled room/object handlers,
and locker persistence schema. This is a shared storage-system area. It contains
no authored resident story or local quest objective; its operation is supplied
by code and persisted locker identities.

## Authored area inventory

The active area list includes lockers at *652 (player lockers)
([AREA entry](../../../areas/AREA#L246)). The zone title is Adventurer's Vaults
([zone header](../../../areas/zon/lockers.zon#L1)). Its WLD file has exactly 99
rooms, #65201–#65299. Every room is named “Inside a Locker,” has an empty room
description, the same raw room fields, and no authored exits
([first record](../../../areas/wld/lockers.wld#L1),
[last record](../../../areas/wld/lockers.wld#L490)). The MOB and OBJ files are
empty. The ZON file contains its zone header and no reset commands
([ZON header](../../../areas/zon/lockers.zon#L3)); there are no local shops.

The QST area index lists lockers ([QST AREA entry](../../../areas/qst/AREA#L221)),
while there is no areas/qst/lockers.qst file. There is no story sidecar and
the coverage row records zero native Q/QA contracts. The area files therefore
describe reusable room shells; they do not supply a player-facing narrative
or quest contract.

## Runtime storage behavior and static code bindings

The storage code reserves VNUM #65201 as its base location and declares a
maximum of 99. The allocator starts at #65202 and scans through #65299, giving
98 reusable private-room slots while #65201 remains the base/staging room
([range constants](../../../src/item/storage_lockers.c#L85),
[allocator](../../../src/item/storage_lockers.c#L2927),
[base placement](../../../src/item/storage_lockers.c#L3297)). On allocation,
the code sets room name and description, assigns storage_locker, installs the
locker room flags, and creates a personalized exit back to the player's entry
room ([room setup and exit](../../../src/item/storage_lockers.c#L3001)).
These dynamic exits do not appear in the static WLD census.

Locker entry is routed by enter locker on three compiled bank-counter object
handlers: Tharnadia object #132581, Verz1 object #3097, and Winterhaven object
#55008 ([assignments](../../../src/specs/specs.assign.c#L1307),
[Tharnadia counter](../../../areas/obj/tharnadia.obj#L1373),
[Verz1 counter](../../../areas/obj/verz1.obj#L1199),
[Winterhaven counter](../../../areas/obj/wh.obj#L99)). A guildhall bank room
also routes CMD_ENTER to guild_locker_room_hook after checking the
guildhall association ([guildhall hook](../../../src/guild/guildhall_procs.c#L341),
[locker room hook](../../../src/item/storage_lockers.c#L2037)). The main entry
hook applies account, player-access, guild, racewar, location, and character
state checks ([entry hook](../../../src/item/storage_lockers.c#L1799)).
These are static code findings; live access and return behavior were not
exercised.

The room allocator checks availability through a numeric band in world[].
In boot_world, room_nr starts at zero and advances once per loaded WLD record;
the loader later uses it as the world[room_nr] array index. The boot-time
locker special case tests room_nr against 65201–65300 and sets the locker
flag/procedure on those array entries
([loader loop](../../../src/world/db.c#L1173),
[boot-time assignment](../../../src/world/db.c#L1425)). The allocator instead
resolves the locker VNUM with real_room0(LOCKERS_START + 1). The intended
mapping from those real-room indices to locker VNUMs requires confirmation
from the storage-code owner before any code or VNUM change.

Locker identities and stored items use SQL persistence. The storage code loads
and saves locker characters and item rows; the tracked locker schema defines
locker identity and item/affect tables
([storage load path](../../../src/item/storage_lockers.c#L3100),
[SQL locker schema](../../../migrations/schema_migration_v3_lockers.sql#L8)).
Historical migrations and current database contents were not run or inspected.

## Static connectivity and dependency census

The authored graph has 99 rooms, zero WLD exits, and 99 singleton weak
components. It has no active inbound or outbound WLD boundary and no
unregistered-WLD approach or room overlap in the all-95-source sweep. The
coverage row records no fall or current settings. At runtime, the locker
service creates a personalized exit and accesses shared storage through its
compiled entry hooks; this behavior is separate from the authored graph.

The exact source scan finds no outside ZON, SHP, QST, or story item reference
into the locker VNUM range. The three bank-counter object assignments, the
guildhall hook, the fixed room band in storage code, and the SQL persistence
schema are the mapped dependencies. The active title Adventurer's Vaults is
unique in the normalized title list. Tracked path history records a move involving the areas/ and mud/areas roots;
no predecessor or replacement area is established.

## Builder and storage-owner questions

| # | Question | Evidence | Recommended resolution |
| --- | --- | --- | --- |
| 1 | Are the 99 identical, blank room records intended solely as reusable locker shells? | [WLD records](../../../areas/wld/lockers.wld#L1) | Preserve all room VNUMs as system support; add no environmental story without an authored requirement. |
| 2 | Is the QST index entry intentional when no local QST file, story sidecar, or native contract exists? | [QST index](../../../areas/qst/AREA#L221), [coverage row](ZONE_COVERAGE_REGISTER.csv#L163) | Keep the index and area coverage aligned with the storage-only role; add no quest claim or contract by inference. |
| 3 | Is a mode-2 zone header with no reset commands deliberate? | [zone header](../../../areas/zon/lockers.zon#L3) | Keep the current empty reset list unless a storage owner identifies a required reset. |
| 4 | What does raw room flag value 32768 represent for these placeholders, and should it remain on all 99 WLD records? | [first room record](../../../areas/wld/lockers.wld#L1) | Confirm the flag's intended meaning against the current constants before editing authored fields. |
| 5 | Is #65201 intended as the shared base/staging room used while locker characters load? | [base placement](../../../src/item/storage_lockers.c#L3297) | Preserve #65201 for the storage code unless the code owner changes its explicit use. |
| 6 | Is the intended live capacity 98 concurrent private rooms for the 99 authored records? | [allocator range](../../../src/item/storage_lockers.c#L2935) | Confirm that #65201 is reserved and #65202–#65299 are the 98 allocatable slots. |
| 7 | Does the boot-time comparison 65201–65300 mean real-room array indices or virtual room VNUMs? | [world loader](../../../src/world/db.c#L1173), [boot assignment](../../../src/world/db.c#L1425), [VNUM allocator](../../../src/item/storage_lockers.c#L2935) | Have the storage-code owner map each checked index to world[index].number; align initialization only after the intended VNUMs are known. |
| 8 | Should enter locker continue to enter through the Tharnadia, Verz1, and Winterhaven counter objects? | [compiled assignments](../../../src/specs/specs.assign.c#L1307), [Winterhaven assignment](../../../src/specs/specs.assign.c#L2094) | Keep these handlers with their current object owners; update any entry point and compiled binding together. |
| 9 | Which location and player-state requirements should gate the shared bank-counter entry hook? | [entry hook](../../../src/item/storage_lockers.c#L1799) | Preserve the current checks until the system owner specifies a change; qualify live access separately. |
| 10 | Is the no-argument locker path intended to create or use the account locker keyed by account and racewar side? | [account naming](../../../src/item/storage_lockers.c#L1896) | Preserve account/racewar identity mapping and confirm any changed identity format with the persistence owner. |
| 11 | Should a named player locker require explicit access validation before entry? | [load and access checks](../../../src/item/storage_lockers.c#L3100) | Keep the current validation path and access records together. |
| 12 | Are guild lockers intentionally gated by membership, parole state, and the prestige.locker.required property? | [guild entry hook](../../../src/item/storage_lockers.c#L2037), [guildhall bank routing](../../../src/guild/guildhall_procs.c#L341) | Retain the existing guild checks unless the guild-system owner approves a specific policy change. |
| 13 | Is the guildhall room association check part of the intended entry boundary? | [guildhall association check](../../../src/guild/guildhall_procs.c#L357) | Keep guildhall validation before routing to the locker hook. |
| 14 | Are paid locker services and item-count fees intended to follow the current accounting gate and per-item calculation? | [accounting gate](../../../src/item/storage_lockers.c#L68), [item fee](../../../src/item/storage_lockers.c#L1968) | Preserve the current code-owned fee policy; ask the economy/storage owners before changing it. |
| 15 | Should each dynamically allocated room carry the current safe, no-magic, no-recall, no-teleport, no-summon, and no-heal flags? | [runtime room flags](../../../src/item/storage_lockers.c#L91) | Keep the shared flag set with the storage implementation and describe it as code behavior, not authored WLD traversal. |
| 16 | Is the personalized exit back to the exact entry room the intended return route? | [dynamic exit setup](../../../src/item/storage_lockers.c#L3027) | Preserve the origin-room return binding and have the owner qualify departure and interrupted-session handling. |
| 17 | Are private chest creation, access grants, activity logs, and their related item rows part of the intended locker service? | [chest command](../../../src/item/storage_lockers.c#L3829), [activity log command](../../../src/item/storage_lockers.c#L4169), [SQL item schema](../../../migrations/schema_migration_v3_lockers.sql#L26) | Keep chest and access data under the locker persistence owner; avoid changing storage identity or table ownership during area work. |
| 18 | Which tracked SQL tables and compatibility rules own player, account, and guild locker identities today? | [locker migration](../../../migrations/schema_migration_v3_lockers.sql#L8), [runtime table contract](../../../src/core/runtime_compatibility_contract.h#L16) | Preserve current tables and migrations; resolve table ownership before any broader storage change. |
| 19 | Should access through a guildhall hook remain separate from the three bank-counter object hooks? | [guildhall hook](../../../src/guild/guildhall_procs.c#L341), [object hooks](../../../src/specs/specs.assign.c#L1307) | Preserve both compiled paths with their current owners and keep their qualification distinct. |
| 20 | Is the active title and fixed VNUM band intended to remain as Adventurer's Vaults with no predecessor or replacement? | [title/history register](DUPLICATE_AND_REPLACED_ZONES.md), [coverage row](ZONE_COVERAGE_REGISTER.csv#L163) | Keep the active system area DO NOT MOVE; any VNUM relocation requires a separate code and persistence plan. |

## Safe disposition

Keep the active lockers contributor DO NOT MOVE. Preserve all 99 room
VNUMs, the 98-room runtime allocation band, the three counter hooks, the
guildhall route, SQL locker identities/items, and the storage-specific code
bindings together. Record the zone as system support with no authored local
quest or environmental narrative. The 20 questions capture code and builder
decisions that remain open; no source code, database, migration, or gameplay
state was changed or exercised.
