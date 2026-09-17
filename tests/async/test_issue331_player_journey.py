#!/usr/bin/env python3
"""Issue331 player acceptance journey on a disposable SQL game runtime.

This is deliberately a new acceptance harness.  It never edits or starts a
shared runtime: the shell runner gives it a fresh network, database, and game
container.  The test stops the game before guarded recovery and verifies the
result through both player commands and SQL readback.
"""
from __future__ import annotations

import json
import os
import re
import shutil
import socket
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parents[2]
PLAN_DIR = Path(os.environ.get(
    "ISSUE331_PLAN_DIR",
    "/opt/data/workspaces/.hermes/plans/duris-issue-331-restitution",
))
DB_CONTAINER = os.environ["ISSUE331_DB_CONTAINER"]
RUNTIME_CONTAINER = os.environ["ISSUE331_RUNTIME_CONTAINER"]
GAME_PORT = int(os.environ["ISSUE331_GAME_PORT"])
GAME_HOST = os.environ["ISSUE331_GAME_HOST"]
DB_NAME = os.environ["DB_NAME"]
DB_PASSWORD = os.environ["DB_PASSWORD"]
ARTIFACT = Path(os.environ.get(
    "ISSUE331_ARTIFACT",
    str(PLAN_DIR / "artifacts" / "dms_new"),
))

PROXY_PORT = 13306
RECOVERED_UIDS = (51000, 51001, 51002, 51003)
ARTIFACT_VNUM = 67259
OPERATION = "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"

class JourneyBlocked(RuntimeError):
    pass

class HarnessError(RuntimeError):
    pass


def run(command: Sequence[str], *, input_text: str | None = None,
        env: dict[str, str] | None = None, timeout: int = 120,
        check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        list(command), input=input_text, text=True, capture_output=True,
        env=env, timeout=timeout,
    )
    if check and result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise HarnessError(f"command failed ({result.returncode}): {' '.join(command[:4])}: {detail[-1200:]}")
    return result


def docker(*args: str, input_text: str | None = None, timeout: int = 120,
           check: bool = True) -> subprocess.CompletedProcess[str]:
    return run(["docker", *args], input_text=input_text, timeout=timeout, check=check)


