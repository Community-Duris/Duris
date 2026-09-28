#ifndef DURIS_FLATFILE_ACCOUNTING_LIFECYCLE_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_LIFECYCLE_TRANSACTION_H

#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_baseline.h"
#include "economy/economic_accounting_types.h"
#include "economy/economic_baseline_command.h"
#include <string>
#include <vector>

class flatfile_identity_lock;

struct flatfile_accounting_lifecycle_wallet_source
{
	uint32_t pid = 0;
	std::string account_name;
	uint8_t racewar = 0;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	economic_digest source_digest = {};
};

struct flatfile_accounting_lifecycle_bank_source
{
	std::string name;
	uint8_t racewar = 0;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	economic_digest source_digest = {};
};

struct flatfile_accounting_lifecycle_native_sources
{
	std::vector<flatfile_accounting_lifecycle_wallet_source> wallets;
	std::vector<flatfile_accounting_lifecycle_bank_source> banks;
};

struct flatfile_accounting_lifecycle_request
{
	critical_operation_id operation_id = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	uint64_t actor_id = 0;
	uint64_t accepted_at_usec = 0;
	economic_digest coverage_digest = {};
	// External proof that the system is in a virgin_state / never_activated state.
	bool virgin_state_proven = false;
};

struct flatfile_accounting_lifecycle_receipt
{
	critical_operation_id operation_id = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	critical_operation_id baseline_operation_id = {};
	economic_digest coverage_digest = {};
	uint64_t baseline_revision = 0;
	std::vector<flatfile_economic_mapping> mappings;
};

// Private flatfile accounting lifecycle owner.
// Coordinates complete native capture of wallets and shared banks, enforces
// one-to-one mapping and lifetime reconciliation, stages the baseline witness
// and reservations, verifies virgin_state (never_activated) proof, and commits
// the lifecycle receipt before selecting the active epoch.
class flatfile_accounting_lifecycle_transaction
{
    public:
	static unsigned int capture_native_sources_locked(
		const std::string &root, const flatfile_identity_lock &identity_lock,
		const flatfile_authority_lock &authority_lock,
		flatfile_accounting_lifecycle_native_sources *sources, std::string *error) noexcept;
	static unsigned int install(const std::string &root,
				    const flatfile_identity_lock &identity_lock,
				    const flatfile_authority_lock &lock,
				    const flatfile_accounting_lifecycle_request &request,
				    const economic_account_key &opening_account,
				    flatfile_accounting_lifecycle_receipt *receipt,
				    std::string *error) noexcept;
};

#endif
