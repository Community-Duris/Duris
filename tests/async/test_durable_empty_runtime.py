#!/usr/bin/env python3
"""Executable regression coverage for durable ``empty`` planning/publication.

The harness compiles the production preflight and publication functions with a
small object-graph double.  It exercises the real cumulative capacity gate,
ancestor/cycle checks, item restrictions, publication failure rollback, and
metadata preservation; source assertions only verify that ``do_empty`` wires
the command into those boundaries.
"""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path

from _paths import SRC


ACTOBJ_PATH = Path(os.environ.get("DURIS_ACTOBJ_OVERRIDE", SRC / "actobj.c"))
ACTOBJ = ACTOBJ_PATH.read_text(encoding="utf-8")


def function_body(signature: str) -> str:
    start = ACTOBJ.index(signature)
    brace = ACTOBJ.index("{", start)
    depth = 0
    for end in range(brace, len(ACTOBJ)):
        if ACTOBJ[end] == "{":
            depth += 1
        elif ACTOBJ[end] == "}":
            depth -= 1
            if depth == 0:
                return ACTOBJ[start : end + 1]
    raise AssertionError(f"unterminated function: {signature}")


assert "item_command_container_is_valid(obj2)" in ACTOBJ
assert "start_empty(ch, obj1, obj2);" in ACTOBJ
do_empty = function_body("void do_empty(")
assert "obj_from_obj" not in do_empty
assert "obj_to_obj" not in do_empty
start_empty = function_body("void start_empty(")
completion = function_body("bool empty_completion(")
publish = function_body("bool publish_empty_objects(")
collect_publication = function_body("bool empty_collect_publication_objects(")
for required in (
    "empty_graph_is_valid",
    "empty_item_restrictions_allow",
    "bulk_put_permitted",
    "item_movement_transaction_submit_batch",
):
    assert required in start_empty
assert "empty_publication_allowed" in completion
assert "Nothing was emptied; the batch ownership move did not commit." in completion
assert "moved.rbegin()" in publish
assert "space = GET_OBJ_SPACE(target);" in start_empty
assert "space = GET_OBJ_SPACE(target);" in collect_publication
assert "corpse_lifecycle_transaction_busy" in start_empty
assert "source_destination.target_container ? source->obj_uid : 0" in start_empty

PRELUDE = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <string>
#include <vector>

#define USE_SPACE 0
#define ITEM_TRANSFER_MAX_ITEMS 3000
#define ITEM_CONTAINER 1
#define ITEM_QUIVER 2
#define ITEM_STORAGE 3
#define ITEM_CORPSE 4
#define ITEM_MISSILE 5
#define ITEM_NODROP (1 << 0)
#define CONT_CLOSED (1 << 0)
#define LOC_INSIDE (1 << 0)
#define LOC_NOWHERE (1 << 1)
#define LOC_ROOM (1 << 2)
#define LOC_CARRIED (1 << 3)
#define OBJ_INSIDE(o) ((o) && ((o)->loc_p & LOC_INSIDE))
#define OBJ_NOWHERE(o) ((o) && ((o)->loc_p & LOC_NOWHERE))
#define OBJ_ROOM(o) ((o) && ((o)->loc_p & LOC_ROOM))
#define OBJ_CARRIED(o) ((o) && ((o)->loc_p & LOC_CARRIED))
#define OBJ_INSIDE_OBJ(o, parent) (OBJ_INSIDE(o) && (o)->loc.inside == (parent))
#define GET_ITEM_TYPE(o) ((o)->type)
#define GET_OBJ_WEIGHT(o) ((o)->weight)
#define IS_SET(flags, bit) (((flags) & (bit)) != 0)
#define IS_ARTIFACT(o) ((o)->artifact)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define OBJ_VNUM(o) ((o)->vnum)

struct obj_data;
struct char_data
{
    bool trusted = false;
    int pid = 7;
};
using P_char = char_data *;
using P_obj = obj_data *;

struct obj_location
{
    P_obj inside = nullptr;
};

struct obj_data
{
    uint64_t obj_uid = 0;
    int type = 0;
    int extra_flags = 0;
    int value[8] = {};
    int loc_p = LOC_NOWHERE;
    obj_location loc = {};
    P_obj contains = nullptr;
    P_obj next_content = nullptr;
    int weight = 0;
    int vnum = 1;
    bool artifact = false;
    bool durable = false;
    bool fail_publish = false;
    std::string metadata = "untouched";
};

int top_of_objt = 100;

struct item_transfer_result
{
    uint16_t item_count = 0;
};

struct empty_state
{
    uint64_t source_uid = 0;
    uint64_t target_uid = 0;
    P_obj source_object = nullptr;
    P_obj target_object = nullptr;
    std::vector<uint64_t> selected_items;
    std::vector<uint64_t> durable_items;
};

