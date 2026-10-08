"""Temporal QP03 captured-cut agreement; no D owner or chronology authority."""

import json
import re
import struct

from case_data import ROOT
import quest_cut_checks as checks
from reconcile_economy_accounting import account_key, decode_source_event


REGISTRY = json.loads((ROOT / "docs/persistence/economy_accounting/registry.json").read_text())
UNITS = ("copper", "silver", "gold", "platinum")
HISTORY = {
    "operations": ("operation_id",), "inbox": ("operation_id",),
    "source_claims": ("lineage", "source_event"),
    "postings": ("operation_id", "line_index"),
    "effects": ("operation_id", "account_index"),
    "ownership_events": ("operation_id", "event_index"),
    "item_references": ("operation_id", "line_index"),
    "birth_origins": ("mobile_instance_id",),
}
FROZEN = ("player", "history", "player_affects", "currency", "obligations", "xp_entitlements")


def registered(group, name):
    return next(entry["number"] for entry in REGISTRY[group] if entry["id"] == name)


def vector(entry, prefix):
    checks.value(entry, prefix)
    return tuple(entry[prefix + unit] for unit in UNITS)


def identity(value):
    return type(value) is int and 0 < value < 2**64 - 1


def operation(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{32}", value) and int(value, 16)


def wallet_key(encoded, lineage):
    decoded = account_key(encoded)
    checks.require(decoded[0] == lineage and decoded[1] == registered("account_kinds", "wallet") and
                   decoded[3] == 12, "explicit observed native wallet required")
    return decoded


def bind_stages(cuts, original, replacement):
    common = ("case", "pid", "source_commit", "binary_sha256", "schema_manifest_sha256",
              "lineage", "epoch", "watched_vnums", "watched_item_uids")
    for stage, cut in enumerate(cuts):
        # Self-validation uses the original metadata. Never sanitize mobile IDs
        # to make the existing pair bind accept different observation stages.
        checks.bind(cut, cut, "QP03")
        checks.require(all(cut["meta"].get(name) == cuts[0]["meta"].get(name) for name in common),
                       "candidate/schema/lineage/epoch/watch binding changed")
        checks.require(checks.rows(cut, "migrations") == checks.rows(cuts[0], "migrations"), "schema changed")
        checks.require(cut["meta"].get("authority") == "native" and checks.rows(cut, "epochs") ==
                       [dict(lineage=cut["meta"]["lineage"], epoch=cut["meta"]["epoch"])], "current native epoch required")
        expected = {original} if stage < 2 else {original, replacement}
        ids = cut["meta"]["mobile_instance_ids"]
        checks.require(all(identity(value) for value in ids) and len(ids) == len(expected) and set(ids) == expected,
                       "wrong stage-specific observed mobile IDs")
        observed = checks.index(checks.rows(cut, "mobiles"), ("mobile_instance_id",))
        checks.require(set(observed) == {(value,) for value in expected}, "missing/extra observed mobile row")


def birth_links(cut, mobile):
    lineage = cut["meta"]["lineage"]
    checks.require(mobile["birth_reference"][0] == 1 and mobile["vnum"] == 16006 and
                   struct.unpack_from('<i', mobile["image"], 108)[0] == 16077, "original reset Auriam required")
    source = decode_source_event(mobile["source"])
    checks.require(source[0] == registered("source_kinds", "npc_generation") and source[1] == source[2] and
                   source[3] == 0 and source[1] != mobile["birth"], "reset birth source shape changed")
    root = checks.index(checks.rows(cut, "operations"), ("operation_id",)).get((mobile["birth"],))
    receipt = checks.index(checks.rows(cut, "inbox"), ("operation_id",)).get((mobile["birth"],))
    checks.require(root is not None and root["source_event"] == mobile["source"] and root["lineage"] == lineage and
                   root["reason"] == registered("reasons", "npc_reward") and root["outcome"] == 1 and root["result_code"] == 0 and
                   operation(root["epoch"]), "retained birth root/source missing")
    checks.require(receipt is not None and receipt["status"] == 1 and receipt["result_code"] == 0 and
                   receipt["failure_stage"] == 0 and receipt["committed_at_present"] == 1 and receipt["result_payload"],
                   "retained birth receipt missing")
    checks.require(sum(claim["operation_id"] == mobile["birth"] and claim["source_event"] == mobile["source"] and
                       claim["lineage"] == lineage and claim["outcome"] == 1 for claim in checks.rows(cut, "source_claims")) == 1,
                   "retained birth claim missing/duplicate")
    carrier = checks.index(checks.rows(cut, "birth_origins"), ("mobile_instance_id",)).get((mobile["row"]["mobile_instance_id"],))
    checks.require(carrier is not None and carrier["birth_operation"] == mobile["birth"] and
                   identity(carrier["publication_revision"]) and carrier["canonical_origin"] and
                   bytes.fromhex(carrier["canonical_origin"]), "retained opaque birth carrier missing")


def interval(before, after, operation_id):
    added = {table: checks.new_rows(before, after, table, keys) for table, keys in HISTORY.items()}
    for table in FROZEN:
        checks.require(checks.rows(before, table) == checks.rows(after, table), "interval repeated player/reward effect: " + table)
    for table in ("operations", "inbox", "source_claims"):
        checks.require(len(added[table]) == 1 and added[table][0]["operation_id"] == operation_id,
                       "extra/missing interval root/receipt/claim: " + table)
    for table in ("postings", "effects", "ownership_events", "item_references"):
        checks.require(all(entry["operation_id"] == operation_id for entry in added[table]), "foreign interval effect: " + table)
    return added


def cash_book(cut, added, mobile_before, mobile_after, wallet, *, birth=False):
    """Exact observed wallet vector links; counterparty semantics stay external."""
    effects = checks.index(added["effects"], ("account_index",))
    checks.require(len(effects) == 2 and all(type(key[0]) is int and key[0] >= 0 for key in effects),
                   "narrow wallet/counterparty pair required")
    accounts = checks.index(added["effects"], ("account_key",))
    checks.require((wallet,) in accounts, "original mapped wallet effect missing")
    debit = accounts[(wallet,)]
    old_cash = (0,) * 4 if birth else mobile_before["cash"]
    old_revision = 0 if birth else mobile_before["cash_revision"]
    checks.require(vector(debit, "before_") == old_cash and vector(debit, "after_") == mobile_after["cash"] and
                   type(debit["before_revision"]) is int and type(debit["after_revision"]) is int and
                   debit["before_revision"] == old_revision and debit["after_revision"] == mobile_after["cash_revision"],
                   "native wallet vector/revision disagreement")
    postings = added["postings"]
    checks.require(all(type(p["account_index"]) is int and (p["account_index"],) in effects and
                       type(p["line_index"]) is int and p["line_index"] >= 0 and
                       type(p["event_index"]) is int and p["event_index"] >= 0 and
                       type(p["child_index"]) is int and p["child_index"] == 0 for p in postings),
                   "posting has missing effect/invalid indices")
    for idx, effect in effects.items():
        parts = [p for p in postings if p["account_index"] == idx[0]]
        checks.require(len(parts) == 1, "extra/missing wallet or counterparty posting")
        actual = vector(parts[0], "delta_")
        if effect["account_key"] == wallet:
            expected = tuple(new - old for old, new in zip(old_cash, mobile_after["cash"]))
            checks.require(actual == expected, "wallet posting denomination disagreement")
        else:
            decoded = account_key(effect["account_key"])
            expected_kind = registered("account_kinds", "issuance" if birth else "sink")
            checks.require(decoded[0] == cut["meta"]["lineage"] and decoded[1] == expected_kind and
                           vector(effect, "before_") == vector(effect, "after_") == (0,) * 4 and
                           type(effect["before_revision"]) is int and type(effect["after_revision"]) is int and
                           effect["before_revision"] == effect["after_revision"] == 0,
                           "unrelated holding changed in narrow interval")
    checks.require(all(sum(vector(p, "delta_")[idx] for p in postings) == 0 for idx in range(4)),
                   "unbalanced denomination vectors")
    checks.book(cut, added["ownership_events"], [], (mobile_after["transition"],))


def stock_interval(before, after, added, instance, operation_id, *, birth=False):
    old = checks.index(checks.rows(before, "items"), ("item_uid",))
    current = checks.index(checks.rows(after, "items"), ("item_uid",))
    stock = [entry for entry in checks.rows(after if birth else before, "items") if
             entry["owner_type"] == 12 and entry["owner_id"] == instance]
    mobile = checks.mobile(after if birth else before, instance)
    checks.require(struct.unpack_from('<I', mobile["image"], 224)[0] == len(stock), "selected native forest/custody count differs")
    selected = {entry["item_uid"] for entry in stock}
    checks.require(all(identity(uid) for uid in selected), "invalid stock UID")
    expected_keys = set(old) | {(uid,) for uid in selected} if birth else set(old)
    checks.require(set(current) == expected_keys, "interval issued/lost unrelated item rows")
    for entry in stock:
        uid = entry["item_uid"]
        end = current[(uid,)]
        checks.require(entry["state"] == 1 and entry["owner_context_id"] == 0 and identity(entry["item_revision"]), "invalid live stock custody")
        if birth:
            checks.require((uid,) not in old and entry["item_revision"] == 1, "replacement borrowed an observed UID lifetime")
        else:
            checks.require(end["state"] == 2 and end["owner_type"] == 8 and end["owner_id"] == 0 and
                           identity(end["item_revision"]) and end["item_revision"] > entry["item_revision"] and
                           all(end[field] == entry[field] for field in
                               ("vnum", "root_item_uid", "parent_item_uid", "owner_context_id", "equipment_slot")),
                           "original residual stock not exactly retired")
        events = [event for event in added["ownership_events"] if event["item_uid"] == uid]
        checks.require(len(events) == 1, "stock lacks one exact ownership event")
        event = events[0]
        checks.require(all(type(event[field]) is int for field in
                           ("event_index", "from_owner_type", "from_owner_id", "to_owner_type", "to_owner_id", "item_revision")) and
                       event["event_index"] >= 0 and event["operation_id"] == operation_id and event["from_owner_type"] == (7 if birth else 12) and
                       event["from_owner_id"] == (0 if birth else instance) and event["to_owner_type"] == (12 if birth else 8) and
                       event["to_owner_id"] == (instance if birth else 0) and event["item_revision"] == end["item_revision"],
                       "stock event borrowed custody/operation")
        refs = [ref for ref in added["item_references"] if ref["item_uid"] == uid]
        checks.require(len(refs) == 1 and all(type(refs[0][field]) is int for field in
                       ("line_index", "legacy_event_index", "before_revision", "after_revision")) and
                       refs[0]["line_index"] >= 0 and refs[0]["before_revision"] == (0 if birth else entry["item_revision"]),
                       "stock lacks exact before reference")
    checks.require(len(added["ownership_events"]) == len(added["item_references"]) == len(stock), "extra stock/reward event")
    checks.require(all(current[key] == entry for key, entry in old.items() if birth or key[0] not in selected),
                   "unselected holding/history changed")
    root = added["operations"][0]
    checks.require(type(root["item_event_count"]) is int and root["item_event_count"] == len(stock) and
                   type(root["child_count"]) is int and root["child_count"] == 0, "wrong stock root counts")
    return selected


def assert_temporal_qp03(before, terminal, replacement_born, *stable_cuts, original_instance,
                         replacement_instance, original_wallet, replacement_wallet, terminal_operation):
    """Require A live->terminal, then observed B creation, then stable cuts.

    Stage order is a caller assertion, not authenticated reset chronology. Wallet
    arguments are observed keys, not proof of mapping or mutation authority.
    """
    try:
        return _temporal((before, terminal, replacement_born, *stable_cuts), original_instance,
                         replacement_instance, original_wallet, replacement_wallet, terminal_operation)
    except (KeyError, TypeError, ValueError, StopIteration, struct.error) as error:
        raise checks.CutError("temporal QP03 cut agreement refused: " + str(error)) from error


def _temporal(cuts, original, replacement, original_wallet, replacement_wallet, terminal_op):
    checks.require(len(cuts) >= 4 and identity(original) and identity(replacement) and original != replacement and
                   operation(terminal_op), "incomplete stages or invalid original/replacement/terminal identity")
    bind_stages(cuts, original, replacement)
    before, terminal, born = cuts[:3]
    a, dead, b = checks.mobile(before, original), checks.mobile(terminal, original), checks.mobile(born, replacement)
    for cut in cuts:
        birth_links(cut, checks.mobile(cut, original))
    checks.require(a["row"]["lifetime_state"] == 1 and dead["row"]["lifetime_state"] == 2 and
                   a["birth_reference"] == dead["birth_reference"] and dead["transition"] == terminal_op and
                   a["transition"] != terminal_op and terminal_op != a["birth"], "missing original live-to-terminal transition")
    checks.require(dead["row"]["mobile_revision"] > a["row"]["mobile_revision"] and
                   dead["row"]["stock_revision"] >= a["row"]["stock_revision"] and
                   not any(dead["cash"]) and dead["cash_revision"] == a["cash_revision"] + bool(any(a["cash"])),
                   "wrong terminal native revision/cash")
    checks.require(b["row"]["lifetime_state"] == 1 and b["birth"] not in (a["birth"], terminal_op) and b["source"] != a["source"] and
                   b["row"]["mobile_revision"] == b["row"]["stock_revision"] == b["cash_revision"] == 1 and
                   b["transition"] == b["birth"],
                   "replacement reused identity/birth/source or is not an observed birth cut")
    birth_links(born, b)
    for cut in (before, terminal):
        checks.require(not any(entry["operation_id"] == b["birth"] for entry in checks.rows(cut, "operations")) and
                       not any(entry["mobile_instance_id"] == replacement for entry in checks.rows(cut, "birth_origins")) and
                       not any(entry["owner_type"] == 12 and entry["owner_id"] == replacement for entry in checks.rows(cut, "items")),
                       "replacement already observed before terminal stage")
    lineage = before["meta"]["lineage"]
    wallet_key(original_wallet, lineage)
    wallet_key(replacement_wallet, lineage)
    checks.require(original_wallet != replacement_wallet, "replacement borrowed original wallet key")
    destroyed = interval(before, terminal, terminal_op)
    checks.require(not destroyed["birth_origins"], "D-only interval issued a birth carrier")
    residual = stock_interval(before, terminal, destroyed, original, terminal_op)
    checks.require(not residual or dead["row"]["stock_revision"] > a["row"]["stock_revision"], "residual stock clock did not advance")
    cash_book(terminal, destroyed, a, dead, original_wallet)
    created = interval(terminal, born, b["birth"])
    checks.require(len(created["birth_origins"]) == 1 and created["birth_origins"][0]["mobile_instance_id"] == replacement,
                   "missing/extra replacement birth carrier")
    checks.require(checks.mobile(born, original)["row"] == dead["row"], "B birth changed original terminal image")
    fresh = stock_interval(terminal, born, created, replacement, b["birth"], birth=True)
    cash_book(born, created, None, b, replacement_wallet, birth=True)
    for previous, current in zip(cuts[2:], cuts[3:]):
        checks.bind(previous, current, "QP03")
        checks.replay(previous, current)
        checks.require(checks.rows(previous, "player_affects") == checks.rows(current, "player_affects"), "stable cut changed affects")
    return dict(scope="captured temporal-QP03 agreement only", original_residual_uids=sorted(residual),
                replacement_stock_uids=sorted(fresh), stable_intervals=len(cuts) - 3,
                external_proof_required=["authenticated reset chronology and complete mobile census; selected absence is insufficient",
                    "real quest-D owner, policy/source/counterparty authority and full physical retirement",
                    "original/replacement wallet mapping authentication and literal full-forest/custody correspondence",
                    "native holds, original parent/child pair ACK and retirement; SQL receipt/carrier bytes are opaque"],
                epoch_rotation="unsupported; historical birth roots may belong to older epochs")