def write_mysql_wrapper(directory: Path) -> Path:
    """Make a mysql-compatible executable which enters only our DB container."""
    wrapper = directory / "mysql"
    wrapper.write_text(
        "#!/usr/bin/env python3\n"
        "import os, subprocess, sys\n"
        f"container = {DB_CONTAINER!r}\n"
        "args = sys.argv[1:]\n"
        "filtered = []\n"
        "skip = False\n"
        "for arg in args:\n"
        "    if skip:\n"
        "        skip = False\n"
        "        continue\n"
        "    if arg in ('-h', '--host', '-P', '--port', '-u', '--user', '--protocol', '--connect-timeout', '--ssl-ca', '--ssl-cert', '--ssl-key', '--ssl-mode'):\n"
        "        skip = True\n"
        "        continue\n"
        "    if arg.startswith(('--host=', '--port=', '--protocol=', '--connect-timeout=', '--ssl-ca=', '--ssl-cert=', '--ssl-key=', '--ssl-mode=')):\n"
        "        continue\n"
        "    if arg == '-u' or arg == '--user' or arg.startswith('-u'):\n"
        "        continue\n"
        "    if arg.startswith('-h') or arg.startswith('-P'):\n"
        "        continue\n"
        "    filtered.append(arg)\n"
        "cmd = ['docker', 'exec', '-i', '-e', 'MYSQL_PWD=' + os.environ.get('MYSQL_PWD', ''), container, 'mysql', '--protocol=tcp', '--host=127.0.0.1', '--port=3306', '-uroot'] + filtered\n"
        "raise SystemExit(subprocess.call(cmd))\n",
        encoding="utf-8",
    )
    wrapper.chmod(stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
    return wrapper


def mysql_env(wrapper: Path) -> dict[str, str]:
    env = os.environ.copy()
    env.update({
        "MYSQL_PWD": DB_PASSWORD,
        "DB_HOST": "127.0.0.1",
        "DB_PORT": "3306",
        "DB_NAME": DB_NAME,
        "DB_USER": "root",
        "DB_PASSWD": DB_PASSWORD,
        "ENVIRONMENT": "local",
        "PERSISTENCE_BACKEND": "mariadb",
        "MYSQL_BIN": str(wrapper),
    })
    return env


def mysql(wrapper: Path, args: Sequence[str], *, input_text: str | None = None,
          timeout: int = 120, check: bool = True) -> str:
    result = run([str(wrapper), *args], input_text=input_text,
                 env=mysql_env(wrapper), timeout=timeout, check=check)
    return result.stdout


def sql(wrapper: Path, statement: str, *, check: bool = True) -> str:
    return mysql(wrapper, ["-N", "-B", "-e", statement, DB_NAME], check=check)


def sql_one(wrapper: Path, statement: str) -> str:
    lines = [line for line in sql(wrapper, statement).splitlines() if line.strip()]
    if not lines:
        raise HarnessError(f"SQL returned no row: {statement[:100]}")
    return lines[0].strip()


def apply_sql_file(wrapper: Path, path: Path) -> None:
    mysql(wrapper, [DB_NAME], input_text=path.read_text(encoding="utf-8"), timeout=180)


def wait_for_database(wrapper: Path) -> None:
    deadline = time.monotonic() + 90
    while time.monotonic() < deadline:
        result = run([str(wrapper), "-N", "-B", "-e", "SELECT 1"],
                     env=mysql_env(wrapper), timeout=10, check=False)
        if result.returncode == 0 and result.stdout.strip() == "1":
            return
        time.sleep(1)
    raise HarnessError("dedicated MariaDB did not become ready")


def prepare_schema(wrapper: Path, wrapper_bin: Path) -> None:
    # Cache only the empty, migrated synthetic schema, never a played character.
    # Source and server-version binding avoids rerunning unrelated migrations on
    # every narrow player-command correction while keeping each DB disposable.
    import hashlib
    digest = hashlib.sha256(sql_one(wrapper, "SELECT VERSION()").encode())
    dependencies = sorted(p for p in (ROOT / "migrations").rglob("*")
                          if p.is_file() and p.suffix in {".sql", ".sh", ".json"})
    dependencies += [ROOT / "scripts" / "migration_runner.py",
                     ROOT / "scripts" / "validate_runtime_compatibility.py"]
    for dependency in dependencies:
        digest.update(str(dependency.relative_to(ROOT)).encode())
        digest.update(dependency.read_bytes())
    cache = PLAN_DIR / "player-empty-schema-cache" / (digest.hexdigest() + ".sql")
    cache.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    check = cache.with_suffix(".sha256")
    if cache.is_file() and check.is_file():
        data = cache.read_bytes()
        if hashlib.sha256(data).hexdigest() != check.read_text().strip():
            raise HarnessError("empty fixture schema cache changed unexpectedly")
        mysql(wrapper, [DB_NAME], input_text=data.decode(), timeout=180)
        env = mysql_env(wrapper)
        env["PATH"] = f"{wrapper_bin}:{env.get('PATH', '')}"
        run(["bash", str(ROOT / "migrations" / "verify_runtime_compatibility.sh")],
            env=env, timeout=120)
        print("verified empty-schema cache loaded", flush=True)
        return
    # Select the database explicitly as well as passing it as a mysql argument;
    # this keeps the operation clear if a future wrapper changes argument order.
    bootstrap = ROOT / "migrations" / "bootstrap_multithread_safe.sql"
    apply_sql_file(wrapper, bootstrap)
    env = mysql_env(wrapper)
    env["PATH"] = f"{wrapper_bin}:{env.get('PATH', '')}"
    env["MYSQL_BIN"] = str(wrapper)
    # The migration runner records the bootstrap baseline, then applies the
    # immutable death/restitution migrations (0011 and 0017) in order.
    run([sys.executable, str(ROOT / "scripts" / "migration_runner.py"),
         "adopt", "--kind", "fresh_bootstrap"], env=env, timeout=180)
    run([sys.executable, str(ROOT / "scripts" / "migration_runner.py"),
         "run"], env=env, timeout=300)
    dumped = run(["docker", "exec", "-e", "MYSQL_PWD", DB_CONTAINER,
                  "mysqldump", "--protocol=TCP", "-h", "127.0.0.1", "-uroot",
                  "--single-transaction", "--routines", "--events", "--triggers",
                  "--hex-blob", "--no-tablespaces", "--skip-add-locks", DB_NAME],
                 env=env, timeout=120).stdout.encode()
    cache.write_bytes(dumped)
    cache.chmod(0o600)
    check.write_text(hashlib.sha256(dumped).hexdigest() + "\n")
    check.chmod(0o600)
    print("fresh empty-schema cache created", flush=True)


def prepare_runtime_root(temp_root: Path) -> None:
    # The existing fixture helper provides the minimal world and login room.
    # It intentionally creates symlinks; dereference them before docker cp so
    # the runtime has no host mounts and no host-path dependency.
    temp_root.mkdir(parents=True, exist_ok=True)
    async_dir = str(ROOT / "tests" / "async")
    if async_dir not in sys.path:
        sys.path.insert(0, async_dir)
    import test_flatfile_combat_journey as journey  # type: ignore

    journey.make_fixture(temp_root)
    journey.generate_certificate(temp_root)
    for name in ("areas", "docs"):
        candidate = temp_root / name
        if candidate.is_symlink():
            candidate.unlink()
            shutil.copytree(ROOT / name, candidate)

    mini = temp_root / "areas_mini" / "mini.obj"
    source = (ROOT / "areas" / "obj" / "unique.obj").read_text(encoding="utf-8")
    lines = source.splitlines()
    try:
        start = lines.index("#67259")
    except ValueError as exc:
        raise HarnessError("unique object #67259 is absent from canonical unique.obj") from exc
    end = next((idx for idx in range(start + 1, len(lines)) if lines[idx].startswith("#")), len(lines))
    unique = lines[start:end]
    # Preserve the canonical prototype and its native, one-based BIT_29 flag.
    text = mini.read_text(encoding="utf-8")
    if "#67259\n" in text:
        raise HarnessError("mini fixture unexpectedly already contains #67259")
    marker = "$~\n"
    if not text.endswith(marker):
        raise HarnessError("mini.obj terminator is not canonical")
    mini.write_text(text[:-len(marker)] + "\n".join(unique) + "\n$~\n", encoding="utf-8")

    (temp_root / "logs" / "log").mkdir(parents=True, exist_ok=True)
    (temp_root / "journals" / "player").mkdir(parents=True, mode=0o700, exist_ok=True)
    (temp_root / "journals" / "critical").mkdir(parents=True, mode=0o700, exist_ok=True)


def start_db_proxy() -> None:
    """Bridge loopback DB connections to the private SQL container.

    The built runtime deliberately requires TLS for non-loopback DB_HOST.  A
    task-owned in-container TCP bridge keeps the game on DB_HOST=127.0.0.1
    while still reaching only the fresh database container; no host socket or
    shared service is used.
    """
    code = f'''import socket, threading
TARGET = {DB_CONTAINER!r}
PORT = {PROXY_PORT}
def relay(left, right):
    try:
        while True:
            data = left.recv(65536)
            if not data:
                break
            right.sendall(data)
    except OSError:
        pass
    finally:
        for sock in (left, right):
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            sock.close()
def accept(client):
    try:
        upstream = socket.create_connection((TARGET, 3306), timeout=10)
        upstream.settimeout(None)
    except OSError:
        client.close()
        return
    threading.Thread(target=relay, args=(client, upstream), daemon=True).start()
    threading.Thread(target=relay, args=(upstream, client), daemon=True).start()
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", PORT))
server.listen(64)
while True:
    client, _ = server.accept()
    threading.Thread(target=accept, args=(client,), daemon=True).start()
'''
    docker("exec", "-d", RUNTIME_CONTAINER, "python3", "-c", code)
    check = (
        "python3 -c 'import socket; s=socket.create_connection((\"127.0.0.1\",13306),5); "
        "s.close()'"
    )
    result = docker("exec", RUNTIME_CONTAINER, "sh", "-lc", check,
                    timeout=15, check=False)
    if result.returncode != 0:
        raise HarnessError("task-owned loopback DB bridge did not become ready")


def verify_runtime_config() -> None:
    env_raw = docker("inspect", "--format", "{{json .Config.Env}}", RUNTIME_CONTAINER).stdout.strip()
    env = set(json.loads(env_raw))
    required = {
        "DB_HOST=127.0.0.1",
        f"DB_PORT={PROXY_PORT}",
        f"DB_NAME={DB_NAME}",
        f"DB_ALLOWED_TARGETS=127.0.0.1/{DB_NAME}",
        "ENVIRONMENT=local",
        "PERSISTENCE_BACKEND=mariadb-primary",
    }
    missing = sorted(item for item in required if item not in env)
    if missing:
        raise HarnessError(f"runtime Config.Env missing explicit database settings: {missing}")
    restart = docker("inspect", "--format", "{{.HostConfig.RestartPolicy.Name}}", RUNTIME_CONTAINER).stdout.strip()
    if restart not in ("", "no"):
        raise HarnessError(f"dedicated runtime has unexpected restart policy: {restart}")


def stage_runtime(temp_root: Path) -> None:
    verify_runtime_config()
    docker("exec", RUNTIME_CONTAINER, "mkdir", "-p", "/work/issue331")
    docker("cp", f"{temp_root}/.", f"{RUNTIME_CONTAINER}:/work/issue331/")
    docker("cp", str(ARTIFACT), f"{RUNTIME_CONTAINER}:/work/issue331/dms_new")
    docker("exec", RUNTIME_CONTAINER, "chmod", "+x", "/work/issue331/dms_new")
    docker("exec", RUNTIME_CONTAINER, "chown", "root:root", "/work/issue331/duris.key", "/work/issue331/duris.crt")
    docker("exec", RUNTIME_CONTAINER, "chown", "-R", "root:root", "/work/issue331/journals")
    docker("exec", RUNTIME_CONTAINER, "chmod", "600", "/work/issue331/duris.key")
    docker("exec", RUNTIME_CONTAINER, "rm", "-f", "/work/issue331/server.pid", "/work/issue331/server.out")
    start_db_proxy()


def server_output() -> str:
    result = docker("exec", RUNTIME_CONTAINER, "sh", "-lc",
                    "test -f /work/issue331/server.out && tail -200 /work/issue331/server.out || true",
                    check=False)
    return result.stdout[-16000:]


def start_server() -> None:
    command = (
        "cd /work/issue331 || exit 1; "
        "rm -f server.pid; : > server.out; "
        "echo $$ > server.pid; "
        "exec ./dms_new --minimal -s -d /work/issue331 4000 "
        ">server.out 2>&1"
    )
    docker("exec", "-d", RUNTIME_CONTAINER, "sh", "-lc", command)
    health = (
        "for i in $(seq 1 120); do "
        "if grep -Fq 'Entering game loop' /work/issue331/server.out 2>/dev/null; then exit 0; fi; "
        "if test -f /work/issue331/server.pid && ! kill -0 $(cat /work/issue331/server.pid) 2>/dev/null; then "
        "cat /work/issue331/server.out; exit 1; fi; sleep 1; done; "
        "cat /work/issue331/server.out; exit 1"
    )
    result = docker("exec", RUNTIME_CONTAINER, "sh", "-lc", health,
                    timeout=140, check=False)
    if result.returncode != 0:
        raise HarnessError(f"game did not become ready: {result.stdout[-4000:]}")
    pipeline_log = docker("exec", RUNTIME_CONTAINER, "python3", "-c",
                          "from pathlib import Path; print(Path('/work/issue331/logs/log/status').read_text(errors='replace'))").stdout
    if any(message in pipeline_log for message in (
            "Player save pipeline unavailable", "Critical command pipeline unavailable")):
        raise HarnessError("game booted without its required persistence pipelines")
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((GAME_HOST, GAME_PORT), timeout=1):
                return
        except OSError:
            time.sleep(0.2)
    raise HarnessError("game loop marker appeared but plain player port never accepted connections")


def stop_server() -> None:
    command = (
        "if test -s /work/issue331/server.pid; then "
        "kill -TERM $(cat /work/issue331/server.pid) 2>/dev/null || true; fi"
    )
    docker("exec", RUNTIME_CONTAINER, "sh", "-lc", command, check=False)
    wait = (
        "for i in $(seq 1 60); do "
        "if test ! -s /work/issue331/server.pid || ! kill -0 $(cat /work/issue331/server.pid) 2>/dev/null; then exit 0; fi; "
        "state=$(ps -o stat= -p $(cat /work/issue331/server.pid) 2>/dev/null || true); "
        "case $state in Z*) exit 0;; esac; "
        "sleep 1; done; exit 1"
    )
    result = docker("exec", RUNTIME_CONTAINER, "sh", "-lc", wait, timeout=75, check=False)
    if result.returncode != 0:
        raise HarnessError("game process did not drain after SIGTERM")


def build_fixture(temp_root: Path, pid: int, owner_revision: int) -> str:
    binary = temp_root / "issue331_player_fixture"
    run([
        "g++", "-std=c++20", "-O2", "-I", str(ROOT / "src"),
        str(ROOT / "tests" / "async" / "issue331_player_fixture.cpp"),
        str(ROOT / "src" / "player" / "player_snapshot_codec.c"),
        "-o", str(binary),
    ], timeout=180)
    payload = run([str(binary), str(pid), str(owner_revision)], timeout=30).stdout.strip()
    if not payload or not re.fullmatch(r"[0-9a-f]+", payload):
        raise HarnessError("fixture encoder returned an invalid payload")
    return payload


def seed_fixture(wrapper: Path, pid: int, payload: str, owner_revision: int) -> None:
    seed = run([
        sys.executable, str(ROOT / "tests" / "async" / "issue331_player_seed.py"),
        str(pid), payload, str(owner_revision),
    ], timeout=30).stdout
    mysql(wrapper, [DB_NAME], input_text=seed, timeout=120)
    # The recovered gear belongs to an established adventurer, not a level-one
    # starter capped at five carried items. This is synthetic fixture setup.
    sql(wrapper, f"UPDATE player_data SET level=50 WHERE pid={pid}")
    counts = sql(wrapper, f"""
SELECT CONCAT(
 (SELECT COUNT(*) FROM player_death_custody WHERE pid={pid} AND save_revision=77), '|',
 (SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND item_uid BETWEEN 51000 AND 51006), '|',
 (SELECT COUNT(*) FROM item_ownership_quarantine WHERE item_uid BETWEEN 51000 AND 51006 AND repaired_at IS NULL), '|',
 (SELECT COUNT(*) FROM artifacts_mortal WHERE vnum={ARTIFACT_VNUM})
)""").strip()
    if counts != "6|6|6|1":
        raise HarnessError(f"disputed fixture backend counts are wrong: {counts}")


def cli_env(wrapper: Path) -> dict[str, str]:
    env = mysql_env(wrapper)
    env.update({
        "PATH": f"{wrapper.parent}:{env.get('PATH', '')}",
        "DB_ALLOWED_TARGETS": f"127.0.0.1/{DB_NAME}",
    })
    return env


def cli(wrapper: Path, args: Sequence[str], *, timeout: int = 180,
        check: bool = True) -> subprocess.CompletedProcess[str]:
    result = run([
        sys.executable, str(ROOT / "scripts" / "player_death_restitution.py"),
        *args,
    ], env=cli_env(wrapper), timeout=timeout, check=False)
    if check and result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise HarnessError(f"guarded CLI failed ({result.returncode}): {detail[-2000:]}")
    return result


class ContainerMudClient:
    """MudClient-compatible connection over the dedicated Docker network IP."""
    def __init__(self, port: int, *, source_host: str | None = None) -> None:
        async_dir = str(ROOT / "tests" / "async")
        if async_dir not in sys.path:
            sys.path.insert(0, async_dir)
        import test_flatfile_combat_journey as journey  # type: ignore
        deadline = time.monotonic() + 30
        while True:
            try:
                self.socket = socket.create_connection((GAME_HOST, port), timeout=1)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)
        self.socket.settimeout(0.25)
        self.pending = bytearray()
        self.transcript = bytearray()
        self._ansi = journey.ANSI

    def close(self) -> None:
        self.socket.close()

    def send(self, line: str) -> None:
        self.socket.sendall(line.encode("ascii") + b"\n")

    def _receive(self) -> bool:
        try:
            chunk = self.socket.recv(65536)
        except socket.timeout:
            return False
        if not chunk:
            raise AssertionError("server closed the gameplay connection")
        cleaned = self._ansi.sub(b"", chunk)
        self.pending.extend(cleaned)
        self.transcript.extend(cleaned)
        return True

    def expect(self, needle: str, timeout: float = 15) -> str:
        matched, output = self.expect_any((needle,), timeout=timeout)
        if matched != needle:
            raise AssertionError(f"unexpected match {matched!r}")
        return output

    def expect_any(self, needles: tuple[str, ...], timeout: float = 15) -> tuple[str, str]:
        targets = tuple((needle, needle.encode("ascii")) for needle in needles)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            cleaned = bytes(self.pending)
            for needle, target in targets:
                position = cleaned.find(target)
                if position >= 0:
                    consumed = cleaned[:position + len(target)]
                    del self.pending[:position + len(target)]
                    return needle, consumed.decode("utf-8", errors="replace")
            self._receive()
        readable = bytes(self.pending).decode("utf-8", errors="replace")
        raise AssertionError(f"timed out waiting for {needles!r}; received:\n{readable[-6000:]}")


