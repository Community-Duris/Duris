#!/usr/bin/env python3
"""Exercise faerie sight's exact dust sink and post-commit effect handoff."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


def main() -> None:
    ethermancer = source("ethermancer.c").read_text(encoding="utf-8")
    lifecycle = source("spell_item_lifecycle.h").read_text(encoding="utf-8")
    start = lifecycle.index("constexpr size_t SPELL_COMPONENT_EFFECT_CONTEXT_MAX_BYTES")
    context = lifecycle[start:lifecycle.index("// Consume up to", start)]
    count = extract_function("ethermancer.c", "static size_t faerie_sight_dust_count(")
    apply = extract_function("ethermancer.c", "static void apply_faerie_sight(")
    completed = extract_function(
        "ethermancer.c", "spell_component_effect_status\nspell_faerie_sight_component_completed("
    )
    cast = extract_function("ethermancer.c", "void spell_faerie_sight(")
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

constexpr int VOBJ_FORAGE_FAERIE_DUST = 100;
constexpr int SPELL_FAERIE_SIGHT = 200;
constexpr uint32_t AFF2_DETECT_MAGIC = 1;
constexpr uint32_t AFF2_DETECT_GOOD = 2;
constexpr uint32_t AFF2_DETECT_EVIL = 4;
constexpr uint32_t AFF_FARSEE = 8;
constexpr uint32_t AFF_DETECT_INVISIBLE = 16;
constexpr int LOG_EXIT = 1;
constexpr unsigned CHAR_RFLAG_LOAD_DEGRADED = 1;
#define TRUE 1
#define FALSE 0
struct character;
struct object {
    int vnum = VOBJ_FORAGE_FAERIE_DUST;
    object *next_content = nullptr;
};
struct affected_type {
    int type;
    int duration;
    uint32_t bitvector;
    uint32_t bitvector2;
    affected_type *next;
};
struct character {
    int pid = 42;
    int level = 40;
    int in_room = 1;
    uint64_t runtime_id = 10;
    bool npc = false;
    bool alive = true;
    uint32_t affected2 = 0;
    unsigned runtime_flags = 0;
    object *carrying = nullptr;
    affected_type *affected = nullptr;
    character *next = nullptr;
};
using P_char = character *;
using P_obj = object *;
struct item_transfer_result { uint64_t root_item_uid = 0; uint16_t item_count = 0; };
enum class spell_component_effect_status { retry, waiting_for_owner, complete };
struct critical_operation_id { std::array<uint8_t, 16> bytes = {}; };
using item_movement_completion_fn = spell_component_effect_status (*)(const critical_operation_id &, P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t);
enum class item_spell_component_effect { faerie_sight = 1 };
#define IS_SET(bits, flag) (((bits) & (flag)) != 0)
#define IS_AFFECTED2(ch, flag) IS_SET((ch)->affected2, flag)
#define IS_PC(ch) (!(ch)->npc)
#define IS_ALIVE(ch) ((ch)->alive)
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->level)
#define OBJ_VNUM(obj) ((obj)->vnum)
P_char character_list = nullptr;
P_char find_character_by_runtime_id(uint64_t id) {
    for (P_char ch = character_list; ch; ch = ch->next)
        if (ch->runtime_id == id) return ch;
    return nullptr;
}
bool active_epoch = false;
int submitted = 0, direct_removed = 0, durable_removed = 0;
int added_affects = 0, messages = 0;
bool accept_submission = true;
size_t requested_count = 0;
bool requested_exact = false;
bool effect_applied = false, save_acknowledged = false;
int saved_pid = 0;
item_movement_completion_fn pending = nullptr;
std::array<uint8_t, 48> pending_context = {};
size_t pending_context_size = 0;
class economic_gameplay_authority {
public:
    static bool active() { return active_epoch; }
};
void logit(int, const char *, ...) {}
void send_to_char(const char *, P_char) { ++messages; }
bool affected_by_spell(P_char ch, int spell) {
    for (affected_type *af = ch->affected; af; af = af->next)
        if (af->type == spell) return true;
    return false;
}
void affect_to_char(P_char ch, affected_type *af) {
    ++added_affects;
    ch->affected2 |= af->bitvector2;
}
int vnum_in_inv(P_char ch, int vnum) {
    int count = 0;
    for (P_obj item = ch->carrying; item; item = item->next_content)
        count += item->vnum == vnum;
    return count;
}
void remove_dust(P_char ch) {
    P_obj *item = &ch->carrying;
    while (*item && (*item)->vnum != VOBJ_FORAGE_FAERIE_DUST)
        item = &(*item)->next_content;
    assert(*item);
    *item = (*item)->next_content;
}
void vnum_from_inv(P_char ch, int, int count) {
    assert(count == 1);
    remove_dust(ch);
    ++direct_removed;
}
void extract_obj(P_obj item) {
    assert(item && character_list);
    remove_dust(character_list);
    ++direct_removed;
}
bool spell_component_retirement_bind_effect_owner(const critical_operation_id &,
    uint32_t actor_pid, uint32_t owner_pid, item_spell_component_effect) {
    return actor_pid == 42 && (owner_pid == 42 || owner_pid == 99);
}
bool spell_component_retirement_effect_applied(const critical_operation_id &) {
    return effect_applied;
}
bool spell_component_retirement_effect_applied_once(const critical_operation_id &) {
    effect_applied = true;
    return true;
}
spell_component_effect_status spell_component_retirement_save_effect(
    const critical_operation_id &, P_char owner, item_spell_component_effect) {
    saved_pid = owner->pid;
    return save_acknowledged ? spell_component_effect_status::complete :
        spell_component_effect_status::waiting_for_owner;
}
bool spell_consume_components(P_char, int vnum, size_t count, uint32_t reason,
                              item_spell_component_effect effect,
                              item_movement_completion_fn continuation,
                              const void *context, size_t context_size, bool exact) {
    assert(active_epoch && vnum == VOBJ_FORAGE_FAERIE_DUST &&
           reason == SPELL_FAERIE_SIGHT &&
           effect == item_spell_component_effect::faerie_sight &&
           context_size <= pending_context.size());
    ++submitted;
    requested_count = count;
    requested_exact = exact;
    if (!accept_submission) return false;
    pending = continuation;
    pending_context_size = context_size;
    std::memcpy(pending_context.data(), context, context_size);
    return true;
}
''' + count + "\n" + apply + "\n" + context + "\n" + completed + "\n" + cast + r'''
void complete(P_char actor, bool committed) {
    assert(pending);
    auto continuation = pending;
    pending = nullptr;
    if (committed)
        for (size_t index = 0; index < requested_count; ++index) {
            remove_dust(actor);
            ++durable_removed;
        }
    item_transfer_result result {1, static_cast<uint16_t>(requested_count)};
    critical_operation_id operation_id{};
    auto status = continuation(operation_id, actor, committed, result, 0,
                               pending_context.data(), pending_context_size);
    if (committed) {
        assert(status == spell_component_effect_status::waiting_for_owner);
        const int effects_before_retry = added_affects;
        assert(continuation(operation_id, actor, true, result, 0,
                            pending_context.data(), pending_context_size) ==
               spell_component_effect_status::waiting_for_owner);
        assert(added_affects == effects_before_retry);
        save_acknowledged = true;
        assert(continuation(operation_id, actor, true, result, 0,
                            pending_context.data(), pending_context_size) ==
               spell_component_effect_status::complete);
    } else assert(status == spell_component_effect_status::complete);
}
void reset(P_char actor) {
    actor->carrying = nullptr;
    actor->affected = nullptr;
    actor->affected2 = 0;
    submitted = direct_removed = durable_removed = added_affects = messages = 0;
    requested_count = 0;
    requested_exact = false;
    pending = nullptr;
    accept_submission = true;
    effect_applied = save_acknowledged = false;
    saved_pid = 0;
}
int main() {
    character actor;
    character target;
    target.pid = 99;
    target.runtime_id = 20;
    actor.next = &target;
    character_list = &actor;
    char argument[] = "";
    object dust1, dust2;

    active_epoch = true;
    reset(&actor);
    actor.carrying = &dust1;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 1 && requested_count == 1 && requested_exact);
    assert(pending_context_size == 22);
    assert(added_affects == 0 && direct_removed == 0);
    complete(&actor, true);
    assert(durable_removed == 1 && direct_removed == 0 && added_affects == 2);
    assert(saved_pid == 42);
    assert(IS_AFFECTED2(&actor, AFF2_DETECT_MAGIC));

    reset(&actor);
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 0 && direct_removed == 0 && added_affects == 2);
    assert(!IS_AFFECTED2(&actor, AFF2_DETECT_MAGIC));

    reset(&actor);
    actor.carrying = &dust1;
    affected_type existing {SPELL_FAERIE_SIGHT, 1, 0, AFF2_DETECT_MAGIC, nullptr};
    actor.affected = &existing;
    actor.affected2 = AFF2_DETECT_MAGIC;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 1 && existing.duration == 1);
    complete(&actor, true);
    assert(existing.duration == 20 && durable_removed == 1 && direct_removed == 0);

    reset(&actor);
    actor.carrying = &dust1;
    affected_type plain {SPELL_FAERIE_SIGHT, 1, 0, 0, nullptr};
    actor.affected = &plain;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 1 && added_affects == 0);
    complete(&actor, true);
    assert(plain.duration == 20 && added_affects == 1);
    assert(durable_removed == 1 && direct_removed == 0);

    reset(&actor);
    actor.carrying = &dust1;
    affected_type long_sight {SPELL_FAERIE_SIGHT, 30, 0, AFF2_DETECT_MAGIC, nullptr};
    actor.affected = &long_sight;
    actor.affected2 = AFF2_DETECT_MAGIC;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 0 && durable_removed == 0 && direct_removed == 0);
    assert(long_sight.duration == 30 && added_affects == 0);

    reset(&actor);
    dust1.next_content = &dust2;
    dust2.next_content = nullptr;
    actor.carrying = &dust1;
    affected_type multi_second {SPELL_FAERIE_SIGHT, 1, 0, AFF2_DETECT_MAGIC, nullptr};
    affected_type multi_first {SPELL_FAERIE_SIGHT, 1, 0, AFF2_DETECT_MAGIC, &multi_second};
    actor.affected = &multi_first;
    actor.affected2 = AFF2_DETECT_MAGIC;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 1 && requested_count == 2 && requested_exact);
    complete(&actor, true);
    assert(multi_first.duration == 20 && multi_second.duration == 20);
    assert(durable_removed == 2 && direct_removed == 0);

    reset(&actor);
    actor.carrying = &dust1;
    dust1.next_content = nullptr;
    accept_submission = false;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 1 && !pending && added_affects == 0 && direct_removed == 0);

    reset(&actor);
    actor.carrying = &dust1;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    complete(&actor, false);
    assert(added_affects == 0 && durable_removed == 0 && direct_removed == 0);

    reset(&actor);
    target.affected = nullptr;
    target.affected2 = 0;
    actor.carrying = &dust1;
    spell_faerie_sight(40, &actor, argument, 0, &target, nullptr);
    target.in_room = 2;
    complete(&actor, true);
    assert(durable_removed == 1 && added_affects == 0 && direct_removed == 0);
    assert(saved_pid == 99);

    active_epoch = false;
    reset(&actor);
    actor.carrying = &dust1;
    spell_faerie_sight(40, &actor, argument, 0, &actor, nullptr);
    assert(submitted == 0 && direct_removed == 1 && added_affects == 2);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        source_file = path / "faerie_sight.cpp"
        binary = path / "faerie_sight"
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("faerie sight item retirement: ok")


if __name__ == "__main__":
    main()
