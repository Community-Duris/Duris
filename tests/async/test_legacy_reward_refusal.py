#!/usr/bin/env python3
"""Execute the production ADD_MONEY and do_split bodies in a focused harness.

The harness stubs the runtime boundary (authority projection, transaction submit,
pickup, and game messaging) to verify these function branches. It is not SQL,
activation, or comprehensive native coverage.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC


def extract_function(source_path, signature):
    source = source_path.read_text()
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if not depth:
            return source[start : end + 1]
    raise AssertionError(f"unterminated production function: {signature}")


PRELUDE = r'''
#include <cassert>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr int MAX_INPUT_LENGTH = 256;
constexpr int MAX_STRING_LENGTH = 1024;
constexpr int LOG_EXIT = 1;
constexpr int LOG_WIZ = 2;
constexpr int PLAYER_COMPONENT_STATUS = 1;
constexpr int TO_ROOM = 1;
constexpr int TO_VICT = 2;
constexpr int MAXLVL = 100;
constexpr size_t CURRENCY_PENDING_CONTEXT_MAX_BYTES = 64;

struct group_list;
struct character {
    bool is_pc = false;
    int pid = 0;
    int platinum = 0;
    int gold = 0;
    int silver = 0;
    int copper = 0;
    int in_room = 0;
    bool visible = true;
    int level = 1;
    uint64_t runtime_id = 0;
    character *original = nullptr;
    const char *name = "player";
    group_list *group = nullptr;
};
struct group_list {
    character *ch = nullptr;
    group_list *next = nullptr;
};
struct object;
using P_char = character *;
using P_obj = object *;
void ADD_MONEY(P_char, int, const char *committed_message = nullptr);

#define IS_PC(ch) ((ch)->is_pc)
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_PLATINUM(ch) ((ch)->platinum)
#define GET_GOLD(ch) ((ch)->gold)
#define GET_SILVER(ch) ((ch)->silver)
#define GET_COPPER(ch) ((ch)->copper)
#define IS_MORPH(ch) ((ch)->original != nullptr)
#define GET_PLYR(ch) ((ch)->original ? (ch)->original : (ch))
#define CAN_SEE(ch, target) ((target)->visible)
#define GET_NAME(ch) ((ch)->name)

struct currency_command_result {};
enum class currency_reason_type { wallet_reward };
enum class critical_source_site { command };
enum class critical_deadline_class { interactive };
using currency_completion_fn = void (*)(P_char, bool,
    const currency_command_result &, unsigned int, const uint8_t *, size_t);
struct coin_transfer_endpoint {
    P_char owner = nullptr;
    std::array<int32_t, 4> before = {};
    std::array<int32_t, 4> after = {};
};
struct coin_transfer_payload {
    coin_transfer_endpoint source, destination;
};
struct coin_transfer_result {};
using coin_completion_fn = bool (*)(P_char, bool, const coin_transfer_payload &,
    const coin_transfer_result &, unsigned int, const uint8_t *, size_t);

bool active_mode = false;
bool wallet_submit_result = true;
bool pickup_result = true;
bool sub_money_result = true;
int wallet_submit_calls = 0;
int wallet_submit_pid = 0;
int64_t wallet_submit_delta = 0;
int pickup_calls = 0;
int pickup_pid = 0;
int pickup_amount = 0;
int sender_debit_calls = 0;
int sender_debit_amount = 0;
int dirty_calls = 0;
int gmcp_calls = 0;
int log_calls = 0;
int act_calls = 0;
int coin_submit_calls = 0;
int exact_endpoint_calls = 0;
bool coin_pending = false;
P_char pending_coin_actor = nullptr;
coin_transfer_payload pending_coin_payload;
coin_completion_fn pending_coin_completion = nullptr;
std::vector<uint8_t> pending_coin_context;
std::vector<P_char> live_characters;
std::vector<std::string> sent_messages;
std::vector<P_char> sent_targets;

class economic_gameplay_authority {
public:
    static bool active() { return active_mode; }
};

static void currency_adjustment_committed(P_char, bool,
    const currency_command_result &, unsigned int, const uint8_t *, size_t) {}

bool currency_transaction_submit_wallet_value(P_char ch, int64_t delta,
    currency_reason_type, int64_t, critical_source_site,
    critical_deadline_class, currency_completion_fn, const void *, size_t)
{
    ++wallet_submit_calls;
    wallet_submit_pid = GET_PID(ch);
    wallet_submit_delta = delta;
    return wallet_submit_result;
}

P_char find_character_by_runtime_id(uint64_t runtime_id)
{
    for (P_char character : live_characters)
        if (character->runtime_id == runtime_id)
            return character;
    return nullptr;
}
P_char find_player_by_pid(int pid)
{
    for (P_char character : live_characters)
        if (IS_PC(character) && GET_PID(character) == pid)
            return character;
    return nullptr;
}
bool currency_transaction_coin_wallet_exact(P_char character, uint8_t denomination,
                                             int32_t amount, bool debit,
                                             coin_transfer_endpoint *endpoint)
{
    ++exact_endpoint_calls;
    if (!character || !IS_PC(character) || denomination >= 4 || amount <= 0)
        return false;
    endpoint->owner = character;
    endpoint->before = {character->copper, character->silver,
                        character->gold, character->platinum};
    endpoint->after = endpoint->before;
    endpoint->after[denomination] += debit ? -amount : amount;
    return endpoint->after[denomination] >= 0;
}
bool currency_transaction_submit_coin(P_char actor, const coin_transfer_payload &payload,
                                      coin_completion_fn completion, const void *context,
                                      size_t context_size)
{
    assert(!coin_pending && context && context_size <= CURRENCY_PENDING_CONTEXT_MAX_BYTES);
    coin_pending = true;
    pending_coin_actor = actor;
    pending_coin_payload = payload;
    pending_coin_completion = completion;
    const uint8_t *bytes = static_cast<const uint8_t *>(context);
    pending_coin_context.assign(bytes, bytes + context_size);
    ++coin_submit_calls;
    return true;
}
void complete_coin(bool committed)
{
    assert(coin_pending);
    coin_pending = false;
    const auto payload = pending_coin_payload;
    const auto context = pending_coin_context;
    const auto completion = pending_coin_completion;
    const P_char actor = pending_coin_actor;
    if (committed)
        for (const auto *endpoint : {&payload.source, &payload.destination})
        {
            endpoint->owner->copper = endpoint->after[0];
            endpoint->owner->silver = endpoint->after[1];
            endpoint->owner->gold = endpoint->after[2];
            endpoint->owner->platinum = endpoint->after[3];
        }
    assert(completion(actor, committed, payload, {}, 0,
                      context.data(), context.size()));
}

bool insert_money_pickup(int pid, int amount)
{
    ++pickup_calls;
    pickup_pid = pid;
    pickup_amount = amount;
    return pickup_result;
}

void send_to_char(const char *message, P_char ch)
{
    sent_messages.emplace_back(message);
    sent_targets.push_back(ch);
}

void logit(int, const char *, ...) { ++log_calls; }
void mark_player_dirty_components(int, int) { ++dirty_calls; }
void gmcp_char_vitals(P_char) { ++gmcp_calls; }
int SUB_MONEY(P_char, int amount, int)
{
    ++sender_debit_calls;
    sender_debit_amount = amount;
    return sub_money_result ? 0 : -1;
}
void act(const char *, int, P_char, P_obj, P_char, int) { ++act_calls; }

const char *coin_names[] = {"copper", "silver", "gold", "platinum"};

void half_chop(char *argument, char *first, char *second)
{
    while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
        ++argument;
    char *write = first;
    while (*argument && !std::isspace(static_cast<unsigned char>(*argument)))
        *write++ = *argument++;
    *write = '\0';
    while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
        ++argument;
    std::strcpy(second, argument);
}

int coin_type(char *type)
{
    if (!std::strcmp(type, "copper")) return 0;
    if (!std::strcmp(type, "silver")) return 1;
    if (!std::strcmp(type, "gold")) return 2;
    if (!std::strcmp(type, "platinum")) return 3;
    return -1;
}

void reset_observations()
{
    wallet_submit_calls = 0;
    wallet_submit_pid = 0;
    wallet_submit_delta = 0;
    pickup_calls = 0;
    pickup_pid = 0;
    pickup_amount = 0;
    sender_debit_calls = 0;
    sender_debit_amount = 0;
    dirty_calls = 0;
    gmcp_calls = 0;
    log_calls = 0;
    act_calls = 0;
    coin_submit_calls = 0;
    exact_endpoint_calls = 0;
    coin_pending = false;
    pending_coin_context.clear();
    sent_messages.clear();
    sent_targets.clear();
}
'''

DRIVER = r'''
int main()
{
    character player;
    player.is_pc = true;
    player.pid = 41;
    player.platinum = 2;
    player.gold = 8;
    player.silver = 11;
    player.copper = 17;

    // Active persisted-PC rewards refuse before submit, auction fallback, or
    // direct in-memory wallet mutation.
    active_mode = true;
    reset_observations();
    ADD_MONEY(&player, 1234);
    assert(wallet_submit_calls == 0);
    assert(pickup_calls == 0);
    assert(player.platinum == 2 && player.gold == 8);
    assert(player.silver == 11 && player.copper == 17);
    assert(dirty_calls == 0 && gmcp_calls == 0);
    assert(sent_messages.size() == 1);
    assert(sent_targets[0] == &player);
    assert(sent_messages[0].find("active accounting") != std::string::npos);

    // Inactive persisted-PC credit preserves the ordinary submission route.
    active_mode = false;
    wallet_submit_result = true;
    reset_observations();
    ADD_MONEY(&player, 1234);
    assert(wallet_submit_calls == 1 && wallet_submit_pid == 41);
    assert(wallet_submit_delta == 1234);
    assert(pickup_calls == 0 && sent_messages.empty());
    assert(player.platinum == 2 && player.gold == 8);
    assert(player.silver == 11 && player.copper == 17);

    // Inactive submit failure preserves the existing auction-pickup fallback.
    wallet_submit_result = false;
    pickup_result = true;
    reset_observations();
    ADD_MONEY(&player, 1234);
    assert(wallet_submit_calls == 1 && wallet_submit_pid == 41);
    assert(pickup_calls == 1 && pickup_pid == 41 && pickup_amount == 1234);
    assert(sent_messages.size() == 1);
    assert(sent_messages[0].find("waiting at the auction house") != std::string::npos);
    assert(player.platinum == 2 && player.gold == 8);
    assert(player.silver == 11 && player.copper == 17);

    // Active mode refuses unsupported NPC cash as well as player cash.
    character npc;
    active_mode = true;
    reset_observations();
    ADD_MONEY(&npc, 1234);
    assert(wallet_submit_calls == 0 && pickup_calls == 0);
    assert(npc.platinum == 0 && npc.gold == 0);
    assert(npc.silver == 0 && npc.copper == 0);
    assert(dirty_calls == 0 && gmcp_calls == 0 && log_calls == 1);

    // Inactive NPC credit preserves the legacy direct-money behavior.
    active_mode = false;
    reset_observations();
    ADD_MONEY(&npc, 1234);
    assert(wallet_submit_calls == 0 && pickup_calls == 0);
    assert(npc.platinum == 1 && npc.gold == 2);
    assert(npc.silver == 3 && npc.copper == 4);
    assert(dirty_calls == 0 && gmcp_calls == 1);

    // Active split commits one exact-denomination child and keeps the remainder.
    character sender;
    character recipient;
    sender.is_pc = true;
    sender.pid = 52;
    sender.copper = 20;
    sender.name = "sender";
    sender.runtime_id = 1;
    recipient.is_pc = true;
    recipient.pid = 53;
    recipient.copper = 3;
    recipient.name = "recipient";
    recipient.runtime_id = 2;
    group_list sender_member{&sender, nullptr};
    group_list recipient_member{&recipient, &sender_member};
    sender.group = &recipient_member;
    sender.in_room = recipient.in_room = 7;
    live_characters = {&sender, &recipient};
    char split_args[] = "10 copper";

    active_mode = true;
    reset_observations();
    do_split(&sender, split_args, 0);
    assert(wallet_submit_calls == 0 && pickup_calls == 0);
    assert(sender_debit_calls == 0 && act_calls == 0);
    assert(sender.copper == 20 && recipient.copper == 3);
    assert(coin_submit_calls == 1 && exact_endpoint_calls == 2 && coin_pending);
    assert(pending_coin_payload.source.after[0] == 15);
    assert(pending_coin_payload.destination.after[0] == 8);
    complete_coin(true);
    assert(!coin_pending && sender.copper == 15 && recipient.copper == 8);
    assert(sent_messages.back().find("keep 5 copper") != std::string::npos);

    sender.silver = 11;
    recipient.silver = 0;
    char silver_args[] = "11 silver";
    reset_observations();
    do_split(&sender, silver_args, 0);
    assert(coin_pending && pending_coin_payload.source.after[1] == 6);
    assert(pending_coin_payload.destination.after[1] == 5);
    complete_coin(true);
    assert(sender.silver == 6 && recipient.silver == 5);
    assert(sent_messages.back().find("keep 6 silver") != std::string::npos);

    // A morphed group member receives the second child in the original wallet.
    character original;
    original.is_pc = true;
    original.pid = 54;
    original.name = "original";
    character morph;
    morph.runtime_id = 3;
    morph.original = &original;
    morph.name = "morph";
    morph.in_room = 7;
    group_list morph_member{&morph, &sender_member};
    recipient_member.next = &morph_member;
    sender.copper = 20;
    recipient.copper = 3;
    live_characters = {&sender, &recipient, &morph};
    char morph_args[] = "11 copper";
    reset_observations();
    do_split(&sender, morph_args, 0);
    assert(coin_submit_calls == 1 && coin_pending);
    complete_coin(true);
    assert(sender.copper == 17 && recipient.copper == 6);
    assert(coin_submit_calls == 2 && coin_pending);
    assert(pending_coin_payload.destination.owner == &original);
    complete_coin(true);
    assert(!coin_pending && sender.copper == 14 && original.copper == 3);
    assert(sent_messages.back().find("keep 5 copper") != std::string::npos);

    // A rejected later child preserves the earlier committed share.
    group_list duplicate_morph_member{&morph, &morph_member};
    recipient_member.next = &duplicate_morph_member;
    sender.copper = 20;
    recipient.copper = 3;
    original.copper = 0;
    reset_observations();
    do_split(&sender, morph_args, 0);
    assert(pending_coin_payload.source.after[0] == 17);
    complete_coin(true);
    assert(coin_pending && sender.copper == 17 && recipient.copper == 6);
    complete_coin(false);
    assert(!coin_pending && coin_submit_calls == 2 && original.copper == 0);
    assert(sent_messages.back().find("keep 8 copper") != std::string::npos);

    // A morph that changes its original player before admission is skipped.
    character substituted;
    substituted.is_pc = true;
    substituted.pid = 55;
    sender.copper = 20;
    recipient.copper = 3;
    reset_observations();
    do_split(&sender, morph_args, 0);
    morph.original = &substituted;
    complete_coin(true);
    assert(!coin_pending && coin_submit_calls == 1);
    assert(sender.copper == 17 && recipient.copper == 6);
    assert(original.copper == 0 && substituted.copper == 0);
    assert(sent_messages.back().find("keep 8 copper") != std::string::npos);
    morph.original = &original;

    // Inactive splitting still submits the recipient share and sender debit.
    active_mode = false;
    sender.group = &recipient_member;
    recipient_member.next = &sender_member;
    sender.copper = 20;
    recipient.copper = 3;
    wallet_submit_result = true;
    reset_observations();
    do_split(&sender, split_args, 0);
    assert(wallet_submit_calls == 1 && wallet_submit_pid == recipient.pid);
    assert(wallet_submit_delta == 5 && pickup_calls == 0);
    assert(sender_debit_calls == 1 && sender_debit_amount == 5);
    assert(act_calls == 2);

    // When sender debit fails, no recipient credit should be issued (debit-first ordering).
    sub_money_result = false;
    reset_observations();
    do_split(&sender, split_args, 0);
    assert(sender_debit_calls == 1 && sender_debit_amount == 5);
    assert(wallet_submit_calls == 0);
    assert(sent_messages.size() == 1 && sent_targets[0] == &sender);
    assert(sent_messages[0].find("enough money") != std::string::npos);

    std::puts("Money reward refusal and split child boundaries passed (function-extraction harness only).");
}
'''


if __name__ == "__main__":
    utility = extract_function(SRC / "core/utility.c", "void ADD_MONEY(P_char ch, int amount,")
    split_source = (SRC / "cmd/actoth.c").read_text()
    helpers_start = split_source.index("namespace\n{\nstruct money_split_recipient")
    helpers_end = split_source.index("} // namespace", helpers_start) + len("} // namespace")
    split_helpers = split_source[helpers_start:helpers_end]
    split = extract_function(SRC / "cmd/actoth.c", "void do_split(P_char ch, char *argument, int /*cmd*/)")
    harness = "\n".join((PRELUDE, utility, split_helpers, split, DRIVER))
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "legacy_reward_refusal.cpp"
        binary = Path(directory) / "legacy_reward_refusal"
        source.write_text(harness)
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-Wno-unused-parameter",
                str(source),
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True)
