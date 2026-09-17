#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-collector-notification-$$"
PASSWORD=$(printf 'collector-notification-%s-%s' "$$" "$RANDOM")
DB_NAME="collector_notification_test"
DB_IMAGE="${COLLECTOR_NOTIFICATION_DB_IMAGE:-mysql:8.0}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
if [[ "$DB_IMAGE" == mariadb:* ]]; then
    ROOT_PASSWORD_ENV="MARIADB_ROOT_PASSWORD"
else
    ROOT_PASSWORD_ENV="MYSQL_ROOT_PASSWORD"
fi

docker run -d --name "$NAME" -e "$ROOT_PASSWORD_ENV=$PASSWORD" "$DB_IMAGE" >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -e \
    "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" >/dev/null
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
mysql_exec() { "${MYSQL[@]}" -e "$1"; }
assert_eq() {
    local label=$1 expected=$2 actual=$3
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAILED: %s expected=%s actual=%s\n' "$label" "$expected" "$actual" >&2
        exit 1
    fi
}

# Start from the pre-0021 queue shape so the additive upgrade is exercised.
mysql_exec "
SET sql_mode='';
CREATE TABLE offline_messages (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  date DATETIME NOT NULL DEFAULT '0000-00-00 00:00:00',
  pid INT NOT NULL DEFAULT 0,
  message MEDIUMTEXT NOT NULL,
  PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
"
docker cp "$ROOT/migrations/immutable/0021_collector_notification_identity.sql" \
    "$NAME:/tmp/0021_collector_notification_identity.sql" >/dev/null
docker cp "$ROOT/migrations/immutable/0021_collector_notification_identity.sh" \
    "$NAME:/tmp/0021_collector_notification_identity.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/0021_collector_notification_identity.sh
for _ in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql --init-command=\"SET sql_mode=''\" -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0021_collector_notification_identity.sql"
done
docker exec -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
    -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" \
    /tmp/0021_collector_notification_identity.sh >/dev/null

PID=4242
MESSAGE="The Collector recovery is ready."
ID_A=00112233445566778899aabbccddeeff
ID_B=ffeeddccbbaa99887766554433221100
enqueue() {
    local id=$1
    mysql_exec "
START TRANSACTION;
INSERT INTO offline_message_receipts
    (pid,message_id,message,status)
VALUES ($PID,UNHEX('$id'),'$MESSAGE',0)
ON DUPLICATE KEY UPDATE message=message;
INSERT IGNORE INTO offline_messages (date,pid,message,message_id)
SELECT UTC_TIMESTAMP(6),pid,message,message_id
FROM offline_message_receipts
WHERE pid=$PID AND message_id=UNHEX('$id') AND status IN (0,1);
COMMIT;
"
}
claim() {
    local id=$1 result
    result=$(mysql_exec "
UPDATE offline_message_receipts
SET status=1,attempt_count=attempt_count+1,last_attempt_at=UTC_TIMESTAMP(6)
WHERE pid=$PID AND message_id=UNHEX('$id') AND status=0;
SELECT ROW_COUNT();")
    assert_eq "claim $id" "1" "$result"
}
ack() {
    local id=$1 result
    result=$(mysql_exec "
UPDATE offline_message_receipts
SET status=2,delivered_at=UTC_TIMESTAMP(6)
WHERE pid=$PID AND message_id=UNHEX('$id') AND status=1;
SELECT ROW_COUNT();")
    assert_eq "ack $id" "1" "$result"
}
delete_queue() {
    local id=$1
    mysql_exec "DELETE FROM offline_messages WHERE pid=$PID AND message_id=UNHEX('$id');"
}

# Two deaths with identical text occupy two durable identities.
enqueue "$ID_A"
enqueue "$ID_B"
enqueue "$ID_A"
assert_eq "distinct durable receipts" "2" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID;")"
assert_eq "distinct message ids" "2" "$(mysql_exec "SELECT COUNT(DISTINCT message_id) FROM offline_message_receipts WHERE pid=$PID;")"
assert_eq "two physical outbox rows" "2" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=$PID AND message_id IS NOT NULL;")"
assert_eq "durable delivery scan" "2" "$(mysql_exec "SELECT COUNT(*) FROM (SELECT 'R' AS delivery_kind, 0 AS queue_id, LOWER(HEX(r.message_id)) AS message_id, r.message, r.created_at AS created_at FROM offline_message_receipts r WHERE r.pid=$PID AND r.status=0 UNION ALL SELECT 'L', m.id, '', m.message, m.date FROM offline_messages m WHERE m.pid=$PID AND m.message_id IS NULL ORDER BY created_at ASC, delivery_kind ASC, queue_id ASC) AS pending;")"

# Claim/ack A; its receipt remains after the queue row is removed.
claim "$ID_A"
assert_eq "A claimed" "1" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID AND message_id=UNHEX('$ID_A') AND status=1 AND attempt_count=1;")"
ack "$ID_A"
delete_queue "$ID_A"
assert_eq "A receipt after ack" "1" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID AND message_id=UNHEX('$ID_A') AND status=2 AND delivered_at IS NOT NULL;")"
assert_eq "A queue removed" "0" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=$PID AND message_id=UNHEX('$ID_A');")"

# Restart while B is in-flight, then recover the durable receipt after its lease.
claim "$ID_B"
delete_queue "$ID_B"
docker restart "$NAME" >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
assert_eq "claimed receipt survives restart" "1" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID AND message_id=UNHEX('$ID_B') AND status=1 AND delivered_at IS NULL;")"
mysql_exec "UPDATE offline_message_receipts SET last_attempt_at=UTC_TIMESTAMP(6)-INTERVAL 31 SECOND WHERE pid=$PID AND message_id=UNHEX('$ID_B');"
recovered=$(mysql_exec "UPDATE offline_message_receipts SET status=0 WHERE pid=$PID AND message_id=UNHEX('$ID_B') AND status=1 AND last_attempt_at < UTC_TIMESTAMP(6)-INTERVAL 30 SECOND; SELECT ROW_COUNT();")
assert_eq "stale claim recovered" "1" "$recovered"
enqueue "$ID_B"
assert_eq "B queue rebuilt from receipt" "1" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=$PID AND message_id=UNHEX('$ID_B');")"
claim "$ID_B"
ack "$ID_B"
delete_queue "$ID_B"

# A delivered retry must not recreate the queue or a receipt.
enqueue "$ID_A"
assert_eq "same delivered identity remains one receipt" "2" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID;")"
assert_eq "same delivered identity stays dequeued" "0" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=$PID AND message_id=UNHEX('$ID_A');")"

# A second restart proves receipts are durable independently of queue deletion.
docker restart "$NAME" >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
assert_eq "delivered receipts survive restart" "2" "$(mysql_exec "SELECT COUNT(*) FROM offline_message_receipts WHERE pid=$PID AND status=2 AND delivered_at IS NOT NULL;")"
assert_eq "all delivered queues remain deleted" "0" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=$PID AND message_id IS NOT NULL;")"

# Legacy callers remain allowed to enqueue rows without a durable identity.
mysql_exec "INSERT INTO offline_messages(date,pid,message) VALUES (UTC_TIMESTAMP(),'9000','legacy offline message');"
assert_eq "legacy queue row preserved" "1" "$(mysql_exec "SELECT COUNT(*) FROM offline_messages WHERE pid=9000 AND message_id IS NULL;")"

printf 'collector notification real SQL claim/ack/retry/restart identity test (%s): ok\n' "$DB_IMAGE"
