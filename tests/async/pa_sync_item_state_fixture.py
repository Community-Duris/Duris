"""Disposable synchronous item-state SQL fixture for manual invocation only."""
from __future__ import annotations

import os
from pathlib import Path
import re
import secrets
import shutil
import subprocess
import time

from pa_accounting_batch_artifact import load_base_build, report_base_build

ROOT = Path(__file__).resolve().parents[2]
MARIADB_IMAGE = "mariadb:10.11"
TOOLS_IMAGE = "duris-issue-213-tools:latest"
HARNESS = ROOT / "tests/async/pa_sync_item_state_sql.cpp"
REQUIRED_SOURCES = (
    "src/sql/sql_player.c",
    "src/player/player_snapshot_codec.c",
    "src/sql/item_extra_descr_codec.c",
    "migrations/bootstrap_multithread_safe.sql",
    "migrations/immutable/0015_output_preferences.sql",
    "migrations/immutable/0020_player_death_restitution.sql",
    "migrations/immutable/0035_player_item_dynamic_state.sql",
    "migrations/immutable/0035_player_item_dynamic_state.sh",
)


def _clean_env(**overrides: str) -> dict[str, str]:
    """Pass only process plumbing and explicit fixture values; never read .env."""
    env: dict[str, str] = {
        "PATH": os.environ.get("PATH", os.defpath),
        "HOME": os.environ.get("HOME", "/tmp"),
        "LANG": "C.UTF-8",
        "LC_ALL": "C.UTF-8",
    }
    for name in ("TMPDIR", "DOCKER_HOST", "DOCKER_CONTEXT", "DOCKER_CONFIG",
                 "XDG_RUNTIME_DIR"):
        if os.environ.get(name):
            env[name] = os.environ[name]
    env.update(overrides)
    return env


def _redact(text: str, env: dict[str, str] | None = None,
            extra_values: tuple[str, ...] = ()) -> str:
    secrets_to_hide = list(extra_values)
    for name, value in (env or {}).items():
        if value and any(marker in name.upper() for marker in
                         ("PASS", "PWD", "SECRET", "TOKEN")):
            secrets_to_hide.append(value)
    for value in sorted(set(secrets_to_hide), key=len, reverse=True):
        text = text.replace(value, "[REDACTED]")
    return text


def _run(command: list[str], *, env: dict[str, str] | None = None,
         input_text: str | None = None, timeout: int = 180,
         check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )
    if check and result.returncode:
        raise RuntimeError(
            f"{command[0]} fixture step failed (exit {result.returncode}):\n" +
            _redact(result.stdout[-8000:], env)
        )
    return result


def _require(result: subprocess.CompletedProcess[str], action: str,
             env: dict[str, str] | None = None) -> str:
    if result.returncode:
        raise RuntimeError(
            f"{action} failed (exit {result.returncode}):\n" +
            _redact(result.stdout[-8000:], env)
        )
    return result.stdout


def _sql(container: str, database: str, password: str, *,
         source: Path | None = None, statement: str | None = None) -> str:
    env = _clean_env(MYSQL_PWD=password)
    command = [
        "docker", "exec", "-i", "-e", "MYSQL_PWD", container, "mariadb",
        "--no-defaults", "--protocol=tcp", "-h127.0.0.1", "-P3306", "-uroot",
        "--batch", "--skip-column-names", database,
    ]
    if statement is not None:
        command.extend(("-e", statement))
        input_text = None
    else:
        if source is None:
            raise ValueError("fixture SQL source is required")
        input_text = source.read_text(encoding="utf-8")
    return _require(_run(command, env=env, input_text=input_text, timeout=180),
                    "container-local fixture SQL", env)


def _verify_removed(name: str) -> None:
    inspect = _run(["docker", "container", "inspect", name],
                   env=_clean_env(), timeout=30, check=False)
    if inspect.returncode == 0:
        raise RuntimeError(f"disposable fixture container remains: {name}")
    detail = inspect.stdout.lower()
    if not any(marker in detail for marker in ("no such container", "no such object")):
        raise RuntimeError(
            f"could not verify disposable fixture container cleanup for {name}: " +
            _redact(inspect.stdout[-2000:])
        )


def _remove_container(name: str, label: str) -> None:
    if not re.fullmatch(r"[0-9a-f]{64}", name):
        raise RuntimeError("cleanup requires this run's returned container ID")
    _run(["docker", "rm", "--force", name], env=_clean_env(),
         timeout=45, check=False)
    _verify_removed(name)
    print(f"C14_FIXTURE_REMOVED kind={label} name={name}")


def _created_container_id(output: str) -> str:
    container_id = output.strip()
    if not re.fullmatch(r"[0-9a-f]{64}", container_id):
        raise RuntimeError("container creation did not return an exact ID; effect unknown")
    return container_id


