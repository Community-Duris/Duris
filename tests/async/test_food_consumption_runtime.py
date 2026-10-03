#!/usr/bin/env python3
"""Execute eating admission and publication before and after custody retirement."""
from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source

ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    text = source("actobj.c").read_text(encoding="utf-8")
    context = text[text.index("struct food_consumption_context\n"):
                   text.index("static bool publish_food_consumption(")]
    functions = "\n".join(extract_function("actobj.c", signature) for signature in (
        "static bool publish_food_consumption(", "static bool submit_food_consumption(",
        "void do_eat(",
    ))
    program = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

constexpr int MAX_STRING_LENGTH = 1024, ITEM_FOOD = 19, AVATAR = 60;
constexpr int RACE_ILLITHID = 1, RACE_LICH = 2, TAG_EATEN = 3, TO_CHAR = 4;
constexpr int TRUE = 1, FALSE = 0;
constexpr unsigned PLAYER_COMPONENT_STATUS = 1, PLAYER_COMPONENT_INVENTORY = 2;
struct character { int pid = 17, level = 50, race = 0; bool npc = false, trusted = false; };
using P_char = character *;
struct object {
    uint64_t obj_uid = 42; int vnum = 15, type = ITEM_FOOD;
    std::array<int, 8> value = {}; std::array<time_t, 6> timer = {};
    P_char carrier = nullptr;
};
using P_obj = object *;
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_RACE(ch) ((ch)->race)
#define IS_NPC(ch) ((ch)->npc)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define IS_ARTIFACT(obj) false
#define OBJ_VNUM(obj) ((obj)->vnum)
#define OBJ_CARRIED_BY(obj, ch) ((obj)->carrier == (ch))
enum class item_owner_type { player, destruction };
struct item_owner_identity { item_owner_type type; uint64_t id, context_id; };
enum class item_custody_state { active, destroyed };
struct item_ownership_runtime_entry { item_owner_identity owner; item_custody_state state; };
enum class item_transfer_reason { destruction };
enum class item_movement_reject { none, owner_mismatch, coordinator_unavailable };
enum class economic_source_kind { intentional_destruction };
struct critical_operation_id {};
struct item_transfer_result {};
using publication_fn = bool (*)(const critical_operation_id &, P_char, bool,
    const item_transfer_result &, unsigned, const uint8_t *, size_t);
P_obj live = nullptr;
bool ownership_found = true, accept = true, durable = true, sated = false;
int submitted = 0, applied = 0, rejected = 0, dirty = 0;
bool artifact_updated = false;
item_ownership_runtime_entry ownership = {{item_owner_type::player, 17, 0}, item_custody_state::active};
std::array<uint8_t, 32> captured = {};
size_t captured_size = 0;
publication_fn publication = nullptr;
char *one_argument(char *argument, char *) { return argument; }
char *skip_spaces(char *argument) { return argument; }
P_obj get_obj_in_list_vis(P_char, char *, void *) { return live; }
void send_to_char(const char *, P_char) {}
template<class... Args> void send_to_char_f(Args...) {}
template<class... Args> void act(Args...) {}
bool affected_by_spell(P_char, int) { return sated; }
bool item_command_uses_durable_ownership(P_obj) { return durable; }
bool item_owner_identity_equal(const item_owner_identity &a, const item_owner_identity &b) {
    return a.type == b.type && a.id == b.id && a.context_id == b.context_id;
}
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *out) {
    assert(uid == 42); *out = ownership; return ownership_found;
}
P_obj find_live_item_uid(uint64_t uid) { return live && live->obj_uid == uid ? live : nullptr; }
void apply_eaten_item(P_char actor, P_obj food, bool update) {
    assert(food == live && food->carrier == actor); ++applied; artifact_updated = update; live = nullptr;
}
void mark_player_dirty_components(int pid, unsigned mask) {
    assert(pid == 17 && mask == (PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_INVENTORY)); ++dirty;
}
void report_movement_reject(P_char, item_movement_reject, const char *, P_obj) { ++rejected; }
bool item_movement_transaction_submit(P_char actor, P_obj food, P_obj target,
    const item_owner_identity &from, const item_owner_identity &to, item_transfer_reason reason,
    int64_t reason_id, void *, const void *context, size_t size, P_obj corpse,
    item_movement_reject *reject, publication_fn callback, economic_source_kind source) {
    assert(actor->pid == 17 && food == live && !target && !corpse);
    assert(from.type == item_owner_type::player && from.id == 17 && from.context_id == 0);
    assert(to.type == item_owner_type::destruction && to.id == 0 && to.context_id == 0);
    assert(reason == item_transfer_reason::destruction && reason_id == 15);
    assert(source == economic_source_kind::intentional_destruction && size <= captured.size());
    ++submitted; std::memcpy(captured.data(), context, size); captured_size = size; publication = callback;
    if (!accept) *reject = item_movement_reject::coordinator_unavailable;
    return accept;
}
'''
    # The command only reads this live list in the eligibility check.
    program = program.replace("bool npc = false, trusted = false;", "bool npc = false, trusted = false; void *carrying = nullptr;")
    program += context + functions + r'''