def install_container_client(journey) -> None:
    journey.MudClient = ContainerMudClient


def create_account(game_port: int, suffix: str):
    sys.path.insert(0, str(ROOT / "tests" / "async"))
    import test_flatfile_combat_journey as journey  # type: ignore
    install_container_client(journey)
    # These are the canonical legal synthetic names used by the isolated
    # journey helpers; the database is fresh on every runner invocation.
    account = "Journeyacct"
    character = "Taverek"
    password = "Qz7!mN4@"
    email = f"{account.lower()}@invalid.example"
    journey.ACCOUNT = account
    journey.PASSWORD = password
    journey.CHARACTER = character
    journey.EMAIL = email
    client = journey.MudClient(game_port)
    journey.create_character(
        client, account=account, character=character, email=email,
        expected_room="The Regression Arena",
    )
    # Starter grants intentionally begin over the inventory count limit.
    # Equip and pack them through real commands, retaining every starter UID.
    command(client, "wear all", ("You ",), timeout=30)
    command(client, "put all bag", ("You put", "Ok."), timeout=30)
    client.send("save")
    client.expect(f"Save complete for {character}.", timeout=20)
    client.send("quit")
    client.expect("Please select an option", timeout=20)
    client.send("0")
    client.close()
    return journey, account, character


def pid_for_character(wrapper: Path, character: str) -> int:
    # Values are consumed internally only; they are not printed in the report.
    raw = sql_one(wrapper, "SELECT pid FROM player_data WHERE name=" +
                  "'" + character.replace("'", "''") + "' LIMIT 1")
    if not raw.isdigit() or int(raw) <= 0:
        raise HarnessError("created character did not receive a valid pid")
    return int(raw)


