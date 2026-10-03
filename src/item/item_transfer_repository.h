#ifndef ITEM_TRANSFER_REPOSITORY_H
#define ITEM_TRANSFER_REPOSITORY_H

#include "item/item_transfer_command.h"
#include "item/economic_accounting_item_reference.h"
#include "economy/economic_accounting_types.h"

#include <mysql/mysql.h>

// Only an enclosing, admitted accounting owner may supply this context. A
// schema-v1 inbox ID alone is not admission. The enclosing owner inserts and
// verifies references after its root operation; nullptr selects the legacy writer.
struct item_transfer_accounting_context
{
	critical_operation_id root_operation_id = {};
	uint16_t child_index = 0;
	uint16_t line_index_base = 0;
};

// Locked custody evidence captured by a successful item transfer. The source
// and target roots are included as witnesses so the accounting plan can prove
// that moved subtrees remain well-formed across the ownership change.
struct item_transfer_custody_delta
{
	std::vector<economic_item_snapshot> before;
	std::vector<economic_item_snapshot> after;
	std::vector<economic_item_event> events;
};

bool item_transfer_repository_execute(
	MYSQL *connection, const critical_command &command, item_transfer_result *result,
	unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr,
	item_transfer_custody_delta *custody_delta = nullptr);
// Compound commands may use one inbox operation for consecutive transfer
// segments. The offset keeps their item-ledger event indexes disjoint.
bool item_transfer_repository_execute_at_offset(
	MYSQL *connection, const critical_command &command, uint16_t event_index_base,
	item_transfer_result *result, unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr,
	item_transfer_custody_delta *custody_delta = nullptr);
// Called only inside the enclosing coin transaction; never commits independently.
bool item_transfer_repository_execute_coin(
	MYSQL *connection, const critical_command &command, const std::array<int32_t, 4> &before,
	item_transfer_result *result, unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr);
// Transaction-scoped owner primitives used by compound authority commands.
// Callers must acquire every participating owner in canonical identity order.
bool item_transfer_repository_ensure_owner(MYSQL *connection, const item_owner_identity &owner);
bool item_transfer_repository_lock_owner(MYSQL *connection, const item_owner_identity &owner,
					 uint64_t *revision);
bool item_transfer_repository_advance_owner(MYSQL *connection, const item_owner_identity &owner,
					    uint64_t prior_revision);
bool item_transfer_repository_destroy_owners(MYSQL *connection, const item_owner_identity *owners,
					     size_t owner_count);
// Retire selected container/item roots while preserving their contents with the
// current owner.  The caller owns the enclosing SQL transaction and its physical
// projection changes; this function advances authoritative custody and ledger rows.
bool item_transfer_repository_revoke_roots_preserving_children(MYSQL *connection,
							       const uint64_t *item_uids,
							       size_t item_count);

#endif
