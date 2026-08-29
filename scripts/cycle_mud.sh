#!/bin/bash

# Always run from the repository root so relative paths resolve correctly.
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Load environment variables from .env if it exists
if [ -L .env ]; then
  echo "Unsafe .env metadata; symbolic links are not allowed" >&2
  exit 1
elif [ -e .env ]; then
  if [ ! -f .env ]; then
    echo "Unsafe .env metadata; a regular file is required" >&2
    exit 1
  fi
  ENV_MODE=$(stat -c '%a' .env) || exit 1
  ENV_OWNER=$(stat -c '%u' .env) || exit 1
  if (( (8#$ENV_MODE & 0177) != 0 )) || [[ "$ENV_OWNER" != "$(id -u)" ]]; then
    echo "Unsafe .env metadata; run: chmod 600 .env" >&2
    exit 1
  fi
  echo "Loading environment from .env"
  set -a
  # shellcheck disable=SC1091
  source .env
  set +a
fi

# Parse command line arguments
DEV_MODE=0
MINIMAL_MODE=0
CONFIG_CHECK_ONLY=0
while (( $# > 0 )); do
  case "$1" in
    --dev)
      DEV_MODE=1
      ;;
    --minimal)
      MINIMAL_MODE=1
      DEV_MODE=1
      ;;
    --check-config)
      CONFIG_CHECK_ONLY=1
      ;;
    --help|-h)
      echo "Usage: $0 [--dev] [--minimal] [--check-config]"
      echo "  --minimal  Use the tracked areas_mini dataset (implies --dev)."
      echo "  --check-config  Validate persistence configuration without starting the game."
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo "Usage: $0 [--dev] [--minimal] [--check-config]" >&2
      exit 2
      ;;
  esac
  shift
done
if (( MINIMAL_MODE == 1 )); then
  echo "Running in minimal world mode from areas_mini"
fi
MUD_PORT=7777
if [ $DEV_MODE -eq 1 ]; then
  MUD_PORT=4000
fi

RESULT=53
STOP_REASON="initial bootup"
SERVER_BIN_DIR="bin/server"
STAGED_BINARY="$SERVER_BIN_DIR/dms_new"
RUNTIME_BINARY="$SERVER_BIN_DIR/dms"
BINARY_HISTORY_DIR="$SERVER_BIN_DIR/history"
BINARY_HISTORY_LIMIT="${DMS_BINARY_HISTORY_LIMIT:-5}"

if ! [[ "$BINARY_HISTORY_LIMIT" =~ ^[0-9]+$ ]]; then
  echo "Warning: invalid DMS_BINARY_HISTORY_LIMIT; using 5"
  BINARY_HISTORY_LIMIT=5
fi

mkdir -p "$BINARY_HISTORY_DIR"

ulimit -c unlimited

PERSISTENCE_MODE="${PERSISTENCE_MODE:-mariadb-primary}"
export PERSISTENCE_MODE
DATABASE_REQUIRED=0
FLATFILE_REQUIRED=0
case "$PERSISTENCE_MODE" in
  mariadb-primary)
    DATABASE_REQUIRED=1
    ;;
  mariadb-primary-flatfile-fallback)
    DATABASE_REQUIRED=1
    FLATFILE_REQUIRED=1
    ;;
  flatfile-primary)
    FLATFILE_REQUIRED=1
    ;;
  *)
    echo "Invalid PERSISTENCE_MODE: $PERSISTENCE_MODE" >&2
    exit 1
    ;;
esac

if [[ -z "${ENVIRONMENT:-}" ]]; then
  echo "Missing required environment field: ENVIRONMENT" >&2
  exit 1
fi
if [[ "$ENVIRONMENT" != "local" && "$ENVIRONMENT" != "production" ]]; then
  echo "ENVIRONMENT must be local or production" >&2
  exit 1
fi
if [[ "$ENVIRONMENT" == "production" && $MUD_PORT -ne 7777 ]]; then
  echo "Production mode requires port 7777" >&2
  exit 1
