#!/usr/bin/env bash
set -euo pipefail

NAME="duris-player-replacement-$$"
PASSWORD=$(printf 'replacement-%s-%s' "$$" "$RANDOM")
DB_NAME="player_replacement_test"

cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
}
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
    mysql -h127.0.0.1 -uroot -N -B)
MYSQL_FORCE=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" \
    mysql --force -h127.0.0.1 -uroot -N -B)

"${MYSQL[@]}" <<SQL
CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $DB_NAME;
CREATE TABLE player_timers (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    pid INT UNSIGNED NOT NULL,
    timer_id INT NOT NULL,
    timer_value DATETIME NOT NULL,
    UNIQUE KEY uk_pid_timer(pid,timer_id)
) ENGINE=InnoDB;
CREATE TABLE player_undead_slots (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    pid INT UNSIGNED NOT NULL,
    circle INT NOT NULL,
    slots INT NOT NULL,
    UNIQUE KEY uk_pid_circle(pid,circle)
) ENGINE=InnoDB;
CREATE TABLE player_forged_items (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    pid INT UNSIGNED NOT NULL,
    forge_index INT NOT NULL,
    item_vnum BIGINT NOT NULL,
    UNIQUE KEY uk_pid_forge(pid,forge_index)
) ENGINE=InnoDB;
CREATE TABLE player_granted_cmds (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    pid INT UNSIGNED NOT NULL,
    cmd_num INT NOT NULL,
    UNIQUE KEY uk_pid_cmd(pid,cmd_num)
) ENGINE=InnoDB;

INSERT INTO player_timers(pid,timer_id,timer_value)
VALUES (77,1,'2026-08-01 00:00:00'),(77,2,'2026-08-02 00:00:00');
INSERT INTO player_undead_slots(pid,circle,slots) VALUES (77,1,2),(77,2,3);
INSERT INTO player_forged_items(pid,forge_index,item_vnum) VALUES (77,1,1001),(77,2,1002);
INSERT INTO player_granted_cmds(pid,cmd_num) VALUES (77,10),(77,20);

START TRANSACTION;
DELETE FROM player_timers WHERE pid=77;
INSERT INTO player_timers(pid,timer_id,timer_value) VALUES (77,2,'2026-09-02 00:00:00');
DELETE FROM player_undead_slots WHERE pid=77;
INSERT INTO player_undead_slots(pid,circle,slots) VALUES (77,2,8);
DELETE FROM player_forged_items WHERE pid=77;
INSERT INTO player_forged_items(pid,forge_index,item_vnum) VALUES (77,2,2002);
DELETE FROM player_granted_cmds WHERE pid=77;
INSERT INTO player_granted_cmds(pid,cmd_num) VALUES (77,20);
COMMIT;
SQL

[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT CONCAT(timer_id,':',DATE_FORMAT(timer_value,'%Y-%m-%d')) FROM player_timers WHERE pid=77;") == "2:2026-09-02" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT CONCAT(circle,':',slots) FROM player_undead_slots WHERE pid=77;") == "2:8" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT CONCAT(forge_index,':',item_vnum) FROM player_forged_items WHERE pid=77;") == "2:2002" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT cmd_num FROM player_granted_cmds WHERE pid=77;") == "20" ]]

"${MYSQL[@]}" "$DB_NAME" -e "
START TRANSACTION;
DELETE FROM player_timers WHERE pid=77;
DELETE FROM player_undead_slots WHERE pid=77;
DELETE FROM player_forged_items WHERE pid=77;
DELETE FROM player_granted_cmds WHERE pid=77;
COMMIT;"
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT (SELECT COUNT(*) FROM player_timers)+(SELECT COUNT(*) FROM player_undead_slots)+(SELECT COUNT(*) FROM player_forged_items)+(SELECT COUNT(*) FROM player_granted_cmds);") == "0" ]]

"${MYSQL[@]}" "$DB_NAME" -e "
INSERT INTO player_timers(pid,timer_id,timer_value) VALUES (77,4,'2026-10-04 00:00:00');
INSERT INTO player_undead_slots(pid,circle,slots) VALUES (77,4,9);
INSERT INTO player_forged_items(pid,forge_index,item_vnum) VALUES (77,4,4004);
INSERT INTO player_granted_cmds(pid,cmd_num) VALUES (77,40);"

force_insert_failure() {
    local table=$1
    local insert_sql=$2
    local output
    set +e
    output=$(printf 'START TRANSACTION;\nDELETE FROM %s WHERE pid=77;\n%s\nROLLBACK;\n' \
        "$table" "$insert_sql" | "${MYSQL_FORCE[@]}" "$DB_NAME" 2>&1)
    set -e
    [[ "$output" == *"cannot be null"* ]]
}

force_insert_failure player_timers \
    "INSERT INTO player_timers(pid,timer_id,timer_value) VALUES (77,5,NULL);"
force_insert_failure player_undead_slots \
    "INSERT INTO player_undead_slots(pid,circle,slots) VALUES (77,5,NULL);"
force_insert_failure player_forged_items \
    "INSERT INTO player_forged_items(pid,forge_index,item_vnum) VALUES (77,5,NULL);"
force_insert_failure player_granted_cmds \
    "INSERT INTO player_granted_cmds(pid,cmd_num) VALUES (77,NULL);"

[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM player_timers WHERE pid=77 AND timer_id=4;") == "1" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM player_undead_slots WHERE pid=77 AND circle=4;") == "1" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM player_forged_items WHERE pid=77 AND forge_index=4;") == "1" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM player_granted_cmds WHERE pid=77 AND cmd_num=40;") == "1" ]]

"${MYSQL[@]}" "$DB_NAME" -e "
CREATE TRIGGER reject_timer_delete BEFORE DELETE ON player_timers
FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='forced delete failure';"
set +e
delete_output=$("${MYSQL_FORCE[@]}" "$DB_NAME" 2>&1 <<SQL
START TRANSACTION;
DELETE FROM player_timers WHERE pid=77;
ROLLBACK;
SQL
)
set -e
[[ "$delete_output" == *"forced delete failure"* ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM player_timers WHERE pid=77 AND timer_id=4;") == "1" ]]

printf 'player replacement-state MySQL regression passed\n'
