#!/usr/bin/env python3

"""Exercise durable player and world-item recovery across a full-world restart.

The first real server process creates an account and character, transactionally
drops an identifiable starter item, saves, and exits.  A fresh process then
boots against the same isolated authority, restores the floor item, reloads the
player with the saved terminal intent, and moves the item back to the player.
This also keeps the corpse, saved-item, and shopkeeper boot stages under a real
full-world boot rather than harnesses alone.
"""

import os
import pathlib
import shutil
import signal
import stat
import subprocess
import tempfile
import time

from test_flatfile_combat_journey import (
    CHARACTER,
    MudClient,
    available_ports,
    create_character,
    reconnect_character,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def wait_for_boot(process: subprocess.Popen[str], output, output_path: pathlib.Path) -> str:
    deadline = time.monotonic() + 600
    boot_output = ""
    while time.monotonic() < deadline:
        output.flush()
        boot_output = output_path.read_text(errors="replace")
        if "Entering game loop." in boot_output:
            break
        if process.poll() is not None:
            break
        time.sleep(0.1)
    require(
        "Entering game loop." in boot_output,
        "full-world server did not reach the game loop:\n" + boot_output,
    )
    for stage in ("-- Player corpses", "-- Shopkeepers"):
        require(stage in boot_output, f"full-world boot skipped {stage}:\n" + boot_output)
    return boot_output


def stop_server(process: subprocess.Popen[str], output, output_path: pathlib.Path) -> str:
    process.send_signal(signal.SIGTERM)
    process.wait(timeout=30)
    output.flush()
    server_output = output_path.read_text(errors="replace")
    require(
        process.returncode == 0,
        f"full-world server did not shut down cleanly ({process.returncode}):\n"
        + server_output,
    )
    require(
        "Normal termination of game." in server_output,
        "full-world shutdown did not reach normal termination:\n" + server_output,
    )
    return server_output


world = subprocess.run(
    ["make", "world"],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    timeout=600,
)
require(world.returncode == 0, "full-world data generation failed:\n" + world.stdout[-8000:])

with tempfile.TemporaryDirectory(prefix="duris-flatfile-world-build-") as build_tmp:
    build_root = pathlib.Path(build_tmp)
    binary = build_root / "server" / "dms_new"
    build = subprocess.run(
        [
            "make",
            "-C",
            "src",
            "PERSISTENCE_BACKEND=flatfile",
            f"BIN_ROOT={build_root}",
            f"OBJDIR={build_root / 'objects' / 'server'}",
            f"SERVER_BIN_DIR={binary.parent}",
            f"DMS_BINARY={binary}",
            "-j2",
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=600,
    )
    require(build.returncode == 0, "client-free server build failed:\n" + build.stdout[-8000:])
    require("-D__NO_MYSQL__" in build.stdout, "flat build did not select __NO_MYSQL__")
    require("-I/usr/include/mysql" not in build.stdout, "flat build used system MySQL headers")
    require("-lmysqlclient" not in build.stdout, "flat build linked the MySQL client")

    with tempfile.TemporaryDirectory(prefix="duris-flatfile-world-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-flatfile-world-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            os.chmod(state_root, 0o700)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            for directory in ("areas", "areas_mini", "docs"):
                (run_root / directory).symlink_to(ROOT / directory, target_is_directory=True)
            runtime_lib = run_root / "lib"
            shutil.copytree(ROOT / "lib", runtime_lib)
            properties_path = runtime_lib / "duris.properties"
            properties = properties_path.read_text()
            require("camp.timer=9.000" in properties, "camp timer fixture changed")
            properties_path.write_text(
                properties.replace("camp.timer=9.000", "camp.timer=2.000")
            )
            certificate = run_root / "duris.crt"
            private_key = run_root / "duris.key"
            generated_certificate = subprocess.run(
                [
                    "openssl", "req", "-x509", "-newkey", "rsa:2048", "-sha256",
                    "-nodes", "-days", "1", "-subj", "/CN=localhost",
                    "-keyout", str(private_key), "-out", str(certificate),
                ],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=30,
            )
            require(
                generated_certificate.returncode == 0,
                "could not generate an isolated boot certificate:\n"
                + generated_certificate.stdout,
            )
            private_key.chmod(0o600)

            journal_root = run_root / "journals"
            player_journal = journal_root / "players"
            critical_journal = journal_root / "critical"
            player_journal.mkdir(parents=True, mode=0o700)
            critical_journal.mkdir(mode=0o700)
            port, tls_port, websocket_port = available_ports()
            output_path = run_root / "boot-first.out"
            environment = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "ENVIRONMENT": "local",
                "PERSISTENCE_MODE": "flatfile-primary",
                "FLATFILE_STATE_DIR": str(state_root),
                "PLAYER_SAVE_JOURNAL_DIR": str(player_journal),
                "CRITICAL_COMMAND_JOURNAL_DIR": str(critical_journal),
                "LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_TLS_PORT": str(tls_port),
                "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_WEBSOCKET_PORT": str(websocket_port),
                "REDIS": "FALSE",
            }
            if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
                environment["LD_LIBRARY_PATH"] = runtime_library_path
            with output_path.open("w", encoding="utf-8") as output:
                process = subprocess.Popen(
                    [str(binary), "-d", str(run_root), str(port)],
                    cwd=run_root,
                    env=environment,
                    text=True,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                client = None
                try:
                    wait_for_boot(process, output, output_path)
                    client = MudClient(port)
                    create_character(client, expected_room=None)
                    client.send("drop mace")
                    client.expect("You drop a small wooden mace", timeout=20)
                    client.send("look")
                    client.expect("A gnoby piece of wood, perhaps a small mace, lies here.")
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.", timeout=15)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None
                    stop_server(process, output, output_path)
                finally:
                    if client is not None:
                        client.close()
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=5)

            port, tls_port, websocket_port = available_ports()
            environment["DURIS_TLS_PORT"] = str(tls_port)
            environment["DURIS_WEBSOCKET_PORT"] = str(websocket_port)
            output_path = run_root / "boot-restart.out"
            with output_path.open("w", encoding="utf-8") as output:
                process = subprocess.Popen(
                    [str(binary), "-d", str(run_root), str(port)],
                    cwd=run_root,
                    env=environment,
                    text=True,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                client = None
                try:
                    wait_for_boot(process, output, output_path)
                    client = reconnect_character(
                        port,
                        return_message="You break camp and get ready to move on",
                        expected_room=None,
                    )
                    client.send("look")
                    client.expect("A gnoby piece of wood, perhaps a small mace, lies here.")
                    client.send("drop all")
                    client.expect("You drop a steel long sword", timeout=45)
                    client.send("get mace")
                    client.expect("You get a small wooden mace", timeout=20)
                    client.send("inventory")
                    client.expect("a small wooden mace", timeout=10)
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.", timeout=15)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None
                    stop_server(process, output, output_path)
                finally:
                    if client is not None:
                        client.close()
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=5)

            expected_dirs = {
                "metadata",
                "identities",
                "identities/accounts",
                "identities/names",
                "players",
                "operations",
                "operations/wal",
                "domains",
                "manifests",
            }
            actual_dirs = {
                str(path.relative_to(state_root))
                for path in state_root.rglob("*")
                if path.is_dir()
            }
            require(actual_dirs == expected_dirs, f"unexpected authority topology: {actual_dirs}")
            for path in [state_root, *(state_root / name for name in expected_dirs)]:
                mode = stat.S_IMODE(path.stat().st_mode)
                require(mode == 0o700, f"insecure mode {mode:o} on {path}")

print("full-world player and floor-item process-restart journey passed")
