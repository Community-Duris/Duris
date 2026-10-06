#!/usr/bin/env python3
"""Assert selected quest state transitions using actual SQL row-shaped cuts.

This is a case oracle, not an independent accounting reconciler or a replacement
for native command/context/ACK validators. It fails on missing evidence. Unit
fixtures prove the oracle's discriminating behavior; they are not native results.
Use capture_quest_cut.py in the actual isolated journey at the documented cuts.
"""

import argparse
from collections import Counter
import json
import struct
from pathlib import Path

from case_data import CASES, ROOT, blocks
from reconcile_economy_accounting import decode_source_event, source_kind_allowed, account_key
from economic_restore_evidence import decode_native_mobile

REASONS = {entry["number"]: entry for entry in
           json.loads((ROOT / "docs/persistence/economy_accounting/registry.json").read_text())["reasons"]}


class CutError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise CutError(message)


def index(rows, keys):
    result = {}
    for row in rows:
        key = tuple(row[name] for name in keys)
        require(key not in result, f"duplicate row identity {keys}: {key}")
        result[key] = row
    return result


def value(row, prefix=""):
    amounts = [row[prefix + unit] for unit in ("copper", "silver", "gold", "platinum")]
    require(all(type(amount) is int for amount in amounts), "money vector must use integer denominations")
    require(all(-(2**63) <= amount < 2**63 for amount in amounts), "native denomination overflow")
    if not prefix or prefix.endswith("after_"):
        require(all(amount >= 0 for amount in amounts), "negative native holding")
    total = sum(amount * factor for amount, factor in zip(amounts, (1, 10, 100, 1000)))
    require(-(2**63) <= total < 2**63, "checked copper total overflow")
    return total


def rows(cut, name):
    require(name in cut and isinstance(cut[name], list), f"missing native cut table {name}")
    return cut[name]


def row(cut, name):
    result = rows(cut, name)
    require(len(result) == 1, f"expected one {name} row")
    return result[0]


def new_rows(before, after, name, keys):
    old = index(rows(before, name), keys)
    current = index(rows(after, name), keys)
    # Append-only evidence must survive restart; ACK/projections use other paths.
    for key, original in old.items():
        require(current.get(key) == original, f"historical {name} evidence changed or disappeared: {key}")
    return [current[key] for key in current.keys() - old.keys()]


def bind(before, after, case_id):
    for name in ("case", "pid", "source_commit", "binary_sha256", "schema_manifest_sha256",
                 "lineage", "epoch", "mobile_instance_ids", "watched_vnums"):
        require(before["meta"][name] == after["meta"][name], f"candidate/capture binding changed: {name}")
    require(before["meta"]["case"] == case_id, "wrong case cuts")
    require(before["meta"]["pid"] > 0, "missing actual player identity")
    for name, size in (("source_commit", 40), ("binary_sha256", 64),
                       ("schema_manifest_sha256", 64), ("lineage", 32), ("epoch", 32)):
        text = before["meta"][name]
        require(isinstance(text, str) and len(text) == size and
                all(c in "0123456789abcdef" for c in text) and int(text, 16) != 0,
                f"invalid {name} pin")
    require(rows(before, "migrations") == rows(after, "migrations"), "database migration set changed between cuts")


