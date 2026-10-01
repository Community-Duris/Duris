#!/usr/bin/env python3
"""Check the guided restitution operator contract and safe verification wording."""

from _paths import ROOT


guide = (ROOT / "docs" / "operations" / "RESTITUTION_GUIDE.md").read_text()
command = (ROOT / "src" / "cmd" / "player_death_restitution.c").read_text()

for text in (
    "inspect",
    "plan",
    "export",
    "restitution begin",
    "restitution chunk <CHUNK_1_FROM_staff-payload.json>",
    "restitution commit",
    "restitution status <OPERATION_ID_FROM_commit>",
    "restitution abort",
    "journal-uncertain",
    "Do not retry",
    "durable-receipt-unverified",
    "delivery=not_verified",
    "exact target readback",
    "process restart",
):
    assert text in guide, text

for text in (
    "restitution help",
    "restitution status <operation-id>",
    "accepted_chunks=",
    "delivery=not_verified",
    "DO NOT RETRY",
    "existing staged data was retained",
):
    assert text in command, text

assert "delivery=verified" not in command
print("[PASS] restitution guide and command expose bounded phases without a false delivery claim")
