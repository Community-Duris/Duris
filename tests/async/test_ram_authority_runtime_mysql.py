#!/usr/bin/env python3
"""Full runtime linkage and crash recovery against an explicitly isolated SQL database.

Requires TEST_DB_HOST=127.0.0.1, TEST_DB_USER, TEST_DB_PASSWORD and TEST_DB_PORT.
RAM_RUNTIME_BIN_ROOT selects an already built tree (otherwise bin is built).
Never reads the checkout .env; creates and drops only its own random schema.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[2]


def main():
    assert os.environ.get("TEST_DB_HOST") == "127.0.0.1", "an explicit isolated loopback database is required"
    database = "ram_authority_test_" + uuid.uuid4().hex[:12]
    env = {"PATH": os.environ["PATH"], "ENVIRONMENT": "test", "DB_HOST": "127.0.0.1",
           "DB_PORT": os.environ["TEST_DB_PORT"], "DB_USER": os.environ["TEST_DB_USER"],
           "DB_PASSWD": os.environ["TEST_DB_PASSWORD"], "DB_NAME": database,
           "DB_ALLOWED_TARGETS": "127.0.0.1/" + database, "DB_TLS": "FALSE",
           "MYSQL_PWD": os.environ["TEST_DB_PASSWORD"], "PERSISTENCE_MODE": "mariadb-primary"}
    mysql = ["mysql", "--protocol=tcp", "-h127.0.0.1", "-P"+env["DB_PORT"], "-u"+env["DB_USER"], "-N", "-B"]
    def sql(text, selected=True):
        subprocess.run(mysql + ([database] if selected else []), input=text, text=True, env=env, check=True)
    bin_root = Path(os.environ.get("RAM_RUNTIME_BIN_ROOT", ROOT / "bin")).resolve()
    if "RAM_RUNTIME_BIN_ROOT" not in os.environ:
        subprocess.run(["make", "-C", "src", "-j2", "BIN_ROOT="+str(bin_root)], cwd=ROOT, check=True)
    sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (["adopt", "--kind", "fresh_bootstrap"], ["run"]):
            subprocess.run(["python3", "scripts/migration_runner.py", *args], cwd=ROOT, env=env, check=True)
        # Link the exact server objects, replacing only its process entry point.
        # No economic, persistence, custody, or writer adapter is mocked.
        objects = shlex.split(subprocess.check_output([
            "make", "-s", "-C", "src", "BIN_ROOT="+str(bin_root),
            "--eval=print-runtime-test-objects: ; @echo $(DMS_OBJS)", "print-runtime-test-objects"], cwd=ROOT, text=True))
        objects = [str(Path(p) if Path(p).is_absolute() else ROOT / "src" / p) for p in objects]
        with tempfile.TemporaryDirectory(prefix="ram-runtime-") as directory:
            scratch = Path(directory)
            comm = next(p for p in objects if p.endswith("/net/comm.o"))
            renamed = scratch / "comm.o"
            subprocess.run(["objcopy", "--redefine-sym", "main=duris_test_server_main", comm, str(renamed)], check=True)
            objects[objects.index(comm)] = str(renamed)
            binary = scratch / "runtime"
            libs = "-lm -lstdc++ -lz -lxml2 -lcrypt -lmysqlclient -lgnutls -lssl -lcrypto -lcjson -lhiredis -lhiredis_ssl -lbsd -lcurl -pthread".split()
            subprocess.run(["g++", "-std=c++20", "-g", "-DTEST_MUD", "-D__NO_TESTS__", "-Isrc", "-I/usr/include/mysql",
                            "tests/async/ram_authority_runtime_mysql_harness.cpp", *objects, *libs, "-o", str(binary)], cwd=ROOT, check=True)
            journal = scratch / "journal"
            journal.mkdir(mode=0o700)
            for mode in ("crash", "recover", "item-crash", "item-recover", "shop-buy-crash", "shop-sell-crash", "shop-produced-crash", "shop-destroy-crash", "shop-discard-crash", "shop-recover"):
                command = [str(binary), str(journal), mode]
                if os.environ.get("RAM_RUNTIME_DEBUG"):
                    command = ["gdb", "--batch", "--return-child-result", "-ex", "run", "-ex", "thread apply all bt", "--args", *command]
                subprocess.run(command, env=env, cwd=scratch, check=True, timeout=120)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    main()
