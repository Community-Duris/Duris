#!/usr/bin/env python3
"""Run production bulk selection/reporting with held item and coin publication."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC


source = (SRC / "actobj.c").read_text(encoding="utf-8")


def function(signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


state_start = source.index("struct bulk_get_state\n")
state_end = source.index("\n};", state_start) + 3

prelude = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#define MAX_STRING_LENGTH 65536
#define FALSE 0
#define LOWEST_MAT_VNUM 1000
#define HIGHEST_MAT_VNUM 2000
#define ITEM_MONEY 1
#define ITEM2_ACCOUNT_BOUND 1
#define ITEM2_NOLOOT 2
#define IS_PC(ch) ((ch)->pc)
#define GET_PID(ch) ((ch)->pid)
#define OBJ_VNUM(obj) 0
#define CAN_SEE_OBJ(ch,obj) true
#define CAN_CARRY_N(ch) ((ch)->count_limit)
#define CAN_CARRY_W(ch) ((ch)->weight_limit)
#define GET_OBJ_WEIGHT(obj) ((obj)->weight)
#define GET_ITEM_TYPE(obj) ((obj)->type)
#define IS_OBJ_STAT2(obj,flag) ((obj)->flags & (flag))
#define IS_TRUSTED(ch) false
#define OBJS(obj,ch) ((obj)->short_description)
#define PERS(victim,ch,hide) ((victim)->name)
#define CAP(text) ((text)[0] = toupper((text)[0]))
static void checked_snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);
}
struct char_data { int pid = 42; bool pc = true; int count_limit = 3;
    int weight_limit = 10; const char *name = "a horse"; };
using P_char = char_data *;
struct obj_data { const char *name = "dagger"; const char *short_description = "a dagger";
    int weight = 1; int condition = 1; int type = 0; int flags = 0;
    uint64_t obj_uid = 1; P_char hitched_to = nullptr; };
using P_obj = obj_data *;
struct synchronous_get_item { uint64_t item_uid; P_obj object; bool scrap; };
struct item_owner_identity {};
enum class item_transfer_reason { unknown };
static std::string output;
static bool isname(const char *filter, const char *name) { return !strcmp(filter, name); }
static bool account_bound_reward_owner(P_char, P_obj) { return false; }
static bool do_get_obj_is_takeable(P_char, P_obj object) { return object->weight >= 0; }
static bool checkgetput(P_char, P_obj) { return false; }
static bool uses_generic_item_ownership(P_obj object) { return object->type != ITEM_MONEY; }
static void send_to_char(const char *message, P_char) { output += message; }
'''

driver = r'''
int main()
{
    char_data actor;
    obj_data container, dagger, heavy;
    container.obj_uid = 50;
    heavy.weight = 200;
    heavy.short_description = "a boulder";
    bulk_get_state state = {};
    state.container_uid = 50;
    int count = 0;
    int64_t weight = 0;
    bool stop = false;
    assert(select_bulk_get_item(&actor, &container, &dagger, nullptr, false,
                               count, weight, state, stop));
    assert(!select_bulk_get_item(&actor, &container, &heavy, nullptr, false,
                                count, weight, state, stop));
    assert(stop && count == 1 && weight == 1 && output.empty());
    assert(state.durable_items.size() == 1);
    // Snapshot rejection descriptions, not pointers into later world state.
    heavy.short_description = "a changed object";
    bulk_gets.emplace(actor.pid, std::move(state));
    assert(bulk_get_player_busy(&actor));
    assert(!bulk_get_player_busy(nullptr));
    actor.pc = false;
    assert(!bulk_get_player_busy(&actor));
    actor.pc = true;
    output += "You get a dagger from the corpse.\r\n";
    bulk_gets.at(actor.pid).total = 1;
    // First and subsequent coin acknowledgements must not release the gate.
    for (int coin = 0; coin < 2; ++coin)
    {
        assert(bulk_get_player_busy(&actor));
        assert(output.find("too heavy") == std::string::npos);
        output += "You get 1 gold coin.\r\n";
        bulk_gets.at(actor.pid).got_coins = true;
    }
    finish_bulk_get(&actor, actor.pid);
    assert(!bulk_get_player_busy(&actor));
    assert(output == "You get a dagger from the corpse.\r\n"
                     "You get 1 gold coin.\r\nYou get 1 gold coin.\r\n"
                     "A boulder is too heavy.\r\n");
    finish_bulk_get(&actor, actor.pid); // No duplicate reporting.
    assert(output.find("too heavy") == output.rfind("too heavy"));

    // Nothing accepted: rejection is reported immediately on ordinary finish,
    // without an additional misleading empty-container message.
    output.clear(); state = {}; state.container_uid = 50;
    count = actor.count_limit; weight = 0; stop = false;
    assert(!select_bulk_get_item(&actor, &container, &dagger, nullptr, false,
                                count, weight, state, stop));
    // Container scans continue after an ordinary count-cap rejection so a
    // later money object can still be selected.
    assert(output.empty() && !stop);
    bulk_gets.emplace(actor.pid, std::move(state));
    finish_bulk_get(&actor, actor.pid);
    assert(output == "You can't carry any more.\r\n");

    // Floor scans continue after overweight/non-takeable/bound/no-loot/hitched
    // items, and report the collected failures only after the success summary.
    output.clear(); state = {}; count = 0; weight = 0; stop = false;
    assert(!select_bulk_get_item(&actor, nullptr, &heavy, nullptr, false,
                                count, weight, state, stop));
    assert(!stop);
    obj_data rejected;
    rejected.weight = -1;
    assert(!select_bulk_get_item(&actor, nullptr, &rejected, nullptr, false,
                                count, weight, state, stop));
    rejected.weight = 1;
    for (int flag : { ITEM2_ACCOUNT_BOUND, ITEM2_NOLOOT })
    {
        rejected.flags = flag;
        assert(!select_bulk_get_item(&actor, nullptr, &rejected, nullptr, false,
                                    count, weight, state, stop));
    }
    rejected.flags = 0; rejected.hitched_to = &actor;
    assert(!select_bulk_get_item(&actor, nullptr, &rejected, nullptr, false,
                                count, weight, state, stop));
    assert(state.rejections.size() == 5 && output.empty());
    state.total = 2;
    report_bulk_get(&actor, state);
    assert(output.starts_with("You got 2 items.\r\n"));
    assert(output.find("is hitched to a horse.") != std::string::npos);
    puts("bulk get publication runtime: ok");
}
'''

harness = "\n".join([
    prelude, source[state_start:state_end],
    "std::unordered_map<uint32_t, bulk_get_state> bulk_gets;",
    function("bool bulk_get_player_busy("),
    function("static void report_bulk_get("),
    function("static void finish_bulk_get("),
    function("static void reject_bulk_get_object("),
    function("static bool select_bulk_get_item("),
    driver,
])
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    cpp = path / "bulk_get_publication.cpp"
    binary = path / "bulk_get_publication"
    cpp.write_text(harness, encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-g", str(cpp), "-o", str(binary)],
                   check=True)
    subprocess.run([str(binary)], check=True)
