#ifndef ITEM_OWNERSHIP_RUNTIME_H
#define ITEM_OWNERSHIP_RUNTIME_H

#include "persistence/corpse_lifecycle_command.h"
#include "item_transfer_command.h"

#include <cstddef>
#include <cstdint>

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

bool item_ownership_runtime_hydrate(const item_ownership_runtime_entry &entry);
bool item_ownership_runtime_hydrate_batch(const item_ownership_runtime_entry *batch, size_t count);
bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *batch,
						size_t count);
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &owner, uint64_t revision);
bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry);
bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision);
bool item_ownership_runtime_apply(const item_transfer_payload &payload,
				  const item_transfer_result &result);
bool item_ownership_runtime_apply_corpse_release(uint32_t owner_pid, uint32_t save_id,
						 int32_t room_vnum,
						 const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_destruction(uint32_t owner_pid, uint32_t save_id,
						     const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_resurrection(uint32_t owner_pid, uint32_t save_id,
						      uint32_t player_pid, int32_t old_room_vnum,
						      const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_raise(uint32_t owner_pid, uint32_t save_id,
					       uint32_t player_pid,
					       const corpse_lifecycle_result &result);
bool item_ownership_runtime_apply_corpse_nested_release(uint32_t owner_pid, uint32_t save_id,
							const item_owner_identity &destination,
							uint64_t target_root_item_uid,
							uint64_t target_parent_item_uid,
							uint64_t expected_target_parent_revision,
							const corpse_lifecycle_result &result);
void item_ownership_runtime_forget(uint64_t item_uid);
void item_ownership_runtime_forget_owner(const item_owner_identity &owner);
void item_ownership_runtime_reset(void);
size_t item_ownership_runtime_size(void);

#endif
