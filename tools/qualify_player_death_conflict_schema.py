#!/usr/bin/env python3
"""Measure and qualify the additive death-conflict schema on real DB engines.

This is intentionally scoped to this worktree. It uses disposable loopback-only
Docker databases, records redacted structural evidence outside the repository,
and updates only the new migration registration and generated schema pins.
"""
from __future__ import annotations

from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import secrets
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
TOOLS = Path("/opt/data/workspaces/duris-persistence-tools")
OUT = Path(tempfile.mkdtemp(prefix="death-conflict-schema-", dir=TOOLS))
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner as migration  # noqa: E402 # type: ignore[import-not-found]

APPLY = ROOT / "migrations/immutable/0034_player_death_conflict_evidence.sql"
VERIFY = ROOT / "migrations/immutable/0034_player_death_conflict_evidence.sh"
RUNTIME_VERIFY = ROOT / "migrations/verify_runtime_compatibility.sh"
RUNTIME_MANIFEST = ROOT / "migrations/runtime_compatibility_manifest.json"
HEADER = ROOT / "src/core/runtime_compatibility_contract.h"
MIGRATION_MANIFEST = ROOT / "migrations/migration_manifest.json"
LIFECYCLE_MANIFEST = ROOT / "migrations/data_lifecycle_manifest.json"
BOOTSTRAP = ROOT / "migrations/bootstrap_multithread_safe.sql"


def run(args, *, env=None, input=None, check=True, timeout=180):
    result = subprocess.run(args, env=env, input=input, text=True, capture_output=True,
                           timeout=timeout, cwd=ROOT)
    if check and result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {args[0]}\n"
                           f"{result.stdout}\n{result.stderr}")
    return result


def save(report):
    (OUT / "evidence.json").write_text(json.dumps(report, indent=2) + "\n")


class Fixture:
    def __init__(self, image):
        self.image = image
        self.key = "mariadb10_11" if image.startswith("mariadb:") else "mysql8"
        self.cid = None
        self.password = secrets.token_hex(32)
        self.db = "death_conflict_schema_test_" + secrets.token_hex(6)
        self.record = {"image": image, "engine_key": self.key, "database": self.db,
                       "cleanup_verified": False}
        self.env = dict(os.environ, ENVIRONMENT="test", MYSQL_PWD=self.password,
                        DB_HOST="127.0.0.1", DB_PORT="3306", DB_NAME=self.db,
                        DB_USER="root", DB_PASSWD=self.password, DB_SOCKET="",
                        RUNTIME_COMPATIBILITY_MANIFEST="/tmp/runtime-manifest.json")

    def start(self):
        password_key = "MARIADB_ROOT_PASSWORD" if self.key == "mariadb10_11" else "MYSQL_ROOT_PASSWORD"
        image_id = run(["docker", "image", "inspect", "--format={{.Id}}", self.image]).stdout.strip()
        self.cid = run(["docker", "run", "--pull=never", "-d", "--rm",
                        "--name", "duris-death-conflict-" + secrets.token_hex(6),
                        "-p", "127.0.0.1::3306", "-e", password_key, self.image],
                       env=dict(self.env, **{password_key: self.password})).stdout.strip()
        self.record.update(container_id=self.cid, image_id=image_id)
        print("FIXTURE_CREATED", json.dumps(self.record), flush=True)
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            result = self.sql("SELECT 1;", database=False, check=False)
            if result.returncode == 0 and result.stdout.strip() == "1":
                break
            time.sleep(1)
        else:
            raise RuntimeError("authenticated disposable database readiness failed")
        version = self.sql("SELECT VERSION();", database=False).stdout.strip()
        if self.key == "mariadb10_11":
            assert "MariaDB" in version and version.startswith("10.11."), version
        else:
            assert "MariaDB" not in version and version.startswith("8.0."), version
        self.record["server_version"] = version
        print("FIXTURE_VERSION", self.key, version, flush=True)
        self.sql(f"CREATE DATABASE `{self.db}` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;",
                 database=False)
        return self

    def sql(self, statement, *, database=True, check=True):
        args = ["docker", "exec", "-i", "-e", "MYSQL_PWD", self.cid, "mysql",
                "--no-defaults", "--protocol=tcp", "-h127.0.0.1", "-uroot", "-N", "-B", "--raw"]
        if database:
            args.append(self.db)
        return run(args, input=statement, env=self.env, check=check)

    def script(self, path, *, args=(), check=True):
        if self.cid is None:
            raise RuntimeError("disposable DB fixture is not started")
        dest = "/tmp/" + Path(path).name
        run(["docker", "cp", str(path), self.cid + ":" + dest])
        docker_env = ["ENVIRONMENT", "MYSQL_PWD", "DB_HOST", "DB_PORT", "DB_NAME",
                      "DB_USER", "DB_PASSWD", "DB_SOCKET", "RUNTIME_COMPATIBILITY_MANIFEST"]
        command = ["docker", "exec"]
        for key in docker_env:
            command += ["-e", key]
        return run(command + [self.cid, "bash", dest, *args], env=self.env, check=check)

    def copy_manifest(self, path):
        if self.cid is None:
            raise RuntimeError("disposable DB fixture is not started")
        run(["docker", "cp", str(path), self.cid + ":/tmp/runtime-manifest.json"])

    def close(self):
        if not self.cid:
            return
        run(["docker", "rm", "-f", self.cid], check=False)
        inspected = run(["docker", "container", "inspect", self.cid], check=False)
        absent = inspected.returncode != 0 and (
            "No such container" in inspected.stderr or "No such object" in inspected.stderr)
        self.record["cleanup_verified"] = absent
        print("FIXTURE_REMOVED", self.cid, absent, flush=True)
        if not absent:
            raise RuntimeError("disposable container cleanup was not verified")


