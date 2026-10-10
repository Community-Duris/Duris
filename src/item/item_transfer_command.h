#ifndef ITEM_TRANSFER_COMMAND_H
#define ITEM_TRANSFER_COMMAND_H

#include "persistence/critical_command.h"
#include "economy/shop_trade_recovery_manifest.h"
#include "world/quest_mobile_native_reference.h"
#include "economy/native_quest_cost.h"
#include "economy/native_quest_coin_give.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr uint16_t ITEM_TRANSFER_PAYLOAD_VERSION = 10;
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION = 11;
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION = 12;
// Explicit value-only cash successors. The old v11/v12 body is length-bound
// unchanged inside the wrapper; execution remains original owner-authorized.
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION = 13;
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION = 14;
// Explicit genuine zero-item coin acceptance. Neither value version admits an
// execution, source, checkpoint or physical publication.
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION = 15;
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION = 16;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES = 64;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES = 24;
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_VERSION = 2;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES = 24;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES = 12;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES = 65536;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MIN_BYTES =
	ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES +
	2 * SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
	ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS = 3000;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MAX_BYTES =
	ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MIN_BYTES +
	(2 * SHOP_TRADE_RECOVERY_MAX_UIDS + ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS) *
		sizeof(uint64_t) +
	2 * (ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES - 1);
constexpr uint16_t ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_VERSION = 1;
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES = 168;
constexpr uint16_t ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION = 9;
constexpr uint16_t ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION = 8;
constexpr uint16_t ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION = 7;
constexpr uint16_t ITEM_TRANSFER_BATCH_PAYLOAD_VERSION = 6;
constexpr uint16_t ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION = 5;
constexpr uint16_t ITEM_TRANSFER_EXACT_PAYLOAD_VERSION = 4;
constexpr uint16_t ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION = 3;
constexpr uint16_t ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION = 2;
constexpr size_t ITEM_TRANSFER_LEGACY_MAX_ITEMS = 12;
constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 3000;
static_assert(ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS == ITEM_TRANSFER_MAX_ITEMS);
constexpr size_t ITEM_TRANSFER_HEADER_BYTES = 96;
constexpr size_t ITEM_TRANSFER_ENTRY_BYTES = 40;
constexpr size_t ITEM_TRANSFER_PAYLOAD_BYTES =
	ITEM_TRANSFER_HEADER_BYTES + ITEM_TRANSFER_LEGACY_MAX_ITEMS * ITEM_TRANSFER_ENTRY_BYTES;
constexpr size_t ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES = 128 * 1024;
constexpr size_t ITEM_TRANSFER_CONTINUATION_MAX_BYTES = 8 * 1024;
constexpr size_t ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES = 128 * 1024;
constexpr size_t ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES = 255;
constexpr size_t ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES = 512;
constexpr size_t ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES = 64 * 1024;
constexpr size_t ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES = 512;
constexpr size_t ITEM_TRANSFER_LEGACY_RESULT_BYTES = 40;
constexpr size_t ITEM_TRANSFER_RESULT_BYTES = 48;
constexpr uint64_t ITEM_TRANSFER_ABSENT_REVISION = UINT64_MAX;
constexpr uint16_t ITEM_TRANSFER_MAX_EQUIPMENT_SLOT = 43;

enum class item_owner_type : uint8_t
{
	unknown = 0,
	player,
	container,
	room,
	corpse,
	locker,
	auction,
	system,
	destruction,
	shopkeeper,
	collector,
	pet,
	// Reserved native NPC lifetime, distinct from runtime IDs and pet ownership.
	native_mobile = 12,
};

