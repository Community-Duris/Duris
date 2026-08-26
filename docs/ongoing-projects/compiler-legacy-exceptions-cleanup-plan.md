# Compiler Legacy-Exception Cleanup Plan

- **Date:** August 26, 2026
- **Status:** In progress
- **Estimated Duration:** 12 working days, plus 2–3 contingency days if const propagation exposes
  behavior-sensitive interfaces
- **Target:** Remove every flag in `LEGACY_WARNING_EXCEPTIONS` from `src/Makefile` while preserving a clean
  C++20 `-Werror` build

---

## 1. Objective

The main build now treats a broad set of correctness and hardening diagnostics as fatal, but it retains six
global compatibility exceptions:

```make
-Wno-write-strings
-Wno-unused-parameter
-Wno-unused-variable
-Wno-unused-but-set-variable
-Wno-missing-field-initializers
-Wno-unused-function
```

This project removes those exceptions properly. The goal is not merely to make the diagnostics disappear;
each warning must be resolved by expressing intent, removing genuinely dead code, correcting broken data
flow, completing initialization, or making interfaces const-correct.

Completion means `LEGACY_WARNING_EXCEPTIONS` is empty or removed, a full clean build succeeds with all six
warning classes enabled under `-Werror`, all regression contracts pass, and no generated build artifacts are
committed.

## 2. Measured Baseline

A clean inventory build on August 26, 2026 enabled all six warning classes without making them fatal. It
linked successfully and produced the following deduplicated diagnostics:

| Warning class | Unique warnings | Affected files |
|---|---:|---:|
| `unused-parameter` | 4,670 | 171 |
| `write-strings` | 3,130 | 94 |
| `unused-variable` | 1,434 | 123 |
| `missing-field-initializers` | 546 | 46 |
| `unused-but-set-variable` | 160 | 60 |
| `unused-function` | 7 | 6 |
| **Total** | **9,947** | **199 compiler-reported files** |

The work is concentrated rather than evenly distributed. The highest-volume files are:

| File | Unique warnings across the six classes |
|---|---:|
| `src/magic.c` | 2,130 |
| `src/specs.mobile.c` | 585 |
| `src/actoff.c` | 461 |
| `src/smagic.c` | 417 |
| `src/innates.c` | 370 |
| `src/specs.object.c` | 335 |
| `src/ethermancer.c` | 295 |
| `src/fight.c` | 293 |
| `src/spells.c` | 277 |
| `src/psionics.c` | 273 |

The counts are compiler diagnostics, not 9,947 independent defects. Some central signature changes will
remove many call-site warnings at once. Conversely, a low-count data-flow warning can require more judgment
than hundreds of mechanical callback annotations.

## 3. Cleanup Rules

1. Remove one exception category at a time. A flag leaves `src/Makefile` only after a clean full build has
   zero instances of that category.
2. Do not introduce new blanket `-Wno-*` flags, warning pragmas, or file-wide suppressions.
3. Use `[[maybe_unused]]` only where an ABI, callback, interface, or build configuration genuinely requires
   the declaration to exist. Do not use it to hide unexplained dead state.
4. Do not use `const_cast` or C-style casts merely to satisfy `-Wwrite-strings`. Change the receiving API to
   `const char *` when it does not mutate input; copy into writable storage when it does.
5. Do not delete a set-but-unused calculation until its intended effect is understood. It may reveal a
   missing assignment, check, resource charge, or return value.
6. For aggregate warnings, preserve the current zero-initialization behavior unless the field's domain
   requires a different explicit default.
7. Keep changes narrow and format touched C/C++ lines with `./scripts/format.sh`.
8. Add or update a focused test whenever cleanup changes behavior or repairs data flow.
9. Use a development database and non-production port for any runtime test. The source-contract suite and
   compiler do not require a live database.
10. Commit in category-sized checkpoints only after the day's build and tests are green. Never commit
    `src/dms_new`, `obj/`, logs, or local environment data.

## 4. Standard Daily Verification

Every day ends with the smallest relevant tests plus these checks:

```bash
./scripts/format.sh
./scripts/format.sh --check
git diff --check -- src tests/async docs/ongoing-projects
make -C src clean
make -C src -j2
```

Run all standalone async contracts at category-removal checkpoints and on the final day. They cannot be
collected as one normal pytest suite because several are intentionally executable source-contract scripts:

