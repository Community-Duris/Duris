#!/usr/bin/env bash
#
# Format C/C++ using the repository's .clang-format.
#
# Only the lines you actually changed are touched.  The legacy tree does not
# conform to .clang-format and must not be mass-reformatted: a whole-file pass
# rewrites thousands of lines and destroys `git blame`.
#
#   ./scripts/format.sh                 # format your changed lines in place
#   ./scripts/format.sh --check         # report, change nothing (exit 1 if dirty)
#   ./scripts/format.sh --staged        # only what is staged
#   ./scripts/format.sh --rev origin/master
#   ./scripts/format.sh --file src/foo.c   # whole file; opt-in, rarely correct
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

# --file: whole-file formatting.  Deliberately noisy; this is not the normal path.
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
if (( CHECK )); then
  ARGS+=("--diff")
else
  # Without --force, git-clang-format refuses to touch a file that has
  # unstaged edits -- i.e. exactly the file you just changed.
  ARGS+=("--force")
fi

if (( CHECK )); then
  # git-clang-format --diff prints the patch it would apply, or one of its
  # "nothing to do" messages, and exits 0 either way.
  out="$(git clang-format "${ARGS[@]}" ${REV:+"$REV"} || true)"
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
