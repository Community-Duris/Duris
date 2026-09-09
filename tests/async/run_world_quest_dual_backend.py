#!/usr/bin/env python3
"""Real local bartender-quest journey for MariaDB and flatfile-primary builds.

This is intentionally a ``run_`` script rather than an automatically discovered
regression: it builds and boots the full world twice and needs Docker for the
isolated MariaDB authority. It never reads the repository's .env and never uses
the long-running local stack. Every account/database/state root is disposable.
"""

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import uuid

from test_flatfile_combat_journey import MudClient
from quest_character_flow import create_quest_character


ROOT = pathlib.Path(__file__).resolve().parents[2]
BARTENDER_ROOM = 16633
DB_IMAGE = os.environ.get("DURIS_WORLD_QUEST_DB_IMAGE", "mariadb:11.4")
REQUESTED_BACKEND = os.environ.get("DURIS_WORLD_QUEST_BACKEND", "both")
BINARY_OVERRIDE = os.environ.get("DURIS_WORLD_QUEST_BINARY")
if REQUESTED_BACKEND not in {"both", "mariadb", "flatfile"}:
    raise ValueError("DURIS_WORLD_QUEST_BACKEND must be both, mariadb, or flatfile")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def available_ports() -> tuple[int, int, int]:
    for _ in range(200):
        probes = [socket.socket() for _ in range(3)]
        try:
            probes[0].bind(("127.0.0.1", 0))
            plain = probes[0].getsockname()[1]
            if plain <= 1024 or plain >= 65533:
                continue
            probes[1].bind(("127.0.0.1", plain + 1))
            probes[2].bind(("127.0.0.1", plain + 2))
            return plain, plain + 1, plain + 2
        except OSError:
            continue
        finally:
            for probe in probes:
                probe.close()
    raise AssertionError("could not reserve isolated listener ports")


