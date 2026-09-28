#ifndef DURIS_ECONOMIC_SQL_PENDING_CLAIM_SOURCE_H
#define DURIS_ECONOMIC_SQL_PENDING_CLAIM_SOURCE_H

#include "economy/economic_accounting_types.h"
#include "persistence/economic_accounting_repository.h"

// Caller owns the transaction that staged a positive native pending-money
// credit and inserted its successful accounting root/effects. Each source slot
// identifies one beneficiary of that root. A nonzero result requires rollback.
unsigned int economic_sql_pending_claim_source_stage(MYSQL *connection,
						     const critical_operation_id &source_operation,
						     uint16_t source_slot,
						     const economic_account_key &claim_account,
						     uint32_t beneficiary_pid, uint64_t amount);

#endif
