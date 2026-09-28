#ifndef DURIS_FLATFILE_ITEM_ACCOUNTING_REFERENCE_H
#define DURIS_FLATFILE_ITEM_ACCOUNTING_REFERENCE_H

#include "item/economic_accounting_item_reference.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

constexpr size_t FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES = 66;

enum class flatfile_item_accounting_status
{
	ok,
	not_found,
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

// Look up a reference by legacy operation ID and legacy event index.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_legacy(
	const std::string &root, const critical_operation_id &legacy_operation_id,
	uint16_t legacy_event_index, economic_accounting_item_reference *ref,
	std::string *error = nullptr);

// Look up a reference by item UID and after revision.
flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_item(
	const std::string &root, uint64_t item_uid, uint64_t after_revision,
	economic_accounting_item_reference *ref, std::string *error = nullptr);

#endif
