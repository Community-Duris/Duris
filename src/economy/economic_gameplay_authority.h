#ifndef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_H
#define DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_H

#include "economy/economic_currency_adapter.h"
#include "economy/collector_codec.h"
#include <span>
#include <string>
#include <string_view>

class quest_native_consumption_capture;

struct quest_mobile_native_image;
struct quest_mobile_native_cash_reference;
class item_native_quest_preparation_owner;
struct native_mobile_birth_item_recipe;
struct quest_mobile_native_constructor_recipe;

struct economic_gameplay_wallet_mapping
{
	uint32_t pid = 0;
	economic_account_key account;
};

struct economic_gameplay_bank_mapping
{
	std::string name;
	uint8_t racewar = 0;
	economic_account_key account;
};

// Original read-only facts for the native shop checkpoint. These values grant
// no storage or mutation authority; the writer locks their native mappings.
// Pure original finite-wallet mapping values; SQL locks/authenticates the actual
// PID/native mapping lifetimes. No mint, source claim or publication capability.
struct economic_native_money_checkpoint_projection
{
	critical_operation_id lineage{}, epoch{};
	economic_account_key player_wallet;
};

struct economic_shop_checkpoint_projection
{
	critical_operation_id lineage, epoch;
	economic_account_key wallet, bank;
};

// Read-only admission projection, never balance or storage authority. Only a
// receipt-bearing native lifecycle owner may install it, after verifying native
// coverage and the selected epoch under its quiesced boundary. Execution still
// locks and verifies the retained mappings and exact frozen intent.
class economic_gameplay_authority
{
    public:
	// Pure builders use no I/O or game pointers. Legacy mode leaves exact bytes unchanged.
	// Active mode freezes supported typed commands; unsupported writers refuse,
	// never fall back to schema 1. Already frozen commands are verified, not
	// rebound to the current epoch (retained exact-ID replay may be historical).
	static economic_accounting_error prepare_currency(critical_command *command);
	// Fresh auction capture borrows the existing native SQL session owner.
	// Retained commands are validated without current authority or recapture.
	static economic_accounting_error prepare_auction(critical_command *command);
	static economic_accounting_error prepare_coin_transfer(critical_command *command);
	static economic_accounting_error
	prepare_collector_purchase(critical_command *command,
				   const collector::record &original_listing);
	// v6 source status/item checkpoint must be established by the native producer.
	// Pure resolution only: no coordinator admission, native mutation or ACK.
	static economic_accounting_error prepare_shop_trade(critical_command *command);
	static economic_accounting_error
	prepare_item_transfer(critical_command *command, uint32_t actor_pid,
			      economic_source_kind lifecycle_source = {});
	// Explicit v12 preparation only; generic/native11 predicates stay closed.
	// Consumption requires the original quest owner's sealed ordered decision.
	// Retained replay verifies original intent without selecting today's epoch.
	static economic_accounting_error
	prepare_native_item_transfer(critical_command *command, uint32_t final_giver_pid,
				     const quest_native_consumption_capture *original);
	static bool active();
	// Existing native SQL scope only; excludes wallet-root qualification and flat.
	// This observes admission state and does not grant native mutation authority.
	static bool active_regular_sql();
	// Selected SQL boot policy keeps legacy writers closed while genuine replay
	// and published lifetimes recover. It is not fresh admission/readiness.
	static bool active_sql_recovery();
	static bool observe_shop_checkpoint(uint32_t pid, std::string_view account_name,
					    uint8_t racewar,
					    economic_shop_checkpoint_projection *output) noexcept;

