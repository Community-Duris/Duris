#!/usr/bin/env python3
"""Compare deferred pickup bytes with synchronous output using real prompt/renderers.

Production item transactions are held at the coordinator boundary; their completion
callback queues fixture pickup text. Real prompt, ANSI, Telnet and WebSocket
serialization run against an in-memory transport.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, SRC


def extract(name, signature):
    text = (SRC / name).read_text()
    start = text.index(signature)
    depth = 0
    for end in range(text.index('{', start), len(text)):
        if text[end] == '{':
            depth += 1
        elif text[end] == '}':
            depth -= 1
            if not depth:
                return text[start:end + 1]
    raise AssertionError(signature)


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/mccp.h"
#include "net/websocket.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <cstring>
#include <gnutls/gnutls.h>

static critical_command submitted;
static const char *publication_message;
static bool publication_success;
P_char character_list = nullptr;
P_obj object_list = nullptr;
static index_data indexes[1]{};
P_index obj_index = indexes;
static room_data rooms[1]{};
P_room world = rooms;
int top_of_objt = 0;
extern const int top_of_world = 0;
static uint64_t busy_coin_uid;
bool currency_transaction_coin_item_busy(uint64_t uid) { return uid && uid == busy_coin_uid; }
void extract_obj(P_obj, int) {}
void obj_from_char(P_obj) {}
void obj_to_char(P_obj, P_char) {}
void obj_to_obj(P_obj, P_obj) {}
void obj_to_room(P_obj, int) {}
void mark_player_dirty_components(int, player_component_mask_t) {}
P_char find_player_by_pid(int pid) { return character_list && GET_PID(character_list) == pid ? character_list : nullptr; }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
critical_submit_result critical_command_coordinator_submit(critical_command command)
{
    submitted = std::move(command);
    return critical_submit_result::accepted;
}
static void publish(P_char actor, bool committed, const item_transfer_result &, unsigned,
                    const uint8_t *, size_t)
{
    assert(committed == publication_success);
    assert(!item_movement_transaction_player_busy(actor));
    write_to_q(publication_message, &actor->desc->output, 1);
}
static std::string delivered;
static std::vector<std::string> frames;
static int ga_count;
P_index mob_index = nullptr;
long sentbytes = 0;
void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}
int IS_MORPH(P_char) { return false; }
bool ac_can_see(P_char, P_char, bool) { return true; }
char *PERS(P_char, P_char, int, bool) { static char name[] = "someone"; return name; }
char *FirstWord(char *s) { return s; }
affected_type *get_ward_from_char(P_char) { return nullptr; }
void delete_doubledollar(char *) {}
void panic_corruption(const char *, const char *, ...) { abort(); }
void __free(void *p, const char *, int) { free(p); }
void format_to_snoopers(const char *in, char *out) { strcpy(out, in); }
void append_prompt(P_char, char *) {}
void write_to_pc_log(P_char, const char *, int) {}
void send_to_char(const char *text, P_char ch) { write_to_q(text, &ch->desc->output, 1); }
extern "C" ssize_t __wrap_write(int, const void *p, size_t n)
{
    std::string bytes((const char *)p, n);
    if (bytes == std::string("\xff\xf9", 2)) ++ga_count;
    delivered += bytes;
    return n;
}
extern "C" ssize_t gnutls_record_send(gnutls_session_t, const void *, size_t) { abort(); }
extern "C" const char *gnutls_strerror(int) { return "fixture"; }
int websocket_send_text(P_desc, const char *text) { frames.emplace_back(text); return 0; }
void write_to_q(const char *text, struct txt_q *q, const int)
{
    auto *block = (txt_block *)calloc(1, sizeof(txt_block));
    block->text = strdup(text);
    if (q->tail) q->tail->next = block; else q->head = block;
    q->tail = block;
}
'''

DRIVER = r'''
static std::string output_bytes(bool websocket)
{
    if (!websocket) return delivered;
    std::string bytes;
    for (const auto &frame : frames) bytes += frame;
    return bytes;
}
static std::string run(bool delayed, bool websocket, int flags, bool two_line,
                       bool fighting, const char *message, bool ambient = false)
{
    descriptor_data d{};
    char_data actor{}, enemy{};
    pc_only_data pc{};
    actor.only.pc = &pc;
    actor.desc = &d;
    actor.specials.act = flags;
    actor.specials.position = POS_STANDING | STAT_NORMAL;
    actor.points.hit = actor.points.max_hit = 100;
    actor.points.vitality = actor.points.max_vitality = 100;
    pc.screen_length = 24;
    pc.prompt = PROMPT_HIT | (two_line ? PROMPT_TWOLINE : 0);
    if (fighting) actor.specials.fighting = &enemy;
    d.character = &actor;
    d.prompt_mode = TRUE;
    d.websocket = websocket;
    delivered.clear(); frames.clear(); ga_count = 0;
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    pc.pid = 42;
    character_list = &actor;
    obj_data items[2]{}, container{};
    const bool in_container = strstr(message, "from") != nullptr;
    indexes[0].virtual_number = 100;
    world[0].number = 500;
    const item_owner_identity source_owner{item_owner_type::room, 500, 0};
    const item_owner_identity target_owner{item_owner_type::player, 42, 0};
    const int count = strstr(message, "shield") ? 2 : 1;
    P_obj roots[] = {&items[0], &items[1]};
    for (int i = 0; i < count; ++i)
    {
        items[i].obj_uid = 100 + i;
        items[i].R_num = 0;
        items[i].loc_p = in_container ? LOC_INSIDE : LOC_ROOM;
        if (in_container) items[i].loc.inside = &container;
        else items[i].loc.room = 0;
        items[i].next = items[i].next_content = i + 1 < count ? &items[i + 1] : nullptr;
        assert(item_ownership_runtime_hydrate({(uint64_t)(100+i),
            in_container ? 200 : (uint64_t)(100+i), in_container ? 200u : 0u,
            source_owner, 1, 3, 100, item_custody_state::active}));
    }
    if (in_container)
    {
        container.obj_uid = 200;
        container.loc_p = LOC_ROOM;
        container.loc.room = 0;
        container.contains = items;
        container.next = items;
        assert(item_ownership_runtime_hydrate({200, 200, 0, source_owner, 1, 3, 100,
                                              item_custody_state::active}));
    }
    object_list = world[0].contents = in_container ? &container : items;
    assert(item_ownership_runtime_hydrate_owner(target_owner, 7));
    if (delayed)
    {
        publication_message = message;
        publication_success = message[0] == 'Y';
        if (count == 2)
            assert(item_movement_transaction_submit_batch(&actor, roots, count, nullptr,
                source_owner, target_owner, item_transfer_reason::player_get, 100,
                publish, nullptr, 0));
        else
            assert(item_movement_transaction_submit(&actor, items, nullptr,
                source_owner, target_owner, item_transfer_reason::player_get, 100,
                publish, nullptr, 0));
        assert(item_movement_transaction_player_busy(&actor));
        for (int pulse = 0; pulse < 6; ++pulse)
        {
            assert(process_output(&d) == 1);
            assert(delivered.empty() && frames.empty() && ga_count == 0);
        }
        if (ambient)
        {
            write_to_q("Someone says hello.\r\n", &d.output, 1);
            assert(process_output(&d) == 1);
            assert(output_bytes(websocket).find("Someone says hello.") != std::string::npos);
            assert(ga_count == 0 && !d.output.head);
            delivered.clear(); frames.clear();
        }
        critical_completion completion{};
        completion.operation_id = submitted.operation_id;
        completion.outcome = publication_success ? critical_apply_outcome::applied :
                                                  critical_apply_outcome::terminal_failure;
        item_transfer_result result{100, (uint16_t)count, 4, 8, 2, 0};
        std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded{};
        assert(item_transfer_command_encode_result(result, &encoded));
        completion.result_size = encoded.size();
        std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
        item_movement_transaction_handle_completions(&completion, 1);
        assert(!item_movement_transaction_player_busy(&actor));
    }
    else
    {
        if (message[0] != 'Y')
        {
            // A rejected submission must keep the ordinary immediate prompt.
            busy_coin_uid = 100;
            item_movement_reject rejected{};
            assert(!item_movement_transaction_submit_batch(&actor, roots, count, nullptr,
                source_owner, target_owner, item_transfer_reason::player_get, 100,
                publish, nullptr, 0, nullptr, &rejected));
            assert(rejected == item_movement_reject::pending_conflict);
            assert(!item_movement_transaction_player_busy(&actor));
            busy_coin_uid = 0;
        }
        write_to_q(message, &d.output, 1);
    }
    assert(process_output(&d) == 1);
    assert(!d.output.head);
    const auto bytes = output_bytes(websocket);
    assert(bytes.find(message[0] == 'Y' ? "You get" : "Unable to pick up") != std::string::npos);
    const bool expect_prompt = !(flags & PLR_OLDSMARTP) || fighting;
    assert(ga_count == (!websocket && expect_prompt ? 1 : 0));
    if (expect_prompt && !(flags & PLR_SMARTPROMPT))
        assert(bytes.find("100h") > bytes.find(message[0] == 'Y' ? "You get" : "Unable to pick up"));
    assert(process_output(&d) == 1);
    assert(output_bytes(websocket) == bytes); // no duplicate completion prompt
    return bytes;
}
int main()
{
    for (bool ws : {false, true})
    for (bool compact : {false, true})
    for (unsigned smart : {0u, PLR_SMARTPROMPT, PLR_OLDSMARTP, PLR_SMARTPROMPT | PLR_OLDSMARTP})
    for (bool two : {false, true})
    for (bool fighting : {false, true})
    for (const char *message : {"You get item from bag.\r\n",
                               "You get sword from corpse.\r\nYou get shield from corpse.\r\n",
                               "You get item.\r\n", "Unable to pick up item.\r\n",
                               "Unable to pick up item from bag.\r\n",
                               "Unable to pick up sword and shield from corpse.\r\n"})
    {
        int flags = smart | (compact ? PLR_COMPACT : 0);
        const auto synchronous = run(false, ws, flags, two, fighting, message);
        assert(run(true, ws, flags, two, fighting, message) == synchronous);
        run(true, ws, flags, two, fighting, message, true);
    }
    puts("Deferred bag/corpse/floor success and failure match synchronous bytes across prompt modes and transports");
}
'''


def main():
    build = ROOT / 'bin/tests'
    build.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='item-prompts-', dir=build) as directory:
        source = Path(directory) / 'harness.cpp'
        binary = Path(directory) / 'harness'
        source.write_text('\n'.join([PRELUDE,
            extract('comm.c', 'int get_from_q(struct txt_q *queue, char *dest)'),
            extract('comm.c', 'int process_output(P_desc t)'), DRIVER]))
        subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-ffunction-sections', '-fdata-sections',
                        '-fsanitize=address,undefined', '-Isrc', str(source),
                        *[str(SRC / name) for name in ['prompt.c', 'ansi.c', 'mccp.c', 'unicode.c', 'json_utils.c', 'safe_format.c',
                            'item_movement_transaction.c', 'item_ownership_runtime.c',
                            'item_transfer_command.c', 'critical_command.c',
                            'player_snapshot_capture.c', 'player_snapshot_codec.c']],
                        '-Wl,--gc-sections', '-Wl,--wrap=write', '-lz', '-lcrypto', '-o', str(binary)],
                       cwd=ROOT, check=True, timeout=120)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == '__main__':
    main()
