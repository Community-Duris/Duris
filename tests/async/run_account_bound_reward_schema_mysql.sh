#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-account-reward-schema-$$"
PASSWORD=$(printf 'account-reward-%s-%s' "$$" "$RANDOM")
LEGACY_DB="account_reward_legacy"
FRESH_DB="account_reward_fresh"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT

docker run --rm -d --name "$NAME" -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
for _ in $(seq 1 60); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then break; fi
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B)

"${MYSQL[@]}" <<SQL
CREATE DATABASE $LEGACY_DB CHARACTER SET latin1 COLLATE latin1_swedish_ci;
CREATE DATABASE $FRESH_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $LEGACY_DB;
CREATE TABLE accounts (
 account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
 PRIMARY KEY(account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE player_data (
 pid INT UNSIGNED NOT NULL AUTO_INCREMENT,
 name VARCHAR(64) NOT NULL,
 account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
 PRIMARY KEY(pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO accounts(account_name) VALUES ('migration_probe'),('cascade_probe');
INSERT INTO player_data(pid,name,account_name) VALUES (1,'Probe','migration_probe'),(2,'Otherprobe','migration_probe');
CREATE TABLE account_bound_rewards (
 account_name VARCHAR(50) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL,
 reward_vnum VARCHAR(20) NULL DEFAULT NULL,
 created_at DATETIME NULL DEFAULT NULL,
 PRIMARY KEY(account_name),
 KEY idx_account_bound_rewards_vnum(account_name)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
INSERT INTO account_bound_rewards(account_name,reward_vnum,created_at)
VALUES ('migration_probe','36419','2026-07-17 00:00:00');
CREATE TABLE account_bound_reward_summons (
 grant_id VARCHAR(30) NOT NULL,
 pid BIGINT NOT NULL,
 last_summoned_at TIMESTAMP NULL DEFAULT NULL,
 PRIMARY KEY(pid),
 KEY idx_account_bound_reward_summons_pid(grant_id)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
CREATE TABLE account_bound_reward_pwipe_state (
 id INT NOT NULL,
 last_processed_at TIMESTAMP NULL DEFAULT NULL,
 PRIMARY KEY(id)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
INSERT INTO account_bound_reward_pwipe_state(id,last_processed_at)
VALUES (1,'2026-06-01 00:00:00'),(2,'2026-07-01 00:00:00');
SQL

docker cp "$ROOT/migrations/account_bound_rewards.sql" "$NAME:/tmp/account_bound_rewards.sql" >/dev/null
docker cp "$ROOT/migrations/bootstrap_multithread_safe.sql" "$NAME:/tmp/bootstrap_multithread_safe.sql" >/dev/null
docker cp "$ROOT/migrations/verify_account_bound_rewards.sh" "$NAME:/tmp/verify_account_bound_rewards.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify_account_bound_rewards.sh
apply_reward_migration() {
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$LEGACY_DB' < /tmp/account_bound_rewards.sql"
}
schema_signature() {
    local database=$1
    "${MYSQL[@]}" "$database" -e "
SELECT CONCAT('T|',table_name,'|',engine,'|',table_collation)
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('account_bound_rewards','account_bound_reward_summons','account_bound_reward_pwipe_state')
UNION ALL
SELECT CONCAT('C|',table_name,'|',ordinal_position,'|',column_name,'|',column_type,'|',is_nullable,'|',COALESCE(column_default,'<NULL>'),'|',extra,'|',COALESCE(collation_name,'<NULL>'))
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ('account_bound_rewards','account_bound_reward_summons','account_bound_reward_pwipe_state')
UNION ALL
SELECT CONCAT('I|',table_name,'|',index_name,'|',non_unique,'|',seq_in_index,'|',column_name)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN ('account_bound_rewards','account_bound_reward_summons','account_bound_reward_pwipe_state')
UNION ALL
SELECT CONCAT('F|',kcu.table_name,'|',kcu.constraint_name,'|',kcu.column_name,'|',kcu.referenced_table_name,'|',kcu.referenced_column_name,'|',rc.update_rule,'|',rc.delete_rule)
FROM information_schema.key_column_usage kcu JOIN information_schema.referential_constraints rc
 ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name IN ('account_bound_rewards','account_bound_reward_summons','account_bound_reward_pwipe_state') AND kcu.referenced_table_name IS NOT NULL
ORDER BY 1;"
}

apply_reward_migration
"${MYSQL[@]}" "$FRESH_DB" < "$ROOT/migrations/bootstrap_multithread_safe.sql"
legacy_signature=$(schema_signature "$LEGACY_DB")
fresh_signature=$(schema_signature "$FRESH_DB")
[[ "$legacy_signature" == "$fresh_signature" ]]

# Legacy assignment survives with stable ID and version-0 fallback semantics.
[[ $("${MYSQL[@]}" "$LEGACY_DB" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36419 AND created_at='2026-07-17 00:00:00' AND id>0 AND template_version=0 AND template_json IS NULL;") == 1 ]]
[[ $("${MYSQL[@]}" "$LEGACY_DB" -e "SELECT COUNT(*) FROM account_bound_reward_pwipe_state WHERE id=1 AND last_processed_at='2026-07-01 00:00:00';") == 1 ]]
[[ $("${MYSQL[@]}" "$LEGACY_DB" -e "SELECT COUNT(*) FROM account_bound_reward_pwipe_state;") == 1 ]]

for database in "$LEGACY_DB" "$FRESH_DB"; do
    if [[ "$database" == "$FRESH_DB" ]]; then
        "${MYSQL[@]}" "$database" -e "INSERT INTO accounts(account_name) VALUES ('migration_probe'),('cascade_probe'); INSERT INTO player_data(pid,name,account_name) VALUES (1,'Probe','migration_probe'),(2,'Otherprobe','migration_probe');"

        # Offline forced removal reparents direct contents before deleting the
        # reward bag; nested descendants retain their existing subtree.
        "${MYSQL[@]}" "$database" -e "
INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,template_json,display_name,granted_by)
VALUES ('migration_probe',36430,1,'{\"template_version\":1}','saved divine bag','Xanadin');
SET @bag_grant=LAST_INSERT_ID();
INSERT INTO player_items(pid,vnum,container_id,name) VALUES (1,50000,NULL,'ordinary outer pack');
SET @outer=LAST_INSERT_ID();
INSERT INTO player_items(pid,vnum,container_id,name) VALUES (1,36430,@outer,CONCAT('account_reward:',@bag_grant,':migration_probe saved divine bag'));
SET @bag=LAST_INSERT_ID();
INSERT INTO player_items(pid,vnum,container_id,name) VALUES (1,50001,@bag,'ordinary child pouch');
SET @child=LAST_INSERT_ID();
INSERT INTO player_items(pid,vnum,container_id,name) VALUES (1,50002,@child,'ordinary nested gem');
SET @grandchild=LAST_INSERT_ID();
UPDATE player_items child JOIN player_items reward ON child.container_id=reward.id
SET child.container_id=reward.container_id
WHERE LEFT(reward.name,CHAR_LENGTH(CONCAT('account_reward:',@bag_grant,':migration_probe ')))=CONCAT('account_reward:',@bag_grant,':migration_probe ');
DELETE FROM player_items WHERE LEFT(name,CHAR_LENGTH(CONCAT('account_reward:',@bag_grant,':migration_probe ')))=CONCAT('account_reward:',@bag_grant,':migration_probe ');
SELECT @outer,@bag,@child,@grandchild INTO @saved_outer,@saved_bag,@saved_child,@saved_grandchild;"
        [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM player_items WHERE name LIKE 'account_reward:%:migration_probe %';") == 0 ]]
        [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM player_items child JOIN player_items outer_item ON child.container_id=outer_item.id WHERE child.name='ordinary child pouch' AND outer_item.name='ordinary outer pack';") == 1 ]]
        [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM player_items nested JOIN player_items child ON nested.container_id=child.id WHERE nested.name='ordinary nested gem' AND child.name='ordinary child pouch';") == 1 ]]
    fi

    # Two exact snapshots sharing account and vnum are distinct grants.
    "${MYSQL[@]}" "$database" -e "
INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,template_json,display_name,granted_by,expires_at)
VALUES ('migration_probe',36420,1,'{\"template_version\":1}','first exact hammer','Xanadin',NOW()+INTERVAL 2 DAY),
       ('migration_probe',36420,1,'{\"template_version\":1}','second exact hammer','Xanadin',NOW()+INTERVAL 2 DAY);
INSERT INTO account_bound_rewards(account_name,reward_vnum,display_name,granted_by,remaining_pwipes)
VALUES ('migration_probe',36421,'two-wipe hat','Xanadin',2);"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36420;") == 2 ]]
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND remaining_pwipes=2;") == 1 ]]

    grant_id=$("${MYSQL[@]}" "$database" -e "SELECT MIN(id) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36420;")
    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_reward_summons(grant_id,pid,last_summoned_at) VALUES ($grant_id,1,NOW()); DELETE FROM account_bound_rewards WHERE id=$grant_id;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_reward_summons WHERE grant_id=$grant_id;") == 0 ]]

    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_rewards(account_name,reward_vnum,display_name,granted_by) VALUES ('cascade_probe',36419,'cascade','Xanadin'); DELETE FROM accounts WHERE account_name='cascade_probe';"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='cascade_probe';") == 0 ]]

    # The preserve=true pwipe transaction clears cooldown state, decrements only
    # finite wipe lifetimes, retains permanent/day grants, and expires at zero.
    surviving_grant=$("${MYSQL[@]}" "$database" -e "SELECT MIN(id) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36420;")
    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_reward_summons(grant_id,pid,last_summoned_at) VALUES ($surviving_grant,1,NOW()); DELETE FROM account_bound_reward_summons; UPDATE account_bound_rewards SET remaining_pwipes=remaining_pwipes-1 WHERE remaining_pwipes IS NOT NULL AND remaining_pwipes>0; DELETE FROM account_bound_rewards WHERE (remaining_pwipes IS NOT NULL AND remaining_pwipes<=0) OR (expires_at IS NOT NULL AND expires_at<=NOW());"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_reward_summons;") == 0 ]]
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND remaining_pwipes=1;") == 1 ]]
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36420;") == 1 ]]
    "${MYSQL[@]}" "$database" -e "UPDATE account_bound_rewards SET remaining_pwipes=remaining_pwipes-1 WHERE remaining_pwipes IS NOT NULL AND remaining_pwipes>0; DELETE FROM account_bound_rewards WHERE remaining_pwipes IS NOT NULL AND remaining_pwipes<=0;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT COUNT(*) FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36421;") == 0 ]]

    # Login selection is per character: first active grant without history, then
    # that character's newest successful summon. Another PID's history is ignored.
    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_rewards(account_name,reward_vnum,display_name,granted_by) VALUES ('migration_probe',36423,'login selection token','Xanadin');"
    first_active=$("${MYSQL[@]}" "$database" -e "SELECT MIN(id) FROM account_bound_rewards WHERE account_name='migration_probe' AND (expires_at IS NULL OR expires_at>NOW()) AND (remaining_pwipes IS NULL OR remaining_pwipes>0);")
    selected_without_history=$("${MYSQL[@]}" "$database" -e "SELECT abr.id FROM account_bound_rewards abr LEFT JOIN account_bound_reward_summons s ON s.grant_id=abr.id AND s.pid=1 WHERE abr.account_name='migration_probe' AND (abr.expires_at IS NULL OR abr.expires_at>NOW()) AND (abr.remaining_pwipes IS NULL OR abr.remaining_pwipes>0) ORDER BY s.last_summoned_at IS NULL, s.last_summoned_at DESC, abr.id LIMIT 1;")
    [[ "$selected_without_history" == "$first_active" ]]
    latest_active=$("${MYSQL[@]}" "$database" -e "SELECT MAX(id) FROM account_bound_rewards WHERE account_name='migration_probe' AND (expires_at IS NULL OR expires_at>NOW()) AND (remaining_pwipes IS NULL OR remaining_pwipes>0);")
    [[ "$latest_active" != "$first_active" ]]
    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_reward_summons(grant_id,pid,last_summoned_at) VALUES ($first_active,2,NOW()),($latest_active,1,NOW());"
    selected_with_history=$("${MYSQL[@]}" "$database" -e "SELECT abr.id FROM account_bound_rewards abr LEFT JOIN account_bound_reward_summons s ON s.grant_id=abr.id AND s.pid=1 WHERE abr.account_name='migration_probe' AND (abr.expires_at IS NULL OR abr.expires_at>NOW()) AND (abr.remaining_pwipes IS NULL OR abr.remaining_pwipes>0) ORDER BY s.last_summoned_at IS NULL, s.last_summoned_at DESC, abr.id LIMIT 1;")
    [[ "$selected_with_history" == "$latest_active" ]]

    # Death recovery readiness bypasses cooldown without erasing selection history;
    # the next successful summon updates history and clears the one-shot override.
    "${MYSQL[@]}" "$database" -e "UPDATE account_bound_reward_summons SET recovery_ready=1 WHERE grant_id=$latest_active AND pid=1;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT CASE WHEN recovery_ready<>0 THEN 0 ELSE GREATEST(0,3600-TIMESTAMPDIFF(SECOND,last_summoned_at,NOW())) END FROM account_bound_reward_summons WHERE grant_id=$latest_active AND pid=1;") == 0 ]]
    selected_after_recovery=$("${MYSQL[@]}" "$database" -e "SELECT abr.id FROM account_bound_rewards abr LEFT JOIN account_bound_reward_summons s ON s.grant_id=abr.id AND s.pid=1 WHERE abr.account_name='migration_probe' AND (abr.expires_at IS NULL OR abr.expires_at>NOW()) AND (abr.remaining_pwipes IS NULL OR abr.remaining_pwipes>0) ORDER BY s.last_summoned_at IS NULL, s.last_summoned_at DESC, abr.id LIMIT 1;")
    [[ "$selected_after_recovery" == "$latest_active" ]]
    "${MYSQL[@]}" "$database" -e "INSERT INTO account_bound_reward_summons(grant_id,pid,last_summoned_at,recovery_ready) VALUES ($latest_active,1,NOW(),0) ON DUPLICATE KEY UPDATE last_summoned_at=NOW(),recovery_ready=0;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT recovery_ready FROM account_bound_reward_summons WHERE grant_id=$latest_active AND pid=1;") == 0 ]]
    "${MYSQL[@]}" "$database" -e "DELETE FROM account_bound_reward_summons;"

    # The global timestamp guard processes one pwipe, makes an immediate retry a
    # successful no-op, and permits the next legitimate event after 28 days.
    "${MYSQL[@]}" "$database" -e "
