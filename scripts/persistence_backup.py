#!/usr/bin/env python3
"""Policy-governed full generations. Linux only.

An optional owner-only environment file is parsed as literal values. Restore creates a
new isolated candidate; qualification never promotes it or changes a generation.
"""
from __future__ import annotations

import argparse
import contextlib
import fcntl
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import threading
import shlex
import signal
import uuid

if __name__ == "__main__":
    sys.modules["persistence_backup"] = sys.modules[__name__]

ROOT = Path(__file__).resolve().parents[1]
MODES = {"flatfile-primary", "mariadb-primary"}
GENERATION = re.compile(r"[0-9]{20}-[0-9a-f]{32}")
LOCKS = {".identity.lock", ".critical-authority.lock", ".accounts.lock"}


class BackupError(Exception):
    """Only redacted, fixed error codes may cross the CLI boundary."""


def require(condition, code):
    if not condition:
        raise BackupError(code)


def checkpoint(stage):
    """Fault-injection seam for unit tests; no operational environment switch."""


def strict_json(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate_json_key")
        result[key] = value
    return result


def secure_path(path, directory=None):
    path = Path(path)
    require(path.is_absolute() and ".." not in path.parts, "unsafe_path")
    for ancestor in reversed([path, *path.parents]):
        if not ancestor.exists() and not ancestor.is_symlink():
            continue
        info = ancestor.lstat()
        require(not stat.S_ISLNK(info.st_mode), "symlink_rejected")
        require(info.st_uid in {0, os.getuid()}, "unexpected_owner")
        # A root-owned sticky /tmp is a safe ancestor, never a managed root.
        sticky = (ancestor != path and info.st_uid == 0 and
                  stat.S_ISDIR(info.st_mode) and info.st_mode & stat.S_ISVTX)
        require(not info.st_mode & 0o022 or sticky, "writable_ancestor")
    if path.exists():
        info = path.lstat()
        require(info.st_uid == os.getuid() and not info.st_mode & 0o077,
                "require_owner_only")
        if directory is not None:
            require(stat.S_ISDIR(info.st_mode) if directory else
                    stat.S_ISREG(info.st_mode) and info.st_nlink == 1,
                    "unexpected_file_type")
    return path


def mkdir(path):
    secure_path(path, True)
    path.mkdir(mode=0o700, parents=True, exist_ok=True)
    secure_path(path, True)


def overlaps(a, b):
    return a == b or a in b.parents or b in a.parents


def read_json(path):
    secure_path(path, False)
    require(path.stat().st_size <= 32 * 1024 * 1024, "metadata_too_large")
    with path.open() as stream:
        return json.load(stream, object_pairs_hook=strict_json)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def sync_dir(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def write_json(path, value):
    secure_path(path.parent, True)
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".metadata-", delete=False) as stream:
        temp = Path(stream.name)
        try:
            stream.write((json.dumps(value, sort_keys=True) + "\n").encode())
            stream.flush()
            os.fsync(stream.fileno())
            os.replace(temp, path)
            sync_dir(path.parent)
        finally:
            temp.unlink(missing_ok=True)


def policy_load(path):
    p = read_json(path)
    fields = {"journal_roots", "version", "approved", "custodian", "schedule_seconds", "rpo_seconds",
              "hourly", "daily", "weekly", "max_bytes", "min_free_bytes",
              "drill_seconds", "root", "restore_root", "live_roots", "replica_root"}
    require(set(p) == fields and p["version"] == 1, "invalid_policy_fields")
    require(p["approved"] is True and isinstance(p["custodian"], str) and
            p["custodian"] and p["custodian"] != "SET_BY_OPERATOR", "policy_not_approved")
    for key in ("schedule_seconds", "rpo_seconds", "hourly", "daily", "weekly",
                "max_bytes", "min_free_bytes", "drill_seconds"):
        require(type(p[key]) is int and 0 <= p[key] <= 2**63 - 1, "invalid_policy_number")
    require(60 <= p["schedule_seconds"] <= p["rpo_seconds"] <= 604800,
            "invalid_schedule_rpo")
    require(2 <= p["hourly"] <= 8760 and p["daily"] <= 3650 and p["weekly"] <= 520,
            "invalid_retention")
    require(p["max_bytes"] > 0 and 3600 <= p["drill_seconds"] <= 2678400,
            "invalid_capacity_drill")
    require(isinstance(p["live_roots"], list) and p["live_roots"], "live_roots_required")
    for key in ("root", "restore_root"):
        p[key] = secure_path(Path(p[key]), True)
        require(len(p[key].parts) >= 3, "unsafe_managed_root")
    p["live_roots"] = [Path(x) for x in p["live_roots"]]
    for path in p["live_roots"]:
        require(path.is_absolute() and ".." not in path.parts, "unsafe_authority_path")
        for ancestor in [path, *path.parents]:
            require(not ancestor.is_symlink(), "authority_symlink_rejected")
    # A DB datadir may belong to the database service UID; it is only a
    # forbidden destination, never a file source. Flatfile/journal sources
    # independently require custodian ownership before reading.
    require(isinstance(p["journal_roots"], dict) and set(p["journal_roots"]) <= {"players", "critical"},
            "invalid_journal_roots")
    p["journal_roots"] = {key: secure_path(Path(value), True) for key, value in p["journal_roots"].items()}
    p["live_roots"] += list(p["journal_roots"].values())
    managed = [p["root"], p["restore_root"]]
    if p["replica_root"] is not None:
        p["replica_root"] = secure_path(Path(p["replica_root"]), True)
        managed.append(p["replica_root"])
    for i, path in enumerate(managed):
        require(all(not overlaps(path, other) for other in managed[i + 1:] + p["live_roots"]),
                "authority_path_overlap")
    return p


@contextlib.contextmanager
def lock(path, wait=0):
    secure_path(path, False)
    fd = os.open(path, os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
    try:
        secure_path(path, False)
        deadline = time.monotonic() + wait
        while True:
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                require(time.monotonic() < deadline, "job_overlap_or_authority_busy")
                time.sleep(0.05)
        yield
    finally:
        os.close(fd)


def inventory(root, exclude_locks=False):
    secure_path(root, True)
    result = {}
    for base, dirs, files in os.walk(root, followlinks=False):
        for name in sorted(dirs + files):
            path = Path(base) / name
            secure_path(path, name in dirs)
            if name in files and not (exclude_locks and name in LOCKS):
                result[path.relative_to(root).as_posix()] = {
                    "sha256": digest(path), "bytes": path.stat().st_size}
    return result


def sync_tree(root):
    inventory(root)
    for base, dirs, files in os.walk(root, topdown=False):
        for name in files:
            fd = os.open(Path(base) / name, os.O_RDONLY | os.O_NOFOLLOW)
            try:
                os.fsync(fd)
            finally:
                os.close(fd)
        sync_dir(base)


def journal_capture(stage, p):
    target = stage / "journals"
    target.mkdir(mode=0o700)
    snapshots = {}
    for name, source in p["journal_roots"].items():
        secure_path(source, True)
        require(source.is_dir(), "journal_source_missing")
        snapshots[name] = inventory(source)
        needed = sum(x["bytes"] for x in snapshots[name].values())
        require(total_size(p["root"]) + needed < p["max_bytes"], "capacity_headroom_required")
        require(shutil.disk_usage(stage).free >= needed + p["min_free_bytes"], "low_free_capacity")
        shutil.copytree(source, target / name)
        require(inventory(target / name) == snapshots[name], "journal_changed_during_capture")
    for variable, name in (("PLAYER_SAVE_JOURNAL_DIR", "players"), ("CRITICAL_COMMAND_JOURNAL_DIR", "critical")):
        if os.environ.get(variable):
            require(p["journal_roots"].get(name) == Path(os.environ[variable]), "journal_policy_mismatch")
    return snapshots


def flatfile_capture(stage, p):
    source = secure_path(Path(os.environ.get("FLATFILE_STATE_DIR", "")), True)
    require(source.is_dir() and source in p["live_roots"], "flatfile_authority_not_configured")
    target = stage / "state"
    with contextlib.ExitStack() as stack:
        for relative in ("identities/names/.identity.lock", "domains/.critical-authority.lock",
                         "identities/accounts/.accounts.lock"):
            path = source / relative
            if path.parent.is_dir():
                stack.enter_context(lock(path, wait=120))
        before = inventory(source, exclude_locks=True)
        require(before, "empty_flatfile_authority")
        needed = sum(x["bytes"] for x in before.values())
        require(needed + total_size(p["root"]) < p["max_bytes"], "capacity_headroom_required")
        require(shutil.disk_usage(stage).free >= needed + p["min_free_bytes"], "low_free_capacity")
        shutil.copytree(source, target)
        for base, dirs, files in os.walk(target):
            for name in files:
                if name in LOCKS:
                    (Path(base) / name).unlink()
        require(before == inventory(target) == inventory(source, exclude_locks=True),
                "flatfile_generation_changed")
    return {"pending_transaction": (target / "domains/.critical-authority-transaction").exists()}


def db_connection():
    e = os.environ
    for name in ("ENVIRONMENT", "DB_HOST", "DB_USER", "DB_PASSWD", "DB_NAME", "DB_ALLOWED_TARGETS"):
        require(bool(e.get(name)), "missing_database_configuration")
    require(e["ENVIRONMENT"] in {"local", "production"}, "invalid_environment")
    require(re.fullmatch(r"[A-Za-z0-9_]+", e["DB_NAME"]) and
            re.fullmatch(r"[A-Za-z0-9_.-]+", e["DB_USER"]), "invalid_database_identifier")
    require(f'{e["DB_HOST"]}/{e["DB_NAME"]}' in e["DB_ALLOWED_TARGETS"].split(","),
            "database_not_allowlisted")
    port = int(e.get("DB_PORT", "3306"))
    require(1 <= port <= 65535, "invalid_database_port")
    local = e["DB_HOST"] in {"localhost", "127.0.0.1", "::1"}
    args = ["--no-defaults", "--user=" + e["DB_USER"]]
    if e.get("DB_SOCKET"):
        require(local and e["ENVIRONMENT"] == "local" and Path(e["DB_SOCKET"]).is_absolute(),
                "unsafe_database_socket")
        args += ["--protocol=socket", "--socket=" + e["DB_SOCKET"]]
    else:
        args += ["--protocol=tcp", "--host=" + e["DB_HOST"], "--port=" + str(port)]
        if not local:
            require(e.get("DB_TLS") == "TRUE" and Path(e.get("DB_SSL_CA", "")).is_file(),
                    "remote_database_tls_required")
            args += ["--ssl-ca=" + e["DB_SSL_CA"], "--ssl-verify-server-cert"]
    env = dict(os.environ, MYSQL_PWD=e["DB_PASSWD"])
    return args, env, e["DB_NAME"]


@contextlib.contextmanager
def streaming_process(args, *, env, input_pipe=False, timeout=300):
    process = subprocess.Popen(args, env=env, start_new_session=True,
                               stdin=subprocess.PIPE if input_pipe else None,
                               stdout=subprocess.DEVNULL if input_pipe else subprocess.PIPE,
                               stderr=subprocess.DEVNULL)
    def expire():
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    timer = threading.Timer(timeout, expire)
    timer.daemon = True
    timer.start()
    try:
        yield process
        require(process.wait(timeout=timeout) == 0, "streaming_process_failed")
    finally:
        timer.cancel()
        timer.join()
        if process.poll() is None:
            expire()
            process.wait()
        for stream in (process.stdin, process.stdout):
            if stream is not None and not stream.closed:
                stream.close()


def run(args, *, env=None, input=None, timeout=300):
    result = subprocess.run(args, input=input, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, timeout=timeout)
    require(result.returncode == 0, "subprocess_failed")
    return result.stdout


def validate_dump(path):
    expected = set(json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
                   ["runtime_table_sql_list"].replace("'", "").split(","))
    found = set()
    with gzip.open(path, "rb") as stream:
        for line in stream:
            match = re.match(rb"CREATE TABLE `([a-z0-9_]+)`", line)
            if match:
                found.add(match[1].decode())
    require(expected <= found, "dump_missing_required_tables")


def mariadb_capture(stage, p):
    args, env, database = db_connection()
    engines = run(["mysql", *args, "-N", "-B", database, "-e",
                   "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                   "AND table_type='BASE TABLE' AND engine<>'InnoDB';"], env=env)
    require(engines.strip() == b"0", "nontransactional_tables")
    path = stage / "database.sql.gz"
    help_text = run(["mysqldump", "--no-defaults", "--help"])
    options = ["--single-transaction", "--quick", "--hex-blob", "--routines", "--events",
               "--triggers", "--databases", database]
    if b"--no-tablespaces" in help_text:
        options.insert(0, "--no-tablespaces")
    remaining = p["max_bytes"] - total_size(p["root"])
    class BoundedOutput:
        def __init__(self, stream):
            self.stream = stream
            self.written = 0
        def write(self, data):
            self.written += len(data)
            require(self.written <= remaining, "generation_exceeds_capacity")
            require(shutil.disk_usage(stage).free >= len(data) + p["min_free_bytes"], "low_free_capacity")
            return self.stream.write(data)
        def flush(self):
            self.stream.flush()
    with path.open("xb") as raw:
        output = BoundedOutput(raw)
        with gzip.GzipFile(fileobj=output, mode="wb", mtime=0) as zipped:
            with streaming_process(["mysqldump", *args, *options], env=env) as process:
                shutil.copyfileobj(process.stdout, zipped)
    validate_dump(path)
    return {"database": database}


def verify(generation):
    secure_path(generation, True)
    require(GENERATION.fullmatch(generation.name), "invalid_generation_name")
    manifest = read_json(generation / "manifest.json")
    require(manifest.get("version") == 1 and manifest.get("mode") in MODES and
            manifest.get("generation") == generation.name and
            type(manifest.get("created")) is int and 0 <= manifest["created"] <= time.time() + 300,
            "invalid_generation_manifest")
    actual = inventory(generation)
    actual.pop("manifest.json")
    require(actual == manifest.get("files") and actual, "generation_checksum_mismatch")
    require(digest(generation / "runtime-schema.json") == manifest.get("runtime_schema_sha256"),
            "schema_manifest_mismatch")
    if manifest["mode"] == "mariadb-primary":
        validate_dump(generation / "database.sql.gz")
    return manifest


def generations(root):
    result = []
    for path in root.iterdir():
        if GENERATION.fullmatch(path.name):
            result.append((path, verify(path)))
        else:
            require(path.name in {".job.lock", ".schedule.lock", "schedule.json", "status.json", "drill.json"} or
                    path.name.startswith((".staging-", ".trash-", ".metadata-")),
                    "unknown_backup_root_entry")
            secure_path(path)
    return sorted(result, key=lambda pair: (pair[1]["created"], pair[0].name), reverse=True)


def retained(items, p, now):
    keep = {path.name for path, _ in items[:2]}
    for seconds, count in ((3600, p["hourly"]), (86400, p["daily"]), (604800, p["weekly"])):
        buckets = set()
        for path, meta in items:
            bucket = meta["created"] // seconds
            if 0 <= now // seconds - bucket < count and bucket not in buckets:
                buckets.add(bucket)
                keep.add(path.name)
    return keep


def total_size(path):
    return sum(x["bytes"] for x in inventory(path).values())


def remove_owned(root, path):
    require(path.parent == root and path != root, "unsafe_delete_target")
    secure_path(root, True)
    inventory(path)  # Reject substitutions before any removal.
    require(shutil.rmtree.avoids_symlink_attacks, "fd_safe_removal_required")
    shutil.rmtree(path)
    sync_dir(root)


def rotate(root, p, newest):
    checkpoint("before_rotation")
    items = generations(root)  # Any invalid generation blocks all pruning.
    require(items and items[0][0] == newest, "new_generation_not_newest")
    keep = retained(items, p, int(time.time()))
    require(sum(total_size(path) for path, _ in items if path.name in keep) <= p["max_bytes"],
            "retention_exceeds_capacity")
    for path, _ in items:
        if path.name not in keep:
            checkpoint("before_prune")
            verify(newest)
            verify(path)
            trash = root / (".trash-" + path.name)
            require(not trash.exists(), "interrupted_prune_requires_inspection")
            os.rename(path, trash)
            sync_dir(root)
            checkpoint("prune_renamed")
            remove_owned(root, trash)
            checkpoint("prune_complete")


def replicate(source, p):
    root = p["replica_root"]
    if root is None:
        return "not_configured"
    # An already-mounted SSHFS destination uses authenticated encrypted SSH.
    # Mount creation, credentials, host keys, and host durability belong to its custodian.
    secure_path(root, True)
    require(root.is_dir(), "replica_mount_missing")
    fs = run(["findmnt", "-n", "-o", "FSTYPE", "--target", str(root)]).strip()
    require(fs == b"fuse.sshfs", "replica_requires_sshfs_transport")
    with lock(root / ".job.lock"):
        generations(root)
        destination = root / source.name
        if destination.exists():
            require(verify(source) == verify(destination), "replica_verification_failed")
            rotate(root, p, destination)
            return "transport_and_readback_verified"
        needed = total_size(source)
        require(total_size(root) + needed <= p["max_bytes"], "replica_capacity_headroom_required")
        require(shutil.disk_usage(root).free >= needed + p["min_free_bytes"], "replica_low_free_capacity")
        stage = root / (".staging-" + uuid.uuid4().hex)
        shutil.copytree(source, stage)
        sync_tree(stage)
        destination = root / source.name
        require(not destination.exists(), "replica_generation_exists")
        os.rename(stage, destination)
        sync_dir(root)
        require(verify(source) == verify(destination), "replica_verification_failed")
        rotate(root, p, destination)
    return "transport_and_readback_verified"


def backup(p, mode):
    if mode == "mariadb-primary-flatfile-fallback":
        mode = "mariadb-primary"
    require(mode in MODES, "invalid_persistence_mode")
    root = p["root"]
    mkdir(root)
    with lock(root / ".job.lock"):
        items = generations(root)
        if mode == "flatfile-primary":
            source = secure_path(Path(os.environ.get("FLATFILE_STATE_DIR", "")), True)
            require(source in p["live_roots"], "flatfile_authority_not_configured")
            if not source.exists() and not items:
                return {"event": "backup", "result": "authority_not_initialized"}
        require(not any(x.name.startswith((".staging-", ".trash-")) for x in root.iterdir()),
                "interrupted_job_requires_inspection")
        if items:
            receipt = read_json(root / "status.json")
            require(receipt.get("generation") == items[0][0].name and receipt.get("result") == "ok",
                    "prior_backup_requires_finalization")
        require(total_size(root) < p["max_bytes"], "capacity_headroom_required")
        require(shutil.disk_usage(root).free >= p["min_free_bytes"], "low_free_capacity")
        name = f"{time.time_ns():020d}-{uuid.uuid4().hex}"
        stage = root / (".staging-" + uuid.uuid4().hex)
        stage.mkdir(mode=0o700)
        published = False
        try:
            checkpoint("before_capture")
            capture_started = int(time.time())
            journals = journal_capture(stage, p)
            detail = flatfile_capture(stage, p) if mode == "flatfile-primary" else mariadb_capture(stage, p)
            require(all(inventory(p["journal_roots"][name]) == files for name, files in journals.items()),
                    "journal_changed_during_authority_capture")
            checkpoint("after_capture")
            shutil.copyfile(ROOT / "migrations/runtime_compatibility_manifest.json", stage / "runtime-schema.json")
            (stage / "runtime-schema.json").chmod(0o600)
            meta = {"version": 1, "generation": name, "created": capture_started, "mode": mode,
                    "runtime_schema_sha256": digest(stage / "runtime-schema.json"),
                    "files": inventory(stage), **detail}
            write_json(stage / "manifest.json", meta)
            require(total_size(root) <= p["max_bytes"], "generation_exceeds_capacity")
            checkpoint("before_sync")
            sync_tree(stage)
            checkpoint("before_publish")
            destination = root / name
            require(not destination.exists(), "generation_collision")
            os.rename(stage, destination)
            published = True
            sync_dir(root)
            checkpoint("after_publish")
            verify(destination)
            checkpoint("after_verify")
            # Separate-destination failure preserves prior local generations too.
            replica = replicate(destination, p)
            rotate(root, p, destination)
            write_json(root / "status.json", {"version": 1, "generation": name,
                       "completed": int(time.time()), "replica": replica, "result": "ok"})
            return {"event": "backup", "result": "ok", "generation": name, "replica": replica}
        finally:
            if not published and stage.exists():
                remove_owned(root, stage)


def status(p, require_drill=False):
    root = p["root"]
    secure_path(root, True)
    require(root.is_dir(), "no_verified_generation")
    with lock(root / ".job.lock"):
        items = generations(root)
        require(items, "no_verified_generation")
        age = int(time.time()) - items[0][1]["created"]
        size = sum(total_size(path) for path, _ in items)
        free = shutil.disk_usage(root).free
        require(age <= p["rpo_seconds"], "rpo_exceeded")
        require(size <= p["max_bytes"] and free >= p["min_free_bytes"], "capacity_exceeded")
        require(not any(x.name.startswith((".trash-", ".staging-")) for x in root.iterdir()),
                "interrupted_job_requires_inspection")
        receipt = read_json(root / "status.json")
        require(receipt.get("generation") == items[0][0].name and receipt.get("result") == "ok",
                "backup_or_rotation_incomplete")
        if p["replica_root"] is not None:
            require(receipt.get("replica") == "transport_and_readback_verified", "replica_not_verified")
        drill_path = root / "drill.json"
        drill = read_json(drill_path) if drill_path.exists() else {}
        drill_age = int(time.time()) - drill.get("completed", 0)
        if require_drill:
            require(drill.get("result") == "qualified" and 0 <= drill_age <= p["drill_seconds"],
                    "restore_drill_missing_or_overdue")
        return {"event": "status", "drill_age_seconds": drill_age if drill else None, "result": "ok", "age_seconds": age,
                "bytes": size, "free_bytes": free, "generations": len(items),
                "replica": receipt["replica"]}


def load_environment(path):
    if not path.exists() and not path.is_symlink():
        return
    secure_path(path, False)
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        require(stat.S_ISREG(info.st_mode) and info.st_uid == os.getuid() and
                info.st_nlink == 1 and not info.st_mode & 0o077, "unsafe_environment_file")
        content = stream.read(65537)
    require(len(content) <= 65536, "environment_file_too_large")
    for line in content.decode().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        key, separator, value = line.partition("=")
        require(separator and re.fullmatch(r"[A-Z][A-Z0-9_]*", key), "invalid_environment_assignment")
        words = shlex.split(value, comments=True, posix=True)
        require(len(words) <= 1, "invalid_environment_value")
        os.environ[key] = words[0] if words else ""


def main():
    os.umask(0o077)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--env-file", type=Path)
    parser.add_argument("--policy", type=Path, default=os.environ.get("BACKUP_POLICY_FILE"))
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("backup")
    sub.add_parser("status").add_argument("--require-drill", action="store_true")
    sub.add_parser("schedule")
    sub.add_parser("finalize")
    for command in ("restore", "drill"):
        child = sub.add_parser(command)
        child.add_argument("--generation")
        child.add_argument("--tombstones", type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.env_file is not None:
            load_environment(args.env_file)
        if args.policy is None:
            args.policy = os.environ.get("BACKUP_POLICY_FILE")
        require(args.policy is not None, "backup_policy_required")
        p = policy_load(Path(args.policy))
        if args.command == "backup":
            result = backup(p, os.environ.get("PERSISTENCE_MODE", "mariadb-primary"))
        elif args.command == "status":
            result = status(p, args.require_drill)
        elif args.command == "finalize":
            with lock(p["root"] / ".job.lock"):
                items = generations(p["root"])
                require(items, "no_verified_generation")
                destination = items[0][0]
                replica = replicate(destination, p)
                rotate(p["root"], p, destination)
                write_json(p["root"] / "status.json", {"version": 1, "generation": destination.name,
                           "completed": int(time.time()), "replica": replica, "result": "ok"})
                result = {"event": "finalize", "result": "ok", "generation": destination.name}
        elif args.command == "schedule":
            # A minute timer evaluates the approved cadence; pre-cycle backups do not
            # move the independently scheduled deadline.
            mkdir(p["root"])
            with lock(p["root"] / ".schedule.lock"):
                receipt_path = p["root"] / "schedule.json"
                last = read_json(receipt_path).get("completed", 0) if receipt_path.exists() else 0
                if time.time() - last >= p["schedule_seconds"]:
                    result = backup(p, os.environ.get("PERSISTENCE_MODE", "mariadb-primary"))
                    require(result.get("result") == "ok", "authority_not_initialized")
                    write_json(receipt_path, {"completed": int(time.time())})
                else:
                    result = status(p)
        else:
            from persistence_restore import restore
            result = restore(p, args.generation, args.tombstones, args.command == "drill")
        print(json.dumps(result, sort_keys=True))
        return 0
    except (BackupError, OSError, ValueError, KeyError, TypeError,
            subprocess.SubprocessError, EOFError) as error:
        code = str(error) if isinstance(error, BackupError) else "operation_failed"
        print(json.dumps({"event": args.command, "result": "failed", "code": code}), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
