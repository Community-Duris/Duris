#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEMPLATE="$ROOT/deploy/systemd/duris-mud-production.service.in"
SERVICE_NAME="duris-mud-production.service"
RUN_USER="$(stat -c '%U' "$ROOT")"
RUN_GROUP=""
RENDER_ONLY=0
ENABLE_SERVICE=1
START_SERVICE=0

usage() {
  cat <<'EOF'
Usage: scripts/install-production-service.sh [options]

Options:
  --user USER    Account that owns and runs the Duris checkout.
  --root PATH    Absolute Duris checkout path (defaults to this checkout).
  --render       Print the rendered unit without installing it.
  --no-enable    Install the unit without enabling it at boot.
  --start        Enable and start (or restart) the service after installation.
  --help         Show this help.

Installation writes /etc/systemd/system/duris-mud-production.service and
therefore must run as root. Enabling or starting first requires the target
account's .env to pass the explicit production configuration check.
EOF
}

while (( $# > 0 )); do
  case "$1" in
    --user)
      [[ $# -ge 2 ]] || { echo "--user requires a value" >&2; exit 2; }
      RUN_USER="$2"
      shift
      ;;
    --root)
      [[ $# -ge 2 ]] || { echo "--root requires a value" >&2; exit 2; }
      ROOT="$2"
      shift
      ;;
    --render)
      RENDER_ONLY=1
      ;;
    --no-enable)
      ENABLE_SERVICE=0
      ;;
    --start)
      START_SERVICE=1
      ENABLE_SERVICE=1
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

ROOT="$(realpath -e "$ROOT")"
TEMPLATE="$ROOT/deploy/systemd/duris-mud-production.service.in"
[[ "$ROOT" == /* && -d "$ROOT" ]] || { echo "Invalid checkout root: $ROOT" >&2; exit 1; }
[[ "$ROOT" != *[[:space:]\\]* ]] || {
  echo "Checkout path contains whitespace or backslashes unsupported by systemd" >&2
  exit 1
}
[[ -r "$TEMPLATE" ]] || { echo "Missing service template: $TEMPLATE" >&2; exit 1; }
[[ -x "$ROOT/scripts/cycle_mud.sh" ]] || {
  echo "Missing executable production launcher: $ROOT/scripts/cycle_mud.sh" >&2
  exit 1
}
id "$RUN_USER" >/dev/null 2>&1 || { echo "Unknown service account: $RUN_USER" >&2; exit 1; }
RUN_GROUP="$(id -gn "$RUN_USER")"
[[ "$(stat -c '%U' "$ROOT")" == "$RUN_USER" ]] || {
  echo "Service account must own the checkout root: $ROOT" >&2
  exit 1
}

render_unit() {
  local unit
  unit="$(<"$TEMPLATE")"
  unit="${unit//@DURIS_ROOT@/$ROOT}"
  unit="${unit//@DURIS_USER@/$RUN_USER}"
  unit="${unit//@DURIS_GROUP@/$RUN_GROUP}"
  printf '%s\n' "$unit"
}

if (( RENDER_ONLY == 1 )); then
  render_unit
  exit 0
fi

if (( EUID != 0 )); then
  echo "Installation requires root; rerun this command with sudo" >&2
  exit 1
fi

if (( ENABLE_SERVICE == 1 )); then
  runuser --user "$RUN_USER" -- \
    "$ROOT/scripts/cycle_mud.sh" --production --check-config
fi

if (( ENABLE_SERVICE == 1 )) &&
   ! systemctl is-active --quiet "$SERVICE_NAME" &&
   ss -H -ltn '( sport = :7777 )' | grep -q .; then
  echo "Port 7777 is already in use; stop the prior MUD service before enabling production" >&2
  exit 1
fi

TEMP_UNIT="$(mktemp --suffix=.service)"
trap 'rm -f -- "$TEMP_UNIT"' EXIT
render_unit > "$TEMP_UNIT"
systemd-analyze verify "$TEMP_UNIT"
install -o root -g root -m 0644 "$TEMP_UNIT" \
  "/etc/systemd/system/$SERVICE_NAME"
systemctl daemon-reload

if (( START_SERVICE == 1 )); then
  systemctl enable --now "$SERVICE_NAME"
elif (( ENABLE_SERVICE == 1 )); then
  systemctl enable "$SERVICE_NAME"
else
  echo "Installed $SERVICE_NAME without enabling it."
fi

if (( ENABLE_SERVICE == 1 )); then
  echo "Installed and enabled $SERVICE_NAME."
fi
