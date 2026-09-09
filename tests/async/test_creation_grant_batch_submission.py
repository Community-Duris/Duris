#!/usr/bin/env python3
"""Contract for submitting detached starter roots as one creation operation."""

from _paths import SRC


MOVEMENT = (SRC / "item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)
MOVEMENT_HEADER = (SRC / "item/item_movement_transaction.h").read_text(
    encoding="utf-8", errors="replace"
)


def function_body(source: str, signature: str, *, last: bool = False) -> str:
    start = source.rindex(signature) if last else source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


BATCH_SUBMIT = function_body(MOVEMENT, "bool item_movement_transaction_submit_batch(")
START_GRANT = function_body(MOVEMENT, "bool start_creation_grant(", last=True)
PRE_ENTRY_BATCH = function_body(
    MOVEMENT, "bool item_creation_grant_submit_batch_to_player_before_entry("
)
CANCEL = function_body(MOVEMENT, "void item_creation_grant_cancel_batch_before_entry(")

assert "const bool creation =" in BATCH_SUBMIT
assert "capture_absent(" in BATCH_SUBMIT
assert "item_transfer_reason::creation" in BATCH_SUBMIT
assert "creation_grant_batch_completion" in START_GRANT
assert "item_movement_transaction_submit_batch(" in START_GRANT
assert "creation_grant_conflicts" in START_GRANT
assert "reconcile_creation_grant_batch" in MOVEMENT
assert "player_load_item_graph_materialize_creation" in MOVEMENT
assert "item_movement_reject_is_transient" in PRE_ENTRY_BATCH
assert "queue.batch_submission = true" in PRE_ENTRY_BATCH
assert "queue.batch_submission ? queue.requests.size()" in CANCEL
assert "ITEM_CREATION_GRANT_MAX_ROOTS" in MOVEMENT_HEADER
assert "count > ITEM_CREATION_GRANT_MAX_ROOTS" in PRE_ENTRY_BATCH

print("[PASS] detached starter roots use one creation batch submission")
