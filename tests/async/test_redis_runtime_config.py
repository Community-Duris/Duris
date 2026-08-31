#!/usr/bin/env python3
"""Runtime checks for scoped Redis endpoint and identity configuration."""

from _paths import rel
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "redis/redis_runtime_config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char *variables[] = {
    "ENVIRONMENT", "REDIS_SOCKET", "REDIS_HOST", "REDIS_PORT", "REDIS_DB", "REDIS_TLS",
    "REDIS_CA_CERT", "REDIS_TLS_SERVER_NAME", "REDIS_USERNAME", "REDIS_PASSWORD",
    "REDIS_WORLD_USERNAME", "REDIS_WORLD_PASSWORD", "REDIS_PRESENCE_USERNAME",
    "REDIS_PRESENCE_PASSWORD", "REDIS_CACHE_USERNAME", "REDIS_CACHE_PASSWORD",
    "REDIS_DONATION_USERNAME", "REDIS_DONATION_PASSWORD", "REDIS_MAINTENANCE_USERNAME",
    "REDIS_MAINTENANCE_PASSWORD",
};

static void clear_environment()
{
    for (const char *name : variables)
        unsetenv(name);
}

static void set_identity(const char *scope, const char *username)
{
    char username_name[64];
    char password_name[64];
    snprintf(username_name, sizeof username_name, "REDIS_%s_USERNAME", scope);
    snprintf(password_name, sizeof password_name, "REDIS_%s_PASSWORD", scope);
    setenv(username_name, username, 1);
    setenv(password_name, "test-password", 1);
}

int main()
{
    redis_runtime_connections connections = {};

    clear_environment();
    setenv("ENVIRONMENT", "development", 1);
    assert(redis_runtime_connections_configure(false, &connections));
    assert(connections.world && connections.presence && connections.cache && connections.maintenance);
    assert(!connections.donation);
    assert(!connections.unix_socket);
    assert(connections.host && !strcmp(connections.host, "127.0.0.1"));
    assert(connections.port == 6379);
    redis_runtime_connections_destroy(&connections);
    assert(!connections.world && !connections.host && connections.port == 0);

    clear_environment();
    setenv("ENVIRONMENT", "development", 1);
    setenv("REDIS_PORT", "0", 1);
    assert(!redis_runtime_connections_configure(false, &connections));
    setenv("REDIS_PORT", "6379junk", 1);
    assert(!redis_runtime_connections_configure(false, &connections));
    unsetenv("REDIS_PORT");
    setenv("REDIS_DB", "256", 1);
    assert(!redis_runtime_connections_configure(false, &connections));
    unsetenv("REDIS_DB");
    setenv("REDIS_TLS", "sometimes", 1);
    assert(!redis_runtime_connections_configure(false, &connections));

    clear_environment();
    setenv("ENVIRONMENT", "development", 1);
    setenv("REDIS_SOCKET", "/tmp/redis.sock", 1);
    setenv("REDIS_HOST", "127.0.0.1", 1);
    assert(!redis_runtime_connections_configure(false, &connections));
    unsetenv("REDIS_HOST");
    assert(redis_runtime_connections_configure(false, &connections));
    assert(connections.unix_socket && !connections.host && connections.port == 0);
    redis_runtime_connections_destroy(&connections);

    clear_environment();
    setenv("ENVIRONMENT", "production", 1);
    setenv("REDIS_HOST", "127.0.0.1", 1);
    set_identity("WORLD", "world-user");
    set_identity("PRESENCE", "presence-user");
    set_identity("CACHE", "cache-user");
    set_identity("MAINTENANCE", "maintenance-user");
    assert(redis_runtime_connections_configure(false, &connections));
    redis_runtime_connections_destroy(&connections);
    assert(!redis_runtime_connections_configure(true, &connections));
    set_identity("DONATION", "donation-user");
    assert(redis_runtime_connections_configure(true, &connections));
    assert(connections.donation);
    redis_runtime_connections_destroy(&connections);

    set_identity("CACHE", "world-user");
    assert(!redis_runtime_connections_configure(true, &connections));
    set_identity("CACHE", "cache-user");
    unsetenv("REDIS_PRESENCE_PASSWORD");
    assert(!redis_runtime_connections_configure(true, &connections));

    clear_environment();
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-redis-runtime-config-") as temp:
    source = Path(temp) / "runtime_config_test.cpp"
    binary = Path(temp) / "runtime_config_test"
    source.write_text(HARNESS, encoding="ascii")
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
            str(source),
            rel("redis_runtime_config.c"),
            rel("redis_connection.c"),
            "-lhiredis",
            "-lhiredis_ssl",
            "-lssl",
            "-lcrypto",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

print("Redis runtime endpoint and scoped identity configuration passed")
