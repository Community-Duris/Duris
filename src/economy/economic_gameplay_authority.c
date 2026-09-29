#include "economy/economic_gameplay_authority.h"
#include "economy/economic_command_admission.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"

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
enum class projection_scope : uint8_t
{
	regular,
	sql_wallet_root_qualification,
};
constexpr uint16_t SQL_WALLET_ROOT_QUALIFICATION_VERSION = 1;

struct admission_projection
{
	critical_operation_id lineage;
	critical_operation_id epoch;
	critical_operation_id receipt;
	projection_scope scope = projection_scope::regular;
	uint16_t scope_version = 0;
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

economic_accounting_error
publish_projection(const critical_operation_id &lineage, const critical_operation_id &epoch,
		   const critical_operation_id &receipt,
		   std::span<const economic_gameplay_wallet_mapping> wallets,
		   std::span<const economic_gameplay_bank_mapping> banks, projection_scope scope)
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
		next->scope = scope;
		next->scope_version = scope == projection_scope::sql_wallet_root_qualification ?
					      SQL_WALLET_ROOT_QUALIFICATION_VERSION :
					      0;
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

bool sql_wallet_root_scope(const admission_projection &selected)
{
	return selected.scope == projection_scope::sql_wallet_root_qualification &&
	       selected.scope_version == SQL_WALLET_ROOT_QUALIFICATION_VERSION;
}

economic_accounting_error qualified_wallet_root_intent(const admission_projection &selected,
						       const critical_command &command,
						       std::vector<uint8_t> *encoded)
{
	using error = economic_accounting_error;
	if (!encoded || command.type != critical_command_type::coin_transfer)
		return error::unauthorized;
	coin_transfer_payload payload = {};
	if (!coin_transfer_command_decode_payload(command, &payload) ||
	    payload.source.change.type != critical_command_type::account_bank ||
	    payload.destination.change.type != critical_command_type::account_bank)
		return error::unauthorized;
	currency_command_payload source = {}, destination = {};
	if (!currency_command_decode_payload(payload.source.change, &source) ||
	    !currency_command_decode_payload(payload.destination.change, &destination))
		return error::corrupt_evidence;
	const auto source_mapping = selected.wallets.find(source.pid);
	const auto destination_mapping = selected.wallets.find(destination.pid);
	if (source_mapping == selected.wallets.end() ||
	    destination_mapping == selected.wallets.end())
		return error::incomplete_coverage;
	critical_command admission = command;
	if (admission.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
	{
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		admission.publication_required = false;
	}
	else if (admission.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION)
		return error::invalid_version;
	return coin_transfer_accounting_intent(admission, selected.epoch, source_mapping->second,
					       destination_mapping->second, encoded);
}
} // namespace

economic_accounting_error
economic_gameplay_authority::install(const critical_operation_id &lineage,
				     const critical_operation_id &epoch,
				     const critical_operation_id &receipt,
				     std::span<const economic_gameplay_wallet_mapping> wallets,
				     std::span<const economic_gameplay_bank_mapping> banks)
{
	return publish_projection(lineage, epoch, receipt, wallets, banks,
				  projection_scope::regular);
}

economic_accounting_error economic_gameplay_authority::install_sql_wallet_root_qualification(
	sql_wallet_root_qualification_install_key, const critical_operation_id &lineage,
	const critical_operation_id &epoch, const critical_operation_id &receipt,
	std::span<const economic_gameplay_wallet_mapping> wallets,
	std::span<const economic_gameplay_bank_mapping> banks)
{
	return publish_projection(lineage, epoch, receipt, wallets, banks,
				  projection_scope::sql_wallet_root_qualification);
}

void economic_gameplay_authority::clear_sql_runtime() noexcept
{
	auto selected = current.load(std::memory_order_acquire);
	if (selected && selected->scope == projection_scope::regular)
		current.compare_exchange_strong(selected, {}, std::memory_order_acq_rel);
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
		const auto selected = current.load(std::memory_order_acquire);
		// Qualification is root-only even for previously frozen schema-2
		// currency commands. Keep the ordinary historical replay path unchanged.
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		// Retained schema-2 commands keep their original epoch and lifetimes.
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return supported_candidate(*command) ? error::ok : error::unauthorized;
		if (!critical_command_legacy_execution_supported(*command))
			return error::corrupt_evidence;
		if (!selected)
			return error::ok;
		// Never retag a previously accepted schema-1 identity. A retained
		// receipt must continue through its original native replay route.
		if (command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
		currency_command_payload payload;
		if (!currency_command_decode_payload(*command, &payload))
			return error::corrupt_evidence;
		const bool quest_wallet_reward =
			payload.reason == currency_reason_type::wallet_reward &&
			payload.reason_id > 0 &&
			command->source_site == critical_source_site::recovery &&
			command->deadline_class == critical_deadline_class::recovery;
		if (payload.reason != currency_reason_type::atm_deposit &&
		    payload.reason != currency_reason_type::atm_withdraw &&
		    payload.reason != currency_reason_type::chaos_starter_reward &&
		    !quest_wallet_reward)
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
		error result;
		if (payload.reason == currency_reason_type::chaos_starter_reward)
			result = economic_chaos_starter_bank_intent(
				frozen, selected->epoch, wallet->second, bank->second, &intent);
		else if (quest_wallet_reward)
			result = economic_quest_wallet_reward_intent(
				frozen, selected->epoch, wallet->second, bank->second, &intent);
		else
			result = economic_bank_transfer_intent(
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

economic_accounting_error
economic_gameplay_authority::prepare_coin_transfer(critical_command *command)
{
	using error = economic_accounting_error;
	if (!command)
		return error::invalid_identity;
	try
	{
		auto supported_candidate = [](const critical_command &candidate)
		{
			auto projection = candidate;
			if (!projection.accepted_at_usec)
				projection.accepted_at_usec = 1;
			return economic_command_admission_supported(projection);
		};
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_wallet_root_scope(*selected))
		{
			if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			{
				if (!supported_candidate(*command))
					return error::unauthorized;
				std::vector<uint8_t> expected_intent;
				const auto result = qualified_wallet_root_intent(
					*selected, *command, &expected_intent);
				if (result != error::ok)
					return result;
				return expected_intent == command->accounting_intent ?
					       error::ok :
					       error::payload_conflict;
			}
			if (!critical_command_legacy_execution_supported(*command))
				return error::corrupt_evidence;
			if (command->accepted_at_usec || command->publication_required)
				return error::unauthorized;
			critical_command frozen = *command;
			std::vector<uint8_t> intent;
			const auto result =
				qualified_wallet_root_intent(*selected, frozen, &intent);
			if (result != error::ok)
				return result;
			frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
			frozen.accounting_intent = std::move(intent);
			if (!supported_candidate(frozen))
				return error::corrupt_evidence;
			*command = std::move(frozen);
			return error::ok;
		}
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return supported_candidate(*command) ? error::ok : error::unauthorized;
		if (!selected)
			return error::ok;
		if (!critical_command_legacy_execution_supported(*command))
			return error::corrupt_evidence;
		if (command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
		coin_transfer_payload payload;
		if (!coin_transfer_command_decode_payload(*command, &payload))
			return error::corrupt_evidence;
		auto account_for = [&](const coin_transfer_endpoint &endpoint,
				       economic_account_key *account) -> error
		{
			if (endpoint.change.type == critical_command_type::account_bank)
			{
				currency_command_payload wallet = {};
				if (!currency_command_decode_payload(endpoint.change, &wallet))
					return error::corrupt_evidence;
				const auto found = selected->wallets.find(wallet.pid);
				if (found == selected->wallets.end())
					return error::incomplete_coverage;
				*account = found->second;
				return error::ok;
			}
			if (endpoint.change.type == critical_command_type::item_transfer)
			{
				item_transfer_payload pile = {};
				if (!item_transfer_command_decode_payload(endpoint.change, &pile) ||
				    pile.item_count != 1 ||
				    pile.selected_item_uid != pile.items[0].item_uid)
					return error::corrupt_evidence;
				*account = { selected->lineage, economic_account_kind::pile,
					     pile.selected_item_uid, 0 };
				return economic_account_key_valid(*account) ?
					       error::ok :
					       error::invalid_identity;
			}
			return error::incomplete_coverage;
		};
		economic_account_key source = {}, destination = {};
		const auto source_result = account_for(payload.source, &source);
		if (source_result != error::ok)
			return source_result;
		const auto destination_result = account_for(payload.destination, &destination);
		if (destination_result != error::ok)
			return destination_result;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		const auto result = coin_transfer_accounting_intent(frozen, selected->epoch, source,
								    destination, &intent);
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

economic_accounting_error
economic_gameplay_authority::prepare_item_transfer(critical_command *command, uint32_t actor_pid,
						   economic_source_kind lifecycle_source)
{
	using error = economic_accounting_error;
	if (!command)
		return error::invalid_identity;
	try
	{
		auto supported_candidate = [](const critical_command &candidate)
		{
			auto projection = candidate;
			if (!projection.accepted_at_usec)
				projection.accepted_at_usec = 1;
			return economic_command_admission_supported(projection);
		};
		const auto selected = current.load(std::memory_order_acquire);
		// SQL wallet-root qualification excludes standalone item roots,
		// including commands frozen before this restricted projection existed.
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return supported_candidate(*command) &&
					       item_transfer_accounting_command_supported(
						       *command) ?
				       error::ok :
				       error::unauthorized;
		if (!critical_command_legacy_execution_supported(*command))
			return error::corrupt_evidence;
		if (!selected)
			return error::ok;
		if (command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		const auto result = item_transfer_accounting_intent(frozen, selected->lineage,
								    selected->epoch, actor_pid,
								    &intent, lifecycle_source);
		if (result != error::ok)
			return result;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(intent);
		if (!supported_candidate(frozen) ||
		    !item_transfer_accounting_command_supported(frozen))
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
