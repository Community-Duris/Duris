#!/usr/bin/env python3
"""One personal-local control qualification; creates only owned Docker resources.

No .env, saved connection, host port, existing database or player account is used.
All SQL is in a new network-isolated runner's namespace. Receipts stay in bin/.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[2]
LABEL = "duris.telemetry.control-qualification"
IMAGES = {"mariadb": "mariadb:10.11.14", "mysql": "mysql:8.0.46"}
FOCUSED = (
    "test_telemetry_control_inventory.py", "test_telemetry_battle_contribution_contract.py",
    "test_telemetry_control_stream.py", "test_telemetry_battle_history.py",
    "test_telemetry_progression.py", "test_telemetry_contract_headers.py", "test_telemetry_repository.py", "test_telemetry_reports_contract.py",
    "test_telemetry_identity_publication.py", "test_telemetry_rollup_schema.py", "test_telemetry_outage.py",
    "test_telemetry_incidents.py",
    "test_telemetry_runtime_integration.py", "test_telemetry_runtime_outage.py", "test_telemetry_runtime_exhaustion.py",
    "test_epic_stone_runtime.py", "test_boon_reward_zone_transactional_cutover.py",
    "test_rested_bonus_runtime.py", "test_world_quest_xp_feedback.py", "test_chaos_infinite_starting_grants.py",
)


def source_digest():
    paths = subprocess.check_output(["git", "ls-files", "-co", "--exclude-standard"],
        cwd=ROOT, text=True, encoding="utf-8").splitlines()
    digest = hashlib.sha256()
    for name in sorted(set(paths)):
        path = ROOT / name
        if path.is_file():
            digest.update(name.encode() + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def fixture_database(token, mode):
    # 33 ASCII bytes leaves room for the runtime's 31-byte exclusion prefix
    # within MySQL's 64-byte named-lock limit. Keep every mode uniquely scoped.
    return "duris_telemetry_test_" + hashlib.sha256((token + ":" + mode).encode()).hexdigest()[:12]


def run(args):
    if not args.disposable:
        raise SystemExit("use --disposable to create this explicit personal-local allow-listed fixture")
    token = uuid.uuid4().hex[:12]
    name = "duris-controls-" + token
    directory = ROOT / "bin/tests" / name
    directory.mkdir(parents=True)
    receipt = {"run": token, "status": "running", "source_sha256": source_digest(),
        "engines": {}, "phases": [], "production_or_staging_access": False,
        "actual_gameplay": False, "tsan": "not run by this command; earlier Docker host probe was unsupported"}
    created = []

    def command(argv, phase, *, timeout=3600):
        started = time.monotonic()
        print("Starting " + phase, flush=True)
        with (directory / (phase + ".log")).open("xb") as output:
            result = subprocess.run(argv, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
                timeout=timeout)
        receipt["phases"].append({"phase": phase, "returncode": result.returncode,
            "elapsed_seconds": round(time.monotonic() - started, 3)})
        if result.returncode:
            lines = (directory / (phase + ".log")).read_text(encoding="utf-8", errors="replace").splitlines()
            print("\n".join(line[:1200] for line in lines[-30:]), flush=True)
            raise RuntimeError("qualification failed: " + phase)
        print("PASS " + phase, flush=True)

    def docker_exec(argv, phase, env=None):
        invocation = ["docker", "exec", "-w", "/workspace"]
        for key, value in (env or {}).items():
            invocation += ["-e", key + "=" + value]
        command(invocation + [name, *argv], phase)

    def owned_remove(container):
        owner = subprocess.check_output(["docker", "inspect", "--format",
            '{{index .Config.Labels "' + LABEL + '"}}', container], text=True).strip()
        if owner != token or container not in created:
            raise RuntimeError("refusing cleanup of an unowned resource")
        subprocess.run(["docker", "rm", "-f", container], check=True, stdout=subprocess.DEVNULL)
        created.remove(container)

    try:
        image = args.tools_image
        if image is None:
            base = "duris-telemetry-control-base:local"
            image = "duris-telemetry-control-tools:local"
            command(["docker", "build", "-f", "tests/integration/Dockerfile", "-t", base, "."], "tools-base")
            command(["docker", "build", "-f", "tests/async/Dockerfile.telemetry-controls",
                "--build-arg", "TOOLS_IMAGE=" + base, "-t", image, "."], "tools-python")
        subprocess.run(["docker", "run", "--detach", "--init", "--name", name, "--label", LABEL + "=" + token,
            "--network", "none", "--mount", "type=bind,source=" + str(ROOT) + ",target=/workspace",
            "--entrypoint", "sleep", image, "infinity"], check=True, stdout=subprocess.DEVNULL)
        created.append(name)
        receipt["tools_image_id"] = subprocess.check_output(
            ["docker", "inspect", "--format", "{{.Image}}", name], text=True).strip()
        docker_exec(["python3", "-c", "import pymysql, cryptography"], "python-driver")
        docker_exec(["make", "-C", "src", "-j2"], "server-build")
        changed = ("src/telemetry/telemetry_runtime.c", "src/telemetry/telemetry_runtime.h",
            "src/cmd/actoff.c", "src/world/epic.c", "src/world/zone_touch_transaction.c",
            "src/world/zone_touch_transaction.h", "src/classes/paladins.c", "src/combat/justice.c",
            "src/core/runtime_compatibility_contract.h", "src/sql/sql.c",
            "src/cmd/actset.c", "src/cmd/staff_setattr.c", "src/combat/fight.c", "src/combat/fight_state.c",
            "src/combat/range.c", "src/world/handler.c", "tests/async/telemetry_gameplay_adapters.cc",
            "tests/async/telemetry_battle_contribution_harness.cc", "tests/async/telemetry_runtime_exhaustion.cc",
            "src/telemetry/telemetry_battle_result.c", "src/telemetry/telemetry_battle_result.h",
            "tests/async/telemetry_battle_result_harness.cc", "src/telemetry/telemetry_types.h",
            "src/telemetry/telemetry_repository.c", "src/telemetry/telemetry_outage.c",
            "tests/async/telemetry_repository_harness.cc", "tests/async/telemetry_outage_harness.cc")
        changed += ("src/telemetry/telemetry_config.c", "src/telemetry/telemetry_progression.c", "src/telemetry/telemetry_progression.h",
            "tests/async/telemetry_progression_harness.cc", "tests/async/telemetry_runtime_integration.cc",
            "src/telemetry/telemetry_session.c", "src/telemetry/telemetry_session.h",
            "src/telemetry/telemetry_health.c", "src/world/limits.c", "src/combat/fight.c", "src/core/prototypes.h",
            "src/economy/boon.c", "src/combat/chaos.c", "src/cmd/actwiz.c", "src/cmd/actinf.c", "src/cmd/actset.c",
            "src/guild/guild.c", "src/magic/affects.c", "src/magic/spells.c")
        format_args = ["bash", "scripts/format.sh", "--check"]
        for path in changed:
            if Path(path).suffix in (".c", ".h", ".cc", ".cpp", ".hpp"):
                format_args += ["--file", path]
        docker_exec(format_args, "format")
        for test in FOCUSED:
            docker_exec(["python3", "tests/async/" + test], test.removesuffix(".py"))
        docker_exec(["python3", "tests/async/test_telemetry_progression.py", "--sanitize"],
            "progression-context-asan-ubsan")
        docker_exec(["python3", "tests/async/test_telemetry_gameplay_adapters.py", "--native-affects", "--sanitize"], "native-affects-asan-ubsan")
        docker_exec(["python3", "tests/async/test_telemetry_gameplay_adapters.py", "--sanitize"],
            "native-result-adapters-asan-ubsan")
        docker_exec(["python3", "tests/async/test_telemetry_battle_contributions.py", "--sanitize"], "control-asan-ubsan")
        docker_exec(["python3", "tests/async/test_telemetry_battle_contribution_contract.py", "ResultContractTests"],
            "battle-result-asan-ubsan", env={"DURIS_TELEMETRY_RESULT_SANITIZE": "1"})
        docker_exec(["python3", "scripts/validate_runtime_compatibility.py"], "runtime-contract")
        docker_exec(["python3", "scripts/validate_data_lifecycle.py"], "lifecycle-contract")
        output_root = "/workspace/bin/tests/" + name
        docker_exec(["python3", "tests/async/test_telemetry_runtime_exhaustion.py", "--sanitize",
            "--clock-performance-output", output_root + "/clock-performance.json"], "paired-clock-asan-ubsan")
        docker_exec(["python3", "tests/async/test_telemetry_control_performance.py", "--output",
            output_root + "/control-performance.json"], "control-performance")
        docker_exec(["python3", "tests/async/test_telemetry_gameplay_adapters.py", "--result-performance-output",
            output_root + "/result-performance.json"], "native-result-performance")
        for engine in args.engines:
            container = name + "-" + engine
            password = "synthetic-" + token
            prefix = "MARIADB" if engine == "mariadb" else "MYSQL"
            startup = ["docker", "run", "--detach", "--name", container, "--label", LABEL + "=" + token,
                "--network", "container:" + name, "-e", prefix + "_ROOT_PASSWORD=" + password,
                "-e", prefix + "_ROOT_HOST=%", IMAGES[engine]]
            if engine == "mysql":
                startup += ["mysqld", "--skip-innodb-use-native-aio"]
            subprocess.run(startup, check=True, stdout=subprocess.DEVNULL)
            created.append(container)
            for _ in range(120):
                ready = subprocess.run(["docker", "exec", "-e", "MYSQL_PWD=" + password, name,
                    "mysql", "--protocol=tcp", "-h127.0.0.1", "-uroot", "-N", "-B", "-e", "SELECT 1"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if ready.returncode == 0:
                    break
                time.sleep(1)
            else:
                raise RuntimeError("owned SQL startup budget exceeded")
            env = dict(TELEMETRY_REPOSITORY_DISPOSABLE="1", TELEMETRY_REPOSITORY_DB_IMAGE=IMAGES[engine],
                TELEMETRY_REPOSITORY_HOST="127.0.0.1", TELEMETRY_REPOSITORY_PORT="3306",
                TELEMETRY_REPOSITORY_USER="root", TELEMETRY_REPOSITORY_PASSWORD=password)
            lineage_env = dict(env, ENVIRONMENT="test", TEST_DB_DISPOSABLE="1", STAGING_FORK_DISPOSABLE_SERVER="1",
                DB_HOST="127.0.0.1", DB_PORT="3306", DB_USER="root", DB_PASSWD=password, DB_NAME="duris_268_ownedtest")
            docker_exec(["python3", "tests/async/test_staging_migration_fork_mysql.py", "--disposable-loopback",
                "mariadb10_11" if engine == "mariadb" else "mysql8", "--report", output_root + "/" + engine + "-lineages.json"],
                engine + "-lineages", lineage_env)
            for mode, test in (("writer", "test_telemetry_repository.py"),
                    ("storage", "test_telemetry_control_storage.py"),
                    ("progression", "test_telemetry_progression.py"),
                    ("native", "test_telemetry_battle_runtime_sql.py"), ("gameplay", "run_telemetry_control_journey.py")):
                env.update(TELEMETRY_REPOSITORY_DATABASE=fixture_database(token, mode),
                    TELEMETRY_REPOSITORY_RESULT=output_root + "/" + engine + "-writer.json",
                    TELEMETRY_CONTROL_STORAGE_RESULT=output_root + "/" + engine + "-storage.json",
                    TELEMETRY_PROGRESSION_STORAGE_RESULT=output_root + "/" + engine + "-progression.json",
                    TELEMETRY_BATTLE_RUNTIME_RESULT=output_root + "/" + engine + "-native.json",
                    TELEMETRY_CONTROL_JOURNEY_RESULT=output_root + "/" + engine + "-gameplay.json")
                docker_exec(["python3", "tests/async/" + test, "--sql-fixture"], engine + "-" + mode, env)
            receipt["engines"][engine] = {mode: json.loads((directory / (engine + "-" + mode + ".json")).read_text(encoding="utf-8"))
                for mode in ("lineages", "writer", "storage", "progression", "native", "gameplay")}
            owned_remove(container)
        receipt["actual_gameplay"] = True
        receipt["performance"] = json.loads((directory / "control-performance.json").read_text(encoding="utf-8"))
        receipt["clock_performance"] = json.loads((directory / "clock-performance.json").read_text(encoding="utf-8"))
        receipt["result_performance"] = json.loads((directory / "result-performance.json").read_text(encoding="utf-8"))
        receipt["source_unchanged"] = receipt["source_sha256"] == source_digest()
        if not receipt["source_unchanged"]:
            raise RuntimeError("tracked qualification sources changed during the run")
        receipt["status"] = "passed"
    except Exception:
        receipt["status"] = "failed"
        raise
    finally:
        for container in list(reversed(created)):
            owned_remove(container)
        receipt["owned_resources_removed"] = not created
        (directory / "qualification.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print("Receipt: " + str(directory / "qualification.json"), flush=True)


if __name__ == "__main__":
    sys.stdout.reconfigure(errors="backslashreplace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disposable", action="store_true")
    parser.add_argument("--tools-image", help="explicit compatible local tools image; omit for the maintained Docker build")
    parser.add_argument("--engines", nargs="+", choices=tuple(IMAGES), default=list(IMAGES))
    run(parser.parse_args())
