#ifndef DURIS_ECONOMIC_ACCOUNTING_PLAN_H
#define DURIS_ECONOMIC_ACCOUNTING_PLAN_H

#include "economy/economic_accounting_types.h"

#include <optional>
#include <vector>

// These numbers are the versioned contract registry, not caller-supplied names.
enum class economic_reason : uint16_t
{
	bank_transfer = 1,
	wallet_transfer = 2,
	coin_transfer = 3,
	group_split = 4,
	quest_reward = 5,
	npc_reward = 6,
	chaos_reward = 7,
	starter_reward = 8,
	boon_reward = 9,
	achievement_reward = 10,
	service_cost = 11,
	training_cost = 12,
	locker_cost = 13,
	shipping_cost = 14,
	insurance_cost = 15,
	guild_cost = 16,
	crafting_cost = 17,
	gambling_stake = 18,
	gambling_payout = 19,
	refund = 20,
	shop_buy = 21,
	shop_sell = 22,
	shop_cleanup = 23,
	collector_purchase = 24,
	collector_custody = 25,
	auction_listing = 26,
	auction_bid = 27,
	auction_outbid = 28,
	auction_cancel = 29,
	auction_settle = 30,
	auction_claim = 31,
	item_move = 32,
	item_create = 33,
	item_destroy = 34,
	first_admission = 35,
	death_transfer = 36,
	corpse_restore = 37,
	baseline = 38,
	correction = 39,
	restitution = 40,
	lifecycle_retirement = 41,
	epoch_transition = 42,
};

enum class economic_actor_kind : uint8_t
{
	domain = 1,
	operator_action = 2
};
enum class economic_source_kind : uint16_t
{
	quest_completion = 1,
	npc_generation = 2,
	starter_grant = 3,
	boon = 4,
	achievement = 5,
	gambling_round = 6,
	world_generation = 7,
	crafting = 8,
	administrator = 9,
	baseline = 10,
	legacy_import = 11,
	shop_stock = 12,
	auction = 13,
	corpse = 14,
	correction = 15,
	lifecycle = 16,
	service = 17,
};

constexpr size_t ECONOMIC_SOURCE_EVENT_BYTES = 48;
constexpr size_t ECONOMIC_PLAN_HEADER_BYTES = 256;
using economic_digest = std::array<uint8_t, 32>;

struct economic_source_event
{
	economic_source_kind kind = {};
	critical_operation_id source = {};
	critical_operation_id generation = {};
	uint64_t sequence = 0;
	uint32_t slot = 0;
};

struct economic_plan_metadata
{
	uint16_t version = ECONOMIC_ACCOUNTING_VERSION;
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	critical_operation_id operation_id = {};
	critical_operation_id original_operation_id = {};
	economic_actor_kind actor_kind = {};
	uint64_t actor_id = 0;
	uint32_t writer_id = 0;
	uint32_t policy_version = 1;
	uint32_t compiler_version = 1;
	economic_reason reason = {};
	std::optional<economic_source_event> source_event;
	economic_digest intent_digest = {};
	economic_digest domain_digest = {};
};

struct economic_accounting_plan
{
	economic_plan_metadata metadata;
	std::vector<economic_account_effect> accounts;
	std::vector<economic_coin_posting> postings;
	std::vector<economic_child_link> children;
	std::vector<economic_item_snapshot> items_before;
	std::vector<economic_item_snapshot> items_after;
	std::vector<economic_item_event> item_events;
};

bool economic_source_event_valid(const economic_source_event &event);
economic_accounting_error
economic_source_event_encode(const economic_source_event &event,
			     std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> *encoded);
economic_accounting_error economic_source_event_decode(std::span<const uint8_t> encoded,
						       economic_source_event *event);

// Canonicalize references without netting audit legs or discarding item events.
// Failure leaves the caller's plan/output unchanged. Structural validation does
// not grant a writer capability or establish that supplied effects are actual
// locked authority; domain adapters must prove both before commit.
economic_accounting_error economic_plan_normalize(economic_accounting_plan *plan);
economic_accounting_error economic_plan_validate_structure(const economic_accounting_plan &plan);
economic_accounting_error economic_plan_encode(const economic_accounting_plan &plan,
					       std::vector<uint8_t> *encoded);
economic_accounting_error economic_plan_decode(std::span<const uint8_t> encoded,
					       economic_accounting_plan *plan);
economic_accounting_error economic_plan_digest(const economic_accounting_plan &plan,
					       economic_digest *digest);

// Bind immutable command facts before admission assigns its timestamp. This is
// distinct from the existing full command/receipt hash, which is unchanged.
economic_accounting_error economic_command_binding_digest(const critical_command &command,
							  economic_digest *digest);

#endif
