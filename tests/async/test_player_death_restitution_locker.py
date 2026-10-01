#!/usr/bin/env python3
"""Focused regression tests for deleted-character locker restitution."""

from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "player_death_restitution", ROOT / "scripts" / "player_death_restitution.py"
)
assert SPEC is not None and SPEC.loader is not None
cli = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cli)


def item_metadata(*, artifact: bool = False) -> dict[str, object]:
    payload = b"captured-item"
    return {
        "object_uid": 100,
        "parent_index": 0,
        "vnum": 677,
        "type": 5,
        "string_mask": 0,
        "name_hex": "",
        "short_description_hex": "",
        "description_hex": "",
        "action_description_hex": "",
        "values": [0] * 8,
        "bitvectors": [0] * 5,
        "weight": 2,
        "cost": 3,
        "timers": [0] * 6,
        "extra_flags": cli.ITEM_ARTIFACT if artifact else 0,
        "wear_flags": 1,
        "material": 1,
        "condition": 100,
        "affects": [],
        "extra_descriptions": [],
        "item_payload_hex": payload.hex(),
    }


def deleted_destination() -> dict[str, object]:
    bag_uid = 9000
    return {
        "destination": cli.DESTINATION_ACCOUNT_LOCKER_BAG,
        "recipient_pid": 42,
        "deleted_character": {
            "pid": 42,
            "account_name": "acct42",
            "character_name": "Deletedchar",
            "racewar": 1,
            "deleted_at": "2026-09-20 01:02:03",
            "account_character_corroboration": None,
        },
        "account_name": "acct42",
        "locker": {
            "locker_name": "account.acct42.1.locker",
            "exists": False,
            "locker_id": None,
            "owner_pid": 0,
            "owner_assoc_id": 0,
            "racewar": 1,
            "race": 0,
            "public_chest": {"exists": False, "chest_id": None},
            "owner_revision_present": False,
            "owner_revision": 0,
            "item_count": 0,
        },
        "allocator_next_uid": bag_uid,
        "bag": {
            "item_uid": bag_uid,
            **cli.restitution_bag_metadata("Deletedchar", 42, 7),
        },
    }


def deleted_inspection(*, artifact: bool = False) -> dict[str, object]:
    item = item_metadata(artifact=artifact)
    return {
        "source": {
            "pid": 42,
            "death_revision": 7,
            "operation_id_hex": "20" * 16,
            "loss_epoch": 1_700_000_000,
            "corpse_item_uid": 999,
            "corpse_room_vnum": 3001,
            "wallet_revision": 5,
            "wallet_pile_uid": 0,
            "wallet_before": [0, 0, 0, 0],
        },
        "recipient_pid": 42,
        "payload_digest": hashlib.sha256(b"death-payload").hexdigest(),
        "evidence_digest": hashlib.sha256(b"evidence").hexdigest(),
        "decoded": {
            "death": {
                "wallet_pile_uid": 0,
                "corpse": [
                    {"object_uid": 999, "parent_index": -1},
                    item,
                ],
            }
        },
        "custody_db": [{
            "item_uid": 100,
            "root_item_uid": 100,
            "parent_item_uid": 0,
            "expected_item_revision": 10,
            "vnum": 677,
            "expected_state": cli.STATE_ACTIVE,
            "owner_type": cli.OWNER_PLAYER,
            "owner_id": 42,
            "owner_context_id": 0,
            "owner_revision": 5,
        }],
        "current_owners": {
            "100": {
                "item_uid": 100,
                "root_item_uid": 100,
                "parent_item_uid": 0,
                "owner_type": cli.OWNER_PLAYER,
                "owner_id": 42,
                "owner_context_id": 0,
                "item_revision": 11,
                "owner_revision": 6,
                "vnum": 677,
                "state": cli.STATE_QUARANTINED,
            }
        },
        "player_projections": [],
        "player_authority": {},
        "recipient_existing_uids": [],
        "related_deaths": [],
        "related_custody": [],
        "item_loss_epochs": {},
        "deliveries": {},
        "artifacts": {"domain": {}, "mortal": {}, "god": {}},
        "destination_evidence": deleted_destination(),
        "consistency_errors": [],
    }


class DestinationDB:
    database = "duris_deleted_destination_test"

    def __init__(self) -> None:
        self.queries: list[str] = []

    def run(self, query: str) -> list[list[str]]:
        self.queries.append(query)
        if "FROM frag_leaderboard WHERE pid=42" in query:
            return [["42", b"acct42".hex(), b"Deletedchar".hex(), "1", "2026-09-20 01:02:03"]]
        if "FROM accounts WHERE" in query:
            return [[b"Acct42".hex()]]
        if "FROM account_characters WHERE pid=42" in query:
            # A same-name replacement with a new PID is deliberately outside
            # this PID-bound query and cannot become the restitution target.
            return []
        if "FROM lockers WHERE locker_name=" in query:
            return []
        if "FROM item_uid_allocator WHERE allocator_id=1" in query:
            return [["9000"]]
        raise AssertionError(f"unexpected destination query: {query}")

    def scalar(self, query: str) -> str:
        raise AssertionError(f"unexpected destination scalar: {query}")


