#ifndef DURIS_ECONOMIC_SQL_BASELINE_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_BASELINE_TRANSACTION_H
#include "economy/economic_baseline_command.h"
#include "persistence/critical_command_completion.h"
#include <mysql/mysql.h>

// Private baseline persistence owner, not authority to establish a native
// snapshot or activate a domain. The lifecycle caller must retain its frozen
// native boundary throughout apply. No production caller exists yet.
class economic_sql_baseline_transaction
{
	friend class economic_sql_accounting_lifecycle_transaction;
#ifdef DURIS_ECONOMIC_SQL_BASELINE_TEST
	friend class economic_sql_baseline_test_access;
#endif
	// Borrow a reconnect-disabled transaction. The caller owns the lineage,
	// initialization receipt, commit and rollback on every error. Absence alone
	// cannot authorize reinitialization after a partial restore.
	static unsigned int initialize(MYSQL *, const critical_operation_id &lineage,
				       const critical_operation_id &epoch,
				       const economic_account_key &opening,
				       const critical_operation_id &creating_operation);
	// Own START/COMMIT/rollback. Require READ COMMITTED; refuse caller
	// transactions and reconnect.
	// Original-ID replay ignores the supplied preparation and verifies retained
	// bytes. No native writes, child commands or publication are performed.
	static critical_apply_result apply(MYSQL *, const critical_command &,
					   const economic_prepared_baseline &);
	static critical_apply_result reconcile(MYSQL *, const critical_command &);
	static critical_apply_result run(MYSQL *, const critical_command &,
					 const economic_prepared_baseline *);
};
#endif