```bash
failed=0
for test_file in tests/async/test_*.py; do
    python3 "$test_file" || failed=1
done
test "$failed" -eq 0
```

After verification, run `make -C src clean` so the worktree contains no build artifacts.

## 5. Daily Execution Plan

### Day 1 — Reproducible inventory and low-volume dead code

**Primary categories:** `unused-function`, initial `unused-but-set-variable` triage

- Add a non-production inventory helper that performs a clean build with selected legacy categories enabled
  as non-fatal warnings and reports deduplicated counts by category and file.
- Record the compiler version and full flag set in the inventory output so counts remain comparable.
- Classify all seven unused functions as one of: obsolete and removable, build-gated, callback registration
  defect, or intentionally retained.
- Remove obsolete functions; correctly gate or annotate functions that are intentionally configuration-only.
- Triage the 160 set-but-unused warnings into a ledger: dead assignment, missing consumer, feature-gated,
  diagnostic-only, or unclear.

**Exit gate:** `unused-function` reaches zero and its Makefile exception is removed. Every
`unused-but-set-variable` warning has an assigned disposition.

### Day 2 — Set-but-unused data-flow fixes

**Primary category:** `unused-but-set-variable` (160 warnings across 60 files)

- Review high-risk paths first: combat, spells, character creation, persistence, event scheduling, object
  lifecycle, and special procedures.
- Restore missing consumers where a calculated value was intended to affect damage, cost, cooldown, target,
  state, or return status.
- Remove assignments only when the value and all side effects are demonstrably dead.
- Move feature-only declarations and assignments inside the same preprocessor guard as their consumer.
- Add focused regression contracts for every behavior repair.

**Exit gate:** zero `unused-but-set-variable` warnings in a clean inventory build; remove
`-Wno-unused-but-set-variable`; run all async contracts.

### Day 3 — Define aggregate defaults centrally

**Primary category:** first half of `missing-field-initializers` (546 warnings across 46 files)

- Inventory the affected aggregate types and group warnings by type rather than editing call sites blindly.
- Establish explicit, domain-correct defaults for common structures such as damage messages, affects, command
  metadata, event records, and configuration tables.
- Prefer default member initializers or named factory helpers when one type accounts for many warnings.
- Confirm that zero, `NULL`, and sentinel values such as `NOWHERE` or `-1` are not interchangeable.
- Convert the highest-volume aggregate families and add compile/source contracts for important defaults.

**Exit gate:** all common aggregate types have reviewed defaults and the category count is reduced by at
least half without behavioral regressions.

### Day 4 — Complete aggregate initialization

**Primary category:** remaining `missing-field-initializers`

- Finish low-volume and area-specific aggregates.
- Use explicit trailing values where a shared type default would be misleading.
- Review nested aggregates and union-like legacy structures carefully; do not replace meaningful sentinel
  initialization with blanket `{}` unless equivalent.
- Run focused spell, affect, event, WebSocket, SQL, and object contracts for touched structures.

**Exit gate:** zero `missing-field-initializers` warnings; remove
`-Wno-missing-field-initializers`; complete clean build and full async suite.

### Day 5 — Unused variables in core runtime paths

**Primary category:** `unused-variable` in high-risk modules

- Audit `magic.c`, `fight.c`, `spells.c`, `psionics.c`, `act*.c`, `db.c`, `handler.c`, `comm.c`, persistence,
  and event-loop code.
- Treat each variable as evidence: determine whether it is obsolete, whether its consumer was accidentally
  removed, or whether it should influence a result.
- Delete genuinely dead declarations and their dead initialization expressions.
- Preserve calls with side effects even when their returned value is unused.
- Add tests for any restored logic.

**Exit gate:** no unexplained unused variables remain in core runtime, persistence, combat, or character
lifecycle code.

### Day 6 — Unused variables in feature and area modules

**Primary category:** remaining `unused-variable` (baseline total 1,434 across 123 files)

- Audit special procedures, area modules, ships, classes, guilds, crafting, and conditional features.
- Move configuration-specific locals under their relevant `#if` guards.
- Use `[[maybe_unused]]` only for declarations intentionally required in more than one build configuration.
- Perform at least one inventory build with the repository's normal `TEST_MUD`/`__NO_TESTS__` definitions and
  one compile-only variant for any configuration whose guards were changed.