UPDATE account_bound_reward_pwipe_state SET last_processed_at=NULL WHERE id=1;
INSERT INTO account_bound_rewards(account_name,reward_vnum,display_name,granted_by,remaining_pwipes)
VALUES ('migration_probe',36422,'guarded three-wipe ring','Xanadin',3);
START TRANSACTION;
INSERT IGNORE INTO account_bound_reward_pwipe_state(id,last_processed_at) VALUES (1,NULL);
SELECT (last_processed_at IS NULL OR last_processed_at <= NOW() - INTERVAL 28 DAY) INTO @process_pwipe FROM account_bound_reward_pwipe_state WHERE id=1 FOR UPDATE;
UPDATE account_bound_rewards SET remaining_pwipes=remaining_pwipes-1 WHERE @process_pwipe=1 AND remaining_pwipes IS NOT NULL AND remaining_pwipes>0;
UPDATE account_bound_reward_pwipe_state SET last_processed_at=NOW() WHERE id=1 AND @process_pwipe=1;
COMMIT;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT remaining_pwipes FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36422;") == 2 ]]
    first_stamp=$("${MYSQL[@]}" "$database" -e "SELECT UNIX_TIMESTAMP(last_processed_at) FROM account_bound_reward_pwipe_state WHERE id=1;")
    "${MYSQL[@]}" "$database" -e "
