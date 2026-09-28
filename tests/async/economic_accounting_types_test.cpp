#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#define CHECK(condition)                                                                           \
	do                                                                                         \
	{                                                                                          \
		if (!(condition))                                                                  \
		{                                                                                  \
			std::cerr << "check failed at " << __LINE__ << ": " << #condition << '\n'; \
			std::abort();                                                              \
		}                                                                                  \
	} while (false)
using error = economic_accounting_error;

economic_account_key key(economic_account_kind kind, uint64_t id, uint64_t context = 0)
{
	economic_account_key result = {};
	result.lineage.bytes[0] = 1;
	result.kind = kind;
	result.authority_id = id;
	result.context_id = context;
	return result;
}

economic_coin_vector coins(int64_t amount)
{
	return { amount % 10, amount / 10 % 10, amount / 100 % 10, amount / 1000 };
}

void identities()
{
	auto original = key(economic_account_kind::bank, UINT64_C(0xfedcba9876543210));
	original.context_id = 7;
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
	CHECK(economic_account_key_encode(original, &encoded) == error::ok);
	CHECK(encoded[0] == 1 && encoded[16] == 1 && encoded[18] == 2);
	CHECK(encoded[20] == 0x10 && encoded[27] == 0xfe && encoded[28] == 7);
	economic_account_key decoded = {};
	CHECK(economic_account_key_decode(encoded, &decoded) == error::ok);
	CHECK(economic_account_key_equal(original, decoded));
	for (size_t size = 0; size < encoded.size(); ++size)
	{
		CHECK(economic_account_key_decode(std::span(encoded).first(size), &decoded) !=
		      error::ok);
		CHECK(economic_account_key_equal(original, decoded));
	}
	for (size_t index = 36; index < encoded.size(); ++index)
	{
		auto changed = encoded;
		changed[index] = 1;
		CHECK(economic_account_key_decode(changed, &decoded) == error::corrupt_evidence);
	}
	auto changed = encoded;
	changed[16] = 2;
	CHECK(economic_account_key_decode(changed, &decoded) == error::invalid_version);
	changed = encoded;
	changed[18] = 255;
	CHECK(economic_account_key_decode(changed, &decoded) == error::invalid_identity);
	changed = encoded;
	std::fill_n(changed.begin(), 16, 0);
	CHECK(economic_account_key_decode(changed, &decoded) == error::invalid_identity);
	CHECK(economic_account_key_equal(original, decoded));
	CHECK(economic_account_key_less(key(economic_account_kind::wallet, 99), original));
	auto renamed = original; // display aliases are deliberately not encoded.
	CHECK(economic_account_key_equal(original, renamed));
	++renamed.authority_id;
	CHECK(!economic_account_key_equal(original, renamed));
}

void arithmetic()
{
	int64_t value = 123;
	CHECK(economic_coin_value({ 0, 0, 0, INT64_MAX }, &value) == error::overflow);
	CHECK(value == 123);
	CHECK(economic_coin_value({ INT64_MAX, 0, 0, 0 }, &value) == error::ok &&
	      value == INT64_MAX);
	CHECK(economic_coin_value({ INT64_MIN, 0, 0, 0 }, &value) == error::ok &&
	      value == INT64_MIN);
	CHECK(economic_coin_value({ INT64_MAX, INT64_MAX, -INT64_MAX, 0 }, &value) ==
	      error::overflow);
	CHECK(economic_coin_value({ 0, INT64_MAX, -INT64_MAX / 10, 0 }, &value) == error::ok &&
	      value == 70);
	economic_coin_vector delta = { 9, 9, 9, 9 };
	CHECK(economic_coin_delta({ 0, 0, 0, 1 }, { 3, 6, 8, 0 }, &delta) == error::ok);
	CHECK((delta == economic_coin_vector{ 3, 6, 8, -1 }));
	auto saved = delta;
	CHECK(economic_coin_delta({ -1, 0, 0, 0 }, {}, &delta) == error::negative_holding);
	CHECK(delta == saved);
}

