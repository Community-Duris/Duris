#!/usr/bin/env python3
"""Source contracts for atomic durable floor/container pickup via ``get all``."""

from _paths import SRC

ACTOBJ = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = (SRC / "item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)


def function_body(source, signature, *, last=False):
    """Extract a balanced function body, optionally skipping earlier declarations."""
    start = source.rindex(signature) if last else source.index(signature)
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
    """Report one source contract and return its success for the aggregate result."""
    print(("PASS: " if condition else "FAIL: ") + name)
    return bool(condition)


do_get = function_body(ACTOBJ, "void do_get(")
start_bulk = function_body(ACTOBJ, "static void start_bulk_get(")
continue_bulk = function_body(ACTOBJ, "static void continue_bulk_get(", last=True)
adoption_completion = function_body(
    ACTOBJ, "static void bulk_get_adoption_completion("
)
start_floor = function_body(ACTOBJ, "static void start_floor_bulk_get(")
start_container = function_body(ACTOBJ, "static void start_container_bulk_get(")
select_item = function_body(ACTOBJ, "static bool select_bulk_get_item(")
completion = function_body(ACTOBJ, "static void bulk_get_completion(")
after_commit = function_body(ACTOBJ, "static void finish_bulk_get_after_commit(")
finish = function_body(ACTOBJ, "static void report_bulk_get(")
single_get = function_body(ACTOBJ, "void get(P_char ch")
submit_batch = function_body(
    MOVEMENT, "bool item_movement_transaction_submit_batch("
)
room_finalize = function_body(ACTOBJ, "static void do_get_finalize_room_item(")
container_finalize = function_body(
    ACTOBJ, "static void do_get_finalize_container_item("
)

ok = True
ok &= check(
    "player room and container batches enter the shared atomic path",
    "start_floor_bulk_get(ch, alldot ? Gbuf2 : NULL);" in do_get
    and "start_container_bulk_get(ch, s_obj" in do_get
    and "start_bulk_get(actor, NULL, filter, false);" in start_floor
    and "start_bulk_get(actor, container, filter, corpse);" in start_container,
)
ok &= check(
    "selection snapshots durable UIDs and preserves the keyword filter",
    "std::vector<uint64_t> durable_items" in ACTOBJ
    and "state.durable_items.push_back(object->obj_uid)" in select_item
    and "isname(filter, object->name)" in select_item,
)
ok &= check(
    "selection applies cumulative count and weight capacity",
    "int carried_count = IS_CARRYING_N(actor)" in start_bulk
    and "int64_t carried_weight = total_carried_weight(actor)" in start_bulk
    and "carried_count >= CAN_CARRY_N(actor)" in select_item
    and "carried_weight + GET_OBJ_WEIGHT(object)" in select_item
    and "++carried_count" in select_item
    and "carried_weight += GET_OBJ_WEIGHT(object)" in select_item,
)
ok &= check(
    "binding, trap, hitch, and no-loot exclusions happen before submission",
    "account_bound_reward_owner" in select_item
    and "checkgetput(actor, object)" in select_item
    and "object->hitched_to" in select_item
    and "ITEM2_NOLOOT" in select_item,
)
ok &= check(
    "bulk corpse pickup waits for lifecycle settlement before selection",
    "corpse_lifecycle_transaction_busy" in start_bulk
    and start_bulk.index("corpse_lifecycle_transaction_busy")
    < start_bulk.index("bulk_get_state state"),
)
ok &= check(
    "missing stock roots adopt in place before one multi-root movement",
    "get_item_source_owner(actor, roots.front(), container, &source)" in start_bulk
    and "continue_bulk_get(actor, actor_pid);" in start_bulk
    and "item_ownership_runtime_lookup(root->obj_uid, &runtime)" in continue_bulk
    and "state.source, state.source" in continue_bulk
    and "bulk_get_adoption_completion" in continue_bulk
    and continue_bulk.count("item_movement_transaction_submit_batch(") == 1
    and "roots.data(), roots.size()" in continue_bulk,
)
ok &= check(
    "failed adoption leaves the complete live batch at its source",
    "if (!committed)" in adoption_completion
    and "Nothing was taken" in adoption_completion
    and "bulk_gets.erase(found)" in adoption_completion
    and "finish_bulk_get_after_commit" not in adoption_completion
    and "do_get_finalize" not in adoption_completion,
)
ok &= check(
    "the movement adapter captures and encodes one forest",
    "ordered_roots.assign(roots, roots + root_count)" in submit_batch
    and "player_item_snapshot_tree_capture" in submit_batch
    and "snapshot.parent_index += static_cast<int32_t>(offset)" in submit_batch
    and ".multi_root = true" in submit_batch,
)
validation = completion.index("for (uint64_t item_uid : state.durable_items)")
publication = completion.index("item_get_ack_publication = true")
ok &= check(
    "completion validates every selected root before publishing any live move",
    validation < publication
    and "bulk_get_source_matches(state, container, object)" in completion
    and "get_batch_publish" in completion,
)
ok &= check(
    "corpse revision publication happens once for the committed forest",
    completion.count("corpse_lifecycle_transaction_note_item_transfer(") == 1
    and completion.index("corpse_lifecycle_transaction_note_item_transfer(")
    < publication,
)
ok &= check(
    "currency and lifecycle-owned roots wait until durable commit succeeds",
    "std::vector<synchronous_get_item> synchronous_items" in ACTOBJ
    and "finish_bulk_get_after_commit(actor, state, container);" in completion
    and "do_get_finalize_room_item" in after_commit
    and "do_get_finalize_container_success" in after_commit
    and completion.index("if (!committed)")
    < completion.index("finish_bulk_get_after_commit(actor, state, container);"),
)
ok &= check(
    "single durable get rejects no-loot before ownership submission",
    single_get.index("uses_generic_item_ownership(o_obj) &&")
    < single_get.index("item_movement_transaction_submit(ch, o_obj"),
)
ok &= check(
    "empty room and container batches retain distinct messages",
    'state.container_uid ? "You find nothing in it.\\r\\n"' in finish
    and '"You see nothing here.\\r\\n"' in finish,
)
ok &= check(
    "coin extraction cannot be followed by stale debug or artifact dereferences",
    "const bool money" in room_finalize
    and "if (!money)" in room_finalize
    and "const bool money" in container_finalize
    and container_finalize.index("if (money)") < container_finalize.index("GETDBG_LOG"),
)

if not ok:
    raise SystemExit(1)

print("\nAll atomic get-all contracts passed.")