enum class item_transfer_reason : uint16_t
{
	unknown = 0,
	synthetic,
	creation,
	destruction,
	operator_repair,
	player_get,
	player_drop,
	player_put,
	player_give,
	corpse_create,
	corpse_restore,
	corpse_loot,
	locker_deposit,
	locker_withdraw,
	auction_list,
	auction_claim,
	shop_buy,
	shop_sell,
	mobile_claim,
	collector_collect,
	collector_buyback,
	collector_expire,
	death_restitution,
	corpse_raise_pet,
	pet_give,
	pet_return,
	// Trusted theft is still a player-to-player custody move.  Keeping a
	// distinct reason preserves the audit trail without weakening the generic
	// player-owner validation used by the transfer repositories.
	trusted_steal,
	// These existing-item handoffs have command-specific post-commit effects.
	soulbind,
	slip,
	player_wear,
	player_remove,
	combat_fumble,
	critical_disarm,
	// Quest turn-ins retire the submitted UIDs under a quest-specific reason and
	// retain the reward continuation with the same durable operation.
	quest_turnin,
	// Atomic retirement and admission of detached crafted outputs.
	craft,
	// Player offering accepted by an addressed native NPC, before quest consumption.
	quest_offering,
};

constexpr bool item_transfer_forced_weapon_drop(item_transfer_reason reason)
{
	return reason == item_transfer_reason::combat_fumble ||
	       reason == item_transfer_reason::critical_disarm;
}

enum class item_custody_state : uint8_t
{
	absent = 0,
	active,
	destroyed,
	quarantined,
};

struct item_owner_identity
{
	item_owner_type type;
	uint64_t id;
	uint64_t context_id;
};

struct item_transfer_entry
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	uint64_t expected_item_revision;
	int32_t vnum;
	item_custody_state expected_state;
};

struct item_corpse_metadata
{
	bool present = false;
	int32_t room_vnum = 0;
	int32_t weight = 0;
	uint8_t actor_racewar = 0;
	std::array<int32_t, 8> values = {};
	std::string owner_name;
	std::string short_description;
	std::string description;
	std::string keywords;
};

// A player-death corpse handoff may carry a collector-intake sidecar. The
// sidecar is committed in the same authority transaction as the custody move;
// its eligible UIDs are an exact, sorted subset of this command's item rows.
struct item_collector_death_policy
{
	uint64_t collection_delay = 0;
	uint64_t sale_delay = 0;
	uint64_t holding_duration = 0;
	uint64_t price_percent = 0;
	uint64_t minimum_value = 0;
};

struct item_collector_death_enrollment
{
	bool present = false;
	critical_operation_id death_operation = {};
	uint32_t beneficiary_pid = 0;
	uint64_t death_time = 0;
	item_collector_death_policy policy = {};
	std::vector<uint64_t> eligible_item_uids;
};

enum class item_transfer_continuation_kind : uint32_t
{
	none = 0,
	quest_offering = 1,
	soulbind_transfer = 2,
	spell_component_retirement = 3,
	account_reward_retirement = 4,
	account_reward_duplicate_promotion = 5,
	craft_pouch_usage = 6,
	craft_recipe = 7,
	lockpick_retirement = 8,
};

enum class item_spell_component_effect : uint32_t
{
	faerie_sight = 1,
	spore_burst_initial = 2,
	spore_burst_repeat = 3,
	summon_insects = 4,
	wall_of_bones = 5,
	vines = 6,
};

constexpr size_t item_transfer_continuation_limit(item_transfer_continuation_kind kind)
{
	return kind == item_transfer_continuation_kind::craft_recipe ?
		       ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES + 52 :
	       kind == item_transfer_continuation_kind::craft_pouch_usage ?
		       ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES :
		       ITEM_TRANSFER_CONTINUATION_MAX_BYTES;
}

struct item_transfer_continuation
{
	item_transfer_continuation_kind kind = item_transfer_continuation_kind::none;
	std::vector<uint8_t> data;
};

enum class item_native_mobile_action : uint8_t
{
	acceptance = 1,
	consumption = 2,
};

struct item_native_mobile_context
{
	bool present = false;
	quest_mobile_native_reference reference;
	item_native_mobile_action action = {};
	uint32_t final_giver_pid = 0;
};

// Immutable value evidence only. The original quest hold/checkpoint and source
// owner must authenticate the PID/save fence and complete literal forest cut.
// Consumption has no player item mutation: both forest bindings are absent.
struct item_native_quest_publication_terms
{
	std::string message;
	std::string disappear_message;
	bool echo_all = false;
	bool disappear = false;
};

