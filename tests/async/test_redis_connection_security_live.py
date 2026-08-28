#!/usr/bin/env python3
"""Prove runtime Redis authentication, database selection, and verified TLS."""

from __future__ import annotations

import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PASSWORD = "redis-connection-live-secret"


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def wait_for_redis(port: int, tls: bool = False, ca_cert: Path | None = None) -> None:
    command = [
        "redis-cli",
        "-h",
        "127.0.0.1",
        "-p",
        str(port),
        "--no-auth-warning",
        "-a",
        PASSWORD,
    ]
    if tls:
        command.extend(["--tls", "--cacert", str(ca_cert), "--sni", "localhost"])
    command.append("PING")
    for _ in range(100):
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode == 0 and "PONG" in result.stdout:
            return
        time.sleep(0.05)
    raise AssertionError(f"Redis on port {port} did not become ready")


def wait_for_redis_socket(path: Path) -> None:
    command = [
        "redis-cli", "-s", str(path), "--no-auth-warning", "-a", PASSWORD, "PING"
    ]
    for _ in range(100):
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode == 0 and "PONG" in result.stdout:
            return
        time.sleep(0.05)
    raise AssertionError(f"Redis socket {path} did not become ready")


def main() -> None:
    for executable in ("redis-server", "redis-cli", "openssl", "g++"):
        if not shutil.which(executable):
            raise SystemExit(f"{executable} is required")

    harness = r'''
#include "redis_connection.h"

#include <hiredis/hiredis.h>

#include <cassert>
#include <cstdlib>

static void set_marker(const redis_connection_options &options, const char *key)
{
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redisContext *context = redis_connection_open(settings);
    assert(context && !context->err);
    redisReply *reply = (redisReply *)redisCommand(context, "SET %s 1", key);
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    redisFree(context);
    redis_connection_settings_destroy(settings);
}

int main(int argc, char **argv)
{
    assert(argc == 6);
    redis_connection_options options = {
        "127.0.0.1", atoi(argv[1]), 250, 250, 2, nullptr, argv[3], false,
        nullptr, nullptr, false, nullptr};
    set_marker(options, "mud:auth-db-marker");

    options.username = "default";
    set_marker(options, "mud:acl-marker");

    options.username = nullptr;
    options.password = "wrong-password";
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redisContext *context = redis_connection_open(settings);
    assert(!context);
    redis_connection_settings_destroy(settings);

    options.password = argv[3];
    options.require_tls = true;
    assert(!redis_connection_settings_create(&options));
    options.require_tls = false;
    options.database = 256;
    assert(!redis_connection_settings_create(&options));

    options.port = atoi(argv[2]);
    options.database = 3;
    options.tls = true;
    options.ca_cert = argv[4];
    options.server_name = "localhost";
    options.require_tls = true;
    set_marker(options, "mud:tls-marker");

    options.server_name = "wrong.invalid";
    settings = redis_connection_settings_create(&options);
    assert(settings);
    context = redis_connection_open(settings);
    assert(!context);
    redis_connection_settings_destroy(settings);

    options.host = nullptr;
    options.port = 0;
    options.database = 4;
    options.tls = false;
    options.ca_cert = nullptr;
    options.server_name = nullptr;
    options.require_tls = false;
    options.unix_socket = argv[5];
    set_marker(options, "mud:socket-marker");

    options.tls = true;
    assert(!redis_connection_settings_create(&options));
    options.tls = false;
    options.host = "127.0.0.1";
    assert(!redis_connection_settings_create(&options));
    options.host = nullptr;
    options.unix_socket = "relative.sock";
    assert(!redis_connection_settings_create(&options));
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-connection-security-") as temp_dir:
        temp = Path(temp_dir)
        plain_port = free_port()
        tls_port = free_port()
        socket_path = temp / "redis.sock"
        source = temp / "harness.cpp"
        binary = temp / "harness"
        ca_key = temp / "ca.key"
        ca_cert = temp / "ca.crt"
        server_key = temp / "server.key"
        server_csr = temp / "server.csr"
        server_cert = temp / "server.crt"
        extension = temp / "server.ext"
        source.write_text(harness, encoding="ascii")
        extension.write_text(
            "subjectAltName=DNS:localhost,IP:127.0.0.1\n"
            "extendedKeyUsage=serverAuth\n",
            encoding="ascii",
        )
        subprocess.run(
            [
                "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                "-keyout", str(ca_key), "-out", str(ca_cert), "-days", "1",
                "-subj", "/CN=Duris Redis Test CA",
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        subprocess.run(
            [
                "openssl", "req", "-newkey", "rsa:2048", "-nodes",
                "-keyout", str(server_key), "-out", str(server_csr),
                "-subj", "/CN=localhost",
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        subprocess.run(
            [
                "openssl", "x509", "-req", "-in", str(server_csr),
                "-CA", str(ca_cert), "-CAkey", str(ca_key), "-CAcreateserial",
                "-out", str(server_cert), "-days", "1", "-extfile", str(extension),
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-I", str(ROOT / "src"), str(ROOT / "src" / "redis_connection.c"),
                str(source), "-lhiredis", "-lhiredis_ssl", "-lssl", "-lcrypto",
                "-pthread", "-o", str(binary),
            ],
            check=True,
        )

        plain_server = subprocess.Popen(
            [
                "redis-server", "--bind", "127.0.0.1", "--port", str(plain_port),
                "--unixsocket", str(socket_path), "--unixsocketperm", "700",
                "--save", "", "--appendonly", "no", "--dir", str(temp),
                "--requirepass", PASSWORD,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        tls_server = subprocess.Popen(
            [
                "redis-server", "--bind", "127.0.0.1", "--port", "0",
                "--tls-port", str(tls_port), "--tls-cert-file", str(server_cert),
                "--tls-key-file", str(server_key), "--tls-ca-cert-file", str(ca_cert),
                "--tls-auth-clients", "no", "--save", "", "--appendonly", "no",
                "--dir", str(temp), "--requirepass", PASSWORD,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            wait_for_redis(plain_port)
            wait_for_redis_socket(socket_path)
            wait_for_redis(tls_port, tls=True, ca_cert=ca_cert)
            subprocess.run(
                [str(binary), str(plain_port), str(tls_port), PASSWORD, str(ca_cert),
                 str(socket_path)],
                check=True,
            )
            db_two = subprocess.run(
                [
                    "redis-cli", "-h", "127.0.0.1", "-p", str(plain_port),
                    "--no-auth-warning", "-a", PASSWORD, "-n", "2", "DBSIZE",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            db_zero = subprocess.run(
                [
                    "redis-cli", "-h", "127.0.0.1", "-p", str(plain_port),
                    "--no-auth-warning", "-a", PASSWORD, "-n", "0", "DBSIZE",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            assert db_two.stdout.strip() == "2"
            assert db_zero.stdout.strip() == "0"
            socket_db = subprocess.run(
                [
                    "redis-cli", "-s", str(socket_path), "--no-auth-warning", "-a",
                    PASSWORD, "-n", "4", "DBSIZE",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            assert socket_db.stdout.strip() == "1"
        finally:
            plain_server.terminate()
            tls_server.terminate()
            plain_server.wait(timeout=5)
            tls_server.wait(timeout=5)

    print("Redis TCP/TLS and Unix-socket authentication and database selection passed")


if __name__ == "__main__":
    main()
