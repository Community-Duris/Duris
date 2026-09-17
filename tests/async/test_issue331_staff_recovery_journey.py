#!/usr/bin/env python3
"""Focused #331/#375 staff-to-restart acceptance journey.

The existing issue331 player journey proves the guarded offline CLI route.  This
harness deliberately exercises the missing integration seam: a real staff
character stages the canonical builder payload through the in-game command,
while the recipient is offline and its save/login fence is held.  All state is
owned by the runner's fresh MariaDB/game containers.
"""
from __future__ import annotations

import hashlib
import json
import os
import shlex
import shutil
import stat
import sys
import tempfile
import time
from pathlib import Path
from typing import Sequence

# Reuse only the disposable-runtime plumbing and the canonical player fixture;
# no production source or builder code is changed by this test.
ASYNC_DIR = Path(__file__).resolve().parent
if str(ASYNC_DIR) not in sys.path:
    sys.path.insert(0, str(ASYNC_DIR))
import test_issue331_player_journey as base  # type: ignore

ROOT = base.ROOT
PLAN_DIR = base.PLAN_DIR
DB_CONTAINER = base.DB_CONTAINER
RUNTIME_CONTAINER = base.RUNTIME_CONTAINER
GAME_PORT = base.GAME_PORT
GAME_HOST = base.GAME_HOST
DB_NAME = base.DB_NAME
DB_PASSWORD = base.DB_PASSWORD
ARTIFACT = base.ARTIFACT
ARTIFACT_VNUM = base.ARTIFACT_VNUM
SQL_TRACE = os.environ.get("ISSUE331_SQL_TRACE") == "1"

RECIPIENT_ACCOUNT = "Journeyacct"
RECIPIENT_NAME = "Taverek"
STAFF_ACCOUNT = "Journeywiz"
STAFF_NAME = "Nereth"
OBSERVER_ACCOUNT = "Journalscout"
OBSERVER_NAME = "Korvyn"
PASSWORD = "Qz7!mN4@"
DEATH_REVISION = 77
RECOVERED_UIDS = (51000, 51001, 51002, 51003, 51005)

# These are fixture/prototype contracts, not a snapshot of the post-save SQL
# rows.  The item fixture supplies the identity, strings, values, materials,
# costs, and condition.  Runtime object loading supplies the prototype-derived
# values called out below (notably the aggregate bag weight and affects).
EXPECTED_RUNTIME_ITEMS = {
    51000: {
        "vnum": 391, "equip_slot": 0, "quantity": 1, "weight": 5,
        "cost": 25, "timer": 0, "extra_flags": 0, "wear_flags": 1,
        "type": 15, "values": (50, 1, 0, 100, 0, 0, 0, 0), "material": 13,
        "condition": 100, "name": b"qabag recovered bag leather",
        "short_description": b"a recovered leather bag",
    },
    51001: {
        "vnum": 15, "equip_slot": 0, "quantity": 1, "weight": 1,
        "cost": 25, "timer": 0, "extra_flags": 0, "wear_flags": 1,
        "type": 19, "values": (10, 1, 1, 0, 0, 0, 0, 0), "material": 48,
        "condition": 100, "name": b"qabanana recovered banana",
        "short_description": b"a recovered banana",
    },
    51002: {
        "vnum": 677, "equip_slot": 0, "quantity": 1, "weight": 1,
        "cost": 25, "timer": 0, "extra_flags": 0, "wear_flags": 8193,
        "type": 5, "values": (6, 1, 6, 7, 0, 0, 0, 0), "material": 6,
        "condition": 100, "name": b"qamace recovered mace wooden",
        "short_description": b"a recovered wooden mace",
    },
    51003: {
        "vnum": 67259, "equip_slot": 0, "quantity": 1, "weight": 1,
        "cost": 1000, "timer": 0, "extra_flags": 268435456, "wear_flags": 129,
        "type": 9, "values": (3, 2, 10, 0, 0, 0, 0, 0), "material": 2,
        "condition": 100, "name": b"qagloves unique recovered gloves",
        "short_description": b"a unique pair of recovered gloves",
    },
    51005: {
        "vnum": 7, "equip_slot": 0, "quantity": 1, "weight": 1,
        "cost": 25, "timer": 0, "extra_flags": 0, "wear_flags": 1,
        "type": 33, "values": (27, 1, 0, 0, 0, 0, 0, 0), "material": 7,
        "condition": 100, "name": b"qaspellbook recovered spellbook",
        "short_description": b"a recovered spellbook",
    },
}
for _item in EXPECTED_RUNTIME_ITEMS.values():
    _short = _item["short_description"]
    _item.update({
        "description": _short + b" rests in the recovered custody tree.",
        "action_description": b"Recovered by the issue331 player journey.",
        "bitvectors": (0, 0, 0, 0, 0),
    })

EXPECTED_RUNTIME_AFFECTS = {
    51000: set(),
    51001: {(19, -10)},  # areas_mini/mini.obj prototype #15
    51002: set(),
    51003: {(13, 25), (20, -2)},  # areas/obj/unique.obj prototype #67259
    51005: set(),
}
EXPECTED_RECOVERED_DESCRIPTION = (b"recovered", b"issue331 custody payload")
EXPECTED_MASTER_DESCRIPTION = (
    b"book spell spellbook master tome",
    b"Every spell a spellbook class can learn is already scribed here.  The pages\r\n"
    b"turn themselves to whichever incantation you are looking for.\r\n",
)
# src/world/db.c calls FillMasterSpellBook() for prototype #7.  Its raw bitmap
# is projected by the SQL extra-description codec into this canonical list;
# pin the source-defined result without copying a mutable player row.
MASTER_SPELLBOOK_MARKER_SHA256 = "255f1df2d437d08d3c155b62146c9c92238e891270cba744f3101cdf519be16f"


class JourneyFailure(RuntimeError):
    pass


def sql(wrapper: Path, statement: str) -> str:
    return base.sql(wrapper, statement)


def sql_one(wrapper: Path, statement: str) -> str:
    return base.sql_one(wrapper, statement)


def enable_sql_trace(wrapper: Path) -> None:
    if SQL_TRACE:
        sql(wrapper, "SET GLOBAL log_output='TABLE'; SET GLOBAL general_log=ON")


def read_sql_trace(wrapper: Path) -> str:
    if not SQL_TRACE:
        return ""
    rows = sql(wrapper, """
SELECT DATE_FORMAT(event_time, '%H:%i:%s.%f'),command_type,
       LEFT(REPLACE(argument, CHAR(10), ' '), 600)
FROM mysql.general_log
WHERE command_type IN ('Query','Execute')
  AND (argument LIKE '%player_death%' OR argument LIKE '%item_current_owner%'
       OR argument LIKE '%artifact_domain%' OR argument LIKE '%artifact_bind%'
       OR argument LIKE '%artifacts_mortal%' OR argument LIKE '%artifacts %')
ORDER BY event_time DESC LIMIT 120
""").splitlines()
    return "\n".join(reversed(rows))


