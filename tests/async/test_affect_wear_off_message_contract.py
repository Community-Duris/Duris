#!/usr/bin/env python3
"""Regression contract for multi-part spell wear-off messages."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/affects.c").read_text()

start = SOURCE.index("void wear_off_message(")
end = SOURCE.index("//=================================================================================", start)
body = SOURCE[start:end]

duplicate_guard = body.index("other != af && other->type == af->type")
message = body.index("send_to_char(skills[af->type].wear_off_char[idx], ch)")

assert duplicate_guard < message

print("multi-part affect wear-off message contract passed")
