#!/usr/bin/env python3
"""Qualify an acknowledged historical schema2 room graph on two cold SQL boots.

Requires supplied, hash-pinned seed and COMPLETE native SQL observer binaries,
generated full-world data, migrations and an explicitly disposable loopback SQL
service. Does not build binaries/worlds or start a database. Run once per engine.

Seed interface: DURIS_SQL_ROOM_ITEM_SEED_EXPORT=1 emits exactly one
ROOM_ITEM_PAYLOAD_SEED uid=<id> payload=<canonical graph hex> after real journal
ACK, coordinator/pool shutdown and its private-fixture historical transition.
There must then be no active accounting epoch or global activation. This is
inactive historical recovery qualification, never active-accounting cutover proof.

The observer binary must compile DURIS_SQL_ROOM_ITEM_RECOVERY_TEST into the
actual sql_room_item_publish path, after actual native room placement. It emits
ROOM_ITEM_PAYLOAD_RECOVERY uid=<id> payload=<canonical captured graph hex> and
one ROOM_ITEM_PAYLOAD_CUSTODY line per native runtime identity (format below).
The runner never injects expected fields into the server or reseeds between boots.
It compares all payload bytes before a gameplay pulse, plus independent native
custody against retained SQL, complete-world stages and durable state stability.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import select
import shutil
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[2]
if "--source-root" in sys.argv:
    ROOT = Path(sys.argv[sys.argv.index("--source-root") + 1]).resolve(strict=True)
sys.path.insert(0, str(ROOT / "tests/async"))
import test_flatfile_combat_journey as journey
PAYLOAD_LIMIT = 3 * 131072 + 1024
# Reviewed independently from src/item/item_transfer_command.h; the command
# envelope version differs from SQL_ROOM_ITEM_PAYLOAD_VERSION (sidecar format 1).
SUPPORTED_ITEM_TRANSFER_PAYLOAD_VERSION = 10
# Independently pinned to src/sql/sql_exclusion_guard.h. MySQL bounds advisory
# lock names to 64 characters; MariaDB accepts the former longer fixture name.
SQL_EXCLUSION_LOCK_PREFIX = "duris.player.death.restitution."
SQL_ADVISORY_LOCK_NAME_LIMIT = 64
CUSTODY_FIELDS = ("uid", "root", "parent", "owner", "room", "revision",
                  "owner_revision", "vnum", "state")
CUSTODY_PATTERN = re.compile(r"^ROOM_ITEM_PAYLOAD_CUSTODY " + " ".join(
    name + r"=(\d+)" for name in CUSTODY_FIELDS) + r"$", re.M)
WORLD_INPUTS = tuple("areas/world." + suffix for suffix in ("mob", "obj", "qst", "shp", "wld", "zon")) + (
    "lib/misc/lookup.mob", "lib/misc/lookup.obj", "lib/misc/lookup.wld", "lib/misc/lookup.zon",
    "lib/misc/lookup_with_limits.zon")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixture_endpoint(environment: dict[str, str]) -> tuple[str, str]:
    host, port = environment.get("TEST_DB_HOST", ""), environment.get("TEST_DB_PORT", "")
    require(environment.get("TEST_DB_DISPOSABLE") == "1" and
            environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1",
            "both explicit disposable SQL fixture flags are required")
    require(host == "127.0.0.1" and not environment.get("DB_SOCKET") and
            not environment.get("TEST_DB_SOCKET"), "explicit TCP IPv4 loopback fixture required")
    require(port.isdigit() and 1 <= int(port) <= 65535, "explicit SQL fixture port required")
    require(bool(environment.get("TEST_DB_USER")) and bool(environment.get("TEST_DB_PASSWORD")),
            "disposable SQL fixture credentials are required")
    return host, port


def fixture_schema_name(database: str) -> None:
    require(len(SQL_EXCLUSION_LOCK_PREFIX + database) <= SQL_ADVISORY_LOCK_NAME_LIMIT,
            "fixture schema exceeds the native MySQL advisory lock name bound")
    require(re.fullmatch(r"economic_schema_test_rb_[0-9a-f]{8}", database) is not None,
            "task-owned fresh schema name required")


def graph_record(output: str, prefix: str) -> tuple[int, bytes]:
    lines = [line.rstrip("\r") for line in output.splitlines() if line.startswith(prefix)]
    require(len(lines) == 1, f"exactly one {prefix} record required")
    matched = re.fullmatch(re.escape(prefix) + r" uid=(\d+) payload=([0-9a-fA-F]+)", lines[0])
    require(matched is not None, f"malformed {prefix} record")
    uid, encoded = int(matched[1]), matched[2]
    require(0 < uid < 1 << 64 and 0 < len(encoded) <= 2 * PAYLOAD_LIMIT and
            len(encoded) % 2 == 0, "invalid UID or bounded canonical graph encoding")
    return uid, bytes.fromhex(encoded)


def native_custody(output: str) -> dict[int, tuple[int, ...]]:
    lines = [line.rstrip("\r") for line in output.splitlines()
             if line.startswith("ROOM_ITEM_PAYLOAD_CUSTODY")]
    result = {}
    for line in lines:
        matched = CUSTODY_PATTERN.fullmatch(line)
        require(matched is not None, "malformed native custody record")
        values = tuple(int(value) for value in matched.groups())
        require(0 < values[0] < 1 << 64 and values[0] not in result,
                "duplicate or invalid native item UID")
        result[values[0]] = values
    require(len(result) == 3, "native observer must report all three original identities")
    return result


def verify_readback(output: str, expected: tuple[int, bytes],
                    custody: dict[int, tuple[int, ...]]) -> None:
    require(graph_record(output, "ROOM_ITEM_PAYLOAD_RECOVERY") == expected,
            "native cold room graph differs from acknowledged original exact payload")
    require(native_custody(output) == custody,
            "native runtime UIDs/revisions/topology differ from retained SQL")


def base_environment() -> dict[str, str]:
    # Never inherit gameplay activation, fault injection, mail, Redis or journal controls.
    result = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": "C"}
    if "LD_LIBRARY_PATH" in os.environ:
        result["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
    return result


def stop_group(process: subprocess.Popen, graceful: bool = False) -> None:
    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        process.wait(timeout=5)
        return
    try:
        process.wait(timeout=30 if graceful else 5)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait(timeout=5)
        if graceful:
            raise AssertionError("native cold-boot server did not stop normally")


def execute(args: list[str], environment: dict[str, str], *, input_text: str | None = None,
            timeout: int = 120) -> tuple[int, str]:
    process = subprocess.Popen(args, cwd=ROOT, env=environment, text=True,
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, start_new_session=True)
    try:
        output, _ = process.communicate(input_text, timeout=timeout)
        return process.returncode, output
    except BaseException:
        stop_group(process)
        raise


class Fixture:
    def __init__(self, host: str, port: str, database: str):
        fixture_schema_name(database)
        self.database = database
        self.password = os.environ["TEST_DB_PASSWORD"]
        self.environment = dict(base_environment(), ENVIRONMENT="local", DB_HOST=host,
            DB_PORT=port, DB_NAME=database, DB_USER=os.environ["TEST_DB_USER"],
            DB_PASSWD=self.password, MYSQL_PWD=self.password,
            DB_ALLOWED_TARGETS=host + "/" + database, DB_TLS="FALSE",
            TEST_DB_DISPOSABLE="1", ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1",
            ITEM_TRANSFER_TEST_DB_NAME=database)
        self.mysql = ["mysql", "--protocol=tcp", "-h", host, "-P", port,
                      "-u", os.environ["TEST_DB_USER"], "-N", "-B", "--unbuffered"]

    def redact(self, text: str) -> str:
        return text.replace(self.password, "[REDACTED]")

    def sql(self, query: str, selected: bool = True) -> str:
        code, output = execute(self.mysql + ([self.database] if selected else []),
                               self.environment, input_text=query, timeout=180)
        require(code == 0, "fixture SQL command failed: " + self.redact(output[-2500:]))
        return output.strip()

    def bootstrap(self, artifacts: Path) -> None:
        self.sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for number, arguments in enumerate((("adopt", "--kind", "fresh_bootstrap"), ("run",))):
            code, output = execute(["python3", "scripts/migration_runner.py", *arguments],
                                   self.environment, timeout=240)
            (artifacts / f"migration-{number}.log").write_text(self.redact(output))
            require(code == 0, "fixture migration failed; see sanitized migration log")

    def no_sessions(self) -> None:
        deadline = time.monotonic() + 10
        while self.sql("SELECT COUNT(*) FROM information_schema.PROCESSLIST "
                      "WHERE DB=DATABASE() AND ID<>CONNECTION_ID()") != "0":
            require(time.monotonic() < deadline, "native SQL sessions survived process shutdown")
            time.sleep(.1)

    def state(self, root: int, *, active: bool = False, operation: str | None = None) -> dict[str, str]:
        expected_active = "1" if active else "0"
        require(self.sql("SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL") == expected_active,
                "fixture authority state differs from selected journey mode")
        require(self.sql("SELECT COUNT(*) FROM economic_sql_global_activation") == expected_active,
                "fixture global activation differs from selected journey mode")
        members = "SELECT item_uid FROM item_current_owner WHERE root_item_uid=" + str(root)
        operations = "SELECT operation_id FROM sql_room_item_payload WHERE item_uid IN (" + members + ")"
        if operation is not None:
            require(re.fullmatch(r"[0-9A-Fa-f]{32}", operation) is not None, "original operation identity required")
            operations = "SELECT UNHEX('" + operation + "')"
        return {
            "custody": self.sql("SELECT own.item_uid,own.root_item_uid,COALESCE(own.parent_item_uid,0),"
                "own.owner_type,own.owner_id,own.item_revision,revision.revision,own.vnum,own.state "
                "FROM item_current_owner own JOIN item_owner_revision revision "
                "ON revision.owner_type=own.owner_type AND revision.owner_id=own.owner_id "
                "AND revision.owner_context_id=own.owner_context_id "
                f"WHERE own.root_item_uid={root} AND own.owner_context_id=0 ORDER BY own.item_uid"),
            "payload": self.sql("SELECT item_uid,item_revision,payload_version,HEX(operation_id),"
                "season_epoch,HEX(payload) FROM sql_room_item_payload WHERE item_uid IN (" + members +
                ") ORDER BY item_uid,item_revision"),
            "inbox": self.sql("SELECT HEX(operation_id),HEX(command_hash),HEX(keys_hash),command_type,"
                "schema_version,payload_version,status,result_code,failure_stage,HEX(result_payload) "
                "FROM critical_operation_inbox WHERE operation_id IN (" + operations + ") ORDER BY operation_id"),
            "accounting": self.sql("SELECT HEX(operation_id),HEX(lineage),HEX(epoch),"
                "HEX(original_operation_id),accounting_version,writer_id,policy_version,compiler_version,"
                "actor_kind,actor_id,reason,HEX(source_event),HEX(intent_digest),HEX(domain_digest),"
                "HEX(plan_digest),HEX(canonical_intent),HEX(canonical_plan),outcome,result_code,"
                "account_count,posting_count,child_count,item_event_count,before_witness_count,"
                "after_witness_count,recorded_at FROM economic_accounting_operation WHERE operation_id IN (" +
                operations + ") ORDER BY operation_id"),
            # Delivery status/backoff may legitimately progress in the native server.
            "outbox": self.sql("SELECT outbox_id,HEX(operation_id),event_index,destination,event_type,"
                "payload_version,HEX(payload),created_at FROM critical_outbox WHERE operation_id IN (" +
                operations + ") ORDER BY operation_id,event_index"),
            "custody_physical": self.sql("SELECT item_uid,root_item_uid,parent_item_uid,owner_type,"
                "owner_id,owner_context_id,item_revision,vnum,state,HEX(coin_payload),updated_at,equipment_slot "
                "FROM item_current_owner "
                f"WHERE root_item_uid={root} ORDER BY item_uid"),
            "references": self.sql("SELECT HEX(operation_id),item_uid,HEX(legacy_operation_id),"
                "legacy_event_index,before_revision,after_revision,child_index "
                "FROM economic_accounting_item_reference WHERE operation_id IN (" + operations +
                ") ORDER BY operation_id,item_uid,child_index"),
            "ledger": self.sql("SELECT HEX(operation_id),event_index,item_uid,root_item_uid,"
                "COALESCE(parent_item_uid,0),item_revision,from_owner_type,from_owner_id,"
                "from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
                "from_owner_revision,to_owner_revision,reason_type,reason_id,source_site,created_at "
                "FROM item_ownership_ledger WHERE item_uid IN (" + members + ") ORDER BY operation_id,event_index"),
            "epochs": self.sql("SELECT HEX(lineage),HEX(epoch),ordinal,HEX(predecessor),transition_kind,"
                "HEX(transition_digest),HEX(creating_operation_id) FROM economic_epoch ORDER BY lineage,ordinal"),
            "historical_lineage": self.sql("SELECT HEX(lineage),HEX(active_epoch),revision "
                "FROM economic_lineage_state ORDER BY lineage"),
            "legacy": self.sql("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN (" + members + ")"),
            "player_projection": self.sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" + members + ")"),
        }


def expected_custody(state: dict[str, str], root: int) -> dict[int, tuple[int, ...]]:
    rows = [tuple(map(int, row.split("\t"))) for row in state["custody"].splitlines()]
    require(len(rows) == 3 and all(len(row) == len(CUSTODY_FIELDS) for row in rows),
            "seed must retain exactly three complete custody identities")
    result = {row[0]: row for row in rows}
    require(len(result) == 3 and root in result and result[root][2] == 0,
            "original root UID and unique members required")
    require(all(row[1] == root and row[3:6] == (3, 22800, 2) and row[6] > 0 and row[8] == 1
                for row in rows), "seed must retain active room custody at revision two")
    children = [row for row in rows if row[2] == root]
    require(len(children) == 1 and result[root][7] == 48 and children[0][7] == 48 and
            len([row for row in rows if row[2] == children[0][0] and row[7] == 5]) == 1,
            "original backpack/backpack/note nested topology required")
    require(len(state["payload"].splitlines()) == 3 and state["legacy"] == "0" and
            state["player_projection"] == "0", "exact sidecar must be sole physical payload source")
    inbox = [row.split("\t") for row in state["inbox"].splitlines()]
    require(len(inbox) == 1 and len(inbox[0]) == 10 and
            inbox[0][3:9] == ["5", "2", str(SUPPORTED_ITEM_TRANSFER_PAYLOAD_VERSION), "1", "0", "0"]
            and len(state["accounting"].splitlines()) == 1 and len(state["references"].splitlines()) == 3,
            "original committed schema2 operation and all native accounting references required")
    return result


def cold_boot(binary: Path, fixture: Fixture, runtime: Path, artifacts: Path,
              number: int, expected: tuple[int, bytes], custody: dict[int, tuple[int, ...]]) -> None:
    runtime.mkdir()
    (runtime / "logs/log").mkdir(parents=True)
    journey.make_fixture(runtime)
    journey.generate_certificate(runtime)
    for name in ("players", "critical"):
        (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
    port, tls, websocket = journey.available_ports()
    environment = dict(fixture.environment, PERSISTENCE_MODE="mariadb-primary", REDIS="FALSE",
        CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls),
        DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_WEBSOCKET_PORT=str(websocket),
        PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
        CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"))
    output_path = artifacts / f"boot-{number}.log"
    process = None
    with output_path.open("w") as output:
        try:
            # No -m, -r or copied Redis/runtime state: actual complete cold boot.
            process = subprocess.Popen([str(binary), "-d", str(runtime), str(port)],
                cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT,
                start_new_session=True)
            deadline = time.monotonic() + 120
            while process.poll() is None:
                if "Entering game loop." in output_path.read_text(errors="replace"):
                    break
                require(time.monotonic() < deadline, "complete SQL world boot deadline exceeded")
                time.sleep(.05)
            require(process.poll() is None, "native SQL server exited before complete boot")
            stop_group(process, graceful=True)
            require(process.returncode == 0, "native SQL cold boot did not terminate normally")
        finally:
            if process is not None:
                stop_group(process)
    text = output_path.read_text(errors="replace") + journey.runtime_logs(runtime)
    text = fixture.redact(text)
    output_path.write_text(text)
    for marker in ("Entering game loop.", "-- Player corpses", "-- Shopkeepers", "Booting Ferries",
                   "Normal termination of game."):
        require(marker in text, f"cold boot {number} omitted complete-world stage {marker}")
    require("no path found!" not in text and "can't find ferry ticket automat" not in text,
            "full-world ferry route is incomplete")
    verify_readback(text, expected, custody)
    fixture.no_sessions()


def active_drop_journey(args, fixture: Fixture, artifacts: Path, world_pins: dict[str, str]) -> None:
    """Existing complete-world fixture, real inactive preparation, guarded drop."""
    runtime = artifacts / "producer"
    runtime.mkdir()
    (runtime / "logs/log").mkdir(parents=True)
    journey.make_fixture(runtime)
    journey.generate_certificate(runtime)
    for name in ("players", "critical"):
        (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
    process = client = None
    output_path = artifacts / "producer.log"
    boot_number = 0

    def start() -> int:
        nonlocal process, output_path, boot_number
        boot_number += 1
        output_path = artifacts / f"producer-{boot_number}.log"
        port, tls, websocket = journey.available_ports()
        environment = dict(fixture.environment, PERSISTENCE_MODE="mariadb-primary", REDIS="FALSE",
            CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls),
            DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_WEBSOCKET_PORT=str(websocket),
            PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
            CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"))
        require(digest(args.server_binary) == args.server_sha256, "producer binary changed")
        with output_path.open("w") as output:
            process = subprocess.Popen([str(args.server_binary), "-d", str(runtime), str(port)],
                cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT,
                start_new_session=True)
        deadline = time.monotonic() + 120
        while "Entering game loop." not in output_path.read_text(errors="replace"):
            require(process.poll() is None and time.monotonic() < deadline, "producer complete-world boot failed")
            time.sleep(.05)
        return port

    def stop() -> None:
        nonlocal process, client
        if client is not None:
            client.close()
            client = None
        if process is not None:
            stop_group(process, graceful=True)
            require(process.returncode == 0, "producer shutdown failed")
            text = fixture.redact(output_path.read_text(errors="replace") + journey.runtime_logs(runtime))
            output_path.write_text(text.replace(journey.PASSWORD, "[REDACTED]"))
            process = None
        fixture.no_sessions()

    try:
        port = start()
        client = journey.MudClient(port)
        journey.create_character(client, expected_room=None, class_name="w")
        # Free the real starter inventory before the private nested reset boot.
        client.send('drop all')
        client.expect('You drop', timeout=30)
        client.send("save")
        client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
        stop()
        pid = int(fixture.sql("SELECT pid FROM player_data WHERE name='" + journey.CHARACTER + "'"))
        room = int(fixture.sql(f"SELECT last_room FROM player_data WHERE pid={pid}"))
        require(pid > 0 and room > 0, "real created character and saved room required")
        def displayed_world_field(relative: str, vnum: int, field: int) -> str:
            path = ROOT / relative
            require(digest(path) == world_pins[relative], "pinned native world changed")
            text = path.read_text(encoding="latin-1")
            headers = list(re.finditer(r"^#" + str(vnum) + r"[ \t]*$", text, re.M))
            require(len(headers) == 1, "native UI fixture record is not unique")
            fields = text[headers[0].end():].split("~", field + 1)
            require(len(fields) == field + 2, "native UI fixture string is incomplete")
            # The existing client removes rendered ANSI; derive the same visible
            # text from the pinned native color-coded room/object string.
            visible = re.sub(r"&(?:[+-][A-Za-z]|[nN])", "", fields[field]).strip()
            require(bool(visible), "native UI fixture string is empty")
            return visible

        expected_room = displayed_world_field("areas/world.wld", room, 0)
        root_short = displayed_world_field("areas/world.obj", 377, 1)
        # Reuse the NPC-container journey's O/P native constructor mechanism,
        # on a private copy of the COMPLETE generated world, never the checkout.
        (runtime / "areas").unlink()
        shutil.copytree(ROOT / "areas", runtime / "areas")
        zone = runtime / "areas/world.zon"
        original_zone = zone.read_bytes()
        zone_text = original_zone.decode("latin-1").replace("\r\n", "\n")
        require("\nS\n" in zone_text, "generated complete zone reset terminator missing")
        resets = (f"\nO 0 377 1000000 {room} 100 0 0 0 * private ordinary drop root\n"
                  "P 1 391 1000000 377 100 0 0 0 * private nested ordinary bag\n"
                  "P 1 5 1000000 391 100 0 0 0 * private nested note\n")
        zone.write_text(zone_text.replace("\nS\n", resets + "S\n", 1), encoding="latin-1")
        port = start()
        client = journey.reconnect_character(port, expected_room=expected_room)
        client.send("get backpack")
        client.expect("get " + root_short, timeout=30)
        client.send("save")
        client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
        stop()
        zone.write_bytes(original_zone)
        roots = fixture.sql("SELECT root.item_uid FROM item_current_owner root "
            "JOIN item_current_owner child ON child.parent_item_uid=root.item_uid "
            "JOIN item_current_owner note ON note.parent_item_uid=child.item_uid "
            f"WHERE root.owner_type=1 AND root.owner_id={pid} AND root.vnum=377 "
            "AND root.parent_item_uid IS NULL AND child.vnum=391 AND note.vnum=5 "
            "AND child.root_item_uid=root.item_uid AND note.root_item_uid=root.item_uid")
        require(len(roots.splitlines()) == 1, "real native nested reset/get graph was not uniquely acquired")
        root = int(roots)
        members = f"SELECT item_uid FROM item_current_owner WHERE root_item_uid={root}"
        original = [tuple(map(int, line.split("\t"))) for line in fixture.sql(
            "SELECT item_uid,COALESCE(parent_item_uid,0),item_revision,vnum FROM item_current_owner "
            f"WHERE root_item_uid={root} ORDER BY item_uid").splitlines()]
        require(len(original) == 3 and len({row[0] for row in original}) == 3,
                "exact three original native UIDs required")
        require(fixture.sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" + members + ") "
            "AND name IS NULL AND short_descr IS NULL AND description IS NULL AND action_descr IS NULL") == "3",
            "original native graph must be unstrung before scoped literal capture")
        require(fixture.sql("SELECT COUNT(*) FROM economic_sql_global_activation") == "0" and
            fixture.sql("SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL") == "0",
            "real character/get/save preparation must remain inactive")
        (artifacts / "original-native-custody.json").write_text(json.dumps(original) + "\n")
        setup_journal = artifacts / "authority-journal"
        setup_journal.mkdir(mode=0o700)
        environment = dict(fixture.environment, PERSISTENCE_MODE="mariadb-primary",
            ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SCHEMA="1",
            ECONOMIC_SQL_LIFECYCLE_JOURNAL_DIR=str(setup_journal))
        code, output = execute([str(args.authority_binary), "--setup-stopped-drop-fixture"], environment)
        (artifacts / "authority.log").write_text(fixture.redact(output))
        require(code == 0 and "PASS STOPPED_DROP_AUTHORITY" in output,
                "real guarded native source/install/activation/readback failed")
        fixture.no_sessions()
        barrier = "p1drop-" + uuid.uuid4().hex[:16]
        trigger = "p1drop_" + uuid.uuid4().hex[:16]
        lease = subprocess.Popen(fixture.mysql + [fixture.database], env=fixture.environment,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, start_new_session=True)
        created_trigger = False
        try:
            lease.stdin.write(f"SELECT GET_LOCK('{barrier}',0);\n")
            lease.stdin.flush()
            require(select.select([lease.stdout], [], [], 5)[0] and lease.stdout.readline().strip() == "1",
                    "disposable source-capture barrier acquisition failed")
            fixture.sql("DELIMITER //\nCREATE TRIGGER " + trigger + " BEFORE INSERT ON critical_operation_inbox "
                "FOR EACH ROW BEGIN IF NEW.command_type=5 AND NEW.schema_version=2 THEN "
                f"SET @p1drop_wait=GET_LOCK('{barrier}',60); "
                "IF @p1drop_wait<>1 OR @p1drop_wait IS NULL THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='private source barrier timeout'; END IF; "
                f"SET @p1drop_released=RELEASE_LOCK('{barrier}'); END IF; END//\nDELIMITER ;")
            created_trigger = True
            port = start()
            client = journey.reconnect_character(port, expected_room=expected_room)
            # Login saves must finish before the real producer can admit a drop.
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            journal = runtime / "journals/critical/critical-command.journal"
            require(journal.is_file() and journal.stat().st_size == 0, "old native critical workload remains")
            client.send("drop backpack")
            deadline = time.monotonic() + 60
            preadmission_retries = 0
            while fixture.sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" + members + ") "
                    "AND name IS NOT NULL AND short_descr IS NOT NULL AND description IS NOT NULL AND action_descr IS NOT NULL") != "3" or journal.stat().st_size == 0:
                client._receive()
                busy = b"That item is busy right now; try again in a moment."
                if busy in client.pending:
                    # Real login/rank autosaves may outlast an earlier manual
                    # save notification. Retry only the explicit native refusal:
                    # no literal checkpoint or critical admission may exist.
                    require(journal.stat().st_size == 0 and
                        fixture.sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" + members + ") "
                            "AND (name IS NOT NULL OR short_descr IS NOT NULL OR description IS NOT NULL OR action_descr IS NOT NULL)") == "0",
                        "busy response accompanied an admitted or literal-checkpointed drop")
                    client.expect(busy.decode("ascii"), timeout=.01)
                    require(process.poll() is None and time.monotonic() < deadline,
                            "pre-admission save conflict exceeded original drop bound")
                    preadmission_retries += 1
                    time.sleep(.25)
                    client.send("drop backpack")
                if process.poll() is not None or time.monotonic() >= deadline:
                    native_rows = fixture.sql("SELECT obj_uid,name IS NOT NULL,short_descr IS NOT NULL,description IS NOT NULL,action_descr IS NOT NULL FROM player_items WHERE obj_uid IN (" + members + ") ORDER BY obj_uid")
                    raise AssertionError("real literal checkpoint did not arrive; native=" + native_rows +
                        "; journal_bytes=" + str(journal.stat().st_size) + "; ui=" +
                        fixture.redact(bytes(client.pending).decode("utf-8", errors="replace")))
                time.sleep(.05)
            require(journal.stat().st_size > 0, "actual drop command was not journaled before SQL mutation")
            code, output = execute([str(args.seed_binary), "--export-live-drop-source", str(pid), str(root)],
                dict(fixture.environment, PERSISTENCE_MODE="mariadb-primary"), timeout=max(1, int(deadline - time.monotonic())))
            (artifacts / "original-literal-source.log").write_text(fixture.redact(output))
            require(code == 0, "actual native loader/codec original source export failed")
            expected = graph_record(output, "ROOM_ITEM_PAYLOAD_SEED")
            require(expected[0] == root, "export changed original native root identity")
            lease.stdin.write(f"SELECT RELEASE_LOCK('{barrier}');\n")
            lease.stdin.flush()
            require(select.select([lease.stdout], [], [], 5)[0] and lease.stdout.readline().strip() == "1",
                    "disposable source barrier release failed")
            client.expect("drop " + root_short, timeout=max(1, deadline - time.monotonic()))
            while journal.stat().st_size:
                require(process.poll() is None and time.monotonic() < deadline, "guarded publication journal ACK not observed")
                time.sleep(.05)
            operation = fixture.sql(f"SELECT DISTINCT HEX(operation_id) FROM item_ownership_ledger WHERE item_uid={root} AND from_owner_type=1 AND from_owner_id={pid} AND to_owner_type=3 AND to_owner_id={room}")
            require(re.fullmatch(r"[A-Fa-f0-9]{32}", operation) is not None, "one original native drop operation required")
        finally:
            # Closing the owned holder releases its lock even on source-export failure.
            stop_group(lease)
            if created_trigger:
                fixture.sql("DROP TRIGGER " + trigger)
        stop()
        before = fixture.state(root, active=True, operation=operation)
        (artifacts / "acknowledged-state.json").write_text(json.dumps(before, indent=2) + "\n")
        custody = {row[0]: row for row in [tuple(map(int, line.split("\t"))) for line in before["custody"].splitlines()]}
        require(len(custody) == 3 and set(custody) == {row[0] for row in original}, "drop changed original UID set")
        for uid, parent, revision, vnum in original:
            row = custody[uid]
            require(row[1:6] == (root, parent, 3, room, revision + 1) and row[6] > 0 and row[7:] == (vnum, 1),
                    "native drop custody/topology/revision does not match original")
        require(before["legacy"] == before["player_projection"] == "0" and len(before["payload"].splitlines()) == 3 and
            len(before["references"].splitlines()) == 3 and len(before["accounting"].splitlines()) == 1,
            "exact schema2 sidecar/accounting/sole physical source missing")
        inbox = before["inbox"].split("\t")
        require(len(inbox) == 10 and inbox[0] == operation and inbox[3:9] == ["5", "2", "10", "1", "0", "0"],
                "original successful immutable schema2 root receipt missing")
        for number in (1, 2):
            cold_boot(args.server_binary, fixture, artifacts / f"runtime-{number}", artifacts, number, expected, custody)
            require(fixture.state(root, active=True, operation=operation) == before, "active cold boot mutated original durable proof")
        require(all(digest(ROOT / name) == sha for name, sha in world_pins.items()) and
                digest(args.seed_binary) == args.seed_sha256 and digest(args.authority_binary) == args.authority_sha256,
                "supplied native/world inputs changed")
        (artifacts / "qualification.json").write_text(json.dumps(dict(scope="disposable synthetic-route Plan1 real ordinary do_drop and two cold boots",
            root_uid=root, operation_id=operation, payload_sha256=hashlib.sha256(expected[1]).hexdigest(),
            native_custody=custody, seed_sha256=args.seed_sha256, server_sha256=args.server_sha256,
            authority_sha256=args.authority_sha256, world_sha256=world_pins, cold_boots=2, redis=False,
            global_route_qualification=False, preadmission_retries=preadmission_retries), indent=2) + "\n")
        print("PASS: actual ordinary drop, guarded ACK, original unstrung graph and two complete SQL cold boots; disposable synthetic coverage only")
    finally:
        if client is not None:
            client.close()
        if process is not None:
            stop_group(process)


def self_test() -> None:
    valid = dict(TEST_DB_DISPOSABLE="1", ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1",
                 TEST_DB_HOST="127.0.0.1", TEST_DB_PORT="3307", TEST_DB_USER="fixture",
                 TEST_DB_PASSWORD="synthetic-fixture-only")
    require(fixture_endpoint(valid) == ("127.0.0.1", "3307"), "valid fixture guard")
    class QueryCapture:
        def __init__(self):
            self.queries = []

        def sql(self, query):
            self.queries.append(query)
            return "0"

    query_capture = QueryCapture()
    Fixture.state(query_capture, 7)
    require(not any("SELECT *" in query for query in query_capture.queries),
            "all retained state fields must have explicit scalar/binary transport")
    accounting_query = next(query for query in query_capture.queries
                            if "FROM economic_accounting_operation WHERE" in query)
    for column in ("operation_id", "lineage", "epoch", "original_operation_id", "source_event",
                   "intent_digest", "domain_digest", "plan_digest", "canonical_intent", "canonical_plan"):
        require("HEX(" + column + ")" in accounting_query, "binary accounting field must be hex transported")
    require(any("HEX(coin_payload)" in query for query in query_capture.queries),
            "binary custody field must be hex transported")
    negatives = 0
    fixture_schema_name("economic_schema_test_rb_0123abcd")
    require(len(SQL_EXCLUSION_LOCK_PREFIX + "economic_schema_test_rb_0123abcd") == 63,
            "fresh schema must leave room for the native advisory lock prefix")
    for database in ("economic_schema_test_room_boot_0123456789abcdef",
                     "economic_schema_test_rb_0123456789abcdef",
                     "economic_schema_test_rb_0123abcg",
                     "economic_schema_test_rb_0123abcd;DROP DATABASE other",
                     "other_schema_0123abcd"):
        try:
            fixture_schema_name(database)
        except AssertionError as error:
            if database.startswith("economic_schema_test_room_boot_"):
                require("advisory lock name bound" in str(error),
                        "former schema must fail the independent native name bound")
            negatives += 1
        else:
            raise AssertionError("invalid or MySQL-incompatible fixture schema accepted")
    for key, value in (("TEST_DB_DISPOSABLE", "0"), ("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "0"),
                       ("TEST_DB_HOST", "192.0.2.1"), ("TEST_DB_PORT", "0"),
                       ("TEST_DB_PORT", "65536"), ("DB_SOCKET", "/tmp/socket")):
        try:
            fixture_endpoint(dict(valid, **{key: value}))
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError("unsafe endpoint accepted")
    line = "ROOM_ITEM_PAYLOAD_SEED uid=7 payload=0102"
    require(graph_record(line, "ROOM_ITEM_PAYLOAD_SEED") == (7, b"\x01\x02"), "exact seed parser")
    for mutated in (line + "\n" + line, line.replace("uid=7", "uid=0"), line + "f"):
        try:
            graph_record(mutated, "ROOM_ITEM_PAYLOAD_SEED")
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError("invalid seed observer record accepted")
    custody = {7: (7, 7, 0, 3, 22800, 2, 1, 48, 1),
               8: (8, 7, 7, 3, 22800, 2, 1, 48, 1),
               9: (9, 7, 8, 3, 22800, 2, 1, 5, 1)}
    acknowledged = dict(custody="\n".join("\t".join(map(str, row)) for row in custody.values()),
        payload="row1\nrow2\nrow3", legacy="0", player_projection="0", accounting="operation",
        references="ref1\nref2\nref3", inbox="op\tcommand\tkeys\t5\t2\t10\t1\t0\t0\tresult")
    require(expected_custody(acknowledged, 7) == custody, "supported native command version accepted")
    for version in ("1", "0", "11"):
        try:
            expected_custody(dict(acknowledged, inbox=acknowledged["inbox"].replace("\t10\t", "\t" + version + "\t")), 7)
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError("unsupported command version accepted")
    observed = "ROOM_ITEM_PAYLOAD_RECOVERY uid=7 payload=0102\n" + "\n".join(
        "ROOM_ITEM_PAYLOAD_CUSTODY " + " ".join(f"{name}={value}" for name, value in zip(CUSTODY_FIELDS, row))
        for row in custody.values())
    verify_readback(observed, (7, b"\x01\x02"), custody)
    for mutated in (observed.replace("payload=0102", "payload=0103"),
                    observed.replace("revision=2", "revision=3", 1),
                    observed.replace("parent=8", "parent=7"),
                    observed[:observed.rfind("\n")],
                    observed + "\n" + observed.splitlines()[-1]):
        try:
            verify_readback(mutated, (7, b"\x01\x02"), custody)
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError("missing, duplicate, changed payload/custody/topology accepted")
    print(f"PASS: fixture/graph parser checks; {negatives} negative cases; no SQL/server execution")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-binary", type=Path)
    parser.add_argument("--seed-sha256")
    parser.add_argument("--server-binary", type=Path)
    parser.add_argument("--server-sha256")
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--active-drop-producer", action="store_true")
    parser.add_argument("--authority-binary", type=Path)
    parser.add_argument("--authority-sha256")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    host, port = fixture_endpoint(os.environ)
    command_header = (ROOT / "src/item/item_transfer_command.h").read_text(encoding="latin-1")
    require(re.search(r"constexpr\s+uint16_t\s+ITEM_TRANSFER_PAYLOAD_VERSION\s*=\s*" +
                      str(SUPPORTED_ITEM_TRANSFER_PAYLOAD_VERSION) + r"\s*;", command_header) is not None,
            "reviewed command-version pin differs from native source header")
    exclusion_header = (ROOT / "src/sql/sql_exclusion_guard.h").read_text(encoding="latin-1")
    require("CONCAT('" + SQL_EXCLUSION_LOCK_PREFIX + "',DATABASE())" in exclusion_header,
            "reviewed advisory lock prefix differs from native source header")
    for name in (("seed", "server", "authority") if args.active_drop_producer else ("seed", "server")):
        path, expected = getattr(args, name + "_binary"), getattr(args, name + "_sha256")
        require(path is not None and expected is not None and re.fullmatch(r"[0-9a-f]{64}", expected),
                "both supplied binaries require explicit SHA256 pins")
        path = path.resolve(strict=True)
        require(path.is_file() and os.access(path, os.X_OK) and digest(path) == expected,
                "supplied executable differs from its qualification pin")
        setattr(args, name + "_binary", path)
    world_pins = {}
    for relative in WORLD_INPUTS:
        path = ROOT / relative
        require(path.is_file() and path.stat().st_size > 0,
                "generated complete world/lookup support is a prerequisite; runner does not build it")
        world_pins[relative] = digest(path)
    for name, ids in (("world.obj", (48, 5)), ("world.wld", (22800,))):
        path = ROOT / "areas" / name
        require(path.is_file(), "generated complete world is a prerequisite; runner does not build it")
        text = path.read_text(encoding="latin-1")
        require(all(len(re.findall(r"^#" + str(uid) + r"\s*$", text, re.M)) == 1 for uid in ids),
                "generated complete world lacks unique fixture prototypes/room")
    if args.active_drop_producer:
        objects = (ROOT / "areas/world.obj").read_text(encoding="latin-1")
        require(all(len(re.findall(r"^#" + str(vnum) + r"\s*$", objects, re.M)) == 1 for vnum in (377, 391)),
                "native distinct nested bag prototype unavailable")
    artifacts = ROOT / "tmp" / ("sql-room-recovery-" + uuid.uuid4().hex[:16])
    artifacts.mkdir(parents=True, exist_ok=False, mode=0o700)
    fixture = Fixture(host, port, "economic_schema_test_rb_" + uuid.uuid4().hex[:8])
    created = False
    try:
        fixture.sql("CREATE DATABASE " + fixture.database, selected=False)
        created = True
        fixture.bootstrap(artifacts)
        if args.active_drop_producer:
            active_drop_journey(args, fixture, artifacts, world_pins)
            return
        seed_environment = dict(fixture.environment, DURIS_SQL_ROOM_ITEM_SEED_EXPORT="1",
            ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        code, output = execute(["bash", "-c", 'ulimit -s 65536 && exec "$1"',
                               "sql-room-seed", str(args.seed_binary)], seed_environment)
        output = fixture.redact(output)
        (artifacts / "seed.log").write_text(output)
        require(code == 0, "native ACK/export seed failed; see sanitized seed log")
        expected = graph_record(output, "ROOM_ITEM_PAYLOAD_SEED")
        fixture.no_sessions()
        before = fixture.state(expected[0])
        (artifacts / "acknowledged-state.json").write_text(json.dumps(before, indent=2) + "\n")
        custody = expected_custody(before, expected[0])
        for number in (1, 2):
            require(digest(args.server_binary) == args.server_sha256, "executing native binary changed")
            cold_boot(args.server_binary, fixture, artifacts / f"runtime-{number}", artifacts,
                      number, expected, custody)
            require(fixture.state(expected[0]) == before, "cold boot mutated retained payload/custody/provenance")
        require(digest(args.seed_binary) == args.seed_sha256 and
                all(digest(ROOT / name) == sha for name, sha in world_pins.items()),
                "qualified seed binary or generated world changed")
        summary = dict(scope="inactive historical schema2 room recovery; not active cutover",
            engine=fixture.sql("SELECT VERSION()"), seed_sha256=args.seed_sha256,
            server_sha256=args.server_sha256, world_sha256=world_pins, cold_boots=2,
            root_uid=expected[0], payload_sha256=hashlib.sha256(expected[1]).hexdigest(),
            native_custody=custody, durable_state_unchanged=True, redis=False)
        (artifacts / "qualification.json").write_text(json.dumps(summary, indent=2) + "\n")
        print("PASS: ACK then two complete native SQL cold boots; exact payload, UIDs, revisions and topology; "
              "Redis disabled; inactive historical fixture only")
        print("evidence:", artifacts)
    finally:
        if created:
            fixture.no_sessions()
            fixture.sql("DROP DATABASE " + fixture.database, selected=False)


if __name__ == "__main__":
    main()