def wait_sql(wrapper: Path, statement: str, expected: str, *, timeout: int = 120) -> str:
    deadline = time.monotonic() + timeout
    last = ""
    while time.monotonic() < deadline:
        last = sql_one(wrapper, statement)
        if last == expected:
            return last
        time.sleep(0.25)
    raise JourneyFailure(f"SQL condition timed out: expected={expected!r} last={last!r}")


def install_client(journey) -> None:
    base.install_container_client(journey)


def expect_response(client, needles: Sequence[str], *, timeout: int = 30) -> tuple[str, str]:
    matched, output = client.expect_any(tuple(needles), timeout=timeout)
    # Do not leave a prompt or a multi-line command response in the next
    # assertion.  The base client has a bounded socket timeout for this drain.
    deadline = time.monotonic() + 1.0
    while time.monotonic() < deadline:
        try:
            if not client._receive():
                break
        except AssertionError:
            break
    output += bytes(client.pending).decode("utf-8", errors="replace")
    client.pending.clear()
    return matched, output


def close_account(client) -> None:
    try:
        client.send("quit")
        expect_response(client, ("Please select an option", "ACCOUNT MENU"), timeout=30)
        client.send("0")
    finally:
        client.close()


def save_and_wait(client, wrapper: Path, character: str, pid: int | None = None,
                  *, timeout: int = 60) -> str:
    before = None
    if pid is not None:
        before = int(sql_one(wrapper, f"SELECT save_revision FROM player_data WHERE pid={pid}"))
    client.send("save")
    matched, output = expect_response(
        client,
        (f"Save complete for {character}.", f"Save queued for {character}.",
         f"Save failed for {character}."),
        timeout=30,
    )
    if matched.startswith("Save failed"):
        raise JourneyFailure(f"runtime rejected save for {character}: {output[-1000:]}")
    if pid is not None and before is not None:
        deadline = time.monotonic() + timeout
        last = before
        while time.monotonic() < deadline:
            last = int(sql_one(wrapper, f"SELECT save_revision FROM player_data WHERE pid={pid}"))
            if last > before:
                return output
            time.sleep(0.25)
        raise JourneyFailure(
            f"runtime reported {matched!r} but SQL save_revision did not advance "
            f"for {character}: before={before} last={last}"
        )
    return output


def create_player(journey, wrapper: Path, account: str, character: str,
                  *, pack_starter_items: bool = False) -> int:
    journey.PASSWORD = PASSWORD
    journey.ACCOUNT = account
    journey.CHARACTER = character
    journey.EMAIL = f"{account.lower()}@invalid.example"
    client = journey.MudClient(GAME_PORT)
    try:
        journey.create_character(
            client,
            account=account,
            character=character,
            email=journey.EMAIL,
            expected_room="The Regression Arena",
        )
        if pack_starter_items:
            base.command(client, "wear all", ("You ",), timeout=30)
            base.command(client, "put all bag", ("You put", "Ok."), timeout=30)
        save_and_wait(client, wrapper, character)
        close_account(client)
    except Exception:
        client.close()
        raise
    return int(sql_one(wrapper, f"SELECT pid FROM player_data WHERE name='{character}'"))


def login_player(journey, account: str, character: str):
    journey.ACCOUNT = account
    journey.PASSWORD = PASSWORD
    journey.CHARACTER = character
    journey.EMAIL = f"{account.lower()}@invalid.example"
    return journey.reconnect_character(GAME_PORT, expected_room="The Regression Arena")


def mirror_artifact_legacy_projection(wrapper: Path, artifact_vnum: int) -> None:
    """Keep the live legacy god projection aligned with mortal evidence.

    The periodic artifact-bind worker reads ``artifacts`` even when the
    canonical disposable fixture is otherwise represented by
    ``artifacts_mortal``.  Both rows are real source evidence for this live
    journey; omitting the god projection lets the worker erase artifact_bind
    before the staff command reaches the repository.
    """
    sql(
        wrapper,
        f"""
INSERT INTO artifacts(vnum,owned,location,timer,type,lastUpdate,locType)
SELECT vnum,owned,location,timer,type,lastUpdate,locType
  FROM artifacts_mortal WHERE vnum={artifact_vnum}
ON DUPLICATE KEY UPDATE
  owned=VALUES(owned), location=VALUES(location), timer=VALUES(timer),
  type=VALUES(type), lastUpdate=VALUES(lastUpdate), locType=VALUES(locType)
""",
    )
    state = sql_one(
        wrapper,
        f"""
SELECT CONCAT(
 (SELECT COUNT(*) FROM artifacts WHERE vnum={artifact_vnum} AND owned='Y'
    AND location=(SELECT location FROM artifacts_mortal WHERE vnum={artifact_vnum})
    AND locType=5), '|',
 (SELECT COUNT(*) FROM artifact_bind WHERE vnum={artifact_vnum}
    AND owner_pid=(SELECT location FROM artifacts_mortal WHERE vnum={artifact_vnum})
    AND timer=UNIX_TIMESTAMP((SELECT timer FROM artifacts_mortal WHERE vnum={artifact_vnum})))
)""",
    )
    if state != "1|1":
        raise JourneyFailure(f"artifact legacy evidence was not mirrored: {state}")


def verify_migration21(wrapper: Path) -> None:
    result = sql_one(wrapper, """
SELECT CONCAT(
 (SELECT COUNT(*) FROM information_schema.tables
    WHERE table_schema=DATABASE() AND table_name='offline_message_receipts'), '|',
 (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='offline_message_receipts'
      AND column_name IN ('pid','message_id','status','delivered_at')), '|',
 (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='offline_messages' AND column_name='message_id'), '|',
 (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema=DATABASE() AND table_name='offline_messages'
      AND index_name='uq_offline_message_identity')
)""")
    if result != "1|4|1|2":
        raise JourneyFailure(f"migration21 schema contract mismatch: {result}")


