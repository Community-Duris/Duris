#!/usr/bin/env bash
# Disposable engine fixture for canonical item_material order and value preservation.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
[[ ! -e "$ROOT/.env" && ! -e "$ROOT/migrations/.env" ]]
NAME="duris-accounting-schema-material-order-$$-$RANDOM"
PASSWORD="material-order-$$-$RANDOM"
IMAGE="${MATERIAL_ORDER_DB_IMAGE:-mysql:8.0}"
[[ "$IMAGE" =~ ^(mysql:8\.0(\.[0-9]+)?|mariadb:10\.11(\.[0-9]+)?)(@sha256:[0-9a-f]{64})?$ ]] || { echo 'unsupported test image' >&2; exit 2; }
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then ROOT_ENV=MARIADB_ROOT_PASSWORD; else ROOT_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -e "$ROOT_ENV=$PASSWORD" "$IMAGE" --innodb-use-native-aio=OFF >/dev/null
# TCP readiness excludes MySQL's temporary, socket-only initialization server.
mysql_clone() {
    docker exec -i "$NAME" sh -c 'MYSQL_PWD="${MYSQL_ROOT_PASSWORD:-$MARIADB_ROOT_PASSWORD}" exec mysql --protocol=tcp -h127.0.0.1 -u root -N -B "$@"' sh "$@"
}
ready=0
for _ in $(seq 1 90); do
    if mysql_clone -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo 'disposable material-order database did not become ready' >&2; exit 1; }
DB=material_order_fixture
mysql_clone -e "CREATE DATABASE $DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
mysql_clone "$DB" < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone "$DB" -e "
ALTER TABLE corpse_items MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE player_pet_items MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE shopkeeper_items MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE siege_items MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
INSERT INTO corpses(id,player_name,save_id) VALUES(9001,'material-probe',9001);
INSERT INTO corpse_items(id,corpse_id,vnum,item_material,bitvector1) VALUES(9001,9001,12345,21,101);
SET FOREIGN_KEY_CHECKS=0;
INSERT INTO player_pets(id,owner_pid,mob_vnum) VALUES(9001,9001,12345);
SET FOREIGN_KEY_CHECKS=1;
INSERT INTO player_pet_items(id,pet_id,vnum,item_material,bitvector1) VALUES(9001,9001,12345,22,102);
INSERT INTO shopkeepers(id,shop_id) VALUES(9001,9001);
INSERT INTO shopkeeper_items(id,shopkeeper_id,vnum,item_material,bitvector1) VALUES(9001,9001,12345,23,103);
INSERT INTO siege_items(id,room_vnum,vnum,item_material,bitvector1) VALUES(9001,9001,12345,24,104);" >/dev/null
if [[ "${MATERIAL_ORDER_DEBUG:-0}" == 1 ]]; then
    mysql_clone "$DB" -e "SELECT table_name,column_type,data_type,is_nullable,COALESCE(column_default,'<SQLNULL>'),extra,column_comment FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ('corpse_items','player_pet_items','shopkeeper_items','siege_items') AND column_name='item_material' ORDER BY table_name"
fi

snapshot() {
    local table result
    for table in corpse_items player_pet_items shopkeeper_items siege_items; do
        result=$(mysql_clone "$DB" -e "SELECT COUNT(*), COALESCE(SUM(id=9001 AND item_material IN (21,22,23,24) AND bitvector1 IN (101,102,103,104)),0), COALESCE(SUM(item_material),0), COALESCE(SUM(bitvector1),0) FROM $table")
        printf '%s:%s\n' "$table" "$result"
    done
}
order_ok() {
    local table anchor position
    for pair in corpse_items:item_condition player_pet_items:item_condition shopkeeper_items:obj_uid siege_items:updated_at; do
        table=${pair%%:*}; anchor=${pair#*:}
        position=$(mysql_clone "$DB" -e "SELECT (SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='$table' AND column_name='item_material')=(SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='$table' AND column_name='$anchor')+1")
        [[ "$position" == 1 ]] || { echo "FAILED: $table item_material has wrong position" >&2; return 1; }
    done
}
before=$(snapshot)
if order_ok 2>/dev/null; then echo 'FAILED: fixture did not create material-order drift' >&2; exit 1; fi
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_material_order.sql" >/dev/null
order_ok
[[ "$(snapshot)" == "$before" ]] || { echo 'FAILED: material or bitvector probe values changed' >&2; exit 1; }
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_material_order.sql" >/dev/null
order_ok
[[ "$(snapshot)" == "$before" ]] || { echo 'FAILED: replay changed material or bitvector values' >&2; exit 1; }
printf 'material-order fixture: four tables canonical, non-default values retained, replay safe\n'

mysql_clone -e 'CREATE DATABASE material_order_wrong_type CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci' >/dev/null
mysql_clone material_order_wrong_type < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
mysql_clone material_order_wrong_type -e '
ALTER TABLE corpse_items MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;
ALTER TABLE player_pet_items MODIFY COLUMN item_material SMALLINT DEFAULT NULL AFTER bitvector5' >/dev/null
if mysql_clone material_order_wrong_type < "$ROOT/migrations/legacy_archive_material_order.sql" >/dev/null 2>&1; then
    echo 'FAILED: wrong material type was silently rewritten' >&2; exit 1
fi
wrong_type=$(mysql_clone material_order_wrong_type -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pet_items' AND column_name='item_material' AND data_type='smallint' AND ordinal_position=(SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pet_items' AND column_name='bitvector5')+1")
first_unchanged=$(mysql_clone material_order_wrong_type -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='corpse_items' AND column_name='item_material' AND ordinal_position=(SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='corpse_items' AND column_name='bitvector5')+1")
[[ "$wrong_type" == 1 && "$first_unchanged" == 1 ]] || { echo 'FAILED: wrong-type preflight partially changed schema' >&2; exit 1; }
printf 'material-order fixture: second-table wrong type refused before first-table DDL\n'

mysql_clone material_order_wrong_type -e 'ALTER TABLE player_pet_items MODIFY COLUMN item_material TINYINT DEFAULT 7 AFTER bitvector5' >/dev/null
if mysql_clone material_order_wrong_type < "$ROOT/migrations/legacy_archive_material_order.sql" >/dev/null 2>&1; then
    echo 'FAILED: wrong material default was silently rewritten' >&2; exit 1
fi
wrong_default=$(mysql_clone material_order_wrong_type -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pet_items' AND column_name='item_material' AND column_default='7' AND ordinal_position=(SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pet_items' AND column_name='bitvector5')+1")
first_unchanged=$(mysql_clone material_order_wrong_type -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='corpse_items' AND column_name='item_material' AND ordinal_position=(SELECT ordinal_position FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='corpse_items' AND column_name='bitvector5')+1")
[[ "$wrong_default" == 1 && "$first_unchanged" == 1 ]] || { echo 'FAILED: wrong-default preflight partially changed schema' >&2; exit 1; }
printf 'material-order fixture: second-table wrong default refused before first-table DDL\n'
