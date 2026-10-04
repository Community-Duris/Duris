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
	// Externally asserted frozen boundary, independently established by the
	// future native cutover producer. Setting these fields is not proof and
	// does not replace complete writer/command census and native exclusion.
	economic_digest boundary_digest = {};
	bool frozen_boundary_proven = false;
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
	economic_digest boundary_digest = {};
	uint64_t baseline_revision = 0;
	std::vector<flatfile_economic_mapping> mappings;
};

// Private flatfile accounting lifecycle owner.
// Coordinates complete native capture of wallets and shared banks, enforces
// one-to-one mapping and lifetime reconciliation, stages the baseline witness
// and reservations, verifies virgin_state (never_activated) proof, and commits
// immutable lifecycle receipt and epoch selection in one authority bundle.
// Exact original-ID retry verifies retained baseline/reservations and historical
// epoch links before EALREADY or native capture, returning the original ordered
// mappings without consulting mutable mapping rows. Changed requests conflict;
// Original native descriptors bind locator/PID/order to retained baseline source
// fingerprints and coverage. Mapping revision/operation metadata is authority of
// the immutable lifecycle frame, not independently rebuilt from later mappings.
// missing/corrupt completed history refuses. Shared authenticated journal recovery
// may finish a previously committed original bundle on retry; no new images,
// native capture/mutation, mapping changes or epoch selection are prepared.
// The common receipt bucket must already be initialized for a fresh install.
// Caller receipt stays unchanged on failure, including allocation/I/O failure.
// Frozen-boundary authority is an external prerequisite; no authenticated native
// boundary producer or production activation wiring is supplied by this owner.
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
