#!/usr/bin/env python3
"""Manual C05 qualification on an explicitly disposable MySQL/MariaDB schema.

The populated case starts at migration history 0035, creates one genuine staged
installation through the lifecycle owner fixture helper, snapshots all seeded,
owner-created, and staged accounting rows, applies/replays 0036, and compares the
snapshots. The constraints case requires that canonical 0036 is already present and tests its
FK/CHECK refusal behavior. This is intentionally not named test_*.py and is not
for generic test discovery.

The disposable marker and schema prefix are guards only; they do not prove target
identity or fixture provenance. The parent qualification driver must provision a
fresh exact disposable container, independently verify its container/image/network
identity and connected database engine/schema identity before mutation, then execute
the real lifecycle owner against that same database while hashing its binary and
retaining/logging its exact PASS output. `--staged-fixture-ready` is SQL transport
coordination only, not owner proof. The parent owns these checks.

Parent handoff: build tests/async/economic_sql_activation_receipt_upgrade_fixture.cpp
with the source list from run_economic_sql_lifecycle_owner_mysql.sh into
bin/tests/economic-sql-activation-receipt-upgrade-fixture, prepare a disposable
schema through 0035, then invoke with --case all. Inject ENVIRONMENT=test,
DB_HOST=127.0.0.1, DB_PORT, DB_USER, DB_PASSWD, and DB_NAME matching
`economic_receipt_test_...`. Set ECONOMIC_SQL_ACTIVATION_RECEIPT_DISPOSABLE_SCHEMA=1.
Never source .env; unset DB_SOCKET and MIGRATION_ENV_FILE. Do not use a shared/live database.

For container-local SQL/verifier transport, set
ECONOMIC_SQL_ACTIVATION_RECEIPT_TRANSPORT=docker and
ECONOMIC_SQL_ACTIVATION_RECEIPT_CONTAINER to the explicit disposable DB
container, use DB_HOST=127.0.0.1 and DB_PORT=3306, and pass
--staged-fixture-ready only after the parent has verified the target and retained
the real lifecycle owner's exact output/hash for that database. The flag does not
attest provenance. SQL/verifier credentials are forwarded to docker exec by
variable name; this mode does not provision or clean up the supplied container.
"""

from __future__ import annotations

import argparse
from contextlib import contextmanager
import os
from pathlib import Path
import re
import secrets
import shutil
import subprocess
import sys
from dataclasses import dataclass


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sql"
RECEIPT_VERIFIER = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sh"
HISTORICAL_VERIFIER = ROOT / "migrations/immutable/0033_economic_sql_lifecycle_owner.sh"
FIXTURE_HELPER = ROOT / "bin/tests/economic-sql-activation-receipt-upgrade-fixture"
SCHEMA_PREFIX = "economic_receipt_test_"
DISPOSABLE_MARKER = "ECONOMIC_SQL_ACTIVATION_RECEIPT_DISPOSABLE_SCHEMA"

FK_DDL = {
    "fk_economic_sql_activation_installation": (
        "ADD CONSTRAINT fk_economic_sql_activation_installation "
        "FOREIGN KEY (operation_id,lineage,epoch,baseline_operation_id) "
        "REFERENCES economic_sql_lifecycle_installation "
        "(operation_id,lineage,epoch,baseline_operation_id) "
        "ON UPDATE RESTRICT ON DELETE RESTRICT"
    ),
    "fk_economic_sql_activation_epoch": (
        "ADD CONSTRAINT fk_economic_sql_activation_epoch "
        "FOREIGN KEY (lineage,epoch) REFERENCES economic_epoch (lineage,epoch) "
        "ON UPDATE RESTRICT ON DELETE RESTRICT"
    ),
    "fk_economic_sql_activation_baseline_revision": (
        "ADD CONSTRAINT fk_economic_sql_activation_baseline_revision "
        "FOREIGN KEY (lineage,epoch,baseline_revision) "
        "REFERENCES economic_baseline_witness (lineage,epoch,book_revision) "
        "ON UPDATE RESTRICT ON DELETE RESTRICT"
    ),
    "fk_economic_sql_activation_operation_inbox": (
        "ADD CONSTRAINT fk_economic_sql_activation_operation_inbox "
        "FOREIGN KEY (operation_id) REFERENCES critical_operation_inbox (operation_id) "
        "ON UPDATE RESTRICT ON DELETE RESTRICT"
    ),
    "fk_economic_sql_activation_baseline_inbox": (
        "ADD CONSTRAINT fk_economic_sql_activation_baseline_inbox "
        "FOREIGN KEY (baseline_operation_id) "
        "REFERENCES critical_operation_inbox (operation_id) "
        "ON UPDATE RESTRICT ON DELETE RESTRICT"
    ),
}

