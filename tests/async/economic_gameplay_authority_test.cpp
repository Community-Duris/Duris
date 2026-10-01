#include "economy/economic_gameplay_authority.h"
#include "economy/economic_command_admission.h"
#include "economy/economic_accounting_intent.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"
#include "core/structs.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>

class economic_gameplay_authority_test_access
{
    public:
	static auto install(const critical_operation_id &lineage,
			    const critical_operation_id &epoch,
			    const critical_operation_id &receipt,
			    std::span<const economic_gameplay_wallet_mapping> wallets,
			    std::span<const economic_gameplay_bank_mapping> banks)
	{
		return economic_gameplay_authority::install(lineage, epoch, receipt, wallets,
							    banks);
	}
	static auto install_sql_wallet_root_qualification(
		const critical_operation_id &lineage, const critical_operation_id &epoch,
		const critical_operation_id &receipt,
		std::span<const economic_gameplay_wallet_mapping> wallets,
		std::span<const economic_gameplay_bank_mapping> banks)
	{
		return economic_gameplay_authority::install_sql_wallet_root_qualification(
			{}, lineage, epoch, receipt, wallets, banks);
	}
	static void reset() { economic_gameplay_authority::reset_for_tests(); }
	static void clear_sql_runtime() { economic_gameplay_authority::clear_sql_runtime(); }
};

