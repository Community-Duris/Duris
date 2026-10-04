#ifndef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_H
#define DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_H

#include "economy/economic_currency_adapter.h"
#include "economy/collector_codec.h"
#include <span>
#include <string>

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

// Read-only admission projection, never balance or storage authority. Only a
// receipt-bearing native lifecycle owner may install it, after verifying native
// coverage and the selected epoch under its quiesced boundary. Execution still
// locks and verifies the retained mappings and exact frozen intent.
class economic_gameplay_authority
{
    public:
	// No I/O or game pointers. Legacy mode leaves exact command bytes unchanged.
	// Active mode freezes supported typed commands; unsupported writers refuse,
	// never fall back to schema 1. Already frozen commands are verified, not
	// rebound to the current epoch (retained exact-ID replay may be historical).
	static economic_accounting_error prepare_currency(critical_command *command);
	static economic_accounting_error prepare_coin_transfer(critical_command *command);
	static economic_accounting_error
	prepare_collector_purchase(critical_command *command,
				   const collector::record &original_listing);
	static economic_accounting_error
	prepare_item_transfer(critical_command *command, uint32_t actor_pid,
			      economic_source_kind lifecycle_source = {});
	static bool active();

    private:
	friend class economic_sql_accounting_lifecycle_transaction;
	friend class flatfile_accounting_lifecycle_transaction;
	friend bool sql_economic_runtime_start() noexcept;
	friend void sql_economic_runtime_shutdown() noexcept;
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
	static void clear_sql_runtime() noexcept;
#ifdef DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
	static void reset_for_tests();
#endif
};

#endif
