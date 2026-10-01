#!/usr/bin/env bash
# Run the spellbook loader regression only against a disposable database.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"
DB_IMAGE=${PLAYER_LOAD_SPELLBOOK_DB_IMAGE:-mariadb:10.11}
TOOLS_IMAGE=${PLAYER_LOAD_SPELLBOOK_TOOLS_IMAGE:-duris-issue-213-tools:latest}
DB_CONTAINER="duris-player-load-spellbook-db-$$-$RANDOM"
TOOLS_CONTAINER="duris-player-load-spellbook-tools-$$-$RANDOM"
NETWORK="duris-player-load-spellbook-net-$$-$RANDOM"
DB_NAME="duris_player_load_spellbook_test"
PASSWORD="spellbook-loader-$$-$RANDOM"
DB_CREATED=0
TOOLS_CREATED=0
NETWORK_CREATED=0
cleanup() {
	if [[ "$TOOLS_CREATED" == 1 ]]; then docker rm -f "$TOOLS_CONTAINER" >/dev/null 2>&1 || true; fi
	if [[ "$DB_CREATED" == 1 ]]; then docker rm -f "$DB_CONTAINER" >/dev/null 2>&1 || true; fi
	if [[ "$NETWORK_CREATED" == 1 ]]; then docker network rm "$NETWORK" >/dev/null 2>&1 || true; fi
}
trap cleanup EXIT HUP INT TERM

if [[ "$DB_IMAGE" == mariadb:* ]]; then
	PASSWORD_ENV=MARIADB_ROOT_PASSWORD
	DATABASE_ENV=MARIADB_DATABASE
	CLIENT_NAME=mariadb
else
	PASSWORD_ENV=MYSQL_ROOT_PASSWORD
	DATABASE_ENV=MYSQL_DATABASE
	CLIENT_NAME=mysql
fi

docker network create "$NETWORK" >/dev/null
NETWORK_CREATED=1
docker create --name "$DB_CONTAINER" --network "$NETWORK" \
	-e "$PASSWORD_ENV=$PASSWORD" -e "$DATABASE_ENV=$DB_NAME" \
	"$DB_IMAGE" --event-scheduler=OFF >/dev/null
DB_CREATED=1
docker start "$DB_CONTAINER" >/dev/null
DB_CLIENT=(docker exec -i "$DB_CONTAINER" "$CLIENT_NAME" --protocol=tcp -h127.0.0.1 \
	-P3306 -uroot -p"$PASSWORD" --batch --skip-column-names)
DB_ADMIN=(docker exec "$DB_CONTAINER" "$CLIENT_NAME" --protocol=tcp -h127.0.0.1 \
	-P3306 -uroot -p"$PASSWORD")
ready=0
for _ in $(seq 1 90); do
	if "${DB_ADMIN[@]}" -e 'SELECT 1' >/dev/null 2>&1; then
		ready=1
		break
	fi
	sleep 1
done
[[ "$ready" == 1 ]] || {
	echo "database container did not become ready" >&2
	exit 1
}
"${DB_CLIENT[@]}" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql

docker create --name "$TOOLS_CONTAINER" --network "container:$DB_CONTAINER" \
	-w /workspace "$TOOLS_IMAGE" sleep infinity >/dev/null
TOOLS_CREATED=1
docker start "$TOOLS_CONTAINER" >/dev/null
docker exec "$TOOLS_CONTAINER" mkdir -p /workspace/src /workspace/tests/async
docker cp "$ROOT/src/." "$TOOLS_CONTAINER:/workspace/src/"
docker cp "$ROOT/migrations" "$TOOLS_CONTAINER:/workspace/"
docker cp "$ROOT/scripts" "$TOOLS_CONTAINER:/workspace/"
docker cp "$ROOT/tests/async/player_load_repository_spellbook_mysql_harness.cpp" \
	"$TOOLS_CONTAINER:/workspace/tests/async/"
# The current loader uses quest witnesses, spell receipts and rich item state.
# Build its disposable schema through the same immutable history as the server.
docker exec "$TOOLS_CONTAINER" env ENVIRONMENT=local DB_HOST=127.0.0.1 DB_PORT=3306 \
	DB_USER=root DB_PASSWD="$PASSWORD" DB_NAME="$DB_NAME" DB_TLS=FALSE \
	DB_ALLOWED_TARGETS="127.0.0.1/$DB_NAME" bash -lc '
	set -euo pipefail
	command -v python3 >/dev/null
	python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
	python3 scripts/migration_runner.py run
	python3 scripts/migration_runner.py run
'
docker exec "$TOOLS_CONTAINER" bash -lc '
	set -euo pipefail
	read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
	read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
	g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
		"${MYSQL_CFLAGS[@]}" -ffunction-sections -fdata-sections \
		tests/async/player_load_repository_spellbook_mysql_harness.cpp \
		src/player/player_load_repository.c src/player/player_load_topology.c src/player/player_death_recovery_query.c src/player/player_death_conflict_repository.c src/persistence/critical_command.c \
		src/player/player_load_items.c src/player/player_snapshot_codec.c \
		src/persistence/persistence_observability.c \
		src/persistence/player_death_restitution_command.c \
		src/persistence/quest_reward_obligation_repository.c \
		src/player/player_save_journal.c \
		src/item/item_transfer_command.c src/item/craft_pouch_mutation.c src/combat/chaos_pouch_ledger.c src/economy/currency_command.c \
		-Wl,--gc-sections "${MYSQL_LIBS[@]}" -lcrypto \
		-o /tmp/player_load_repository_spellbook_mysql_harness
'
docker exec "$TOOLS_CONTAINER" env DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root \
	DB_PASSWD="$PASSWORD" DB_NAME="$DB_NAME" \
	/tmp/player_load_repository_spellbook_mysql_harness
