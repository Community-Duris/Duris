#!/usr/bin/env python3
"""Issue #569: replacement item payload cannot diverge from active custody."""

from _paths import SRC
from contract_text import contains, index


repository = (SRC / "player_snapshot_repository.c").read_text()
capture = (SRC / "player_snapshot_capture.c").read_text()
pipeline = (SRC / "player_save_pipeline.c").read_text()
worker = (SRC / "player_save_worker.c").read_text()
worker_header = (SRC / "player_save_worker.h").read_text()
diagnostics = (SRC / "actinf.c").read_text()


def function(text: str, signature: str) -> str:
    """Return a complete C++ function body including its signature."""
    start = index(text, signature)
    opening = text.index("{", start)
    depth = 0
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


verify = function(repository, "query_result verify_player_item_custody(")
apply_items = function(repository, "query_result apply_items(")

# The proof uses the sealed replacement graph, reconstructs every root/parent,
# and locks the authoritative rows in the same transaction as replacement.
for token in (
    "item.object_uid",
    "item.parent_index",
    "root_item_uid",
    "parent_item_uid",
    "item.vnum",
    "item_current_owner",
    "owner_type=",
    "owner_id=",
    "owner_context_id=0",
    "state=",
    "ORDER BY item_uid FOR UPDATE",
):
    assert contains(verify, token), token
assert contains(verify, "PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH")
assert contains(verify, "coin_payload IS NOT NULL")
assert contains(verify, "inline_coin_payload")
assert contains(verify, "expected.empty()")

# Verification precedes the destructive projection; a rejection therefore
# rolls back with every old payload row untouched.
assert index(apply_items, "verify_player_item_custody") < index(
    apply_items, '"DELETE FROM player_items WHERE pid="'
)
assert not contains(verify, "DELETE FROM")
assert not contains(verify, "UPDATE item_current_owner")

# New item dirtiness is always a complete equipment+inventory graph. This also
# prevents equip-root children (stored with equip_slot=0) from being mistaken
# for an independent inventory-only replacement.
mark = function(pipeline, "bool player_save_pipeline_mark(")
assert contains(
    mark,
    "components |= PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;",
)
assert "#ifdef __NO_MYSQL__" not in mark

# A lossy flag cannot trump already-active authority. Truly unowned NORENT
# runtime objects keep their historical filter behavior.
capture_tree = function(capture, "capture_item_tree(")
assert contains(capture_tree, "active_durable_custody")
assert contains(capture_tree, "durable_norent_included")
assert contains(capture_tree, "!active_durable_custody")
assert index(capture_tree, "active_durable_custody =") < index(
    capture_tree, "!active_durable_custody"
)

# Rejections are separately measurable and produce a redacted operational alert.
assert contains(worker_header, "PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH")
assert contains(worker_header, "custody_payload_mismatches")
assert contains(worker, "health.custody_payload_mismatches")
assert contains(diagnostics, '"custody_payload_mismatch=%llu')
assert contains(pipeline, '"custody_payload_mismatch_rejected"')
assert contains(pipeline, "destructive_write=0")
assert contains(pipeline, '"custody-mismatch-recapture"')
assert contains(pipeline, "recapture_scheduled=%d")
assert contains(pipeline, "custody_recapture_armed.insert")
assert contains(pipeline, "custody_recapture_armed.erase")
assert contains(pipeline, "custody_recapture_allowed")
assert contains(pipeline, "GET_STAT(ch) != STAT_DEAD")
assert contains(pipeline, "CHAR_RFLAG_LOAD_DEGRADED")

print("player item custody write guard contract: ok")