CHECK_NAMES = (
    "ck_economic_sql_activation_operation_nonzero",
    "ck_economic_sql_activation_lineage_nonzero",
    "ck_economic_sql_activation_epoch_nonzero",
    "ck_economic_sql_activation_baseline_operation_nonzero",
    "ck_economic_sql_activation_baseline_revision",
    "ck_economic_sql_activation_scope",
    "ck_economic_sql_activation_coverage_version",
    "ck_economic_sql_activation_receipt_version",
)
ZERO_ID = "0" * 32
SNAPSHOT_TABLES = (
    "accounts",
    "account_banks",
    "player_data",
    "critical_operation_inbox",
    "economic_lineage_state",
    "economic_epoch",
    "economic_account_mapping",
    "economic_sql_lifecycle_installation",
    "economic_baseline_control",
    "economic_baseline_witness",
    "economic_baseline_reservation",
    "economic_accounting_operation",
    "economic_accounting_account_effect",
    "economic_accounting_coin_posting",
    "economic_accounting_source_claim",
)


class QualificationError(RuntimeError):
    pass


@dataclass(frozen=True)
class Target:
    host: str
    port: str
    user: str
    password: str
    database: str
    mysql: str
    docker: str
    transport: str
    container: str
    child_env: dict[str, str]

    @classmethod
    def from_environment(cls) -> "Target":
        if os.environ.get("ENVIRONMENT") != "test":
            raise QualificationError("ENVIRONMENT=test is required")
        if os.environ.get(DISPOSABLE_MARKER) != "1":
            raise QualificationError(f"{DISPOSABLE_MARKER}=1 is required")
        if "DB_SOCKET" in os.environ:
            raise QualificationError("DB_SOCKET must be unset; TCP only is allowed")

        host = os.environ.get("DB_HOST", "")
        if host not in {"127.0.0.1", "::1", "localhost"}:
            raise QualificationError("DB_HOST must be an explicitly disposable loopback target")
        database = os.environ.get("DB_NAME", "")
        if (len(database) > 64 or
                not re.fullmatch(r"economic_receipt_test_[A-Za-z0-9_]{1,48}", database)):
            raise QualificationError(f"DB_NAME must begin {SCHEMA_PREFIX!r}")
        port = os.environ.get("DB_PORT", "")
        if not port.isdecimal() or not 1 <= int(port) <= 65535:
            raise QualificationError("DB_PORT must be an explicit TCP port")
        user = os.environ.get("DB_USER", "")
        password = os.environ.get("DB_PASSWD", "")
        if not user or not re.fullmatch(r"[A-Za-z0-9_.-]{1,64}", user) or not password:
            raise QualificationError("explicit DB_USER and DB_PASSWD are required")
        transport = os.environ.get("ECONOMIC_SQL_ACTIVATION_RECEIPT_TRANSPORT", "tcp")
        if transport not in {"tcp", "docker"}:
            raise QualificationError("transport must be tcp or docker")
        docker = ""
        container = ""
        mysql = ""
        if transport == "docker":
            container = os.environ.get("ECONOMIC_SQL_ACTIVATION_RECEIPT_CONTAINER", "")
            if (host != "127.0.0.1" or port != "3306" or
                    not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,63}", container)):
                raise QualificationError(
                    "docker transport requires DB_HOST=127.0.0.1, DB_PORT=3306, "
                    "and an explicit container name"
                )
            docker = shutil.which("docker") or ""
            if not docker:
                raise QualificationError("docker client is unavailable for container-local transport")
        else:
            mysql = shutil.which("mysql") or ""
            if not mysql:
                raise QualificationError("mysql client is unavailable")

        # Do not inherit hidden DB socket/credential overrides. MYSQL_PWD is
        # passed through the child environment, never in an argv or log line.
        child_env = {
            key: value for key, value in os.environ.items()
            if key not in {"DB_SOCKET", "MIGRATION_ENV_FILE", "MYSQL_TEST_LOGIN_FILE", "MYSQL_HOME"}
        }
        child_env.update({
            "DB_HOST": host,
            "DB_PORT": port,
            "DB_USER": user,
            "DB_PASSWD": password,
            "DB_NAME": database,
            "ENVIRONMENT": "test",
            DISPOSABLE_MARKER: "1",
            "MYSQL_PWD": password,
        })
        return cls(host, port, user, password, database, mysql, docker, transport,
                   container, child_env)

    def docker_exec_argv(self) -> list[str]:
        return [
            self.docker, "exec", "-i",
            "-e", "MYSQL_PWD", "-e", "DB_HOST", "-e", "DB_PORT", "-e", "DB_USER",
            "-e", "DB_PASSWD", "-e", "DB_NAME", "-e", "ENVIRONMENT",
            "-e", DISPOSABLE_MARKER, self.container,
        ]

    def mysql_argv(self) -> list[str]:
        client = [
            "mysql", "--no-defaults", "--connect-timeout=5", "--protocol=tcp",
            "-h", self.host, "-P", self.port, "-u", self.user,
            "-N", "-B", "--raw", self.database,
        ]
        if self.transport == "docker":
            return [*self.docker_exec_argv(), *client]
        return [self.mysql, *client[1:]]

    def _safe_detail(self, text: str) -> str:
        for secret in (self.password, self.user, self.database):
            if secret:
                text = text.replace(secret, "<redacted>")
        return text.strip()

    def sql_result(self, statement: str) -> tuple[int, str]:
        try:
            result = subprocess.run(
                [*self.mysql_argv(), "-e", statement],
                env=self.child_env,
                text=True,
                capture_output=True,
                timeout=20,
                check=False,
            )
        except subprocess.TimeoutExpired as error:
            raise QualificationError("bounded mysql command timed out") from error
        except OSError as error:
            raise QualificationError("mysql/docker executable invocation failed") from error
        return result.returncode, result.stdout + result.stderr

    def sql(self, statement: str) -> str:
        returncode, output = self.sql_result(statement)
        if returncode:
            raise QualificationError(
                f"mysql query failed: {self._safe_detail(output) or 'no diagnostic'}"
            )
        return output.strip()

    def expect_constraint_rejection(
        self, statement: str, expected_constraint: str, operation_id: str
    ) -> None:
        returncode, output = self.sql_result(statement)
        if returncode == 0:
            # If a broken constraint accepts this disposable-only probe, remove
            # precisely its synthetic row before reporting the failed proof.
            self.sql(
                "DELETE FROM economic_sql_activation_receipt WHERE operation_id=" + operation_id
            )
            raise QualificationError(f"constraint probe was accepted: {expected_constraint}")
        if expected_constraint.lower() not in output.lower():
            raise QualificationError(
                f"probe failed for an unexpected constraint; expected {expected_constraint}: "
                f"{self._safe_detail(output)}"
            )

    def sql_file(self, path: Path) -> None:
        try:
            payload = path.read_bytes()
            result = subprocess.run(
                self.mysql_argv(),
                input=payload,
                env=self.child_env,
                capture_output=True,
                timeout=90,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired) as error:
            raise QualificationError(f"bounded SQL apply failed for {path.name}") from error
        if result.returncode:
            detail = (result.stdout + result.stderr).decode(errors="replace")
            raise QualificationError(
                f"SQL apply failed for {path.name}: {self._safe_detail(detail) or 'no diagnostic'}"
            )

    def run_script(self, path: Path, label: str, *, expect_rejection: str | None = None) -> str:
        command = [str(path)]
        payload = None
        text_output = True
        if self.transport == "docker":
            command = [*self.docker_exec_argv(), "bash", "-s"]
            payload = path.read_bytes()
            text_output = False
        try:
            result = subprocess.run(
                command,
                input=payload,
                env=self.child_env,
                text=text_output,
                capture_output=True,
                timeout=45,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired) as error:
            raise QualificationError(f"bounded {label} verifier invocation failed") from error
        if text_output:
            output = result.stdout + result.stderr
        else:
            output = (result.stdout + result.stderr).decode(errors="replace")
        if expect_rejection is None:
            if result.returncode:
                raise QualificationError(
                    f"{label} verifier failed: "
                    f"{self._safe_detail(output) or 'no diagnostic'}"
                )
            return result.stdout.strip() if text_output else result.stdout.decode(errors="replace").strip()
        if result.returncode == 0 or expect_rejection.lower() not in output.lower():
            raise QualificationError(
                f"{label} verifier did not refuse {expect_rejection}: "
                f"{self._safe_detail(output) or 'no diagnostic'}"
            )
        return output

    def run_fixture_helper(self) -> None:
        if self.transport == "docker":
            raise QualificationError(
                "container-local SQL transport needs a pre-staged owner fixture; "
                "use --staged-fixture-ready after the actual lifecycle owner creates it"
            )
        if not FIXTURE_HELPER.is_file() or not os.access(FIXTURE_HELPER, os.X_OK):
            raise QualificationError(
                "owner fixture helper is missing; parent must compile the new C++ fixture "
                "to bin/tests/economic-sql-activation-receipt-upgrade-fixture"
            )
        try:
            result = subprocess.run(
                [str(FIXTURE_HELPER)],
                env=self.child_env,
                text=True,
                capture_output=True,
                timeout=120,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired) as error:
            raise QualificationError("bounded lifecycle owner fixture invocation failed") from error
        output = result.stdout + result.stderr
        if result.returncode:
            raise QualificationError(
                "lifecycle owner fixture failed; discard this disposable schema: "
                f"{self._safe_detail(output) or 'no diagnostic'}"
            )
        print("owner fixture: staged baseline created through lifecycle transaction")


