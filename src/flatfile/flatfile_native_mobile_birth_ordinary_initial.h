#ifndef DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_INITIAL_H
#define DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_INITIAL_H

#include "flatfile/flatfile_accounting_authority.h"
#include "economy/native_mobile_birth_recovery.h"

// Values and proposed after-images only. This is not an allocated/published
// wallet lifetime, receipt, source, native-body, custody or execution permit.
struct flatfile_native_mobile_birth_ordinary_initial_stage
{
	flatfile_economic_mapping mapping;
	uint64_t lineage_revision_before = 0, lineage_revision_after = 0;
	std::vector<flatfile_authority_operation> operations;
};

class flatfile_native_mobile_birth_ordinary_initial_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	// The genuine future ordinary participant supplies the full ORIGINAL INITIAL
	// carrier and SAME already-recovered root lock/lifecycle exclusion. This leaf
	// authenticates canonical ordinary structure and installed selected authority,
	// full historical native/creator mapping absence and real allocator images.
	// Caller separately proves actual sourceclaim/native/custody/physical absence,
	// then binds mapping+native+custody+financial evidence in one original bundle.
	// No recovery/acquire/commit, identity issue, world/publication/ACK or readiness.
	// Strong complete output. Original allocating codecs, no memory-bound claim.
	static unsigned int prepare_locked(const std::string &, const flatfile_authority_lock &,
					   const critical_native_recovery_envelope &,
					   uint64_t expected_control_revision,
					   flatfile_native_mobile_birth_ordinary_initial_stage *,
					   std::string *) noexcept;
};

#endif