static bool training_dummy = false;
static uint64_t fail_uid = 0;
static P_obj failure_target = nullptr;
static bool collect_ok = true;
static std::vector<P_obj> planned;

static P_char training_dummy_item_owner(P_obj)
{
    return training_dummy ? reinterpret_cast<P_char>(1) : nullptr;
}

static bool item_command_container_is_valid(P_obj object)
{
    return object && (object->type == ITEM_CONTAINER || object->type == ITEM_QUIVER ||
                      object->type == ITEM_STORAGE || object->type == ITEM_CORPSE);
}

static bool item_command_uses_durable_ownership(P_obj object)
{
    return object && object->durable;
}

static P_obj empty_state_source(const empty_state &state)
{
    return state.source_object;
}

static P_obj empty_state_target(const empty_state &state)
{
    return state.target_object;
}

static bool empty_collect_publication_objects(
    P_char, const empty_state &, const item_transfer_result &, std::vector<P_obj> *objects)
{
    if (!collect_ok || !objects)
        return false;
    *objects = planned;
    return true;
}

static void unlink_from(P_obj object, P_obj parent)
{
    if (parent->contains == object)
        parent->contains = object->next_content;
    else
    {
        P_obj previous = parent->contains;
        while (previous && previous->next_content != object)
            previous = previous->next_content;
        assert(previous);
        previous->next_content = object->next_content;
    }
    parent->weight -= object->weight;
    object->next_content = nullptr;
}

static void obj_from_obj(P_obj object)
{
    assert(object && object->loc.inside);
    P_obj parent = object->loc.inside;
    unlink_from(object, parent);
    object->loc_p = LOC_NOWHERE;
    object->loc.inside = nullptr;
}

static void obj_to_obj(P_obj object, P_obj target)
{
    if (object->obj_uid == fail_uid && target == failure_target)
        return;
    object->loc_p = LOC_INSIDE;
    object->loc.inside = target;
    object->next_content = target->contains;
    target->contains = object;
    target->weight += object->weight;
}

static void obj_from_room(P_obj object)
{
    object->loc_p = LOC_NOWHERE;
}

static void obj_from_char(P_obj object)
{
    object->loc_p = LOC_NOWHERE;
}
'''

DRIVER = r'''
static void attach(P_obj parent, P_obj object)
{
    object->loc_p = LOC_INSIDE;
    object->loc.inside = parent;
    object->next_content = parent->contains;
    parent->contains = object;
    parent->weight += object->weight;
}

static void reset_graph(P_obj source, P_obj target, P_obj first, P_obj second)
{
    source->contains = nullptr;
    source->weight = 0;
    target->contains = nullptr;
    target->weight = 0;
    first->loc_p = LOC_NOWHERE;
    first->loc.inside = nullptr;
    first->next_content = nullptr;
    second->loc_p = LOC_NOWHERE;
    second->loc.inside = nullptr;
    second->next_content = nullptr;
    attach(source, first);
    attach(source, second);
}