def actual_table_pin(fixture: Fixture) -> str:
    """Measure the exact target table fingerprint from fresh bootstrap metadata."""
    count = fixture.sql(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_type='BASE TABLE' AND table_name='player_death_conflict_evidence';"
    ).stdout.strip()
    assert count == "1", f"fresh bootstrap omitted death-conflict table on {fixture.key}"
    source = VERIFY.read_text()
    marker = 'actual=$("${MYSQL[@]}" -e "$query" | sha256sum | cut -d\' \' -f1)'
    assert source.count(marker) == 1
    measure = OUT / "measure-death-conflict-table.sh"
    measure.write_text(source.split(marker, 1)[0] + marker
                       + '\nprintf "MEASURED=%s\\n" "$actual"\n')
    result = fixture.script(measure)
    match = re.search(r"^MEASURED=([0-9a-f]{64})$", result.stdout or "", re.M)
    assert match, result.stdout or ""
    return match.group(1)


def update_migration_checksums(table_pins):
    lines = VERIFY.read_text().splitlines()
    for engine, key in (("mariadb", "mariadb10_11"), ("mysql", "mysql8")):
        position = lines.index("    engine=" + engine)
        assert lines[position + 1].startswith("    expected=")
        lines[position + 1] = "    expected=" + table_pins[key]
    VERIFY.write_text("\n".join(lines) + "\n")
    manifest = json.loads(MIGRATION_MANIFEST.read_text())
    target = next(row for row in manifest["migrations"]
                  if row["id"] == "0034_player_death_conflict_evidence")
    target["apply_checksum"] = migration.checksum(APPLY.read_bytes())
    target["verify_checksum"] = migration.checksum(VERIFY.read_bytes())
    MIGRATION_MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n")


