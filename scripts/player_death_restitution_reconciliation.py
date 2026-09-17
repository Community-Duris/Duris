"""Evidence-only reconciliation rules for legacy artifact identities.

The native artifact code has two independent identity signals:

* ``ITEM_ARTIFACT`` (bit 29) marks an object as an artifact in memory;
* ``IS_UNIQUE`` is a name lookup used to choose the legacy artifact type.

The SQL ``artifacts``/``artifacts_mortal`` rows are the legacy gameplay
tracking authority.  ``artifact_domain_state`` and ``artifact_domain_baseline``
are the newer canonical state/baseline authority.  This module never writes
SQL.  It turns a captured item, current ownership fence, and those authority
snapshots into an auditable decision that the runner can fence in one
transaction.
"""

from __future__ import annotations

from typing import Any

REAL_ARTIFACT_FLAG = 1 << 28  # Native BIT_29 is one-based (268435456U).
ARTIFACT_MAJOR = 1
ARTIFACT_UNIQUE = 2
ARTIFACT_IOUN = 3
LOCATION_ON_PLAYER = 3
LOCATION_ON_CORPSE = 5
LEGACY_UNKNOWN_PLAYER_LOCATION = -2
STATE_DESTROYED = 2
STATE_QUARANTINED = 3


def _int(value: Any, default: int = 0) -> int:
    if value is None or isinstance(value, bool):
        return default
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _name_text(item: dict[str, Any]) -> str:
    value = item.get("name_hex") or ""
    if not isinstance(value, str):
        return ""
    try:
        return bytes.fromhex(value).decode("ascii", "ignore").lower()
    except ValueError:
        return ""


def is_unique_name(item: dict[str, Any]) -> bool:
    """Match native ``IS_UNIQUE`` without treating ``powerunique`` as unique."""

    words = _name_text(item).replace("_", " ").replace("-", " ").split()
    return "unique" in words and "powerunique" not in words


def legacy_rows(artifacts: dict[str, Any], vnum: int) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for table in ("mortal", "god"):
        rows = artifacts.get(table)
        row = rows.get(str(vnum)) if isinstance(rows, dict) else None
        if isinstance(row, dict):
            result[table] = row
    return result


def classify_item(item: dict[str, Any], artifacts: dict[str, Any]) -> dict[str, Any]:
    """Separate native artifact flags, unique naming, and authority tracking."""

    vnum = _int(item.get("vnum"))
    domain = artifacts.get("domain", {}).get(str(vnum))
    rows = legacy_rows(artifacts, vnum)
    real_flag = bool(_int(item.get("extra_flags")) & REAL_ARTIFACT_FLAG)
    unique_name = is_unique_name(item)
    tracked_types = {
        _int(row.get("artifact_type"))
        for row in rows.values()
        if _int(row.get("artifact_type"))
    }
    if isinstance(domain, dict) and _int(domain.get("artifact_type")):
        tracked_types.add(_int(domain.get("artifact_type")))
    authority_required = real_flag or isinstance(domain, dict) or bool(rows)
    # A legacy/canonical row is an artifact authority even when the item has
    # no ITEM_ARTIFACT bit.  Conversely, a name-marked unique with no row is
    # only an ordinary UID item and must not be minted as an artifact.
    if authority_required:
        kind = "artifact"
    elif unique_name:
        kind = "unique"
    else:
        kind = "normal"
    return {
        "kind": kind,
        "real_artifact_flag": real_flag,
        "unique_name": unique_name,
        "unique_tracking": unique_name or ARTIFACT_UNIQUE in tracked_types,
        "authority_required": authority_required,
        "tracked_types": sorted(tracked_types),
        "domain_present": isinstance(domain, dict),
        "legacy_tables": sorted(rows),
    }


def _source_location(location_type: int, location: int, pid: int) -> tuple[bool, bool]:
    """Return (supported, native_unknown_marker).

    ``artifact_update_sql`` writes an alive/dead PC as locType 3 and obtains
    ``-2`` from native ``GET_ID`` when the PC is no longer alive.  locType 2 is
    the NPC path and is never accepted as evidence for a player restoration.
    """

    if location_type == LOCATION_ON_PLAYER and location == pid:
        return True, False
    if location_type == LOCATION_ON_PLAYER and location == LEGACY_UNKNOWN_PLAYER_LOCATION:
        return True, True
    if location_type == LOCATION_ON_CORPSE and location == pid:
        return True, False
    return False, False