void randomized_transfers()
{
	std::mt19937_64 rng(474);
	for (size_t iteration = 0; iteration < 5000; ++iteration)
	{
		const int64_t start = 1 + static_cast<int64_t>(rng() % 1000000000);
		const int64_t amount =
			1 + static_cast<int64_t>(rng() % static_cast<uint64_t>(start));
		const int64_t other = static_cast<int64_t>(rng() % 1000000000);
		std::vector<economic_account_effect> effects = {
			{ key(economic_account_kind::wallet, 1), coins(start),
			  coins(start - amount), 5, 6 },
			{ key(economic_account_kind::wallet, 2), coins(other),
			  coins(other + amount), 9, 10 }
		};
		std::vector<economic_coin_posting> postings(2);
		for (uint16_t index = 0; index < 2; ++index)
		{
			postings[index].event_index = index;
			postings[index].account_index = index;
			CHECK(economic_coin_delta(effects[index].before, effects[index].after,
						  &postings[index].delta) == error::ok);
			CHECK(economic_coin_value(postings[index].delta, &postings[index].copper) ==
			      error::ok);
		}
		CHECK(economic_coin_effects_validate(effects, postings, 0) == error::ok);
		// Reordering audit legs preserves all evidence when indexes follow order.
		std::swap(postings[0], postings[1]);
		postings[0].event_index = 0;
		postings[1].event_index = 1;
		CHECK(economic_coin_effects_validate(effects, postings, 0) == error::ok);
		++effects[0].after[0];
		CHECK(economic_coin_effects_validate(effects, postings, 0) ==
		      error::corrupt_evidence);
		--effects[0].after[0];
		++postings[0].copper;
		CHECK(economic_coin_effects_validate(effects, postings, 0) ==
		      error::corrupt_evidence);
	}
}

void limits_and_rejections()
{
	std::vector<economic_account_effect> effects;
	std::vector<economic_coin_posting> postings;
	for (uint16_t index = 0; index < 3000; ++index)
	{
		effects.push_back(
			{ key(economic_account_kind::pile, index + 1U), {}, { 1, 0, 0, 0 }, 0, 1 });
		postings.push_back(
			{ static_cast<uint32_t>(postings.size()), index, 0, { 1, 0, 0, 0 }, 1 });
		postings.push_back(
			{ static_cast<uint32_t>(postings.size()), 3000, 0, { -1, 0, 0, 0 }, -1 });
	}
	effects.push_back({ key(economic_account_kind::issuance, 1), {}, {}, 0, 0 });
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::ok);
	postings[0].delta[0] = 2;
	postings[0].copper = 2;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::unbalanced);
	postings[0].delta[0] = 1;
	postings[0].copper = 1;
	postings[0].event_index = 1;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::duplicate_event);
	postings[0].event_index = 0;
	postings[0].child_index = 1;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::invalid_identity);
	CHECK(economic_coin_effects_validate(effects, postings, 1) == error::ok);
	postings[0].child_index = 0;
	effects[0].after_revision = 0;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::stale_revision);
	effects[0].after_revision = 1;
	effects[0].key.lineage.bytes[1] = 1;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::invalid_identity);
	effects[0].key.lineage.bytes[1] = 0;
	effects[1].key = effects[0].key;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::invalid_identity);
	effects[1].key.authority_id = 2;
	effects.back().after[0] = 1;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::corrupt_evidence);
	effects.back().after[0] = 0;
	CHECK(economic_coin_effects_validate(effects, postings, 65) == error::capacity);
	effects.clear();
	postings.clear();
	for (uint16_t index = 0; index < ECONOMIC_ACCOUNTING_MAX_ACCOUNTS; ++index)
	{
		const int64_t sign = index < ECONOMIC_ACCOUNTING_MAX_ACCOUNTS / 2 ? -1 : 1;
		effects.push_back({ key(economic_account_kind::wallet, index + 1U),
				    coins(sign < 0 ? 2 : 0), coins(sign < 0 ? 0 : 2), 0, 1 });
		for (unsigned int leg = 0; leg < 2; ++leg)
			postings.push_back({ static_cast<uint32_t>(postings.size()),
					     index,
					     0,
					     { sign, 0, 0, 0 },
					     sign });
	}
	CHECK(postings.size() == ECONOMIC_ACCOUNTING_MAX_POSTINGS);
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::ok);
	effects[0].before_revision = UINT64_MAX;
	effects[0].after_revision = 0;
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::stale_revision);
	effects[0].before_revision = 0;
	effects[0].after_revision = 1;
	auto revision_touch =
		economic_account_effect{ key(economic_account_kind::bank, 1), {}, {}, 10, 11 };
	CHECK(economic_coin_effects_validate(std::span(&revision_touch, 1), {}, 0) == error::ok);
	revision_touch.after_revision = 10;
	CHECK(economic_coin_effects_validate(std::span(&revision_touch, 1), {}, 0) ==
	      error::corrupt_evidence);
	postings.resize(ECONOMIC_ACCOUNTING_MAX_POSTINGS + 1);
	CHECK(economic_coin_effects_validate(effects, postings, 0) == error::capacity);
	effects.resize(ECONOMIC_ACCOUNTING_MAX_ACCOUNTS + 1);
	CHECK(economic_coin_effects_validate(effects, {}, 0) == error::capacity);
}