def _verify_identity(container_id: str, name: str, image_id: str,
                     network: str, token: str) -> None:
    # No Config.Env capture: it contains the disposable credentials.
    template = '{{.Id}}\n{{.Name}}\n{{.Image}}\n{{.HostConfig.NetworkMode}}\n{{index .Config.Labels "duris.run_id"}}'
    actual = _require(_run(["docker", "inspect", "--format", template, container_id],
                          env=_clean_env(), timeout=30), "verify created container identity")
    if actual.strip().splitlines() != [container_id, "/" + name, image_id, network, token]:
        raise RuntimeError("created fixture container identity mismatch")


def _compile_and_run(tools_container: str, database: str, password: str) -> str:
    if not HARNESS.is_file():
        raise RuntimeError(f"manual SQL harness is missing: {HARNESS}")

    _require(_run(["docker", "exec", tools_container, "mkdir", "-p",
                   "/workspace/src", "/workspace/tests/async"], timeout=30),
             "prepare disposable compile workspace")
    _require(_run(["docker", "cp", str(ROOT / "src") + "/.",
                   tools_container + ":/workspace/src/"]), "copy production source")
    _require(_run(["docker", "cp", str(HARNESS),
                   tools_container + ":/workspace/tests/async/pa_sync_item_state_sql.cpp"]),
             "copy C14 harness")

    compile_script = (
        "set -euo pipefail; "
        "read -r -a MYSQL_CFLAGS <<< \"$(mysql_config --cflags)\"; "
        "read -r -a MYSQL_LIBS <<< \"$(mysql_config --libs)\"; "
        "g++ -std=c++20 -pthread -U__NO_MYSQL__ -Isrc "
        "-ffunction-sections -fdata-sections \"${MYSQL_CFLAGS[@]}\" "
        "tests/async/pa_sync_item_state_sql.cpp "
        "src/player/player_snapshot_codec.c src/sql/item_extra_descr_codec.c "
        "-Wl,--gc-sections \"${MYSQL_LIBS[@]}\" -lcrypto "
        "-o /tmp/c14_sync_item_state"
    )
    compiled = _run(["docker", "exec", tools_container, "bash", "-lc", compile_script],
                    timeout=300)
    _require(compiled, "compile actual synchronous sql_player source")

    db_env = _clean_env(
        DB_HOST="127.0.0.1",
        DB_PORT="3306",
        DB_USER="root",
        DB_PASSWD=password,
        DB_NAME=database,
    )
    exec_prefix = ["docker", "exec"]
    for name in ("DB_HOST", "DB_PORT", "DB_USER", "DB_PASSWD", "DB_NAME"):
        exec_prefix.extend(("-e", name))
    positive = _run(exec_prefix + [tools_container, "/tmp/c14_sync_item_state"],
                    env=db_env, timeout=90, check=False)
    positive_text = _redact(positive.stdout, db_env)
    if positive.returncode or "[PASS] C14 synchronous item dynamic-state save/readback/repeat" not in positive_text:
        raise RuntimeError(
            f"positive C14 synchronous SQL journey failed (exit {positive.returncode}):\n" +
            positive_text[-8000:]
        )

    negative = _run(exec_prefix + [tools_container, "/tmp/c14_sync_item_state",
                                   "--sabotage-drop-item-properties"],
                    env=db_env, timeout=90, check=False)
    negative_text = _redact(negative.stdout, db_env)
    expected_detection = "ASSERTION FAILED: item_properties missing from SQL readback"
    if negative.returncode == 0 or expected_detection not in negative_text:
        raise RuntimeError(
            f"C14 sabotage variant did not detect item_properties loss "
            f"(exit {negative.returncode}):\n{negative_text[-8000:]}"
        )
    return "\n".join((positive_text.strip(),
                      "[EXPECTED-RED] C14 sabotage detected item_properties loss"))


