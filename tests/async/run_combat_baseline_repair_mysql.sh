#!/usr/bin/env bash
# Rehearse guarded combat-baseline repair against an isolated production schema.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
container_name="duris-combat-baseline-$RANDOM-$$"
database_name="duris_combat_baseline_test"
password="duris-combat-baseline-$RANDOM-$$"
image="${COMBAT_BASELINE_DB_IMAGE:-mysql:8.0}"
temporary_root="$(mktemp -d)"
if [[ "$image" == mariadb:* ]]; then
	root_password_environment=MARIADB_ROOT_PASSWORD
else
	root_password_environment=MYSQL_ROOT_PASSWORD
fi

# Remove only the uniquely named disposable container and private fixture directory.
cleanup() {
	docker rm -f "$container_name" >/dev/null 2>&1 || true
	rm -rf "$temporary_root"
}
trap cleanup EXIT HUP INT TERM

chmod 700 "$temporary_root"
docker run --rm -d --name "$container_name" -p 127.0.0.1::3306 \
	-e "$root_password_environment=$password" "$image" >/dev/null
mapping="$(docker port "$container_name" 3306/tcp)"
db_port="${mapping##*:}"
ready=0
for _ in $(seq 1 90); do
	if MYSQL_PWD="$password" mysql -h127.0.0.1 -P"$db_port" \
		-uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
		ready=1
		break
	fi
	sleep 1
done
[[ "$ready" == 1 ]]

MYSQL=(mysql -h127.0.0.1 -P"$db_port" -uroot -N -B "$database_name")
MYSQL_PWD="$password" mysql -h127.0.0.1 -P"$db_port" -uroot \
	-e "CREATE DATABASE $database_name;"
MYSQL_PWD="$password" "${MYSQL[@]}" <"$ROOT/migrations/bootstrap_multithread_safe.sql"
MYSQL_PWD="$password" "${MYSQL[@]}" <<'SQL'
INSERT INTO player_data
  (pid,name,account_name,active,copper,silver,gold,platinum,wallet_revision,
   epics,epic_revision,frags,frag_revision)
VALUES
  (101,'SafeHero','Fixture',1,1,2,3,4,0,5,0,7,0),
  (200,'UnrelatedHero','Other',0,9,8,7,6,0,4,0,3,0);
INSERT INTO account_characters(account_name,pid,char_name,blocked,deleted_at)
VALUES ('Fixture',101,'SafeHero',0,NULL);
INSERT INTO currency_wallet_baseline
  (pid,opening_copper,opening_silver,opening_gold,opening_platinum,opening_revision)
VALUES (101,1,2,3,4,0),(200,9,8,7,6,0);
INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision)
VALUES (101,5,0),(200,4,0);
INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision)
VALUES (200,3,0);
SQL

rollback="$temporary_root/rollback.sql"
printf '%s\n' \
	'-- Reviewed fixture rollback: delete only the exact inserted row before later activity.' \
	'DELETE FROM combat_frag_baseline WHERE pid=101 AND opening_frags=7 AND opening_revision=0;' \
	>"$rollback"
chmod 600 "$rollback"
rollback_sha="$(sha256sum "$rollback" | cut -d' ' -f1)"
repair=("$ROOT/migrations/repair_missing_combat_baselines.sh")
common_env=(
	'ENVIRONMENT=test'
	'DB_HOST=127.0.0.1'
	"DB_PORT=$db_port"
	'DB_USER=root'
	"DB_PASSWD=$password"
	"DB_NAME=$database_name"
	'WRITERS_QUIESCED=TRUE'
	'COMBAT_BASELINE_BACKUP_ID=fixture-backup'
	"COMBAT_BASELINE_ROLLBACK_EVIDENCE=$rollback"
	"COMBAT_BASELINE_ROLLBACK_SHA256=$rollback_sha"
)

