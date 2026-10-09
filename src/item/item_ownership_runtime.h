#ifndef ITEM_OWNERSHIP_RUNTIME_H
#define ITEM_OWNERSHIP_RUNTIME_H

#include "persistence/corpse_lifecycle_command.h"
#include "item/item_transfer_command.h"

#include <cstddef>
#include <cstdint>
#include <span>
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

// Complete read-only active cache census on the serialized game thread. All
// owner domains and malformed relationships remain visible; no native authority
// or world completeness is inferred. Original 262144 ceiling; errno result and
// unchanged output on any failure, including allocation or excessive rows.
unsigned int item_ownership_runtime_snapshot_all_active(
	size_t limit, std::vector<item_ownership_runtime_entry> *output) noexcept;

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
// Read-only game-thread census of every row claiming this root, including
// conflicting owners, states and topology. Sorted by item UID; this does not
// prove root existence, valid custody or durable/native authority. A zero root
// or zero limit is invalid. Failure empties a nonnull output; success is complete.
bool item_ownership_runtime_snapshot_root(uint64_t root_item_uid, size_t limit,
					  std::vector<item_ownership_runtime_entry> *snapshot);
// Same bounded root observation, restricted to active custody before counting
// or allocating. Retained destroyed/quarantined history consumes no budget and
// remains untouched. All active owners/topology count, including conflicting
// claims. Explicit historical rows need separate per-UID comparison.
bool item_ownership_runtime_snapshot_active_root(
	uint64_t root_item_uid, size_t limit, std::vector<item_ownership_runtime_entry> *snapshot);
// Pure serialized game-thread cache observation. Never inserts a missing owner,
// hydrates, allocates or grants native authority. Missing/invalid/null refuses
// with output unchanged; a cached revision of zero is a valid observation.
bool item_ownership_runtime_peek_owner_revision(const item_owner_identity &owner,
						uint64_t *revision) noexcept;
bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision);
class item_native_quest_publication_owner;
// Only the original native publication owner may project an authenticated
// CURRENT cut. This grants neither SQL nor physical publication authority.
class item_ownership_runtime_native_quest_publication_owner final
{
    private:
	friend class item_native_quest_publication_owner;
	enum class publication_result : uint8_t
	{
		applied,
		rejected
	};
	static bool apply(const item_transfer_payload &, const item_transfer_result &,
			  publication_result,
			  std::span<const item_ownership_runtime_entry> current_custody,
			  uint64_t current_from_revision, uint64_t current_to_revision,
			  uint64_t current_player_revision) noexcept;
};

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
bool item_ownership_runtime_apply_world_corpse_raise(uint64_t source_uid, int32_t room_vnum,
						     uint32_t player_pid, uint64_t pet_uid,
						     const std::vector<uint64_t> &durable_uids,
						     const std::vector<uint64_t> &discarded_uids,
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

// Private complete boot-world cache observation, on the serialized game thread.
// Strict ascending UID values are evidence only. Current active UID/root/parent
// links all count, including foreign owners/contexts; no hydrate or authority.
class item_ownership_runtime_published_native_observer final
{
	friend class quest_mobile_published_world_owner;
	friend class quest_mobile_native_item_stage;
	friend class zone_reset_room_publication_owner;
	friend class zone_reset_item_owner;
	friend class shop_trade_native_checkpoint_owner;
	friend class shop_trade_current_runtime_owner;
	static bool snapshot_links(std::span<const uint64_t> selected_uids, size_t limit,
				   std::vector<item_ownership_runtime_entry> *output) noexcept;
};

#endif
