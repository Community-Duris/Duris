#include "economy/economic_gameplay_authority.h"
#include "economy/economic_command_admission.h"

#include <atomic>
#include <climits>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string_view>
#include <utility>

namespace
{
struct admission_projection
{
	critical_operation_id lineage;
	critical_operation_id epoch;
	critical_operation_id receipt;
	std::map<uint32_t, economic_account_key> wallets;
	std::map<std::pair<std::string, uint8_t>, economic_account_key> banks;
};
std::atomic<std::shared_ptr<const admission_projection>> current;

bool bank_locator(std::string_view input, std::string *canonical)
{
	if (input.empty() || input.size() > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	std::string name;
	name.reserve(input.size());
	for (unsigned char character : input)
	{
		if (character >= 'A' && character <= 'Z')
			character += 'a' - 'A';
		if (!((character >= 'a' && character <= 'z') ||
		      (character >= '0' && character <= '9') || character == '_' ||
		      character == '-'))
			return false;
		name.push_back(static_cast<char>(character));
	}
	*canonical = std::move(name);
	return true;
}

bool mapping_valid(const economic_account_key &account, economic_account_kind kind,
		   const critical_operation_id &lineage, uint64_t context)
{
	return economic_account_key_valid(account) && account.kind == kind &&
	       account.context_id == context && account.lineage.bytes == lineage.bytes;
}
} // namespace

economic_accounting_error
economic_gameplay_authority::install(const critical_operation_id &lineage,
				     const critical_operation_id &epoch,
				     const critical_operation_id &receipt,
				     std::span<const economic_gameplay_wallet_mapping> wallets,
				     std::span<const economic_gameplay_bank_mapping> banks)
{
	using error = economic_accounting_error;
	if (critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch) ||
	    critical_operation_id_is_zero(receipt))
		return error::invalid_identity;
	try
	{
		auto next = std::make_shared<admission_projection>();
		next->lineage = lineage;
		next->epoch = epoch;
		next->receipt = receipt;
		std::set<uint64_t> lifetimes;
		for (const auto &wallet : wallets)
		{
			if (!wallet.pid || wallet.pid > INT32_MAX ||
			    !mapping_valid(wallet.account, economic_account_kind::wallet, lineage,
					   0) ||
			    !lifetimes.insert(wallet.account.authority_id).second ||
			    !next->wallets.emplace(wallet.pid, wallet.account).second)
				return error::invalid_identity;
		}
		for (const auto &bank : banks)
		{
			std::string canonical;
			if (bank.racewar > INT8_MAX || !bank_locator(bank.name, &canonical) ||
			    !mapping_valid(bank.account, economic_account_kind::bank, lineage,
					   bank.racewar) ||
			    !lifetimes.insert(bank.account.authority_id).second ||
			    !next->banks
				     .emplace(std::make_pair(std::move(canonical), bank.racewar),
					      bank.account)
				     .second)
				return error::invalid_identity;
		}
		std::shared_ptr<const admission_projection> published = std::move(next);
		current.store(std::move(published), std::memory_order_release);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool economic_gameplay_authority::active()
{
	return bool(current.load(std::memory_order_acquire));
}

economic_accounting_error economic_gameplay_authority::prepare_currency(critical_command *command)
{
	using error = economic_accounting_error;
	if (!command)
		return error::invalid_identity;
	try
	{
		// The coordinator assigns accepted_at_usec after this builder. Validate
		// only a timestamped projection, as the existing command-binding codec
		// does; never store the sentinel in the command or its durable receipt.
		auto supported_candidate = [](const critical_command &candidate)
		{
			auto projection = candidate;
			if (!projection.accepted_at_usec)
				projection.accepted_at_usec = 1;
			return economic_command_admission_supported(projection);
		};
		// Retained schema-2 commands keep their original epoch and lifetimes.
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return supported_candidate(*command) ? error::ok : error::unauthorized;
		if (!critical_command_legacy_execution_supported(*command))
			return error::corrupt_evidence;
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected)
			return error::ok;
		// Never retag a previously accepted schema-1 identity. A retained
		// receipt must continue through its original native replay route.
		if (command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
		currency_command_payload payload;
		if (!currency_command_decode_payload(*command, &payload))
			return error::corrupt_evidence;
		if (payload.reason != currency_reason_type::atm_deposit &&
		    payload.reason != currency_reason_type::atm_withdraw)
			return error::incomplete_coverage;
		std::string canonical;
		if (!bank_locator(payload.account_name.data(), &canonical))
			return error::invalid_identity;
		const auto wallet = selected->wallets.find(payload.pid);
		const auto bank = selected->banks.find({ canonical, payload.racewar });
		if (wallet == selected->wallets.end() || bank == selected->banks.end())
			return error::incomplete_coverage;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		const auto result = economic_bank_transfer_intent(
			frozen, selected->epoch, wallet->second, bank->second, &intent);
		if (result != error::ok)
			return result;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(intent);
		if (!supported_candidate(frozen))
			return error::corrupt_evidence;
		*command = std::move(frozen);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

#ifdef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
void economic_gameplay_authority::reset_for_tests()
{
	current.store(std::shared_ptr<const admission_projection>{}, std::memory_order_release);
}
#endif
