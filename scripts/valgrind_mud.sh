#!/usr/bin/env bash
#
# Run the Duris server under Valgrind.
#
#   ./scripts/valgrind_mud.sh                 # memcheck on port 4000
#   ./scripts/valgrind_mud.sh --tool=helgrind # data-race hunt
#   ./scripts/valgrind_mud.sh --build --port 4321 -- --fair-sched=yes
#
# See docs/valgrind.md.
set -euo pipefail

# Always run from the repository root so relative paths resolve correctly.
cd "$(dirname "$0")/.." || exit 1
ROOT="$PWD"

TOOL="memcheck"
PORT="4000"
BUILD=0
GEN_SUPP=0
TRACE_CHILDREN="no"
EXTRA=()

usage() {
  cat <<'USAGE'
Usage: scripts/valgrind_mud.sh [options] [-- valgrind-args...]

Options:
  --tool=TOOL         memcheck (default), helgrind, drd, massif, callgrind
  --port N            port to bind (default 4000; 7777 is refused)
  --build             run `make -C src` and refresh ./dms first
  --gen-suppressions  emit ready-to-paste suppression blocks for every error
  --trace-children    follow exec() (copyover); off by default
  -h, --help          this message

Everything after `--` is passed through to valgrind verbatim.
Reports land in logs/valgrind/.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tool=*)           TOOL="${1#*=}" ;;
    --tool)             TOOL="${2:?--tool needs a value}"; shift ;;
    --port=*)           PORT="${1#*=}" ;;
    --port)             PORT="${2:?--port needs a value}"; shift ;;
    --build)            BUILD=1 ;;
    --gen-suppressions) GEN_SUPP=1 ;;
    --trace-children)   TRACE_CHILDREN="yes" ;;
    -h|--help)          usage; exit 0 ;;
    --)                 shift; EXTRA=("$@"); break ;;
    *)                  echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

if ! command -v valgrind >/dev/null 2>&1; then
  echo "ERROR: valgrind is not installed." >&2
  echo "       Debian/Ubuntu: sudo apt-get install valgrind" >&2
  exit 1
fi

case "$TOOL" in
  memcheck|helgrind|drd|massif|callgrind) ;;
  *) echo "ERROR: unsupported tool '$TOOL'." >&2; exit 2 ;;
esac

if ! [[ "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1024 || PORT > 65535 )); then
  echo "ERROR: --port must be a number between 1024 and 65535." >&2
  exit 2
fi

# Port 7777 is DFLT_PORT (src/config.h); src/sql.c only redirects to the
# development database when RUNNING_PORT != DFLT_PORT, so 7777 means live
# player data.  A Valgrind run is 20-50x slower than native and is expected to
# be killed mid-session, so it must never touch production.
if (( PORT == 7777 )); then
  echo "ERROR: port 7777 is the production port/database; refusing to run." >&2
  echo "       Pick a development port, e.g. --port 4000." >&2
  exit 2
fi

if (( BUILD )); then
  echo "Building src/dms_new..."
  make -C src
fi

if [[ -f src/dms_new ]] && { (( BUILD )) || [[ ! -f dms ]] || [[ src/dms_new -nt dms ]]; }; then
  cp -f src/dms_new dms
fi

if [[ ! -x dms ]]; then
  echo "ERROR: ./dms not found. Run with --build, or 'make -C src && cp src/dms_new dms'." >&2
  exit 1
fi

# The game opens logs/log/* with fopen(), which fails silently when the
# directory is missing.
mkdir -p logs/log logs/valgrind

STAMP="$(date +%Y%m%d-%H%M%S)"
LOG="logs/valgrind/${TOOL}-${STAMP}.log"

# -g is already in src/Makefile CFLAGS with no -O, so ./dms carries the full
# debug info Valgrind needs for readable stacks.
COMMON=(
  "--tool=$TOOL"
  "--log-file=$ROOT/$LOG"
  "--suppressions=$ROOT/scripts/valgrind.supp"
  "--num-callers=40"
  "--error-limit=no"
  "--time-stamp=yes"
  "--trace-children=$TRACE_CHILDREN"
)

case "$TOOL" in
  memcheck)
    COMMON+=(
      "--leak-check=full"
      "--show-leak-kinds=definite,indirect"
      "--errors-for-leak-kinds=definite,indirect"
      "--track-origins=yes"
      "--track-fds=yes"
      "--keep-stacktraces=alloc-and-free"
    )
    ;;
  helgrind)
    COMMON+=("--history-level=full" "--free-is-write=yes")
    ;;
  drd)
    COMMON+=("--check-stack-var=yes")
    ;;
  massif)
    COMMON+=("--massif-out-file=$ROOT/logs/valgrind/massif-${STAMP}.out" "--detailed-freq=5")
    ;;
  callgrind)
    COMMON+=("--callgrind-out-file=$ROOT/logs/valgrind/callgrind-${STAMP}.out")
    ;;
esac

if (( GEN_SUPP )); then
  COMMON+=("--gen-suppressions=all")
fi

# The MUD ignores SIGPIPE and reboots itself on some exit codes; under
# Valgrind we want a single, plain, foreground run instead.
echo "Running: valgrind --tool=$TOOL ./dms $PORT"
echo "Report:  $LOG"
echo "Expect the game to boot roughly 20-50x slower than normal."
echo

set +e
valgrind "${COMMON[@]}" ${EXTRA[@]+"${EXTRA[@]}"} ./dms "$PORT"
RESULT=$?
set -e

echo
echo "dms exited with code ${RESULT}; report written to ${LOG}"
if [[ "$TOOL" == "memcheck" ]]; then
  echo "Summary:"
  grep -E "ERROR SUMMARY|definitely lost|indirectly lost|Open file descriptor" "$LOG" | sed 's/^/  /' || true
fi
exit "$RESULT"
