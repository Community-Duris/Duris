#include "economy/economic_accounting_plan.h"

#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <numeric>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
constexpr size_t ACCOUNT_BYTES = 120, POSTING_BYTES = 48, SNAPSHOT_BYTES = 64, EVENT_BYTES = 128;
constexpr std::array<uint8_t, 4> PLAN_MAGIC = { 'E', 'A', 'P', '1' };
struct reason_rule
{
	economic_reason reason;
	uint32_t accounts;
	economic_actor_kind actor;
	bool source_required;
	bool original_required;
	bool reverse_sink;
};
constexpr reason_rule RULES[] = {
	{ economic_reason::bank_transfer, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::wallet_transfer, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::coin_transfer, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::group_split, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::quest_reward, 134, economic_actor_kind::domain, true, false, false },
	{ economic_reason::npc_reward, 134, economic_actor_kind::domain, true, false, false },
	{ economic_reason::chaos_reward, 134, economic_actor_kind::domain, true, false, false },
	{ economic_reason::starter_reward, 134, economic_actor_kind::domain, true, false, false },
	{ economic_reason::boon_reward, 134, economic_actor_kind::domain, true, false, false },
	{ economic_reason::achievement_reward, 134, economic_actor_kind::domain, true, false,
	  false },
	{ economic_reason::service_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::training_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::locker_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::shipping_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::insurance_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::guild_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::crafting_cost, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::gambling_stake, 2050, economic_actor_kind::domain, true, false, false },
	{ economic_reason::gambling_payout, 2178, economic_actor_kind::domain, true, true, false },
	{ economic_reason::gambling_loss, 2304, economic_actor_kind::domain, true, true, false },
	{ economic_reason::gambling_interruption, 2050, economic_actor_kind::domain, true, true,
	  false },
	{ economic_reason::refund, 382, economic_actor_kind::domain, true, true, true },
	{ economic_reason::shop_buy, 326, economic_actor_kind::domain, true, false, false },
	{ economic_reason::shop_sell, 198, economic_actor_kind::domain, true, false, false },
	{ economic_reason::shop_cleanup, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::collector_purchase, 326, economic_actor_kind::domain, false, false,
	  false },
	{ economic_reason::collector_custody, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_listing, 274, economic_actor_kind::domain, true, false, false },
	{ economic_reason::auction_bid, 306, economic_actor_kind::domain, true, false, false },
	{ economic_reason::auction_outbid, 306, economic_actor_kind::domain, true, false, false },
	{ economic_reason::auction_cancel, 306, economic_actor_kind::domain, true, false, false },
	{ economic_reason::auction_settle, 306, economic_actor_kind::domain, true, false, false },
	{ economic_reason::auction_claim, 306, economic_actor_kind::domain, true, false, false },
	{ economic_reason::item_move, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::item_create, 136, economic_actor_kind::domain, true, false, false },
	{ economic_reason::item_destroy, 264, economic_actor_kind::domain, true, false, false },
	{ economic_reason::first_admission, 138, economic_actor_kind::domain, true, false, false },
	{ economic_reason::death_transfer, 382, economic_actor_kind::domain, true, false, false },
	{ economic_reason::corpse_restore, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::baseline, 638, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::correction, 126, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::restitution, 1150, economic_actor_kind::operator_action, true, true,
	  false },
	{ economic_reason::lifecycle_retirement, 382, economic_actor_kind::operator_action, true,
	  false, false },
	{ economic_reason::epoch_transition, 638, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::item_reward, 394, economic_actor_kind::domain, true, false, false },
	{ economic_reason::quest_cost, 326, economic_actor_kind::domain, true, false, false },
};

static_assert(ECONOMIC_PLAN_HEADER_BYTES + ACCOUNT_BYTES * ECONOMIC_ACCOUNTING_MAX_ACCOUNTS +
		      POSTING_BYTES * ECONOMIC_ACCOUNTING_MAX_POSTINGS +
		      ECONOMIC_CHILD_LINK_BYTES * ECONOMIC_ACCOUNTING_MAX_CHILDREN +
		      2 * SNAPSHOT_BYTES * ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES +
		      EVENT_BYTES * ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS <=
	      ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES);

bool zero(std::span<const uint8_t> bytes)
{
	return std::all_of(bytes.begin(), bytes.end(), [](uint8_t value) { return value == 0; });
}

const reason_rule *rule_for(economic_reason reason)
{
	for (const auto &rule : RULES)
		if (rule.reason == reason)
			return &rule;
	return nullptr;
}

bool source_kind_allowed(economic_reason reason, economic_source_kind kind)
{
	switch (reason)
	{
	case economic_reason::coin_transfer:
		return kind == economic_source_kind::lifecycle;
	case economic_reason::quest_reward:
		return kind == economic_source_kind::quest_completion;
	case economic_reason::quest_cost:
		return kind == economic_source_kind::quest_action;
	case economic_reason::npc_reward:
		return kind == economic_source_kind::npc_generation;
	case economic_reason::chaos_reward:
		return kind == economic_source_kind::starter_grant ||
		       kind == economic_source_kind::world_generation;
	case economic_reason::starter_reward:
		return kind == economic_source_kind::starter_grant;
	case economic_reason::boon_reward:
		return kind == economic_source_kind::boon;
	case economic_reason::achievement_reward:
		return kind == economic_source_kind::achievement;
	case economic_reason::service_cost:
	case economic_reason::training_cost:
	case economic_reason::locker_cost:
	case economic_reason::shipping_cost:
	case economic_reason::insurance_cost:
	case economic_reason::guild_cost:
		return kind == economic_source_kind::service;
	case economic_reason::crafting_cost:
		return kind == economic_source_kind::crafting;
	case economic_reason::gambling_stake:
	case economic_reason::gambling_payout:
	case economic_reason::gambling_loss:
	case economic_reason::gambling_interruption:
		return kind == economic_source_kind::gambling_round;
	case economic_reason::shop_buy:
	case economic_reason::shop_sell:
		return kind == economic_source_kind::shop_stock ||
		       kind == economic_source_kind::item_action;
	case economic_reason::collector_purchase:
		return kind == economic_source_kind::service ||
		       kind == economic_source_kind::item_action;
	case economic_reason::auction_listing:
	case economic_reason::auction_bid:
	case economic_reason::auction_outbid:
	case economic_reason::auction_cancel:
	case economic_reason::auction_settle:
		return kind == economic_source_kind::auction;
	case economic_reason::auction_claim:
		return kind == economic_source_kind::auction ||
		       kind == economic_source_kind::service;
	case economic_reason::death_transfer:
		return kind == economic_source_kind::corpse;
	case economic_reason::item_reward:
		return kind == economic_source_kind::item_action;
	case economic_reason::baseline:
		return kind == economic_source_kind::baseline;
	case economic_reason::lifecycle_retirement:
	case economic_reason::epoch_transition:
		return kind == economic_source_kind::lifecycle;
	default:
		return true;
	}
}

struct writer
{
	std::vector<uint8_t> bytes;
	template <typename T> void integer(T value)
	{
		for (size_t i = 0; i < sizeof(T); ++i)
			bytes.push_back(
				static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i)));
	}
	void block(std::span<const uint8_t> value)
	{
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	void zeros(size_t count) { bytes.insert(bytes.end(), count, 0); }
	void id(const critical_operation_id &value) { block(value.bytes); }
	void coins(const economic_coin_vector &value)
	{
		for (int64_t part : value)
			integer(part);
	}
	void position(const economic_item_position &value)
	{
		integer<uint8_t>(static_cast<uint8_t>(value.owner.type));
		integer<uint8_t>(static_cast<uint8_t>(value.state));
		zeros(6);
		integer(value.owner.id);
		integer(value.owner.context_id);
		integer(value.root_uid);
		integer(value.parent_uid);
		integer(value.revision);
		integer(value.equipment_slot);
		zeros(6);
	}
};

struct reader
{
	std::span<const uint8_t> bytes;
	size_t offset = 0;
	bool good = true;
	std::span<const uint8_t> take(size_t count)
	{
		if (!good || offset > bytes.size() || count > bytes.size() - offset)
		{
			good = false;
			return {};
		}
		const auto result = bytes.subspan(offset, count);
		offset += count;
		return result;
	}
	template <typename T> T integer()
	{
		const auto input = take(sizeof(T));
		uint64_t value = 0;
		for (size_t i = 0; i < input.size(); ++i)
			value |= static_cast<uint64_t>(input[i]) << (8 * i);
		if constexpr (std::is_signed_v<T>)
			return std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(value));
		else
			return static_cast<T>(value);
	}
	void zeros(size_t count)
	{
		if (!zero(take(count)))
			good = false;
	}
	template <size_t N> void block(std::array<uint8_t, N> &output)
	{
		const auto input = take(N);
		if (good)
			std::copy(input.begin(), input.end(), output.begin());
	}
	critical_operation_id id()
	{
		critical_operation_id result = {};
		block(result.bytes);
		return result;
	}
	economic_coin_vector coins()
	{
		economic_coin_vector result = {};
		for (auto &part : result)
			part = integer<int64_t>();
		return result;
	}
	economic_item_position position()
	{
		economic_item_position result = {};
		result.owner.type = static_cast<item_owner_type>(integer<uint8_t>());
		result.state = static_cast<item_custody_state>(integer<uint8_t>());
		zeros(6);
		result.owner.id = integer<uint64_t>();
		result.owner.context_id = integer<uint64_t>();
		result.root_uid = integer<uint64_t>();
		result.parent_uid = integer<uint64_t>();
		result.revision = integer<uint64_t>();
		result.equipment_slot = integer<uint16_t>();
		zeros(6);
		return result;
	}
};

bool sizes_valid(const economic_accounting_plan &plan)
{
	return plan.accounts.size() <= ECONOMIC_ACCOUNTING_MAX_ACCOUNTS &&
	       plan.postings.size() <= ECONOMIC_ACCOUNTING_MAX_POSTINGS &&
	       plan.children.size() <= ECONOMIC_ACCOUNTING_MAX_CHILDREN &&
	       plan.items_before.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES &&
	       plan.items_after.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES &&
	       plan.item_events.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS;
}

size_t encoded_size(const economic_accounting_plan &plan)
{
	return ECONOMIC_PLAN_HEADER_BYTES + ACCOUNT_BYTES * plan.accounts.size() +
	       POSTING_BYTES * plan.postings.size() +
	       ECONOMIC_CHILD_LINK_BYTES * plan.children.size() +
	       SNAPSHOT_BYTES * (plan.items_before.size() + plan.items_after.size()) +
	       EVENT_BYTES * plan.item_events.size();
}

// Canonical topological order: choose the smallest child ID whose parent has
// already been emitted. References are remapped; event identities stay intact.
economic_accounting_error normalize_children(economic_accounting_plan &plan)
{
	const auto original = plan.children;
	std::vector<size_t> remap(original.size() + 1, 0);
	std::vector<bool> used(original.size(), false);
	for (size_t target = 0; target < original.size(); ++target)
	{
		size_t chosen = original.size();
		for (size_t index = 0; index < original.size(); ++index)
		{
			const size_t parent = original[index].parent_index;
			if (parent > original.size())
				return economic_accounting_error::invalid_identity;
			if (used[index] || (parent && !remap[parent]))
				continue;
			if (chosen == original.size() ||
			    original[index].operation_id.bytes <
				    original[chosen].operation_id.bytes)
				chosen = index;
		}
		if (chosen == original.size())
			return economic_accounting_error::topology;
		used[chosen] = true;
		remap[chosen + 1] = target + 1;
		plan.children[target] = original[chosen];
		plan.children[target].parent_index =
			static_cast<uint16_t>(remap[original[chosen].parent_index]);
	}
	for (auto &posting : plan.postings)
	{
		if (posting.child_index >= remap.size())
			return economic_accounting_error::invalid_identity;
		posting.child_index = static_cast<uint16_t>(remap[posting.child_index]);
	}
	for (auto &event : plan.item_events)
	{
		if (event.child_index >= remap.size())
			return economic_accounting_error::invalid_identity;
		event.child_index = static_cast<uint16_t>(remap[event.child_index]);
	}
	return economic_accounting_error::ok;
}

