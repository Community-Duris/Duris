#!/usr/bin/env python3
"""Production fire/gather commands must account for worn quiver weight once."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function


handler_c = (ROOT / "src/world/handler.c").read_text()
# Include the complete production weight implementation, including its local helpers.
weight_function = extract_function("world/handler.c", "void add_weight(P_obj obj, int weight)\n{")
weight_start = handler_c.index("// A little more complicated than it would first seem.")
weight_code = handler_c[weight_start:handler_c.index(weight_function) + len(weight_function)]

handler = "\n".join(
    weight_code if signature.startswith("void add_weight") else
    extract_function("world/handler.c", signature) for signature in (
    "static bool obj_is_container_type(",
    "static int obj_prototype_weight(",
    "static int container_weight_reduction_pct(",
    "static int sum_direct_contents_weight(",
    "static void encumbrance_adjust(",
    "void recalc_container_weight(",
    "static void resync_reducing_container(",
    "void add_weight(P_obj obj, int weight)\n{",
    "bool obj_can_nest(",
    "bool obj_is_in_container(",
    "void obj_to_obj(",
    "void obj_from_obj(",
))
commands = "\n".join(extract_function("combat/range.c", signature) for signature in (
    "void do_fire(", "void do_gather(",
))
harness = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static room_data rooms[1] = {};
P_room world = rooms;
static index_data indices[1] = {};
P_index obj_index = indices;
int top_of_objt = 0;
const char *dirs[] = {"north"};
const char *dirs2[] = {"the north"};
extern const int rev_dir[];
const int rev_dir[] = {0};
static char_data target;
static bool cursed;
static int fired;

// Only misses in one room are relevant. Unexpected combat services abort.
int GET_CHAR_SKILL_P(P_char, int skill) { return skill == SKILL_ARCHERY ? 100 : 0; }
bool notch_skill(P_char, int skill, float) { return skill == SKILL_CURSED_ARROWS && cursed; }
int number(int, int high) { return high; }
int dice(int, int) { std::abort(); }
int get_property(const char *, int fallback) { return fallback; }
float get_property(const char *, double fallback) { return fallback; }
int chance_to_hit(P_char, P_char, int, P_obj) { return 0; }
P_char misfire_check(P_char, P_char victim, int) { return victim; }
P_char get_char_room_vis(P_char, const char *) { return &target; }
P_char get_char_ranged(const char *, P_char, int, int) { std::abort(); }
int dir_from_keyword(char *) { std::abort(); }
int check_wall(int, int) { std::abort(); }
bool AdjacentInRoom(P_char, P_char) { std::abort(); }
bool is_natural_mount(P_char, P_char) { std::abort(); }
void stop_riding(P_char) { std::abort(); }
room_affect *get_spell_from_room(P_room, int) { return nullptr; }
void CharWait(P_char, int) {}
void startPvP(P_char, bool) { std::abort(); }
void set_short_affected_by(P_char, int, int) { std::abort(); }
int enchant_arrows(P_char, P_char, P_obj, int) { std::abort(); }
void MakeScrap(P_char, P_obj) { std::abort(); }
int melee_damage(P_char, P_char, double, int, damage_messages *, int *) { std::abort(); }
void MobRetaliateRange(P_char, P_char) { std::abort(); }
void obj_to_char(P_obj, P_char) { std::abort(); }
void affect_from_char(P_char, int) { std::abort(); }
int BOUNDED(int, int, int) { std::abort(); }
int checked_snprintf_at(const char *, int, char *, size_t, const char *, ...) { std::abort(); }
void half_chop(char *input, char *first, char *second) {
    assert(!strcmp(input, "target"));
    strcpy(first, input);
    *second = '\0';
}
char *one_argument(const char *input, char *word) {
    assert(!*input);
    *word = '\0';
    return const_cast<char *>(input);
}
int generic_find(const char *, int, P_char, P_char *, P_obj *) { std::abort(); }
void act(const char *message, int, P_char, P_obj, void *, int) {
    if (!strcmp(message, "You fire $p at $N and miss!")) ++fired;
}
P_obj read_object(int, int) { std::abort(); }
void extract_obj(P_obj, int) { std::abort(); }
static void mark_container_dirty(P_obj) {}
void artifact_update_location_sql(P_obj) {}
void writeSavedItem(P_obj) { std::abort(); }
P_char training_dummy_item_owner(P_obj) { return nullptr; }
void add_weight(P_obj, int);
void world_activity_object_enter(P_obj) {}
void world_activity_object_leave(P_obj) {}

void obj_to_room(P_obj object, int room) {
    assert(room == 0 && object->loc_p == LOC_NOWHERE);
    object->loc_p = LOC_ROOM;
    object->loc.room = room;
    object->next_content = rooms[room].contents;
    rooms[room].contents = object;
}
void obj_from_room(P_obj object) {
    assert(object->loc_p == LOC_ROOM && object->loc.room == 0);
    P_obj *link = &rooms[0].contents;
    while (*link != object) { assert(*link); link = &(*link)->next_content; }
    *link = object->next_content;
    object->next_content = nullptr;
    object->loc_p = LOC_NOWHERE;
}
// Match the equip/unequip contribution while the real gather command reloads.
void equip_char(P_char ch, P_obj object, int slot, int) {
    assert(!ch->equipment[slot] && object->loc_p == LOC_NOWHERE);
    ch->equipment[slot] = object;
    object->loc_p = LOC_WORN;
    object->loc.wearing = ch;
    GET_CARRYING_W(ch) += GET_OBJ_WEIGHT(object) / 2;
}
P_obj unequip_char(P_char ch, int slot, bool) {
    P_obj object = ch->equipment[slot];
    assert(object && object->loc_p == LOC_WORN);
    GET_CARRYING_W(ch) -= GET_OBJ_WEIGHT(object) / 2;
    ch->equipment[slot] = nullptr;
    object->loc_p = LOC_NOWHERE;
    return object;
}

#define ARROW_MARK 1
// PRODUCTION

static void scenario(int shell, int missile_weight, int requested, int loaded, bool returning) {
    char_data shooter = {};
    pc_only_data player = {};
    npc_only_data npc = {};
    shooter.only.pc = &player;
    player.pid = 42;
    shooter.specials.position = POS_STANDING | STAT_NORMAL;
    shooter.curr_stats.Dex = 100;
    shooter.curr_stats.Str = 100;
    target = {};
    target.only.npc = &npc;
    target.specials.act = ACT_ISNPC;
    target.specials.position = POS_STANDING | STAT_NORMAL;
    obj_data bow = {}, quiver = {}, arrows[3] = {};
    bow.type = ITEM_FIREWEAPON;
    bow.value[0] = requested * 30;
    bow.value[1] = 100;
    shooter.equipment[WIELD] = &bow;
    quiver.type = ITEM_QUIVER;
    quiver.loc_p = LOC_NOWHERE;
    quiver.weight = shell;
    quiver.value[0] = 10;
    quiver.value[3] = loaded;
    for (int i = 0; i < loaded; ++i) {
        arrows[i].type = ITEM_MISSILE;
        arrows[i].loc_p = LOC_NOWHERE;
        arrows[i].weight = missile_weight;
        obj_to_obj(&arrows[i], &quiver);
    }
    GET_CARRYING_W(&shooter) = 100;
    equip_char(&shooter, &quiver, WEAR_QUIVER, FALSE);
    const int initial_weight = quiver.weight;
    const int initial_carried = GET_CARRYING_W(&shooter);
    cursed = returning;
    for (int cycle = 0; cycle < 50; ++cycle) {
        fired = 0;
        char target_name[] = "target";
        do_fire(&shooter, target_name, CMD_FIRE);
        const int removed = returning ? 0 : MIN(requested, loaded);
        assert(fired == (returning ? requested : removed));
        assert(quiver.value[3] == loaded - removed);
        assert(quiver.weight == initial_weight - removed * MAX(0, missile_weight));
        assert(GET_CARRYING_W(&shooter) == 100 + GET_OBJ_WEIGHT(&quiver) / 2);
        char gather[] = "";
        do_gather(&shooter, gather, 0);
        assert(!rooms[0].contents && quiver.value[3] == loaded);
        assert(quiver.weight == initial_weight && GET_CARRYING_W(&shooter) == initial_carried);
    }
}

int main() {
    for (int shell : {4, 5})
        for (int weight : {-1, 0, 1, 2})
            for (int shots : {1, 2, 3})
                for (int loaded : {1, 3})
                    for (bool returning : {false, true})
                        scenario(shell, weight, shots, loaded, returning);
    puts("fire/gather quiver encumbrance: 96 scenarios, 50 cycles each passed");
}
'''.replace("// PRODUCTION", handler + "\n" + commands)

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="fired-missile-weight-", dir=build) as temporary:
    source = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-ffunction-sections", "-fdata-sections", "-I", str(ROOT / "src"),
        str(source), str(HARNESS_STUBS), "-Wl,--gc-sections", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
