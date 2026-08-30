#!/usr/bin/env python3

import json
import os
import pathlib
import signal
import socket
import stat
import subprocess
import tempfile
import time
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def available_game_port() -> int:
    for _ in range(100):
        with socket.socket() as first, socket.socket() as second:
            first.bind(("127.0.0.1", 0))
            port = first.getsockname()[1]
            if port <= 1024 or port >= 65535:
                continue
            try:
                second.bind(("127.0.0.1", port + 1))
            except OSError:
                continue
            return port
    raise AssertionError("could not reserve an available game/SSL port pair")


def available_websocket_port(game_port: int) -> int:
    for _ in range(100):
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            port = listener.getsockname()[1]
            if port > 1024 and port not in (game_port, game_port + 1):
                return port
    raise AssertionError("could not reserve an available WebSocket port")


with tempfile.TemporaryDirectory(prefix="duris-flatfile-build-") as build_tmp:
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

    with tempfile.TemporaryDirectory(prefix="duris-flatfile-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-flatfile-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            os.chmod(state_root, 0o700)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            for directory in ("areas_mini", "lib"):
                (run_root / directory).symlink_to(ROOT / directory, target_is_directory=True)
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
            port = available_game_port()
            websocket_port = available_websocket_port(port)
            output_path = run_root / "boot.out"
            environment = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "ENVIRONMENT": "local",
                "PERSISTENCE_MODE": "flatfile-primary",
                "FLATFILE_STATE_DIR": str(state_root),
                "PLAYER_SAVE_JOURNAL_DIR": str(player_journal),
                "CRITICAL_COMMAND_JOURNAL_DIR": str(critical_journal),
                "LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_WEBSOCKET_PORT": str(websocket_port),
                "REDIS": "FALSE",
            }
            with output_path.open("w", encoding="utf-8") as output:
                process = subprocess.Popen(
                    [str(binary), "--minimal", "-d", str(run_root), str(port)],
                    cwd=run_root,
                    env=environment,
                    text=True,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                try:
                    deadline = time.monotonic() + 30
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
                        "client-free server did not reach the game loop:\n" + boot_output,
                    )
                    with urllib.request.urlopen(
                        f"http://127.0.0.1:{websocket_port}/health", timeout=3
                    ) as response:
                        health = json.load(response)
                    require(
                        response.status == 200
                        and health == {"status": "healthy", "persistence": "ready"},
                        f"client-free health endpoint was not ready: {health}",
                    )
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    output.flush()
                    boot_output = output_path.read_text(errors="replace")
                    require(
                        process.returncode == 0,
                        f"client-free server did not shut down cleanly ({process.returncode}):\n"
                        + boot_output,
                    )
                    require(
                        "Normal termination of game." in boot_output,
                        "client-free shutdown did not reach normal termination:\n"
                        + boot_output,
                    )
                finally:
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=5)

            # A normal install may be started before `make world` has generated
            # the full-world files.  That is a configuration error, but it must
            # remain a controlled exit: no joinable worker may turn it into a
            # misleading std::terminate/SIGABRT failure.
            with tempfile.TemporaryDirectory(prefix="duris-flatfile-missing-world-") as missing_tmp:
                missing_root = pathlib.Path(missing_tmp)
                (missing_root / "logs/log").mkdir(parents=True)
                (missing_root / "lib").symlink_to(ROOT / "lib", target_is_directory=True)
                missing_world = subprocess.run(
                    [str(binary), "-d", str(missing_root), str(available_game_port())],
                    cwd=missing_root,
                    env=environment,
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    timeout=30,
                )
                require(
                    missing_world.returncode == 1,
                    "missing full-world data did not fail with a controlled exit:\n"
                    + missing_world.stdout,
                )
                require(
                    "Trouble opening mobile file world.mob" in missing_world.stdout,
                    "missing full-world startup did not identify world.mob:\n"
                    + missing_world.stdout,
                )
                require(
                    "terminate called" not in missing_world.stdout,
                    "missing full-world startup invoked std::terminate:\n"
                    + missing_world.stdout,
                )

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

print("client-free build, health, game-loop boot, and clean shutdown preflight passed")
