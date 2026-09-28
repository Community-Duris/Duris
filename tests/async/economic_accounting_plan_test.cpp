#include "economy/economic_accounting_intent.h"

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

	auto expense = base_plan();
	expense.metadata.reason = economic_reason::service_cost;
	expense.metadata.source_event = source_event(economic_source_kind::service);
	expense.accounts = {
		{ key(economic_account_kind::wallet, 1), { 5, 0, 0, 0 }, { 4, 0, 0, 0 }, 4, 5 },
		{ key(economic_account_kind::sink, 1), {}, {}, 0, 0 }
	};
	expense.postings = { { 0, 0, 0, { -1, 0, 0, 0 }, -1 }, { 1, 1, 0, { 1, 0, 0, 0 }, 1 } };
	roundtrip(expense);
	changed = expense;
	changed.metadata.source_event.reset();
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed = expense;
	changed.metadata.source_event->kind = economic_source_kind::quest_completion;
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = expense;
	changed.postings[0].copper = 1;
	CHECK(economic_plan_validate_structure(changed) != error::ok);
	auto quest_cost = expense;
	quest_cost.metadata.reason = economic_reason::quest_cost;
	quest_cost.metadata.source_event->kind = economic_source_kind::quest_action;
	roundtrip(quest_cost);
	auto item_reward = reward;
	item_reward.metadata.reason = economic_reason::item_reward;
	item_reward.metadata.source_event->kind = economic_source_kind::item_action;
	item_reward.accounts.erase(item_reward.accounts.begin() + 1);
	item_reward.postings[1].account_index = 1;
	roundtrip(item_reward);

	const std::pair<economic_reason, economic_source_kind> sourced[] = {
		{ economic_reason::quest_reward, economic_source_kind::quest_completion },
		{ economic_reason::npc_reward, economic_source_kind::npc_generation },
		{ economic_reason::chaos_reward, economic_source_kind::world_generation },
		{ economic_reason::starter_reward, economic_source_kind::starter_grant },
		{ economic_reason::boon_reward, economic_source_kind::boon },
		{ economic_reason::achievement_reward, economic_source_kind::achievement },
		{ economic_reason::service_cost, economic_source_kind::service },
		{ economic_reason::training_cost, economic_source_kind::service },
		{ economic_reason::locker_cost, economic_source_kind::service },
		{ economic_reason::shipping_cost, economic_source_kind::service },
		{ economic_reason::insurance_cost, economic_source_kind::service },
		{ economic_reason::guild_cost, economic_source_kind::service },
		{ economic_reason::crafting_cost, economic_source_kind::crafting },
		{ economic_reason::gambling_stake, economic_source_kind::gambling_round },
		{ economic_reason::gambling_payout, economic_source_kind::gambling_round },
		{ economic_reason::gambling_loss, economic_source_kind::gambling_round },
		{ economic_reason::gambling_interruption, economic_source_kind::gambling_round },
		{ economic_reason::shop_buy, economic_source_kind::shop_stock },
		{ economic_reason::shop_sell, economic_source_kind::shop_stock },
		{ economic_reason::auction_listing, economic_source_kind::auction },
		{ economic_reason::auction_bid, economic_source_kind::auction },
		{ economic_reason::auction_outbid, economic_source_kind::auction },
		{ economic_reason::auction_cancel, economic_source_kind::auction },
		{ economic_reason::auction_settle, economic_source_kind::auction },
		{ economic_reason::auction_claim, economic_source_kind::auction },
		{ economic_reason::death_transfer, economic_source_kind::corpse },
		{ economic_reason::item_reward, economic_source_kind::item_action },
		{ economic_reason::quest_cost, economic_source_kind::quest_action },
	};
	for (const auto &[reason, kind] : sourced)
	{
		auto meta = base_plan().metadata;
		meta.reason = reason;
		CHECK(economic_operation_metadata_validate(meta) == error::invalid_identity);
		meta.source_event = source_event(kind);
		if (reason == economic_reason::gambling_payout ||
		    reason == economic_reason::gambling_loss ||
		    reason == economic_reason::gambling_interruption)
			meta.original_operation_id = id(9);
		CHECK(economic_operation_metadata_validate(meta) == error::ok);
		meta.source_event->kind = economic_source_kind::administrator;
		CHECK(economic_operation_metadata_validate(meta) == error::unauthorized);
	}
	auto collector = base_plan().metadata;
	collector.reason = economic_reason::collector_purchase;
	CHECK(economic_operation_metadata_validate(collector) == error::ok);
	collector.source_event = source_event(economic_source_kind::service);
	CHECK(economic_operation_metadata_validate(collector) == error::ok);
	collector.source_event->kind = economic_source_kind::administrator;
	CHECK(economic_operation_metadata_validate(collector) == error::unauthorized);
	auto restitution = base_plan().metadata;
	restitution.reason = economic_reason::restitution;
	restitution.actor_kind = economic_actor_kind::operator_action;
	restitution.source_event = source_event(economic_source_kind::correction);
	CHECK(economic_operation_metadata_validate(restitution) == error::invalid_identity);
	restitution.original_operation_id = id(9);
	CHECK(economic_operation_metadata_validate(restitution) == error::ok);
}

