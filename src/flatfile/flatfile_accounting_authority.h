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
	uint16_t kind = 0; // 1: wallet PID; 2: bank lifetime and canonical name.
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
struct flatfile_economic_epoch
{
	critical_operation_id epoch = {}, predecessor = {}, creating_operation = {};
	uint64_t ordinal = 0;
	uint16_t transition_kind = 0;
	economic_digest transition_digest = {};
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
class flatfile_accounting_authority_storage
{
	friend class flatfile_accounting_lifecycle_transaction;
#ifdef DURIS_FLATFILE_ACCOUNTING_TEST
	friend class flatfile_accounting_test_access;
#endif
	using operations = std::vector<flatfile_authority_operation>;
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
	static unsigned int select_epoch(const std::string &, const flatfile_authority_lock &,
					 uint64_t expected_revision, bool active,
					 const critical_operation_id &operation, operations *,
					 std::string *);
};
#endif
