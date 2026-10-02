#!/usr/bin/env bash
# Real-FK disposable proof of the one-locker-per-account index transition.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
[[ ! -e "$ROOT/.env" && ! -e "$ROOT/migrations/.env" ]]
NAME="duris-accounting-schema-locker-index-$$-$RANDOM"
PASSWORD="locker-index-$$-$RANDOM"
IMAGE="${LOCKER_INDEX_DB_IMAGE:-mysql:8.0}"
[[ "$IMAGE" =~ ^(mysql:8\.0(\.[0-9]+)?|mariadb:10\.11(\.[0-9]+)?)(@sha256:[0-9a-f]{64})?$ ]] || { echo 'unsupported test image' >&2; exit 2; }
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then ROOT_ENV=MARIADB_ROOT_PASSWORD; else ROOT_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -e "$ROOT_ENV=$PASSWORD" "$IMAGE" --innodb-use-native-aio=OFF >/dev/null
mysql_clone() {
    docker exec -i "$NAME" sh -c 'MYSQL_PWD="${MYSQL_ROOT_PASSWORD:-$MARIADB_ROOT_PASSWORD}" exec mysql --protocol=tcp -h127.0.0.1 -u root -N -B "$@"' sh "$@"
}
ready=0
for _ in $(seq 1 90); do
    if mysql_clone -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo 'disposable locker-index database did not become ready' >&2; exit 1; }
create_db() {
    mysql_clone -e "CREATE DATABASE $1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
    mysql_clone "$1" < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
}
legacy_shape() {
    mysql_clone "$1" -e 'ALTER TABLE account_lockers ADD UNIQUE KEY uk_account_racewar (account_name,racewar), DROP INDEX account_name' >/dev/null
}
index_exact() {
    local db=$1 name=$2 expected=$3 result
    result=$(mysql_clone "$db" -e "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_lockers' AND index_name='$name'")
    [[ "$result" == "$expected" ]] || { echo "FAILED: $db index $name has $result metadata rows (expected $expected)" >&2; return 1; }
}
canonical() {
    local db=$1 signature support outbound inbound
    signature=$(mysql_clone "$db" -e "SELECT COUNT(*)=1 AND COALESCE(SUM(seq_in_index=1 AND column_name='account_name' AND non_unique=0 AND sub_part IS NULL),0)=1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_lockers' AND index_name='account_name'")
    support=$(mysql_clone "$db" -e "SELECT COUNT(*)=1 AND COALESCE(SUM(seq_in_index=1 AND column_name='account_name' AND non_unique=1 AND sub_part IS NULL),0)=1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_lockers' AND index_name='idx_account_name'")
    outbound=$(mysql_clone "$db" -e "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='account_lockers' AND column_name='account_name' AND referenced_table_name='accounts'")
    inbound=$(mysql_clone "$db" -e "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE referenced_table_schema=DATABASE() AND referenced_table_name='account_lockers' AND referenced_column_name='id' AND table_name='locker_chests'")
    [[ "$signature" == 1 && "$support" == 1 && "$outbound" == 1 && "$inbound" == 1 ]] || { echo 'FAILED: canonical index or real-FK graph differs' >&2; return 1; }
    index_exact "$db" uk_account_racewar 0
}
DB=locker_index_fixture
create_db "$DB"
legacy_shape "$DB"
mysql_clone "$DB" -e "
INSERT INTO accounts(account_name,password) VALUES('locker-probe','fixture-only');
INSERT INTO account_lockers(id,account_name,racewar) VALUES(9001,'locker-probe',1);
INSERT INTO locker_chests(id,locker_id,keyword) VALUES(9001,9001,'probe');" >/dev/null
if canonical "$DB" 2>/dev/null; then echo 'FAILED: fixture did not create legacy index shape' >&2; exit 1; fi
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null
canonical "$DB"
rows=$(mysql_clone "$DB" -e "SELECT (SELECT COUNT(*) FROM account_lockers WHERE id=9001 AND account_name='locker-probe' AND racewar=1)+(SELECT COUNT(*) FROM locker_chests WHERE id=9001 AND locker_id=9001)")
[[ "$rows" == 2 ]] || { echo 'FAILED: locker or child row changed' >&2; exit 1; }
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null
canonical "$DB"
mysql_clone "$DB" -e 'ALTER TABLE account_lockers ADD UNIQUE KEY uk_account_racewar (account_name,racewar)' >/dev/null
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null
canonical "$DB"
rows=$(mysql_clone "$DB" -e "SELECT (SELECT COUNT(*) FROM account_lockers WHERE id=9001 AND account_name='locker-probe' AND racewar=1)+(SELECT COUNT(*) FROM locker_chests WHERE id=9001 AND locker_id=9001)")
[[ "$rows" == 2 ]] || { echo 'FAILED: replay or partial-state recovery changed rows' >&2; exit 1; }
printf 'locker-index fixture: real FKs and child row retained; converge/replay/partial recovery pass\n'

