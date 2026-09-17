#!/usr/bin/env python3
"""Evaluate an approved shadow proposal at a safe config boundary.

This is a pure decision/recording contract.  It returns an application intent
and audit record but performs no write, property mutation, SQL operation, or
gameplay call.  A future authorized config owner may consume the intent after
the separate production-approval gate is satisfied.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys
from typing import Any, Iterable, Mapping, Sequence

try:  # Running as a package.
    from .recommend import (
        RecommendationInputError,
        TARGET_MAX_MILLI,
        TARGET_MIN_MILLI,
        TARGET_PARAMETER,
        _boolean,
        _integer,
        _mapping,
        _parse_utc,
        _reject_forbidden_fields,
        _string,
        _token,
    )
except ImportError:  # Running scripts/telemetry/balance/application.py directly.
    from recommend import (  # type: ignore[no-redef]
        RecommendationInputError,
        TARGET_MAX_MILLI,
        TARGET_MIN_MILLI,
        TARGET_PARAMETER,
        _boolean,
        _integer,
        _mapping,
        _parse_utc,
        _reject_forbidden_fields,
        _string,
        _token,
    )


SCHEMA_VERSION = 1
MAX_HISTORY = 64
MAX_INPUT_BYTES = 16 * 1024 * 1024
MAX_ID_LENGTH = 128
ACTION_STATUSES = frozenset({"accepted", "rolled_back"})


class ApplicationInputError(ValueError):
    """The application/rollback command is malformed."""


def _dedupe(values: Iterable[str]) -> list[str]:
    return list(dict.fromkeys(values))


def _digest(value: Any) -> str:
    canonical = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def _read_token(raw: Mapping[str, Any], name: str, reasons: list[str]) -> str | None:
    value = raw.get(name)
    try:
        return _token(value, name)
    except RecommendationInputError:
        reasons.append(f"invalid_{name}")
        return None


def _parse_proposal(raw_proposal: Any) -> tuple[dict[str, Any], list[str]]:
    proposal = _mapping(raw_proposal, "proposal")
    reasons: list[str] = []
    recommendation_id = _read_token(proposal, "recommendation_id", reasons)
    policy_version = proposal.get("policy_version")
    if policy_version != "balance-shadow-v1":
        reasons.append("unsupported_policy_version")
    if proposal.get("status") != "recommend":
        reasons.append("proposal_not_recommendable")
    target = _mapping(_required(proposal, "target"), "proposal.target")
    if target.get("parameter") != TARGET_PARAMETER:
        reasons.append("proposal_target_mismatch")
    current_value = target.get("current_value_milli")
    proposed_value = target.get("proposed_value_milli")
    for name, value in (("proposal_current_value", current_value), ("proposal_value", proposed_value)):
        if (
            isinstance(value, bool)
            or not isinstance(value, int)
            or not TARGET_MIN_MILLI <= value <= TARGET_MAX_MILLI
        ):
            reasons.append(f"{name}_out_of_bounds")
    if isinstance(current_value, int) and isinstance(proposed_value, int) and current_value == proposed_value:
        reasons.append("proposal_has_no_change")
    target_config_generation = target.get("config_generation")
    if not isinstance(target_config_generation, str):
        reasons.append("proposal_target_config_generation_missing")
    input_reference = _mapping(
        _required(proposal, "input_reference"), "proposal.input_reference"
    )
    report_generation = _integer(
        _required(input_reference, "generation"),
        "proposal.input_reference.generation",
        minimum=1,
    )
    config_generation = _token(
        _required(input_reference, "config_generation"),
        "proposal.input_reference.config_generation",
    )
    if target_config_generation != config_generation:
        reasons.append("proposal_target_config_generation_mismatch")
    expires_at = _parse_utc(
        _required(input_reference, "expires_at_utc"),
        "proposal.input_reference.expires_at_utc",
    )
    evidence = _mapping(_required(proposal, "evidence"), "proposal.evidence")
    evidence_fingerprint = _read_token(evidence, "fingerprint", reasons)
    constraints = _mapping(_required(proposal, "constraints"), "proposal.constraints")
    if constraints.get("shadow_only") is not True:
        reasons.append("proposal_not_shadow_only")
    if constraints.get("can_apply") is not False:
        reasons.append("proposal_application_capability_mismatch")
    if constraints.get("gameplay_mutation") is not False:
        reasons.append("proposal_gameplay_capability_mismatch")
    if constraints.get("automatic_balance_mutation") is not False:
        reasons.append("proposal_automatic_capability_mismatch")
    return (
        {
            "recommendation_id": recommendation_id,
            "policy_version": "balance-shadow-v1",
            "report_generation": report_generation,
            "config_generation": config_generation,
            "expires_at_utc": expires_at.isoformat().replace("+00:00", "Z"),
            "expires_at": expires_at,
            "evidence_fingerprint": evidence_fingerprint,
            "current_value_milli": current_value
            if isinstance(current_value, int) and TARGET_MIN_MILLI <= current_value <= TARGET_MAX_MILLI
            else None,
            "proposed_value_milli": proposed_value
            if isinstance(proposed_value, int) and TARGET_MIN_MILLI <= proposed_value <= TARGET_MAX_MILLI
            else None,
        },
        _dedupe(reasons),
    )


def _required(mapping: Mapping[str, Any], name: str) -> Any:
    if name not in mapping:
        raise ApplicationInputError(f"missing required field: {name}")
    return mapping[name]


def _parse_config(raw_config: Any, name: str) -> dict[str, Any]:
    config = _mapping(raw_config, name)
    parameter = _string(_required(config, "parameter"), f"{name}.parameter", max_length=128)
    if parameter != TARGET_PARAMETER:
        raise ApplicationInputError(f"{name}.parameter is outside the owned target")
    generation = _token(_required(config, "generation"), f"{name}.generation")
    revision = _integer(_required(config, "revision"), f"{name}.revision", minimum=0)
    value_milli = _integer(
        _required(config, "value_milli"),
        f"{name}.value_milli",
        minimum=TARGET_MIN_MILLI,
        maximum=TARGET_MAX_MILLI,
    )
    return {
        "parameter": parameter,
        "generation": generation,
        "revision": revision,
        "value_milli": value_milli,
    }


def _parse_approval(raw_approval: Any, *, policy_version: str, now: datetime) -> dict[str, Any]:
    approval = _mapping(raw_approval, "approval")
    if _boolean(_required(approval, "approved"), "approval.approved") is not True:
        raise ApplicationInputError("approval.approved must be true")
    actor_token = _token(_required(approval, "actor_token"), "approval.actor_token")
    action_id = _token(_required(approval, "action_id"), "approval.action_id")
    experiment_id = _token(_required(approval, "experiment_id"), "approval.experiment_id")
    if _string(
        _required(approval, "reviewed_policy_version"),
        "approval.reviewed_policy_version",
        max_length=64,
    ) != policy_version:
        raise ApplicationInputError("approval policy version does not match the proposal")
    approved_at = _parse_utc(_required(approval, "approved_at_utc"), "approval.approved_at_utc")
    if approved_at > now:
        raise ApplicationInputError("approval cannot be from the future")
    return {
        "actor_token": actor_token,
        "action_id": action_id,
        "experiment_id": experiment_id,
        "reviewed_policy_version": policy_version,
        "approved_at_utc": approved_at.isoformat().replace("+00:00", "Z"),
    }


def _parse_history(raw_history: Any) -> list[dict[str, Any]]:
    if not isinstance(raw_history, list) or len(raw_history) > MAX_HISTORY:
        raise ApplicationInputError(f"history must be a list of at most {MAX_HISTORY} records")
    normalized: list[dict[str, Any]] = []
    seen_actions: set[str] = set()
    for index, raw in enumerate(raw_history):
        record = _mapping(raw, f"history[{index}]")
        action_id = _token(_required(record, "action_id"), f"history[{index}].action_id")
        if action_id in seen_actions:
            raise ApplicationInputError("history contains a duplicate action_id")
        seen_actions.add(action_id)
        status = _string(_required(record, "status"), f"history[{index}].status", max_length=32)
        if status not in ACTION_STATUSES:
            raise ApplicationInputError(f"history[{index}].status is not a recognized action status")
        normalized.append(
            {
                "action_id": action_id,
                "recommendation_id": _token(
                    _required(record, "recommendation_id"),
                    f"history[{index}].recommendation_id",
                ),
                "operation": _string(
                    _required(record, "operation"), f"history[{index}].operation", max_length=16
                ),
                "status": status,
                "parameter": _string(
                    _required(record, "parameter"), f"history[{index}].parameter", max_length=128
                ),
                "previous_value_milli": _integer(
                    _required(record, "previous_value_milli"),
                    f"history[{index}].previous_value_milli",
                    minimum=TARGET_MIN_MILLI,
                    maximum=TARGET_MAX_MILLI,
                ),
                "applied_value_milli": _integer(
                    _required(record, "applied_value_milli"),
                    f"history[{index}].applied_value_milli",
                    minimum=TARGET_MIN_MILLI,
                    maximum=TARGET_MAX_MILLI,
                ),
                "config_generation": _token(
                    _required(record, "config_generation"),
                    f"history[{index}].config_generation",
                ),
                "config_revision": _integer(
                    _required(record, "config_revision"),
                    f"history[{index}].config_revision",
                    minimum=0,
                ),
                "policy_version": _string(
                    _required(record, "policy_version"),
                    f"history[{index}].policy_version",
                    max_length=64,
                ),
                "report_generation": _integer(
                    _required(record, "report_generation"),
                    f"history[{index}].report_generation",
                    minimum=1,
                ),
                "evidence_fingerprint": _token(
                    _required(record, "evidence_fingerprint"),
                    f"history[{index}].evidence_fingerprint",
                ),
                "actor_token": _token(
                    _required(record, "actor_token"), f"history[{index}].actor_token"
                ),
                "experiment_id": _token(
                    _required(record, "experiment_id"), f"history[{index}].experiment_id"
                ),
                "issued_at_utc": _parse_utc(
                    _required(record, "issued_at_utc"), f"history[{index}].issued_at_utc"
                ).isoformat().replace("+00:00", "Z"),
                "expires_at_utc": _parse_utc(
                    _required(record, "expires_at_utc"), f"history[{index}].expires_at_utc"
                ).isoformat().replace("+00:00", "Z"),
                "command_fingerprint": _token(
                    _required(record, "command_fingerprint"),
                    f"history[{index}].command_fingerprint",
                ),
            }
        )
    return normalized


def _result_base(
    *,
    status: str,
    reason_codes: Sequence[str],
    proposal: Mapping[str, Any],
    current_config: Mapping[str, Any],
    action_id: str | None,
) -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "status": status,
        "applied": False,
        "reason_codes": _dedupe(reason_codes),
        "action_id": action_id,
        "recommendation_id": proposal.get("recommendation_id"),
        "target": TARGET_PARAMETER,
        "previous_value_milli": current_config.get("value_milli"),
        "next_config": dict(current_config),
        "fallback_value_milli": current_config.get("value_milli"),
        "audit_record": None,
        "constraints": {
            "safe_boundary_intent_only": True,
            "writes_performed": 0,
            "gameplay_mutation": False,
            "production_activation": False,
            "sql_or_network": False,
        },
    }


def _audit_record(
    *,
    operation: str,
    approval: Mapping[str, Any],
    proposal: Mapping[str, Any],
    previous_value: int,
    applied_value: int,
    config: Mapping[str, Any],
    issued_at: str,
    command_fingerprint: str,
) -> dict[str, Any]:
    return {
        "action_id": approval["action_id"],
        "recommendation_id": proposal["recommendation_id"],
        "operation": operation,
        "status": "accepted",
        "parameter": TARGET_PARAMETER,
        "previous_value_milli": previous_value,
        "applied_value_milli": applied_value,
        "config_generation": config["generation"],
        "config_revision": config["revision"] + 1,
        "policy_version": proposal["policy_version"],
        "report_generation": proposal["report_generation"],
        "evidence_fingerprint": proposal["evidence_fingerprint"],
        "actor_token": approval["actor_token"],
        "experiment_id": approval["experiment_id"],
        "issued_at_utc": issued_at,
        "expires_at_utc": proposal["expires_at_utc"],
        "command_fingerprint": command_fingerprint,
    }


def evaluate_command(payload: Mapping[str, Any]) -> dict[str, Any]:
    """Evaluate an apply or rollback command without performing a write."""

    try:
        _reject_forbidden_fields(payload)
    except RecommendationInputError as exc:
        raise ApplicationInputError(str(exc)) from exc
    if _integer(_required(payload, "schema_version"), "schema_version", minimum=0) != SCHEMA_VERSION:
        raise ApplicationInputError(f"schema_version must be {SCHEMA_VERSION}")
    operation = _string(_required(payload, "operation"), "operation", max_length=16)
    if operation not in {"apply", "rollback"}:
        raise ApplicationInputError("operation must be apply or rollback")
    now = _parse_utc(_required(payload, "now_utc"), "now_utc")
    proposal, proposal_reasons = _parse_proposal(_required(payload, "proposal"))
    current_config = _parse_config(_required(payload, "current_config"), "current_config")
    expected_config = _parse_config(_required(payload, "expected_config"), "expected_config")
    approval = _parse_approval(
        _required(payload, "approval"), policy_version=proposal["policy_version"], now=now
    )
    history = _parse_history(_required(payload, "history"))
    controls = _mapping(_required(payload, "controls"), "controls")
    kill_switch = _boolean(_required(controls, "kill_switch"), "controls.kill_switch")
    application_enabled = _boolean(
        _required(controls, "application_enabled"), "controls.application_enabled"
    )
    rollback_of_action_id = payload.get("rollback_of_action_id")
    if rollback_of_action_id is not None:
        rollback_of_action_id = _token(rollback_of_action_id, "rollback_of_action_id")
    if operation == "rollback" and rollback_of_action_id is None:
        raise ApplicationInputError("rollback_of_action_id is required for rollback")
    if operation == "apply" and rollback_of_action_id is not None:
        raise ApplicationInputError("rollback_of_action_id is only valid for rollback")

    command_basis = {
        "operation": operation,
        "action_id": approval["action_id"],
        "recommendation_id": proposal["recommendation_id"],
        "proposal": {
            "policy_version": proposal["policy_version"],
            "report_generation": proposal["report_generation"],
            "config_generation": proposal["config_generation"],
            "evidence_fingerprint": proposal["evidence_fingerprint"],
            "current_value_milli": proposal["current_value_milli"],
            "proposed_value_milli": proposal["proposed_value_milli"],
            "expires_at_utc": proposal["expires_at_utc"],
        },
        "expected_config": expected_config,
        "rollback_of_action_id": rollback_of_action_id,
    }
    command_fingerprint = _digest(command_basis)
    prior = next(
        (record for record in history if record["action_id"] == approval["action_id"]),
        None,
    )
    if prior is not None:
        if prior["command_fingerprint"] != command_fingerprint:
            result = _result_base(
                status="rejected",
                reason_codes=["action_id_conflict"],
                proposal=proposal,
                current_config=current_config,
                action_id=approval["action_id"],
            )
            result["constraints"]["kill_switch"] = kill_switch
            return result
        result = _result_base(
            status="idempotent_replay",
            reason_codes=[],
            proposal=proposal,
            current_config=current_config,
            action_id=approval["action_id"],
        )
        result["audit_record"] = prior
        result["constraints"]["kill_switch"] = kill_switch
        return result

    reasons = list(proposal_reasons)
    if kill_switch:
        reasons.append("kill_switch_active")
    if not application_enabled:
        reasons.append("application_disabled")
    if current_config != {
        "parameter": expected_config["parameter"],
        "generation": expected_config["generation"],
        "revision": expected_config["revision"],
        "value_milli": expected_config["value_milli"],
    }:
        reasons.append("stale_config")
    if proposal["config_generation"] != expected_config["generation"]:
        reasons.append("proposal_config_generation_mismatch")
    if operation == "apply" and now >= proposal["expires_at"]:
        reasons.append("proposal_expired")
    if operation == "apply" and proposal["current_value_milli"] != expected_config["value_milli"]:
        reasons.append("proposal_current_value_mismatch")

    source_record = None
    if operation == "rollback" and rollback_of_action_id is not None:
        source_record = next(
            (
                record
                for record in history
                if record["action_id"] == rollback_of_action_id
                and record["operation"] == "apply"
                and record["status"] == "accepted"
            ),
            None,
        )
        if source_record is None:
            reasons.append("rollback_source_missing")
        elif source_record["applied_value_milli"] != expected_config["value_milli"]:
            reasons.append("rollback_source_not_current")

    result = _result_base(
        status="rejected" if reasons else "accepted_for_safe_boundary",
        reason_codes=reasons,
        proposal=proposal,
        current_config=current_config,
        action_id=approval["action_id"],
    )
    result["constraints"]["kill_switch"] = kill_switch
    result["constraints"]["application_enabled"] = application_enabled
    if reasons:
        return result

    if operation == "apply":
        previous_value = current_config["value_milli"]
        applied_value = proposal["proposed_value_milli"]
    else:
        assert source_record is not None
        previous_value = current_config["value_milli"]
        applied_value = source_record["previous_value_milli"]
    next_config = {
        **current_config,
        "revision": current_config["revision"] + 1,
        "value_milli": applied_value,
    }
    audit = _audit_record(
        operation=operation,
        approval=approval,
        proposal=proposal,
        previous_value=previous_value,
        applied_value=applied_value,
        config=current_config,
        issued_at=now.isoformat().replace("+00:00", "Z"),
        command_fingerprint=command_fingerprint,
    )
    result["applied"] = False
    result["previous_value_milli"] = previous_value
    result["next_config"] = next_config
    result["fallback_value_milli"] = previous_value
    result["audit_record"] = audit
    result["application_intent"] = {
        "operation": operation,
        "parameter": TARGET_PARAMETER,
        "value_milli": applied_value,
        "expected_generation": current_config["generation"],
        "expected_revision": current_config["revision"],
        "next_revision": next_config["revision"],
        "requires_config_owner_commit": True,
    }
    return result


def _read_input(path: Path) -> Mapping[str, Any]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise ApplicationInputError(f"cannot read input: {path}") from exc
    if len(data) > MAX_INPUT_BYTES:
        raise ApplicationInputError(f"input exceeds hard bound of {MAX_INPUT_BYTES} bytes")
    try:
        parsed = json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ApplicationInputError("input must be UTF-8 JSON") from exc
    return _mapping(parsed, "input")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="administrator command JSON")
    parser.add_argument("--output", type=Path, help="write the safe-boundary result JSON here")
    args = parser.parse_args(argv)
    try:
        result = evaluate_command(_read_input(args.input))
    except (OSError, ApplicationInputError, RecommendationInputError) as exc:
        print(f"balance-application input error: {exc}", file=sys.stderr)
        return 2
    serialized = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        sys.stdout.write(serialized)
    else:
        try:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(serialized, encoding="utf-8", newline="\n")
        except OSError as exc:
            print(f"balance-application output error: {exc}", file=sys.stderr)
            return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
