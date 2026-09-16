#ifndef DURIS_COLLECTOR_RUNTIME_H
#define DURIS_COLLECTOR_RUNTIME_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"
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
					     size_t held_count,
					     const collector_death_snapshot *deaths = nullptr,
					     size_t death_count = 0);
bool collector_runtime_publish(const collector_command_result &result);
bool collector_runtime_find(uint64_t listing, collector::record *entry);
bool collector_runtime_find_death(uint32_t beneficiary_pid, uint64_t death_time,
				  collector_death_snapshot *death);
// Return a bounded, ordered batch of available listings whose per-death hint is
// not delivered. State PENDING is intentionally included so a restart can
// resume a claim that was committed before delivery finished.
bool collector_runtime_hint_candidates(uint64_t after_listing, size_t scan_limit,
					       size_t result_limit, uint64_t now,
					       std::vector<collector::record> *entries,
					       uint64_t *next_after_listing, bool *reached_end);
// Monotonic game-thread publication of the durable hint projection. Persistence
// remains authoritative; a missing in-memory death is not a publication error.
void collector_runtime_publish_hint(const collector::record &entry, uint8_t state,
					    uint64_t hint_revision);
bool collector_runtime_snapshot(collector::catalog *catalog);
bool collector_runtime_available_for(uint32_t beneficiary, size_t limit,
				     std::vector<collector::record> *entries);
std::vector<uint64_t> collector_runtime_lease_due(uint64_t now, size_t limit, uint64_t lease_until);
// Bounded map walk used to converge feature-wide pause/resume state without a
// full catalog copy on the game thread. Pass after_listing=0 to start a pass;
// next_after_listing becomes zero and reached_end is true at the end.
bool collector_runtime_pause_mismatches(bool should_pause, uint64_t after_listing,
					size_t scan_limit, size_t result_limit,
					std::vector<collector::record> *entries,
					uint64_t *next_after_listing, bool *reached_end);
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
