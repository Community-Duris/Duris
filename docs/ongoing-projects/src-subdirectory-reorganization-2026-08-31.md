# src/ subdirectory reorganization

Status: plan, not started. Date: 2026-08-31.

## Goal

Move every file in `src/` into a topical subdirectory, following the pattern
`src/ships/` already establishes.

## Scope

`src/` root holds 632 files (356 `.c`, 276 `.h`), excluding the 15 already in
`src/ships/`. All 632 are assignable to a subdirectory by name; none required
reading the file to classify.

## What makes this cheap

- **The build already supports it.** The Makefile's rules are generic:
  `$(OBJDIR)/%.o: %.c` with `mkdir -p $(dir $@)`, and `DEPFILES` derives from
  the object list. `ships/` needed only `SHIP_OBJS = $(SHIP_SRCS:%.c=ships/%.o)`.
  Each moved file costs one edited line in `OBJS` (`foo.o` -> `sub/foo.o`).
- **Naming already encodes the layout.** Existing prefixes (`flatfile_`,
  `specs.`, `redis_`, `player_`, `sql_`, `critical_`, `persistence_`, `item_`,
  `ws_`, `vnum.`) map directly to directories.

## What makes it expensive

240 files under `tests/`, `scripts/`, and `.github/` hardcode `src/<name>.c`
paths (286 distinct paths). There is no shared path helper: `ROOT =
Path(__file__).resolve().parents[2]` is copy-pasted 298 times and `SRC = ROOT /
"src"` 49 times. Every move breaks source-contract tests until this is fixed.

## Proposed layout

| Directory | Files | Contents |
|---|---:|---|
| `flatfile/` | 78 | `flatfile_*` |
| `world/` | 74 | `db`, `zone_touch_*`, `world_*`, `weather`, `random.*`, `vnum.*`, `map`, `graph`, `quest`, `events`, `epic*`, `handler`, terrain/rooms |
| `specs/` | 54 | `specs.*` |
| `economy/` | 46 | `shop*`, `auction*`, `boon*`, `currency*`, `mining*`, `crafting`, `tradeskill`, `cardgames` |
| `persistence/` | 42 | `persistence_*`, `critical_*`, `maintenance_*`, `corpse_lifecycle_*`, save policy |
| `redis/` | 41 | `redis*`, `wizredis` |
| `core/` | 39 | `structs.h`, `prototypes.h`, `utils.h`, `defines.h`, `types.h`, `config.h`, `constant`, `utility`, `mm`, `safe_*`, `json_utils` |
| `combat/` | 35 | `fight`, `damage`, `dam_mods`, `combat_outcome_*`, `arena`, `ctf`, `chaos*`, `siege`, `justice`, `guard`, `frag*` |
| `classes/` | 33 | class modules, `skills`, `specializations`, `innates`, `memorize` |
| `item/` | 32 | `item_*`, `obj*`, `rand*`, `forge_items`, `salvage`, `enhance`, `storage_lockers`, `trophy` |
| `player/` | 28 | `player_*` |
| `net/` | 27 | `comm`, `websocket`, `ws_*`, `gmcp`, `mccp`, `telnet`, `ttype`, `ansi`, `unicode`, `ssl`, `editor` |
| `guild/` | 25 | `guild*`, `artifact*`, `alliances`, `assocs` |
| `account/` | 25 | `account*`, `password_hash`, `session_audit_*`, `nanny`, `pfile*` |
| `cmd/` | 21 | `act*.c`, `interp`, `boards`, `mail`, `wikihelp`, `testcmd` |
| `mob/` | 12 | `mobact`, `mobconv`, `mobpatrol`, `specials`, `studioproc*`, `encounters` |
| `sql/` | 10 | `sql*` |
| `magic/` | 8 | `magic`, `smagic`, `spells`, `blispells`, `beh_magic`, `affects` |
| `test/` | 2 | `test_async.*` |
| `ships/` | 15 | unchanged |

Alternative: leave the 39 `core/` files in `src/` root. One-line difference in
execution; decide before starting.

## Execution order

1. **Add `tests/async/_paths.py`** exposing `repo_root()` and `source(name)`,
   which resolves a bare filename against `src/` and its subdirectories.
   Convert the 240 hardcoded callers. Worth doing on its own merits; after
   this, moves stop breaking tests.
2. **Move one directory per commit**, largest and most mechanical first
   (`flatfile/`, `specs/`, `redis/`, `player/`, `sql/`), then the judgment-heavy
   ones. Each commit: `git mv`, update `OBJS` paths, update `#include` lines,
   `make -C src`, run the affected tests.
3. **Build `src-migrate` explicitly after each batch.** It is excluded from the
   default `make`, so a moved header that breaks it will not surface otherwise.

## Conventions

- **Directory-qualified includes only** (`#include "flatfile/foo.h"`). Do not
  add a `-I` per subdirectory. `ships/` currently does both — `-I./ships` in
  `INCLUDES` plus `ships/ships.h` at call sites; drop the `-I` when convenient.
  Qualified includes keep ownership greppable and prevent header-name
  collisions between subdirectories.
- **Do not rename while moving.** `flatfile/flatfile_account_repository.c` is
  redundant but harmless. Stripping prefixes on top of the move doubles the
  blast radius and breaks grep continuity mid-migration. Prefix removal is a
  separate, optional, later pass.
- Use plain `git mv`; history follows via `git log --follow` and `blame -C`.

## Verification per commit

- `make -C src` clean under the existing `-Werror` profile.
- `make -C src src-migrate` (or equivalent explicit target).
- `python3 tests/async/test_<affected>.py` for tests touching the moved files.
- `./scripts/format.sh --check` on touched lines.