safe_artifact="$temporary_root/safe.tsv"
safe_report="$(env "${common_env[@]}" "${repair[@]}" --classify "$safe_artifact")"
grep -q 'missing_combat_baselines=1' <<<"$safe_report"
safe_sha="$(sha256sum "$safe_artifact" | cut -d' ' -f1)"
unrelated_before=$(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT CONCAT_WS(":",pid,opening_frags,opening_revision,HEX(captured_at)) FROM combat_frag_baseline WHERE pid=200;')
env "${common_env[@]}" "${repair[@]}" --apply "$safe_artifact" "$safe_sha" \
	>/dev/null
inserted=$(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT COUNT(*) FROM combat_frag_baseline WHERE pid=101 AND opening_frags=7 AND opening_revision=0;')
unrelated_after=$(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT CONCAT_WS(":",pid,opening_frags,opening_revision,HEX(captured_at)) FROM combat_frag_baseline WHERE pid=200;')
[[ "$inserted" == 1 && "$unrelated_after" == "$unrelated_before" ]]
env "${common_env[@]}" "${repair[@]}" --apply "$safe_artifact" "$safe_sha" \
	>/dev/null
[[ $(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT COUNT(*) FROM combat_frag_baseline WHERE pid=101;') == 1 ]]

MYSQL_PWD="$password" "${MYSQL[@]}" <<'SQL'
INSERT INTO player_data
  (pid,name,account_name,active,copper,silver,gold,platinum,wallet_revision,
   epics,epic_revision,frags,frag_revision)
VALUES (102,'ConflictHero','Fixture',1,0,0,0,0,0,0,0,8,0);
INSERT INTO account_characters(account_name,pid,char_name,blocked,deleted_at)
VALUES ('Fixture',102,'ConflictHero',0,NULL);
INSERT INTO currency_wallet_baseline
  (pid,opening_copper,opening_silver,opening_gold,opening_platinum,opening_revision)
VALUES (102,0,0,0,0,0);
INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision)
VALUES (102,0,0);
SQL
conflict_artifact="$temporary_root/conflict.tsv"
env "${common_env[@]}" "${repair[@]}" --classify "$conflict_artifact" >/dev/null
conflict_sha="$(sha256sum "$conflict_artifact" | cut -d' ' -f1)"
MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision) VALUES(102,999,0);'
if env "${common_env[@]}" "${repair[@]}" --apply "$conflict_artifact" \
	"$conflict_sha" >/dev/null 2>&1; then
	echo 'repair accepted a conflicting existing baseline' >&2
	exit 1
fi
[[ $(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT opening_frags FROM combat_frag_baseline WHERE pid=102;') == 999 ]]

MYSQL_PWD="$password" "${MYSQL[@]}" <<'SQL'
INSERT INTO player_data
  (pid,name,account_name,active,copper,silver,gold,platinum,wallet_revision,
   epics,epic_revision,frags,frag_revision)
VALUES (103,'HistoryHero','Fixture',1,0,0,0,0,0,0,0,9,1);
INSERT INTO account_characters(account_name,pid,char_name,blocked,deleted_at)
VALUES ('Fixture',103,'HistoryHero',0,NULL);
INSERT INTO currency_wallet_baseline
  (pid,opening_copper,opening_silver,opening_gold,opening_platinum,opening_revision)
VALUES (103,0,0,0,0,0);
INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision)
VALUES (103,0,0);
SQL
history_artifact="$temporary_root/history.tsv"
history_report="$(env "${common_env[@]}" "${repair[@]}" --classify "$history_artifact")"
grep -q 'revision_without_ledger=1' <<<"$history_report"
history_sha="$(sha256sum "$history_artifact" | cut -d' ' -f1)"
if env "${common_env[@]}" "${repair[@]}" --apply "$history_artifact" \
	"$history_sha" >/dev/null 2>&1; then
	echo 'repair inferred an opening value for revision-bearing history' >&2
	exit 1
fi
[[ $(MYSQL_PWD="$password" "${MYSQL[@]}" -e \
	'SELECT COUNT(*) FROM combat_frag_baseline WHERE pid=103;') == 0 ]]

echo 'combat baseline repair: safe apply, repeat, conflict, history, and preservation passed'
