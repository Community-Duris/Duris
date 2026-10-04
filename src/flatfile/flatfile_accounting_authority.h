#ifndef DURIS_FLATFILE_ACCOUNTING_AUTHORITY_H
#define DURIS_FLATFILE_ACCOUNTING_AUTHORITY_H
#include "flatfile/flatfile_accounting_store.h"
#include <array>
#include <span>

constexpr size_t FLATFILE_ECONOMIC_METADATA_BUCKETS = 256;
constexpr size_t FLATFILE_ECONOMIC_BUCKET_MAPPINGS = 4096;
constexpr uint64_t FLATFILE_ECONOMIC_MAX_MAPPINGS = 256 * 4096;
constexpr size_t FLATFILE_ECONOMIC_MAX_EPOCHS = 4096;
constexpr size_t FLATFILE_ECONOMIC_METADATA_MAX_BYTES = 2 * 1024 * 1024;
// A name locates the current bank; only its allocated lifetime identifies it.
struct flatfile_economic_locator
{
	// 1: wallet PID; 2: bank name; 4: auction ID; 5: claim PID;
	// 6: shopkeeper owner ID (shop ID + 1, including shop ID zero).
	uint16_t kind = 0;
	uint64_t native_id = 0;
	std::string name;
};
struct flatfile_economic_mapping
{
	economic_account_key account;
	flatfile_economic_locator locator;
	critical_operation_id creating_operation = {}, retiring_operation = {}, last_operation = {};
	uint64_t revision = 0;
};
enum class flatfile_baseline_initialization : uint8_t
{
	legacy_unknown = 0,
	never_initialized = 1,
	initialized = 2,
};
struct flatfile_economic_epoch
{
	critical_operation_id epoch = {}, predecessor = {}, creating_operation = {};
	uint64_t ordinal = 0;
	uint16_t transition_kind = 0;
	economic_digest transition_digest = {};
	// Catalog v1 decodes as unknown. Only native append proves never initialized.
	flatfile_baseline_initialization baseline_initialization =
		flatfile_baseline_initialization::legacy_unknown;
	critical_operation_id baseline_initializing_operation = {};
	economic_account_key baseline_opening = {};
};
struct flatfile_economic_control
{
	critical_operation_id lineage = {}, creating_operation = {}, last_operation = {};
	critical_operation_id last_epoch = {}, active_epoch = {};
	uint64_t revision = 0, next_mapping_id = 1;
	uint32_t epoch_count = 0;
	economic_digest epochs_digest = {};
	std::array<economic_digest, 256> native_digests = {}, mapping_digests = {};
	std::array<uint8_t, 32> evidence_initialized = {};
};
struct flatfile_economic_mapping_request
{
	economic_account_key account;
	flatfile_economic_locator locator;
};
struct flatfile_economic_authority_snapshot
{
	critical_operation_id lineage = {}, epoch = {};
	uint64_t lineage_revision = 0;
	std::vector<flatfile_economic_mapping> mappings;
};
// Borrow the shared lock; recover first; preserve outputs on error. errno-style
// errors distinguish absent/inactive ENODATA, stale ESTALE, invalid input EINVAL,
// corrupt state EILSEQ, capacity ENOSPC/EOVERFLOW and I/O/allocation failure.
// Snapshots are values, never capabilities to write financial evidence.
unsigned int flatfile_economic_control_read(const std::string &, const flatfile_authority_lock &,
					    flatfile_economic_control *, std::string *);
// Call under the domain's authority lock after checking an exact replay and
// before preparing any new legacy mutation. An absent accounting store is an
// inactive installation; a present but corrupt store must fail closed.
unsigned int flatfile_economic_legacy_domain_gate(const std::string &,
						  const flatfile_authority_lock &, std::string *);
// Retained membership only: historical epochs remain readable after selection changes.
unsigned int flatfile_economic_epoch_read(const std::string &, const flatfile_authority_lock &,
					  const critical_operation_id &lineage,
					  const critical_operation_id &epoch,
					  flatfile_economic_epoch *, std::string *);
unsigned int flatfile_economic_mapping_read(const std::string &, const flatfile_authority_lock &,
					    const economic_account_key &,
					    flatfile_economic_mapping *, std::string *);
unsigned int flatfile_economic_native_lookup(const std::string &, const flatfile_authority_lock &,
					     economic_account_kind, uint64_t context,
					     const flatfile_economic_locator &,
					     flatfile_economic_mapping *, std::string *);
unsigned int economic_flatfile_lock_authority(const std::string &, const flatfile_authority_lock &,
					      const critical_operation_id &lineage,
					      const critical_operation_id &epoch,
					      std::span<const flatfile_economic_mapping_request>,
					      flatfile_economic_authority_snapshot *,
					      std::string *);