struct item_native_mobile_cost_context
{
	bool present = false;
	bool fee_only = false;
	uint32_t completion_slot = 0;
	uint64_t wallet_mapping_id = 0;
	native_quest_cost_projection projection;
};

struct item_native_mobile_money_context
{
	bool present = false;
	int32_t original_room_vnum = 0;
	uint64_t player_wallet_mapping_id = 0;
	uint64_t mobile_wallet_mapping_id = 0;
	native_quest_coin_give_projection projection;
};

struct item_native_mobile_recovery_context
{
	bool present = false;
	uint32_t player_pid = 0;
	uint64_t acknowledged_save_revision = 0;
	shop_trade_recovery_forest_binding player_before;
	shop_trade_recovery_forest_binding player_after;
	// Original ITEM pass then TYPE pass decision; never UID/native order.
	// Empty for acceptance, including an empty no-reward continuation prefix.
	std::vector<uint64_t> consumed_root_order;
	item_native_quest_publication_terms publication_terms;
};

struct item_transfer_payload
{
	item_owner_identity from_owner;
	item_owner_identity to_owner;
	item_transfer_reason reason;
	int64_t reason_id;
	// Stable issuance identity for a sourced creation; zero uses the item UID lifetime.
	uint64_t logical_source_id = 0;
	uint64_t expected_from_revision;
	uint64_t expected_to_revision;
	uint64_t selected_item_uid;
	uint64_t target_root_item_uid;
	uint64_t target_parent_item_uid;
	uint64_t expected_target_parent_revision;
	bool multi_root;
	uint16_t item_count;
	std::array<item_transfer_entry, ITEM_TRANSFER_MAX_ITEMS> items;
	uint32_t item_blob_size;
	std::array<uint8_t, ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES> item_blob;
	item_corpse_metadata corpse;
	item_collector_death_enrollment collector;
	item_transfer_continuation continuation;
	item_native_mobile_context native_mobile;
	item_native_mobile_recovery_context native_recovery;
	item_native_mobile_cost_context native_cost = {};
	item_native_mobile_money_context native_money = {};
};

// Distinct zero-item fee result. Scope identities do not transfer any item.
struct item_native_mobile_fee_result
{
	uint64_t mobile_instance_id = 0;
	uint32_t player_pid = 0;
	uint64_t mobile_cash_revision = 0, mobile_revision = 0, stock_revision = 0;
	uint64_t native_custody_revision = 0, player_custody_revision = 0;
	bool operator==(const item_native_mobile_fee_result &) const = default;
};
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES = 64;
bool item_native_mobile_fee_result_build(const item_transfer_payload &,
					 item_native_mobile_fee_result *) noexcept;
bool item_native_mobile_fee_result_encode(
	const item_native_mobile_fee_result &,
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> *) noexcept;
bool item_native_mobile_fee_result_decode(std::span<const uint8_t>,
					  item_native_mobile_fee_result *) noexcept;

// Dedicated zero-item money result; legacy item-result framing stays strict.
struct item_native_mobile_money_result
{
	uint64_t mobile_instance_id = 0;
	uint32_t player_pid = 0;
	uint64_t player_wallet_revision = 0, mobile_cash_revision = 0;
	uint64_t mobile_revision = 0, player_custody_revision = 0, stock_revision = 0;
	bool operator==(const item_native_mobile_money_result &) const = default;
};
constexpr size_t ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES = 72;
// Pure expected result and canonical value transport only; the original durable
// root must authenticate real applied revisions and bind this exact result.
bool item_native_mobile_money_result_build(const item_transfer_payload &,
					   item_native_mobile_money_result *) noexcept;
bool item_native_mobile_money_result_encode(
	const item_native_mobile_money_result &,
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES> *) noexcept;
bool item_native_mobile_money_result_decode(std::span<const uint8_t>,
					    item_native_mobile_money_result *) noexcept;

struct item_transfer_result
{
	uint64_t root_item_uid;
	uint16_t item_count;
	uint64_t from_owner_revision;
	uint64_t to_owner_revision;
	uint64_t max_item_revision;
	uint64_t corpse_revision;
	// Set only when the same durable authority commit also changed collector
	// metadata. The game thread uses this replay-safe flag to invalidate its
	// asynchronous collector projection after the item result is published.
	bool collector_catalog_changed = false;
	// Populated by the game-thread publication path from the coordinator receipt.
	// It is deliberately not part of the encoded domain result.
	critical_operation_id operation_id = {};
};

