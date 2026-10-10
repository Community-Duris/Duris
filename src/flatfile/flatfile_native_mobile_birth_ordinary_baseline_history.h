#ifndef DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_BASELINE_HISTORY_H
#define DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_BASELINE_HISTORY_H

#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_baseline.h"
#include "economy/native_mobile_birth_recovery.h"

// Unsealed actual retained metadata. These values are neither write/activation
// authority nor proof of CURRENT native custody. The private same-root join
// reads them from original storage; arbitrary caller DTOs are not accepted.
struct flatfile_native_mobile_birth_ordinary_retained_metadata
{
	flatfile_economic_control control;
	std::vector<flatfile_economic_epoch> epochs;
};

// Informational counts after the complete retained baseline UID exclusion.
// Retired epochs participate, and unknown initialization never means empty.
struct flatfile_native_mobile_birth_ordinary_baseline_absence
{
	critical_operation_id lineage = {};
	uint64_t lineage_revision = 0;
	economic_digest epochs_digest = {};
	uint32_t epochs_verified = 0, initialized_books = 0, legacy_books = 0,
		 never_initialized_namespaces = 0, indexes_verified = 0;
	uint64_t reservations_verified = 0, item_reservations_verified = 0;
	size_t born_uids = 0;
};

class flatfile_native_mobile_birth_ordinary_baseline_history_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	friend class flatfile_ordinary_native_birth_receipt_storage;
	// Full actual control including all initialized bits, plus the original
	// complete v1/v2/v3 retained catalog/digest/count/chain validation. Caller
	// resolves journals before this SAME borrowed lock; no recover or acquire.
	// Strong output; informational values cannot authorize a later mutation.
	static unsigned int
	read_metadata_locked(const std::string &, const flatfile_authority_lock &,
			     flatfile_native_mobile_birth_ordinary_retained_metadata *,
			     std::string *);

	// Full original ordinary INITIAL envelope/intent and genuine born UID set.
	// Independently reads actual metadata, every retained book/all16 canonical
	// reservation indexes, including retired epochs and kind2 UID history.
	// never_initialized requires original whole-prefix empty-namespace proof;
	// legacy_unknown requires a real structurally valid book and all indexes.
	// No recovery/acquire/stage/write/commit/world/ACK/activation selection.
	// Original allocating decoders: no memory bound or native qualification.
	// Strong counts output, never a write/financial/physical permit.
	static flatfile_accounting_status
	verify_initial_absence_locked(const std::string &, const flatfile_authority_lock &,
				      const critical_native_recovery_envelope &,
				      flatfile_native_mobile_birth_ordinary_baseline_absence *,
				      std::string *);
	// Full original retained catalog, passive same-root lock, strong transfer.
	static unsigned int
	read_metadata_locked_bounded(const std::string &, const flatfile_authority_lock &,
				     flatfile_native_mobile_birth_ordinary_retained_metadata *,
				     flatfile_scratch_reserve_fn, void *, size_t,
				     size_t *retained_output_payload_bytes = nullptr) noexcept;
};

#endif
