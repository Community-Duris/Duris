"""Optional captured replacement-task agreement; no native or financial proof."""

import json

from capture_quest_cut import TASK_COLUMNS
import quest_cut_checks as checks

EXTERNAL_PROOF_REQUIRED = (
    "authentic command ordering, real map availability, positive-share configuration, donor/consent/quota/history eligibility and original operation delay/release",
    "original request/attempt, player PID/runtime lifetime and native giver birth/source/custody authentication",
    "original debit/receipt linkage and exact once-only restitution or a retained durable unresolved obligation",
    "actual physical publication, observed item/affect custody, GMCP, task/history/XP save, hold/ACK/replay/cold and retirement evidence",
)


def _signed(value, bits):
    return type(value) is int and -(2**(bits - 1)) <= value < 2**(bits - 1)


def _player(cut):
    player = checks.row(cut, "player")
    pid = player["pid"]
    checks.require(_signed(pid, 32) and pid > 0 and
                   type(cut["meta"]["pid"]) is int and pid == cut["meta"]["pid"],
                   "one exact observed native player PID required")
    checks.require(all(_signed(player[name], 32) for name in TASK_COLUMNS),
                   "complete task fields must be signed int32 values")
    checks.require(_signed(player["exp"], 64), "captured SQL XP must be signed bigint")
    return player


def assert_replacement_task_stable(request, reset, replacement, completed, *, legacy=False):
    """Compare caller-supplied QP07 A/reset/B-before/B-after captured cuts.

    All fourteen task columns are required. Only B-before/B-after task, XP and
    literal captured history stability is asserted; reset/share may change A.
    Wallets, receipts, items and affects are deliberately outside this result.
    No caller opts in automatically. Inputs are never changed. Raises CutError
    for missing/malformed/inconsistent observations, not for financial changes.
    """
    try:
        return _stable(request, reset, replacement, completed, legacy)
    except (KeyError, TypeError, ValueError, IndexError, OverflowError) as error:
        raise checks.CutError("replacement task captured agreement refused: " + str(error)) from error


def _stable(request, reset, replacement, completed, legacy):
    checks.require(type(legacy) is bool, "legacy must be an explicit boolean")
    cuts = (request, reset, replacement, completed)
    players = [_player(cut) for cut in cuts]
    for cut in cuts:
        checks.bind(request, cut, "QP07", legacy=legacy)
        checks.require(cut["meta"].get("authority") == ("legacy-no-epoch" if legacy else "native"),
                       "capture authority label disagrees with selected mode")
    original, cleared, before, after = players
    # int32 is the storage domain, not a universal nonnegative/task-validity rule.
    # These narrower conditions describe this explicitly selected stale-map slice.
    for player in (original, before):
        checks.require(player["quest_active"] == 1 and player["quest_accomplished"] == 0 and
                       player["quest_type"] in (1, 2) and player["quest_map_bought"] == 0,
                       "unfinished active original and replacement map tasks required")
    started = original["quest_started"]
    checks.require(started > 0, "positive original attempt watermark required")
    checks.require(cleared["quest_started"] == started, "reset lost original attempt watermark")
    checks.require(all(cleared[name] == (-1 if name == "quest_zone_number" else 0)
                       for name in TASK_COLUMNS if name != "quest_started"),
                   "post-reset task does not match source reset fields")
    checks.require(before["quest_started"] > started, "replacement attempt must strictly advance")
    checks.require(before["quest_map_room"] == 0, "shared replacement must retain reset map state")
    checks.require(all(before[name] == after[name] for name in TASK_COLUMNS),
                   "original completion changed replacement task fields")
    checks.require(before["exp"] == after["exp"], "original completion changed replacement XP")
    histories = [checks.rows(cut, "history") for cut in (replacement, completed)]
    checks.require(all(isinstance(entry, dict) for history in histories for entry in history),
                   "captured history must contain row objects")
    # Literal JSON distinguishes e.g. integer1 from bool true or float1.0 while
    # allowing dictionary key order to differ. Capture already emits JSON rows.
    checks.require(json.dumps(histories[0], sort_keys=True, allow_nan=False) ==
                   json.dumps(histories[1], sort_keys=True, allow_nan=False),
                   "original completion changed replacement history")
    return dict(scope="captured replacement task/history/XP agreement only",
                pid=original["pid"], original_attempt=started,
                replacement_attempt=before["quest_started"], task_columns=list(TASK_COLUMNS),
                owner_authenticated=False, native_journey_proven=False,
                financial_restitution_proven=False,
                external_proof_required=list(EXTERNAL_PROOF_REQUIRED))
