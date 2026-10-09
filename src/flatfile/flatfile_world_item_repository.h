#ifndef DURIS_FLATFILE_WORLD_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_WORLD_ITEM_REPOSITORY_H

#include "economy/zone_reset_item_recovery.h"
#include "persistence/corpse_lifecycle_command.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct flatfile_corpse_record
{
	uint32_t owner_pid = 0;
	std::string owner_name;
	uint32_t save_id = 0;
	int32_t room_vnum = 0;
	std::string short_description;
	std::string description;
	std::string keywords;
	int32_t weight = 0;
	std::array<int32_t, 8> values = {};
	std::array<int32_t, 4> money = {};
	uint64_t revision = 0;
	std::vector<player_item_snapshot> items;
};

struct flatfile_saved_world_item_record
{
	std::string item_key;
	int32_t room_vnum = 0;
	uint64_t revision = 0;
	std::vector<player_item_snapshot> items;
};

struct flatfile_room_item_record
{
	int32_t room_vnum = 0;
	uint64_t revision = 0;
	std::array<int32_t, 4> money = {};
	std::vector<player_item_snapshot> items;
};

struct flatfile_corpse_custody_item
{
	uint64_t item_uid = 0;
	int32_t vnum = 0;
	uint64_t root_item_uid = 0;
	uint64_t parent_item_uid = 0;
};

struct flatfile_corpse_custody_owner
{
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	std::vector<flatfile_corpse_custody_item> items;
};

struct flatfile_world_item_player_removal
{
	flatfile_authority_operation operation;
	std::vector<flatfile_corpse_custody_owner> custody;
};

struct flatfile_corpse_transfer_mutation
{
	flatfile_authority_after_image after_image;
	std::vector<flatfile_corpse_custody_item> expected_items;
	uint64_t corpse_revision = 0;
	bool created = false;
};

struct flatfile_room_transfer_mutation
{
	flatfile_authority_after_image after_image;
	std::vector<flatfile_corpse_custody_item> expected_items;
	uint64_t room_revision = 0;
	bool created = false;
};

struct flatfile_corpse_lifecycle_mutation
{
	flatfile_authority_after_image after_image;
	uint64_t corpse_revision = 0;
	uint64_t catalog_revision = 0;
};

struct flatfile_corpse_release_mutation
{
	flatfile_authority_after_image after_image;
	std::vector<flatfile_corpse_custody_item> expected_items;
	std::vector<player_item_snapshot> items;
	std::array<int32_t, 4> money = {};
	uint64_t room_revision = 0;
	uint64_t catalog_revision = 0;
};

struct flatfile_world_corpse_raise_mutation
{
	flatfile_authority_after_image after_image;
	std::vector<flatfile_corpse_custody_item> expected_items;
	std::vector<player_item_snapshot> pet_items;
	std::vector<uint64_t> durable_uids;
	std::vector<uint64_t> discarded_uids;
	uint64_t catalog_revision = 0;
};

struct collector_command_payload;
struct flatfile_collector_world_mutation
{
	flatfile_authority_after_image after_image;
	bool changed = false;
};

enum class flatfile_world_item_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	not_empty,
	invalid,
	io_error
};

flatfile_world_item_result flatfile_world_item_establish(
	const std::string &root, const std::vector<flatfile_corpse_record> &corpses,
	const std::vector<flatfile_saved_world_item_record> &saved_items, std::string *error);
flatfile_world_item_result
flatfile_world_item_list(const std::string &root, std::vector<flatfile_corpse_record> *corpses,
			 std::vector<flatfile_saved_world_item_record> *saved_items,
			 std::string *error);
flatfile_world_item_result
flatfile_world_item_list_rooms(const std::string &root,
			       std::vector<flatfile_room_item_record> *rooms, std::string *error);
// Read-only recovery inspection under the caller's authority lock; never replay.
flatfile_world_item_result flatfile_world_item_recovery_list_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_corpse_record> *corpses, std::vector<flatfile_room_item_record> *rooms,
	std::string *error);
// Complete world physical inspection under the existing authority freeze.
// Includes saved-item records omitted by the legacy two-output recovery reader.
// No replay or lock acquisition; outputs remain unchanged on failure.
flatfile_world_item_result flatfile_world_item_recovery_list_all_locked(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_corpse_record> *corpses,
    std::vector<flatfile_room_item_record> *rooms,
    std::vector<flatfile_saved_world_item_record> *saved_items, std::string *error);
flatfile_world_item_result flatfile_world_item_read_coin(const std::string &root,
							 const flatfile_authority_lock &lock,
							 const item_owner_identity &owner,
							 uint64_t uid, player_item_snapshot *item,
							 std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &expected_name, flatfile_world_item_player_removal *removal,
	std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_corpse_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, flatfile_corpse_transfer_mutation *mutation,
	std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_room_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, flatfile_room_transfer_mutation *mutation,
	std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_collector_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const collector_command_payload &payload, flatfile_collector_world_mutation *mutation,
	unsigned int *result_code, std::string *error);
struct coin_transfer_payload;
struct coin_transfer_result;
flatfile_world_item_result
flatfile_world_item_prepare_coin_rooms(const std::string &root, const flatfile_authority_lock &lock,
				       const coin_transfer_payload &payload,
				       const coin_transfer_result &result,
				       flatfile_authority_after_image *image, std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_corpse_lifecycle(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_corpse_lifecycle_mutation *mutation,
	std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_corpse_release_mutation *mutation,
	std::string *error);
flatfile_world_item_result flatfile_world_item_prepare_world_corpse_raise(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_world_corpse_raise_mutation *mutation,
	std::string *error);

// Passive native ROOM proposal; values alone grant no source, season,
// execution, receipt, commit or publication permission. The genuine atomic
// owner must authenticate the installed root/epoch and original source cut.
struct flatfile_initial_room_reset_world_stage
{
	critical_command original_command;
	bool catalog_before_present = false, room_before_present = false;
	uint64_t catalog_before_revision = 0, catalog_revision_after = 0;
	uint64_t room_revision_before = 0, room_revision_after = 0;
	std::vector<player_item_snapshot> room_before_items;
	flatfile_authority_operation operation;
};
class flatfile_accounting_zone_reset_item_transaction;
class flatfile_initial_room_reset_world_storage final
{
    private:
	friend class flatfile_accounting_zone_reset_item_transaction;
	// SAME already-recovered exclusive root lock; original INITIAL carrier,
	// actual ROOM counter and whole-catalog born UID absence. Full literals
	// retain native order/properties; original detached slot normalization only.
	// Every refusal preserves output; no acquire/recover/commit/live changes.
	static flatfile_world_item_result
	prepare_locked(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_native_recovery_envelope &original,
		       flatfile_initial_room_reset_world_stage *output) noexcept;
};

// Prospective storage admission for the same complete passive world read.
// Caller accounts for root/lock, existing outputs and callback context in outer;
// retains each admitted peak until this call's temporaries have died, then uses
// retained_output_payload_bytes (if supplied) for the transferred row/string/item
// requests, excluding the caller's inline vector objects. Strong outputs on every
// refusal, including that scalar. No lock acquisition, recovery or new authority.
// Requires the pinned libstdc++13 C++11 ABI request policy; no allocating diagnostics.
flatfile_world_item_result flatfile_world_item_recovery_list_all_locked_bounded(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_corpse_record> *corpses,
    std::vector<flatfile_room_item_record> *rooms,
    std::vector<flatfile_saved_world_item_record> *saved_items,
    flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
    size_t outer_live_scratch, size_t *retained_output_payload_bytes = nullptr) noexcept;

#endif
