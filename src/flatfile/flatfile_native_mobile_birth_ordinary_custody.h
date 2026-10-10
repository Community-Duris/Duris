#ifndef DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_CUSTODY_H
#define DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_CUSTODY_H

#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_native_mobile_wallet.h"
#include "economy/native_mobile_birth_recovery.h"

// Passive actual catalog proposal; only the genuine atomic transaction may
// bind this with real native/mapping/financial/source/ledger participants.
struct flatfile_native_mobile_birth_ordinary_custody_stage
{
	bool catalog_before_present = false;
	uint64_t catalog_revision_before = 0, catalog_revision_after = 0;
	uint64_t owner_revision_after = 0;
	economic_accounting_plan plan;
	flatfile_authority_operation operation;
};

struct flatfile_native_mobile_birth_ordinary_current_custody
{
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> rows;
};

class flatfile_native_mobile_birth_ordinary_custody_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	// Caller supplies the actual ORIGINAL INITIAL and mapping from its genuine
	// same-root closed allocator stage, never a UID-derived or fabricated key.
	// Actual source/factory/native/financial admission and physical all-domain
	// absence remain caller-owned. Complete catalog/history is observed here.
	// Always creates native/context0 owner revision1, including zero inventory.
	// Strong local stage only: no recover/acquire/commit/world/publication/ACK.
	static flatfile_item_repository_result
	prepare_locked(const std::string &, const flatfile_authority_lock &,
		       const critical_native_recovery_envelope &, const flatfile_economic_mapping &,
		       flatfile_native_mobile_birth_ordinary_custody_stage *,
		       std::string *) noexcept;
	// Transaction first authenticates the actual immutable financial receipt
	// through the genuine closed owner. This sibling rechecks canonical full
	// original EAP1/MBR4, attached receipt core and whole CURRENT custody catalog.
	// Does not authenticate source/global claim count, native body, independent
	// creation ledger, immutable terminal origin or producer/world/ACK lifetime.
	// Strong output; original allocating codecs, no retained-memory bound claim.
	static flatfile_item_repository_result
	read_locked(const std::string &, const flatfile_authority_lock &,
		    const critical_native_recovery_envelope &, const flatfile_accounting_record &,
		    flatfile_native_mobile_birth_ordinary_current_custody *,
		    std::string *) noexcept;
};

#endif
