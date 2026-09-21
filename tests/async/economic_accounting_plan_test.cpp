#include "economy/economic_accounting_plan.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <type_traits>

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
critical_operation_id id(uint8_t value)
{
	critical_operation_id result = {};
	result.bytes[0] = value;
	return result;
}
economic_account_key key(economic_account_kind kind, uint64_t authority, uint64_t context = 0)
{
	return { id(1), kind, authority, context };
}
economic_source_event
source_event(economic_source_kind kind = economic_source_kind::quest_completion)
{
	return { kind, id(4), id(5), UINT64_MAX, UINT32_MAX };
}
economic_accounting_plan base_plan()
{
	economic_accounting_plan result;
	auto &meta = result.metadata;
	meta.lineage = id(1);
	meta.epoch = id(2);
	meta.operation_id = id(3);
	meta.actor_kind = economic_actor_kind::domain;
	meta.actor_id = 7;
	meta.writer_id = 1;
	meta.reason = economic_reason::wallet_transfer;
	meta.intent_digest[0] = 11;
	meta.domain_digest[0] = 12;
	return result;
}
economic_accounting_plan wallet_plan()
{
	auto plan = base_plan();
	plan.accounts = {
		{ key(economic_account_kind::wallet, 1), { 0, 0, 0, 1 }, { 3, 6, 8, 0 }, 4, 5 },
		{ key(economic_account_kind::wallet, 2), {}, { 7, 3, 1, 0 }, 8, 9 }
	};
	plan.postings = { { 0, 0, 0, { 3, 6, 8, -1 }, -137 }, { 1, 1, 0, { 7, 3, 1, 0 }, 137 } };
	return plan;
}
void roundtrip(const economic_accounting_plan &plan)
{
	std::vector<uint8_t> encoded;
	const auto status = economic_plan_encode(plan, &encoded);
	if (status != error::ok)
		std::cerr << "roundtrip reason " << static_cast<unsigned>(plan.metadata.reason)
			  << " status " << static_cast<unsigned>(status) << "\n";
	CHECK(status == error::ok);
	economic_accounting_plan decoded;
	CHECK(economic_plan_decode(encoded, &decoded) == error::ok);
	std::vector<uint8_t> second;
	CHECK(economic_plan_encode(decoded, &second) == error::ok);
	CHECK(encoded == second);
	economic_digest first_hash = {}, second_hash = {};
	CHECK(economic_plan_digest(plan, &first_hash) == error::ok);
	CHECK(economic_plan_digest(decoded, &second_hash) == error::ok);
	CHECK(first_hash == second_hash);
}
#include "golden.inc"
#include "reference.inc"

void fixed_bytes_and_rejection()
{
	const auto plan = wallet_plan();
	std::vector<uint8_t> bytes;
	CHECK(economic_plan_encode(plan, &bytes) == error::ok);
	CHECK(bytes == REFERENCE_WALLET_PLAN);
	CHECK(bytes.size() == 256 + 2 * 120 + 2 * 48);
	economic_accounting_plan output = plan;
	output.metadata.writer_id = 99;
	for (size_t length = 0; length < bytes.size(); ++length)
	{
		CHECK(economic_plan_decode(std::span(bytes).first(length), &output) != error::ok);
		CHECK(output.metadata.writer_id == 99);
	}
	auto changed = bytes;
	changed.push_back(0);
	CHECK(economic_plan_decode(changed, &output) == error::corrupt_evidence);
	for (size_t offset : { 6U, 7U, 73U, 74U, 75U, 98U, 99U, 101U, 102U, 103U, 240U, 255U })
	{
		changed = bytes;
		changed[offset] = 1;
		CHECK(economic_plan_decode(changed, &output) == error::corrupt_evidence);
	}
	changed = bytes;
	changed[4] = 2;
	CHECK(economic_plan_decode(changed, &output) == error::invalid_version);
	changed = bytes;
	changed[216] = 255;
	changed[217] = 255; // account count before allocation
	CHECK(economic_plan_decode(changed, &output) == error::capacity);
	changed = bytes;
	changed[104] = 1; // source bytes present with absent flag
	CHECK(economic_plan_decode(changed, &output) == error::corrupt_evidence);
	auto invalid = plan;
	invalid.metadata.policy_version = 2;
	std::vector<uint8_t> sentinel = { 1, 2, 3 };
	CHECK(economic_plan_encode(invalid, &sentinel) == error::invalid_version);
	CHECK((sentinel == std::vector<uint8_t>{ 1, 2, 3 }));
	invalid = plan;
	invalid.postings[0].copper = -136;
	CHECK(economic_plan_normalize(&invalid) != error::ok);
	CHECK(invalid.postings[0].copper == -136);
	economic_digest digest = {};
	digest[0] = 99;
	CHECK(economic_plan_digest(invalid, &digest) != error::ok && digest[0] == 99);

	std::mt19937_64 rng(476);
	for (size_t iteration = 0; iteration < 1000; ++iteration)
	{
		changed = bytes;
		changed[rng() % changed.size()] ^= static_cast<uint8_t>(1U << (rng() % 8));
		if (economic_plan_decode(changed, &output) == error::ok)
		{
			std::vector<uint8_t> again;
			CHECK(economic_plan_encode(output, &again) == error::ok &&
			      again == changed);
		}
	}
}

