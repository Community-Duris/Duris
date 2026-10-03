"""Exact-UID SQL payload repair, deliberately separate from restitution delivery.

The public entry points are the repair-* commands in player_death_restitution.py.
No transfer, UID allocator, currency writer, or artifact writer is used here.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import stat
import subprocess
from typing import Any

EVIDENCE_FORMAT = "duris-item-payload-evidence-v1"
PLAN_FORMAT = "duris-item-payload-repair-v1"
CLASSIFICATION = "payload_repair"
# A distinct offline receipt disposition; native delivery decoders never admit it.
DISPOSITION = 30
MAX_EVIDENCE_BYTES = 32 * 1024 * 1024


def codec(api: Any, mode: str, payload: bytes) -> Any:
    result = subprocess.run([str(api.ensure_codec()), mode], input=payload,
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=30)
    if result.returncode:
        raise api.ToolError("corrupt_or_unsupported_encoding: production codec rejected evidence")
    return json.loads(result.stdout)


def read_evidence(api: Any, path: Path | None) -> dict[str, Any] | None:
    if path is None or not path.exists():
        return None
    if path.lstat().st_size > MAX_EVIDENCE_BYTES:
        raise api.ToolError("evidence_limit: evidence envelope is too large")
    return api.read_protected_json(path)


def select_evidence(api: Any, evidence: dict[str, Any] | None, uid: int) -> dict[str, Any]:
    if evidence is None:
        raise api.ToolError("missing_evidence: an identity-bound native snapshot is required")
    if not isinstance(evidence, dict):
        raise api.ToolError("insufficient_evidence: expected a native snapshot envelope")
    frames = evidence.get("snapshots_hex")
    if evidence.get("format") != EVIDENCE_FORMAT or not isinstance(frames, list) or not 1 <= len(frames) <= 16:
        raise api.ToolError("insufficient_evidence: expected a bounded native snapshot envelope")
    matches = []
    identities = []
    for frame in frames:
        if not isinstance(frame, str):
            raise api.ToolError("corrupt_or_unsupported_encoding: native frame must be hex bytes")
        try:
            payload = api.hex_bytes(frame, "snapshot evidence")
        except api.ToolError as exc:
            raise api.ToolError("corrupt_or_unsupported_encoding: native frame must be hex bytes") from exc
        if not payload or len(payload) > 4 * 1024 * 1024:
            raise api.ToolError("corrupt_or_unsupported_encoding: snapshot exceeds codec bounds")
        decoded = codec(api, "decode-evidence", payload)
        seen = set()
        for item in decoded["items"]:
            item_uid = item["object_uid"]
            if not item_uid or item_uid in seen:
                raise api.ToolError("ambiguous_evidence: duplicate or unbound item identity")
            seen.add(item_uid)
            if item_uid == uid:
                # Custody owns placement. Evidence of an equipped root cannot
                # prove its now-missing equipment projection; refuse that case.
                if item["equipment_slot"] > 0:
                    raise api.ToolError("unsupported_placement: equipped payload evidence requires separate review")
                detached = bytearray(api.hex_bytes(item["item_payload_hex"]))
                detached[8:10] = b"\0\0"  # native inventory slot; parent already detached by bridge
                canonical = codec(api, "decode-items", bytes(detached))[0]
                matches.append(canonical)
                identities.append({"pid": decoded["pid"], "revision": decoded["revision"],
                                   "operation_id_hex": decoded["operation_id_hex"],
                                   "snapshot_digest": api.digest_bytes(payload)})
    if not matches:
        raise api.ToolError("missing_uid_evidence: native evidence does not contain this exact UID")
    if len({item["item_payload_hex"] for item in matches}) != 1:
        raise api.ToolError("conflicting_evidence: different serialized versions of this UID")
    return {"item": matches[0], "identities": identities,
            "evidence_digest": api.digest_json(evidence)}


def journals(api: Any) -> dict[str, str]:
    """Fail closed on queued/replayable work, including a crash-left temp file.

    The operator must use the runtime's explicit configuration. Missing roots
    are not proof that the deployed runtime has no pending operations.
    """
    result = {}
    for variable, filename in (("PLAYER_SAVE_JOURNAL_DIR", "player-save.journal"),
                               ("CRITICAL_COMMAND_JOURNAL_DIR", "critical-command.journal")):
        configured = os.environ.get(variable)
        if not configured or not Path(configured).is_absolute():
            raise api.ToolError("pending_authority_unproven: configure both runtime journal directories")
        root = Path(configured)
        info = root.lstat()
        if root.is_symlink() or not stat.S_ISDIR(info.st_mode) or info.st_uid != os.getuid() or info.st_mode & 0o077:
            raise api.ToolError("pending_authority_unproven: journal root must be an owner-only directory")
        for entry in root.iterdir():
            info = entry.lstat()
            if entry.name != filename or not stat.S_ISREG(info.st_mode) or entry.is_symlink() or info.st_size:
                raise api.ToolError("pending_authority: drain/replay runtime journals before repair")
            if info.st_uid != os.getuid() or info.st_mode & 0o077:
                raise api.ToolError("pending_authority_unproven: journal must be owner-only")
        result[variable] = str(root.resolve())
    return result


def fingerprint_sql(table: str, fields: list[str], where: str) -> str:
    # Every field is fixed by repository code. HEX distinguishes NULL, empty,
    # binary bytes and delimiters; row hashes retain duplicate cardinality.
    parts = ",".join("COALESCE(HEX(" + field + "),'NULL')" for field in fields)
    return ("SELECT COUNT(*) n,COALESCE(SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ''),256),SHA2('',256)) d "
            "FROM (SELECT SHA2(CONCAT_WS('|'," + parts + "),256) h FROM " + table + " WHERE " + where + ") repair_rows")


def observation_specs(api: Any, uid: int, pid: int, vnum: int) -> list[tuple[str, list[str], str]]:
    own_fields = ["item_uid", "root_item_uid", "parent_item_uid", "owner_type", "owner_id",
                  "owner_context_id", "vnum", "item_revision", "state", "coin_payload"]
    specs = [
        ("item_current_owner", own_fields, f"item_uid={uid} OR (owner_type=1 AND owner_id={pid} AND owner_context_id=0 AND state=1)"),
        ("item_owner_revision", ["owner_type", "owner_id", "owner_context_id", "revision"],
         f"owner_type=1 AND owner_id={pid} AND owner_context_id=0"),
        ("player_data", ["pid", "HEX(name)", "HEX(account_name)", "active", "save_revision", "wallet_revision"], f"pid={pid}"),
        ("critical_operation_inbox", ["operation_id", "status"], "status<>1"),
        ("critical_outbox", ["operation_id", "event_index", "status"], "status<>1"),
        ("player_death_restitution_delivery", ["item_uid", "metadata_digest"], f"item_uid={uid}"),
        ("player_death_restitution_runtime", ["item_uid", "state_digest"], f"item_uid={uid}"),
    ]
    # Bind all active topology rows and every domain's occurrence of the UID.
    domain = f"SELECT item_uid FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND owner_context_id=0 AND state=1"
    for table in api.ITEM_PROJECTION_TABLES:
        fields = ["id", "obj_uid", "vnum"]
        where = f"obj_uid={uid} OR obj_uid IN ({domain})"
        if table == "player_items":
            fields += ["pid", "equip_slot", "container_id", "item_type"]
            where += f" OR pid={pid} OR obj_uid IN ({domain})"
        specs.append((table, fields, where))
    # No artifact writes. Existing identity rules and every authority/competitor
    # input used by them are fenced even for an ordinary or name-marked item.
    for table, fields in (
        ("artifact_domain_state", ["vnum", "owned", "loc_type", "location", "timer_epoch", "artifact_type", "bind_owner_pid", "bind_timer_epoch", "item_uid", "item_revision", "revision"]),
        ("artifact_domain_baseline", ["vnum", "opening_timer_epoch", "opening_bind_owner_pid", "opening_bind_timer_epoch", "opening_revision"]),
        ("artifact_bind", ["vnum", "owner_pid", "timer"]),
        ("artifacts", ["vnum", "owned", "locType", "location", "timer", "type"]),
        ("artifacts_mortal", ["vnum", "owned", "locType", "location", "timer", "type"]),
        ("item_current_owner", own_fields),
        ("player_death_custody", ["pid", "save_revision", "item_uid", "state", "item_revision"]),
    ):
        specs.append((table, fields, f"vnum={vnum}"))
    return specs


def observations(api: Any, db: Any, uid: int, pid: int, vnum: int) -> list[dict[str, Any]]:
    specs = observation_specs(api, uid, pid, vnum)
    result = []
    for table, fields, where in specs:
        query = fingerprint_sql(table, fields, where)
        rows = db.run("SET SESSION group_concat_max_len=1048576; " + query)
        if len(rows) != 1 or len(rows[0]) != 2 or int(rows[0][0]) > 8192:
            raise api.ToolError("state_limit: repair observation exceeds bounded codec cardinality")
        result.append({"table": table, "fields": fields, "where": where,
                       "count": int(rows[0][0]), "digest": rows[0][1].lower()})
    return result


def existing_repair(api: Any, db: Any, uid: int) -> list[list[str]]:
    return db.run("SELECT HEX(r.restitution_id),HEX(r.evidence_digest),HEX(r.plan_digest),r.status,"
                  "HEX(i.metadata_digest),HEX(i.metadata_payload) FROM player_death_restitution_item i "
                  "JOIN player_death_restitution_receipt r ON r.restitution_id=i.restitution_id "
                  f"WHERE i.item_uid={uid} AND i.classification='payload_repair' ORDER BY r.restitution_id")


def inspect_state(api: Any, db: Any, uid: int, selected: dict[str, Any]) -> dict[str, Any]:
    api.require_schema(db)
    if not api.table_exists(db, "player_item_runtime_state"):
        raise api.ToolError("unsupported_backend: complete SQL runtime companion is required")
    required = set(api.ITEM_PROJECTION_TABLES) | {
        "player_data", "item_current_owner", "item_owner_revision", "player_item_runtime_state",
        "player_item_affects", "player_item_extra_descr", "critical_operation_inbox", "critical_outbox",
        "player_death_restitution_receipt", "player_death_restitution_item", "player_death_restitution_delivery",
        "player_death_restitution_runtime", "artifact_domain_state", "artifact_domain_baseline",
        "artifact_bind", "artifacts", "artifacts_mortal", "player_death_custody",
    }
    transactional = db.run("SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "
                           "AND engine='InnoDB' AND table_name IN (" +
                           ",".join("'" + name + "'" for name in sorted(required)) + ")")
    if {row[0] for row in transactional} != required:
        raise api.ToolError("unsupported_backend: every authority, projection and receipt table must be InnoDB")
    owner = api.fetch_current_owners(db, [uid]).get(str(uid))
    if owner is None:
        raise api.ToolError("custody_absent: no current authority for this UID")
    if owner["state"] != api.STATE_ACTIVE:
        raise api.ToolError("retired_or_inactive: repair requires active custody")
    if owner["owner_type"] != api.OWNER_PLAYER or owner["owner_context_id"]:
        raise api.ToolError("unsupported_owner: only SQL active player inventory custody is supported")
    pid = owner["owner_id"]
    if owner["vnum"] != selected["item"]["vnum"]:
        raise api.ToolError("identity_conflict: custody and evidence vnums differ")
    data = db.run(f"SELECT active,save_revision FROM player_data WHERE pid={pid}")
    if len(data) != 1 or data[0][0] != "1" or not owner["owner_revision"]:
        raise api.ToolError("owner_missing: active player and owner revision are required")
    if db.scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE status<>1") != "0" or \
            db.scalar("SELECT COUNT(*) FROM critical_outbox WHERE status<>1") != "0":
        raise api.ToolError("pending_authority: pending inbox/outbox work must finish first")
    if db.scalar(f"SELECT COUNT(*) FROM item_current_owner WHERE item_uid={uid} AND coin_payload IS NOT NULL") != "0" or \
            selected["item"]["type"] == api.ITEM_MONEY or owner["vnum"] == api.VOBJ_COINS:
        raise api.ToolError("currency_refused: coin authority is outside payload repair")
    if api.fetch_deliveries(db, [uid]) or api.fetch_runtime_state(db, [uid]):
        raise api.ToolError("existing_delivery: a restitution runtime requires its separate reconciliation workflow")
    count = int(db.scalar("SELECT " + api.projection_count_sql([uid])))
    if count:
        own_projections = api.fetch_player_projections(db, [uid])
        label = "competing_instance" if count > 1 or not own_projections or own_projections[0]["pid"] != pid else "existing_payload_conflict"
        raise api.ToolError(label + ": an existing physical payload must not be overwritten")
    parent_id = validate_topology(api, db, uid, owner, missing=True)
    validate_artifact(api, db, selected["item"], owner)
    return {"owner": owner, "save_revision": int(data[0][1]), "parent_id": parent_id,
            "observations": observations(api, db, uid, pid, owner["vnum"])}


def validate_topology(api: Any, db: Any, uid: int, owner: dict[str, Any], *, missing: bool) -> int:
    pid = owner["owner_id"]
    domain_uids = [int(row[0]) for row in db.run(
        f"SELECT item_uid FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND owner_context_id=0 AND state=1 ORDER BY item_uid")]
    if len(domain_uids) > 4096:
        raise api.ToolError("state_limit: player topology exceeds codec bounds")
    if int(db.scalar("SELECT " + api.projection_count_sql(domain_uids))) != len(domain_uids) - int(missing):
        raise api.ToolError("competing_instance: another custody UID has ambiguous or absent physical instances")
    owners = api.fetch_current_owners(db, domain_uids)
    projections = api.fetch_player_projections(db, domain_uids)
    by_uid = {}
    for projection in projections:
        key = str(projection["item_uid"])
        if key in by_uid or projection["pid"] != pid:
            raise api.ToolError("competing_instance: ambiguous player topology projection")
        by_uid[key] = projection
    for key, row in owners.items():
        visited = set()
        current = row
        while current["parent_item_uid"]:
            if current["item_uid"] in visited or len(visited) >= 31:  # production maximum 32 counts the root
                raise api.ToolError("invalid_topology: cycle or excessive depth")
            visited.add(current["item_uid"])
            parent = owners.get(str(current["parent_item_uid"]))
            if parent is None or parent["root_item_uid"] != row["root_item_uid"]:
                raise api.ToolError("invalid_topology: parent is absent or belongs to another root")
            current = parent
        if current["item_uid"] != row["root_item_uid"]:
            raise api.ToolError("invalid_topology: root relationship is inconsistent")
        if missing and int(key) == uid:
            continue
        projection = by_uid.get(key)
        parent_projection = by_uid.get(str(row["parent_item_uid"]))
        if projection is None or projection["vnum"] != row["vnum"] or \
                (row["parent_item_uid"] and (parent_projection is None or projection["container_id"] != parent_projection["id"])) or \
                (not row["parent_item_uid"] and projection["container_id"]):
            raise api.ToolError("invalid_topology: another custody payload is missing or its projection disagrees")
        if row["parent_item_uid"] and projection["equip_slot"] != 0:
            raise api.ToolError("invalid_topology: nested projection claims equipment custody")
    parent_id = by_uid[str(owner["parent_item_uid"])]["id"] if owner["parent_item_uid"] else 0
    if parent_id and db.scalar(f"SELECT item_type FROM player_items WHERE id={parent_id}") != "15":
        raise api.ToolError("invalid_topology: authoritative parent is not a container")
    return parent_id


def validate_artifact(api: Any, db: Any, item: dict[str, Any], owner: dict[str, Any]) -> None:
    pid = owner["owner_id"]
    artifact_inputs = api.fetch_artifacts(db, [owner["vnum"]])
    if item["native_artifact_flag"]:
        decision = api.artifact_reconciliation(item, pid, artifact_inputs, owner)
        if not decision["ok"] or decision.get("reconciliation_required"):
            raise api.ToolError("artifact_reconciliation_required: " + decision["classification"])
        # Repair must not resurrect/extend a timer or alter location/binding.
        domain = decision.get("domain_before")
        if not domain or domain["loc_type"] != 3 or domain["location"] != pid or \
                domain["timer_epoch"] != item["timers"][0]:
            raise api.ToolError("artifact_reconciliation_required: current artifact timing/placement differs")
    elif str(owner["vnum"]) in artifact_inputs["domain"]:
        raise api.ToolError("artifact_identity_conflict: ordinary evidence conflicts with canonical artifact authority")


def prepare(api: Any, db: Any, uid: int, evidence: dict[str, Any] | None,
            target: dict[str, Any], backup: dict[str, Any] | None = None,
            boundary: dict[str, Any] | None = None) -> dict[str, Any]:
    uid = api.int_value(uid, "item UID", 1, 2**64 - 1)
    plan = {"format": PLAN_FORMAT, "item_uid": uid, "target": target, "evidence": evidence,
            "backup_receipt": backup, "maintenance_boundary": boundary, "applyable": False}
    try:
        selected = select_evidence(api, evidence, uid)
        plan.update(selected)
        prior = existing_repair(api, db, uid)
        if prior:
            if len(prior) != 1 or prior[0][1].lower() != selected["evidence_digest"] or \
                    prior[0][4].lower() != api.digest_bytes(api.hex_bytes(selected["item"]["item_payload_hex"])):
                raise api.ToolError("conflicting_repair_receipt: this UID already has a different repair")
            raise api.ToolError("already_applied: use the original plan to verify the existing repair")
        plan["journal_roots"] = journals(api)
        plan["state"] = inspect_state(api, db, uid, selected)
        plan["classification"] = "repairable_exact"
        plan["note"] = "Complete native payload; current custody and placement remain authoritative"
        plan["applyable"] = True
    except api.ToolError as exc:
        plan["classification"], _, plan["note"] = str(exc).partition(": ")
    plan["plan_digest"] = api.digest_json(plan)
    return plan


def load_plan(api: Any, path: Path) -> dict[str, Any]:
    plan = api.read_protected_json(path)
    if plan.get("format") != PLAN_FORMAT:
        raise api.ToolError("unsupported_plan: expected an exact-UID payload repair plan")
    api.verify_artifact_digest(plan, "plan_digest")
    return plan


def projection_values(api: Any, item: dict[str, Any], pid: int, parent: str) -> tuple[str, list[str]]:
    columns = ("pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,extra_flags,wear_flags,item_type,"
               "value0,value1,value2,value3,value4,value5,value6,value7,name,short_descr,description,action_descr,"
               "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,obj_uid,item_condition")
    strings = [api.sql_blob(item[field]) if item["string_mask"] & mask else "NULL"
               for field, mask in (("name_hex", 1), ("short_description_hex", 4), ("description_hex", 2), ("action_description_hex", 8))]
    values = [str(pid), str(item["vnum"]), "0", parent, "1", str(item["weight"]), str(item["cost"]),
              str(item["timers"][0]), str(item["extra_flags"]), str(item["wear_flags"]), str(item["type"]),
              *map(str, item["values"]), *strings, *map(str, item["bitvectors"]), str(item["material"]),
              str(item["object_uid"]), str(item["condition"])]
    return columns, values


def descriptions(api: Any, item: dict[str, Any]) -> set[tuple[bytes, bytes]]:
    result = set()
    for row in item["extra_descriptions"]:
        keyword = api.hex_bytes(row["keyword_hex"])
        text = api.hex_bytes(row["description_hex"])
        if row["spellbook"]:
            keyword = b"SPELLBOOK"
            spells = json.loads(text) if text else row["spell_ids"]
            text = ("[" + ",".join(map(str, sorted(spells))) + "]").encode()
        if keyword:
            result.add((keyword, text))
    return result


def build_sql(api: Any, db: Any, plan: dict[str, Any], actor: str, reason: str,
              *, verified_observations: list[dict[str, Any]] | None = None) -> str:
    mark_verified = verified_observations is not None
    uid = api.int_value(plan["item_uid"], "repair UID", 1, 2**64 - 1)
    state = plan["state"]
    owner = state["owner"]
    pid = api.int_value(owner["owner_id"], "repair player", 1, 2**31 - 1)
    for field in ("root_item_uid", "parent_item_uid", "item_revision"):
        api.int_value(owner[field], field, 0, 2**64 - 1)
    api.int_value(state["parent_id"], "parent projection", 0, 2**32 - 1)
    api.int_value(owner["vnum"], "custody vnum", 1, 2**31 - 1)
    item = plan["item"]
    rid = api.digest_bytes(b"duris-payload-repair-v1" + bytes.fromhex(plan["plan_digest"]))[:32]
    payload = api.sql_blob(item["item_payload_hex"])
    digest = api.digest_bytes(api.hex_bytes(item["item_payload_hex"]))
    source = plan["identities"][0]
    api.int_value(source["pid"], "evidence player", 1, 2**31 - 1)
    api.int_value(source["revision"], "evidence revision", 1, 2**64 - 1)
    op = source["operation_id_hex"]
    if op == "0" * 32:
        op = source["snapshot_digest"][:32]
    actor, reason = api.validate_staff_approval(actor, reason)
    lock = api.RUNTIME_EXCLUSION_LOCK_EXPRESSION
    lines = ["SET SESSION group_concat_max_len=1048576;", "SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE;",
             f"SELECT GET_LOCK({lock},0) INTO @repair_lock;", "START TRANSACTION;",
             f"SET @repair_ok=(@repair_lock=1 AND IS_USED_LOCK({lock})=CONNECTION_ID());",
             "SET @repair_ok=@repair_ok AND (" + api.database_visibility_sql() + ")=1 "
             "AND (SELECT COUNT(*) FROM information_schema.processlist WHERE ID<>CONNECTION_ID())=0 "
             "AND (SELECT COUNT(*) FROM information_schema.innodb_trx WHERE trx_mysql_thread_id<>CONNECTION_ID())=0;",
             f"SELECT item_uid FROM item_current_owner WHERE item_uid={uid} FOR UPDATE;",
             f"SELECT HEX(r.restitution_id) FROM player_death_restitution_item i JOIN player_death_restitution_receipt r "
             f"ON r.restitution_id=i.restitution_id WHERE i.item_uid={uid} AND i.classification='payload_repair' FOR UPDATE;",
             f"SET @repair_existing=(SELECT COUNT(*) FROM player_death_restitution_item WHERE item_uid={uid} AND classification='payload_repair');",
             "SET @repair_completed=((SELECT COUNT(*) FROM player_death_restitution_receipt r JOIN "
             "player_death_restitution_item i ON i.restitution_id=r.restitution_id WHERE "
             f"i.item_uid={uid} AND i.classification='payload_repair' AND i.disposition={DISPOSITION} "
             f"AND r.restitution_id={api.sql_blob(rid)} AND r.plan_digest={api.sql_blob(plan['plan_digest'])} "
             f"AND r.evidence_digest={api.sql_blob(plan['evidence_digest'])} AND r.status IN (2,3) "
             f"AND i.metadata_digest={api.sql_blob(digest)} AND i.metadata_payload={payload})=1);",
             "SET @repair_ok=@repair_ok AND (@repair_existing=0 OR (@repair_existing=1 AND @repair_completed=1));",
             # The validated bytes, not a reopenable path, enter the transaction.
             f"SET @repair_ok=@repair_ok AND SHA2({api.sql_blob(api.canonical_json(plan['evidence']))},256)='{plan['evidence_digest']}' "
             f"AND SHA2({payload},256)='{digest}';"]
    if mark_verified:
        # Fresh verification observations include payload/metadata companions.
        checks = verified_observations
        lines.append("SET @repair_ok=@repair_ok AND @repair_completed=1;")
    else:
        checks = state["observations"]
    expected_specs = observation_specs(api, uid, pid, owner["vnum"])
    if mark_verified:
        expected_specs += verification_specs(api, plan)
    if len(checks) != len(expected_specs):
        raise api.ToolError("invalid_plan: observation cardinality differs")
    for check, (table, fields, where) in zip(checks, expected_specs):
        if (check["table"], check["fields"], check["where"]) != (table, fields, where):
            raise api.ToolError("invalid_plan: SQL observation is not a repository predicate")
        api.int_value(check["count"], "observation count", 0, 8192)
        api.hex_digest(check["digest"], "observation digest")
        # SELECT FOR UPDATE locks both rows and empty ranges. SERIALIZABLE plus
        # the runtime advisory exclusion protects the absence predicates.
        lines.append("SELECT HEX(" + check["fields"][0] + ") FROM " + check["table"] + " WHERE " + check["where"] + " FOR UPDATE;")
        query = fingerprint_sql(check["table"], check["fields"], check["where"])
        lines.append("SET @repair_ok=@repair_ok AND " + ("" if mark_verified else "(@repair_completed=1 OR ") +
                     "((SELECT CONCAT(n,':',d) FROM (" + query +
                     ") repair_check)='" + str(check["count"]) + ":" + check["digest"] + "')" +
                     ("" if mark_verified else ")") + ";")
    gate = "@repair_ok=1 AND @repair_completed=0"
    if mark_verified:
        lines.append(f"UPDATE player_death_restitution_receipt SET status=3,verified_at=CURRENT_TIMESTAMP(6) WHERE restitution_id={api.sql_blob(rid)} AND @repair_ok=1;")
    else:
        lines += ["INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,recipient_pid,death_operation_id,"
                  "evidence_digest,plan_digest,status,actor,reason,candidate_count,delivered_count,unresolved_count,applied_at) SELECT " +
                  ",".join([api.sql_blob(rid), str(source["pid"]), str(source["revision"]), str(pid), api.sql_blob(op),
                            api.sql_blob(plan["evidence_digest"]), api.sql_blob(plan["plan_digest"]), "2", api.sql_text(actor),
                            api.sql_text(reason), "1", "0", "0", "CURRENT_TIMESTAMP(6)"]) + " WHERE " + gate + ";",
                  "SET @repair_receipt=ROW_COUNT();"]
        columns, values = projection_values(api, item, pid, str(state["parent_id"]) if state["parent_id"] else "NULL")
        lines += ["INSERT INTO player_items(" + columns + ") SELECT " + ",".join(values) + " WHERE " + gate + ";",
                  "SET @repair_projection=ROW_COUNT(),@repair_item_id=LAST_INSERT_ID();",
                  "INSERT INTO player_item_runtime_state(item_id,payload) SELECT @repair_item_id," + payload + " WHERE " + gate + ";",
                  "SET @repair_runtime=ROW_COUNT(),@repair_affects=0,@repair_descriptions=0;"]
        affects = {tuple(pair) for pair in item["affects"] if pair != [0, 0]}
        for location, modifier in sorted(affects):
            lines += [f"INSERT INTO player_item_affects(item_id,location,modifier) SELECT @repair_item_id,{location},{modifier} WHERE {gate};",
                      "SET @repair_affects=@repair_affects+ROW_COUNT();"]
        extra = descriptions(api, item)
        for keyword, text in sorted(extra):
            lines += ["INSERT INTO player_item_extra_descr(item_id,keyword,description) SELECT @repair_item_id," +
                      api.sql_blob(keyword) + "," + api.sql_blob(text) + " WHERE " + gate + ";",
                      "SET @repair_descriptions=@repair_descriptions+ROW_COUNT();"]
        lines += ["INSERT INTO player_death_restitution_item(restitution_id,item_uid,source_root_item_uid,source_parent_item_uid,"
                  "delivered_root_item_uid,delivered_parent_item_uid,source_item_revision,delivered_item_revision,vnum,"
                  "disposition,classification,metadata_digest,metadata_payload,note) SELECT " +
                  ",".join([api.sql_blob(rid), str(uid), str(owner["root_item_uid"]), str(owner["parent_item_uid"]),
                            str(owner["root_item_uid"]), str(owner["parent_item_uid"]), str(owner["item_revision"]),
                            str(owner["item_revision"]), str(owner["vnum"]), str(DISPOSITION), "'payload_repair'",
                            api.sql_blob(digest), payload, "'Existing custody retained; payload repair only'"]) + " WHERE " + gate + ";",
                  "SET @repair_item_receipt=ROW_COUNT();",
                  "SET @repair_ok=@repair_ok AND (@repair_completed=1 OR (@repair_receipt=1 AND @repair_projection=1 "
                  f"AND @repair_runtime=1 AND @repair_item_receipt=1 AND @repair_affects={len(affects)} AND @repair_descriptions={len(extra)}));"]
    lines += [f"SET @repair_ok=@repair_ok AND IS_USED_LOCK({lock})=CONNECTION_ID();",
              "SET @repair_decision=IF(@repair_ok=1,'COMMIT','ROLLBACK');",
              "PREPARE repair_decision_stmt FROM @repair_decision;", "EXECUTE repair_decision_stmt;",
              "DEALLOCATE PREPARE repair_decision_stmt;",
              "SELECT CONCAT('DURIS_REPAIR|',IFNULL(@repair_ok,0),'|',IFNULL(@repair_completed,0));",
              f"DO RELEASE_LOCK({lock});"]
    return "\n".join(lines)


def verification_specs(api: Any, plan: dict[str, Any]) -> list[tuple[str, list[str], str]]:
    uid = plan["item_uid"]
    owner = plan["state"]["owner"]
    return [("player_items", projection_values(api, plan["item"], owner["owner_id"], "NULL")[0].split(","), f"obj_uid={uid}"),
             ("player_item_runtime_state", ["item_id", "payload"], f"item_id IN (SELECT id FROM player_items WHERE obj_uid={uid})"),
             ("player_item_affects", ["item_id", "location", "modifier"], f"item_id IN (SELECT id FROM player_items WHERE obj_uid={uid})"),
             ("player_item_extra_descr", ["item_id", "keyword", "description"], f"item_id IN (SELECT id FROM player_items WHERE obj_uid={uid})")]


def verification_observations(api: Any, db: Any, plan: dict[str, Any]) -> list[dict[str, Any]]:
    uid = plan["item_uid"]
    owner = plan["state"]["owner"]
    result = observations(api, db, uid, owner["owner_id"], owner["vnum"])
    for table, fields, where in verification_specs(api, plan):
        row = db.run("SET SESSION group_concat_max_len=1048576; " + fingerprint_sql(table, fields, where))[0]
        result.append({"table": table, "fields": fields, "where": where, "count": int(row[0]), "digest": row[1].lower()})
    return result


def verify(api: Any, db: Any, plan: dict[str, Any]) -> None:
    selected = select_evidence(api, plan["evidence"], plan["item_uid"])
    if selected["item"] != plan["item"] or selected["evidence_digest"] != plan["evidence_digest"]:
        raise api.ToolError("evidence_changed: plan metadata does not match the production codec")
    uid = plan["item_uid"]
    prior = existing_repair(api, db, uid)
    digest = api.digest_bytes(api.hex_bytes(plan["item"]["item_payload_hex"]))
    if len(prior) != 1 or prior[0][1].lower() != plan["evidence_digest"] or prior[0][2].lower() != plan["plan_digest"] or \
            prior[0][3] not in {"2", "3"} or prior[0][4].lower() != digest or prior[0][5].lower() != plan["item"]["item_payload_hex"]:
        raise api.ToolError("receipt_conflict: no matching applied repair receipt")
    expected = plan["state"]["owner"]
    actual = api.fetch_current_owners(db, [uid]).get(str(uid))
    fields = ("item_uid", "owner_type", "owner_id", "owner_context_id", "root_item_uid", "parent_item_uid", "vnum", "state")
    if actual is None or any(actual[field] != expected[field] for field in fields):
        raise api.ToolError("custody_changed: expected active custody/topology no longer matches")
    if int(db.scalar("SELECT " + api.projection_count_sql([uid]))) != 1:
        raise api.ToolError("competing_instance: repaired UID must have exactly one physical instance")
    projection = api.fetch_player_projections(db, [uid])
    if len(projection) != 1 or projection[0]["pid"] != expected["owner_id"] or projection[0]["equip_slot"] != 0:
        raise api.ToolError("projection_conflict: wrong player or equipment placement")
    parent_id = validate_topology(api, db, uid, actual, missing=False)
    validate_artifact(api, db, plan["item"], actual)
    original_artifacts = [row for row in plan["state"]["observations"] if row["table"].startswith("artifact")]
    current_artifacts = [row for row in observations(api, db, uid, expected["owner_id"], expected["vnum"])
                         if row["table"].startswith("artifact")]
    if original_artifacts != current_artifacts:
        raise api.ToolError("artifact_authority_changed: repair cannot certify changed artifact authority")
    if db.scalar(f"SELECT COUNT(*) FROM player_data WHERE pid={expected['owner_id']} AND active=1") != "1" or \
            db.scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE status<>1") != "0" or \
            db.scalar("SELECT COUNT(*) FROM critical_outbox WHERE status<>1") != "0":
        raise api.ToolError("pending_authority: verification requires active player and drained operations")
    if projection[0]["container_id"] != parent_id:
        raise api.ToolError("invalid_topology: repaired parent projection does not match custody")
    item_id = projection[0]["id"]
    columns, values = projection_values(api, plan["item"], expected["owner_id"], str(parent_id) if parent_id else "NULL")
    predicates = " AND ".join(column + " <=> " + value for column, value in zip(columns.split(","), values))
    if db.scalar(f"SELECT COUNT(*) FROM player_items WHERE id={item_id} AND " + predicates) != "1":
        raise api.ToolError("payload_fidelity: canonical serialized properties differ")
    runtime = db.run(f"SELECT HEX(payload) FROM player_item_runtime_state WHERE item_id={item_id}")
    if len(runtime) != 1:
        raise api.ToolError("payload_fidelity: complete runtime companion is missing")
    decoded = codec(api, "decode-items", api.hex_bytes(runtime[0][0]))
    if len(decoded) != 1 or decoded[0] != plan["item"]:
        raise api.ToolError("payload_fidelity: native runtime payload differs")
    affects, extra = api.fetch_item_metadata(db, [uid])
    if affects.get(uid, set()) != {tuple(pair) for pair in plan["item"]["affects"] if pair != [0, 0]} or \
            extra.get(uid, set()) != descriptions(api, plan["item"]):
        raise api.ToolError("payload_fidelity: affects or extra descriptions differ")


def run(api: Any, args: Any) -> int:
    if args.command == "repair-prepare":
        target_info = api.load_target_info(args.target_info) if args.target_info else None
        expected = target_info["target"] if target_info else None
        policy, _ = api.policy_for_command(args, expected_target=expected)
        db = api.Mysql(policy)
        target = api.identify_target(db, policy, expected)
        backup = api.load_backup_receipt(args.backup_receipt) if args.backup_receipt else None
        boundary = backup["maintenance_boundary"] if backup else None
        plan = prepare(api, db, args.item_uid, read_evidence(api, args.evidence), target, backup, boundary)
        if backup:
            api.validate_plan_backup(plan, target, boundary)
        api.atomic_write_json(args.artifact, plan, args.overwrite)
        print("payload repair preparation: classification=%s applyable=%s plan_digest=%s" %
              (plan["classification"], str(plan["applyable"]).lower(), plan["plan_digest"]))
        return 0
    plan = load_plan(api, args.plan)
    if not plan["applyable"]:
        raise api.ToolError("repair_refused: " + plan["classification"] + ": " + plan["note"])
    # Schema/identity checks are read-only. Mutation uses the existing exact
    # target, stopped boundary, backup and live quiescence checks on every run.
    policy, target_info = api.policy_for_command(args, expected_target=plan["target"], require_maintenance=True)
    db = api.Mysql(policy)
    target, boundary = api.validate_plan_target_controls(db, plan, policy, require_maintenance=True, target_info=target_info)
    api.validate_plan_backup(plan, target, boundary)
    if journals(api) != plan["journal_roots"]:
        raise api.ToolError("pending_authority_unproven: runtime journal configuration differs from plan")
    api.check_quiescence(db, args.offline_proof)
    selected = select_evidence(api, read_evidence(api, args.evidence), plan["item_uid"])
    if selected["evidence_digest"] != plan["evidence_digest"] or selected["item"] != plan["item"]:
        raise api.ToolError("evidence_changed: evidence differs from the prepared native payload")
    mark = args.command == "repair-verify"
    if mark:
        # Bind verified marking to a stable stopped snapshot, including metadata.
        before = verification_observations(api, db, plan)
        verify(api, db, plan)
        if verification_observations(api, db, plan) != before:
            raise api.ToolError("stale_verification: current state changed during verification")
    elif not args.approve:
        raise api.ToolError("authorization_required: payload repair apply requires explicit --approve")
    elif not existing_repair(api, db, plan["item_uid"]):
        fresh = inspect_state(api, db, plan["item_uid"], selected)
        if fresh != plan["state"]:
            raise api.ToolError("stale_plan: current custody, topology, or relevant revisions changed")
    rows = db.run(build_sql(api, db, plan, args.actor, args.reason,
                           verified_observations=before if mark else None))
    marker = next((row[0] for row in reversed(rows) if row[0].startswith("DURIS_REPAIR|")), "")
    if not marker.startswith("DURIS_REPAIR|1|"):
        raise api.ToolError("transaction_refused: exclusive writer, evidence, receipt, or state fence did not match")
    if mark:
        verify(api, db, plan)
        print("payload repair verified: exact instance, payload, custody and topology checked; historical recovery not certified")
    else:
        print("payload repair %s; verification remains separate" % ("already applied" if marker.endswith("|1") else "applied"))
    return 0