**Exit gate:** zero `unused-variable` warnings; remove `-Wno-unused-variable`; clean build and full async suite.

### Day 7 — Callback parameter policy and spell interfaces

**Primary category:** first tranche of `unused-parameter` (4,670 warnings across 171 files)

- Document one consistent policy:
  - omit names in declarations when the name is not part of documentation;
  - use unnamed parameters in definitions when a fixed callback signature requires the slot;
  - use `[[maybe_unused]]` only when conditional compilation makes use vary by build;
  - remove parameters from private functions only when every caller can be changed safely.
- Apply the policy to `magic.c`, `smagic.c`, `spells.c`, `innates.c`, `psionics.c`, class spell modules, and
  their headers.
- Do not change public spell dispatch signatures or function-pointer compatibility.

**Exit gate:** spell/class modules are clean for `unused-parameter`, compile with exact function-pointer
types, and pass spell/affect regression contracts.

### Day 8 — Command, object, and special-procedure callbacks

**Primary category:** second tranche of `unused-parameter`

- Clean command handlers, object procedures, mobile procedures, room procedures, guild procedures, ship
  callbacks, and event callbacks.
- Prioritize `specs.mobile.c`, `actoff.c`, `specs.object.c`, `ethermancer.c`, and other high-count files.
- Verify registration tables and special-procedure casts; annotation must not disguise an incompatible
  callback type.
- Remove truly private unused parameters where doing so improves the interface without propagating churn
  through public dispatch tables.

**Exit gate:** command and special-procedure families compile cleanly with their registration contracts
intact.

### Day 9 — Finish parameter cleanup and remove the exception

**Primary category:** remaining `unused-parameter`

- Sweep lower-volume modules and headers.
- Check build-gated parameters under relevant configurations.
- Review the diff for meaningless annotations on non-callback business logic; those should be removed or
  repaired rather than excused.
- Run the inventory twice from a clean object directory to catch header or dependency-order omissions.

**Exit gate:** zero `unused-parameter` warnings; remove `-Wno-unused-parameter`; clean build and full async
suite.

### Day 10 — Const-correct infrastructure boundaries

**Primary category:** first tranche of `write-strings` (3,130 warnings across 94 files)

- Start with high-fan-out infrastructure: allocation/free metadata, `__FILE__` plumbing, memory tags, logging,
  formatting, immutable lookup helpers, command parsing inputs, and message-delivery APIs.
- Change parameters to `const char *` only after confirming the callee does not write through them.
- Where a callee tokenizes or edits input, retain a writable parameter and make the caller provide a mutable
  buffer.
- Propagate constness through declarations, definitions, function-pointer types, and wrappers as one coherent
  change.
- Add compile contracts around the most widely used APIs so writable parameters are not reintroduced.

**Exit gate:** central memory/logging/message boundaries are const-correct, and warning reduction demonstrates
that high-fan-out fixes removed call-site diagnostics rather than casting them away.

### Day 11 — Const-correct messages, tables, and spell APIs

**Primary category:** second tranche of `write-strings`

- Convert immutable message tables and string arrays from `char *` to `const char *`.
- Audit spell, damage-message, command metadata, race/class lookup, and special-procedure APIs.
- Separate immutable input from scratch buffers in functions that currently reuse one `char *` for both.
- Replace any legitimate writable string literal use with an owned array or copied buffer.
- Pay particular attention to static storage returned to callers: document lifetime and mutability rather than
  changing types mechanically.

**Exit gate:** all core spell, combat, lookup, and metadata modules compile with `-Wwrite-strings`; targeted
runtime/source contracts pass.

### Day 12 — Final const sweep, exception removal, and release-quality validation

**Primary category:** remaining `write-strings`; final integration

- Finish area-specific and low-volume const propagation.
- Remove `-Wno-write-strings`, then remove the now-empty `LEGACY_WARNING_EXCEPTIONS` variable from
  `src/Makefile`.
- Run a fresh warning inventory and require zero diagnostics in all six former exception categories.
- Run formatting, source diff checks, a full clean strict build, and all standalone async contracts.
- Perform a development-only smoke boot on a non-7777 port if configuration and development database access
  are available.
