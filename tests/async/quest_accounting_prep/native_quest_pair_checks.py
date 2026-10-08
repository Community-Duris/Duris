"""Original QP03 owner-observation joins only; reports cannot self-authenticate."""

import hashlib
import re

import quest_cut_checks as checks


EXTERNAL = [
    "authentic owner export, binary/candidate binding and actual guarded owner execution",
    "real quest-D authority, full physical retirement, literal custody/world census and chronology",
    "original save-hold/publication and SQL/ACK/session/journal observations; caller values are not capabilities",
]
PINS = ("source_commit", "binary_sha256", "schema_manifest_sha256", "lineage", "epoch", "pid")


def need(condition, message):
    checks.require(condition, "original QP03 pair agreement: " + message)


def number(value, *, zero=False):
    return type(value) is int and (0 if zero else 1) <= value < 2**64


def hex_value(value, size):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{" + str(size) + r"}", value) and int(value, 16)


def exact_bool(value, expected):
    return type(value) is bool and value is expected


def literal_same(left, right):
    if type(left) is not type(right):
        return False
    if isinstance(left, dict):
        return left.keys() == right.keys() and all(literal_same(left[key], right[key]) for key in left)
    if isinstance(left, list):
        return len(left) == len(right) and all(literal_same(a, b) for a, b in zip(left, right))
    return left == right


def safe(call, *args):
    try:
        return call(*args)
    except (KeyError, TypeError, ValueError, IndexError) as error:
        raise checks.CutError("original QP03 pair agreement refused: " + str(error)) from error


def receipt(entry, *, success=True):
    need(type(entry["present"]) is bool and all(type(entry[name]) is int for name in
         ("outcome", "durable_revision", "error_code", "failure_stage", "result_size")), "invalid receipt fields")
    need(0 <= entry["result_size"] <= 4096 and hex_value(entry["result_sha256"], 64) and
         hex_value(entry["result_array_sha256"], 64), "invalid receipt bounds/digests")
    if success:
        need(entry["present"] and entry["outcome"] in (0, 1) and number(entry["durable_revision"]) and
             entry["error_code"] == entry["failure_stage"] == 0 and entry["result_size"] == 48, "successful original receipt required")


def pair_value(cut, pair):
    need(pair["scope"] == "original-pair value correlation only" and pair["external_proof_required"] == EXTERNAL and
         exact_bool(pair["pair_checked"], pair["child"]["phase"] == 2) and
         exact_bool(pair["pair_valid"], pair["child"]["phase"] == 2), "reader stage/structural result disagrees")
    need(pair["required_xp_mask"] == 0 and pair["required_economic_mask"] == 3 and
         type(pair["required_xp_mask"]) is type(pair["required_economic_mask"]) is int and
         hex_value(pair["continuation_sha256"], 64), "non-fee QP03 frozen reward contract required")
    parent, child = pair["parent"], pair["child"]
    need(parent["operation"] != child["operation"] and parent["phase"] == 2 and child["phase"] in (1, 2), "wrong original pair/phase")
    for value in (parent, child):
        root = checks.index(checks.rows(cut, "operations"), ("operation_id",)).get((value["operation"],))
        need(root is not None and root["lineage"] == cut["meta"]["lineage"] and
             type(root["outcome"]) is type(root["result_code"]) is int and
             root["outcome"] == 1 and root["result_code"] == 0, "original successful accounting root missing")
        need(hex_value(value["operation"], 32) and hex_value(value["birth"], 32) and
             all(hex_value(value[key], 64) for key in ("command_sha256", "attachment_sha256", "source_sha256")) and
             all(number(value[key]) for key in ("revision", "pid", "instance", "save_revision")) and
             type(value["phase"]) is int and type(value["payload_version"]) is int and value["payload_version"] == 12 and
             number(value["command_bytes"]) and value["command_bytes"] <= 512 * 1024 and
             number(value["attachment_bytes"]) and value["attachment_bytes"] <= 32 * 1024 * 1024,
             "missing/bad original value identity or bounds")
        for key, length in (("publication_steps", 6), ("give_messages", 3), ("give_hooks", 9)):
            need(isinstance(value[key], list) and len(value[key]) == length and
                 all(type(part) is int and part in (0, 1, 2) for part in value[key]), "invalid publication step carrier")
        need(isinstance(value["consumed_steps"], list) and len(value["consumed_steps"]) <= 3000 and
             all(type(part) is int and part in (0, 1, 2) for part in value["consumed_steps"]) and
             type(value["publication_stage"]) is int and value["publication_stage"] in (0, 1, 2) and
             type(value["handoff"]) is int and value["handoff"] in (0, 1, 2) and number(value["branch"], zero=True),
             "invalid original progress carrier")
        receipt(value["receipt"], success=value["receipt"]["present"])
    need(all(parent[key] == child[key] for key in ("pid", "instance", "birth", "source_sha256")) and
         child["pid"] == cut["meta"]["pid"] and child["instance"] in cut["meta"]["mobile_instance_ids"] and
         cut["meta"].get("authority") == "native", "original PID/native birth binding differs")
    for value in (parent, child):
        entry = checks.index(checks.rows(cut, "inbox"), ("operation_id",)).get((value["operation"],))
        need(entry is not None and entry["status"] == 1 and entry["result_code"] == entry["failure_stage"] == 0 and
             all(type(entry[key]) is int for key in ("status", "result_code", "failure_stage", "committed_at_present", "payload_version")) and
             entry["committed_at_present"] == 1 and entry["command_hash"] == value["command_sha256"] and
             entry["payload_version"] == 12, "original successful SQL inbox/command link missing")
        if value["receipt"]["present"]:
            payload = bytes.fromhex(entry["result_payload"])
            expected = value["receipt"]
            need(len(payload) == expected["result_size"] and entry["durable_revision"] == expected["durable_revision"] and
                 hashlib.sha256(payload).hexdigest() == expected["result_sha256"] and
                 hashlib.sha256(payload + bytes(4096 - len(payload))).hexdigest() == expected["result_array_sha256"],
                 "retained context/SQL receipt disagreement")
    obligation = checks.index(checks.rows(cut, "obligations"), ("offering_operation_id",)).get((child["operation"],))
    need(obligation is not None and obligation["player_pid"] == child["pid"] and obligation["xp_applied_mask"] == 0 and
         type(obligation["player_pid"]) is type(obligation["xp_applied_mask"]) is int and
         type(obligation["acknowledged"]) is int and obligation["acknowledged"] in (0, 1) and
         hashlib.sha256(bytes.fromhex(obligation["continuation"])).hexdigest() == pair["continuation_sha256"],
         "original literal obligation missing/different")
    need(not any(entry["offering_operation_id"] == child["operation"] for entry in checks.rows(cut, "xp_entitlements")),
         "zero-XP original acquired an XP entitlement")


