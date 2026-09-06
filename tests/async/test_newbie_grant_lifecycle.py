#!/usr/bin/env python3
"""Synthetic unit/runtime regression for held newbie creation grants (issue #160).

Links the production grant queue, movement runtime, ownership registry and codecs.
Only coordinator delivery, player lookup, and world mutation are fixtures. No DB,
Redis, sockets, journal, or game instance is used. Requires Linux g++/OpenSSL and
ASan/UBSan like the existing item-movement runtime tests; scheduling is explicit.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function, rel

PRELUDE = r'''
#include "core/utils.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <deque>
#include <map>
#include <string>
#include <utility>

P_obj object_list = nullptr;
P_char character_list = nullptr;
static index_data indexes[1]{};
P_index obj_index = indexes;
int top_of_objt = 0;
static room_data rooms[1]{};
P_room world = rooms;
extern const int top_of_world = 0;
static std::deque<critical_command> submitted;
static critical_submit_result submit_result = critical_submit_result::accepted;
static std::map<uint64_t, int> publications, extractions;
static std::map<int, int> dirty, commands;
static std::string fixture_messages;
void logit(const char *, const char *, ...) {}
void __free(void *p, const char *, int) { free(p); }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
bool currency_transaction_coin_item_busy(uint64_t) { return false; }
void send_to_char(const char *text, P_char ch)
{
    if (ch && ch->desc) fixture_messages += text;
}
void send_to_char(const char *text, P_char ch, int) { send_to_char(text, ch); }
void mark_player_dirty_components(int pid, player_component_mask_t) { ++dirty[pid]; }
// Deliberately preserve the real descriptor-only lookup contract: linkdead
// characters are still in character_list but cannot be found by this API.
P_char find_player_by_pid(int pid)
{
    for (P_char ch = character_list; ch; ch = ch->next)
        if (GET_PID(ch) == pid && ch->desc && ch->desc->connected == CON_PLAYING)
            return ch;
    return nullptr;
}
void obj_to_char(P_obj obj, P_char ch)
{
    assert(OBJ_NOWHERE(obj));
    item_ownership_runtime_entry row{};
    assert(item_ownership_runtime_lookup(obj->obj_uid, &row));
    assert(row.owner.type == item_owner_type::player && row.owner.id == (uint64_t)GET_PID(ch));
    ++publications[obj->obj_uid];
    obj->loc_p = LOC_CARRIED;
    obj->loc.carrying = ch;
    obj->next_content = ch->carrying;
    ch->carrying = obj;
}
void obj_from_char(P_obj obj)
{
    assert(OBJ_CARRIED(obj));
    P_obj *at = &obj->loc.carrying->carrying;
    while (*at && *at != obj) at = &(*at)->next_content;
    assert(*at == obj);
    *at = obj->next_content;
    obj->next_content = nullptr;
    obj->loc_p = LOC_NOWHERE;
}
void obj_to_obj(P_obj obj, P_obj parent)
{
    assert(OBJ_NOWHERE(obj));
    item_ownership_runtime_entry row{};
    assert(item_ownership_runtime_lookup(obj->obj_uid, &row));
    assert(row.root_item_uid == parent->obj_uid && row.parent_item_uid == parent->obj_uid);
    obj->loc_p = LOC_INSIDE;
    obj->loc.inside = parent;
    obj->next_content = parent->contains;
    parent->contains = obj;
}
void obj_to_room(P_obj, int) { abort(); }
void extract_obj(P_obj obj, int)
{
    assert(OBJ_NOWHERE(obj));
    assert(++extractions[obj->obj_uid] == 1);
    P_obj *at = &object_list;
    while (*at && *at != obj) at = &(*at)->next;
    assert(*at == obj);
    *at = obj->next;
    obj->next = nullptr;
}
critical_submit_result critical_command_coordinator_submit(critical_command command)
{
    if (submit_result == critical_submit_result::accepted) submitted.push_back(std::move(command));
    return submit_result;
}
void command_interpreter(P_char ch, char *input)
{
    assert(strcmp(input, "look") == 0);
    ++commands[GET_PID(ch)];
}
void process_with_paging(P_char, char *) { abort(); }
'''

DRIVER = r'''
struct fixture
{
    char_data actor{}, other{};
    pc_only_data pc{}, other_pc{};
    descriptor_data desc{}, other_desc{};
    obj_data bag{}, food{}, extra{};
    fixture()
    {
        item_movement_transaction_reset_for_tests();
        item_ownership_runtime_reset();
        submitted.clear(); publications.clear(); extractions.clear(); dirty.clear(); commands.clear();
        fixture_messages.clear(); submit_result = critical_submit_result::accepted;
        pc.pid = 42; other_pc.pid = 43;
        actor.only.pc = &pc; other.only.pc = &other_pc;
        actor.desc = &desc; other.desc = &other_desc;
        desc.character = &actor; other_desc.character = &other;
        desc.connected = other_desc.connected = CON_PLAYING;
        actor.next = &other; character_list = &actor;
        indexes[0].virtual_number = 100;
        int id = 100;
        for (P_obj obj : {&bag, &food, &extra})
        {
            obj->obj_uid = id++; obj->R_num = 0; obj->loc_p = LOC_NOWHERE;
            obj->next = object_list; object_list = obj;
        }
        bag.type = ITEM_CONTAINER;
        assert(item_ownership_runtime_hydrate_owner({item_owner_type::system, 0, 0}, 0));
        assert(item_ownership_runtime_hydrate_owner({item_owner_type::player, 42, 0}, 0));
        assert(item_ownership_runtime_hydrate_owner({item_owner_type::player, 43, 0}, 0));
    }
    ~fixture()
    {
        item_movement_transaction_reset_for_tests();
        item_ownership_runtime_reset();
        character_list = nullptr; object_list = nullptr;
    }
};
static critical_completion next_completion(critical_apply_outcome outcome)
{
    assert(!submitted.empty());
    critical_command command = std::move(submitted.front()); submitted.pop_front();
    item_transfer_payload payload{};
    assert(item_transfer_command_decode_payload(command, &payload));
    assert(payload.reason == item_transfer_reason::creation);
    item_transfer_result result{item_transfer_result_root(payload), payload.item_count,
        payload.expected_from_revision + 1, payload.expected_to_revision + 1, 1, 0};
    critical_completion completion{};
    completion.operation_id = command.operation_id; completion.outcome = outcome;
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded{};
    assert(item_transfer_command_encode_result(result, &encoded));
    completion.result_size = encoded.size();
    std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
    return completion;
}
static void deliver(const critical_completion &completion)
{
    item_movement_transaction_handle_completions(&completion, 1);
}
int main()
{
    // A held grant does not publish early or hold an unrelated player's dispatch.
    // After disconnect, publish to the retained character and continue its queue.
    {
        fixture f;
        assert(item_creation_grant_submit_to_player(&f.actor, &f.bag, &f.actor));
        assert(item_creation_grant_submit_to_player(&f.actor, &f.food, &f.actor, &f.bag));
        assert(item_creation_grant_mark_blocking(&f.actor));
        const auto first = next_completion(critical_apply_outcome::already_applied);
        for (int pulse = 0; pulse < 5; ++pulse)
        {
            item_movement_transaction_handle_completions(nullptr, 0);
            assert(item_movement_transaction_player_busy(&f.actor));
            assert(!item_movement_transaction_player_busy(&f.other));
            char input[] = "look"; dispatch_playing_command(&f.other, input);
            assert(publications.empty() && OBJ_NOWHERE(&f.bag) && OBJ_NOWHERE(&f.food));
        }
        assert(commands[43] == 5);
        f.actor.desc = nullptr; f.desc.character = nullptr;
        assert(find_player_by_pid(42) == nullptr);
        deliver(first);
        assert(OBJ_CARRIED_BY(&f.bag, &f.actor) && publications[100] == 1);
        assert(submitted.size() == 1 && item_creation_grant_blocks_commands(&f.actor));
        deliver(first); // duplicate completion while the next item is pending
        assert(publications[100] == 1 && submitted.size() == 1);
        const auto second = next_completion(critical_apply_outcome::applied);
        deliver(second); deliver(second);
        assert(f.bag.contains == &f.food && f.food.loc.inside == &f.bag);
        assert(publications[101] == 1 && dirty[42] == 2 && extractions.empty());
        assert(!item_movement_transaction_player_busy(&f.actor));
        assert(item_movement_transaction_health_copy().pending == 0);
        assert(fixture_messages.find("starter kit is ready") == std::string::npos);
        assert(!f.desc.prompt_mode); // never touch the detached descriptor
        f.actor.desc = &f.desc; f.desc.character = &f.actor;
        item_movement_transaction_player_ready(&f.actor);
        assert(publications[100] == 1 && publications[101] == 1);
    }
    // A rejected bag discards its dependent, still-hidden contents exactly once.
    {
        fixture f;
        assert(item_creation_grant_submit_to_player(&f.actor, &f.bag, &f.actor));
        assert(item_creation_grant_submit_to_player(&f.actor, &f.food, &f.actor, &f.bag));
        assert(item_creation_grant_mark_blocking(&f.actor));
        const auto failed = next_completion(critical_apply_outcome::terminal_failure);
        deliver(failed); deliver(failed);
        assert(extractions[100] == 1 && extractions[101] == 1 && publications.empty());
        assert(submitted.empty() && !item_movement_transaction_player_busy(&f.actor));
        assert(!item_creation_grant_blocks_commands(&f.actor) && f.desc.prompt_mode);
        assert(fixture_messages.find("starter kit is ready") == std::string::npos);
    }
    // Refused submission releases its queue; the caller still owns cleanup.
    {
        fixture f;
        submit_result = critical_submit_result::unavailable;
        assert(!item_creation_grant_submit_to_player(&f.actor, &f.bag, &f.actor));
        assert(!item_movement_transaction_player_busy(&f.actor));
        assert(OBJ_NOWHERE(&f.bag) && extractions.empty());
        submit_result = critical_submit_result::accepted;
        assert(item_creation_grant_submit_to_player(&f.actor, &f.bag, &f.actor));
        deliver(next_completion(critical_apply_outcome::applied));
        assert(publications[100] == 1 && !item_movement_transaction_player_busy(&f.actor));
    }
    // Offline retention resolves against the current character with the same PID,
    // without keeping the original descriptor or character pointer in the queue.
    {
        fixture f;
        assert(item_creation_grant_submit_to_player(&f.actor, &f.bag, &f.actor));
        const auto completed = next_completion(critical_apply_outcome::applied);
        character_list = &f.other;
        f.actor.desc = nullptr; f.desc.character = nullptr;
        deliver(completed);
        assert(publications.empty() && item_movement_transaction_health_copy().retained_offline == 1);
        char_data replacement{}; pc_only_data replacement_pc{}; descriptor_data replacement_desc{};
        replacement_pc.pid = 42; replacement.only.pc = &replacement_pc;
        replacement.desc = &replacement_desc; replacement_desc.character = &replacement;
        replacement_desc.connected = CON_PLAYING;
        replacement.next = &f.other; character_list = &replacement;
        item_movement_transaction_player_ready(&replacement);
        item_movement_transaction_player_ready(&replacement); deliver(completed);
        assert(OBJ_CARRIED_BY(&f.bag, &replacement) && f.actor.carrying == nullptr);
        assert(publications[100] == 1 && !item_movement_transaction_player_busy(&replacement));
    }
    // Chaos pre-entry submission remains valid before joining character_list.
    {
        fixture f;
        character_list = &f.other; f.desc.connected = CON_GET_RACE;
        assert(item_creation_grant_submit_to_player_before_entry(&f.actor, &f.bag, &f.actor));
        const auto completed = next_completion(critical_apply_outcome::applied);
        deliver(completed);
        assert(publications.empty() && item_movement_transaction_player_busy(&f.actor));
        character_list = &f.actor; f.desc.connected = CON_PLAYING;
        item_movement_transaction_player_ready(&f.actor);
        assert(OBJ_CARRIED_BY(&f.bag, &f.actor) && publications[100] == 1);
        assert(fixture_messages.find("Chaos Equipment has been prepared") != std::string::npos);
    }
    puts("newbie grant lifecycle runtime: ok");
}
'''


def main() -> int:
    harness = "\n".join([PRELUDE, extract_function(
        "comm.c", "static void dispatch_playing_command(P_char character, char *input)"), DRIVER])
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "newbie_grant_lifecycle.cpp"
        binary = Path(directory) / "newbie_grant_lifecycle"
        source.write_text(harness, encoding="utf-8")
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
            "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
            "-Isrc", str(source), rel("item_movement_transaction.c"),
            rel("item_ownership_runtime.c"), rel("item_transfer_command.c"),
            rel("critical_command.c"), rel("player_snapshot_capture.c"),
            rel("player_snapshot_codec.c"), "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)
    print("All newbie grant lifecycle checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
