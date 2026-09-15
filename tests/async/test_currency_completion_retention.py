#!/usr/bin/env python3
"""Retain unresolved currency receipts without publishing failure or stale balances.

Links the production transaction adapter and codecs; only the coordinator and
live-world endpoints are controlled. This is not a database integration test.
"""

from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import ROOT, rel


HARNESS = r'''
#include "core/utils.h"
#include "economy/currency_transaction.h"
#include "sql/sql_player.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

static P_char online = nullptr, online_other = nullptr;
static critical_command submitted;
static int submissions = 0, callbacks = 0, alerts = 0, bank_publications = 0;
static bool callback_committed = false, chain_after_callback = false, rehash_in_callback = false;
static unsigned int callback_error = 0;

[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }

const char *get_account_name_safe(P_char ch)
{
    return ch && GET_PID(ch) == 45 ? "other_account" : "retention_account";
}
P_char find_player_by_pid(int pid)
{
    if (online && GET_PID(online) == pid) return online;
    return online_other && GET_PID(online_other) == pid ? online_other : nullptr;
}
bool critical_command_coordinator_is_fenced(const critical_entity_key &, critical_operation_id *)
{
    // A final coordinator notification can release its fence before domain publication.
    return false;
}
critical_submit_result critical_command_coordinator_submit(critical_command command)
{
    submitted = std::move(command);
    ++submissions;
    return critical_submit_result::accepted;
}
bool critical_command_coordinator_get_completed(const critical_operation_id &, critical_completion *)
{
    return false;
}
void gmcp_char_vitals(P_char) {}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
                       const char *, ...) { ++alerts; }
void publish_account_bank_balances_revision(const char *, int, const AccountBankBalances *, uint64_t)
{
    ++bank_publications;
}

bool submit_reward(P_char actor, currency_completion_fn callback)
{
    return currency_transaction_submit_wallet_value(
        actor, 10, currency_reason_type::wallet_reward, 0,
        critical_source_site::command, critical_deadline_class::interactive,
        callback, nullptr, 0);
}

void completed(P_char actor, bool committed, const currency_command_result &, unsigned int error,
               const uint8_t *, size_t)
{
    ++callbacks;
    callback_committed = committed;
    callback_error = error;
    if (rehash_in_callback)
    {
        // Force inserts/rehashing during the real callback; the adapter must not
        // erase an invalidated iterator or accidentally erase a newly queued job.
        for (int i = 0; i < 200; ++i) assert(submit_reward(actor, nullptr));
    }
    if (chain_after_callback)
    {
        assert(!currency_transaction_player_busy(actor));
        assert(currency_transaction_submit_wallet_value(
            actor, -1, currency_reason_type::wallet_spend, 0,
            critical_source_site::command, critical_deadline_class::interactive,
            nullptr, nullptr, 0));
    }
}

void encode(critical_completion &receipt, int64_t wallet = 15, int64_t bank = 0)
{
    currency_command_result result = {};
    result.wallet.amount[0] = wallet;
    result.bank.amount[0] = bank;
    result.wallet_revision = 2;
    result.bank_revision = 2;
    std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> bytes;
    assert(currency_command_encode_result(result, &bytes));
    receipt.result_size = bytes.size();
    std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
}

bool coin_completed(P_char, bool committed, const coin_transfer_payload &,
                    const coin_transfer_result &, unsigned int error, const uint8_t *, size_t)
{
    ++callbacks;
    callback_committed = committed;
    callback_error = error;
    return true;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string scenario = argv[1];
    pc_only_data player = {};
    player.pid = 42;
    player.wallet_revision = 1;
    player.bank_revision = 1;
    char_data actor = {};
    actor.only.pc = &player;
    actor.player.racewar = 1;
    GET_COPPER(&actor) = 5;
    online = &actor;
    currency_transaction_reset_for_tests();
    assert(submit_reward(&actor, completed));
    const critical_command original = submitted;
    critical_completion receipt = {};
    receipt.operation_id = original.operation_id;
    receipt.outcome = critical_apply_outcome::applied;
    receipt.attempt = CRITICAL_COORDINATOR_MAX_RETRIES + 1;

    if (scenario == "coin_ambiguous" || scenario == "coin_exhausted_retry")
    {
        currency_transaction_reset_for_tests();
        pc_only_data recipient_player = {};
        recipient_player.pid = 45;
        char_data recipient = {};
        recipient.only.pc = &recipient_player;
        recipient.player.racewar = actor.player.racewar;
        online_other = &recipient;
        coin_transfer_payload transfer;
        assert(currency_transaction_coin_wallet(&actor, -1, &transfer.source));
        assert(currency_transaction_coin_wallet(&recipient, 1, &transfer.destination));
        assert(currency_transaction_submit_coin(&actor, transfer, coin_completed, nullptr, 0));
        receipt.operation_id = submitted.operation_id;
        receipt.outcome = scenario == "coin_ambiguous" ? critical_apply_outcome::ambiguous_commit :
                                                       critical_apply_outcome::retryable_failure;
        currency_transaction_handle_completions(&receipt, 1);
        for (int i = 0; i < 5; ++i) currency_transaction_handle_completions(nullptr, 0);
        assert(callbacks == 0 && GET_COPPER(&actor) == 5 && GET_COPPER(&recipient) == 0);
        assert(currency_transaction_player_busy(&actor) && currency_transaction_player_busy(&recipient));
        assert(currency_transaction_health_copy().pending == 1 && alerts == 1);
        receipt.outcome = critical_apply_outcome::terminal_failure;
        receipt.error_code = EACCES;
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && !callback_committed && callback_error == EACCES);
        assert(!currency_transaction_player_busy(&actor) && !currency_transaction_player_busy(&recipient));
        return 0;
    }

    if (scenario == "rejected" || scenario == "rejected_without_payload")
    {
        receipt.outcome = critical_apply_outcome::terminal_failure;
        receipt.error_code = EACCES;
        if (scenario == "rejected") encode(receipt, 5);
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && !callback_committed && callback_error == EACCES);
        assert(GET_COPPER(&actor) == 5 && !currency_transaction_player_busy(&actor));
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && currency_transaction_health_copy().rejected == 1);
        return 0;
    }
    if (scenario == "callback_chain" || scenario == "callback_rehash")
    {
        chain_after_callback = scenario == "callback_chain";
        rehash_in_callback = scenario == "callback_rehash";
        encode(receipt);
        currency_transaction_handle_completions(&receipt, 1);
        const int next_jobs = chain_after_callback ? 1 : 200;
        assert(callbacks == 1 && submissions == next_jobs + 1);
        assert(currency_transaction_player_busy(&actor));
        assert(currency_transaction_health_copy().pending == static_cast<uint64_t>(next_jobs));
        return 0;
    }

    if (scenario == "already_applied")
        receipt.outcome = critical_apply_outcome::already_applied;
    else if (scenario == "ambiguous" || scenario == "ambiguous_with_payload")
        receipt.outcome = critical_apply_outcome::ambiguous_commit;
    else if (scenario == "exhausted_retry")
        receipt.outcome = critical_apply_outcome::retryable_failure;
    else if (scenario == "wallet_range")
        encode(receipt, static_cast<int64_t>(INT_MAX) + 1);
    else if (scenario == "bank_range")
        encode(receipt, 15, -1);
    else
        assert(scenario == "malformed" || scenario == "offline" || scenario == "bounded_rebasable");
    if (scenario == "ambiguous_with_payload") encode(receipt);
    if (scenario == "offline") online = nullptr;
    currency_transaction_handle_completions(&receipt, 1);
    online = &actor;
    currency_transaction_player_ready(&actor);

    assert(callbacks == 0 && "unresolved receipt must not become a rejected transaction");
    assert(GET_COPPER(&actor) == 5 && player.wallet_revision == 1);
    assert(bank_publications == 0);
    assert(currency_transaction_health_copy().pending == 1);
    assert(currency_transaction_health_copy().rejected == 0);
    assert(currency_transaction_player_busy(&actor));
    assert(!currency_transaction_can_submit(&actor));
    assert(currency_transaction_submit_prepared(&actor, original, completed, nullptr, 0));
    assert(submissions == 1);
    assert(!currency_transaction_submit_wallet_value(
        &actor, -1, currency_reason_type::wallet_spend, 0,
        critical_source_site::command, critical_deadline_class::interactive,
        nullptr, nullptr, 0));

    pc_only_data sibling_player = {};
    sibling_player.pid = 43;
    char_data sibling = {};
    sibling.only.pc = &sibling_player;
    sibling.player.racewar = actor.player.racewar;
    assert(currency_transaction_player_busy(&sibling));
    assert(!currency_transaction_can_submit(&sibling));
    assert(!currency_transaction_submit_bank_payment(
        &sibling, 1, currency_reason_type::wallet_spend, 0,
        critical_source_site::command, critical_deadline_class::interactive,
        nullptr, nullptr, 0));
    sibling.player.racewar = 2;
    assert(!currency_transaction_player_busy(&sibling));
    assert(currency_transaction_can_submit(&sibling));
    for (int i = 0; i < 5; ++i)
        currency_transaction_handle_completions(nullptr, 0);
    assert(callbacks == 0 && submissions == 1 && alerts <= 1);

    if (scenario == "bounded_rebasable")
    {
        // Independent accounts still progress; safe rebasable rewards remain
        // admissible, but cannot turn retained uncertainty into unbounded work.
        sibling_player.pid = 45;
        sibling.player.racewar = actor.player.racewar;
        GET_COPPER(&sibling) = 5;
        assert(currency_transaction_submit_wallet_value(
            &sibling, -1, currency_reason_type::wallet_spend, 0,
            critical_source_site::command, critical_deadline_class::interactive,
            nullptr, nullptr, 0));
        for (size_t i = 2; i < CURRENCY_PENDING_MAX; ++i)
            assert(submit_reward(&actor, nullptr));
        assert(!submit_reward(&actor, nullptr));
        assert(submissions == static_cast<int>(CURRENCY_PENDING_MAX));
        assert(currency_transaction_health_copy().pending == CURRENCY_PENDING_MAX);
        assert(callbacks == 0);
        return 0;
    }

    // A corrected exact receipt can complete the retained operation; no new debit/credit.
    receipt.outcome = critical_apply_outcome::already_applied;
    encode(receipt);
    currency_transaction_handle_completions(&receipt, 1);
    assert(callbacks == 1 && callback_committed && callback_error == 0);
    assert(GET_COPPER(&actor) == 15 && player.wallet_revision == 2);
    assert(bank_publications == 1 && submissions == 1);
    assert(!currency_transaction_player_busy(&actor));
    assert(currency_transaction_health_copy().pending == 0);
    currency_transaction_handle_completions(&receipt, 1);
    assert(callbacks == 1 && bank_publications == 1);
}
'''