def owner_binding(cut, pair, observation):
    need(literal_same(observation["pair"], pair) and all(observation["meta"][key] == cut["meta"][key] for key in PINS) and
         number(observation["process_generation"]) and number(observation["coordinator_generation"]),
         "owner observation not bound to original pair/candidate")


def assert_qp03_held_pair(before, after, pair_before, pair_after, owner_before, owner_after):
    return safe(_held, before, after, pair_before, pair_after, owner_before, owner_after)


def _held(before, after, pair_before, pair_after, owner_before, owner_after):
    checks.bind(before, after, "QP03")
    for cut, pair, owner in ((before, pair_before, owner_before), (after, pair_after, owner_after)):
        pair_value(cut, pair)
        owner_binding(cut, pair, owner)
        child = pair["child"]
        need(child["phase"] == 1, "held assertion requires original execution frame, not phase2 release")
        hold = owner["hold"]
        need(exact_bool(hold["held"], True) and hold["operation"] == child["operation"] and
             hold["pid"] == child["pid"] and type(hold["pid"]) is int and number(hold["generation"]) and
             type(hold["save_revision"]) is int and hold["save_revision"] == child["save_revision"] and
             hold["command_sha256"] == child["command_sha256"] and
             exact_bool(owner["physical_released"], False) and exact_bool(owner["retired"], False), "original held owner identity missing/advanced")
        need(type(owner["uncertain"]) is bool and exact_bool(owner["poisoned"], False), "held uncertainty missing/poisoned")
    need(literal_same(pair_before, pair_after) and literal_same(owner_before, owner_after), "stable held interval changed original context/hold")
    checks.held(before, after, pair_before["child"]["operation"])
    need(checks.rows(before, "player_affects") == checks.rows(after, "player_affects"), "held interval changed affects")
    return dict(scope="captured QP03 held-pair agreement only", external_proof_required=list(EXTERNAL))


def publication(value, observed):
    receipt(value["receipt"])
    receipt(observed["receipt"])
    need(observed["operation"] == value["operation"] and observed["command_sha256"] == value["command_sha256"] and
         observed["receipt"] == value["receipt"] and observed["checkpoint_result"] == "ok" and
         number(observed["hold_generation"]) and all(exact_bool(observed[key], True) for key in
             ("hold_consumed", "physical_released")) and all(exact_bool(observed[key], False) for key in
             ("native_ack_uncertain", "publication_checkpointing")) and
         all(hex_value(observed[key], 64) for key in ("physical_proof_sha256", "census_sha256")) and
         value["phase"] == value["publication_stage"] == 2 and
         not any(1 in value[key] for key in ("publication_steps", "give_messages", "give_hooks", "consumed_steps")),
         "guarded publication/hold-consumption observation missing")


