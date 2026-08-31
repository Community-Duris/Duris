# src/ subdirectory reorganization

Status: complete. Date: 2026-08-31. Branch: `src-subdirectory-reorg`.

All 633 files moved into 18 subdirectories; `src/` root now holds only
`Makefile`. Server builds clean; regression suite 373/373, identical to the
pre-move baseline. `core/` moved into `src/core/` rather than staying in root.

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

Decided: `core/` moved into `src/core/` like everything else. `src/` root
holds only `Makefile` and `.gitignore`.

## Execution order (as performed)

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

## Outcome

- 20 commits: one path-helper refactor, 18 directory moves, one reference sweep.
- `make -C src` clean under the existing `-Werror` profile after every move.
- Regression suite 373/373 before and after.
- No file lost: the set of tracked `src/` basenames is byte-identical to master
  (650 files).

## Deviations from the plan

- The reference sweep needed four passes beyond the helper, for forms the plan
  did not anticipate: inline C++ harness text embedded in tests, `os.path.join`
  constructions, evidence paths inside `tests/async/*.json`, and assertions
  pinning bare header names.
- `-I./ships` was dropped as planned, which exposed 34 files still using the
  unqualified `#include "ships.h"` spelling. All are now directory-qualified.

## Pre-existing problems surfaced

Both were unrelated to the move and failed identically on `master`. Neither
target is in the default build, which is why both rotted unnoticed. Both are
now fixed -- see "Auxiliary build targets" below.

- `make -C src pfile` failed with 751 `-Werror` errors, then failed to link.
- `make -C src migrate_pfiles` failed with 74. Its `../src/*.h` includes were
  repointed at the new layout, so it failed on code rot rather than missing
  files.

## Auxiliary build targets

`src-migrate/` moved to `migrations/tools/`, so the offline converters sit
beside the SQL and shell migrations they serve and out of the server's
source-contract sweep (`tests/async/_paths.py` indexes `src/**`).

| Binary | Build with | Status |
| --- | --- | --- |
| `bin/server/dms_new` | `make -C src` | clean |
| `bin/tools/pfile` | `make -C src pfile` | clean |
| `bin/migrations/migrate_pfiles` | `make -C migrations/tools` | clean |
| `bin/migrations/migrate_locker_affects` | `make -C migrations/tools affects` | clean |
| `bin/migrations/pfile_converter` | `make -C migrations/tools pfile_converter` | clean |

`src/Makefile` used to carry a second, divergent `migrate_pfiles` target that
compiled `../src-migrate/*.c` with the server's strict profile. It had drifted
(it omitted `migrate_shopkeepers.o`) and had not built in a long time.
`migrations/tools/Makefile` is now the single entry point for those binaries.

### What `pfile` needed

- `classes/skills.c`: one macro declared `int i`, shadowing the `i` in
  `initialize_skills()` -- 686 of the 751 errors came from that single line.
- `core/structs.h`: the `Skill` typedef was hidden from `_PFILE_`, so
  `skills[]` had no type; the underlying `struct s_skill` was always visible.
- The `_PFILE_` arm had no `m_class` variants of `SPELL_ADD`/`SPEC_SPELL_ADD`.
- `account/pfile-stubs.c` gained 32 link stubs. `files.c` and `skills.c` had
  grown references to the wider runtime (SQL loaders, flatfile deletion,
  persistence mode, guilds) that the offline scanner never reaches.

### Data fix

`SKILL_ADD(CLASS_ASSASSIN, 20, 900)` in `classes/skills.c` overflowed the
byte-wide `maxlearn` field (900 wraps to -124). Neighbouring rogue-family
entries use 90, so it now reads 90. The server's `SKILL_ADD` discards its
`MaxLearn` argument entirely, so this changes no server behaviour -- only the
`_PFILE_` arm, which is the only one that stores it.

## Resolved after the move

- `src/vc140.pdb`, a 54KB committed MSVC debug database, violated the
  `AGENTS.md` rule that compiled artifacts live under `bin/` and stay out of
  the tree. Initially deferred as out of scope, it was removed in commit
  `f5f77dd` along with a `*.pdb` entry in `src/.gitignore` so it cannot return.
