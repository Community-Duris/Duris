#!/usr/bin/env python3
"""Execute the production direct/bulk pickup gates at and above the count cap.

Currency commit/rollback is covered separately by test_currency_input_queue.py;
this harness checks which objects reach that existing transaction path.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC


def extract(signature):
    source = (SRC / "actobj.c").read_text()
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if not depth:
            return source[start:end + 1]
    raise AssertionError(signature)


PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
constexpr int ITEM_MONEY = 20, ITEM2_ACCOUNT_BOUND = 1, ITEM2_NOLOOT = 2;
constexpr int LOWEST_MAT_VNUM = 100, HIGHEST_MAT_VNUM = 200;
constexpr int MAX_STRING_LENGTH = 1024;
constexpr bool TRUE = true, FALSE = false;
struct character { int count = 11, cap = 11, weight = 0, max_weight = 100; };
struct object {
    int type = ITEM_MONEY, vnum = 3, weight = 0, condition = 1;
    uint64_t obj_uid = 1;
    const char *name = "coins", *short_description = "coins";
    bool visible = true, takeable = true;
    character *hitched_to = nullptr;
};
using P_char = character *;
using P_obj = object *;
struct synchronous_item { uint64_t uid; P_obj object; bool scrap; };
struct bulk_get_state {
    std::vector<std::string> rejections;
    bool failed = false;
    std::vector<uint64_t> durable_items;
    std::vector<synchronous_item> synchronous_items;
};
#define GET_ITEM_TYPE(o) ((o)->type)
#define GET_OBJ_WEIGHT(o) ((o)->weight)
#define IS_CARRYING_N(c) ((c)->count)
#define CAN_CARRY_N(c) ((c)->cap)
#define CAN_CARRY_W(c) ((c)->max_weight)
#define CAN_SEE_OBJ(c,o) ((o)->visible)
#define OBJ_VNUM(o) ((o)->vnum)
#define GETDBG_LOG(...) ((void)0)
#define IS_OBJ_STAT2(o,f) false
#define IS_TRUSTED(c) false
#define OBJS(o,c) ((o)->name)
#define PERS(...) "someone"
#define checked_snprintf snprintf
int finalized = 0;
int total_carried_weight(P_char ch) { return ch->weight; }
void send_to_char(const char *, P_char) {}
void do_get_reject_out_of_sight(P_char, P_obj, bool &fail) { fail = true; }
bool do_get_finalize_container_item_or_reject(P_char, P_char, P_obj, P_obj obj,
    int &, bool &, bool, int, int, int, const char *, const char *, bool &fail)
{ if (!obj->takeable) { fail = true; return false; } ++finalized; return true; }
bool isname(const char *filter, const char *name) { return !strcmp(filter, name); }
bool do_get_obj_is_takeable(P_char, P_obj obj) { return obj->takeable; }
bool account_bound_reward_owner(P_char, P_obj) { return true; }
bool checkgetput(P_char, P_obj) { return false; }
bool uses_generic_item_ownership(P_obj obj) { return obj->type != ITEM_MONEY; }
void reject_bulk_get_object(bulk_get_state &state, P_obj, const char *) { state.failed = true; }
'''

DRIVER = r'''
bool direct(P_char actor, P_obj bag, P_obj item, bool local = false)
{
    int total = 0;
    bool found = false, stop = false, fail = false;
    return do_get_try_container_item(actor, nullptr, bag, item, total, found,
        false, local, stop, fail, "", "", "", "", "", true);
}
int main()
{
    for (int count : {10, 11, 28}) {
        for (int vnum : {3, 402013}) {
            for (bool local : {false, true}) {
                character actor;
                actor.count = count;
                object bag, money, ordinary;
                money.vnum = vnum;
                ordinary.type = 1;
                ordinary.name = "sword";
                ordinary.obj_uid = 2;
                const int before = finalized;
                assert(direct(&actor, &bag, &money, local));
                assert(finalized == before + 1);
                assert(direct(&actor, &bag, &ordinary, local) == (count < actor.cap));
                // all.coins and take all selection; an ordinary item precedes coins.
                for (const char *filter : {"coins", static_cast<const char *>(nullptr)}) {
                    bulk_get_state state;
                    int carried = count;
                    int64_t weight = 0;
                    bool stop = false;
                    bool accepted = select_bulk_get_item(&actor, &bag, &ordinary,
                        filter, local, carried, weight, state, stop);
                    assert(accepted == (!filter && count < actor.cap));
                    assert(!stop);
                    const int before_coins = carried;
                    assert(select_bulk_get_item(&actor, &bag, &money, filter,
                        local, carried, weight, state, stop));
                    assert(carried == before_coins);
                    assert(state.synchronous_items.size() == 1);
                    assert(state.durable_items.size() == (accepted ? 1u : 0u));
                }
                // Other eligibility gates still reject money.
                money.visible = false;
                assert(!direct(&actor, &bag, &money));
                money.visible = true;
                money.takeable = false;
                assert(!direct(&actor, &bag, &money));
                money.takeable = true;
                money.weight = actor.max_weight + 1;
                assert(!direct(&actor, &bag, &money));
                assert(direct(&actor, &bag, &money, true));
                assert(actor.count == count);
            }
        }
    }
    puts("Money count gates passed: direct, all.coins, mixed bulk; vnums 3/402013; counts 10/11/28.");
}
'''


if __name__ == "__main__":
    harness = "\n".join((PRELUDE, extract("static bool do_get_try_container_item("),
                         extract("static bool select_bulk_get_item("), DRIVER))
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "money_carry_count.cpp"
        binary = Path(directory) / "money_carry_count"
        source.write_text(harness)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-parameter", "-fsanitize=address,undefined",
                        "-g", str(source), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)
