#!/usr/bin/env bash
# Disposable MariaDB/MySQL qualification of production SQL runtime authority.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
PLAN_ROOT="${PA_RUNTIME_SQL_PLAN_ROOT:-/opt/data/workspaces/duris-persistence-tools/parallel-db-rollout-23d086e3}"
BASE_BUILD_JSON=""
BINARY=""
EXPECTED_HEAD=""
EXPECTED_BINARY_SHA256=""
while (($#)); do
    case "$1" in
        --binary|--base-build|--expected-head|--expected-binary-sha256)
            [[ $# -ge 2 ]] || { echo "Missing value for $1" >&2; exit 2; }
            case "$1" in
                --binary) BINARY="$2" ;;
                --base-build) BASE_BUILD_JSON="$2" ;;
                --expected-head) EXPECTED_HEAD="$2" ;;
                --expected-binary-sha256) EXPECTED_BINARY_SHA256="$2" ;;
            esac
            shift 2
            ;;
        -h|--help)
            echo "usage: $0 --binary PATH --base-build JSON --expected-head SHA --expected-binary-sha256 SHA"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done
if [[ -z "$BINARY" || -z "$BASE_BUILD_JSON" || -z "$EXPECTED_HEAD" || -z "$EXPECTED_BINARY_SHA256" ]]; then
    echo "binary, base-build, expected-head, and expected-binary-sha256 are required" >&2
    exit 2
fi
[[ "$EXPECTED_HEAD" =~ ^[0-9a-f]{40}$ ]] || { echo "Invalid expected source HEAD" >&2; exit 2; }
[[ "$EXPECTED_BINARY_SHA256" =~ ^[0-9a-f]{64}$ ]] || { echo "Invalid expected binary SHA-256" >&2; exit 2; }
SLOT_LOCK="${PA_RUNTIME_SQL_SLOT_LOCK:-$PLAN_ROOT/resources/test-slot-0.lock}"
HEAVY_LOCK="${PA_RUNTIME_SQL_HEAVY_LOCK:-/opt/data/workspaces/duris-persistence-tools/docker-heavy.lock}"
[[ -f "$SLOT_LOCK" ]] || { echo "Missing assigned fixture lock: $SLOT_LOCK" >&2; exit 2; }
[[ -f "$HEAVY_LOCK" ]] || { echo "Missing shared Docker-heavy lock: $HEAVY_LOCK" >&2; exit 2; }
for required_command in flock timeout docker python3; do
    command -v "$required_command" >/dev/null || { echo "Required command unavailable: $required_command" >&2; exit 2; }
done

# Keep an exclusive slot-0 lease and shared Docker lease across the complete run.
if [[ "${PA_RUNTIME_SQL_LEASE_HELD:-}" != 1 ]]; then
    exec flock -w 900 -x "$SLOT_LOCK" flock -w 900 -s "$HEAVY_LOCK" env PA_RUNTIME_SQL_LEASE_HELD=1 "$0" --binary "$BINARY" --base-build "$BASE_BUILD_JSON" --expected-head "$EXPECTED_HEAD" --expected-binary-sha256 "$EXPECTED_BINARY_SHA256"
fi