// The private lifecycle owner must supply proven native effects and an operation
// receipt in the same bundle. Helpers never publish or authorize creation,
// baseline or activation. Bootstrap additionally requires external durable proof
// of never-activated state; an empty directory alone is NOT that proof.
class flatfile_accounting_staging_view;
class flatfile_accounting_authority_storage
{
	friend class flatfile_accounting_lifecycle_transaction;
	friend class flatfile_accounting_baseline_storage;
	friend class flatfile_accounting_staging_view;
	friend class flatfile_accounting_auction_item_claim_transaction;
#ifdef DURIS_FLATFILE_ACCOUNTING_TEST
	friend class flatfile_accounting_test_access;
#endif
	using operations = std::vector<flatfile_authority_operation>;
	static unsigned int read_control(const std::string &, const flatfile_authority_lock &,
					 const flatfile_accounting_staging_view *,
					 flatfile_economic_control *, std::string *);
	static unsigned int read_epoch(const std::string &, const flatfile_authority_lock &,
				       const flatfile_accounting_staging_view *,
				       const critical_operation_id &lineage,
				       const critical_operation_id &epoch,
				       flatfile_economic_epoch *, std::string *);
	static unsigned int bootstrap(const std::string &, const flatfile_authority_lock &,
				      const critical_operation_id &lineage,
				      const critical_operation_id &operation, operations *,
				      std::string *);
	static unsigned int initialize_native_bucket(const std::string &,
						     const flatfile_authority_lock &,
						     uint64_t expected_revision, size_t bucket,
						     const critical_operation_id &operation,
						     operations *, std::string *);
	static unsigned int initialize_evidence_bucket(const std::string &,
						       const flatfile_authority_lock &,
						       uint64_t expected_revision, size_t bucket,
						       const critical_operation_id &operation,
						       operations *, std::string *);
	static unsigned int create_mapping(const std::string &, const flatfile_authority_lock &,
					   uint64_t expected_revision, economic_account_kind,
					   uint64_t context, const flatfile_economic_locator &,
					   const critical_operation_id &operation,
					   flatfile_economic_mapping *, operations *,
					   std::string *);
	static unsigned int
	create_mapping_staged(const std::string &, const flatfile_authority_lock &, uint64_t,
			      economic_account_kind, uint64_t, const flatfile_economic_locator &,
			      const critical_operation_id &, flatfile_economic_mapping *,
			      operations *, std::string *, flatfile_accounting_staging_view *);
	static unsigned int retire_mapping(const std::string &, const flatfile_authority_lock &,
					   uint64_t expected_revision, const economic_account_key &,
					   uint64_t mapping_revision,
					   const critical_operation_id &operation, operations *,
					   std::string *);
	static unsigned int rename_bank(const std::string &, const flatfile_authority_lock &,
					uint64_t expected_revision, const economic_account_key &,
					uint64_t mapping_revision, const std::string &new_name,
					const critical_operation_id &operation, operations *,
					std::string *);
	static unsigned int append_epoch(const std::string &, const flatfile_authority_lock &,
					 uint64_t expected_revision,
					 const flatfile_economic_epoch &, operations *,
					 std::string *);
	static unsigned int append_epoch_staged(const std::string &,
						const flatfile_authority_lock &, uint64_t,
						const flatfile_economic_epoch &, operations *,
						std::string *, flatfile_accounting_staging_view *);
	// Private initialization participant: records native authority proof in the
	// same complete shared-journal bundle as the book head and sixteen indexes.
	static unsigned int stage_baseline_initialization(
		const std::string &, const flatfile_authority_lock &, uint64_t expected_revision,
		const critical_operation_id &lineage, const critical_operation_id &epoch,
		const economic_account_key &opening, const critical_operation_id &operation,
		operations *, std::string *);
	static unsigned int stage_baseline_initialization_staged(
		const std::string &, const flatfile_authority_lock &, uint64_t,
		const critical_operation_id &, const critical_operation_id &,
		const economic_account_key &, const critical_operation_id &, operations *,
		std::string *, flatfile_accounting_staging_view *);
	static unsigned int select_epoch(const std::string &, const flatfile_authority_lock &,
					 uint64_t expected_revision, bool active,
					 const critical_operation_id &operation, operations *,
					 std::string *);
	static unsigned int select_epoch_staged(const std::string &,
						const flatfile_authority_lock &, uint64_t, bool,
						const critical_operation_id &, operations *,
						std::string *, flatfile_accounting_staging_view *);
};
#endif
