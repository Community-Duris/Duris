# Codebase Guide

A map of the server sources. Hundreds of `.c` files under `src/` are compiled
individually as C++20 and linked into `bin/server/dms_new`. The `src/` root
contains only the Makefile and repository metadata; implementation and headers
live in subsystem directories, and includes use paths qualified from `src/`
(for example, `#include "core/structs.h"`). File boundaries retain the DikuMUD
conventions: `act*.c` files group player commands, `do_<name>` functions are
commands, and `specs.*.c` files hold special procedures.

Source scope: publication onto `experimental-accounting` at implementation base
`626e338461cbd803d69b64cccb2d3849b01a5fff`, reviewed on October 9, 2026.
The architecture guide's
[newer branch implementations](ARCHITECTURE.md#newer-branch-implementations)
preserve the separately reviewed October 7 snapshots; the current publication
base already includes the transport, poll-based networking and account-load work
identified there. Documentation publication does not qualify combined runtime
behavior.

Related: [ARCHITECTURE.md](ARCHITECTURE.md), [DATABASE.md](DATABASE.md).

Quick navigation:

- [Source layout](#source-layout)
- [Persistence](#persistence)
- [Telemetry](#telemetry)
- [Dispatch signatures](#dispatch-signatures) and [C++ conventions](#c-conventions-the-warning-profile-enforces)
- [Indexing invariants](#indexing-invariants)
- [Gameplay feature references](#gameplay-feature-references)

## Source layout

| Directories | Responsibility |
| --- | --- |
| `account/`, `player/` | Account/login/creation flows and player load/save/materialization. |
| `classes/`, `magic/`, `combat/` | Class and skill rules, spells, affects, and combat. |
| `cmd/` | Command registration plus player, object, social, and staff handlers. |
| `core/` | Shared structures, constants, prototypes, utilities, configuration, and legacy pfile support. |
| `economy/`, `item/` | Shops, auctions, balances, crafting, equipment, lockers, and durable item movement. |
| `flatfile/`, `no_mysql/` | Selected flat-file repositories, authority transactions, and client-free build stubs; optional-feature limits are described below. |
| `guild/`, `kingdom/` | Guild/social ownership and the current map-territory kingdom feature. |
| `mob/`, `specs/` | Mobile behavior, studio procs, and area/special procedures. |
| `net/` | Telnet, TLS, WebSocket, GMCP, MCCP, prompts, and descriptor I/O. |
| `persistence/`, `sql/`, `redis/` | Typed durability coordinators, MariaDB repositories, and Redis integrations. |
| `ships/` | Naval simulation, dock economy, and player/NPC ship control. |
| `telemetry/` | Optional observation capture, bounded transport, and a private SQL writer; independent of gameplay mutation authority. |
| `world/` | Boot/loading, world lifecycle, movement, maps, events, quests, and global updates. |

## Core engine

| Files | Role |
|-------|------|
| `src/net/comm.c` | `main()`, `game_loop()` — bounded `poll()` network turns, simulation phases, worker wakeup hints, and lifecycle signals; persistent mode separates transport parent and world child. |
| `src/world/db.c` | Boot: loads world files and zone resets; core allocation helpers. |
| `src/cmd/interp.c` | Command table (`CMD_*` rows) and command dispatch/aliasing. |
| `src/world/handler.c` | Object/character lifecycle and generic character maintenance events. |
| `src/core/structs.h` | Central data structures (`char_data`, `obj_data`, `descriptor_data`, ...). |
| `src/core/prototypes.h` | Cross-subsystem function declarations. Prefer a subsystem header for new APIs. |
| `src/core/utility.c` | String/number helpers and logging (`logit`, `wizlog`). |
| `src/core/constant.c`, `src/core/utils.h` | Constants, tables, and accessor macros used throughout the tree. |

## Events, time, and triggers

| Files | Role |
|-------|------|
| `src/world/events.c`, `src/world/new_events.c` | Timed callback wheel executed inside the game loop. |
| `src/world/timers.c` | Persisted named timestamps and elapsed-time checks for periodic work, including ship cargo updates. |
| `src/magic/affects.c` | Affect application and expiry callbacks scheduled through the event system. |
| `src/mob/studioproc.c`, `src/mob/studioproc.h` | Studio-proc trigger engine for `areas/world.trg`; see [STUDIOPROC.md](../content/STUDIOPROC.md). |
| `src/mob/studioproclib.c`, `src/mob/studioproclib.h` | Built-in proc function library callable from triggers. |
| `src/persistence/latency_trace.c` | Per-callback latency telemetry (`NEVENT BUDGET`). |

## Account services and cached reads

| Files | Role |
| --- | --- |
| `src/account/account.c`, `src/account/nanny.c` | Account and connection flows, character selection, and player-load continuations. Account-name reads and some character-load fallbacks remain synchronous in this checkout. |
| `src/account/password_async.c`, `src/account/password_hash.c` | Game-thread authentication continuations and the password hashing/verification worker. |
| `src/account/account_recovery.c`, `src/account/account_recovery_nanny.c`, `src/net/mail_sender.c` | Recovery-token policy, Telnet recovery adapter, and optional best-effort SMTP worker. |
| `src/cmd/help_cache.c`, `src/cmd/information_cache.c` | Cached help/information reads and refresh publication; backend-specific local catalogs have their own load rules. |
| `src/economy/collector_catalog_cache.c`, `src/economy/collector_listing_pipeline.c` | Collector catalog refresh/publication and bounded listing reads; durable Collector mutations stay with their transaction owners. |

## Persistence

The backend is selected once at boot and must match the executable. The
supported modes are `mariadb-primary` and client-free `flatfile-primary`;
mixed per-operation fallback is rejected. These are representative ownership
entry points rather than a list of every domain repository.

| Files | Role |
|-------|------|
| `src/persistence/persistence_mode.c` | Whole-server backend selection, build compatibility, and private flat-file state-root validation. |
| `src/sql/sql.c`, `src/sql/sql.h` | Main MariaDB connection, target selection, boot schema checks, and retained synchronous queries. |
| `src/sql/sql_pool.c` | Bounded connection pool used by typed persistence workers. |
| `src/player/player_load_pipeline.c`, `src/player/player_load_repository.c`, `src/player/player_load_materialize.c` | Bounded load orchestration, consistent SQL snapshot reads, and game-thread character materialization; client-free builds select the flat-file load repository. |
| `src/player/player_save_pipeline.c` | Revisioned checkpoint admission, journal handoff, and exact completion tracking. |
| `src/player/player_snapshot_capture.c`, `src/player/player_snapshot_codec.c` | Game-thread snapshot capture and bounded snapshot/item encoding and decoding. |
| `src/player/player_save_journal.c`, `src/player/player_save_worker.c` | Typed checkpoint journal and keyed save workers; journal durability and applied-revision acknowledgements are separate stages. |
| `src/player/player_snapshot_repository.c`, `src/flatfile/flatfile_player_repository.c` | SQL snapshot application and the selected flat-file player load/save adapter. |
| `src/persistence/critical_command_coordinator.c`, `src/persistence/critical_command_journal.c` | Immutable, non-coalescing critical operations, entity fences, journal admission, and completion tracking. |
| `src/persistence/critical_command_repository.c`, `src/persistence/critical_outbox.c` | Typed SQL execution/reconciliation and separate outbox delivery, retry, and deduplication boundaries. |
| `src/flatfile/flatfile_authority_transaction.c` | Recoverable, checksummed authority transactions and after-images used by flat-file domain owners. |
| `src/persistence/economic_sql_accounting_lifecycle_transaction.c`, `src/flatfile/flatfile_accounting_lifecycle_transaction.c` | Native accounting lifecycle transaction owners for the selected backend. Domain-specific transaction modules retain their own native effects and receipts. |
| `src/economy/economic_gameplay_authority.c` | Read-only lifecycle/admission projection installed by verified native owners; it is neither a balance store nor storage authority. |
| `src/persistence/maintenance_scheduler.c`, `src/persistence/maintenance_snapshot.c`, `src/persistence/maintenance_repository.c` | Budgeted recurring persistence: scheduling and completion, game-thread request capture, and worker-side repository execution. |
| `src/world/world_recovery_pipeline.c`, `src/world/world_recovery_codec.c`, `src/world/world_recovery_npc_items.c` | Bounded world-graph capture, immutable recovery encoding/publication, and boot restore with item-custody reconciliation. |
| `src/redis/redis_world_runtime.c`, `src/redis/redis_floor_runtime.c`, `src/redis/redis_presence_runtime.c` | Separate optional world-recovery, floor-delta, and presence service lifecycles and worker integrations. |
| `src/redis/redis.c`, `src/redis/wizredis.c` | Redis integration orchestration and staff diagnostics; the service modules own their specific work. |
| `src/persistence/persistence_queue.c` | Retained item/scalar/large-payload compatibility queues and workers. |
| `src/sql/sql_persistence_raw.c` | Raw SQL executor retained for large-payload compatibility producers. |
| `src/sql/sql_player.c` | Character row mapping. |
| `src/core/files.c` | Legacy binary playerfile I/O and pfile utilities. |

Backend support has feature-specific limits. Client-free builds disable the
SQL telemetry sink. Redis world restores that require item-custody
reconciliation depend on the SQL implementation; the client-free stub accepts
only an empty reconciliation. See the architecture guide's
[backend boundaries](ARCHITECTURE.md#backend-and-authority-selection) and
[load, checkpoint, and world-recovery flow](ARCHITECTURE.md#player-loading-and-checkpoints).
See [DATABASE.md](DATABASE.md) for behavior and migrations,
[PLAYER_SAVE_PIPELINE.md](../persistence/PLAYER_SAVE_PIPELINE.md) and
[PLAYER_SAVE_JOURNAL.md](../persistence/PLAYER_SAVE_JOURNAL.md) for checkpoint
stages, [CRITICAL_COMMAND_PIPELINE.md](../persistence/CRITICAL_COMMAND_PIPELINE.md)
for critical operations, and
[ECONOMY_ACCOUNTING.md](../persistence/ECONOMY_ACCOUNTING.md) for route coverage
and qualification limits.

## Telemetry

| Files | Role |
| --- | --- |
| `src/telemetry/telemetry_runtime.c` | Opt-in observation capture, configuration publication, and private writer lifecycle. |
| `src/telemetry/telemetry_transport.c` | Fixed-capacity value-record queue, bounded admission, and worker-side batching/drain. |
| `src/telemetry/telemetry_repository.c` | Private SQL sink connection and bounded, replay-aware record application. |

Gameplay hooks capture observations without transferring mutation authority.
Client-free builds report `flatfile_disabled` and start no telemetry writer.
See [telemetry architecture](ARCHITECTURE.md#telemetry),
[TRANSPORT.md](../telemetry/TRANSPORT.md), and
[runtime configuration](../operations/CONFIGURATION.md#telemetry-runtime).

## Networking

| Files | Role |
|-------|------|
| `src/account/nanny.c` | Connection state machine: login, account/character selection, creation, and hints. |
| `src/net/ssl.c` | TLS telnet listener (`duris.crt`/`duris.key`). |
| `src/net/websocket.c`, `src/net/websocket.h` | RFC 6455 WebSocket server and HTTP health endpoint. |
| `src/core/json_utils.c` | JSON encode/decode helpers used by network protocols. |
| `src/net/gmcp.c` | GMCP negotiation and outbound game-data packages. |
| `src/net/mccp.c` | MCCP (MUD Client Compression Protocol). |
| `src/persistence/copyover.c` | Stateful hot reboot across `exec()` for eligible plain Telnet sessions, with connection and combat recovery. |
| `src/net/editor.c` | In-game line editor for mail and boards. |
| `src/cmd/mail.c` | Internal mail store and command handling. |

This checkout uses the native-layout version 17 handoff defined in
`src/persistence/copyover.h`, with readers for versions 12 through 17. The state
file defaults to `copyover.dat` and can be selected with `COPYOVER_STATE_FILE`.
Admission cancels the whole copyover if any live connection is outside
`CON_PLAYING`, has no character, or uses TLS or WebSocket; the remaining
character-state and persistence guards must also pass. See
[copyover runtime validation](../testing/COPYOVER_RUNTIME.md) for focused checks
and their coverage limits.

## Gameplay systems

Representative entry points by feature:

- **Combat and effects:** `src/combat/fight.c`, `src/combat/mobcombat.c`,
  `src/magic/affects.c`, and the other modules under `combat/`.
- **Magic, classes, and progression:** `src/magic/`, `src/classes/`,
  `src/world/epic.c`, and `src/world/achievements.c`.
- **Objects and economy:** `src/cmd/actobj.c`, `src/item/`, and `src/economy/`.
  Batch transfer syntax and atomicity are documented in
  [BATCH_ITEM_COMMANDS.md](BATCH_ITEM_COMMANDS.md).
- **World and movement:** `src/cmd/actmove.c`, `src/world/map.c`,
  `src/world/weather.c`, and the rest of `src/world/`.
- **Guilds and social systems:** `src/guild/` plus social handlers in
  `src/cmd/` and `src/world/`.
- **Quests:** `src/world/quest.c`, `src/cmd/nq.c`,
  `src/world/world_quest.c`, and `src/mob/encounters.c`.
- **Immortal and building commands:** primarily `src/cmd/actwiz.c`,
  `src/cmd/wikihelp.c`, `src/world/properties.c`, and `src/cmd/testcmd.c`.
- **Chaos mode:** `src/combat/chaos.c`, `src/combat/chaos_config.c`, and the
  pre-entry grant flow in `src/account/nanny.c`; see
  [CHAOS_MODE.md](CHAOS_MODE.md).

## Ships subsystem

`src/ships/` is the self-contained naval simulation. `src/ships/ship_base.c` owns the
core lifecycle, `src/ships/ship_cargo.c` cargo, `src/ships/ship_combat.c` naval combat,
`src/ships/ship_control.c` commands and movement, and `src/ships/ship_shop.c` dock construction and
trade. `src/ships/ship_auto.c` is the player autopilot; it is not generated and is
separate from the NPC combat brain in `src/ships/ship_npc_ai.c`. `src/ships/ship_identity.c`
provides process-local generation-checked references, `src/ships/ship_variables.c` owns
the append-only static identifier tables, and `src/ships/ship_utils.c` contains shared
map/contact helpers. The external API is `src/ships/ships.h`; ship index data is
`lib/etc/ship_index`.

## Kingdom subsystem and retired siege identifiers

The current `src/kingdom/` feature is a guild map-territory system, not the
retired siege/town-defense implementation. It is runtime-gated by
`kingdom.enabled` in `lib/kingdom.cfg`, represents an 80-square ordered realm
with one `highest_claim` integer, and persists it in `kingdom_realms`.
`src/kingdom/kingdom.h` is the main external interface;
`src/kingdom/kingdom_store_piece.h` supplies narrow store-gear classification
and purchase-owner checks used by command and spell/effect code. Player rules
are in `lib/information/helpkingdoms`.

Several old siege identifiers are permanent compatibility reservations and
must not be reused:

| Surface | Reservation |
| --- | --- |
| Command table | Slots 827 and 828 remain `_retired_827` and `_retired_828`, named `CMD_RETIRED_827` and `CMD_RETIRED_828`. |
| Persisted player flags | `act2` bit 4 remains `PLR2_RETIRED_KINGDOMVIEW`. |
| World data | Area zone 4010 and the former 401000-series siege room/mobile range remain retired. |
| Object prototypes | VNUMs 160, 161, 178, 179, and 461 through 464 remain retired. |
| Schema | The five retired table tombstones are listed in [DATABASE.md](DATABASE.md#tables-worth-knowing). |

The old `SIEGE_ENABLED` compile-time surface and its runtime code are gone.
These reservations protect persisted flags, command numbering, backups, and
area identities; they do not gate or describe the new kingdom module.

## Configuration and data

- Compile-time: `src/core/config.h` (default ports, pulses, paths) and Makefile
  defines.
- Runtime environment: startup loads `.env` through `src/core/env_file.c`,
  preserving existing process environment values. Database connection settings
  and credentials come from `DB_*` environment variables; `src/sql/sql.h`
  provides accessors, not stored credentials. See
  [CONFIGURATION.md](../operations/CONFIGURATION.md) for precedence and settings.
- Runtime data: `lib/` — `duris.properties`, per-feature `*.cfg`
  (`crafting.cfg`, `mining.cfg`, `hardcore.cfg`, `frag_cap.cfg`,
  `account_rewards.cfg`, `creation_availability.cfg`, `random_equipment.cfg`,
  `kingdom.cfg`),
  greetings/MOTD/news/help text under `lib/information/`, misc runtime files
  under `lib/misc/`, boards under `lib/boards/`.
- World data: `areas/world.*` combined files generated from `areas/{wld,mob,obj,zon,qst,shp}/`
  by the `make_*` tools (see [BUILDING.md](../guides/BUILDING.md)). Optional
  per-zone room-position exports live under `areas/map/`; the server build does
  not consume them. Independently, `areas/dump_map_image.rb` renders a world
  file to a PNG overview for offline inspection; its arguments are
  `<file.wld> <width> <height> <tile-pixels>`.

`src/world/map.h` classifies the surface window as VNUMs 500000 through
659999, the main Underdark window as 700000 through 859999, and the Alatorin
Underdark window as 120000 through 123833. These are coordinate windows, not a
promise that every VNUM is loaded; callers must still resolve through
`real_room()` and handle `NOWHERE`.

## Dispatch signatures

Most of the tree is reached through fixed-shape function pointers held in
tables. These signatures are load-bearing: a parameter may be unused by a given
implementation, but the slot cannot be removed without breaking every table it
is registered in.

| Parameter signature | Held in |
|---|---|
| `(int, P_char, char *, int, P_char, P_obj)` | `skills[].spell_pointer` — spell dispatch |
| `(P_char, char *, int)` | command handlers (`src/cmd/interp.c` `CMD_*` table, `ACMD()`) |
| `(P_char, P_char, int, char *)` | `mob_proc_type` — mobile special procedures |
| `(int, P_char, int, char *)` | `room_proc_type`, stored in `room_data::funct` — first argument is the room index |
| `(P_char, P_char, P_obj, void *)` | `event_func_type` — event callbacks |
| `(P_obj, P_char, int, char *)` | `obj_proc_type` — object special procedures |
| `(void *, int, char *, int, int)` | `src/cmd/actset.c` `ac_*` setters, in `setBitTable::sb_func` |
| `(descriptor_data *, cJSON *)` | WebSocket command handlers registered in `ws_handle_command` (`src/net/ws_handlers.c`) |

When adding a handler to one of these families, register it in the owning table
in the same change. A handler that compiles and is never dispatched is a silent
defect — `ws_cmd_request_wholist` was written, authorization-guarded and
documented, but never added to `ws_handle_command`, so backend who-list requests
fell through to the unknown-command path until the compiler cleanup found it.

## C++ conventions the warning profile enforces

The build is `-Werror` with no `-Wno-*` exceptions (see
[BUILDING.md](../guides/BUILDING.md#warning-profile)). Four conventions follow from that:

- **Unused parameters in dispatch signatures** are written with the name
  commented out — `P_obj /*obj*/` — which keeps the documentation while
  satisfying `-Wunused-parameter`. Parameter names are not part of a function's
  type, so this can never change table compatibility. Use `[[maybe_unused]]`
  *instead* when the parameter's only use sits inside an `#if` that is inactive
  in this build; unnaming it would break that configuration silently.
  `src/cmd/interp.h`'s `ACMD(c)` and `src/combat/dam_mods.h`'s
  `MAKE_DAM_MOD_PRED()` expand into
  bodies that variously do and do not read a slot, so their slots carry
  `[[maybe_unused]]` for the same reason.
- **String literals into dispatch-pinned callees.** Command handlers, spell
  functions and special procedures take a writable `char *` because their type
  is pinned, and several tokenise the argument in place
  (`half_chop(argument, arg, argument)`). Passing a literal is a potential write
  to read-only memory, so `src/core/utils.h` provides `writable_arg`, a stack copy sized
  from the literal by deduction:

  ```cpp
  do_say(ch, writable_arg("Fill me with your strength!"), CMD_SAY);
  ```

  Do not add a `const_cast` or C-style cast in place of a writable copy.
  `str_free` has a separate string-ownership purpose: it takes `const char *`
  and casts internally to release an owned allocation, keeping that cast out
  of callers. Other compatibility casts exist in the tree; they need their
  own local rationale and do not make a string literal writable.
- **Immutable message and lookup tables are `const char *`.** `damage_messages`
  in particular is const-only, and its helpers take the caller's real buffer
  size — `tests/async/test_message_buffer_bounds.py` pins both. It also carries
  default member initializers, which makes it non-trivial: value-initialize it
  with `msg = {}`, never `memset(&msg, 0, sizeof(msg))`.
- **Set-but-unused values are evidence, not noise.** Never delete an assignment
  whose right-hand side calls anything but a known-pure helper. `generic_find()`
  is the canonical trap: it returns a bitmask most callers ignore while
  depending entirely on the character/object it writes through its
  out-parameters.

## Indexing invariants

Every one of these was a live crash. The guards are in place; keep them when
touching the surrounding code.

| Invariant | Why |
|---|---|
| `real_room()` may return `NOWHERE` (`-1`). Resolve once, check, then index `world[]`. | The random-labyrinth vnum ranges (`700000+`, `800000+`, `900000+`) are largely absent from the loaded world, so `reset_lab()` performed thousands of `world[-1]` accesses and walked the garbage pointers in `world[-1].people` / `.contents`. Same hazard in `create_lab`, `connect_lab`, `connect_other`. |
| `obj_index[obj->R_num]` requires `obj->R_num >= 0`. | Dynamic and uninstantiated objects have `R_num = -1`. The `OBJ_VNUM()` and `GET_OBJ_PROC()` macros in `src/core/utils.h` now return `-1`/`NULL` for a negative `R_num`, and `free_obj`, `do_wear`, `do_grab`, `do_remove`, `do_search` guard it directly. |
| There are exactly five bitvector banks, indices `0..4` (`bitvector` … `bitvector5`) in `obj_data` and `affected_type`. | `affect_modify` read `bitv[5]`, running off the array into the adjacent `affected[0]` and pointer fields. |
| Race indices into `stat_factor[]` / `combat_by_race[]` (`[LAST_RACE + 1]`) need `BOUNDED(0, race, LAST_RACE)`. | Equipment `affected[].modifier` values are attacker-controlled data, not a validated race. Guarded in `calculate_hitpoints2`, `apply_affs`, `affect_total`, `do_score`. |
| `wear()` must not fall back to a weapon slot for a non-weapon `ITEM_HOLD` item. | Equipping a non-weapon into `WIELD`/`WIELD3`/`WIELD4` breaks combat-round and damage invariants. `HOLD` now rejects when occupied. |

Command-time inventory dispatch in `special()` (`src/cmd/interp.c`) walks the
direct `ch->carrying` list before the typed command handler and does not recurse
into containers. An item pulled out of a portable hole becomes eligible for
that traversal on a later command that reaches it; command gates or earlier
special procedures may stop dispatch, and the item's proc decides whether to
act. This is not a restriction on all object-proc execution: periodic callbacks
in `src/world/db.c` and explicit lifecycle/combat calls also use
`invoke_object_special()`. `tests/async/test_wear_all_regression.py` and
`test_relic_lab_reset_bounds.py` cover the focused wear-slot and indexing guards
described above.

## Standalone tools

- `make -C src pfile` builds the offline pfile scanner at `bin/tools/pfile`;
  it is a distinct target and is not covered by the default server build.
- `migrations/tools/` — offline conversion/migration binaries (`pfile_converter`,
  account/guild/locker/ship migrators). Not part of the server build; build
  the primary `migrate_pfiles` tool with `make -C migrations/tools`, or use
  the `affects` and `pfile_converter` targets for those dedicated binaries.
- `areas/src/` — area compiler tools that turn per-area source dirs into
  combined `world.*` files.

## Gameplay feature references

### Bartender world-quest catalog

`src/world/world_quest_policy.c` builds the global catalog once at boot through
`calc_zone_mob_level()`, after world indexes, special procedures, and map setup.
Both persistence backends use loaded content; `zones.quest_zone` is not an
approval list. Repeated quest requests reuse cached mobile profiles, reward
pools, and per-level scores rather than scanning all prototypes or querying SQL
for zone eligibility. Temporary probes restore live entity counts.

Policy excludes sentinel/non-normal zones, explicit zones 0/292/536, towns and
hometowns, and zones without eligible local rewards. The truncated average
prototype level must lie strictly inside `(L - 7, L + 5)`. Mapless zones require
level 41 or higher. Source-eligible rewards pass the floor `2 * itemvalue >= L`;
reward counts and means are calculated after that filter. Items that any
hand-built quest pays out or asks players to hand in (the item goals of
`quest_index`) are never rewards, flagged `ITEM2_QUESTITEM` or not. Each zone
also withholds its most valuable `world.quest.reward.top.withheld.percent`
(default 20) percent of source-eligible items, rounded up and including value
ties. A zone whose items are all worth the same has no top tier, so nothing is
withheld there; a zone's only item is. Per-level pools and scores are built from
what remains, and the boot log reports both withheld counts. The setting is read
when the catalog is built, so a change applies at the next boot.

For eligible zones, the relative selection weight is
`exp(-abs(average_level - L) / 6) * (average_ivalue / L) * eligible_item_count`.
Item count is deliberately linear; zero-reward zones have zero weight. See
`world_quest_policy_math.h` for boundaries and constants.

Targets use the inclusive level band `[L - 4, L + 5]`. Kill targets need at
least two existing instances; ask targets need exactly one, speech capability,
and no player-specific aggression. Below level 31, invisible/concealed/hidden
ask targets are excluded. Dynamic aggression probes and completion-history
checks each have a request-wide budget of 32. Previously completed targets are
excluded from subsequent retries; a history-read error stops assignment.

`createQuest()` distinguishes no eligible zone from no valid target. Bartender
payments are admitted through the currency transaction coordinator; quest
state changes happen only after the debit publishes, and a post-commit
selection failure receives an asynchronous, rebasable refund. The deferred
continuation stores the bartender's stable vnum rather than a live NPC pointer,
so disconnect/reconnect cannot resume against stale room state. Completion
history still uses each backend's world-quest adapter. Reward selection uses
the accepted quest level's cached pool, with the existing logged random-
equipment fallback if unavailable.
`tests/async/run_world_quest_dual_backend.py` exercises real quest grant and
persistence in disposable MariaDB and flat-file instances; focused
`test_world_quest_*` tests cover policy and failure boundaries.

Quests cannot be shared unless `world.quest.share.max` (default 0, at most 4)
allows it. Every completed quest pays one item reward to the player who
completed it; kill quests pay at completion instead of rolling rewards onto
corpses. The fee is `world.quest.cost.per.level` copper per level (default 20).

### Shared NPC area-target pruning

The shared area-selection helper in `src/core/utility.c` protects an autonomous
NPC caster's eligible melee opponent as well as its explicit spell target from
random player-target pruning. It caps the skip count at the remaining
unprotected players. This policy applies to shared NPC area spells, including
Death Field; it does not bypass altitude, safe-room, alive-target or damage
checks. Charmed NPCs and bodies controlled by switched immortals retain the
player-controlled pruning policy.

`test_death_field_runtime.py` runs the production casting/selection/damage
chain with controlled world and defense fixtures. Its group-target cases
verify the pruning policy; they do not reproduce every reported encounter.

### Epic point and epic skill levels

`epic.gain.minLevel` (default 50) is the lowest level that earns epic points. The
gate sits in `prepare_epic_award()`, which every `gain_epic()` caller and each epic stone
participant pass through, and in `epic_calculate_pvp_award()` for PvP. A zone-touch
payload needs the toucher as its first recipient and a positive award for every
recipient, so a toucher below the level is refused and group members below it are left
out of the award. Touch-stone level costs are unchanged. `epic.skills.minLevel` (default
56) is the lowest level at which an epic teacher will teach. Epic potions use the same
gain-level requirement before quaffing, so an ineligible character keeps the potion
and receives the required level without consuming it or incurring a wait.

### Server difficulty dials

`src/world/difficulty.c` reads eighteen server-wide dials from the `[difficulty]`
section of `lib/duris.properties`. Each runs from 1 to 10. Setting 5 is always an exact
multiplier of 1.0 and every hook skips its arithmetic at 1.0, so a dial left at 5 is the
game as it was; `difficulty.curve.NN` maps the other settings to multipliers. Dials on
something players want (experience earned, loot drops and quality, mob gold, player
regeneration, epic points, artefact feeding) use the reciprocal, so a higher setting is
always harder. The dials stack on top of per-zone difficulty, and the mob dials apply
only to NPCs that are not a player's pet or morph.

The `difficulty` command lists the dials for gods; Forgers can `set`, `preset` and
`save`. It routes through `properties set`, which re-applies every property at once but
changes memory only until `difficulty save`. Mob hitpoints and mob gold apply to mobs
loaded afterwards, zone repop at each zone's next reset, and the rest immediately.
The dials multiply with zone difficulty: with the shipped zone factors, a
difficulty-10 zone under dial 10 gives mobs 9x hitpoints (2.0 x 4.5), 6x melee
damage (2.0 x 3.0) and 3x spell damage (2.0 x 1.5); no zone ships above 9. The
legacy 20-platinum coin bonus in `read_mobile()` is decided on the file's value,
so the mob gold dial scales the payout linearly.

Hooks: mob hitpoints in `apply_zone_modifier()`; melee after `damage_mod` in `hit()`;
spell damage after the modifier profile in `spell_damage()`, outside its 2.0 cap; hit
chance in `chance_to_hit()`; breath through `breath_damage_mod()`; saving throws in
`NewSaves()`; memorisation in `get_circle_memtime()`; mob coins in `read_mobile()`; the
experience table in `update_exp_table()`; earned experience (not PvP) and the death loss
in `gain_exp()`; PC corpse decay through `difficulty_pc_corpse_decay_minutes()`; player
regeneration in `hit_regen()`, `mana_regen()` and `move_regen()`; loot in
`check_random_drop()` and `create_random_eq_new()`; zone lifespan in `reset_zone()`;
epic points (not PvP) in `prepare_epic_award()`; artefact feeding in
`artifact_feed_seconds()`; bartender quests in the bartender fee, both backends'
`sql_world_quest_can_do_another()` (never fewer than one a day) and the kill count in
`createQuest()`.

### Racial pulse

Each race's action rates live in `lib/duris.properties`. `spellcast.pulse.racial.<Race>`
multiplies every spell's cast time; `damage.pulse.racial.<Race>` is the base melee round in
beats, to which `damage.pulse.class.all` and the class's `damage.pulse.class.<Class>` are
added in `affects.c`. Lower is faster in both. `src/world/racial_pulse.c` adds the `pulse`
command: gods list the rates with `pulse` or `pulse list all`; Forgers use
`pulse adjust <cast|melee> <race> <value>`, where the value is absolute and must have exactly
three decimals, and `pulse save`. Like the difficulty dials it routes through
`properties set`, so a change applies at once (melee by re-totalling every character of that
race) and stays in memory until saved.

Gear does not grant faster pulse. Every player-obtainable object that carried a negative
`APPLY_COMBAT_PULSE` or `APPLY_SPELL_PULSE` carries a racial-maximum stat instead, worth at
least as much in `rate_object()`, and the shaman spirit totem gives wisdom. Mob-only objects
(no-show or untakeable procs) keep theirs, as do spells and skills such as berserk and the
reaver buffs.
