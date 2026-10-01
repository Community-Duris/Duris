#!/usr/bin/env python3
"""Write a deterministic deleted-character locker plan and its apply SQL."""

from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "player_death_restitution", ROOT / "scripts" / "player_death_restitution.py"
)
assert SPEC is not None and SPEC.loader is not None
cli = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cli)


def inspection() -> dict[str, object]:
    item = {
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
        "extra_flags": 0,
        "wear_flags": 1,
        "material": 1,
        "condition": 100,
        "affects": [],
        "extra_descriptions": [],
        "item_payload_hex": b"captured-item".hex(),
    }
    destination = {
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
        "allocator_next_uid": 9000,
        "bag": {
            "item_uid": 9000,
            **cli.restitution_bag_metadata("Deletedchar", 42, 7),
        },
    }
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
        "evidence_digest": hashlib.sha256(b"locker-evidence").hexdigest(),
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
        "destination_evidence": destination,
        "consistency_errors": [],
    }


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: player_death_restitution_locker_fixture.py PLAN SQL")
    plan = cli.plan_from_inspection(inspection())
    if not plan["applyable"] or plan["eligible_count"] != 1:
        raise AssertionError("deleted-character fixture did not produce one applyable item")
    cli.atomic_write_json(Path(sys.argv[1]), plan)
    Path(sys.argv[2]).write_text(
        cli.build_apply_sql(plan, "test-harness", "deleted character locker fixture") + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