def offline_proof(path: Path) -> None:
    path.write_text(
        "format=duris-death-restitution-quiescence-v3\n" +
        "database=%s\n" % DB_NAME +
        "boundary=mysql-advisory-exclusion\n" +
        "guard=duris.player.death.restitution\n" +
        "expires_at=2099-01-01T00:00:00+00:00\n",
        encoding="utf-8",
    )
    path.chmod(stat.S_IRUSR | stat.S_IWUSR)


def assert_guarded_refusal(result: subprocess.CompletedProcess[str]) -> None:
    text = (result.stdout + "\n" + result.stderr).lower()
    needles = (
        "database session", "quiescen", "advisory lock", "runtime is active",
        "could not acquire", "other session", "game process",
    )
    if result.returncode == 0 or not any(needle in text for needle in needles):
        raise HarnessError("guarded apply did not refuse while the game runtime was live")


def command(client, text: str, needles: Sequence[str], *, timeout: int = 30) -> str:
    print(time.strftime("%H:%M:%S UTC", time.gmtime()), "player command:", text, flush=True)
    client.send(text)
    try:
        _, output = client.expect_any(tuple(needles), timeout=timeout)
    except AssertionError as exc:
        print(str(exc), flush=True)
        raise
    # The first matched item can precede the rest of a multi-item response.
    # Drain the response through the socket's bounded idle timeout instead of
    # treating a substring ending halfway through inventory as the full list.
    response_deadline = time.monotonic() + 2
    while time.monotonic() < response_deadline and client._receive():
        pass
    output += bytes(client.pending).decode("utf-8", errors="replace")
    client.pending.clear()
    return output


