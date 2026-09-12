# Codebase Guide

A map of the server sources. Hundreds of `.c` files under `src/` are compiled
individually as C++20 and linked into `bin/server/dms_new`. The `src/` root
contains only the Makefile and repository metadata; implementation and headers
live in subsystem directories, and includes use paths qualified from `src/`
(for example, `#include "core/structs.h"`). File boundaries retain the DikuMUD
conventions: `act*.c` files group player commands, `do_<name>` functions are
commands, and `specs.*.c` files hold special procedures.

Related: [ARCHITECTURE.md](ARCHITECTURE.md), [DATABASE.md](DATABASE.md).

## Source layout

| Directories | Responsibility |
| --- | --- |
| `account/`, `player/` | Account/login/creation flows and player load/save/materialization. |
| `classes/`, `magic/`, `combat/` | Class and skill rules, spells, affects, and combat. |
| `cmd/` | Command registration plus player, object, social, and staff handlers. |
| `core/` | Shared structures, constants, prototypes, utilities, configuration, and legacy pfile support. |
| `economy/`, `item/` | Shops, auctions, balances, crafting, equipment, lockers, and durable item movement. |
| `flatfile/`, `no_mysql/` | Complete flat-file persistence backend and client-free build stubs. |
| `guild/`, `kingdom/` | Guild/social ownership and the current map-territory kingdom feature. |
| `mob/`, `specs/` | Mobile behavior, studio procs, and area/special procedures. |
| `net/` | Telnet, TLS, WebSocket, GMCP, MCCP, prompts, and descriptor I/O. |
| `persistence/`, `sql/`, `redis/` | Typed durability coordinators, MariaDB repositories, and Redis integrations. |
| `ships/` | Naval simulation, dock economy, and player/NPC ship control. |
| `world/` | Boot/loading, world lifecycle, movement, maps, events, quests, and global updates. |

## Core engine

| Files | Role |
|-------|------|
| `src/net/comm.c` | `main()`, `game_loop()` — `select()` loop, socket I/O, pulse dispatch, signal handling. |
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
| `src/world/timers.c` | Player-facing timers such as affect durations. |
| `src/mob/studioproc.c`, `src/mob/studioproc.h` | Studio-proc trigger engine for `areas/world.trg`; see [STUDIOPROC.md](../content/STUDIOPROC.md). |
| `src/mob/studioproclib.c`, `src/mob/studioproclib.h` | Built-in proc function library callable from triggers. |
| `src/persistence/latency_trace.c` | Per-callback latency telemetry (`NEVENT BUDGET`). |

## Persistence

| Files | Role |
|-------|------|
| `src/sql/sql.c`, `src/sql/sql.h` | Main MariaDB connection, target selection, boot schema checks, and retained synchronous queries. |
| `src/sql/sql_pool.c` | Bounded connection pool used by typed persistence workers. |
| `src/persistence/persistence_queue.c` | Retained item/scalar/large-payload compatibility queues and workers. |
| `src/sql/sql_persistence_raw.c` | Raw SQL executor retained for large-payload compatibility producers. |
| `src/sql/sql_player.c` | Character row mapping. |
| `src/player/player_load_pipeline.c`, `src/player/player_save_pipeline.c` | Bounded typed player load and revisioned checkpoint orchestration. |
| `src/persistence/critical_command_coordinator.c` | Non-coalescing critical gameplay operations and entity fencing. |
| `src/core/files.c` | Legacy binary playerfile I/O and pfile utilities. |
| `src/redis/redis.c`, `src/redis/wizredis.c` | Redis caches, floor deltas, immutable world recovery, and staff diagnostics. |

See [DATABASE.md](DATABASE.md) for behavior and migrations.

## Networking

