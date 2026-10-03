"""Read-only death custody status derived from existing authority and receipts."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time


def correlation(pid: int, save_id: int) -> str:
    return hashlib.sha256(
        b"duris-death-recovery-v1" + struct.pack("<Q", (pid << 32) | save_id)
    ).digest()[:16].hex()


def summarize(decoded: dict, owners: list[dict], *, loss_epoch: int = 0) -> dict:
    """Conservative classification; a receipt alone never proves current delivery."""
    death = decoded["death"]
    pid, revision = decoded["pid"], decoded["revision"]
    captured = {item["object_uid"] for item in death["corpse"][1:]}
    evidence = {row["item_uid"] for row in death["custody"]} | captured
    current = {row["item_uid"]: row for row in owners}
    counts = dict.fromkeys(("durable", "restored", "quarantine", "unresolved", "safely_retired"), 0)
    items = []
    verification_pending = 0
    for uid in sorted(evidence | current.keys()):
        row = current.get(uid, {})
        custody = "unresolved"
        owner = "custody_reconciliation"
        if row.get("state") == 3:
            custody, owner = "quarantine", "reviewed_restitution"
        elif row.get("state") == 2 and row.get("owner_type") == 8:
            custody, owner = "safely_retired", "none"
        elif row.get("state") == 1 and row.get("materialized"):
            # Current physical projection plus matching authority is durable
            # custody. Restitution must also retain its delivery relationship.
            if row.get("delivery_matches"):
                custody, owner = "restored", "none"
                if not row.get("delivery_verified"):
                    owner = "restitution_verification"
                    verification_pending += 1
            else:
                custody, owner = "durable", "none"
        counts[custody] += 1
        items.append({"item_uid": uid, "captured": uid in captured,
                      "custody": custody, "recovery_owner": owner,
                      "authority": row or None})
    required = counts["quarantine"] + counts["unresolved"] + verification_pending
    terminal = ("unresolved" if counts["unresolved"] else
                "quarantine" if counts["quarantine"] else
                "restored" if counts["restored"] else
                "durable" if counts["durable"] else "safely_retired")
    if verification_pending:
        terminal = "unresolved"
    # Missing/inconsistent immutable evidence cannot be certified as an empty retirement.
    retained = death.get("conflict_evidence_retained", False)
    if retained or death.get("unresolved_operations") or any(death.get("wallet_before", [])):
        required += 1
        terminal = "unresolved"
    verification_only = verification_pending and not (
        counts["unresolved"] or death.get("unresolved_operations") or
        any(death.get("wallet_before", [])))
    return {"pid": pid, "death_revision": revision,
            "operation_id_hex": death["operation_id_hex"],
            "correlation": correlation(pid, death["corpse"][0]["values"][6]),
            "death_disposition": decoded.get("recovery_disposition", "completed_with_retained_conflict" if retained else "completed"),
            "terminal_custody": terminal,
            "exact_delivery_verification": "use_protected_restitution_verify",
            "verification_requires_review": bool(verification_pending),
            "wallet_requires_review": any(death.get("wallet_before", [])),
            "unresolved_operations": death.get("unresolved_operations", []),
            "recovery_required": bool(required),
            "recovery_owner": "retained_death_conflict" if retained else
                              "restitution_verification" if verification_only else
                              "custody_reconciliation" if terminal == "unresolved" else
                              "reviewed_restitution" if terminal == "quarantine" else "none",
            "scope": "batch", "item_uid": 0, "captured_count": len(captured),
            "authority_count": len(current), "unmatched_count": len(current.keys() - captured),
            "counts": counts, "elapsed_seconds": max(0, int(time.time()) - loss_epoch) if loss_epoch else None,
            "items": items}


def sql_owners(api, db, pid: int, decoded: dict) -> list[dict]:
    death = decoded["death"]
    uids = {item["object_uid"] for item in death["corpse"][1:]}
    uids.update(row["item_uid"] for row in death["custody"])
    roots = {row["root_item_uid"] for row in death["custody"]}
    if not uids:
        return []
    selected = "own.item_uid IN " + api.sql_list(sorted(uids))
    if roots:
        selected += " OR (own.owner_type=1 AND own.owner_id=" + str(pid) + \
                    " AND own.owner_context_id=0 AND own.root_item_uid IN " + api.sql_list(sorted(roots)) + ")"
    # One SELECT observes authority, projections and receipt relationships at
    # one DB statement boundary. No mutation, approval or delivery is inferred.
    rows = db.run(
        "SELECT own.item_uid,own.root_item_uid,COALESCE(own.parent_item_uid,0),"
        "own.owner_type,own.owner_id,own.owner_context_id,own.item_revision,own.vnum,own.state,"
        "((own.owner_type=1 AND (SELECT COUNT(*) FROM player_items pi WHERE pi.obj_uid=own.item_uid "
        "AND pi.pid=own.owner_id AND pi.vnum=own.vnum)=1) OR "
        "(own.owner_type=4 AND (SELECT COUNT(*) FROM corpse_items ci JOIN corpses c ON c.id=ci.corpse_id "
        "WHERE ci.obj_uid=own.item_uid AND ci.vnum=own.vnum AND "
        "((CAST(c.value3 AS UNSIGNED)<<32)|c.save_id)=own.owner_id)=1) OR "
        "(own.owner_type=5 AND (SELECT COUNT(*) FROM locker_items li WHERE li.obj_uid=own.item_uid "
        "AND li.vnum=own.vnum AND li.locker_id=own.owner_id AND "
        "COALESCE(li.chest_id,0)=own.owner_context_id)=1) OR "
        "(own.vnum=3 AND own.coin_payload IS NOT NULL AND OCTET_LENGTH(own.coin_payload)>0)),"
        "(d.item_uid IS NOT NULL AND receipt.status IN (2,3) AND own.state=1 AND "
        "own.item_revision>=d.delivered_item_revision AND receipt.recipient_pid=d.recipient_pid AND "
        "receipt.source_pid=" + str(pid) + " AND receipt.death_revision=" + str(decoded["revision"]) + " AND "
        "own.owner_type=1 AND own.owner_id=d.recipient_pid),"
        "COALESCE(receipt.status=3,0),COALESCE(HEX(d.restitution_id),'') FROM item_current_owner own "
        "LEFT JOIN player_death_restitution_delivery d ON d.item_uid=own.item_uid "
        "LEFT JOIN player_death_restitution_receipt receipt ON receipt.restitution_id=d.restitution_id "
        "WHERE " + selected + " ORDER BY own.item_uid"
    )
    fields = ("item_uid", "root_item_uid", "parent_item_uid", "owner_type", "owner_id",
              "owner_context_id", "item_revision", "vnum", "state", "materialized", "delivery_matches",
              "delivery_verified")
    result = []
    for row in rows:
        if len(row) != 13:
            raise api.ToolError("recovery authority row has an unexpected shape")
        item = {field: api.row_int(row, i, field, 0) for i, field in enumerate(fields)}
        item["restitution_id_hex"] = row[12].lower() or None
        result.append(item)
    return result


def flatfile_reader(api) -> Path:
    binary = api.ROOT / "bin/tools/player_death_recovery_flatfile"
    sources = [api.ROOT / p for p in (
        "scripts/player_death_recovery_flatfile.cpp", "src/flatfile/flatfile_item_repository.c",
        "src/flatfile/flatfile_world_item_repository.c", "src/flatfile/flatfile_locker_repository.c",
        "src/flatfile/flatfile_player_snapshot_file.c", "src/flatfile/flatfile_store.c",
        "src/flatfile/flatfile_authority_transaction.c", "src/player/player_snapshot_codec.c",
        "src/item/item_transfer_command.c", "src/persistence/critical_command.c")]
    dependencies = sources + list((api.ROOT / "src").rglob("*.h"))
    if binary.exists() and binary.stat().st_mtime_ns >= max(p.stat().st_mtime_ns for p in dependencies):
        return binary
    binary.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", "-ffunction-sections", "-fdata-sections",
        *map(str, sources), "-Wl,--gc-sections", "-lcrypto", "-pthread", "-o", str(binary)],
        cwd=api.ROOT, capture_output=True, timeout=120)
    if result.returncode:
        raise api.ToolError("supported flatfile recovery reader failed to build")
    return binary


def status(api, args) -> dict:
    limit = api.int_value(args.limit, "status limit", 1, 100)
    after = (api.int_value(args.after_pid, "cursor pid", 0, 2**31 - 1),
             api.int_value(args.after_revision, "cursor revision", 0, 2**64 - 1))
    cases = []
    if args.flatfile_root is not None:
        root = args.flatfile_root.resolve(strict=True)
        paths = []
        for path in (root / "player-deaths").glob("*.death"):
            match = re.fullmatch(r"([1-9][0-9]*)-([1-9][0-9]*)\.death", path.name)
            if match:
                pair = tuple(map(int, match.groups()))
                if pair > after:
                    paths.append((pair, path))
        page = sorted(paths)[:limit + 1]
        reader = flatfile_reader(api)
        for (pid, revision), path in page[:limit]:
            result = subprocess.run([str(reader), str(root), str(pid), str(revision)],
                                    capture_output=True, timeout=30)
            if result.returncode:
                raise api.ToolError("flatfile recovery read failed or authority replay is pending; preserve custody and restart recovery")
            data = json.loads(result.stdout)
            decoded = api.decode_payload(bytes.fromhex(data["payload_hex"]), recovery_status=True)
            cases.append(summarize(decoded, data["owners"], loss_epoch=int(path.stat().st_mtime)))
        backend = "flatfile"
        target = None
    else:
        info = api.load_target_info(args.target_info) if args.target_info else None
        expected = info["target"] if info else None
        policy, _ = api.policy_for_command(args, expected_target=expected)
        db = api.Mysql(policy)
        target = api.identify_target(db, policy, expected)
        api.require_schema(db)
        source = "SELECT pid,save_revision FROM player_death_disposition"
        conflicts = api.table_exists(db, "player_death_conflict_evidence")
        if conflicts:
            source += (" UNION SELECT evidence.pid,evidence.save_revision FROM player_death_conflict_evidence evidence "
                       "WHERE NOT EXISTS (SELECT 1 FROM player_death_disposition death WHERE "
                       "death.pid=evidence.pid AND death.save_revision=evidence.save_revision)")
        page = db.run("SELECT pid,save_revision FROM (" + source + ") recovery_cases WHERE "
                      "(pid>" + str(after[0]) + " OR (pid=" + str(after[0]) + " AND save_revision>" +
                      str(after[1]) + ")) ORDER BY pid,save_revision LIMIT " + str(limit + 1))
        for pair in page[:limit]:
            pid, revision = map(int, pair)
            death = api.fetch_death(db, pid, revision)
            pending = death is None
            if pending:
                rows = db.run("SELECT HEX(payload),LOWER(HEX(operation_id)),FLOOR(UNIX_TIMESTAMP(recorded_at)) "
                              "FROM player_death_conflict_evidence WHERE payload_hash=UNHEX(SHA2(payload,256)) AND pid=" + str(pid) + " AND save_revision=" + str(revision))
                if len(rows) != 1 or len(rows[0]) != 3:
                    raise api.ToolError("retained conflict identity could not be read")
                death = dict(zip(("payload_hex", "operation_id_hex", "loss_epoch"), rows[0]))
                death["loss_epoch"] = int(death["loss_epoch"])
            decoded = api.decode_payload(bytes.fromhex(death["payload_hex"]), recovery_status=True)
            if (decoded["pid"], decoded["revision"], decoded["death"]["operation_id_hex"]) != \
                    (pid, revision, death["operation_id_hex"]):
                raise api.ToolError("recovery death evidence identity conflicts")
            if pending:
                if not decoded["death"].get("conflict_evidence_retained"):
                    raise api.ToolError("retained conflict does not carry native conflict evidence")
                decoded["recovery_disposition"] = "retained_not_completed"
            else:
                decoded["death"]["custody"] = api.fetch_custody(db, pid, revision)
            cases.append(summarize(decoded, sql_owners(api, db, pid, decoded), loss_epoch=death["loss_epoch"]))
        backend = "sql"
    cursor = None
    if len(page) > limit:
        last = cases[-1]
        cursor = {"after_pid": last["pid"], "after_revision": last["death_revision"]}
    return {"format": "duris-death-recovery-status-v1", "backend": backend,
            "target": target, "scanned": len(cases), "next_cursor": cursor,
            "unresolved_cases": sum(case["recovery_required"] for case in cases),
            "cases": cases if args.include_resolved else [case for case in cases if case["recovery_required"]]}
