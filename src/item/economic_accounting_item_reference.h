#ifndef DURIS_ECONOMIC_ACCOUNTING_ITEM_REFERENCE_H
#define DURIS_ECONOMIC_ACCOUNTING_ITEM_REFERENCE_H

#include "persistence/critical_command.h"
#include <cstdint>
#include <mysql/mysql.h>
#include <span>
#include <vector>

struct economic_accounting_item_reference
{
	critical_operation_id operation_id = {};
	uint16_t line_index = 0;
	uint32_t event_index = 0;
	uint16_t child_index = 0;
	uint64_t item_uid = 0;
	uint64_t before_revision = 0;
	uint64_t after_revision = 0;
	critical_operation_id legacy_operation_id = {};
	uint16_t legacy_event_index = 0;
};

// Validates the structure and invariants matching the DB constraints:
// - line_index < 3000
// - event_index == line_index
// - child_index <= 64
// - item_uid > 0
// - after_revision > before_revision
// - operation_id and legacy_operation_id non-empty
bool economic_accounting_item_reference_validate(const economic_accounting_item_reference &ref);

// Insert a single item accounting reference record into MySQL.
bool economic_accounting_item_reference_insert(MYSQL *connection,
					       const economic_accounting_item_reference &ref);

// Insert a contiguous batch of item references in order.
bool economic_accounting_item_reference_insert_batch(
	MYSQL *connection, std::span<const economic_accounting_item_reference> refs);

// Look up an item reference by legacy operation ID and legacy event index.
bool economic_accounting_item_reference_find_by_legacy(
	MYSQL *connection, const critical_operation_id &legacy_operation_id,
	uint16_t legacy_event_index, economic_accounting_item_reference *ref);

// Look up all references for a given root operation, ordered by line_index.
bool economic_accounting_item_reference_find_by_operation(
	MYSQL *connection, const critical_operation_id &operation_id,
	std::vector<economic_accounting_item_reference> *refs);

// Look up the full custody history for an item UID, ordered by after_revision.
bool economic_accounting_item_reference_find_history(
	MYSQL *connection, uint64_t item_uid,
	std::vector<economic_accounting_item_reference> *history);

#endif
