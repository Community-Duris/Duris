#ifndef DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_REFERENCE_HISTORY_H
#define DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_REFERENCE_HISTORY_H

#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/native_mobile_birth_recovery.h"

// Unsealed passive counts. No mapping/source/ledger/physical or writer authority
// follows; the actual same-root transaction retains the input and real lock.
struct flatfile_native_mobile_birth_ordinary_reference_absence
{
	uint16_t buckets_verified = 0, missing_buckets = 0;
	uint64_t retained_records = 0;
	size_t born_uids = 0;
};

// Unsealed census of the currently retained original quarantine files only.
// Original repair's overwrite/truncation behavior prevents a completeness
// claim for discarded history. No source/ledger/custody or CURRENT authority.
struct flatfile_native_mobile_birth_ordinary_reference_quarantine_absence
{
	uint16_t buckets_verified = 0, missing_files = 0;
	uint64_t retained_records = 0;
	size_t born_uids = 0, unique_operation_lines = 0, unique_legacy_events = 0;
};

class flatfile_native_mobile_birth_ordinary_reference_history_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	// Complete canonical ordinary INITIAL and ALL 256 original reference files.
	// All histories participate regardless retirement, identity or current state.
	// Caller owns original recovery/namespace establishment before this passive
	// same-root read. Only true missing FILES are empty; directory/security/read
	// errors remain errors. No recover/acquire/repair/commit/world/ACK selection.
	// Strong counts output. Original allocating codecs/sets: no memory bound claim.
	static flatfile_item_accounting_status
	verify_initial_absence_locked(const std::string &, const flatfile_authority_lock &,
				      const critical_native_recovery_envelope &,
				      flatfile_native_mobile_birth_ordinary_reference_absence *,
				      std::string *) noexcept;
	// Passive INITIAL evidence check over all256 exact original .bin.corrupt
	// names at the recovered same-root exclusion cut. True missing FILES are
	// empty; unreadable/security-invalid/undecodable evidence refuses. Canonical
	// reference history is checked separately. This cannot recover discarded
	// history or establish full SQL quarantine parity. No recovery or writes.
	static flatfile_item_accounting_status verify_initial_quarantine_absence_locked(
		const std::string &, const flatfile_authority_lock &,
		const critical_native_recovery_envelope &,
		flatfile_native_mobile_birth_ordinary_reference_quarantine_absence *,
		std::string *) noexcept;
};

#endif
