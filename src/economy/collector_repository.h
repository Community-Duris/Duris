#ifndef DURIS_COLLECTOR_REPOSITORY_H
#define DURIS_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"

#include <mysql/mysql.h>
#include <vector>

struct collector_enrollment_repository_plan
{
	bool active = false;
	bool death_exists = false;
	uint64_t catalog_revision = 0;
	uint64_t next_listing = 0;
	collector::rules policy = {};
	std::vector<item_transfer_entry> new_items;
};

struct collector_item_boundary_repository_entry
{
	collector_listing_detail prior;
	collector::record cancelled;
};

struct collector_item_boundary_repository_plan
{
	collector::reason reason = collector::reason::none;
	uint32_t actor_pid = 0;
	uint64_t catalog_revision = 0;
	std::vector<collector_item_boundary_repository_entry> entries;
};

// Read-only restart bootstrap. The implementation uses bounded buffered reads
// so a validation failure never leaves unread packets on a pooled connection.
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

// The prepare/apply pair runs inside an item-transfer transaction. Prepare
// locks collector allocation state before item ownership rows are locked;
// apply inserts candidate records only after the corpse custody move succeeds.
bool collector_repository_prepare_death_enrollment(MYSQL *connection,
						   const item_transfer_payload &payload,
						   collector_enrollment_repository_plan *plan,
						   unsigned int *result_code);
bool collector_repository_apply_death_enrollment(MYSQL *connection, const critical_command &command,
						 const item_transfer_payload &payload,
						 const item_transfer_result &transfer,
						 const collector_enrollment_repository_plan &plan);

// Prepare is called before item ownership rows are changed. It protects the
// indexed candidate ranges (including empty ranges) and, when candidates are
// present, follows the global catalog->listing lock order. Apply runs only
// after the item transfer succeeded and writes cancellation records, ledger
// evidence, and outbox-ready results in that same database transaction.
bool collector_repository_prepare_item_boundary(MYSQL *connection,
						const item_transfer_payload &payload,
						collector_item_boundary_repository_plan *plan,
						unsigned int *result_code);
bool collector_repository_apply_item_boundary(MYSQL *connection, const critical_command &command,
					      const collector_item_boundary_repository_plan &plan,
					      uint64_t *catalog_revision,
					      std::vector<collector_command_result> *events);

// Executes one already-journaled collector command inside the caller's active
// transaction. Terminal policy/custody failures are returned through
// result_code without mutating domain state; database failures return false.
bool collector_repository_execute(MYSQL *connection, const critical_command &command,
				  collector_command_result *result, unsigned int *result_code,
				  bool *mutation_applied);

#endif
