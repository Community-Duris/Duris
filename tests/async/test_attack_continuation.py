#!/usr/bin/env python3
"""Exercise the checked combat continuation contract under destructive callbacks."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, source


PREFIX = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include <cassert>
#include <cstdio>

static char_data actor = {}, target = {};
static pc_only_data actor_pc = {}, target_pc = {};
static obj_data weapon = {}, replacement = {};
static bool actor_listed = true, target_listed = true;
P_obj object_list = nullptr;

P_char find_character_by_runtime_id(uint64_t id) {
    if (actor_listed && actor.runtime_id == id) return &actor;
    if (target_listed && target.runtime_id == id) return &target;
    return nullptr;
}

static void reset() {
    actor = {};
    target = {};
    weapon = {};
    replacement = {};
    actor.only.pc = &actor_pc;
    target.only.pc = &target_pc;
    actor.runtime_id = 10;
    target.runtime_id = 20;
    actor.in_room = target.in_room = 1;
    actor.specials.z_cord = target.specials.z_cord = 0;
    SET_POS(&actor, STAT_NORMAL + POS_STANDING);
    SET_POS(&target, STAT_NORMAL + POS_STANDING);
    weapon.obj_uid = 100;
    replacement.obj_uid = 200;
    weapon.next = &replacement;
    replacement.next = nullptr;
    object_list = &weapon;
    actor.equipment[PRIMARY_WEAPON] = &weapon;
    actor_listed = target_listed = true;
}

static attack_continuation guard() {
    return begin_attack_continuation(&actor, &target, &weapon, PRIMARY_WEAPON);
}

static void expect(attack_continuation guard, attack_continuation_outcome outcome) {
    const attack_continuation_result checked = check_attack_continuation(guard);
    assert(checked.outcome == outcome);
    if (outcome == attack_continuation_outcome::continue_attack) {
        assert(checked.actor == &actor);
        assert(checked.target == &target);
        assert(checked.weapon == &weapon);
    }
}
'''

SUFFIX = r'''
int main() {
    expect(begin_attack_continuation(nullptr, &target),
           attack_continuation_outcome::cancelled);
    expect(begin_attack_continuation(&actor, nullptr),
           attack_continuation_outcome::cancelled);
    reset();
    expect(guard(), attack_continuation_outcome::continue_attack);

    auto captured = guard();
    actor_listed = false;
    expect(captured, attack_continuation_outcome::actor_gone);
    reset();
    captured = guard();
    captured.weapon_slot = MAX_WEAR;
    expect(captured, attack_continuation_outcome::cancelled);

    reset();
    captured = guard();
    SET_POS(&actor, STAT_DEAD);
    expect(captured, attack_continuation_outcome::actor_gone);
    reset();
    captured = guard();
    ++actor.runtime_id;
    expect(captured, attack_continuation_outcome::actor_gone);

    reset();
    captured = guard();
    target_listed = false;
    expect(captured, attack_continuation_outcome::target_gone);
    reset();
    captured = guard();
    SET_POS(&target, STAT_DEAD);
    expect(captured, attack_continuation_outcome::target_gone);
    reset();
    captured = guard();
    ++target.runtime_id;
    expect(captured, attack_continuation_outcome::target_gone);

    reset();
    captured = guard();
    ++actor.in_room;
    expect(captured, attack_continuation_outcome::relocated);
    reset();
    captured = guard();
    ++target.specials.z_cord;
    expect(captured, attack_continuation_outcome::relocated);

    reset();
    captured = guard();
    actor.equipment[PRIMARY_WEAPON] = nullptr;
    expect(captured, attack_continuation_outcome::weapon_changed);
    reset();
    captured = guard();
    actor.equipment[PRIMARY_WEAPON] = &replacement;
    expect(captured, attack_continuation_outcome::weapon_changed);
    reset();
    captured = guard();
    ++weapon.obj_uid;
    expect(captured, attack_continuation_outcome::weapon_changed);

    // An unrelated removal does not cancel a valid continuation.
    reset();
    actor_listed = target_listed = true;
    expect(guard(), attack_continuation_outcome::continue_attack);

    puts("combat continuation outcomes and live-pointer revalidation passed");
}
'''


vicious_section = source("fight.c").read_text(encoding="utf-8")
vicious_section = vicious_section[
    vicious_section.index("if (GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK)") :
    vicious_section.index("/* calculate the damage */", vicious_section.index(
        "if (GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK)"
    ))
]
assert "begin_attack_continuation" in vicious_section
assert "check_attack_continuation" in vicious_section
assert "is_char_in_room(ch, room)" not in vicious_section

with tempfile.TemporaryDirectory(prefix="duris-attack-continuation-") as temporary:
    source_path = Path(temporary) / "attack_continuation.cpp"
    binary = Path(temporary) / "attack_continuation"
    source_path.write_text(PREFIX + SUFFIX)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-g",
            "-O0",
            "-D__NO_MYSQL__",
            "-Isrc",
            "-Isrc/no_mysql",
            "-I/usr/include/libxml2",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-fno-pie",
            "-no-pie",
            str(source_path),
            str(ROOT / "src" / "combat" / "attack_continuation.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=30)