START TRANSACTION;
SELECT (last_processed_at IS NULL OR last_processed_at <= NOW() - INTERVAL 28 DAY) INTO @process_pwipe FROM account_bound_reward_pwipe_state WHERE id=1 FOR UPDATE;
UPDATE account_bound_rewards SET remaining_pwipes=remaining_pwipes-1 WHERE @process_pwipe=1 AND remaining_pwipes IS NOT NULL AND remaining_pwipes>0;
UPDATE account_bound_reward_pwipe_state SET last_processed_at=NOW() WHERE id=1 AND @process_pwipe=1;
COMMIT;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT remaining_pwipes FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36422;") == 2 ]]
    [[ $("${MYSQL[@]}" "$database" -e "SELECT UNIX_TIMESTAMP(last_processed_at) FROM account_bound_reward_pwipe_state WHERE id=1;") == "$first_stamp" ]]
    "${MYSQL[@]}" "$database" -e "
UPDATE account_bound_reward_pwipe_state SET last_processed_at=NOW()-INTERVAL 29 DAY WHERE id=1;
START TRANSACTION;
SELECT (last_processed_at IS NULL OR last_processed_at <= NOW() - INTERVAL 28 DAY) INTO @process_pwipe FROM account_bound_reward_pwipe_state WHERE id=1 FOR UPDATE;
UPDATE account_bound_rewards SET remaining_pwipes=remaining_pwipes-1 WHERE @process_pwipe=1 AND remaining_pwipes IS NOT NULL AND remaining_pwipes>0;
UPDATE account_bound_reward_pwipe_state SET last_processed_at=NOW() WHERE id=1 AND @process_pwipe=1;
COMMIT;"
    [[ $("${MYSQL[@]}" "$database" -e "SELECT remaining_pwipes FROM account_bound_rewards WHERE account_name='migration_probe' AND reward_vnum=36422;") == 1 ]]

    # preserve=false is an all-grants delete; prove it without changing fixture
    # state by rolling the transaction back after observing zero rows.
    [[ $("${MYSQL[@]}" "$database" -e "START TRANSACTION; DELETE FROM account_bound_rewards; SELECT COUNT(*) FROM account_bound_rewards; ROLLBACK;") == 0 ]]