- Build with AddressSanitizer and UndefinedBehaviorSanitizer using a compile path that does not overwrite the
  runtime `dms` binary. `scripts/build-san.sh` currently copies `src/dms_new` to `dms`, so amend it or use an
  equivalent isolated build before this step.
- Update this document with final counts, defects found, tests added, validation results, and any deliberately
  retained local annotations.
- Clean all generated binaries and object files.

**Exit gate:** no legacy warning exceptions remain; clean C++20 `-Werror` build; 100% async contracts pass;
sanitizer build succeeds; worktree contains no build artifacts.

## 6. Contingency Days

Reserve 2–3 days rather than forcing risky const changes into the nominal schedule. Use them only for:

- const propagation through public function-pointer or callback interfaces;
- genuine bugs exposed by set-but-unused or unused-variable review;
- configuration-specific compile failures;
- sanitizer-only failures caused by cleanup;
- new focused regression coverage for behavior-sensitive fixes.

Contingency is not for weakening the compiler profile or introducing replacement suppressions.

## 7. Progress Tracking

Update this table at the end of each day from a clean inventory build:

| Category | Baseline | Current | Exception removed? |
|---|---:|---:|:---:|
| `unused-function` | 7 | 0 | **Yes** |
| `unused-but-set-variable` | 160 | 0 | **Yes** |
| `missing-field-initializers` | 546 | 0 | **Yes** |
| `unused-variable` | 1,434 | 0 | **Yes** |
| `unused-parameter` | 4,670 | 0 | **Yes** |
| `write-strings` | 3,130 | 3,077 | No |
| **Total** | **9,947** | **3,077** | — |

## 8. Execution Log

### Day 1 — inventory helper and `unused-function` (complete)

**Inventory helper.** `scripts/warning-inventory.sh` performs a clean, non-fatal build with the six legacy
categories enabled and writes `build/warning-inventory/{raw.log,dedup.txt,report.txt}`. The report records the
compiler version and the full flag set so counts stay comparable. `build/` is already git-ignored, so the
inventory leaves no tracked artifacts. It reproduced the documented 9,947 baseline exactly.

**`unused-function` dispositions (7 warnings, 6 files):**

| Site | Classification | Resolution |
|---|---|---|
| `actobj.c` `do_get_reject_out_of_sight` | extraction never adopted | Wired in: `do_get_try_container_item` had an inline byte-for-byte duplicate of the helper. Helper moved above its caller and the duplicate replaced; the `GETDBG_LOG` trace is preserved. |
| `actobj.c` `do_get_reject_too_much_stuff` | obsolete | Removed. Its message string ("That is too much stuff at once.") exists nowhere else and no bulk-get path ever rejected on it. |
| `actset.c` `ac_stringCopy` | obsolete | Removed. It was never forward-declared with the rest of the `ac_*` setter family and appears in no `set` dispatch table; it was also an unbounded `strcpy` into a struct at a caller-supplied offset. |
| `actwiz.c` `load_locker_char` | stale declaration | Declaration removed. The real function is `static` inside `storage_lockers.c`; `actwiz.c` could never have linked against it, and its only reference there is commented out. |
| `mobact.c` `mem_str_dup` | stale declaration | Declaration removed. Never defined, never called anywhere in the tree. |
| `comm.c` `check_section_time` | partially integrated upstream code | Removed. Introduced by upstream `81f4f813`, which also added five `game_loop` call sites; only the helper was carried into this tree, so the instrumentation it belongs to does not exist here. |
| `ws_handlers.c` `ws_cmd_request_wholist` | **registration defect** | Wired into `ws_handle_command` as `"request_wholist"`. The handler, its `ws_send_wholist_to_client` producer, and its `durisweb_verified` authorization check were all written but the dispatcher branch was never added, so the backend who-list request silently fell through to the "unknown = game cmd" path. |

**Behavioral defects found:** 1 (`ws_cmd_request_wholist` never dispatched). **Mechanical cleanups:** 6.

**Validation:** `./scripts/format.sh --check` clean; `git diff --check` clean; `make -C src clean && make -C src -j14`
succeeds with `-Wunused-function` fatal; all `tests/async/test_*.py` contracts pass.

Record behavioral defects separately from mechanical cleanups. The most important result of this project is
not the warning count reaching zero; it is making the compiler's future signal trustworthy without hiding
real mistakes in legacy-noise categories.

### Day 2 — `unused-but-set-variable` (complete)

