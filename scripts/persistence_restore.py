"""Fresh-candidate restore and qualification. No production promotion operation."""
import contextlib
import gzip
import json
import os
from pathlib import Path
import pwd
import secrets
import shutil
import signal
import subprocess
import time
import uuid

import persistence_backup as backup


def clean_environment(candidate):
    return {"PATH": "/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin", "HOME": str(candidate),
            "ENVIRONMENT": "local", "REDIS": "FALSE",
            "PLAYER_SAVE_JOURNAL_DIR": str(candidate / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(candidate / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": "4050"}


def tombstone_preflight(path, p, captured):
    path = backup.secure_path(path, False)
    backup.require(not backup.overlaps(path, p["root"]) and
                   not backup.overlaps(path, p["restore_root"]), "independent_tombstones_required")
    ledger = backup.read_json(path)
    backup.require(set(ledger) == {"version", "captured_at", "policy_sha256", "tombstones"} and
                   ledger["version"] == 1 and type(ledger["captured_at"]) is int,
                   "invalid_tombstone_evidence")
    now = int(time.time())
    backup.require(max(captured, now - p["rpo_seconds"]) <= ledger["captured_at"] <= now,
                   "tombstone_evidence_stale")
    policy = backup.ROOT / "migrations/data_lifecycle_manifest.json"
    backup.require(ledger["policy_sha256"] == backup.digest(policy), "erasure_policy_mismatch")
    backup.require(isinstance(ledger["tombstones"], list), "invalid_tombstone_evidence")
    # account_erasure.TombstoneLedger has no durable source-wide propagation
    # adapter. Refuse the entire candidate rather than republish erased accounts,
    # journal records, cache inputs, or indirect value-domain references.
    backup.require(not ledger["tombstones"], "erasure_propagation_required")
    return backup.digest(path)


@contextlib.contextmanager
def private_database(candidate):
    datadir = candidate / "mysql"
    datadir.mkdir(mode=0o700)
    env = clean_environment(candidate)
    user = pwd.getpwuid(os.getuid()).pw_name
    backup.run(["mariadb-install-db", "--no-defaults", "--datadir=" + str(datadir),
                "--auth-root-authentication-method=normal", "--skip-test-db", "--user=" + user],
               env=env)
    exports = candidate / "exports"
    exports.mkdir(mode=0o700)
    socket = candidate / "mysql.sock"
    args = ["mariadbd", "--no-defaults", "--datadir=" + str(datadir),
            "--socket=" + str(socket), "--pid-file=" + str(candidate / "mysql.pid"),
            "--skip-networking", "--skip-log-bin", "--event-scheduler=OFF",
            "--local-infile=0", "--secure-file-priv=" + str(exports), "--user=" + user]
    with (candidate / "database.log").open("wb") as log:
        process = subprocess.Popen(args, env=env, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 60
            client = ["mysql", "--no-defaults", "--protocol=socket", "--socket=" + str(socket),
                      "--user=root", "-N", "-B"]
            while True:
                probe = subprocess.run([*client, "-e", "SELECT 1"], env=env,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if probe.returncode == 0:
                    break
                backup.require(process.poll() is None and time.monotonic() < deadline,
                               "isolated_database_start_failed")
                time.sleep(0.2)
            password = secrets.token_hex(24)
            # Only this newly initialized daemon receives an administrative query.
            # Import uses a schema-only account without FILE/SUPER/CREATE USER.
            backup.run([*client], env=env, input=(
                "CREATE DATABASE duris_restore CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
                "CREATE USER 'restore'@'localhost' IDENTIFIED BY '" + password + "';"
                "GRANT ALL ON duris_restore.* TO 'restore'@'localhost';").encode())
            env.update(DB_HOST="localhost", DB_SOCKET=str(socket), DB_USER="restore",
                       DB_PASSWD=password, MYSQL_PWD=password, DB_NAME="duris_restore",
                       DB_ALLOWED_TARGETS="localhost/duris_restore", DB_PORT="3306", DB_TLS="FALSE")
            yield env
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def database_import(generation, env):
    args = ["mysql", "--no-defaults", "--binary-mode", "--local-infile=0",
            "--protocol=socket", "--socket=" + env["DB_SOCKET"], "--user=restore", "duris_restore"]
    with backup.streaming_process(args, env=env, input_pipe=True) as process:
        with gzip.open(generation / "database.sql.gz", "rb") as source:
            database = backup.read_json(generation / "manifest.json")["database"]
            for line in source:
                # Full-database dumps preserve source identity for other operational
                # backup consumers. This schema-only importer selects only its new DB.
                if line.startswith(b"CREATE DATABASE "):
                    continue
                if line.strip() == ("USE " + chr(96) + database + chr(96) + ";").encode():
                    line = b"USE duris_restore;\n"
                process.stdin.write(line)
        process.stdin.close()


def database_qualify(env):
    backup.run(["bash", str(backup.ROOT / "migrations/verify_runtime_compatibility.sh")], env=env)
    backup.run(["python3", str(backup.ROOT / "scripts/qualify_database_restore.py")], env=env)


def service_load(candidate, mode, env):
    binary = backup.ROOT / ("bin/server/dms_restore_flatfile" if mode == "flatfile-primary"
                            else "bin/server/dms_new")
    backup.require(binary.is_file(), "server_build_required")
    runtime = candidate / "runtime"
    runtime.mkdir(mode=0o700)
    # Namespace root cannot use the caller's host DAC override to traverse a
    # checkout in another user's private home (including hosted CI checkouts).
    # Stage all executed project code under the operator-owned candidate before
    # entering the namespace, just as we already do with the runtime data.
    staged_binary = runtime / "server"
    staged_qualifier = runtime / "qualify_service_restore.py"
    shutil.copyfile(binary, staged_binary)
    staged_binary.chmod(0o700)
    shutil.copyfile(backup.ROOT / "scripts/qualify_service_restore.py", staged_qualifier)
    staged_qualifier.chmod(0o600)
    (runtime / "logs/log").mkdir(mode=0o700, parents=True)
    for name in ("areas_mini", "lib"):
        shutil.copytree(backup.ROOT / name, runtime / name, symlinks=False)
        for path in (runtime / name).rglob("*"):
            path.chmod(0o700 if path.is_dir() else 0o600)
    for name in ("players", "critical"):
        (candidate / "journals" / name).mkdir(mode=0o700, parents=True, exist_ok=True)
    backup.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-sha256", "-nodes",
                "-days", "1", "-subj", "/CN=localhost", "-keyout", str(runtime / "duris.key"),
                "-out", str(runtime / "duris.crt")], env=env)
    env = dict(env, PERSISTENCE_MODE=mode)
    backup.run(["unshare", "--user", "--map-root-user", "--net", "--pid", "--fork", "--kill-child=KILL",
                "python3", str(staged_qualifier),
                str(candidate), str(staged_binary)], env=env, timeout=120)


def remove_candidate(root, candidate):
    # Only copied runtime assets and this candidate's stopped database are removed.
    for base, dirs, files in os.walk(candidate, followlinks=False):
        for name in dirs + files:
            path = Path(base) / name
            backup.require(not path.is_symlink(), "candidate_symlink_rejected")
            path.chmod(0o700 if name in dirs else 0o600)
    backup.remove_owned(root, candidate)


def restore_capacity(p):
    root = backup.secure_path(p["restore_root"], True)
    backup.require(root.is_dir() and root.is_mount(), "dedicated_restore_filesystem_required")
    device = root.stat().st_dev
    backup.require(all(device != path.stat().st_dev for path in [p["root"], *p["live_roots"]]
                       if path.exists()), "restore_filesystem_shared_with_authority")
    usage = shutil.disk_usage(root)
    backup.require(usage.total <= p["max_bytes"] and usage.free >= p["min_free_bytes"],
                   "restore_filesystem_capacity_invalid")


def restore(p, generation_name, tombstones, drill=False):
    backup.mkdir(p["root"])
    restore_capacity(p)
    with backup.lock(p["root"] / ".job.lock"), backup.lock(p["restore_root"] / ".restore.lock"):
        if drill and (p["root"] / "drill.json").exists():
            receipt = backup.read_json(p["root"] / "drill.json")
            if time.time() - receipt.get("completed", 0) < p["drill_seconds"]:
                return {"event": "drill", "result": "not_due"}
        items = backup.generations(p["root"])
        backup.require(items, "no_verified_generation")
        if generation_name is None:
            generation, meta = items[0]
        else:
            backup.require(backup.GENERATION.fullmatch(generation_name), "invalid_generation_name")
            generation = p["root"] / generation_name
            meta = backup.verify(generation)
        backup.require(meta["runtime_schema_sha256"] ==
                       backup.digest(backup.ROOT / "migrations/runtime_compatibility_manifest.json"),
                       "restore_requires_matching_runtime")
        ledger_hash = tombstone_preflight(tombstones, p, meta["created"])
        candidate = p["restore_root"] / ("candidate-" + uuid.uuid4().hex)
        candidate.mkdir(mode=0o700)
        backup.write_json(candidate / "ISOLATED_RESTORE", {"generation": generation.name})
        try:
            env = clean_environment(candidate)
            shutil.copytree(generation / "journals", candidate / "journals")
            qualifier = str(backup.ROOT / "bin/tools/qualify_flatfile_restore")
            backup.run([qualifier, "--journals-preflight", str(candidate)], env=env)
            if meta["mode"] == "flatfile-primary":
                shutil.copytree(generation / "state", candidate / "state")
                backup.require(backup.inventory(generation / "state") == backup.inventory(candidate / "state"),
                               "restore_copy_checksum_mismatch")
                env["FLATFILE_STATE_DIR"] = str(candidate / "state")
                result = backup.run([str(backup.ROOT / "bin/tools/qualify_flatfile_restore"),
                                     "--state-preflight", str(candidate / "state")], env=env)
                aggregates = json.loads(result)
                service_load(candidate, meta["mode"], env)
                backup.run([qualifier, "--journals-drained", str(candidate)], env=env)
                aggregates = json.loads(backup.run([qualifier, str(candidate / "state")], env=env))
            else:
                with private_database(candidate) as env:
                    database_import(generation, env)
                    database_qualify(env)
                    aggregates = {"schema_history_and_value_reconciliation": "ok"}
                    service_load(candidate, meta["mode"], env)
                    backup.run([qualifier, "--journals-drained", str(candidate)], env=env)
                    database_qualify(env)
            backup.require(ledger_hash == tombstone_preflight(tombstones, p, meta["created"]),
                           "erasure_evidence_changed_during_restore")
            backup.verify(generation)
            receipt = {"event": "drill" if drill else "restore", "result": "qualified",
                       "generation": generation.name, "candidate": candidate.name,
                       "completed": int(time.time()), "checks": aggregates}
            backup.write_json(candidate / "QUALIFIED.json", receipt)
            if drill:
                remove_candidate(p["restore_root"], candidate)
                backup.write_json(p["root"] / "drill.json", receipt)
            return receipt
        except BaseException:
            if candidate.exists():
                backup.write_json(candidate / "FAILED.json", {"result": "failed"})
                if drill:
                    remove_candidate(p["restore_root"], candidate)
            raise

