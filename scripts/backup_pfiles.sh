#!/usr/bin/env bash
set -euo pipefail

# Back up the authority selected by the explicit persistence mode. Redis is never
# treated as the durable authority here.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

umask 077

ENV_FILE="${BACKUP_ENV_FILE:-$PROJECT_ROOT/.env}"
if [[ -L "$ENV_FILE" ]]; then
  echo "Unsafe environment file: symbolic links are not allowed" >&2
  exit 2
elif [[ -e "$ENV_FILE" ]]; then
  if [[ ! -f "$ENV_FILE" || "$(stat -c '%u' "$ENV_FILE")" != "$(id -u)" ||
        $((8#$(stat -c '%a' "$ENV_FILE") & 0177)) -ne 0 ]]; then
    echo "Unsafe environment file: require an owner-only regular file" >&2
    exit 2
  fi
  set -a
  # shellcheck disable=SC1090
  source "$ENV_FILE"
  set +a
fi

DATESTR="$(date +%s)"
DATESTR_FULL="$(date -u +%Y.%m.%d-%H.%M.%S-UTC)"

if [[ "${PERSISTENCE_MODE:-mariadb-primary}" == "flatfile-primary" ]]; then
  if [[ -z "${FLATFILE_STATE_DIR:-}" || "$FLATFILE_STATE_DIR" != /* ]]; then
    echo "Flat-file backup requires an absolute FLATFILE_STATE_DIR" >&2
    exit 1
  fi
  if [[ ! -e "$FLATFILE_STATE_DIR" ]]; then
    echo "Flat-file state does not exist yet; skipping pre-boot backup."
    exit 0
  fi
  if [[ -L "$FLATFILE_STATE_DIR" || ! -d "$FLATFILE_STATE_DIR" ]]; then
    echo "Flat-file state root must be a real directory" >&2
    exit 1
  fi
  STATE_MODE="$(stat -c '%a' -- "$FLATFILE_STATE_DIR")"
  STATE_OWNER="$(stat -c '%u' -- "$FLATFILE_STATE_DIR")"
  if (( (8#$STATE_MODE & 0077) != 0 )) || [[ "$STATE_OWNER" != "$(id -u)" ]]; then
    echo "Flat-file state root must be owner-controlled with mode 0700 or stricter" >&2
    exit 1
  fi

  FLATFILE_BACKUP_DIR="${FLATFILE_BACKUP_DIR:-$PROJECT_ROOT/backups/flatfile}"
  if [[ "$FLATFILE_BACKUP_DIR" != /* ]]; then
    echo "FLATFILE_BACKUP_DIR must be an absolute path" >&2
    exit 1
  fi
  if [[ -L "$FLATFILE_BACKUP_DIR" ||
        ( -e "$FLATFILE_BACKUP_DIR" && ! -d "$FLATFILE_BACKUP_DIR" ) ]]; then
    echo "Flat-file backup root must be a real directory" >&2
    exit 1
  fi
  STATE_REAL="$(realpath -e "$FLATFILE_STATE_DIR")"
  BACKUP_REAL="$(realpath -m "$FLATFILE_BACKUP_DIR")"
  case "$BACKUP_REAL/" in
    "$STATE_REAL/"|"$STATE_REAL/"*)
      echo "FLATFILE_BACKUP_DIR must not be inside FLATFILE_STATE_DIR" >&2
      exit 1
      ;;
  esac

  install -d -m 700 -- "$FLATFILE_BACKUP_DIR"
  BACKUP_MODE="$(stat -c '%a' -- "$FLATFILE_BACKUP_DIR")"
  BACKUP_OWNER="$(stat -c '%u' -- "$FLATFILE_BACKUP_DIR")"
  if (( (8#$BACKUP_MODE & 0077) != 0 )) || [[ "$BACKUP_OWNER" != "$(id -u)" ]]; then
    echo "Flat-file backup root must be owner-controlled with mode 0700 or stricter" >&2
    exit 1
  fi
  BACKUP_DESTINATION="$FLATFILE_BACKUP_DIR/$DATESTR"
  if ! mkdir -m 0700 -- "$BACKUP_DESTINATION"; then
    echo "Could not create flat-file backup destination" >&2
    exit 1
  fi
  if ! cp -a -- "$FLATFILE_STATE_DIR/." "$BACKUP_DESTINATION/"; then
    echo "Flat-file state backup failed" >&2
    exit 1
  fi
  chmod 0700 -- "$BACKUP_DESTINATION"
  echo "Flat-file state backup complete: $BACKUP_DESTINATION ($DATESTR_FULL)"
  exit 0
fi

for REQUIRED_DB_FIELD in \
  ENVIRONMENT DB_HOST DB_USER DB_PASSWD DB_NAME DB_ALLOWED_TARGETS; do
  if [[ -z "${!REQUIRED_DB_FIELD:-}" ]]; then
    echo "Missing required database field: $REQUIRED_DB_FIELD" >&2
    exit 2
  fi
done

if [[ "$ENVIRONMENT" != "local" && "$ENVIRONMENT" != "production" ]]; then
  echo "ENVIRONMENT must be local or production" >&2
  exit 2
fi
DB_PORT="${DB_PORT:-3306}"
if ! [[ "$DB_PORT" =~ ^[0-9]+$ ]] || (( DB_PORT < 1 || DB_PORT > 65535 )); then
  echo "DB_PORT must be between 1 and 65535" >&2
  exit 2
fi
if ! [[ "$DB_USER" =~ ^[A-Za-z0-9_.-]+$ ]] ||
   ! [[ "$DB_NAME" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo "DB_USER and DB_NAME contain unsupported characters" >&2
  exit 2
fi
case ",$DB_ALLOWED_TARGETS," in
  *",$DB_HOST/$DB_NAME,"*) ;;
  *)
    echo "Database backup target is not allow-listed" >&2
    exit 2
    ;;
esac

if ! command -v mysqldump >/dev/null 2>&1; then
  echo "mysqldump is required for database backups" >&2
  exit 2
fi
if ! command -v gzip >/dev/null 2>&1; then
  echo "gzip is required for database backups" >&2
  exit 2
fi

DUMP_ARGS=(
  --connect-timeout=10
  --user="$DB_USER"
  --single-transaction
  --quick
  --hex-blob
)
if [[ -n "${DB_SOCKET:-}" ]]; then
  if [[ "$ENVIRONMENT" != "local" ||
        ( "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" &&
          "$DB_HOST" != "::1" ) ]]; then
    echo "DB_SOCKET is restricted to local loopback mode" >&2
    exit 2
  fi
  DUMP_ARGS+=(--protocol=socket --socket="$DB_SOCKET")
elif [[ "$DB_HOST" == "localhost" || "$DB_HOST" == "127.0.0.1" ||
        "$DB_HOST" == "::1" ]]; then
  DUMP_ARGS+=(--protocol=tcp --host="$DB_HOST" --port="$DB_PORT")
else
  if [[ "${DB_TLS:-}" != "TRUE" || ! -f "${DB_SSL_CA:-}" ]]; then
    echo "Remote database backup requires TLS and a CA file" >&2
    exit 2
  fi
  DUMP_ARGS+=(--protocol=tcp --host="$DB_HOST" --port="$DB_PORT"
             --ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
fi

BACKUP_DIR="${DATABASE_BACKUP_DIR:-$PROJECT_ROOT/db/Backup}"
if [[ -L "$BACKUP_DIR" ]]; then
  echo "Unsafe backup directory: symbolic links are not allowed" >&2
  exit 2
elif [[ -e "$BACKUP_DIR" ]]; then
  if [[ ! -d "$BACKUP_DIR" || "$(stat -c '%u' "$BACKUP_DIR")" != "$(id -u)" ||
        $((8#$(stat -c '%a' "$BACKUP_DIR") & 0077)) -ne 0 ]]; then
    echo "Unsafe backup directory: require an owner-only directory" >&2
    exit 2
  fi
else
  install -d -m 700 -- "$BACKUP_DIR"
fi

BACKUP_FILE="$BACKUP_DIR/$DATESTR.sql.gz"
if [[ -e "$BACKUP_FILE" ]]; then
  echo "Refusing to overwrite existing backup: $BACKUP_FILE" >&2
  exit 1
fi

TEMP_FILE="$(mktemp "$BACKUP_DIR/.backup-$DATESTR.sql.gz.tmp.XXXXXX")"
cleanup_temp() {
  if [[ -n "${TEMP_FILE:-}" && -e "$TEMP_FILE" ]]; then
    rm -f -- "$TEMP_FILE"
  fi
}
trap cleanup_temp EXIT HUP INT TERM

echo "Creating MySQL backup: $BACKUP_FILE ($DATESTR_FULL)"
export MYSQL_PWD="$DB_PASSWD"
if ! mysqldump "${DUMP_ARGS[@]}" --databases "$DB_NAME" | gzip -c >"$TEMP_FILE"; then
  echo "Database dump or compression failed; no backup was published" >&2
  exit 1
fi
unset MYSQL_PWD

if ! gzip -t "$TEMP_FILE"; then
  echo "Compressed backup validation failed; no backup was published" >&2
  exit 1
fi
if ! gzip -cd "$TEMP_FILE" | awk '
  index($0, "CREATE TABLE `accounts`") { accounts = 1 }
  index($0, "CREATE TABLE `player_data`") { player_data = 1 }
  index($0, "CREATE TABLE `ships`") { ships = 1 }
  END { exit !(accounts && player_data && ships) }
'; then
  echo "Database dump is missing required Duris schema markers; no backup was published" >&2
  exit 1
fi

sync -f "$TEMP_FILE"
mv -T -- "$TEMP_FILE" "$BACKUP_FILE"
TEMP_FILE=""
sync -f "$BACKUP_DIR"

find "$BACKUP_DIR" -maxdepth 1 -type f -name '*.sql.gz' -mmin +2880 \
  -print -delete

BACKUP_SIZE="$(du -h "$BACKUP_FILE" | awk '{print $1}')"
echo "Backup complete: $BACKUP_FILE ($BACKUP_SIZE)"
