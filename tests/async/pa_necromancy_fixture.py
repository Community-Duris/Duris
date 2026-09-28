#!/usr/bin/env python3
"""Focused production-function fixture for corpse raise and resurrection.

The generated C++ translation unit contains the exact owned completion functions
from the C++-compiled game sources. Small world/object API stubs provide an
isolated in-memory room graph; the separate MySQL harness verifies durable SQL
ownership and UID rows.
"""
from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
NECROMANCY = SRC / "classes/necromancy.c"
CORPSE_LIFECYCLE = SRC / "magic/spell_corpse_lifecycle.c"


def _function(source: str, name: str, return_type: str) -> str:
    needle = f"{return_type} {name}("
    start = None
    for match in re.finditer(re.escape(needle), source):
        line_start = source.rfind("\n", 0, match.start()) + 1
        if not source[line_start : match.start()].strip():
            start = line_start
            break
    if start is None:
        raise AssertionError(f"{name} definition not found")
    opening = source.find("{", start + len(needle))
    if opening < 0:
        raise AssertionError(f"{name} body not found")

    # Count braces while skipping comments and quoted literals so extraction
    # tracks the real function body, rather than a brace in a diagnostic string.
    depth = 0
    i = opening
    state = "code"
    while i < len(source):
        char = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if char == '"':
                state = "string"
            elif char == "'":
                state = "char"
            elif char == "/" and nxt == "/":
                state = "line_comment"
                i += 1
            elif char == "/" and nxt == "*":
                state = "block_comment"
                i += 1
            elif char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    return source[start : i + 1]
        elif state in ("string", "char"):
            if char == "\\":
                i += 1
            elif (state == "string" and char == '"') or (state == "char" and char == "'"):
                state = "code"
        elif state == "line_comment" and char == "\n":
            state = "code"
        elif state == "block_comment" and char == "*" and nxt == "/":
            state = "code"
            i += 1
        i += 1
    raise AssertionError(f"unterminated function body for {name}")


def _production_units() -> str:
    necromancy = NECROMANCY.read_text()
    lifecycle = CORPSE_LIFECYCLE.read_text()
    return "\n\n".join(
        (
            _function(necromancy, "corpse_trace_enabled", "static bool"),
            "namespace {\n"
            + _function(necromancy, "discard_nested_raise_exclusions", "void")
            + "\n}",
            "bool corpse_raise_exceeds_carry_capacity(P_char, P_obj) { return false; }",
            _function(necromancy, "complete_corpse_raise_after_commit", "void"),
            _function(lifecycle, "resurrection_item_is_transient", "static bool"),
            _function(lifecycle, "complete_player_resurrection_after_commit", "void"),
        )
    )


