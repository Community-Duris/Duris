# Codebase Guide

A map of the server sources. There are ~220 `.c` files under `src/`, compiled
individually as C++20 and linked into `bin/server/dms_new`. File boundaries follow
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
| `studioproc.c`, `studioproc.h` | Studio-proc trigger engine for `areas/world.trg`; see [STUDIOPROC.md](../content/STUDIOPROC.md). |
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
| `redis.c`, `wizredis.c` | Redis caches, floor deltas, and immutable world-recovery generations (hiredis). |
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
  by the `make_*` tools (see [BUILDING.md](../guides/BUILDING.md)).

## Dispatch signatures

Most of the tree is reached through fixed-shape function pointers held in
tables. These signatures are load-bearing: a parameter may be unused by a given
implementation, but the slot cannot be removed without breaking every table it
is registered in.

| Signature | Held in | Functions |
|---|---|---:|
| `(int, P_char, char *, int, P_char, P_obj)` | `skills[].spell_pointer` — spell dispatch | ~690 |
| `(P_char, char *, int)` | command handlers (`interp.c` `CMD_*` table, `ACMD()`) | ~510 |
| `(P_char, P_char, int, char *)` | mobile and room special procedures | ~455 |
| `(P_char, P_char, P_obj, void *)` | event callbacks | ~210 |
| `(P_obj, P_char, int, char *)` | object special procedures | ~150 |
| `(void *, int, char *, int, int)` | `actset.c` `ac_*` setters, in `setBitTable::sb_func` | 18 |
| `(descriptor_data *, cJSON *)` | WebSocket command handlers (`ws_handlers.c`) | ~10 |

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
  `interp.h`'s `ACMD(c)` and `dam_mods.h`'s `MAKE_DAM_MOD_PRED()` expand into
  bodies that variously do and do not read a slot, so their slots carry
  `[[maybe_unused]]` for the same reason.
- **String literals into dispatch-pinned callees.** Command handlers, spell
  functions and special procedures take a writable `char *` because their type
  is pinned, and several tokenise the argument in place
  (`half_chop(argument, arg, argument)`). Passing a literal is a potential write
  to read-only memory, so `utils.h` provides `writable_arg`, a stack copy sized
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
| `obj_index[obj->R_num]` requires `obj->R_num >= 0`. | Dynamic and uninstantiated objects have `R_num = -1`. The `OBJ_VNUM()` and `GET_OBJ_PROC()` macros in `utils.h` now return `-1`/`NULL` for a negative `R_num`, and `free_obj`, `do_wear`, `do_grab`, `do_remove`, `do_search` guard it directly. |
| There are exactly five bitvector banks, indices `0..4` (`bitvector` … `bitvector5`) in `obj_data` and `affected_type`. | `affect_modify` read `bitv[5]`, running off the array into the adjacent `affected[0]` and pointer fields. |
| Race indices into `stat_factor[]` / `combat_by_race[]` (`[LAST_RACE + 1]`) need `BOUNDED(0, race, LAST_RACE)`. | Equipment `affected[].modifier` values are attacker-controlled data, not a validated race. Guarded in `calculate_hitpoints2`, `apply_affs`, `affect_total`, `do_score`. |
| `wear()` must not fall back to a weapon slot for a non-weapon `ITEM_HOLD` item. | Equipping a non-weapon into `WIELD`/`WIELD3`/`WIELD4` breaks combat-round and damage invariants. `HOLD` now rejects when occupied. |

Object special procedures do **not** fire for items inside a container. They fire
from `special()` (`interp.c`), which walks `ch->carrying` *before* the typed
command runs — so the proc for an item pulled out of a portable hole fires on the
player's *next* command, whatever that command is. `tests/async/test_wear_all_regression.py`
and `test_relic_lab_reset_bounds.py` cover these paths.

## Standalone tools

- `src-migrate/` — offline conversion/migration binaries (`pfile_converter`,
  account/guild/locker/ship migrators). Not part of the server build.
- `areas/src/` — area compiler tools that turn per-area source dirs into
  combined `world.*` files.