void operation_links()
{
	critical_operation_id root = {};
	root.bytes[0] = 1;
	std::vector<economic_child_link> links(ECONOMIC_ACCOUNTING_MAX_CHILDREN);
	for (size_t index = 0; index < links.size(); ++index)
	{
		links[index].domain = 474;
		links[index].discriminator = index;
		links[index].parent_index = static_cast<uint16_t>(index);
		const auto &parent = index ? links[index - 1].operation_id : root;
		CHECK(critical_operation_id_derive(parent, 474, index, &links[index].operation_id));
	}
	CHECK(economic_child_links_validate(root, links) == error::ok);
	auto changed = links;
	changed[0].discriminator = 99;
	CHECK(economic_child_links_validate(root, changed) == error::payload_conflict);
	changed = links;
	changed[0].parent_index = 1;
	CHECK(economic_child_links_validate(root, changed) == error::invalid_identity);
	changed = links;
	changed[1] = changed[0];
	CHECK(economic_child_links_validate(root, changed) == error::duplicate_event);
	changed = links;
	changed[0].operation_id = root;
	CHECK(economic_child_links_validate(root, changed) == error::invalid_identity);
	CHECK(economic_child_links_validate({}, links) == error::invalid_identity);
	links.resize(ECONOMIC_ACCOUNTING_MAX_CHILDREN + 1);
	CHECK(economic_child_links_validate(root, links) == error::capacity);
}

economic_item_position position(uint64_t owner, uint64_t root, uint64_t parent, uint64_t revision)
{
	return { { item_owner_type::player, owner, 0 },
		 root,
		 parent,
		 revision,
		 item_custody_state::active };
}

