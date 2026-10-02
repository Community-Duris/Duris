"""Owned SQL service and fresh schemas for the regression matrix.

Only a new labelled container is used. No checkout configuration is read, no
existing service is selected, and every diagnostic is retained before removal.
"""
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
LABEL = "duris.regression.matrix"


def clean_environment():
    return {key: value for key, value in os.environ.items()
            if key in {"PATH", "LD_LIBRARY_PATH", "TMPDIR", "CXX", "CC", "HOME",
                       "DURIS_TEST_CONTAINER", "DOCKER_HOST", "DOCKER_CONTEXT"}}


def docker(*args, environment=None):
    result = subprocess.run(["docker", *args], text=True, capture_output=True,
                            env=environment or clean_environment(), timeout=120)
    if result.returncode:
        raise RuntimeError("docker operation failed: " + result.stderr)
    return result.stdout.strip()


def image_identity(image):
    metadata = json.loads(docker("image", "inspect", image))[0]
    return {"requested": image, "id": metadata["Id"],
            "digests": metadata.get("RepoDigests", [])}


def private_network():
    """A container may share only its own network namespace with a fixture."""
    configured = os.environ.get("DURIS_TEST_CONTAINER")
    if not configured:
        if Path("/.dockerenv").exists():
            raise RuntimeError("container matrix needs DURIS_TEST_CONTAINER naming this runner")
        return None
    metadata = json.loads(docker("inspect", configured))[0]
    if not metadata["Id"].startswith(socket.gethostname()):
        raise RuntimeError("DURIS_TEST_CONTAINER must identify this runner, not another runtime")
    return "container:" + metadata["Id"]


