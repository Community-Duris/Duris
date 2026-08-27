#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-lifecycle-archive-$$"
PASSWORD=$(printf 'lifecycle-archive-%s-%s' "$$" "$RANDOM")
DB_NAME="lifecycle_archive_test"
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
docker cp "$ROOT/migrations/lifecycle_archive_execution.sql" \
    "$NAME:/tmp/lifecycle_archive_execution.sql" >/dev/null
docker cp "$ROOT/migrations/verify_lifecycle_archive_schema.sh" \
    "$NAME:/tmp/verify_lifecycle_archive_schema.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify_lifecycle_archive_schema.sh

apply_schema() {
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/lifecycle_archive_execution.sql"
}
apply_schema
apply_schema
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
    -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" \
    "$NAME" /tmp/verify_lifecycle_archive_schema.sh >/dev/null

"${MYSQL[@]}" -e "
INSERT INTO lifecycle_archive_jobs(
 job_id,job_key,policy_id,policy_schema_version,manifest_checksum,store_id,action,
 dry_run,target_environment,approval_reference,status,source_cursor,source_upper_bound,
 row_budget,byte_budget,time_budget_usec)
VALUES(
 UNHEX('00000000000000000000000000000001'),UNHEX(REPEAT('01',32)),
 'duris-lifecycle-v1',1,UNHEX(REPEAT('02',32)),'database:test_source',1,
 1,'test','TEST-APPROVAL',1,'000','999',64,262144,25000),
(
 UNHEX('00000000000000000000000000000002'),UNHEX(REPEAT('03',32)),
 'duris-lifecycle-v1',1,UNHEX(REPEAT('02',32)),'database:test_source',1,
 1,'test','TEST-APPROVAL',1,'000','999',64,262144,25000);
INSERT INTO lifecycle_archive_batches(
 batch_id,batch_key,job_id,sequence_number,status,cursor_start,cursor_end)
VALUES(
 UNHEX('10000000000000000000000000000001'),UNHEX(REPEAT('04',32)),
 UNHEX('00000000000000000000000000000001'),0,2,'000','002');
INSERT INTO lifecycle_archive_rows(batch_id,source_key,source_checksum,payload,payload_bytes)
VALUES(
 UNHEX('10000000000000000000000000000001'),'001',UNHEX(SHA2('payload',256)),
 'payload',7);
INSERT INTO lifecycle_archive_evidence(job_id,batch_id,event_type,status,row_count,byte_count)
VALUES(
 UNHEX('00000000000000000000000000000001'),
 UNHEX('10000000000000000000000000000001'),1,2,1,7);"

[[ $("${MYSQL[@]}" -e "SELECT COUNT(*) FROM lifecycle_archive_rows WHERE source_key='001' AND payload='payload' AND source_checksum=UNHEX(SHA2(payload,256));") == "1" ]]
if "${MYSQL[@]}" -e "INSERT INTO lifecycle_archive_rows(batch_id,source_key,source_checksum,payload,payload_bytes) VALUES(UNHEX('10000000000000000000000000000001'),'001',UNHEX(SHA2('different',256)),'different',9);" >/dev/null 2>&1; then
    echo "duplicate archive source identity was accepted" >&2
    exit 1
fi
if "${MYSQL[@]}" -e "INSERT INTO lifecycle_archive_evidence(job_id,batch_id,event_type,status) VALUES(UNHEX('00000000000000000000000000000002'),UNHEX('10000000000000000000000000000001'),1,2);" >/dev/null 2>&1; then
    echo "cross-job archive evidence was accepted" >&2
    exit 1
fi
if "${MYSQL[@]}" -e "INSERT INTO lifecycle_archive_jobs(job_id,job_key,policy_id,policy_schema_version,manifest_checksum,store_id,action,dry_run,target_environment,approval_reference,status,source_cursor,source_upper_bound,row_budget,byte_budget,time_budget_usec) VALUES(UNHEX('00000000000000000000000000000003'),UNHEX(REPEAT('05',32)),'duris-lifecycle-v1',1,UNHEX(REPEAT('02',32)),'database:test_source',1,1,'test','TEST-APPROVAL',1,'000','999',257,262144,25000);" >/dev/null 2>&1; then
    echo "out-of-bound archive job was accepted" >&2
    exit 1
fi

printf 'lifecycle archive schema replay, bounds, identity, FK, and restore envelope: ok\n'
