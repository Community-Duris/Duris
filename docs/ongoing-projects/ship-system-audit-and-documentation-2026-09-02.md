# Ship system audit + documentation pass

Status: **in progress**. Started: 2026-09-02. Branch: `master` (working tree).

## Goal

Audit `src/ships/` for defects and fix them **without changing any game
mechanics**, and bring the subsystem to 100% docstring coverage plus enough
navigational commentary that a developer new to the codebase can find their way
around it.

## Non-goals

- No mechanic, balance, formula, or message changes.
- No refactors, renames, or file moves.
- No new abstractions, dependencies, or test infrastructure.
- No schema/migration/persistence-format changes.
- No deletion of dormant code (documented as dormant instead — deleting it is a
  separate, riskier call that belongs to the owner).

## Scope

`src/ships/` — 11 `.c` files and 4 headers, ~19,400 lines, ~440 functions.

| File | Lines | ~Fns | Audited | Documented |
| --- | ---: | ---: | :---: | :---: |
| `ship_auto.c` | 452 | 11 | yes | **yes (11/11)** |
| `ship_base.c` | 3010 | 45 | partial | — |
| `ship_cargo.c` | 1280 | 40 | yes | **yes (40/40)** |
| `ship_combat.c` | 1503 | 23 | yes | **yes (23/23)** |
| `ship_control.c` | 1770 | 31 | partial | — |
| `ship_identity.c` | 95 | 4 | yes | **yes (4/4)** |
| `ship_npc.c` | 1424 | 64 | — | — |
| `ship_npc_ai.c` | 2211 | 59 | — | — |
| `ship_shop.c` | 3481 | 37 | partial | — |
| `ship_utils.c` | 1981 | 116 | yes | **yes (116/116)** |
| `ship_variables.c` | 1022 | 0 (tables) | yes | n/a — tables |
| `ships.h` | 884 | — | yes | — |
| `ship_auto.h` | 47 | — | yes | — |
| `ship_npc.h` | 62 | — | — | — |
| `ship_npc_ai.h` | 175 | — | — | — |

Docstring coverage is measured with
`python3 <scratchpad>/cover.py src/ships/<file>.c`, which flags any top-level
definition with no comment block directly above it.

## Baseline

- `make -C src` builds clean at the project's full `-Werror` warning set
  (`-Wall -Wextra -Wpedantic -Wformat=2 -Warray-bounds=2 -Wnull-dereference …`),
  so every finding below is a *logic* defect the compiler cannot see.
- All 15 files in `src/ships/` are already whole-file `clang-format` clean.
  They must stay that way: CI formats every tracked file, not just changed
  lines. Re-run `clang-format --dry-run -Werror src/ships/*.c src/ships/*.h`
  after each file.

## Findings

Severity key: **P1** memory-safety / player-reachable, **P2** latent
memory-safety, **P3** correctness, **P4** clarity only (no code change).

### Fixed