def gameplay(journey, wrapper: Path, character: str, pid: int, game_port: int) -> None:
    client = journey.reconnect_character(game_port, expected_room="The Regression Arena")
    try:
        inventory = command(client, "inventory", ("recovered leather bag",), timeout=30)
        if "recovered wooden mace" not in inventory or "unique pair of recovered gloves" not in inventory:
            raise HarnessError(f"inventory omitted a recovered top-level item: {inventory!r}")
        nested = command(client, "look in qabag", ("recovered banana",), timeout=30)
        if "recovered banana" not in nested:
            raise HarnessError("nested recovered item was not inspectable")
        command(client, "put qamace qabag", ("You put", "Ok."), timeout=30)
        command(client, "get qabanana qabag",
                ("You get", "Ok."), timeout=30)
        command(client, "eat qabanana",
                ("You eat", "You munch", "You enjoy", "You consume", "delicious"), timeout=30)
        client.send("save")
        client.expect(f"Save complete for {character}.", timeout=30)
        client.send("quit")
        client.expect("Please select an option", timeout=30)
        client.send("0")
        client.close()

        moved = sql_one(wrapper, f"""
SELECT COUNT(*) FROM player_items child
JOIN player_items bag ON bag.pid=child.pid AND bag.obj_uid=51000
WHERE child.pid={pid} AND child.obj_uid=51002 AND child.container_id=bag.id
""")
        if moved != "1":
            raise HarnessError("save did not persist inventory-to-container movement")

        client = journey.reconnect_character(game_port, expected_room="The Regression Arena")
        command(client, "look in qabag", ("recovered wooden mace",), timeout=30)
        command(client, "get qamace qabag", ("You get", "Ok."), timeout=30)
        command(client, "remove sword", ("You stop using", "You remove", "Ok."), timeout=30)
        command(client, "remove gloves", ("You stop using", "You remove", "Ok."), timeout=30)
        command(client, "wield qamace", ("You wield", "already wielding"), timeout=30)
        command(client, "wear qagloves", ("You wear", "You equip", "You pull", "already wearing"), timeout=30)
        equipment = command(client, "equipment", ("recovered wooden mace",), timeout=30)
        if "recovered" not in equipment or "gloves" not in equipment:
            raise HarnessError("equipment inspection omitted recovered equipment")
        artifact_list = command(client, "artifacts unique", (character, "unique pair", "67259"), timeout=30)
        if character.lower() not in artifact_list.lower():
            raise HarnessError("artifact/unique list did not resolve the recovered item to the recipient")
        client.send("save")
        client.expect(f"Save complete for {character}.", timeout=30)
        client.send("quit")
        client.expect("Please select an option", timeout=30)
        client.send("0")
        client.close()

        rows = sql(wrapper, f"""
SELECT CONCAT(
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid IN (51000,51002,51003)), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid=51001), '|',
 (SELECT COUNT(*) FROM player_items p JOIN player_items b ON b.pid=p.pid AND b.obj_uid=51000 WHERE p.pid={pid} AND p.obj_uid=51002 AND p.container_id=b.id), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid=51002 AND equip_slot>0), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid=51003 AND equip_slot>0), '|',
 (SELECT COUNT(*) FROM player_death_restitution_delivery WHERE recipient_pid={pid} AND death_revision=77), '|',
 (SELECT COUNT(*) FROM artifact_domain_state WHERE vnum={ARTIFACT_VNUM} AND location={pid} AND loc_type=3)
)""").strip()
        if rows != "3|0|0|1|1|4|1":
            raise HarnessError(f"post-reconnect backend state mismatch: {rows}")
    finally:
        try:
            client.close()
        except Exception:
            pass