| Files | Role |
|-------|------|
| `src/account/nanny.c` | Connection state machine: login, account/character selection, creation, and hints. |
| `src/net/ssl.c` | TLS telnet listener (`duris.crt`/`duris.key`). |
| `src/net/websocket.c`, `src/net/websocket.h` | RFC 6455 WebSocket server and HTTP health endpoint. |
| `src/core/json_utils.c` | JSON encode/decode helpers used by network protocols. |
| `src/net/gmcp.c` | GMCP negotiation and outbound game-data packages. |
| `src/net/mccp.c` | MCCP (MUD Client Compression Protocol). |
| `src/persistence/copyover.c` | Hot reboot: survives `exec()` via `copyover.dat` and restores connections and combat. |
| `src/net/editor.c` | In-game line editor for mail and boards. |
| `src/cmd/mail.c` | Internal mail store and command handling. |

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
with one `highest_claim` integer, and persists it in `kingdom_realms`. Only
`src/kingdom/kingdom.h` is public outside the subsystem. Player rules are in
`lib/information/helpkingdoms`.

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

- Compile-time: `src/core/config.h` (ports, pulses, paths), `src/sql/sql.h`
  (credentials), Makefile defines.
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

| Signature | Held in | Functions |
|---|---|---:|
| `(int, P_char, char *, int, P_char, P_obj)` | `skills[].spell_pointer` — spell dispatch | ~690 |
| `(P_char, char *, int)` | command handlers (`src/cmd/interp.c` `CMD_*` table, `ACMD()`) | ~510 |
| `(P_char, P_char, int, char *)` | mobile and room special procedures | ~455 |
| `(P_char, P_char, P_obj, void *)` | event callbacks | ~210 |
| `(P_obj, P_char, int, char *)` | object special procedures | ~150 |
| `(void *, int, char *, int, int)` | `src/cmd/actset.c` `ac_*` setters, in `setBitTable::sb_func` | 18 |
| `(descriptor_data *, cJSON *)` | WebSocket command handlers (`src/net/ws_handlers.c`) | ~10 |

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

  Do not add a `const_cast` or C-style cast instead. The one deliberate
  exception in the tree is inside `str_free`, which takes `const char *`
  because an owning pointer to string data is normally spelled that way;
  the cast lives in that one function rather than at every call site.
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

Object special procedures do **not** fire for items inside a container. They fire
from `special()` (`src/cmd/interp.c`), which walks `ch->carrying` *before* the typed
command runs — so the proc for an item pulled out of a portable hole fires on the
player's *next* command, whatever that command is. `tests/async/test_wear_all_regression.py`
and `test_relic_lab_reset_bounds.py` cover these paths.

## Standalone tools

- `make -C src pfile` builds the offline pfile scanner at `bin/tools/pfile`;
  it is a distinct target and is not covered by the default server build.
- `migrations/tools/` — offline conversion/migration binaries (`pfile_converter`,
  account/guild/locker/ship migrators). Not part of the server build; build
  the primary `migrate_pfiles` tool with `make -C migrations/tools`, or use
  the `affects` and `pfile_converter` targets for those dedicated binaries.
- `areas/src/` — area compiler tools that turn per-area source dirs into
  combined `world.*` files.

## Bartender world-quest catalog

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

`createQuest()` distinguishes no eligible zone from no valid target, and the
bartender refunds the fee on failure. Completion history still uses each
backend's world-quest adapter. Reward selection uses the accepted quest level's
cached pool, with the existing logged random-equipment fallback if unavailable.
`tests/async/run_world_quest_dual_backend.py` exercises real quest grant and
persistence in disposable MariaDB and flat-file instances; focused
`test_world_quest_*` tests cover policy and failure boundaries.

Quests cannot be shared unless `world.quest.share.max` (default 0, at most 4)
allows it. Every completed quest pays one item reward to the player who
completed it; kill quests pay at completion instead of rolling rewards onto
corpses. The fee is `world.quest.cost.per.level` copper per level (default 20).

## Shared NPC area-target pruning

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

## Epic point and epic skill levels

`epic.gain.minLevel` (default 50) is the lowest level that earns epic points. The
gate sits in `prepare_epic_award()`, which every `gain_epic()` caller and each epic stone
participant pass through, and in `epic_calculate_pvp_award()` for PvP. A zone-touch
payload needs the toucher as its first recipient and a positive award for every
recipient, so a toucher below the level is refused and group members below it are left
out of the award. Touch-stone level costs are unchanged. `epic.skills.minLevel` (default
56) is the lowest level at which an epic teacher will teach.

## Server difficulty dials

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
