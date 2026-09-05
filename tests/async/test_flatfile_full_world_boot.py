#!/usr/bin/env python3

"""Exercise durable player and world-item recovery across a full-world restart.

The first real server process creates an account and character, transactionally
drops an identifiable starter item, saves, and exits.  A fresh process then
boots against the same isolated authority, restores the floor item, reloads the
player with the saved terminal intent, and moves the item back to the player.
This also keeps the corpse, saved-item, and shopkeeper boot stages under a real
full-world boot rather than harnesses alone.

DURIS_FULL_WORLD_ARTIFACT_DIR retains synthetic authority, journals, server logs,
exit status, and a redacted monotonic command/response timeline on failure only.
Use a private, untracked destination. DURIS_FULL_WORLD_REPEATS (1..100) repeats
fresh fixture journeys with one freshly built binary. DURIS_FULL_WORLD_BINARY_CACHE
may name a binary below bin/; reuse requires matching source/environment and
binary hashes, otherwise a fresh build replaces the cache. The two DURIS_NEVENT trace
switches are forwarded explicitly; no database or live environment is inherited.
DURIS_FULL_WORLD_DELAY_CAMP=1 holds the synthetic player lock through the camp
failure deadline, then verifies online usability, automatic retry, and fresh camp.
DURIS_FULL_WORLD_CRASH_PHASE=before_ack or after_ack replaces the first clean
logout with SIGKILL and verifies journal recovery or acknowledged state on restart.
The before_ack case requires DURIS_NEVENT_TRACE_PLAYER=1 to observe submission.
"""

import os
import json
import hashlib
import fcntl
import re
from contextlib import contextmanager

import test_flatfile_combat_journey as journey
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


# Optional diagnostic controls; defaults retain the ordinary one-journey gate.
ARTIFACT_ROOT = os.environ.get("DURIS_FULL_WORLD_ARTIFACT_DIR")
CRASH_PHASE = os.environ.get("DURIS_FULL_WORLD_CRASH_PHASE", "")
if CRASH_PHASE not in ("", "before_ack", "after_ack"):
    raise ValueError("DURIS_FULL_WORLD_CRASH_PHASE must be before_ack or after_ack")
if CRASH_PHASE == "before_ack" and os.environ.get("DURIS_NEVENT_TRACE_PLAYER") != "1":
    raise ValueError("before_ack requires DURIS_NEVENT_TRACE_PLAYER=1")
REPEATS = int(os.environ.get("DURIS_FULL_WORLD_REPEATS", "1"))
if not 1 <= REPEATS <= 100:
    raise ValueError("DURIS_FULL_WORLD_REPEATS must be between 1 and 100")
records = []
phase = "build"
attempt = 0
artifact_destination = None


def record(event, **fields):
    """Append a monotonic journey event and print non-receive events immediately."""
    entry = dict(monotonic=time.monotonic(), attempt=attempt, phase=phase,
                 event=event, **fields)
    records.append(entry)
    if event != "receive":
        print(json.dumps(entry), flush=True)


class RecordedClient(MudClient):
    def send(self, line):
        """Record a password-redacted command before sending it to the test server."""
        record("send", command=line.replace(journey.PASSWORD, "[REDACTED]"))
        super().send(line)

    def _receive(self):
        """Record newly received transcript bytes with the fixture password redacted."""
        offset = len(self.transcript)
        received = super()._receive()
        if received:
            record("receive", text=self.transcript[offset:].decode(
                "utf-8", errors="replace").replace(journey.PASSWORD, "[REDACTED]"))
        return received

    def expect_any(self, needles, timeout=15):
        """Wait for expected output and classify rejection versus observation failures."""
        started = time.monotonic()
        transcript_offset = len(self.transcript)
        try:
            result = super().expect_any(needles, timeout)
        except Exception:
            response = self.transcript[transcript_offset:]
            rejection = any(message in response for message in (
                b"Save failed for", b"your camp is cancelled"))
            record("assertion_failure", expected=needles,
                   classification="server_rejection" if rejection else "observation_deadline_or_disconnect",
                   elapsed=time.monotonic() - started)
            raise
        record("observed", expected=result[0], elapsed=time.monotonic() - started)
        return result


