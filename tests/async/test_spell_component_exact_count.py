#!/usr/bin/env python3
"""Require a complete component set before submitting a priced spell sink."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


def main() -> None:
    magic = source("magic.c").read_text(encoding="utf-8")
    start = magic.index("struct spell_component_retirement_context\n{")
    context = magic[start:magic.index("\n};", start) + 3]
    consume = extract_function("magic.c", "bool spell_consume_components(")
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

struct object {
    uint64_t obj_uid = 0;
    int R_num = 0;
    object *next_content = nullptr;
};
struct character {
    int pid = 42;
    bool npc = false;
    object *carrying = nullptr;
};
using P_obj = object *;
using P_char = character *;
#define IS_PC(ch) (!(ch)->npc)
#define GET_PID(ch) ((ch)->pid)
struct index_data { int virtual_number; };
index_data indexes[] = {{100}};
index_data *obj_index = indexes;
enum class item_owner_type { player, destruction };
struct item_owner_identity { item_owner_type type; uint64_t id; uint64_t context; };
enum class item_transfer_reason { destruction };
enum class item_transfer_continuation_kind { none, spell_component_retirement };
enum class item_spell_component_effect { faerie_sight = 1, vines = 6 };
struct item_transfer_continuation {
    item_transfer_continuation_kind kind = item_transfer_continuation_kind::none;
    std::vector<uint8_t> data;
};
enum class economic_source_kind { spell_consumption };
enum class item_movement_reject { none };
struct item_transfer_result {};
struct critical_operation_id {};
enum class spell_component_effect_status { retry, waiting_for_owner, complete };
using item_movement_completion_fn = void (*)(P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t);
using spell_component_effect_completion_fn = spell_component_effect_status (*)(const critical_operation_id &, P_char,
    bool, const item_transfer_result &, unsigned int, const uint8_t *, size_t);
using item_movement_publication_fn = bool (*)(const critical_operation_id &, P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t);
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 128;
bool active_epoch = true;
class economic_gameplay_authority {
public:
    static bool active() { return active_epoch; }
};
int submitted = 0;
size_t submitted_roots = 0;
std::array<uint64_t, 8> submitted_uids = {};
spell_component_effect_status spell_faerie_sight_component_completed(const critical_operation_id &, P_char, bool, const item_transfer_result &,
    unsigned int, const uint8_t *, size_t) { return spell_component_effect_status::complete; }
spell_component_effect_completion_fn spell_component_effect_callback(item_spell_component_effect effect) {
    return effect == item_spell_component_effect::faerie_sight ?
        spell_faerie_sight_component_completed : nullptr;
}
bool spell_component_retirement_published(const critical_operation_id &, P_char, bool,
    const item_transfer_result &, unsigned int, const uint8_t *, size_t) { return true; }
bool item_movement_transaction_submit_batch(P_char, P_obj const *roots,
    size_t count, P_obj, const item_owner_identity &, const item_owner_identity &,
    item_transfer_reason, int64_t, item_movement_completion_fn,
    const void *context, size_t context_size, P_obj, item_movement_reject *,
    item_movement_publication_fn publication, economic_source_kind, uint64_t,
    const item_transfer_continuation &continuation_data) {
    assert(publication == spell_component_retirement_published);
    assert(context_size <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);
    ++submitted;
    submitted_roots = count;
    for (size_t index = 0; index < count; ++index)
        submitted_uids[index] = roots[index]->obj_uid;
    assert(context);
    assert(continuation_data.kind ==
           item_transfer_continuation_kind::spell_component_retirement);
    assert(continuation_data.data.size() == 6 && continuation_data.data[0] == 1);
    assert(continuation_data.data[1] == 1 && continuation_data.data[5] == 0);
    return true;
}
''' + context + "\n" + consume + r'''
void continuation(P_char, bool, const item_transfer_result &,
                  unsigned int, const uint8_t *, size_t) {}
int main() {
    character actor;
    object first {10};
    object second {20};
    actor.carrying = &first;
    assert(!spell_consume_components(&actor, 100, 2, 200,
                                     item_spell_component_effect::faerie_sight,
                                     spell_faerie_sight_component_completed,
                                     nullptr, 0, true));
    assert(submitted == 0);
    assert(spell_consume_components(&actor, 100, 2, 200,
                                    item_spell_component_effect::faerie_sight,
                                    spell_faerie_sight_component_completed,
                                    nullptr, 0, false));
    assert(submitted == 1 && submitted_roots == 1 && submitted_uids[0] == 10);
    assert(spell_consume_components(&actor, 100, 1, 200,
                                    item_spell_component_effect::faerie_sight,
                                    spell_faerie_sight_component_completed,
                                    nullptr, 0, true));
    assert(submitted == 2 && submitted_roots == 1);
    first.next_content = &second;
    assert(spell_consume_components(&actor, 100, 2, 200,
                                    item_spell_component_effect::faerie_sight,
                                    spell_faerie_sight_component_completed,
                                    nullptr, 0, true));
    assert(submitted == 3 && submitted_roots == 2 && submitted_uids[1] == 20);
    active_epoch = false;
    assert(!spell_consume_components(&actor, 100, 2, 200,
                                     item_spell_component_effect::faerie_sight,
                                     spell_faerie_sight_component_completed,
                                     nullptr, 0, true));
    assert(submitted == 3);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        source_file = path / "spell_component_exact_count.cpp"
        binary = path / "spell_component_exact_count"
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("spell component exact count: ok")


if __name__ == "__main__":
    main()
