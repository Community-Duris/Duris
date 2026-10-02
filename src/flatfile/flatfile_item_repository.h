#ifndef DURIS_FLATFILE_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_ITEM_REPOSITORY_H

#include "economy/auction_command.h"
#include "economy/collector_custody_boundary.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "persistence/critical_command_coordinator.h"
#include "item/item_transfer_command.h"
#include "item/item_ownership_runtime.h"
#include "item/quest_reward_continuation.h"
#include "economy/shop_trade_command.h"
#include "economy/coin_transfer_command.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_item_ownership_record
{
	uint64_t item_uid = 0;
	uint64_t root_item_uid = 0;
	uint64_t parent_item_uid = 0;
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t item_revision = 0;
	int32_t vnum = 0;
	item_custody_state state = item_custody_state::absent;
	std::vector<uint8_t> coin_payload = {};
	uint16_t equipment_slot = 0;
};

struct flatfile_coin_pile_source
{
	flatfile_item_ownership_record ownership;
	player_item_snapshot item;
};

struct flatfile_quest_reward_obligation
{
	critical_operation_id offering_operation = {};
	std::vector<uint8_t> continuation;
	uint64_t xp_applied_mask = 0;
	uint64_t economic_applied_mask = 0;
	bool economic_history_verified = true;
};

struct flatfile_quest_xp_entitlement
{
	critical_operation_id offering_operation = {};
	quest_reward_continuation terms;
	uint32_t reward_index = 0;
	uint32_t amount = 0;
};

enum class flatfile_item_repository_result
{
	ok,
	not_found,
	unchanged,
	invalid,
	io_error
};

enum class flatfile_item_baseline_result
{
	applied,
	already_applied,
	conflict,
	invalid,
	io_error
};

struct flatfile_item_auction_mutation
{
	uint64_t player_owner_revision = 0;
	uint64_t auction_owner_revision = 0;
	uint16_t item_count = 0;
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_uids = {};
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_revisions = {};
	flatfile_authority_after_image after_image;
};

struct flatfile_item_shop_trade_mutation
{
	uint64_t player_owner_revision = 0;
	uint64_t counterparty_owner_revision = 0;
	uint16_t item_count = 0;
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_uids = {};
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_revisions = {};
	flatfile_authority_after_image after_image;
};

struct flatfile_item_corpse_release_mutation
{
	flatfile_authority_after_image after_image;
	uint64_t corpse_owner_revision = 0;
	uint64_t room_owner_revision = 0;
	uint64_t player_owner_revision = 0;
	uint64_t pet_owner_revision = 0;
	uint64_t max_item_revision = 0;
	uint64_t item_count = 0;
	uint64_t destruction_owner_revision = 0;
	uint64_t max_discarded_item_revision = 0;
	uint64_t discarded_item_count = 0;
	std::vector<collector_custody_boundary_item> collector_items;
};

struct collector_command_payload;
struct flatfile_item_collector_mutation
{
	flatfile_authority_after_image after_image;
	uint64_t from_owner_revision = 0;
	uint64_t to_owner_revision = 0;
	uint64_t item_revision = 0;
};

flatfile_item_repository_result flatfile_item_repository_load_owner(
	const std::string &root, const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error);
flatfile_item_repository_result flatfile_item_repository_load_owner_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error);
// Includes retired UIDs so a read-only caller can reconstruct their last
// owner and historical root/parent without treating them as live inventory.
flatfile_item_repository_result
flatfile_item_repository_lookup_uid(const std::string &root, uint64_t uid,
				    flatfile_item_ownership_record *item, std::string *error);
flatfile_item_repository_result flatfile_item_repository_lookup_uid_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	flatfile_item_ownership_record *item, std::string *error);
flatfile_item_repository_result flatfile_item_repository_load_coins_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<uint64_t> &uids, std::vector<flatfile_item_ownership_record> *coins,
	std::string *error);
// Read one active native money pile and its exact saved coin payload under the
// authority lock. Legacy payloads are resolved from their current owner file.
// Outputs are unchanged on failure; absence or ambiguity cannot seed a baseline.
flatfile_item_repository_result flatfile_item_repository_read_coin_pile_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	flatfile_coin_pile_source *source, std::string *error);
// Enumerate catalog coin candidates under the same lock. Active entries with
// the virtual coin vnum or a retained coin payload must decode as money;
// unresolved custody is refused. A lifecycle owner must also inspect legacy
// owner files for money objects with a nonstandard vnum and no retained payload.
flatfile_item_repository_result flatfile_item_repository_list_coin_piles_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_coin_pile_source> *sources, std::string *error);
// Prepare the pile endpoints of a coin root under the caller's authority lock.
// The caller stages returned domain images with wallet images and accounting
// evidence in one journal commit. No image or result is published on error.
flatfile_item_repository_result flatfile_item_repository_prepare_coin_piles(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, coin_transfer_result *result,
	std::vector<flatfile_authority_after_image> *images, unsigned int *result_code,
	std::string *error);