def run(command: list[str], *, env: dict[str, str] | None = None,
        timeout: int = 600, input_bytes: bytes | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(command, cwd=ROOT, env=env, input=input_bytes,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=timeout, check=False)


def build_server(backend: str, build_root: pathlib.Path) -> pathlib.Path:
    if BINARY_OVERRIDE:
        binary = pathlib.Path(BINARY_OVERRIDE).resolve()
        require(binary.is_file() and os.access(binary, os.X_OK),
                f"configured binary override is not executable: {binary}")
        return binary
    backend_root = build_root / backend
    backend_root.mkdir(parents=True, exist_ok=True)
    binary = backend_root / "server" / "dms_new"
    result = run([
        "make", "-C", "src", f"PERSISTENCE_BACKEND={backend}",
        "BUILD_PROFILE=development", f"BIN_ROOT={backend_root}",
        f"OBJDIR={backend_root / 'objects' / 'server'}",
        f"SERVER_BIN_DIR={binary.parent}", f"DMS_BINARY={binary}", "-j2",
    ], timeout=600)
    require(result.returncode == 0,
            f"{backend} server build failed:\n{result.stdout[-12000:].decode(errors='replace')}")
    output = result.stdout.decode(errors="replace")
    if backend == "flatfile":
        require("-D__NO_MYSQL__" in output, "flat build did not define __NO_MYSQL__")
        require("-I/usr/include/mysql" not in output, "flat build used MySQL headers")
        require("-lmysqlclient" not in output, "flat build linked MySQL")
    else:
        require("-D__NO_MYSQL__" not in output, "MariaDB build selected flatfile mode")
    require(binary.is_file() and os.access(binary, os.X_OK),
            f"{backend} build did not create an executable")
    return binary


def wait_for_db(container: str, root_password: str) -> None:
    for _ in range(120):
        result = subprocess.run([
            "docker", "exec", "-e", "MYSQL_PWD=" + root_password,
            container, "mariadb", "-h127.0.0.1", "-uroot", "-N", "-e", "SELECT 1",
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        if result.returncode == 0:
            return
        time.sleep(1)
    raise AssertionError("isolated MariaDB did not become ready")


def docker_published_port(container: str) -> int:
    result = subprocess.run(
        ["docker", "port", container, "3306/tcp"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    require(result.returncode == 0, "could not read disposable MariaDB published port")
    for mapping in result.stdout.splitlines():
        _, separator, port_text = mapping.strip().rpartition(":")
        if not separator:
            continue
        try:
            port = int(port_text)
        except ValueError:
            continue
        if 1024 < port <= 65535:
            return port
    raise AssertionError("disposable MariaDB did not publish a usable host port")


def start_database(database: str) -> tuple[str, int, str, str]:
    container = f"duris-world-quest-{os.getpid()}-{uuid.uuid4().hex[:8]}"
    root_password = "root-" + uuid.uuid4().hex
    db_password = "db-" + uuid.uuid4().hex
    try:
        result = subprocess.run([
            "docker", "run", "--rm", "-d", "--name", container,
            "-e", "MARIADB_DATABASE=" + database,
            "-e", "MARIADB_USER=duris",
            "-e", "MARIADB_PASSWORD=" + db_password,
            "-e", "MARIADB_ROOT_PASSWORD=" + root_password,
            "--publish", "127.0.0.1::3306/tcp", DB_IMAGE,
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
        require(result.returncode == 0, "could not start disposable MariaDB")
        host_port = docker_published_port(container)
        wait_for_db(container, root_password)
        bootstrap = (ROOT / "migrations" / "bootstrap_multithread_safe.sql").read_bytes()
        loaded = subprocess.run([
            "docker", "exec", "-i", "-e", "MYSQL_PWD=" + root_password,
            container, "mariadb", "-h127.0.0.1", "-uroot", database,
        ], input=bootstrap, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
        require(loaded.returncode == 0,
                "disposable MariaDB bootstrap failed:\n" + loaded.stdout.decode(errors="replace")[-12000:])
        return container, host_port, root_password, db_password
    except BaseException:
        subprocess.run(["docker", "rm", "-f", container], stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=False)
        raise


def migration_environment(port: int, database: str, password: str) -> dict[str, str]:
    environment = os.environ.copy()
    environment.update({
        "ENVIRONMENT": "local",
        "DB_HOST": "127.0.0.1",
        "DB_PORT": str(port),
        "DB_USER": "duris",
        "DB_PASSWD": password,
        "DB_NAME": database,
        "DB_ALLOWED_TARGETS": "127.0.0.1/" + database,
        "DB_TLS": "FALSE",
        "DB_SOCKET": "",
        "REDIS": "FALSE",
    })
    return environment


def prepare_mariadb(database: str) -> tuple[str, int, str, str, dict[str, str]]:
    container, port, root_password, db_password = start_database(database)
    environment = migration_environment(port, database, db_password)
    adopted = run(["python3", "scripts/migration_runner.py", "adopt", "--kind", "fresh_bootstrap"], env=environment)
    require(adopted.returncode == 0,
            "fresh MariaDB baseline adoption failed:\n" + adopted.stdout.decode(errors="replace")[-12000:])
    migrated = run(["python3", "scripts/migration_runner.py", "run"], env=environment)
    require(migrated.returncode == 0,
            "fresh MariaDB migrations failed:\n" + migrated.stdout.decode(errors="replace")[-12000:])
    return container, port, root_password, db_password, environment


def setup_run_root(run_root: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path, pathlib.Path]:
    for directory in ("areas", "docs"):
        (run_root / directory).symlink_to(ROOT / directory, target_is_directory=True)
    shutil.copytree(ROOT / "lib", run_root / "lib")
    (run_root / "logs" / "log").mkdir(parents=True)
    (run_root / "logs" / "log" / ".gitignore").write_text("*\n!.gitignore\n")
    journals = run_root / "journals"
    player_journal = journals / "players"
    critical_journal = journals / "critical"
    player_journal.mkdir(parents=True, mode=0o700)
    critical_journal.mkdir(mode=0o700)
    certificate = run_root / "duris.crt"
    private_key = run_root / "duris.key"
    generated = run([
        "openssl", "req", "-x509", "-newkey", "rsa:2048", "-sha256", "-nodes",
        "-days", "1", "-subj", "/CN=localhost", "-keyout", str(private_key),
        "-out", str(certificate),
    ], timeout=30)
    require(generated.returncode == 0, "could not create isolated TLS certificate")
    private_key.chmod(0o600)
    return player_journal, critical_journal, run_root / "boot.out"


def runtime_environment(backend: str, state_root: pathlib.Path, player_journal: pathlib.Path,
                        critical_journal: pathlib.Path, plain_port: int, tls_port: int,
                        websocket_port: int, database_environment: dict[str, str] | None) -> dict[str, str]:
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local",
        "PERSISTENCE_MODE": "flatfile-primary" if backend == "flatfile" else "mariadb-primary",
        "PLAYER_SAVE_JOURNAL_DIR": str(player_journal),
        "CRITICAL_COMMAND_JOURNAL_DIR": str(critical_journal),
        "LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_TLS_PORT": str(tls_port),
        "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_WEBSOCKET_PORT": str(websocket_port),
        "REDIS": "FALSE",
        "CHAOS_MUD": "TRUE",
        "CHAOS_TEST_COMMANDS": "TRUE",
        "CHAOS_TEST_ACCOUNT": "Journeyacct",
        "CHAOS_STARTER_BONUSES": "FALSE",
    }
    if backend == "flatfile":
        environment["FLATFILE_STATE_DIR"] = str(state_root)
    else:
        if database_environment is None:
            raise AssertionError("MariaDB runtime environment missing")
        environment.update({key: database_environment[key] for key in (
            "DB_HOST", "DB_PORT", "DB_USER", "DB_PASSWD", "DB_NAME",
            "DB_ALLOWED_TARGETS", "DB_TLS", "DB_SOCKET",
        )})
    return environment


def wait_for_boot(process: subprocess.Popen[str], output_path: pathlib.Path) -> str:
    deadline = time.monotonic() + 180
    output = ""
    while time.monotonic() < deadline:
        output = output_path.read_text(errors="replace")
        if "Entering game loop." in output:
            evidence = output + "\n" + collect_logs(output_path.parent)
            require("World quest catalog ready:" in evidence,
                    "server entered the game loop without catalog-ready evidence:\n" + evidence[-12000:])
            return evidence
        if process.poll() is not None:
            break
        time.sleep(0.1)
    raise AssertionError("server did not reach the game loop:\n" + output[-16000:])


def collect_logs(run_root: pathlib.Path) -> str:
    sections = []
    for path in sorted((run_root / "logs" / "log").glob("*")):
        if not path.is_file() or path.name == ".gitignore":
            continue
        text = path.read_text(errors="replace").strip()
        if text:
            sections.append(f"--- {path.name} ---\n{text[-12000:]}")
    return "\n".join(sections)[-50000:]


def perform_quest_journey(binary: pathlib.Path, backend: str, state_root: pathlib.Path,
                          database_environment: dict[str, str] | None) -> dict:
    with tempfile.TemporaryDirectory(prefix=f"world-quest-{backend}-run-") as run_tmp:
        run_root = pathlib.Path(run_tmp)
        player_journal, critical_journal, output_path = setup_run_root(run_root)
        plain_port, tls_port, websocket_port = available_ports()
        environment = runtime_environment(
            backend, state_root, player_journal, critical_journal, plain_port,
            tls_port, websocket_port, database_environment)
        with output_path.open("w") as output:
            process = subprocess.Popen(
                [str(binary), "-d", str(run_root), str(plain_port)],
                cwd=run_root, env=environment, stdout=output, stderr=subprocess.STDOUT,
                text=True,
            )
        client = None
        try:
            boot_output = wait_for_boot(process, output_path)
            binary_path = binary.resolve()
            running_path = pathlib.Path(os.readlink(f"/proc/{process.pid}/exe")).resolve()
            require(running_path == binary_path,
                    f"running executable mismatch: expected {binary_path}, got {running_path}")
            client = MudClient(plain_port)
            create_quest_character(client)
            client.send(f"chaos questroom {BARTENDER_ROOM}")
            client.expect("Quest-room test move complete.", timeout=15)
            client.expect("Quest-room test funds queued.", timeout=15)
            client.expect("Quest-room test funds committed.", timeout=60)
            client.send("save")
            client.expect("Save complete for Taverek.", timeout=120)
            client.send("look")
            client.expect("Mathorn's Bar", timeout=15)
            client.send("ask bartender quest")
            first_result, _ = client.expect_any(("Go kill ", "Go ask "), timeout=30)
            transcript_after_first = bytes(client.transcript).decode("utf-8", errors="replace")
            require("unable to help" not in transcript_after_first.lower(),
                    "bartender returned its generic failure after quest command")
            require("Unable to find" not in transcript_after_first,
                    "bartender quest path reported an internal selection failure")
            require("The Great Realm of Duris" not in transcript_after_first,
                    "explicitly denied zone was assigned")

            client.send("ask bartender abandon confirm")
            client.expect("You no longer have a task.", timeout=30)
            client.send("ask bartender quest")
            second_result, _ = client.expect_any(("Go kill ", "Go ask "), timeout=30)
            transcript_after_second = bytes(client.transcript).decode("utf-8", errors="replace")
            require("Unable to find" not in transcript_after_second,
                    "repeated bartender quest path reported an internal selection failure")
            require(output_path.read_text(errors="replace").count("World quest catalog ready:") == 1,
                    "catalog was rebuilt during repeated quest/abandon commands")
            client.send("save")
            client.expect("Save complete for Taverek.", timeout=120)
            player_state = None
            if database_environment is not None:
                player_state = verify_mariadb_player_state(database_environment)

            client.send("ask bartender abandon confirm")
            client.expect("You no longer have a task.", timeout=30)
            client.close()
            client = None
            process.send_signal(signal.SIGTERM)
            process.wait(timeout=30)
            require(process.returncode == 0,
                    f"{backend} bartender server did not stop cleanly: {output_path.read_text(errors='replace')[-12000:]}")
            final_output = output_path.read_text(errors="replace")
            require("Normal termination of game." in final_output,
                    f"{backend} server missed normal termination")
            logs = collect_logs(run_root)
            return {
                "backend": backend,
                "binary_path": str(binary_path),
                "running_executable": str(running_path),
                "binary_sha256": hashlib.sha256(binary_path.read_bytes()).hexdigest(),
                "pid": process.pid,
                "quest_first_result": first_result,
                "quest_second_result": second_result,
                "catalog_ready_count": final_output.count("World quest catalog ready:"),
                "player_state": player_state,
                "server_output_tail": final_output[-4000:],
                "log_tail": logs[-8000:],
            }
        except Exception as error:
            output = output_path.read_text(errors="replace")
            logs = collect_logs(run_root)
            raise AssertionError(
                f"{backend} bartender journey failed: {error}\n"
                f"--- server output ---\n{output[-16000:]}\n"
                f"--- runtime logs ---\n{logs}"
            ) from error
        finally:
            if client is not None:
                client.close()
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)


def verify_mariadb_zone_flag(container: str, root_password: str, database: str) -> dict:
    query = "SELECT COUNT(*), COALESCE(SUM(quest_zone <> 0), 0) FROM zones;"
    result = subprocess.run([
        "docker", "exec", "-e", "MYSQL_PWD=" + root_password, container,
        "mariadb", "-h127.0.0.1", "-uroot", "-N", "-B", database, "-e", query,
    ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    require(result.returncode == 0, "could not read disposable MariaDB zone flags")
    fields = result.stdout.strip().split()
    require(len(fields) == 2, "unexpected MariaDB zone flag result")
    return {"zone_rows": int(fields[0]), "quest_enabled_rows": int(fields[1])}


def verify_mariadb_player_state(environment: dict[str, str]) -> dict[str, int | str]:
    query = (
        "SELECT name, account_name, quest_active, quest_mob_vnum, quest_type, "
        "quest_started, quest_zone_number, quest_giver, quest_level, quest_receiver, "
        "quest_shares_left, quest_kill_how_many, quest_kill_original, quest_map_room, "
        "quest_map_bought FROM player_data "
        "WHERE name='Taverek' AND account_name='Journeyacct' LIMIT 1;"
    )
    result = subprocess.run([
        "mysql", "--protocol=tcp", "--ssl=0", "-h", environment["DB_HOST"],
        "-P", environment["DB_PORT"], "-u", environment["DB_USER"], "-N", "-B",
        environment["DB_NAME"], "-e", query,
    ], env={**environment, "MYSQL_PWD": environment["DB_PASSWD"]},
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
    require(result.returncode == 0,
            "could not read MariaDB player quest state:\n" + result.stderr[-4000:])
    fields = result.stdout.strip().split("\t")
    require(len(fields) == 15, "unexpected MariaDB player quest-state result")
    quest_active = int(fields[2])
    quest_mob_vnum = int(fields[3])
    quest_started = int(fields[5])
    quest_zone_number = int(fields[6])
    quest_giver = int(fields[7])
    quest_level = int(fields[8])
    state: dict[str, int | str] = {
        "name": fields[0],
        "account_name": fields[1],
        "quest_active": quest_active,
        "quest_mob_vnum": quest_mob_vnum,
        "quest_type": int(fields[4]),
        "quest_started": quest_started,
        "quest_zone_number": quest_zone_number,
        "quest_giver": quest_giver,
        "quest_level": quest_level,
        "quest_receiver": int(fields[9]),
        "quest_shares_left": int(fields[10]),
        "quest_kill_how_many": int(fields[11]),
        "quest_kill_original": int(fields[12]),
        "quest_map_room": int(fields[13]),
        "quest_map_bought": int(fields[14]),
    }
    detail = "raw quest fields=" + repr(fields[2:])
    require(quest_active == 1, "MariaDB player row did not persist an active quest; " + detail)
    require(quest_giver == 16553, "MariaDB player row has the wrong bartender giver; " + detail)
    require(quest_mob_vnum > 0, "MariaDB player row has no quest target; " + detail)
    require(quest_started > 0, "MariaDB player row has no quest start time; " + detail)
    require(quest_level == 56, "MariaDB player row has the wrong accepted quest level; " + detail)
    require(quest_zone_number not in {0, 292, 536},
            "MariaDB player row persisted an explicitly denied quest zone")
    return state


def run_backend(backend: str, build_root: pathlib.Path) -> dict:
    database_environment = None
    database = None
    container = None
    root_password = None
    state_tmp = tempfile.TemporaryDirectory(prefix=f"world-quest-{backend}-state-")
    state_root = pathlib.Path(state_tmp.name)
    try:
        if backend == "mariadb":
            database = "duris_world_quest_" + uuid.uuid4().hex[:10]
            container, port, root_password, db_password, database_environment = prepare_mariadb(database)
            if container is None or root_password is None or database_environment is None:
                raise AssertionError("MariaDB setup returned incomplete authority")
        binary = build_server(backend, build_root)
        result = perform_quest_journey(binary, backend, state_root, database_environment)
        if backend == "mariadb":
            if container is None or root_password is None or database is None:
                raise AssertionError("MariaDB verification authority is incomplete")
            result["database_zone_flags"] = verify_mariadb_zone_flag(container, root_password, database)
            require(result["database_zone_flags"]["quest_enabled_rows"] == 0,
                    "MariaDB fixture unexpectedly needed quest_zone=1 for successful quest")
        else:
            result["flatfile_state_root_created"] = state_root.is_dir()
        return result
    finally:
        if container:
            subprocess.run(["docker", "rm", "-f", container], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, check=False)
        state_tmp.cleanup()


def main() -> None:
    world = run(["make", "world"], timeout=600)
    require(world.returncode == 0,
            "full-world data generation failed:\n" + world.stdout.decode(errors="replace")[-12000:])
    build_parent = ROOT / "bin" / "tests"
    build_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="world-quest-build-", dir=build_parent) as build_tmp:
        build_root = pathlib.Path(build_tmp)
        backends = ("mariadb", "flatfile") if REQUESTED_BACKEND == "both" else (REQUESTED_BACKEND,)
        results = [run_backend(backend, build_root) for backend in backends]
    print(json.dumps({"status": "passed", "results": results}, indent=2))


if __name__ == "__main__":
    main()
