#!/usr/bin/env bash
set -euo pipefail

NAME="duris-corpse-persistence-$$"
PASSWORD="corpse-persistence-test"
DATABASE="corpse_persistence_test"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker run --rm -d --name "$NAME" -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
for _ in $(seq 1 60); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null
docker cp "$ROOT/migrations/corpse_persistence_state.sql" "$NAME:/tmp/corpse_persistence_state.sql"

MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B)
"${MYSQL[@]}" -e "
CREATE DATABASE $DATABASE CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $DATABASE;
CREATE TABLE corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    UNIQUE KEY uk_player_saveid (player_name, save_id)
);
INSERT INTO corpses (player_name, save_id, short_descr, description) VALUES
    ('Amoz', 1001, '100', 'the corpse of Amoz'),
    ('Numeral', 1002, '100', 'a legitimate numbered object'),
    ('Intact', 1003, 'the corpse of Intact', 'The corpse of an Elf is lying here.');
"

apply_migration() {
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$DATABASE' < /tmp/corpse_persistence_state.sql"
}

assert_equal() {
    local expected="$1"
    local actual="$2"
    local label="$3"
    if [[ "$actual" != "$expected" ]]; then
        printf '%s: expected <%s>, got <%s>\n' "$label" "$expected" "$actual" >&2
        exit 1
    fi
}

apply_migration

columns=$("${MYSQL[@]}" "$DATABASE" -e "
SELECT COUNT(*) FROM information_schema.columns
WHERE table_schema = DATABASE() AND table_name = 'corpses'
  AND column_name IN ('name','weight','value0','value1','value2','value3','value4','value5','value7');")
assert_equal "9" "$columns" "outer corpse columns"

repaired=$("${MYSQL[@]}" "$DATABASE" -e "
SELECT CONCAT_WS('|', short_descr, description, name, value1,
                     IF(value2 IS NULL, 'NULL', value2))
FROM corpses WHERE player_name = 'Amoz';")
assert_equal "the corpse of Amoz|The corpse of Amoz is lying here.|Amoz corpse _pcorpse_|1|NULL" "$repaired" "corrupt row repair"

untouched=$("${MYSQL[@]}" "$DATABASE" -e "
SELECT CONCAT_WS('|', short_descr, description) FROM corpses WHERE player_name = 'Numeral';")
assert_equal "100|a legitimate numbered object" "$untouched" "guarded repair signature"

"${MYSQL[@]}" "$DATABASE" -e "
INSERT INTO corpses (
    player_name, save_id, room_vnum, short_descr, description, name, weight,
    value0, value1, value2, value3, value4, value5, value7
) VALUES (
    'Roundtrip', 2001, 42, 'corpse short', 'corpse long',
    'Roundtrip corpse _pcorpse_', 913, 401, 65537, 56, 9876, -123456, 3, 55
);"

roundtrip=$("${MYSQL[@]}" "$DATABASE" -e "
SELECT CONCAT_WS('|', name, weight, value0, value1, value2, value3, value4, value5,
                     save_id, value7, short_descr, description)
FROM corpses WHERE player_name = 'Roundtrip';")
assert_equal "Roundtrip corpse _pcorpse_|913|401|65537|56|9876|-123456|3|2001|55|corpse short|corpse long" "$roundtrip" "outer corpse state round trip"

apply_migration
replay_count=$("${MYSQL[@]}" "$DATABASE" -e "SELECT COUNT(*) FROM corpses;")
assert_equal "4" "$replay_count" "migration replay preserves rows"

echo "player corpse persistence schema: ok"