All 160 warnings across 60 files are resolved and `-Wno-unused-but-set-variable` is removed.

**Dispositions used**

| Disposition | Count (approx.) | Notes |
|---|---:|---|
| Dead assignment (reader removed or commented out) | ~120 | Removed the declaration and its assignments; every call with side effects was kept. |
| Feature-gated | 3 | Moved the declaration and assignment inside the `#if` that holds their only consumer. |
| Format/interface slot | 12 | `files.c` pfile-header reads, marked `[[maybe_unused]]` with the reason; the reads must stay to keep the save-file cursor aligned. |
| Missing consumer, repaired | 3 | See defects below. |
| Unreachable code behind an early `return` | 2 | `do_artireset`, and the disabled `boon_random_maintenance` accumulator. |

**Behavioral defects found and repaired**

1. `sql_player.c` `sql_save_player_status` — the `snprintf` return for the player-save
   `INSERT`/`UPDATE` was discarded, so a query longer than the 16 KB buffer was silently truncated and
   handed to MySQL as malformed SQL. Now detected: the save logs the overflow, rolls back its own
   transaction if it opened one, and returns failure.
2. `artifact.c` `arti_player_sql` — `artifact player <name>` printed nothing at all when the query
   returned rows but every row was filtered out by `locType`. Restored the `!shownData` report that the
   sibling artifact listing already had.
3. `artifact.c` `event_artifact_wars` — read the `artifact.wars.modifier` property into a local that was
   never applied, and declared a `punishment` local that was never even assigned. The penalty is not
   implemented; the misleading property read is gone rather than left implying it does something.

**Unimplemented features made explicit rather than silently deleted**

- `nq.c`: the `<class>` and `<race>` quest handlers are empty stubs, so the `listedclasses` and
  `listedraces` allow/deny modes were parsed into locals nothing read. The stubs now say what a real
  handler has to do instead of carrying a bare `//!!!`.
- `epic.c`: `epic_stone_feed_artifacts` computed a feed amount, used it for nothing, and had no callers.
  Removed from both the source and `epic.h`.
- `boon.c`: `boon_random_maintenance` is disabled by a leading `return` and its `create_boon` call is
  still commented out. Documented, and its unread `id[]` collection dropped.
- `range.c`: `ITEM_RETURNING` set flags nothing consumed, leaving `if`/`else` branches already identical
  in effect. Collapsed with a note.