unset DB_SOCKET MYSQL_PWD
TEMP="$(mktemp -d "${TMPDIR:-/tmp}/duris-pa-runtime-sql-XXXXXXXX")"
CONTAINER_NAME=""
CONTAINER_ID=""
PASSWORD=""
inspect_fixture_container() {
    timeout --signal=TERM --kill-after=5s 10s docker container inspect --format '{{index .Config.Labels "duris.task"}}|{{index .Config.Labels "duris.run_id"}}' "$1"
}
fixture_container_absent() {
    local output rc
    if output="$(inspect_fixture_container "$1" 2>&1)"; then
        if [[ "$output" == "pa-runtime-sql|$1" ]]; then
            return 1
        fi
        echo "Refusing cleanup of unowned fixture container $1 (labels=$output)" >&2
        return 2
    else
        rc=$?
    fi
    if [[ "$rc" == 1 ]] && grep -qi 'no such container' <<<"$output"; then
        return 0
    fi
    echo "Could not verify fixture container $1 state (exit=$rc): $output" >&2
    return 2
}
cleanup_fixture_container() {
    local name="$1"
    if fixture_container_absent "$name"; then
        return 0
    else
        [[ $? == 1 ]] || return 1
    fi
    timeout --signal=TERM --kill-after=5s 30s docker rm -f "$name" >/dev/null 2>&1 || true
    fixture_container_absent "$name"
}
docker_probe() {
    timeout --signal=TERM --kill-after=3s 5s docker "$@"
}
compile_bounded() {
    timeout --signal=TERM --kill-after=10s 900s "$@"
}
docker_bounded() {
    timeout --signal=TERM --kill-after=5s 120s docker "$@"
}
cleanup() {
    local status=$?
    trap - EXIT HUP INT TERM
    if [[ -n "$CONTAINER_NAME" ]]; then
        if cleanup_fixture_container "$CONTAINER_NAME"; then
            echo "PA_RUNTIME_SQL_FIXTURE_REMOVED name=$CONTAINER_NAME"
        else
            echo "Fixture container cleanup could not be verified: $CONTAINER_NAME" >&2
            status=1
        fi
    fi
    if [[ -n "$TEMP" ]]; then
        if ! timeout --signal=TERM --kill-after=5s 30s rm -rf -- "$TEMP"; then
            echo "Temporary fixture directory cleanup timed out: $TEMP" >&2
            status=1
        fi
    fi
    unset MYSQL_PWD DB_PASSWD PASSWORD
    exit "$status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

python3 tests/async/test_pa_runtime_sql.py
python3 - "$BASE_BUILD_JSON" "$BINARY" "$EXPECTED_HEAD" "$EXPECTED_BINARY_SHA256" "$ROOT" <<'PY'
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

build_path, binary_path, expected_head, expected_binary_sha256, root_text = sys.argv[1:]
root = Path(root_text).resolve(strict=True)
build_input = Path(build_path)
binary_input = Path(binary_path)
if not build_input.is_absolute() or build_input.is_symlink() or not build_input.is_file():
    raise SystemExit("base-build descriptor must be an absolute regular non-symlink file")
if (not binary_input.is_absolute() or binary_input.is_symlink() or
        not binary_input.is_file() or not os.access(binary_input, os.X_OK) or
        binary_input.stat().st_mode & 0o022):
    raise SystemExit("parent binary must be an absolute executable, read-only regular file")
build_path = build_input.resolve(strict=True)
binary = binary_input.resolve(strict=True)
if not re.fullmatch(r"[0-9a-f]{40}", expected_head):
    raise SystemExit("expected source HEAD must be a full lowercase Git SHA-1")
if not re.fullmatch(r"[0-9a-f]{64}", expected_binary_sha256):
    raise SystemExit("expected binary digest must be a full lowercase SHA-256")
data = json.loads(build_path.read_text(encoding="utf-8"))
status = str(data.get("status", ""))
if not status.endswith("_PASS"):
    raise SystemExit(f"frozen base build is not PASS: {status!r}")
checks = data.get("checks")
if not isinstance(checks, list) or not checks or any(
    not isinstance(item, dict) or type(item.get("exit")) is not int or item["exit"] != 0
    for item in checks
):
    raise SystemExit("frozen base build contains missing or failed checks")
if data.get("head") != expected_head:
    raise SystemExit("frozen base build does not match --expected-head")
current_head = subprocess.check_output(
    ["git", "-C", str(root), "rev-parse", "HEAD"], text=True, timeout=15
).strip()
if current_head != expected_head:
    raise SystemExit("checked-out source HEAD differs from the frozen build")
if subprocess.check_output(
    ["git", "-C", str(root), "status", "--porcelain"], text=True, timeout=15
).strip():
    raise SystemExit("checked-out worktree is dirty; refusing stale source provenance")
if data.get("backend") != "mariadb" or data.get("profile") != "development/TEST_MUD":
    raise SystemExit("frozen binary is not the expected MariaDB development test build")
recorded_binary = Path(data["binary"])
if (not recorded_binary.is_absolute() or recorded_binary.is_symlink() or
        not recorded_binary.is_file() or not os.access(recorded_binary, os.X_OK) or
        recorded_binary.stat().st_mode & 0o022):
    raise SystemExit("recorded parent binary must be an absolute executable, read-only regular file")
recorded_binary = recorded_binary.resolve(strict=True)
if recorded_binary != binary:
    raise SystemExit("--binary path differs from base-build.json")
binary_sha = hashlib.sha256(binary.read_bytes()).hexdigest()
if binary_sha != expected_binary_sha256 or binary_sha != data.get("binary_sha256"):
    raise SystemExit("frozen binary SHA-256 does not match the explicit batch identity")
manifest_input = Path(data["source_manifest"])
if not manifest_input.is_absolute() or manifest_input.is_symlink() or not manifest_input.is_file():
    raise SystemExit("source manifest must be an absolute regular non-symlink file")
manifest_path = manifest_input.resolve(strict=True)
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if not isinstance(manifest, dict) or not manifest:
    raise SystemExit("source manifest is empty or malformed")
tracked = set(subprocess.check_output(
    ["git", "-C", str(root), "ls-files", "src", "migrations", "Makefile"],
    text=True, timeout=15,
).splitlines())
if set(manifest) != tracked:
    raise SystemExit("source manifest does not cover exactly the tracked build inputs")
for name, expected in manifest.items():
    relative = Path(name)
    if relative.is_absolute() or ".." in relative.parts:
        raise SystemExit(f"unsafe source manifest path: {name!r}")
    source = (root / relative).resolve(strict=True)
    if not source.is_relative_to(root):
        raise SystemExit(f"source manifest path escapes checkout: {name!r}")
    if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise SystemExit(f"invalid source digest for {name!r}")
    if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise SystemExit(f"source manifest mismatch: {name}")
manifest_sha = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
print(f"PA_RUNTIME_SQL_BASE_BUILD status={status} head={current_head}")
print(f"PA_RUNTIME_SQL_BASE_BINARY sha256={binary_sha} bytes={binary.stat().st_size}")
print(f"PA_RUNTIME_SQL_BASE_SOURCE_MANIFEST sha256={manifest_sha}")
PY

run_image() {
    local image="$1"
    local engine="$2"
    local password_env client schema mapping ready version db_host
    PASSWORD="$(python3 -c 'import secrets; print(secrets.token_hex(32))')"
    CONTAINER_NAME="duris-pa-runtime-sql-${$}-${RANDOM}-${engine}"
    if [[ "$engine" == mariadb ]]; then
        password_env=MARIADB_ROOT_PASSWORD
        client=mariadb
    else
        password_env=MYSQL_ROOT_PASSWORD
        client=mysql
    fi
    source "$ROOT/tests/async/_sql_fixture_network.sh"
    sql_fixture_network
    export "$password_env=$PASSWORD"
    CONTAINER_ID="$(docker_bounded run --pull=never --cpus=2 --memory=2g -d --rm \
        --name "$CONTAINER_NAME" --label duris.task=pa-runtime-sql --label "duris.run_id=$CONTAINER_NAME" "${SQL_FIXTURE_NETWORK[@]}" -e "$password_env" "$image" "${SQL_FIXTURE_SERVER[@]}")"
    unset "$password_env"
    export MYSQL_PWD="$PASSWORD"
    if [[ -n "$SQL_FIXTURE_PRIVATE_PORT" ]]; then
        mapping="$(sql_fixture_mapping "$CONTAINER_ID")"
    else
        mapping="$(docker_bounded port "$CONTAINER_ID" 3306/tcp)"
    fi
    [[ "$mapping" =~ ^127\.0\.0\.1:([0-9]+)$ ]]
    DB_PORT="${BASH_REMATCH[1]}"
    ready=0
    deadline=$((SECONDS + 120))
    while ((SECONDS < deadline)); do
        if docker_probe exec -e MYSQL_PWD "$CONTAINER_ID" "$client" --protocol=tcp \
            -h 127.0.0.1 -P"${SQL_FIXTURE_PRIVATE_PORT:-3306}" -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
            ready=1
            break
        fi
        sleep 1
    done
    [[ "$ready" == 1 ]]
    version="$(docker_bounded exec -e MYSQL_PWD "$CONTAINER_ID" "$client" --protocol=tcp \
        -h 127.0.0.1 -P"${SQL_FIXTURE_PRIVATE_PORT:-3306}" -uroot -N -B -e 'SELECT VERSION()')"
    if [[ "$engine" == mariadb ]]; then
        [[ "$version" == *MariaDB* && "$version" == 10.11.* ]]
    else
        [[ "$version" == 8.0.* && "$version" != *MariaDB* ]]
    fi
    printf 'PA_RUNTIME_SQL_FIXTURE engine=%s image=%s version=%s name=%s port=%s\n' \
        "$engine" "$image" "$version" "$CONTAINER_NAME" "$DB_PORT"

    schema="pa_runtime_sql_test_$(python3 -c 'import secrets; print(secrets.token_hex(6))')"
    [[ "$schema" =~ ^pa_runtime_sql_test_[a-f0-9]{12}$ && ${#schema} -le 33 ]]
    docker_bounded exec -e MYSQL_PWD "$CONTAINER_ID" "$client" --protocol=tcp \
        -h 127.0.0.1 -P"${SQL_FIXTURE_PRIVATE_PORT:-3306}" -uroot -e "CREATE DATABASE \`$schema\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
    verify_schema() {
        DB_HOST=127.0.0.1 DB_PORT="${SQL_FIXTURE_PRIVATE_PORT:-3306}" DB_NAME="$schema" DB_USER=root DB_PASSWD="$PASSWORD" \
            docker_bounded exec -i -e DB_HOST -e DB_PORT -e DB_NAME -e DB_USER -e DB_PASSWD \
            "$CONTAINER_ID" bash -s < migrations/immutable/0033_economic_sql_lifecycle_owner.sh
    }
    docker_bounded exec -i -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        < migrations/bootstrap_multithread_safe.sql
    verify_schema
    docker_bounded exec -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        -e 'DROP TABLE economic_sql_global_activation'
    docker_bounded exec -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        -e 'DROP TABLE economic_sql_lifecycle_installation'
    docker_bounded exec -i -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        < migrations/immutable/0033_economic_sql_lifecycle_owner.sql
    verify_schema
    docker_bounded exec -i -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        < migrations/economic_sql_lifecycle_owner.sql
    verify_schema
    docker_bounded exec -i -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        < migrations/immutable/0040_economic_sql_global_activation.sql

    db_host=127.0.0.1
    if [[ -z "$SQL_FIXTURE_PRIVATE_PORT" ]] && python3 -c 'import socket; socket.gethostbyname("host.docker.internal")' >/dev/null 2>&1; then
        db_host=host.docker.internal
    fi
    export DB_HOST="$db_host" DB_PORT DB_NAME="$schema" DB_USER=root DB_PASSWD="$PASSWORD"
    export PA_RUNTIME_SQL_DISPOSABLE_SCHEMA=1
    unset DB_SOCKET
    local -a mysql_cflags mysql_libs
    read -r -a mysql_cflags <<< "$(mysql_config --cflags)"
    read -r -a mysql_libs <<< "$(mysql_config --libs)"
    local cxx="${CXX:-g++}"
    local -a sanitizer_flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie)
    compile_bounded "$cxx" -std=c++20 -w -ffunction-sections -fdata-sections "${sanitizer_flags[@]}" \
        -Isrc "${mysql_cflags[@]}" -c src/sql/sql.c -o "$TEMP/sql.o"
    compile_bounded "$cxx" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
        -ffunction-sections -fdata-sections "${sanitizer_flags[@]}" -Isrc "${mysql_cflags[@]}" \
        -Dsql_open_configured_connection=pa_runtime_sql_test_open_connection \
        -c src/sql/sql_economic_runtime.c -o "$TEMP/runtime-owner.o"
    compile_bounded "$cxx" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
        "${sanitizer_flags[@]}" -Isrc "${mysql_cflags[@]}" \
        -c src/persistence/economic_sql_lifecycle_guard.c -o "$TEMP/lifecycle-guard.o"
    compile_bounded "$cxx" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
        "${sanitizer_flags[@]}" -Isrc "${mysql_cflags[@]}" \
        -c src/persistence/persistence_observability.c -o "$TEMP/observability.o"
    compile_bounded "$cxx" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
        "${sanitizer_flags[@]}" -Isrc "${mysql_cflags[@]}" \
        -c tests/async/pa_runtime_sql_harness.cpp -o "$TEMP/harness.o"
    # Startup now recovers the actual accounting lifecycle and gameplay
    # authority. Keep those owners linked instead of replacing them with stubs.
    local -a runtime_sources=(
        src/persistence/economic_sql_accounting_lifecycle_transaction.c
        src/persistence/economic_sql_source_snapshot.c
        src/economy/economic_sql_source_normalize.c
        src/economy/economic_gameplay_authority.c
        src/persistence/economic_sql_baseline_transaction.c
        src/economy/economic_baseline_command.c
        src/economy/economic_baseline_adapter.c
        src/economy/economic_baseline_codec.c
        src/economy/economic_accounting_intent.c
        src/economy/economic_accounting_plan.c src/economy/economic_source_event.c
        src/economy/economic_accounting_types.c
        src/economy/currency_command.c
        src/persistence/critical_command.c
        src/item/item_transfer_command.c
        src/player/player_snapshot_codec.c
    )
    local -a runtime_objects=()
    local source object
    for source in "${runtime_sources[@]}"; do
        object="$TEMP/$(basename "$source").o"
        compile_bounded "$cxx" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
            -ffunction-sections -fdata-sections "${sanitizer_flags[@]}" \
            -Isrc "${mysql_cflags[@]}" -c "$source" -o "$object"
        runtime_objects+=("$object")
    done
    compile_bounded "$cxx" "${sanitizer_flags[@]}" -pthread -Wl,--gc-sections -no-pie \
        "$TEMP/harness.o" "$TEMP/runtime-owner.o" "$TEMP/lifecycle-guard.o" \
        "$TEMP/observability.o" "$TEMP/sql.o" "${runtime_objects[@]}" "${mysql_libs[@]}" \
        -lcrypto -lz -o "$TEMP/pa-runtime-sql"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        timeout --signal=TERM --kill-after=10s 240s "$TEMP/pa-runtime-sql"

    docker_bounded exec -e MYSQL_PWD "$CONTAINER_ID" "$client" -uroot "$schema" \
        -e 'DROP TABLE IF EXISTS pa_runtime_sql_rows'
    cleanup_fixture_container "$CONTAINER_NAME"
    if ! fixture_container_absent "$CONTAINER_NAME"; then
        printf 'Fixture container still exists after scenario: %s\n' "$CONTAINER_NAME" >&2
        return 1
    fi
    printf 'PA_RUNTIME_SQL_FIXTURE_REMOVED name=%s\n' "$CONTAINER_NAME"
    CONTAINER_NAME=""
    CONTAINER_ID=""
    unset MYSQL_PWD DB_PASSWD PA_RUNTIME_SQL_DISPOSABLE_SCHEMA DB_HOST DB_PORT DB_NAME DB_USER
    PASSWORD=""
}

run_image "${PA_RUNTIME_SQL_MARIADB_IMAGE:-mariadb:10.11}" mariadb
run_image "${PA_RUNTIME_SQL_MYSQL_IMAGE:-mysql:8.0}" mysql
printf 'PASS real SQL runtime authority qualification: MariaDB 10.11 + MySQL 8.0\n'
