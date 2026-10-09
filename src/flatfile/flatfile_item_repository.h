#ifndef DURIS_FLATFILE_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_ITEM_REPOSITORY_H

#include "economy/zone_reset_item_accounting.h"
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
#include "economy/native_mobile_birth_cash_role_accounting.h"

#include "flatfile/flatfile_store.h"
#include "world/quest_mobile_native.h"
#include <cstdint>
#include <span>
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
// Pure complete prepared-catalog value read, including inactive historical rows.
// The two original participants' clocks are read from that same canonical image.
// No custody/storage/publication authority is granted; failure preserves outputs.
flatfile_item_repository_result flatfile_item_repository_read_trade_after_image(
	const flatfile_authority_after_image &image,
	const std::array<item_owner_identity, 2> &owners, std::array<uint64_t, 2> *owner_revisions,
	std::vector<flatfile_item_ownership_record> *records, std::string *error);
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
// Protected recovery inspection needs inactive rows and unmatched descendants.
// This is a read of existing authority, never a repair or a new custody store.
flatfile_item_repository_result flatfile_item_repository_recovery_catalog_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_item_ownership_record> *items, std::string *error);
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
// Capture all physically stored room money, including saved-item records,
// and require exact active root custody and literal agreement. Refuses physical
// world/locker or catalog money in unsupported owners. This does not enumerate
// unindexed player/pet snapshots: lifecycle must inspect its selected snapshots.
// Borrows the lock, recovers only the existing journal, publishes no evidence,
// and leaves outputs unchanged on refusal. Aggregate cash is a separate domain.
unsigned int flatfile_item_repository_capture_room_coin_piles_locked(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_coin_pile_source> *sources, std::string *error) noexcept;
// Verify the original native custody catalog's exact COIN root digest/result.
// Borrows the authority lock and recovers existing journal only; never applies
// a command, creates a catalog receipt, mutates custody, or grants an ACK.
unsigned int flatfile_item_repository_verify_coin_root_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, std::span<const uint8_t> retained_result,
	std::string *error);
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

struct item_transfer_custody_delta;
struct quest_mobile_native_image;
// Borrow the already recovered original authority lock. Root authenticates
// admission/source and commits the returned native/catalog/player-removal
// operations with evidence/references/receipt in ONE authority bundle.
// Never selected by generic execution, never commits or fabricates generations.
// Successful refusal sets result_code and publishes no output operations.
flatfile_item_repository_result flatfile_item_repository_prepare_native_mobile(
	const std::string &root, const flatfile_authority_lock &, const critical_command &,
	std::span<const player_item_snapshot> original_player_items, item_transfer_result *,
	unsigned int *result_code, item_transfer_custody_delta *, quest_mobile_native_image *after,
	std::vector<flatfile_authority_operation> *operations, std::string *error);

struct smith_native_compound_images;
struct smith_native_player_grant_projection;
struct player_snapshot;
struct economic_account_key;
struct economic_accounting_plan;
class flatfile_smith_native_observation
{
	friend class smith_native_compound_owner;
	// Pure observation of the actual ALL-component file under the original
	// recovered lock. The revision is original observed ACK DATA: the owner
	// authenticates its ACK chain/lifetime. This helper grants no ACK/admission,
	// custody, save completion, publication or release authority.
	static flatfile_item_repository_result
	capture_persisted_before(const std::string &root, const flatfile_authority_lock &,
				 uint32_t pid, uint64_t original_observed_ack_revision,
				 player_snapshot *full_persisted_before, std::string *error);
};
// Stages only under the original recovered authority lock. The original owner
// authenticates retained save ACK/source/carrier, adds references/evidence/root
// receipt, and commits ONE bundle. No generic Smith17 support or replay here.
// Every output is unchanged on refusal; the returned images are not authority.
flatfile_item_repository_result flatfile_item_repository_prepare_smith_native(
	const std::string &root, const flatfile_authority_lock &, const critical_command &,
	const smith_native_compound_images &, const smith_native_player_grant_projection &,
	const player_snapshot &original_acknowledged_filtered_save,
	const player_snapshot &original_full_persisted_before,
	const economic_account_key &original_player_wallet, item_transfer_result *,
	unsigned int *result_code, item_transfer_custody_delta *, economic_accounting_plan *,
	std::vector<flatfile_authority_operation> *operations, std::string *error);

// Passive proposed initial shared custody participant. Real before owner
// presence distinguishes absent from present-zero. Zero stock leaves both
// owner and catalog exactly unchanged and returns has_operation=false.
// Values/plan alone grant no native/source/bundle/publication authority.
struct flatfile_shared_shop_initial_custody_stage
{
	native_mobile_birth_shared_shop_participant participant;
	economic_accounting_plan plan;
	bool catalog_before_present = false, catalog_after_present = false;
	uint64_t catalog_before_revision = 0, catalog_after_revision = 0;
	bool has_operation = false;
	flatfile_authority_operation operation;
};
class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
class flatfile_shared_shop_initial_custody_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
	// The genuine root owns original admission/source/epoch/constructor proof,
	// journal recovery, the SAME borrowed lock and sole atomic bundle/receipt.
	// This stage authenticates the complete original INITIAL carrier/checkpoint,
	// actual absent native/SHOP and selected empty active owner cut. Every born
	// UID/history/root/parent must be absent. Preserve unrelated catalog/history.
	// No recovery, acquire, commit, receipt, publication, ACK or generic route.
	// ITEM_MONEY properties remain in the same full native/SHOP literals; this
	// role does not invent a coin mutation or general spending permission.
	// Every refusal preserves the caller's complete output.
	static flatfile_item_repository_result
	prepare_locked(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_native_recovery_envelope &original,
		       flatfile_shared_shop_initial_custody_stage *output,
		       std::string *error) noexcept;
};