def book(after, events, currency, extra_roots=()):
    """Require row links and balance, leaving canonical-byte authentication to Plan5."""
    operations = index(rows(after, "operations"), ("operation_id",))
    receipts = index(rows(after, "inbox"), ("operation_id",))
    references = rows(after, "item_references")
    claims = rows(after, "source_claims")
    roots = set(extra_roots)
    for event in events:
        matching = [ref for ref in references if
                    ref["legacy_operation_id"] == event["operation_id"] and
                    ref["legacy_event_index"] == event["event_index"] and
                    ref["item_uid"] == event["item_uid"] and
                    ref["after_revision"] == event["item_revision"]]
        require(len(matching) == 1, "native item event lacks one exact accounting reference")
        roots.add(matching[0]["operation_id"])
    roots.update(entry["operation_id"] for entry in currency)
    for operation_id in roots:
        operation = operations.get((operation_id,))
        receipt = receipts.get((operation_id,))
        require(operation is not None and operation["outcome"] == 1 and operation["result_code"] == 0,
                "missing successful accounting root")
        require(operation["lineage"] == after["meta"]["lineage"] and
                operation["epoch"] == after["meta"]["epoch"], "root belongs to another lineage/epoch")
        require(receipt is not None and receipt["status"] == 1 and
                receipt["result_code"] == 0 and receipt["failure_stage"] == 0 and
                receipt["committed_at_present"] == 1 and receipt["result_payload"],
                "root lacks its actual committed inbox receipt")
        source = operation["source_event"]
        require(operation["reason"] in REASONS, "unregistered quest/movement reason")
        if REASONS[operation["reason"]]["source_event_required"] or source is not None:
            require(source is not None and len(source) == 96 and int(source, 16) != 0,
                    "missing named original source event")
            require(source_kind_allowed(operation["reason"], decode_source_event(source)[0]),
                    "source kind not allowed by original reason policy")
            require(sum(claim["operation_id"] == operation_id and claim["source_event"] == source and
                        claim["lineage"] == operation["lineage"] and claim["outcome"] == 1
                        for claim in claims) == 1, "source event lacks its unique original claim")
        index(rows(after, "postings"), ("operation_id", "line_index"))
        postings = [entry for entry in rows(after, "postings") if entry["operation_id"] == operation_id]
        require(len(postings) == operation["posting_count"], "root posting count disagrees with native rows")
        require(all(value(entry, "delta_") == entry["copper_value"] for entry in postings),
                "posting denomination arithmetic disagrees")
        require(sum(entry["copper_value"] for entry in postings) == 0, "unbalanced quest money root")
    return roots


def money(before, after):
    old, current = row(before, "player"), row(after, "player")
    entries = new_rows(before, after, "currency", ("operation_id",))
    require(value(current) - value(old) == sum(value(entry, "wallet_delta_") for entry in entries),
            "wallet change has missing, extra or duplicate native currency evidence")
    return entries