# reconnect_character constructs its client in the helper module too.
MudClient = journey.MudClient = RecordedClient


@contextmanager
def fixture_directory(kind):
    """Yield isolated fixture storage and preserve configured diagnostics only on failure."""
    global artifact_destination
    with tempfile.TemporaryDirectory(prefix=f"duris-flatfile-world-{kind}-") as tmp:
        try:
            yield tmp
        except BaseException:
            if ARTIFACT_ROOT:
                if artifact_destination is None:
                    pathlib.Path(ARTIFACT_ROOT).mkdir(parents=True, exist_ok=True, mode=0o700)
                    artifact_destination = pathlib.Path(tempfile.mkdtemp(
                        prefix=f"attempt-{attempt}-", dir=ARTIFACT_ROOT))
                destination = artifact_destination
                source = pathlib.Path(tmp)
                if kind == "state":
                    shutil.copytree(source, destination / "authority", dirs_exist_ok=True)
                else:
                    # Only synthetic persistence and diagnostics; no TLS private key.
                    for name in ("logs", "journals", "boot-first.out", "boot-restart.out"):
                        path = source / name
                        if path.is_dir():
                            shutil.copytree(path, destination / name, dirs_exist_ok=True)
                        elif path.is_file():
                            shutil.copy2(path, destination / name)
                (destination / "timeline.json").write_text(json.dumps(records, indent=2))
                print(f"Failure artifacts: {destination}", flush=True)
            raise


def require(condition: bool, message: str) -> None:
    """Raise an assertion with the supplied diagnostic when a journey invariant fails."""
    if not condition:
        raise AssertionError(message)


def inspect_authority(state_root):
    """Read synthetic durable state and reject missing or multiply owned item identities."""
    snapshot = json.loads(subprocess.check_output(
        [str(inspector), str(state_root), "inspect", "1"], text=True, timeout=15))
    player = [item["uid"] for item in snapshot["player_items"]]
    room = [item["uid"] for item in snapshot["room_items"]]
    require(len(set(player + room)) == len(player + room), "duplicate item UID ownership")
    require(all(uid > 0 for uid in player + room), "missing durable item UID")
    record("authority_observed", **snapshot)
    return snapshot


def assert_mace_owner(state_root, uid, owner):
    """Require the tracked mace UID to belong exclusively to the expected owner."""
    snapshot = inspect_authority(state_root)
    locations = [name for name in ("player_items", "room_items")
                 for item in snapshot[name] if item["uid"] == uid]
    require(locations == [owner], f"mace UID {uid} ownership was {locations}, expected {owner}")
    return snapshot


def exercise_cancelled_camp(client, state_root, mace_uid):
    """Block storage through camp timeout, then verify usability and automatic durable retry."""
    baseline = inspect_authority(state_root)
    lock_path = state_root / "players/.player-1.lock"
    with lock_path.open("r+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        record("storage_lock_held")
        try:
            client.send("quit")
            client.expect("Your character could not be saved, so your camp is cancelled.", timeout=30)
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)
            record("storage_lock_released")
    # Character remains online and the retry must save without a manual command.
    client.send("look")
    client.expect("A gnoby piece of wood, perhaps a small mace, lies here.")
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        recovered = assert_mace_owner(state_root, mace_uid, "room_items")
        if recovered["revision"] > baseline["revision"] and recovered["intent"] == 1:
            record("deferred_retry_durable", revision=recovered["revision"])
            return
        time.sleep(0.1)
    raise AssertionError("cancelled camp did not durably retry RENT_CRASH without another save")


def change_saved_setting(client):
    """Change a persistent player setting to identify the snapshot under test."""
    client.send("toggle wimpy 5")
    client.expect("You now flee at 5 hit points or less!")


def crash_server(process):
    """Kill the isolated server and verify the intended crash termination boundary."""
    process.kill()
    process.wait(timeout=10)
    require(process.returncode == -signal.SIGKILL, "intentional crash did not terminate the server")
    record("intentional_crash", boundary=CRASH_PHASE, pid=process.pid, returncode=process.returncode)


