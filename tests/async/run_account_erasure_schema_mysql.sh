#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-account-erasure-$$"
PASSWORD=$(printf 'account-erasure-%s-%s' "$$" "$RANDOM")
DB_NAME="account_erasure_test"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
docker run --rm -d --name "$NAME" -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
for _ in $(seq 1 60); do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1 && break
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
docker cp "$ROOT/migrations/account_erasure.sql" "$NAME:/tmp/account_erasure.sql" >/dev/null
docker cp "$ROOT/migrations/verify_account_erasure_schema.sh" "$NAME:/tmp/verify_account_erasure_schema.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify_account_erasure_schema.sh
for _ in 1 2; do docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/account_erasure.sql"; done
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/verify_account_erasure_schema.sh >/dev/null
"${MYSQL[@]}" -e "
INSERT INTO account_erasure_requests(request_id,request_key,account_scope_hash,subject_token,policy_id,policy_schema_version,manifest_checksum,status,expected_stores) VALUES(UNHEX('00000000000000000000000000000001'),UNHEX(REPEAT('01',32)),UNHEX(REPEAT('02',32)),UNHEX(REPEAT('03',32)),'duris-lifecycle-v1',1,UNHEX(REPEAT('04',32)),1,184);
INSERT INTO account_erasure_stores(request_id,store_id,action,status,sequence_number) VALUES(UNHEX('00000000000000000000000000000001'),'database:accounts',4,2,0);
INSERT INTO account_erasure_evidence(request_id,store_id,event_type,status) VALUES(UNHEX('00000000000000000000000000000001'),'database:accounts',1,2);
INSERT INTO account_erasure_tombstones(subject_token,request_id,account_scope_hash,policy_id,policy_schema_version,manifest_checksum,completed_at) VALUES(UNHEX(REPEAT('03',32)),UNHEX('00000000000000000000000000000001'),UNHEX(REPEAT('02',32)),'duris-lifecycle-v1',1,UNHEX(REPEAT('04',32)),CURRENT_TIMESTAMP(6));"
[[ $("${MYSQL[@]}" -e "SELECT COUNT(*) FROM account_erasure_tombstones t JOIN account_erasure_requests r USING(request_id) WHERE t.subject_token=r.subject_token;") == 1 ]]
if "${MYSQL[@]}" -e "INSERT INTO account_erasure_stores(request_id,store_id,action,status,sequence_number) VALUES(UNHEX('ffffffffffffffffffffffffffffffff'),'database:player_data',4,2,1);" >/dev/null 2>&1; then echo 'orphan erasure store accepted' >&2; exit 1; fi
if "${MYSQL[@]}" -e "INSERT INTO account_erasure_evidence(request_id,event_type,status) VALUES(UNHEX('00000000000000000000000000000001'),11,2);" >/dev/null 2>&1; then echo 'invalid erasure event accepted' >&2; exit 1; fi
printf 'account erasure schema replay, identity, FK, bounds, and tombstone: ok\n'