def require_equal(label: str, actual: str, expected: str) -> None:
    if actual != expected:
        raise QualificationError(f"{label} mismatch: expected {expected}, got {actual}")


def verify_engine(target: Target) -> str:
    version = target.sql("SELECT VERSION()")
    if "MariaDB" in version and version.startswith("10.11."):
        engine = "mariadb-10.11"
    elif "MariaDB" not in version and version.startswith("8.0."):
        engine = "mysql-8.0"
    else:
        raise QualificationError("only MySQL 8.0 and MariaDB 10.11 are supported")
    print(f"engine={engine}")
    return engine


def verify_history_0035(target: Target) -> None:
    require_equal(
        "migration state head",
        target.sql("SELECT applied_count FROM mud_schema_migration_state WHERE state_id=1"),
        "35",
    )
    require_equal(
        "migration history count/min/max",
        target.sql(
            "SELECT COUNT(*),MIN(sequence_number),MAX(sequence_number) FROM mud_schema_history"
        ),
        "35\t1\t35",
    )
    require_equal(
        "migration 0035 id",
        target.sql(
            "SELECT migration_id FROM mud_schema_history WHERE sequence_number=35"
        ),
        "0035_player_item_dynamic_state",
    )
    require_equal(
        "migration 0036 absent from history",
        target.sql(
            "SELECT COUNT(*) FROM mud_schema_history WHERE sequence_number=36 "
            "OR migration_id='0036_economic_sql_activation_receipt'"
        ),
        "0",
    )