def verification(pair, observation):
    for role in ("parent", "child"):
        value = pair[role]
        publication(value, observation["publication"][role])
        actual = dict(value["receipt"], outcome=1)
        receipt(observation["verification"][role]["receipt"])
        need(observation["verification"][role] == dict(command_sha256=value["command_sha256"], receipt=actual),
             "original historical native receipt verification missing/different")
    obligation = observation["verification"]["obligation"]
    need(obligation == dict(operation=pair["child"]["operation"], continuation_sha256=pair["continuation_sha256"],
                           acknowledged=True, reader_result="ok", error_code=0, required_xp_mask=0,
                           xp_applied_mask=0, required_economic_mask=3, economic_applied_mask=3, complete_xp_set=True),
         "exact original obligation/ACK/complete receipts missing")
    need(type(obligation["acknowledged"]) is type(obligation["complete_xp_set"]) is bool and
         all(type(obligation[key]) is int for key in
             ("error_code", "required_xp_mask", "xp_applied_mask", "required_economic_mask", "economic_applied_mask")),
         "invalid obligation proof field types")
    cleanup = observation["cleanup"]
    need(number(cleanup["original_session"]) and all(type(cleanup[key]) is int and cleanup[key] == cleanup["original_session"]
         for key in ("verified_session", "after_session")) and exact_bool(cleanup["rollback_confirmed"], True) and
         type(cleanup["cleanup_error"]) is int and cleanup["cleanup_error"] == 0 and cleanup["disposition"] == "idle_verified",
         "same-session confirmed cleanup missing")


def assert_qp03_pair_retirement(before, after, pair, attempts):
    return safe(_retirement, before, after, pair, attempts)


def _retirement(before, after, pair, attempts):
    checks.bind(before, after, "QP03")
    pair_value(before, pair)
    pair_value(after, pair)
    checks.acknowledged(before, pair["child"]["operation"])
    checks.acknowledged(after, pair["child"]["operation"])
    checks.replay(before, after)
    need(checks.rows(before, "player_affects") == checks.rows(after, "player_affects"), "cleanup changed affects")
    need(isinstance(attempts, list) and 1 <= len(attempts) <= 16 and pair["pair_valid"] and pair["parent"]["handoff"] in (1, 2),
         "bounded terminal phase2 attempts and original handoff required")
    expected_postimage = None
    retired = False
    prior_sequence = 0
    successful_sequence = None
    generation = None
    for observed in attempts:
        owner_binding(before, pair, observed)
        current_generation = (observed["process_generation"], observed["coordinator_generation"])
        need(generation is None or generation == current_generation, "uncertain retry rebound owner generation")
        generation = current_generation
        verification(pair, observed)
        need(type(observed["parent_live"]) is bool and (pair["parent"]["handoff"] != 1 or not observed["parent_live"]),
             "live handoff1 parent must first reach its actual successor")
        need(observed["successor"] is None and
             number(observed["sequence"]) and observed["sequence"] > prior_sequence, "wrong terminal attempt/order")
        prior_sequence = observed["sequence"]
        postimage = observed["postimage"]
        need(number(postimage["bytes"]) and hex_value(postimage["sha256"], 64) and
             exact_bool(postimage["complete"], True), "whole original attempted postimage missing")
        need(expected_postimage is None or postimage == expected_postimage, "uncertain retry changed complete attempted postimage")
        expected_postimage = postimage
        if retired:
            need(observed["kind"] == "latched_repeat" and observed["journal_result"] is None and
                 exact_bool(observed["terminal_pair_attempted"], False) and
                 number(observed["verification_origin_sequence"]) and
                 observed["verification_origin_sequence"] == successful_sequence and
                 exact_bool(observed["coordinator_return"], True) and exact_bool(observed["retired"], True) and
                 exact_bool(observed["parent_present"], False) and exact_bool(observed["child_present"], False) and
                 exact_bool(observed["context_uncertain"], False),
                 "repeat attempted another journal mutation")
            continue
        need(observed["kind"] == "attempt" and exact_bool(observed["terminal_pair_attempted"], True) and
             number(observed["verification_origin_sequence"]) and
             observed["verification_origin_sequence"] == observed["sequence"],
             "latch/absence without original successful attempt or current verification")
        if observed["journal_result"] == "ok":
            need(exact_bool(observed["coordinator_return"], True) and exact_bool(observed["retired"], True) and
                 exact_bool(observed["parent_present"], False) and exact_bool(observed["child_present"], False) and
                 exact_bool(observed["context_uncertain"], False), "journal OK without guarded terminal success")
            retired = True
            successful_sequence = observed["sequence"]
        else:
            need(observed["journal_result"] in ("append_uncertain", "io_failure") and
                 exact_bool(observed["coordinator_return"], False) and exact_bool(observed["retired"], False) and
                 exact_bool(observed["parent_present"], True) and exact_bool(observed["child_present"], True) and
                 type(observed["context_uncertain"]) is bool and
                 (observed["journal_result"] != "append_uncertain" or observed["context_uncertain"]),
                 "failed/uncertain attempt discarded original pair or claimed retirement")
    need(retired, "no captured successful original guarded retirement")
    return dict(scope="captured QP03 pair-retirement agreement only", observations=len(attempts),
                external_proof_required=list(EXTERNAL))
