#!/usr/bin/env bash
# Run the issue331 player journey only on fresh, task-owned Docker resources.
set -Eeuo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
IMAGE=${DURIS_TEST_TOOLS_IMAGE:-duris-issue-213-tools:latest}
DB_IMAGE=${DURIS_TEST_DB_IMAGE:-mariadb:10.11}
SUFFIX="issue331-player-${BASHPID}-${RANDOM}"
NETWORK="${SUFFIX}-net"
DB_CONTAINER="${SUFFIX}-db"
RUNTIME_CONTAINER="${SUFFIX}-game"
DB_NAME="duris_issue331_player_${BASHPID}_${RANDOM}"
DB_PASSWORD="issue331-disposable-only"


DB_ID=""
RUNTIME_ID=""
NETWORK_ID=""
cleanup() {
  status=$?
  set +e
  if [[ "$status" != 0 && "${ISSUE331_KEEP_ON_FAILURE:-0}" == 1 ]]; then
    printf 'Retained task-only failure resources: runtime=%s db=%s network=%s\n' "$RUNTIME_ID" "$DB_ID" "$NETWORK_ID"
    return
  fi
  if [[ -n "$RUNTIME_ID" ]]; then docker rm -f "$RUNTIME_ID" >/dev/null 2>&1; fi
  if [[ -n "$DB_ID" ]]; then docker rm -f "$DB_ID" >/dev/null 2>&1; fi
  if [[ -n "$NETWORK_ID" ]]; then docker network rm "$NETWORK_ID" >/dev/null 2>&1; fi
}
trap cleanup EXIT INT TERM

# Explicit --restart=no and no host mounts make the lifecycle disposable.
NETWORK_ID=$(docker network create "$NETWORK")
DB_ID=$(docker create --name "$DB_CONTAINER" --restart=no --network "$NETWORK" \
  -e "MARIADB_ROOT_PASSWORD=$DB_PASSWORD" -e "MARIADB_DATABASE=$DB_NAME" \
  "$DB_IMAGE" --event-scheduler=OFF)
docker start "$DB_CONTAINER" >/dev/null

RUNTIME_ID=$(docker create --name "$RUNTIME_CONTAINER" --restart=no --network "$NETWORK" \
  -p "127.0.0.1::4000" \
  -e ENVIRONMENT=local \
  -e PERSISTENCE_MODE=mariadb-primary \
  -e PERSISTENCE_BACKEND=mariadb-primary \
  -e DB_HOST=127.0.0.1 \
  -e DB_PORT=13306 \
  -e DB_USER=root \
  -e "DB_PASSWD=$DB_PASSWORD" \
  -e "MYSQL_PWD=$DB_PASSWORD" \
  -e "DB_NAME=$DB_NAME" \
  -e "DB_ALLOWED_TARGETS=127.0.0.1/$DB_NAME" \
  -e DB_TLS=FALSE \
  -e REDIS=FALSE \
  -e PLAYER_SAVE_JOURNAL_DIR=/work/issue331/journals/player \
  -e CRITICAL_COMMAND_JOURNAL_DIR=/work/issue331/journals/critical \
  -e LISTEN_ADDRESS=0.0.0.0 \
  "$IMAGE" sleep infinity)
docker start "$RUNTIME_CONTAINER" >/dev/null
GAME_HOST=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$RUNTIME_CONTAINER")

export ISSUE331_DB_CONTAINER="$DB_CONTAINER"
export ISSUE331_RUNTIME_CONTAINER="$RUNTIME_CONTAINER"
export ISSUE331_GAME_PORT=4000
export ISSUE331_GAME_HOST="$GAME_HOST"
export DB_NAME DB_PASSWORD
export ISSUE331_PLAN_DIR="${ISSUE331_PLAN_DIR:-/opt/data/workspaces/.hermes/plans/duris-issue-331-restitution}"
export ISSUE331_ARTIFACT="${ISSUE331_ARTIFACT:-$ISSUE331_PLAN_DIR/artifacts/dms_new}"

cd "$ROOT"
python3 -u tests/async/test_issue331_player_journey.py