def update_header(values):
    text = HEADER.read_text()
    mappings = {
        "RUNTIME_CURRENT_TABLE_COUNT": values["current_table_count"],
        "RUNTIME_MYSQL8_METADATA_FINGERPRINT": values["normalized_metadata_fingerprints"]["mysql8"],
        "RUNTIME_MARIADB10_11_METADATA_FINGERPRINT": values["normalized_metadata_fingerprints"]["mariadb10_11"],
        "RUNTIME_MIGRATION_HEAD_ID": values["migration_head"]["id"],
        "RUNTIME_MIGRATION_HEAD_SEQUENCE": values["migration_head"]["sequence"],
        "RUNTIME_MIGRATION_APPLY_CHECKSUM": values["migration_head"]["apply_checksum"],
        "RUNTIME_MIGRATION_VERIFY_CHECKSUM": values["migration_head"]["verify_checksum"],
        "RUNTIME_MIGRATION_HISTORY_CHECKSUM": values["migration_head"]["history_checksum"],
    }
    for name, value in mappings.items():
        pattern = r"(" + re.escape(name) + r"\s*=\s*)(?:\"[^\"]*\"|[0-9]+)(;)"
        text, count = re.subn(pattern, lambda match: match[1] + json.dumps(value) + match[2], text)
        assert count == 1, (name, count)
    tokens = values["runtime_table_sql_list"].split(",")
    pieces, current = [], ""
    for index, token in enumerate(tokens):
        token += "," if index + 1 < len(tokens) else ""
        if len(current) + len(token) > 92:
            pieces.append(current)
            current = ""
        current += token
    pieces.append(current)
    literal = "\n".join("\t" + json.dumps(part) for part in pieces)
    text, count = re.subn(r"RUNTIME_TABLE_SQL_LIST\s*=\s*(?:\"[^\"]*\"\s*)+;",
                          lambda _: "RUNTIME_TABLE_SQL_LIST =\n" + literal + ";", text)
    assert count == 1
    anchor = " * Migration 0033 adds staged SQL lifecycle receipts, not gameplay activation.\n"
    evidence_line = " * Migration 0034 adds retained death-conflict evidence, not capture activation.\n"
    text = text.replace(evidence_line, "")
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + evidence_line)
    HEADER.write_text(text)


def seed_history(fixture: Fixture, manifest):
    quote = lambda value: "CONVERT(UNHEX('" + value.encode().hex() + "') USING utf8mb4)"
    applied = []
    statements = [
        "INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES("
        + quote(manifest.baseline_id) + ",'fresh_bootstrap',UNHEX('"
        + manifest.required_table_fingerprint + "')," + str(manifest.version) + ","
        + str(manifest.runner_version) + ");"
    ]
    for step in manifest.migrations:
        row = migration.AppliedMigration(step.migration_id, step.sequence, step.description,
                                         step.apply_checksum, step.verify_checksum,
                                         step.compatibility, manifest.runner_version)
        applied.append(row)
        values = [quote(step.migration_id), str(step.sequence), quote(step.description),
                  "UNHEX('" + step.apply_checksum + "')", "UNHEX('" + step.verify_checksum + "')",
                  quote(step.compatibility), str(manifest.runner_version)]
        statements.append(
            "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES("
            + ",".join(values) + ");")
    statements.append("UPDATE mud_schema_migration_state SET applied_count=" + str(len(applied))
                      + ",history_checksum=UNHEX('" + migration.history_checksum(applied)
                      + "') WHERE state_id=1;")
    fixture.sql("\n".join(statements))
    return applied


def make_measure_runtime_script():
    source = RUNTIME_VERIFY.read_text()
    marker = 'fingerprint=$("${MYSQL[@]}" -e "$query" | sha256sum | cut -d\' \' -f1)'
    assert source.count(marker) == 1
    path = OUT / "measure-runtime-compatibility.sh"
    path.write_text(source.split(marker, 1)[0] + marker
                    + '\nprintf "MEASURED=%s\\n" "$fingerprint"\n')
    return path


def measure_runtime_pin(fixture, values, provisional):
    provisional.write_text(json.dumps(values, indent=2) + "\n")
    fixture.copy_manifest(provisional)
    result = fixture.script(make_measure_runtime_script())
    match = re.search(r"^MEASURED=([0-9a-f]{64})$", result.stdout, re.M)
    assert match, result.stdout
    return match.group(1)