| # | Sev | Site | Defect |
| --- | --- | --- | --- |
| 1 | P1 | `ship_auto.c` `engage_autopilot()` | `order sail <dir> <rooms>` only rejected `dist > 35`; a negative `<rooms>` projected the target far outside `tactical_map[101][101]`, giving an out-of-bounds read that then fed `world[t_room]`. Player-reachable. Now rejects `dist < 0` and clamps both map subscripts. |
| 2 | P2 | `ship_auto.c` `stop_autopilot()` | Dereferenced `ship->autopilot` with no NULL check, while `clear_autopilot()` nulls it and most ships never have one. Callers in `ship_base.c` / `ship_control.c` reach it from ordinary steering paths. Now returns early. |
| 3 | P1 | `ship_shop.c` `repair_weapon()` | Guard was `slot > MAXSLOTS`, so a player typing `repair weapon 16` indexed `ship->slot[16]` — one past the end of a 16-element array. Now `slot >= MAXSLOTS`. |
| 4 | P2 | `ship_utils.c` `change_crew()` / `set_crew()` | Guard was `crew_index > MAXCREWS`, so `crew_index == 25` passed and read `ship_crew_data[25]` (valid 0..24). Worse, it was then stored in `ship->crew.index`, poisoning every later `ship_crew_data[index]` read. Reachable from the immortal `setship <name> crew <n>` command (`src/cmd/actset.c:823`). Now `>= MAXCREWS`. |
| 5 | P2 | `ship_utils.c` `set_chief()` | Had no bounds check at all on `chief_index` before `ship_chief_data[chief_index]` (valid 0..12), while `src/cmd/actset.c:830` passes `atoi(val)` straight through. Now range-guarded (and NULL-ship guarded); out-of-range input changes nothing. |
| 6 | P2 | `ship_utils.c` `ShipCrewData::get_next_bonus()` | `ulong flag = 1 << (*cur);` with `*cur` running to 31 — `1 << 31` is signed overflow (UB), and the sign-extended result would test the wrong bits of a 64-bit `ulong`. No current crew sets bit 31, so today's behaviour is unchanged. Now `1UL << (*cur)`. |
| 7 | P3 | `ship_utils.c` `pc_is_aboard()` | `ch_next` was initialised to 0 and never advanced, so the loop examined only the *first* character in each ship room and stopped — a PC standing behind an NPC was not seen. Both callers use the result to *protect* players (`finish_sinking()` delays the sink; `NPCShipAI::try_unload()` refuses to unload), so the fix is strictly protective. Now iterates `ch->next_in_room`. |
| 8 | P2 | `ship_utils.c` `getcontacts()` | `setcontact(counter++, …)` wrote into `contacts[MAXSHIPS]` with no upper bound on `counter`, overflowing once more than `MAXSHIPS` (2000) ships exist. Now stops at `MAXSHIPS`. |
| 9 | P4 | `ship_utils.c` `ShipSlot::get_weight()` | `if (type == SLOT_WEAPON) {…} if (type == SLOT_EQUIPMENT) {…} else if …` — the second `if` was missing an `else`. Harmless (the conditions are mutually exclusive) but fragile; now `else if`, which is exactly behaviour-preserving. |
| 10 | P2 | `ship_cargo.c` `calculate_port_distances()` | `strcat()` onto an uninitialised `char line[MAX_STRING_LENGTH]` — it walked whatever stack garbage was there looking for a NUL, then appended past it. Every later reuse of the buffer correctly does `line[0] = '\0'` first; only the first one was missing. Now initialised. |
| 11 | P2 | `ship_combat.c` (×3), `ship_control.c` (×5), `ship_base.c` (×1) | `get_char2(str_dup(SHIP_OWNER(ship)))` and `isname(str_dup(...), ...)` — nine straight memory leaks. `get_char2()` copies the name into its own stack buffer (`src/world/handler.c:2435`) and `isname()` only compares; neither takes ownership, so every `str_dup()` was lost. Two of the sites are inside per-contact loops in `sink_ship()`, so the leak scaled with the number of ships present at a kill. Now the owner string is passed directly — byte-identical argument, no allocation. |

### Found, NOT fixed (needs an owner decision)

| # | Sev | Site | Issue |
| --- | --- | --- | --- |
| A | P3 | `ship_combat.c` `ch_damage_hull()` | Calls `damage_weapon(target, target, arc, dam * 5)` — passing the *target* as the attacker, where `damage_hull()` correctly passes `attacker`. The consequence is that the victim's own crew is shown the attacker-side "You damage a …" message on top of their own "Your … has been damaged". It looks like a copy-paste slip (the real attacker here is a `P_char`, which `damage_weapon()` cannot take, so `NULL` is the likely intent). Left alone because it changes player-visible output, which is out of scope for this pass. Documented inline at the call site. |
| B | P4 | `ship_utils.c` `update_maxspeed()` | The flying 1.2× bonus is applied twice — once into `ceil` and once into `maxspeed` — and the result is then clamped to `ceil`. The second application therefore only bites when the other multipliers are below 1.0. Possibly intentional, possibly a duplicated line; either way it is a balance question, not a defect. Documented inline. |
| C | P4 | `ship_cargo.c` `write_cargo()` | `strncat(dst, buf, ARRAY_SIZE(buf))` bounds the *source* read rather than the destination — the usual `strncat` misuse. Safe as written (`buf` is NUL-terminated within 1024 bytes and each destination has 16 KB), so it is left as-is rather than churned. |

