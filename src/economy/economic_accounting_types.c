#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <limits>
#include <new>
#include <tuple>
#include <vector>

namespace
{
using wide_integer = __int128_t;
constexpr std::array<int64_t, 4> COPPER_UNITS = { 1, 10, 100, 1000 };

bool narrow(wide_integer value, int64_t *output)
{
	if (!output || value < std::numeric_limits<int64_t>::min() ||
	    value > std::numeric_limits<int64_t>::max())
		return false;
	*output = static_cast<int64_t>(value);
	return true;
}

bool nonnegative(const economic_coin_vector &value)
{
	return std::all_of(value.begin(), value.end(), [](int64_t part) { return part >= 0; });
}

bool all_zero(const economic_coin_vector &value)
{
	return std::all_of(value.begin(), value.end(), [](int64_t part) { return part == 0; });
}

void put_u64(std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> &output, size_t offset, uint64_t value)
{
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		output[offset + byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint64_t get_u64(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		value |= static_cast<uint64_t>(input[offset + byte]) << (byte * 8);
	return value;
}
} // namespace

bool economic_account_kind_valid(economic_account_kind kind)
{
	return kind >= economic_account_kind::wallet && kind <= economic_account_kind::restitution;
}

bool economic_account_is_ordinary(economic_account_kind kind)
{
	return kind >= economic_account_kind::wallet && kind <= economic_account_kind::treasury;
}

bool economic_account_key_valid(const economic_account_key &key)
{
	return !critical_operation_id_is_zero(key.lineage) &&
	       economic_account_kind_valid(key.kind) && key.authority_id != 0;
}

bool economic_account_key_equal(const economic_account_key &left, const economic_account_key &right)
{
	return left.lineage.bytes == right.lineage.bytes && left.kind == right.kind &&
	       left.authority_id == right.authority_id && left.context_id == right.context_id;
}

bool economic_account_key_less(const economic_account_key &left, const economic_account_key &right)
{
	return std::tie(left.lineage.bytes, left.kind, left.authority_id, left.context_id) <
	       std::tie(right.lineage.bytes, right.kind, right.authority_id, right.context_id);
}

economic_accounting_error
economic_account_key_encode(const economic_account_key &key,
			    std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> *encoded)
{
	if (!encoded || !economic_account_key_valid(key))
		return economic_accounting_error::invalid_identity;
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> result = {};
	std::copy(key.lineage.bytes.begin(), key.lineage.bytes.end(), result.begin());
	result[16] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION);
	result[17] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION >> 8);
	const auto kind = static_cast<uint16_t>(key.kind);
	result[18] = static_cast<uint8_t>(kind);
	result[19] = static_cast<uint8_t>(kind >> 8);
	put_u64(result, 20, key.authority_id);
	put_u64(result, 28, key.context_id);
	*encoded = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_account_key_decode(std::span<const uint8_t> encoded,
						      economic_account_key *key)
{
	if (!key || encoded.size() != ECONOMIC_ACCOUNT_KEY_BYTES)
		return economic_accounting_error::corrupt_evidence;
	if (encoded[16] != ECONOMIC_ACCOUNTING_VERSION || encoded[17] != 0)
		return economic_accounting_error::invalid_version;
	for (size_t index = 36; index < ECONOMIC_ACCOUNT_KEY_BYTES; ++index)
		if (encoded[index] != 0)
			return economic_accounting_error::corrupt_evidence;
	economic_account_key result = {};
	std::copy_n(encoded.begin(), result.lineage.bytes.size(), result.lineage.bytes.begin());
	result.kind = static_cast<economic_account_kind>(static_cast<uint16_t>(encoded[18]) |
							 (static_cast<uint16_t>(encoded[19]) << 8));
	result.authority_id = get_u64(encoded, 20);
	result.context_id = get_u64(encoded, 28);
	if (!economic_account_key_valid(result))
		return economic_accounting_error::invalid_identity;
	*key = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_coin_value(const economic_coin_vector &vector, int64_t *value)
{
	wide_integer total = 0;
	for (size_t index = 0; index < vector.size(); ++index)
		total += static_cast<wide_integer>(vector[index]) * COPPER_UNITS[index];
	return narrow(total, value) ? economic_accounting_error::ok :
				      economic_accounting_error::overflow;
}

economic_accounting_error economic_coin_delta(const economic_coin_vector &before,
					      const economic_coin_vector &after,
					      economic_coin_vector *delta)
{
	if (!delta)
		return economic_accounting_error::corrupt_evidence;
	if (!nonnegative(before) || !nonnegative(after))
		return economic_accounting_error::negative_holding;
	int64_t ignored = 0;
	if (economic_coin_value(before, &ignored) != economic_accounting_error::ok ||
	    economic_coin_value(after, &ignored) != economic_accounting_error::ok)
		return economic_accounting_error::overflow;
	economic_coin_vector result = {};
	for (size_t index = 0; index < result.size(); ++index)
		if (!narrow(static_cast<wide_integer>(after[index]) - before[index],
			    &result[index]))
			return economic_accounting_error::overflow;
	*delta = result;
	return economic_accounting_error::ok;
}

economic_accounting_error
economic_coin_effects_validate(std::span<const economic_account_effect> effects,
			       std::span<const economic_coin_posting> postings, size_t child_count)
{
	if (effects.size() > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS ||
	    postings.size() > ECONOMIC_ACCOUNTING_MAX_POSTINGS ||
	    child_count > ECONOMIC_ACCOUNTING_MAX_CHILDREN)
		return economic_accounting_error::capacity;
	try
	{
		std::vector<std::array<wide_integer, 4>> totals(effects.size());
		std::vector<bool> referenced(effects.size(), false);
		for (size_t index = 0; index < effects.size(); ++index)
		{
			const auto &effect = effects[index];
			if (!economic_account_key_valid(effect.key) ||
			    (index &&
			     !economic_account_key_less(effects[index - 1].key, effect.key)))
				return economic_accounting_error::invalid_identity;
			if (index && effect.key.lineage.bytes != effects[0].key.lineage.bytes)
				return economic_accounting_error::invalid_identity;
			if (economic_account_is_ordinary(effect.key.kind))
			{
				economic_coin_vector ignored = {};
				const auto status =
					economic_coin_delta(effect.before, effect.after, &ignored);
				if (status != economic_accounting_error::ok)
					return status;
				if (effect.after_revision < effect.before_revision ||
				    (effect.before != effect.after &&
				     effect.after_revision == effect.before_revision))
					return economic_accounting_error::stale_revision;
			}
			else if (!all_zero(effect.before) || !all_zero(effect.after) ||
				 effect.before_revision || effect.after_revision)
				return economic_accounting_error::corrupt_evidence;
		}
		wide_integer balance = 0;
		for (size_t index = 0; index < postings.size(); ++index)
		{
			const auto &posting = postings[index];
			if (posting.event_index != index)
				return economic_accounting_error::duplicate_event;
			if (posting.account_index >= effects.size() ||
			    posting.child_index > child_count)
				return economic_accounting_error::invalid_identity;
			if (all_zero(posting.delta))
				return economic_accounting_error::corrupt_evidence;
			int64_t value = 0;
			const auto status = economic_coin_value(posting.delta, &value);
			if (status != economic_accounting_error::ok)
				return status;
			if (value != posting.copper)
				return economic_accounting_error::corrupt_evidence;
			balance += value;
			referenced[posting.account_index] = true;
			for (size_t part = 0; part < posting.delta.size(); ++part)
				totals[posting.account_index][part] += posting.delta[part];
		}
		if (balance != 0)
			return economic_accounting_error::unbalanced;
		for (size_t index = 0; index < effects.size(); ++index)
		{
			const auto &effect = effects[index];
			if (!referenced[index] && !(economic_account_is_ordinary(effect.key.kind) &&
						    effect.before == effect.after &&
						    effect.after_revision > effect.before_revision))
				return economic_accounting_error::corrupt_evidence;
			if (!economic_account_is_ordinary(effect.key.kind))
				continue;
			for (size_t part = 0; part < effect.before.size(); ++part)
				if (static_cast<wide_integer>(effect.before[part]) +
					    totals[index][part] !=
				    effect.after[part])
					return economic_accounting_error::corrupt_evidence;
		}
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_child_links_validate(const critical_operation_id &root,
							std::span<const economic_child_link> links)
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
		if (!critical_operation_id_derive(parent, link.domain, link.discriminator,
						  &expected) ||
		    !critical_operation_id_equal(expected, link.operation_id))
			return economic_accounting_error::payload_conflict;
		for (size_t prior = 0; prior < index; ++prior)
			if (critical_operation_id_equal(links[prior].operation_id,
							link.operation_id))
				return economic_accounting_error::duplicate_event;
	}
	return economic_accounting_error::ok;
}

namespace
{
bool live_custody(item_custody_state state)
{
	return state == item_custody_state::active || state == item_custody_state::quarantined;
}

bool item_position_valid(uint64_t uid, const economic_item_position &position)
{
	if (position.state == item_custody_state::absent)
		return position.owner.type == item_owner_type::unknown && !position.owner.id &&
		       !position.owner.context_id && !position.root_uid && !position.parent_uid &&
		       !position.revision;
	if (!item_owner_identity_valid(position.owner))
		return false;
	if (position.state == item_custody_state::destroyed)
		// Existing authority keeps a destroyed child's former root/parent.
		// This is retained evidence, not a current live containment edge.
		return position.owner.type == item_owner_type::destruction &&
		       position.root_uid != 0 && position.parent_uid != uid &&
		       position.revision != 0;
	return live_custody(position.state) &&
	       position.owner.type != item_owner_type::destruction && position.root_uid != 0 &&
	       position.parent_uid != uid && (position.parent_uid != 0 || position.root_uid == uid);
}

size_t item_index(std::span<const economic_item_snapshot> items, uint64_t uid)
{
	const auto found = std::lower_bound(items.begin(), items.end(), uid,
					    [](const economic_item_snapshot &item, uint64_t value)
					    { return item.uid < value; });
	return found == items.end() || found->uid != uid ?
		       items.size() :
		       static_cast<size_t>(found - items.begin());
}

economic_accounting_error item_forest_validate(std::span<const economic_item_snapshot> items)
{
	for (size_t index = 0; index < items.size(); ++index)
		if (!items[index].uid || (index && items[index - 1].uid >= items[index].uid) ||
		    !item_position_valid(items[index].uid, items[index].position))
			return economic_accounting_error::invalid_identity;
	std::vector<uint8_t> color(items.size(), 0);
	std::vector<size_t> path;
	path.reserve(items.size());
	for (size_t start = 0; start < items.size(); ++start)
	{
		if (color[start] == 2 || !live_custody(items[start].position.state))
			continue;
		path.clear();
		size_t current = start;
		while (color[current] != 2)
		{
			if (color[current] == 1)
				return economic_accounting_error::topology;
			color[current] = 1;
			path.push_back(current);
			const auto &position = items[current].position;
			if (!position.parent_uid)
				break;
			const size_t parent = item_index(items, position.parent_uid);
			if (parent == items.size() || !live_custody(items[parent].position.state) ||
			    !item_owner_identity_equal(position.owner,
						       items[parent].position.owner) ||
			    position.root_uid != items[parent].position.root_uid)
				return economic_accounting_error::topology;
			current = parent;
		}
		for (size_t index : path)
			color[index] = 2;
	}
	return economic_accounting_error::ok;
}
} // namespace

bool economic_item_position_equal(const economic_item_position &left,
				  const economic_item_position &right)
{
	return item_owner_identity_equal(left.owner, right.owner) &&
	       left.root_uid == right.root_uid && left.parent_uid == right.parent_uid &&
	       left.revision == right.revision && left.state == right.state;
}

economic_accounting_error
economic_item_effects_validate(std::span<const economic_item_snapshot> before,
			       std::span<const economic_item_snapshot> after,
			       std::span<const economic_item_event> events, size_t child_count)
{
	if (before.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
	    after.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES ||
	    events.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
	    child_count > ECONOMIC_ACCOUNTING_MAX_CHILDREN)
		return economic_accounting_error::capacity;
	if (before.size() != after.size())
		return economic_accounting_error::corrupt_evidence;
	try
	{
		auto status = item_forest_validate(before);
		if (status != economic_accounting_error::ok)
			return status;
		status = item_forest_validate(after);
		if (status != economic_accounting_error::ok)
			return status;
		for (size_t index = 0; index < before.size(); ++index)
			if (before[index].uid != after[index].uid)
				return economic_accounting_error::invalid_identity;
		std::vector<economic_item_snapshot> current(before.begin(), before.end());
		for (size_t index = 0; index < events.size(); ++index)
		{
			const auto &event = events[index];
			if (event.event_index != index)
				return economic_accounting_error::duplicate_event;
			const size_t target = item_index(current, event.uid);
			if (target == current.size() || event.child_index > child_count ||
			    !item_position_valid(event.uid, event.after))
				return economic_accounting_error::invalid_identity;
			if (!economic_item_position_equal(current[target].position, event.before))
				return economic_accounting_error::stale_revision;
			// Restoration from a retained destruction tombstone is not normal
			// creation. A separately reviewed correction adapter must handle it.
			if (event.before.state == item_custody_state::destroyed ||
			    event.after.state == item_custody_state::absent ||
			    (event.before.state == item_custody_state::absent &&
			     !live_custody(event.after.state)))
				return economic_accounting_error::unauthorized;
			if (event.after.revision <= event.before.revision)
				return economic_accounting_error::stale_revision;
			current[target].position = event.after;
		}
		for (size_t index = 0; index < current.size(); ++index)
			if (!economic_item_position_equal(current[index].position,
							  after[index].position))
				return economic_accounting_error::corrupt_evidence;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}