void gambling_rounds()
{
	auto stake = base_plan();
	stake.metadata.reason = economic_reason::gambling_stake;
	stake.metadata.source_event = { economic_source_kind::gambling_round, id(31), id(32), 7,
					0 };
	stake.accounts = {
		{ key(economic_account_kind::wallet, 1), { 0, 10, 0, 0 }, { 0, 5, 0, 0 }, 4, 5 },
		{ key(economic_account_kind::gambling_stake, 700, 7), {}, { 0, 5, 0, 0 }, 0, 1 }
	};
	stake.postings = { { 0, 0, 0, { 0, -5, 0, 0 }, -50 }, { 1, 1, 0, { 0, 5, 0, 0 }, 50 } };
	roundtrip(stake);
	auto changed = stake;
	changed.metadata.source_event->slot = 1;
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = stake;
	changed.accounts[1].key.context_id = 8;
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed = stake;
	changed.accounts[0].after = { 10, 4, 0, 0 };
	changed.postings[0] = { 0, 0, 0, { 10, -6, 0, 0 }, -50 };
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = stake;
	changed.accounts[1].key.kind = economic_account_kind::treasury;
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);
	changed = stake;
	changed.accounts[0].after = { 0, 4, 0, 0 };
	changed.accounts.push_back(
		{ key(economic_account_kind::gambling_stake, 701, 7), {}, { 0, 1, 0, 0 }, 0, 1 });
	changed.postings = { { 0, 0, 0, { 0, -6, 0, 0 }, -60 },
			     { 1, 1, 0, { 0, 5, 0, 0 }, 50 },
			     { 2, 2, 0, { 0, 1, 0, 0 }, 10 } };
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);

	auto push = base_plan();
	push.metadata.operation_id = id(6);
	push.metadata.original_operation_id = stake.metadata.operation_id;
	push.metadata.reason = economic_reason::gambling_payout;
	push.metadata.source_event = *stake.metadata.source_event;
	push.metadata.source_event->slot = 1;
	push.accounts = {
		{ key(economic_account_kind::wallet, 1), { 0, 5, 0, 0 }, { 0, 10, 0, 0 }, 5, 6 },
		{ key(economic_account_kind::gambling_stake, 700, 7), { 0, 5, 0, 0 }, {}, 1, 2 }
	};
	push.postings = { { 0, 0, 0, { 0, 5, 0, 0 }, 50 }, { 1, 1, 0, { 0, -5, 0, 0 }, -50 } };
	roundtrip(push);
	changed = push;
	changed.metadata.original_operation_id = {};
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);
	changed = push;
	changed.accounts[1] = { key(economic_account_kind::issuance, 1), {}, {}, 0, 0 };
	changed.postings[1] = { 1, 1, 0, { 0, -5, 0, 0 }, -50 };
	CHECK(economic_plan_validate_structure(changed) == error::invalid_identity);

	auto win = push;
	win.accounts[0].after = { 0, 15, 0, 0 };
	win.accounts.insert(win.accounts.begin() + 1,
			    { key(economic_account_kind::issuance, 1), {}, {}, 0, 0 });
	win.postings = { { 0, 0, 0, { 0, 10, 0, 0 }, 100 },
			 { 1, 1, 0, { 0, -5, 0, 0 }, -50 },
			 { 2, 2, 0, { 0, -5, 0, 0 }, -50 } };
	roundtrip(win);
	changed = win;
	changed.accounts[0].after = { 0, 20, 0, 0 };
	changed.postings[0] = { 0, 0, 0, { 0, 15, 0, 0 }, 150 };
	changed.postings[1] = { 1, 1, 0, { 0, -10, 0, 0 }, -100 };
	CHECK(economic_plan_validate_structure(changed) == error::unauthorized);

	auto loss = push;
	loss.metadata.reason = economic_reason::gambling_loss;
	loss.accounts = { { key(economic_account_kind::sink, 1), {}, {}, 0, 0 }, push.accounts[1] };
	loss.postings = { { 0, 0, 0, { 0, 5, 0, 0 }, 50 }, { 1, 1, 0, { 0, -5, 0, 0 }, -50 } };
	roundtrip(loss);
	auto interrupted = push;
	interrupted.metadata.reason = economic_reason::gambling_interruption;
	roundtrip(interrupted);
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