    private:
	friend class item_native_quest_preparation_owner;
	static bool
	observe_native_money_checkpoint(uint32_t final_giver_pid,
					const quest_mobile_native_cash_reference &,
					economic_native_money_checkpoint_projection *) noexcept;
	static economic_accounting_error prepare_native_money_transfer(
		critical_command *, uint32_t final_giver_pid,
		const quest_mobile_native_cash_reference *original_before) noexcept;
	// Only the actual original birth owner may freeze fresh native birth facts.
	// The installed regular SQL projection supplies lineage/epoch, not provenance.
	// This does not reserve identities, admit a command or authorize publication.
	friend class quest_mobile_native_birth_owner;
	static economic_accounting_error
	prepare_native_mobile_birth(const quest_mobile_native_image &, critical_source_site,
				    uint64_t accepted_at_usec, critical_command *output) noexcept;
	static economic_accounting_error prepare_native_mobile_birth(
		const quest_mobile_native_image &, std::span<const native_mobile_birth_item_recipe>,
		critical_source_site, uint64_t accepted_at_usec, critical_command *output) noexcept;
	static economic_accounting_error prepare_native_mobile_birth(
		const quest_mobile_native_image &, std::span<const native_mobile_birth_item_recipe>,
		const quest_mobile_native_constructor_recipe &, critical_source_site,
		uint64_t accepted_at_usec, critical_command *output) noexcept;

	friend class economic_sql_accounting_lifecycle_transaction;

	friend class flatfile_accounting_lifecycle_transaction;
	friend bool sql_economic_runtime_start() noexcept;
	friend void sql_economic_runtime_shutdown() noexcept;
	friend void flatfile_economic_runtime_shutdown() noexcept;
#ifdef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
	friend class economic_gameplay_authority_test_access;
#endif
	// Receipt identity is retained for provenance; its presence is not proof.
	// The private native caller owns receipt verification, complete enumeration,
	// coverage admission and safe publication. No operator/config setter exists.
	static economic_accounting_error
	install(const critical_operation_id &lineage, const critical_operation_id &epoch,
		const critical_operation_id &receipt,
		std::span<const economic_gameplay_wallet_mapping> wallets,
		std::span<const economic_gameplay_bank_mapping> banks);
	// This token keeps the qualification installer unavailable even to the
	// unrelated flatfile lifecycle friend. Only the SQL lifecycle owner and the
	// explicitly unit-only test seam can construct it.
	class sql_wallet_root_qualification_install_key
	{
	    private:
		sql_wallet_root_qualification_install_key() = default;
		friend class economic_sql_accounting_lifecycle_transaction;
#ifdef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
		friend class economic_gameplay_authority_test_access;
#endif
	};
	static economic_accounting_error install_sql_wallet_root_qualification(
		sql_wallet_root_qualification_install_key, const critical_operation_id &lineage,
		const critical_operation_id &epoch, const critical_operation_id &receipt,
		std::span<const economic_gameplay_wallet_mapping> wallets,
		std::span<const economic_gameplay_bank_mapping> banks);
	// Only the authentic SQL lifecycle caller can issue this private key after
	// installation/activation/baseline/opening and original-session proof.
	class sql_runtime_recovery_install_key
	{
		sql_runtime_recovery_install_key() = default;
		friend class economic_sql_accounting_lifecycle_transaction;
	};
	static economic_accounting_error install_sql_recovery(sql_runtime_recovery_install_key,
							      const critical_operation_id &lineage,
							      const critical_operation_id &epoch,
							      const critical_operation_id &receipt);
	static bool sql_recovery_selection_matches(sql_runtime_recovery_install_key,
						   const critical_operation_id &lineage,
						   const critical_operation_id &epoch,
						   const critical_operation_id &receipt) noexcept;
	static economic_accounting_error
	finish_sql_recovery(sql_runtime_recovery_install_key, const critical_operation_id &lineage,
			    const critical_operation_id &epoch,
			    const critical_operation_id &receipt,
			    std::span<const economic_gameplay_wallet_mapping> wallets,
			    std::span<const economic_gameplay_bank_mapping> banks);
	static void clear_sql_runtime() noexcept;
	// Trusted flat runtime shutdown only; never clears the SQL qualification scope.
	static void clear_flat_runtime() noexcept;
#ifdef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
	static void reset_for_tests();
#endif
};

#endif