def migration21_notification_probe(journey, wrapper: Path, recipient_pid: int,
                                    recipient_name: str, staff_client) -> bool:
    """Exercise the existing staff offline-message SQL path, not PDR logic.

    Collector's new receipt delivery is not emitted by the #331 repository in
    this build.  The legacy `offlinemsg` path is still a real notification SQL
    path and is useful evidence that migration 0021 keeps NULL legacy identity
    rows compatible.  A false result is reported as unsupported evidence rather
    than being promoted to a PDR notification claim.
    """
    marker = f"issue331-notification-{os.getpid()}"
    staff_client.send(f"offlinemsg {recipient_name} {marker}")
    _, output = expect_response(
        staff_client,
        ("Sending offline message", "Could not find player", "what message"),
        timeout=30,
    )
    count = sql_one(
        wrapper,
        f"SELECT COUNT(*) FROM offline_messages WHERE pid={recipient_pid} "
        f"AND message LIKE '%{marker}%' AND message_id IS NULL",
    )
    if count == "1":
        return True
    # Retain the response for the report, but do not fail the PDR acceptance
    # journey: this is a separate, legacy notification surface.
    print(f"notification probe unsupported: count={count} response={output[-500:]}", flush=True)
    return False


def acquire_recipient_lock(pid: int) -> tuple[str, str]:
    """Hold the recipient row lock in one SQL session until marker removal."""
    lock_name = f"issue331_staff_{os.getpid()}"
    marker = f"/tmp/{lock_name}.release"
    sql_text = (
        "SET autocommit=0; START TRANSACTION; "
        f"SELECT save_revision FROM player_data WHERE pid={pid} FOR UPDATE; "
        f"SELECT GET_LOCK('{lock_name}',0);"
    )
    release_text = f"SELECT RELEASE_LOCK('{lock_name}'); ROLLBACK;"
    shell = (
        "{ printf '%s\\n' " + shlex.quote(sql_text) + "; "
        "while test -f " + shlex.quote(marker) + "; do sleep 0.1; done; "
        "printf '%s\\n' " + shlex.quote(release_text) + "; } | "
        "mysql --protocol=tcp --host=127.0.0.1 --port=3306 -uroot " + shlex.quote(DB_NAME)
    )
    # The background mysql session must start with the release marker present;
    # release_recipient_lock removes it after the fenced-login/concurrency probe.
    # Without this, the shell loop exits immediately and the row lock is gone
    # before wait_for_lock can observe it.
    base.docker("exec", DB_CONTAINER, "touch", marker)
    base.docker(
        "exec", "-d", "-e", f"MYSQL_PWD={DB_PASSWORD}", DB_CONTAINER,
        "sh", "-lc", shell,
    )
    return lock_name, marker


def wait_for_lock(wrapper: Path, lock_name: str, *, timeout: int = 20) -> None:
    deadline = time.monotonic() + timeout
    last = "0"
    while time.monotonic() < deadline:
        last = sql_one(wrapper, f"SELECT IF(IS_USED_LOCK('{lock_name}') IS NULL,0,1)")
        if last == "1":
            return
        time.sleep(0.2)
    raise JourneyFailure(f"recipient SQL lock did not become visible: {last}")


def release_recipient_lock(wrapper: Path, lock_name: str, marker: str) -> None:
    base.docker("exec", DB_CONTAINER, "rm", "-f", marker, check=False)
    deadline = time.monotonic() + 20
    last = "1"
    while time.monotonic() < deadline:
        last = sql_one(wrapper, f"SELECT IF(IS_USED_LOCK('{lock_name}') IS NULL,0,1)")
        if last == "0":
            return
        time.sleep(0.2)
    raise JourneyFailure(f"recipient SQL lock did not release: {last}")


def attempt_fenced_login(journey, account: str, character: str) -> str:
    """Attempt recipient login while the live PDR save/login fence is held."""
    journey.ACCOUNT = account
    journey.PASSWORD = PASSWORD
    journey.CHARACTER = character
    client = journey.MudClient(GAME_PORT)
    try:
        entry, output = client.expect_any(("term type", "account name"), timeout=30)
        if entry == "term type":
            client.send("9")
            _, text = expect_response(client, ("account name",), timeout=15)
            output += text
        client.send(account)
        _, text = expect_response(client, ("enter your password",), timeout=15)
        output += text
        client.send(PASSWORD)
        _, text = expect_response(client, ("PRESS RETURN",), timeout=15)
        output += text
        client.send("")
        _, text = expect_response(client, ("Please select an option",), timeout=15)
        output += text
        # `Please select an option` can arrive before a delayed character-list
        # frame.  Discard only that already-consumed menu tail so a stale
        # character name cannot satisfy the next selection assertion.
        client.pending.clear()
        client.send("1")
        matched, text = expect_response(
            client,
            ("temporarily unavailable", "being saved", "try again", "unavailable", character),
            timeout=15,
        )
        output += text
        if matched != character or any(
            marker in output.lower()
            for marker in ("temporarily unavailable", "being saved", "try again", "unavailable")
        ):
            # The native fence can reject the character at selection time,
            # before the normal `Play as` confirmation is emitted.
            return output
        client.send("1")
        matched, text = expect_response(
            client,
            ("Play as", "temporarily unavailable", "being saved", "try again", "unavailable"),
            timeout=15,
        )
        output += text
        if matched != "Play as":
            # A stale character-list match can consume the first selection
            # response; accept the native fence refusal on this retry too.
            return output
        client.send("y")
        try:
            matched, text = expect_response(
                client,
                (
                    "The Regression Arena", "Please select an option", "currently",
                    "being saved", "try again", "unavailable", "cannot", "failed",
                ),
                timeout=12,
            )
            output += text
            if matched == "The Regression Arena":
                raise JourneyFailure(
                    "recipient reached the game room while the staff submission was pending"
                )
        except AssertionError as exc:
            # A fenced login may be closed without a final prompt.  The client
            # transcript is retained and the room marker remains the decisive
            # admission signal.
            output += bytes(client.pending).decode("utf-8", errors="replace")
            if "The Regression Arena" in output:
                raise JourneyFailure(
                    "recipient reached the game room while the staff submission was pending"
                ) from exc
        return output
    finally:
        client.close()