int main() {
    character actor, other;
    object food; food.carrier = &actor;
    char argument[] = "food";
    auto reset = [&] {
        live = &food; food.value = {}; food.timer = {}; actor.level = 50; actor.race = 0;
        actor.npc = false; ownership_found = accept = durable = true; sated = false;
        ownership = {{item_owner_type::player, 17, 0}, item_custody_state::active};
        submitted = applied = rejected = dirty = 0; publication = nullptr; food.carrier = &actor;
    };
    reset(); do_eat(&actor, argument, 0);
    assert(submitted == 1 && applied == 0 && dirty == 0 && live == &food);
    assert(publication({}, &actor, false, {}, 0, captured.data(), captured_size));
    assert(applied == 0 && live == &food && dirty == 0);
    assert(publication({}, &actor, true, {}, 0, captured.data(), captured_size));
    assert(applied == 1 && !live && dirty == 1);
    assert(publication({}, &actor, true, {}, 0, captured.data(), captured_size));
    assert(applied == 1 && dirty == 1);
    reset(); accept = false; do_eat(&actor, argument, 0);
    assert(submitted == 1 && rejected == 1 && applied == 0 && live == &food);
    reset(); ownership_found = false; do_eat(&actor, argument, 0);
    assert(submitted == 0 && rejected == 1 && applied == 0);
    reset(); ownership.owner.id = 18; do_eat(&actor, argument, 0);
    assert(submitted == 0 && rejected == 1 && applied == 0);
    reset(); ownership.state = item_custody_state::destroyed; do_eat(&actor, argument, 0);
    assert(submitted == 0 && rejected == 1 && applied == 0);
    reset(); do_eat(&actor, argument, 0); food.carrier = &other;
    assert(!publication({}, &actor, true, {}, 0, captured.data(), captured_size));
    assert(applied == 0 && live == &food && dirty == 0);
    assert(!publication({}, &actor, true, {}, 0, captured.data(), captured_size - 1));
    food_consumption_context empty = {};
    assert(!publication({}, &actor, true, {}, 0, reinterpret_cast<uint8_t *>(&empty), sizeof(empty)));
    reset(); food.value[5] = 1337; do_eat(&actor, argument, 0);
    assert(submitted == 0 && applied == 0);
    reset(); sated = true; do_eat(&actor, argument, 0);
    assert(submitted == 0 && applied == 0);
    reset(); food.value[1] = -1; do_eat(&actor, argument, 0);
    assert(submitted == 0 && applied == 0);
    reset(); actor.npc = true; do_eat(&actor, argument, 0);
    assert(submitted == 0 && applied == 0);
    reset(); durable = false; do_eat(&actor, argument, 0);
    assert(submitted == 0 && applied == 1);
    reset(); assert(submit_food_consumption(&actor, &food, true));
    assert(publication({}, &actor, true, {}, 0, captured.data(), captured_size));
    assert(applied == 1 && artifact_updated);
    std::puts("FOOD-NATIVE: admission, refusal, stale publication, committed effect and duplicate boundaries passed");
}
'''
    (ROOT / "bin").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="food-consumption-", dir=ROOT / "bin") as directory:
        cpp, binary = Path(directory) / "food.cpp", Path(directory) / "food"
        cpp.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
