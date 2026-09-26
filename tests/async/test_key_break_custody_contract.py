#!/usr/bin/env python3
"""Broken durable keys retire custody before leaving the live object graph."""

from _paths import SRC

actmove = (SRC / "actmove.c").read_text()

helper_start = actmove.index("static bool publish_key_break(")
helper_end = actmove.index("static void telemetry_gameplay_context_changed", helper_start)
helper = actmove[helper_start:helper_end]
unlock_start = actmove.index("void do_unlock(")
unlock_end = actmove.index("void do_pick(", unlock_start)
unlock = actmove[unlock_start:unlock_end]

assert "item_ownership_runtime_lookup" in helper
assert "item_owner_type::destruction" in helper
assert "item_transfer_reason::destruction" in helper
assert "item_movement_transaction_submit" in helper
assert "publish_key_break" in helper
assert helper.index("if (!committed)") < helper.index("extract_obj(key, TRUE)")
assert unlock.count("break_key(ch, key_obj);") == 2
assert "extract_obj(key_obj" not in unlock

print("broken key custody contract passed")
