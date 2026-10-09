#include "economy/economic_gameplay_authority.h"
#include "economy/auction_repository.h"
#include "economy/auction_native_command_context.h"
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#endif
#include <cerrno>
#include "economy/native_quest_consumption_capture.h"
#include "world/quest_mobile_native_binding.h"
#include "player/player_snapshot_codec.h"
#include "economy/economic_command_admission.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/zone_reset_item_command.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"
#include "economy/collector_accounting.h"
#include "persistence/persistence_mode.h"
#include "economy/shop_trade_accounting.h"
#include "economy/shop_trade_item_payload.h"

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
	sql_runtime_recovery,
};
constexpr uint16_t SQL_WALLET_ROOT_QUALIFICATION_VERSION = 1;
constexpr uint16_t SQL_RUNTIME_RECOVERY_VERSION = 1;

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
		   std::span<const economic_gameplay_bank_mapping> banks, projection_scope scope,
		   std::shared_ptr<const admission_projection> expected = {})
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
				      scope == projection_scope::sql_runtime_recovery ?
					      SQL_RUNTIME_RECOVERY_VERSION :
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
		if (expected)
		{
			if (!current.compare_exchange_strong(expected, std::move(published),
							     std::memory_order_acq_rel,
							     std::memory_order_acquire))
				return error::unauthorized;
		}
		else
			current.store(std::move(published), std::memory_order_release);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool sql_recovery_scope(const admission_projection &selected)
{
	return selected.scope == projection_scope::sql_runtime_recovery &&
	       selected.scope_version == SQL_RUNTIME_RECOVERY_VERSION;
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

economic_accounting_error economic_gameplay_authority::install_sql_recovery(
	sql_runtime_recovery_install_key, const critical_operation_id &lineage,
	const critical_operation_id &epoch, const critical_operation_id &receipt)
{
	if (!persistence_mode_requires_mysql() || current.load(std::memory_order_acquire))
		return economic_accounting_error::unauthorized;
	return publish_projection(lineage, epoch, receipt, {}, {},
				  projection_scope::sql_runtime_recovery);
}

bool economic_gameplay_authority::sql_recovery_selection_matches(
	sql_runtime_recovery_install_key, const critical_operation_id &lineage,
	const critical_operation_id &epoch, const critical_operation_id &receipt) noexcept
{
	const auto selected = current.load(std::memory_order_acquire);
	return persistence_mode_requires_mysql() && selected && sql_recovery_scope(*selected) &&
	       selected->lineage.bytes == lineage.bytes && selected->epoch.bytes == epoch.bytes &&
	       selected->receipt.bytes == receipt.bytes;
}

economic_accounting_error economic_gameplay_authority::finish_sql_recovery(
	sql_runtime_recovery_install_key, const critical_operation_id &lineage,
	const critical_operation_id &epoch, const critical_operation_id &receipt,
	std::span<const economic_gameplay_wallet_mapping> wallets,
	std::span<const economic_gameplay_bank_mapping> banks)
{
	const auto selected = current.load(std::memory_order_acquire);
	if (!persistence_mode_requires_mysql() || !selected || !sql_recovery_scope(*selected) ||
	    selected->lineage.bytes != lineage.bytes || selected->epoch.bytes != epoch.bytes ||
	    selected->receipt.bytes != receipt.bytes)
		return economic_accounting_error::unauthorized;
	// Publish only the exact selected recovery incarnation. A failed CAS leaves
	// the current authority intact and grants no ready projection.
	return publish_projection(lineage, epoch, receipt, wallets, banks,
				  projection_scope::regular, selected);
}

bool economic_gameplay_authority::sql_runtime_projection_matches(
	sql_runtime_recovery_install_key, const critical_operation_id &lineage,
	const critical_operation_id &epoch, const critical_operation_id &receipt,
	std::span<const economic_gameplay_wallet_mapping> wallets,
	std::span<const economic_gameplay_bank_mapping> banks) noexcept
{
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!persistence_mode_requires_mysql() || !selected ||
		    selected->scope != projection_scope::regular || selected->scope_version ||
		    selected->lineage.bytes != lineage.bytes ||
		    selected->epoch.bytes != epoch.bytes ||
		    selected->receipt.bytes != receipt.bytes ||
		    selected->wallets.size() != wallets.size() ||
		    selected->banks.size() != banks.size())
			return false;
		std::set<uint32_t> seen_wallets;
		std::set<std::pair<std::string, uint8_t>> seen_banks;
		for (const auto &wallet : wallets)
		{
			const auto found = selected->wallets.find(wallet.pid);
			if (!seen_wallets.insert(wallet.pid).second ||
			    found == selected->wallets.end() ||
			    !economic_account_key_equal(found->second, wallet.account))
				return false;
		}
		for (const auto &bank : banks)
		{
			std::string canonical;
			if (!bank_locator(bank.name, &canonical))
				return false;
			const auto locator = std::make_pair(std::move(canonical), bank.racewar);
			const auto found = selected->banks.find(locator);
			if (!seen_banks.insert(locator).second || found == selected->banks.end() ||
			    !economic_account_key_equal(found->second, bank.account))
				return false;
		}
		return current.load(std::memory_order_acquire) == selected;
	}
	catch (...)
	{
		return false;
	}
}

