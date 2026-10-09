#ifndef DURIS_FLATFILE_ITEM_ACCOUNTING_REFERENCE_H
#define DURIS_FLATFILE_ITEM_ACCOUNTING_REFERENCE_H

#include "item/economic_accounting_item_reference.h"
#include "flatfile/flatfile_authority_transaction.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

constexpr size_t FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES = 66;
constexpr size_t FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES = 64 * 1024 * 1024;

enum class flatfile_item_accounting_status
{
	ok,
	not_found,
	already_exists,
	invalid,
	capacity,
	io_error
};

// Encode an item accounting reference into exact 66 little-endian bytes.
flatfile_item_accounting_status
flatfile_item_accounting_reference_encode(const economic_accounting_item_reference &ref,
					  std::vector<uint8_t> *bytes);

// Decode an item accounting reference from 66 bytes.
flatfile_item_accounting_status
flatfile_item_accounting_reference_decode(std::span<const uint8_t> bytes,
					  economic_accounting_item_reference *ref);

// Append a reference to the flatfile store under <root>/accounting/item_references.
flatfile_item_accounting_status
flatfile_item_accounting_reference_append(const std::string &root,
					  const economic_accounting_item_reference &ref,
					  std::string *error = nullptr);

// Stage one exact operation's references in the authority journal. The caller
// commits this operation together with the custody and accounting record.
flatfile_item_accounting_status flatfile_item_accounting_reference_stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &legacy_operation_id,
	std::span<const economic_accounting_item_reference> references,
	std::vector<flatfile_authority_operation> *operations, std::string *error = nullptr);

// Verify that the retained reference set for one legacy operation is exactly
// the expected set, including the absence of extra rows.
flatfile_item_accounting_status flatfile_item_accounting_reference_verify_operation(
	const std::string &root, const critical_operation_id &legacy_operation_id,
	std::span<const economic_accounting_item_reference> expected, std::string *error = nullptr);

// Look up a reference by legacy operation ID and legacy event index.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_legacy(
	const std::string &root, const critical_operation_id &legacy_operation_id,
	uint16_t legacy_event_index, economic_accounting_item_reference *ref,
	std::string *error = nullptr);

// Look up a reference by item UID and after revision.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_item(
	const std::string &root, uint64_t item_uid, uint64_t after_revision,
	economic_accounting_item_reference *ref, std::string *error = nullptr);

// Look up all references for a given root operation, ordered by line_index.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_operation(
	const std::string &root, const critical_operation_id &operation_id,
	std::vector<economic_accounting_item_reference> *refs, std::string *error = nullptr);

// Look up the full custody history for an item UID, ordered by after_revision.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_history(
	const std::string &root, uint64_t item_uid,
	std::vector<economic_accounting_item_reference> *history, std::string *error = nullptr);

struct flatfile_accounting_audit_report
{
	size_t total_buckets_scanned = 0;
	size_t valid_records_count = 0;
	size_t partial_trailing_bytes_truncated = 0;
	size_t corrupted_buckets_quarantined = 0;
};

// Scan all 256 sharded item reference buckets, repair partial trailing records from crashes,
// and quarantine unrecoverable buckets.
flatfile_item_accounting_status
flatfile_item_accounting_reference_verify_and_repair(const std::string &root,
						     flatfile_accounting_audit_report *report,
						     std::string *error = nullptr);

// Complete original operation set verification under borrowed genuine root lock.
// Caller admits expected span/buffers and existing root/lock/context in outer.
// No recovery, repair, staging or publication; unsupported request ABI refuses.
flatfile_item_accounting_status flatfile_item_accounting_reference_verify_operation_bounded(
	const std::string &, const flatfile_authority_lock &, const critical_operation_id &,
	const std::span<const economic_accounting_item_reference> &, flatfile_scratch_reserve_fn,
	void *, size_t) noexcept;

#endif
