#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DB_IMAGE=${DURIS_TEST_DB_IMAGE:-mariadb:11.4}
DB_CONTAINER="duris-issue-577-db-$$"
DB_NAME=duris_issue_331_test
DB_PASSWORD=issue577-test-only
TMP=$(mktemp -d)
MYSQL_WRAPPER="$TMP/mysql"
PLAN="$TMP/plan.json"
APPLY_SQL="$TMP/apply.sql"
RESULT="$TMP/apply.result"

cleanup() {
    docker rm -fv "$DB_CONTAINER" >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap cleanup EXIT HUP INT TERM

if [[ "$DB_IMAGE" == mysql:* ]]; then
    ROOT_PASSWORD_ENV=MYSQL_ROOT_PASSWORD
    DATABASE_ENV=MYSQL_DATABASE
    MYSQL_CLIENT=mysql
else
    ROOT_PASSWORD_ENV=MARIADB_ROOT_PASSWORD
    DATABASE_ENV=MARIADB_DATABASE
    MYSQL_CLIENT=mariadb
fi
docker run -d --name "$DB_CONTAINER" \
    -e "$ROOT_PASSWORD_ENV=$DB_PASSWORD" -e "$DATABASE_ENV=$DB_NAME" \
    "$DB_IMAGE" --event-scheduler=OFF >/dev/null
printf '%s\n' '#!/usr/bin/env bash' 'set -euo pipefail' \
    'exec docker exec -e MYSQL_PWD="${MYSQL_PWD:-}" -i "'"$DB_CONTAINER"'" '"$MYSQL_CLIENT"' --protocol=TCP --host=127.0.0.1 --port=3306 "$@"' \
    > "$MYSQL_WRAPPER"
chmod 700 "$MYSQL_WRAPPER"
export MYSQL_PWD="$DB_PASSWORD"
for attempt in $(seq 1 45); do
    if "$MYSQL_WRAPPER" -uroot -Nse 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    if [[ "$attempt" == 45 ]]; then
        printf 'database container did not become ready\n' >&2
        exit 1
    fi
    sleep 1
done

"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/tests/async/player_death_restitution_test_schema.sql"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/tests/async/player_death_restitution_locker_test_schema.sql"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/migrations/immutable/0020_player_death_restitution.sql"
python3 "$ROOT/tests/async/player_death_restitution_locker_fixture.py" "$PLAN" "$APPLY_SQL"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$APPLY_SQL" > "$RESULT"
grep -F 'DURIS_RESULT|1|0|1|1' "$RESULT" >/dev/null

export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root \
    DB_PASSWD="$DB_PASSWORD" DB_NAME="$DB_NAME" MYSQL_BIN="$MYSQL_WRAPPER"
python3 "$ROOT/scripts/player_death_restitution.py" verify --plan "$PLAN"

# The receipt is the idempotency fence: replay must not mint another bag,
# duplicate a delivered UID, or advance either revision/allocator again.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$APPLY_SQL" > "$RESULT"
grep -F 'DURIS_RESULT|1|1|1|1' "$RESULT" >/dev/null

read -r locker_count bag_count item_count marker_count source_revision locker_revision allocator_next \
    < <("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e \
        "SELECT (SELECT COUNT(*) FROM lockers WHERE locker_name='account.acct42.1.locker'),
        (SELECT COUNT(*) FROM locker_items WHERE obj_uid=9000 AND vnum=7670 AND
            short_descr='a restitution lost items bag for Deletedchar' AND (extra_flags & 524288)<>0),
        (SELECT COUNT(*) FROM locker_items child JOIN locker_items bag
            ON child.container_id=bag.id WHERE child.obj_uid=100 AND bag.obj_uid=9000),
        (SELECT COUNT(*) FROM locker_item_extra_descr ed JOIN locker_items bag
            ON bag.id=ed.item_id WHERE bag.obj_uid=9000 AND ed.keyword='restitution_lost_items'),
        (SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id=42 AND owner_context_id=0),
        (SELECT r.revision FROM item_owner_revision r JOIN lockers l ON l.id=r.owner_id
            JOIN private_chests c ON c.id=r.owner_context_id WHERE r.owner_type=5
            AND l.locker_name='account.acct42.1.locker' AND c.is_public=1),
        (SELECT next_uid FROM item_uid_allocator WHERE allocator_id=1)"
    )
[[ "$locker_count $bag_count $item_count $marker_count" == "1 1 1 1" ]]
[[ "$source_revision $locker_revision $allocator_next" == "7 1 9001" ]]
[[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e \
    'SELECT COUNT(*) FROM player_death_restitution_delivery') == 1 ]]
[[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e \
    'SELECT COUNT(*) FROM locker_items') == 2 ]]

printf 'deleted-character account-locker restitution MySQL test passed\n'