def static_complete(before, after, case_id, selected_uids, reward_uids, spare_uids, reward_vnum, expected_xp=None, original_mobile=None):
    require(not CASES[case_id].get("dynamic"), "static completion requires a native Q case")
    choices = [term for term in blocks(case_id) if ("I", reward_vnum) in term["receive"]]
    require(len(choices) == 1, "choose the exact production contract by reward VNUM")
    terms = choices[0]
    original = index(rows(before, "items"), ("item_uid",))
    current = index(rows(after, "items"), ("item_uid",))
    require(len(set(selected_uids)) == len(selected_uids) > 0, "selected roots must be distinct")
    require(not set(selected_uids) & set(spare_uids), "spare was selected")
    inputs = [original.get((uid,)) for uid in selected_uids]
    require(all(item is not None for item in inputs), "selected original UID absent from BEFORE")
    expected = Counter(number for kind, number in terms["give"] if kind == "I")
    require(Counter(item["vnum"] for item in inputs) == expected, "wrong prototype kinds or quantities")
    pid = before["meta"]["pid"]
    events = new_rows(before, after, "ownership_events", ("operation_id", "event_index"))
    retirement_operations = set()
    for item in inputs:
        uid = item["item_uid"]
        require(item["state"] == 1 and item["root_item_uid"] == uid and item["parent_item_uid"] is None,
                "input is not an original loose live root")
        require((item["owner_type"] == 1 and item["owner_id"] == pid) or
                (item["owner_type"] == 12 and item["owner_id"] == original_mobile and original_mobile in before["meta"]["mobile_instance_ids"]),
                "input custody does not belong to original player/native recipient")
        tombstone = current.get((uid,))
        require(tombstone is not None and tombstone["state"] == 2 and tombstone["owner_type"] == 8 and
                tombstone["vnum"] == item["vnum"] and tombstone["root_item_uid"] == uid and
                tombstone["parent_item_uid"] is None and tombstone["item_revision"] > item["item_revision"],
                "input not retired with exact UID/kind/revision")
        terminal = [event for event in events if event["item_uid"] == uid and
                    event["to_owner_type"] == 8 and event["item_revision"] == tombstone["item_revision"]]
        require(len(terminal) == 1, "input lacks one exact native retirement event")
        retirement_operations.add(terminal[0]["operation_id"])
    require(len(retirement_operations) == 1, "selected roots were consumed in different quest operations")
    for uid in spare_uids:
        require(original.get((uid,)) is not None and current.get((uid,)) == original[(uid,)],
                "spare UID/custody/revision changed")
        require(not any(event["item_uid"] == uid for event in events), "spare has a partial native effect")
    require(len(set(reward_uids)) == len(reward_uids), "duplicate reward UID")
    outputs = [current.get((uid,)) for uid in reward_uids]
    require(all(item is not None for item in outputs), "reward UID absent from AFTER")
    require(Counter(item["vnum"] for item in outputs) ==
            Counter(number for kind, number in terms["receive"] if kind == "I"), "wrong reward slots/counts")
    for item in outputs:
        uid = item["item_uid"]
        require((uid,) not in original, "reward reuses an observed UID lifetime")
        require(item["state"] == 1 and item["owner_type"] == 1 and item["owner_id"] == pid and
                item["root_item_uid"] == uid and item["parent_item_uid"] is None, "reward not uniquely player-owned")
        creation = [event for event in events if event["item_uid"] == uid and event["from_owner_type"] == 7 and
                    event["to_owner_type"] == item["owner_type"] and event["to_owner_id"] == item["owner_id"] and
                    event["item_revision"] == item["item_revision"]]
        require(len(creation) == 1, "reward lacks one native creation event")
    fresh = [item for key, item in current.items() if key not in original and item["vnum"] in CASES[case_id]["rewards"] and
             ((item["owner_type"] == 1 and item["owner_id"] == pid) or
              any(event["item_uid"] == item["item_uid"] and event["from_owner_type"] == 7 and
                  event["to_owner_type"] == 1 and event["to_owner_id"] == pid for event in events))]
    require({item["item_uid"] for item in fresh} == set(reward_uids), "extra/unobserved reward issuance")
    currency = money(before, after)
    expected_money = (sum(number for kind, number in terms["receive"] if kind == "C") -
                      sum(number for kind, number in terms["give"] if kind == "C"))
    require([value(entry, "wallet_delta_") for entry in currency] == ([expected_money] if expected_money else []),
            "quest has extra offsetting fee/reward effects")
    require(value(row(after, "player")) - value(row(before, "player")) == expected_money,
            "wrong quest net fee/reward money")
    nominal_xp = sum(number for kind, number in terms["receive"] if kind == "E")
    if nominal_xp:
        require(type(expected_xp) is int and 0 <= expected_xp <= nominal_xp,
                "original frozen XP award required; nominal XP is not the admitted cap")
        allocated = [entry for entry in rows(after, "xp_entitlements") if
                     entry["offering_operation_id"] in retirement_operations and entry["recipient_pid"] == pid]
        require(all(entry["applied"] == 1 for entry in allocated) and
                sum(entry["amount"] for entry in allocated) == expected_xp, "wrong/missing original XP entitlement")
    else:
        expected_xp = 0
    require(row(after, "player")["exp"] - row(before, "player")["exp"] == expected_xp,
            "XP missing or paid twice")
    book(after, events, currency)
    return terms


def unchanged(before, after):
    for name in ("items", "player", "history", "mobiles"):
        require(rows(before, name) == rows(after, name), f"refusal/stale callback changed {name}")
    require(not new_rows(before, after, "ownership_events", ("operation_id", "event_index")), "partial item effect")
    require(not new_rows(before, after, "currency", ("operation_id",)), "partial money effect")
    for name in ("obligations", "xp_entitlements"):
        require(rows(before, name) == rows(after, name), f"refusal/retry changed {name}")


def replay(before, after):
    unchanged(before, after)
    for name in ("operations", "item_references", "postings", "source_claims", "inbox", "effects"):
        require(rows(before, name) == rows(after, name), f"recovery repeated/changed durable {name}")