def _signature(row: dict[str, Any]) -> tuple[int, int, int, int]:
    return (
        _int(row.get("loc_type")),
        _int(row.get("location")),
        _int(row.get("timer")),
        _int(row.get("artifact_type")),
    )


def _bind_snapshot(artifacts: dict[str, Any], vnum: int) -> tuple[dict[str, Any] | None, int, int]:
    row = artifacts.get("bind", {}).get(str(vnum))
    if not isinstance(row, dict):
        return None, 0, 0
    return row, _int(row.get("owner_pid"), -1), _int(row.get("timer"))


def _competitor_reason(competitors: Any, uid: int) -> str | None:
    if isinstance(competitors, dict):
        rows = competitors.get("rows", [])
    else:
        rows = competitors
    if not isinstance(rows, list):
        return None
    for row in rows:
        if not isinstance(row, dict) or _int(row.get("item_uid")) == uid:
            continue
        # Destruction is an explicit terminal state.  Missing/unknown state
        # remains a live ambiguity and therefore blocks reconciliation.
        if _int(row.get("state"), -1) != STATE_DESTROYED:
            return "another surviving current-owner identity uses this artifact vnum"
    return None


def _result(tracking: dict[str, Any], rows: dict[str, dict[str, Any]], domain: Any,
            baseline: Any, bind: Any) -> dict[str, Any]:
    return {
        "ok": False,
        "classification": "artifact_authority_missing",
        "note": "",
        "reconciliation_required": False,
        "mode": "none",
        "legacy_unknown": False,
        "tracking": tracking,
        "domain_before": domain if isinstance(domain, dict) else None,
        "baseline_before": baseline if isinstance(baseline, dict) else None,
        "bind_before": bind if isinstance(bind, dict) else None,
        "legacy_before": rows,
        "domain_seed": None,
        "baseline_seed": None,
        "timing_status": "missing",
        "loss_epoch": 0,
        "source_timer_epoch": 0,
        "usable_lifetime_seconds": 0,
    }