def require_pre_0036_shape(target: Target) -> None:
    require_equal(
        "receipt table absent before migration",
        target.sql(
            "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
            "AND table_name='economic_sql_activation_receipt'"
        ),
        "0",
    )
    require_equal(
        "0036 parent index absent before migration",
        target.sql(
            "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() "
            "AND table_name='economic_sql_lifecycle_installation' "
            "AND index_name='uq_economic_sql_lifecycle_activation_binding'"
        ),
        "0",
    )


def fixture_values(target: Target) -> tuple[str, str, str, str, int]:
    require_equal(
        "one staged installation",
        target.sql(
            "SELECT COUNT(*) FROM economic_sql_lifecycle_installation "
            "WHERE phase=2 AND selected_epoch=epoch AND baseline_operation_id IS NOT NULL"
        ),
        "1",
    )
    require_equal(
        "one total lifecycle installation",
        target.sql("SELECT COUNT(*) FROM economic_sql_lifecycle_installation"),
        "1",
    )
    require_equal(
        "valid staged installation/baseline/control/witness/epoch binding",
        target.sql(
            "SELECT COUNT(*) FROM economic_sql_lifecycle_installation i "
            "JOIN economic_baseline_control c ON c.lineage=i.lineage AND c.epoch=i.epoch "
            "JOIN economic_baseline_witness w ON w.operation_id=i.baseline_operation_id "
            "AND w.lineage=i.lineage AND w.epoch=i.epoch AND w.book_revision=c.revision "
            "JOIN economic_epoch e ON e.lineage=i.lineage AND e.epoch=i.epoch "
            "WHERE i.phase=2 AND i.selected_epoch=i.epoch AND i.baseline_operation_id IS NOT NULL"
        ),
        "1",
    )
    require_equal(
        "active epoch remains NULL",
        target.sql(
            "SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL"
        ),
        "0",
    )
    if target.sql(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_name='economic_sql_activation_receipt'"
    ) == "1":
        require_equal(
            "receipt table empty",
            target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt"),
            "0",
        )
    row = target.sql(
        "SELECT LOWER(HEX(i.operation_id)),LOWER(HEX(i.lineage)),LOWER(HEX(i.epoch)),"
        "LOWER(HEX(i.baseline_operation_id)),w.book_revision "
        "FROM economic_sql_lifecycle_installation i "
        "JOIN economic_baseline_control c ON c.lineage=i.lineage AND c.epoch=i.epoch "
        "JOIN economic_baseline_witness w ON w.operation_id=i.baseline_operation_id "
        "AND w.lineage=i.lineage AND w.epoch=i.epoch AND w.book_revision=c.revision "
        "JOIN economic_epoch e ON e.lineage=i.lineage AND e.epoch=i.epoch "
        "WHERE i.phase=2 AND i.selected_epoch=i.epoch"
    )
    fields = row.split("\t")
    if len(fields) != 5 or any(not re.fullmatch(r"[0-9a-f]{32}", value) for value in fields[:4]):
        raise QualificationError("staged owner fixture identity has an unexpected shape")
    revision = int(fields[4])
    if revision <= 0:
        raise QualificationError("baseline witness revision must be positive")
    return fields[0], fields[1], fields[2], fields[3], revision