fi
if (( FLATFILE_REQUIRED == 1 )); then
  if [[ -z "${FLATFILE_STATE_DIR:-}" ]]; then
    echo "FLATFILE_STATE_DIR is required for persistence mode $PERSISTENCE_MODE" >&2
    exit 1
  fi
  if [[ "$FLATFILE_STATE_DIR" != /* ]]; then
    echo "FLATFILE_STATE_DIR must be an absolute path" >&2
    exit 1
  fi
fi

if (( DATABASE_REQUIRED == 1 )); then
  for REQUIRED_DB_FIELD in DB_HOST DB_USER DB_PASSWD DB_NAME DB_ALLOWED_TARGETS; do
    if [[ -z "${!REQUIRED_DB_FIELD:-}" ]]; then
      echo "Missing required database field: $REQUIRED_DB_FIELD" >&2
      exit 1
    fi
  done
  if [[ -n "${DB_PORT:-}" ]] &&
     { ! [[ "$DB_PORT" =~ ^[0-9]+$ ]] || (( DB_PORT < 1 || DB_PORT > 65535 )); }; then
    echo "DB_PORT must be between 1 and 65535" >&2
    exit 1
  fi
  EFFECTIVE_DB_NAME="$DB_NAME"
  if [[ $MUD_PORT -ne 7777 && ( "$DB_NAME" == "duris" || "$DB_NAME" == "duris_prod" ) ]]; then
    EFFECTIVE_DB_NAME="duris_dev"
  fi
  case ",$DB_ALLOWED_TARGETS," in
    *",$DB_HOST/$EFFECTIVE_DB_NAME,"*) ;;
    *) echo "Resolved database target is not allow-listed" >&2; exit 1 ;;
  esac
  export DB_NAME="$EFFECTIVE_DB_NAME"

  MYSQL_CONNECTION_ARGS=(--connect-timeout=10 -u "$DB_USER")
  if [[ -n "${DB_SOCKET:-}" ]]; then
    if [[ "$ENVIRONMENT" != "local" ||
          ( "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ) ]]; then
      echo "DB_SOCKET is restricted to local loopback mode" >&2
      exit 1
    fi
    MYSQL_CONNECTION_ARGS+=(--protocol=socket --socket="$DB_SOCKET")
  elif [[ "$DB_HOST" == "localhost" || "$DB_HOST" == "127.0.0.1" || "$DB_HOST" == "::1" ]]; then
    MYSQL_CONNECTION_ARGS+=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}")
  else
    if [[ "${DB_TLS:-}" != "TRUE" || ! -f "${DB_SSL_CA:-}" ]]; then
      echo "Remote database transport requires TLS and a CA file" >&2
      exit 1
    fi
    MYSQL_CONNECTION_ARGS+=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}"
                            --ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
  fi
  export MYSQL_PWD="$DB_PASSWD"
  echo "Validated explicit database configuration for $PERSISTENCE_MODE"
else
  echo "Validated database-independent configuration for flatfile-primary"
fi
if (( DEV_MODE == 1 )); then
  echo "Running in DEV mode"
fi
if (( CONFIG_CHECK_ONLY == 1 )); then
  exit 0
fi

while [[ $RESULT != 0 && $RESULT != 55 ]]; do
	DATESTR=$(date +%C%y.%m.%d-%H.%M.%S)

  # Refuse to publish the service against a stale or incompatible schema. Local
  # databases can be advanced safely by the guarded immutable runner;
  # production remains read-only and must be migrated through the runbook.
  if (( DATABASE_REQUIRED == 1 )); then
    if [[ "$ENVIRONMENT" == "local" ]]; then
      echo "Applying pending immutable database migrations..."
      if ! python3 scripts/migration_runner.py run; then
        echo "Database migrations are not up to date; refusing to boot" >&2
        exit 1
      fi
    fi
    echo "Verifying runtime database compatibility..."
    if ! ./migrations/verify_runtime_compatibility.sh; then
      echo "Database schema is incompatible with this server; refusing to boot" >&2
      exit 1
    fi
  fi

  if [[ $RESULT == 53 || $RESULT == 57 ]]; then
    if [ -f "$STAGED_BINARY" ]; then
      if [ -f "$RUNTIME_BINARY" ]; then
        mv "$RUNTIME_BINARY" "$BINARY_HISTORY_DIR/dms.$DATESTR"
      fi
      mv "$STAGED_BINARY" "$RUNTIME_BINARY"

      mapfile -t OLD_BINARIES < <(
        find "$BINARY_HISTORY_DIR" -maxdepth 1 -type f -name 'dms.*' \
          -printf '%T@ %p\n' | sort -nr | tail -n "+$((BINARY_HISTORY_LIMIT + 1))" | cut -d' ' -f2-
      )
      if (( ${#OLD_BINARIES[@]} > 0 )); then
        rm -f -- "${OLD_BINARIES[@]}"
      fi
    fi
  fi

  if [ -d logs/log ]; then
    #LOGNAME=`date +%b%d-%H%M`
    mkdir -p "logs/old-logs/$DATESTR"
    find logs/log -mindepth 1 -maxdepth 1 ! -name .gitignore \
      -exec mv -t "logs/old-logs/$DATESTR" {} +
    if [ -f core ]; then
      mv core "core.$DATESTR"
    fi
  fi

  # The game opens logs/log/* with fopen(), which fails silently when the
  # directory is missing; every logit() write would be dropped.
  mkdir -p logs/log

  echo "Backing up authoritative persistence state..."
  if ! ./scripts/backup_pfiles.sh; then
    echo "Required $PERSISTENCE_MODE backup failed; refusing to boot" >&2
    exit 1
  fi

  if (( MINIMAL_MODE == 1 )); then
    for MINIMAL_FILE in mini.mob mini.obj mini.qst mini.wld mini.zon world.shp world.tab world.weather; do
      if [[ ! -s "areas_mini/$MINIMAL_FILE" ]]; then
        echo "Missing required minimal world file: areas_mini/$MINIMAL_FILE" >&2
        exit 1
      fi
    done
    echo "Using tracked minimal world data; skipping full world generation."
  else
    echo "Building area tools if needed..."
    if [ ! -x "bin/areas/tools/make_mob" ] || [ ! -x "bin/areas/tools/make_obj" ] || [ ! -x "bin/areas/tools/make_qst" ] || [ ! -x "bin/areas/tools/make_shp" ] || [ ! -x "bin/areas/tools/make_wld" ] || [ ! -x "bin/areas/tools/make_zon" ]; then
      (cd ./areas/src && make -j1) || exit 1
    fi

    echo "Building areas..."
    (cd ./areas && ./m_slow)
  fi

  echo "Generating list of function names.."
  nm --demangle "$RUNTIME_BINARY" | grep " T " | sed -e 's/[(].*//g' > lib/misc/event_names

	if [ -f /usr/bin/sendemail ]; then
		if [ -f "/logs/old-logs/$DATESTR/exit" ]; then
			/usr/bin/sendEmail -t alert@durismud.com \
				-f mud@durismud.com -u "Duris Booting..." \
				-m "Mud booting at ${DATESTR}, previous shutdown reason: ${STOP_REASON} [${RESULT}]." \
      	-a "logs/old-logs/${DATESTR}/exit"
		else
			/usr/bin/sendEmail -t alert@durismud.com \
				-f mud@durismud.com -u "Duris Booting..." \
				-m "Mud booting at ${DATESTR}, previous shutdown reason: ${STOP_REASON} [${RESULT}]."
		fi
	fi

  # Record boot time (will be used for shutdown record later)
  BOOT_TIME=$(date +%s)

  echo "Starting duris on port ${MUD_PORT}..."
  SERVER_ARGS=()
  if (( MINIMAL_MODE == 1 )); then
    SERVER_ARGS+=(--minimal)
  fi
  "$RUNTIME_BINARY" "${SERVER_ARGS[@]}" "${MUD_PORT}" # > dms.out

	# capture the exit code
  RESULT=${PIPESTATUS[0]}

	# determine the reason for shutting down
	case $RESULT in
		0) STOP_REASON="shutdown";;
		139) STOP_REASON="crash";;
		52) STOP_REASON="reboot";;
		53) STOP_REASON="copyover reboot";;
		54) STOP_REASON="auto reboot";;
		55) STOP_REASON="pwipe shutdown";;
		56) STOP_REASON="mud hung reboot";;
		57) STOP_REASON="auto reboot with copyover";;
		*) STOP_REASON="unknown";;
	esac

	echo "Mud stopped, reason: ${STOP_REASON} [${RESULT}]"

  # Log shutdown to database for server reboot tracking
  if (( DATABASE_REQUIRED == 1 )); then
    SHUTDOWN_TIME=$(date +%s)

    # Default values for database
    DB_SHUTDOWN_TYPE="unknown"
    INITIATED_BY=""
    SHUTDOWN_REASON=""

    # Parse shutdown info file if it exists
    if [ -f "logs/shutdown_info.txt" ]; then
      SHUTDOWN_INFO=$(cat "logs/shutdown_info.txt")
      INITIATED_BY=$(echo "$SHUTDOWN_INFO" | cut -d'|' -f1)
      SHUTDOWN_REASON=$(echo "$SHUTDOWN_INFO" | cut -d'|' -f2)
      # Delete the file after reading
      rm -f "logs/shutdown_info.txt"
    fi

    # Map exit code to database shutdown_type enum
    case $RESULT in
      0) DB_SHUTDOWN_TYPE="shutdown";;
      52) DB_SHUTDOWN_TYPE="reboot";;
      53) DB_SHUTDOWN_TYPE="copyover";;
      54) DB_SHUTDOWN_TYPE="autoreboot";;
      55) DB_SHUTDOWN_TYPE="pwipe";;
      56) DB_SHUTDOWN_TYPE="hung";;
      57) DB_SHUTDOWN_TYPE="autoreboot_copyover";;
      139) DB_SHUTDOWN_TYPE="crash";;
      *) DB_SHUTDOWN_TYPE="unknown";;
    esac

    # Calculate MUD uptime (shutdown_time - boot_time)
    MUD_UPTIME=$((SHUTDOWN_TIME - BOOT_TIME))

    # Insert a complete reboot record (boot + shutdown)
    mysql "${MYSQL_CONNECTION_ARGS[@]}" "$EFFECTIVE_DB_NAME" -e "
      INSERT INTO server_reboots
        (boot_time, shutdown_time, uptime_seconds, shutdown_type, initiated_by, reason)
      VALUES
        (${BOOT_TIME}, ${SHUTDOWN_TIME}, ${MUD_UPTIME}, '${DB_SHUTDOWN_TYPE}',
         IF('${INITIATED_BY}' = '', NULL, '${INITIATED_BY}'),
         IF('${SHUTDOWN_REASON}' = '', NULL, '${SHUTDOWN_REASON}'));
    " 2>/dev/null
    echo "Logged reboot: ${MUD_UPTIME}s uptime, type: ${DB_SHUTDOWN_TYPE}"
  fi

  echo "Sleeping 10 seconds to prevent coreflood..."
  sleep 10
done

if [ "$RESULT" == 55 ]; then
  echo "Wiping player data..."
  if [ ! -x "./Players/wipers/wipe_it_all" ]; then
    echo "ERROR: required filesystem wipe artifact is missing or not executable" >&2
    exit 1
  fi
  if ! ./Players/wipers/wipe_it_all; then
    echo "ERROR: filesystem wipe failed; refusing to report pwipe success" >&2
    exit 1
  fi
  echo "Moving player-logs to backup.."
  if [ -d logs/player-log ]; then
    #LOGNAME=`date +%b%d-%H%M`
    mkdir "logs/player-log/$DATESTR"
    mv logs/player-log/* "logs/player-log/$DATESTR"
  fi
  echo "Wiped!"
fi

if [ -f /usr/bin/sendemail ]; then
	/usr/bin/sendEmail -t alert@durismud.com \
		-f mud@durismud.com -u "Duris Shutdown..." \
		-m "Mud shutdown at ${DATESTR}, shutdown reason: ${STOP_REASON} [${RESULT}]."
fi
