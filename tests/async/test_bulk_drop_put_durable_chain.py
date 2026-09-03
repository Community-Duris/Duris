#!/usr/bin/env python3
"""Source contracts for atomic durable `drop all` / `put all` batches."""

from _paths import SRC

ACTOBJ = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = (SRC / "item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)
MOVEMENT_HEADER = (SRC / "item_movement_transaction.h").read_text(
    encoding="utf-8", errors="replace"
)


def function_body(source, signature):
    """Return one complete brace-delimited C++ function."""
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start : pos + 1]
    raise AssertionError(f"unterminated function: {signature}")


def check(name, condition):
    """Print one source-contract result and return its truth value."""
    print(("PASS: " if condition else "FAIL: ") + name)
    return bool(condition)


def normalize_cxx(source):
    """Remove formatting whitespace before comparing a C++ expression."""
    return "".join(source.split())


do_drop = function_body(ACTOBJ, "void do_drop(")
do_dropalldot = function_body(ACTOBJ, "void do_dropalldot(")
do_put = function_body(ACTOBJ, "void do_put(")
ownership_filter = function_body(ACTOBJ, "static bool uses_generic_item_ownership(")
ownership_filter_body = ownership_filter[
    ownership_filter.index("{") + 1 : ownership_filter.rindex("}")
]
start_drop = function_body(ACTOBJ, "void start_bulk_drop(")
drop_completion = function_body(ACTOBJ, "void bulk_drop_completion(")
drop_finish = function_body(ACTOBJ, "void finish_bulk_drop_after_commit(")
start_put = function_body(ACTOBJ, "void start_bulk_put(")
put_completion = function_body(ACTOBJ, "void bulk_put_completion(")
put_finish = function_body(ACTOBJ, "void finish_bulk_put_after_commit(")
put_preflight = function_body(ACTOBJ, "bool bulk_put_permitted(")
batch_submit = function_body(
    MOVEMENT, "bool item_movement_transaction_submit_batch("
)

ok = True

ok &= check(
    "drop all and drop all.<name> enter the durable batch path",
    "start_bulk_drop(ch, NULL, false);" in do_drop
    and "start_bulk_drop(ch, name, true);" in do_dropalldot,
)
ok &= check(
    "put all and put all.<name> enter the durable batch path",
    "start_bulk_put(ch, s_obj, type == PUT_ALLDOT ? obj_name : NULL," in do_put,
)
ok &= check(
    "wallet coins retain their dedicated commands",
    'strcmp(name, "coins")' in do_dropalldot
    and 'strcmp(obj_name, "all.coins")' in do_put,
)
ok &= check(
    "active owned transients use durable movement while unowned transients stay synchronous",
    normalize_cxx(ownership_filter_body)
    == normalize_cxx(
        """if (!object || object->obj_uid == 0 || object->type == ITEM_MONEY ||
        (object->type == ITEM_CORPSE &&
         IS_SET(object->value[CORPSE_FLAGS], PC_CORPSE)))
            return false;
        if (!IS_SET(object->extra_flags, ITEM_TRANSIENT))
            return true;
        item_ownership_runtime_entry ownership = {};
        return item_ownership_runtime_lookup(object->obj_uid, &ownership) &&
               ownership.state == item_custody_state::active;"""
    ),
)

ok &= check(
    "the movement API exposes one multi-root submission",
    "item_movement_transaction_submit_batch" in MOVEMENT_HEADER
    and ".multi_root = true" in batch_submit,
)
ok &= check(
    "the adapter captures every root and rebases snapshot parents",
    "ordered_roots.assign(roots, roots + root_count);" in batch_submit
    and "capture(root, runtime.root_item_uid, runtime.parent_item_uid, &items)"
    in batch_submit
    and "snapshot.parent_index += static_cast<int32_t>(offset);" in batch_submit,
)
ok &= check(
    "the adapter emits one command after the complete forest is encoded",
    batch_submit.count("item_transfer_command_build(") == 1
    and batch_submit.count("critical_command_coordinator_submit(") == 1,
)

ok &= check(
    "drop selection preflights eligibility before one batch submit",
    "bulk_drop_permitted(actor, object, state)" in start_drop
    and start_drop.count("item_movement_transaction_submit_batch(") == 1,
)
ok &= check(
    "drop publication validates every live root before moving any",
    drop_completion.index("for (uint64_t item_uid : state.durable_items)")
    < drop_completion.index("for (P_obj object : objects)")
    < drop_completion.index("publish_player_drop(actor"),
)
ok &= check(
    "transient drop paths run only after the durable commit",
    "finish_bulk_drop_after_commit(actor, state);" in drop_completion
    and "uses_generic_item_ownership(object)" in drop_finish,
)
ok &= check(
    "cursed and soulbound roots remain excluded",
    "ITEM2_SOULBIND" in ACTOBJ
    and "ITEM_NODROP" in ACTOBJ
    and "it must be CURSED!" in ACTOBJ,
)

ok &= check(
    "put uses cumulative quiver, weight, and optional space preflight",
    "quiver_count" in put_preflight
    and "weight + GET_OBJ_WEIGHT(object)" in put_preflight
    and "space + GET_OBJ_SPACE(object)" in put_preflight,
)
ok &= check(
    "put selection submits all permitted durable roots once",
    "bulk_put_permitted(actor, object, container, weight, space," in start_put
    and "quiver_count))" in start_put
    and start_put.count("item_movement_transaction_submit_batch(") == 1,
)
ok &= check(
    "put publication revalidates the full set before publishing it",
    put_completion.index("for (uint64_t item_uid : state.durable_items)")
    < put_completion.index("item_put_ack_publication = true;")
    < put_completion.index("for (P_obj object : objects)"),
)
ok &= check(
    "transient put paths run only after the durable commit",
    "finish_bulk_put_after_commit(actor, state, container);" in put_completion
    # The split is chosen once, in start_bulk_put(). Re-deriving it here from
    # uses_generic_item_ownership() would strand an item whose runtime ownership
    # row activates while the batch commits: neither pass would claim it.
    and "bulk_put_batch_claimed(state, object)" in put_finish
    and "uses_generic_item_ownership(object)" not in put_finish,
)
ok &= check(
    "the serialized durable chains are gone",
    "continue_bulk_drop" not in ACTOBJ and "continue_bulk_put" not in ACTOBJ,
)

if not ok:
    raise SystemExit(1)

print("\nAll atomic durable bulk drop/put contracts passed.")
