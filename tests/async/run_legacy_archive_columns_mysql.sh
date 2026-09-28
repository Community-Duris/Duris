#!/usr/bin/env bash
# Disposable MySQL proof: preserve every archive-only value before removing it.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
[[ ! -e "$ROOT/.env" && ! -e "$ROOT/migrations/.env" ]]
NAME="duris-accounting-schema-archive-columns-$$-$RANDOM"
PASSWORD="archive-columns-$$-$RANDOM"
IMAGE="${ARCHIVE_COLUMN_DB_IMAGE:-mysql:8.0}"
case "$IMAGE" in mysql:8.0|mariadb:10.11) ;; *) echo 'unsupported test image' >&2; exit 2;; esac
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then ROOT_ENV=MARIADB_ROOT_PASSWORD; else ROOT_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -e "$ROOT_ENV=$PASSWORD" "$IMAGE" >/dev/null
mysql_clone() {
    docker exec -i "$NAME" sh -c 'MYSQL_PWD="${MYSQL_ROOT_PASSWORD:-$MARIADB_ROOT_PASSWORD}" exec mysql -u root -N -B "$@"' sh "$@"
}
prepare_clone_guard() {
    mysql_clone "$1" < "$ROOT/migrations/prepare_archive_offline_clone_guard.sql" >/dev/null
}
ready=0
for _ in $(seq 1 90); do
    if mysql_clone -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo 'disposable test database did not become ready' >&2; exit 1; }
mysql_clone -e 'CREATE DATABASE archive_columns_fixture CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone archive_columns_fixture < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone archive_columns_fixture -e "
ALTER TABLE ships
    ADD COLUMN owner_pid INT UNSIGNED DEFAULT NULL,
    ADD COLUMN crew_index INT DEFAULT 0,
    ADD COLUMN crew_sail_skill INT DEFAULT 0,
    ADD COLUMN crew_guns_skill INT DEFAULT 0,
    ADD COLUMN crew_rpar_skill INT DEFAULT 0,
    ADD COLUMN crew_sail_chief INT DEFAULT 0,
    ADD COLUMN crew_guns_chief INT DEFAULT 0,
    ADD COLUMN crew_rpar_chief INT DEFAULT 0,
    ADD COLUMN maxspeed_bonus INT DEFAULT 0,
    ADD COLUMN capacity_bonus INT DEFAULT 0,
    ADD KEY idx_ships_owner_pid (owner_pid);
ALTER TABLE account_locker_items ADD COLUMN item_type TINYINT DEFAULT NULL;
INSERT INTO ships(id,owner_name,ship_name,owner_pid,crew_index,crew_sail_skill,
    crew_guns_skill,crew_rpar_skill,crew_sail_chief,crew_guns_chief,
    crew_rpar_chief,maxspeed_bonus,capacity_bonus)
