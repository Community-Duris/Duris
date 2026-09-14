#ifndef DURIS_COLLECTOR_RUNTIME_H
#define DURIS_COLLECTOR_RUNTIME_H

#include "economy/collector_command.h"
#include "item/item_ownership_runtime.h"

#include <cstddef>
#include <cstdint>
#include <vector>

// Game-thread projection of the durable collector catalog. Persistence workers
// rebuild it at boot; committed completions and outbox delivery advance it.
bool collector_runtime_rebuild(const collector::catalog &catalog);
// Installs a restart/reconciliation snapshot only after both its catalog and
// exact collector-held custody projection validate. Publication is game-thread
// only and does not expose a half-applied pair.
bool collector_runtime_rebuild_authoritative(const collector::catalog &catalog,
					     const item_ownership_runtime_entry *held_items,
					     size_t held_count);
bool collector_runtime_publish(const collector_command_result &result);
bool collector_runtime_find(uint64_t listing, collector::record *entry);
bool collector_runtime_snapshot(collector::catalog *catalog);
bool collector_runtime_available_for(uint32_t beneficiary, size_t limit,
				     std::vector<collector::record> *entries);
std::vector<uint64_t> collector_runtime_lease_due(uint64_t now, size_t limit, uint64_t lease_until);
uint64_t collector_runtime_catalog_revision(void);
uint64_t collector_runtime_next_listing(void);
size_t collector_runtime_size(void);
size_t collector_runtime_available_count(void);
void collector_runtime_reset(void);

// Outbox-facing adapter. The id is evidence owned by the durable outbox; the
// catalog mutation itself is idempotent by listing and listing revision.
bool collector_publish_committed_event(const collector_command_result &result,
				       unsigned long long outbox_id);

#endif