def snapshot(target: Target) -> tuple[str, ...]:
    table_list = ",".join(f"'{table}'" for table in SNAPSHOT_TABLES)
    column_rows = target.sql(
        "SELECT TABLE_NAME,COLUMN_NAME FROM information_schema.columns "
        f"WHERE table_schema=DATABASE() AND table_name IN ({table_list}) "
        "ORDER BY TABLE_NAME,ORDINAL_POSITION"
    )
    columns_by_table = {table: [] for table in SNAPSHOT_TABLES}
    for metadata_row in column_rows.splitlines():
        fields = metadata_row.split("\t")
        if (len(fields) != 2 or fields[0] not in columns_by_table or
                not re.fullmatch(r"[A-Za-z0-9_]+", fields[1])):
            raise QualificationError("snapshot schema returned malformed column metadata")
        columns_by_table[fields[0]].append(fields[1])

    key_rows = target.sql(
        "SELECT TABLE_NAME,COLUMN_NAME FROM information_schema.statistics "
        f"WHERE table_schema=DATABASE() AND table_name IN ({table_list}) "
        "AND index_name='PRIMARY' ORDER BY TABLE_NAME,SEQ_IN_INDEX"
    )
    keys_by_table = {table: [] for table in SNAPSHOT_TABLES}
    for metadata_row in key_rows.splitlines():
        fields = metadata_row.split("\t")
        if len(fields) != 2 or fields[0] not in keys_by_table:
            raise QualificationError("snapshot schema returned malformed primary-key metadata")
        keys_by_table[fields[0]].append(fields[1])

    result = []
    for table in SNAPSHOT_TABLES:
        columns = columns_by_table[table]
        primary_key = keys_by_table[table]
        if (not columns or any(column not in columns for column in primary_key) or
                not primary_key):
            raise QualificationError(f"snapshot schema has no valid columns/primary key for {table}")
        encoded_columns = ",".join(
            "IF(`{column}` IS NULL,NULL,CONCAT('V',HEX(CAST(`{column}` AS BINARY))))".format(
                column=column
            )
            for column in columns
        )
        order_by = ",".join(f"`{column}`" for column in primary_key)
        output = target.sql(
            f"SELECT JSON_ARRAY({encoded_columns}) FROM `{table}` ORDER BY {order_by}"
        )
        rows = output.splitlines() if output else []
        header = f"<{table} columns={','.join(columns)} order={','.join(primary_key)}>"
        result.append(header + "\n" + "\n".join(rows))
    return tuple(result)


