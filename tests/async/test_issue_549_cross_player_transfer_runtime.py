#!/usr/bin/env python3
"""Sanitizer harness for issue 549's held-publication callbacks.

The harness exercises terminal refusal, committed publication, and callback replay
for both commands.  It deliberately models the live UID lists and keeps all
post-commit logging/message paths linked under ASan/UBSan.
"""

import subprocess

from _paths import ROOT, extract_function, source


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "item/item_movement_transaction.h"
#include "net/comm.h"
#include "persistence/persistence_checkpoint.h"
#include "magic/spells.h"
#include "kingdom/kingdom_store_piece.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

static index_data object_index_storage[1]{};
P_index obj_index = object_index_storage;
P_char character_list = nullptr;
P_obj object_list = nullptr;

static std::string output;
static int alert_count = 0;
static int act_count = 0;
static int notch_count = 0;
static int save_count = 0;
static bool save_succeeds = true;

#undef IS_TRUSTED
#define IS_TRUSTED(ch) true
#undef GET_LEVEL
#define GET_LEVEL(ch) 1
#undef CAN_CARRY_W
#define CAN_CARRY_W(ch) 100000
#ifndef WIZLOG
#define WIZLOG "wiz"
#endif
#ifndef SKILL_SLIP
#define SKILL_SLIP 0
#endif

void persistence_alert(int, const char *, const char *, const char *, const char *,
                       const char *, const char *, ...) { ++alert_count; }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
bool isname(const char *, const char *) { return false; }
bool kingdom_store_bound(P_obj) { return false; }
void send_to_char(const char *text, P_char) { output += text ? text : ""; }
void act(const char *text, int, P_char, P_obj, void *, int) {
    if (text) ++act_count;
}
void statuslog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
int writeCharacter(P_char, int, int) { ++save_count; return 1; }
bool do_save_silent(P_char, int) { ++save_count; return save_succeeds; }
bool notch_skill(P_char, int, float) { ++notch_count; return true; }
int char_light(P_char) { return 0; }
int room_light(int, int) { return 0; }
int nq_action_check(P_char, P_char, char *) { return 0; }
int total_carried_weight(P_char) { return 0; }
int get_property(const char *, int fallback) { return fallback; }
void mark_player_dirty_components(int, player_component_mask_t) {}

affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected;
    ch->affected = copy;
    return copy;
}
void affect_remove(P_char ch, affected_type *af) {
    if (!ch || !af) return;
    if (ch->affected == af) ch->affected = af->next;
    else for (auto *cursor = ch->affected; cursor; cursor = cursor->next)
        if (cursor->next == af) { cursor->next = af->next; break; }
    delete af;
}
void str_free(const char *text) { std::free(const_cast<char *>(text)); }
char *str_dup(const char *text) {
    if (!text) return nullptr;
    const size_t length = std::strlen(text) + 1;
    auto *copy = static_cast<char *>(std::malloc(length));
    std::memcpy(copy, text, length);
    return copy;
}
void set_short_description(P_obj object, const char *text) {
    if (object->short_description) std::free(object->short_description);
    object->short_description = str_dup(text);
}

void extract_obj(P_obj object, int) {
    if (!object) return;
    object->loc_p = LOC_NOWHERE;
    object->loc.carrying = nullptr;
}

void obj_from_char(P_obj object) {
    if (!object || !object->loc.carrying) return;
    P_char owner = object->loc.carrying;
    if (owner->carrying == object) owner->carrying = object->next_content;
    else {
        for (P_obj cursor = owner->carrying; cursor; cursor = cursor->next_content)
            if (cursor->next_content == object) {
                cursor->next_content = object->next_content;
                break;
            }
    }
    object->next_content = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.carrying = nullptr;
}

void obj_to_char(P_obj object, P_char owner) {
    assert(object && owner);
    assert(object->loc_p == LOC_NOWHERE);
    object->next_content = owner->carrying;
    owner->carrying = object;
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = owner;
}

'''

MAGIC = source("magic.c").read_text(encoding="utf-8")
ROGUES = source("classes/rogues.c").read_text(encoding="utf-8")

FUNCTIONS = "\n".join(
    [
        r'''
struct soulbind_movement_context {
    uint64_t item_uid;
    uint32_t source_pid;
    uint32_t victim_pid;
    int32_t source_room;
    int32_t victim_room;
    uint8_t replace_existing;
};
struct slip_movement_context {
    uint64_t item_uid;
    uint32_t source_pid;
    uint32_t victim_pid;
    int32_t source_room;
    int32_t victim_room;
};
''',
        extract_function("magic.c", "int has_soulbind("),
        extract_function("magic.c", "static void remove_soulbind_except("),
        extract_function("magic.c", "void remove_soulbind("),
        extract_function("magic.c", "static P_char find_soulbind_player("),
        extract_function("magic.c", "static P_obj find_soulbind_item("),
        extract_function("magic.c", "static bool soulbind_metadata_applied("),
        extract_function("magic.c", "static bool apply_soulbind_metadata("),
        extract_function("magic.c", "static bool soulbind_transfer_publication("),
        extract_function("classes/rogues.c", "static P_char find_slip_player("),
        extract_function("classes/rogues.c", "static P_obj find_slip_item("),
        extract_function("classes/rogues.c", "static bool slip_transfer_publication("),
    ]
)

DRIVER = r'''
static void link_players(char_data &source, pc_only_data &source_pc,
                         char_data &victim, pc_only_data &victim_pc) {
    while (victim.affected) {
        auto *next = victim.affected->next;
        delete victim.affected;
        victim.affected = next;
    }
    source = {};
    victim = {};
    source.only.pc = &source_pc;
    victim.only.pc = &victim_pc;
    source_pc = {};
    victim_pc = {};
    source_pc.pid = 101;
    victim_pc.pid = 202;
    static char source_name[] = "source";
    static char victim_name[] = "victim";
    source.player.name = source_name;
    victim.player.name = victim_name;
    source.next = &victim;
    character_list = &source;
}