void economic_gameplay_authority::clear_sql_runtime() noexcept
{
	auto selected = current.load(std::memory_order_acquire);
	if (selected &&
	    (selected->scope == projection_scope::regular || sql_recovery_scope(*selected)))
		current.compare_exchange_strong(selected, {}, std::memory_order_acq_rel);
}

void economic_gameplay_authority::clear_flat_runtime() noexcept
{
	auto selected = current.load(std::memory_order_acquire);
	if (selected && selected->scope == projection_scope::regular)
		current.compare_exchange_strong(selected, {}, std::memory_order_acq_rel);
}

bool economic_gameplay_authority::active()
{
	return bool(current.load(std::memory_order_acquire));
}

bool economic_gameplay_authority::active_sql_recovery()
{
	const auto selected = current.load(std::memory_order_acquire);
	return persistence_mode_requires_mysql() && selected && sql_recovery_scope(*selected);
}

bool economic_gameplay_authority::active_regular_sql()
{
	const auto selected = current.load(std::memory_order_acquire);
	return persistence_mode_requires_mysql() && selected &&
	       selected->scope == projection_scope::regular && selected->scope_version == 0;
}

bool economic_gameplay_authority::active_regular_flat()
{
	const auto selected = current.load(std::memory_order_acquire);
	const char *root = persistence_mode_flatfile_root();
	return persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY && root && *root &&
	       selected && selected->scope == projection_scope::regular && !selected->scope_version;
}

