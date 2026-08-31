#!/usr/bin/env python3
"""Contracts for serialized durable floor pickup via `get all`."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ACTOBJ = (ROOT / "src/actobj.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = (ROOT / "src/item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)


def function_body(source, signature):
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
    print(("PASS: " if condition else "FAIL: ") + name)
    return bool(condition)


do_get = function_body(ACTOBJ, "void do_get(")
start_floor = function_body(ACTOBJ, "static void start_floor_bulk_get(")
start_container = function_body(ACTOBJ, "static void start_container_bulk_get(")
continue_bulk = function_body(
    ACTOBJ, "void continue_bulk_get(P_char actor, bool previous_succeeded)\n{"
)
completion = function_body(ACTOBJ, "void item_get_completion(")
finish = function_body(ACTOBJ, "static void finish_bulk_get(")
publish = function_body(MOVEMENT, "void publish(")
player_ready = function_body(
    MOVEMENT, "void item_movement_transaction_player_ready("
)
room_finalize = function_body(ACTOBJ, "static void do_get_finalize_room_item(")
container_finalize = function_body(
    ACTOBJ, "static void do_get_finalize_container_item("
)

ok = True
ok &= check(
    "player get-all enters the serialized bulk path",
    "start_floor_bulk_get(ch, alldot ? Gbuf2 : NULL);" in do_get,
)
ok &= check(
    "the blanket player rejection is gone",
    "Durable floor items must be collected one at a time" not in ACTOBJ,
)
ok &= check(
    "get-all snapshots durable item identities rather than live pointers",
    "std::vector<uint64_t> durable_items" in ACTOBJ
    and "durable_items.push_back(object->obj_uid)" in start_floor,
)
ok &= check(
    "filtered get-all preserves the requested keyword",
    "filter ? filter" in start_floor
    and "isname(state.filter.c_str(), object->name)" in continue_bulk,
)
ok &= check(
    "durable siblings submit one at a time",
    "item_get_deferred" in continue_bulk
    and "continue_bulk_get(actor, true);" in completion,
)
ok &= check(
    "bulk pickup stops if the player leaves the source room",
    "actor->in_room != state.room" in continue_bulk
    and "source is no longer here" in continue_bulk,
)
ok &= check(
    "currency and lifecycle-owned floor items retain synchronous pickup",
    "uses_generic_item_ownership(object)" in continue_bulk
    and "PC_CORPSE" in ACTOBJ
    and "do_get_finalize_room_item(actor, object" in continue_bulk,
)
ok &= check(
    "container get-all uses the same serialized durable chain",
    "start_container_bulk_get(ch, s_obj" in do_get
    and "container->obj_uid" in start_container
    and "do_get_finalize_container_success" in continue_bulk,
)
ok &= check(
    "the blanket container rejection is gone",
    "Durable container items must be collected one at a time" not in ACTOBJ,
)
ok &= check(
    "an empty container reports that no items were found",
    'from_container ? "You find nothing in it.\\r\\n"' in finish,
)
ok &= check(
    "floor coin extraction cannot be followed by an artifact dereference",
    "const bool money" in room_finalize and "if (!money)" in room_finalize,
)
ok &= check(
    "container coin extraction cannot be followed by a debug dereference",
    "const bool money" in container_finalize
    and "if (money)" in container_finalize
    and container_finalize.index("if (money)")
    < container_finalize.index("GETDBG_LOG"),
)

erase = publish.index("pending.erase(found);")
callback = publish.index("completion_fn(actor")
ok &= check(
    "completed movement is erased before a chaining callback can rehash pending state",
    erase < callback,
)
ok &= check(
    "offline completion publication restarts traversal after a chaining callback",
    "std::find_if" in player_ready
    and "publish(found, actor);" in player_ready
    and "found++" not in player_ready,
)

if not ok:
    raise SystemExit(1)

print("\nAll durable get-all contracts passed.")