void canonical_permutations()
{
	auto plan = wallet_plan();
	plan.children.resize(3);
	for (size_t index = 0; index < 3; ++index)
	{
		auto &child = plan.children[index];
		child.domain = 476;
		child.discriminator = index;
		child.parent_index = index == 2 ? 1 : 0;
		const auto &parent = child.parent_index ?
					     plan.children[child.parent_index - 1].operation_id :
					     plan.metadata.operation_id;
		CHECK(critical_operation_id_derive(parent, child.domain, child.discriminator,
						   &child.operation_id));
	}
	plan.postings[0].child_index = 3;
	plan.postings[1].child_index = 2;
	std::vector<uint8_t> original;
	CHECK(economic_plan_encode(plan, &original) == error::ok);
	auto shuffled = plan;
	std::swap(shuffled.accounts[0], shuffled.accounts[1]);
	for (auto &posting : shuffled.postings)
		posting.account_index = 1 - posting.account_index;
	std::reverse(shuffled.postings.begin(), shuffled.postings.end());
	// Move the child before its parent; remap all references with the rows.
	std::swap(shuffled.children[0], shuffled.children[2]);
	shuffled.children[0].parent_index = 3;
	for (auto &posting : shuffled.postings)
		if (posting.child_index == 3)
			posting.child_index = 1;
	std::vector<uint8_t> second;
	CHECK(economic_plan_encode(shuffled, &second) == error::ok);
	CHECK(original == second);
	roundtrip(shuffled);
	auto cyclic = shuffled;
	cyclic.children[2].parent_index = 1;
	CHECK(economic_plan_encode(cyclic, &second) == error::topology);
	CHECK(second == original);
}

void source_and_policy()
{
	const auto source = source_event();
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> bytes = {};
	CHECK(economic_source_event_encode(source, &bytes) == error::ok);
	CHECK(bytes[0] == 1 && bytes[2] == 1 && bytes[4] == 4 && bytes[20] == 5 &&
	      bytes[36] == 255 && bytes[47] == 255);
	economic_source_event decoded;
	CHECK(economic_source_event_decode(bytes, &decoded) == error::ok);
	CHECK(decoded.source.bytes == source.source.bytes &&
	      decoded.generation.bytes == source.generation.bytes &&
	      decoded.sequence == source.sequence && decoded.slot == source.slot);
	auto bad = bytes;
	bad[2] = 2;
	CHECK(economic_source_event_decode(bad, &decoded) == error::invalid_version);
	for (size_t length = 0; length < bytes.size(); ++length)
		CHECK(economic_source_event_decode(std::span(bytes).first(length), &decoded) !=
		      error::ok);
	auto reward = base_plan();
	reward.metadata.reason = economic_reason::quest_reward;
	reward.metadata.source_event = source;
	reward.accounts = {
		{ key(economic_account_kind::wallet, 1), {}, { 1, 0, 0, 0 }, 0, 1 },
		{ key(economic_account_kind::bank, 1), {}, {}, 4, 5 }, // actual revision-only touch
		{ key(economic_account_kind::issuance, 1), {}, {}, 0, 0 }
	};
	reward.postings = { { 0, 0, 0, { 1, 0, 0, 0 }, 1 }, { 1, 2, 0, { -1, 0, 0, 0 }, -1 } };
	roundtrip(reward);
	auto changed = reward;
	changed.metadata.source_event.reset();
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed = reward;
	changed.metadata.actor_kind = economic_actor_kind::operator_action;
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = reward;
	changed.metadata.reason = economic_reason::bank_transfer;
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = reward;
	changed.accounts[0].before = { 1, 0, 0, 0 };
	changed.accounts[0].after = {};
	for (auto &posting : changed.postings)
	{
		posting.copper = -posting.copper;
		for (auto &part : posting.delta)
			part = -part;
	}
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = reward;
	changed.metadata.original_operation_id = changed.metadata.operation_id;
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed = reward;
	changed.metadata.reason = economic_reason::refund;
	changed.accounts[2].key.kind = economic_account_kind::sink;
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed.metadata.original_operation_id = id(9);
	roundtrip(
		changed); // Original receipt/entitlement authorization belongs to its typed adapter.
}

