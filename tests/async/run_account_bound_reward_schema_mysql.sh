#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-account-reward-schema-$$"
PASSWORD=$(printf 'account-reward-%s-%s' "$$" "$RANDOM")
LEGACY_DB="account_reward_legacy"
FRESH_DB="account_reward_fresh"

cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker run --rm -d --name "$NAME" \
    -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null

for _ in $(seq 1 60); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" \
        mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" \
    mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null

MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B)

"${MYSQL[@]}" <<SQL
CREATE DATABASE $LEGACY_DB CHARACTER SET latin1 COLLATE latin1_swedish_ci;
CREATE DATABASE $FRESH_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $LEGACY_DB;
CREATE TABLE accounts (
    account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
    PRIMARY KEY (account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO accounts (account_name) VALUES ('migration_probe'), ('cascade_probe');
CREATE TABLE account_bound_rewards (
    account_name VARCHAR(50) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL,
    reward_vnum VARCHAR(20) NULL DEFAULT NULL,
    created_at DATETIME NULL DEFAULT NULL,
    PRIMARY KEY (account_name),
    KEY idx_account_bound_rewards_vnum (account_name)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
INSERT INTO account_bound_rewards (account_name, reward_vnum, created_at)
VALUES ('migration_probe', '36419', '2026-07-17 00:00:00');
SQL

docker cp "$ROOT/migrations/account_bound_rewards.sql" \
    "$NAME:/tmp/account_bound_rewards.sql" >/dev/null
docker cp "$ROOT/migrations/bootstrap_multithread_safe.sql" \
    "$NAME:/tmp/bootstrap_multithread_safe.sql" >/dev/null

apply_reward_migration() {
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$LEGACY_DB' < /tmp/account_bound_rewards.sql"
}

schema_signature() {
    local database=$1
    "${MYSQL[@]}" "$database" -e "
SELECT CONCAT('T|', table_name, '|', engine, '|', table_collation)
FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_name='account_bound_rewards'
UNION ALL
SELECT CONCAT('C|', ordinal_position, '|', column_name, '|', column_type, '|', is_nullable, '|',
              COALESCE(column_default, '<NULL>'), '|', extra, '|',
              COALESCE(character_set_name, '<NULL>'), '|', COALESCE(collation_name, '<NULL>'))
FROM information_schema.columns
WHERE table_schema=DATABASE() AND table_name='account_bound_rewards'
UNION ALL
SELECT CONCAT('I|', index_name, '|', non_unique, '|', seq_in_index, '|', column_name)
FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name='account_bound_rewards'
UNION ALL
SELECT CONCAT('F|', kcu.constraint_name, '|', kcu.column_name, '|',
              kcu.referenced_table_name, '|', kcu.referenced_column_name, '|',
              rc.update_rule, '|', rc.delete_rule)
FROM information_schema.key_column_usage kcu
JOIN information_schema.referential_constraints rc
  ON rc.constraint_schema=kcu.constraint_schema
 AND rc.table_name=kcu.table_name
 AND rc.constraint_name=kcu.constraint_name
WHERE kcu.constraint_schema=DATABASE()
  AND kcu.table_name='account_bound_rewards'
  AND kcu.referenced_table_name IS NOT NULL
ORDER BY 1;"
}

# RED on the original implementation: CREATE TABLE IF NOT EXISTS cannot repair
# the deliberately partial existing table.
apply_reward_migration
"${MYSQL[@]}" "$FRESH_DB" < "$ROOT/migrations/bootstrap_multithread_safe.sql"

legacy_signature=$(schema_signature "$LEGACY_DB")
fresh_signature=$(schema_signature "$FRESH_DB")
[[ "$legacy_signature" == "$fresh_signature" ]]

# Existing assignments must survive convergence.
[[ $("${MYSQL[@]}" "$LEGACY_DB" -e \
    "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36419 AND created_at='2026-07-17 00:00:00';") == "1" ]]

# Exercise the runtime UPSERT contract on both paths.
for database in "$LEGACY_DB" "$FRESH_DB"; do
    if [[ "$database" == "$FRESH_DB" ]]; then
        "${MYSQL[@]}" "$database" -e \
            "INSERT INTO accounts (account_name) VALUES ('migration_probe'), ('cascade_probe');"
    fi
    "${MYSQL[@]}" "$database" -e \
        "INSERT INTO account_bound_rewards (account_name,reward_vnum,granted_by)
         VALUES ('migration_probe',36419,'Xanadin')
         ON DUPLICATE KEY UPDATE reward_vnum=VALUES(reward_vnum),
             granted_by=VALUES(granted_by),updated_at=CURRENT_TIMESTAMP;"
    [[ $("${MYSQL[@]}" "$database" -e \
        "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36419 AND granted_by='Xanadin';") == "1" ]]

    # Distinct vnums coexist; replaying the same account/vnum remains idempotent.
    "${MYSQL[@]}" "$database" -e \
        "INSERT INTO account_bound_rewards (account_name,reward_vnum,granted_by)
         VALUES ('migration_probe',36420,'Xanadin')
         ON DUPLICATE KEY UPDATE granted_by=VALUES(granted_by),updated_at=CURRENT_TIMESTAMP;
         INSERT INTO account_bound_rewards (account_name,reward_vnum,granted_by)
         VALUES ('migration_probe',36420,'Xanadin')
         ON DUPLICATE KEY UPDATE granted_by=VALUES(granted_by),updated_at=CURRENT_TIMESTAMP;"
    [[ $("${MYSQL[@]}" "$database" -e \
        "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe';") == "2" ]]
    "${MYSQL[@]}" "$database" -e \
        "DELETE FROM account_bound_rewards
         WHERE account_name='migration_probe' AND reward_vnum=36420;"
    [[ $("${MYSQL[@]}" "$database" -e \
        "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36419;") == "1" ]]

    "${MYSQL[@]}" "$database" -e \
        "INSERT INTO account_bound_rewards (account_name,reward_vnum,granted_by)
         VALUES ('cascade_probe',36419,'Xanadin');
         DELETE FROM accounts WHERE account_name='cascade_probe';"
    [[ $("${MYSQL[@]}" "$database" -e \
        "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='cascade_probe';") == "0" ]]
done

# Replay must preserve rows and structure exactly.
before_replay=$(schema_signature "$LEGACY_DB")
apply_reward_migration
after_replay=$(schema_signature "$LEGACY_DB")
[[ "$before_replay" == "$after_replay" ]]
[[ $("${MYSQL[@]}" "$LEGACY_DB" -e \
    "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe';") == "1" ]]

printf 'account reward migration/bootstrap convergence and replay: ok\n'