void encode_valid(const economic_accounting_plan &plan, writer &output)
{
	const auto &meta = plan.metadata;
	output.bytes.reserve(encoded_size(plan));
	output.block(PLAN_MAGIC);
	output.integer(meta.version);
	output.zeros(2);
	output.id(meta.lineage);
	output.id(meta.epoch);
	output.id(meta.operation_id);
	output.id(meta.original_operation_id);
	output.integer<uint8_t>(static_cast<uint8_t>(meta.actor_kind));
	output.zeros(3);
	output.integer(meta.actor_id);
	output.integer(meta.writer_id);
	output.integer(meta.policy_version);
	output.integer(meta.compiler_version);
	output.integer<uint16_t>(static_cast<uint16_t>(meta.reason));
	output.zeros(2);
	output.integer<uint8_t>(meta.source_event ? 1 : 0);
	output.zeros(3);
	if (meta.source_event)
	{
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded = {};
		economic_source_event_encode(*meta.source_event, &encoded);
		output.block(encoded);
	}
	else
		output.zeros(ECONOMIC_SOURCE_EVENT_BYTES);
	output.block(meta.intent_digest);
	output.block(meta.domain_digest);
	for (size_t count :
	     { plan.accounts.size(), plan.postings.size(), plan.children.size(),
	       plan.items_before.size(), plan.items_after.size(), plan.item_events.size() })
		output.integer<uint32_t>(static_cast<uint32_t>(count));
	output.zeros(16);
	for (const auto &effect : plan.accounts)
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		economic_account_key_encode(effect.key, &encoded);
		output.block(encoded);
		output.coins(effect.before);
		output.coins(effect.after);
		output.integer(effect.before_revision);
		output.integer(effect.after_revision);
	}
	for (const auto &posting : plan.postings)
	{
		output.integer(posting.event_index);
		output.integer(posting.account_index);
		output.integer(posting.child_index);
		output.coins(posting.delta);
		output.integer(posting.copper);
	}
	for (const auto &child : plan.children)
	{
		output.id(child.operation_id);
		output.integer(child.domain);
		output.integer(child.discriminator);
		output.integer(child.parent_index);
		output.integer(child.relationship);
	}
	for (const auto *items : { &plan.items_before, &plan.items_after })
		for (const auto &item : *items)
		{
			output.integer(item.uid);
			output.position(item.position);
		}
	for (const auto &event : plan.item_events)
	{
		output.integer(event.event_index);
		output.integer(event.child_index);
		output.zeros(2);
		output.integer(event.uid);
		output.position(event.before);
		output.position(event.after);
	}
}

economic_accounting_error gambling_round_structure(const economic_accounting_plan &plan)
{
	const auto reason = plan.metadata.reason;
	if (reason != economic_reason::gambling_stake &&
	    reason != economic_reason::gambling_payout &&
	    reason != economic_reason::gambling_loss &&
	    reason != economic_reason::gambling_interruption)
		return economic_accounting_error::ok;
	const bool opening = reason == economic_reason::gambling_stake;
	if (!plan.metadata.source_event ||
	    plan.metadata.source_event->slot != (opening ? 0U : 1U) || !plan.children.empty() ||
	    !plan.items_before.empty() || !plan.items_after.empty() || !plan.item_events.empty())
		return economic_accounting_error::unauthorized;
	const economic_account_effect *held = nullptr;
	size_t wallets = 0, sinks = 0, issuances = 0, stakes = 0;
	for (const auto &account : plan.accounts)
	{
		if (account.key.kind == economic_account_kind::gambling_stake)
		{
			++stakes;
			held = &account;
		}
		wallets += account.key.kind == economic_account_kind::wallet;
		sinks += account.key.kind == economic_account_kind::sink;
		issuances += account.key.kind == economic_account_kind::issuance;
	}
	const bool shape = reason == economic_reason::gambling_loss ?
				   (sinks == 1 && stakes == 1 && plan.accounts.size() == 2) :
			   reason == economic_reason::gambling_payout ?
				   (wallets == 1 && stakes == 1 && issuances <= 1 &&
				    plan.accounts.size() == 2 + issuances) :
				   (wallets == 1 && stakes == 1 && plan.accounts.size() == 2);
	if (!shape || held->key.context_id != plan.metadata.source_event->sequence ||
	    !held->key.context_id)
		return economic_accounting_error::invalid_identity;
	const economic_coin_vector empty = {};
	const auto &stake = opening ? held->after : held->before;
	if ((opening ? held->before : held->after) != empty)
		return economic_accounting_error::unauthorized;
	size_t denomination = stake.size();
	for (size_t index = 0; index < stake.size(); ++index)
		if (stake[index])
		{
			if (stake[index] < 0 || denomination != stake.size())
				return economic_accounting_error::unauthorized;
			denomination = index;
		}
	if (denomination == stake.size())
		return economic_accounting_error::unauthorized;
	for (const auto &posting : plan.postings)
		for (size_t index = 0; index < posting.delta.size(); ++index)
			if (index != denomination && posting.delta[index])
				return economic_accounting_error::unauthorized;
	if (reason == economic_reason::gambling_payout && issuances)
	{
		size_t issuance_postings = 0;
		for (const auto &posting : plan.postings)
			if (plan.accounts[posting.account_index].key.kind ==
			    economic_account_kind::issuance)
			{
				++issuance_postings;
				if (posting.delta[denomination] != -stake[denomination])
					return economic_accounting_error::unauthorized;
			}
		if (issuance_postings != 1)
			return economic_accounting_error::unauthorized;
	}
	return economic_accounting_error::ok;
}
} // namespace

