#include "economy/economic_gameplay_authority.h"
#include "economy/economic_command_admission.h"
#include "economy/item_transfer_accounting.h"
#include "item/item_transfer_command.h"

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>

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
	static void reset() { economic_gameplay_authority::reset_for_tests(); }
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
	std::cout
		<< "gameplay authority admission, exact replay, fail-closed coverage and atomic cache passed\n";
}
