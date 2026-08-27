#!/usr/bin/env bash
set -euo pipefail

NAME="duris-account-bank-$PPID-$$"
PASSWORD=$(printf 'bank-delta-%s-%s' "$$" "$RANDOM")
DB_NAME="account_bank_delta_test"

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

"${MYSQL[@]}" <<SQL
CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $DB_NAME;
CREATE TABLE account_banks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    racewar TINYINT NOT NULL DEFAULT 0,
    bank_copper BIGINT UNSIGNED NOT NULL DEFAULT 0,
    bank_silver BIGINT UNSIGNED NOT NULL DEFAULT 0,
    bank_gold BIGINT UNSIGNED NOT NULL DEFAULT 0,
    bank_platinum BIGINT UNSIGNED NOT NULL DEFAULT 0,
    UNIQUE KEY uk_account_racewar(account_name,racewar)
) ENGINE=InnoDB;
INSERT INTO account_banks(account_name,racewar,bank_copper)
VALUES ('shared',0,100),('concurrent',0,0),('vector',0,3);
UPDATE account_banks SET bank_silver=2,bank_gold=1,bank_platinum=1
WHERE account_name='vector' AND racewar=0;

START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper+25
WHERE account_name='shared' AND racewar=0;
SELECT bank_copper FROM account_banks WHERE account_name='shared' AND racewar=0;
COMMIT;
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper+40
WHERE account_name='shared' AND racewar=0;
SELECT bank_copper FROM account_banks WHERE account_name='shared' AND racewar=0;
COMMIT;
SQL

[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT bank_copper FROM account_banks WHERE account_name='shared' AND racewar=0;") == "165" ]]

withdraw_result=$("${MYSQL[@]}" "$DB_NAME" <<'SQL'
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper-70
WHERE account_name='shared' AND racewar=0 AND bank_copper>=70;
SELECT ROW_COUNT();
SELECT bank_copper FROM account_banks WHERE account_name='shared' AND racewar=0;
COMMIT;
SQL
)
[[ "$withdraw_result" == $'1\n95' ]]

insufficient_result=$("${MYSQL[@]}" "$DB_NAME" <<'SQL'
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper-100
WHERE account_name='shared' AND racewar=0 AND bank_copper>=100;
SELECT ROW_COUNT();
ROLLBACK;
SELECT bank_copper FROM account_banks WHERE account_name='shared' AND racewar=0;
SQL
)
[[ "$insufficient_result" == $'0\n95' ]]

"${MYSQL[@]}" "$DB_NAME" -e "
INSERT INTO account_banks(account_name,racewar,bank_copper) VALUES ('failure',0,50);
CREATE TRIGGER reject_failure_update BEFORE UPDATE ON account_banks
FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='forced bank update failure';"
set +e
failure_output=$("${MYSQL[@]}" "$DB_NAME" 2>&1 <<'SQL'
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper+25
WHERE account_name='failure' AND racewar=0;
ROLLBACK;
SQL
)
failure_status=$?
set -e
[[ $failure_status -ne 0 ]]
[[ "$failure_output" == *"forced bank update failure"* ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT bank_copper FROM account_banks WHERE account_name='failure' AND racewar=0;") == "50" ]]
"${MYSQL[@]}" "$DB_NAME" -e "DROP TRIGGER reject_failure_update;"

"${MYSQL[@]}" "$DB_NAME" <<'SQL'
START TRANSACTION;
SELECT bank_copper,bank_silver,bank_gold,bank_platinum
FROM account_banks WHERE account_name='vector' AND racewar=0 FOR UPDATE;
UPDATE account_banks SET bank_copper=bank_copper-3,bank_silver=bank_silver-2,
    bank_gold=bank_gold-1,bank_platinum=bank_platinum-0
WHERE account_name='vector' AND racewar=0
AND bank_copper>=3 AND bank_silver>=2 AND bank_gold>=1 AND bank_platinum>=0;
SELECT bank_copper,bank_silver,bank_gold,bank_platinum
FROM account_banks WHERE account_name='vector' AND racewar=0;
COMMIT;
SQL
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT CONCAT_WS(',',bank_copper,bank_silver,bank_gold,bank_platinum) FROM account_banks WHERE account_name='vector';") == "0,0,0,1" ]]

("${MYSQL[@]}" "$DB_NAME" >/dev/null <<'SQL'
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper+10
WHERE account_name='concurrent' AND racewar=0;
SELECT SLEEP(1);
COMMIT;
SQL
) &
first_client=$!
"${MYSQL[@]}" "$DB_NAME" >/dev/null <<'SQL'
START TRANSACTION;
UPDATE account_banks SET bank_copper=bank_copper+20
WHERE account_name='concurrent' AND racewar=0;
COMMIT;
SQL
wait "$first_client"
[[ $("${MYSQL[@]}" "$DB_NAME" -e \
    "SELECT bank_copper FROM account_banks WHERE account_name='concurrent' AND racewar=0;") == "30" ]]

printf 'account-bank isolated MySQL delta regression passed\n'
