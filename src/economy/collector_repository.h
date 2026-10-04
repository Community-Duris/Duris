#ifndef DURIS_COLLECTOR_REPOSITORY_H
#define DURIS_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"
#include "economy/economic_accounting_plan.h"

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

// Values copied from rows locked by one collector command, before its first
// native mutation. Only the accounted entry point returns this evidence, and
// only on a successful singleton purchase or held-item transition.
struct collector_repository_locked_before
{
	collector::record listing = {};
	currency_command_result balances = {};
	uint32_t bank_id = 0;
	economic_item_snapshot item = {};
	uint64_t catalog_revision = 0;
	uint64_t from_owner_revision = 0;
	uint64_t to_owner_revision = 0;
};

// Current locked native projection, distinct from the historical completion.
// Caller owns economic lifetime locks and the reconnect-disabled transaction.
// The reader never applies a purchase or manufactures an absent owner key.
// Successful singleton payload proof includes exact existing runtime and
// item-properties encodings, not only the scalar/affect/description rows.
struct collector_purchase_current_projection
{
	collector_command_result result = {};
	uint64_t player_save_revision = 0;
	uint32_t bank_id = 0;
};
bool collector_repository_read_purchase_projection(
	MYSQL *connection, const critical_command &command,
	const collector_command_result &original_result, unsigned int original_result_code,
	collector_purchase_current_projection *projection) noexcept;

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

// Schema-2 singleton purchase, expiry, and held cancellation only. The caller
// owns the SQL transaction, locked economic lifetime mapping, accounting root,
// receipt, outbox, and rollback on any failure. Other collector actions refuse
// before domain mutation. Accounted purchase payload extensions are written
// in this same transaction. The legacy entry point remains schema-1 only.
bool collector_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					    collector_command_result *result,
					    unsigned int *result_code, bool *mutation_applied,
					    collector_repository_locked_before *before);

#endif
