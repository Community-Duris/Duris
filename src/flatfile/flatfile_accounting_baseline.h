#ifndef DURIS_FLATFILE_ACCOUNTING_BASELINE_H
#define DURIS_FLATFILE_ACCOUNTING_BASELINE_H
#include "flatfile/flatfile_accounting_store.h"
#include "economy/economic_baseline_command.h"

constexpr size_t FLATFILE_BASELINE_BUCKETS = 16;
constexpr size_t FLATFILE_BASELINE_BUCKET_RECORDS = 65536;
constexpr size_t FLATFILE_BASELINE_INDEX_MAX_BYTES =
	48 + 40 + 32 * FLATFILE_BASELINE_BUCKET_RECORDS;
constexpr size_t FLATFILE_BASELINE_STAGE_MAX_OPERATIONS = FLATFILE_BASELINE_BUCKETS + 4;
static_assert(FLATFILE_BASELINE_STAGE_MAX_OPERATIONS <=
	      flatfile_authority_transaction_maximum_operations);
// Returns the original common receipt only after regenerating its bound plan
// from the complete retained witness and verifying opening reservations. This
// may recover the shared journal; it never executes a native mutation. Outputs
// remain unchanged on failure. Missing/corrupt books or witnesses fail closed.
flatfile_accounting_status
flatfile_accounting_baseline_lookup(const std::string &, const flatfile_authority_lock &,
				    const critical_command &, flatfile_accounting_record *,
				    std::vector<uint8_t> *witness, std::string *error);

// Private storage seam, NOT cutover authority. Only a lifecycle owner may stage
// these with verified native sources, frozen coverage and lifecycle receipts.
// No production caller exists yet. All operations use the existing shared
// journal; no native balances/custody are changed here. Keep account lifetimes
// and item UIDs unique within the epoch, including across preparation IDs.
class flatfile_accounting_baseline_storage
{
	friend class flatfile_accounting_lifecycle_transaction;
#ifdef DURIS_FLATFILE_ACCOUNTING_TEST
	friend class flatfile_accounting_test_access;
#endif
	// Requires retained lineage/epoch membership and no files in this epoch's
	// baseline namespace. Caller must prove never initialized; absence alone
	// cannot establish that fact after an incomplete restore or deletion.
	static flatfile_accounting_status
	initialize(const std::string &, const flatfile_authority_lock &,
		   const critical_operation_id &lineage, const critical_operation_id &epoch,
		   const economic_account_key &opening,
		   const critical_operation_id &creating_operation,
		   std::vector<flatfile_authority_operation> *, std::string *);
	// Append witness, exact receipt, reservation buckets and book head together.
	// Refuses duplicate account/UID openings, conflicting command IDs, orphan
	// witnesses and missing/corrupt indexes. An exact retained retry returns
	// already_exists without appending anything. Outputs unchanged on error.
	static flatfile_accounting_status
	stage(const std::string &, const flatfile_authority_lock &, const critical_command &,
	      const economic_prepared_baseline &, std::vector<flatfile_authority_operation> *,
	      std::string *);
};
#endif
