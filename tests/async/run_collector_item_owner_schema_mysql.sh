#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# This upgrade check always uses an isolated server and never reads the checkout's .env.
NAME="duris-collector-owner-$$-$RANDOM"
PASSWORD="collector-owner-$$-$RANDOM"
IMAGE="${COLLECTOR_OWNER_DB_IMAGE:-mariadb:10.11}"
BIND_ADDRESS="${COLLECTOR_OWNER_BIND_ADDRESS:-127.0.0.1}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi
docker run -d --name "$NAME" -p "${BIND_ADDRESS}::3306" \
    -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST="${COLLECTOR_OWNER_DB_HOST:-127.0.0.1}"
export DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=collector_owner_test
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo "FAILED: $IMAGE did not accept connections on $DB_HOST:$DB_PORT" >&2; exit 1; }
"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/bootstrap_multithread_safe.sql"

# Recreate the pre-collector contract, retain one row under the previous maximum,
# and prove type 10 is unavailable until the new migration commits.
"${MYSQL[@]}" "$DB_NAME" -e "ALTER TABLE item_owner_revision DROP CONSTRAINT chk_item_owner_revision_type, ADD CONSTRAINT chk_item_owner_revision_type CHECK (owner_type BETWEEN 1 AND 9); ALTER TABLE item_current_owner DROP CONSTRAINT chk_item_current_owner_type, ADD CONSTRAINT chk_item_current_owner_type CHECK (owner_type BETWEEN 1 AND 9); ALTER TABLE item_ownership_baseline DROP CONSTRAINT chk_item_baseline_owner_type, ADD CONSTRAINT chk_item_baseline_owner_type CHECK (owner_type BETWEEN 1 AND 9);"
"${MYSQL[@]}" "$DB_NAME" -e "INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (9,9001,0,1); INSERT INTO item_current_owner (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES (9001,9001,9,9001,0,1,1,1); INSERT INTO item_ownership_baseline (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,opening_item_revision,vnum,source_table,source_row_id) VALUES (9002,9002,9,9001,0,1,1,'collector_test',9002);"
if "${MYSQL[@]}" "$DB_NAME" -e "INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (10,10001,0,0)" >/dev/null 2>&1; then
    echo 'FAILED: pre-collector ownership schema accepted type 10' >&2
    exit 1
fi

# Both the first upgrade and a rerun must preserve old custody and admit the new
# namespace across every ownership authority.
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/collector_item_owner.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/collector_item_owner.sql"
"$ROOT/migrations/verify_collector_item_owner.sh"
retained=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT (SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=9 AND owner_id=9001)+(SELECT COUNT(*) FROM item_current_owner WHERE item_uid=9001 AND owner_type=9)+(SELECT COUNT(*) FROM item_ownership_baseline WHERE item_uid=9002 AND owner_type=9)")
[[ "$retained" == 3 ]] || { echo "FAILED: collector migration did not preserve prior custody; found $retained rows" >&2; exit 1; }
accepted=$("${MYSQL[@]}" "$DB_NAME" -e "START TRANSACTION; INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (10,10001,0,0); INSERT INTO item_current_owner (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES (10001,10001,10,10001,0,1,1,1); INSERT INTO item_ownership_baseline (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,opening_item_revision,vnum,source_table,source_row_id) VALUES (10002,10002,10,10001,0,1,1,'collector_test',10002); SELECT (SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=10 AND owner_id=10001)+(SELECT COUNT(*) FROM item_current_owner WHERE item_uid=10001 AND owner_type=10)+(SELECT COUNT(*) FROM item_ownership_baseline WHERE item_uid=10002 AND owner_type=10); ROLLBACK;")
[[ "$accepted" == 3 ]] || { echo "FAILED: collector custody was not accepted by all authorities; found $accepted rows" >&2; exit 1; }

# A later full migration run reaches the older shopkeeper step first. Commit
# collector rows, rerun that step, and ensure it remains monotonic.
"${MYSQL[@]}" "$DB_NAME" -e "INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (10,10001,0,0); INSERT INTO item_current_owner (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES (10001,10001,10,10001,0,1,1,1); INSERT INTO item_ownership_baseline (item_uid,root_item_uid,owner_type,owner_id,owner_context_id,opening_item_revision,vnum,source_table,source_row_id) VALUES (10002,10002,10,10001,0,1,1,'collector_test',10002);"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/shopkeeper_item_owner.sql"
"$ROOT/migrations/verify_collector_item_owner.sh"
preserved=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT (SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=10 AND owner_id=10001)+(SELECT COUNT(*) FROM item_current_owner WHERE item_uid=10001 AND owner_type=10)+(SELECT COUNT(*) FROM item_ownership_baseline WHERE item_uid=10002 AND owner_type=10)")
[[ "$preserved" == 3 ]] || { echo "FAILED: older shopkeeper migration narrowed collector custody; found $preserved rows" >&2; exit 1; }

printf 'collector item owner migration preserved type 9, admitted type 10, and resisted later narrowing\n'
