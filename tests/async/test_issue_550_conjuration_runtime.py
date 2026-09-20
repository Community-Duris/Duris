#!/usr/bin/env python3
"""ASan/UBSan callback harness for issue 550 commit-aware grants."""

import subprocess

from _paths import ROOT, extract_function, source


MAGIC = source("magic.c").read_text(encoding="utf-8", errors="replace")
SKILLS = source("classes/new_skills.c").read_text(encoding="utf-8", errors="replace")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "combat/damage.h"
#include "item/item_movement_transaction.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

static index_data object_index_storage[1]{};
P_index obj_index = object_index_storage;
P_obj object_list = nullptr;
P_char character_list = nullptr;

static std::string output;
static int act_calls = 0;
static int damage_calls = 0;
static double damage_amount = 0.0;
static int binding_vnum = 9001;

#undef IS_ALIVE
#define IS_ALIVE(ch) ((ch) != nullptr)

void act(const char *, int, P_char, P_obj, void *, int) { ++act_calls; }
void send_to_char(const char *text, P_char) { output += text ? text : ""; }
void logit(const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
int spell_damage(P_char, P_char, double dam, int, uint, damage_messages *, int *) {
    ++damage_calls;
    damage_amount += dam;
    return 0;
}
bool kingdom_store_bound(P_obj) { return false; }
bool isname(const char *needle, const char *haystack) {
    return needle && haystack && std::strstr(haystack, needle) != nullptr;
}

void extract_obj(P_obj object, int) {
    if (!object) return;
    P_char owner = OBJ_CARRIED(object) ? object->loc.carrying : nullptr;
    if (owner) {
        if (owner->carrying == object) owner->carrying = object->next_content;
        else for (P_obj cursor = owner->carrying; cursor; cursor = cursor->next_content)
            if (cursor->next_content == object) {
                cursor->next_content = object->next_content;
                break;
            }
    }
    object->next_content = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.carrying = nullptr;
}

'''

FUNCTIONS = "\n".join(
    [
        "enum class conjured_weapon_kind : uint8_t { ensis_unguis, lancea_cineralae, simulacrum_anguis };",
        "struct conjured_weapon_grant_context { uint64_t item_uid; uint32_t actor_pid; float self_damage; uint8_t kind; };",
        "enum class summoned_replacement_kind : uint8_t { book, totem };",
        "struct summoned_replacement_context { uint64_t item_uid; uint32_t recipient_pid; uint8_t kind; };",
        "struct soulbind_reload_context { uint64_t item_uid; uint32_t recipient_pid; int32_t item_vnum; };",
        "int has_soulbind(P_char) { return binding_vnum; }",
        extract_function("magic.c", "static P_obj magic_find_object_by_uid("),
        extract_function("magic.c", "static void conjured_weapon_publish_effect("),
        extract_function("magic.c", "static void conjured_weapon_grant_completed("),
        extract_function("classes/new_skills.c", "static P_obj new_skills_find_object_by_uid("),
        extract_function("classes/new_skills.c", "static bool summoned_book_matches("),
        extract_function("classes/new_skills.c", "static bool summoned_totem_matches("),
        extract_function("classes/new_skills.c", "static void retire_other_summoned_items("),
        extract_function("classes/new_skills.c", "static void summoned_replacement_completed("),
        extract_function("magic.c", "static void remove_soulbind_except("),
        extract_function("magic.c", "static void soulbind_reload_completed("),
    ]
)

DRIVER = r'''
static void setup_actor(char_data &actor, pc_only_data &pc) {
    actor = {};
    pc = {};
    actor.only.pc = &pc;
    pc.pid = 101;
    static char name[] = "tester";
    actor.player.name = name;
}

static void link_object(obj_data &object, uint64_t uid, P_char owner,
                        const char *name, ulong extra2 = 0) {
    object = {};
    object.obj_uid = uid;
    object.name = const_cast<char *>(name);
    object.extra2_flags = extra2;
    object.loc_p = owner ? LOC_CARRIED : LOC_NOWHERE;
    object.loc.carrying = owner;
    object.next_content = owner ? owner->carrying : nullptr;
    if (owner) owner->carrying = &object;
}

static void link_object_list(P_obj first, P_obj second = nullptr, P_obj third = nullptr) {
    first->next = second;
    if (second) second->next = third;
    if (third) third->next = nullptr;
    object_list = first;
}

static int active_named(P_obj first, P_obj second, P_obj third = nullptr) {
    int count = 0;
    for (P_obj object : {first, second, third})
        if (object && !OBJ_NOWHERE(object)) ++count;
    return count;
}

int main() {
    char_data actor{};
    pc_only_data pc{};
    obj_data blade{};
    setup_actor(actor, pc);
    link_object(blade, 1001, &actor, "ensis blade");
    link_object_list(&blade);

    conjured_weapon_grant_context weapon = {blade.obj_uid, 101, 10.0f, 0};
    output.clear();
    act_calls = damage_calls = 0;
    damage_amount = 0.0;
    conjured_weapon_grant_completed(&actor, false, {}, 7,
        reinterpret_cast<const uint8_t *>(&weapon), sizeof(weapon));
    assert(damage_calls == 0);
    assert(act_calls == 0);
    assert(output.find("no health was spent") != std::string::npos);

    output.clear();
    act_calls = damage_calls = 0;
    damage_amount = 0.0;
    conjured_weapon_grant_completed(&actor, true, {}, 0,
        reinterpret_cast<const uint8_t *>(&weapon), sizeof(weapon));
    assert(damage_calls == 1);
    assert(damage_amount == 10.0);
    assert(act_calls == 2);

    // A committed callback with a missing UID never dereferences a stale object.
    output.clear();
    act_calls = damage_calls = 0;
    conjured_weapon_grant_context missing = {9999, 101, 10.0f, 0};
    conjured_weapon_grant_completed(&actor, true, {}, 0,
        reinterpret_cast<const uint8_t *>(&missing), sizeof(missing));
    assert(damage_calls == 0 && act_calls == 0);

    // Book failure keeps the old copy; success publishes the new copy then retires old.
    obj_data old_book{}, new_book{};
    setup_actor(actor, pc);
    link_object(old_book, 2001, &actor, "book spellbook bookoftester");
    link_object(new_book, 2002, nullptr, "book spellbook bookoftester");
    new_book.loc_p = LOC_NOWHERE;
    link_object_list(&new_book, &old_book);
    summoned_replacement_context book = {new_book.obj_uid, 101, 0};
    output.clear();
    summoned_replacement_completed(&actor, false, {}, 4,
        reinterpret_cast<const uint8_t *>(&book), sizeof(book));
    assert(!OBJ_NOWHERE(&old_book));
    assert(output.find("existing spellbook was kept") != std::string::npos);

    new_book.loc_p = LOC_CARRIED;
    new_book.loc.carrying = &actor;
    new_book.next_content = actor.carrying;
    actor.carrying = &new_book;
    output.clear();
    summoned_replacement_completed(&actor, true, {}, 0,
        reinterpret_cast<const uint8_t *>(&book), sizeof(book));
    assert(OBJ_NOWHERE(&old_book));
    assert(!OBJ_NOWHERE(&new_book));
    assert(active_named(&old_book, &new_book) == 1);
    assert(output.find("materializes") != std::string::npos);

    // Totem cleanup uses the vnum/name identity and also leaves one live copy.
    obj_data old_totem{}, new_totem{};
    setup_actor(actor, pc);
    object_index_storage[0].virtual_number = 417;
    old_totem.R_num = new_totem.R_num = 0;
    link_object(old_totem, 3001, &actor, "totem spirit totem spirit tester");
    link_object(new_totem, 3002, &actor, "totem spirit totem spirit tester");
    link_object_list(&new_totem, &old_totem);
    summoned_replacement_context totem = {new_totem.obj_uid, 101, 1};
    summoned_replacement_completed(&actor, true, {}, 0,
        reinterpret_cast<const uint8_t *>(&totem), sizeof(totem));
    assert(OBJ_NOWHERE(&old_totem));
    assert(!OBJ_NOWHERE(&new_totem));

    // Soulbind failure preserves the old item; commit keeps exactly the new UID.
    obj_data old_soul{}, new_soul{};
    setup_actor(actor, pc);
    link_object(old_soul, 4001, &actor, "tester old soul", ITEM2_SOULBIND);
    link_object(new_soul, 4002, &actor, "tester new soul", ITEM2_SOULBIND);
    link_object_list(&new_soul, &old_soul);
    soulbind_reload_context soul = {new_soul.obj_uid, 101, binding_vnum};
    output.clear();
    soulbind_reload_completed(&actor, false, {}, 3,
        reinterpret_cast<const uint8_t *>(&soul), sizeof(soul));
    assert(!OBJ_NOWHERE(&old_soul));
    assert(output.find("existing soulbound item was kept") != std::string::npos);

    output.clear();
    soulbind_reload_completed(&actor, true, {}, 0,
        reinterpret_cast<const uint8_t *>(&soul), sizeof(soul));
    assert(OBJ_NOWHERE(&old_soul));
    assert(!OBJ_NOWHERE(&new_soul));
    assert(output.find("feel") != std::string::npos);

    std::puts("Issue 550 commit-aware callbacks passed under ASan/UBSan.");
}
'''

out_dir = ROOT / "bin/tests/issue-550-conjuration"
out_dir.mkdir(parents=True, exist_ok=True)
harness = out_dir / "harness.cpp"
harness.write_text(PRELUDE + FUNCTIONS + DRIVER, encoding="utf-8")
binary = out_dir / "harness"
subprocess.run(
    [
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-I" + str(ROOT / "src"),
        str(harness), "-o", str(binary),
    ],
    check=True,
)
subprocess.run([str(binary)], check=True)