void item_effects()
{
	std::vector<economic_item_snapshot> before = { { 1, position(1, 1, 0, 1) },
						       { 2, position(1, 2, 0, 1) } };
	auto after = before;
	after[1].position = position(1, 1, 1, 2);
	std::vector<economic_item_event> events = { { 0, 0, 2, before[1].position,
						      after[1].position } };
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::ok);
	auto restored = position(1, 2, 0, 3);
	events.push_back({ 1, 0, 2, after[1].position, restored });
	after[1].position = restored;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::ok);
	events[1].before.revision = 1;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::stale_revision);
	events[1].before.revision = 2;
	events[1].event_index = 0;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::duplicate_event);
	events[1].event_index = 1;
	events[1].child_index = 1;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::invalid_identity);
	CHECK(economic_item_effects_validate(before, after, events, 1) == error::ok);
	CHECK(economic_item_effects_validate(before, after, {}, 0) == error::corrupt_evidence);
	auto cyclic = before;
	cyclic[0].position.parent_uid = 2;
	cyclic[1].position.root_uid = 1;
	cyclic[1].position.parent_uid = 1;
	CHECK(economic_item_effects_validate(before, cyclic, events, 1) == error::topology);
	auto missing_parent = before;
	missing_parent[1].position.root_uid = 1;
	missing_parent[1].position.parent_uid = 99;
	CHECK(economic_item_effects_validate(before, missing_parent, {}, 0) == error::topology);
	auto duplicate = before;
	duplicate[1].uid = 1;
	CHECK(economic_item_effects_validate(duplicate, after, events, 1) ==
	      error::invalid_identity);
	auto retired = before;
	retired[1].position = {
		{ item_owner_type::destruction, 0, 0 }, 2, 0, 2, item_custody_state::destroyed
	};
	events = { { 0, 0, 2, before[1].position, retired[1].position } };
	CHECK(economic_item_effects_validate(before, retired, events, 0) == error::ok);
	auto resurrected = before;
	resurrected[1].position.revision = 3;
	events = { { 0, 0, 2, retired[1].position, resurrected[1].position } };
	CHECK(economic_item_effects_validate(retired, resurrected, events, 0) ==
	      error::unauthorized);
	auto nested_before = before;
	nested_before[1].position = position(1, 1, 1, 1);
	auto nested_dead = nested_before;
	events.clear();
	for (size_t index = 0; index < nested_dead.size(); ++index)
	{
		auto &dead = nested_dead[index].position;
		dead.owner = { item_owner_type::destruction, 0, 0 };
		dead.state = item_custody_state::destroyed;
		dead.revision = 2;
		events.push_back({ static_cast<uint32_t>(index), 0, nested_dead[index].uid,
				   nested_before[index].position, dead });
	}
	CHECK(economic_item_effects_validate(nested_before, nested_dead, events, 0) == error::ok);
	CHECK(economic_item_effects_validate(nested_dead, nested_dead, {}, 0) == error::ok);
	auto absent = before;
	absent[1].position = {};
	events = { { 0, 0, 2, {}, before[1].position } };
	CHECK(economic_item_effects_validate(absent, before, events, 0) == error::ok);
	events = { { 0, 0, 2, before[1].position, {} } };
	CHECK(economic_item_effects_validate(before, absent, events, 0) == error::unauthorized);
	auto shortened = before;
	shortened.pop_back();
	CHECK(economic_item_effects_validate(before, shortened, {}, 0) == error::corrupt_evidence);

	before.clear();
	after.clear();
	events.clear();
	for (uint64_t uid = 1; uid <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS; ++uid)
	{
		before.push_back({ uid, position(1, 1, uid - 1, 1) });
		after.push_back({ uid, position(2, 1, uid - 1, 2) });
		events.push_back({ static_cast<uint32_t>(uid - 1), 0, uid, before.back().position,
				   after.back().position });
	}
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::ok);
	events.push_back(events.back());
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::capacity);
	before.clear();
	after.clear();
	events.clear();
	for (uint64_t uid = 1; uid <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++uid)
	{
		const uint64_t root = uid <= 3000 ? 1 : 3001;
		before.push_back({ uid, position(1, root, uid == root ? 0 : uid - 1, 1) });
	}
	after = before;
	CHECK(economic_item_effects_validate(before, after, {}, 0) == error::ok);
	after.back().position.state = item_custody_state::quarantined;
	after.back().position.revision = 2;
	events = { { 0, 0, after.back().uid, before.back().position, after.back().position } };
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::ok);
	after.back().position.owner.id = 2;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::topology);
	after.back().position.owner.id = 1;
	before.back().position.revision = UINT64_MAX;
	events[0].before = before.back().position;
	CHECK(economic_item_effects_validate(before, after, events, 0) == error::stale_revision);
	before.resize(ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES + 1);
	CHECK(economic_item_effects_validate(before, {}, {}, 0) == error::capacity);
}

#include "golden.inc"

int main()
{
	item_effects();
	operation_links();
	identities();
	arithmetic();
	randomized_transfers();
	limits_and_rejections();
	golden_cases();
	std::cout
		<< "accounting types: golden coin/item effects, 5000 transfers, 3000-item forest, limits and rejection checks passed\n";
}