def main():
    failures = []
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    scenarios = (
        "malformed", "already_applied", "wallet_range", "bank_range", "ambiguous",
        "ambiguous_with_payload", "exhausted_retry", "offline", "rejected",
        "rejected_without_payload", "callback_chain", "callback_rehash", "bounded_rebasable",
        "coin_ambiguous", "coin_exhausted_retry",
    )
    with tempfile.TemporaryDirectory(prefix="currency-retention-") as directory:
        source = Path(directory) / "retention.cpp"
        source.write_text(HARNESS, encoding="utf-8")
        for flatfile in (False, True):
            backend = "flatfile" if flatfile else "mysql"
            binary = Path(directory) / backend
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
                *(["-D__NO_MYSQL__", "-Isrc/no_mysql"] if flatfile else []),
                "-Isrc", *cflags, str(source), rel("currency_transaction.c"),
                rel("currency_command.c"), rel("critical_command.c"),
                rel("coin_transfer_command.c"), rel("item_transfer_command.c"),
                rel("player_snapshot_codec.c"), "-Wl,--gc-sections", "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            for scenario in scenarios:
                result = subprocess.run([str(binary), scenario], capture_output=True, text=True)
                print(f"{backend}/{scenario}: {'PASS' if result.returncode == 0 else 'FAIL'}", flush=True)
                if result.returncode:
                    failures.append(f"{backend}/{scenario}: {result.stdout}{result.stderr}")
    if failures:
        raise AssertionError("\n".join(failures))
    print(f"Currency completion retention checks passed ({2 * len(scenarios)} sanitizer scenarios).")


if __name__ == "__main__":
    main()
