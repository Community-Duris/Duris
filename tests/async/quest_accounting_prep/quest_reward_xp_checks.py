"""Optional captured group-XP tuple agreement, never native/save/ACK authority."""

import json

from capture_quest_cut import MAX_ROWS
import quest_cut_checks as checks

# Published continuation arrays contain at most64 rewards and64 XP awards.
# Exported amounts are positive and bounded by the decoder's signed-int reward.
MAX_AWARDS = 64
MAX_SLOT = 63
MAX_AMOUNT = 2**31 - 1
BIND_FIELDS = ("case", "source_commit", "binary_sha256", "schema_manifest_sha256",
               "lineage", "epoch", "mobile_instance_ids", "watched_vnums")
EXTERNAL_PROOF_REQUIRED = (
    "maintained decoder execution on the selected original literal; JSON alone is unauthenticated",
    "actual original operation, participant/runtime and NPC birth/source/custody authority",
    "externally proven coherent multi-recipient snapshot or schedule for any cross-cut ACK implication",
    "actual effective XP, exact receipt/save/ACK/replay/cold and original pair retirement evidence",
)


def _uint(value, bits, *, positive=False):
    return type(value) is int and (1 if positive else 0) <= value < 2**bits


def _operation(value):
    checks.require(type(value) is str and len(value) == 32 and
                   all(c in "0123456789abcdef" for c in value) and int(value, 16) != 0,
                   "exact positive lowercase original operation hex required")
    return value


def _flag(value):
    checks.require(type(value) is int and value in (0, 1), "captured SQL flag must be integer0/1")
    return value


def _literal(value):
    checks.require(type(value) is str and 0 < len(value) <= 2 * 8192 and len(value) % 2 == 0 and
                   all(c in "0123456789abcdef" for c in value),
                   "bounded captured original continuation hex required")


def _selected(cut, table, operation):
    entries = checks.rows(cut, table)
    checks.require(len(entries) <= MAX_ROWS, "captured table row bound exceeded")
    selected = []
    for entry in entries:
        checks.require(type(entry) is dict, "captured table must contain row objects")
        # Only operation identity is needed to classify unrelated operations.
        # Their remaining columns and effects are outside this optional check.
        if _operation(entry["offering_operation_id"]) == operation:
            selected.append(entry)
    return selected


def _player(cut):
    player = checks.row(cut, "player")
    pid = player["pid"]
    checks.require(_uint(pid, 32, positive=True) and
                   type(cut["meta"]["pid"]) is int and cut["meta"]["pid"] == pid,
                   "selected SQL player PID and metadata must agree exactly")
    return pid


def _bind(owner, cut, legacy):
    # Per-PID captures necessarily select different players. Validate those PIDs
    # first, then reuse maintained binding for every other existing dimension.
    normalized = dict(cut, meta=dict(cut["meta"], pid=owner["meta"]["pid"]))
    checks.bind(owner, normalized, owner["meta"]["case"], legacy=legacy)
    checks.require(cut["meta"].get("authority") == ("legacy-no-epoch" if legacy else "native"),
                   "capture authority label disagrees with selected mode")
    for name in BIND_FIELDS:
        checks.require(json.dumps(owner["meta"][name], sort_keys=True, allow_nan=False) ==
                       json.dumps(cut["meta"][name], sort_keys=True, allow_nan=False),
                       "typed candidate/capture binding changed: " + name)
    checks.require(json.dumps(checks.rows(owner, "migrations"), sort_keys=True, allow_nan=False) ==
                   json.dumps(checks.rows(cut, "migrations"), sort_keys=True, allow_nan=False),
                   "typed migration observations changed")


def _awards(terms):
    checks.require(type(terms) is dict and set(terms) ==
                   {"version", "pid", "mobile_vnum", "level", "party_size", "xp_awards"},
                   "full maintained decoder export fields required")
    checks.require(type(terms["version"]) is int and terms["version"] in (5, 6),
                   "group entitlement agreement requires exported version5/6")
    checks.require(_uint(terms["pid"], 32, positive=True) and
                   _uint(terms["mobile_vnum"], 31, positive=True) and
                   _uint(terms["level"], 31) and
                   type(terms["party_size"]) is int and 2 <= terms["party_size"] <= MAX_AWARDS,
                   "bounded exported group identity/level required")
    awards = terms["xp_awards"]
    checks.require(type(awards) is list and 2 <= len(awards) <= MAX_AWARDS,
                   "bounded nonempty full group award list required")
    expected = {}
    for award in awards:
        checks.require(type(award) is dict and set(award) == {"pid", "index", "amount"},
                       "full exported XP tuple required")
        pid, slot, amount = award["pid"], award["index"], award["amount"]
        checks.require(_uint(pid, 32, positive=True) and type(slot) is int and 0 <= slot <= MAX_SLOT and
                       type(amount) is int and 1 <= amount <= MAX_AMOUNT,
                       "invalid exported XP PID/slot/amount domain")
        key = (pid, slot)
        checks.require(key not in expected, "duplicate/conflicting exported XP tuple")
        expected[key] = amount
    pids = {pid for pid, _ in expected}
    slots = {slot for _, slot in expected}
    # This is a consequence of the maintained v5/v6 decoder: party_size equals
    # credited_count and every XP slot has exactly one award for every credited
    # PID. No credited_count field is invented in the smaller JSON export.
    checks.require(terms["pid"] in pids and len(pids) == terms["party_size"] and
                   set(expected) == {(pid, slot) for pid in pids for slot in slots},
                   "exported full group/slot coverage disagrees")
    return expected, pids, slots


