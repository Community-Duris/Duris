"""Disposable MariaDB + immutable-server fixture for spell item creation."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import secrets
import socket
import subprocess
import tempfile
import time
import uuid
from contextlib import contextmanager
from typing import Any

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
MARIADB_IMAGE = "mariadb:10.11"
WIND_BLADE_VNUM = 98


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def _redact_text(text: str, extra_values: tuple[str, ...] = ()) -> str:
    values = [
        getattr(journey, name, "")
        for name in ("PASSWORD", "ACCOUNT", "CHARACTER", "EMAIL")
    ]
    values.extend(extra_values)
    for value in sorted({value for value in values if value}, key=len, reverse=True):
        text = text.replace(value, "[REDACTED]")
    return text


def _clean_env(**overrides: str) -> dict[str, str]:
    """Build a small explicit process environment; never import .env values."""
    env = {
        "PATH": os.environ.get("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"),
        "HOME": os.environ.get("HOME", "/tmp"),
        "LANG": "C.UTF-8",
        "LC_ALL": "C.UTF-8",
    }
    if "TMPDIR" in os.environ:
        env["TMPDIR"] = os.environ["TMPDIR"]
    env.update(overrides)
    return env


def _run(command: list[str], *, env: dict[str, str] | None = None,
         cwd: Path | None = None, timeout: int = 120, input_text: str | None = None,
         check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout,
        check=False,
    )
    if check and result.returncode != 0:
        secrets_from_env = tuple(
            value for key, value in (env or {}).items()
            if any(marker in key.upper() for marker in ("PASS", "PWD", "SECRET", "TOKEN"))
        )
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout: {_redact_text(result.stdout[-3000:], secrets_from_env)}\n"
            f"stderr: {_redact_text(result.stderr[-3000:], secrets_from_env)}"
        )
    return result


def verify_source_manifest(manifest_path: Path) -> str:
    require(manifest_path.is_absolute() and manifest_path.is_file() and
            not manifest_path.is_symlink(),
            "parent source manifest must be an absolute regular non-symlink file")
    manifest_path = manifest_path.resolve(strict=True)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    require(isinstance(manifest, dict) and bool(manifest),
            "parent source manifest must be a non-empty path-to-SHA-256 object")
    listed = _run(
        ["git", "-C", str(ROOT), "ls-files", "src", "migrations", "Makefile"],
        cwd=ROOT, env=_clean_env(), timeout=15,
    ).stdout.splitlines()
    require(set(manifest) == set(listed),
            "parent source manifest path set differs from tracked build inputs")
    root = ROOT.resolve(strict=True)
    for name, expected in manifest.items():
        relative = Path(name)
        require(not relative.is_absolute() and ".." not in relative.parts,
                "parent source manifest contains an unsafe path")
        candidate = (root / relative).resolve(strict=True)
        require(candidate.is_relative_to(root),
                "parent source manifest path escapes the checked-out source root")
        require(isinstance(expected, str) and len(expected) == 64 and
                all(char in "0123456789abcdef" for char in expected),
                f"parent source manifest has an invalid digest for {name}")
        actual = hashlib.sha256(candidate.read_bytes()).hexdigest()
        require(actual == expected,
                f"checked-out source differs from parent manifest: {name}")
    return hashlib.sha256(manifest_path.read_bytes()).hexdigest()


def verify_immutable_binary(binary: Path, base_build: Path, expected_head: str,
                            expected_binary_sha256: str) -> tuple[str, str]:
    require(len(expected_head) == 40 and
            all(char in "0123456789abcdef" for char in expected_head),
            "expected parent source HEAD must be a full lowercase Git SHA-1")
    require(len(expected_binary_sha256) == 64 and
            all(char in "0123456789abcdef" for char in expected_binary_sha256),
            "expected parent binary digest must be a full lowercase SHA-256")
    require(binary.is_absolute() and binary.is_file() and not binary.is_symlink() and
            os.access(binary, os.X_OK) and not (binary.stat().st_mode & 0o022),
            "parent DB binary must be an absolute executable, read-only regular file")
    require(base_build.is_absolute() and base_build.is_file() and
            not base_build.is_symlink(),
            "base-build descriptor must be an absolute regular non-symlink file")
    binary = binary.resolve(strict=True)
    build = json.loads(base_build.resolve(strict=True).read_text(encoding="utf-8"))
    require(build.get("status", "").endswith("_PASS"),
            "parent build record is not a passing frozen build")
    checks = build.get("checks")
    require(isinstance(checks, list) and bool(checks) and all(
        isinstance(item, dict) and type(item.get("exit")) is int and item["exit"] == 0
        for item in checks
    ), "parent build record contains missing or failed checks")
    require(build.get("head") == expected_head,
            "parent build record does not match the expected source HEAD")
    current_head = _run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        cwd=ROOT, env=_clean_env(), timeout=15,
    ).stdout.strip()
    require(current_head == expected_head,
            "checked-out source HEAD differs from the parent build record")
    dirty = _run(
        ["git", "-C", str(ROOT), "status", "--porcelain"],
        cwd=ROOT, env=_clean_env(), timeout=15,
    ).stdout.strip()
    require(not dirty, "checked-out worktree is dirty; refusing stale source provenance")
    require(build.get("backend") == "mariadb" and
            build.get("profile") == "development/TEST_MUD",
            "parent build record is not the expected disposable MariaDB test binary")
    recorded_binary = Path(build["binary"])
    require(recorded_binary.is_absolute() and recorded_binary.is_file() and
            not recorded_binary.is_symlink() and os.access(recorded_binary, os.X_OK) and
            not (recorded_binary.stat().st_mode & 0o022),
            "recorded parent DB binary must be an absolute executable, read-only regular file")
    require(recorded_binary.resolve(strict=True) == binary,
            "binary path differs from the immutable parent build record")
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    require(digest == expected_binary_sha256 and digest == build.get("binary_sha256"),
            "immutable parent DB binary SHA-256 mismatch")
    manifest_sha256 = verify_source_manifest(Path(build["source_manifest"]))
    return digest, manifest_sha256


class DisposableMariaDB:
    """Task-owned, loopback-published MariaDB container with explicit cleanup."""

    def __init__(self) -> None:
        self.run_id = uuid.uuid4().hex[:12]
        self.container = f"duris-s04-item-create-{self.run_id}"
        self.database = f"s04_create_{self.run_id}"
        self.user = f"s04_{self.run_id[:8]}"
        self.root_password = secrets.token_urlsafe(32)
        self.password = secrets.token_urlsafe(32)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            probe.bind(("127.0.0.1", 0))
            self.port = probe.getsockname()[1]
        self.started = False
        self.cleanup_verified = False
        self.start_attempted = False
        self.process_env = _clean_env(
            MARIADB_ROOT_PASSWORD=self.root_password,
            MARIADB_ROOT_HOST="%",
            MARIADB_DATABASE=self.database,
            MARIADB_USER=self.user,
            MARIADB_PASSWORD=self.password,
        )

    def start(self) -> None:
        network_mode = _run(
            ["docker", "inspect", "--format", "{{.HostConfig.NetworkMode}}", "hermes"],
            env=_clean_env(), timeout=30,
        ).stdout.strip()
        require(network_mode.startswith("container:"),
                f"worker gateway does not use a shared container network namespace: {network_mode!r}")
        command = [
            "docker", "run", "--detach",
            "--name", self.container,
            "--label", "duris.task=s04-item-creation",
            "--label", f"duris.run_id={self.run_id}",
            "--cpus=2", "--memory=2g",
            "--network", network_mode,
            "--env", "MARIADB_ROOT_PASSWORD",
            "--env", "MARIADB_ROOT_HOST",
            "--env", "MARIADB_DATABASE",
            "--env", "MARIADB_USER",
            "--env", "MARIADB_PASSWORD",
            MARIADB_IMAGE,
            f"--port={self.port}", "--bind-address=127.0.0.1",
        ]
        self.start_attempted = True
        _run(command, env=self.process_env, timeout=90)
        self.started = True
        deadline = time.monotonic() + 120
        last_result: subprocess.CompletedProcess[str] | None = None
        while time.monotonic() < deadline:
            last_result = _run(
                self.mysql_command("SELECT 1"), env=self.db_env(), timeout=10, check=False
            )
            if last_result.returncode == 0 and last_result.stdout.strip() == "1":
                return
            time.sleep(0.5)
        logs = _run(["docker", "logs", self.container], env=_clean_env(),
                    timeout=20, check=False)
        probe = "no MySQL probe result"
        if last_result:
            probe = (f"MySQL probe exit={last_result.returncode} "
                     f"stdout={last_result.stdout[-1000:]} "
                     f"stderr={last_result.stderr[-1000:]}")
        raise RuntimeError(_redact(
            "task MariaDB did not become ready; " + probe + "\\ncontainer log tail:\\n" +
            (logs.stdout + logs.stderr)[-3000:], self
        ))

    def db_env(self) -> dict[str, str]:
        require(self.port is not None, "MariaDB port is not assigned")
        return _clean_env(MYSQL_PWD=self.password)

    def mysql_command(self, statement: str | None = None) -> list[str]:
        require(self.port is not None, "MariaDB port is not assigned")
        command = [
            "mysql", "--no-defaults", "--protocol=tcp",
            "--host=127.0.0.1", f"--port={self.port}",
            f"--user={self.user}", "--batch", "--skip-column-names",
            "--raw", self.database,
        ]
        if statement is not None:
            command.extend(["--execute", statement])
        return command

    def sql(self, statement: str, *, input_text: str | None = None) -> str:
        result = _run(self.mysql_command(statement if input_text is None else None),
                      env=self.db_env(), timeout=120, input_text=input_text)
        return result.stdout.strip()

    def initialize(self) -> None:
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text(
            encoding="utf-8"
        )
        self.sql("", input_text=bootstrap)
        migration_env = self.runtime_env(None, None, None)
        for arguments in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            _run(
                ["python3", "scripts/migration_runner.py", *arguments],
                cwd=ROOT,
                env=migration_env,
                timeout=240,
            )

    def runtime_env(self, game: Path | None, ports: tuple[int, int, int] | None,
                    journals: tuple[Path, Path] | None) -> dict[str, str]:
        require(self.port is not None, "MariaDB port is not assigned")
        env = _clean_env(
            ENVIRONMENT="local",
            DB_HOST="127.0.0.1",
            DB_PORT=str(self.port),
            DB_NAME=self.database,
            DB_USER=self.user,
            DB_PASSWD=self.password,
            MYSQL_PWD=self.password,
            DB_ALLOWED_TARGETS=f"127.0.0.1/{self.database}",
            PERSISTENCE_MODE="mariadb-primary",
            DB_TLS="FALSE",
            REDIS="FALSE",
            CHAOS_MUD="FALSE",
            LISTEN_ADDRESS="127.0.0.1",
            DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1",
        )
        if ports:
            env.update({
                "DURIS_TLS_PORT": str(ports[1]),
                "DURIS_WEBSOCKET_PORT": str(ports[2]),
            })
        if journals:
            env.update({
                "PLAYER_SAVE_JOURNAL_DIR": str(journals[0]),
                "CRITICAL_COMMAND_JOURNAL_DIR": str(journals[1]),
            })
        return env

    def cleanup(self) -> None:
        if not self.start_attempted:
            self.cleanup_verified = True
            return
        inspect = ["docker", "inspect", "--format",
                   "{{index .Config.Labels \"duris.task\"}}|{{index .Config.Labels \"duris.run_id\"}}",
                   self.container]
        inspection = _run(inspect, env=_clean_env(), timeout=30, check=False)
        if inspection.returncode != 0:
            if "no such object" in inspection.stderr.lower():
                self.cleanup_verified = True
                self.started = False
                return
            raise RuntimeError("could not inspect task MariaDB container: " +
                               _redact_text(inspection.stderr[-2000:]))
        require(inspection.stdout.strip() == f"s04-item-creation|{self.run_id}",
                "refusing cleanup because the named container lacks this fixture's ownership labels")
        _run(["docker", "rm", "--force", self.container], env=_clean_env(),
             timeout=60, check=False)
        after = _run(inspect, env=_clean_env(), timeout=30, check=False)
        self.cleanup_verified = (after.returncode != 0 and
                                 "no such object" in after.stderr.lower())
        require(self.cleanup_verified,
                "task MariaDB container still exists or absence was not verifiable")
        self.started = False


def _redact(text: str, db: DisposableMariaDB) -> str:
    return _redact_text(text, (db.root_password, db.password))


def _preserve_failure_artifacts(game: Path, evidence_path: Path, db: DisposableMariaDB,
                                state: dict[str, Any], error: BaseException) -> dict[str, str]:
    run_tag = f"{time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())}-{uuid.uuid4().hex[:8]}"
    artifact_dir = evidence_path.parent / f"{evidence_path.stem}-failure-{run_tag}"
    artifact_dir.mkdir(parents=True, exist_ok=False)

    client = state.get("client")
    transcript = ""
    if client is not None:
        transcript = bytes(client.transcript[-20000:]).decode("utf-8", errors="replace")
    transcript_path = artifact_dir / "socket-transcript.txt"
    transcript_path.write_text(_redact(transcript, db), encoding="utf-8")

    outputs: list[Path] = []
    for candidate in game.rglob("*"):
        if not candidate.is_file() or candidate.is_symlink():
            continue
        relative = candidate.relative_to(game)
        if relative.parts[0] == "logs" or (len(relative.parts) == 1 and
                                             candidate.name.startswith("server-") and
                                             candidate.name.endswith(".out")):
            outputs.append(candidate)
    outputs.sort(key=lambda path: (path.stat().st_mtime_ns, str(path.relative_to(game))))
    ordered: list[str] = []
    budget = 160_000
    for path in outputs:
        if budget <= 0:
            break
        relative = str(path.relative_to(game))
        text = _redact(path.read_text(errors="replace")[-24000:], db)
        block = f"===== {relative} (mtime_ns={path.stat().st_mtime_ns}) =====\n{text}\n"
        block = block[-budget:]
        ordered.append(block)
        budget -= len(block)
    ordered_path = artifact_dir / "ordered-wizlog-server-output.txt"
    ordered_path.write_text("".join(ordered), encoding="utf-8")

    failure = {
        "phase": state.get("phase", "unknown"),
        "exception_type": type(error).__name__,
        "message": _redact(str(error), db),
        "socket_transcript": transcript_path.name,
        "ordered_output": ordered_path.name,
        "output_files": [str(path.relative_to(game)) for path in outputs],
    }
    (artifact_dir / "failure.json").write_text(json.dumps(failure, indent=2) + "\n",
                                                encoding="utf-8")
    return {"directory": str(artifact_dir), "summary": str(artifact_dir / "failure.json"),
            "socket_transcript": str(transcript_path), "ordered_output": str(ordered_path)}


def _reconnect_character(port: int, state: dict[str, Any],
                         expected_room: str | None = None) -> journey.MudClient:
    client = journey.MudClient(port)
    state["client"] = client
    try:
        entry, _ = client.expect_any(("term type", "account name"))
        if entry == "term type":
            client.send("9")
            client.expect("account name")
        client.send(journey.ACCOUNT)
        client.expect("enter your password")
        client.send(journey.PASSWORD)
        client.expect("PRESS RETURN")
        client.send("")
        client.expect("Please select an option")
        client.send("1")
        client.expect(journey.CHARACTER)
        client.send("1")
        client.expect("Play as")
        client.send("y")
        state["phase"] = "character_load_completion"
        client.expect(expected_room or "Pos: standing >", timeout=45)
        return client
    except BaseException:
        client.close()
        raise


@contextmanager
def _diagnostic_temporary_directory(evidence_path: Path, db: DisposableMariaDB,
                                    evidence: dict[str, Any], state: dict[str, Any]):
    with tempfile.TemporaryDirectory(prefix="duris-s04-wind-blade-") as temporary:
        game = Path(temporary)
        try:
            yield game
        except BaseException as error:
            evidence["failure_artifacts"] = _preserve_failure_artifacts(
                game, evidence_path, db, state, error)
            raise


def _wait_for_server(process: subprocess.Popen[bytes], output_path: Path,
                     db: DisposableMariaDB) -> None:
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline and process.poll() is None:
        if "Entering game loop." in output_path.read_text(errors="replace"):
            return
        time.sleep(0.1)
    tail = _redact(output_path.read_text(errors="replace")[-5000:], db)
    raise AssertionError("server did not enter game loop:\n" + tail)


def _start_server(binary: Path, game: Path, db: DisposableMariaDB,
                  output_path: Path) -> tuple[subprocess.Popen[bytes], int]:
    ports = journey.available_ports()
    env = db.runtime_env(
        game, ports,
        (game / "journals/players", game / "journals/critical"),
    )
    process: subprocess.Popen[bytes] | None = None
    try:
        with output_path.open("wb") as output:
            process = subprocess.Popen(
                [str(binary), "--minimal", "-s", "-d", str(game), str(ports[0])],
                cwd=game,
                env=env,
                stdout=output,
                stderr=subprocess.STDOUT,
            )
        _wait_for_server(process, output_path, db)
        return process, ports[0]
    except BaseException as error:
        if process is not None:
            try:
                _stop_server(process, require_zero=False)
            except BaseException as cleanup_error:
                error.add_note("server process cleanup failed: " + _redact(str(cleanup_error), db))
        raise


def _stop_server(process: subprocess.Popen[bytes], *, require_zero: bool = True) -> None:
    if process.poll() is None:
        process.terminate()
    try:
        result = process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        process.kill()
        result = process.wait(timeout=10)
    if require_zero:
        require(result == 0, f"test server exited with status {result}")


def _query_blade(db: DisposableMariaDB, pid: int) -> dict[str, int]:
    rows = db.sql(
        "SELECT obj_uid,equip_slot,timer FROM player_items "
        f"WHERE pid={pid} AND vnum={WIND_BLADE_VNUM}"
    )
    parsed = []
    for row in rows.splitlines():
        if row:
            uid, equip_slot, timer = (int(value) for value in row.split("\t"))
            parsed.append({"uid": uid, "equip_slot": equip_slot, "timer": timer})
    require(len(parsed) == 1, f"expected one saved Wind Blade item row, found {len(parsed)}")
    return parsed[0]


def _assert_current_custody(db: DisposableMariaDB, pid: int, uid: int) -> None:
    current = db.sql(
        "SELECT COUNT(*) FROM item_current_owner "
        f"WHERE item_uid={uid} AND root_item_uid={uid} AND parent_item_uid IS NULL "
        f"AND owner_type=1 AND owner_id={pid} AND owner_context_id=0 "
        "AND state=1 AND vnum=98"
    )
    require(current == "1", f"Wind Blade SQL custody row is not active and player-bound: {current}")
    ledger = db.sql(
        "SELECT COUNT(*) FROM item_ownership_ledger "
        f"WHERE item_uid={uid} AND root_item_uid={uid} AND to_owner_type=1 "
        f"AND to_owner_id={pid} AND to_owner_context_id=0"
    )
    require(ledger == "1", f"expected one creation custody event for the blade UID, found {ledger}")


def _restore_wind_blade_cast_slot(client: journey.MudClient,
                                  state: dict[str, Any]) -> None:
    """Recover a real Ethermancer circle-3 slot through the normal tupor path."""
    state["phase"] = "wind_blade_slot_recovery"
    client.pending.clear()
    client.send("sleep")
    client.expect("You fall asleep.", timeout=15)
    # The normal prompt reports posture (GET_POS), not sleep status (GET_STAT);
    # sleeping while standing therefore still displays "Pos: standing".
    client.pending.clear()
    client.send("tupor")
    client.expect("Your mind drifts into a deep meditation", timeout=30)
    client.expect("restoring your 3rd circle powers!", timeout=180)
    client.pending.clear()
    client.send("wake")
    client.expect("You wake up.", timeout=15)
    client.pending.clear()
    client.send("stand")
    client.expect("Pos: standing >", timeout=15)


def run_wind_blade_sql_journey(binary: Path, base_build: Path, expected_head: str,
                               expected_binary_sha256: str,
                               evidence_path: Path) -> dict[str, Any]:
    digest, source_manifest_sha256 = verify_immutable_binary(
        binary, base_build, expected_head, expected_binary_sha256
    )
    db = DisposableMariaDB()
    evidence: dict[str, Any] = {
        "base": expected_head,
        "binary_sha256": digest,
        "source_manifest_sha256": source_manifest_sha256,
        "image": MARIADB_IMAGE,
        "phases": [],
    }
    state: dict[str, Any] = {"phase": "database_container_start", "client": None}
    run_error: BaseException | None = None
    try:
        db.start()
        state["phase"] = "fresh_schema_setup"
        db.initialize()
        evidence["phases"].append("fresh_schema_bootstrapped_and_migrated")
        state["phase"] = "fixture_setup"
        with _diagnostic_temporary_directory(evidence_path, db, evidence, state) as game:
            (game / "logs/log").mkdir(parents=True)
            journey.make_fixture(game)
            journey.generate_certificate(game)
            (game / "journals/players").mkdir(parents=True, mode=0o700)
            (game / "journals/critical").mkdir(parents=True, mode=0o700)
            boot_number = 0

            def start_server() -> tuple[subprocess.Popen[bytes], int]:
                nonlocal boot_number
                boot_number += 1
                return _start_server(binary, game, db, game / f"server-{boot_number:02d}.out")

            state["phase"] = "initial_server_start"
            process, port = start_server()
            client = None
            try:
                client = journey.MudClient(port)
                state["client"] = client
                state["phase"] = "character_creation"
                journey.create_character(client, expected_room=None, class_name="g", hometown="p")
                state["phase"] = "initial_character_save"
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.send("0")
                client.close()
                client = None
                state["client"] = None
                _stop_server(process)
                evidence["phases"].append("ethermancer_character_saved_to_fresh_sql_database")

                pid_text = db.sql(
                    "SELECT pid FROM player_data WHERE name='" + journey.CHARACTER + "'"
                )
                require(pid_text.isdigit() and int(pid_text) > 0,
                        "saved test character has no SQL player identity")
                pid = int(pid_text)
                # Set only the disposable actor's level to the first point where
                # Ethermancers reach Wind Blade's circle-3 spell slot. Spell slots
                # and item/accounting authority are recovered through gameplay.
                db.sql(
                    "UPDATE player_data SET level=11,highest_level=11,exp=0 "
                    f"WHERE pid={pid}"
                )
                before = db.sql(
                    "SELECT COUNT(*) FROM item_current_owner "
                    f"WHERE owner_type=1 AND owner_id={pid} AND vnum={WIND_BLADE_VNUM} "
                    "AND state=1"
                )
                require(before == "0", "fresh test actor unexpectedly owns a Wind Blade")
                state["phase"] = "wind_blade_server_start"
                process, port = start_server()
                client = _reconnect_character(port, state, expected_room=None)
                _restore_wind_blade_cast_slot(client, state)
                state["phase"] = "wind_blade_creation"
                client.pending.clear()
                client.send("cast 'wind blade'")
                outcome, transcript = client.expect_any(
                    (
                        "Swirling air solidifies into a slender sword.",
                        "You do not know that spell",
                        "You don't have that spell memorized.",
                        "Your lucidity is not sufficient",
                        "The winds could not create the blade right now.",
                        "The winds could not deliver the blade",
                        "The ownership authority",
                    ),
                    timeout=45,
                )
                require(outcome.startswith("Swirling air"),
                        "Wind Blade creation did not publish successfully: " + transcript[-2500:])
                evidence["phases"].append("wind_blade_creation_committed_and_published")

                # A second cast is a genuine gameplay retry after commit. It must
                # reuse the carried/equipped blade instead of minting a second UID.
                _restore_wind_blade_cast_slot(client, state)
                state["phase"] = "post_commit_retry"
                client.pending.clear()
                client.send("cast 'wind blade'")
                retry_outcome, retry_transcript = client.expect_any(
                    (
                        "The winds are already aiding you!",
                        "The winds are still resolving another item movement.",
                        "Swirling air solidifies into a slender sword.",
                        "Your lucidity is not sufficient",
                    ),
                    timeout=30,
                )
                require(not retry_outcome.startswith("Swirling air"),
                        "retry created a duplicate Wind Blade: " + retry_transcript[-2000:])
                require(not retry_outcome.startswith("Your lucidity"),
                        "retry was refused because the test actor had no lucidity")
                evidence["phases"].append("post_commit_retry_did_not_create_a_second_blade")

                state["phase"] = "wind_blade_save"
                client.pending.clear()
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                saved = _query_blade(db, pid)
                require(saved["timer"] > 0 and saved["timer"] <= 180,
                        "saved Wind Blade timer did not preserve its transient expiry")
                _assert_current_custody(db, pid, saved["uid"])
                evidence["item"] = {
                    "vnum": WIND_BLADE_VNUM,
                    "uid": saved["uid"],
                    "equip_slot": saved["equip_slot"],
                    "remaining_timer": saved["timer"],
                    "active_custody_rows": 1,
                    "creation_ledger_events": 1,
                }
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.send("0")
                client.close()
                client = None
                state["client"] = None
                _stop_server(process)
                evidence["phases"].append("saved_transient_blade_before_expiry")

                state["phase"] = "reconnect_server_start"
                process, port = start_server()
                client = _reconnect_character(port, state, expected_room=None)
                state["phase"] = "reconnect_visibility"
                client.pending.clear()
                client.send("inventory")
                inventory = client.expect("Pos: standing >", timeout=20)
                client.pending.clear()
                client.send("equipment")
                equipment = client.expect("Pos: standing >", timeout=20)
                require("a slender sword of vapor" in (inventory + equipment).lower(),
                        "reconnected character could not see the still-unexpired Wind Blade")
                restored = _query_blade(db, pid)
                require(restored["uid"] == saved["uid"],
                        "reconnect changed the durable Wind Blade UID")
                require(restored["timer"] > 0 and restored["timer"] <= saved["timer"],
                        "reconnect did not preserve the remaining transient lifetime")
                _assert_current_custody(db, pid, restored["uid"])

                _restore_wind_blade_cast_slot(client, state)
                state["phase"] = "reconnect_retry"
                client.pending.clear()
                client.send("cast 'wind blade'")
                retry_after_reconnect, retry_after_reconnect_text = client.expect_any(
                    (
                        "The winds are already aiding you!",
                        "The winds are still resolving another item movement.",
                        "Swirling air solidifies into a slender sword.",
                        "Your lucidity is not sufficient",
                    ),
                    timeout=30,
                )
                require(not retry_after_reconnect.startswith("Swirling air"),
                        "reconnect retry created a second Wind Blade: " +
                        retry_after_reconnect_text[-2000:])
                require(not retry_after_reconnect.startswith("Your lucidity"),
                        "reconnect retry was refused because the test actor had no lucidity")
                state["phase"] = "reconnect_save"
                client.pending.clear()
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                final = _query_blade(db, pid)
                require(final["uid"] == saved["uid"],
                        "retry/reconnect changed the durable Wind Blade identity")
                _assert_current_custody(db, pid, final["uid"])
                evidence["phases"].append("reconnect_restored_same_blade_and_retry_was_idempotent")
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.send("0")
                client.close()
                client = None
                state["client"] = None
                _stop_server(process)
            finally:
                if client:
                    client.close()
                if process.poll() is None:
                    _stop_server(process, require_zero=False)
        evidence["result"] = "passed"
    except BaseException as error:
        run_error = error
        evidence["result"] = "failed"
        evidence["failure"] = {
            "phase": state["phase"],
            "exception_type": type(error).__name__,
            "message": _redact(str(error), db),
        }
        raise
    finally:
        try:
            db.cleanup()
        except BaseException as error:
            evidence["cleanup"] = "unverified"
            evidence["cleanup_error"] = _redact(str(error), db)
            if run_error is None:
                run_error = error
        else:
            evidence["cleanup"] = "task MariaDB container absent after cleanup"
            evidence["cleanup_verified"] = db.cleanup_verified
        evidence_path.parent.mkdir(parents=True, exist_ok=True)
        evidence_path.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
        if run_error is not None and evidence.get("result") == "passed":
            raise RuntimeError("fixture cleanup did not verify; see " + str(evidence_path)) from run_error
    return evidence