### Verified NOT defects (documented so nobody re-audits them)

- `ShipTypeData::get_hull_mod()` indexes `hull_mod[_classid - 1]`. `_classid` is
  1-based (1..13) while `m_class` / `SHIP_CLASS()` is 0-based (0..12), so the
  `- 1` lands exactly on the table index. Confusing, not wrong.
- `calculate_relative_room()` (`src/world/map.c`) returns `real_room0()`, which
  yields 0 rather than `NOWHERE` for an unknown room, so the many unchecked
  `world[calculate_relative_room(…)]` reads in `getmap()` stay in bounds.
- `ship_identity.c` `ref.slot > MAXSHIPS` is correct: slots are 1-based
  (`index + 1`), so the guard admits 1..MAXSHIPS and subscripts `ref.slot - 1`.
- `crew_bonuses()` trailing `", "` → `"."` rewrite is safe for every input,
  including the empty-bonus case.
- `ship_shop.c` `reload_weapon`, `sell_slot`, `buy_weapon`, `buy_equipment`,
  `buy_hull`, `buy_swap`, and `ship_control.c` `weaponsight` all bound their
  `atoi()` input correctly.

### Dormant code (documented in place, not removed)

- `ship_auto.c` `shipgroupadd()` / `shipgroupremove()` — ship "groups" have no
  caller anywhere in the tree and no `shipai_data` ever gets a non-NULL
  `->group`. Both carry comment banners saying so.
- `ship_auto.c` `initialize_shipai()` and the file-static `autopilot` handle —
  never called, no header declaration.

## Documentation conventions adopted

- Each `.c` file opens with an `OVERVIEW` block: what the module owns, how it
  relates to its sibling modules, the lifecycle of the state it manages, and
  any coordinate/units conventions a reader needs before line 1 of real code.
- Every function gets a `/* … */` block above it stating what it does, what the
  parameters mean (including which may be NULL), what it returns, and any side
  effect a caller must know about.
- Non-obvious in-body decisions get short inline comments; obvious ones do not.
- `ReflowComments: false` is set in `.clang-format`, so hand-wrapped comment
  prose is preserved.

## Validation

- `make -C src` after every file (must stay exit 0 under `-Werror`).
- `clang-format --dry-run -Werror src/ships/*.c src/ships/*.h` after every file.
- Ship-related regression tests under `tests/async/`. All 13 pass as of the
  fixes above:
  `test_ship_cargo_txn`, `test_ship_nested_transaction`,
  `test_ship_owner_rename_failure`, `test_ship_rename_save_guards`,
  `test_ship_save_guards`, `test_ship_save_queue_dedup`,
  `test_ship_shop_list_contract`, `test_ship_shutdown_txn`,
  `test_nevent_ship_volley_runtime`, `test_redis_ship_snapshot_invalidation`,
  `test_auction_ship_txn_fixes`, `test_flatfile_ship_repository`,
  `test_flatfile_cargo_market`.

## Resume checklist

1. `git status` — expect only `src/ships/*` and this file modified.
2. `make -C src` to confirm the tree still builds.
3. Pick the next file with a `—` in the Scope table's *Documented* column.
4. Audit and document that file in one pass; land any fix it turns up
   immediately, verify, and add a row to *Fixed*.
5. Re-run the 13 ship tests before the final report.

Helper scripts used this session live in the session scratchpad
(`doc.py` inserts a comment block above a unique anchor line, `cover.py`
reports undocumented definitions, `verify.sh` runs clang-format + build).
They are throwaway tooling and are deliberately not checked in.
