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
	{ economic_reason::service_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::training_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::locker_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::shipping_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::insurance_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::guild_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::crafting_cost, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::gambling_stake, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::gambling_payout, 130, economic_actor_kind::domain, true, false, false },
	{ economic_reason::refund, 382, economic_actor_kind::domain, true, true, true },
	{ economic_reason::shop_buy, 326, economic_actor_kind::domain, false, false, false },
	{ economic_reason::shop_sell, 198, economic_actor_kind::domain, true, false, false },
	{ economic_reason::shop_cleanup, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::collector_purchase, 326, economic_actor_kind::domain, false, false,
	  false },
	{ economic_reason::collector_custody, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_listing, 258, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_bid, 306, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_outbid, 306, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_cancel, 306, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_settle, 306, economic_actor_kind::domain, false, false, false },
	{ economic_reason::auction_claim, 306, economic_actor_kind::domain, false, false, false },
	{ economic_reason::item_move, 0, economic_actor_kind::domain, false, false, false },
	{ economic_reason::item_create, 136, economic_actor_kind::domain, true, false, false },
	{ economic_reason::item_destroy, 264, economic_actor_kind::domain, true, false, false },
	{ economic_reason::first_admission, 138, economic_actor_kind::domain, true, false, false },
	{ economic_reason::death_transfer, 382, economic_actor_kind::domain, false, false, false },
	{ economic_reason::corpse_restore, 126, economic_actor_kind::domain, false, false, false },
	{ economic_reason::baseline, 638, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::correction, 126, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::restitution, 1150, economic_actor_kind::operator_action, true, false,
	  false },
	{ economic_reason::lifecycle_retirement, 382, economic_actor_kind::operator_action, true,
	  false, false },
	{ economic_reason::epoch_transition, 638, economic_actor_kind::operator_action, true, false,
	  false },
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
		zeros(8);
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
		zeros(8);
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
} // namespace

bool economic_source_event_valid(const economic_source_event &event)
{
	return event.kind >= economic_source_kind::quest_completion &&
	       event.kind <= economic_source_kind::service &&
	       !critical_operation_id_is_zero(event.source) &&
	       !critical_operation_id_is_zero(event.generation);
}

economic_accounting_error
economic_source_event_encode(const economic_source_event &event,
			     std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> *encoded)
{
	if (!encoded || !economic_source_event_valid(event))
		return economic_accounting_error::invalid_identity;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> result = {};
	const auto kind = static_cast<uint16_t>(event.kind);
	result[0] = static_cast<uint8_t>(kind);
	result[1] = static_cast<uint8_t>(kind >> 8);
	result[2] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION);
	result[3] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION >> 8);
	std::copy(event.source.bytes.begin(), event.source.bytes.end(), result.begin() + 4);
	std::copy(event.generation.bytes.begin(), event.generation.bytes.end(),
		  result.begin() + 20);
	for (size_t byte = 0; byte < 8; ++byte)
		result[36 + byte] = static_cast<uint8_t>(event.sequence >> (byte * 8));
	for (size_t byte = 0; byte < 4; ++byte)
		result[44 + byte] = static_cast<uint8_t>(event.slot >> (byte * 8));
	*encoded = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_source_event_decode(std::span<const uint8_t> encoded,
						       economic_source_event *event)
{
	if (!event || encoded.size() != ECONOMIC_SOURCE_EVENT_BYTES)
		return economic_accounting_error::corrupt_evidence;
	reader input{ encoded };
	economic_source_event result = {};
	result.kind = static_cast<economic_source_kind>(input.integer<uint16_t>());
	if (input.integer<uint16_t>() != ECONOMIC_ACCOUNTING_VERSION)
		return economic_accounting_error::invalid_version;
	result.source = input.id();
	result.generation = input.id();
	result.sequence = input.integer<uint64_t>();
	result.slot = input.integer<uint32_t>();
	if (!input.good || !economic_source_event_valid(result))
		return economic_accounting_error::invalid_identity;
	*event = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_plan_validate_structure(const economic_accounting_plan &plan)
{
	if (!sizes_valid(plan))
		return economic_accounting_error::capacity;
	const auto &meta = plan.metadata;
	if (meta.version != ECONOMIC_ACCOUNTING_VERSION || meta.policy_version != 1 ||
	    meta.compiler_version != 1)
		return economic_accounting_error::invalid_version;
	if (critical_operation_id_is_zero(meta.lineage) ||
	    critical_operation_id_is_zero(meta.epoch) ||
	    critical_operation_id_is_zero(meta.operation_id) || !meta.actor_id || !meta.writer_id ||
	    zero(meta.intent_digest) || zero(meta.domain_digest) ||
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
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION)
		return economic_accounting_error::invalid_version;
	if (command.keys.size() > CRITICAL_COMMAND_MAX_KEYS ||
	    command.expected_revisions.size() > CRITICAL_COMMAND_MAX_KEYS ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return economic_accounting_error::capacity;
	try
	{
		auto projection = command;
		// Only this binding projection uses a sentinel. Actual admission,
		// journal bytes, exact-ID equality and durable receipts keep real time.
		projection.accepted_at_usec = 1;
		if (!critical_command_normalize(&projection))
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