CPP_FIXTURE = r'''#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utils.h"
#include "core/defines.h"
#include "classes/necromancy.h"
#include "player/pet_restore_state.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

static room_data test_rooms[16]{};
P_room world = test_rooms;
static index_data test_indexes[16]{};
P_index obj_index = test_indexes;
P_index mob_index = test_indexes;
const int top_of_world = 15;
float exp_mods[EXPMOD_MAX + 1]{};

static std::vector<P_obj> room_items[16];
static std::set<uint64_t> extracted_uids;
static std::vector<std::string> visible_events;
static int saved_characters = 0;
static int failed = 0;

static void detach(P_obj obj)
{
    if (!obj) return;
    P_obj *head = nullptr;
    if (obj->loc_p == LOC_INSIDE && obj->loc.inside) head = &obj->loc.inside->contains;
    else if (obj->loc_p == LOC_CARRIED && obj->loc.carrying) head = &obj->loc.carrying->carrying;
    else if (obj->loc_p == LOC_ROOM && obj->loc.room >= 0 && obj->loc.room < 16) {
        auto &items = room_items[obj->loc.room];
        items.erase(std::remove(items.begin(), items.end(), obj), items.end());
        head = &world[obj->loc.room].contents;
    }
    if (head) {
        while (*head && *head != obj) head = &(*head)->next_content;
        if (*head == obj) *head = obj->next_content;
    }
    obj->loc_p = LOC_NOWHERE;
    obj->loc.inside = nullptr;
    obj->next_content = nullptr;
}

void obj_from_obj(P_obj obj) { detach(obj); }
void obj_from_char(P_obj obj) { detach(obj); }
void obj_from_room(P_obj obj) { detach(obj); }
void obj_to_char(P_obj obj, P_char ch)
{
    detach(obj);
    obj->loc_p = LOC_CARRIED;
    obj->loc.carrying = ch;
    obj->next_content = ch->carrying;
    ch->carrying = obj;
}
void obj_to_char_at_end(P_obj obj, P_char ch)
{
    detach(obj);
    obj->loc_p = LOC_CARRIED;
    obj->loc.carrying = ch;
    obj->next_content = nullptr;
    P_obj *tail = &ch->carrying;
    while (*tail) tail = &(*tail)->next_content;
    *tail = obj;
}
void obj_to_room(P_obj obj, int room)
{
    detach(obj);
    obj->loc_p = LOC_ROOM;
    obj->loc.room = room;
    obj->next_content = world[room].contents;
    world[room].contents = obj;
    room_items[room].push_back(obj);
}
void extract_obj(P_obj obj, int)
{
    if (!obj) return;
    detach(obj);
    extracted_uids.insert(obj->obj_uid);
}
P_obj unequip_char(P_char ch, int slot, bool)
{
    P_obj obj = ch->equipment[slot];
    ch->equipment[slot] = nullptr;
    if (obj) detach(obj);
    return obj;
}
void char_from_room(P_char ch) { ch->in_room = NOWHERE; }
bool char_to_room(P_char ch, int room, int) { ch->in_room = room; return true; }
void act(const char *message, int, P_char, P_obj, void *, int) { visible_events.emplace_back(message ? message : ""); }
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}
void debug(const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
int STAT_INDEX(int value) { return value; }
void StartRegen(P_char, regen_resource) {}
void stop_fighting(P_char) {}
void StopAllAttackers(P_char) {}
void stop_riding(P_char) {}
void affect_remove(P_char, affected_type *) {}
void obj_affect_remove(P_obj, obj_affect *) {}
obj_affect *get_obj_affect(P_obj, int) { return nullptr; }
int obj_affect_time(P_obj, obj_affect *) { return 0; }
void affect_from_char(P_char, int) {}
bool affected_by_spell(P_char, int) { return false; }
void gain_exp(P_char, P_char, long, int) {}
void MobStartFight(P_char, P_char) {}
void add_follower(P_char, P_char) {}
int setup_pet(P_char, P_char, int, int) { return -1; }
void schedule_pet_death(P_char, int) {}
void remove_plushit_bits(P_char) {}
void balance_affects(P_char) {}
void radiate_message_from_room(int, const char *, int, RMFR_FLAGS, int) {}
P_char get_linked_char(P_char, ush_int) { return nullptr; }
int writeCharacter(P_char, int, int) { ++saved_characters; return 1; }
int number(int low, int) { return low; }
bool pet_restore_state_decode(const std::string &, pet_restore_state *) { return false; }
bool pet_restore_state_decode(const std::string &, pet_restore_state &, std::string *) { return false; }

P_obj make_object(uint64_t uid, int type, const char *name, unsigned int flags = 0)
{
    P_obj obj = new obj_data{};
    obj->obj_uid = uid;
    obj->type = static_cast<::byte>(type);
    obj->extra_flags = flags;
    obj->name = const_cast<char *>(name);
    obj->short_description = const_cast<char *>(name);
    obj->action_description = const_cast<char *>(name);
    obj->R_num = 0;
    return obj;
}
void put_inside(P_obj container, P_obj child)
{
    child->loc_p = LOC_INSIDE;
    child->loc.inside = container;
    child->next_content = container->contains;
    container->contains = child;
}
P_char make_player(const char *name, int pid, int room)
{
    P_char ch = new char_data{};
    ch->only.pc = new pc_only_data{};
    ch->only.pc->pid = pid;
    ch->in_room = room;
    ch->player.name = const_cast<char *>(name);
    ch->player.level = 20;
    return ch;
}
P_char make_npc(const char *name, int room)
{
    P_char ch = new char_data{};
    ch->only.npc = new npc_only_data{};
    ch->specials.act = ACT_ISNPC;
    ch->in_room = room;
    ch->player.name = const_cast<char *>(name);
    return ch;
}
P_obj find_uid(P_obj list, uint64_t uid)
{
    for (P_obj item = list; item; item = item->next_content)
        if (item->obj_uid == uid) return item;
    return nullptr;
}
void check(bool condition, const char *message)
{
    if (!condition) { std::cerr << "ASSERTION FAILED: " << message << '\n'; ++failed; }
}

'''