def restart_and_replay(journey, wrapper: Path, character: str, pid: int,
                       proof: Path, plan: Path) -> None:
    # The caller has already saved and disconnected.  Restart the actual game,
    # reconnect once, then stop it for an idempotent guarded replay.
    start_server()
    client = journey.reconnect_character(GAME_PORT, expected_room="The Regression Arena")
    try:
        inventory = command(client, "inventory", ("recovered leather bag",), timeout=30)
        if "recovered leather bag" not in inventory:
            raise HarnessError("restart login lost recovered inventory")
        equipment = command(client, "equipment", ("recovered wooden mace",), timeout=30)
        if "recovered wooden mace" not in equipment or "gloves" not in equipment:
            raise HarnessError("restart login lost recovered equipment")
        client.send("quit")
        client.expect("Please select an option", timeout=30)
        client.send("0")
        client.close()
    finally:
        try:
            client.close()
        except Exception:
            pass
    stop_server()
    replay = cli(wrapper, [
        "apply", "--plan", str(plan), "--offline-proof", str(proof),
        "--approve", "--approve-artifact-reconciliation", "--actor", "issue331-player-journey", "--reason", "disposable-player-acceptance",
    ], timeout=180, check=False)
    text = (replay.stdout + "\n" + replay.stderr).lower()
    if replay.returncode != 0 or not any(word in text for word in ("already", "idempot", "applied", "receipt")):
        raise HarnessError("guarded replay did not report an idempotent receipt")
    unchanged = sql_one(wrapper, f"""
SELECT CONCAT(
 (SELECT COUNT(*) FROM player_death_restitution_delivery WHERE recipient_pid={pid} AND death_revision=77), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid IN (51000,51002,51003)), '|',
 (SELECT COUNT(*) FROM artifacts_mortal WHERE vnum={ARTIFACT_VNUM} AND location={pid} AND locType=3)
)""")
    if unchanged != "4|3|1":
        raise HarnessError(f"idempotent replay changed durable counts: {unchanged}")