def assert_no_runtime_authority(target: Target) -> None:
    require_equal(
        "active epoch remains NULL",
        target.sql("SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL"),
        "0",
    )
    if target.sql(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_name='economic_sql_activation_receipt'"
    ) == "1":
        require_equal(
            "activation receipt remains empty",
            target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt"),
            "0",
        )


def run_populated(target: Target, staged_fixture_ready: bool) -> tuple[str, ...]:
    print("case=populated-upgrade-replay")
    verify_history_0035(target)
    require_pre_0036_shape(target)
    target.run_script(HISTORICAL_VERIFIER, "immutable-0033 historical checkpoint")
    if staged_fixture_ready:
        print("owner fixture: validating caller-prepared staged baseline")
    else:
        target.run_fixture_helper()
    fixture_values(target)
    before = snapshot(target)
    target.sql_file(MIGRATION)
    target.run_script(RECEIPT_VERIFIER, "0036 canonical")
    after_first_apply = snapshot(target)
    if before != after_first_apply:
        raise QualificationError("0036 changed complete fixture/owner snapshot")
    assert_no_runtime_authority(target)
    target.sql_file(MIGRATION)
    target.run_script(RECEIPT_VERIFIER, "0036 direct replay")
    after_replay = snapshot(target)
    if before != after_replay:
        raise QualificationError("direct 0036 replay changed complete fixture/owner snapshot")
    assert_no_runtime_authority(target)
    print("PASS populated_rows_preserved=true direct_replay_stable=true active_epoch=NULL receipt_rows=0")
    return before


def receipt_insert_sql(values: dict[str, str]) -> str:
    fields = (
        "operation_id", "lineage", "epoch", "baseline_operation_id", "baseline_revision",
        "source_capture_digest", "native_boundary_digest", "activation_scope",
        "coverage_contract_version", "coverage_evidence_digest", "activation_digest",
        "receipt_version",
    )
    return (
        "INSERT INTO economic_sql_activation_receipt (" + ",".join(fields) + ") VALUES (" +
        ",".join(values[field] for field in fields) + ")"
    )


def id_literal(value: str) -> str:
    if not re.fullmatch(r"[0-9a-f]{32}", value):
        raise QualificationError("internal synthetic ID is malformed")
    return f"X'{value}'"


@contextmanager
def without_foreign_keys(target: Target, names: tuple[str, ...]):
    dropped: list[str] = []
    try:
        for name in names:
            if name not in FK_DDL:
                raise QualificationError("internal FK probe name is not allow-listed")
            target.sql(
                "ALTER TABLE economic_sql_activation_receipt DROP FOREIGN KEY " + name
            )
            dropped.append(name)
        yield
    finally:
        for name in reversed(dropped):
            target.sql("ALTER TABLE economic_sql_activation_receipt " + FK_DDL[name])


def expect_fk_refusal(target: Target, label: str, constraint: str, values: dict[str, str]) -> None:
    operation_id = values["operation_id"]
    target.expect_constraint_rejection(receipt_insert_sql(values), constraint, operation_id)
    if target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt") != "0":
        raise QualificationError(f"{label} left a synthetic receipt row behind")


def expect_check_refusal(
    target: Target, label: str, check_name: str, values: dict[str, str], drop: tuple[str, ...] = ()
) -> None:
    with without_foreign_keys(target, drop):
        target.expect_constraint_rejection(
            receipt_insert_sql(values), check_name, values["operation_id"]
        )
        if target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt") != "0":
            raise QualificationError(f"{label} left a synthetic receipt row behind")


