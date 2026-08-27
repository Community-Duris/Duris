#!/usr/bin/env python3
"""Runtime database and listener trust-boundary source contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
sql_h = (root / "src/sql.h").read_text()
sql = (root / "src/sql.c").read_text()
runtime_contract = (root / "src/runtime_compatibility_contract.h").read_text()
pool = (root / "src/sql_pool.c").read_text()
player = (root / "src/sql_player.c").read_text()
comm = (root / "src/comm.c").read_text()
comm_h = (root / "src/comm.h").read_text()
websocket = (root / "src/websocket.c").read_text()
ssl = (root / "src/ssl.c").read_text()
example = (root / ".env.example").read_text()
config = (root / "docs/CONFIGURATION.md").read_text()
readme = (root / "README.md").read_text()
cycle = (root / "scripts/cycle_mud.sh").read_text()


def section(text: str, start_marker: str, end_marker: str) -> str:
    start = text.rfind(start_marker)
    assert start >= 0, start_marker
    end = text.find(end_marker, start)
    assert end >= 0, end_marker
    return text[start:end]


assert "DB_HOST_DEFAULT" not in sql_h
assert "DB_USER_DEFAULT" not in sql_h
assert "DB_PASSWD_DEFAULT" not in sql_h
assert "DB_NAME_DEFAULT" not in sql_h
for getter in ("get_db_host", "get_db_user", "get_db_passwd", "get_db_name"):
    body = section(sql_h, f"static inline const char *{getter}", "}")
    assert 'return val ? val : "";' in body
print("[PASS] compiled database credential and target defaults are gone")

env_loader = section(sql, "int load_env_file(void)\n{", "int initialize_mysql()")
assert 'open(".env", O_RDONLY | O_CLOEXEC | O_NOFOLLOW)' in env_loader
assert "fstat(fd, &file_stat)" in env_loader
assert "S_ISREG(file_stat.st_mode)" in env_loader
assert "file_stat.st_uid != geteuid()" in env_loader
assert "file_stat.st_mode & 0177" in env_loader
assert "return -1;" in env_loader
assert 'fatal_boot_error("comm", "Unsafe environment configuration file")' in comm
assert "chmod 600 .env" in readme and "chmod 600 .env" in config
print("[PASS] unsafe .env metadata fails before file contents are loaded")

runtime = section(sql, "static bool sql_runtime_config_valid(void)", "static bool sql_connection_execute")
for required in (
    '"DB_HOST"',
    '"DB_USER"',
    '"DB_PASSWD"',
    '"DB_NAME"',
    '"DB_ALLOWED_TARGETS"',
):
    assert required in runtime
assert 'strcmp(role, "local") && strcmp(role, "production")' in runtime
assert "strtol(port, &end, 10)" in runtime
assert "parsed < 1 || parsed > 65535" in runtime
assert "sql_target_is_allowed(DB_HOST, database)" in runtime
assert "production role requires the production port" in runtime

constructor = section(
    sql,
    "MYSQL *sql_open_configured_connection(unsigned long client_flags)",
    "/* Escapes a string.",
)
assert constructor.index("sql_runtime_config_valid") < constructor.index("mysql_real_connect")
print("[PASS] role, credentials, strict port, and exact resolved target are preflighted")
for option in (
    "MYSQL_OPT_CONNECT_TIMEOUT",
    "MYSQL_OPT_READ_TIMEOUT",
    "MYSQL_OPT_WRITE_TIMEOUT",
    "MYSQL_OPT_RECONNECT",
    "MYSQL_SET_CHARSET_NAME",
):
    assert option in constructor
for option in (
    "MYSQL_OPT_SSL_ENFORCE",
    "MYSQL_OPT_SSL_VERIFY_SERVER_CERT",
    "MYSQL_OPT_SSL_CA",
    "CLIENT_SSL",
    "mysql_get_ssl_cipher",
):
    assert option in constructor
assert "DB_TLS" in runtime and "DB_SSL_CA" in runtime
print("[PASS] database connection deadlines and verified remote TLS are canonical")

session = section(sql, "static bool sql_apply_session_contract", "MYSQL *sql_open_configured_connection")
verify = section(sql, "static bool sql_verify_session_contract", "static bool sql_apply_session_contract")
assert "mysql_set_character_set(conn, RUNTIME_DB_CHARACTER_SET)" in session
assert "SET SESSION time_zone='+00:00'" in session
assert "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED" in session
assert "RUNTIME_DB_SQL_MODE" in session
assert "STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION" in runtime_contract
assert "@@character_set_connection,@@time_zone,@@sql_mode" in verify
assert "@@transaction_isolation" in verify and "@@tx_isolation" in verify
assert "strcasecmp(row[0], RUNTIME_DB_ISOLATION)" in verify
print("[PASS] charset, UTC, isolation, and SQL mode are set and verified")

assert sql.count("mysql_real_connect(") == 1
assert "mysql_real_connect(" not in pool
assert "mysql_real_connect(" not in player
assert "sql_open_configured_connection(CLIENT_MULTI_STATEMENTS)" in pool
assert "return sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);" in player
assert "persistenceDB = sql_open_configured_connection(0);" in sql
assert "DB = sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);" in sql
print("[PASS] main, pool, child, and legacy connections share one constructor")

assert "bool runtime_listener_address(struct sockaddr_in6 *address);" in comm_h
listener = section(comm, "bool runtime_listener_address(sockaddr_in6 *address)", "int init_socket")
assert 'getenv("LISTEN_ADDRESS")' in listener
assert "inet_pton(AF_INET6" in listener and "inet_pton(AF_INET" in listener
assert "runtime_listener_address(&sa)" in websocket
init_socket = section(comm, "int init_socket(int port)", "int new_connection")
assert "runtime_listener_address(&sa)" in init_socket
print("[PASS] every game listener applies the explicit numeric bind address")

fallback = section(ssl, "static bool ssl_local_fallback_allowed", "#define YELL")
assert 'strcmp(role, "local")' in fallback
assert 'strcmp(address, "127.0.0.1")' in fallback
assert 'strcmp(address, "::1")' in fallback
cert = section(ssl, "void ssl_read_cert(void)", "gnutls_session_t ssl_new")
assert "ssl_local_fallback_allowed()" in cert
assert "key_st.st_uid != geteuid()" in cert
assert "(key_st.st_mode & 0777) & ~0600" in cert
assert "exit(1);" in cert
assert comm.count("ssl_read_cert();") == 1
print("[PASS] tracked localhost key is loopback-local only; network key failure is fatal")

for field in (
    "ENVIRONMENT=local",
    "LISTEN_ADDRESS=127.0.0.1",
    "DB_ALLOWED_TARGETS=127.0.0.1/duris_dev",
    "DB_TLS=FALSE",
    "DB_SSL_CA=",
):
    assert field in example
assert "DB_USER=\n" in example and "DB_PASSWD=\n" in example and "DB_NAME=\n" in example
assert "grep '#define DB_" not in cycle
assert "CREATE TABLE IF NOT EXISTS server_reboots" not in cycle
assert "[ -L .env ]" in cycle
assert "Resolved database target is not allow-listed" in cycle
assert 'EFFECTIVE_DB_NAME="duris_dev"' in cycle
assert "--ssl-verify-server-cert" in cycle
assert 'mysql "${MYSQL_CONNECTION_ARGS[@]}" "$EFFECTIVE_DB_NAME"' in cycle
assert 'export MYSQL_PWD="$DB_PASSWD"' in cycle
print("[PASS] examples and launcher provide no deployable defaults or preflight writes")

print("runtime connection trust-boundary contracts passed")
