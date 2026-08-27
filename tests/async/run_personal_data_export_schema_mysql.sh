#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-personal-export-$$"
PASSWORD=$(printf 'personal-export-%s-%s' "$$" "$RANDOM")
DB_NAME="personal_export_test"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT

docker run --rm -d --name "$NAME" -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
for _ in $(seq 1 60); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" \
        mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" \
    mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" \
    mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")

docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot \
    -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
docker cp "$ROOT/migrations/personal_data_export.sql" \
    "$NAME:/tmp/personal_data_export.sql" >/dev/null
docker cp "$ROOT/migrations/verify_personal_data_export_schema.sh" \
    "$NAME:/tmp/verify_personal_data_export_schema.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify_personal_data_export_schema.sh

apply_schema() {
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/personal_data_export.sql"
}
apply_schema
apply_schema
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
    -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" \
    "$NAME" /tmp/verify_personal_data_export_schema.sh >/dev/null

"${MYSQL[@]}" -e "
INSERT INTO personal_data_export_requests(
 request_id,request_key,account_name,account_scope_hash,policy_id,
 policy_schema_version,manifest_checksum,status,expected_sections,expires_at)
VALUES(
 UNHEX('00000000000000000000000000000001'),UNHEX(REPEAT('01',32)),'Tester',
 UNHEX(REPEAT('02',32)),'duris-lifecycle-v1',1,UNHEX(REPEAT('03',32)),1,180,
 TIMESTAMPADD(HOUR,1,CURRENT_TIMESTAMP(6)));
INSERT INTO personal_data_export_sections(
 request_id,store_id,disposition,status,snapshot_id,record_count,byte_count)
VALUES(
 UNHEX('00000000000000000000000000000001'),'database:accounts',2,2,
 UNHEX('10000000000000000000000000000001'),1,128);
INSERT INTO personal_data_export_audit(
 request_id,event_type,status,section_count,record_count,byte_count)
VALUES(UNHEX('00000000000000000000000000000001'),1,2,1,1,128);"

[[ $("${MYSQL[@]}" -e "SELECT COUNT(*) FROM personal_data_export_requests r JOIN personal_data_export_sections s USING(request_id) JOIN personal_data_export_audit a USING(request_id) WHERE r.account_name='Tester' AND s.store_id='database:accounts';") == "1" ]]
if "${MYSQL[@]}" -e "INSERT INTO personal_data_export_requests(request_id,request_key,account_name,account_scope_hash,policy_id,policy_schema_version,manifest_checksum,status,expected_sections,expires_at) VALUES(UNHEX('00000000000000000000000000000002'),UNHEX(REPEAT('01',32)),'Tester',UNHEX(REPEAT('02',32)),'duris-lifecycle-v1',1,UNHEX(REPEAT('03',32)),1,180,TIMESTAMPADD(HOUR,1,CURRENT_TIMESTAMP(6)));" >/dev/null 2>&1; then
    echo "duplicate export request identity was accepted" >&2
    exit 1
fi
if "${MYSQL[@]}" -e "INSERT INTO personal_data_export_sections(request_id,store_id,disposition,status) VALUES(UNHEX('ffffffffffffffffffffffffffffffff'),'database:accounts',2,2);" >/dev/null 2>&1; then
    echo "orphan export section was accepted" >&2
    exit 1
fi
if "${MYSQL[@]}" -e "INSERT INTO personal_data_export_audit(request_id,event_type,status) VALUES(UNHEX('00000000000000000000000000000001'),10,2);" >/dev/null 2>&1; then
    echo "out-of-bound export audit event was accepted" >&2
    exit 1
fi

printf 'personal data export schema replay, identity, bounds, FK, and audit envelope: ok\n'