def base_probe_values(fixture: tuple[str, str, str, str, int]) -> dict[str, str]:
    operation_id, lineage, epoch, baseline_operation_id, revision = fixture
    return {
        "operation_id": id_literal(operation_id),
        "lineage": id_literal(lineage),
        "epoch": id_literal(epoch),
        "baseline_operation_id": id_literal(baseline_operation_id),
        "baseline_revision": str(revision),
        "source_capture_digest": "UNHEX(REPEAT('11',32))",
        "native_boundary_digest": "UNHEX(REPEAT('22',32))",
        "activation_scope": "1",
        "coverage_contract_version": "1",
        "coverage_evidence_digest": "UNHEX(REPEAT('33',32))",
        "activation_digest": "UNHEX(REPEAT('44',32))",
        "receipt_version": "1",
    }


def fresh_operation_id() -> str:
    return secrets.token_hex(16)


def missing_inbox_operation_id(target: Target) -> str:
    value = fresh_operation_id()
    while target.sql(
        "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" + id_literal(value)
    ) != "0":
        value = fresh_operation_id()
    return value


def run_schema_refusals(target: Target) -> None:
    print("case=canonical-verifier-drift")
    target.run_script(RECEIPT_VERIFIER, "0036 pre-verifier-negative canonical")
    target.sql(
        "CREATE INDEX c05_receipt_qualification_probe "
        "ON economic_sql_activation_receipt(receipt_version)"
    )
    try:
        target.run_script(
            RECEIPT_VERIFIER, "0036 extra-index probe", expect_rejection="receipt-indexes"
        )
    finally:
        target.sql(
            "DROP INDEX c05_receipt_qualification_probe ON economic_sql_activation_receipt"
        )
        target.run_script(RECEIPT_VERIFIER, "0036 after extra-index restoration")

    target.sql(
        "ALTER TABLE economic_sql_activation_receipt DROP FOREIGN KEY "
        "fk_economic_sql_activation_installation"
    )
    try:
        target.run_script(
            RECEIPT_VERIFIER, "0036 missing-FK probe", expect_rejection="foreign-keys"
        )
    finally:
        target.sql_file(MIGRATION)
        target.run_script(RECEIPT_VERIFIER, "0036 after FK replay restoration")


def run_receipt_constraint_probes(
    target: Target, fixture: tuple[str, str, str, str, int]
) -> None:
    base = base_probe_values(fixture)

    # The real owner-created installation, epoch, baseline witness and inbox
    # rows are used as parents. Each rejected INSERT is one-shot and must leave
    # no receipt. In particular no probe sets active_epoch or inserts a success.
    wrong_binding = dict(base)
    wrong_binding["baseline_operation_id"] = base["operation_id"]
    expect_fk_refusal(
        target, "installation composite binding", "fk_economic_sql_activation_installation",
        wrong_binding,
    )

    bad_epoch = dict(base)
    bad_epoch_id = fresh_operation_id()
    while target.sql(
        "SELECT COUNT(*) FROM economic_epoch WHERE lineage=" + base["lineage"] +
        " AND epoch=" + id_literal(bad_epoch_id)
    ) != "0":
        bad_epoch_id = fresh_operation_id()
    bad_epoch["epoch"] = id_literal(bad_epoch_id)
    with without_foreign_keys(
        target,
        ("fk_economic_sql_activation_installation", "fk_economic_sql_activation_baseline_revision"),
    ):
        expect_fk_refusal(target, "epoch reference", "fk_economic_sql_activation_epoch", bad_epoch)

    missing_revision = dict(base)
    missing_revision["baseline_revision"] = str(fixture[4] + 1)
    expect_fk_refusal(
        target, "baseline witness revision", "fk_economic_sql_activation_baseline_revision",
        missing_revision,
    )

    missing_operation = dict(base)
    missing_operation["operation_id"] = id_literal(missing_inbox_operation_id(target))
    with without_foreign_keys(target, ("fk_economic_sql_activation_installation",)):
        expect_fk_refusal(
            target, "activation operation inbox", "fk_economic_sql_activation_operation_inbox",
            missing_operation,
        )

    missing_baseline_operation = dict(base)
    missing_baseline_operation["baseline_operation_id"] = id_literal(missing_inbox_operation_id(target))
    with without_foreign_keys(target, ("fk_economic_sql_activation_installation",)):
        expect_fk_refusal(
            target, "baseline operation inbox", "fk_economic_sql_activation_baseline_inbox",
            missing_baseline_operation,
        )

    bad = dict(base)
    bad["operation_id"] = id_literal(ZERO_ID)
    bad["source_capture_digest"] = "UNHEX(REPEAT('55',32))"
    expect_check_refusal(
        target, "zero operation ID", CHECK_NAMES[0], bad,
        ("fk_economic_sql_activation_installation", "fk_economic_sql_activation_operation_inbox"),
    )

    bad = dict(base)
    bad["lineage"] = id_literal(ZERO_ID)
    expect_check_refusal(
        target, "zero lineage", CHECK_NAMES[1], bad,
        ("fk_economic_sql_activation_installation", "fk_economic_sql_activation_epoch",
         "fk_economic_sql_activation_baseline_revision"),
    )

    bad = dict(base)
    bad["epoch"] = id_literal(ZERO_ID)
    expect_check_refusal(
        target, "zero epoch", CHECK_NAMES[2], bad,
        ("fk_economic_sql_activation_installation", "fk_economic_sql_activation_epoch",
         "fk_economic_sql_activation_baseline_revision"),
    )

    bad = dict(base)
    bad["baseline_operation_id"] = id_literal(ZERO_ID)
    expect_check_refusal(
        target, "zero baseline operation ID", CHECK_NAMES[3], bad,
        ("fk_economic_sql_activation_installation", "fk_economic_sql_activation_baseline_inbox"),
    )

    bad = dict(base)
    bad["baseline_revision"] = "0"
    expect_check_refusal(
        target, "nonpositive baseline revision", CHECK_NAMES[4], bad,
        ("fk_economic_sql_activation_baseline_revision",),
    )

    for check_index, field, invalid in (
        (5, "activation_scope", "2"),
        (6, "coverage_contract_version", "2"),
        (7, "receipt_version", "2"),
    ):
        bad = dict(base)
        bad[field] = invalid
        expect_check_refusal(target, field, CHECK_NAMES[check_index], bad)