economic_accounting_error
economic_operation_metadata_validate(const economic_operation_metadata &meta)
{
	if (meta.version != ECONOMIC_ACCOUNTING_VERSION || meta.policy_version != 1 ||
	    meta.compiler_version != 1)
		return economic_accounting_error::invalid_version;
	if (critical_operation_id_is_zero(meta.lineage) ||
	    critical_operation_id_is_zero(meta.epoch) ||
	    critical_operation_id_is_zero(meta.operation_id) || !meta.actor_id || !meta.writer_id ||
	    critical_operation_id_equal(meta.original_operation_id, meta.operation_id))
		return economic_accounting_error::invalid_identity;
	const auto *rule = rule_for(meta.reason);
	if (!rule)
		return economic_accounting_error::invalid_reason;
	if (meta.actor_kind != rule->actor)
		return economic_accounting_error::unauthorized;
	if ((rule->source_required && !meta.source_event) ||
	    (meta.source_event && !economic_source_event_valid(*meta.source_event)) ||
	    (rule->original_required && critical_operation_id_is_zero(meta.original_operation_id)))
		return economic_accounting_error::invalid_identity;
	if (meta.source_event && !source_kind_allowed(meta.reason, meta.source_event->kind))
		return economic_accounting_error::unauthorized;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_plan_validate_structure(const economic_accounting_plan &plan)
{
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	const auto &meta = plan.metadata;
	const auto metadata_status = economic_operation_metadata_validate(meta);
	if (metadata_status != economic_accounting_error::ok)
		return metadata_status;
	if (zero(meta.intent_digest) || zero(meta.domain_digest))
		return economic_accounting_error::invalid_identity;
	const auto *rule = rule_for(meta.reason);
	auto status = economic_child_links_validate(meta.operation_id, plan.children);
	if (status != economic_accounting_error::ok)
		return status;
	status = economic_coin_effects_validate(plan.accounts, plan.postings, plan.children.size());
	if (status != economic_accounting_error::ok)
		return status;
	for (const auto &account : plan.accounts)
		if (account.key.lineage.bytes != meta.lineage.bytes ||
		    !(rule->accounts & (1U << static_cast<unsigned>(account.key.kind))))
			return economic_accounting_error::unauthorized;
	for (const auto &posting : plan.postings)
	{
		const auto kind = plan.accounts[posting.account_index].key.kind;
		if ((kind == economic_account_kind::issuance ||
		     kind == economic_account_kind::restitution) &&
		    posting.copper >= 0)
			return economic_accounting_error::unauthorized;
		if (kind == economic_account_kind::sink &&
		    (rule->reverse_sink ? posting.copper >= 0 : posting.copper <= 0))
			return economic_accounting_error::unauthorized;
		if (!economic_account_is_ordinary(kind) && posting.copper == 0)
			return economic_accounting_error::corrupt_evidence;
	}
	status = gambling_round_structure(plan);
	if (status != economic_accounting_error::ok)
		return status;
	return economic_item_effects_validate(plan.items_before, plan.items_after, plan.item_events,
					      plan.children.size());
}

economic_accounting_error economic_plan_normalize(economic_accounting_plan *plan)
{
	if (!plan)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(*plan))
		return economic_accounting_error::capacity;
	try
	{
		auto normalized = *plan;
		std::vector<size_t> order(plan->accounts.size()), remap(plan->accounts.size());
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(),
			  [&](size_t left, size_t right) {
				  return economic_account_key_less(plan->accounts[left].key,
								   plan->accounts[right].key);
			  });
		for (size_t index = 0; index < order.size(); ++index)
		{
			normalized.accounts[index] = plan->accounts[order[index]];
			remap[order[index]] = index;
		}
		for (auto &posting : normalized.postings)
		{
			if (posting.account_index >= remap.size())
				return economic_accounting_error::invalid_identity;
			posting.account_index = static_cast<uint16_t>(remap[posting.account_index]);
		}
		auto status = normalize_children(normalized);
		if (status != economic_accounting_error::ok)
			return status;
		std::sort(normalized.postings.begin(), normalized.postings.end(),
			  [](const auto &left, const auto &right)
			  { return left.event_index < right.event_index; });
		std::sort(normalized.item_events.begin(), normalized.item_events.end(),
			  [](const auto &left, const auto &right)
			  { return left.event_index < right.event_index; });
		for (auto *items : { &normalized.items_before, &normalized.items_after })
			std::sort(items->begin(), items->end(),
				  [](const auto &left, const auto &right)
				  { return left.uid < right.uid; });
		status = economic_plan_validate_structure(normalized);
		if (status != economic_accounting_error::ok)
			return status;
		*plan = std::move(normalized);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_plan_encode(const economic_accounting_plan &plan,
					       std::vector<uint8_t> *encoded)
{
	if (!encoded)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	try
	{
		auto normalized = plan;
		const auto status = economic_plan_normalize(&normalized);
		if (status != economic_accounting_error::ok)
			return status;
		writer output;
		encode_valid(normalized, output);
		if (output.bytes.size() != encoded_size(normalized))
			return economic_accounting_error::corrupt_evidence;
		*encoded = std::move(output.bytes);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_plan_decode(std::span<const uint8_t> encoded,
					       economic_accounting_plan *plan)
{
	if (!plan || encoded.size() < ECONOMIC_PLAN_HEADER_BYTES)
		return economic_accounting_error::corrupt_evidence;
	if (encoded.size() > ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES)
		return economic_accounting_error::capacity;
	try
	{
		reader input{ encoded };
		const auto magic = input.take(4);
		if (!std::equal(magic.begin(), magic.end(), PLAN_MAGIC.begin()))
			return economic_accounting_error::corrupt_evidence;
		economic_accounting_plan result;
		auto &meta = result.metadata;
		meta.version = input.integer<uint16_t>();
		input.zeros(2);
		if (meta.version != ECONOMIC_ACCOUNTING_VERSION)
			return economic_accounting_error::invalid_version;
		meta.lineage = input.id();
		meta.epoch = input.id();
		meta.operation_id = input.id();
		meta.original_operation_id = input.id();
		meta.actor_kind = static_cast<economic_actor_kind>(input.integer<uint8_t>());
		input.zeros(3);
		meta.actor_id = input.integer<uint64_t>();
		meta.writer_id = input.integer<uint32_t>();
		meta.policy_version = input.integer<uint32_t>();
		meta.compiler_version = input.integer<uint32_t>();
		meta.reason = static_cast<economic_reason>(input.integer<uint16_t>());
		input.zeros(2);
		const auto present = input.integer<uint8_t>();
		input.zeros(3);
		const auto source = input.take(ECONOMIC_SOURCE_EVENT_BYTES);
		if (present > 1)
			return economic_accounting_error::corrupt_evidence;
		if (present)
		{
			economic_source_event event = {};
			const auto status = economic_source_event_decode(source, &event);
			if (status != economic_accounting_error::ok)
				return status;
			meta.source_event = event;
		}
		else if (!zero(source))
			return economic_accounting_error::corrupt_evidence;
		input.block(meta.intent_digest);
		input.block(meta.domain_digest);
		std::array<uint32_t, 6> counts = {};
		for (auto &count : counts)
			count = input.integer<uint32_t>();
		input.zeros(16);
		if (!input.good)
			return economic_accounting_error::corrupt_evidence;
		if (counts[0] > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
		    counts[1] > ECONOMIC_ACCOUNTING_MAX_POSTINGS ||
		    counts[2] > ECONOMIC_ACCOUNTING_MAX_CHILDREN ||
		    counts[3] > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    counts[4] > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    counts[5] > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
			return economic_accounting_error::capacity;
		const size_t expected =
			ECONOMIC_PLAN_HEADER_BYTES + ACCOUNT_BYTES * counts[0] +
			POSTING_BYTES * counts[1] + ECONOMIC_CHILD_LINK_BYTES * counts[2] +
			SNAPSHOT_BYTES * (counts[3] + counts[4]) + EVENT_BYTES * counts[5];
		if (expected != encoded.size())
			return economic_accounting_error::corrupt_evidence;
		result.accounts.resize(counts[0]);
		result.postings.resize(counts[1]);
		result.children.resize(counts[2]);
		result.items_before.resize(counts[3]);
		result.items_after.resize(counts[4]);
		result.item_events.resize(counts[5]);
		for (auto &effect : result.accounts)
		{
			const auto status = economic_account_key_decode(
				input.take(ECONOMIC_ACCOUNT_KEY_BYTES), &effect.key);
			if (status != economic_accounting_error::ok)
				return status;
			effect.before = input.coins();
			effect.after = input.coins();
			effect.before_revision = input.integer<uint64_t>();
			effect.after_revision = input.integer<uint64_t>();
		}
		for (auto &posting : result.postings)
		{
			posting.event_index = input.integer<uint32_t>();
			posting.account_index = input.integer<uint16_t>();
			posting.child_index = input.integer<uint16_t>();
			posting.delta = input.coins();
			posting.copper = input.integer<int64_t>();
		}
		for (auto &child : result.children)
		{
			child.operation_id = input.id();
			child.domain = input.integer<uint32_t>();
			child.discriminator = input.integer<uint64_t>();
			child.parent_index = input.integer<uint16_t>();
			child.relationship = input.integer<uint16_t>();
		}
		for (auto *items : { &result.items_before, &result.items_after })
			for (auto &item : *items)
			{
				item.uid = input.integer<uint64_t>();
				item.position = input.position();
			}
		for (auto &event : result.item_events)
		{
			event.event_index = input.integer<uint32_t>();
			event.child_index = input.integer<uint16_t>();
			input.zeros(2);
			event.uid = input.integer<uint64_t>();
			event.before = input.position();
			event.after = input.position();
		}
		if (!input.good || input.offset != encoded.size())
			return economic_accounting_error::corrupt_evidence;
		const auto status = economic_plan_validate_structure(result);
		if (status != economic_accounting_error::ok)
			return status;
		std::vector<uint8_t> canonical;
		const auto canonical_status = economic_plan_encode(result, &canonical);
		if (canonical_status != economic_accounting_error::ok)
			return canonical_status;
		if (canonical.size() != encoded.size() ||
		    !std::equal(canonical.begin(), canonical.end(), encoded.begin()))
			return economic_accounting_error::corrupt_evidence;
		*plan = std::move(result);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_plan_digest(const economic_accounting_plan &plan,
					       economic_digest *digest)
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	std::vector<uint8_t> encoded;
	const auto status = economic_plan_encode(plan, &encoded);
	if (status != economic_accounting_error::ok)
		return status;
	economic_digest result = {};
	SHA256(encoded.data(), encoded.size(), result.data());
	*digest = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_command_binding_digest(const critical_command &command,
							  economic_digest *digest)
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return economic_accounting_error::invalid_version;
	if (command.accounting_intent.size() > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES)
		return economic_accounting_error::capacity;
	if ((command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	     !command.accounting_intent.empty()) ||
	    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	     command.accounting_intent.empty()))
		return economic_accounting_error::invalid_identity;
	// Only the native auction envelope/preparation projection needs all4096
	// original UID fences. This keeps the existing binding tag and schema1
	// preimage; its legacy execution predicate remains closed to auctionv2.
	const size_t max_keys = critical_command_native_auction_envelope(command) ?
					CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
					CRITICAL_COMMAND_MAX_KEYS;
	if (command.keys.size() > max_keys || command.expected_revisions.size() > max_keys ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return economic_accounting_error::capacity;
	try
	{
		auto projection = command;
		projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projection.accounting_intent.clear();
		projection.publication_required = false;
		// Only this binding projection uses a sentinel. Actual admission,
		// journal bytes, exact-ID equality and durable receipts keep real time.
		projection.accepted_at_usec = 1;
		// Structural projections also cover accounting-only command types.
		// Do not pass them through the legacy execution predicate.
		std::sort(projection.keys.begin(), projection.keys.end(), critical_entity_key_less);
		std::sort(projection.expected_revisions.begin(),
			  projection.expected_revisions.end(), [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		if (!critical_command_envelope_valid(projection))
			return economic_accounting_error::invalid_identity;
		std::vector<uint8_t> encoded;
		const auto result = critical_command_encode(projection, &encoded);
		if (result != critical_command_codec_result::ok)
			return result == critical_command_codec_result::overflow ?
				       economic_accounting_error::capacity :
				       economic_accounting_error::corrupt_evidence;
		static constexpr char tag[] = "DURIS-ECONOMIC-COMMAND-V1";
		// Include the NUL delimiter. This tag and schema-1 projection are the
		// versioned preimage contract, not an arbitrary display string.
		encoded.insert(encoded.begin(), tag, tag + sizeof(tag));
		economic_digest result_digest = {};
		SHA256(encoded.data(), encoded.size(), result_digest.data());
		*digest = result_digest;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

#include <limits>

namespace
{
struct economic_plan_storage_scan
{
	size_t request = 0, normalize_base = 0, child_working = 0;
	size_t validation_working = 0, encode_base = 0, encode_tail = 0;
	bool ordinary = false, before_parent = false, after_parent = false;
};
bool plan_storage_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
bool plan_storage_rows(size_t &total, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && plan_storage_add(total, count * width);
}
}

size_t economic_plan_allocation_preflight_working_bytes() noexcept
{
	return sizeof(economic_plan_storage_scan) +
	       sizeof(economic_accounting_plan_allocation_profile);
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool plan_validation_storage(const economic_accounting_plan &plan, size_t *output) noexcept
{
	if (!output)
		return false;
	bool ordinary = false, before_parent = false, after_parent = false;
	size_t working = 0, request = 0;
	for (const auto &account : plan.accounts)
		ordinary = ordinary || economic_account_is_ordinary(account.key.kind);
	for (const auto &item : plan.items_before)
		before_parent = before_parent ||
				((item.position.state == item_custody_state::active ||
				  item.position.state == item_custody_state::quarantined) &&
				 item.position.parent_uid);
	for (const auto &item : plan.items_after)
		after_parent = after_parent ||
			       ((item.position.state == item_custody_state::active ||
				 item.position.state == item_custody_state::quarantined) &&
				item.position.parent_uid);
	if (!economic_effects_validation_working_bytes(
		    plan.accounts.size(), ordinary, plan.items_before.size(),
		    plan.items_after.size(), plan.item_events.size(), before_parent, after_parent,
		    &working))
		return false;
	request = sizeof(std::span<const economic_child_link>);
	if (!plan.children.empty() &&
	    (!plan_storage_add(request, sizeof(critical_operation_id)) ||
	     !plan_storage_add(
		     request,
		     sizeof(std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + sizeof(uint32_t) +
							sizeof(uint64_t)>)) ||
	     !plan_storage_add(request, sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>))))
		return false;
	working = std::max(working, request);
	// zero(digest) passes a span by value; it does not coexist with validators.
	working = std::max(working, sizeof(std::span<const uint8_t>));
	if (plan.metadata.reason == economic_reason::gambling_stake ||
	    plan.metadata.reason == economic_reason::gambling_payout ||
	    plan.metadata.reason == economic_reason::gambling_loss ||
	    plan.metadata.reason == economic_reason::gambling_interruption)
		working = std::max(working, sizeof(economic_coin_vector));
	*output = working;
	return true;
}
#endif
} // namespace

economic_accounting_error
economic_plan_allocation_preflight(const economic_accounting_plan &plan,
				   economic_accounting_plan_allocation_profile *output) noexcept
{
	if (!output)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	economic_plan_storage_scan scan;
	(void)scan; // Supported request-policy fields are ABI-conditional.
	economic_accounting_plan_allocation_profile profile;
	if (!plan_storage_rows(profile.clone_heap_bytes, plan.accounts.size(),
			       sizeof(economic_account_effect)) ||
	    !plan_storage_rows(profile.clone_heap_bytes, plan.postings.size(),
			       sizeof(economic_coin_posting)) ||
	    !plan_storage_rows(profile.clone_heap_bytes, plan.children.size(),
			       sizeof(economic_child_link)) ||
	    !plan_storage_rows(profile.clone_heap_bytes, plan.items_before.size(),
			       sizeof(economic_item_snapshot)) ||
	    !plan_storage_rows(profile.clone_heap_bytes, plan.items_after.size(),
			       sizeof(economic_item_snapshot)) ||
	    !plan_storage_rows(profile.clone_heap_bytes, plan.item_events.size(),
			       sizeof(economic_item_event)))
		return economic_accounting_error::capacity;
	profile.encoded_bytes = ECONOMIC_PLAN_HEADER_BYTES;
	if (!plan_storage_rows(profile.encoded_bytes, plan.accounts.size(), ACCOUNT_BYTES) ||
	    !plan_storage_rows(profile.encoded_bytes, plan.postings.size(), POSTING_BYTES) ||
	    !plan_storage_rows(profile.encoded_bytes, plan.children.size(),
			       ECONOMIC_CHILD_LINK_BYTES) ||
	    !plan_storage_rows(profile.encoded_bytes, plan.items_before.size(), SNAPSHOT_BYTES) ||
	    !plan_storage_rows(profile.encoded_bytes, plan.items_after.size(), SNAPSHOT_BYTES) ||
	    !plan_storage_rows(profile.encoded_bytes, plan.item_events.size(), EVENT_BYTES) ||
	    profile.encoded_bytes > ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES)
		return economic_accounting_error::capacity;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	constexpr size_t bit_width = std::numeric_limits<std::_Bit_type>::digits;
	scan.normalize_base = sizeof(economic_accounting_plan) + 2 * sizeof(std::vector<size_t>);
	if (!plan_storage_add(scan.normalize_base, profile.clone_heap_bytes) ||
	    !plan_storage_rows(scan.normalize_base, plan.accounts.size(), 2 * sizeof(size_t)))
		return economic_accounting_error::capacity;
	scan.child_working = sizeof(std::vector<economic_child_link>) +
			     sizeof(std::vector<size_t>) + sizeof(std::vector<bool>);
	if (!plan_storage_rows(scan.child_working, plan.children.size(),
			       sizeof(economic_child_link)) ||
	    !plan_storage_rows(scan.child_working, plan.children.size() + 1, sizeof(size_t)) ||
	    !plan_storage_rows(scan.child_working,
			       plan.children.size() / bit_width +
				       (plan.children.size() % bit_width != 0),
			       sizeof(std::_Bit_type)))
		return economic_accounting_error::capacity;
	if (!plan_validation_storage(plan, &scan.validation_working))
		return economic_accounting_error::capacity;
	profile.normalize_working_bytes = scan.normalize_base;
	if (!plan_storage_add(profile.normalize_working_bytes,
			      std::max(scan.child_working, scan.validation_working)))
		return economic_accounting_error::capacity;
	// economic_plan_encode makes its own clone, then normalization makes a
	// second clone. The first clone persists into the separate writer phase.
	scan.encode_base = sizeof(economic_accounting_plan);
	if (!plan_storage_add(scan.encode_base, profile.clone_heap_bytes))
		return economic_accounting_error::capacity;
	scan.request =
		std::max(sizeof(std::span<const uint8_t>),
			 sizeof(std::initializer_list<size_t>) + sizeof(std::array<size_t, 6>));
	// writer::block and the six-count initializer-list phase are sequential.
	if (plan.metadata.source_event)
		scan.request = std::max(
			scan.request, 2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>));
	if (!plan.accounts.empty())
		scan.request = std::max(
			scan.request, 2 * sizeof(std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES>));
	scan.encode_tail = sizeof(writer);
	if (!plan_storage_add(scan.encode_tail, profile.encoded_bytes) ||
	    !plan_storage_add(scan.encode_tail, scan.request))
		return economic_accounting_error::capacity;
	profile.encode_working_bytes = scan.encode_base;
	if (!plan_storage_add(profile.encode_working_bytes,
			      std::max(profile.normalize_working_bytes, scan.encode_tail)))
		return economic_accounting_error::capacity;
	profile.storage_policy_supported = true;
#endif
	*output = profile;
	return economic_accounting_error::ok;
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
struct plan_decode_bound_workspace
{
	reader input;
	std::span<const uint8_t> magic, source;
	economic_accounting_plan result;
	std::array<uint32_t, 6> counts{};
	economic_accounting_plan_allocation_profile profile;
	std::vector<uint8_t> canonical;
	explicit plan_decode_bound_workspace(const std::span<const uint8_t> &encoded) noexcept
		: input{ encoded }
	{
	}
};
#endif
} // namespace

economic_accounting_error economic_plan_decode_bounded(const std::span<const uint8_t> &encoded,
						       economic_accounting_plan *plan,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live,
						       size_t *retained_plan_heap_bytes) noexcept
{
	if (!plan || encoded.size() < ECONOMIC_PLAN_HEADER_BYTES)
		return economic_accounting_error::corrupt_evidence;
	if (encoded.size() > ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES)
		return economic_accounting_error::capacity;
	if (!reserve)
		return economic_accounting_error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return economic_accounting_error::capacity;
#else
	size_t fixed = outer_live;
	if (!plan_storage_add(fixed, sizeof(plan_decode_bound_workspace)))
		return economic_accounting_error::capacity;
	size_t header_working =
		2 * sizeof(critical_operation_id) + 2 * sizeof(std::span<const uint8_t>);
	header_working = std::max(header_working,
				  sizeof(economic_source_event) + sizeof(std::span<const uint8_t>) +
					  economic_source_event_decode_object_bytes());
	size_t header_peak = fixed;
	if (!plan_storage_add(header_peak, header_working) || !reserve(header_peak, context))
		return economic_accounting_error::capacity;
	try
	{
		plan_decode_bound_workspace work(encoded);
		auto &input = work.input;
		auto &result = work.result;
		auto &counts = work.counts;
		auto &canonical = work.canonical;
		work.magic = input.take(4);
		const auto &magic = work.magic;
		if (!std::equal(magic.begin(), magic.end(), PLAN_MAGIC.begin()))
			return economic_accounting_error::corrupt_evidence;
		auto &meta = result.metadata;
		meta.version = input.integer<uint16_t>();
		input.zeros(2);
		if (meta.version != ECONOMIC_ACCOUNTING_VERSION)
			return economic_accounting_error::invalid_version;
		meta.lineage = input.id();
		meta.epoch = input.id();
		meta.operation_id = input.id();
		meta.original_operation_id = input.id();
		meta.actor_kind = static_cast<economic_actor_kind>(input.integer<uint8_t>());
		input.zeros(3);
		meta.actor_id = input.integer<uint64_t>();
		meta.writer_id = input.integer<uint32_t>();
		meta.policy_version = input.integer<uint32_t>();
		meta.compiler_version = input.integer<uint32_t>();
		meta.reason = static_cast<economic_reason>(input.integer<uint16_t>());
		input.zeros(2);
		const auto present = input.integer<uint8_t>();
		input.zeros(3);
		work.source = input.take(ECONOMIC_SOURCE_EVENT_BYTES);
		const auto &source = work.source;
		if (present > 1)
			return economic_accounting_error::corrupt_evidence;
		if (present)
		{
			economic_source_event event = {};
			const auto status = economic_source_event_decode(source, &event);
			if (status != economic_accounting_error::ok)
				return status;
			meta.source_event = event;
		}
		else if (!zero(source))
			return economic_accounting_error::corrupt_evidence;
		input.block(meta.intent_digest);
		input.block(meta.domain_digest);
		for (auto &count : counts)
			count = input.integer<uint32_t>();
		input.zeros(16);
		if (!input.good)
			return economic_accounting_error::corrupt_evidence;
		if (counts[0] > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
		    counts[1] > ECONOMIC_ACCOUNTING_MAX_POSTINGS ||
		    counts[2] > ECONOMIC_ACCOUNTING_MAX_CHILDREN ||
		    counts[3] > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    counts[4] > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
		    counts[5] > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
			return economic_accounting_error::capacity;
		const size_t expected =
			ECONOMIC_PLAN_HEADER_BYTES + ACCOUNT_BYTES * counts[0] +
			POSTING_BYTES * counts[1] + ECONOMIC_CHILD_LINK_BYTES * counts[2] +
			SNAPSHOT_BYTES * (counts[3] + counts[4]) + EVENT_BYTES * counts[5];
		if (expected != encoded.size())
			return economic_accounting_error::corrupt_evidence;
		size_t heap = 0;
		if (!plan_storage_rows(heap, counts[0], sizeof(economic_account_effect)) ||
		    !plan_storage_rows(heap, counts[1], sizeof(economic_coin_posting)) ||
		    !plan_storage_rows(heap, counts[2], sizeof(economic_child_link)) ||
		    !plan_storage_rows(heap, counts[3], sizeof(economic_item_snapshot)) ||
		    !plan_storage_rows(heap, counts[4], sizeof(economic_item_snapshot)) ||
		    !plan_storage_rows(heap, counts[5], sizeof(economic_item_event)))
			return economic_accounting_error::capacity;
		size_t rows = 2 * sizeof(std::span<const uint8_t>);
		if (counts[0])
			rows = std::max(rows,
					std::max(sizeof(economic_account_key) +
							 2 * sizeof(std::span<const uint8_t>),
						 2 * sizeof(economic_coin_vector) +
							 2 * sizeof(std::span<const uint8_t>)));
		if (counts[1])
			rows = std::max(rows, 2 * sizeof(economic_coin_vector) +
						      2 * sizeof(std::span<const uint8_t>));
		if (counts[2])
			rows = std::max(rows, 2 * sizeof(critical_operation_id) +
						      2 * sizeof(std::span<const uint8_t>));
		if (counts[3] || counts[4])
			rows = std::max(
				rows,
				2 * sizeof(economic_item_position) +
					2 * sizeof(std::span<const uint8_t>) +
					sizeof(std::initializer_list<
						std::vector<economic_item_snapshot> *>) +
					sizeof(std::array<std::vector<economic_item_snapshot> *, 2>));
		if (counts[5])
			rows = std::max(rows, 2 * sizeof(economic_item_position) +
						      2 * sizeof(std::span<const uint8_t>));
		size_t live = fixed, row_peak = 0;
		if (!plan_storage_add(live, heap))
			return economic_accounting_error::capacity;
		row_peak = live;
		if (!plan_storage_add(row_peak, rows) || !reserve(row_peak, context))
			return economic_accounting_error::capacity;
		result.accounts.resize(counts[0]);
		result.postings.resize(counts[1]);
		result.children.resize(counts[2]);
		result.items_before.resize(counts[3]);
		result.items_after.resize(counts[4]);
		result.item_events.resize(counts[5]);
		for (auto &effect : result.accounts)
		{
			const auto status = economic_account_key_decode(
				input.take(ECONOMIC_ACCOUNT_KEY_BYTES), &effect.key);
			if (status != economic_accounting_error::ok)
				return status;
			effect.before = input.coins();
			effect.after = input.coins();
			effect.before_revision = input.integer<uint64_t>();
			effect.after_revision = input.integer<uint64_t>();
		}
		for (auto &posting : result.postings)
		{
			posting.event_index = input.integer<uint32_t>();
			posting.account_index = input.integer<uint16_t>();
			posting.child_index = input.integer<uint16_t>();
			posting.delta = input.coins();
			posting.copper = input.integer<int64_t>();
		}
		for (auto &child : result.children)
		{
			child.operation_id = input.id();
			child.domain = input.integer<uint32_t>();
			child.discriminator = input.integer<uint64_t>();
			child.parent_index = input.integer<uint16_t>();
			child.relationship = input.integer<uint16_t>();
		}
		for (auto *items : { &result.items_before, &result.items_after })
			for (auto &item : *items)
			{
				item.uid = input.integer<uint64_t>();
				item.position = input.position();
			}
		for (auto &event : result.item_events)
		{
			event.event_index = input.integer<uint32_t>();
			event.child_index = input.integer<uint16_t>();
			input.zeros(2);
			event.uid = input.integer<uint64_t>();
			event.before = input.position();
			event.after = input.position();
		}
		if (!input.good || input.offset != encoded.size())
			return economic_accounting_error::corrupt_evidence;
		size_t validation_working = 0;
		if (!plan_validation_storage(result, &validation_working))
			return economic_accounting_error::capacity;
		size_t validation_peak = live;
		if (!plan_storage_add(validation_peak, validation_working) ||
		    !reserve(validation_peak, context))
			return economic_accounting_error::capacity;
		const auto status = economic_plan_validate_structure(result);
		if (status != economic_accounting_error::ok)
			return status;
		size_t scan_peak = live;
		if (!plan_storage_add(scan_peak,
				      economic_plan_allocation_preflight_working_bytes()) ||
		    !reserve(scan_peak, context))
			return economic_accounting_error::capacity;
		const auto profiled = economic_plan_allocation_preflight(result, &work.profile);
		if (profiled != economic_accounting_error::ok)
			return profiled;
		if (!work.profile.storage_policy_supported)
			return economic_accounting_error::capacity;
		size_t encode_peak = live;
		if (!plan_storage_add(encode_peak, work.profile.encode_working_bytes) ||
		    !reserve(encode_peak, context))
			return economic_accounting_error::capacity;
		const auto canonical_status = economic_plan_encode(result, &canonical);
		if (canonical_status != economic_accounting_error::ok)
			return canonical_status;
		if (canonical.size() != encoded.size() ||
		    !std::equal(canonical.begin(), canonical.end(), encoded.begin()))
			return economic_accounting_error::corrupt_evidence;
		static_assert(std::is_nothrow_move_assignable_v<economic_accounting_plan>);
		*plan = std::move(result);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = heap;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
#endif
	return economic_accounting_error::ok;
}

namespace
{
bool binding_digest_add(size_t &total, size_t extra) noexcept
{
	if (extra > SIZE_MAX - total)
		return false;
	total += extra;
	return true;
}
constexpr size_t binding_digest_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t binding_digest_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t binding_digest_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t binding_digest_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t binding_digest_vector_frames =
	binding_digest_allocator_frames + binding_digest_copy_frames +
	binding_digest_relocate_frames + binding_digest_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t binding_digest_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + binding_digest_allocator_frames;
constexpr size_t binding_digest_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + binding_digest_vector_frames;
template <typename T, typename Comparator> constexpr size_t binding_digest_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}
constexpr size_t binding_digest_command_default_frames =
	// Real command generated default/destructor and four vector default
	// constructor/_Vector_base/_Vector_impl/_Vector_impl_data/allocator
	// carriers; current object inline is separately owned by its lifetime.
	2 * sizeof(void *) + 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	4 * (sizeof(void *) + binding_digest_allocator_frames);
constexpr size_t binding_digest_critical_codec_frames =
	// Original encoder, working-bytes and bounded-encode parameter/return/
	// wire_bytes/status scopes, loop key+revision refs/endpoints/pad locals.
	10 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(critical_command_codec_result) +
	6 * sizeof(void *) + 2 * sizeof(unsigned int) +
	// append_le genuine widest uint64_t value plus byte loop and vector
	// reference; array begin/end and data query sources.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) +
	// Actual original decoder/bounded counterpart fixed scalar locals:
	// encoded/size/destination/reserve/context/outer/heap output, live,
	// offset/type/source/3 counts/auction flag/limit/required/intent locals.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(critical_command_codec_result) +
	2 * sizeof(size_t) + 2 * sizeof(uint16_t) + 3 * sizeof(uint32_t) + sizeof(bool) +
	sizeof(size_t) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(uint32_t) +
	// Key/revision loop indices and padding, prospective request/extra,
	// retained scalar and original bad_alloc reference. Object carriers
	// decoded/key/revision are admitted by existing real decoder itself.
	2 * sizeof(uint32_t) + 2 * sizeof(size_t) + 4 * sizeof(size_t) + sizeof(size_t) +
	sizeof(void *) +
	// Genuine widest read_le input/size/offset/value/decoded/index/return;
	// decode_add/admit/heap actual parameters/locals/query scopes.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
	10 * sizeof(void *) + 9 * sizeof(size_t) + 3 * sizeof(bool) +
	// vector constructions/destruction/calls, allocator and all fitting
	// insert/assign/append profiles, original command nonthrow final move.
	binding_digest_command_default_frames + binding_digest_vector_frames +
	4 * binding_digest_move_frames;
constexpr size_t binding_digest_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t binding_digest_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t binding_digest_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						      11 * sizeof(unsigned int) + 2 * sizeof(int) +
						      2 * sizeof(void *);
constexpr size_t binding_digest_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t binding_digest_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						    2 * sizeof(void *) + sizeof(unsigned int) +
						    sizeof(size_t) + sizeof(int);
constexpr size_t binding_digest_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						   sizeof(unsigned long) + sizeof(unsigned int) +
						   sizeof(int);
[[maybe_unused]] constexpr size_t binding_digest_sha_frames =
	std::max(binding_digest_sha_assembly_frames,
		 std::max(binding_digest_sha_c_small_frames, binding_digest_sha_c_normal_frames)) +
	std::max(binding_digest_sha_init_frames,
		 std::max(binding_digest_sha_update_frames, binding_digest_sha_final_frames));

struct binding_digest_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *projection = nullptr;
	const std::vector<uint8_t> *encoded = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 10 * sizeof(void *) + 8 * sizeof(size_t) +
					       6 * sizeof(bool) +
					       4 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, heap = 0;
		if (!binding_digest_add(total, sizeof(*this)) ||
		    !binding_digest_add(total, frames) || !binding_digest_add(total, observation) ||
		    !binding_digest_add(total, critical_command_copy_frame_bytes()) ||
		    !binding_digest_add(total, critical_command_valid_frame_bytes()))
			return false;
		if (projection && (!binding_digest_add(total, sizeof(*projection)) ||
				   !critical_command_current_heap_bytes(*projection, &heap) ||
				   !binding_digest_add(total, heap)))
			return false;
		if (encoded && (!binding_digest_add(total, sizeof(*encoded)) ||
				!binding_digest_add(total, encoded->capacity())))
			return false;
		if (!binding_digest_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		size_t request = binding_digest_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!binding_digest_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !binding_digest_add(request, next * sizeof(T)))
				return false;
		}
		return binding_digest_add(request,
					  2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		size_t levels = 0, remaining = count,
		       request = binding_digest_sort_leaf_frames<T, Comparator>();
		while (remaining > 1)
		{
			remaining >>= 1;
			++levels;
		}
		constexpr size_t recursive =
			3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(Comparator);
		return 2 * levels + 1 <= SIZE_MAX / recursive &&
		       binding_digest_add(request, (2 * levels + 1) * recursive) &&
		       binding_digest_add(request,
					  sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
};
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
economic_accounting_error binding_digest_owned(const critical_command &command,
					       economic_digest *digest,
					       binding_digest_budget &budget)
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return economic_accounting_error::invalid_version;
	if (command.accounting_intent.size() > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES)
		return economic_accounting_error::capacity;
	if ((command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	     !command.accounting_intent.empty()) ||
	    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	     command.accounting_intent.empty()))
		return economic_accounting_error::invalid_identity;
	// Only the native auction envelope/preparation projection needs all4096
	// original UID fences. This keeps the existing binding tag and schema1
	// preimage; its legacy execution predicate remains closed to auctionv2.
	const size_t max_keys = critical_command_native_auction_envelope(command) ?
					CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
					CRITICAL_COMMAND_MAX_KEYS;
	if (command.keys.size() > max_keys || command.expected_revisions.size() > max_keys ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return economic_accounting_error::capacity;
	try
	{
		size_t admission_prefix = 0, admission_request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &admission_request) ||
		    !binding_digest_add(admission_request,
					sizeof(critical_command) +
						critical_command_copy_frame_bytes()) ||
		    !budget.peak(admission_request))
			return economic_accounting_error::capacity;
		auto projection = command;
		budget.projection = &projection;
		projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projection.accounting_intent.clear();
		projection.publication_required = false;
		// Only this binding projection uses a sentinel. Actual admission,
		// journal bytes, exact-ID equality and durable receipts keep real time.
		projection.accepted_at_usec = 1;
		// Structural projections also cover accounting-only command types.
		// Do not pass them through the legacy execution predicate.
		if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
			    projection.keys.size()))
			return economic_accounting_error::capacity;
		std::sort(projection.keys.begin(), projection.keys.end(), critical_entity_key_less);
		if (!budget.sort_frame<critical_expected_revision, char>(
			    projection.expected_revisions.size()))
			return economic_accounting_error::capacity;
		std::sort(projection.expected_revisions.begin(),
			  projection.expected_revisions.end(), [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		if (!budget.peak(critical_command_valid_frame_bytes()))
			return economic_accounting_error::capacity;
		if (!critical_command_envelope_valid(projection))
			return economic_accounting_error::invalid_identity;
		if (!budget.peak(sizeof(std::vector<uint8_t>) +
				 binding_digest_command_default_frames))
			return economic_accounting_error::capacity;
		std::vector<uint8_t> encoded;
		budget.encoded = &encoded;
		const auto result =
			(!budget.prefix(admission_prefix, binding_digest_critical_codec_frames) ?
				 critical_command_codec_result::overflow :
				 critical_command_encode_bounded(projection, &encoded,
								 budget.reserve, budget.context,
								 admission_prefix));
		if (result != critical_command_codec_result::ok)
			return result == critical_command_codec_result::overflow ?
				       economic_accounting_error::capacity :
				       economic_accounting_error::corrupt_evidence;
		static constexpr char tag[] = "DURIS-ECONOMIC-COMMAND-V1";
		// Include the NUL delimiter. This tag and schema-1 projection are the
		// versioned preimage contract, not an arbitrary display string.
		if (!budget.growth(encoded, sizeof(tag)))
			return economic_accounting_error::capacity;
		encoded.insert(encoded.begin(), tag, tag + sizeof(tag));
		if (!budget.peak(sizeof(economic_digest) + sizeof(SHA256_CTX) +
				 binding_digest_sha_frames + 4 * (sizeof(void *) + sizeof(size_t)) +
				 sizeof(void *)))
			return economic_accounting_error::capacity;
		economic_digest result_digest = {};
		SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		if (SHA256_Init(&digest_context) != 1 ||
		    SHA256_Update(&digest_context, encoded.data(), encoded.size()) != 1 ||
		    SHA256_Final(result_digest.data(), &digest_context) != 1)
			return economic_accounting_error::corrupt_evidence;
#pragma GCC diagnostic pop
		*digest = result_digest;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}
#endif
} // namespace
economic_accounting_error
economic_command_binding_digest_bounded(const critical_command &command, economic_digest *digest,
					bool (*reserve)(size_t, void *) noexcept, void *context,
					size_t outer_live) noexcept
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	if (!reserve)
		return economic_accounting_error::capacity;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return economic_accounting_error::capacity;
	constexpr size_t frames =
		// Public/owned command/digest/budget/reserve/context/outer/return,
		// original max_keys, real admission_prefix/request, original result
		// and catch bad_alloc reference. Private tag is static storage.
		7 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(economic_accounting_error) +
		3 * sizeof(size_t) + sizeof(critical_command_codec_result) + sizeof(void *);
	binding_digest_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return economic_accounting_error::capacity;
	return binding_digest_owned(command, digest, budget);
#else
	(void)command;
	(void)context;
	(void)outer_live;
	return economic_accounting_error::capacity;
#endif
}

#include <cerrno>
#include <limits>
#include <tuple>
#include <stdexcept>

#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	__cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                          \
	!defined(__SANITIZE_THREAD__) &&                                                          \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                  \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&                           \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                           \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                          \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
#define DURIS_PLAN_BOUNDED_SOURCE_TARGET 1
namespace
{
struct plan_bounded_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, source, working = 0;
	bool denied = false;
	// The whole actual prospective peak is computed from this original input,
	// not a cached CURRENT census. It includes both clone lifetimes where needed.
	// Root callback separately refreshes authentic foreign physical owners once.
	bool peak(size_t child_extra = 0) noexcept
	{
		size_t total = outer;
		if (!plan_storage_add(total, sizeof(*this)) ||
		    !plan_storage_add(total, sizeof(economic_accounting_plan_allocation_profile)) ||
		    !plan_storage_add(total, source) || !plan_storage_add(total, working) ||
		    !plan_storage_add(total, child_extra) || !reserve || !reserve(total, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool child(size_t extra, void *opaque) noexcept
	{
		return static_cast<plan_bounded_budget *>(opaque)->peak(extra);
	}
};
// Input/prior output stay caller-owned; these are only the real early objects.
constexpr size_t plan_bound_entry_inline =
	sizeof(plan_bounded_budget) + sizeof(economic_accounting_plan_allocation_profile);
constexpr size_t plan_bound_digest_entry_inline =
	plan_bound_entry_inline + sizeof(std::vector<uint8_t>);

// Real selected comparator types, matching the original lambda predicates.
// Named types let the source proof use actual GNU iterator/value adapters.
struct plan_bound_account_compare
{
	const economic_accounting_plan *plan;
	bool operator()(size_t left, size_t right) const
	{
		return economic_account_key_less(plan->accounts[left].key,
						 plan->accounts[right].key);
	}
};
struct plan_bound_event_compare
{
	template <class T> bool operator()(const T &left, const T &right) const
	{
		return left.event_index < right.event_index;
	}
};
struct plan_bound_uid_compare
{
	bool operator()(const economic_item_snapshot &left,
			const economic_item_snapshot &right) const
	{
		return left.uid < right.uid;
	}
};

// SOURCE inventory below is a conservative sum of genuine named call families,
// not emitted stack, retained heap, allocator metadata or a CURRENT baseline.
// The corresponding immutable headers/current source are pinned in SOURCE-PINS.
constexpr size_t plan_bound_P = sizeof(void *), plan_bound_N = sizeof(size_t),
		 plan_bound_B = sizeof(bool), plan_bound_E = sizeof(economic_accounting_error);
constexpr size_t plan_bound_log(size_t n) noexcept
{
	size_t value = 0;
	while (n > 1)
	{
		n >>= 1;
		++value;
	}
	return value;
}
template <class T, class C, size_t Maximum> constexpr size_t plan_bound_sort_frames() noexcept
{
	using I = typename std::vector<T>::iterator;
	using D = typename std::vector<T>::difference_type;
	using IC = __gnu_cxx::__ops::_Iter_comp_iter<C>;
	using VC = __gnu_cxx::__ops::_Val_comp_iter<C>;
	using IV = __gnu_cxx::__ops::_Iter_comp_val<C>;
	// Full actual GNU13 sort/partition/insertion/heap inventory, lifted from the
	// settled UID provider with the real selected row/iterator/adapter types.
	constexpr size_t setup = 4 * sizeof(I) + 2 * sizeof(IC) + 2 * plan_bound_P + 3 * sizeof(D) +
				 sizeof(int) + 8 * (plan_bound_P + sizeof(I)) + 4 * plan_bound_B;
	constexpr size_t recursive = 3 * sizeof(I) + sizeof(D) + sizeof(IC);
	constexpr size_t partition =
		12 * sizeof(I) + 3 * sizeof(IC) + 2 * plan_bound_B + 7 * plan_bound_P + sizeof(T);
	constexpr size_t insertion = 10 * sizeof(I) + 2 * sizeof(IC) + 2 * sizeof(VC) + sizeof(IV) +
				     3 * sizeof(T) + 2 * sizeof(D) + 12 * plan_bound_P +
				     3 * plan_bound_B;
	constexpr size_t heap = 12 * sizeof(I) + 14 * sizeof(D) + 5 * sizeof(IC) + 2 * sizeof(VC) +
				2 * sizeof(IV) + 4 * sizeof(T) + 18 * plan_bound_P +
				3 * plan_bound_B;
	// Iter-comp and value-comp adapters own selected comparator constructor,
	// moved comparator and call operator, not the UID default comparison type.
	constexpr size_t adapter = 2 * sizeof(I) + 2 * plan_bound_P + 3 * plan_bound_B +
				   3 * (2 * plan_bound_P + sizeof(C)) +
				   3 * (3 * plan_bound_P + plan_bound_B);
	return setup + (2 * plan_bound_log(Maximum) + 1) * recursive +
	       (partition > insertion ? (partition > heap ? partition : heap) :
					(insertion > heap ? insertion : heap)) +
	       adapter;
}
constexpr size_t plan_bound_account_sort =
	plan_bound_sort_frames<size_t, plan_bound_account_compare,
			       ECONOMIC_ACCOUNTING_MAX_ACCOUNTS>();
constexpr size_t plan_bound_posting_sort =
	plan_bound_sort_frames<economic_coin_posting, plan_bound_event_compare,
			       ECONOMIC_ACCOUNTING_MAX_POSTINGS>();
constexpr size_t plan_bound_event_sort =
	plan_bound_sort_frames<economic_item_event, plan_bound_event_compare,
			       ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS>();
constexpr size_t plan_bound_item_sort =
	plan_bound_sort_frames<economic_item_snapshot, plan_bound_uid_compare,
			       ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES>();
constexpr size_t plan_bound_sort_max_a = plan_bound_account_sort > plan_bound_posting_sort ?
						 plan_bound_account_sort :
						 plan_bound_posting_sort;
constexpr size_t plan_bound_sort_max_b =
	plan_bound_event_sort > plan_bound_item_sort ? plan_bound_event_sort : plan_bound_item_sort;
constexpr size_t plan_bound_sort_max = plan_bound_sort_max_a > plan_bound_sort_max_b ?
					       plan_bound_sort_max_a :
					       plan_bound_sort_max_b;

// vector<bool>(n,false), _Bvector_base/impl/impl_data, _M_initialize,
// _M_allocate/_S_nword, real bit iterator arithmetic, fill, index/proxy,
// base cleanup/deallocation/_M_reset. Runtime consteval branches do not execute.
constexpr size_t plan_bound_bit_constructor =
	2 * plan_bound_P + plan_bound_N + plan_bound_B + sizeof(std::allocator<bool>) +
	2 * plan_bound_P + 2 * plan_bound_P + sizeof(std::allocator<std::_Bit_type>) +
	plan_bound_P + 2 * sizeof(std::_Bit_iterator) + plan_bound_P +
	// initialize(this,n,q,__start), allocate(this,n,p,result), S_nword(n,result).
	plan_bound_P + plan_bound_N + plan_bound_P + sizeof(std::_Bit_iterator) + 3 * plan_bound_P +
	plan_bound_N + 2 * plan_bound_N +
	// Bit_iterator/base constructors this/pointer/offset; operator+/+=/_M_incr
	// genuine iterator return and __n local, base offset integer carriers.
	2 * (2 * plan_bound_P + sizeof(unsigned int)) + sizeof(std::_Bit_iterator) +
	2 * plan_bound_P + 2 * sizeof(std::ptrdiff_t) + sizeof(std::_Bit_iterator) + plan_bound_P +
	2 * sizeof(std::ptrdiff_t) +
	// initialize_value(this,x,p), end_addr/addressof, fill_n and memset boundary.
	2 * plan_bound_P + plan_bound_B + 4 * plan_bound_P + plan_bound_P + plan_bound_N +
	plan_bound_B + 2 * plan_bound_P + plan_bound_N + sizeof(int) +
	// Actual default allocator/new/deallocate call family already authenticated.
	binding_digest_allocator_frames;
constexpr size_t plan_bound_bit_access =
	// vector[index] this/n/result, begin this/returned real iterator,
	// iterator[index]/operator+/+=, dereference/proxy ctor/bool/assignment.
	plan_bound_P + plan_bound_N + sizeof(std::_Bit_reference) + plan_bound_P +
	sizeof(std::_Bit_iterator) + 2 * plan_bound_P + 2 * sizeof(std::ptrdiff_t) +
	2 * sizeof(std::_Bit_iterator) + plan_bound_P + 2 * sizeof(std::ptrdiff_t) + plan_bound_P +
	sizeof(std::_Bit_reference) + 2 * plan_bound_P + sizeof(std::_Bit_type) + plan_bound_P +
	plan_bound_B + 2 * plan_bound_P + plan_bound_B;
constexpr size_t plan_bound_bit_cleanup =
	// vector/base dtors, deallocate this/n, end_addr/addressof, reset actual
	// impl-data temporary (its genuine two iterator + pointer fields), generated
	// assignment/ctor/destruction and word allocator deallocation path.
	4 * plan_bound_P + plan_bound_N + 4 * plan_bound_P + 2 * sizeof(std::_Bit_iterator) +
	plan_bound_P + 4 * plan_bound_P + binding_digest_allocator_frames;

using plan_bound_key_tuple =
	decltype(std::tie(std::declval<const std::array<uint8_t, 16> &>(),
			  std::declval<const economic_account_kind &>(),
			  std::declval<const uint64_t &>(), std::declval<const uint64_t &>()));
constexpr size_t plan_bound_key_comparison =
	// key_less two refs/result, two actual returned four-reference tuple objects,
	// tie/tuple/_Tuple_impl recursive constructors/forward/head-base families.
	2 * plan_bound_P + plan_bound_B + 2 * sizeof(plan_bound_key_tuple) +
	2 * (4 * plan_bound_P + 5 * plan_bound_P +
	     4 * (3 * plan_bound_P + 2 * plan_bound_P + 2 * plan_bound_P)) +
	// tuple <=> and four nested __tuple_cmp scopes plus terminal equivalent:
	// tuple refs, true index_sequence tag, actual __c/result ordering objects;
	// get -> __get_helper -> _M_head per argument and synth3way receiver/ref args.
	2 * plan_bound_P + sizeof(std::strong_ordering) + sizeof(std::index_sequence<0, 1, 2, 3>) +
	4 * (2 * plan_bound_P + sizeof(std::index_sequence<0>) + 2 * sizeof(std::strong_ordering) +
	     2 * (6 * plan_bound_P) + 3 * plan_bound_P + sizeof(std::strong_ordering)) +
	2 * plan_bound_P +
	// ordering comparisons with actual unspecified nullptr tag/bool; byte-array
	// <=> runtime memcmp source and data, size/index/constant-eval result carriers.
	3 * (plan_bound_P + sizeof(std::strong_ordering) + plan_bound_B) + 2 * plan_bound_P +
	plan_bound_N + sizeof(int) + sizeof(std::strong_ordering) + 4 * plan_bound_P + plan_bound_B;
constexpr size_t plan_bound_array_equal =
	// array== -> equal -> __equal_aux/aux1/equal<true> -> memcmp, array begin/end;
	// widest integer array comparator also carries the same actual three pointers.
	2 * plan_bound_P + plan_bound_B + 4 * (3 * plan_bound_P + plan_bound_B) + 2 * plan_bound_P +
	plan_bound_N + sizeof(int) + 8 * plan_bound_P;
constexpr size_t plan_bound_all_of =
	// all_of -> find_if_not -> __find_if_not -> __find_if RA unrolled loop,
	// predicate adapters/lambda and iterator operations. No count-depth term.
	4 * (3 * plan_bound_P + sizeof(char) + plan_bound_B) + 3 * plan_bound_P + 2 * sizeof(char) +
	sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) + 3 * plan_bound_P +
	sizeof(int64_t) + plan_bound_B;
constexpr size_t plan_bound_optional =
	// metadata/source-event optional bool -> has_value -> _M_is_engaged;
	// deref -> _M_get -> _M_get(payload) and copy construction/destruction.
	3 * (plan_bound_P + plan_bound_B) + 4 * (2 * plan_bound_P) + 4 * plan_bound_P +
	2 * sizeof(std::in_place_t);
constexpr size_t plan_bound_lower_bound =
	// item_index span by value,uid,found; lower_bound/__lower_bound distance,
	// len/half/middle and genuine iter_comp_val adapter/actual item comparator.
	sizeof(std::span<const economic_item_snapshot>) + sizeof(uint64_t) + plan_bound_P +
	2 * (3 * plan_bound_P + sizeof(char)) + 2 * sizeof(std::ptrdiff_t) + plan_bound_P +
	2 * (2 * plan_bound_P + sizeof(std::ptrdiff_t)) + plan_bound_P + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * plan_bound_P + sizeof(std::ptrdiff_t) +
	3 * plan_bound_P + sizeof(uint64_t) + plan_bound_B;

// Original lexical source inventories. Workspaces/clones/vectors are separately
// in the genuine prospective allocation profile; only references/indices/status
// and scalar-method/range/query carriers are inventoried here.
constexpr size_t plan_bound_metadata_source =
	// validate meta/rule/status, fixed RULES range hidden-ref/begin/end/current,
	// source_kind_allowed formals, ID zero/equal array endpoints/value/result.
	6 * plan_bound_P + 2 * plan_bound_E + sizeof(economic_reason) +
	sizeof(economic_source_kind) + 3 * plan_bound_B + 4 * plan_bound_P +
	sizeof(economic_reason) +
	3 * (plan_bound_P + 2 * plan_bound_P + sizeof(uint8_t) + plan_bound_B) +
	plan_bound_array_equal + plan_bound_optional;
constexpr size_t plan_bound_coin_source =
	// coin effects spans/child_count/status, four lexical loop indices and refs,
	// balance/value/narrow wider integer, ordinary delta ignored/result arrays
	// owned by storage profile; delta/value nonnegative/zero scan and numeric limits.
	sizeof(std::span<const economic_account_effect>) +
	sizeof(std::span<const economic_coin_posting>) + plan_bound_N + plan_bound_E +
	4 * plan_bound_N + 3 * plan_bound_P + sizeof(__int128_t) + sizeof(int64_t) +
	2 * plan_bound_E + 3 * plan_bound_P + 2 * plan_bound_N + sizeof(__int128_t) +
	sizeof(int64_t) + plan_bound_B +
	// ordinary/kind/key valid/equal helpers, caller key and before/after equality.
	5 * plan_bound_P + 3 * sizeof(economic_account_kind) + 5 * plan_bound_B +
	plan_bound_key_comparison + plan_bound_array_equal + plan_bound_all_of +
	plan_bound_bit_constructor + plan_bound_bit_access + plan_bound_bit_cleanup +
	binding_digest_vector_constructor_frames + binding_digest_default_frames +
	binding_digest_allocator_frames;
constexpr size_t plan_bound_item_source =
	// item_effects three input spans/count/status/index/event/target/current refs;
	// item_forest input span/index/start/current/position/parent/path-range state;
	// live_custody, position_valid, owner validity/equality, position equality.
	2 * sizeof(std::span<const economic_item_snapshot>) +
	sizeof(std::span<const economic_item_event>) + plan_bound_N + 2 * plan_bound_E +
	2 * plan_bound_N + 3 * plan_bound_P + sizeof(std::span<const economic_item_snapshot>) +
	5 * plan_bound_N + 4 * plan_bound_P + 2 * plan_bound_N + 2 * plan_bound_P +
	sizeof(item_custody_state) + 4 * plan_bound_B + sizeof(uint64_t) + 5 * plan_bound_P +
	3 * plan_bound_B + plan_bound_lower_bound + binding_digest_vector_constructor_frames +
	binding_digest_vector_frames + binding_digest_copy_frames;
constexpr size_t plan_bound_gambling_source =
	// reason/opening/shape/held, wallets/sinks/issuances/stakes/denomination,
	// real stake/ref, account/posting/index and issuance_postings loops.
	plan_bound_P + sizeof(economic_reason) + 2 * plan_bound_B + 2 * plan_bound_P +
	7 * plan_bound_N + 6 * plan_bound_P + plan_bound_array_equal + plan_bound_optional;
constexpr size_t plan_bound_preflight_source =
	// Public profile/scan plus validation_storage locals and three range loops.
	// These actual scanner objects coexist before any owned clone exists.
	sizeof(economic_plan_storage_scan) + sizeof(economic_accounting_plan_allocation_profile) +
	4 * plan_bound_P + 2 * plan_bound_E + 6 * plan_bound_N + 3 * plan_bound_B +
	3 * 3 * plan_bound_P + plan_bound_B +
	// storage add/rows and effects working profile actual arguments/locals/forest
	// lambda capture/args, max(initializer_list3) true backing and descriptor.
	2 * plan_bound_P + 3 * plan_bound_N + 2 * plan_bound_B + 2 * plan_bound_P +
	4 * plan_bound_N + 3 * plan_bound_B + 9 * plan_bound_N +
	sizeof(std::initializer_list<size_t>) + 3 * sizeof(size_t) + 3 * plan_bound_P +
	3 * plan_bound_N + 3 * plan_bound_B + 4 * plan_bound_P + 2 * plan_bound_N + plan_bound_B +
	6 * (plan_bound_P + plan_bound_N);
constexpr size_t plan_bound_derive_copy_array_frames =
	// copy first/last/result/return; __miter_base three calls and return;
	// __copy_move_a/a1/a2 pointer arguments/results; __niter_base/wrap.
	4 * sizeof(void *) + 3 * (2 * sizeof(void *)) + 3 * (4 * sizeof(void *)) +
	3 * (2 * sizeof(void *)) + 3 * sizeof(void *) +
	// Trivial __copy_m first/last/result/_Num/result and real memmove.
	4 * sizeof(void *) + sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) +
	// copy_n first/n/result/__n2/return, size-to-integer param/result,
	// __copy_n first/n/result/tag/return and category reference/tag.
	3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(size_t) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(std::random_access_iterator_tag) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) +
	// Genuine array begin/end/data this/result plus size and subscripting.
	8 * (2 * sizeof(void *)) + 2 * (sizeof(void *) + sizeof(size_t)) +
	2 * (2 * sizeof(void *) + sizeof(size_t));
constexpr size_t plan_bound_derive_source =
	// Exact existing bounded-ID derive parameter/loop/predicate/add/reserve
	// declarations, plus its authenticated raw-pointer array-copy and SHA graph.
	4 * sizeof(void *) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
	sizeof(size_t) + 2 * sizeof(size_t) +
	2 * (sizeof(void *) + 2 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool)) +
	2 * sizeof(size_t) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(void *) +
	sizeof(bool) + plan_bound_derive_copy_array_frames + binding_digest_sha_frames;
constexpr size_t plan_bound_normalize_lexical =
	// public, owned, budget peak/child/add frames and source query; original
	// iota value is int, iterator endpoints and true returned/vector methods.
	10 * plan_bound_P + 8 * plan_bound_N + 4 * plan_bound_E + 4 * plan_bound_B +
	3 * plan_bound_P + sizeof(int) + 6 * (plan_bound_P + plan_bound_N) +
	// Account copy/remap index/posting range; child selected/target/index/parent,
	// posting/event loops, two-pointer item-vector list descriptor/backing/endpoints.
	3 * plan_bound_N + 4 * plan_bound_P + 4 * plan_bound_N + 4 * plan_bound_P +
	sizeof(std::initializer_list<std::vector<economic_item_snapshot> *>) + 5 * plan_bound_P +
	// child validation root/span/link/parent/expected/indices and derive fixed
	// entry formals before its own first complete fixed SOURCE/CTX admission.
	sizeof(std::span<const economic_child_link>) + 4 * plan_bound_P + 2 * plan_bound_N +
	sizeof(critical_operation_id) + 4 * plan_bound_P + sizeof(uint32_t) + sizeof(uint64_t) +
	2 * plan_bound_N + plan_bound_B +
	// Full structure validator status/meta/rule and all authentic range loops.
	4 * plan_bound_P + 2 * plan_bound_E + 9 * plan_bound_P + plan_bound_metadata_source +
	plan_bound_coin_source + plan_bound_item_source + plan_bound_gambling_source +
	plan_bound_preflight_source +
	// Six genuine plan vector COPY/move/generated metadata/optional methods;
	// child vector original/range/index constructors and complete bool cleanup.
	6 * (binding_digest_vector_constructor_frames + binding_digest_copy_frames +
	     binding_digest_move_frames) +
	6 * plan_bound_P + plan_bound_optional + plan_bound_bit_constructor +
	plan_bound_bit_access + plan_bound_bit_cleanup + plan_bound_key_comparison +
	plan_bound_array_equal + plan_bound_sort_max + plan_bound_derive_source;
constexpr size_t plan_bound_encode_lexical =
	// encode public/owned parameters/status, writer/meta/row refs and integer
	// this/value/index loops, block span and zeros/id/coins/position call scopes.
	9 * plan_bound_P + 5 * plan_bound_N + 3 * plan_bound_E + 2 * plan_bound_P +
	sizeof(uint64_t) + plan_bound_N + sizeof(std::span<const uint8_t>) + plan_bound_P +
	plan_bound_N + 2 * plan_bound_P + 3 * plan_bound_P + sizeof(int64_t) + 2 * plan_bound_P +
	6 * plan_bound_P +
	// Six-count initializer-list's actual stored values/container/begin/end/count.
	sizeof(std::initializer_list<size_t>) + 6 * plan_bound_N + 2 * plan_bound_P + plan_bound_N +
	// Full original source/key encoder local fixed result arrays, typed scalar
	// loops/copy accessors. They coexist with the output temporary arrays already
	// in encode working bytes, so only original callee array scratch is added here.
	sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
	sizeof(std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES>) + 4 * plan_bound_P +
	sizeof(uint16_t) + 3 * plan_bound_N + 2 * plan_bound_E + 2 * plan_bound_P + plan_bound_N +
	sizeof(uint64_t) + plan_bound_N + binding_digest_copy_frames + plan_bound_metadata_source +
	// First encode clone/default/final move and all exact writer vector operations.
	6 * (binding_digest_vector_constructor_frames + binding_digest_copy_frames +
	     binding_digest_move_frames) +
	binding_digest_vector_frames + plan_bound_normalize_lexical;
constexpr size_t plan_bound_normalize_source = plan_bound_normalize_lexical;
constexpr size_t plan_bound_encode_source = plan_bound_encode_lexical;
constexpr size_t plan_bound_digest_source =
	plan_bound_encode_lexical +
	// Real digest wrapper/formals/source/profile/status/encoded_status, cap/data
	// scalar queries, fixed array assignment. SHA context/result are heap-profile
	// private inline in peak, not an allocator request or a versioned hash tag.
	5 * plan_bound_P + 3 * plan_bound_N + 3 * plan_bound_E + plan_bound_B +
	3 * (plan_bound_P + plan_bound_N) + binding_digest_sha_frames;

// Original binding child retains its original admission. The supplement below
// supplies only source families for which its old local subtotal was incomplete.
// In particular codec and CURRENT use their authentic complete owning exports;
// the old family subtotal is credited rather than retained twice.
using plan_bound_binding_revision_compare =
	decltype([](const critical_expected_revision &a, const critical_expected_revision &b)
		 { return critical_entity_key_less(a.key, b.key); });
constexpr size_t plan_bound_binding_key_sort =
	plan_bound_sort_frames<critical_entity_key, decltype(&critical_entity_key_less),
			       CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS>();
constexpr size_t plan_bound_binding_revision_sort =
	plan_bound_sort_frames<critical_expected_revision, plan_bound_binding_revision_compare,
			       CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS>();
constexpr size_t plan_bound_binding_old_key_sort =
	binding_digest_sort_leaf_frames<critical_entity_key, decltype(&critical_entity_key_less)>() +
	(2 * plan_bound_log(CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS) + 1) *
		(3 * plan_bound_P + sizeof(std::ptrdiff_t) +
		 sizeof(decltype(&critical_entity_key_less)));
constexpr size_t plan_bound_binding_old_revision_sort =
	binding_digest_sort_leaf_frames<critical_expected_revision, char>() +
	(2 * plan_bound_log(CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS) + 1) *
		(3 * plan_bound_P + sizeof(std::ptrdiff_t) + sizeof(char));
static_assert(sizeof(plan_bound_binding_revision_compare) == sizeof(char));
// Recursion scopes have the same width at every actual count, so subtracting the
// equal maximum-depth terms leaves a count-independent typed adapter/setup gap.
static_assert(sizeof(std::vector<critical_entity_key>::iterator) == plan_bound_P);
static_assert(sizeof(std::vector<critical_expected_revision>::iterator) == plan_bound_P);
constexpr size_t plan_bound_binding_sort_gap_a =
	plan_bound_binding_key_sort > plan_bound_binding_old_key_sort ?
		plan_bound_binding_key_sort - plan_bound_binding_old_key_sort :
		0;
constexpr size_t plan_bound_binding_sort_gap_b =
	plan_bound_binding_revision_sort > plan_bound_binding_old_revision_sort ?
		plan_bound_binding_revision_sort - plan_bound_binding_old_revision_sort :
		0;
constexpr size_t plan_bound_binding_sort_gap = plan_bound_binding_sort_gap_a >
							       plan_bound_binding_sort_gap_b ?
						       plan_bound_binding_sort_gap_a :
						       plan_bound_binding_sort_gap_b;
constexpr size_t plan_bound_binding_old_frames =
	7 * plan_bound_P + plan_bound_N + 2 * plan_bound_E + 3 * plan_bound_N +
	sizeof(critical_command_codec_result) + plan_bound_P;
constexpr size_t plan_bound_binding_old_observation =
	10 * plan_bound_P + 8 * plan_bound_N + 6 * plan_bound_B + 4 * (plan_bound_P + plan_bound_N);
constexpr size_t plan_bound_binding_supplement_lexical =
	// Fresh request public command/output/total/result; four size queries, and
	// checked-add formals/results; copy frame query result and actual add return.
	2 * plan_bound_P + plan_bound_N + plan_bound_B + 4 * (plan_bound_P + plan_bound_N) +
	plan_bound_P + plan_bound_N + plan_bound_B + plan_bound_N +
	// Native auction predicate command/result and vector.empty this/result.
	plan_bound_P + plan_bound_B + plan_bound_P + plan_bound_B +
	// Actual budget prefix/peak/growth/sort methods: this/output/extra/total/heap,
	// returned status; growth count/request/next, actual max params/result;
	// sort levels/remaining/request and its true caller count/this/result.
	4 * plan_bound_P + 4 * plan_bound_N + 2 * plan_bound_B + 2 * plan_bound_P +
	3 * plan_bound_N + plan_bound_B + 2 * plan_bound_P + plan_bound_N + plan_bound_P +
	4 * plan_bound_N + plan_bound_B +
	// Actual key less scalar field comparison formals/result; revision lambda
	// this/left/right/result and pointer comparator return/call source carriers.
	2 * plan_bound_P + plan_bound_B + 3 * plan_bound_P + plan_bound_B +
	// Generated projection construction/destruction and original final digest
	// array assignment/data/size accessors, caught bad_alloc reference.
	2 * plan_bound_P + 4 * plan_bound_P + 2 * (plan_bound_P + plan_bound_N) + plan_bound_P +
	plan_bound_binding_sort_gap;
bool plan_bound_binding_profiles(size_t &full, size_t &supplement) noexcept
{
	size_t codec = 0, current = 0;
	if (!critical_command_startup_codec_source_frame_bytes(&codec) ||
	    !critical_command_current_heap_observer_frame_bytes(&current))
		return false;
	size_t value = plan_bound_binding_supplement_lexical;
	if (codec > binding_digest_critical_codec_frames &&
	    !plan_storage_add(value, codec - binding_digest_critical_codec_frames))
		return false;
	if (current > plan_bound_binding_old_observation &&
	    !plan_storage_add(value, current - plan_bound_binding_old_observation))
		return false;
	supplement = value;
	// Sequential original admission subtotals are summed conservatively for the
	// whole preentry closure, but only supplement survives in caller outer_live.
	if (!plan_storage_add(value, plan_bound_binding_old_frames) ||
	    !plan_storage_add(value, plan_bound_binding_old_observation) ||
	    !plan_storage_add(value, critical_command_copy_frame_bytes()) ||
	    !plan_storage_add(value, critical_command_valid_frame_bytes()) ||
	    !plan_storage_add(value, binding_digest_command_default_frames) ||
	    !plan_storage_add(value, binding_digest_vector_frames) ||
	    !plan_storage_add(value, plan_bound_binding_old_key_sort) ||
	    !plan_storage_add(value, plan_bound_binding_old_revision_sort) ||
	    !plan_storage_add(value, binding_digest_critical_codec_frames) ||
	    !plan_storage_add(value, binding_digest_sha_frames))
		return false;
	full = value;
	return true;
}
economic_accounting_error plan_bound_normalize_children(economic_accounting_plan &plan)
{
	const auto original = plan.children;
	std::vector<size_t> remap(original.size() + 1, 0);
	std::vector<bool> used(original.size(), false);
	for (size_t target = 0; target < original.size(); ++target)
	{
		size_t chosen = original.size();
		for (size_t index = 0; index < original.size(); ++index)
		{
			const size_t parent = original[index].parent_index;
			if (parent > original.size())
				return economic_accounting_error::invalid_identity;
			if (used[index] || (parent && !remap[parent]))
				continue;
			if (chosen == original.size() ||
			    original[index].operation_id.bytes <
				    original[chosen].operation_id.bytes)
				chosen = index;
		}
		if (chosen == original.size())
			return economic_accounting_error::topology;
		used[chosen] = true;
		remap[chosen + 1] = target + 1;
		plan.children[target] = original[chosen];
		plan.children[target].parent_index =
			static_cast<uint16_t>(remap[original[chosen].parent_index]);
	}
	for (auto &posting : plan.postings)
	{
		if (posting.child_index >= remap.size())
			return economic_accounting_error::invalid_identity;
		posting.child_index = static_cast<uint16_t>(remap[posting.child_index]);
	}
	for (auto &event : plan.item_events)
	{
		if (event.child_index >= remap.size())
			return economic_accounting_error::invalid_identity;
		event.child_index = static_cast<uint16_t>(remap[event.child_index]);
	}
	return economic_accounting_error::ok;
}

economic_accounting_error
plan_bound_child_links_validate(const critical_operation_id &root,
				std::span<const economic_child_link> links,
				plan_bounded_budget &budget)
{
	if (critical_operation_id_is_zero(root))
		return economic_accounting_error::invalid_identity;
	if (links.size() > ECONOMIC_ACCOUNTING_MAX_CHILDREN)
		return economic_accounting_error::capacity;
	for (size_t index = 0; index < links.size(); ++index)
	{
		const auto &link = links[index];
		if (link.parent_index > index || link.relationship != 1 || !link.domain ||
		    critical_operation_id_is_zero(link.operation_id) ||
		    critical_operation_id_equal(root, link.operation_id))
			return economic_accounting_error::invalid_identity;
		const auto &parent = link.parent_index ? links[link.parent_index - 1].operation_id :
							 root;
		critical_operation_id expected = {};
		if (!critical_operation_id_derive_bounded(parent, link.domain, link.discriminator,
							  &expected, plan_bounded_budget::child,
							  &budget, 0) ||
		    !critical_operation_id_equal(expected, link.operation_id))
			return budget.denied ? economic_accounting_error::capacity :
					       economic_accounting_error::payload_conflict;
		for (size_t prior = 0; prior < index; ++prior)
			if (critical_operation_id_equal(links[prior].operation_id,
							link.operation_id))
				return economic_accounting_error::duplicate_event;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error plan_bound_validate_structure(const economic_accounting_plan &plan,
							plan_bounded_budget &budget)
{
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	const auto &meta = plan.metadata;
	const auto metadata_status = economic_operation_metadata_validate(meta);
	if (metadata_status != economic_accounting_error::ok)
		return metadata_status;
	if (zero(meta.intent_digest) || zero(meta.domain_digest))
		return economic_accounting_error::invalid_identity;
	const auto *rule = rule_for(meta.reason);
	auto status = plan_bound_child_links_validate(meta.operation_id, plan.children, budget);
	if (status != economic_accounting_error::ok)
		return status;
	status = economic_coin_effects_validate(plan.accounts, plan.postings, plan.children.size());
	if (status != economic_accounting_error::ok)
		return status;
	for (const auto &account : plan.accounts)
		if (account.key.lineage.bytes != meta.lineage.bytes ||
		    !(rule->accounts & (1U << static_cast<unsigned>(account.key.kind))))
			return economic_accounting_error::unauthorized;
	for (const auto &posting : plan.postings)
	{
		const auto kind = plan.accounts[posting.account_index].key.kind;
		if ((kind == economic_account_kind::issuance ||
		     kind == economic_account_kind::restitution) &&
		    posting.copper >= 0)
			return economic_accounting_error::unauthorized;
		if (kind == economic_account_kind::sink &&
		    (rule->reverse_sink ? posting.copper >= 0 : posting.copper <= 0))
			return economic_accounting_error::unauthorized;
		if (!economic_account_is_ordinary(kind) && posting.copper == 0)
			return economic_accounting_error::corrupt_evidence;
	}
	status = gambling_round_structure(plan);
	if (status != economic_accounting_error::ok)
		return status;
	return economic_item_effects_validate(plan.items_before, plan.items_after, plan.item_events,
					      plan.children.size());
}

economic_accounting_error plan_bound_normalize_owned(economic_accounting_plan *plan,
						     plan_bounded_budget &budget)
{
	if (!plan)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(*plan))
		return economic_accounting_error::capacity;
	try
	{
		auto normalized = *plan;
		std::vector<size_t> order(plan->accounts.size()), remap(plan->accounts.size());
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), plan_bound_account_compare{ plan });
		for (size_t index = 0; index < order.size(); ++index)
		{
			normalized.accounts[index] = plan->accounts[order[index]];
			remap[order[index]] = index;
		}
		for (auto &posting : normalized.postings)
		{
			if (posting.account_index >= remap.size())
				return economic_accounting_error::invalid_identity;
			posting.account_index = static_cast<uint16_t>(remap[posting.account_index]);
		}
		auto status = plan_bound_normalize_children(normalized);
		if (status != economic_accounting_error::ok)
			return status;
		std::sort(normalized.postings.begin(), normalized.postings.end(),
			  plan_bound_event_compare{});
		std::sort(normalized.item_events.begin(), normalized.item_events.end(),
			  plan_bound_event_compare{});
		for (auto *items : { &normalized.items_before, &normalized.items_after })
			std::sort(items->begin(), items->end(), plan_bound_uid_compare{});
		status = plan_bound_validate_structure(normalized, budget);
		if (status != economic_accounting_error::ok)
			return status;
		*plan = std::move(normalized);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error plan_bound_encode_owned(const economic_accounting_plan &plan,
						  std::vector<uint8_t> *encoded,
						  plan_bounded_budget &budget)
{
	if (!encoded)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	try
	{
		auto normalized = plan;
		const auto status = plan_bound_normalize_owned(&normalized, budget);
		if (status != economic_accounting_error::ok)
			return status;
		writer output;
		encode_valid(normalized, output);
		if (output.bytes.size() != encoded_size(normalized))
			return economic_accounting_error::corrupt_evidence;
		*encoded = std::move(output.bytes);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

} // namespace
#endif

// Fixed source getters evaluate no lower observer/query or storage operation.
bool economic_plan_normalize_source_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_normalize_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_plan_encode_source_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_encode_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_plan_digest_source_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_digest_source;
	return true;
#else
	(void)out;
	return false;
#endif
}

bool economic_plan_normalize_initial_inline_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_entry_inline;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_plan_encode_initial_inline_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_entry_inline;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_plan_digest_initial_inline_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_digest_entry_inline;
	return true;
#else
	(void)out;
	return false;
#endif
}

economic_accounting_error economic_plan_normalize_bounded(economic_accounting_plan *plan,
							  bool (*reserve)(size_t, void *) noexcept,
							  void *context, size_t outer) noexcept
{
	if (!plan)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(*plan))
		return economic_accounting_error::capacity;
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!reserve || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return economic_accounting_error::capacity;
	size_t source = 0;
	if (!economic_plan_normalize_source_frame_bytes(&source))
		return economic_accounting_error::capacity;
	economic_accounting_plan_allocation_profile profile;
	plan_bounded_budget budget{ reserve, context, outer, source };
	if (!budget.peak())
		return economic_accounting_error::capacity;
	const auto status = economic_plan_allocation_preflight(*plan, &profile);
	if (status != economic_accounting_error::ok)
		return status;
	if (!profile.storage_policy_supported)
		return economic_accounting_error::capacity;
	budget.working = profile.normalize_working_bytes;
	if (!budget.peak())
		return economic_accounting_error::capacity;
	return plan_bound_normalize_owned(plan, budget);
#else
	(void)reserve;
	(void)context;
	(void)outer;
	return economic_accounting_error::capacity;
#endif
}

economic_accounting_error economic_plan_encode_bounded(const economic_accounting_plan &plan,
						       std::vector<uint8_t> *encoded,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer) noexcept
{
	if (!encoded)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!reserve || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return economic_accounting_error::capacity;
	size_t source = 0;
	if (!economic_plan_encode_source_frame_bytes(&source))
		return economic_accounting_error::capacity;
	economic_accounting_plan_allocation_profile profile;
	plan_bounded_budget budget{ reserve, context, outer, source };
	if (!budget.peak())
		return economic_accounting_error::capacity;
	const auto status = economic_plan_allocation_preflight(plan, &profile);
	if (status != economic_accounting_error::ok)
		return status;
	if (!profile.storage_policy_supported)
		return economic_accounting_error::capacity;
	budget.working = profile.encode_working_bytes;
	if (!budget.peak())
		return economic_accounting_error::capacity;
	return plan_bound_encode_owned(plan, encoded, budget);
#else
	(void)reserve;
	(void)context;
	(void)outer;
	return economic_accounting_error::capacity;
#endif
}

economic_accounting_error economic_plan_digest_bounded(const economic_accounting_plan &plan,
						       economic_digest *digest,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer) noexcept
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!reserve || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return economic_accounting_error::capacity;
	size_t source = 0;
	if (!economic_plan_digest_source_frame_bytes(&source))
		return economic_accounting_error::capacity;
	economic_accounting_plan_allocation_profile profile;
	plan_bounded_budget budget{ reserve, context, outer, source };
	// Caller preadmits this actual empty vector inline before entry. It is not
	// part of the encoding workspace and survives the entire encoding child.
	std::vector<uint8_t> encoded;
	budget.working = sizeof(encoded);
	if (!budget.peak())
		return economic_accounting_error::capacity;
	const auto status = economic_plan_allocation_preflight(plan, &profile);
	if (status != economic_accounting_error::ok)
		return status;
	if (!profile.storage_policy_supported ||
	    !plan_storage_add(budget.working, profile.encode_working_bytes) || !budget.peak())
		return economic_accounting_error::capacity;
	const auto encoded_status = plan_bound_encode_owned(plan, &encoded, budget);
	if (encoded_status != economic_accounting_error::ok)
		return encoded_status;
	// Both normalization clones and writer are now dead; observe the real
	// surviving encoded capacity instead of retaining a prospective baseline.
	budget.working = sizeof(encoded);
	if (!plan_storage_add(budget.working, encoded.capacity()) ||
	    !budget.peak(sizeof(economic_digest) + sizeof(SHA256_CTX)))
		return economic_accounting_error::capacity;
	economic_digest result = {};
	SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	if (SHA256_Init(&digest_context) != 1 ||
	    SHA256_Update(&digest_context, encoded.data(), encoded.size()) != 1 ||
	    SHA256_Final(result.data(), &digest_context) != 1)
		return economic_accounting_error::corrupt_evidence;
#pragma GCC diagnostic pop
	*digest = result;
	return economic_accounting_error::ok;
#else
	(void)reserve;
	(void)context;
	(void)outer;
	return economic_accounting_error::capacity;
#endif
}

bool economic_command_binding_digest_source_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	size_t full = 0, supplement = 0;
	if (!plan_bound_binding_profiles(full, supplement))
		return false;
	*out = full;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_command_binding_digest_source_supplement_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	size_t full = 0, supplement = 0;
	if (!plan_bound_binding_profiles(full, supplement))
		return false;
	*out = supplement;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool economic_command_binding_digest_initial_inline_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = sizeof(binding_digest_budget);
	return true;
#else
	(void)out;
	return false;
#endif
}

bool economic_operation_metadata_validate_source_frame_bytes(size_t *out) noexcept
{
#if defined(DURIS_PLAN_BOUNDED_SOURCE_TARGET)
	if (!out)
		return false;
	*out = plan_bound_metadata_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
#undef DURIS_PLAN_BOUNDED_SOURCE_TARGET
