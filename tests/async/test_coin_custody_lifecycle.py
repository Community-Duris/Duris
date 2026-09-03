#!/usr/bin/env python3
"""Physical coin puts retain or establish custody before live publication."""

from _paths import ROOT, SRC


def body(source: str, signature: str, terminator: str) -> str:
    start = source.index(signature)
    return source[start:source.index(terminator, start)]


actobj = (SRC / "actobj.c").read_text()
movement = (SRC / "item_movement_transaction.c").read_text()
mysql = (ROOT / "tests/async/item_transfer_mysql_harness.cpp").read_text()
flatfile = (ROOT / "tests/async/flatfile_item_repository_harness.cpp").read_text()

publish = body(actobj, "bool publish_coin_put(", "\nvoid publish_coin_drop(")
completion = body(
    actobj, "void coin_put_custody_completion(", "\nbool publish_transient_coin_put(")
transient = body(actobj, "bool publish_transient_coin_put(", "\nbool publish_coin_put(")

# Combining uses the existing pile and UID. An empty durable container creates a
# nowhere object and submits its serialized payload before attaching it live.
assert "P_obj money = old_money;" in publish
assert "add_coins(money" not in publish
assert "extract_obj(old_money)" not in publish
assert "create_money(combined[0]" in publish
assert publish.index("create_money(combined[0]") < publish.index(
    "item_movement_transaction_submit(")
assert "destination, destination" in publish
assert "ownership_parent" in publish

# A failed item command leaves an existing pile untouched or discards the
# unpublished new object, then compensates the already committed wallet debit.
failed = completion.index("if (!committed)")
refund = completion.index("refund_committed_coin_debit", failed)
publication = completion.index("finish_coin_put_publication", refund)
assert "extract_obj(money, FALSE)" in completion[failed:refund]
assert failed < refund < publication

# Successful combination and empty-container publication happen only after the
# custody callback.
assert completion.index("if (!committed)") < completion.index("obj_to_obj(money, container)")
assert completion.index("if (!committed)") < completion.index("add_coins(money")
assert "The coins are durable" in completion

# Explicitly unowned transient containers retain their old synchronous behavior,
# while a durable UID-bearing container without authority fails closed.
assert "ITEM_TRANSIENT" in publish
assert "return false;" in publish[publish.index("coin_put_destination_custody"):]
assert "put(actor, money, container, FALSE)" in transient

# The shared item command serializes the pile before submission. Existing backend
# harnesses exercise direct custody creation under a container, which is the
# backend-neutral boundary used here; MariaDB snapshots may publish only after it.
assert "player_item_snapshot_list_encode" in movement
assert "nested_creation.target_parent_item_uid" in mysql
assert "nested_single_creation" in flatfile and "target_parent_item_uid" in flatfile
assert "item_transfer_repository_execute" in (
    SRC / "item_transfer_repository.c").read_text()
assert "flatfile_item_transfer_materialization_prepare" in (
    SRC / "flatfile_item_repository.c").read_text()

print("physical coin puts preserve or establish authoritative custody")