def assert_recovered(wrapper: Path, pid: int, source_timer: int, plan: dict,
                    timer_reference: tuple[int, int] | None = None) -> tuple[str, tuple[int, int]]:
    timing = next(
        row["artifact_timing"] for row in plan["items"]
        if row.get("item_uid") == 51003 and row.get("eligible") is True
    )
    usable = int(timing["usable_lifetime_seconds"])
    row = sql_one(wrapper, f"""
SELECT CONCAT(
 (SELECT COUNT(*) FROM player_death_restitution_delivery
    WHERE recipient_pid={pid} AND death_revision={DEATH_REVISION}), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid IN (51000,51001,51002,51003,51005)), '|',
 (SELECT COUNT(*) FROM player_items child JOIN player_items bag
    ON bag.pid=child.pid AND bag.obj_uid=51000
    WHERE child.pid={pid} AND child.obj_uid=51001 AND child.container_id=bag.id), '|',
 (SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid IN (51000,51002,51003,51005)
    AND container_id IS NULL), '|',
 (SELECT COUNT(*) FROM player_item_extra_descr descr
    JOIN player_items item ON item.id=descr.item_id
    WHERE item.pid={pid} AND item.obj_uid=51005 AND descr.keyword='SPELLBOOK'
      AND descr.description='[601,602]'), '|',
 (SELECT COUNT(*) FROM artifact_domain_state WHERE vnum={ARTIFACT_VNUM}
    AND location={pid} AND loc_type=3 AND item_uid=51003
    AND timer_epoch > UNIX_TIMESTAMP()), '|',
 (SELECT COUNT(*) FROM artifacts_mortal WHERE vnum={ARTIFACT_VNUM}
    AND location={pid} AND locType=3 AND UNIX_TIMESTAMP(timer) > UNIX_TIMESTAMP())
 , '|',
 (SELECT COUNT(*) FROM artifacts WHERE vnum={ARTIFACT_VNUM}
    AND location={pid} AND locType=3 AND UNIX_TIMESTAMP(timer) > UNIX_TIMESTAMP())
 , '|',
 COALESCE((SELECT timer_epoch FROM artifact_domain_state
    WHERE vnum={ARTIFACT_VNUM} AND location={pid} AND loc_type=3 AND item_uid=51003), -1), '|',
 COALESCE((SELECT UNIX_TIMESTAMP(timer) FROM artifacts_mortal
    WHERE vnum={ARTIFACT_VNUM} AND location={pid} AND locType=3), -1), '|',
 COALESCE((SELECT UNIX_TIMESTAMP(timer) FROM artifacts
    WHERE vnum={ARTIFACT_VNUM} AND location={pid} AND locType=3), -1)
)""")
    fields = row.split("|")
    if len(fields) != 11:
        raise JourneyFailure(f"recovered graph/artifact query shape mismatch: {row!r}")
    state = "|".join(fields[:8])
    timers = [int(value) for value in fields[8:]]
    expected = "5|5|1|4|1|1|1|1"
    if state != expected:
        raise JourneyFailure(
            f"recovered graph/spellbook/artifact state mismatch: {state} expected {expected} "
            f"source_timer={source_timer} usable={usable}"
        )
    if len(set(timers)) != 1 or timers[0] < 0:
        raise JourneyFailure(f"recovered artifact timer projections disagree: {timers}")
    observed_at = int(time.time())
    remaining = timers[0] - observed_at
    if timer_reference is None:
        if remaining < usable - 30 or remaining > usable + 30:
            raise JourneyFailure(
                f"recovered artifact remaining time mismatch: remaining={remaining} "
                f"usable={usable} timer={timers[0]} observed={observed_at}"
            )
    else:
        previous_timer, previous_observed_at = timer_reference
        elapsed = max(0, observed_at - previous_observed_at)
        if timers[0] > previous_timer + 30 or timers[0] < previous_timer - elapsed - 30:
            raise JourneyFailure(
                f"recovered artifact timer did not survive reload: previous={previous_timer} "
                f"current={timers[0]} elapsed={elapsed}"
            )
    return state, (timers[0], observed_at)


def immutable_receipt_fingerprint(wrapper: Path, restitution_id_hex: str) -> str:
    """Fingerprint receipt/delivery evidence without using production verify."""
    rid = restitution_id_hex.lower()
    if len(rid) != 32 or any(char not in "0123456789abcdef" for char in rid):
        raise JourneyFailure("restitution ID is not a canonical 16-byte hex value")
    receipt = sql_one(wrapper, f"""
SELECT CONCAT_WS('|', LOWER(HEX(restitution_id)), source_pid, death_revision,
       recipient_pid, LOWER(HEX(death_operation_id)),
       COALESCE(LOWER(HEX(evidence_digest)), '<NULL>'),
       COALESCE(LOWER(HEX(plan_digest)), '<NULL>'), status,
       candidate_count, delivered_count, unresolved_count,
       COALESCE(LOWER(HEX(actor)), '<NULL>'), COALESCE(LOWER(HEX(reason)), '<NULL>'))
FROM player_death_restitution_receipt
WHERE restitution_id=UNHEX('{rid}')
""")
    if not receipt:
        raise JourneyFailure("immutable receipt fingerprint found no receipt")
    deliveries = sql(wrapper, f"""
SELECT CONCAT_WS('|', item_uid, source_pid, death_revision, recipient_pid,
       source_item_revision, delivered_item_revision, delivered_item_id,
       COALESCE(LOWER(HEX(metadata_digest)), '<NULL>'),
       COALESCE(LOWER(HEX(original_payload)), '<NULL>'))
FROM player_death_restitution_delivery
WHERE restitution_id=UNHEX('{rid}') ORDER BY item_uid
""").splitlines()
    if len(deliveries) != len(RECOVERED_UIDS):
        raise JourneyFailure(
            f"immutable delivery fingerprint expected {len(RECOVERED_UIDS)} rows, got {len(deliveries)}"
        )
    delivery_uids = []
    for line in deliveries:
        fields = line.split('|')
        if len(fields) != 9:
            raise JourneyFailure(f"immutable delivery fingerprint row shape mismatch: {line!r}")
        delivery_uids.append(int(fields[0]))
    if tuple(delivery_uids) != RECOVERED_UIDS:
        raise JourneyFailure(f"immutable delivery UID set changed: {delivery_uids!r}")
    receipt_items = sql(wrapper, f"""
SELECT CONCAT_WS('|', item_uid,
       COALESCE(artifact_vnum, '<NULL>'), COALESCE(artifact_loss_epoch, '<NULL>'),
       COALESCE(artifact_source_timer_epoch, '<NULL>'),
       COALESCE(artifact_usable_lifetime_seconds, '<NULL>'),
       COALESCE(artifact_delivered_timer_epoch, '<NULL>'),
       COALESCE(LOWER(HEX(artifact_timing_basis)), '<NULL>'),
       COALESCE(LOWER(HEX(artifact_compensation_reference)), '<NULL>'))
FROM player_death_restitution_item
WHERE restitution_id=UNHEX('{rid}') ORDER BY item_uid
""").splitlines()
    canonical = "\n".join([
        "receipt=" + receipt,
        "delivery=" + "\n".join(deliveries),
        "receipt_item=" + "\n".join(receipt_items),
    ]).encode("utf-8")
    return hashlib.sha256(canonical).hexdigest()


def _sql_hex_bytes(value: str, field: str) -> bytes:
    if value in ("", "\\N"):
        return b""
    try:
        return bytes.fromhex(value)
    except ValueError as exc:
        raise JourneyFailure(f"{field} is not hexadecimal: {value!r}") from exc


