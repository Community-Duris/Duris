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
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

constexpr int MAX_INPUT_LENGTH = 256;
constexpr int MAX_STRING_LENGTH = 1024;
constexpr int LOG_EXIT = 1;
constexpr int LOG_WIZ = 2;
constexpr int PLAYER_COMPONENT_STATUS = 1;
constexpr int TO_ROOM = 1;
constexpr int TO_VICT = 2;

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

#define IS_PC(ch) ((ch)->is_pc)
#define GET_PID(ch) ((ch)->pid)
#define GET_PLATINUM(ch) ((ch)->platinum)
#define GET_GOLD(ch) ((ch)->gold)
#define GET_SILVER(ch) ((ch)->silver)
#define GET_COPPER(ch) ((ch)->copper)
#define IS_MORPH(ch) (false)
#define CAN_SEE(ch, target) ((target)->visible)
#define GET_NAME(ch) ((ch)->name)

struct currency_command_result {};
enum class currency_reason_type { wallet_reward };
enum class critical_source_site { command };
enum class critical_deadline_class { interactive };
using currency_completion_fn = void (*)(P_char, bool,
    const currency_command_result &, unsigned int, const uint8_t *, size_t);

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

    // Active mode does not change the legacy direct-wallet path for NPCs.
    character npc;
    active_mode = true;
    reset_observations();
    ADD_MONEY(&npc, 1234);
    assert(wallet_submit_calls == 0 && pickup_calls == 0);
    assert(npc.platinum == 1 && npc.gold == 2);
    assert(npc.silver == 3 && npc.copper == 4);
    assert(dirty_calls == 0 && gmcp_calls == 1);

    // Active split refuses before recipient ADD_MONEY effects or sender debit.
    character sender;
    character recipient;
    sender.is_pc = true;
    sender.pid = 52;
    sender.copper = 20;
    sender.name = "sender";
    recipient.is_pc = true;
    recipient.pid = 53;
    recipient.copper = 3;
    recipient.name = "recipient";
    group_list sender_member{&sender, nullptr};
    group_list recipient_member{&recipient, &sender_member};
    sender.group = &recipient_member;
    sender.in_room = recipient.in_room = 7;
    char split_args[] = "10 copper";

    reset_observations();
    do_split(&sender, split_args, 0);
    assert(wallet_submit_calls == 0 && pickup_calls == 0);
    assert(sender_debit_calls == 0 && act_calls == 0);
    assert(sender.copper == 20 && recipient.copper == 3);
    assert(sent_messages.size() == 1 && sent_targets[0] == &sender);
    assert(sent_messages[0].find("unavailable") != std::string::npos);

    // Inactive splitting still submits the recipient share and sender debit.
    active_mode = false;
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

    std::puts("Legacy reward refusal branches passed (function-extraction harness only).");
}
'''


if __name__ == "__main__":
    utility = extract_function(SRC / "core/utility.c", "void ADD_MONEY(P_char ch, int amount)")
    split = extract_function(SRC / "cmd/actoth.c", "void do_split(P_char ch, char *argument, int /*cmd*/)")
    harness = "\n".join((PRELUDE, utility, split, DRIVER))
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