def crash_before_ack(client, process, state_root, run_root):
    """Hold player storage until a newer save is journaled, then crash before materialization."""
    baseline = inspect_authority(state_root)
    with (state_root / "players/.player-1.lock").open("r+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        record("storage_lock_held")
        try:
            change_saved_setting(client)
            client.send("save")
            client.expect(f"Save queued for {CHARACTER}.")
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                log = (run_root / "logs/log/status").read_text(errors="replace")
                submitted = [int(revision) for revision in re.findall(
                    r"PLAYER SAVE TRACE: stage=submit .*?pid=1 revision=(\d+) outcome=0", log)]
                if submitted and max(submitted) > baseline["revision"]:
                    retained = inspect_authority(state_root)
                    require(retained["wimpy"] != 5, "changed state was already materialized before the crash")
                    record("journaled_before_authority", revision=max(submitted), authority_revision=retained["revision"])
                    crash_server(process)
                    return
                time.sleep(0.05)
            raise AssertionError("save did not reach journaled worker submission while authority was blocked")
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)
            record("storage_lock_released")


def wait_for_boot(process: subprocess.Popen[str], output, output_path: pathlib.Path) -> str:
    """Wait for the full-world game loop and verify required world restore stages ran."""
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
    record("boot_ready", pid=process.pid)
    return boot_output


def stop_server(process: subprocess.Popen[str], output, output_path: pathlib.Path) -> str:
    """Request graceful shutdown and require successful normal termination."""
    process.send_signal(signal.SIGTERM)
    process.wait(timeout=30)
    record("server_exit", pid=process.pid, returncode=process.returncode)
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


record("configuration", revision=subprocess.check_output(
    ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
    source_diff_sha256=hashlib.sha256(subprocess.check_output(
        ["git", "diff", "HEAD", "--", "src"], cwd=ROOT)).hexdigest(),
    backend="flatfile-primary", compiler=subprocess.check_output(
        ["g++", "--version"], text=True).splitlines()[0],
    cpu_affinity=sorted(os.sched_getaffinity(0)), repeats=REPEATS,
    scheduler="default: 25000 us / 4000 callbacks; catchup +5000 us / +4000 callbacks",
    analytics=os.environ.get("DURIS_NEVENT_ANALYTICS", "0"),
    player_trace=os.environ.get("DURIS_NEVENT_TRACE_PLAYER", "0"))
world = subprocess.run(
    ["make", "world"],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    timeout=600,
)
require(world.returncode == 0, "full-world data generation failed:\n" + world.stdout[-8000:])

def build_server(build_root):
    """Reuse a verified cached binary or build an isolated flatfile server."""
    cache_value = os.environ.get("DURIS_FULL_WORLD_BINARY_CACHE")
    cache = pathlib.Path(cache_value).resolve() if cache_value else None
    if cache:
        require(cache.is_relative_to((ROOT / "bin").resolve()), "binary cache must be below bin/")
        inputs = subprocess.check_output(
            ["git", "ls-files", "-co", "--exclude-standard", "-z", "--", "src"], cwd=ROOT)
        digest = hashlib.sha256()
        for name in sorted(set(inputs.split(b"\0")) - {b""}):
            digest.update(name + b"\0" + (ROOT / os.fsdecode(name)).read_bytes())
        digest.update(subprocess.check_output(["g++", "--version"]))
        # Build tools and user-provided make flags must match as well as sources.
        digest.update(json.dumps({k: v for k, v in os.environ.items() if not k.startswith("DURIS_FULL_WORLD_") and k not in ("DURIS_NEVENT_ANALYTICS", "DURIS_NEVENT_TRACE_PLAYER", "PWD", "OLDPWD", "SHLVL", "_")}, sort_keys=True).encode())
        key = digest.hexdigest()
        manifest = cache.with_suffix(cache.suffix + ".json")
        if cache.is_file() and manifest.is_file():
            metadata = json.loads(manifest.read_text())
            if metadata.get("inputs") == key and metadata.get("binary") == hashlib.sha256(cache.read_bytes()).hexdigest():
                record("verified_binary_reused")
                return cache
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

    if cache:
        cache.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, cache)
        manifest.write_text(json.dumps(dict(inputs=key, binary=hashlib.sha256(cache.read_bytes()).hexdigest())))
    return binary


