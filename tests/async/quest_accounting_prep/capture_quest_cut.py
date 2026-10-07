#!/usr/bin/env python3
"""Capture bounded native quest rows in one disposable SQL read-only snapshot.

No .env loading, setup, mutation or activation. Capture must be called by the
actual isolated journey at its quiescent cuts, before its schema is removed.
The primary's binary/source/schema and native world/context proof remain required.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from case_data import ACCOUNTING_PIN, CASES, ROOT, blocks, digest
from economic_restore_evidence import decode_native_mobile

MAX_ROWS = 2048
MAX_BYTES = 16 * 1024 * 1024
TASK_COLUMNS = ("quest_active", "quest_mob_vnum", "quest_type", "quest_accomplished",
                "quest_started", "quest_zone_number", "quest_giver", "quest_level",
                "quest_receiver", "quest_shares_left", "quest_kill_how_many",
                "quest_kill_original", "quest_map_room", "quest_map_bought")


def identity(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{32}", value) or int(value, 16) == 0:
        raise ValueError("actual nonzero 16-byte native identity required")
    return bytes.fromhex(value)


def selected(cursor, query, params=()):
    cursor.execute(query + f" LIMIT {MAX_ROWS + 1}", params)
    result = cursor.fetchall()
    if len(result) > MAX_ROWS:
        raise ValueError("quest cut row limit exceeded; narrow the actual scenario")
    encoded = [{key: value.hex() if isinstance(value, bytes) else value
                for key, value in entry.items()} for entry in result]
    if len(json.dumps(encoded).encode()) > MAX_BYTES:
        raise ValueError("quest cut byte limit exceeded")
    return encoded


def placeholders(values):
    return ",".join(["%s"] * len(values))


def capture(connection, case_id, pid, mobile_ids, operations, meta, *, legacy_no_epoch=False):
    if pid <= 0 or any(value <= 0 for value in mobile_ids):
        raise ValueError("actual positive player/mobile identity required")
    terms = blocks(case_id)
    vnums = sorted({number for term in terms for group in ("give", "receive")
                    for kind, number in term[group] if kind == "I"})
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        tables = ("economic_epoch", "mud_schema_migrations", "mud_schema_history", "mud_schema_migration_state",
                  "mud_schema_baselines", "player_data", "player_affects", "world_quest_accomplished",
                  "item_current_owner", "item_ownership_ledger", "currency_ledger", "quest_reward_obligation",
                  "quest_reward_xp_entitlement", "quest_mobile_native", "quest_mobile_native_birth_origin",
                  "economic_accounting_item_reference",
                  "economic_accounting_operation", "critical_operation_inbox", "economic_accounting_coin_posting",
                  "economic_accounting_source_claim", "economic_accounting_account_effect")
        engines = selected(cursor, "SELECT TABLE_NAME AS table_name,ENGINE AS engine FROM information_schema.tables "
                           "WHERE table_schema=DATABASE() AND table_name IN (" + placeholders(tables) + ")",
                           tables)
        if len(engines) != len(tables) or any(entry["engine"] != "InnoDB" for entry in engines):
            raise ValueError("quest cut source is missing or not transactional")
        if legacy_no_epoch:
            if meta.get("lineage") is not None or meta.get("epoch") is not None or mobile_ids:
                raise ValueError("legacy capture cannot assert active identities or native births")
            epochs = selected(cursor, "SELECT lineage,epoch FROM economic_epoch")
            if epochs:
                raise ValueError("legacy no-epoch capture refuses any installed epoch")
        else:
            epochs = selected(cursor, "SELECT lineage,epoch FROM economic_epoch WHERE lineage=%s AND epoch=%s",
                              (identity(meta["lineage"]), identity(meta["epoch"])))
            if len(epochs) != 1:
                raise ValueError("actual isolated accounting epoch missing; do not seed substitute authority")
        result = {"meta": dict(meta, case=case_id, pid=pid, mobile_instance_ids=sorted(mobile_ids),
                               watched_vnums=vnums,
                               authority="legacy-no-epoch" if legacy_no_epoch else "native"),
                  "epochs": epochs}
        result["snapshot"] = selected(cursor,
            "SELECT @@in_transaction AS in_transaction,@@tx_isolation AS isolation_level")
        result["legacy_migration_markers"] = selected(cursor, "SELECT migration_name FROM mud_schema_migrations ORDER BY migration_name")
        result["migrations"] = selected(cursor, "SELECT migration_id,sequence_number,apply_checksum,verify_checksum,"
            "compatibility,runner_version FROM mud_schema_history ORDER BY sequence_number")
        result["migration_state"] = selected(cursor,
            "SELECT state_id,applied_count,history_checksum FROM mud_schema_migration_state ORDER BY state_id")
        result["baselines"] = selected(cursor, "SELECT baseline_id,baseline_kind,schema_fingerprint,"
            "manifest_version,runner_version FROM mud_schema_baselines ORDER BY baseline_id")
        result["player"] = selected(cursor, "SELECT pid,race,level,copper,silver,gold,platinum,exp," +
                                    ",".join(TASK_COLUMNS) + " FROM player_data WHERE pid=%s", (pid,))
        if len(result["player"]) != 1:
            raise ValueError("actual isolated player row missing")
        result["player_affects"] = selected(cursor, "SELECT type,duration,flags FROM player_affects WHERE pid=%s "
                                           "ORDER BY type,duration,flags", (pid,))
        result["history"] = selected(cursor, "SELECT id,quest_giver,quest_target,reward_vnum "
                                     "FROM world_quest_accomplished WHERE pid=%s ORDER BY id", (str(pid),))
        item_where, item_params = ["owner_type=1 AND owner_id=%s"], [pid]
        if vnums:
            item_where += ["vnum IN (" + placeholders(vnums) + ")"]
            item_params += vnums
        if mobile_ids:
            item_where += ["owner_type=12 AND owner_id IN (" + placeholders(mobile_ids) + ")"]
            item_params += mobile_ids
        result["items"] = selected(cursor,
            "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
            "item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE " +
            " OR ".join("(" + term + ")" for term in item_where) + " ORDER BY item_uid", tuple(item_params))
        uids = [entry["item_uid"] for entry in result["items"]]
        if uids:
            result["ownership_events"] = selected(cursor,
                "SELECT operation_id,event_index,item_uid,root_item_uid,parent_item_uid,"
                "from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,"
                "to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,"
                "reason_id,source_site FROM item_ownership_ledger WHERE item_uid IN (" +
                placeholders(uids) + ") ORDER BY operation_id,event_index", tuple(uids))
        else:
            result["ownership_events"] = []
        result["currency"] = selected(cursor,
            "SELECT operation_id,pid,bank_id,wallet_delta_copper,wallet_delta_silver,wallet_delta_gold,"
            "wallet_delta_platinum,wallet_after_copper,wallet_after_silver,wallet_after_gold,"
            "wallet_after_platinum,wallet_revision,bank_revision,reason_type,reason_id,source_site "
            "FROM currency_ledger WHERE pid=%s ORDER BY operation_id", (pid,))
        # Bound aggregate BLOB material before buffered fetch/hex expansion.
        size = selected(cursor, "SELECT CAST(COALESCE(SUM(OCTET_LENGTH(continuation)),0) AS UNSIGNED) AS size,COUNT(*) AS row_count "
                        "FROM quest_reward_obligation WHERE player_pid=%s", (pid,))[0]
        blob_bytes = int(size["size"])
        if int(size["row_count"]) > MAX_ROWS or blob_bytes * 2 > MAX_BYTES:
            raise ValueError("quest obligation BLOB budget exceeded")
        if mobile_ids:
            size = selected(cursor, "SELECT CAST(COALESCE(SUM(OCTET_LENGTH(canonical_image)),0) AS UNSIGNED) AS size,COUNT(*) AS row_count "
                            "FROM quest_mobile_native WHERE mobile_instance_id IN (" + placeholders(mobile_ids) + ")",
                            tuple(mobile_ids))[0]
            blob_bytes += int(size["size"])
            if int(size["row_count"]) > MAX_ROWS or blob_bytes * 2 > MAX_BYTES:
                raise ValueError("combined native BLOB budget exceeded")
            size = selected(cursor, "SELECT CAST(COALESCE(SUM(OCTET_LENGTH(canonical_origin)),0) AS UNSIGNED) AS size,COUNT(*) AS row_count "
                            "FROM quest_mobile_native_birth_origin WHERE mobile_instance_id IN (" +
                            placeholders(mobile_ids) + ")", tuple(mobile_ids))[0]
            blob_bytes += int(size["size"])
            if int(size["row_count"]) > MAX_ROWS or blob_bytes * 2 > MAX_BYTES:
                raise ValueError("combined native birth-origin BLOB budget exceeded")
        result["obligations"] = selected(cursor,
            "SELECT offering_operation_id,player_pid,continuation,xp_applied_mask,"
            "(acknowledged_at IS NOT NULL) AS acknowledged FROM quest_reward_obligation "
            "WHERE player_pid=%s ORDER BY offering_operation_id", (pid,))
        result["xp_entitlements"] = selected(cursor,
            "SELECT offering_operation_id,recipient_pid,reward_index,amount,(applied_at IS NOT NULL) AS applied "
            "FROM quest_reward_xp_entitlement WHERE recipient_pid=%s ORDER BY offering_operation_id,reward_index", (pid,))
        result["mobiles"] = selected(cursor,
            "SELECT mobile_instance_id,mobile_revision,stock_revision,lifetime_state,canonical_image "
            "FROM quest_mobile_native WHERE " +
            ("mobile_instance_id IN (" + placeholders(mobile_ids) + ")" if mobile_ids else "1=0") +
            " ORDER BY mobile_instance_id", tuple(mobile_ids))
        # Retained schema63 origin is distinct from the progressed current image.
        # Capture exact bytes for the maintained owner validator; this reader
        # neither interprets its envelope nor grants birth/publication authority.
        result["birth_origins"] = selected(cursor,
            "SELECT mobile_instance_id,birth_operation,publication_revision,canonical_origin "
            "FROM quest_mobile_native_birth_origin WHERE " +
            ("mobile_instance_id IN (" + placeholders(mobile_ids) + ")" if mobile_ids else "1=0") +
            " ORDER BY mobile_instance_id", tuple(mobile_ids))
        native_ids = set(operations)
        native_ids.update(entry["birth_operation"] for entry in result["birth_origins"])
        for name in ("ownership_events", "currency"):
            native_ids.update(entry["operation_id"] for entry in result[name])
        native_ids.update(entry["offering_operation_id"] for entry in result["obligations"])
        for entry in result["mobiles"]:
            image = bytes.fromhex(entry["canonical_image"])
            decoded = decode_native_mobile(image)
            expected = tuple(entry[name] for name in ("mobile_instance_id", "mobile_revision", "stock_revision", "lifetime_state"))
            if tuple(decoded) != expected:
                raise ValueError("native mobile image/header disagreement")
            # Layout already validated by the maintained complete value grammar.
            native_ids.add(image[40:56].hex())  # original birth operation
            native_ids.add(image[164:180].hex())  # original last transition
        native_ids = sorted(native_ids)
        # Source rows remain empty when genuine effects/authority have not appeared.
        # Checks reject their absence; this reader does not manufacture evidence.
        for name in ("item_references", "operations", "inbox", "postings", "source_claims", "effects"):
            result[name] = []
        if native_ids:
            binary_ids = tuple(identity(value) for value in native_ids)
            result["item_references"] = selected(cursor,
                "SELECT operation_id,line_index,event_index,child_index,item_uid,before_revision,after_revision,"
                "legacy_operation_id,legacy_event_index FROM economic_accounting_item_reference "
                "WHERE legacy_operation_id IN (" + placeholders(binary_ids) +
                ") ORDER BY operation_id,line_index", binary_ids)
            economic_ids = set(native_ids) | {entry["operation_id"] for entry in result["item_references"]}
            binary_ids = tuple(identity(value) for value in sorted(economic_ids))
            clause = "operation_id IN (" + placeholders(binary_ids) + ")"
            result["operations"] = selected(cursor,
                "SELECT operation_id,lineage,epoch,original_operation_id,reason,source_event,outcome,result_code,"
                "writer_id,policy_version,compiler_version,intent_digest,domain_digest,plan_digest,posting_count,"
                "item_event_count,child_count FROM economic_accounting_operation WHERE " + clause +
                " ORDER BY operation_id", binary_ids)
            result["inbox"] = selected(cursor,
                "SELECT operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,"
                "result_code,failure_stage,durable_revision,result_payload,(committed_at IS NOT NULL) "
                "AS committed_at_present FROM critical_operation_inbox WHERE " + clause +
                " ORDER BY operation_id", binary_ids)
            result["effects"] = selected(cursor,
                "SELECT operation_id,account_index,account_key,before_copper,before_silver,before_gold,"
                "before_platinum,after_copper,after_silver,after_gold,after_platinum,"
                "before_revision,after_revision FROM economic_accounting_account_effect WHERE " +
                clause + " ORDER BY operation_id,account_index", binary_ids)
            result["postings"] = selected(cursor,
                "SELECT operation_id,line_index,event_index,account_index,child_index,delta_copper,delta_silver,"
                "delta_gold,delta_platinum,copper_value FROM economic_accounting_coin_posting WHERE " +
                clause + " ORDER BY operation_id,line_index", binary_ids)
            result["source_claims"] = selected(cursor,
                "SELECT lineage,source_event,operation_id,outcome FROM economic_accounting_source_claim WHERE " +
                clause + " ORDER BY operation_id", binary_ids)
        if len(json.dumps(result).encode()) > MAX_BYTES:
            raise ValueError("combined quest cut byte limit exceeded")
        return result
    finally:
        try:
            connection.rollback()
            cursor.execute("SELECT @@in_transaction AS in_transaction")
            if cursor.fetchone()["in_transaction"] != 0:
                raise ValueError("quest capture read-only transaction did not close")
        finally:
            cursor.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=CASES, required=True)
    parser.add_argument("--database", required=True)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--mobile-instance", type=int, nargs="*", default=[])
    parser.add_argument("--operation", nargs="*", default=[])
    parser.add_argument("--lineage")
    parser.add_argument("--epoch")
    parser.add_argument("--legacy-no-epoch", action="store_true",
                        help="capture inactive legacy gameplay only; refuses any epoch or native birth identity")
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True, help="owner-recorded actual integrated binary source commit")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        if os.environ.get("TEST_DB_DISPOSABLE") != "1" or os.environ.get("TEST_DB_HOST") != "127.0.0.1":
            raise ValueError("explicit loopback disposable authority required")
        if not re.fullmatch(r"(?:quest_journey_test_[0-9a-f]{12}|(?:quest_accounting|native_quest_publication)_test_[0-9a-f]{16})", args.database):
            raise ValueError("fresh generated quest-only schema name required")
        if args.pid <= 0 or any(value <= 0 for value in args.mobile_instance):
            raise ValueError("actual positive player/mobile identity required")
        if digest(args.server) != args.server_sha256:
            raise ValueError("integrated binary pin differs")
        if args.legacy_no_epoch:
            if args.lineage is not None or args.epoch is not None or args.mobile_instance:
                raise ValueError("legacy capture requires absent active identities and native births")
        else:
            if args.lineage is None or args.epoch is None:
                raise ValueError("actual lineage and epoch required for native capture")
            identity(args.lineage), identity(args.epoch)
        for operation in args.operation:
            identity(operation)
        if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit) or int(args.source_commit, 16) == 0:
            raise ValueError("actual integrated source revision required")
        meta = dict(source_commit=args.source_commit, prep_accounting_pin=ACCOUNTING_PIN, binary_sha256=args.server_sha256,
                    schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
                    lineage=args.lineage, epoch=args.epoch)
        import pymysql
        try:
            connection = pymysql.connect(host="127.0.0.1", port=int(os.environ.get("TEST_DB_PORT", "3306")),
                user=os.environ["TEST_DB_USER"], password=os.environ["TEST_DB_PASSWORD"], database=args.database,
                charset="utf8mb4", autocommit=True, cursorclass=pymysql.cursors.DictCursor,
                connect_timeout=5, read_timeout=10, write_timeout=5)
            try:
                cut = capture(connection, args.case, args.pid, args.mobile_instance, args.operation, meta,
                              legacy_no_epoch=args.legacy_no_epoch)
            finally:
                connection.close()
        except pymysql.MySQLError as error:
            raise ValueError(f"SQL read failed with code {error.args[0]}") from error
        descriptor = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as output:
            json.dump(cut, output, indent=2)
            output.write("\n")
        print("Captured SQL state only; native world/context/ACK and integrated qualification remain separate.")
    except (ValueError, KeyError, OSError, ImportError) as error:
        parser.exit(1, f"quest cut capture refused: {error}\n")


if __name__ == "__main__":
    main()
