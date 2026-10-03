#!/usr/bin/env python3

import server_build_artifacts

import argparse
import hashlib
import json
import os
import pathlib
import signal
import socket
import stat
import subprocess
import tempfile
import time
import urllib.error
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


parser = argparse.ArgumentParser(description="Qualify client-free boot on a disposable root")
parser.add_argument("--server", type=pathlib.Path, help="use an already qualified flatfile binary")
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix="duris-flatfile-build-") as build_tmp:
    build_root = pathlib.Path(build_tmp)
    if args.server:
        binary = args.server.resolve(strict=True)
        require(binary.is_file() and os.access(binary, os.X_OK), "server must be an executable file")
        print("Supplied flatfile server sha256=" + hashlib.sha256(binary.read_bytes()).hexdigest())
    else:
        binary = server_build_artifacts.build_flatfile_server(build_root)

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
            if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
                environment["LD_LIBRARY_PATH"] = runtime_library_path
            def boot_and_shutdown(label):
                output_path = run_root / (label + ".out")
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
                        # The game-loop marker precedes listener initialization.
                        # Wait for HTTP readiness instead of racing the socket bind.
                        deadline = time.monotonic() + 30
                        while True:
                            try:
                                with urllib.request.urlopen(
                                    f"http://127.0.0.1:{websocket_port}/health", timeout=3
                                ) as response:
                                    health = json.load(response)
                                break
                            except urllib.error.URLError:
                                if process.poll() is not None or time.monotonic() >= deadline:
                                    raise
                                time.sleep(0.1)
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

            boot_and_shutdown("initial")

            allocator = state_root / "metadata/item_uid_allocator"
            marker = state_root / "metadata/item_uid_allocator.initialized"
            require(allocator.is_file() and marker.is_file(),
                    "healthy boot did not retain both UID allocator authority files")
            marker_bytes = marker.read_bytes()
            preserved_allocator = run_root / "preserved-item-uid-allocator"
            allocator.rename(preserved_allocator)
            try:
                missing_allocator = subprocess.run(
                    [str(binary), "--minimal", "-d", str(run_root), str(port)],
                    cwd=run_root, env=environment, text=True,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
                )
                require(missing_allocator.returncode == 1 and
                        "Could not reserve a collision-free flat item UID range" in missing_allocator.stdout,
                        "missing initialized allocator did not fail at boot admission:\n" +
                        missing_allocator.stdout)
                require(not allocator.exists() and marker.read_bytes() == marker_bytes,
                        "failed boot recreated allocator authority or changed initialization evidence")
            finally:
                preserved_allocator.rename(allocator)

            earlier_allocator = allocator.read_bytes()
            boot_and_shutdown("next-generation")
            current_allocator = allocator.read_bytes()
            current_marker = marker.read_bytes()
            require(current_allocator != earlier_allocator and len(current_marker) == 56 and
                    current_marker[6] == 2,
                    "second healthy boot did not advance sealed UID high-water evidence")
            allocator.write_bytes(earlier_allocator)
            try:
                stale_allocator = subprocess.run(
                    [str(binary), "--minimal", "-d", str(run_root), str(port)],
                    cwd=run_root, env=environment, text=True,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
                )
                require(stale_allocator.returncode == 1 and
                        "Could not reserve a collision-free flat item UID range" in stale_allocator.stdout,
                        "older valid allocator did not fail at boot admission:\n" +
                        stale_allocator.stdout)
                require(allocator.read_bytes() == earlier_allocator and
                        marker.read_bytes() == current_marker,
                        "failed stale-state boot changed authority or its witness")
            finally:
                allocator.write_bytes(current_allocator)
            boot_and_shutdown("restored-generation")

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
                "player-deaths",
                "economic-evidence",
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

print("client-free build, health, missing/stale UID authority refusal, exact restoration, game-loop boot and clean shutdown preflight passed")