with tempfile.TemporaryDirectory(prefix="full-world-build-", dir=ROOT / "bin") as build_tmp:
    binary = build_server(pathlib.Path(build_tmp))
    inspector = pathlib.Path(build_tmp) / "authority-inspector"
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(inspector)], cwd=ROOT, check=True, timeout=180)
    record("build_complete", binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
    for attempt in range(1, REPEATS + 1):
        with fixture_directory("state") as state_tmp:
            with fixture_directory("run") as run_tmp:
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
                for switch in ("DURIS_NEVENT_ANALYTICS", "DURIS_NEVENT_TRACE_PLAYER"):
                    if switch in os.environ:
                        environment[switch] = os.environ[switch]
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
                        phase = "first_boot" if output_path.name == "boot-first.out" else "second_boot"
                        record("phase_start")
                        wait_for_boot(process, output, output_path)
                        phase = "first_boot:character_creation"
                        client = MudClient(port)
                        create_character(client, expected_room=None)
                        before_drop = inspect_authority(state_root)
                        client.send("drop mace")
                        client.expect("You drop a small wooden mace", timeout=20)
                        after_drop = inspect_authority(state_root)
                        removed = {item["uid"] for item in before_drop["player_items"]} - {
                            item["uid"] for item in after_drop["player_items"]}
                        require(len(removed) == 1, f"drop changed unexpected UIDs: {removed}")
                        mace_uid = removed.pop()
                        assert_mace_owner(state_root, mace_uid, "room_items")
                        client.send("look")
                        client.expect("A gnoby piece of wood, perhaps a small mace, lies here.")
                        phase = output_path.stem + ":manual_save"
                        if CRASH_PHASE == "before_ack":
                            crash_before_ack(client, process, state_root, run_root)
                        else:
                            if CRASH_PHASE == "after_ack":
                                change_saved_setting(client)
                            client.send("save")
                            client.expect(f"Save complete for {CHARACTER}.", timeout=45)
                            if CRASH_PHASE == "after_ack":
                                crash_server(process)
                                require(inspect_authority(state_root)["wimpy"] == 5, "acknowledged setting was not durable")
                        if not CRASH_PHASE:
                            phase = output_path.stem + ":terminal_save"
                            if os.environ.get("DURIS_FULL_WORLD_DELAY_CAMP") == "1":
                                exercise_cancelled_camp(client, state_root, mace_uid)
                            client.send("quit")
                            client.expect("ACCOUNT MENU", timeout=30)
                            client.send("0")
                            client.close()
                            client = None
                            phase = output_path.stem + ":shutdown"
                            stop_server(process, output, output_path)
                            require(inspect_authority(state_root)["intent"] == 6, "successful camp did not persist RENT_CAMPED")
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
                        record("process_cleanup", pid=process.pid, returncode=process.returncode)

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
                        phase = "first_boot" if output_path.name == "boot-first.out" else "second_boot"
                        record("phase_start")
                        wait_for_boot(process, output, output_path)
                        recovered = assert_mace_owner(state_root, mace_uid, "room_items")
                        if CRASH_PHASE:
                            require(recovered["wimpy"] == 5, "saved setting did not recover across intentional crash")
                        phase = "restart_recovery"
                        client = reconnect_character(
                            port,
                            return_message=("Restoring items and pets from crash save info..." if CRASH_PHASE else
                                            "You break camp and get ready to move on"),
                            expected_room=None,
                        )
                        client.send("look")
                        client.expect("A gnoby piece of wood, perhaps a small mace, lies here.")
                        client.send("drop all")
                        client.expect("You drop a steel long sword", timeout=45)
                        client.send("get mace")
                        client.expect("You get a small wooden mace", timeout=20)
                        assert_mace_owner(state_root, mace_uid, "player_items")
                        client.send("inventory")
                        client.expect("a small wooden mace", timeout=10)
                        phase = output_path.stem + ":manual_save"
                        client.send("save")
                        client.expect(f"Save complete for {CHARACTER}.", timeout=45)
                        phase = output_path.stem + ":terminal_save"
                        client.send("quit")
                        client.expect("ACCOUNT MENU", timeout=30)
                        client.send("0")
                        client.close()
                        client = None
                        phase = output_path.stem + ":shutdown"
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
                        record("process_cleanup", pid=process.pid, returncode=process.returncode)

                assert_mace_owner(state_root, mace_uid, "player_items")
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
