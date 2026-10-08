"""Paid QP02 cut agreement only; not mapping, projection or publication authority."""

from collections import Counter
import json
import re

from case_data import ROOT, blocks
import quest_cut_checks as checks
from reconcile_economy_accounting import account_key, decode_source_event


REGISTRY = json.loads((ROOT / "docs/persistence/economy_accounting/registry.json").read_text())


def registered(group, name):
    return next(entry["number"] for entry in REGISTRY[group] if entry["id"] == name)


def constant(path, name):
    # Read maintained numeric policy constants, never a copied mutation codec.
    matches = re.findall(r"constexpr uint\d+_t " + re.escape(name) + r" = (\d+);",
                         (ROOT / path).read_text())
    checks.require(len(matches) == 1, "maintained cost policy constant unavailable: " + name)
    return int(matches[0])


POLICY = constant("src/economy/native_quest_cost_policy.h", "ECONOMIC_QUEST_REQUIREMENT_POLICY_VERSION")
SINK_ID = constant("src/economy/native_quest_cost_policy.h", "ECONOMIC_QUEST_REQUIREMENT_SINK_ID")
SINK_CONTEXT = constant("src/economy/native_quest_cost_policy.h", "ECONOMIC_QUEST_REQUIREMENT_SINK_CONTEXT")
WRITER = constant("src/economy/item_transfer_accounting.h", "ECONOMIC_WRITER_ITEM_TRANSFER")
UNITS = ("copper", "silver", "gold", "platinum")


def vector(entry, prefix):
    checks.value(entry, prefix)  # maintained integer/overflow arithmetic
    return tuple(entry[prefix + unit] for unit in UNITS)


def revision(value):
    return type(value) is int and 0 < value < 2**64


def assert_paid_qp02(before, after, *, original_instance, native_wallet, cost_operation, reward_vnum, fee):
    """Check a funded-before/consumed-after interval, before reward publication.

    native_wallet is an explicit observed mapping key, not a capability. Exact
    frozen projector/command authentication and the physical cut remain external.
    Raises CutError on missing/inconsistent captured inputs; does not mutate cuts.
    """
    try:
        return _paid(before, after, original_instance, native_wallet, cost_operation, reward_vnum, fee)
    except (KeyError, TypeError, ValueError, StopIteration) as error:
        raise checks.CutError("paid QP02 cut agreement refused: " + str(error)) from error


