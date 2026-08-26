# Compiler Legacy-Exception Cleanup Plan

**Date:** August 26, 2026  
**Status:** Planned  
**Estimated Duration:** 12 working days, plus 2–3 contingency days if const propagation exposes
behavior-sensitive interfaces  
**Target:** Remove every flag in `LEGACY_WARNING_EXCEPTIONS` from `src/Makefile` while preserving a clean
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
| `unused-function` | 7 | 7 | No |
| `unused-but-set-variable` | 160 | 160 | No |
| `missing-field-initializers` | 546 | 546 | No |
| `unused-variable` | 1,434 | 1,434 | No |
| `unused-parameter` | 4,670 | 4,670 | No |
| `write-strings` | 3,130 | 3,130 | No |
| **Total** | **9,947** | **9,947** | — |

Record behavioral defects separately from mechanical cleanups. The most important result of this project is
not the warning count reaching zero; it is making the compiler's future signal trustworthy without hiding
real mistakes in legacy-noise categories.
