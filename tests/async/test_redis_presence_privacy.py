#!/usr/bin/env python3
"""Runtime JSON escaping and source contracts for Redis presence privacy."""

from _paths import SRC, rel
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "redis/redis_presence_payload.h"
#include "persistence/presence_policy.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cjson/cJSON.h>

static cJSON *encode(bool private_fields)
{
    redis_presence_fields fields = {
        "Name\"\\\n", "acct\"", "elf", "mage", "127.0.0.1",
        "Client\"\\\n", "1.0\tdev", 56, 1, 12345, private_fields
    };
    char *json = redis_presence_payload_encode(fields);
    assert(json);
    cJSON *root = cJSON_Parse(json);
    free(json);
    return root;
}

int main()
{
    unsetenv("DURISWEB_PRIVATE_PRESENCE");
    assert(!durisweb_private_presence_enabled());
    setenv("DURISWEB_PRIVATE_PRESENCE", "true", 1);
    assert(!durisweb_private_presence_enabled());
    setenv("DURISWEB_PRIVATE_PRESENCE", "TRUE", 1);
    assert(durisweb_private_presence_enabled());

    cJSON *public_payload = encode(false);
    assert(public_payload);
    assert(!strcmp(cJSON_GetObjectItem(public_payload, "name")->valuestring,
                   "Name\"\\\n"));
    assert(!cJSON_GetObjectItem(public_payload, "account"));
    assert(!cJSON_GetObjectItem(public_payload, "ip"));
    assert(!cJSON_GetObjectItem(public_payload, "client"));
    assert(!cJSON_GetObjectItem(public_payload, "client_version"));
    cJSON_Delete(public_payload);

    cJSON *private_payload = encode(true);
    assert(private_payload);
    assert(!strcmp(cJSON_GetObjectItem(private_payload, "account")->valuestring,
                   "acct\""));
    assert(!strcmp(cJSON_GetObjectItem(private_payload, "client")->valuestring,
                   "Client\"\\\n"));
    assert(!strcmp(cJSON_GetObjectItem(private_payload, "client_version")->valuestring,
                   "1.0\tdev"));
    cJSON_Delete(private_payload);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-presence-") as temp:
    source = Path(temp) / "presence_test.cpp"
    binary = Path(temp) / "presence_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
            str(source), rel("redis_presence_payload.c"), rel("presence_policy.c"),
            "-lcjson", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

runtime = (SRC / "redis_presence_runtime.c").read_text(encoding="ascii")
online = runtime[
    runtime.index("void redis_player_online") : runtime.index("void redis_player_offline")
]
offline = runtime[
    runtime.index("void redis_player_offline") : runtime.index(
        "void redis_clear_online_players"
    )
]
clear = runtime[runtime.index("void redis_clear_online_players") :]
assert "durisweb_presence_character_visible" in online
assert "durisweb_private_presence_enabled" in online
assert "redis_presence_payload_encode" in online
assert "snprintf" not in online
assert "durisweb_presence_character_visible" in offline
assert "redis_presence_worker_submit_online" in online
assert "redis_presence_worker_submit_offline" in online and "redis_presence_worker_submit_offline" in offline
assert "redis_presence_worker_submit_clear" in clear
for path in (online, offline, clear):
    assert "redis_command" not in path and "redis_ctx" not in path

handlers = (SRC / "ws_handlers.c").read_text(encoding="utf-8")
assert "ws_private_presence_enabled" not in handlers
assert "durisweb_presence_character_visible" in handlers
assert "durisweb_private_presence_enabled" in handlers

print("Redis presence privacy and JSON escaping checks passed")
