#ifndef DURIS_COLLECTOR_REPOSITORY_H
#define DURIS_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"

#include <mysql/mysql.h>

// Read-only restart bootstrap. The implementation uses one streaming statement
// so the catalog cursor and every record come from the same committed snapshot.
// On failure, catalog is unchanged and errno carries an errno/MySQL-style code.
bool collector_repository_read_catalog(MYSQL *connection, collector::catalog *catalog);

// The restart path additionally reads and validates the exact collector-held
// custody authority in the same statement snapshot. On failure, snapshot is
// unchanged.
bool collector_repository_read_bootstrap(MYSQL *connection, collector_bootstrap_snapshot *snapshot);

// Bounded, non-locking detail read for an asynchronous inspect/buy preparation.
// Not-found is a successful read with found=false. On failure, detail and found
// are unchanged and errno carries the cause.
bool collector_repository_read_listing(MYSQL *connection, uint64_t listing,
				       collector_listing_detail *detail, bool *found);

// Executes one already-journaled collector command inside the caller's active
// transaction. Terminal policy/custody failures are returned through
// result_code without mutating domain state; database failures return false.
bool collector_repository_execute(MYSQL *connection, const critical_command &command,
				  collector_command_result *result, unsigned int *result_code,
				  bool *mutation_applied);

#endif
