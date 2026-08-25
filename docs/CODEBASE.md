# Codebase Guide

A map of the server sources. There are ~220 `.c` files under `src/`, compiled
individually as C++20 and linked into `src/dms_new`. File boundaries follow
DikuMUD convention: `act*.c` files hold player commands grouped by theme,
`do_<name>` functions are commands, `specs.*.c` files hold zone-specific
special procedures.

Related: [ARCHITECTURE.md](ARCHITECTURE.md), [DATABASE.md](DATABASE.md).

## Core engine

| Files | Role |
|-------|------|
| `comm.c` | `main()`, `game_loop()` — select() loop, socket I/O, pulse dispatch, signal handling. |
| `db.c` | Boot: loads world files, zone resets, character loading; core allocation helpers. |
| `interp.c` | Command table (`CMD_*` rows) and command dispatch/aliasing. |
| `handler.c` | Object/character lifecycle (give, extract, affect attach/detach), generic char maintenance events. |
| `structs.h` | Central data structures (`char_data`, `obj_data`, `descriptor_data`, ...). |
| `prototypes.h` | Function declarations for the whole tree. |
| `utility.c` | String/number helpers, logging (`logit`, `wizlog`). |
| `constant.c`, `utils.h` | Constants, tables, accessor macros used everywhere. |

## Events, time, and triggers

| Files | Role |
|-------|------|
| `events.c`, `new_events.c` | Timed callback wheel executed inside the game loop. |
| `timers.c` | Player-facing timers (affect durations etc.). |
| `studioproc.c`, `studioproc.h` | Studio-proc trigger engine for `areas/world.trg`; see [STUDIOPROC.md](STUDIOPROC.md). |
| `studioproclib.c/.h` | Built-in proc function library callable from triggers. |
| `latency_trace.c` | Per-callback latency telemetry (`NEVENT BUDGET`). |

## Persistence

| Files | Role |
|-------|------|
| `sql.c`, `sql.h` | Main MySQL connection(s), `qry()` helper, credentials (`DB_HOST`... in `sql.h`), boot-time schema checks. Port-based dev/prod database selection lives in `initialize_mysql()`. |
| `sql_pool.c` | Fixed-size connection pool shared by the three async persistence workers. |
| `persistence_queue.c` | Item / scalar / large-payload event queues and their worker threads. |
| `sql_persistence_raw.c` | Raw SQL execution for the large-payload worker. |
| `sql_player.c` | Character row mapping (players table). |
| `files.c` | Legacy binary playerfile I/O and pfile utilities. |
| `redis.c`, `wizredis.c` | Dirty-save buffering and world-state snapshots for crash recovery (hiredis). |
| `persistence_queue.h`, `sql_pool.h` | Queue/pool APIs. |

See [DATABASE.md](DATABASE.md) for behavior and migrations.

## Networking

| Files | Role |
|-------|------|
| `nanny.c` | Connection state machine: login, account/character selection, creation, hints. |
| `ssl.c` | TLS telnet listener (`duris.crt`/`duris.key`). |
| `websocket.c`, `websocket.h` | RFC 6455 WebSocket server on port 4050. |
| `json_utils.c` | JSON encode/decode helpers for WebSocket traffic. |
| `gmcp.c` | GMCP outbound protocol support. |
| `mccp.c` | MCCP (MUD Client Compression Protocol). |
| `copyover.c` | Hot reboot: survives `exec()` via `copyover.dat`, restores connections and combat. |
| `editor.c` | In-game line editor (mail, boards). |
| `mail.c` | Internal mail store/format. |

## Gameplay systems

Large, roughly alphabetical by feature:

- **Combat & effects:** `fight.c`, `mobcombat.c`, `affects.c`, `dam_mods.c`,
  `grapple.c`, `breath_weapons.c`.
- **Magic:** `magic.c`, `spells.c`, `smagic.c`, class spell files
  (`blispells.c`, `beh_magic.c`, `necromancy.c`, `psionics.c`, `shaman.c`,
  `bard.c`, `rogues.c`, `paladins.c`, ...), `memorize.c`.
- **Classes/specs:** `specifications`-style per-specialization modules named
  `specs.<name>.c` (e.g. `specs.dragoon`-style content lives here — one file
  per special/zone/specialization).
- **Objects & economy:** `actobj.c`, `shop.c`, `auction_houses.c`,
  `storage_lockers.c`, `locker_async.c`, `forge_items.c`, `salvage.c`,
  `crafting.c`, `mining.c`, `enhance.c`, `randomeq.c`, `random_equipment_config.c`,
  `material_rarity.c`, `artifact.c`, `relic`-related code in specs.
- **World & movement:** `actmove.c`, `track.c`, `mount.c`, `ferry.c`,
  `ferryact.c`, `transport.c`, `map.c`, `weather.c`, `makeexit.c`.
- **Social & meta:** `boards.c`, `cardgames.c`, `arena.c`, `ctf.c`,
  `alliances.c`, `assocs.c`, `guild*.c`, `justice.c`, `disguise.c`.
- **Progression:** `limits.c`, `skills.c`, `new_skills.c`, `epic*.c`,
  `achievements.c`, `innates.c`, `specializations.c`, `trophy.c`.
- **Accounts & rewards:** `account.c`, `account_reward*.c`,
  `frag_cap_config.c`, `hardcore.c`, `hardcore_config.c`, `multiplay_whitelist.c`.
- **Quests:** `quest.c`, `nq.c` (newbie quests), `world_quest.c`, `encounters.c`.
- **Immortal/building:** `actwiz.c` (immortal commands incl. shutdown),
  `wikihelp.c` (help rendering + command attributes), `properties.c`
  (runtime-tunable properties via `get_property()`), `testcmd.c`.

## Ships subsystem

`src/ships/` — self-contained naval simulation compiled separately
(`ship_base`, `ship_cargo`, `ship_combat`, `ship_control`, `ship_npc`,
`ship_npc_ai`, `ship_shop`, plus auto-generated glue in `ship_auto.c`).
Public API in `ships.h`. Ship index data at `lib/etc/ship_index`.

## Configuration and data

- Compile-time: `src/config.h` (ports, pulses, paths), `src/sql.h`
  (credentials), Makefile defines.
- Runtime data: `lib/` — `duris.properties`, per-feature `*.cfg`
  (`crafting.cfg`, `mining.cfg`, `hardcore.cfg`, `frag_cap.cfg`,
  `account_rewards.cfg`, `creation_availability.cfg`, `random_equipment.cfg`),
  greetings/MOTD/news/help text under `lib/information/`, misc runtime files
  under `lib/misc/`, boards under `lib/boards/`.
- World data: `areas/world.*` combined files generated from `areas/{wld,mob,obj,zon,qst,shp}/`
  by the `make_*` tools (see [BUILDING.md](BUILDING.md)).

## Standalone tools

- `src-migrate/` — offline conversion/migration binaries (`pfile_converter`,
  account/guild/locker/ship migrators). Not part of the server build.
- `areas/src/` — area compiler tools that turn per-area source dirs into
  combined `world.*` files.