def write_private_evidence(lines: list[str], transcript: str = "") -> None:
    PLAN_DIR.mkdir(parents=True, exist_ok=True)
    report = PLAN_DIR / "issue331-player-journey-last.txt"
    report.write_text("\n".join(lines) + "\n", encoding="utf-8")
    report.chmod(stat.S_IRUSR | stat.S_IWUSR)
    if transcript:
        path = PLAN_DIR / "issue331-player-journey-transcript.txt"
        path.write_text(transcript, encoding="utf-8")
        path.chmod(stat.S_IRUSR | stat.S_IWUSR)


def main() -> int:
    if not ARTIFACT.is_file() or not os.access(ARTIFACT, os.X_OK):
        raise HarnessError(f"verified server artifact is unavailable: {ARTIFACT}")
    PLAN_DIR.mkdir(parents=True, exist_ok=True)
    temp = Path(tempfile.mkdtemp(prefix="issue331-player-", dir=str(PLAN_DIR)))
    wrapper_bin = temp / "bin"
    wrapper_bin.mkdir(mode=0o700)
    wrapper = write_mysql_wrapper(wrapper_bin)
    server_started = False
    evidence = ["issue331 player journey: dedicated disposable runtime"]
    def mark(message: str) -> None:
        evidence.append(message)
        print(time.strftime("%H:%M:%S UTC", time.gmtime()), message, flush=True)
        write_private_evidence(evidence)
    try:
        wait_for_database(wrapper)
        prepare_schema(wrapper, wrapper_bin)
        prepare_runtime_root(temp / "run_root")
        stage_runtime(temp / "run_root")
        start_server()
        server_started = True
        journey, account, character = create_account(GAME_PORT, f"{os.getpid():x}")
        pid = pid_for_character(wrapper, character)
        stop_server()
        server_started = False
        start_server()
        server_started = True
        baseline = journey.reconnect_character(GAME_PORT, expected_room="The Regression Arena")
        baseline.send("save")
        baseline.expect(f"Save complete for {character}.", timeout=30)
        baseline.send("quit")
        baseline.expect("Please select an option", timeout=30)
        baseline.send("0")
        baseline.close()
        mark("baseline SQL cold login/save before recovery: verified")
        stop_server()
        server_started = False
        owner_revision = int(sql_one(wrapper, f"SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id={pid} AND owner_context_id=0")) + 1
        payload = build_fixture(temp, pid, owner_revision)
        seed_fixture(wrapper, pid, payload, owner_revision)
        start_server()
        server_started = True
        mark("disputed death/quarantine fixture: seeded and backend-count verified")

        inspect_path = temp / "inspect.json"
        plan_path = temp / "plan.json"
        proof_path = temp / "offline-proof.txt"
        for path in (inspect_path, plan_path):
            path.chmod(stat.S_IRUSR | stat.S_IWUSR) if path.exists() else None
        try:
            inspected = cli(wrapper, ["inspect", "--pid", str(pid), "--death-revision", "77",
                                      "--recipient-pid", str(pid), "--artifact", str(inspect_path)], timeout=180, check=True)
            planned = cli(wrapper, ["plan", "--inspect", str(inspect_path), "--artifact", str(plan_path),
                                    "--approve-artifact-reconciliation"], timeout=180, check=True)
        except HarnessError as exc:
            if "reconciliation" in str(exc).lower() or "module" in str(exc).lower() or "import" in str(exc).lower():
                write_private_evidence(evidence + ["BLOCKED: guarded CLI artifact reconciliation module is not integrated",
                                                   str(exc), "server_log_tail:", server_output()])
                raise JourneyBlocked("guarded CLI could not load the unintegrated artifact reconciliation module") from exc
            raise
        mark("guarded CLI inspect/plan: executed")
        offline_proof(proof_path)
        live_apply = cli(wrapper, ["apply", "--plan", str(plan_path), "--offline-proof", str(proof_path),
                                   "--approve", "--approve-artifact-reconciliation",
                                   "--actor", "issue331-player-journey", "--reason", "disposable-player-acceptance"], timeout=180, check=False)
        assert_guarded_refusal(live_apply)
        mark("guarded CLI live-runtime refusal: verified")
        stop_server()
        server_started = False
        applied = cli(wrapper, ["apply", "--plan", str(plan_path), "--offline-proof", str(proof_path),
                                "--approve", "--approve-artifact-reconciliation",
                                "--actor", "issue331-player-journey", "--reason", "disposable-player-acceptance"], timeout=180, check=True)
        cli(wrapper, ["verify", "--plan", str(plan_path)], timeout=180, check=True)
        mark("guarded CLI offline apply/verify: executed")
        delivery = sql_one(wrapper, f"SELECT COUNT(*) FROM player_death_restitution_delivery WHERE recipient_pid={pid} AND death_revision=77")
        recovered = sql_one(wrapper, f"SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid IN (51000,51001,51002,51003)")
        if delivery != "4" or recovered != "4":
            raise HarnessError(f"backend recovery readback mismatch: delivery={delivery}, recovered={recovered}")
        mark("backend delivery/player_items/artifact authority readback: verified")
        start_server()
        server_started = True
        gameplay(journey, wrapper, character, pid, GAME_PORT)
        mark("normal login, nested inspection, equip/use, container move, save/reconnect: verified")
        stop_server()
        server_started = False
        restart_and_replay(journey, wrapper, character, pid, proof_path, plan_path)
        mark("restart login and guarded replay without duplication: verified")
        write_private_evidence(evidence)
        print("ISSUE331_PLAYER_JOURNEY_OK")
        for line in evidence[1:]:
            print(line)
        return 0
    except JourneyBlocked as exc:
        print(f"ISSUE331_PLAYER_JOURNEY_BLOCKED: {exc}", file=sys.stderr)
        return 77
    except Exception as exc:
        logs = docker("exec", RUNTIME_CONTAINER, "python3", "-c",
                      "from pathlib import Path; p=Path('/work/issue331/logs/log/status'); "
                      "print(p.read_text(errors='replace')[-18000:] if p.exists() else '')",
                      check=False).stdout
        extra = docker("exec", RUNTIME_CONTAINER, "python3", "-c",
                       "from pathlib import Path; import json; root=Path('/work/issue331/logs'); "
                       "print(json.dumps({str(p.relative_to(root)): [s for s in p.read_text(errors='replace').splitlines() if any(k in s.lower() for k in ['load','error','fail','invalid','snapshot'])][-80:] for p in root.rglob('*') if p.is_file() and p.stat().st_size<1000000}))",
                       check=False).stdout
        write_private_evidence(evidence + ["FAILED: " + str(exc), logs, extra, server_output()])
        raise
    finally:
        if server_started:
            try:
                stop_server()
            except Exception:
                pass
        shutil.rmtree(temp, ignore_errors=True)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (HarnessError, subprocess.TimeoutExpired) as exc:
        print(f"ISSUE331_PLAYER_JOURNEY_FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1)
