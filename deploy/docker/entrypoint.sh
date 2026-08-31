#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

for required_name in DB_HOST DB_USER DB_PASSWD DB_NAME DB_SOCKET; do
  if [[ -z "${!required_name:-}" ]]; then
    echo "Missing required container database field: $required_name" >&2
    exit 1
  fi
done

TLS_DIRECTORY=/var/lib/duris/tls
TLS_CERTIFICATE="$TLS_DIRECTORY/duris.crt"
TLS_KEY="$TLS_DIRECTORY/duris.key"
if [[ -e "$TLS_CERTIFICATE" || -e "$TLS_KEY" ]]; then
  if [[ ! -f "$TLS_CERTIFICATE" || ! -f "$TLS_KEY" ]]; then
    echo "Container TLS state is incomplete; both certificate and key are required" >&2
    exit 1
  fi
else
  echo "Generating the persistent local Docker TLS certificate..."
  umask 077
  openssl req -x509 -newkey rsa:3072 -sha256 -nodes -days 365 \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1" \
    -addext "keyUsage=critical,digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth" \
    -keyout "$TLS_KEY" \
    -out "$TLS_CERTIFICATE" >/dev/null 2>&1
  chmod 0600 "$TLS_KEY"
  chmod 0644 "$TLS_CERTIFICATE"
fi

MYSQL=(
  mysql
  --protocol=socket
  --socket="$DB_SOCKET"
  --user="$DB_USER"
  --database="$DB_NAME"
  --batch
  --skip-column-names
)

echo "Waiting for the initialized MariaDB database..."
database_ready=0
for _ in $(seq 1 120); do
  if MYSQL_PWD="$DB_PASSWD" "${MYSQL[@]}" --execute='SELECT 1' >/dev/null 2>&1; then
    database_ready=1
    break
  fi
  sleep 1
done
if [[ "$database_ready" != 1 ]]; then
  echo "MariaDB did not become ready within 120 seconds" >&2
  exit 1
fi

if ! baseline_count="$(
  MYSQL_PWD="$DB_PASSWD" "${MYSQL[@]}" \
    --execute='SELECT COUNT(*) FROM mud_schema_baselines;'
)"; then
  echo "Unable to query mud_schema_baselines; the Docker database bootstrap may be incomplete" >&2
  exit 1
fi
if [[ "$baseline_count" == 0 ]]; then
  echo "Seeding tracked help content into the fresh Docker database..."
  ./scripts/import_help_to_prod.sh --local < <(yes yes)
  echo "Adopting the verified fresh database baseline..."
  python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
elif [[ "$baseline_count" != 1 ]]; then
  echo "Unexpected migration baseline count: $baseline_count" >&2
  exit 1
fi

exec ./scripts/cycle_mud.sh --dev
