"""Optional LIVE wallet join over captured values; never native owner authority."""

import re

from capture_quest_cut import MAX_ROWS, watched_mapping_ids
import quest_cut_checks as checks
from reconcile_economy_accounting import account_key

# Public observe_mapping/native birth predicates, not the generic cursor fixture.
# economic_sql_native_mobile_birth_transaction.h: locator7;
# native_mobile_birth_accounting.h / item_owner_type::native_mobile: context12.
WALLET_KIND, WALLET_CONTEXT, SQL_BACKEND, WALLET_LOCATOR = 1, 12, 1, 7
EXTERNAL_PROOF_REQUIRED = (
    "original owner authentication of the birth command, retained receipt/carrier, lineage and wallet mapping lifetime",
    "actual same-cut reconnect-disabled IN_TRANS owner borrow, lifecycle exclusion, mapping/native locks and current cash",
    "complete current native world/stock/custody correspondence and physical publication/hold/ACK proof",
)


def unsigned(value, *, positive=False):
    return type(value) is int and (1 if positive else 0) <= value < 2**64


def operation(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{32}", value) and int(value, 16) != 0


def assert_live_native_wallet(cut, *, original_instance, native_wallet):
    """Join explicit observed key/mapping/head/live image/original birth values.

    Does not mutate inputs, resolve authority, interpret opaque historical
    receipts/carriers, or establish a retired/D lifetime contract. No callers
    opt in automatically. Raises CutError for missing/inconsistent capture.
    """
    try:
        return _live(cut, original_instance, native_wallet)
    except (KeyError, TypeError, ValueError, IndexError) as error:
        raise checks.CutError("live native wallet captured agreement refused: " + str(error)) from error


def _live(cut, instance, wallet):
    meta = cut["meta"]
    checks.bind(cut, cut, meta["case"])  # maintained candidate/lineage/epoch pin grammar
    checks.require(meta.get("authority") == "native" and
                   meta.get("mapping_observation_authenticated") is False,
                   "explicit native-shaped unauthenticated mapping capture required")
    checks.require(unsigned(instance, positive=True) and instance != 2**64 - 1,
                   "explicit original native instance invalid")
    ids = meta["mobile_instance_ids"]
    checks.require(isinstance(ids, list) and all(unsigned(uid, positive=True) and uid != 2**64 - 1 for uid in ids)
                   and len(ids) == len(set(ids)) and instance in ids, "selected native observation missing/duplicate")
    lineage, kind, mapping_id, context = account_key(wallet)
    checks.require(lineage == meta["lineage"] and kind == WALLET_KIND and context == WALLET_CONTEXT,
                   "explicit observed native wallet key disagrees with capture")
    for name in ("watched_mapping_ids", "missing_mapping_ids"):
        checks.require(isinstance(meta[name], list) and watched_mapping_ids(meta[name]) == meta[name],
                       "mapping selection metadata must be bounded, sorted and distinct")
    watched, missing = meta["watched_mapping_ids"], meta["missing_mapping_ids"]
    mappings = checks.rows(cut, "account_mappings")
    checks.require(len(mappings) <= MAX_ROWS and all(unsigned(row["mapping_id"], positive=True) for row in mappings),
                   "invalid mapping row identity/budget")
    by_id = checks.index(mappings, ("mapping_id",))
    observed = {key[0] for key in by_id}
    checks.require(observed <= set(watched) and missing == sorted(set(watched) - observed) and
                   mapping_id in watched and mapping_id not in missing,
                   "explicit mapping selection/presence disagrees")
    mapping = by_id[(mapping_id,)]
    expected = dict(account_kind=kind, context_id=context, backend_kind=SQL_BACKEND,
                    locator_kind=WALLET_LOCATOR, native_id=instance, active_native_id=instance, revision=0)
    checks.require(all(type(mapping[name]) is int and mapping[name] == value for name, value in expected.items())
                   and mapping["lineage"] == lineage and mapping["retiring_operation_id"] is None,
                   "selected mapping is not the exact live native wallet lifetime")
    head = checks.row(cut, "lineage_head")
    checks.require(head["lineage"] == lineage and head["active_epoch"] == meta["epoch"] and unsigned(head["revision"]),
                   "current lineage head/epoch/revision disagrees")
    checks.require(checks.rows(cut, "epochs") == [dict(lineage=lineage, epoch=meta["epoch"])],
                   "captured current epoch row missing/different")
    mobile = checks.mobile(cut, instance)  # maintained full image grammar and row binding; requires known cash
    checks.require(all(unsigned(mobile["row"][name], positive=True) for name in
                       ("mobile_instance_id", "mobile_revision", "stock_revision", "lifetime_state")) and
                   mobile["row"]["lifetime_state"] == 1, "selected native image must be LIVE")
    checks.require(mapping["creating_operation_id"] == mobile["birth"], "mapping creator differs from original birth")
    birth = checks.index(checks.rows(cut, "operations"), ("operation_id",)).get((mobile["birth"],))
    checks.require(birth is not None and birth["lineage"] == lineage and operation(birth["epoch"]) and
                   birth["source_event"] == mobile["source"], "original birth reference/root source disagrees")
    # Birth epoch is historical. Do not equate it with the current head, nor
    # mistake these literal root/carrier links for the original owner's proof.
    origin = checks.index(checks.rows(cut, "birth_origins"), ("mobile_instance_id",)).get((instance,))
    checks.require(origin is not None and type(origin["mobile_instance_id"]) is int and
                   origin["birth_operation"] == mobile["birth"], "original opaque carrier row binding disagrees")
    return dict(scope="captured live native-wallet agreement only", owner_authenticated=False,
                world_publication_proven=False, mapping_id=mapping_id, native_instance=instance,
                lineage=lineage, current_epoch=meta["epoch"], lineage_revision=head["revision"],
                historical_birth_epoch=birth["epoch"], birth_operation=mobile["birth"],
                cash=mobile["cash"], cash_revision=mobile["cash_revision"],
                external_proof_required=list(EXTERNAL_PROOF_REQUIRED), retirement_scope="unsupported")
