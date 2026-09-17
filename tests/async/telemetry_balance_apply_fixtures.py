"""Deterministic proposal/config fixtures for #275."""
from __future__ import annotations

from copy import deepcopy


def recommendation() -> dict:
    return {
        "schema_version": 1,
        "policy_version": "balance-shadow-v1",
        "status": "recommend",
        "recommendation_id": "shadow-recommendation-001",
        "target": {
            "parameter": "payout.epic.zone.alignmentMod",
            "config_generation": "cfg-v1",
            "current_value_milli": 200,
            "proposed_value_milli": 250,
        },
        "input_reference": {
            "generation": 42,
            "config_generation": "cfg-v1",
            "expires_at_utc": "2026-02-14T00:00:00Z",
        },
        "evidence": {"fingerprint": "e" * 64},
        "constraints": {
            "shadow_only": True,
            "can_apply": False,
            "gameplay_mutation": False,
            "automatic_balance_mutation": False,
        },
    }


def apply_command() -> dict:
    return {
        "schema_version": 1,
        "operation": "apply",
        "now_utc": "2026-02-02T00:00:00Z",
        "proposal": recommendation(),
        "current_config": {
            "parameter": "payout.epic.zone.alignmentMod",
            "generation": "cfg-v1",
            "revision": 7,
            "value_milli": 200,
        },
        "expected_config": {
            "parameter": "payout.epic.zone.alignmentMod",
            "generation": "cfg-v1",
            "revision": 7,
            "value_milli": 200,
        },
        "approval": {
            "approved": True,
            "actor_token": "admin-reviewer-1",
            "action_id": "action-apply-1",
            "experiment_id": "experiment-275-1",
            "reviewed_policy_version": "balance-shadow-v1",
            "approved_at_utc": "2026-02-01T12:00:00Z",
        },
        "history": [],
        "controls": {"kill_switch": False, "application_enabled": True},
    }


def copy_command() -> dict:
    return deepcopy(apply_command())
