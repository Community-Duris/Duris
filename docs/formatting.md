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

The script wraps `git clang-format`, so it rewrites **only lines your diff
touches**. Run it before committing; `--check` is the form to use in a hook or
CI job.

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

## Do not mass-format

The legacy tree does not conform, and it is not supposed to. Reformatting
`src/comm.c` wholesale produces a ~3,500-line diff on a 4,200-line file. That
destroys `git blame`, buries real changes in review, and risks behavioral
surprises in macro-heavy code.

`./scripts/format.sh --file <path>` exists for the rare deliberate case (a new
file, or a module being rewritten anyway). It warns, and it should be its own
commit, never mixed with a behavior change.

## Editor setup

Point your editor at the repo's `.clang-format` and enable format-on-save for
*modified lines only* — whole-file format-on-save will mangle legacy files.

- **VS Code:** C/C++ or clangd extension, `"C_Cpp.formatting": "clangFormat"`,
  `"editor.formatOnSaveMode": "modifications"`.
- **Vim/Neovim:** `clang-format.py` mapped to a key, or `vim-clang-format` with
  `g:clang_format#detect_style_file = 1`.
- **Emacs:** `clang-format-region` on the region you edited.

## History

The file originally shipped with two keys no released clang-format
understands, `BreakAfterOpenBracketBracedList` and
`BreakBeforeCloseBracketBracedList`, which made *every* clang-format
invocation fail with `Error reading .clang-format: Invalid argument`. They were
removed, and the deprecated boolean spellings (`AlignConsecutiveAssignments:
true`, `AlignOperands: true`, ...) were replaced with the enum forms they map
to, so the config stays valid as clang-format retires the legacy aliases. The
resulting style is byte-identical on real source files.
