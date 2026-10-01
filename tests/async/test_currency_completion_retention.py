#!/usr/bin/env python3
"""Retain unresolved currency receipts without publishing failure or stale balances.

Links the production transaction adapter and codecs; only the coordinator and
live-world endpoints are controlled. This is not a database integration test.
"""

from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from _paths import ROOT, rel


HARNESS = r'''
#include "core/utils.h"
#include "economy/account_bank_balances.h"
#include "economy/currency_transaction.h"
#include "economy/economic_currency_adapter.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_snapshot_codec.h"
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
P_desc descriptor_list = nullptr;
static critical_command submitted;
static int submissions = 0, callbacks = 0, alerts = 0, bank_publications = 0;
static AccountBankBalances last_bank_balances = {};
static int held_submissions = 0, publication_acks = 0;
static bool ack_available = true;
static bool callback_committed = false, chain_after_callback = false, rehash_in_callback = false;
static unsigned int callback_error = 0;
static std::string last_alert_operation;

class economic_gameplay_authority_test_access
{
public:
    static void install(const critical_operation_id &lineage,
                        const critical_operation_id &epoch,
                        const critical_operation_id &receipt)
    {
        const std::array wallets = {
            economic_gameplay_wallet_mapping{42, {lineage, economic_account_kind::wallet, 42, 0}},
            economic_gameplay_wallet_mapping{45, {lineage, economic_account_kind::wallet, 45, 0}}
        };
        const std::array banks = { economic_gameplay_bank_mapping{
            "retention_account", 1, {lineage, economic_account_kind::bank, 52, 1}} };
        assert(economic_gameplay_authority::install(lineage, epoch, receipt,
                                                   wallets, banks) ==
               economic_accounting_error::ok);
    }
};

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
int IS_MORPH(P_char ch)
{
    return ch && IS_NPC(ch) && ch->only.npc && ch->only.npc->orig_char;
}
bool critical_command_coordinator_is_fenced(const critical_entity_key &, critical_operation_id *)
{
    // A final coordinator notification can release its fence before domain publication.
    return false;
}
critical_submit_result critical_command_coordinator_submit(critical_command command)
{
    command.accepted_at_usec = 1; // assigned by the actual coordinator at admission
    submitted = std::move(command);
    ++submissions;
    return critical_submit_result::accepted;
}
critical_submit_result critical_command_coordinator_submit_for_publication(critical_command command)
{
    command.publication_required = true;
    command.accepted_at_usec = 1; // assigned by the actual coordinator at admission
    submitted = std::move(command);
    ++held_submissions;
    return critical_submit_result::accepted;
}
bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &)
{
    ++publication_acks;
    return ack_available;
}
bool critical_command_coordinator_get_completed(const critical_operation_id &, critical_completion *)
{
    return false;
}
void gmcp_char_vitals(P_char) {}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *operation, const char *,
                       const char *, const char *, ...)
{
    ++alerts;
    last_alert_operation = operation ? operation : "";
}
void publish_account_bank_balances_revision(const char *, int,
                                            const AccountBankBalances *balances, uint64_t)
{
    ++bank_publications;
    last_bank_balances = *balances;
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
    if (scenario == "exact_morph_coin")
    {
        critical_operation_id lineage = {}, epoch = {}, activation = {};
        lineage.bytes[0] = 1;
        epoch.bytes[0] = 2;
        activation.bytes[0] = 3;
        economic_gameplay_authority_test_access::install(lineage, epoch, activation);
        GET_SILVER(&actor) = 12;
        pc_only_data recipient_player = {};
        recipient_player.pid = 45;
        recipient_player.wallet_revision = 1;
        recipient_player.bank_revision = 1;
        char_data recipient = {};
        recipient.only.pc = &recipient_player;
        recipient.player.racewar = actor.player.racewar;
        npc_only_data morph_data = {};
        morph_data.orig_char = &recipient;
        char_data morph = {};
        morph.specials.act |= ACT_ISNPC;
        morph.only.npc = &morph_data;
        descriptor_data descriptor = {};
        descriptor.connected = CON_PLAYING;
        descriptor.character = &morph;
        descriptor_list = &descriptor;
        coin_transfer_payload transfer = {};
        assert(currency_transaction_coin_wallet_exact(&actor, 1, 10, true,
                                                       &transfer.source));
        assert(currency_transaction_coin_wallet_exact(&recipient, 1, 10, false,
                                                       &transfer.destination));
        assert((transfer.source.after == std::array<int32_t, 4>{5, 2, 0, 0}));
        assert((transfer.destination.after == std::array<int32_t, 4>{0, 10, 0, 0}));
        assert(currency_transaction_submit_coin(&actor, transfer, coin_completed, nullptr, 0));
        assert(held_submissions == 1 && submissions == 0);
        coin_transfer_result result = {};
        const coin_transfer_endpoint *endpoints[] = {&transfer.source, &transfer.destination};
        for (size_t index = 0; index < 2; ++index)
        {
            for (size_t denomination = 0; denomination < 4; ++denomination)
                result.wallets[index].wallet.amount[denomination] =
                    endpoints[index]->after[denomination];
            result.wallets[index].wallet_revision = 2;
            result.wallets[index].bank_revision = 2;
        }
        std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes = {};
        assert(coin_transfer_command_encode_result(transfer, result, &bytes));
        critical_completion receipt = {};
        receipt.operation_id = submitted.operation_id;
        receipt.outcome = critical_apply_outcome::applied;
        receipt.result_size = bytes.size();
        std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && callback_committed &&
               GET_SILVER(&actor) == 2 && GET_SILVER(&recipient) == 10 &&
               GET_COPPER(&actor) == 5 && GET_COPPER(&recipient) == 0);
        assert(currency_transaction_health_copy().pending == 0);
        return 0;
    }
    if (scenario == "accounted_coin_producer_restart")
    {
        critical_operation_id lineage = {}, epoch = {}, activation = {};
        lineage.bytes[0] = 1;
        epoch.bytes[0] = 2;
        activation.bytes[0] = 3;
        economic_gameplay_authority_test_access::install(lineage, epoch, activation);
        pc_only_data recipient_player = {};
        recipient_player.pid = 45;
        recipient_player.wallet_revision = 1;
        recipient_player.bank_revision = 1;
        char_data recipient = {};
        recipient.only.pc = &recipient_player;
        recipient.player.racewar = actor.player.racewar;
        online_other = &recipient;
        coin_transfer_payload transfer = {};
        assert(currency_transaction_coin_wallet(&actor, -1, &transfer.source));
        assert(currency_transaction_coin_wallet(&recipient, 1, &transfer.destination));
        assert(currency_transaction_submit_coin(&actor, transfer, coin_completed, nullptr, 0));
        assert(held_submissions == 1 && submissions == 0 && submitted.publication_required);
        assert(submitted.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
        assert(critical_command_envelope_valid(submitted));

        const critical_command replayed = submitted;
        currency_transaction_reset_for_tests();
        assert(currency_transaction_restore_replayed_command(replayed));
        assert(currency_transaction_health_copy().pending == 1);
        coin_transfer_result result = {};
        const coin_transfer_endpoint *endpoints[] = { &transfer.source, &transfer.destination };
        for (size_t index = 0; index < 2; ++index)
        {
            for (size_t denomination = 0; denomination < 4; ++denomination)
                result.wallets[index].wallet.amount[denomination] =
                    endpoints[index]->after[denomination];
            result.wallets[index].wallet_revision =
                endpoints[index]->change.expected_revisions[0].revision + 1;
            result.wallets[index].bank_revision =
                endpoints[index]->change.expected_revisions[1].revision + 1;
        }
        std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes = {};
        assert(coin_transfer_command_encode_result(transfer, result, &bytes));
        critical_completion receipt = {};
        receipt.operation_id = replayed.operation_id;
        receipt.outcome = critical_apply_outcome::already_applied;
        receipt.result_size = bytes.size();
        std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
        currency_transaction_handle_completions(&receipt, 1);
        assert(publication_acks == 1 && GET_COPPER(&actor) == 4 &&
               GET_COPPER(&recipient) == 1);
        assert(currency_transaction_health_copy().pending == 0 &&
               !currency_transaction_player_busy(&actor) &&
               !currency_transaction_player_busy(&recipient));

        constexpr uint64_t pile_uid = 99001;
        coin_transfer_payload pile_transfer = {};
        assert(currency_transaction_coin_wallet(&actor, -1, &pile_transfer.source));
        auto &pile_endpoint = pile_transfer.destination;
        pile_endpoint.after = {1, 0, 0, 0};
        item_transfer_payload pile = {};
        pile.from_owner = {item_owner_type::system, 0, 0};
        pile.to_owner = {item_owner_type::room, 77, 0};
        pile.reason = item_transfer_reason::creation;
        pile.selected_item_uid = pile_uid;
        pile.target_root_item_uid = pile_uid;
        pile.item_count = 1;
        pile.items[0] = {pile_uid, pile_uid, 0, ITEM_TRANSFER_ABSENT_REVISION,
                         402013, item_custody_state::absent};
        player_item_snapshot snapshot = {};
        snapshot.object_uid = pile_uid;
        snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
        snapshot.vnum = 402013;
        snapshot.type = ITEM_MONEY;
        snapshot.values[0] = 1;
        std::vector<uint8_t> item_blob;
        assert(player_item_snapshot_list_encode({snapshot}, &item_blob) ==
               player_snapshot_codec_result::ok);
        pile.item_blob_size = static_cast<uint32_t>(item_blob.size());
        std::copy(item_blob.begin(), item_blob.end(), pile.item_blob.begin());
        critical_operation_id pile_operation = {};
        assert(critical_operation_id_generate(&pile_operation));
        assert(item_transfer_command_build(&pile_endpoint.change, pile_operation, pile,
                                           critical_source_site::command,
                                           critical_deadline_class::interactive));
        assert(currency_transaction_submit_coin(&actor, pile_transfer, coin_completed,
                                                nullptr, 0));
        assert(held_submissions == 2 && submissions == 0 && submitted.publication_required);
        assert(submitted.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
               critical_command_envelope_valid(submitted));
        const critical_command pile_replayed = submitted;
        currency_transaction_reset_for_tests();
        assert(currency_transaction_restore_replayed_command(pile_replayed));
        coin_transfer_result pile_result = {};
        for (size_t denomination = 0; denomination < 4; ++denomination)
            pile_result.wallets[0].wallet.amount[denomination] =
                pile_transfer.source.after[denomination];
        pile_result.wallets[0].wallet_revision =
            pile_transfer.source.change.expected_revisions[0].revision + 1;
        pile_result.wallets[0].bank_revision =
            pile_transfer.source.change.expected_revisions[1].revision + 1;
        pile_result.piles[1] = {pile_uid, 1, 1, 1, 1, 0};
        assert(coin_transfer_command_encode_result(pile_transfer, pile_result, &bytes));
        receipt = {};
        receipt.operation_id = pile_replayed.operation_id;
        receipt.outcome = critical_apply_outcome::already_applied;
        receipt.result_size = bytes.size();
        std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
        currency_transaction_handle_completions(&receipt, 1);
        assert(publication_acks == 2 && GET_COPPER(&actor) == 3);
        assert(currency_transaction_health_copy().pending == 0 &&
               !currency_transaction_player_busy(&actor));
        return 0;
    }
    if (scenario == "active_prepared_wallet_payment" ||
        scenario == "active_prepared_bank_payment" ||
        scenario == "active_prepare_wallet_payment" ||
        scenario == "active_prepare_bank_payment" ||
        scenario == "legacy_prepared_wallet_payment" ||
        scenario == "legacy_prepared_bank_payment" ||
        scenario == "active_pending_prepared_payment")
    {
        // Exercise the real locker payment builder, including its prewritten
        // timestamp. A timestamp is not proof that the coordinator accepted it.
        const bool activate = !scenario.starts_with("legacy_");
        const bool prepare_after_activation = scenario.starts_with("active_prepare_");
        const bool retained = scenario == "active_pending_prepared_payment";
        const bool use_bank = scenario.find("bank") != std::string::npos;
        const int64_t cost = use_bank ? 10 : 1;
        GET_BALANCE_COPPER(&actor) = 20;
        critical_command command = {};
        if (!prepare_after_activation)
        {
            assert(currency_transaction_prepare_identify(&actor, cost, &command));
            assert(command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
                   command.accepted_at_usec && command.accounting_intent.empty());
            currency_command_payload payload = {};
            assert(currency_command_decode_payload(command, &payload));
            assert(payload.reason == (use_bank ? currency_reason_type::bank_payment :
                                                currency_reason_type::wallet_spend));
            if (retained)
                assert(currency_transaction_submit_prepared(&actor, command, completed,
                                                            nullptr, 0));
        }
        if (activate)
        {
            critical_operation_id lineage = {}, epoch = {}, activation = {};
            lineage.bytes[0] = 1;
            epoch.bytes[0] = 2;
            activation.bytes[0] = 3;
            economic_gameplay_authority_test_access::install(lineage, epoch, activation);
        }
        if (prepare_after_activation || (activate && !retained))
        {
            if (prepare_after_activation)
                assert(!currency_transaction_prepare_identify(&actor, cost, &command));
            else
                assert(!currency_transaction_submit_prepared(&actor, command, completed,
                                                             nullptr, 0));
            assert(submissions == 0 && held_submissions == 0 && callbacks == 0 &&
                   publication_acks == 0 && bank_publications == 0);
            assert(currency_transaction_health_copy().pending == 0 &&
                   !currency_transaction_player_busy(&actor));
            assert(GET_COPPER(&actor) == 5 && GET_BALANCE_COPPER(&actor) == 20);
            assert(player.wallet_revision == 1 && player.bank_revision == 1);
            return 0;
        }
        // Legacy mode still works. A genuinely pending command may attach after
        // cache replacement, but must not be re-enqueued or retagged.
        assert(currency_transaction_submit_prepared(&actor, command, completed, nullptr, 0));
        assert(submissions == 1 && held_submissions == 0 && callbacks == 0);
        assert(submitted.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
               submitted.accounting_intent.empty());
        assert(GET_COPPER(&actor) == 5 && GET_BALANCE_COPPER(&actor) == 20);
        critical_completion receipt = {};
        receipt.operation_id = command.operation_id;
        receipt.outcome = retained ? critical_apply_outcome::already_applied :
                                     critical_apply_outcome::applied;
        encode(receipt, use_bank ? 5 : 5 - cost, use_bank ? 20 - cost : 20);
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && callback_committed && publication_acks == 0 &&
               bank_publications == 1 && submissions == 1);
        assert(GET_COPPER(&actor) == (use_bank ? 5 : 5 - cost) &&
               !currency_transaction_player_busy(&actor));
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && bank_publications == 1 && submissions == 1);
        return 0;
    }
    if (scenario == "accounted_bank_producer" || scenario == "accounted_bank_producer_restart")
    {
        critical_operation_id lineage = {}, epoch = {}, activation = {};
        lineage.bytes[0] = 1;
        epoch.bytes[0] = 2;
        activation.bytes[0] = 3;
        economic_gameplay_authority_test_access::install(lineage, epoch, activation);
        const currency_vector wallet = { {-1, 0, 0, 0} };
        const currency_vector bank = { {1, 0, 0, 0} };
        assert(currency_transaction_submit(&actor, wallet, bank,
            currency_reason_type::atm_deposit, 0, critical_source_site::command,
            critical_deadline_class::interactive, completed, nullptr, 0));
        assert(held_submissions == 1 && submissions == 0 && submitted.publication_required);
        assert(submitted.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
        assert(critical_command_envelope_valid(submitted));
        assert(GET_COPPER(&actor) == 5 && bank_publications == 0 && callbacks == 0);
        assert(!submit_reward(&actor, nullptr)); // active unsupported writer cannot bypass.
        const auto operation = submitted.operation_id;
        if (scenario == "accounted_bank_producer_restart")
        {
            submitted.accepted_at_usec = 1;
            currency_transaction_reset_for_tests();
            assert(currency_transaction_restore_replayed_command(submitted));
            online = nullptr;
        }
        critical_completion receipt = {};
        receipt.operation_id = operation;
        receipt.outcome = critical_apply_outcome::already_applied;
        encode(receipt, 4, 1);
        ack_available = false;
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 0 && currency_transaction_player_busy(&actor));
        if (scenario == "accounted_bank_producer_restart")
        {
            assert(publication_acks == 0 && GET_COPPER(&actor) == 5);
            online = &actor;
            currency_transaction_player_ready(&actor);
        }
        assert(publication_acks == 1 && GET_COPPER(&actor) == 4);
        ack_available = true;
        currency_transaction_player_ready(&actor);
        assert(publication_acks == 2 && !currency_transaction_player_busy(&actor));
        assert(callbacks == (scenario == "accounted_bank_producer_restart" ? 0 : 1));
        assert(held_submissions == 1 && submissions == 0);
        return 0;
    }
    if (scenario == "accounted_chaos_starter_producer")
    {
        critical_operation_id lineage = {}, epoch = {}, activation = {}, seed = {};
        lineage.bytes[0] = 1;
        epoch.bytes[0] = 2;
        activation.bytes[0] = 3;
        economic_gameplay_authority_test_access::install(lineage, epoch, activation);
        std::memcpy(seed.bytes.data(), "CHAOSEED", 8);
        seed.bytes[8] = 42;
        critical_operation_id operation = {};
        assert(critical_operation_id_derive(seed, 0x43484250, 1, &operation));
        const currency_vector wallet = {};
        const currency_vector bank = { {0, 0, 0, 1000000} };
        auto invalid = operation;
        invalid.bytes[0] ^= 1;
        assert(!currency_transaction_submit_identified(
            &actor, invalid, wallet, bank, currency_reason_type::chaos_starter_reward,
            42, critical_source_site::login, critical_deadline_class::recovery,
            completed, nullptr, 0));
        assert(held_submissions == 0 && !currency_transaction_player_busy(&actor));
        assert(currency_transaction_submit_identified(
            &actor, operation, wallet, bank, currency_reason_type::chaos_starter_reward,
            42, critical_source_site::login, critical_deadline_class::recovery,
            completed, nullptr, 0));
        assert(held_submissions == 1 && submissions == 0 && submitted.publication_required);
        assert(submitted.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
        assert(submitted.expected_revisions[1].revision == UINT64_MAX);
        assert(critical_command_envelope_valid(submitted));
        economic_frozen_intent intent = {};
        assert(economic_intent_decode(submitted.accounting_intent, &intent) ==
               economic_accounting_error::ok);
        assert(intent.admission.metadata.writer_id == ECONOMIC_WRITER_CHAOS_STARTER_BANK);
        assert(intent.admission.metadata.reason == economic_reason::starter_reward);
        assert(intent.admission.metadata.source_event &&
               intent.admission.metadata.source_event->kind ==
                   economic_source_kind::starter_grant);
        critical_completion receipt = {};
        receipt.operation_id = operation;
        receipt.outcome = critical_apply_outcome::applied;
        currency_command_result result = {};
        result.wallet.amount[0] = 5;
        result.bank.amount[3] = 1000000;
        result.wallet_revision = 2;
        result.bank_revision = 2;
        std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> bytes = {};
        assert(currency_command_encode_result(result, &bytes));
        receipt.result_size = bytes.size();
        std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1 && callback_committed && publication_acks == 1);
        assert(GET_COPPER(&actor) == 5 && bank_publications == 1 &&
               last_bank_balances.platinum == 1000000);
        assert(!currency_transaction_player_busy(&actor));
        return 0;
    }
    if (scenario == "accounted_bank_publication" ||
        scenario == "accounted_bank_ack_retry" ||
        scenario == "accounted_bank_invalid_result" ||
        scenario == "accounted_bank_restart")
    {
        critical_operation_id operation = {}, lineage = {}, epoch = {};
        operation.bytes[0] = 3;
        lineage.bytes[0] = 1;
        epoch.bytes[0] = 2;
        currency_command_payload payload = {};
        payload.pid = 42;
        payload.racewar = 1;
        payload.reason = currency_reason_type::atm_deposit;
        std::strcpy(payload.account_name.data(), "retention_account");
        payload.wallet_delta.amount[0] = -1;
        payload.bank_delta.amount[0] = 1;
        critical_command command = {};
        assert(currency_command_build(&command, operation, payload, 1, 1,
                                      critical_source_site::command,
                                      critical_deadline_class::interactive));
        const economic_account_key wallet = { lineage, economic_account_kind::wallet, 42, 0 };
        const economic_account_key bank = { lineage, economic_account_kind::bank, 52, 1 };
        assert(economic_bank_transfer_intent(command, epoch, wallet, bank,
                                             &command.accounting_intent) ==
               economic_accounting_error::ok);
        command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
        command.accepted_at_usec = 1;
        assert(critical_command_envelope_valid(command));
        if (scenario == "accounted_bank_restart")
        {
            command.publication_required = true;
            online = nullptr;
            assert(currency_transaction_restore_replayed_command(command));
            assert(!currency_transaction_restore_replayed_command(command));
            assert(currency_transaction_player_busy(&actor));
            critical_completion receipt = {};
            receipt.operation_id = operation;
            receipt.outcome = critical_apply_outcome::already_applied;
            encode(receipt, 4, 1);
            currency_transaction_handle_completions(&receipt, 1);
            assert(publication_acks == 0 && callbacks == 0);
            online = &actor;
            currency_transaction_player_ready(&actor);
            assert(publication_acks == 1 && callbacks == 0 &&
                   bank_publications == 1 && GET_COPPER(&actor) == 4 &&
                   !currency_transaction_player_busy(&actor));
            assert(held_submissions == 0 && submissions == 0);
            return 0;
        }
        assert(currency_transaction_submit_prepared(&actor, command, completed, nullptr, 0));
        assert(held_submissions == 1 && submissions == 0 &&
               submitted.publication_required);
        critical_completion receipt = {};
        receipt.operation_id = operation;
        receipt.outcome = critical_apply_outcome::applied;
        if (scenario == "accounted_bank_invalid_result")
            encode(receipt, static_cast<int64_t>(INT_MAX) + 1, 1);
        else
            encode(receipt, 4, 1);
        if (scenario == "accounted_bank_ack_retry") ack_available = false;
        currency_transaction_handle_completions(&receipt, 1);
        if (scenario == "accounted_bank_invalid_result")
        {
            assert(publication_acks == 0 && callbacks == 0 &&
                   currency_transaction_player_busy(&actor));
            return 0;
        }
        if (scenario == "accounted_bank_ack_retry")
        {
            assert(publication_acks == 1 && callbacks == 0 &&
                   currency_transaction_player_busy(&actor));
            ack_available = true;
            currency_transaction_player_ready(&actor);
        }
        assert(callbacks == 1 && callback_committed &&
               publication_acks == (scenario == "accounted_bank_ack_retry" ? 2 : 1));
        assert(GET_COPPER(&actor) == 4 && bank_publications >= 1);
        assert(!currency_transaction_player_busy(&actor));
        currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 1);
        return 0;
    }
    assert(submit_reward(&actor, completed));
    const critical_command original = submitted;
    critical_completion receipt = {};
    receipt.operation_id = original.operation_id;
    receipt.outcome = critical_apply_outcome::applied;
    receipt.attempt = CRITICAL_COORDINATOR_MAX_RETRIES + 1;

    if (scenario == "coin_ambiguous" || scenario == "coin_exhausted_retry" ||
        scenario == "coin_malformed_commit")
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
                          scenario == "coin_exhausted_retry" ?
                              critical_apply_outcome::retryable_failure :
                              critical_apply_outcome::applied;
        currency_transaction_handle_completions(&receipt, 1);
        for (int i = 0; i < 5; ++i) currency_transaction_handle_completions(&receipt, 1);
        assert(callbacks == 0 && GET_COPPER(&actor) == 5 && GET_COPPER(&recipient) == 0);
        assert(currency_transaction_player_busy(&actor) && currency_transaction_player_busy(&recipient));
        const auto blocked = currency_transaction_health_copy();
        assert(blocked.pending == 1 && blocked.publication_blocked == 1 &&
               blocked.publication_retrying == 0 && alerts == 1);
        assert(last_alert_operation.size() == 32 && last_alert_operation != "none");
        assert(blocked.malformed_completions ==
               static_cast<uint64_t>(scenario == "coin_malformed_commit"));
        assert(!currency_transaction_can_submit_nonrebasable(&actor));
        if (scenario == "coin_malformed_commit") return 0;
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
        const auto rejected = currency_transaction_health_copy();
        assert(callbacks == 1 && rejected.rejected == 1 &&
               rejected.malformed_completions == 0 && rejected.publication_blocked == 0);
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

    if (scenario == "active_rebasable")
    {
        // Rewards do not read the stale live balance and may queue behind an
        // ordinary in-flight operation while its outcome is still unknown.
        for (int i = 0; i < 10; ++i) assert(submit_reward(&actor, nullptr));
        const auto active = currency_transaction_health_copy();
        assert(submissions == 11 && active.pending == 11 && active.publication_blocked == 0);
        return 0;
    }

    if (scenario == "already_applied")
        receipt.outcome = critical_apply_outcome::already_applied;
    else if (scenario == "ambiguous" || scenario == "ambiguous_with_payload" ||
             scenario == "blocked_rebasable")
        receipt.outcome = critical_apply_outcome::ambiguous_commit;
    else if (scenario == "exhausted_retry")
        receipt.outcome = critical_apply_outcome::retryable_failure;
    else if (scenario == "wallet_range")
        encode(receipt, static_cast<int64_t>(INT_MAX) + 1);
    else if (scenario == "bank_range")
        encode(receipt, 15, -1);
    else
        assert(scenario == "malformed" || scenario == "offline" ||
               scenario == "blocked_rebasable");
    if (scenario == "ambiguous_with_payload") encode(receipt);
    if (scenario == "offline")
    {
        encode(receipt);
        online = nullptr;
    }
    currency_transaction_handle_completions(&receipt, 1);

    if (scenario == "offline")
    {
        const auto retained = currency_transaction_health_copy();
        assert(callbacks == 0 && retained.pending == 1 && retained.retained_offline == 1);
        assert(retained.publication_blocked == 0 && retained.publication_retrying == 0);
        online = &actor;
        currency_transaction_player_ready(&actor);
        const auto published = currency_transaction_health_copy();
        assert(callbacks == 1 && callback_committed && GET_COPPER(&actor) == 15);
        assert(published.pending == 0 && published.retained_offline == 0);
        return 0;
    }

    assert(callbacks == 0 && "unresolved receipt must not become a rejected transaction");
    assert(GET_COPPER(&actor) == 5 && player.wallet_revision == 1);
    assert(bank_publications == 0);
    const auto blocked = currency_transaction_health_copy();
    assert(blocked.pending == 1 && blocked.rejected == 0);
    assert(blocked.retained_offline == 0 && blocked.publication_blocked == 1);
    assert(blocked.publication_retrying == 0);
    const bool malformed = scenario == "malformed" || scenario == "already_applied" ||
                           scenario == "wallet_range" || scenario == "bank_range";
    assert(blocked.malformed_completions == static_cast<uint64_t>(malformed));
    assert(currency_transaction_player_busy(&actor));
    assert(!currency_transaction_can_submit_nonrebasable(&actor));
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
    assert(!currency_transaction_can_submit_nonrebasable(&sibling));
    assert(!currency_transaction_submit_bank_payment(
        &sibling, 1, currency_reason_type::wallet_spend, 0,
        critical_source_site::command, critical_deadline_class::interactive,
        nullptr, nullptr, 0));
    sibling.player.racewar = 2;
    assert(!currency_transaction_player_busy(&sibling));
    assert(currency_transaction_can_submit_nonrebasable(&sibling));
    for (int i = 0; i < 5; ++i)
        currency_transaction_handle_completions(&receipt, 1);
    assert(callbacks == 0 && submissions == 1 && alerts == 1);
    assert(last_alert_operation.size() == 32 && last_alert_operation != "none");

    if (scenario == "blocked_rebasable")
    {
        // Once publication is unresolved, even rebasable rewards for the affected
        // player/account stop. An unrelated account can still make progress.
        assert(!submit_reward(&actor, nullptr));
        sibling_player.pid = 45;
        sibling.player.racewar = actor.player.racewar;
        GET_COPPER(&sibling) = 5;
        assert(currency_transaction_submit_wallet_value(
            &sibling, -1, currency_reason_type::wallet_spend, 0,
            critical_source_site::command, critical_deadline_class::interactive,
            nullptr, nullptr, 0));
        assert(submissions == 2);
        assert(currency_transaction_health_copy().pending == 2);
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
        "rejected_without_payload", "callback_chain", "callback_rehash", "active_rebasable",
        "blocked_rebasable", "coin_ambiguous", "coin_exhausted_retry",
        "coin_malformed_commit",
        "exact_morph_coin",
        "accounted_coin_producer_restart",
        "accounted_bank_publication", "accounted_bank_ack_retry",
        "accounted_bank_invalid_result", "accounted_bank_restart",
        "accounted_bank_producer", "accounted_bank_producer_restart",
        "accounted_chaos_starter_producer",
        "active_prepared_wallet_payment", "active_prepared_bank_payment",
        "active_prepare_wallet_payment", "active_prepare_bank_payment",
        "legacy_prepared_wallet_payment", "legacy_prepared_bank_payment",
        "active_pending_prepared_payment",
    )
    with tempfile.TemporaryDirectory(prefix="currency-retention-") as directory:
        source = Path(directory) / "retention.cpp"
        source.write_text(HARNESS, encoding="utf-8")
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        for flatfile in (False, True):
            backend = "flatfile" if flatfile else "mysql"
            binary = Path(directory) / backend
            subprocess.run([
                *compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
                "-pthread", "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
                *(["-D__NO_MYSQL__", "-Isrc/no_mysql"] if flatfile else []),
                "-Isrc", *cflags, str(source), rel("currency_transaction.c"),
                rel("currency_command.c"), rel("critical_command.c"),
                rel("economic_currency_adapter.c"), rel("economic_accounting_intent.c"),
                rel("economic_gameplay_authority.c"), rel("economic_command_admission.c"),
                rel("economic_accounting_plan.c"), rel("economic_accounting_types.c"),
                rel("coin_transfer_command.c"), rel("item_transfer_command.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"),
                rel("coin_transfer_accounting.c"),
                rel("item_transfer_accounting.c"),
                rel("player_snapshot_codec.c"), "-Wl,--gc-sections", "-lcrypto",
                *shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True)),
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