def _post_runtime_items(wrapper: Path, pid: int) -> dict[int, dict[str, object]]:
    rows = sql(wrapper, f"""
SELECT id,pid,vnum,equip_slot,COALESCE(container_id,0),quantity,weight,cost,timer,
       extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,
       HEX(name),HEX(short_descr),HEX(description),HEX(action_descr),
       bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,item_condition,obj_uid
FROM player_items WHERE pid={pid} AND obj_uid IN ({','.join(str(uid) for uid in RECOVERED_UIDS)})
ORDER BY obj_uid
""").splitlines()
    items: dict[int, dict[str, object]] = {}
    for line in rows:
        fields = line.split('\t')
        if len(fields) != 32:
            raise JourneyFailure(f"post-runtime item row shape mismatch: {line!r}")
        uid = int(fields[31])
        if uid in items:
            raise JourneyFailure(f"duplicate post-runtime player item UID {uid}")
        items[uid] = {
            "id": int(fields[0]), "pid": int(fields[1]), "vnum": int(fields[2]),
            "equip_slot": int(fields[3]), "container_id": int(fields[4]), "quantity": int(fields[5]),
            "weight": int(fields[6]), "cost": int(fields[7]), "timer": int(fields[8]),
            "extra_flags": int(fields[9]), "wear_flags": int(fields[10]), "type": int(fields[11]),
            "values": tuple(int(value) for value in fields[12:20]),
            "name": _sql_hex_bytes(fields[20], "item name"),
            "short_description": _sql_hex_bytes(fields[21], "item short description"),
            "description": _sql_hex_bytes(fields[22], "item description"),
            "action_description": _sql_hex_bytes(fields[23], "item action description"),
            "bitvectors": tuple(int(value) for value in fields[24:29]),
            "material": int(fields[29]), "condition": int(fields[30]),
        }
    if set(items) != set(RECOVERED_UIDS):
        raise JourneyFailure(f"post-runtime item UID set mismatch: {sorted(items)}")
    duplicate_rows = sql(wrapper, f"""
SELECT obj_uid,COUNT(*) FROM player_items
WHERE pid={pid} AND obj_uid IN ({','.join(str(uid) for uid in RECOVERED_UIDS)})
GROUP BY obj_uid HAVING COUNT(*) <> 1
""").splitlines()
    if duplicate_rows:
        raise JourneyFailure(f"post-runtime duplicate player rows: {duplicate_rows!r}")
    return items