def run_sync_item_state_fixture() -> str:
    """Run only against an exact parent-frozen source/build and fresh DB container."""
    build = load_base_build(ROOT, required_sources=REQUIRED_SOURCES)
    report_base_build(build, scope="C14 sync sql_save_player_items; frozen binary not executed")

    if not shutil.which("docker"):
        raise RuntimeError("Docker is required for the disposable C14 SQL fixture")
    image_ids: dict[str, str] = {}
    for image in (MARIADB_IMAGE, TOOLS_IMAGE):
        inspected = _run(["docker", "image", "inspect", "--format", "{{.Id}}", image],
                         env=_clean_env(), timeout=30, check=False)
        if inspected.returncode:
            raise RuntimeError(f"required local image is unavailable (no pull attempted): {image}")
        image_ids[image] = inspected.stdout.strip()
        if not re.fullmatch(r"sha256:[0-9a-f]{64}", image_ids[image]):
            raise RuntimeError("cannot pin required local image identity")

    token = secrets.token_hex(8)
    run_id = f"{os.getpid()}-{token}"
    database = f"c14_sync_item_state_test_{token}"
    db_container = f"duris-c14-sync-items-{run_id}"
    tools_container = f"{db_container}-tools"
    db_name, tools_name = db_container, tools_container
    for name in (db_name, tools_name):
        _verify_removed(name)
    created: list[tuple[str, str]] = []
    password = secrets.token_urlsafe(32)
    container_env = _clean_env(
        MARIADB_ROOT_PASSWORD=password,
        MARIADB_DATABASE=database,
    )
    output: list[str] = []
    try:
        db_container = _created_container_id(_require(_run([
            "docker", "run", "--pull=never", "--rm", "--detach", "--name", db_name,
            "--network=none",
            "--label", "duris.task=c14-sync-item-state",
            "--label", f"duris.run_id={token}", "--cpus=2", "--memory=2g",
            "--memory-swap=2g", "--env", "MARIADB_ROOT_PASSWORD", "--env",
            "MARIADB_DATABASE", image_ids[MARIADB_IMAGE],
        ], env=container_env, timeout=90), "start unique disposable MariaDB", container_env))
        created.append((db_container, "database"))
        print(f"C14_FIXTURE_CREATED kind=database id={db_container} token={token} name={db_name}", flush=True)
        _verify_identity(db_container, db_name, image_ids[MARIADB_IMAGE], "none", token)

        ready = False
        mysql_env = _clean_env(MYSQL_PWD=password)
        for _ in range(120):
            probe = _run([
                "docker", "exec", "-e", "MYSQL_PWD", db_container, "mariadb",
                "--protocol=tcp", "-h127.0.0.1", "-P3306", "-uroot",
                "--batch", "--skip-column-names", "-e", "SELECT 1",
            ], env=mysql_env, timeout=10, check=False)
            if probe.returncode == 0 and probe.stdout.strip() == "1":
                ready = True
                break
            time.sleep(0.5)
        if not ready:
            logs = _run(["docker", "logs", db_container], env=_clean_env(),
                        timeout=30, check=False)
            raise RuntimeError("disposable MariaDB did not become ready:\n" +
                               _redact(logs.stdout[-5000:], mysql_env))

        identity = _sql(db_container, database, password,
                        statement="SELECT DATABASE(),VERSION()").strip().split("\t")
        if len(identity) != 2 or identity[0] != database or not identity[1].startswith("10.11.") or "MariaDB" not in identity[1]:
            raise RuntimeError("connected schema/engine is not the provisioned supported target")

        for migration in (
            ROOT / "migrations/bootstrap_multithread_safe.sql",
            ROOT / "migrations/immutable/0015_output_preferences.sql",
            ROOT / "migrations/immutable/0020_player_death_restitution.sql",
            ROOT / "migrations/immutable/0035_player_item_dynamic_state.sql",
        ):
            output.append(_sql(db_container, database, password, source=migration))

        verifier = ROOT / "migrations/immutable/0035_player_item_dynamic_state.sh"
        verifier_env = _clean_env(
            DB_HOST="127.0.0.1", DB_PORT="3306", DB_USER="root",
            DB_PASSWD=password, DB_NAME=database,
        )
        verifier_result = _run([
            "docker", "exec", "-i", "-e", "DB_HOST", "-e", "DB_PORT", "-e",
            "DB_USER", "-e", "DB_PASSWD", "-e", "DB_NAME", db_container,
            "bash", "-s",
        ], env=verifier_env, input_text=verifier.read_text(encoding="utf-8"), timeout=90)
        output.append(_require(verifier_result, "verify immutable schema 0035", verifier_env))

        tools_container = _created_container_id(_require(_run([
            "docker", "create", "--pull=never", "--rm", "--name", tools_name,
            "--label", "duris.task=c14-sync-item-state", "--label", f"duris.run_id={token}",
            "--network=container:" + db_container, "--cpus=2", "--memory=2g",
            "--memory-swap=2g", "-w", "/workspace", image_ids[TOOLS_IMAGE],
            "sleep", "infinity",
        ], env=_clean_env(), timeout=45), "create local compiler container"))
        created.append((tools_container, "tools"))
        print(f"C14_FIXTURE_CREATED kind=tools id={tools_container} token={token} name={tools_name}", flush=True)
        _require(_run(["docker", "start", tools_container], env=_clean_env(), timeout=45),
                 "start local compiler container")
        _verify_identity(tools_container, tools_name, image_ids[TOOLS_IMAGE],
                         "container:" + db_container, token)
        output.append(_compile_and_run(tools_container, database, password))
    finally:
        cleanup_errors: list[str] = []
        for name, label in reversed(created):
            try:
                _remove_container(name, label)
            except RuntimeError as exc:
                cleanup_errors.append(str(exc))
        if cleanup_errors:
            raise RuntimeError("C14 disposable cleanup failed: " + " | ".join(cleanup_errors))

    return "\n".join(part.strip() for part in output if part.strip())