static void link_item(char_data &source, obj_data &object, uint64_t uid) {
    if (object.name) std::free(object.name);
    if (object.short_description) std::free(object.short_description);
    object = {};
    object.obj_uid = uid;
    object.R_num = 0;
    object.weight = 1;
    object.loc_p = LOC_CARRIED;
    object.loc.carrying = &source;
    object.str_mask = STRUNG_KEYS;
    object.name = str_dup("test item");
    object.short_description = str_dup("a test item");
    source.carrying = &object;
    object_list = &object;
}

int main() {
    object_index_storage[0].virtual_number = 9001;
    char_data source{}, victim{};
    pc_only_data source_pc{}, victim_pc{};
    obj_data object{};
    link_players(source, source_pc, victim, victim_pc);
    link_item(source, object, 77001);

    soulbind_movement_context soulbind = { object.obj_uid, 101, 202, 0, 0, 0 };
    item_transfer_result soulbind_result{};
    soulbind_result.root_item_uid = object.obj_uid;
    output.clear();
    act_count = alert_count = save_count = 0;
    assert(soulbind_transfer_publication(&source, false, {}, 5,
        reinterpret_cast<const uint8_t *>(&soulbind), sizeof(soulbind)));
    assert(source.carrying == &object);
    assert(victim.carrying == nullptr);
    assert(has_soulbind(&victim) == 0);
    assert(act_count == 0 && save_count == 0);
    assert(output.find("did not commit") != std::string::npos);

    item_transfer_result mismatched{};
    mismatched.root_item_uid = object.obj_uid + 1;
    output.clear();
    act_count = alert_count = save_count = 0;
    assert(!soulbind_transfer_publication(&source, true, mismatched, 0,
        reinterpret_cast<const uint8_t *>(&soulbind), sizeof(soulbind)));
    assert(source.carrying == &object && victim.carrying == nullptr);
    assert(has_soulbind(&victim) == 0);
    assert(alert_count == 1 && act_count == 0 && save_count == 0);

	// A committed ownership move must still publish if forced movement changed
	// either live room while the asynchronous command was in flight.
	source.in_room = 7;
	victim.in_room = 8;
    output.clear();
    act_count = alert_count = save_count = 0;
    save_succeeds = false;
    assert(!soulbind_transfer_publication(&source, true, soulbind_result, 0,
        reinterpret_cast<const uint8_t *>(&soulbind), sizeof(soulbind)));
    assert(source.carrying == nullptr);
    assert(victim.carrying == &object);
    assert(has_soulbind(&victim) == 9001);
    assert((object.extra2_flags & ITEM2_SOULBIND) != 0);
    assert(act_count == 2 && save_count >= 1);
    const auto soulbind_output = output;
    save_succeeds = true;
    assert(soulbind_transfer_publication(&source, true, soulbind_result, 0,
        reinterpret_cast<const uint8_t *>(&soulbind), sizeof(soulbind)));
    assert(output == soulbind_output);
    assert(has_soulbind(&victim) == 9001);

    // Slip uses a fresh original UID and must not report/notch on refusal.
    link_players(source, source_pc, victim, victim_pc);
    link_item(source, object, 77002);
    slip_movement_context slip = { object.obj_uid, 101, 202, 0, 0 };
    item_transfer_result slip_result{};
    slip_result.root_item_uid = object.obj_uid;
    output.clear();
    act_count = alert_count = notch_count = save_count = 0;
    assert(slip_transfer_publication(&source, false, {}, 5,
        reinterpret_cast<const uint8_t *>(&slip), sizeof(slip)));
    assert(source.carrying == &object && victim.carrying == nullptr);
    assert(notch_count == 0 && act_count == 0 && save_count == 0);
	// A destination-only retry of a rejected secret handoff must not disclose
	// the Slip attempt to its intended victim.
	character_list = &victim;
	output.clear();
	assert(slip_transfer_publication(&victim, false, {}, 5,
		reinterpret_cast<const uint8_t *>(&slip), sizeof(slip)));
	assert(output.empty());
	character_list = &source;

    mismatched = {};
    mismatched.root_item_uid = object.obj_uid + 1;
    assert(!slip_transfer_publication(&source, true, mismatched, 0,
        reinterpret_cast<const uint8_t *>(&slip), sizeof(slip)));
    assert(source.carrying == &object && victim.carrying == nullptr);
    assert(notch_count == 0 && act_count == 0 && save_count == 0);

	source.in_room = 9;
	victim.in_room = 10;
    output.clear();
    act_count = alert_count = notch_count = save_count = 0;
    assert(slip_transfer_publication(&source, true, slip_result, 0,
        reinterpret_cast<const uint8_t *>(&slip), sizeof(slip)));
    assert(source.carrying == nullptr && victim.carrying == &object);
    assert(notch_count == 1 && save_count == 2);
    assert(act_count == 1);
    const auto slip_output = output;
    assert(slip_transfer_publication(&source, true, slip_result, 0,
        reinterpret_cast<const uint8_t *>(&slip), sizeof(slip)));
    assert(output == slip_output);
    assert(notch_count == 1 && act_count == 1);

    while (victim.affected) {
        auto *next = victim.affected->next;
        delete victim.affected;
        victim.affected = next;
    }
    std::free(object.name);
    std::free(object.short_description);
    std::puts("Soulbind and Slip held-publication callbacks passed under ASan/UBSan.");
}
'''

out_dir = ROOT / "bin/tests/issue-549"
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