int main()
{
    char_data actor;
    obj_data source;
    obj_data target;
    source.type = ITEM_CONTAINER;
    target.type = ITEM_CONTAINER;
    target.value[0] = 5;

    // The production cumulative gate preserves the old prefix-on-capacity behavior.
    obj_data light;
    obj_data medium;
    obj_data blocked;
    light.obj_uid = 1; light.weight = 2;
    medium.obj_uid = 2; medium.weight = 3;
    blocked.obj_uid = 3; blocked.weight = 1;
    int64_t weight = 0, space = 0, quiver = 0;
    std::vector<uint64_t> selected;
    for (P_obj object : { &light, &medium, &blocked })
    {
        if (!bulk_put_permitted(&actor, object, &target, weight, space, quiver))
            break;
        selected.push_back(object->obj_uid);
    }
    assert((selected == std::vector<uint64_t>{ 1, 2 }));
    assert(weight == 5 && blocked.weight == 1);

    // A non-container target and all prohibited item variants fail before a detach.
    obj_data not_container;
    not_container.type = 0;
    assert(!empty_target_accepts(&light, &not_container));
    light.artifact = true;
    assert(!empty_item_restrictions_allow(&actor, &light, &target));
    light.artifact = false;
    light.extra_flags = ITEM_NODROP;
    assert(!empty_item_restrictions_allow(&actor, &light, &target));
    light.extra_flags = 0;
    obj_data quiver_target;
    quiver_target.type = ITEM_QUIVER;
    quiver_target.value[2] = 9;
    light.type = ITEM_MISSILE;
    light.value[3] = 8;
    assert(!empty_item_restrictions_allow(&actor, &light, &quiver_target));

    // Nested roots are accepted, but a target inside the moved graph and cycles are not.
    obj_data root;
    obj_data nested;
    root.obj_uid = 10; root.type = ITEM_CONTAINER; root.durable = true;
    nested.obj_uid = 11; nested.weight = 1; nested.durable = true;
    attach(&root, &nested);
    std::vector<P_obj> visited;
    assert(empty_tree_contains(&root, &nested, &visited));
    visited.clear();
    assert(!empty_tree_contains(&root, &target, &visited));
    visited.clear();
    size_t count = 0;
    assert(empty_graph_is_valid(&root, &target, &visited, &count));
    target.loc_p = LOC_INSIDE;
    target.loc.inside = &root;
    root.contains = &target;
    target.next_content = &nested;
    visited.clear(); count = 0;
    assert(!empty_graph_is_valid(&root, &target, &visited, &count));
    target.loc_p = LOC_NOWHERE;
    target.loc.inside = nullptr;
    root.contains = &nested;
    nested.next_content = nullptr;
    visited.clear(); count = 0;
    nested.contains = &root;
    root.loc_p = LOC_INSIDE;
    root.loc.inside = &nested;
    assert(!empty_graph_is_valid(&root, &target, &visited, &count));
    nested.contains = nullptr;
    root.loc_p = LOC_NOWHERE;
    root.loc.inside = nullptr;

    // A transient outer root cannot hide a durable nested root from the plan.
    root.durable = false;
    nested.durable = true;
    visited.clear();
    assert(empty_graph_has_durable(&root, &visited));

    // Commit, admission, stale-topology, and publication failures all gate before
    // the production publication loop is allowed to detach anything.
    assert(!empty_publication_allowed(false, true, true, true, true, true, true, 2, 2));
    assert(!empty_publication_allowed(true, false, true, true, true, true, true, 2, 2));
    assert(!empty_publication_allowed(true, true, false, true, true, true, true, 2, 2));
    assert(!empty_publication_allowed(true, true, true, false, true, true, true, 2, 2));
    assert(!empty_publication_allowed(true, true, true, true, true, false, true, 2, 2));
    assert(!empty_publication_allowed(true, true, true, true, true, true, false, 2, 2));
    assert(!empty_publication_allowed(true, true, true, true, true, true, true, 1, 2));
    assert(empty_publication_allowed(true, true, true, true, true, true, true, 2, 2));

    // Successful publication preserves object metadata while moving the roots.
    obj_data first;
    obj_data second;
    first.obj_uid = 21; first.weight = 1; first.metadata = "first-metadata";
    second.obj_uid = 22; second.weight = 1; second.metadata = "second-metadata";
    reset_graph(&source, &target, &first, &second);
    empty_state state;
    state.source_object = &source;
    state.target_object = &target;
    planned = { &first, &second };
    fail_uid = 0;
    failure_target = &target;
    std::vector<P_obj> published;
    item_transfer_result result;
    assert(publish_empty_objects(&actor, state, result, &published));
    assert(OBJ_INSIDE_OBJ(&first, &target) && OBJ_INSIDE_OBJ(&second, &target));
    assert(first.metadata == "first-metadata" && second.metadata == "second-metadata");

    // A stale/mismatched live snapshot is rejected before publication starts.
    reset_graph(&source, &target, &first, &second);
    collect_ok = false;
    published.clear();
    assert(!publish_empty_objects(&actor, state, result, &published));
    assert(OBJ_INSIDE_OBJ(&first, &source) && OBJ_INSIDE_OBJ(&second, &source));
    collect_ok = true;

    // Inject a publication failure after the first candidate: rollback restores
    // every root to the source, so no partial detach or false success is possible.
    reset_graph(&source, &target, &first, &second);
    fail_uid = second.obj_uid;
    published.clear();
    assert(!publish_empty_objects(&actor, state, result, &published));
    assert(OBJ_INSIDE_OBJ(&first, &source) && OBJ_INSIDE_OBJ(&second, &source));
    assert(target.contains == nullptr);
    assert(first.metadata == "first-metadata" && second.metadata == "second-metadata");

    puts("durable empty planning/publication runtime: ok");
}
'''

HARNESS = "\n".join(
    [
        PRELUDE,
        function_body("bool empty_item_restrictions_allow("),
        function_body("bool empty_target_accepts("),
        function_body("bool empty_tree_contains("),
        function_body("bool empty_graph_is_valid("),
        function_body("bool empty_graph_has_durable("),
        function_body("bool empty_publication_allowed("),
        function_body("bool bulk_put_permitted("),
        function_body("bool publish_empty_objects("),
        DRIVER,
    ]
)

with tempfile.TemporaryDirectory(prefix="duris-empty-523-") as directory:
    root = Path(directory)
    source = root / "durable_empty_runtime.cpp"
    binary = root / "durable_empty_runtime"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-g",
            str(source),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=30)