VALUES (90001,'archive-probe','preserved',70001,101,102,103,104,105,106,107,108,109);
INSERT INTO accounts(account_name,password) VALUES('archive-probe','fixture-only');
INSERT INTO account_lockers(id,account_name,racewar) VALUES(90001,'archive-probe',0);
INSERT INTO locker_chests(id,locker_id,keyword) VALUES(90001,90001,'main');
INSERT INTO account_locker_items(id,chest_id,vnum,item_type)
VALUES(90001,90001,12345,7);" >/dev/null
prepare_clone_guard archive_columns_fixture
mysql_clone archive_columns_fixture < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null
verify() {
    local extras ships_saved locker_saved live_rows
    extras=$(mysql_clone archive_columns_fixture -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='ships' AND column_name IN ('owner_pid','crew_index','crew_sail_skill','crew_guns_skill','crew_rpar_skill','crew_sail_chief','crew_guns_chief','crew_rpar_chief','maxspeed_bonus','capacity_bonus')) OR (table_name='account_locker_items' AND column_name='item_type'))")
    [[ "$extras" == 0 ]] || { echo "FAILED: archive-only runtime columns remain ($extras)" >&2; return 1; }
    ships_saved=$(mysql_clone archive_columns_fixture -e "SELECT COUNT(*)=1 AND COALESCE(SUM(id=90001 AND owner_pid=70001 AND crew_index=101 AND crew_sail_skill=102 AND crew_guns_skill=103 AND crew_rpar_skill=104 AND crew_sail_chief=105 AND crew_guns_chief=106 AND crew_rpar_chief=107 AND maxspeed_bonus=108 AND capacity_bonus=109),0)=1 FROM legacy_import_archive_ships")
    locker_saved=$(mysql_clone archive_columns_fixture -e "SELECT COUNT(*)=1 AND COALESCE(SUM(id=90001 AND item_type=7 AND chest_id=90001 AND vnum=12345),0)=1 FROM legacy_import_archive_account_locker_items")
    live_rows=$(mysql_clone archive_columns_fixture -e "SELECT (SELECT COUNT(*) FROM ships WHERE id=90001 AND owner_name='archive-probe')+(SELECT COUNT(*) FROM account_locker_items WHERE id=90001 AND vnum=12345)")
    [[ "$ships_saved" == 1 && "$locker_saved" == 1 && "$live_rows" == 2 ]] || { echo "FAILED: preserved archive values or live rows differ: ship=$ships_saved locker=$locker_saved live=$live_rows" >&2; return 1; }
}
verify
mysql_clone archive_columns_fixture < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null
verify
printf 'archive-column fixture: non-default values retained, runtime columns reconciled, replay safe\n'

# An existing archive table is not an excuse to overwrite earlier evidence.
mysql_clone -e 'CREATE DATABASE archive_columns_collision CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone archive_columns_collision < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone archive_columns_collision -e "ALTER TABLE ships ADD COLUMN owner_pid INT UNSIGNED DEFAULT NULL; INSERT INTO ships(id,owner_name,owner_pid) VALUES(90002,'collision-probe',70002); CREATE TABLE legacy_import_archive_ships LIKE ships;" >/dev/null
prepare_clone_guard archive_columns_collision
if mysql_clone archive_columns_collision < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null 2>&1; then
    echo 'FAILED: existing sidecar was accepted during archive copy' >&2; exit 1
fi
collision=$(mysql_clone archive_columns_collision -e "SELECT (SELECT COUNT(*) FROM ships WHERE id=90002 AND owner_pid=70002)+(SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='ships' AND column_name='owner_pid')")
[[ "$collision" == 2 ]] || { echo 'FAILED: sidecar collision altered the source row/column' >&2; exit 1; }
printf 'archive-column fixture: pre-existing sidecar rejected without source mutation\n'

# A partial ship schema is not a license to discard a subset of values.
mysql_clone -e 'CREATE DATABASE archive_columns_partial CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone archive_columns_partial < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone archive_columns_partial -e "ALTER TABLE ships ADD COLUMN crew_index INT DEFAULT 0; INSERT INTO ships(id,owner_name,crew_index) VALUES(90003,'partial-probe',777);" >/dev/null
prepare_clone_guard archive_columns_partial
if mysql_clone archive_columns_partial < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null 2>&1; then
    echo 'FAILED: partial ship schema was accepted' >&2; exit 1
fi
partial=$(mysql_clone archive_columns_partial -e "SELECT (SELECT COUNT(*) FROM ships WHERE id=90003 AND crew_index=777)+(SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='ships' AND column_name='crew_index')+(SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legacy_import_archive_ships')")
[[ "$partial" == 2 ]] || { echo 'FAILED: partial schema refusal altered source or created sidecar' >&2; exit 1; }
printf 'archive-column fixture: partial schema rejected before copying or mutation\n'