void maximum_plan()
{
	auto plan = base_plan();
	for (size_t index = 0; index < ECONOMIC_ACCOUNTING_MAX_CHILDREN; ++index)
	{
		economic_child_link child;
		child.domain = 476;
		child.discriminator = index;
		CHECK(critical_operation_id_derive(plan.metadata.operation_id, child.domain,
						   child.discriminator, &child.operation_id));
		plan.children.push_back(child);
	}
	for (size_t index = 0; index < ECONOMIC_ACCOUNTING_MAX_ACCOUNTS; ++index)
	{
		plan.accounts.push_back({ key(economic_account_kind::wallet, index + 1),
					  { 10, 0, 0, 0 },
					  { 10, 0, 0, 0 },
					  1,
					  2 });
		const auto account = static_cast<uint16_t>(index);
		const auto child = static_cast<uint16_t>(index % plan.children.size() + 1);
		plan.postings.push_back(
			{ static_cast<uint32_t>(index * 2), account, child, { 1, 0, 0, 0 }, 1 });
		plan.postings.push_back({ static_cast<uint32_t>(index * 2 + 1),
					  account,
					  child,
					  { -1, 0, 0, 0 },
					  -1 });
	}
	for (size_t index = 0; index < ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++index)
	{
		const uint64_t uid = index + 1;
		economic_item_position before = {
			{ item_owner_type::player, 1, 0 }, uid, 0, 1, item_custody_state::active
		};
		auto after = before;
		if (index < ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
		{
			after.owner.id = 2;
			after.revision = 2;
			plan.item_events.push_back(
				{ static_cast<uint32_t>(index),
				  static_cast<uint16_t>(index % plan.children.size() + 1), uid,
				  before, after });
		}
		plan.items_before.push_back({ uid, before });
		plan.items_after.push_back({ uid, after });
	}
	roundtrip(plan);
	std::vector<uint8_t> encoded;
	CHECK(economic_plan_encode(plan, &encoded) == error::ok);
	CHECK(encoded.size() == 256 + 3072 * 120 + 6144 * 48 + 64 * 32 + 12000 * 64 + 3000 * 128);
	auto over = plan;
	over.accounts.push_back(plan.accounts.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	over = plan;
	over.postings.push_back(plan.postings.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	over = plan;
	over.children.push_back(plan.children.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	over = plan;
	over.item_events.push_back(plan.item_events.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	over = plan;
	over.items_before.push_back(plan.items_before.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	over = plan;
	over.items_after.push_back(plan.items_after.front());
	CHECK(economic_plan_encode(over, &encoded) == error::capacity);
	// Valid effects in noncanonical account order cannot be accepted as wire bytes.
	auto wallet = REFERENCE_WALLET_PLAN;
	std::swap_ranges(wallet.begin() + 256, wallet.begin() + 376, wallet.begin() + 376);
	wallet[500] = 1;
	wallet[548] = 0;
	CHECK(economic_plan_decode(wallet, &over) != error::ok);
}

void command_binding()
{
	critical_command command = {};
	command.schema_version = 1;
	command.operation_id = id(3);
	command.type = critical_command_type::account_bank;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.keys = { { critical_entity_type::player, 7 },
			 { critical_entity_type::account, 8 } };
	command.expected_revisions = { { command.keys[0], 9 }, { command.keys[1], 10 } };
	command.payload = { 1, 2, 3 };
	economic_digest digest = {};
	CHECK(economic_command_binding_digest(command, &digest) == error::ok);
	CHECK(digest == REFERENCE_COMMAND_DIGEST);
	CHECK(command.accepted_at_usec == 0);
	command.accepted_at_usec = UINT64_MAX;
	std::reverse(command.keys.begin(), command.keys.end());
	std::reverse(command.expected_revisions.begin(), command.expected_revisions.end());
	CHECK(economic_command_binding_digest(command, &digest) == error::ok);
	CHECK(digest == REFERENCE_COMMAND_DIGEST && command.accepted_at_usec == UINT64_MAX);
	const auto baseline = command;
	auto differs = [&]
	{
		CHECK(economic_command_binding_digest(command, &digest) == error::ok);
		CHECK(digest != REFERENCE_COMMAND_DIGEST);
		command = baseline;
	};
	command.payload[0] ^= 1;
	differs();
	command.operation_id = id(4);
	differs();
	++command.expected_revisions[0].revision;
	differs();
	command.source_site = critical_source_site::recovery;
	differs();
	command.deadline_class = critical_deadline_class::recovery;
	differs();
	++command.payload_version;
	differs();
	command.type = critical_command_type::wallet;
	differs();
	command.keys[0].id = 80;
	command.expected_revisions[0].key.id = 80;
	differs();
	digest = REFERENCE_COMMAND_DIGEST;
	command.schema_version = 2;
	CHECK(economic_command_binding_digest(command, &digest) == error::invalid_version);
	CHECK(digest == REFERENCE_COMMAND_DIGEST);
	command = baseline;
	command.keys.push_back(command.keys[0]);
	CHECK(economic_command_binding_digest(command, &digest) == error::invalid_identity);
	CHECK(digest == REFERENCE_COMMAND_DIGEST);
	command = baseline;
	command.payload.resize(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES + 1);
	CHECK(economic_command_binding_digest(command, &digest) == error::capacity);
	CHECK(digest == REFERENCE_COMMAND_DIGEST);
}

int main()
{
	fixed_bytes_and_rejection();
	canonical_permutations();
	source_and_policy();
	maximum_plan();
	command_binding();
	golden_cases();
	std::cout
		<< "accounting plan: canonical bytes, source identities, permutations, malformed inputs and 13 goldens passed\n";
}