def later_move(before, after, uid):
    old = index(rows(before, "items"), ("item_uid",))[(uid,)]
    current = index(rows(after, "items"), ("item_uid",))[(uid,)]
    require(old["owner_type"] == 1 and current["owner_type"] == 3 and current["state"] == 1 and
            current["vnum"] == old["vnum"] and current["root_item_uid"] == uid and
            current["parent_item_uid"] is None and current["item_revision"] > old["item_revision"],
            "later drop must retain original UID/kind and advance actual custody")
    require(value(row(before, "player")) == value(row(after, "player")) and
            row(before, "player")["exp"] == row(after, "player")["exp"], "ordinary move paid money/XP")
    require(rows(before, "obligations") == rows(after, "obligations"), "later move rewrote original obligation")
    events = new_rows(before, after, "ownership_events", ("operation_id", "event_index"))
    require(len(events) == 1 and events[0]["item_uid"] == uid and events[0]["to_owner_type"] == 3 and
            events[0]["item_revision"] == current["item_revision"], "move lacks exact native event")
    book(after, events, money(before, after))


def refunded(before, after, quoted_fee):
    require(quoted_fee > 0, "actual quoted fee required")
    for name in ("items", "history", "mobiles"):
        require(rows(before, name) == rows(after, name), f"failed/stale service changed {name}")
    require(row(before, "player") == row(after, "player"), "refund did not preserve original task/money/XP")
    currency = money(before, after)
    require(sorted(value(entry, "wallet_delta_") for entry in currency) == [-quoted_fee, quoted_fee],
            "need one genuine debit and one once-only exact restitution, not refund prose")
    debit = next(entry["operation_id"] for entry in currency if value(entry, "wallet_delta_") < 0)
    refund = next(entry["operation_id"] for entry in currency if value(entry, "wallet_delta_") > 0)
    operation = index(rows(after, "operations"), ("operation_id",)).get((refund,))
    require(operation is not None and operation["original_operation_id"] == debit,
            "refund is not bound to the original committed debit")
    require(not new_rows(before, after, "ownership_events", ("operation_id", "event_index")), "failed service created an item")
    book(after, [], currency)


def mobile(cut, instance):
    entry = index(rows(cut, "mobiles"), ("mobile_instance_id",)).get((instance,))
    require(entry is not None, "original native mobile row missing; actor absence supplies no authority")
    image = bytes.fromhex(entry["canonical_image"])
    decoded = decode_native_mobile(image)  # maintained complete value grammar, not a copied codec
    require(tuple(decoded) == tuple(entry[name] for name in
            ("mobile_instance_id", "mobile_revision", "stock_revision", "lifetime_state")), "native header/image disagreement")
    require(int.from_bytes(image[8:10], "little") == 2, "historical unknown cash is not zero")
    cash_revision, *cash = struct.unpack_from("<Q4q", image, 180)
    return dict(row=entry, image=image, birth=image[40:56].hex(), source=image[56:104].hex(),
                birth_reference=(image[26], image[32:116]), vnum=struct.unpack_from("<i", image, 104)[0],
                transition=image[164:180].hex(), cash_revision=cash_revision, cash=tuple(cash))


