#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "economy/auction_command.h"

#include <mysql/mysql.h>
#include "economy/economic_accounting_types.h"

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

// Only schema-2 auction components may call this entry point, inside their
// already-locked inbox transaction. The caller must record the matching EAP1
// root before writing the receipt and committing. Other actions refuse.
bool auction_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					  auction_command_result *result, unsigned int *result_code,
					  bool *mutation_applied);

// Fresh bid/settlement producer capture under the existing trusted native SQL
// caller's transaction with reconnect disabled. No mutation, admission, inbox,
// identity reservation, or transaction management. Caller supplies its qualified
// lineage/epoch; mappings and all absence facts come from native SQL. On failure
// roll back; command stays unchanged. Schema-2 replay cannot enter this function.
unsigned int auction_repository_prepare_accounting(MYSQL *, const critical_operation_id &lineage,
						   const critical_operation_id &epoch,
						   critical_command *);

// Canonical original typed command only. No current lineage/epoch/mapping read.
bool auction_repository_frozen_accounting_valid(const critical_command &) noexcept;

struct auction_accounting_endpoint_readback
{
	economic_account_key bidder_claim{}, previous_claim{}, seller_claim{};
	bool unused_bidder = false;
};
// Read original committed AEC1 creator proof using the complete retained command.
// Resolved keys are observations; the original command/tags/zero IDs stay exact.
// Authenticated unused bidder ENODATA is retained as unused_bidder with an empty
// key, including after a later genuine endpoint. No tagged role returns ENODATA
// without claiming mapped-only receipt proof. Output stays unchanged on failure.
unsigned int auction_repository_readback_endpoints(MYSQL *, const critical_command &,
						   auction_accounting_endpoint_readback *);

#endif