def _paid(before, after, instance, wallet, operation_id, reward_vnum, fee):
    checks.bind(before, after, "QP02")
    checks.require(type(instance) is int and 0 < instance < 2**64 - 1 and
                   instance in before["meta"]["mobile_instance_ids"], "original recipient identity missing")
    checks.require(isinstance(operation_id, str) and re.fullmatch(r"[0-9a-f]{32}", operation_id) and
                   int(operation_id, 16), "original cost operation invalid")
    choices = [term for term in blocks("QP02") if ("I", reward_vnum) in term["receive"]]
    checks.require(len(choices) == 1 and type(reward_vnum) is int, "exact QP02 contract required")
    terms = choices[0]
    checks.require(type(fee) is int and fee > 0 and
                   [number for kind, number in terms["give"] if kind == "C"] == [fee], "wrong paid QP02 fee")
    # Loader prepends Q: gloves0/backpack1/shirt2/shoes3, not catalog sort order.
    slot = {19010: 0, 19008: 2, 19007: 3}[reward_vnum]
    for cut in (before, after):
        checks.require(cut["meta"].get("authority") == "native", "native-shaped cuts required")
        checks.require(checks.rows(cut, "epochs") == [dict(lineage=cut["meta"]["lineage"], epoch=cut["meta"]["epoch"])],
                       "original current epoch row missing")
    original, charged = checks.mobile(before, instance), checks.mobile(after, instance)
    checks.require(original["vnum"] == charged["vnum"] == 19005 and
                   original["row"]["lifetime_state"] == charged["row"]["lifetime_state"] == 1 and
                   original["birth_reference"] == charged["birth_reference"] and original["birth_reference"][0] == 1,
                   "original live reset birth identity changed")
    for field in ("mobile_revision", "stock_revision"):
        old, new = original["row"][field], charged["row"][field]
        checks.require(revision(old) and revision(new) and new == old + 1, "wrong one-transition " + field)
    checks.require(revision(original["cash_revision"]) and charged["cash_revision"] == original["cash_revision"] + 1 and
                   charged["transition"] == operation_id and original["transition"] != operation_id,
                   "wrong original cash revision/transition")
    lineage, kind, mapping, context = account_key(wallet)
    checks.require(lineage == before["meta"]["lineage"] and kind == registered("account_kinds", "wallet") and
                   context == 12 and mapping > 0, "explicit original mapped native wallet required")
    birth_source = decode_source_event(original["source"])
    checks.require(birth_source[0] == registered("source_kinds", "npc_generation") and
                   birth_source[1] == birth_source[2] and birth_source[3] == 0, "reset NPC birth source required")

    # Keep historical birth epoch separate from the current charged book.
    for cut in (before, after):
        birth = checks.index(checks.rows(cut, "operations"), ("operation_id",)).get((original["birth"],))
        receipt = checks.index(checks.rows(cut, "inbox"), ("operation_id",)).get((original["birth"],))
        checks.require(birth is not None and birth["source_event"] == original["source"] and
                       birth["lineage"] == lineage and birth["reason"] == registered("reasons", "npc_reward") and
                       birth["outcome"] == 1 and birth["result_code"] == 0, "original birth root/source missing")
        checks.require(receipt is not None and receipt["status"] == 1 and receipt["result_code"] == 0 and
                       receipt["failure_stage"] == 0 and receipt["committed_at_present"] == 1 and receipt["result_payload"],
                       "original birth receipt missing")
        checks.require(sum(claim["operation_id"] == original["birth"] and claim["source_event"] == original["source"] and
                           claim["lineage"] == lineage and claim["outcome"] == 1 for claim in checks.rows(cut, "source_claims")) == 1,
                       "original birth claim missing/duplicate")
        origins = checks.index(checks.rows(cut, "birth_origins"), ("mobile_instance_id",))
        origin = origins.get((instance,))
        checks.require(origin is not None and origin["birth_operation"] == original["birth"] and
                       revision(origin["publication_revision"]) and origin["canonical_origin"] and
                       bytes.fromhex(origin["canonical_origin"]), "original opaque birth carrier missing")
    checks.require(checks.rows(before, "birth_origins") == checks.rows(after, "birth_origins"), "birth carrier changed")
    operations = checks.new_rows(before, after, "operations", ("operation_id",))
    receipts = checks.new_rows(before, after, "inbox", ("operation_id",))
    checks.require(len(operations) == len(receipts) == 1 and operations[0]["operation_id"] ==
                   receipts[0]["operation_id"] == operation_id, "cost-only interval has extra/missing roots")
    root = operations[0]
    checks.require(all(type(root[field]) is int for field in ("reason", "writer_id", "policy_version", "compiler_version")) and
                   root["reason"] == registered("reasons", "quest_cost") and root["writer_id"] == WRITER and
                   root["policy_version"] == POLICY and root["compiler_version"] == 1, "wrong maintained quest cost policy")
    checks.require(decode_source_event(root["source_event"]) ==
                   (registered("source_kinds", "quest_action"), original["birth"], birth_source[2],
                    original["row"]["stock_revision"], slot), "wrong original item-cost action source")
    claims = checks.new_rows(before, after, "source_claims", ("lineage", "source_event"))
    checks.require(len(claims) == 1 and claims[0]["operation_id"] == operation_id and
                   claims[0]["source_event"] == root["source_event"], "extra/missing original cost claim")

    effects = checks.new_rows(before, after, "effects", ("operation_id", "account_index"))
    checks.require(len(effects) == 2 and all(e["operation_id"] == operation_id for e in effects), "extra/missing cost effects")
    effect_by_index = checks.index(effects, ("account_index",))
    debit, sink = effect_by_index[(0,)], effect_by_index[(1,)]
    checks.require(debit["account_key"] == wallet and vector(debit, "before_") == original["cash"] and
                   vector(debit, "after_") == charged["cash"] and debit["before_revision"] == original["cash_revision"] and
                   debit["after_revision"] == charged["cash_revision"] and revision(debit["before_revision"]) and
                   revision(debit["after_revision"]), "original wallet cash/effect disagreement")
    checks.require(0 <= checks.value(debit, "before_") <= 2**31 - 1 and
                   0 <= checks.value(debit, "after_") <= 2**31 - 1, "cash exceeds maintained native cost range")
    checks.require(account_key(sink["account_key"]) ==
                   (lineage, registered("account_kinds", "sink"), SINK_ID, SINK_CONTEXT) and
                   vector(sink, "before_") == vector(sink, "after_") == (0, 0, 0, 0) and
                   type(sink["before_revision"]) is int and type(sink["after_revision"]) is int and
                   sink["before_revision"] == sink["after_revision"] == 0, "wrong requirement sink effect")
    delta = tuple(new - old for old, new in zip(original["cash"], charged["cash"]))
    postings = checks.new_rows(before, after, "postings", ("operation_id", "line_index"))
    checks.require(len(postings) == 2 and all(p["operation_id"] == operation_id for p in postings), "extra/duplicate cost postings")
    for posting in postings:
        idx = posting["account_index"]
        checks.require(all(type(posting[field]) is int for field in
                           ("account_index", "line_index", "event_index", "child_index", "copper_value")) and
                       idx in (0, 1) and posting["line_index"] == posting["event_index"] == idx and
                       posting["child_index"] == 0, "wrong cost posting indices")
        checks.require(vector(posting, "delta_") == tuple(part * (1 if idx == 0 else -1) for part in delta),
                       "cost posting denominations disagree")
    checks.require({p["account_index"] for p in postings} == {0, 1} and checks.value(debit, "after_") -
                   checks.value(debit, "before_") == -fee, "wrong fee or duplicate wallet posting")

    checks.require(checks.row(before, "player") == checks.row(after, "player"), "cost repeated player money/XP/task effect")
    checks.require(not checks.new_rows(before, after, "currency", ("operation_id",)), "cost has second player debit/credit")
    for table in ("history", "player_affects", "xp_entitlements"):
        checks.require(checks.rows(before, table) == checks.rows(after, table), "unexpected cost-only effect: " + table)
    obligations = checks.new_rows(before, after, "obligations", ("offering_operation_id",))
    checks.require(len(obligations) == 1 and obligations[0]["offering_operation_id"] == operation_id and
                   obligations[0]["player_pid"] == before["meta"]["pid"] and obligations[0]["continuation"] and
                   obligations[0]["xp_applied_mask"] == obligations[0]["acknowledged"] == 0,
                   "original unacknowledged reward obligation missing/advanced")
    events = checks.new_rows(before, after, "ownership_events", ("operation_id", "event_index"))
    old_items = checks.index(checks.rows(before, "items"), ("item_uid",))
    new_items = checks.index(checks.rows(after, "items"), ("item_uid",))
    checks.require(old_items.keys() == new_items.keys(), "cost-only interval issued/lost item rows")
    consumed = set()
    for event in events:
        uid = event["item_uid"]
        old, new = old_items[(uid,)], new_items[(uid,)]
        checks.require(uid not in consumed and event["operation_id"] == operation_id and
                       event["from_owner_type"] == old["owner_type"] == 12 and event["from_owner_id"] == old["owner_id"] == instance and
                       event["to_owner_type"] == new["owner_type"] == 8 and event["to_owner_id"] == new["owner_id"] == 0 and
                       old["state"] == 1 and new["state"] == 2 and old["vnum"] == new["vnum"] == 19006 and
                       old["owner_context_id"] == new["owner_context_id"] == 0 and
                       old["root_item_uid"] == new["root_item_uid"] == uid and old["parent_item_uid"] is new["parent_item_uid"] is None and
                       new["item_revision"] == old["item_revision"] + 1 == event["item_revision"], "unexpected item-cost consumption")
        consumed.add(uid)
    checks.require(len(consumed) == Counter(n for kind, n in terms["give"] if kind == "I")[19006], "wrong QP02 input quantity")
    checks.require(type(root["item_event_count"]) is int and root["item_event_count"] == len(events) and
                   type(root["child_count"]) is int and root["child_count"] == 0, "wrong cost root item/child counts")
    checks.require(all(new_items[key] == value for key, value in old_items.items() if key[0] not in consumed), "unselected holding changed")
    refs = checks.new_rows(before, after, "item_references", ("operation_id", "line_index"))
    checks.require(len(refs) == len(events) and all(r["operation_id"] == operation_id for r in refs), "extra/missing item references")
    for reference in refs:
        uid = reference["item_uid"]
        checks.require(uid in consumed and reference["legacy_operation_id"] == operation_id and
                       reference["before_revision"] == old_items[(uid,)]["item_revision"], "wrong original input reference")
    checks.book(after, events, [], (operation_id,))
    others_before = [m for m in checks.rows(before, "mobiles") if m["mobile_instance_id"] != instance]
    others_after = [m for m in checks.rows(after, "mobiles") if m["mobile_instance_id"] != instance]
    checks.require(others_before == others_after, "cost changed replacement/other recipient")
    return dict(scope="captured paid-QP02 agreement only", fee=fee, wallet_delta=delta,
                mapping_projection_publication_authentication="required from original owner")