def retired(before, after, original_instance, replacement_instance, native_cash_account):
    require(before["meta"]["case"] == "QP03", "D check applies to original QP03 contract")
    require(original_instance != replacement_instance and original_instance in before["meta"]["mobile_instance_ids"] and
            replacement_instance in before["meta"]["mobile_instance_ids"], "distinct original and replacement IDs required")
    original, terminal = mobile(before, original_instance), mobile(after, original_instance)
    replacement, replacement_after = mobile(before, replacement_instance), mobile(after, replacement_instance)
    require(original["vnum"] == terminal["vnum"] == CASES["QP03"]["giver"] and
            replacement["vnum"] == CASES["QP03"]["giver"], "wrong native quest recipient prototype")
    require(original["row"]["lifetime_state"] == 1 and terminal["row"]["lifetime_state"] == 2,
            "original mobile lacks explicit live-to-retired evidence")
    require(terminal["birth_reference"] == original["birth_reference"], "retirement rebound original birth/source")
    require(terminal["row"]["mobile_revision"] > original["row"]["mobile_revision"] and
            terminal["row"]["stock_revision"] >= original["row"]["stock_revision"], "retirement did not advance original revision")
    require(replacement["row"] == replacement_after["row"] and replacement["row"]["lifetime_state"] == 1 and
            replacement["birth"] != original["birth"] and replacement["source"] != original["source"],
            "replacement birth/stock/cash changed or lent original identity")
    require(not any(terminal["cash"]), "retired original retained cash")
    require(terminal["cash_revision"] == original["cash_revision"] + bool(any(original["cash"])), "wrong original cash revision")
    events = new_rows(before, after, "ownership_events", ("operation_id", "event_index"))
    current = index(rows(after, "items"), ("item_uid",))
    stock = [entry for entry in rows(before, "items") if entry["owner_type"] == 12 and entry["owner_id"] == original_instance]
    require(struct.unpack_from("<I", original["image"], 224)[0] == len(stock),
            "original native literal forest/current custody count differs")
    for entry in stock:
        end = current.get((entry["item_uid"],))
        require(end is not None and end["vnum"] == entry["vnum"] and end["state"] == 2 and end["owner_type"] == 8 and
                end["item_revision"] > entry["item_revision"], "original remaining stock not durably destroyed")
        matching = [event for event in events if event["item_uid"] == entry["item_uid"] and
                    event["operation_id"] == terminal["transition"] and event["to_owner_type"] == 8 and
                    event["item_revision"] == end["item_revision"]]
        require(len(matching) == 1, "remaining original stock lacks exact D transition")
    require(not any(entry["state"] == 1 and entry["owner_type"] == 12 and entry["owner_id"] == original_instance
                    for entry in rows(after, "items")), "retired original still owns native stock")
    if stock:
        require(terminal["row"]["stock_revision"] > original["row"]["stock_revision"], "stock revision did not advance")
    for entry in rows(before, "items"):
        if entry["owner_type"] == 12 and entry["owner_id"] == replacement_instance:
            require(current.get((entry["item_uid"],)) == entry and
                    not any(event["item_uid"] == entry["item_uid"] for event in events), "replacement stock affected by original D")
    require(row(before, "player") == row(after, "player"), "D-only terminal cut repeated reward/money/XP/task")
    require(rows(before, "history") == rows(after, "history") and rows(before, "obligations") == rows(after, "obligations") and
            rows(before, "xp_entitlements") == rows(after, "xp_entitlements"), "D-only cut changed original reward evidence")
    require(not money(before, after), "D paid player money again")
    for birth in (original, replacement):
        operation = index(rows(after, "operations"), ("operation_id",)).get((birth["birth"],))
        require(operation is not None and operation["source_event"] == birth["source"], "actual birth source/receipt absent")
    lineage, kind, _, context = account_key(native_cash_account)
    require(lineage == before["meta"]["lineage"] and kind == 1 and context == 12, "original native cash account must be genuine mapped wallet")
    effects = new_rows(before, after, "effects", ("operation_id", "account_index"))
    matching = [entry for entry in effects if entry["operation_id"] == terminal["transition"] and
                entry["account_key"] == native_cash_account]
    require(len(matching) == 1, "D lacks original mapped native cash effect; never infer mapping from mobile ID")
    effect = matching[0]
    require(tuple(effect["before_" + unit] for unit in ("copper", "silver", "gold", "platinum")) == original["cash"] and
            tuple(effect["after_" + unit] for unit in ("copper", "silver", "gold", "platinum")) == terminal["cash"] and
            effect["before_revision"] == original["cash_revision"] and effect["after_revision"] == terminal["cash_revision"],
            "D native cash/economic effect disagreement")
    book(after, events, [], (original["birth"], replacement["birth"], terminal["transition"]))
    postings = [entry for entry in rows(after, "postings") if entry["operation_id"] == terminal["transition"] and
                entry["account_index"] == effect["account_index"]]
    require(sum(entry["copper_value"] for entry in postings) ==
            -sum(coin * factor for coin, factor in zip(original["cash"], (1, 10, 100, 1000))),
            "original cash lacks exact terminal debit posting")