flatfile_item_repository_result flatfile_item_repository_list_collector_items_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<item_ownership_runtime_entry> *items, std::string *error);
flatfile_item_repository_result flatfile_item_repository_list_active_player_items(
	const std::string &root, std::vector<flatfile_item_ownership_record> *items,
	std::string *error);
// The offering result and this continuation are committed in one authority
// image. Acknowledgement is separate and only follows completion of all
// promised reward effects.
flatfile_item_repository_result flatfile_item_repository_pending_quest_rewards(
	const std::string &root, uint32_t player_pid,
	std::vector<flatfile_quest_reward_obligation> *obligations, std::string *error,
	std::vector<flatfile_quest_xp_entitlement> *entitlements = nullptr,
	player_revision_t durable_revision = UINT64_MAX);
flatfile_item_repository_result flatfile_item_repository_pending_quest_rewards_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t player_pid,
	std::vector<flatfile_quest_reward_obligation> *obligations, std::string *error,
	std::vector<flatfile_quest_xp_entitlement> *entitlements,
	player_revision_t durable_revision);
// Stage application markers with the XP-bearing player image under the same
// authority lock. Existing markers must precede the player's durable revision.
// Verification never prepares an image or advances a marker.
flatfile_item_repository_result flatfile_item_repository_prepare_quest_xp_receipts(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t player_pid,
	player_revision_t durable_revision, player_revision_t applied_revision,
	const std::vector<player_quest_xp_receipt_snapshot> &receipts, bool verify_only,
	flatfile_authority_operation *operation, std::string *error);
flatfile_item_repository_result
flatfile_item_repository_ack_quest_reward(const std::string &root, uint32_t player_pid,
					  const critical_operation_id &offering_operation,
					  std::string *error);
flatfile_item_baseline_result
flatfile_item_repository_establish_owner(const std::string &root, const item_owner_identity &owner,
					 const std::vector<flatfile_item_ownership_record> &items,
					 std::string *error);
critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command);
// Exact retained receipt lookup only. The caller holds native authority; a
// missing operation is a refusal, never an instruction to create the item.
critical_apply_result flatfile_item_repository_verify_creation_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_auction_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const auction_command_payload &payload, uint32_t auction_id, bool to_auction,
	bool require_isolated_roots, flatfile_item_auction_mutation *mutation,
	unsigned int *result_code, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_shop_trade(
	const std::string &root, const flatfile_authority_lock &lock,
	const shop_trade_payload &payload, flatfile_item_shop_trade_mutation *mutation,
	unsigned int *result_code, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_collector_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const collector_command_payload &payload, flatfile_item_collector_mutation *mutation,
	unsigned int *result_code, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload,
	const std::vector<flatfile_corpse_custody_item> &expected_items,
	flatfile_item_corpse_release_mutation *mutation, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_world_corpse_raise(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload,
	const std::vector<flatfile_corpse_custody_item> &expected_items,
	const std::vector<uint64_t> &durable_uids, const std::vector<uint64_t> &discarded_uids,
	flatfile_item_corpse_release_mutation *mutation, std::string *error);
flatfile_item_repository_result flatfile_item_repository_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error);
/* Retain refused death custody without allowing normal inventory materialization. */
flatfile_item_repository_result flatfile_item_repository_prepare_death_quarantine(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<uint64_t> &custody_uids, flatfile_authority_operation *operation,
	std::string *error, const std::vector<player_quest_xp_receipt_snapshot> &receipts = {},
	player_revision_t durable_revision = 0, player_revision_t applied_revision = 0);
/* Prepare player and verified locker-custody removal as one authority image. */
flatfile_item_repository_result flatfile_item_repository_prepare_player_and_locker_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	flatfile_authority_operation *operation, std::string *error);
/* Prepare removal of verified locker custody without a player owner. */
flatfile_item_repository_result flatfile_item_repository_prepare_locker_remove(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	flatfile_authority_operation *operation, std::string *error);
/* Prepare player, locker, and corpse custody removal as one authority image. */
flatfile_item_repository_result flatfile_item_repository_prepare_player_and_custody_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	const std::vector<flatfile_corpse_custody_owner> &corpse_custody,
	flatfile_authority_operation *operation, std::string *error);
critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context);

// Validate a progression obligation against its committed item root while the
// caller holds the same authority cut as the player snapshot and receipt.
flatfile_item_repository_result flatfile_item_repository_craft_root_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, std::string *error);

#endif