class DisposableSQL:
    def __init__(self, image, evidence):
        if not image.startswith(("mysql:", "mysql@", "mariadb:", "mariadb@")):
            raise ValueError("matrix accepts only the declared MySQL/MariaDB providers")
        self.identity = image_identity(image)
        self.image = image
        self.evidence = Path(evidence)
        self.evidence.mkdir(parents=True, exist_ok=True)
        self.token = secrets.token_hex(6)
        self.name = "duris-test-matrix-" + self.token
        self.password = secrets.token_hex(32)
        self.container = None
        self.timeline = []
        self.environment = dict(clean_environment(), ENVIRONMENT="test", DB_HOST="127.0.0.1",
                                DB_USER="root", DB_PASSWD=self.password, MYSQL_PWD=self.password,
                                TEST_DB_HOST="127.0.0.1", TEST_DB_USER="root",
                                TEST_DB_PASSWORD=self.password, TEST_DB_DISPOSABLE="1", DB_TLS="FALSE")

    def redact(self, text):
        return text.replace(self.password, "<fixture-password>")

    def record(self, event, **details):
        self.timeline.append({"time": time.time(), "event": event, **details})
        temporary = self.evidence / "timeline.json.tmp"
        temporary.write_text(json.dumps(self.timeline, indent=2))
        temporary.replace(self.evidence / "timeline.json")

    def __enter__(self):
        prefix = "MARIADB" if self.image.startswith("mariadb") else "MYSQL"
        environment = dict(self.environment, **{prefix + "_ROOT_PASSWORD": self.password,
                                               prefix + "_ROOT_HOST": "%"})
        network = private_network()
        args = ["run", "--pull=never", "--detach", "--restart=no", "--name", self.name,
                "--label", LABEL + "=" + self.token, "--cpus=2", "--memory=1536m",
                "--env", prefix + "_ROOT_PASSWORD", "--env", prefix + "_ROOT_HOST"]
        if network:
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 0))
                port = probe.getsockname()[1]
            args += ["--network", network, self.identity["id"], "--port=" + str(port),
                     "--bind-address=127.0.0.1", "--event-scheduler=OFF"]
        else:
            args += ["--publish", "127.0.0.1::3306", self.identity["id"], "--event-scheduler=OFF"]
        try:
            self.container = docker(*args, environment=environment)
            if not network:
                mapping = docker("port", self.container, "3306/tcp")
                if not re.fullmatch(r"127\.0\.0\.1:\d+", mapping):
                    raise RuntimeError("SQL fixture was not published exclusively on loopback")
                port = int(mapping.rsplit(":", 1)[1])
            self.environment.update(DB_PORT=str(port), TEST_DB_PORT=str(port))
            self.record("created", image=self.identity, container=self.container)
            deadline = time.monotonic() + 90
            while True:
                result = self.sql("SELECT 1", selected=False, check=False)
                if result.returncode == 0 and result.stdout.strip() == "1":
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError("owned SQL did not become authenticated-ready")
                time.sleep(0.5)
            version = self.sql("SELECT VERSION()", selected=False).stdout.strip()
            self.record("authenticated-ready", version=version)
            return self
        except BaseException:
            self.close()
            raise

    def sql(self, statement, *, selected=True, check=True):
        args = ["mysql", "--no-defaults", "--protocol=tcp", "--connect-timeout=2",
                "-h127.0.0.1", "-P" + self.environment["DB_PORT"], "-uroot", "-N", "-B"]
        if selected:
            args.append(self.environment["DB_NAME"])
        result = subprocess.run(args, input=statement, text=True, capture_output=True,
                                env=self.environment, timeout=120)
        self.record("sql", sha256=hashlib.sha256(statement.encode()).hexdigest(),
                    exit=result.returncode, error=self.redact(result.stderr))
        if check and result.returncode:
            raise RuntimeError("original SQL client failure: " + self.redact(result.stderr))
        return result

    @contextmanager
    def schema(self, prefix="economic_schema_test_", *, migrated=True, suffix=""):
        if not re.fullmatch(r"[a-z0-9_]+_", prefix):
            raise ValueError("invalid private schema prefix")
        if suffix not in {"", "_test"}:
            raise ValueError("invalid private schema suffix")
        database = prefix + secrets.token_hex(6) + suffix
        self.sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", selected=False)
        self.environment.update(DB_NAME=database, DB_ALLOWED_TARGETS="127.0.0.1/" + database,
                                ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1")
        try:
            if migrated:
                self.sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
                for command in (["python3", "scripts/migration_runner.py", "adopt", "--kind", "fresh_bootstrap"],
                                ["python3", "scripts/migration_runner.py", "run"],
                                ["python3", "scripts/migration_runner.py", "run"],
                                ["bash", "migrations/verify_runtime_compatibility.sh"]):
                    result = subprocess.run(command, cwd=ROOT, env=self.environment,
                                            text=True, capture_output=True, timeout=180)
                    self.record("migration", command=command, exit=result.returncode,
                                output=self.redact(result.stdout + result.stderr))
                    if result.returncode:
                        raise RuntimeError("fixture migration failure: " + self.redact(result.stdout + result.stderr))
            yield dict(self.environment)
        finally:
            # Retain schema metadata before cleanup, including on an assertion failure.
            snapshot = self.sql("SHOW TABLES", check=False)
            self.record("schema-before-cleanup", schema=database,
                        tables=snapshot.stdout.splitlines(), exit=snapshot.returncode)
            self.sql("DROP DATABASE " + database, selected=False)

    def close(self):
        if self.container:
            metadata = json.loads(docker("inspect", self.container))[0]
            if metadata["Config"]["Labels"].get(LABEL) != self.token:
                raise RuntimeError("refusing cleanup of unowned SQL container")
            logs = subprocess.run(["docker", "logs", self.container], text=True,
                                  capture_output=True, timeout=30, env=clean_environment())
            (self.evidence / "server.log").write_text(self.redact(logs.stdout + logs.stderr))
            docker("rm", "--force", self.container)
            self.record("removed", container=self.container)
            self.container = None

    def __exit__(self, *_):
        self.close()