def assert_frozen_xp_agreement(original_operation, decoded_terms, owner_cut, recipient_cuts, *, legacy=False):
    """Compare explicit v5/v6 group exports with owner and all peer PID cuts.

    Caller obtains decoded_terms by passing the selected obligation literal to
    existing legacy_xp.decode (maintained read_quest_continuation.cpp adapter).
    This pure helper does not execute/replace the decoder or authenticate that
    pairing. recipient_cuts is a list of all peers, excluding owner_cut.
    Only same-owner-cut mask/ACK implications are checked. Different cuts have
    no asserted common time, even if caller metadata claims coherence.
    """
    try:
        return _agreement(original_operation, decoded_terms, owner_cut, recipient_cuts, legacy)
    except (KeyError, TypeError, ValueError, IndexError, OverflowError) as error:
        raise checks.CutError("captured group XP agreement refused: " + str(error)) from error


def _agreement(operation, terms, owner, peers, legacy):
    checks.require(type(legacy) is bool, "legacy must be explicit boolean")
    operation = _operation(operation)
    expected, pids, slots = _awards(terms)
    owner_pid = _player(owner)
    checks.require(owner_pid == terms["pid"], "decoded owner disagrees with selected owner PID")
    checks.require(type(peers) is list and len(peers) == len(pids) - 1,
                   "one explicit cut for every peer PID required")
    cuts = [owner, *peers]
    observed = {}
    for cut in cuts:
        pid = _player(cut)
        checks.require(pid in pids and pid not in observed, "foreign/duplicate recipient cut")
        _bind(owner, cut, legacy)
        obligations = _selected(cut, "obligations", operation)
        if pid == owner_pid:
            checks.require(len(obligations) == 1, "one original owner obligation required")
            obligation = obligations[0]
            checks.require(type(obligation["player_pid"]) is int and obligation["player_pid"] == pid,
                           "original obligation owner disagrees")
            _literal(obligation["continuation"])
            mask, acknowledged = obligation["xp_applied_mask"], _flag(obligation["acknowledged"])
            checks.require(_uint(mask, 64), "original XP mask must be unsigned64")
        else:
            checks.require(not obligations, "peer-selected cut contains foreign original obligation")
        entitlements = _selected(cut, "xp_entitlements", operation)
        local = {}
        for entry in entitlements:
            recipient, slot, amount = entry["recipient_pid"], entry["reward_index"], entry["amount"]
            checks.require(_uint(recipient, 32, positive=True) and recipient == pid and
                           type(slot) is int and 0 <= slot <= MAX_SLOT and
                           type(amount) is int and 1 <= amount <= MAX_AMOUNT,
                           "invalid/foreign selected-original entitlement tuple")
            checks.require(slot not in local and expected.get((recipient, slot)) == amount,
                           "duplicate/extra/conflicting selected-original entitlement")
            local[slot] = _flag(entry["applied"])
        checks.require(set(local) == slots, "missing selected-original recipient XP evidence")
        observed[pid] = local
    checks.require(set(observed) == pids, "missing captured recipient")
    required_mask = sum(1 << slot for slot in slots)
    applied_owner_mask = sum(1 << slot for slot, applied in observed[owner_pid].items() if applied)
    checks.require(mask == applied_owner_mask, "owner-cut XP mask disagrees with local entitlements")
    checks.require(not acknowledged or mask == required_mask,
                   "owner-cut ACK lacks all local owner XP applications")
    return dict(scope="captured frozen XP entitlement agreement only", original_operation=operation,
                owner_pid=owner_pid, version=terms["version"],
                expected_awards=[dict(pid=pid, index=slot, amount=amount)
                                 for (pid, slot), amount in sorted(expected.items())],
                observations=[dict(pid=pid, applications=[dict(index=slot, applied=applied)
                              for slot, applied in sorted(local.items())])
                              for pid, local in sorted(observed.items())],
                observed_owner_mask=mask, observed_owner_acknowledged=acknowledged,
                owner_authenticated=False, decoded_terms_authenticated=False,
                common_time_snapshot_proven=False, ack_order_proven=False,
                effective_xp_proven=False, save_completion_proven=False,
                native_journey_proven=False, retirement_proven=False,
                external_proof_required=list(EXTERNAL_PROOF_REQUIRED))
