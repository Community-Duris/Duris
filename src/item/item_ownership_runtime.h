#ifndef ITEM_OWNERSHIP_RUNTIME_H
#define ITEM_OWNERSHIP_RUNTIME_H

#include "persistence/corpse_lifecycle_command.h"
#include "item/item_transfer_command.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct item_ownership_runtime_entry
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	item_owner_identity owner;
	uint64_t item_revision;
	uint64_t owner_revision;
	int32_t vnum;
	item_custody_state state;
};

struct collector_command_payload;
struct collector_command_result;

bool item_ownership_runtime_hydrate(const item_ownership_runtime_entry &entry);
bool item_ownership_runtime_hydrate_batch(const item_ownership_runtime_entry *batch, size_t count);
bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *batch,
						size_t count);
// Atomically replaces only the collector-owned runtime domain. Other custody is
// preserved. This is the restart/reconciliation publication primitive.
bool item_ownership_runtime_reconcile_collector(const item_ownership_runtime_entry *batch,
						size_t count);
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &owner, uint64_t revision);
bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry);
bool item_ownership_runtime_snapshot_owner(const item_owner_identity &owner, size_t limit,
					   std::vector<item_ownership_runtime_entry> *snapshot);
bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision);
bool item_ownership_runtime_apply(const item_transfer_payload &payload,
				  const item_transfer_result &result);
bool item_ownership_runtime_apply_collector(const collector_command_payload &payload,
					    const collector_command_result &result);
bool item_ownership_runtime_apply_corpse_release(uint32_t owner_pid, uint32_t save_id,
						 int32_t room_vnum,
						 const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_destruction(uint32_t owner_pid, uint32_t save_id,
						     const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_resurrection(uint32_t owner_pid, uint32_t save_id,
						      uint32_t player_pid, int32_t old_room_vnum,
						      const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_raise(uint32_t owner_pid, uint32_t save_id,
					       uint32_t player_pid, uint64_t pet_uid,
					       const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_discarded(uint32_t owner_pid, uint32_t save_id,
						   const std::vector<uint64_t> &item_uids,
						   const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_world_corpse_raise(
	uint64_t source_uid, int32_t room_vnum, uint32_t player_pid, uint64_t pet_uid,
	const std::vector<uint64_t> &durable_uids, const std::vector<uint64_t> &discarded_uids,
	const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_nested_release(uint32_t owner_pid, uint32_t save_id,
							const item_owner_identity &destination,
							uint64_t target_root_item_uid,
							uint64_t target_parent_item_uid,
							uint64_t expected_target_parent_revision,
							const corpse_lifecycle_result &result);
void item_ownership_runtime_forget(uint64_t item_uid);
void item_ownership_runtime_forget_owner(const item_owner_identity &owner);
void item_ownership_runtime_forget_player_domain(uint32_t player_pid);
void item_ownership_runtime_reset(void);
size_t item_ownership_runtime_size(void);

#endif