economic_accounting_error
economic_gameplay_authority::prepare_zone_reset_item(const zone_reset_item_image &original,
						     uint64_t accepted_at_usec,
						     critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql() || original.items.empty())
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.operation_id;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.items.front().object_uid;
		metadata.writer_id = ECONOMIC_WRITER_ZONE_RESET_ITEM_BIRTH;
		metadata.reason = economic_reason::item_create;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reset_source;
		// Replay decodes the retained command; never select a new current epoch.
		return zone_reset_item_command_build(metadata, original, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error
economic_gameplay_authority::prepare_zone_reset_item_flat(const zone_reset_item_image &original,
							  uint64_t accepted_at_usec,
							  critical_command *output) noexcept
{
	using error = economic_accounting_error;
	const char *root = persistence_mode_flatfile_root();
	if (!output || persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    persistence_mode_requires_mysql() || !root || !*root || original.items.empty())
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.operation_id;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.items.front().object_uid;
		metadata.writer_id = ECONOMIC_WRITER_ZONE_RESET_ITEM_BIRTH;
		metadata.reason = economic_reason::item_create;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reset_source;
		// Replay decodes the retained command; never select a new current epoch.
		return zone_reset_item_command_build(metadata, original, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_native_mobile_birth(
	const quest_mobile_native_image &original, critical_source_site original_site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql())
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_command_build(metadata, original, original_site,
							 accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_native_mobile_birth(
	const quest_mobile_native_image &original,
	std::span<const native_mobile_birth_item_recipe> recipes,
	critical_source_site original_site, uint64_t accepted_at_usec,
	critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql())
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_command_build(metadata, original, recipes, original_site,
							 accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_native_mobile_birth(
	const quest_mobile_native_image &original,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const quest_mobile_native_constructor_recipe &constructor,
	critical_source_site original_site, uint64_t accepted_at_usec,
	critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql())
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_command_build(metadata, original, recipes, constructor,
							 original_site, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_native_mobile_birth_ordinary_wallet(
	const quest_mobile_native_image &original,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site original_site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql() ||
	    role.role != native_mobile_birth_cash_role::ordinary_wallet)
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_cash_role_command_build(
			metadata, original, recipes, role, original_site, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error
economic_gameplay_authority::prepare_native_mobile_birth_shared_shopkeeper(
	const quest_mobile_native_image &original,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site original_site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	using error = economic_accounting_error;
	if (!output || !persistence_mode_requires_mysql() ||
	    role.role != native_mobile_birth_cash_role::shared_shopkeeper)
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_cash_role_command_build(
			metadata, original, recipes, role, original_site, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

bool economic_gameplay_authority::observe_craft_wallet_checkpoint(
	uint32_t pid, economic_native_money_checkpoint_projection *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX ||
	    (!persistence_mode_requires_mysql() &&
	     (persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	      !active_regular_flat())))
		return false;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version || critical_operation_id_is_zero(selected->lineage) ||
		    critical_operation_id_is_zero(selected->epoch))
			return false;
		const auto wallet = selected->wallets.find(pid);
		if (wallet == selected->wallets.end() ||
		    !mapping_valid(wallet->second, economic_account_kind::wallet, selected->lineage,
				   0))
			return false;
		*output = { selected->lineage, selected->epoch, wallet->second };
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool economic_gameplay_authority::observe_shop_checkpoint(
	uint32_t pid, std::string_view account_name, uint8_t racewar,
	economic_shop_checkpoint_projection *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX || racewar > INT8_MAX ||
	    !persistence_mode_requires_mysql())
		return false;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version)
			return false;
		std::string canonical;
		if (!bank_locator(account_name, &canonical))
			return false;
		const auto wallet = selected->wallets.find(pid);
		const auto bank = selected->banks.find({ canonical, racewar });
		if (wallet == selected->wallets.end() || bank == selected->banks.end())
			return false;
		*output = { selected->lineage, selected->epoch, wallet->second, bank->second };
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool economic_gameplay_authority::observe_flat_shop_checkpoint(
	uint32_t pid, std::string_view account_name, uint8_t racewar,
	economic_shop_checkpoint_projection *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX || racewar > INT8_MAX ||
	    (persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	     !persistence_mode_flatfile_root() || !*persistence_mode_flatfile_root()))
		return false;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version)
			return false;
		std::string canonical;
		if (!bank_locator(account_name, &canonical))
			return false;
		const auto wallet = selected->wallets.find(pid);
		const auto bank = selected->banks.find({ canonical, racewar });
		if (wallet == selected->wallets.end() || bank == selected->banks.end())
			return false;
		if (!mapping_valid(wallet->second, economic_account_kind::wallet, selected->lineage,
				   0) ||
		    !mapping_valid(bank->second, economic_account_kind::bank, selected->lineage,
				   racewar) ||
		    current.load(std::memory_order_acquire) != selected)
			return false;
		*output = { selected->lineage, selected->epoch, wallet->second, bank->second };
		return true;
	}
	catch (...)
	{
		return false;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_auction(critical_command *command)
{
	using error = economic_accounting_error;
	if (!command || command->type != critical_command_type::auction)
		return error::invalid_identity;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
		auto candidate = *command;
		// Structural validation uses the existing binding projection sentinel;
		// admission still assigns the real timestamp to the original command.
		if (!candidate.accepted_at_usec)
			candidate.accepted_at_usec = 1;
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return auction_repository_frozen_accounting_valid(candidate) ?
				       error::ok :
				       error::corrupt_evidence;
		const bool native = candidate.payload_version ==
				    AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION;
		if (native)
		{
			if (!selected || selected->scope != projection_scope::regular ||
			    selected->scope_version || !persistence_mode_requires_mysql() ||
			    command->accepted_at_usec || command->publication_required ||
			    !command->accounting_intent.empty())
				return error::unauthorized;
			auction_native_command_context context;
			const auto decoded = auction_native_command_decode(candidate, &context);
			if (decoded != error::ok)
				return decoded;
		}
		else if (!critical_command_legacy_execution_supported(candidate) ||
			 !critical_command_envelope_valid(candidate))
			return error::corrupt_evidence;
		if (!selected)
			return error::ok;
		if (sql_wallet_root_scope(*selected) || !persistence_mode_requires_mysql() ||
		    command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
#ifdef __NO_MYSQL__
		return error::incomplete_coverage;
#else
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return error::unresolved;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		unsigned int capture_error = EIO;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
				capture_error = auction_repository_prepare_accounting(
					connection, selected->lineage, selected->epoch, &candidate);
			if (!transaction.same_session())
				capture_error = ENOTCONN;
		}
		catch (const std::bad_alloc &)
		{
			capture_error = ENOMEM;
		}
		catch (...)
		{
			capture_error = EIO;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return error::unresolved;
		if (capture_error)
		{
			switch (capture_error)
			{
			case E2BIG:
			case ENOMEM:
				return error::capacity;
			case ENOENT:
				return error::incomplete_coverage;
			case EPROTONOSUPPORT:
				return error::incomplete_coverage;
			case ESTALE:
				return error::stale_revision;
			case EPERM:
				return error::unauthorized;
			case EILSEQ:
				return error::corrupt_evidence;
			default:
				return error::unresolved;
			}
		}
		// Session cleanup precedes publishing any frozen native/absence facts.
		// The current activation owner may have paused while capture ran.
		if (current.load(std::memory_order_acquire) != selected)
			return error::unauthorized;
		candidate.accepted_at_usec = command->accepted_at_usec;
		*command = std::move(candidate);
		return error::ok;
#endif
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
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
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
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
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
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
economic_gameplay_authority::prepare_collector_purchase(critical_command *command,
							const collector::record &original_listing)
{
	using error = economic_accounting_error;
	if (!command)
		return error::invalid_identity;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
		// The existing qualification projection admits wallet roots only.
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		auto supported_candidate = [](const critical_command &candidate)
		{
			auto projection = candidate;
			if (!projection.accepted_at_usec)
				projection.accepted_at_usec = 1;
			return economic_command_admission_supported(projection);
		};
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			// Historical replay keeps the admitted listing and lifetimes. A
			// mutable catalog entry cannot replace its original frozen listing.
			if (!supported_candidate(*command))
				return error::unauthorized;
			auto projection = *command;
			if (!projection.accepted_at_usec)
				projection.accepted_at_usec = 1;
			economic_frozen_intent intent;
			collector_command_payload payload;
			collector::record retained_listing;
			economic_account_key wallet, bank;
			const auto status = collector_purchase_accounting_decode(
				projection, &intent, &payload, &retained_listing, &wallet, &bank);
			if (status != error::ok)
				return status;
			std::array<uint8_t, collector::encoded_record_bytes> retained{}, supplied{};
			if (collector::record_encode(retained_listing, &retained) !=
				    collector::codec_result::ok ||
			    collector::record_encode(original_listing, &supplied) !=
				    collector::codec_result::ok)
				return error::invalid_identity;
			return retained == supplied ? error::ok : error::payload_conflict;
		}
		if (!critical_command_legacy_execution_supported(*command))
			return error::corrupt_evidence;
		if (!selected)
			return error::ok;
		if (command->accepted_at_usec || command->publication_required)
			return error::unauthorized;
		collector_command_payload payload;
		if (!collector_command_decode_payload(*command, &payload) ||
		    payload.action != collector_action::purchase)
			return error::corrupt_evidence;
		std::string canonical;
		if (!bank_locator(payload.account_name.data(), &canonical))
			return error::invalid_identity;
		const auto wallet = selected->wallets.find(payload.actor_pid);
		const auto bank = selected->banks.find({ canonical, payload.racewar });
		if (wallet == selected->wallets.end() || bank == selected->banks.end())
			return error::incomplete_coverage;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		const auto status = collector_purchase_accounting_intent(frozen, selected->epoch,
									 wallet->second,
									 bank->second,
									 original_listing, &intent);
		if (status != error::ok)
			return status;
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

economic_accounting_error economic_gameplay_authority::prepare_shop_trade(critical_command *command)
{
	using error = economic_accounting_error;
	if (!command)
		return error::invalid_identity;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		if (!shop_trade_payload_version_is_accounted(command->payload_version))
			return !selected && critical_command_legacy_execution_supported(*command) &&
					       command->type == critical_command_type::shop_trade ?
				       error::ok :
				       error::unauthorized;
		if (command->type != critical_command_type::shop_trade)
			return error::unauthorized;
		auto projection = *command;
		if (!projection.accepted_at_usec)
			projection.accepted_at_usec = 1;
		if (!critical_command_envelope_valid(projection))
			return error::corrupt_evidence;
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			// Exact-ID history keeps the original epoch, lifetimes and status.
			// This pure check never rebinds it to today's admission projection.
			economic_frozen_intent intent;
			shop_trade_payload payload = {};
			economic_account_key wallet, bank, counterparty;
			const auto result = shop_trade_accounting_decode(
				projection, &intent, &payload, &wallet, &bank, &counterparty);
			if (result != error::ok)
				return result;
			std::vector<player_item_snapshot> after;
			return shop_trade_accounted_after_items(payload, &after) ?
				       error::ok :
				       error::corrupt_evidence;
		}
		// v6 is only a transient pure source checkpoint until schema2 freezing.
		// It cannot enter the legacy native executor, even without authority.
		if (!selected || command->schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
		    !command->accounting_intent.empty() || command->accepted_at_usec ||
		    command->publication_required)
			return error::unauthorized;
		shop_trade_payload payload = {};
		std::vector<player_item_snapshot> after;
		if (!shop_trade_command_decode_payload(*command, &payload) ||
		    !shop_trade_accounted_after_items(payload, &after))
			return error::corrupt_evidence;
		std::string canonical;
		if (!bank_locator(payload.account_name.data(), &canonical))
			return error::invalid_identity;
		const auto wallet = selected->wallets.find(payload.player_pid);
		const auto bank = selected->banks.find({ canonical, payload.racewar });
		if (wallet == selected->wallets.end() || bank == selected->banks.end())
			return error::incomplete_coverage;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		const auto result = shop_trade_shared_accounting_intent(
			frozen, selected->epoch, wallet->second, bank->second, &intent);
		if (result != error::ok)
			return result;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(intent);
		projection = frozen;
		projection.accepted_at_usec = 1;
		economic_frozen_intent verified;
		shop_trade_payload verified_payload = {};
		economic_account_key verified_wallet, verified_bank, counterparty;
		if (shop_trade_accounting_decode(projection, &verified, &verified_payload,
						 &verified_wallet, &verified_bank,
						 &counterparty) != error::ok)
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
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
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
		// Fresh compound craft fees must use this SAME installed projection.
		// An absent mapping is an invalid key, not nullptr: nullptr is reserved
		// for canonical regeneration of an already-frozen historical command.
		const economic_account_key absent_wallet{};
		const auto wallet = selected->wallets.find(actor_pid);
		const auto *fresh_wallet = wallet == selected->wallets.end() ?
					  &absent_wallet : &wallet->second;
		const auto result = item_transfer_accounting_intent(frozen, selected->lineage,
								    selected->epoch, actor_pid,
								    &intent, lifecycle_source, fresh_wallet);
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

quest_native_consumption_capture::quest_native_consumption_capture(
	critical_command original, const quest_mobile_native_reference &reference,
	uint64_t runtime_generation, uint32_t final_giver_pid, uint32_t completion_slot,
	std::vector<uint64_t> ordered_consumed_roots,
	item_native_quest_publication_terms publication_terms, economic_source_kind action,
	bool fee_only)
	: original_(std::move(original))
	, reference_(reference)
	, runtime_generation_(runtime_generation)
	, final_giver_pid_(final_giver_pid)
	, consumed_root_order_(std::move(ordered_consumed_roots))
	, publication_terms_(std::move(publication_terms))
	, source_{ action, fee_only ? original_.operation_id : reference.birth_operation,
		   reference.birth_source.generation,
		   fee_only ? reference.mobile_revision : reference.stock_revision,
		   completion_slot }
{
}

economic_accounting_error economic_gameplay_authority::prepare_native_item_transfer(
	critical_command *command, uint32_t final_giver_pid,
	const quest_native_consumption_capture *original)
{
	using error = economic_accounting_error;
	if (!command || !final_giver_pid || final_giver_pid > INT32_MAX)
		return error::invalid_identity;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		if (command->type != critical_command_type::item_transfer ||
		    (command->payload_version !=
			     ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
		     command->payload_version !=
			     ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION))
			return error::unauthorized;
		item_transfer_payload payload = {};
		if (!item_transfer_command_decode_payload(*command, &payload) ||
		    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    payload.native_mobile.final_giver_pid != final_giver_pid)
			return error::corrupt_evidence;
		const economic_source_event *event = nullptr;
		if (payload.native_mobile.action == item_native_mobile_action::acceptance)
		{
			if (original ||
			    payload.continuation.kind != item_transfer_continuation_kind::none)
				return error::unauthorized;
		}
		else
		{
			if (!original || !original->runtime_generation() ||
			    original->final_giver_pid() != final_giver_pid ||
			    !quest_mobile_native_reference_valid(original->reference()) ||
			    !critical_operation_id_equal(command->operation_id,
							 original->original_command().operation_id))
				return error::unauthorized;
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> captured = {},
										 supplied = {};
			if (quest_mobile_native_reference_encode(original->reference(),
								 &captured) !=
				    player_snapshot_codec_result::ok ||
			    quest_mobile_native_reference_encode(payload.native_mobile.reference,
								 &supplied) !=
				    player_snapshot_codec_result::ok ||
			    captured != supplied)
				return error::payload_conflict;
			// Failed consumed prefixes retain their real ITEM-pass/TYPE-pass
			// order even without a reward continuation. Original notifications
			// are immutable decision facts, never today's completion table.
			const auto &terms = payload.native_recovery.publication_terms;
			const auto &captured_terms = original->publication_terms();
			if (payload.native_recovery.consumed_root_order !=
				    original->consumed_root_order() ||
			    terms.message != captured_terms.message ||
			    terms.disappear_message != captured_terms.disappear_message ||
			    terms.echo_all != captured_terms.echo_all ||
			    terms.disappear != captured_terms.disappear)
				return error::payload_conflict;
			// Rebuild the original structural command. The recovery extension
			// may add only its acknowledged player hold/fence, not new quest
			// selection, revisions, ordered roots, continuation, or operation.
			auto selected = payload;
			selected.native_recovery = {};
			critical_command structural = {};
			auto captured_command = original->original_command();
			if (captured_command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
			    (captured_command.payload_version !=
				     ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION &&
			     captured_command.payload_version !=
				     ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION) ||
			    captured_command.accepted_at_usec ||
			    captured_command.publication_required ||
			    !captured_command.accounting_intent.empty())
				return error::unauthorized;
			if (!item_transfer_command_build_native_mobile(
				    &structural, command->operation_id, selected,
				    command->source_site, command->deadline_class))
				return error::payload_conflict;
			// The wire comparator requires accepted envelopes. Normalize only
			// these local copies after proving the capture was unaccepted.
			structural.accepted_at_usec = 1;
			captured_command.accepted_at_usec = 1;
			if (!critical_command_equal(structural, captured_command))
				return error::payload_conflict;
			event = &original->source_event();
			if (!economic_source_event_valid(*event) ||
			    (event->kind != economic_source_kind::quest_action &&
			     event->kind != economic_source_kind::quest_completion) ||
			    (payload.native_cost.present ?
				     event->kind != economic_source_kind::quest_action :
				     (payload.continuation.kind ==
				      item_transfer_continuation_kind::none) !=
					     (event->kind == economic_source_kind::quest_action)))
				return error::unauthorized;
		}

		auto envelope = *command;
		if (!envelope.accepted_at_usec)
			envelope.accepted_at_usec = 1;
		if (!critical_command_envelope_valid(envelope))
			return error::corrupt_evidence;
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			economic_frozen_intent retained;
			auto status = economic_intent_decode(command->accounting_intent, &retained);
			if (status != error::ok)
				return status;
			status = economic_intent_verify_binding(envelope, retained);
			if (status != error::ok)
				return status;
			// Only this temporary projection becomes unaccepted. Retained
			// command bytes, acceptance time, epoch and source stay untouched.
			auto projection = *command;
			projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			projection.accounting_intent.clear();
			projection.accepted_at_usec = 0;
			projection.publication_required = false;
			std::vector<uint8_t> rebuilt;
			status = item_native_mobile_accounting_intent(
				projection, retained.admission.metadata.lineage,
				retained.admission.metadata.epoch, final_giver_pid, event,
				&rebuilt);
			return status != error::ok		     ? status :
			       rebuilt == command->accounting_intent ? error::ok :
								       error::payload_conflict;
		}

		if (!selected || command->schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
		    !command->accounting_intent.empty() || command->accepted_at_usec ||
		    command->publication_required)
			return error::unauthorized;
		if (selected->wallets.find(final_giver_pid) == selected->wallets.end())
			return error::incomplete_coverage;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		auto status = item_native_mobile_accounting_intent(frozen, selected->lineage,
								   selected->epoch, final_giver_pid,
								   event, &intent);
		if (status != error::ok)
			return status;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(intent);
		envelope = frozen;
		envelope.accepted_at_usec = 1;
		economic_frozen_intent verified;
		status = economic_intent_decode(frozen.accounting_intent, &verified);
		if (status != error::ok)
			return status;
		status = economic_intent_verify_binding(envelope, verified);
		if (status != error::ok)
			return status;
		*command = std::move(frozen);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool economic_gameplay_authority::observe_native_money_checkpoint(
	uint32_t pid, const quest_mobile_native_cash_reference &native,
	economic_native_money_checkpoint_projection *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX || !persistence_mode_requires_mysql() ||
	    !native.wallet_mapping_id || !native.cash_revision ||
	    critical_operation_id_is_zero(native.lineage) ||
	    critical_operation_id_is_zero(native.birth_epoch) ||
	    !quest_mobile_native_reference_valid(native.reference))
		return false;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version || selected->lineage.bytes != native.lineage.bytes)
			return false;
		const auto wallet = selected->wallets.find(pid);
		if (wallet == selected->wallets.end() ||
		    !mapping_valid(wallet->second, economic_account_kind::wallet, selected->lineage,
				   0) ||
		    wallet->second.authority_id == native.wallet_mapping_id)
			return false;
		*output = { selected->lineage, selected->epoch, wallet->second };
		return true;
	}
	catch (...)
	{
		return false;
	}
}

economic_accounting_error economic_gameplay_authority::prepare_native_money_transfer(
	critical_command *command, uint32_t pid,
	const quest_mobile_native_cash_reference *original_before) noexcept
{
	using error = economic_accounting_error;
	if (!command || !pid || pid > INT32_MAX)
		return error::invalid_identity;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (selected && sql_recovery_scope(*selected) &&
		    (command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		     !command->accepted_at_usec))
			return error::unauthorized;
		if (selected && sql_wallet_root_scope(*selected))
			return error::unauthorized;
		if (command->type != critical_command_type::item_transfer ||
		    command->payload_version !=
			    ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION)
			return error::unauthorized;
		item_transfer_payload payload{};
		if (!item_transfer_command_decode_payload(*command, &payload) ||
		    !payload.native_money.present ||
		    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    payload.native_mobile.final_giver_pid != pid)
			return error::corrupt_evidence;
		auto envelope = *command;
		if (!envelope.accepted_at_usec)
			envelope.accepted_at_usec = 1;
		if (!critical_command_envelope_valid(envelope))
			return error::corrupt_evidence;
		if (command->schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			// Pure exact-ID replay retains its original epoch/mappings. Original
			// SQL/current native/held guards still authenticate the real money cut.
			economic_frozen_intent retained;
			auto status = economic_intent_decode(command->accounting_intent, &retained);
			if (status != error::ok)
				return status;
			status = economic_intent_verify_binding(envelope, retained);
			if (status != error::ok || retained.admission.metadata.source_event)
				return status != error::ok ? status : error::unauthorized;
			auto structural = *command;
			structural.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			structural.accounting_intent.clear();
			structural.accepted_at_usec = 0;
			structural.publication_required = false;
			std::vector<uint8_t> rebuilt;
			status = item_native_mobile_accounting_intent(
				structural, retained.admission.metadata.lineage,
				retained.admission.metadata.epoch, pid, nullptr, &rebuilt);
			return status != error::ok		     ? status :
			       rebuilt == command->accounting_intent ? error::ok :
								       error::payload_conflict;
		}
		if (!original_before ||
		    command->schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
		    !command->accounting_intent.empty() || command->accepted_at_usec ||
		    command->publication_required)
			return error::unauthorized;
		economic_native_money_checkpoint_projection admission;
		if (!observe_native_money_checkpoint(pid, *original_before, &admission))
			return error::incomplete_coverage;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> before{}, frozen_ref{};
		if (quest_mobile_native_reference_encode(original_before->reference, &before) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(payload.native_mobile.reference,
							 &frozen_ref) !=
			    player_snapshot_codec_result::ok ||
		    before != frozen_ref ||
		    payload.native_money.player_wallet_mapping_id !=
			    admission.player_wallet.authority_id ||
		    payload.native_money.mobile_wallet_mapping_id !=
			    original_before->wallet_mapping_id ||
		    payload.native_money.projection.mobile_before_revision !=
			    original_before->cash_revision ||
		    payload.native_money.projection.mobile_before != original_before->denominations)
			return error::payload_conflict;
		critical_command frozen = *command;
		std::vector<uint8_t> intent;
		auto status = item_native_mobile_accounting_intent(
			frozen, admission.lineage, admission.epoch, pid, nullptr, &intent);
		if (status != error::ok)
			return status;
		frozen.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		frozen.accounting_intent = std::move(intent);
		envelope = frozen;
		envelope.accepted_at_usec = 1;
		economic_frozen_intent verified;
		status = economic_intent_decode(frozen.accounting_intent, &verified);
		if (status != error::ok)
			return status;
		status = economic_intent_verify_binding(envelope, verified);
		if (status != error::ok)
			return status;
		*command = std::move(frozen);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error
economic_gameplay_authority::prepare_native_mobile_birth_shared_shopkeeper_flat(
	const quest_mobile_native_image &original,
	std::span<const native_mobile_birth_item_recipe> recipes,
	const native_mobile_birth_cash_role_recipe &role, critical_source_site original_site,
	uint64_t accepted_at_usec, critical_command *output) noexcept
{
	using error = economic_accounting_error;
	const char *root = persistence_mode_flatfile_root();
	if (!output || persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    persistence_mode_requires_mysql() || !root || !*root ||
	    role.role != native_mobile_birth_cash_role::shared_shopkeeper)
		return error::unauthorized;
	try
	{
		const auto selected = current.load(std::memory_order_acquire);
		if (!selected || selected->scope != projection_scope::regular ||
		    selected->scope_version != 0)
			return error::unauthorized;
		economic_operation_metadata metadata{};
		metadata.operation_id = original.reference.birth_operation;
		metadata.lineage = selected->lineage;
		metadata.epoch = selected->epoch;
		metadata.actor_kind = economic_actor_kind::domain;
		metadata.actor_id = original.reference.mobile_instance_id;
		metadata.writer_id = ECONOMIC_WRITER_NATIVE_MOBILE_BIRTH;
		metadata.reason = economic_reason::npc_reward;
		metadata.policy_version = 1;
		metadata.compiler_version = 1;
		metadata.source_event = original.reference.birth_source;
		// Freeze the actual retained image exactly once. Replay decodes its
		// original command and never calls this current-projection preparation.
		return native_mobile_birth_cash_role_command_build(
			metadata, original, recipes, role, original_site, accepted_at_usec, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}