**Adjacent defects observed but not changed** (out of this project's scope; recorded so they are not lost)

- `actoth.c` `do_fly` and `do_swim` both test `if (!*buf)` before `buf` is initialized — `argument_interpreter`
  / `one_argument` fill it only afterwards. This reads uninitialized stack memory on every invocation.
- `sql_player.c` `sql_load_all_corpses` — the two `last_item_id = item_id; continue;` paths taken when an
  object fails to load leave `num_objs` unchanged, so a following affect row for the same item can match the
  "same item, another affect" branch and apply the affect to `obj_map[num_objs - 1]`, a different object.

**Validation:** `./scripts/format.sh --check` clean; `git diff --check` clean; `make -C src clean &&
make -C src -j14` succeeds with `-Wunused-but-set-variable` fatal; all `tests/async/test_*.py` pass.

**Tooling note:** `scripts/warning-inventory.sh` grew a companion scratch helper for this pass; the rule it
enforces is worth keeping in mind for the remaining categories — never delete an assignment statement whose
right-hand side calls anything but a known-pure helper. `generic_find()` in particular returns a bitmask that
most callers ignore while depending entirely on the character/object it writes through its out-parameters.

### Days 3-4 — `missing-field-initializers` (complete)

All 546 warnings across 46 files are resolved and `-Wno-missing-field-initializers` is removed. The work was
grouped by aggregate type rather than by call site, which is why it took far fewer edits than warnings.

| Aggregate | Warnings | Resolution |
|---|---:|---|
| `damage_messages` | 388 | Default member initializers on the type. Nearly every construction is a brace-initializer listing only the message strings it needs; `type` and `obj` were always left zero. The defaults now say so, and the type stays an aggregate so every existing positional initializer is unaffected. |
| `_flagDef` | 39 | Table-terminator rows written `{ 0 }` changed to `{}`, which is exactly equivalent and is not a partial initializer. |
| `setBitTable` | 14 | Default member initializers. Only rows carrying a subtable set the `entry_size`/`entry_offset` stride members. |
| `ferry_definition` (+ `stop_info`), `ctfData`, `BuildingType`, `epic_reward`, `boon_*`, `poison`, `potion`, `weapon_type`, `song_*`, `mine_range_data`, `continent`, `attr_names_struct`, and other one-off tables | ~90 | Terminator rows converted from `{ 0 }` / `{ 0, 0, 0 }` / `{ "\0" }` to `{}`. |
| `random_spells`, `material`, `transport_route`, `epic_teacher_skill` | ~13 | Default member initializers, with a comment naming the optional tail: no adjective, no preferred weapon, no waypoint list, no prerequisite level. |
| `innate_data`, `nq_interface_mapping` | 2 | Genuinely incomplete rows, filled in explicitly rather than defaulted: the "acid blood" innate has no handler, and the `quest load` command is immortal-only. |

Every default introduced is the value the omitted field already received under aggregate
initialization, so behavior is unchanged. No `{}` was substituted for a sentinel: the tables here terminate on
a zero/NULL first member, not on `NOWHERE` or `-1`.

**Defect surfaced by the change.** Giving `damage_messages` default member initializers makes it non-trivial,
and `-Wclass-memaccess` (already fatal in this build) immediately flagged six `memset(&msg, 0, sizeof(...))`
calls in `fight.c` and `studioproc.c`. They are now `msg = {}` value-initializations. This is the kind of
signal the project exists to restore: the compiler could not have pointed at those lines while the aggregate
was a bare C struct.

**Validation:** `./scripts/format.sh --check` clean; `git diff --check` clean; `make -C src clean &&
make -C src -j14` succeeds with `-Wmissing-field-initializers` fatal; all `tests/async/test_*.py` pass.

### Days 5-6 — `unused-variable` (complete)

All 1,434 warnings across 123 files are resolved and `-Wno-unused-variable` is removed.

Unlike the set-but-unused category, these are declarations with no reads *and* no writes, so the judgment is
narrower: the only real question is whether the declaration's initializer does work that must survive. The
sweep was therefore mechanised, with the tool refusing anything it could not prove safe.

**Method.** A scratch helper consumed the inventory's warnings (the inventory build now passes
`-fdiagnostics-column-unit=byte`, so each warning's column is an exact byte offset) and removed the named
declarator. It works one whole declaration statement at a time so that pointer decorations move with their
declarator and a statement whose declarators are *all* dead is dropped entirely. It scans a copy of the file
with comments and string literals blanked, so a `;` inside `/* ... */` cannot be mistaken for a statement end.
It refuses, and reports for manual handling, any declarator whose initializer calls a function not on an
explicit pure list, and any declaration inside `( )` — a `for`-init or an `if (T x = ...)` condition.

1,254 declarators were removed automatically over three passes; 12 were handled by hand:

- `magic.c`: `int duration = setup_pet(...)` — the call places the pet and must stay; only the variable went.
- `new_skills.c` (two sites): `if (P_char mount = get_linked_char(...))` where the body's only use of `mount`
  is commented out. Rewritten as a plain condition rather than annotated.
- `assocs.c`, `magic.c`, `mobcombat.c`: declarators the tool declined on a false "inside ( )" reading.
- Five file-scope statics with no reader anywhere in the tree: a shadowed `buf` in `affects.c`, a shadowed
  `aliaslist` in `drannak.c`, `songcounter` in `specs.mobile.c`, `recipefile` in `tradeskill.c`, and the
  65-line halfling social-thievery `steal_chance` table in `innates.c`.

**Contract updated.** `tests/async/test_crafting_enhancement_regressions.py` pinned the exact text
`int minval = itemvalue(source) - enhance_material_ival_delta;`, and that text lived in `modenhance()`, where
the value is computed and then never compared against anything. The tunable it exists to protect is used in
`enhance()`. The contract now pins the live computation *and* its comparison, which is a stronger assertion
than before.

**Behavioral difference recorded, deliberately not changed:** `enhance()` rejects a material whose item value
is below `itemvalue(source) - enhance_material_ival_delta`; `modenhance()` computes the same floor and
enforces nothing. Restoring the check would start rejecting materials that are accepted today, which is a
balance decision rather than a cleanup.

**Validation:** `./scripts/format.sh --check` clean; `git diff --check` clean; `make -C src clean &&
make -C src -j14` succeeds with `-Wunused-variable` fatal; all `tests/async/test_*.py` pass.

### Days 7-9 — `unused-parameter` (complete)

All 4,670 warnings across 171 files are resolved and `-Wno-unused-parameter` is removed.

**The policy, and why it is safe here.** Parameter names are not part of a function's type, so nothing done in
this category can change function-pointer compatibility with a dispatch table. Two outcomes per parameter:

1. **The body never mentions the name.** Drop the name and keep it visible as a comment:
   `P_obj /*obj*/`. 4,337 parameters.
2. **The body does mention the name**, which means its only use sits inside an `#if` that is inactive in this
   build. Keep the name and mark it `[[maybe_unused]]`, so the other configuration still compiles.
   329 parameters, plus 7 in two macros (below).

That second rule is the load-bearing one. A first attempt without it unnamed `email` in
`account.c:is_email_taken()`, whose only use is inside `#ifdef REQUIRE_EMAIL_VERIFICATION` — which would have
broken that build silently, since this configuration never compiles it.

**Why the slots stay.** The distribution shows the category is almost entirely fixed dispatch signatures:

| Signature | Functions |
|---|---:|
| `(int, P_char, char *, int, P_char, P_obj)` — spell dispatch, `skills[].spell_pointer` | 686 |
| `(P_char, char *, int)` — command handlers | 510 |
| `(P_char, P_char, int, char *)` — mobile and room special procedures | 455 |
| `(P_char, P_char, P_obj, void *)` — event callbacks | 209 |
| `(P_obj, P_char, int, char *)` — object special procedures | 149 |
| `(void *, int, char *, int, int)` — `actset.c` `ac_*` setters, held in `setBitTable::sb_func` | 18 |
| `(descriptor_data *, cJSON *)` — WebSocket command handlers | 9 |

**Two macros needed the annotation instead.** `interp.h`'s `ACMD(c)` and `dam_mods.h`'s
`MAKE_DAM_MOD_PRED()` each expand into many bodies, some of which read a given parameter and some of which do
not. There is no single unnaming that is correct for every expansion, so their slots carry `[[maybe_unused]]`
with a comment saying why. This is the conditional-use case rule 3 exists for.

**Diff reviewed for meaningless annotations.** Every `static` (file-private) function that gained an unnamed
parameter was checked by hand — 17 of them. All 17 are genuinely shape-bound: eleven `ac_*Copy` setters held
in `setBitTable`, `master_set_adapter` held in the `sets[]` table, two event callbacks, and three members of
uniform private families (`locker_*cmd(P_char, char *arg)` and
`crafting_handle_*_command(P_char, char *, int cmd)`) whose siblings do use the slot. None was a private
helper that should simply have lost the parameter.

**Configuration checks.** Beyond the normal `TEST_MUD` / `__NO_TESTS__` build, every `.c` was compiled
`-fsyntax-only` with `REQUIRE_EMAIL_VERIFICATION`, `CTF_MUD=1`, and `SIEGE_ENABLED` in turn, requiring zero
unused-parameter, unused-variable, or set-but-unused diagnostics in each. All clean.

**Contracts updated.** Three source-contract tests pinned a function's full signature text, which now carries
`/*name*/` on its unused slots: `test_auction_persistence.py` (`finalize_auction`),
`test_event_loop_hotspots.py` (`generic_char_event`), and `test_eqrate_contract.py` (`do_eqrate`). Each now
matches on the function name and its used parameters, so it still anchors the right function without being
brittle about the unused ones.

**Functions worth a second look** (found while reviewing, deliberately unchanged — each ignores an argument
its caller still supplies): `sql_link_player_to_account()` is a `// todo: implement` stub returning `false`;
`quested_spell(ch, spl)` ignores the spell; `language_known(ch, vict)` ignores both characters;
`createSetItem`, `createUniqueItem`, `create_material`, `create_stones`, and `get_gem_from_mine` ignore their
difficulty or character arguments.

**Validation:** `./scripts/format.sh --check` clean; `git diff --check` clean; `make -C src clean &&
make -C src -j14` succeeds with `-Wunused-parameter` fatal; all `tests/async/test_*.py` pass.