using error = economic_accounting_error;
using lifecycle_access = economic_gameplay_authority_test_access;
critical_operation_id id(uint8_t value)
{
	critical_operation_id result = {};
	result.bytes[0] = value;
	return result;
}
critical_command transfer(currency_reason_type reason = currency_reason_type::atm_deposit,
			  uint32_t pid = 7, const char *name = "Fixture")
{
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = reason;
	std::strcpy(payload.account_name.data(), name);
	payload.wallet_delta.amount[0] = -5;
	payload.bank_delta.amount[0] = 5;
	if (reason == currency_reason_type::atm_withdraw)
	{
		payload.wallet_delta.amount[0] = 5;
		payload.bank_delta.amount[0] = -5;
	}
	critical_command command;
	assert(currency_command_build(&command, id(9), payload, 2, 4, critical_source_site::command,
				      critical_deadline_class::interactive));
	return command;
}
critical_command chaos_starter_bank(uint32_t pid = 7, const char *name = "Fixture")
{
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = currency_reason_type::chaos_starter_reward;
	payload.reason_id = pid;
	std::strcpy(payload.account_name.data(), name);
	payload.bank_delta.amount[3] = 1000000;
	critical_operation_id seed = {};
	std::memcpy(seed.bytes.data(), "CHAOSEED", 8);
	for (size_t byte = 0; byte < 8; ++byte)
		seed.bytes[8 + byte] = static_cast<uint8_t>(uint64_t(pid) >> (8 * byte));
	critical_operation_id operation = {};
	assert(critical_operation_id_derive(seed, 0x43484250, 1, &operation));
	critical_command command;
	assert(currency_command_build(&command, operation, payload, 2, UINT64_MAX,
				      critical_source_site::login,
				      critical_deadline_class::recovery));
	return command;
}
critical_command item_transfer()
{
	const item_owner_identity player = { item_owner_type::player, 7, 0 };
	const item_owner_identity room = { item_owner_type::room, 77, 0 };
	item_transfer_payload payload = {};
	payload.from_owner = player;
	payload.to_owner = room;
	payload.reason = item_transfer_reason::player_drop;
	payload.expected_from_revision = 3;
	payload.expected_to_revision = 4;
	payload.selected_item_uid = 500;
	payload.target_root_item_uid = 500;
	payload.item_count = 1;
	payload.items[0] = { 500, 500, 0, 8, 9001, item_custody_state::active };
	critical_command command;
	assert(item_transfer_command_build(&command, id(10), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	return command;
}
bool candidate_equal(critical_command left, critical_command right)
{
	if (left.accepted_at_usec != right.accepted_at_usec)
		return false;
	// Comparison projection only: the candidates must keep their unset time.
	left.accepted_at_usec = right.accepted_at_usec = 1;
	return critical_command_equal(left, right);
}
bool supported_candidate(critical_command command)
{
	command.accepted_at_usec = 1;
	return economic_command_admission_supported(command);
}
critical_command wallet_reason_command(currency_reason_type reason, uint32_t pid)
{
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = reason;
	std::strcpy(payload.account_name.data(), "Fixture");
	payload.wallet_delta.amount[0] = reason == currency_reason_type::wallet_reward ? 5 : -5;
	critical_command command;
	assert(currency_command_build(&command, id(41), payload, 2, 4,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	return command;
}
critical_command quest_wallet_reward_command()
{
	auto command = wallet_reason_command(currency_reason_type::wallet_reward, 7);
	currency_command_payload payload = {};
	assert(currency_command_decode_payload(command, &payload));
	payload.reason_id = 1;
	assert(currency_command_encode_payload(payload, &command.payload));
	command.source_site = critical_source_site::recovery;
	command.deadline_class = critical_deadline_class::recovery;
	command.expected_revisions[0].revision = UINT64_MAX;
	command.expected_revisions[1].revision = UINT64_MAX;
	return command;
}
coin_transfer_endpoint wallet_endpoint(uint32_t pid, const char *account_name, uint8_t operation,
				       bool source)
{
	coin_transfer_endpoint endpoint = {};
	endpoint.before[0] = 100;
	endpoint.after[0] = source ? 95 : 105;
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = currency_reason_type::coin_transfer;
	std::strcpy(payload.account_name.data(), account_name);
	payload.wallet_delta.amount[0] = source ? -5 : 5;
	assert(currency_command_build(&endpoint.change, id(operation), payload, source ? 8 : 9, 7,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	return endpoint;
}
critical_command wallet_root(uint8_t root_id = 30, uint32_t source_pid = 7,
			     uint32_t destination_pid = 8, const char *source_bank = "fixture",
			     const char *destination_bank = "other_bank")
{
	coin_transfer_payload payload = {};
	payload.source = wallet_endpoint(source_pid, source_bank, root_id + 1, true);
	payload.destination =
		wallet_endpoint(destination_pid, destination_bank, root_id + 2, false);
	critical_command command;
	assert(coin_transfer_command_build(&command, id(root_id), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	return command;
}
coin_transfer_endpoint coin_pile_endpoint(uint64_t uid, uint32_t owner_pid)
{
	coin_transfer_endpoint endpoint = {};
	endpoint.before[0] = 0;
	endpoint.after[0] = 5;
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, owner_pid, 0 };
	payload.expected_from_revision = 7;
	payload.expected_to_revision = 7;
	payload.reason = item_transfer_reason::creation;
	payload.selected_item_uid = uid;
	payload.target_root_item_uid = uid;
	payload.item_count = 1;
	payload.items[0] = { uid,	 uid,
			     0,		 ITEM_TRANSFER_ABSENT_REVISION,
			     VOBJ_COINS, item_custody_state::absent };
	player_item_snapshot snapshot = {};
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.equipment_slot = -1;
	snapshot.object_uid = uid;
	snapshot.vnum = VOBJ_COINS;
	snapshot.type = ITEM_MONEY;
	snapshot.values[0] = 5;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ snapshot }, &blob) ==
	       player_snapshot_codec_result::ok);
	payload.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	assert(item_transfer_command_build(&endpoint.change, id(static_cast<uint8_t>(uid)), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	return endpoint;
}
critical_command wallet_to_pile_root()
{
	coin_transfer_payload payload = {};
	payload.source = wallet_endpoint(7, "fixture", 51, true);
	payload.destination = coin_pile_endpoint(900, 8);
	critical_command command;
	assert(coin_transfer_command_build(&command, id(50), payload, critical_source_site::command,
					   critical_deadline_class::interactive));
	return command;
}
void replace_source_change(critical_command *command, currency_reason_type reason,
			   bool add_bank_delta)
{
	coin_transfer_payload payload = {};
	assert(coin_transfer_command_decode_payload(*command, &payload));
	currency_command_payload changed = {};
	assert(currency_command_decode_payload(payload.source.change, &changed));
	changed.reason = reason;
	if (add_bank_delta)
		changed.bank_delta.amount[0] = 1;
	critical_command replacement;
	assert(currency_command_build(&replacement, payload.source.change.operation_id, changed,
				      payload.source.change.expected_revisions[0].revision,
				      payload.source.change.expected_revisions[1].revision,
				      payload.source.change.source_site,
				      payload.source.change.deadline_class));
	replacement.accepted_at_usec = payload.source.change.accepted_at_usec;
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(replacement, &encoded) == critical_command_codec_result::ok);
	const size_t size = static_cast<size_t>(command->payload[32]) |
			    (static_cast<size_t>(command->payload[33]) << 8) |
			    (static_cast<size_t>(command->payload[34]) << 16) |
			    (static_cast<size_t>(command->payload[35]) << 24);
	assert(encoded.size() == size);
	std::copy(encoded.begin(), encoded.end(), command->payload.begin() + 36);
}
critical_command frozen_wallet_reason(currency_reason_type reason)
{
	auto command = wallet_reason_command(reason, 7);
	economic_admission_facts facts = {};
	facts.metadata.lineage = id(1);
	facts.metadata.epoch = id(2);
	facts.metadata.actor_kind = economic_actor_kind::domain;
	facts.metadata.actor_id = 101;
	facts.metadata.writer_id = 77;
	facts.metadata.reason = economic_reason::coin_transfer;
	facts.facts = { 1, 2 };
	std::vector<uint8_t> intent;
	assert(economic_intent_freeze(command, facts, &intent) == error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.accepted_at_usec = 103;
	return command;
}
void qualified_projection_regressions()
{
	lifecycle_access::reset();
	const std::array wallets = { economic_gameplay_wallet_mapping{
					     7, { id(1), economic_account_kind::wallet, 101, 0 } },
				     economic_gameplay_wallet_mapping{
					     8,
					     { id(1), economic_account_kind::wallet, 102, 0 } } };
	const std::array banks = { economic_gameplay_bank_mapping{
		"fixture", 1, { id(1), economic_account_kind::bank, 202, 1 } } };
	const std::array<economic_gameplay_bank_mapping, 0> no_banks = {};
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, banks) == error::ok);
	auto quest_reward = quest_wallet_reward_command();
	assert(economic_gameplay_authority::prepare_currency(&quest_reward) == error::ok);
	assert(supported_candidate(quest_reward));
	economic_frozen_intent quest_reward_intent;
	assert(economic_intent_decode(quest_reward.accounting_intent, &quest_reward_intent) ==
	       error::ok);
	assert(quest_reward_intent.admission.metadata.writer_id ==
		       ECONOMIC_WRITER_QUEST_WALLET_REWARD &&
	       quest_reward_intent.admission.metadata.reason == economic_reason::quest_reward);

	// The ordinary projection still freezes wallet roots and shared-bank roots.
	auto regular_root = wallet_root(60);
	assert(economic_gameplay_authority::prepare_coin_transfer(&regular_root) == error::ok);
	assert(supported_candidate(regular_root));
	auto regular_shared_bank = wallet_root(62, 7, 8, "shared", "shared");
	assert(economic_gameplay_authority::prepare_coin_transfer(&regular_shared_bank) ==
	       error::ok);
	coin_transfer_payload shared_payload = {};
	assert(coin_transfer_command_decode_payload(regular_shared_bank, &shared_payload));
	assert(shared_payload.source.change.expected_revisions[1].revision ==
	       shared_payload.destination.change.expected_revisions[1].revision);

	// Normal active projection does not admit an unrelated fresh wallet spend.
	auto normal_wallet_spend = wallet_reason_command(currency_reason_type::wallet_spend, 7);
	const auto normal_wallet_spend_before = normal_wallet_spend;
	assert(economic_gameplay_authority::prepare_currency(&normal_wallet_spend) ==
	       error::incomplete_coverage);
	assert(candidate_equal(normal_wallet_spend, normal_wallet_spend_before));

	// Keep genuine normal-projection schema-2 ATM commands for both scope-bypass
	// controls and historical replay compatibility checks.
	auto frozen_deposit = transfer(currency_reason_type::atm_deposit);
	auto frozen_withdraw = transfer(currency_reason_type::atm_withdraw);
	auto frozen_starter = chaos_starter_bank();
	assert(economic_gameplay_authority::prepare_currency(&frozen_deposit) == error::ok);
	assert(economic_gameplay_authority::prepare_currency(&frozen_withdraw) == error::ok);
	assert(economic_gameplay_authority::prepare_currency(&frozen_starter) == error::ok);
	frozen_deposit.accepted_at_usec = 101;
	frozen_withdraw.accepted_at_usec = 102;
	frozen_starter.accepted_at_usec = 105;
	assert(supported_candidate(frozen_deposit) && supported_candidate(frozen_withdraw) &&
	       supported_candidate(frozen_starter));
	const auto frozen_deposit_bytes = frozen_deposit;
	const auto frozen_withdraw_bytes = frozen_withdraw;
	const auto frozen_starter_bytes = frozen_starter;
	const auto frozen_reward = frozen_wallet_reason(currency_reason_type::wallet_reward);
	auto frozen_quest_reward = quest_reward;
	frozen_quest_reward.accepted_at_usec = 106;
	const auto frozen_spend = frozen_wallet_reason(currency_reason_type::wallet_spend);
	// Upstream also admits standalone item roots in regular mode. Keep a real
	// frozen one to prove that qualification closes the historical fast path.
	auto frozen_item = item_transfer();
	assert(economic_gameplay_authority::prepare_item_transfer(&frozen_item, 7) == error::ok);
	frozen_item.accepted_at_usec = 104;
	assert(supported_candidate(frozen_item));
	const auto frozen_item_bytes = frozen_item;

	assert(lifecycle_access::install_sql_wallet_root_qualification(id(1), id(2), id(3), wallets,
								       no_banks) == error::ok);
	auto qualified_root = wallet_root(70);
	assert(economic_gameplay_authority::prepare_coin_transfer(&qualified_root) == error::ok);
	assert(supported_candidate(qualified_root));
	economic_frozen_intent intent;
	assert(economic_intent_decode(qualified_root.accounting_intent, &intent) == error::ok);
	assert(intent.admission.metadata.lineage.bytes == id(1).bytes);
	assert(intent.admission.metadata.epoch.bytes == id(2).bytes);
	assert(intent.admission.metadata.actor_id == 101);
	assert(intent.admission.metadata.writer_id == ECONOMIC_WRITER_WALLET_COIN_TRANSFER);
	qualified_root.accepted_at_usec = 71;
	const auto replay_bytes = qualified_root;
	assert(economic_gameplay_authority::prepare_coin_transfer(&qualified_root) == error::ok);
	assert(critical_command_equal(qualified_root, replay_bytes));

	// Shared-bank wallet roots remain admitted; the existing executor owns its
	// second-endpoint revision rebase after this immutable admission step.
	auto qualified_shared_bank = wallet_root(72, 7, 8, "shared", "shared");
	assert(economic_gameplay_authority::prepare_coin_transfer(&qualified_shared_bank) ==
	       error::ok);

	// Wallet-root qualification must reject BOTH new and frozen standalone
	// item roots without changing the caller's command.
	auto new_item = item_transfer();
	const auto new_item_before = new_item;
	assert(economic_gameplay_authority::prepare_item_transfer(&new_item, 7) ==
	       error::unauthorized);
	assert(candidate_equal(new_item, new_item_before));
	auto retained_item = frozen_item_bytes;
	assert(economic_gameplay_authority::prepare_item_transfer(&retained_item, 7) ==
	       error::unauthorized);
	assert(critical_command_equal(retained_item, frozen_item_bytes));

	for (auto command : { transfer(currency_reason_type::atm_deposit),
			      transfer(currency_reason_type::atm_withdraw), chaos_starter_bank(),
			      wallet_reason_command(currency_reason_type::wallet_reward, 7),
			      wallet_reason_command(currency_reason_type::wallet_spend, 7),
			      frozen_deposit_bytes, frozen_withdraw_bytes, frozen_starter_bytes,
			      frozen_reward, frozen_quest_reward, frozen_spend })
	{
		const auto before = command;
		assert(economic_gameplay_authority::prepare_currency(&command) ==
		       error::unauthorized);
		const bool unchanged = candidate_equal(command, before);
		assert(unchanged);
	}
	for (auto command :
	     { frozen_deposit_bytes, frozen_withdraw_bytes, frozen_reward, frozen_spend })
	{
		const auto before = command;
		assert(economic_gameplay_authority::prepare_coin_transfer(&command) ==
		       error::unauthorized);
		assert(candidate_equal(command, before));
	}
	for (auto command : { transfer(currency_reason_type::atm_deposit), wallet_to_pile_root() })
	{
		const auto before = command;
		assert(economic_gameplay_authority::prepare_coin_transfer(&command) != error::ok);
		assert(candidate_equal(command, before));
	}

	// Malformed wallet endpoint reason and bank denomination delta are rejected
	// after a real root build, without modifying the command.
	auto wrong_reason = wallet_root(74);
	replace_source_change(&wrong_reason, currency_reason_type::atm_deposit, false);
	const auto wrong_reason_bytes = wrong_reason;
	assert(economic_gameplay_authority::prepare_coin_transfer(&wrong_reason) != error::ok);
	assert(candidate_equal(wrong_reason, wrong_reason_bytes));
	auto bank_delta = wallet_root(76);
	replace_source_change(&bank_delta, currency_reason_type::coin_transfer, true);
	const auto bank_delta_bytes = bank_delta;
	assert(economic_gameplay_authority::prepare_coin_transfer(&bank_delta) != error::ok);
	assert(candidate_equal(bank_delta, bank_delta_bytes));

	auto unknown_wallet = wallet_root(78, 99, 8);
	const auto unknown_wallet_bytes = unknown_wallet;
	assert(economic_gameplay_authority::prepare_coin_transfer(&unknown_wallet) ==
	       error::incomplete_coverage);
	assert(candidate_equal(unknown_wallet, unknown_wallet_bytes));

	// Old frozen lineage/lifetime selections cannot replay under a different
	// qualified mapping or epoch; the same-ID exact replay succeeds again when
	// its selected projection is restored.
	auto wrong_lifetime = wallets;
	wrong_lifetime[0].account.authority_id = 111;
	assert(lifecycle_access::install_sql_wallet_root_qualification(
		       id(1), id(2), id(4), wrong_lifetime, no_banks) == error::ok);
	auto stale_lifetime = replay_bytes;
	assert(economic_gameplay_authority::prepare_coin_transfer(&stale_lifetime) ==
	       error::payload_conflict);
	assert(critical_command_equal(stale_lifetime, replay_bytes));
	assert(lifecycle_access::install_sql_wallet_root_qualification(id(1), id(4), id(5), wallets,
								       no_banks) == error::ok);
	auto stale_epoch = replay_bytes;
	assert(economic_gameplay_authority::prepare_coin_transfer(&stale_epoch) ==
	       error::payload_conflict);
	assert(critical_command_equal(stale_epoch, replay_bytes));
	assert(lifecycle_access::install_sql_wallet_root_qualification(id(1), id(2), id(3), wallets,
								       no_banks) == error::ok);

	std::array duplicate_lifetimes = { wallets[0], wallets[1] };
	duplicate_lifetimes[1].account.authority_id = 101;
	std::array duplicate_pid = { wallets[0], wallets[1] };
	duplicate_pid[1].pid = duplicate_pid[0].pid;
	duplicate_pid[1].account.authority_id = 103;
	assert(lifecycle_access::install_sql_wallet_root_qualification(
		       id(1), id(2), id(6), duplicate_pid, no_banks) == error::invalid_identity);
	assert(lifecycle_access::install_sql_wallet_root_qualification(
		       id(1), id(2), id(6), duplicate_lifetimes, no_banks) ==
	       error::invalid_identity);
	assert(lifecycle_access::install_sql_wallet_root_qualification(
		       id(1), {}, id(3), wallets, no_banks) == error::invalid_identity);
	auto still_qualified = replay_bytes;
	assert(economic_gameplay_authority::prepare_coin_transfer(&still_qualified) == error::ok);
	assert(critical_command_equal(still_qualified, replay_bytes));
	auto still_closed = transfer(currency_reason_type::atm_deposit);
	assert(economic_gameplay_authority::prepare_currency(&still_closed) == error::unauthorized);
	assert(still_closed.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION);
}
int main()
{
	lifecycle_access::reset();
	assert(!economic_gameplay_authority::active());
	auto legacy = transfer();
	const auto original = legacy;
	assert(economic_gameplay_authority::prepare_currency(&legacy) == error::ok);
	assert(candidate_equal(legacy, original));
	assert(economic_gameplay_authority::prepare_currency(nullptr) == error::invalid_identity);
	const std::array wallets = { economic_gameplay_wallet_mapping{
		7, { id(1), economic_account_kind::wallet, 101, 0 } } };
	const std::array banks = { economic_gameplay_bank_mapping{
		"fixture", 1, { id(1), economic_account_kind::bank, 202, 1 } } };
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, banks) == error::ok);
	assert(economic_gameplay_authority::active());
	auto previously_accepted = original;
	previously_accepted.accepted_at_usec = 17;
	const auto previous_bytes = previously_accepted;
	assert(economic_gameplay_authority::prepare_currency(&previously_accepted) ==
	       error::unauthorized);
	assert(candidate_equal(previously_accepted, previous_bytes));
	assert(economic_gameplay_authority::prepare_currency(&legacy) == error::ok);
	assert(legacy.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
	assert(legacy.payload == original.payload &&
	       legacy.operation_id.bytes == original.operation_id.bytes);
	assert(supported_candidate(legacy));
	economic_frozen_intent intent;
	assert(economic_intent_decode(legacy.accounting_intent, &intent) == error::ok);
	assert(intent.admission.metadata.epoch.bytes == id(2).bytes);
	assert(intent.admission.metadata.actor_id == 101);
	const auto retained = legacy;
	auto withdraw = transfer(currency_reason_type::atm_withdraw);
	assert(economic_gameplay_authority::prepare_currency(&withdraw) == error::ok);
	assert(supported_candidate(withdraw));
	auto starter = chaos_starter_bank();
	assert(economic_gameplay_authority::prepare_currency(&starter) == error::ok);
	assert(supported_candidate(starter));
	economic_frozen_intent starter_intent;
	assert(economic_intent_decode(starter.accounting_intent, &starter_intent) == error::ok);
	assert(starter_intent.admission.metadata.writer_id == ECONOMIC_WRITER_CHAOS_STARTER_BANK);
	assert(starter_intent.admission.metadata.reason == economic_reason::starter_reward);
	assert(starter_intent.admission.metadata.source_event);
	assert(starter_intent.admission.metadata.source_event->kind ==
	       economic_source_kind::starter_grant);
	for (auto invalid : { chaos_starter_bank(8), chaos_starter_bank(7, "another") })
	{
		const auto before = invalid;
		assert(economic_gameplay_authority::prepare_currency(&invalid) ==
		       error::incomplete_coverage);
		assert(candidate_equal(invalid, before));
	}
	for (int variant = 0; variant < 4; ++variant)
	{
		auto invalid = chaos_starter_bank();
		if (variant == 0)
			invalid.operation_id.bytes[0] ^= 1;
		else if (variant == 1)
			invalid.source_site = critical_source_site::command;
		else if (variant == 2)
			invalid.deadline_class = critical_deadline_class::interactive;
		else
		{
			currency_command_payload payload = {};
			assert(currency_command_decode_payload(invalid, &payload));
			payload.bank_delta.amount[3] = 999999;
			assert(currency_command_build(&invalid, invalid.operation_id, payload, 2,
						      UINT64_MAX, critical_source_site::login,
						      critical_deadline_class::recovery));
		}
		const auto before = invalid;
		assert(economic_gameplay_authority::prepare_currency(&invalid) != error::ok);
		assert(candidate_equal(invalid, before));
	}
	auto item_drop = item_transfer();
	assert(economic_gameplay_authority::prepare_item_transfer(&item_drop, 7) == error::ok);
	assert(item_drop.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
	assert(supported_candidate(item_drop));
	economic_frozen_intent item_intent;
	assert(economic_intent_decode(item_drop.accounting_intent, &item_intent) == error::ok);
	assert(item_intent.admission.metadata.epoch.bytes == id(2).bytes);
	assert(item_intent.admission.metadata.actor_id == 7);
	assert(item_intent.admission.metadata.writer_id == ECONOMIC_WRITER_ITEM_TRANSFER);
	assert(item_intent.admission.metadata.reason == economic_reason::item_move);
	item_drop.accepted_at_usec = 19;
	auto retained_item_drop = item_drop;
	assert(economic_gameplay_authority::prepare_item_transfer(&item_drop, 7) == error::ok);
	assert(candidate_equal(item_drop, retained_item_drop));
	auto unauthorized_item_drop = item_transfer();
	const auto unauthorized_item_drop_before = unauthorized_item_drop;
	assert(economic_gameplay_authority::prepare_item_transfer(&unauthorized_item_drop, 8) ==
	       error::unauthorized);
	assert(candidate_equal(unauthorized_item_drop, unauthorized_item_drop_before));
	item_transfer_payload craft_payload = {};
	assert(item_transfer_command_decode_payload(item_transfer(), &craft_payload));
	craft_payload.to_owner = craft_payload.from_owner;
	craft_payload.expected_to_revision = craft_payload.expected_from_revision;
	craft_payload.reason = item_transfer_reason::craft;
	craft_payload.reason_id = 551;
	craft_payload.multi_root = true;
	craft_payload.selected_item_uid = 600;
	craft_payload.target_root_item_uid = 0;
	player_item_snapshot craft_output = {};
	craft_output.object_uid = 600;
	craft_output.vnum = 9002;
	craft_output.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	craft_output.equipment_slot = -1;
	std::vector<uint8_t> craft_bytes;
	assert(player_item_snapshot_list_encode({ craft_output }, &craft_bytes) ==
	       player_snapshot_codec_result::ok);
	craft_payload.item_blob_size = craft_bytes.size();
	std::copy(craft_bytes.begin(), craft_bytes.end(), craft_payload.item_blob.begin());
	critical_command craft;
	assert(item_transfer_command_build(&craft, id(80), craft_payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	const auto unqualified_craft = craft;
	assert(economic_gameplay_authority::prepare_item_transfer(&craft, 7) ==
	       error::unauthorized);
	assert(candidate_equal(craft, unqualified_craft));
	assert(economic_gameplay_authority::prepare_item_transfer(
		       &craft, 8, economic_source_kind::crafting) == error::unauthorized);
	assert(candidate_equal(craft, unqualified_craft));
	assert(economic_gameplay_authority::prepare_item_transfer(
		       &craft, 7, economic_source_kind::crafting) == error::ok);
	assert(supported_candidate(craft) && item_transfer_accounting_command_supported(craft));
	economic_frozen_intent craft_intent;
	assert(economic_intent_decode(craft.accounting_intent, &craft_intent) == error::ok);
	assert(craft_intent.admission.metadata.reason == economic_reason::crafting_cost &&
	       craft_intent.admission.metadata.source_event->sequence == 500);

	// Failed cache replacement must not discard a usable verified projection.
	assert(lifecycle_access::install(id(1), {}, id(3), wallets, banks) ==
	       error::invalid_identity);
	auto wrong = wallets;
	wrong[0].account.lineage = id(4);
	assert(lifecycle_access::install(id(1), id(2), id(3), wrong, banks) ==
	       error::invalid_identity);
	std::array duplicate_wallets{ wallets[0], wallets[0] };
	duplicate_wallets[1].account.authority_id = 102;
	assert(lifecycle_access::install(id(1), id(2), id(3), duplicate_wallets, banks) ==
	       error::invalid_identity);
	std::array duplicate_banks{ banks[0], banks[0] };
	duplicate_banks[1].account.authority_id = 203;
	duplicate_banks[1].name = "FIXTURE";
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, duplicate_banks) ==
	       error::invalid_identity);
	auto invalid_bank = banks;
	invalid_bank[0].account.authority_id = 101;
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, invalid_bank) ==
	       error::invalid_identity);
	invalid_bank = banks;
	invalid_bank[0].name = "invalid/name";
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, invalid_bank) ==
	       error::invalid_identity);
	auto valid_again = original;
	assert(economic_gameplay_authority::prepare_currency(&valid_again) == error::ok);
	assert(candidate_equal(valid_again, retained));

	for (auto missing : { transfer(currency_reason_type::atm_deposit, 8),
			      transfer(currency_reason_type::atm_deposit, 7, "another"),
			      transfer(currency_reason_type::wallet_spend) })
	{
		const auto before = missing;
		assert(economic_gameplay_authority::prepare_currency(&missing) ==
		       error::incomplete_coverage);
		assert(candidate_equal(missing, before));
	}
	// Recreated native accounts get a new lifetime; retained commands never do.
	auto recreated = banks;
	recreated[0].account.authority_id = 303;
	assert(lifecycle_access::install(id(1), id(4), id(5), wallets, recreated) == error::ok);
	assert(economic_gameplay_authority::prepare_currency(&legacy) == error::ok);
	assert(candidate_equal(legacy, retained));
	legacy.payload.back() ^= 1;
	assert(economic_gameplay_authority::prepare_currency(&legacy) != error::ok);
	const auto corrupt = legacy;
	assert(economic_gameplay_authority::prepare_currency(&legacy) != error::ok);
	assert(candidate_equal(legacy, corrupt));

	// Readers see one complete immutable generation while lifecycle publication
	// replaces it. This checks the cache, not native lifecycle authorization.
	std::thread reader(
		[&]
		{
			for (unsigned index = 0; index < 500; ++index)
			{
				auto command = original;
				assert(economic_gameplay_authority::prepare_currency(&command) ==
				       error::ok);
				assert(supported_candidate(command));
			}
		});
	for (unsigned index = 0; index < 100; ++index)
		assert(lifecycle_access::install(id(1), id(index % 2 ? 2 : 4), id(5), wallets,
						 index % 2 ? banks : recreated) == error::ok);
	reader.join();
	qualified_projection_regressions();
	lifecycle_access::clear_sql_runtime();
	assert(economic_gameplay_authority::active());
	lifecycle_access::reset();
	assert(lifecycle_access::install(id(1), id(2), id(3), wallets, banks) == error::ok);
	lifecycle_access::clear_sql_runtime();
	assert(!economic_gameplay_authority::active());
	std::cout
		<< "gameplay authority admission, exact replay, fail-closed coverage and atomic cache passed\n";
}
