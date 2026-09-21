#ifndef DURIS_FLATFILE_ACCOUNTING_STORE_H
#define DURIS_FLATFILE_ACCOUNTING_STORE_H

#include "flatfile/flatfile_authority_transaction.h"
#include "economy/economic_accounting_intent.h"
#include "persistence/critical_command_completion.h"

constexpr size_t FLATFILE_ACCOUNTING_BUCKETS = 256;
constexpr size_t FLATFILE_ACCOUNTING_BUCKET_RECORDS = 4096;
constexpr uint64_t FLATFILE_ACCOUNTING_BUCKET_MAX_BYTES = uint64_t{ 256 } << 20;
constexpr size_t FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES = 8 * 1024 * 1024;
constexpr size_t FLATFILE_ACCOUNTING_RECORD_MAX_BYTES =
	48 + 26 + CRITICAL_COMMAND_MAX_ENCODED_BYTES + ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES +
	CRITICAL_COMPLETION_RESULT_MAX_BYTES;

static_assert(FLATFILE_ACCOUNTING_RECORD_MAX_BYTES + 80 < FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES);
constexpr size_t FLATFILE_ACCOUNTING_BUCKET_SEGMENTS =
	FLATFILE_ACCOUNTING_BUCKET_MAX_BYTES / (FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES -
						FLATFILE_ACCOUNTING_RECORD_MAX_BYTES - 80) +
	1;
static_assert(flatfile_authority_transaction_maximum_operations >= 2);

struct flatfile_accounting_record
{
	critical_command command;
	// Canonical EAP1 bytes; empty only for a committed rejection.
	std::vector<uint8_t> plan;
	uint64_t durable_revision = 0;
	uint32_t result_code = 0;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	std::vector<uint8_t> result;
};

enum class flatfile_accounting_status
{
	ok,
	not_found,
	already_exists,
	conflict,
	invalid,
	capacity,
	io_error
};

// Structural storage codecs, not writer/source authorization. Outputs unchanged
// on error. Domain adapters must verify actual locked effects before staging.
flatfile_accounting_status flatfile_accounting_record_encode(const flatfile_accounting_record &,
							     std::vector<uint8_t> *);
flatfile_accounting_status flatfile_accounting_record_decode(std::span<const uint8_t>,
							     flatfile_accounting_record *);

// Every lookup recovers the authority journal, verifies the active segment and
// rejects stale/missing indexes. A missing bucket never means an empty store.
flatfile_accounting_status flatfile_accounting_lookup(const std::string &root,
						      const flatfile_authority_lock &,
						      const critical_command &,
						      flatfile_accounting_record *,
						      std::string *error);

// Check the bucket index, active segment and stale-next fence without selecting
// a command. This is not a full semantic scan of sealed history.
flatfile_accounting_status flatfile_accounting_check_bucket(const std::string &,
							    const flatfile_authority_lock &,
							    const critical_operation_id &lineage,
							    size_t bucket, std::string *error);

// Private staging bridge. Only typed domain/lifecycle owners can combine
// actual locked effects with evidence; structural records grant no capability.
class flatfile_accounting_storage
{
	friend class flatfile_accounting_baseline_storage;
	friend class flatfile_accounting_bank_transaction;
	friend class flatfile_accounting_lifecycle_transaction;
	friend class flatfile_accounting_authority_storage;
#ifdef DURIS_FLATFILE_ACCOUNTING_TEST
	friend class flatfile_accounting_test_access;
#endif
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_commit_operations(
		const std::string &, const flatfile_authority_lock &,
		const std::vector<flatfile_authority_operation> &, std::string *);
	static flatfile_accounting_status
	initialize_bucket(const std::string &, const flatfile_authority_lock &,
			  const critical_operation_id &, size_t bucket,
			  std::vector<flatfile_authority_operation> *, std::string *);
	static flatfile_accounting_status stage(const std::string &,
						const flatfile_authority_lock &,
						const flatfile_accounting_record &,
						std::vector<flatfile_authority_operation> *,
						std::string *);
	static flatfile_authority_transaction_result
	commit(const std::string &, const flatfile_authority_lock &,
	       const std::vector<flatfile_authority_operation> &, std::string *);
};

#endif
