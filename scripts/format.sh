#!/usr/bin/env bash
#
# Format C/C++ using the repository's .clang-format.
#
# The tree is fully clang-formatted.  By default only the lines you changed are
# touched, which keeps diffs reviewable; --all re-formats everything and is how
# the tree was brought into line in the first place.
#
#   ./scripts/format.sh                 # format your changed lines in place
#   ./scripts/format.sh --check         # report, change nothing (exit 1 if dirty)
#   ./scripts/format.sh --staged        # format staged lines in the index
#   ./scripts/format.sh --rev origin/master
#   ./scripts/format.sh --all           # every tracked C/C++ file, to a fixpoint
#   ./scripts/format.sh --file src/foo.c   # one whole file
#
# See docs/formatting.md.
set -euo pipefail

# Always run from the repository root so relative paths resolve correctly.
cd "$(dirname "$0")/.." || exit 1

MODE="worktree"
CHECK=0
REV=""
FILES=()

usage() {
  sed -n '3,15p' "$0" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --check)   CHECK=1 ;;
    --all)     MODE="all" ;;
    --staged|--cached) MODE="staged" ;;
    --rev=*)   REV="${1#*=}" ;;
    --rev)     REV="${2:?--rev needs a commit-ish}"; shift ;;
    --file=*)  MODE="files"; FILES+=("${1#*=}") ;;
    --file)    MODE="files"; FILES+=("${2:?--file needs a path}"); shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

if ! command -v clang-format >/dev/null 2>&1; then
  echo "ERROR: clang-format is not installed." >&2
  echo "       Debian/Ubuntu: sudo apt-get install clang-format" >&2
  exit 1
fi

# A malformed .clang-format makes clang-format fail on every file; catch it
# here with a clear message instead of once per source file.
if ! clang-format --dump-config -style=file:.clang-format >/dev/null 2>&1; then
  echo "ERROR: .clang-format is not valid for $(clang-format --version):" >&2
  clang-format --dump-config -style=file:.clang-format >/dev/null || true
  exit 1
fi