critical_command binding_command()
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
	return command;
}

void command_binding()
{
	auto command = binding_command();
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
	command.schema_version = 3;
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

void frozen_intent()
{
	auto command = binding_command();
	economic_admission_facts facts;
	facts.metadata = base_plan().metadata;
	facts.metadata.operation_id = {}; // freeze binds the command's existing ID
	facts.facts = { 4, 5, 6 };
	std::vector<uint8_t> bytes;
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::ok);
	CHECK(bytes == REFERENCE_INTENT);
	CHECK(critical_operation_id_is_zero(facts.metadata.operation_id));
	economic_frozen_intent intent;
	CHECK(economic_intent_decode(bytes, &intent) == error::ok);
	CHECK(economic_intent_verify_binding(command, intent) == error::ok);
	economic_digest digest = {};
	CHECK(economic_intent_digest(intent, &digest) == error::ok);
	CHECK(digest == REFERENCE_INTENT_DIGEST);
	economic_plan_metadata metadata;
	CHECK(economic_intent_plan_metadata(command, intent, &metadata) == error::ok);
	CHECK(metadata.intent_digest == digest && metadata.domain_digest == intent.domain_digest);
	CHECK(metadata.operation_id.bytes == command.operation_id.bytes && metadata.writer_id == 1);
	command.accepted_at_usec = 123456;
	CHECK(economic_intent_verify_binding(command, intent) == error::ok);
	command.payload[0] = 99;
	metadata.writer_id = 99;
	CHECK(economic_intent_plan_metadata(command, intent, &metadata) == error::payload_conflict);
	CHECK(metadata.writer_id == 99);
	command = binding_command();
	auto changed = intent;
	changed.domain_digest[0] ^= 1;
	CHECK(economic_intent_verify_binding(command, changed) == error::payload_conflict);
	changed = intent;
	changed.command_binding[0] ^= 1;
	CHECK(economic_intent_verify_binding(command, changed) == error::payload_conflict);
	changed = intent;
	changed.admission.facts[0] ^= 1;
	CHECK(economic_intent_digest(changed, &digest) == error::ok &&
	      digest != REFERENCE_INTENT_DIGEST);
	// Facts need the typed writer's semantic authorization; digesting them alone
	// intentionally cannot establish a reward or refund entitlement.
	economic_frozen_intent output = intent;
	output.admission.metadata.writer_id = 99;
	for (size_t size = 0; size < bytes.size(); ++size)
	{
		CHECK(economic_intent_decode(std::span(bytes).first(size), &output) != error::ok);
		CHECK(output.admission.metadata.writer_id == 99);
	}
	for (size_t offset : { 30U, 31U, 108U, 111U, 224U, 255U, 112U, 159U })
	{
		auto bad = bytes;
		bad[offset] = 1;
		CHECK(economic_intent_decode(bad, &output) == error::corrupt_evidence);
	}
	for (size_t offset : { 4U, 16U, 20U, 28U })
	{
		auto bad = bytes;
		bad[offset] = 2;
		CHECK(economic_intent_decode(bad, &output) == error::invalid_version);
	}
	auto bad = bytes;
	bad[27] = 2;
	CHECK(economic_intent_decode(bad, &output) == error::corrupt_evidence);
	bad = bytes;
	bad[104] = 255;
	bad[105] = 255;
	CHECK(economic_intent_decode(bad, &output) == error::corrupt_evidence);
	bad = bytes;
	bad.push_back(0);
	CHECK(economic_intent_decode(bad, &output) == error::corrupt_evidence);
	facts.facts.resize(ECONOMIC_INTENT_MAX_FACT_BYTES, 7);
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::ok);
	CHECK(bytes.size() == 8192 && economic_intent_decode(bytes, &output) == error::ok);
	std::vector<uint8_t> again;
	CHECK(economic_intent_encode(output, &again) == error::ok && again == bytes);
	facts.facts.push_back(8);
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::capacity);
	CHECK(bytes == again);
	facts.facts.clear();
	facts.metadata.operation_id = id(9);
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::payload_conflict);
	facts.metadata.operation_id = {};
	facts.metadata.reason = economic_reason::quest_reward;
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::invalid_identity);
	facts.metadata.source_event = source_event();
	CHECK(economic_intent_freeze(command, facts, &bytes) == error::ok);
	CHECK(economic_intent_decode(bytes, &output) == error::ok);
	CHECK(output.admission.metadata.source_event->generation.bytes == id(5).bytes);
	CHECK(economic_intent_verify_binding(command, output) == error::ok);
}

