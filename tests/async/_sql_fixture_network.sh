# Shared network setup for owned SQL wrappers with a native client.
# A container runner shares only its own validated namespace; a native runner
# publishes exclusively on loopback. Each service still has its own port/name.
sql_fixture_network() {
    local configuration network port
    configuration=$(cd "$ROOT" && python3 - <<'PY'
import socket, sys
sys.path.insert(0, "tests/async")
from disposable_sql_fixture import private_network
network = private_network()
with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    print(network or "native", probe.getsockname()[1])
PY
    )
    read -r network port <<< "$configuration"
    SQL_FIXTURE_NETWORK=(-p 127.0.0.1::3306)
    SQL_FIXTURE_SERVER=(--innodb-use-native-aio=OFF)
    SQL_FIXTURE_PRIVATE_PORT=
    if [[ "$network" != native ]]; then
        SQL_FIXTURE_NETWORK=(--network "$network")
        SQL_FIXTURE_SERVER+=("--port=$port" --bind-address=127.0.0.1)
        SQL_FIXTURE_PRIVATE_PORT=$port
    fi
}

sql_fixture_mapping() {
    if [[ -n "$SQL_FIXTURE_PRIVATE_PORT" ]]; then
        printf '127.0.0.1:%s\n' "$SQL_FIXTURE_PRIVATE_PORT"
    else
        docker port "$1" 3306/tcp
    fi
}
