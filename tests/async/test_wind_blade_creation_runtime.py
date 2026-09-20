#!/usr/bin/env python3
"""Regression test for Wind Blade's post-publication creation callback."""

import subprocess
from pathlib import Path

from _paths import ROOT, extract_function, source

ETHERMANCER = source("ethermancer.c").read_text(encoding="utf-8", errors="replace")
ACTOBJ = source("actobj.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = source("item/item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


# The old synchronous handoff is the defect under test.
grant = function_body(ETHERMANCER, "void grant_wind_blade(")
spell = function_body(ETHERMANCER, "void spell_wind_blade(")
callback = function_body(ETHERMANCER, "static void wind_blade_grant_completed(")
wear = function_body(ACTOBJ, "int wear(P_char")
reconcile = function_body(MOVEMENT, "bool reconcile_creation_grant_batch(")

assert "item_creation_grant_submit_to_player_with_completion" in grant
assert "obj_to_char(blade, ch)" not in grant
assert "wear(ch, blade, 12, TRUE)" not in grant
assert "wind_blade_grant_completed" in grant
assert "grant_wind_blade(ch);" in spell and "return;" in spell
assert "if (!committed)" in callback
assert callback.index("find_carried_object_by_uid") < callback.index("wear(actor, blade, 12, TRUE)")
assert "OBJ_CARRIED_BY(obj_object, ch)" in wear
assert wear.index("OBJ_CARRIED_BY(obj_object, ch)") < wear.index("can_equip_soulbound_item")
assert "creation_grant_request_live_ready(actor, request)" in reconcile
for token in ("kind=%s", "uid=%llu", "vnum=%d", "loc_p=%u", "carrier_pid=%u",
              "wearer_pid=%u", "container_uid=%llu", "room=%d"):
    assert token in MOVEMENT, token

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "item/item_movement_transaction.h"

static int wear_calls = 0;
static std::string output;

void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *text, P_char) { output += text ? text : ""; }
void logit(const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) {
    std::abort();
}

bool has_wind_blade_wielded(P_char ch) {
    return ch && ch->equipment[PRIMARY_WEAPON] != nullptr;
}
void wind_blade_attack_routine(P_char, P_char) {}
int wear(P_char ch, P_obj object, int, bool) {
    ++wear_calls;
    ch->carrying = object->next_content;
    object->next_content = nullptr;
    object->loc_p = LOC_WORN;
    object->loc.wearing = ch;
    ch->equipment[PRIMARY_WEAPON] = object;
    return TRUE;
}
'''

DRIVER = r'''
static wind_blade_grant_context context_for(uint64_t uid) {
    wind_blade_grant_context context = {};
    context.item_uid = uid;
    return context;
}

int main() {
    char_data actor{};
    pc_only_data player_data{};
    actor.only.pc = &player_data;
    obj_data blade{};
    actor.carrying = nullptr;
    blade.obj_uid = 88001;
    blade.loc_p = LOC_NOWHERE;

    auto failed = context_for(blade.obj_uid);
    output.clear();
    wind_blade_grant_completed(&actor, false, {}, 0,
                               reinterpret_cast<const uint8_t *>(&failed), sizeof(failed));
    assert(wear_calls == 0);
    assert(actor.equipment[PRIMARY_WEAPON] == nullptr);

    // A successful publication links the exact UID into carrying before the
    // callback runs. The callback wears it once and does not duplicate it.
    actor = {};
    actor.only.pc = &player_data;
    blade = {};
    blade.obj_uid = 88001;
    blade.loc_p = LOC_CARRIED;
    blade.loc.carrying = &actor;
    actor.carrying = &blade;
    wear_calls = 0;
    output.clear();
    auto committed = context_for(blade.obj_uid);
    wind_blade_grant_completed(&actor, true, {}, 0,
                               reinterpret_cast<const uint8_t *>(&committed), sizeof(committed));
    assert(wear_calls == 1);
    assert(actor.equipment[PRIMARY_WEAPON] == &blade);
    assert(actor.carrying == nullptr);
    wind_blade_grant_completed(&actor, true, {}, 0,
                               reinterpret_cast<const uint8_t *>(&committed), sizeof(committed));
    assert(wear_calls == 1);

    // If another weapon arrives while the grant is in flight, the blade stays
    // carried and the callback does not replace the player's new weapon.
    actor = {};
    actor.only.pc = &player_data;
    blade = {};
    obj_data other{};
    blade.obj_uid = 88002;
    blade.loc_p = LOC_CARRIED;
    blade.loc.carrying = &actor;
    actor.carrying = &blade;
    actor.equipment[PRIMARY_WEAPON] = &other;
    wear_calls = 0;
    output.clear();
    auto occupied = context_for(blade.obj_uid);
    wind_blade_grant_completed(&actor, true, {}, 0,
                               reinterpret_cast<const uint8_t *>(&occupied), sizeof(occupied));
    assert(wear_calls == 0);
    assert(actor.equipment[PRIMARY_WEAPON] == &other);
    assert(actor.carrying == &blade);

    std::puts("Wind Blade completion callback and detached-object guard passed.");
}
'''

out_dir = ROOT / "bin/tests/wind-blade"
out_dir.mkdir(parents=True, exist_ok=True)
harness = out_dir / "harness.cpp"
harness.write_text(
    PRELUDE
    + "\n"
    + "struct wind_blade_grant_context\n{\n    uint64_t item_uid;\n};\n"
    + extract_function("ethermancer.c", "static P_obj find_carried_object_by_uid(")
    + "\n"
    + callback
    + DRIVER,
    encoding="utf-8",
)
binary = out_dir / "harness"
subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-fsanitize=address,undefined",
        "-I" + str(ROOT / "src"),
        str(harness),
        "-o",
        str(binary),
    ],
    check=True,
)
subprocess.run([str(binary)], check=True)