def reconcile_artifact_authority(
    item: dict[str, Any],
    pid: int,
    current: dict[str, Any],
    artifacts: dict[str, Any],
    competitors: Any = None,
    loss_epoch: int | None = None,
) -> dict[str, Any]:
    """Prove or reject one artifact identity without creating an identity.

    A missing canonical row is recoverable only when both legacy tables agree,
    the payload/current-owner fence identifies one UID, the native Unknown
    marker is tied to that same source PID, no other live UID uses the vnum,
    and the baseline table is available for an additive reconstruction.  The
    returned seeds are evidence snapshots for the SQL runner; they do not by
    themselves authorize a write.
    """

    vnum = _int(item.get("vnum"))
    uid = _int(item.get("object_uid"))
    tracking = classify_item(item, artifacts)
    rows = legacy_rows(artifacts, vnum)
    domain = artifacts.get("domain", {}).get(str(vnum))
    baseline = artifacts.get("baseline", {}).get(str(vnum))
    bind, bind_owner, bind_timer = _bind_snapshot(artifacts, vnum)
    result = _result(tracking, rows, domain, baseline, bind)

    if not tracking["authority_required"]:
        result.update({
            "ok": True,
            "classification": None,
            "note": "UID/current-owner recovery is not artifact-authority recovery",
        })
        return result
    if not rows:
        result["note"] = "no legacy artifacts/artifacts_mortal row proves the runtime artifact identity"
        return result

    if uid <= 0 or _int(current.get("vnum")) != vnum:
        result.update({
            "classification": "artifact_identity_unbound",
            "note": "captured UID and current-owner vnum do not identify one artifact",
        })
        return result
    if (_int(current.get("owner_type")) != 1 or _int(current.get("owner_id")) != pid or
            _int(current.get("owner_context_id")) != 0 or
            _int(current.get("state")) != STATE_QUARANTINED):
        result.update({
            "classification": "artifact_identity_unbound",
            "note": "current ownership is not the quarantined source-player identity",
        })
        return result

    competitor = _competitor_reason(competitors, uid)
    if competitor:
        result.update({"classification": "artifact_competing_instance", "note": competitor})
        return result

    signatures: list[tuple[int, int, int, int]] = []
    legacy_unknown = False
    for name, row in rows.items():
        if str(row.get("owned", "")).upper() != "Y":
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": f"legacy {name} artifact row is not owned",
            })
            return result
        signature = _signature(row)
        supported, unknown = _source_location(signature[0], signature[1], pid)
        if not supported:
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "legacy artifact location is not the source PC/corpse or native Unknown PC marker",
            })
            return result
        if signature[2] < 0:
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "owned legacy artifact has an invalid negative poof epoch",
            })
            return result
        if signature[3] not in {ARTIFACT_MAJOR, ARTIFACT_UNIQUE, ARTIFACT_IOUN}:
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "legacy artifact type is outside the native artifact type set",
            })
            return result
        signatures.append(signature)
        legacy_unknown = legacy_unknown or unknown
    if len(set(signatures)) != 1:
        result.update({
            "classification": "artifact_legacy_conflict",
            "note": "artifacts and artifacts_mortal disagree on location/timer/type",
        })
        return result
    loc_type, location, timer, artifact_type = signatures[0]

    loss_epoch_value = _int(loss_epoch)
    if timer <= 0 or loss_epoch_value <= 0:
        timing_status = "missing"
        usable_lifetime_seconds = 0
    elif timer <= loss_epoch_value:
        timing_status = "expired_at_loss"
        usable_lifetime_seconds = 0
    else:
        timing_status = "historical"
        usable_lifetime_seconds = timer - loss_epoch_value
    result.update({
        "timing_status": timing_status,
        "loss_epoch": loss_epoch_value,
        "source_timer_epoch": timer,
        "usable_lifetime_seconds": usable_lifetime_seconds,
    })

    if tracking["unique_name"] and artifact_type != ARTIFACT_UNIQUE:
        result.update({
            "classification": "artifact_legacy_conflict",
            "note": "native unique-name semantics disagree with the legacy artifact type",
        })
        return result
    if bind is not None and bind_owner not in {pid, 0, -1}:
        result.update({
            "classification": "artifact_binding_conflict",
            "note": "artifact_bind belongs to another player",
        })
        return result

    domain_present = isinstance(domain, dict)
    domain_uid = _int(domain.get("item_uid")) if domain_present else 0
    if domain_present:
        if _int(domain.get("owned")) != 1:
            result.update({
                "classification": "artifact_identity_unbound",
                "note": "canonical artifact state is not owned",
            })
            return result
        if domain_uid not in {0, uid}:
            result.update({
                "classification": "artifact_identity_unbound",
                "note": "canonical artifact state is bound to a different original UID",
            })
            return result
        supported, domain_unknown = _source_location(
            _int(domain.get("loc_type")), _int(domain.get("location")), pid
        )
        if not supported:
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "canonical artifact state has an unsupported source location",
            })
            return result
        legacy_unknown = legacy_unknown or domain_unknown
        if (_int(domain.get("artifact_type")) != artifact_type or
                _int(domain.get("timer_epoch")) != timer):
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "canonical artifact state does not preserve legacy timer/type evidence",
            })
            return result
        if bind is not None:
            if (_int(domain.get("bind_owner_pid")) != bind_owner or
                    _int(domain.get("bind_timer_epoch")) != bind_timer):
                result.update({
                    "classification": "artifact_binding_conflict",
                    "note": "canonical artifact binding differs from artifact_bind",
                })
                return result
        elif (_int(domain.get("bind_timer_epoch")) != 0 or
              _int(domain.get("bind_owner_pid")) not in {0, -1}):
            # With no artifact_bind row, a non-empty canonical binding has no
            # legacy evidence to reproduce and is therefore not guessed.
            result.update({
                "classification": "artifact_binding_conflict",
                "note": "canonical binding exists without retained artifact_bind evidence",
            })
            return result
        if domain_uid == uid and _int(domain.get("item_revision")) != _int(current.get("item_revision")):
            result.update({
                "classification": "artifact_binding_conflict",
                "note": "canonical artifact item revision differs from current ownership",
            })
            return result

    if isinstance(baseline, dict):
        expected_owner = bind_owner if bind is not None else _int(domain.get("bind_owner_pid")) if domain_present else 0
        expected_timer = bind_timer if bind is not None else _int(domain.get("bind_timer_epoch")) if domain_present else 0
        if (_int(baseline.get("opening_timer_epoch")) != timer or
                _int(baseline.get("opening_bind_owner_pid")) != expected_owner or
                _int(baseline.get("opening_bind_timer_epoch")) != expected_timer):
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "canonical artifact baseline disagrees with timer/binding evidence",
            })
            return result
        if not domain_present and _int(baseline.get("opening_revision")) != 0:
            result.update({
                "classification": "artifact_legacy_conflict",
                "note": "missing canonical state has an advanced baseline revision",
            })
            return result

    reconciliation_required = not domain_present or domain_uid == 0
    if reconciliation_required and (
            not artifacts.get("domain_table_present", False) or
            not artifacts.get("baseline_table_present", False) or
            not artifacts.get("bind_table_present", False)):
        result.update({
            "classification": "artifact_baseline_missing",
            "note": "canonical domain state/baseline/binding tables are unavailable for additive identity reconciliation",
        })
        return result

    if not domain_present:
        domain_seed = {
            "vnum": vnum,
            "owned": 1,
            "loc_type": loc_type,
            "location": location,
            "timer_epoch": timer,
            "artifact_type": artifact_type,
            "bind_owner_pid": bind_owner if bind is not None else 0,
            "bind_timer_epoch": bind_timer if bind is not None else 0,
            "item_uid": uid,
            "item_revision": _int(current.get("item_revision")),
            "revision": 0,
        }
    else:
        domain_seed = dict(domain)
        domain_seed.update({
            "item_uid": uid,
            "item_revision": _int(current.get("item_revision")),
            "artifact_type": artifact_type,
            "timer_epoch": timer,
        })
    baseline_seed = {
        "vnum": vnum,
        "opening_timer_epoch": timer,
        "opening_bind_owner_pid": (
            bind_owner if bind is not None else _int(domain.get("bind_owner_pid")) if domain_present else 0
        ),
        "opening_bind_timer_epoch": (
            bind_timer if bind is not None else _int(domain.get("bind_timer_epoch")) if domain_present else 0
        ),
        "opening_revision": _int(baseline.get("opening_revision")) if isinstance(baseline, dict) else 0,
    }
    mode = "missing_domain" if not domain_present else "unbound_domain"
    result.update({
        "ok": True,
        "classification": None,
        "reconciliation_required": reconciliation_required,
        "mode": mode if reconciliation_required else "none",
        "legacy_unknown": legacy_unknown,
        "note": (
            "native locType=3/location=-2 Unknown marker is tied to the source PID; "
            "exact payload/current custody and global competitor fence justify additive canonical recovery"
            if legacy_unknown else
            "exact payload/current custody and global competitor fence justify additive canonical recovery"
        ) if reconciliation_required else "canonical state, baseline, and legacy tracking agree",
        "domain_seed": domain_seed,
        "baseline_seed": baseline_seed,
    })
    return result