def held(before, after, operation_id):
    # This verifies a stable held cut only, not authority to finish/repeat an effect.
    replay(before, after)
    obligation = index(rows(after, "obligations"), ("offering_operation_id",)).get((operation_id,))
    require(obligation is not None and obligation["acknowledged"] == 0 and obligation["continuation"],
            "visible original unacknowledged obligation required; absence is not held proof")
    receipt = index(rows(after, "inbox"), ("operation_id",)).get((operation_id,))
    require(receipt is not None and receipt["status"] == 1 and receipt["result_code"] == 0 and
            receipt["committed_at_present"] == 1 and receipt["result_payload"], "held original accepted receipt missing")
    # Caller must also preserve matching actual parent/child preimages and native
    # uncertainty disposition from the existing continuation owner.


def acknowledged(cut, operation_id):
    obligation = index(rows(cut, "obligations"), ("offering_operation_id",)).get((operation_id,))
    require(obligation is not None and obligation["acknowledged"] == 1 and obligation["continuation"],
            "missing exact historical acknowledged obligation; frame absence is insufficient")
    receipt = index(rows(cut, "inbox"), ("operation_id",)).get((operation_id,))
    require(receipt is not None and receipt["status"] == 1 and receipt["result_code"] == 0 and
            receipt["committed_at_present"] == 1 and receipt["result_payload"], "original receipt missing")
    entitlements = [entry for entry in rows(cut, "xp_entitlements") if entry["offering_operation_id"] == operation_id]
    require(all(entry["applied"] == 1 for entry in entitlements), "ACK precedes durable original XP application")
    return sum(entry["amount"] for entry in entitlements if entry["recipient_pid"] == cut["meta"]["pid"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=CASES, required=True)
    parser.add_argument("--before", type=Path, required=True)
    parser.add_argument("--after", type=Path, required=True)
    parser.add_argument("--check", choices=("complete", "refused", "replay", "later-move", "refunded", "ack", "retired", "held"), required=True)
    parser.add_argument("--selected", type=int, nargs="*", default=[])
    parser.add_argument("--rewards", type=int, nargs="*", default=[])
    parser.add_argument("--spares", type=int, nargs="*", default=[])
    parser.add_argument("--reward-vnum", type=int)
    parser.add_argument("--expected-xp", type=int, help="original frozen admitted award, not nominal QST XP")
    parser.add_argument("--quoted-fee", type=int)
    parser.add_argument("--offering-operation")
    parser.add_argument("--original-mobile", type=int)
    parser.add_argument("--replacement-mobile", type=int)
    parser.add_argument("--native-cash-account", help="actual mapped wallet key, never inferred from mobile identity")
    args = parser.parse_args()
    try:
        before, after = (json.loads(path.read_text()) for path in (args.before, args.after))
        bind(before, after, args.case)
        if args.check == "complete":
            static_complete(before, after, args.case, args.selected, args.rewards, args.spares, args.reward_vnum, args.expected_xp, args.original_mobile)
        elif args.check == "refused":
            unchanged(before, after)
        elif args.check == "replay":
            replay(before, after)
        elif args.check == "later-move":
            require(len(args.rewards) == 1, "select original reward UID")
            later_move(before, after, args.rewards[0])
        elif args.check == "refunded":
            refunded(before, after, args.quoted_fee)
        elif args.check == "retired":
            retired(before, after, args.original_mobile, args.replacement_mobile, args.native_cash_account)
        elif args.check == "held":
            held(before, after, args.offering_operation)
        else:
            original = index(rows(before, "obligations"), ("offering_operation_id",)).get((args.offering_operation,))
            current = index(rows(after, "obligations"), ("offering_operation_id",)).get((args.offering_operation,))
            require(original is not None and current is not None and
                    current["continuation"] == original["continuation"], "original frozen reward terms changed or missing")
            acknowledged(after, args.offering_operation)
        print(json.dumps(dict(case=args.case, check=args.check, result="captured-state predicates passed",
                              native_command_context_ack_qualification="separate primary-owned proof required")))
    except (CutError, KeyError, TypeError, ValueError) as error:
        parser.exit(1, f"quest cut refused: {error}\n")


if __name__ == "__main__":
    main()
