#ifndef DURIS_ECONOMIC_SQL_ACCOUNTING_LIFECYCLE_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_ACCOUNTING_LIFECYCLE_TRANSACTION_H

#include "economy/economic_accounting_types.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/economic_sql_source_snapshot.h"
#include <mysql/mysql.h>
#include <string>
#include <vector>

struct economic_sql_lifecycle_request
{
	critical_operation_id operation_id = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	uint64_t actor_id = 0;
	// Stable across exact retry; it is bound into the durable request receipt.
	uint64_t accepted_at_usec = 0;
};

// Parent converts these verified SQL-lifetime exports to its private gameplay
// cache types only after this owner returns success. The cache is not storage
// authority and its installation never selects active_epoch.
struct economic_sql_lifecycle_wallet_mapping
{
	uint32_t pid = 0;
	economic_account_key account;
};
struct economic_sql_lifecycle_bank_mapping
{
	std::string name;
	uint8_t racewar = 0;
	economic_account_key account;
};
struct economic_sql_lifecycle_treasury_mapping
{
	uint32_t shop_id = 0;
	uint32_t native_id = 0;
	economic_account_key account;
};
struct economic_sql_lifecycle_receipt
{
	critical_operation_id operation_id = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	critical_operation_id baseline_operation_id = {};
	economic_sql_source_digest source_capture_digest = {};
	economic_sql_source_digest native_boundary_digest = {};
	uint64_t baseline_revision = 0;
	std::vector<economic_sql_lifecycle_wallet_mapping> wallets;
	std::vector<economic_sql_lifecycle_bank_mapping> banks;
	std::vector<economic_sql_lifecycle_treasury_mapping> treasuries;
};

// Plans 2-4 register their route proofs with the Plan 5 verifier. Its
// manifest digest binds the complete reviewed writer census; audit_digest
// binds the separate native/evidence reconciliation. This owner rejects a
// missing verifier, incomplete count, or zero digest before selecting an epoch.
struct economic_sql_activation_evidence
{
	economic_sql_source_digest manifest_digest = {};
	economic_sql_source_digest audit_digest = {};
	uint64_t route_count = 0;
	uint64_t verified_route_count = 0;
	uint64_t unclassified_route_count = 0;
};
// The synchronous verifier borrows the caller's exact request as context; it
// must independently authenticate that binding against retained SQL evidence.
// The request itself grants no authority and must not be retained. The existing
// owner still authenticates the staged request before recording a decision.
using economic_sql_activation_verifier = unsigned int (*)(
	MYSQL *, const economic_sql_lifecycle_request &, const economic_sql_activation_evidence &,
	const economic_sql_source_snapshot &) noexcept;

// Private SQL lifecycle owner. Requires a live, lock-backed maintenance token
// obtained from economic_sql_lifecycle_guard::acquire_maintenance(). It captures
// full native sources, creates durable wallet PID/bank-row lifetimes, persists
// and reads back a complete baseline receipt, then selects a staged epoch in its
// own installation table. It deliberately leaves economic_lineage_state.active_epoch
// NULL because gameplay-wide producer coverage is not complete in this phase.
class economic_sql_accounting_lifecycle_transaction
{
    public:
	static unsigned int install(MYSQL *, const economic_sql_lifecycle_guard &,
				    const economic_sql_lifecycle_request &,
				    economic_sql_lifecycle_receipt *) noexcept;
	// The caller owns the quiesced transaction and its terminal outcome. Only
	// verified activation may create the global decision in that transaction;
	// the older pointer selector below requires that decision already be present.
	// Plan 5 supplies the independent verifier; production registers none yet.
	static unsigned int activate_verified(MYSQL *,
					      economic_sql_cutover_transaction_owner &owner,
					      const economic_sql_lifecycle_request &,
					      const economic_sql_activation_evidence &,
					      economic_sql_activation_verifier) noexcept;
	static unsigned int activate(MYSQL *connection,
				     economic_sql_cutover_transaction_owner &owner,
				     const critical_operation_id &lineage,
				     uint64_t *new_lineage_revision = nullptr) noexcept;
	static unsigned int pause(MYSQL *, economic_sql_cutover_transaction_owner &owner,
				  const critical_operation_id &lineage) noexcept;
	// Called at runtime boot while its named lock is held and before gameplay.
	static unsigned int recover_runtime(MYSQL *, const economic_sql_lifecycle_guard &,
					    bool *active) noexcept;
};

#endif