class DeletedCharacterLockerRestitutionTests(unittest.TestCase):
    def test_deleted_pid_resolves_to_surviving_account_not_reused_name(self) -> None:
        db = DestinationDB()
        with mock.patch.object(cli, "table_exists", return_value=True), mock.patch.object(
            cli, "fetch_current_owners", return_value={}
        ):
            destination = cli.fetch_restitution_destination(db, 42, 42, {}, 7)
        self.assertEqual(destination["destination"], cli.DESTINATION_ACCOUNT_LOCKER_BAG)
        self.assertEqual(destination["account_name"], "Acct42")
        self.assertEqual(destination["locker"]["locker_name"], "account.acct42.1.locker")
        self.assertEqual(destination["bag"]["item_uid"], 9000)
        self.assertEqual(destination["bag"]["original_character_name"], "Deletedchar")
        self.assertTrue(all("char_name=" not in query for query in db.queries))

    def test_plan_nests_exact_items_under_marked_transient_locker_bag(self) -> None:
        plan = cli.plan_from_inspection(deleted_inspection())
        self.assertTrue(plan["applyable"])
        self.assertFalse(plan["exportable"])
        self.assertEqual(plan["destination"], cli.DESTINATION_ACCOUNT_LOCKER_BAG)
        self.assertEqual(plan["eligible_count"], 1)
        item = plan["items"][0]
        self.assertEqual(item["delivered_root_item_uid"], 9000)
        self.assertEqual(item["delivered_parent_item_uid"], 9000)
        bag = plan["locker_delivery"]["bag"]
        self.assertEqual(bag["extra_flags"] & cli.ITEM_TRANSIENT, cli.ITEM_TRANSIENT)
        self.assertEqual(bag["marker_keyword"], cli.RESTITUTION_BAG_MARKER)
        self.assertIn("Deletedchar", bag["short_description"])

        sql = cli.build_apply_sql(plan, "test", "deleted character locker restitution")
        self.assertIn("INSERT INTO locker_items", sql)
        self.assertIn("UPDATE item_uid_allocator SET next_uid=next_uid+1", sql)
        self.assertIn("owner_type=5", sql)
        self.assertIn(cli.RESTITUTION_BAG_MARKER.encode().hex(), sql.lower())
        self.assertNotIn("INSERT INTO player_items", sql)
        self.assertIn("@recipient_deleted_ok", sql)
        self.assertIn("BINARY char_name=BINARY UNHEX('44656c6574656463686172')", sql)
        self.assertIn(
            "CREATE TEMPORARY TABLE restitution_apply_parents LIKE restitution_apply_items;", sql
        )
        parent_gate = next(line for line in sql.splitlines() if line.startswith("SET @parent_ok="))
        self.assertEqual(parent_gate.count("restitution_apply_items"), 1)
        self.assertIn("JOIN restitution_apply_parents pp", parent_gate)

    def test_artifact_for_deleted_character_remains_quarantined(self) -> None:
        plan = cli.plan_from_inspection(deleted_inspection(artifact=True))
        self.assertFalse(plan["applyable"])
        self.assertEqual(plan["eligible_count"], 0)
        self.assertEqual(plan["items"][0]["classification"], "artifact_recipient_deleted")
        self.assertIn("artifact_recipient_deleted", plan["blocking_classifications_present"])

    def test_artifact_blocks_partial_locker_delivery_of_mixed_payload(self) -> None:
        inspection = deleted_inspection(artifact=True)
        ordinary = item_metadata()
        ordinary["object_uid"] = 101
        ordinary["vnum"] = 678
        inspection["decoded"]["death"]["corpse"].append(ordinary)
        inspection["custody_db"].append({
            "item_uid": 101,
            "root_item_uid": 101,
            "parent_item_uid": 0,
            "expected_item_revision": 10,
            "vnum": 678,
            "expected_state": cli.STATE_ACTIVE,
            "owner_type": cli.OWNER_PLAYER,
            "owner_id": 42,
            "owner_context_id": 0,
            "owner_revision": 5,
        })
        inspection["current_owners"]["101"] = {
            "item_uid": 101,
            "root_item_uid": 101,
            "parent_item_uid": 0,
            "owner_type": cli.OWNER_PLAYER,
            "owner_id": 42,
            "owner_context_id": 0,
            "item_revision": 11,
            "owner_revision": 6,
            "vnum": 678,
            "state": cli.STATE_QUARANTINED,
        }

        plan = cli.plan_from_inspection(inspection)

        self.assertFalse(plan["applyable"])
        self.assertEqual(plan["eligible_count"], 1)
        self.assertEqual(
            {item["classification"] for item in plan["items"]},
            {"artifact_recipient_deleted", "recoverable_exact"},
        )
        with self.assertRaisesRegex(
            cli.ToolError, "artifacts block partial account-locker restitution"
        ):
            cli.build_apply_sql(plan, "test", "mixed deleted character payload")

    def test_server_contract_keeps_bag_in_locker_and_warns_on_login(self) -> None:
        source = (ROOT / "src" / "player" / "player_death_restitution_locker.c").read_text()
        handler = (ROOT / "src" / "world" / "handler.c").read_text()
        nanny = (ROOT / "src" / "account" / "nanny.c").read_text()
        makefile = (ROOT / "src" / "Makefile").read_text()
        self.assertIn("PLAYER_DEATH_RESTITUTION_BAG_MARKER", source)
        self.assertIn("ITEM RESTITUTION IS WAITING IN YOUR ACCOUNT LOCKER", source)
        self.assertIn("player_death_restitution_is_locker_bag(object)", handler)
        self.assertIn("player_death_restitution_locker_notice(ch);", nanny)
        self.assertIn("player/player_death_restitution_locker.o", makefile)


if __name__ == "__main__":
    unittest.main()