// Internal classification returned by the SQL executor. The enclosing coin
// command maps these bounded flags to source/destination stages before the
// failure receipt is persisted.
enum class item_transfer_failure_stage : uint8_t
{
	none = 0,
	from_owner_revision = 1u << 0,
	to_owner_revision = 1u << 1,
	item_revision = 1u << 2,
	target_parent_revision = 1u << 3,
	coin_payload_revision = 1u << 4,
};

bool item_owner_identity_valid(const item_owner_identity &owner);
bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right);
uint64_t item_transfer_selected_root(const item_transfer_payload &payload, uint64_t item_uid);
bool item_transfer_selected_roots(const item_transfer_payload &payload,
				  std::vector<uint64_t> *roots);
uint64_t item_transfer_result_root(const item_transfer_payload &payload);
bool item_transfer_target_topology(const item_transfer_payload &payload, uint64_t item_uid,
				   uint64_t *root_item_uid, uint64_t *parent_item_uid);
uint64_t item_corpse_owner_id(uint32_t player_pid, uint32_t corpse_save_id);
uint64_t item_shopkeeper_owner_id(uint32_t shop_id);
uint64_t item_collector_owner_id(uint64_t listing_id);
bool item_owner_key(const item_owner_identity &owner, critical_entity_key *key);
bool item_transfer_command_encode_payload(const item_transfer_payload &payload,
					  std::vector<uint8_t> *encoded);
// Native v11 only; the default encoder/builder continue to emit v10.
// Pure value shape is not source/epoch/admission/backend execution authority.
// The fixed tail follows continuation: LE version16/action8/flags8=0/length32,
// final_giver32/zero32, original 148-byte reference, then four zero padding bytes.
// Wire classification only. Original source/epoch/native mapping/root/held
// receipt/publication proof remains mandatory; these do not grant admission.
bool item_transfer_native_mobile_structural_version(uint16_t) noexcept;
bool item_transfer_native_mobile_acknowledged_version(uint16_t) noexcept;
bool item_transfer_native_mobile_shape_valid(const item_transfer_payload &) noexcept;
bool item_transfer_command_encode_native_mobile(const item_transfer_payload &,
						std::vector<uint8_t> *encoded) noexcept;
bool item_transfer_command_build_native_mobile(critical_command *, critical_operation_id,
					       const item_transfer_payload &, critical_source_site,
					       critical_deadline_class) noexcept;
// Explicit v12 successor; v11/context1 and default v10 remain literal.
// Tail: context1, then LE recovery version16/zero16/length32/PID32/zero32/
// acknowledged_save_revision64, followed by existing role1/role2 forest frames.
bool item_transfer_native_mobile_recovery_shape_valid(const item_transfer_payload &) noexcept;
bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items) noexcept;
// Consumption needs its independently sealed original decision order. The
// four-argument compatibility entry remains and refuses a missing order.
bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items,
	std::span<const uint64_t> consumed_root_order) noexcept;
bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items,
	std::span<const uint64_t> consumed_root_order,
	const item_native_quest_publication_terms &) noexcept;
bool item_transfer_command_encode_native_mobile_recovery(const item_transfer_payload &,
							 std::vector<uint8_t> *encoded) noexcept;
bool item_transfer_command_build_native_mobile_recovery(critical_command *, critical_operation_id,
							const item_transfer_payload &,
							critical_source_site,
							critical_deadline_class) noexcept;
bool item_transfer_command_decode_payload(const critical_command &command,
					  item_transfer_payload *payload);
bool item_transfer_command_encode_result(const item_transfer_result &result,
					 std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> *encoded);
bool item_transfer_command_decode_result(const uint8_t *encoded, size_t size,
					 item_transfer_result *result);
bool item_transfer_command_build(critical_command *command, critical_operation_id operation_id,
				 const item_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class);