def _check_post_runtime_semantics(wrapper: Path, pid: int,
                                  items: dict[int, dict[str, object]]) -> None:
    for uid, expected in EXPECTED_RUNTIME_ITEMS.items():
        actual = items[uid]
        if actual["pid"] != pid:
            raise JourneyFailure(f"post-runtime item {uid} has wrong owner pid: {actual['pid']}")
        for field, expected_value in expected.items():
            if actual[field] != expected_value:
                raise JourneyFailure(
                    f"post-runtime item {uid} {field} differs: "
                    f"{actual[field]!r} expected {expected_value!r}"
                )
    expected_bag_row_id = int(items[51000]["id"])
    for uid, actual in items.items():
        expected_container = expected_bag_row_id if uid == 51001 else 0
        if actual["container_id"] != expected_container:
            raise JourneyFailure(
                f"post-runtime item {uid} container differs: "
                f"{actual['container_id']} expected {expected_container}"
            )

    affect_rows = sql(wrapper, f"""
SELECT pi.obj_uid,ia.location,ia.modifier
FROM player_item_affects ia JOIN player_items pi ON pi.id=ia.item_id
WHERE pi.pid={pid} AND pi.obj_uid IN ({','.join(str(uid) for uid in RECOVERED_UIDS)})
ORDER BY pi.obj_uid,ia.id
""").splitlines()
    affects = {uid: set() for uid in RECOVERED_UIDS}
    for line in affect_rows:
        fields = line.split('\t')
        if len(fields) != 3:
            raise JourneyFailure(f"post-runtime affect row shape mismatch: {line!r}")
        affects[int(fields[0])].add((int(fields[1]), int(fields[2])))
    if affects != EXPECTED_RUNTIME_AFFECTS:
        raise JourneyFailure(f"post-runtime prototype affects differ: {affects!r}")

    description_rows = sql(wrapper, f"""
SELECT pi.obj_uid,HEX(ed.keyword),HEX(ed.description)
FROM player_item_extra_descr ed JOIN player_items pi ON pi.id=ed.item_id
WHERE pi.pid={pid} AND pi.obj_uid IN ({','.join(str(uid) for uid in RECOVERED_UIDS)})
ORDER BY pi.obj_uid,ed.id
""").splitlines()
    descriptions = {uid: set() for uid in RECOVERED_UIDS}
    for line in description_rows:
        fields = line.split('\t')
        if len(fields) != 3:
            raise JourneyFailure(f"post-runtime description row shape mismatch: {line!r}")
        descriptions[int(fields[0])].add(
            (_sql_hex_bytes(fields[1], "description keyword"), _sql_hex_bytes(fields[2], "description text"))
        )
    for uid in RECOVERED_UIDS:
        expected = {EXPECTED_RECOVERED_DESCRIPTION}
        if uid != 51005 and descriptions[uid] != expected:
            raise JourneyFailure(
                f"post-runtime descriptions for UID {uid} differ: {descriptions[uid]!r} expected {expected!r}"
            )
    spellbook = descriptions[51005]
    master_markers = {
        text for keyword, text in spellbook
        if keyword == b"SPELLBOOK" and text != b"[601,602]"
    }
    if len(master_markers) != 1:
        raise JourneyFailure(
            f"post-runtime spellbook prototype normalization count differs: {master_markers!r}"
        )
    master_marker = next(iter(master_markers))
    if hashlib.sha256(master_marker).hexdigest() != MASTER_SPELLBOOK_MARKER_SHA256:
        raise JourneyFailure("post-runtime master spellbook marker differs from FillMasterSpellBook source output")
    try:
        spell_ids = json.loads(master_marker.decode("ascii"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise JourneyFailure("post-runtime master spellbook marker is not canonical JSON") from exc
    if (not isinstance(spell_ids, list) or
            any(not isinstance(value, int) for value in spell_ids) or
            spell_ids != sorted(set(spell_ids))):
        raise JourneyFailure("post-runtime master spellbook marker is not a sorted unique spell list")
    expected_spellbook = {
        EXPECTED_RECOVERED_DESCRIPTION,
        (b"SPELLBOOK", b"[601,602]"),
        EXPECTED_MASTER_DESCRIPTION,
        (b"SPELLBOOK", master_marker),
    }
    if spellbook != expected_spellbook:
        raise JourneyFailure(
            f"post-runtime spellbook descriptions differ: {spellbook!r} expected {expected_spellbook!r}"
        )


def _check_post_runtime_graph(wrapper: Path, pid: int, restitution_id_hex: str,
                              items: dict[int, dict[str, object]]) -> None:
    expected_graph = {
        51000: (51000, 0), 51001: (51000, 51000), 51002: (51002, 0),
        51003: (51003, 0), 51005: (51005, 0),
    }
    delivery_rows = sql(wrapper, f"""
SELECT item_uid,delivered_item_revision,delivered_item_id
FROM player_death_restitution_delivery
WHERE restitution_id=UNHEX('{restitution_id_hex.lower()}') ORDER BY item_uid
""").splitlines()
    deliveries = {}
    for line in delivery_rows:
        fields = line.split('\t')
        if len(fields) != 3:
            raise JourneyFailure(f"post-runtime delivery identity row shape mismatch: {line!r}")
        deliveries[int(fields[0])] = (int(fields[1]), int(fields[2]))
    if set(deliveries) != set(RECOVERED_UIDS):
        raise JourneyFailure(f"post-runtime delivery identity UID set differs: {sorted(deliveries)}")

    owner_rows = sql(wrapper, f"""
SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,
       owner_context_id,item_revision,vnum,state
FROM item_current_owner WHERE owner_type=1 AND owner_id={pid}
  AND item_uid IN ({','.join(str(uid) for uid in RECOVERED_UIDS)}) ORDER BY item_uid
""").splitlines()
    owners = {}
    for line in owner_rows:
        fields = line.split('\t')
        if len(fields) != 9:
            raise JourneyFailure(f"post-runtime owner row shape mismatch: {line!r}")
        uid = int(fields[0])
        if uid in owners:
            raise JourneyFailure(f"duplicate post-runtime ownership UID {uid}")
        owners[uid] = tuple(int(value) for value in fields[1:])
    if set(owners) != set(RECOVERED_UIDS):
        raise JourneyFailure(f"post-runtime owner UID set differs: {sorted(owners)}")
    for uid, (root_uid, parent_uid) in expected_graph.items():
        owner = owners[uid]
        expected_revision, expected_delivery_row_id = deliveries[uid]
        if expected_delivery_row_id <= 0:
            raise JourneyFailure(f"post-runtime delivery identity for UID {uid} has no delivered row ID")
        # The delivery row is immutable evidence.  Runtime save may
        # rematerialize player_items with a new mutable row ID; ownership is
        # therefore checked by UID/revision rather than equating that ID.
        expected = (root_uid, parent_uid, 1, pid, 0, expected_revision,
                    EXPECTED_RUNTIME_ITEMS[uid]["vnum"], 1)
        if owner != expected:
            raise JourneyFailure(f"post-runtime owner graph for UID {uid} differs: {owner!r} expected {expected!r}")


def assert_native_runtime_tamper_negatives(wrapper: Path, plan_path: Path) -> None:
    """Prove strict verify rejects a changed field and a truncated IST1 payload."""
    uid = 51002
    original_hex = sql_one(wrapper, f"""
SELECT LOWER(HEX(state_payload)) FROM player_death_restitution_runtime WHERE item_uid={uid}
""")
    if not original_hex:
        raise JourneyFailure("native tamper regression found no runtime payload")
    original = bytes.fromhex(original_hex)
    if len(original) <= 24 or original[:6] != b"IST1\x01\x00":
        raise JourneyFailure("native tamper regression did not find an IST1 payload with a stable header")
    changed = bytearray(original)
    changed[24] ^= 1  # native item weight; magic/version/flags remain unchanged
    truncated = original[:-1]
    mutations = ("changed-field", bytes(changed), "truncated", truncated)
    try:
        for label, payload in ((mutations[0], mutations[1]), (mutations[2], mutations[3])):
            if payload[:6] != original[:6]:
                raise JourneyFailure(f"{label} tamper changed the IST1 header")
            encoded = payload.hex()
            sql(wrapper, f"""
UPDATE player_death_restitution_runtime
SET state_payload=UNHEX('{encoded}'),
    state_digest=UNHEX(SHA2(UNHEX('{encoded}'),256))
WHERE item_uid={uid}
""")
            result = base.cli(wrapper, ["verify", "--plan", str(plan_path)], timeout=180, check=False)
            output = ((result.stdout or "") + (result.stderr or "")).lower()
            if result.returncode == 0 or "exact runtime metadata payload differs" not in output:
                raise JourneyFailure(
                    f"strict native verify accepted {label} tamper: "
                    f"returncode={result.returncode} output={output[-1000:]}"
                )
    finally:
        sql(wrapper, f"""
UPDATE player_death_restitution_runtime
SET state_payload=UNHEX('{original_hex}'),
    state_digest=UNHEX(SHA2(UNHEX('{original_hex}'),256))
WHERE item_uid={uid}
""")
    if sql_one(wrapper, f"SELECT LOWER(HEX(state_payload)) FROM player_death_restitution_runtime WHERE item_uid={uid}") != original_hex:
        raise JourneyFailure("native tamper regression did not restore the original payload")


def post_runtime_acceptance(wrapper: Path, pid: int, source_timer: int, plan: dict,
                            restitution_id_hex: str, pre_login_fingerprint: str,
                            timer_reference: tuple[int, int]) -> tuple[str, str]:
    """Accept materialized runtime state without weakening production verify."""
    after_fingerprint = immutable_receipt_fingerprint(wrapper, restitution_id_hex)
    if after_fingerprint != pre_login_fingerprint:
        raise JourneyFailure(
            "immutable receipt/original-payload/delivery fingerprint changed across login/save/replay: "
            f"before={pre_login_fingerprint} after={after_fingerprint}"
        )
    items = _post_runtime_items(wrapper, pid)
    _check_post_runtime_semantics(wrapper, pid, items)
    _check_post_runtime_graph(wrapper, pid, restitution_id_hex, items)
    state, timer = assert_recovered(wrapper, pid, source_timer, plan, timer_reference)
    return state, after_fingerprint


def player_view(journey, wrapper: Path, account: str, character: str, pid: int,
                *, expect_notification: bool = False) -> str:
    client = login_player(journey, account, character)
    try:
        transcript = bytes(client.transcript).decode("utf-8", errors="replace")
        if expect_notification and "issue331-notification" not in transcript:
            # SQL deletion/readback below is the stronger assertion if the
            # server's prompt ordering hides the message from the transcript.
            print("notification text was not present in login transcript", flush=True)
        inventory = base.command(client, "inventory", ("recovered leather bag",), timeout=30)
        if ("recovered wooden mace" not in inventory or
                "unique pair of recovered gloves" not in inventory or
                "recovered spellbook" not in inventory):
            raise JourneyFailure(f"recovered inventory incomplete: {inventory[-2000:]}")
        nested = base.command(client, "look in qabag", ("recovered banana",), timeout=30)
        if "recovered banana" not in nested:
            raise JourneyFailure("recovered nested graph is not visible in-game")
        artifacts = base.command(client, "artifacts unique", (character, "67259", "unique pair"), timeout=30)
        if character.lower() not in artifacts.lower():
            raise JourneyFailure("recovered artifact is not visible in the recipient unique list")
        save_and_wait(client, wrapper, character, pid)
        close_account(client)
    except Exception:
        client.close()
        raise
    return transcript


def report(lines: list[str]) -> None:
    PLAN_DIR.mkdir(parents=True, exist_ok=True)
    path = PLAN_DIR / "issue331-staff-recovery-journey-last.txt"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    path.chmod(stat.S_IRUSR | stat.S_IWUSR)


def main() -> int:
    if not ARTIFACT.is_file() or not os.access(ARTIFACT, os.X_OK):
        raise JourneyFailure(f"verified server artifact is unavailable: {ARTIFACT}")
    PLAN_DIR.mkdir(parents=True, exist_ok=True)
    temp = Path(tempfile.mkdtemp(prefix="issue331-staff-", dir=str(PLAN_DIR)))
    wrapper_bin = temp / "bin"
    wrapper_bin.mkdir(mode=0o700)
    wrapper = base.write_mysql_wrapper(wrapper_bin)
    evidence = ["issue331/#375 staff recovery journey: dedicated disposable runtime"]
    server_started = False
    lock_name = ""
    lock_marker = ""
    lock_held = False
    staff_client = None
    try:
        base.wait_for_database(wrapper)
        base.prepare_schema(wrapper, wrapper_bin)
        verify_migration21(wrapper)
        evidence.append("migration 0021 schema/index compatibility: verified")
        base.prepare_runtime_root(temp / "run_root")
        base.stage_runtime(temp / "run_root")
        base.start_server()
        server_started = True
        sys.path.insert(0, str(ASYNC_DIR))
        import test_flatfile_combat_journey as journey  # type: ignore
        install_client(journey)

        recipient_pid = create_player(journey, wrapper, RECIPIENT_ACCOUNT, RECIPIENT_NAME, pack_starter_items=True)
        staff_pid = create_player(journey, wrapper, STAFF_ACCOUNT, STAFF_NAME)
        observer_pid = create_player(journey, wrapper, OBSERVER_ACCOUNT, OBSERVER_NAME)
        evidence.append("recipient/staff/observer accounts created through real login flow")

        base.stop_server()
        server_started = False
        sql(wrapper, f"UPDATE player_data SET level=61 WHERE pid={staff_pid}")
        base.start_server()
        server_started = True
        recipient = login_player(journey, RECIPIENT_ACCOUNT, RECIPIENT_NAME)
        try:
            save_and_wait(recipient, wrapper, RECIPIENT_NAME, recipient_pid)
            close_account(recipient)
        except Exception:
            recipient.close()
            raise
        evidence.append("recipient cold reconnect and SQL-acknowledged save: verified")

        base.stop_server()
        server_started = False
        owner_revision = int(sql_one(
            wrapper,
            f"SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id={recipient_pid} AND owner_context_id=0",
        )) + 1
        payload = base.build_fixture(temp, recipient_pid, owner_revision)
        base.seed_fixture(wrapper, recipient_pid, payload, owner_revision)
        mirror_artifact_legacy_projection(wrapper, ARTIFACT_VNUM)
        source_timer = int(sql_one(
            wrapper, f"SELECT timer_epoch FROM artifact_domain_state WHERE vnum={ARTIFACT_VNUM}"
        ))
        evidence.append("quarantined graph/artifact fixture and original spellbook: seeded")
        base.start_server()
        server_started = True

        inspect_path = temp / "inspect.json"
        plan_path = temp / "plan.json"
        export_path = temp / "staff-payload.json"
        inspect_result = base.cli(
            wrapper,
            ["inspect", "--pid", str(recipient_pid), "--death-revision", str(DEATH_REVISION),
             "--recipient-pid", str(recipient_pid), "--artifact", str(inspect_path)],
            timeout=180,
        )
        plan_result = base.cli(
            wrapper,
            ["plan", "--inspect", str(inspect_path), "--artifact", str(plan_path),
             "--approve-artifact-reconciliation"],
            timeout=180,
        )
        plan = json.loads(plan_path.read_text(encoding="utf-8"))
        if not plan.get("applyable") or plan.get("eligible_count") != 5:
            raise JourneyFailure(
                f"canonical builder did not produce the limited applyable plan: "
                f"applyable={plan.get('applyable')} eligible={plan.get('eligible_count')}"
            )
        export_result = base.cli(
            wrapper,
            ["export", "--plan", str(plan_path), "--inspect", str(inspect_path),
             "--artifact", str(export_path), "--overwrite", "--approve",
             "--actor", STAFF_NAME, "--reason", "disposable staff recovery acceptance"],
            timeout=180,
        )
        staff_payload = json.loads(export_path.read_text(encoding="utf-8"))
        chunks = staff_payload.get("chunks")
        if not isinstance(chunks, list) or not chunks or "canonical_hex" not in staff_payload:
            raise JourneyFailure("canonical builder export did not produce a chunked payload")
        canonical_hex = staff_payload["canonical_hex"]
        # The export chunks are codec-sized (1022 hex chars), but the live
        # command line is MAX_INPUT_LENGTH=1024 including `restitution chunk `.
        # Reframe the exact exported canonical bytes for transport; do not
        # decode/rebuild or otherwise alter the approved command.
        transport_chunks = [
            canonical_hex[offset:offset + 800]
            for offset in range(0, len(canonical_hex), 800)
        ]
        if "".join(transport_chunks) != canonical_hex:
            raise JourneyFailure("staff transport chunking changed the exported canonical payload")
        evidence.append(
            f"canonical inspect/plan/export: verified eligible={plan['eligible_count']} "
            f"export_chunks={len(chunks)} transport_chunks={len(transport_chunks)}"
        )
        enable_sql_trace(wrapper)

        staff_client = login_player(journey, STAFF_ACCOUNT, STAFF_NAME)
        lock_name, lock_marker = acquire_recipient_lock(recipient_pid)
        wait_for_lock(wrapper, lock_name)
        lock_held = True
        expect_response(
            staff_client,
            ("Regression Arena",),
            timeout=2,
        ) if False else None
        staff_client.send("restitution begin")
        matched, _ = expect_response(staff_client, ("staging started",), timeout=30)
        if matched != "staging started":
            raise JourneyFailure("staff staging did not start")
        for index, chunk in enumerate(transport_chunks):
            staff_client.send(f"restitution chunk {chunk}")
            matched, _ = expect_response(staff_client, ("chunk accepted",), timeout=30)
            if matched != "chunk accepted":
                raise JourneyFailure(f"staff chunk {index} was not accepted")
        staff_client.send("restitution commit")
        matched, commit_output = expect_response(
            staff_client,
            ("Restitution submission accepted", "journal-uncertain",
             "recipient must be offline", "pending save", "fence is unavailable",
             "coordinator is unavailable", "not a canonical"),
            timeout=30,
        )
        if matched not in ("Restitution submission accepted", "journal-uncertain"):
            raise JourneyFailure(f"actual staff submission rejected: {commit_output[-1500:]}")
        evidence.append(f"actual in-game staff submission: verified ({matched})")

        fence_transcript = attempt_fenced_login(journey, RECIPIENT_ACCOUNT, RECIPIENT_NAME)
        evidence.append("offline recipient login was rejected while native fence was held: verified")

        observer = login_player(journey, OBSERVER_ACCOUNT, OBSERVER_NAME)
        try:
            base.command(observer, "look", ("The Regression Arena",), timeout=30)
            save_and_wait(observer, wrapper, OBSERVER_NAME, observer_pid)
            close_account(observer)
        except Exception:
            observer.close()
            raise
        evidence.append("concurrent unrelated observer look/save while recipient was fenced: verified")

        # Release the row only after the login rejection and unrelated work have
        # been observed.  The live critical worker then commits the canonical
        # recovery; no CLI apply is used as the primary submission.
        release_recipient_lock(wrapper, lock_name, lock_marker)
        lock_held = False
        wait_sql(
            wrapper,
            f"SELECT COUNT(*) FROM player_death_restitution_delivery WHERE recipient_pid={recipient_pid} AND death_revision={DEATH_REVISION}",
            "5",
            timeout=180,
        )
        # Verify original delivery before materialization/save can legitimately
        # change container weights or add prototype-derived metadata.
        base.cli(wrapper, ["verify", "--plan", str(plan_path)], timeout=180)
        evidence.append("strict pre-login delivery metadata/ownership verification: passed")
        assert_native_runtime_tamper_negatives(wrapper, plan_path)
        evidence.append("strict native tamper negatives: changed field and truncated IST1 payload rejected")
        immutable_before_login = immutable_receipt_fingerprint(wrapper, plan["restitution_id_hex"])
        evidence.append(f"immutable receipt/original-payload/delivery fingerprint before login: {immutable_before_login}")
        recovered_plan = plan
        recovered_state, timer_reference = assert_recovered(wrapper, recipient_pid, source_timer, recovered_plan)
        evidence.append(f"native SQL recovery readback graph/spellbook/artifact timer: verified ({recovered_state})")

        notification = migration21_notification_probe(
            journey, wrapper, recipient_pid, RECIPIENT_NAME, staff_client
        )
        if notification:
            evidence.append("actual legacy offline notification SQL path under migration21: verified")
        else:
            evidence.append("notification limitation: PDR has no collector receipt emission; legacy probe not observed")
        close_account(staff_client)
        staff_client = None

        player_view(
            journey, wrapper, RECIPIENT_ACCOUNT, RECIPIENT_NAME, recipient_pid,
            expect_notification=notification,
        )
        if notification:
            wait_sql(
                wrapper,
                f"SELECT COUNT(*) FROM offline_messages WHERE pid={recipient_pid}",
                "0",
                timeout=30,
            )
        evidence.append("recipient reconnect/in-game graph inspection/save: verified")

        base.stop_server()
        server_started = False
        base.start_server()
        server_started = True
        player_view(journey, wrapper, RECIPIENT_ACCOUNT, RECIPIENT_NAME, recipient_pid)
        state_after_restart, _ = assert_recovered(
            wrapper, recipient_pid, source_timer, plan, timer_reference
        )
        evidence.append(f"runtime restart/reconnect preserved graph/spellbook/artifact timer: verified ({state_after_restart})")
        base.stop_server()
        server_started = False

        proof = temp / "offline-proof.txt"
        base.offline_proof(proof)
        replay = base.cli(
            wrapper,
            ["apply", "--plan", str(plan_path), "--offline-proof", str(proof), "--approve",
             "--approve-artifact-reconciliation", "--actor", "issue331-staff-recovery-journey",
             "--reason", "disposable staff recovery replay"],
            timeout=180,
        )
        replay_text = (replay.stdout + replay.stderr).lower()
        if "already applied" not in replay_text:
            raise JourneyFailure(f"replay was not idempotent: {replay.stdout[-1000:]} {replay.stderr[-1000:]}")
        post_state, immutable_after_replay = post_runtime_acceptance(
            wrapper, recipient_pid, source_timer, plan, plan["restitution_id_hex"],
            immutable_before_login, timer_reference,
        )
        evidence.append(
            "post-save acceptance (immutable fingerprint, fixture/prototype normalization, "
            f"UID graph, artifact timer, no duplicate): verified ({post_state})"
        )
        evidence.append(f"immutable receipt/original-payload/delivery fingerprint after replay: {immutable_after_replay}")
        evidence.append(
            "post-save strict CLI verify intentionally not used: production strict verification is bounded "
            "to immediate delivery before login/materialization; runtime normalization is checked explicitly"
        )
        report(evidence)
        print("ISSUE331_STAFF_RECOVERY_JOURNEY_OK")
        for line in evidence[1:]:
            print(line)
        return 0
    except Exception as exc:
        evidence.append("FAILED: " + str(exc))
        if SQL_TRACE:
            try:
                evidence.append("sql_trace:\n" + read_sql_trace(wrapper))
            except Exception as trace_exc:
                evidence.append("sql_trace unavailable: " + str(trace_exc))
        try:
            evidence.append("server_output:")
            evidence.append(base.server_output())
        except Exception:
            pass
        report(evidence)
        raise
    finally:
        if staff_client is not None:
            try:
                staff_client.close()
            except Exception:
                pass
        if lock_held and lock_name and lock_marker:
            try:
                release_recipient_lock(wrapper, lock_name, lock_marker)
            except Exception:
                pass
        if server_started:
            try:
                base.stop_server()
            except Exception:
                pass
        shutil.rmtree(temp, ignore_errors=True)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (base.HarnessError, JourneyFailure, AssertionError) as exc:
        print(f"ISSUE331_STAFF_RECOVERY_JOURNEY_FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1)
