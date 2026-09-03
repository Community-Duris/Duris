"""The bulk put split must be decided once, not re-derived after the commit.

`uses_generic_item_ownership()` is not stable for a transient object: it reads the
runtime ownership row, which becomes active asynchronously.  `start_bulk_put()` uses
it to choose the durable batch, and that batch commits asynchronously.  If
`finish_bulk_put_after_commit()` re-tested the same predicate, an item that became
owned while the batch was in flight would be claimed by neither pass -- silently left
in the inventory and missing from the "You put N items" total.  The synchronous pass
must instead skip only what the batch actually published.
"""

from __future__ import annotations

import re

from _paths import SRC

src = (SRC / "actobj.c").read_text(encoding="utf-8")


def body(name: str) -> str:
    match = re.search(
        r"^\w[\w \*&:<>,]*\b" + name + r"\([^)]*\)\s*\n\{(.*?)^\}",
        src,
        re.S | re.M,
    )
    assert match, f"{name}() definition not found in actobj.c"
    return match.group(1)


put_finish = body("finish_bulk_put_after_commit")

assert "uses_generic_item_ownership" not in put_finish, (
    "finish_bulk_put_after_commit() re-derives the durable split from "
    "uses_generic_item_ownership(); an item whose ownership row activates during the "
    "commit would then be dropped by both passes"
)
assert "bulk_put_batch_claimed(state, object)" in put_finish, (
    "finish_bulk_put_after_commit() must skip only what the durable batch published"
)

# The claim test is by recorded UID, so a UID-less object always stays synchronous.
claimed = body("bulk_put_batch_claimed")
assert "state.durable_items" in claimed and "std::find" in claimed, (
    "bulk_put_batch_claimed() must test membership of the recorded durable batch"
)
assert "object->obj_uid != 0" in claimed, (
    "bulk_put_batch_claimed() must treat a UID-less object as never batched"
)

# start_bulk_put() remains the single place the durable batch is chosen.
start = body("start_bulk_put")
assert "uses_generic_item_ownership(object)" in start, (
    "start_bulk_put() must classify the durable batch"
)
assert "state.durable_items.push_back(object->obj_uid)" in start, (
    "start_bulk_put() must record each durable candidate by UID"
)

# The drop path moves synchronously without an ownership transaction, so it must keep
# refusing any object that is generic-owned at publication time.
drop_finish = body("finish_bulk_drop_after_commit")
assert "uses_generic_item_ownership(object)" in drop_finish, (
    "finish_bulk_drop_after_commit() must keep refusing generic-owned objects; "
    "drop_transient_object() moves them without an ownership transaction"
)