def test_apply_replay_probe_and_drift(fixture):
    # Fresh bootstrap parity: bootstrap's table has the measured exact signature.
    fixture.script(VERIFY)
    fixture.record["fresh_bootstrap_exact_verifier"] = "PASS"
    # Model a database immediately before migration 0034 by removing only the new
    # table from the otherwise complete disposable bootstrap schema.
    fixture.sql("DROP TABLE player_death_conflict_evidence;")
    fixture.sql(APPLY.read_text())
    fixture.script(VERIFY)
    operation = "00112233445566778899aabbccddeeff"
    request_hash = "aa" * 32
    payload_hash = "bb" * 32
    payload_hex = "7631302d666f72656e7369632d70726f6265"  # synthetic v10 fixture marker
    fixture.sql(
        "INSERT INTO player_death_conflict_evidence(operation_id,pid,save_revision,source_revision,"
        "corpse_item_uid,request_hash,payload_hash,payload) VALUES(UNHEX('" + operation
        + "'),321,90001,90000,80001,UNHEX('" + request_hash + "'),UNHEX('" + payload_hash
        + "'),UNHEX('" + payload_hex + "'));"
    )
    before = fixture.sql(
        "SELECT CONCAT(HEX(operation_id),'|',pid,'|',save_revision,'|',source_revision,'|',"
        "corpse_item_uid,'|',HEX(request_hash),'|',HEX(payload_hash),'|',HEX(payload)) "
        "FROM player_death_conflict_evidence WHERE operation_id=UNHEX('" + operation + "');"
    ).stdout.strip()
    assert before
    # CREATE IF NOT EXISTS replay must preserve the complete first observation.
    fixture.sql(APPLY.read_text())
    fixture.script(VERIFY)
    after = fixture.sql(
        "SELECT CONCAT(HEX(operation_id),'|',pid,'|',save_revision,'|',source_revision,'|',"
        "corpse_item_uid,'|',HEX(request_hash),'|',HEX(payload_hash),'|',HEX(payload)) "
        "FROM player_death_conflict_evidence WHERE operation_id=UNHEX('" + operation + "');"
    ).stdout.strip()
    assert after == before, "migration replay changed or removed the evidence probe row"
    # Deliberate same-table definition drift must fail closed; restore it and prove
    # the forensic row survives both ALTERs.
    fixture.sql("ALTER TABLE player_death_conflict_evidence MODIFY payload LONGBLOB NOT NULL;")
    rejected = fixture.script(VERIFY, check=False)
    assert rejected.returncode == 1 and "metadata fingerprint mismatch" in rejected.stderr
    fixture.sql("ALTER TABLE player_death_conflict_evidence MODIFY payload MEDIUMBLOB NOT NULL;")
    fixture.script(VERIFY)
    restored = fixture.sql(
        "SELECT CONCAT(HEX(operation_id),'|',pid,'|',save_revision,'|',source_revision,'|',"
        "corpse_item_uid,'|',HEX(request_hash),'|',HEX(payload_hash),'|',HEX(payload)) "
        "FROM player_death_conflict_evidence WHERE operation_id=UNHEX('" + operation + "');"
    ).stdout.strip()
    assert restored == before
    fixture.record["pre34_apply_replay"] = "PASS"
    fixture.record["first_observation_preserved"] = True
    fixture.record["exact_payload_type_drift_rejected_and_restored"] = True