done

# Reproduce a real interrupted/legacy state where the composite parent primary
# key is the only index supporting the account FK.  The migration must create
# its target account index before replacing that primary key, and then restore
# the child grant FK.
"${MYSQL[@]}" "$LEGACY_DB" -e "
ALTER TABLE account_bound_reward_summons DROP FOREIGN KEY account_bound_reward_summons_grant_fk;
ALTER TABLE account_bound_rewards DROP FOREIGN KEY account_bound_rewards_ibfk_1;
ALTER TABLE account_bound_rewards DROP PRIMARY KEY, ADD UNIQUE KEY id(id), ADD PRIMARY KEY(account_name,reward_vnum);
ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_account;
ALTER TABLE account_bound_rewards ADD CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY(account_name) REFERENCES accounts(account_name) ON DELETE CASCADE;"
apply_reward_migration
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$LEGACY_DB" "$NAME" /tmp/verify_account_bound_rewards.sh >/dev/null

before_replay=$(schema_signature "$LEGACY_DB")
rows_before=$("${MYSQL[@]}" "$LEGACY_DB" -e "SELECT COUNT(*) FROM account_bound_rewards;")
apply_reward_migration
after_replay=$(schema_signature "$LEGACY_DB")
rows_after=$("${MYSQL[@]}" "$LEGACY_DB" -e "SELECT COUNT(*) FROM account_bound_rewards;")
[[ "$before_replay" == "$after_replay" ]]
[[ "$rows_before" == "$rows_after" ]]
printf 'account reward stable-ID/template/expiry/summon/pwipe-guard migration convergence and replay: ok\n'