def run_constraint_refusals(target: Target) -> None:
    print("case=receipt-fk-check-refusals")
    target.run_script(RECEIPT_VERIFIER, "0036 pre-negative canonical")
    fixture = fixture_values(target)
    require_equal(
        "receipt table empty before probes",
        target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt"),
        "0",
    )
    before = snapshot(target)
    try:
        run_receipt_constraint_probes(target, fixture)
    finally:
        # Restore through the idempotent migration even after a failed probe;
        # the target is disposable and no synthetic row may be left behind.
        target.sql_file(MIGRATION)
        target.run_script(RECEIPT_VERIFIER, "0036 post-negative canonical restoration")
    require_equal(
        "receipt table empty after probes",
        target.sql("SELECT COUNT(*) FROM economic_sql_activation_receipt"),
        "0",
    )
    if before != snapshot(target):
        raise QualificationError("constraint probes changed complete fixture/owner snapshot")
    assert_no_runtime_authority(target)
    print("PASS fk_refusals=5 check_refusals=8 canonical_shape_restored=true receipt_rows=0")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--case", choices=("populated", "constraints", "all"), default="all",
        help="populated starts at history 0035; constraints needs canonical 0036",
    )
    parser.add_argument(
        "--staged-fixture-ready", action="store_true",
        help="use parent-staged SQL fixture only; this flag is not owner proof",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        target = Target.from_environment()
        if arguments.staged_fixture_ready and arguments.case == "constraints":
            raise QualificationError("--staged-fixture-ready applies only to populated or all")
        if target.transport == "docker" and arguments.case in {"populated", "all"} and \
                not arguments.staged_fixture_ready:
            raise QualificationError(
                "docker transport requires --staged-fixture-ready; create stage only through "
                "the lifecycle owner before using container-local SQL"
            )
        verify_engine(target)
        if arguments.case in {"populated", "all"}:
            run_populated(target, arguments.staged_fixture_ready)
        if arguments.case in {"constraints", "all"}:
            run_constraint_refusals(target)
            run_schema_refusals(target)
        print("C05 receipt qualification complete; disposable schema remains caller-owned")
        return 0
    except (QualificationError, ValueError) as error:
        print(f"C05 qualification refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
