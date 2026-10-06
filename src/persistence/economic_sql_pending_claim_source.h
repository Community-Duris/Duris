#ifndef DURIS_ECONOMIC_SQL_PENDING_CLAIM_SOURCE_H
#define DURIS_ECONOMIC_SQL_PENDING_CLAIM_SOURCE_H

#include "economy/economic_accounting_types.h"
#include "persistence/economic_accounting_repository.h"

// Presence/selected-shape preflight for additive0062. This never mutates SQL;
// exact engine metadata remains the immutable migration/runtime contract gate.
unsigned int economic_sql_pending_claim_source_contract(MYSQL *);

// AEC1 absent endpoints are resolved only inside the original admitted auction
// transaction. Absence is a row/lifetime proof, not money==0. Create runs only
// after that original native refund/proceeds, before its final plan is recorded.
unsigned int economic_sql_pending_claim_endpoint_lock_absent(MYSQL *, const critical_command &,
							     uint32_t beneficiary_pid);
unsigned int economic_sql_pending_claim_endpoint_create(MYSQL *, const critical_command &,
							uint32_t beneficiary_pid,
							economic_account_key *);
// Retained replay reads the original resolved mapping/plan. It allocates nothing
// and never reseals original AEC1 facts with a current mapping identity.
unsigned int economic_sql_pending_claim_endpoint_readback(MYSQL *, const critical_command &,
							  uint32_t beneficiary_pid,
							  economic_account_key *);

// Cold recovery's narrow exception for a genuine later zero-proceeds endpoint.
// Requires original AEC1 seller request and committed native/financial creator;
// arbitrary zero mappings and original positive-origin checks stay refused.
unsigned int economic_sql_pending_claim_endpoint_verify_zero_creator(
	MYSQL *, const critical_operation_id &creator, const economic_account_key &,
	uint32_t beneficiary_pid);

// Immutable original AEC1 creator/effect/source proof. Retired mappings and
// later claim consumption remain valid; current native balances are not read.
// This authenticates known retained values, never an unseen creator header.
unsigned int
economic_sql_pending_claim_endpoint_verify_retained_creator(MYSQL *, const critical_operation_id &,
							    const economic_account_key &,
							    uint32_t beneficiary_pid);

// Caller owns the transaction that staged a positive native pending-money
// credit and inserted its successful accounting root/effects. Each source slot
// identifies one beneficiary of that root. A nonzero result requires rollback.
unsigned int economic_sql_pending_claim_source_stage(MYSQL *connection,
						     const critical_operation_id &source_operation,
						     uint16_t source_slot,
						     const economic_account_key &claim_account,
						     uint32_t beneficiary_pid, uint64_t amount);

// Read remaining amounts under the caller's mapping/native transaction lock.
// Original source amounts and legacy whole-consumption links are immutable.
// New consumption rows are independently bound to successful exact debit effects.
struct economic_sql_pending_claim_remaining
{
	critical_operation_id operation = {};
	uint16_t slot = 0;
	uint64_t original_amount = 0, remaining_amount = 0;
};
unsigned int
economic_sql_pending_claim_source_remaining(MYSQL *, const economic_account_key &,
					    uint32_t beneficiary_pid,
					    std::vector<economic_sql_pending_claim_remaining> *);

// Compare immutable positive opening allocations to the authenticated original
// baseline. Caller owns its transaction and native mapping locks; no rows are
// inserted or reopened, and genuine later consumption remains valid.
struct economic_baseline_batch;
unsigned int economic_sql_pending_claim_source_verify_baseline(
	MYSQL *, const critical_operation_id &baseline_operation, const economic_baseline_batch &);

// Retain exact whole or partial consumption links in the caller's transaction.
// The successful debit root/effect must already exist; no origin is rewritten.
unsigned int economic_sql_pending_claim_source_consume(
	MYSQL *connection, const critical_operation_id &spending_operation,
	const economic_account_key &claim_account, uint32_t beneficiary_pid, uint64_t claim_balance,
	uint64_t amount);

#endif