// Passive exact CURRENT initial shared birth custody, not an admission,
// source, native-body or publication capability. Whole catalog observation
// retains claims hidden by an active exact-owner inventory projection.
struct flatfile_shared_shop_current_custody
{
	bool owner_present = false;
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> rows;
};
// Untrusted full-store framing and allocation requests, not authenticated custody.
// Full v1-v8 catalog, every coin literal and continuation is scanned without
// allocation or digest calls. Original decode_catalog/valid_catalog still run
// AFTER admission. Inline objects and requested payload/capacity are counted;
// malloc metadata/OpenSSL internals and process stack frames are excluded.
// Supported requests are pinned to release-13 libstdc++, C++11 string ABI.
struct flatfile_item_catalog_allocation_profile
{
	uint32_t format_version = 0, owner_count = 0, item_count = 0, operation_count = 0;
	size_t decoded_catalog_payload_bytes = 0; // Includes one catalog object.
	size_t validation_working_bytes = 0; // Sequential coin/quest/hash peak.
	size_t authenticated_decode_working_bytes = 0; // Includes local decoded catalog.
	size_t framing_working_object_bytes = 0; // Named scan DTO/decoder objects only.
	bool storage_policy_supported = false;
};
flatfile_item_repository_result
flatfile_item_catalog_preflight(std::span<const uint8_t>,
				flatfile_item_catalog_allocation_profile *) noexcept;
size_t flatfile_item_catalog_preflight_object_bytes() noexcept;
class flatfile_shared_shop_current_custody_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
	// Caller owns genuine original envelope/storage receipt provenance and
	// the SAME already-recovered exclusive lock. Verify canonical original
	// command/checkpoint/MBR4/plan and exact whole current ownership catalog.
	// No born UID/history/foreign root/parent or crossed SHOP/native claim
	// may hide outside the selected active forest. Unrelated history remains.
	// Native body/cash/AF/literal proof remains a separate original root cut.
	// Strong output, no acquire/recover/stage/commit or generic route changes.
	// The genuine SAME-lock transaction MUST authenticate the full original
	// carrier, checkpoint, result, plan and receipt before calling this sibling.
	// Already-live original references and caller output capacities belong in outer.
	// References alone grant no authority. Callback admits absolute prospective
	// scratch; caller holds its maximum through output transfer and restores it
	// only after temporary destruction. No diagnostic allocation or independent
	// budget. Original read_locked remains byte-identical and available.
	static flatfile_item_repository_result read_original_projection_locked_bounded(
		const std::string &, const flatfile_authority_lock &,
		const quest_mobile_native_image &original_birth,
		const native_mobile_birth_shared_shop_participant &original_participant,
		const economic_accounting_plan &original_plan,
		const critical_operation_id &original_operation,
		flatfile_shared_shop_current_custody *, flatfile_scratch_reserve_fn, void *context,
		size_t outer_live_scratch) noexcept;
	static flatfile_item_repository_result
	read_locked(const std::string &, const flatfile_authority_lock &,
		    const critical_native_recovery_envelope &,
		    std::span<const uint8_t> stored_result, std::span<const uint8_t> stored_plan,
		    flatfile_shared_shop_current_custody *, std::string *) noexcept;
};

// Passive native ROOM proposal; values alone grant no source, season,
// execution, receipt, commit or publication permission. The genuine atomic
// owner must authenticate the installed root/epoch and original source cut.
struct flatfile_initial_room_reset_custody_stage
{
	bool catalog_before_present = false, owner_before_present = false;
	uint64_t catalog_revision_before = 0, catalog_revision_after = 0;
	uint64_t owner_revision_before = 0, owner_revision_after = 0;
	economic_accounting_plan plan;
	item_transfer_result result{};
	flatfile_authority_operation operation;
};
class flatfile_initial_room_reset_custody_storage final
{
    private:
	friend class flatfile_accounting_zone_reset_item_transaction;
	// Consume the genuine SAME-lock world proposal and exact original carrier;
	// prove complete active ROOM topology and every born UID/history/root/parent
	// absence over the full custody catalog. No generic transfer receipt/history.
	// Full money literals are retained; pile head/source/accounting/root receipt
	// remain the sole atomic owner's same-bundle obligations. Strong output.
	static flatfile_item_repository_result
	prepare_locked(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_native_recovery_envelope &original,
		       const flatfile_initial_room_reset_world_stage &world,
		       flatfile_initial_room_reset_custody_stage *output) noexcept;
};

// Exact CURRENT original ROOM-reset custody observation. These returned rows
// grant no source, recovery, transaction, publication or ACK permission.
class flatfile_room_reset_current_custody_storage final
{
    private:
	friend class flatfile_accounting_zone_reset_item_transaction;
	// The genuine transaction supplies its original full carrier and canonical
	// durable typed48/compiled plan plus actual complete selected DURWRLD room
	// read under this SAME recovered exclusive selected-root lock. This helper
	// independently validates command/source metadata, receipt core, original
	// born literals, whole current ROOM forest and complete custody history.
	// Returned rows are only original born rows, never an active-only proof.
	// Caller owns prospective aggregate/copy lifetimes; this ordinary observer
	// is not a bounded reader. No acquire/recover/stage/commit/receipt mutation.
	// Every failure preserves caller output, including allocation failure.
	static flatfile_item_repository_result
	read_locked(const std::string &, const flatfile_authority_lock &,
		    const critical_native_recovery_envelope &,
		    std::span<const uint8_t> stored_typed48,
		    std::span<const uint8_t> stored_compiled_plan,
		    const flatfile_room_item_record &actual_room,
		    std::vector<flatfile_item_ownership_record> *output) noexcept;
};

#endif
