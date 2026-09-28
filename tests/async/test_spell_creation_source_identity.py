#!/usr/bin/env python3
"""Exercise live spell grant IDs independently of allocated item UIDs."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function


def main() -> None:
    source_id = extract_function("spell_conjuration.c", "static uint64_t spell_creation_source_id(")
    room = extract_function("spell_conjuration.c", "static bool submit_spell_room_creation(")
    player = extract_function("spell_conjuration.c", "static bool submit_spell_player_creation(")
    program = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#define FALSE 0
struct character { int in_room = 123; bool npc = false; };
struct object { uint64_t obj_uid = 0; bool nowhere = true; bool extracted = false; };
using P_char = character *;
using P_obj = object *;
struct critical_operation_id { std::array<uint8_t, 16> bytes = {}; };
enum class economic_source_kind { spell_creation };
using item_creation_grant_completion_fn = void (*)(P_char, uint64_t, bool, unsigned int);
int generated = 0, submitted = 0, extracted = 0, messages = 0;
uint64_t last_source = 0, last_uid = 0;
bool generation_ready = true, submission_ready = true;
bool critical_operation_id_generate(critical_operation_id *operation) {
    if (!generation_ready) return false;
    uint64_t value = 7000 + ++generated;
    for (size_t index = 0; index < 8; ++index)
        operation->bytes[index] = static_cast<uint8_t>(value >> (index * 8));
    return true;
}
void spell_room_creation_completed(P_char, uint64_t, bool, unsigned int) {}
void spell_player_creation_completed(P_char, uint64_t, bool, unsigned int) {}
bool item_creation_grant_submit_to_room(P_char, P_obj object, int room,
        economic_source_kind source, item_creation_grant_completion_fn completion,
        uint64_t source_id) {
    assert(room == 123 && source == economic_source_kind::spell_creation);
    assert(completion == spell_room_creation_completed);
    ++submitted;
    last_source = source_id;
    last_uid = object->obj_uid;
    return submission_ready;
}
bool item_creation_grant_submit_to_player_with_completion(P_char actor, P_obj object,
        P_char recipient, P_obj target, item_creation_grant_completion_fn completion,
        economic_source_kind source, uint64_t source_id) {
    assert(actor == recipient && !target && source == economic_source_kind::spell_creation);
    assert(completion == spell_player_creation_completed);
    ++submitted;
    last_source = source_id;
    last_uid = object->obj_uid;
    return submission_ready;
}
#define OBJ_NOWHERE(obj) ((obj)->nowhere)
#define IS_PC(ch) (!(ch)->npc)
void extract_obj(P_obj object, int) { object->extracted = true; ++extracted; }
void send_to_char(const char *, P_char) { ++messages; }
''' + source_id + "\n" + room + "\n" + player + r'''
int main() {
    character actor;
    object first {101}, second {202}, third {303};
    assert(submit_spell_room_creation(&actor, &first));
    assert(last_source == 7001 && last_uid == 101 && !first.extracted);
    assert(submit_spell_player_creation(&actor, &second));
    assert(last_source == 7002 && last_uid == 202 && !second.extracted);
    assert(submitted == 2 && generated == 2);
    submission_ready = false;
    assert(!submit_spell_player_creation(&actor, &third));
    assert(last_source == 7003 && last_uid == 303 && third.extracted);
    assert(submitted == 3 && extracted == 1 && messages == 1);
    generation_ready = false;
    object fourth {404};
    assert(!submit_spell_room_creation(&actor, &fourth));
    assert(submitted == 3 && fourth.extracted && extracted == 2 && messages == 2);
    actor.npc = true;
    generation_ready = true;
    object fifth {505};
    assert(!submit_spell_room_creation(&actor, &fifth));
    assert(generated == 3 && submitted == 3 && fifth.extracted);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        source_file = Path(directory) / "spell_creation_source_identity.cpp"
        binary = source_file.with_suffix("")
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)

    for name, boundary in (("spell_flame_blade", "blade = read_object"),
                           ("spell_shield", "shield = read_object"),
                           ("spell_create_food", "food = read_object"),
                           ("spell_doom_blade", "weapon = read_object")):
        body = extract_function("spell_conjuration.c", f"void {name}(")
        assert body.index("economic_gameplay_authority::active() && !IS_PC(ch)") < body.index(boundary)
    minor = extract_function("spell_conjuration.c", "void spell_minor_creation(")
    assert minor.index("economic_gameplay_authority::active() && !IS_PC(ch)") < minor.index("SET_BIT(obj->extra2_flags")
    print("spell creation source identity: ok")


if __name__ == "__main__":
    main()