void versioned_envelopes()
{
	auto command = binding_command();
	command.accepted_at_usec = 1;
	std::vector<uint8_t> legacy;
	CHECK(critical_command_encode(command, &legacy) == critical_command_codec_result::ok);
	CHECK(legacy == REFERENCE_LEGACY_COMMAND);
	economic_admission_facts facts;
	facts.metadata = base_plan().metadata;
	facts.facts = { 4, 5, 6 };
	CHECK(economic_intent_freeze(command, facts, &command.accounting_intent) == error::ok);
	CHECK(!critical_command_envelope_valid(command)); // schema 1 cannot hide an extension
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	CHECK(critical_command_envelope_valid(command) && !critical_command_valid(command));
	CHECK(!critical_command_legacy_execution_supported(command));
	std::vector<uint8_t> encoded;
	CHECK(critical_command_encode(command, &encoded) == critical_command_codec_result::ok);
	CHECK(encoded == REFERENCE_ACCOUNTING_COMMAND);
	CHECK(legacy[31] == 0 && encoded[31] == 0); // Existing wire bytes do not move.
	critical_command decoded;
	CHECK(critical_command_decode(encoded.data(), encoded.size(), &decoded) ==
	      critical_command_codec_result::ok);
	CHECK(critical_command_equal(command, decoded));
	economic_frozen_intent intent;
	CHECK(economic_intent_decode(decoded.accounting_intent, &intent) == error::ok);
	CHECK(economic_intent_verify_binding(decoded, intent) == error::ok);
	auto marked = command;
	marked.publication_required = true;
	std::vector<uint8_t> marked_bytes;
	CHECK(critical_command_encode(marked, &marked_bytes) == critical_command_codec_result::ok);
	CHECK(marked_bytes.size() == encoded.size() && marked_bytes[31] == 1);
	critical_command marked_decoded;
	CHECK(critical_command_decode(marked_bytes.data(), marked_bytes.size(), &marked_decoded) ==
	      critical_command_codec_result::ok);
	CHECK(marked_decoded.publication_required &&
	      critical_command_equal(marked, marked_decoded) &&
	      !critical_command_equal(command, marked_decoded));
	CHECK(economic_intent_verify_binding(marked_decoded, intent) == error::ok);
	auto malformed_flag = marked_bytes;
	malformed_flag[31] = 2;
	CHECK(critical_command_decode(malformed_flag.data(), malformed_flag.size(), &decoded) ==
	      critical_command_codec_result::invalid);
	malformed_flag = legacy;
	malformed_flag[31] = 1;
	CHECK(critical_command_decode(malformed_flag.data(), malformed_flag.size(), &decoded) ==
	      critical_command_codec_result::invalid);
	marked.type = critical_command_type::coin_transfer;
	CHECK(critical_command_envelope_valid(marked) && !critical_command_valid(marked));
	CHECK(economic_intent_verify_binding(marked, intent) == error::payload_conflict);
	marked.type = critical_command_type::item_transfer;
	CHECK(critical_command_envelope_valid(marked) && !critical_command_valid(marked));
	CHECK(economic_intent_verify_binding(marked, intent) == error::payload_conflict);
	auto changed = decoded;
	changed.accounting_intent.back() ^= 1;
	CHECK(!critical_command_equal(changed, decoded));
	CHECK(economic_intent_verify_binding(changed, intent) == error::payload_conflict);
	for (size_t size = 0; size < encoded.size(); ++size)
		CHECK(critical_command_decode(encoded.data(), size, &decoded) !=
		      critical_command_codec_result::ok);
	auto bad = encoded;
	bad[legacy.size()] = 0;
	bad[legacy.size() + 1] = 0;
	CHECK(critical_command_decode(bad.data(), bad.size(), &decoded) ==
	      critical_command_codec_result::overflow);
	bad = encoded;
	bad[legacy.size()] = 255;
	bad[legacy.size() + 1] = 255;
	CHECK(critical_command_decode(bad.data(), bad.size(), &decoded) ==
	      critical_command_codec_result::overflow);
	bad = encoded;
	bad.push_back(0);
	CHECK(critical_command_decode(bad.data(), bad.size(), &decoded) ==
	      critical_command_codec_result::invalid);
	bad = encoded;
	bad[4] = 3;
	CHECK(critical_command_decode(bad.data(), bad.size(), &decoded) ==
	      critical_command_codec_result::unsupported_version);
	command = binding_command();
	command.accepted_at_usec = 1;
	command.keys.clear();
	command.expected_revisions.clear();
	for (size_t index = 0; index < CRITICAL_COMMAND_MAX_KEYS; ++index)
	{
		critical_entity_key key = { critical_entity_type::player, index + 1 };
		command.keys.push_back(key);
		command.expected_revisions.push_back({ key, index });
	}
	command.payload.resize(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES, 17);
	facts.facts.resize(ECONOMIC_INTENT_MAX_FACT_BYTES, 7);
	CHECK(economic_intent_freeze(command, facts, &command.accounting_intent) == error::ok);
	command.schema_version = 2;
	CHECK(critical_command_encode(command, &encoded) == critical_command_codec_result::ok);
	CHECK(encoded.size() == 521584);
	CHECK(critical_command_decode(encoded.data(), encoded.size(), &decoded) ==
	      critical_command_codec_result::ok);
	CHECK(critical_command_equal(command, decoded));
	CHECK(economic_intent_decode(decoded.accounting_intent, &intent) == error::ok);
	CHECK(economic_intent_verify_binding(decoded, intent) == error::ok);
	command.accounting_intent.push_back(0);
	CHECK(critical_command_encode(command, &encoded) == critical_command_codec_result::invalid);
}

int main()
{
	fixed_bytes_and_rejection();
	canonical_permutations();
	source_and_policy();
	gambling_rounds();
	maximum_plan();
	command_binding();
	frozen_intent();
	versioned_envelopes();
	golden_cases();
	std::cout
		<< "accounting plan and intent: reference bytes/digests, limits, bindings, malformed inputs and 14 goldens passed\n";
}
