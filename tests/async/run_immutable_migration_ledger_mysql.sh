#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-migration-ledger-$$"
PASSWORD=$(printf 'migration-ledger-%s-%s' "$$" "$RANDOM")
DB_NAME="migration_ledger_test"
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
docker cp "$ROOT/migrations/immutable_migration_ledger.sql" "$NAME:/tmp/ledger.sql" >/dev/null
docker cp "$ROOT/migrations/verify_immutable_migration_ledger.sh" "$NAME:/tmp/verify.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify.sh
for _ in 1 2; do docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/ledger.sql"; done
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/verify.sh >/dev/null
"${MYSQL[@]}" -e "
CREATE TABLE IF NOT EXISTS mud_schema_migrations(version INT PRIMARY KEY,description VARCHAR(255));
INSERT INTO mud_schema_migrations VALUES(8,'legacy data-copy marker');
INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES('test-baseline','verified_legacy_adoption',UNHEX(REPEAT('01',32)),1,1);
INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0001_test',1,'test',UNHEX(REPEAT('02',32)),UNHEX(REPEAT('03',32)),'mysql8-mariadb10',1);"
[[ $("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_migrations WHERE version=8;") == 1 ]]
if "${MYSQL[@]}" -e "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0002_test',1,'duplicate sequence',UNHEX(REPEAT('04',32)),UNHEX(REPEAT('05',32)),'mysql8-mariadb10',1);" >/dev/null 2>&1; then echo 'duplicate migration sequence accepted' >&2; exit 1; fi
if "${MYSQL[@]}" -e "INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES('other-baseline','verified_legacy_adoption',UNHEX(REPEAT('06',32)),1,1);" >/dev/null 2>&1; then echo 'duplicate baseline kind accepted' >&2; exit 1; fi
printf 'immutable migration ledger replay, baseline, ordering, and legacy marker separation: ok\n'