// Allocation-free actual payload heap and fresh generated-copy request census,
// under libstdc++13 C++11 ABI nondebug C++20. Excludes payload inline/caller
// owners/allocator metadata; strong scalar output. No semantic authority.
bool item_transfer_payload_current_heap_bytes(const item_transfer_payload &, size_t *) noexcept;
bool item_transfer_payload_fresh_copy_request_bytes(const item_transfer_payload &,
						    size_t *) noexcept;
size_t item_transfer_payload_copy_frame_bytes() noexcept;
// Full private generated payload copy with all twelve member requests admitted
// before allocation, then nonthrowing strong-output move. Outer owns actual
// source/prior output and all other live caller owners. Unselected companion.
bool item_transfer_payload_clone_bounded(const item_transfer_payload &, item_transfer_payload *,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept;

// Complete original corpse/collector value wire codecs with strong output and
// genuine prospective requests. Outer owns input/prior output/all other actual
// caller owners; excludes the callee private candidate. No semantic/custody/
// publication authority; original active codecs unchanged.
bool item_transfer_corpse_context_encode_bounded(const item_corpse_metadata &,
						 std::vector<uint8_t> *,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live) noexcept;
bool item_transfer_corpse_context_decode_bounded(const uint8_t *, size_t, item_corpse_metadata *,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live) noexcept;
bool item_transfer_collector_context_encode_bounded(const item_collector_death_enrollment &,
						    std::vector<uint8_t> *,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer_live) noexcept;
bool item_transfer_collector_context_decode_bounded(const uint8_t *, size_t,
						    item_collector_death_enrollment *,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer_live) noexcept;

// Exact original owner key companion; fixed-context SHA preserves the original
// 17-byte owner wire and little-endian first digest word. Outer owns authentic
// owner/output and caller storage. Strong output; no semantic authority.
bool item_owner_key_bounded(const item_owner_identity &, critical_entity_key *,
			    bool (*reserve)(size_t, void *) noexcept, void *context,
			    size_t outer_live) noexcept;

// Full original native recovery forests/consumed-root/publication wire codecs.
// Outer owns authentic input/prior output and other caller state. Private real
// heaps remain current at every request; outputs/actual retained scalars strong.
// Pure companions only; original native authority/refusal/dispatch remain required.
bool item_transfer_native_recovery_encode_bounded(
	const item_native_mobile_recovery_context &, std::vector<uint8_t> *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_encoded_heap_bytes = nullptr) noexcept;
bool item_transfer_native_recovery_decode_bounded(
	std::span<const uint8_t>, item_native_mobile_recovery_context *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_recovery_heap_bytes = nullptr) noexcept;

// Complete original native recovery value validator, including full selected
// item-list and ordered forest/quest-continuation laws.
// Outer owns authentic input and other caller state; no native authority or route.
bool item_transfer_native_recovery_valid_bounded(const item_transfer_payload &,
						 const item_native_mobile_recovery_context &,
						 uint16_t, bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live) noexcept;

struct player_item_snapshot;
// Complete original allocating continuation/craft-output validation leaves.
// Actual private selected-row, quest-string and child-UID lifetimes are admitted
// prospectively. Outer owns source/prior destination; no execution authority.
bool item_transfer_quest_offering_continuation_valid_bounded(
	const item_transfer_payload &, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;
bool item_transfer_duplicate_promotion_continuation_valid_bounded(
	const item_transfer_payload &, uint16_t, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;
bool item_transfer_craft_outputs_decode_bounded(
	const item_transfer_payload &, std::vector<player_item_snapshot> *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_item_heap_bytes = nullptr) noexcept;

// Complete original native mobile context predicate, with genuine fixed
// reference codec and full item-list canonical/DFS/UID proof. Source/prior caller
// owners stay outer; no UID issuance/native execution/admission authority or route.
bool item_transfer_native_mobile_context_valid_bounded(const item_transfer_payload &, uint16_t,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept;

// Complete original generic payload validator, including all original native,
// continuation/craft/corpse/collector/pet/multi-root/topology refusals in order.
// Pure unselected companion; authentic input/sibling/caller owners stay outer.
bool item_transfer_payload_valid_bounded(const item_transfer_payload &, uint16_t,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept;

#endif
