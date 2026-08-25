# Code formatting

`.clang-format` at the repository root is authoritative for C/C++. It is a
Linux-kernel/git-derived style:

| Setting | Value |
| --- | --- |
| Indentation | hard tabs, width 8 (`UseTab: Always`) |
| Braces | Allman (`BreakBeforeBraces: Allman`) |
| Line length | 100 columns |
| Pointers | bound to the name — `char *p` |
| Includes | never reordered (`SortIncludes: false`) |
| Comments | never reflowed (`ReflowComments: false`) |
| Language standard | `c++20`, matching `src/Makefile` |

`ForEachMacros` teaches clang-format about `LOOP_THRU_PEOPLE`,
`LOOP_EVENTS_CH`, and `LOOP_EVENTS_OBJ`, so those are spaced like control
statements rather than calls.

## Requirement

clang-format 14 or later (18 is what this tree is validated against):

```bash
sudo apt-get install clang-format    # Debian/Ubuntu; ships git-clang-format too
```

It is also in `packaging/duris-build-deps.equivs`.

## Formatting your changes

```bash
./scripts/format.sh                    # format the lines you changed, in place
./scripts/format.sh --check            # report only; exit 1 if anything is off
./scripts/format.sh --staged           # only what is staged
./scripts/format.sh --rev origin/master
```

Without `--all`, the script wraps `git clang-format`, so it rewrites **only
lines your diff touches**. Run it before committing; `--check` is the form to
use in a hook or CI job.

## Pre-commit hook

```bash
./scripts/install-hooks.sh              # once per clone
./scripts/install-hooks.sh --uninstall  # revert
```

This sets `core.hooksPath` to the versioned `scripts/git-hooks/`, whose
`pre-commit` rejects a commit when the staged C/C++ lines do not match
`.clang-format`. It prints the offending diff and how to fix it. Hooks are
per-clone git config, so each checkout runs the installer once.

- Bypass a single commit with `git commit --no-verify`.
- If clang-format is not installed, the hook warns and lets the commit
  through rather than blocking work.
- `core.hooksPath` replaces `.git/hooks` wholesale; the installer warns if you
  already have hooks there.

## The whole tree is formatted

Every tracked C/C++ file under `src/`, `src-migrate/`, `areas/src/`, and
`tests/async/` matches `.clang-format`, and `--all --check` verifies that in
about 12 seconds:

```bash
./scripts/format.sh --all          # re-format everything, iterated to a fixpoint
./scripts/format.sh --all --check  # verify the tree; exit 1 and name offenders
```

Day to day you still want the default (changed lines only) — a full pass in a
feature branch buries the real change in review. Reformat-only work belongs in
its own commit, never mixed with a behavior change.

### Things that fight the formatter

- **clang-format is not idempotent here.** A second pass can rewrite more than
  the first. `--all` iterates until a pass changes nothing, and fails loudly if
  five passes are not enough.
- **One statement oscillates forever.** `do_vote()` in `src/actnew.c` has
  literal tab characters inside an `fprintf` format string, which breaks
  clang-format's column arithmetic. It is fenced with `// clang-format off`.
- **Constant defines with long trailing comments.** clang-format wrapped 23 of
  them so the *value* landed on a backslash continuation line
  (`#define MAX_TRADE \` / `16 /* ... */`), which is unreadable and breaks
  every tool that scans headers for `#define NAME VALUE`. Their explanations
  now precede the one-line defines. Move a long comment above its define; fence
  the region only when restructuring is not practical.

When you hit a construct the formatter mangles, fence it the same way and say
why in a comment. Do not disable the option globally.

## Editor setup

Point your editor at the repo's `.clang-format` and enable format-on-save for
*modified lines only* — whole-file format-on-save will mangle legacy files.

- **VS Code:** C/C++ or clangd extension, `"C_Cpp.formatting": "clangFormat"`,
  `"editor.formatOnSaveMode": "modifications"`.
- **Vim/Neovim:** `clang-format.py` mapped to a key, or `vim-clang-format` with
  `g:clang_format#detect_style_file = 1`.
- **Emacs:** `clang-format-region` on the region you edited.

## History

The config shipped with two keys no released clang-format understands,
`BreakAfterOpenBracketBracedList` and `BreakBeforeCloseBracketBracedList`,
which made *every* clang-format invocation fail with
`Error reading .clang-format: Invalid argument`. Removing them exposed two more
problems, both found by actually formatting the tree:

- `RemoveBracesLLVM: true` stripped the braces from single-statement
  `if`/`else`. The `ADD_BYTE`/`ADD_SHORT` macros in `files.h` expand to a brace
  block that is *not* `do { } while (0)` wrapped, so `if (x) ADD_BYTE(...);
  else ...` became `if (x) { ... }; else` and stopped compiling.
- `AlignConsecutiveAssignments`/`Declarations` padded columns with tabs (they
  only line up at tab width 8), and clang-format chose different columns for a
  whole-file pass than for the line ranges `git clang-format` formats — so the
  hook rejected a fully formatted tree. Both are off now, as in the kernel,
  LLVM, and Google styles.

The deprecated boolean spellings (`AlignOperands: true`, ...) were also
replaced with the enum forms they map to, so the config stays valid as
clang-format retires the legacy aliases.

Removing the alignment padding had one side effect worth knowing: several
tests used to locate a function definition by its bare signature, and only
matched the definition because the *forward declaration* was column-padded and
therefore spelled differently. Anchor on `signature\n{` instead.