# An outbound FK can make the owner index undroppable. Refuse before copying,
# otherwise a failed DDL leaves a sidecar that blocks a subsequent replay.
mysql_clone -e 'CREATE DATABASE archive_columns_fk_guard CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone archive_columns_fk_guard < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone archive_columns_fk_guard -e "
ALTER TABLE ships
    ADD COLUMN owner_pid INT UNSIGNED DEFAULT NULL,
    ADD COLUMN crew_index INT DEFAULT 0,
    ADD COLUMN crew_sail_skill INT DEFAULT 0,
    ADD COLUMN crew_guns_skill INT DEFAULT 0,
    ADD COLUMN crew_rpar_skill INT DEFAULT 0,
    ADD COLUMN crew_sail_chief INT DEFAULT 0,
    ADD COLUMN crew_guns_chief INT DEFAULT 0,
    ADD COLUMN crew_rpar_chief INT DEFAULT 0,
    ADD COLUMN maxspeed_bonus INT DEFAULT 0,
    ADD COLUMN capacity_bonus INT DEFAULT 0,
    ADD KEY idx_ships_owner_pid (owner_pid);
CREATE TABLE archive_owners(owner_pid INT UNSIGNED PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO archive_owners VALUES (70004);
ALTER TABLE ships ADD CONSTRAINT fk_archive_ships_owner
    FOREIGN KEY (owner_pid) REFERENCES archive_owners(owner_pid);
INSERT INTO ships(id,owner_name,owner_pid,crew_index)
    VALUES (90004,'fk-probe',70004,444);" >/dev/null
prepare_clone_guard archive_columns_fk_guard
if mysql_clone archive_columns_fk_guard < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null 2>&1; then
    echo 'FAILED: FK-dependent owner index was removed' >&2; exit 1
fi
fk_guard=$(mysql_clone archive_columns_fk_guard -e "SELECT (SELECT COUNT(*) FROM ships WHERE id=90004 AND owner_pid=70004 AND crew_index=444)+(SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legacy_import_archive_ships')")
[[ "$fk_guard" == 1 ]] || { echo 'FAILED: FK refusal left a sidecar or altered source row' >&2; exit 1; }
printf 'archive-column fixture: FK-dependent index refused before sidecar creation\n'

# A schema with all legacy columns but no explicit offline-clone marker must
# never remove them, even when no writer is visible at this instant.
mysql_clone -e 'CREATE DATABASE archive_columns_unmarked CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone archive_columns_unmarked < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone archive_columns_unmarked -e "
ALTER TABLE ships
    ADD COLUMN owner_pid INT UNSIGNED DEFAULT NULL,
    ADD COLUMN crew_index INT DEFAULT 0,
    ADD COLUMN crew_sail_skill INT DEFAULT 0,
    ADD COLUMN crew_guns_skill INT DEFAULT 0,
    ADD COLUMN crew_rpar_skill INT DEFAULT 0,
    ADD COLUMN crew_sail_chief INT DEFAULT 0,
    ADD COLUMN crew_guns_chief INT DEFAULT 0,
    ADD COLUMN crew_rpar_chief INT DEFAULT 0,
    ADD COLUMN maxspeed_bonus INT DEFAULT 0,
    ADD COLUMN capacity_bonus INT DEFAULT 0;
INSERT INTO ships(id,owner_name,owner_pid,crew_index)
    VALUES (90005,'unmarked-probe',70005,555);" >/dev/null
if mysql_clone archive_columns_unmarked < "$ROOT/migrations/legacy_archive_schema_reconciliation.sql" >/dev/null 2>&1; then
    echo 'FAILED: unmarked database accepted archive-only column removal' >&2; exit 1
fi
unmarked=$(mysql_clone archive_columns_unmarked -e "SELECT (SELECT COUNT(*) FROM ships WHERE id=90005 AND owner_pid=70005 AND crew_index=555)+(SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legacy_import_archive_ships')")
[[ "$unmarked" == 1 ]] || { echo 'FAILED: unmarked refusal altered source or created sidecar' >&2; exit 1; }
printf 'archive-column fixture: unmarked database refused before copying or mutation\n'