def final_domain_expectation(candidate: dict[str, Any], recipient_pid: int) -> dict[str, Any]:
    """Return the post-apply canonical state expected by the verifier."""

    reconciliation = candidate.get("artifact_reconciliation") or {}
    source = candidate.get("artifact_before") or reconciliation.get("domain_seed")
    current = candidate.get("expected_current")
    if not isinstance(source, dict) or not isinstance(current, dict):
        raise ValueError("artifact candidate has incomplete canonical/current evidence")
    timing = candidate.get("artifact_timing") or {}
    return {
        "vnum": _int(candidate.get("vnum")),
        "owned": 1,
        "loc_type": LOCATION_ON_PLAYER,
        "location": int(recipient_pid),
        # The absolute deadline is assigned once inside the apply transaction.
        # The plan records the lifetime, never the stale source deadline.
        "timer_epoch": None,
        "timer_epoch_mode": "delivery_epoch_plus_usable_lifetime",
        "usable_lifetime_seconds": _int(timing.get("usable_lifetime_seconds")),
        "artifact_type": _int(source.get("artifact_type")),
        "bind_owner_pid": _int(source.get("bind_owner_pid")),
        "bind_timer_epoch": _int(source.get("bind_timer_epoch")),
        "item_uid": _int(candidate.get("item_uid")),
        "item_revision": _int(current.get("item_revision")) + 1,
        "revision": _int(source.get("revision")) + 1,
    }