def _compile_and_run() -> str:
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pa-necromancy-", dir=build_root) as td:
        build = Path(td)
        source = build / "fixture.cpp"
        executable = build / "fixture"
        include_lines = []
        for production_file in (NECROMANCY, CORPSE_LIFECYCLE):
            for line in production_file.read_text().splitlines():
                if (line.lstrip().startswith("#include") and line != '#include "sql/sql.h"' and
                    line not in include_lines):
                    include_lines.append(line)
        source.write_text("\n".join(include_lines) + "\n\n" + CPP_FIXTURE + _production_units() + r'''
int main()
{
    test_rooms[3].number = 4103;
    test_rooms[2].number = 4104;
    {
        P_char caster = make_player("Raiser", 42101, 3);
        P_char follower = make_npc("skeletal servant", NOWHERE);
        P_obj corpse = make_object(300, ITEM_CORPSE, "the fallen guardian");
        P_obj pack = make_object(301, ITEM_CONTAINER, "a field pack");
        P_obj gem = make_object(302, ITEM_OTHER, "a blue gem");
        P_obj fleeting = make_object(303, ITEM_OTHER, "a vanishing charm", ITEM_TRANSIENT);
        P_obj fleeting_child = make_object(304, ITEM_OTHER, "a nested fading charm", ITEM_TRANSIENT);
        put_inside(corpse, pack);
        put_inside(corpse, fleeting);
        put_inside(pack, gem);
        put_inside(pack, fleeting_child);
        complete_corpse_raise_after_commit(caster, follower, corpse,
            corpse_raise_kind::undead, 20, 0, nullptr, 99001, false, 600,
            std::string(), true);
        check(follower->durable_pet_uid == 99001, "raise keeps the committed pet UID");
        check(follower->durable_pet_owner_pid == 42101, "raise keeps player custody owner");
        check(find_uid(follower->carrying, 301) == pack, "raised follower receives original root item");
        check(pack->contains == gem && gem->loc.inside == pack && gem->obj_uid == 302,
              "raise preserves nested parent and original child UID");
        check(extracted_uids.count(303) == 1 && extracted_uids.count(304) == 1,
              "raise excludes transient objects at each depth");
        check(extracted_uids.count(300) == 1, "raise consumes the original corpse");
        check(follower->in_room == 3, "raised follower becomes visible in caster room");
        check(saved_characters == 1, "raise invokes its caster checkpoint after visible completion");
        bool raised_message = false;
        for (const auto &event : visible_events)
            raised_message |= event.find("breathe life") != std::string::npos;
        check(raised_message, "raise emits its visible completion event");
        std::cout << "RAISE visible_room=3 pet_uid=" << follower->durable_pet_uid
                  << " owner_pid=" << follower->durable_pet_owner_pid
                  << " root_uid=301 child_uid=302 parent_uid=301 transient_destroyed=2 checkpoint_calls=1\n";
    }
    {
        extracted_uids.clear();
        visible_events.clear();
        saved_characters = 0;
        for (auto &items : room_items) items.clear();
        P_char caster = make_player("Resurrector", 42102, 3);
        P_char target = make_player("Returned", 42103, 2);
        P_obj corpse = make_object(400, ITEM_CORPSE, "Returned's corpse");
        P_obj old_kept = make_object(410, ITEM_OTHER, "a kept old item");
        P_obj old_transient = make_object(411, ITEM_OTHER, "an old transient item", ITEM_TRANSIENT);
        P_obj root = make_object(420, ITEM_CONTAINER, "a sealed case");
        P_obj child = make_object(421, ITEM_OTHER, "a silver key");
        obj_to_char(old_kept, target);
        obj_to_char_at_end(old_transient, target);
        put_inside(corpse, root);
        put_inside(root, child);
        complete_player_resurrection_after_commit(caster, target, corpse, true, 2);
        check(find_uid(world[2].contents, 410) == old_kept,
              "normal previous inventory is visible in its old room after resurrection");
        check(std::find(room_items[2].begin(), room_items[2].end(), old_kept) != room_items[2].end(),
              "resurrection leaves normal previous inventory in its old room");
        check(std::find(room_items[2].begin(), room_items[2].end(), old_transient) == room_items[2].end(),
              "resurrection does not drop a transient item into the old room");
        check(extracted_uids.count(411) == 1, "resurrection destroys the transient item itself");
        check(find_uid(target->carrying, 420) == root, "resurrection transfers corpse root into returned player inventory");
        check(root->contains == child && child->loc.inside == root && child->obj_uid == 421,
              "resurrection preserves nested parent and original child UID");
        check(target->in_room == 3, "resurrected player becomes visible beside caster");
        check(extracted_uids.count(400) == 1, "resurrection consumes the original corpse");
        check(saved_characters == 1, "resurrection invokes its player checkpoint after visible completion");
        bool returned_message = false;
        for (const auto &event : visible_events)
            returned_message |= event.find("comes to life again") != std::string::npos;
        check(returned_message, "resurrection emits its visible completion event");
        std::cout << "RESURRECTION visible_room=3 old_inventory_room=2 root_uid=420"
                  << " child_uid=421 parent_uid=420 transient_destroyed=1 checkpoint_calls=1\n";
    }
    return failed ? 1 : 0;
}
''')
        command = ["g++", "-std=c++20", "-O0", "-ffunction-sections", "-fdata-sections",
                   "-Isrc", str(source), "-Wl,--gc-sections", "-o", str(executable)]
        compiled = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
        if compiled.returncode:
            raise AssertionError("production corpse-function fixture did not compile:\n" +
                                 compiled.stderr[-12000:])
        env = os.environ.copy()
        env.pop("DURIS_CORPSE_TRACE", None)
        result = subprocess.run([str(executable)], cwd=ROOT, env=env,
                                text=True, capture_output=True, check=False)
        if result.returncode:
            raise AssertionError(f"production corpse-function journey failed ({result.returncode}):\n"
                                 f"{result.stdout}\n{result.stderr}")
        return result.stdout


def run_runtime_fixture() -> str:
    """Compile and execute both exact production post-commit continuations."""
    return _compile_and_run()
