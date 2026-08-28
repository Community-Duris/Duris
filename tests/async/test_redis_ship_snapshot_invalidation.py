from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis_maintenance.h").read_text()
source = (ROOT / "src/redis_maintenance.c").read_text()
legacy_header = (ROOT / "src/redis_ship_legacy.h").read_text()
legacy = (ROOT / "src/redis_ship_legacy.c").read_text()
sql = (ROOT / "src/sql_player.c").read_text()

assert "bool redis_clear_pwipe_state(void);" in header
assert "redis_load_ship_snapshot" not in header
assert "redis_cache_ship_snapshot" not in header
assert "redis_load_ship_snapshot" not in legacy
assert "redis_cache_ship_snapshot" not in legacy

load_start = sql.index("P_ship sql_load_ship(const char *owner_name)")
load_end = sql.index("bool sql_load_all_ships()", load_start)
load = sql[load_start:load_end]
assert "from ships where owner_name" in load
assert "redis_" not in load

save_start = sql.index("bool sql_save_ship(P_ship ship)")
save_end = sql.index("static bool sql_load_ship_armor", save_start)
assert "redis_" not in sql[save_start:save_end]

assert "bool redis_clear_ship_snapshots(struct redisContext *context)" in legacy_header
assert '"SCAN %s MATCH %s COUNT 256"' in legacy
assert "REDIS_SHIP_SNAPSHOT_PATTERN" in legacy
assert "redis_cache_store_delete(key)" in legacy
assert "redis_shared_command_observability_record" in legacy
assert "redis_clear_ship_snapshots(context)" in source
for caller in ("src/sql_player.c", "src/ships/ship_base.c"):
    caller_source = (ROOT / caller).read_text()
    assert '#include "redis_ship_legacy.h"' in caller_source
    assert '#include "redis.h"' not in caller_source

HARNESS = r'''
#include "redis_ship_legacy.h"
#include "redis_command_observability.h"

#include <hiredis/hiredis.h>

#include <cassert>
#include <cstdlib>

bool redis_cache_store_delete(const char *)
{
    return true;
}

static void command_ok(redisContext *context, const char *format, int value)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(context, format, value));
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    redisContext *context = redisConnect("127.0.0.1", atoi(argv[1]));
    assert(context && !context->err);
    for (int index = 0; index < 600; ++index)
        command_ok(context, "SET ship:snapshot:Owner%d value", index);
    command_ok(context, "SET unrelated:key value%d", 1);

    redis_shared_command_observability_reset(true);
    assert(!redis_clear_ship_snapshots(nullptr));
    assert(redis_clear_ship_snapshots(context));

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(context, "KEYS ship:snapshot:*")
    );
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 0);
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(redisCommand(context, "EXISTS unrelated:key"));
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    const redis_shared_command_health health = redis_shared_command_health_copy();
    const redis_shared_scope_health &maintenance = health.scopes[REDIS_SHARED_SCOPE_MAINTENANCE];
    assert(maintenance.calls > 2 && maintenance.successes == maintenance.calls);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_SCAN] >= 2);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_WRITE] == 600);
    redisFree(context);
    return 0;
}
'''


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


if not shutil.which("redis-server"):
    raise SystemExit("redis-server is required")

with tempfile.TemporaryDirectory(prefix="duris-ship-legacy-") as temp:
    source_path = Path(temp) / "ship_legacy_test.cpp"
    binary = Path(temp) / "ship_legacy_test"
    source_path.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            str(source_path),
            "src/redis_ship_legacy.c",
            "src/redis_command_observability.c",
            "-lhiredis",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    port = free_port()
    server = subprocess.Popen(
        [
            "redis-server",
            "--bind",
            "127.0.0.1",
            "--port",
            str(port),
            "--save",
            "",
            "--appendonly",
            "no",
            "--dir",
            temp,
            "--dbfilename",
            "isolated.rdb",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        for _ in range(100):
            probe = subprocess.run(
                ["redis-cli", "-h", "127.0.0.1", "-p", str(port), "PING"],
                capture_output=True,
                text=True,
                check=False,
            )
            if probe.returncode == 0 and probe.stdout.strip() == "PONG":
                break
            time.sleep(0.02)
        else:
            raise AssertionError("isolated redis-server did not start")
        subprocess.run([str(binary), str(port)], check=True)
    finally:
        server.terminate()
        server.wait(timeout=5)
print("MySQL ship authority and legacy Redis invalidation checks passed")