def main():
    started = datetime.now(timezone.utc)
    report = {"status": "RUNNING", "started_at": started.isoformat(),
              "worktree": str(ROOT), "evidence_dir": str(OUT), "fixtures": []}
    fixtures = []
    save(report)
    print("EVIDENCE_FILE", OUT / "evidence.json", flush=True)
    try:
        # Explicitly record only locally available immutable image identities.
        for image in ("mariadb:10.11", "mysql:8.0"):
            fixture = Fixture(image)
            fixtures.append(fixture)
            report["fixtures"].append(fixture.record)
            fixture.start()
            save(report)
            fixture.sql(BOOTSTRAP.read_text())
        # Measure target table pins from the actual fresh-bootstrap schemas.
        table_pins = {fixture.key: actual_table_pin(fixture) for fixture in fixtures}
        update_migration_checksums(table_pins)
        report["migration_checksums_updated_from_worktree"] = True
        report["table_schema_fingerprints"] = table_pins
        for fixture in fixtures:
            fixture.record["death_conflict_schema_fingerprint"] = table_pins[fixture.key]
            test_apply_replay_probe_and_drift(fixture)
        save(report)

        manifest = migration.load_manifest()
        assert manifest.migrations[-1].migration_id == "0034_player_death_conflict_evidence"
        # Replay every immutable apply/verify pair over the fresh bootstrap. This
        # checks the registered migration chain without touching a non-disposable DB.
        for fixture in fixtures:
            completed = []
            for step in manifest.migrations:
                fixture.sql(step.apply_path.read_text())
                fixture.script(step.verify_path)
                completed.append(step.migration_id)
            fixture.record["verified_migration_count"] = len(completed)
            fixture.record["migration_head"] = completed[-1]
            seed_history(fixture, manifest)

        lifecycle = json.loads(LIFECYCLE_MANIFEST.read_text())
        tables = sorted(entry["locator"] for entry in lifecycle["entries"]
                        if entry["kind"] == "database_table")
        assert len(tables) == len(set(tables)) == 217, len(tables)
        values = json.loads(RUNTIME_MANIFEST.read_text())
        values["current_table_count"] = len(tables)
        values["runtime_table_sql_list"] = ",".join("'" + name + "'" for name in tables)
        head = manifest.migrations[-1]
        applied = [migration.AppliedMigration(step.migration_id, step.sequence,
                   step.description, step.apply_checksum, step.verify_checksum,
                   step.compatibility, manifest.runner_version) for step in manifest.migrations]
        values["migration_head"] = {"id": head.migration_id, "sequence": head.sequence,
            "apply_checksum": head.apply_checksum, "verify_checksum": head.verify_checksum,
            "history_checksum": migration.history_checksum(applied)}
        provisional = OUT / "runtime-manifest-provisional.json"
        values["normalized_metadata_fingerprints"] = {
            "mysql8": "0" * 64, "mariadb10_11": "0" * 64}
        for fixture in fixtures:
            fingerprint = measure_runtime_pin(fixture, values, provisional)
            values["normalized_metadata_fingerprints"][fixture.key] = fingerprint
            fixture.record["normalized_runtime_fingerprint"] = fingerprint
            print("MEASURED_RUNTIME_PIN", fixture.key, fingerprint, flush=True)
            save(report)
        RUNTIME_MANIFEST.write_text(json.dumps(values, indent=2) + "\n")
        update_header(values)
        report["runtime_manifest_and_generated_header_updated"] = True

        # Run the actual runtime schema gate against both engines with pins measured
        # above and its migration/baseline history seeded from the manifest.
        for fixture in fixtures:
            fixture.copy_manifest(RUNTIME_MANIFEST)
            fixture.script(RUNTIME_VERIFY, args=("--schema-only",))
            fixture.record["runtime_schema_gate"] = "PASS"
            save(report)

        # Repository-level validators cross-check migration, lifecycle, manifest,
        # runtime pins, and generated header after all measured writes.
        run(["python3", str(ROOT / "scripts/validate_runtime_compatibility.py")])
        run(["python3", str(ROOT / "scripts/validate_data_lifecycle.py"),
             "--manifest", str(LIFECYCLE_MANIFEST), "--json"])
        report["offline_contract_validators"] = "PASS"
        report["status"] = "PASS"
    except Exception as error:
        report["status"] = "FAIL"
        report["error"] = str(error)
        raise
    finally:
        for fixture in fixtures:
            try:
                fixture.close()
            except Exception as error:
                report.setdefault("cleanup_errors", []).append(str(error))
                report["status"] = "FAIL"
        report["finished_at"] = datetime.now(timezone.utc).isoformat()
        report["elapsed_seconds"] = (datetime.now(timezone.utc) - started).total_seconds()
        report["fixtures"] = [fixture.record for fixture in fixtures]
        save(report)
        print("FINAL_EVIDENCE", json.dumps(report), flush=True)
    if report["status"] != "PASS":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
