#ifndef ITEM_OWNERSHIP_RUNTIME_H
#define ITEM_OWNERSHIP_RUNTIME_H

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
void item_ownership_runtime_forget(uint64_t item_uid);
void item_ownership_runtime_reset(void);
size_t item_ownership_runtime_size(void);

#endif