# --all: the whole tree.  clang-format is not idempotent on this codebase, so
# iterate until a pass stops changing anything rather than trusting one pass.
if [[ "$MODE" == "all" ]]; then
  mapfile -t ALL_FILES < <(git ls-files src src-migrate areas/src tests |
                             grep -E '\.(c|h|cpp|hpp|cc|hh)$')
  if (( ${#ALL_FILES[@]} == 0 )); then
    echo "ERROR: no tracked C/C++ files found." >&2
    exit 1
  fi

  if (( CHECK )); then
    rc=0
    for f in "${ALL_FILES[@]}"; do
      clang-format --style=file --dry-run --Werror "$f" 2>/dev/null || {
        echo "would reformat: $f"
        rc=1
      }
    done
    if (( rc == 0 )); then
      echo "Formatting OK: all ${#ALL_FILES[@]} tracked C/C++ files match .clang-format."
    else
      echo
      echo "Fix with: ./scripts/format.sh --all"
    fi
    exit "$rc"
  fi

  for pass in 1 2 3 4 5; do
    printf '%s\0' "${ALL_FILES[@]}" | xargs -0 -P "$(nproc)" -n 20 clang-format --style=file -i
    # Comparing `git diff --name-only` before and after is not sufficient:
    # an already-dirty file remains in both lists even when this pass changed
    # its contents.  Ask clang-format whether another pass would be a no-op.
    if printf '%s\0' "${ALL_FILES[@]}" |
         xargs -0 -P "$(nproc)" -n 20 clang-format --style=file --dry-run --Werror 2>/dev/null; then
      echo "Formatted ${#ALL_FILES[@]} files; stable after ${pass} pass(es)."
      exit 0
    fi
  done
  echo "WARNING: still changing after 5 passes; a construct may be oscillating." >&2
  echo "         Fence it with // clang-format off / on." >&2
  exit 1
fi

# --file: one whole file.
if [[ "$MODE" == "files" ]]; then
  if (( CHECK )); then
    rc=0
    for f in "${FILES[@]}"; do
      clang-format --style=file --dry-run --Werror "$f" || rc=1
    done
    exit "$rc"
  fi
  echo "WARNING: formatting whole files rewrites unrelated legacy lines." >&2
  clang-format --style=file -i "${FILES[@]}"
  echo "Formatted: ${FILES[*]}"
  exit 0
fi

if ! command -v git-clang-format >/dev/null 2>&1; then
  echo "ERROR: git-clang-format is missing (part of the clang-format package)." >&2
  exit 1
fi

ARGS=("--extensions" "c,h,cpp,hpp,cc,hh")
[[ "$MODE" == "staged" ]] && ARGS+=("--staged")
if (( CHECK )) || [[ "$MODE" == "staged" ]]; then
  ARGS+=("--diff")
else
  # Without --force, git-clang-format refuses to touch a file that has
  # unstaged edits -- i.e. exactly the file you just changed.
  ARGS+=("--force")
fi

if (( CHECK )); then
  # git-clang-format --diff prints the patch it would apply, or one of its
  # "nothing to do" messages, and exits 0 either way.
  out="$(git -c color.ui=false clang-format "${ARGS[@]}" ${REV:+"$REV"} || true)"
  case "$out" in
    ""|*"no modified files to format"*|*"clang-format did not modify any files"*)
      echo "Formatting OK: changed lines match .clang-format."
      exit 0
      ;;
  esac
  echo "$out"
  echo
  echo "Changed lines do not match .clang-format. Fix with: ./scripts/format.sh"
  exit 1
fi

# Formatting staged content through git-clang-format normally rewrites the
# working tree and then expects the user to run git add.  Applying its patch to
# the index instead lets the pre-commit hook auto-fix safely without staging or
# discarding unrelated worktree edits.  Mirror the patch into the worktree when
# it applies cleanly; otherwise leave those edits alone.
if [[ "$MODE" == "staged" ]]; then
  worktree_updated=1
  for pass in 0 1 2 3 4 5; do
    set +e
    out="$(git -c color.ui=false clang-format "${ARGS[@]}" ${REV:+"$REV"})"
    rc=$?
    set -e

    if (( rc != 0 && rc != 1 )); then
      echo "ERROR: git clang-format failed while formatting staged changes." >&2
      [[ -n "$out" ]] && echo "$out" >&2
      exit "$rc"
    fi

    case "$out" in
      ""|*"no modified files to format"*|*"clang-format did not modify any files"*)
        if (( pass == 0 )); then
          echo "Formatting OK: staged lines match .clang-format."
        else
          echo "Auto-formatted staged C/C++ changes; stable after ${pass} pass(es)."
          if (( ! worktree_updated )); then
            echo "Working tree left unchanged where formatting conflicted with unstaged edits." >&2
          fi
        fi
        exit 0
        ;;
    esac

    if (( rc != 1 )) || [[ "$out" != "diff --git "* ]]; then
      echo "ERROR: git clang-format could not produce a staged formatting patch." >&2
      [[ -n "$out" ]] && echo "$out" >&2
      exit 1
    fi
    if (( pass == 5 )); then
      echo "ERROR: staged formatting is still changing after 5 passes." >&2
      echo "       Fence an oscillating construct with // clang-format off / on." >&2
      exit 1
    fi

    if ! printf '%s\n' "$out" | git apply --cached --whitespace=nowarn; then
      echo "ERROR: could not apply clang-format changes to the Git index." >&2
      exit 1
    fi
    if (( worktree_updated )) &&
       ! printf '%s\n' "$out" | git apply --whitespace=nowarn >/dev/null 2>&1; then
      worktree_updated=0
    fi
  done
fi

# git-clang-format exits 1 when it reformatted something, which is a success
# for us; only a real failure should propagate.
set +e
out="$(git clang-format "${ARGS[@]}" ${REV:+"$REV"} 2>&1)"
rc=$?
set -e

echo "$out"
case "$out" in
  *"no modified files to format"*|*"did not modify any files"*|*"changed files"*)
    exit 0
    ;;
esac
exit "$rc"
