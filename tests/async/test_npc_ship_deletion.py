#!/usr/bin/env python3
"""Issue #110: real sinking/purge/unload paths, hash and identities under sanitizers.

Only external world/persistence services and unrelated ship mechanics are stubbed.
Persistence failures are injected for both compiled backends; no database or live
server is used. The production heartbeat and teardown functions run unmodified.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function

FUNCTIONS = "\n\n".join(
    extract_function(file, signature)
    for file, signatures in (
        ("ship_utils.c", (
            "ShipObjHash::ShipObjHash()", "P_ship ShipObjHash::find(P_obj key)",
            "bool ShipObjHash::add(P_ship ship)", "bool ShipObjHash::erase(P_ship ship)\n",
            "bool ShipObjHash::erase(P_ship ship, unsigned t_index)",
            "bool ShipObjHash::get_first(visitor &vs)", "bool ShipObjHash::get_next(visitor &vs)",
            "bool pc_is_aboard(P_ship ship)", "void clear_ship_content(P_ship ship)",
        )),
        ("ship_base.c", (
            "void clear_ship_layout(P_ship ship)", "void clear_references_to_ship(",
            "void delete_ship(P_ship ship, bool npc)", "void finish_sinking(P_ship ship)",
            "void ship_activity()", "void dock_ship(P_ship ship, int to_room)",
        )),
        ("ship_auto.c", ("void clear_autopilot(P_ship ship)",)),
        ("ship_npc.c", ("bool try_unload_npc_ship(P_ship ship)",)),
        ("ship_npc_ai.c", ("NPCShipAI::NPCShipAI(P_ship s, P_char ch)",)),
        ("ship_shop.c", ("int list_hulls(P_char ch, P_ship ship, int owned)",)),
        ("actwiz.c", ("void do_purge(P_char ch, char *argument, int /*cmd*/)",)),
    )
    for signature in signatures
)

HARNESS = r'''
#include <functional>
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/utility.h"
#include "core/mm.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/map.h"
#include "world/vnum.obj.h"
#include "ships/ships.h"
#include "ships/ship_auto.h"
#include "ships/ship_npc.h"
#include "ships/ship_npc_ai.h"
#include "magic/spells.h"
#include "economy/currency_transaction.h"
#include "sql/sql_player.h"
#include "sql/sql.h"
#include "flatfile/flatfile_ship_repository.h"
#include "persistence/persistence_mode.h"
#include "ships/ship_identity.c"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <set>
#include <string>

static room_data rooms[32] = {};
P_room world = rooms;
static index_data objects[3] = {};
P_index obj_index = objects;
ShipObjHash shipObjHash;
P_ship cyrics_revenge = nullptr;
int davy_jones_locker_rnum = 1;
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {{ ._mxsail = 100, ._speedgain = 1 }};
const ShipArcProperties ship_arc_properties[MAXSHIPCLASS] = {};
const WeaponData weapon_data[MAXWEAPON] = {};
static int persistence_calls, freed_ships, extracted_objects, extracted_crew, invariant_logs;
static bool persistence_ok = false;
static std::set<void *> live_ships;
static std::string output_text;
struct mm_ds *dead_mob_pool = nullptr, *dead_pconly_pool = nullptr;

void *__malloc(size_t size, const char *, const char *, int) { return calloc(1, size); }
void __free(void *p, const char *, int) {
    if (live_ships.erase(p)) ++freed_ships;
    free(p);
}
int real_room(const int vnum) { assert(vnum > 0 && vnum < 32); return vnum; }
int real_room0(const int vnum) { return real_room(vnum); }
int real_object(const int vnum) { return vnum == VOBJ_ALL_SHIPS ? 1 : 2; }
int ship_room_proc(int, P_char, int, char *) { return 0; }
void obj_to_room(P_obj obj, int room) {
    obj->next_content = world[room].contents;
    world[room].contents = obj;
    obj->loc_p = LOC_ROOM; obj->loc.room = room;
}
void obj_from_room(P_obj obj) {
    assert(obj && OBJ_ROOM(obj));
    P_obj *at = &world[obj->loc.room].contents;
    while (*at && *at != obj) at = &(*at)->next_content;
    assert(*at == obj); *at = obj->next_content;
    obj->next_content = nullptr; obj->loc_p = LOC_NOWHERE;
}
void extract_obj(P_obj obj, int) {
    if (OBJ_ROOM(obj)) obj_from_room(obj);
    ++extracted_objects; delete obj;
}
void char_from_room(P_char ch) {
    P_char *at = &world[ch->in_room].people;
    while (*at && *at != ch) at = &(*at)->next_in_room;
    assert(*at == ch); *at = ch->next_in_room;
    ch->next_in_room = nullptr; ch->in_room = NOWHERE;
}
bool char_to_room(P_char ch, int room, int) {
    ch->next_in_room = world[room].people;
    world[room].people = ch; ch->in_room = room; return true;
}
void logit(const char *, const char *format, ...) {
    char text[1024]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof text, format, args); va_end(args);
    if (strstr(text, "ownerless non-NPC")) ++invariant_logs;
}
void send_to_char(const char *text, P_char) { output_text += text; }
void send_to_char_f(P_char, const char *format, ...) {
    char text[4096]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof text, format, args); va_end(args); output_text += text;
}
std::string strip_ansi(const char *text) { return text; }
bool sql_delete_ship(const char *) { ++persistence_calls; return persistence_ok; }
const char *persistence_mode_flatfile_root() { return "/unused-test-authority"; }
flatfile_ship_result flatfile_ship_remove(const char *, uint32_t, const char *, std::string *) {
    ++persistence_calls;
    return persistence_ok ? flatfile_ship_result::ok : flatfile_ship_result::invalid;
}

// Stable deterministic mechanics: production heartbeat changes the timers,
// sail repair, position and reload state itself.
int number(int low, int) { return low; }
float get_property(const char *, double fallback) { return fallback; }
void act_to_all_in_ship(P_ship, const char *) {}
void act_to_all_in_ship_f(P_ship, const char *, ...) {}
void act_to_outside(P_ship, int, const char *, ...) {}
void act_to_outside_ships(P_ship, P_ship, int, const char *, ...) {}
void update_ship_status(P_ship, P_ship) {}
void update_crew(P_ship) {}
void reset_crew_stamina(P_ship) {}
void assignid(P_ship, const char *, bool) {}
void jettison_all(P_ship) {}
void queue_ship_save(P_ship, const char *) {}
void reset_ship(P_ship, bool) { abort(); }
P_char get_char2(char *) { return nullptr; }
int get_player_pid_from_name(char *) { abort(); }
bool insert_money_pickup(int, int) { abort(); }
char *coin_stringv(int, int) { abort(); }
bool chaos_starter_frigate_enabled() { return false; }
const char *epic_cost_string(int) { return ""; }
struct ship_insurance_context { char owner[MAX_NAME_LENGTH]; int pid; int platinum; };
static void ship_insurance_committed(P_char, bool, const currency_command_result&, unsigned,
                                     const unsigned char *, size_t) { abort(); }
bool currency_transaction_submit_bank_reward(P_char, int64_t, currency_reason_type,
    int64_t, critical_source_site, critical_deadline_class, currency_completion_fn,
    const void *, size_t) { abort(); }

float ShipCrew::get_stamina_mod() { return 1; }
int ShipCrew::get_sail_repair_mod() const { return 1; }
int ShipCrew::get_weapon_repair_mod() const { return 1; }
int ShipCrew::get_hull_repair_mod() const { return 1; }
void ShipCrew::reduce_stamina(float val, P_ship) { stamina -= val; }
void ShipCrew::rpar_skill_raise(float) {}
void ShipCrew::sail_skill_raise(float) {}
void ShipCrew::guns_skill_raise(float) {}
int ShipData::get_maxspeed(P_char) const { return 10; }
float ShipTypeData::get_hull_mod() const { return 1; }
char *ShipSlot::get_description() { return const_cast<char *>("test cannon"); }
P_char captain_is_aboard(P_ship) { return nullptr; }
int get_next_speed_change(P_ship ship) { return ship->setspeed - ship->speed; }
float get_next_heading_change(P_ship) { abort(); }
void normalize_direction(float &) { abort(); }
ShipMap tactical_map[101][101] = {};
ContactData contacts[MAXSHIPS] = {};
bool getmap(P_ship, bool) { abort(); }
bool is_valid_sailing_location(P_ship, int) { abort(); }
void everyone_look_out_ship(P_ship) { abort(); }
void stop_autopilot(P_ship) { abort(); }
void autopilot_activity(P_ship) {}
void NPCShipAI::activity() {}
int getcontacts(P_ship, bool) { abort(); }
int try_ram_ship(P_ship, P_ship, float) { abort(); }
void land_ship(P_ship) { abort(); }
void crash_land(P_ship) { abort(); }
int dice(int, int) { abort(); }
bool has_eq_diplomat(const ShipData *) { return false; }
int get_property(const char *, int fallback) { return fallback; }
P_ship try_load_pirate_ship(P_ship) { return nullptr; }
void send_to_room_f(int, const char *, ...) {}
void send_to_room(const char *, int) {}

// Command lookup selects the real hull object. Unrelated purge modes must
// never be entered by these cases.
char *one_argument(const char *argument, char *first) {
    assert(!strcmp(argument, "hull")); strcpy(first, argument); return nullptr;
}
P_char get_char_room_vis(P_char, const char *) { return nullptr; }
static P_obj purge_hull;
P_obj get_obj_in_list_vis(P_char, const char *, P_obj list, bool) {
    for (P_obj obj = list; obj; obj = obj->next_content)
        if (obj == purge_hull) return obj;
    return nullptr;
}
int IS_MORPH(P_char) { return false; }
void extract_char(P_char ch) {
    assert(IS_NPC(ch)); char_from_room(ch); ++extracted_crew;
}
void update_ingame_racewar(int) { abort(); }
int writeCharacter(P_char, int, int) { abort(); }
void close_socket(P_desc) { abort(); }
void free_char(P_char) { abort(); }
int checked_snprintf_at(const char *, int, char *, size_t, const char *, ...) { abort(); }
void *_mm_get(mm_ds *, const char *, int) { abort(); }
void ensure_pconly_pool() { abort(); }
char *skip_spaces(char *) { abort(); }
int restoreCharOnly(P_char, char *) { abort(); }
void delete_character(P_char, bool, std::function<void(character_delete_result)>) { abort(); }

''' + FUNCTIONS + r'''

static P_ship make_ship(int race, int interior) {
    auto ship = static_cast<P_ship>(calloc(1, sizeof(ShipData)));
    live_ships.insert(ship);
    ship->race = race; ship->db_id = race == NPCSHIP ? 0 : 42;
    ship->ownername = race == NPCSHIP ? nullptr : const_cast<char *>("Captain");
    ship->name = const_cast<char *>("Test vessel"); ship->id = const_cast<char *>("AA");
    ship->flags = LOADED | DOCKED;
    ship->location = 1; ship->crew.stamina = ship->crew.max_stamina = 100;
    for (auto &room : ship->room) room.roomnum = -1;
    ship->room[0].roomnum = interior; ship->room_count = 1;
    world[interior].funct = ship_room_proc;
    ship->shipobj = new obj_data{}; ship->shipobj->R_num = 1;
    ship->shipobj->short_description = const_cast<char *>("test hull");
    ship->panel = new obj_data{};
    ship->autopilot = static_cast<shipai_data *>(calloc(1, sizeof(shipai_data)));
    if (race == NPCSHIP) ship->npc_ai = new NPCShipAI(ship);
    obj_to_room(ship->shipobj, 1); obj_to_room(ship->panel, interior);
    register_ship_runtime(ship); assert(shipObjHash.add(ship));
    return ship;
}

static void remove_runtime(P_ship ship) {
    assert(shipObjHash.erase(ship)); delete_ship(ship, true);
}

static void check_sinking(bool passenger) {
    P_ship first = make_ship(NPCSHIP, 2), later = make_ship(NPCSHIP, 3);
    // Hash order is object-address dependent: choose the actual first visitor.
    ShipVisitor visitor; assert(shipObjHash.get_first(visitor));
    if (visitor.get_value() != first) std::swap(first, later);
    later->race = 0; later->ownername = const_cast<char *>("Captain");
    later->db_id = 42; later->timer[T_MAINTENANCE] = 150;
    later->timer[T_UNDOCK] = 20;
    first->flags |= SINKING; first->timer[T_SINKING] = 1;
    P_ship escort = make_ship(NPCSHIP, 4);
    escort->target = later; // An engaged escort still holds the sinking ship.
    escort->npc_ai->escort = escort->npc_ai->did_board = first;
    const auto ref = first->runtime_ref;
    const int interior = first->room[0].roomnum;
    char_data pc = {};
    if (passenger) char_to_room(&pc, interior, 0);
    const int before_freed = freed_ships, before_calls = persistence_calls;
    ship_activity();
    assert(later->timer[T_MAINTENANCE] == 150); // one skipped tick is intentional
    if (passenger) {
        assert(find_ship_by_runtime_ref(ref) == first);
        assert(first->timer[T_SINKING] == 30 && freed_ships == before_freed);
        assert(escort->npc_ai->escort == first && escort->npc_ai->did_board == first);
        for (int i = 0; i < 29; ++i) ship_activity();
        assert(first->timer[T_SINKING] == 1);
        assert(later->timer[T_MAINTENANCE] == 121);
        char_from_room(&pc);
        ship_activity();
    }
    assert(!find_ship_by_runtime_ref(ref));
    assert(shipObjHash.size() == 2 && freed_ships == before_freed + 1);
    assert(!escort->npc_ai->escort && !escort->npc_ai->did_board);
    assert(escort->target == later);
    assert(!world[interior].funct && !world[interior].contents);
    assert(persistence_calls == before_calls);
    const int maintenance = later->timer[T_MAINTENANCE];
    output_text.clear(); list_hulls(&pc, later, true);
    const std::string before_display = output_text;
    later->timer[T_UNDOCK] = 3;
    later->repair = 10; later->mainsail = 10;
    later->crew.rpar_mod_applied = 1;
    later->x = later->y = 50.2f; later->speed = later->setspeed = 3;
    later->slot[0].type = SLOT_WEAPON; later->slot[0].timer = 3;
    for (int i = 1; i <= 5; ++i) {
        ship_activity();
        assert(later->timer[T_MAINTENANCE] == maintenance - i);
        assert(!find_ship_by_runtime_ref(ref));
    }
    assert(later->timer[T_UNDOCK] == 0 && !SHIP_DOCKED(later));
    assert(later->mainsail == 15 && later->repair == 5);
    assert(later->y > 50.2f && later->slot[0].timer == 0);
    output_text.clear(); list_hulls(&pc, later, true);
    char before_hours[64], after_hours[64];
    snprintf(before_hours, sizeof before_hours, "&n%.1f&+g hours", maintenance / 75.0);
    snprintf(after_hours, sizeof after_hours, "&n%.1f&+g hours", (maintenance - 5) / 75.0);
    assert(strcmp(before_hours, after_hours));
    assert(before_display.find(before_hours) != std::string::npos);
    assert(output_text.find(after_hours) != std::string::npos);
    assert(freed_ships == before_freed + 1);
    remove_runtime(later);
    remove_runtime(escort);
    puts(passenger ? "PASS player-aboard delay and later destruction" : "PASS sinking and heartbeat recovery");
}

static void check_delete(bool ownerless, int db_id = 42) {
    P_ship ship = make_ship(0, 2), observer = make_ship(NPCSHIP, 3);
    if (ownerless) ship->ownername = nullptr;
    ship->db_id = db_id;
    observer->target = ship;
    observer->npc_ai->escort = observer->npc_ai->did_board = ship;
    char_data passenger = {}; char_to_room(&passenger, 2, 0);
    const auto ref = ship->runtime_ref;
    const int before = persistence_calls, before_freed = freed_ships, logs = invariant_logs;
    assert(shipObjHash.erase(ship)); delete_ship(ship);
    assert(find_ship_by_runtime_ref(ref) == ship && shipObjHash.find(ship->shipobj) == ship);
    assert(shipObjHash.size() == 2 && observer->target == ship);
    assert(observer->npc_ai->escort == ship && observer->npc_ai->did_board == ship);
    assert(passenger.in_room == 2 && world[2].people == &passenger);
    assert(world[2].funct && world[2].contents == ship->panel);
    assert(freed_ships == before_freed);
    assert(persistence_calls == before + (ownerless ? 0 : 1));
    assert(invariant_logs == logs + (ownerless ? 1 : 0));
    ship->ownername = const_cast<char *>("Captain");
    persistence_ok = true;
    assert(shipObjHash.erase(ship)); delete_ship(ship);
    assert(!find_ship_by_runtime_ref(ref) && !observer->target);
    assert(!observer->npc_ai->escort && !observer->npc_ai->did_board);
    assert(passenger.in_room == 1 && !world[2].people);
    char_from_room(&passenger);
    assert(freed_ships == before_freed + 1);
    persistence_ok = false;
    remove_runtime(observer);
    puts(ownerless ? "PASS ownerless player retained and logged" : "PASS failed player persistence retained, retry succeeds");
}

static void check_npc_removal(bool purge) {
    P_ship ship = make_ship(NPCSHIP, 2), observer = make_ship(0, 3);
    // Even a stale durable id must not send an NPC to the flat-file backend.
    ship->db_id = 99;
    observer->target = ship; cyrics_revenge = ship;
    const auto ref = ship->runtime_ref;
    const int before = persistence_calls, objects_before = extracted_objects, freed_before = freed_ships;
    if (purge) {
        purge_hull = ship->shipobj;
        char_data staff = {}; staff.player.level = 60; staff.in_room = 1;
        staff.player.name = const_cast<char *>("Staff");
        char argument[] = "hull"; output_text.clear();
        do_purge(&staff, argument, 0);
        assert(output_text.find("Ok.") != std::string::npos);
    } else assert(try_unload_npc_ship(ship));
    assert(!find_ship_by_runtime_ref(ref) && !observer->target && !cyrics_revenge);
    assert(shipObjHash.size() == 1 && freed_ships == freed_before + 1);
    assert(extracted_objects == objects_before + 2);
    assert(!world[2].funct && !world[2].contents);
    assert(persistence_calls == before);
    remove_runtime(observer);
    puts(purge ? "PASS staff purge and reference cleanup" : "PASS normal NPC unload");
}

static void check_occupied_purge() {
    P_ship ship = make_ship(NPCSHIP, 2);
    char_data passenger = {}, crew = {}, staff = {};
    crew.specials.act = ACT_ISNPC;
    char_to_room(&passenger, 2, 0); char_to_room(&crew, 2, 0);
    P_obj loot = new obj_data{}, corpse = new obj_data{};
    corpse->type = ITEM_CORPSE; // Player corpses must be moved outside.
    obj_to_room(loot, 2); obj_to_room(corpse, 2);
    const int before = extracted_objects;
    staff.player.level = 60; staff.player.name = const_cast<char *>("Staff"); staff.in_room = 1;
    purge_hull = ship->shipobj;
    char argument[] = "hull"; do_purge(&staff, argument, 0);
    assert(!world[2].people && !world[2].contents && !world[2].funct);
    assert(passenger.in_room == 1 && crew.in_room == NOWHERE && extracted_crew == 1);
    assert(extracted_objects == before + 3); // loot, panel and hull
    assert(OBJ_ROOM(corpse) && corpse->loc.room == 1);
    P_ship replacement = make_ship(NPCSHIP, 2);
    assert(!world[2].people && world[2].contents == replacement->panel);
    remove_runtime(replacement);
    extract_obj(corpse, true); char_from_room(&passenger);
    puts("PASS occupied purge evacuates player/corpse, removes crew and loot");
}

static void check_ai_references() {
    P_ship ship = make_ship(NPCSHIP, 2), observer = make_ship(NPCSHIP, 3);
    P_ship unrelated = make_ship(NPCSHIP, 4);
    observer->target = ship;
    observer->npc_ai->escort = observer->npc_ai->did_board = ship;
    unrelated->npc_ai->escort = unrelated->npc_ai->did_board = observer;
    world[1].number = 1;
    dock_ship(ship, 1);
    assert(!observer->target);
    assert(observer->npc_ai->escort == ship && observer->npc_ai->did_board == ship);
    observer->target = ship;
    remove_runtime(ship);
    assert(!observer->target && !observer->npc_ai->escort && !observer->npc_ai->did_board);
    assert(unrelated->npc_ai->escort == observer && unrelated->npc_ai->did_board == observer);
    remove_runtime(observer); remove_runtime(unrelated);
    puts("PASS destruction forgets NPC AI references; docking preserves them");
}

static void check_unloaded_cleanup() {
    P_ship ship = make_ship(0, 2);
    // Failed load: the abstract layout remains, but no real rooms were claimed.
    world[2].funct = nullptr;
    obj_from_room(ship->panel); obj_to_room(ship->panel, 1);
    ship->room[0].roomnum = -1;
    ship->flags &= ~LOADED;
    const int before = persistence_calls;
    remove_runtime(ship);
    assert(persistence_calls == before);
    puts("PASS failed-load cleanup never traverses unassigned room numbers");
}

int main(int argc, char **argv) {
    assert(argc == 2);
    objects[1].virtual_number = VOBJ_ALL_SHIPS;
    if (!strcmp(argv[1], "sinking")) check_sinking(false);
    else if (!strcmp(argv[1], "passenger")) check_sinking(true);
    else if (!strcmp(argv[1], "purge")) check_npc_removal(true);
    else if (!strcmp(argv[1], "unload")) check_npc_removal(false);
    else if (!strcmp(argv[1], "persistence")) check_delete(false);
    else if (!strcmp(argv[1], "ownerless")) { check_delete(true); check_delete(true, 0); }
    else if (!strcmp(argv[1], "occupied")) check_occupied_purge();
    else if (!strcmp(argv[1], "references")) check_ai_references();
    else if (!strcmp(argv[1], "unloaded")) check_unloaded_cleanup();
    else abort();
    assert(shipObjHash.size() == 0 && live_ships.empty());
}
'''

build_root = ROOT / "bin" / "tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="npc-ship-deletion-", dir=build_root) as directory:
    path = Path(directory)
    failures = []
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    for backend in ("mysql", "flatfile"):
        defines = ["-D__NO_MYSQL__", "-Isrc/no_mysql"] if backend == "flatfile" else ["-I/usr/include/mysql"]
        binary = path / backend
        subprocess.run(
            ["g++", "-std=c++20", "-g", "-Og", "-Isrc", "-I/usr/include/libxml2", *defines,
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
             str(path / "test.cpp"), str(HARNESS_STUBS), "-o", str(binary)],
            cwd=ROOT, check=True,
        )
        for case in ("sinking", "passenger", "purge", "unload", "persistence", "ownerless",
                     "occupied", "references", "unloaded"):
            print(f"{backend}: {case}", flush=True)
            result = subprocess.run([str(binary), case], timeout=30)
            if result.returncode:
                failures.append(f"{backend}: {case}")
    assert not failures, f"Failed cases: {', '.join(failures)}"