DB=locker_index_conflict
create_db "$DB"
legacy_shape "$DB"
mysql_clone "$DB" -e "
INSERT INTO accounts(account_name,password) VALUES('locker-conflict','fixture-only');
INSERT INTO account_lockers(id,account_name,racewar) VALUES
    (9101,'locker-conflict',0),(9102,'locker-conflict',1);
INSERT INTO locker_chests(id,locker_id,keyword) VALUES(9102,9102,'must-retain');" >/dev/null
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null 2>&1; then
    echo 'FAILED: duplicate-account lockers silently reconciled' >&2; exit 1
fi
index_exact "$DB" account_name 0
index_exact "$DB" uk_account_racewar 2
rows=$(mysql_clone "$DB" -e "SELECT (SELECT COUNT(*) FROM account_lockers WHERE account_name='locker-conflict' AND id IN (9101,9102))+(SELECT COUNT(*) FROM locker_chests WHERE id=9102 AND locker_id=9102)")
[[ "$rows" == 3 ]] || { echo 'FAILED: duplicate refusal changed parent or child rows' >&2; exit 1; }
printf 'locker-index fixture: duplicate-account conflict refused without row or index mutation\n'

DB=locker_index_wrong_key
create_db "$DB"
legacy_shape "$DB"
mysql_clone "$DB" -e "
INSERT INTO accounts(account_name,password) VALUES('locker-wrong-key','fixture-only');
INSERT INTO account_lockers(id,account_name,racewar) VALUES(9201,'locker-wrong-key',1);
ALTER TABLE account_lockers ADD KEY account_name (racewar);" >/dev/null
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null 2>&1; then
    echo 'FAILED: wrong same-named account index was accepted' >&2; exit 1
fi
index_exact "$DB" account_name 1
index_exact "$DB" uk_account_racewar 2
rows=$(mysql_clone "$DB" -e "SELECT COUNT(*) FROM account_lockers WHERE id=9201 AND account_name='locker-wrong-key' AND racewar=1")
[[ "$rows" == 1 ]] || { echo 'FAILED: wrong-index refusal changed a locker row' >&2; exit 1; }
printf 'locker-index fixture: wrong same-named index refused without mutation\n'

DB=locker_index_dependent_fk
create_db "$DB"
legacy_shape "$DB"
mysql_clone "$DB" -e "
INSERT INTO accounts(account_name,password) VALUES('locker-dependent','fixture-only');
INSERT INTO account_lockers(id,account_name,racewar) VALUES(9301,'locker-dependent',1);
CREATE TABLE locker_pair_refs (
    id INT UNSIGNED PRIMARY KEY,
    account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
    racewar TINYINT NOT NULL,
    CONSTRAINT fk_locker_pair FOREIGN KEY (account_name,racewar)
        REFERENCES account_lockers(account_name,racewar)
) ENGINE=InnoDB;
INSERT INTO locker_pair_refs(id,account_name,racewar) VALUES(9301,'locker-dependent',1);" >/dev/null
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_locker_index.sql" >/dev/null 2>&1; then
    echo 'FAILED: inbound composite-key FK was accepted' >&2; exit 1
fi
index_exact "$DB" account_name 0
index_exact "$DB" uk_account_racewar 2
rows=$(mysql_clone "$DB" -e "SELECT (SELECT COUNT(*) FROM account_lockers WHERE id=9301 AND account_name='locker-dependent')+(SELECT COUNT(*) FROM locker_pair_refs WHERE id=9301 AND account_name='locker-dependent')")
[[ "$rows" == 2 ]] || { echo 'FAILED: dependent-FK refusal changed parent or child' >&2; exit 1; }
printf 'locker-index fixture: inbound composite-key FK refused with parent and child intact\n'
