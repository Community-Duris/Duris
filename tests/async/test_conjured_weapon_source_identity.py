#!/usr/bin/env python3
"""Exercise the typed source ID and active NPC refusal for conjured weapons."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


def main() -> None:
    lifecycle = source("spell_item_lifecycle.c").read_text(encoding="utf-8")
    context_start = lifecycle.index("enum class conjured_weapon_kind : uint8_t\n{")
    context_end = lifecycle.index("\nstatic_assert(sizeof(conjured_weapon_grant_context)", context_start)
    definitions = lifecycle[context_start:context_end]
    source_id = extract_function("spell_item_lifecycle.c", "static uint64_t conjured_weapon_source_id(")
    submit = extract_function("spell_item_lifecycle.c", "static bool submit_conjured_weapon(")
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#define FALSE 0
struct character { int pid = 42; int hit = 1000; bool npc = false; };
struct object { uint64_t obj_uid = 0; bool nowhere = true; bool extracted = false; };
using P_char = character *;
using P_obj = object *;
struct critical_operation_id { std::array<uint8_t, 16> bytes = {}; };
struct item_transfer_result { uint64_t root_item_uid = 0; uint16_t item_count = 0; };
using item_movement_completion_fn = void (*)(P_char, bool, const item_transfer_result &,
    unsigned int, const uint8_t *, size_t);
enum class economic_source_kind { spell_creation };
enum class conjured_weapon_kind : uint8_t;
int generated = 0, submitted = 0, extracted = 0, messages = 0, published = 0;
uint64_t last_source = 0, last_uid = 0;
bool active_epoch = true, generation_ready = true, submission_ready = true;
bool critical_operation_id_generate(critical_operation_id *operation) {
    if (!generation_ready) return false;
    uint64_t value = 9000 + ++generated;
    for (size_t index = 0; index < 8; ++index)
        operation->bytes[index] = static_cast<uint8_t>(value >> (index * 8));
    return true;
}
class economic_gameplay_authority {
public:
    static bool active() { return active_epoch; }
};
item_movement_completion_fn conjured_weapon_grant_completed =
    +[](P_char, bool, const item_transfer_result &, unsigned int, const uint8_t *, size_t) {};
bool item_creation_grant_submit_to_player_with_completion(P_char actor, P_obj object,
        P_char recipient, item_movement_completion_fn callback, const void *, size_t,
        P_obj target, economic_source_kind source, uint64_t source_id) {
    assert(actor == recipient && callback == conjured_weapon_grant_completed && !target);
    assert(source == economic_source_kind::spell_creation);
    ++submitted;
    last_source = source_id;
    last_uid = object->obj_uid;
    return submission_ready;
}
#define IS_PC(ch) (!(ch)->npc)
#define GET_PID(ch) ((ch)->pid)
#define GET_HIT(ch) ((ch)->hit)
#define OBJ_NOWHERE(obj) ((obj)->nowhere)
void extract_obj(P_obj object, int) { object->extracted = true; ++extracted; }
void obj_to_char(P_obj object, P_char) { object->nowhere = false; }
void conjured_weapon_publish_effect(P_char, P_obj, conjured_weapon_kind, float) { ++published; }
void send_to_char(const char *, P_char) { ++messages; }
''' + definitions + "\n" + source_id + "\n" + submit + r'''
int main() {
    character actor;
    object first {101}, second {202}, third {303}, fourth {404};
    assert(submit_conjured_weapon(&actor, &first, conjured_weapon_kind::ensis_unguis));
    assert(submitted == 1 && last_source == 9001 && last_uid == 101 && !first.extracted);
    submission_ready = false;
    assert(!submit_conjured_weapon(&actor, &second, conjured_weapon_kind::lancea_cineralae));
    assert(submitted == 2 && last_source == 9002 && last_uid == 202 && second.extracted);
    generation_ready = false;
    assert(!submit_conjured_weapon(&actor, &third, conjured_weapon_kind::simulacrum_anguis));
    assert(submitted == 2 && third.extracted && messages == 2);
    actor.npc = true;
    generation_ready = true;
    assert(!submit_conjured_weapon(&actor, &fourth, conjured_weapon_kind::ensis_unguis));
    assert(generated == 2 && submitted == 2 && fourth.extracted && published == 0);
    active_epoch = false;
    object fifth {505};
    assert(submit_conjured_weapon(&actor, &fifth, conjured_weapon_kind::ensis_unguis));
    assert(!fifth.nowhere && published == 1 && generated == 2);
    actor.npc = false;
    submission_ready = true;
    object sixth {606};
    assert(submit_conjured_weapon(&actor, &sixth, conjured_weapon_kind::lancea_cineralae));
    assert(submitted == 3 && last_source == 9003 && last_uid == 606 && generated == 3);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        source_file = Path(directory) / "conjured_weapon_source_identity.cpp"
        binary = source_file.with_suffix("")
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("conjured weapon source identity: ok")


if __name__ == "__main__":
    main()
