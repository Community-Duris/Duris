// Modeled values through actual providers. No SQL, world, owner or journal doubles.
#include "core/config.h"
#include "core/structs.h"
#include "economy/item_transfer_accounting.h"
#include "item/quest_reward_continuation.h"
#include "world/native_quest_recovery_context.h"
#include "world/quest_mobile_native.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>

namespace
{
using result = player_snapshot_codec_result;
using cost_result = native_quest_cost_projection_result;
using context = native_quest_recovery_context;
using stage = native_quest_recovery_publication_stage;
size_t controls = 0;

void passed(const std::string &name)
{
	++controls;
	std::printf("PASS %s\n", name.c_str());
	std::fflush(stdout);
}

critical_operation_id id(uint8_t value)
{
	critical_operation_id out{};
	out.bytes[0] = value;
	return out;
}

player_item_snapshot item(uint64_t uid, int vnum)
{
	player_item_snapshot out{};
	out.object_uid = uid;
	out.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	out.vnum = vnum;
	out.string_mask = 15;
	out.name = "modeled item";
	out.short_description = "a modeled item";
	out.description = "A modeled item is here.";
	return out;
}

std::vector<uint8_t> forest(std::span<const player_item_snapshot> items)
{
	std::vector<uint8_t> out;
	const std::vector<player_item_snapshot> values(items.begin(), items.end());
	assert(player_item_snapshot_list_encode(values, &out) == result::ok);
	assert(!out.empty()); // An encoded empty forest is not a zero-byte image.
	return out;
}

std::vector<uint8_t> frame(const critical_command &command)
{
	std::vector<uint8_t> out;
	assert(critical_command_encode(command, &out) == critical_command_codec_result::ok);
	return out;
}

std::vector<uint8_t> attachment(const critical_command &command, const context &value)
{
	std::vector<uint8_t> out;
	assert(native_quest_recovery_context_encode(command, value, &out) == result::ok);
	context decoded;
	assert(native_quest_recovery_context_decode(command, out, &decoded) == result::ok);
	std::vector<uint8_t> again;
	assert(native_quest_recovery_context_encode(command, decoded, &again) == result::ok);
	assert(out == again);
	return out;
}

struct original
{
	item_transfer_payload payload{};
	critical_command command{};
	context retained;
};

struct fixture
{
	original parent, child;
	quest_reward_continuation terms;
	item_transfer_result accepted{};
	item_native_mobile_fee_result fee_result{};
	quest_mobile_native_image before;
};

critical_command build(const item_transfer_payload &payload, critical_operation_id operation,
		       const economic_source_event *source)
{
	critical_command out;
	assert(item_transfer_command_build_native_mobile_recovery(
		&out, operation, payload, critical_source_site::command,
		critical_deadline_class::interactive));
	assert(item_native_mobile_accounting_intent(out, id(7), id(8), 10, source,
						    &out.accounting_intent) ==
	       economic_accounting_error::ok);
	out.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	out.publication_required = true;
	out.accepted_at_usec = 1700000000000000ULL;
	assert(critical_command_envelope_valid(out));
	return out;
}

critical_native_recovery_envelope carrier(const original &value)
{
	critical_native_recovery_envelope out;
	out.command = value.command;
	out.revision = 11;
	out.phase = critical_native_recovery_phase::continuation_pending;
	out.attachment = attachment(value.command, value.retained);
	return out;
}

void fee_receipt(fixture &value, bool rejected = false)
{
	auto expected = value.fee_result;
	if (rejected)
	{
		expected.mobile_cash_revision =
			value.child.payload.native_cost.projection.before_revision;
		expected.mobile_revision =
			value.child.payload.native_mobile.reference.mobile_revision;
	}
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> bytes{};
	assert(item_native_mobile_fee_result_encode(expected, &bytes));
	auto &receipt = value.child.retained.receipt;
	receipt = {};
	receipt.present = true;
	receipt.outcome = rejected ? critical_apply_outcome::terminal_failure :
				     critical_apply_outcome::applied;
	receipt.error_code = rejected ? ESTALE : 0;
	receipt.durable_revision = std::max(
		{ expected.mobile_cash_revision, expected.mobile_revision, expected.stock_revision,
		  expected.native_custody_revision, expected.player_custody_revision });
	receipt.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
	value.child.retained.publication_stage = stage::physically_proven;
	value.child.retained.publication_steps[0] = rejected ? 0 : 2;
}

std::unique_ptr<fixture> modeled(bool unrelated = true, uint64_t mobile_sequence = 6)
{
	auto f = std::make_unique<fixture>();
	auto &parent = f->parent;
	auto &p = parent.payload;
	p.from_owner = { item_owner_type::player, 10, 0 };
	p.to_owner = { item_owner_type::native_mobile, 9000, 0 };
	p.reason = item_transfer_reason::quest_offering;
	p.reason_id = 70023;
	p.expected_from_revision = 3;
	p.expected_to_revision = 7;
	p.selected_item_uid = p.target_root_item_uid = 100;
	p.item_count = 1;
	p.items[0] = { 100, 100, 0, 2, 70020, item_custody_state::active };
	const auto offered = forest(std::vector<player_item_snapshot>{ item(100, 70020) });
	p.item_blob_size = offered.size();
	std::copy(offered.begin(), offered.end(), p.item_blob.begin());
	p.native_mobile.present = true;
	p.native_mobile.action = item_native_mobile_action::acceptance;
	p.native_mobile.final_giver_pid = 10;
	auto &r = p.native_mobile.reference;
	r.mobile_instance_id = 9000;
	r.birth_operation = id(1);
	r.birth_source = { economic_source_kind::npc_generation, id(2), id(3), 4, 5 };
	r.mobile_vnum = 70023;
	r.reset_zone_vnum = 700;
	r.birthplace_vnum = 70029;
	r.provenance = quest_mobile_birth_provenance::reset;
	r.mobile_revision = 5;
	r.stock_revision = 7;
	parent.retained.player_before = { item(100, 70020) };
	if (unrelated)
	{
		parent.retained.player_before.push_back(item(900, 70021));
		parent.retained.native_before.push_back(item(500, 70022));
	}
	assert(item_transfer_native_mobile_recovery_freeze(&p, 10, 23,
							   forest(parent.retained.player_before)));
	parent.command = build(p, id(6), nullptr);
	f->accepted = { 100, 1, 4, 8, 3, 0, false };
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> typed48{};
	assert(item_transfer_command_encode_result(f->accepted, &typed48));
	parent.retained.receipt.present = true;
	parent.retained.receipt.outcome = critical_apply_outcome::applied;
	parent.retained.receipt.durable_revision = 8;
	parent.retained.receipt.result_size = typed48.size();
	std::copy(typed48.begin(), typed48.end(), parent.retained.receipt.result_payload.begin());
	parent.retained.publication_stage = stage::physically_proven;
	parent.retained.publication_steps.fill(2);
	parent.retained.branch_program_frozen = true;
	parent.retained.give_hooks.fill(2);
	native_quest_recovery_branch branch;
	branch.definition_id = "modeled:fee-owner-boundary";
	branch.give = { { 3, 25 } };
	branch.receive = { { 1, 70018 }, { 1, 70018 }, { 3, 17 }, { 5, 100 }, { 4, 2 } };
	parent.retained.branches = { branch };

	auto &child = f->child;
	auto &q = child.payload;
	q.from_owner = { item_owner_type::native_mobile, 9000, 0 };
	q.to_owner = { item_owner_type::player, 10, 0 };
	q.reason = item_transfer_reason::quest_turnin;
	q.reason_id = 70023;
	q.expected_from_revision = 8;
	q.expected_to_revision = 4;
	q.native_mobile.present = true;
	q.native_mobile.action = item_native_mobile_action::consumption;
	q.native_mobile.final_giver_pid = 10;
	q.native_mobile.reference = r;
	q.native_mobile.reference.mobile_revision = mobile_sequence;
	q.native_mobile.reference.stock_revision = 8;
	q.native_cost.present = q.native_cost.fee_only = true;
	q.native_cost.wallet_mapping_id = 77;
	const std::array<native_quest_cost_requirement, 1> cost{ { { 0, 25 } } };
	assert(native_quest_cost_project({ 0, 0, 1, 0 }, 17, cost, &q.native_cost.projection) ==
	       cost_result::ok);
	auto &terms = f->terms;
	terms.version = 6;
	terms.player_pid = 10;
	terms.mobile_vnum = 70023;
	terms.room_vnum = 70029;
	terms.completed_at = 1700000000;
	terms.reward_count = 5;
	terms.rewards[0] = { 1, 70018, 0, 0 };
	terms.rewards[1] = { 1, 70018, 0, 0 };
	terms.rewards[2] = { 3, 17, 0, 0 };
	terms.rewards[3] = { 5, 100, 0, 70 };
	terms.rewards[4] = { 4, 2, QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION, 0 };
	terms.zone_number = 700;
	terms.player_level = terms.strongest_party_level = 20;
	terms.party_size = terms.credited_count = 2;
	terms.credited_pids[0] = 10;
	terms.credited_pids[1] = 11;
	terms.xp_award_count = 2;
	terms.xp_awards[0] = { 10, 3, 70 };
	terms.xp_awards[1] = { 11, 3, 30 };
	terms.character_name = "Modeled";
	terms.definition_id = branch.definition_id;
	terms.action_operation = id(16);
	terms.triggering_acceptance = parent.command.operation_id;
	terms.action_mobile_instance_id = 9000;
	terms.action_source = { economic_source_kind::quest_action, id(16),
				r.birth_source.generation, mobile_sequence, 0 };
	terms.original_acceptance_result = typed48;
	assert(quest_fee_reward_trigger_binding_valid(terms, parent.command));
	q.continuation.kind = item_transfer_continuation_kind::quest_offering;
	assert(quest_fee_reward_continuation_encode(terms, &q.continuation.data));
	if (unrelated)
		child.retained.player_before = { item(900, 70021) };
	assert(quest_mobile_native_items_transition(parent.retained.native_before, r, p,
						    &child.retained.native_before) == result::ok);
	assert(item_transfer_native_mobile_recovery_freeze(&q, 10, 23,
							   forest(child.retained.player_before)));
	child.command = build(q, terms.action_operation, &terms.action_source);
	assert(child.command.payload_version == 14);
	assert(item_native_mobile_fee_result_build(q, &f->fee_result));
	fee_receipt(*f);
	parent.retained.next_child_operation = child.command.operation_id;
	parent.retained.parent_acceptance = parent.command.operation_id;
	parent.retained.next_child_command = frame(child.command);
	parent.retained.child_handoff_stage = 2;
	f->before.reference = q.native_mobile.reference;
	f->before.state = quest_mobile_lifetime_state::live;
	f->before.last_transition_operation = parent.command.operation_id;
	f->before.items = child.retained.native_before;
	f->before.cash = quest_mobile_native_cash{ 17, {} };
	f->before.cash->denominations.amount = q.native_cost.projection.before;
	return f;
}

void reject_payload(const item_transfer_payload &bad, const std::string &name)
{
	std::vector<uint8_t> out{ 0xa5, 0x5a };
	assert(!item_transfer_command_encode_native_mobile_recovery(bad, &out));
	assert((out == std::vector<uint8_t>{ 0xa5, 0x5a }));
	passed(name);
}

void reject_context(const original &base, const context &bad, const std::string &name)
{
	std::vector<uint8_t> out{ 0xa5, 0x5a };
	assert(native_quest_recovery_context_encode(base.command, bad, &out) != result::ok);
	assert((out == std::vector<uint8_t>{ 0xa5, 0x5a }));
	passed(name);
}

void valid_values()
{
	for (bool unrelated : { false, true })
		for (uint64_t sequence : { 6ULL, 9ULL })
		{
			auto f = modeled(unrelated, sequence);
			assert(native_quest_recovery_pair_context_valid(
				carrier(f->parent), carrier(f->child), nullptr));
			const auto child = carrier(f->child);
			assert(std::equal(child.attachment.begin(), child.attachment.begin() + 4,
					  std::array<uint8_t, 4>{ 'N', 'Q', 'R', '5' }.begin()));
			assert(std::equal(child.command.payload.begin(),
					  child.command.payload.begin() + 4,
					  std::array<uint8_t, 4>{ 'N', 'Q', 'F', '2' }.begin()));
			item_transfer_payload decoded{};
			assert(item_transfer_command_decode_payload(f->child.command, &decoded));
			assert(decoded.native_cost.fee_only && !decoded.item_count &&
			       !decoded.item_blob_size && !decoded.multi_root &&
			       !decoded.selected_item_uid &&
			       decoded.native_recovery.consumed_root_order.empty());
			std::vector<uint8_t> bytes;
			assert(item_transfer_command_encode_native_mobile_recovery(decoded,
										   &bytes));
			assert(bytes == f->child.command.payload);
			assert(decoded.native_cost.projection.after ==
			       (std::array<int64_t, 4>{ 5, 7, 0, 0 }));
			quest_mobile_native_image after;
			assert(quest_mobile_native_fee_transition(f->before, decoded,
								  f->child.command.operation_id,
								  &after) == result::ok);
			assert(after.reference.mobile_revision == sequence + 1 &&
			       after.reference.stock_revision == 8 && after.cash->revision == 18 &&
			       after.cash->denominations.amount ==
				       decoded.native_cost.projection.after &&
			       after.last_transition_operation.bytes ==
				       f->child.command.operation_id.bytes &&
			       forest(after.items) == forest(f->before.items));
			assert(decoded.expected_from_revision == 8 &&
			       decoded.expected_to_revision == 4 &&
			       decoded.native_recovery.player_before.canonical_bytes ==
				       decoded.native_recovery.player_after.canonical_bytes);
			if (!unrelated)
				assert(f->parent.retained.native_before.empty() &&
				       f->child.retained.player_before.empty());
			passed(std::string("modeled-pair-transition-empty-") +
			       (unrelated ? "no" : "yes") + "-sequence-" +
			       std::to_string(sequence));
		}
	// Later original branch values may have no remaining native stock. No history is proved.
	auto empty = modeled(false, 9);
	empty->child.retained.native_before.clear();
	++empty->child.payload.native_mobile.reference.stock_revision;
	++empty->child.payload.expected_from_revision;
	empty->child.command = build(empty->child.payload, empty->terms.action_operation,
				     &empty->terms.action_source);
	assert(item_native_mobile_fee_result_build(empty->child.payload, &empty->fee_result));
	fee_receipt(*empty);
	empty->parent.retained.next_child_command = frame(empty->child.command);
	empty->before.items.clear();
	empty->before.reference = empty->child.payload.native_mobile.reference;
	assert(native_quest_recovery_pair_context_valid(carrier(empty->parent),
							carrier(empty->child), nullptr));
	quest_mobile_native_image after_empty;
	assert(quest_mobile_native_fee_transition(empty->before, empty->child.payload,
						  empty->child.command.operation_id,
						  &after_empty) == result::ok);
	assert(after_empty.items.empty() && forest(after_empty.items) == forest({}) &&
	       empty->child.retained.player_before.empty() &&
	       after_empty.reference.stock_revision == 9);
	passed("fee-native-and-player-encoded-empty-forests");
	// All-byte coherent values still pass: this deliberately establishes NO retained owner.
	passed("coherent-modeled-pairs-remain-externally-unauthenticated");
}

void payload_controls()
{
	auto f = modeled();
	for (int kind = 0; kind < 15; ++kind)
	{
		auto bad = f->child.payload;
		switch (kind)
		{
		case 0:
			bad.item_count = 1;
			break;
		case 1:
			bad.item_blob_size = 1;
			break;
		case 2:
			bad.selected_item_uid = 100;
			break;
		case 3:
			bad.target_root_item_uid = 100;
			break;
		case 4:
			bad.multi_root = true;
			break;
		case 5:
			bad.native_recovery.consumed_root_order = { 100 };
			break;
		case 6:
			bad.native_recovery.publication_terms.disappear = true;
			break;
		case 7:
			++bad.native_cost.completion_slot;
			break;
		case 8:
			++bad.native_mobile.final_giver_pid;
			break;
		case 9:
			++bad.native_mobile.reference.mobile_instance_id;
			break;
		case 10:
			++bad.native_mobile.reference.mobile_vnum;
			break;
		case 11:
			bad.native_mobile.reference.birth_source.generation = id(99);
			break;
		case 12:
			++bad.native_mobile.reference.mobile_revision;
			break;
		case 13:
			bad.continuation.data[0] = 5;
			break;
		case 14:
			bad.native_mobile.action = item_native_mobile_action::acceptance;
			break;
		}
		reject_payload(bad, "payload-shape-" + std::to_string(kind));
	}
	for (uint16_t version : { uint16_t{ 12 }, uint16_t{ 13 }, uint16_t{ 16 } })
	{
		auto bad = f->child.command;
		bad.payload_version = version;
		auto out = f->child.payload;
		assert(!item_transfer_command_decode_payload(bad, &out));
		std::vector<uint8_t> sentinel;
		assert(item_transfer_command_encode_native_mobile_recovery(out, &sentinel) &&
		       sentinel == f->child.command.payload);
		passed("payload-version-" + std::to_string(version));
	}
	for (int kind = 0; kind < 7; ++kind)
	{
		auto bad = f->child.retained;
		switch (kind)
		{
		case 0:
			bad.parent_acceptance = f->parent.command.operation_id;
			break;
		case 1:
			bad.consumed_root_steps = { 2 };
			break;
		case 2:
			bad.give_hooks[0] = 2;
			break;
		case 3:
			bad.give_messages[0] = 2;
			break;
		case 4:
			bad.publication_steps[1] = 2;
			break;
		case 5:
			bad.publication_stage = stage::captured;
			break;
		case 6:
			bad.publication_steps[0] = 1;
			break;
		}
		reject_context(f->child, bad, "context-shape-" + std::to_string(kind));
	}
	// Started-but-unreturned is a legal publishing VALUE; no replay permission follows.
	auto ambiguous = f->child.retained;
	ambiguous.publication_stage = stage::publishing;
	ambiguous.publication_steps[0] = 1;
	attachment(f->child.command, ambiguous);
	auto ambiguous_carrier = carrier(f->child);
	ambiguous_carrier.attachment = attachment(f->child.command, ambiguous);
	assert(!native_quest_recovery_pair_context_valid(carrier(f->parent), ambiguous_carrier,
							 nullptr));
	passed("ambiguous-publishing-value-valid-but-terminal-pair-refused");
	auto captured = f->child.retained;
	captured.receipt = {};
	captured.publication_stage = stage::captured;
	captured.publication_steps.fill(0);
	attachment(f->child.command, captured);
	auto pending = carrier(f->child);
	pending.attachment = attachment(f->child.command, captured);
	assert(!native_quest_recovery_pair_context_valid(carrier(f->parent), pending, nullptr));
	passed("captured-without-receipt-valid-but-terminal-pair-refused");
}

void trigger_controls()
{
	auto f = modeled();
	for (int kind = 0; kind < 13; ++kind)
	{
		auto terms = f->terms;
		switch (kind)
		{
		case 0:
			terms.version = 5;
			break;
		case 1:
			terms.root_count = 1;
			break;
		case 2:
			terms.triggering_acceptance = id(99);
			break;
		case 3:
			++terms.player_pid;
			break;
		case 4:
			++terms.action_mobile_instance_id;
			break;
		case 5:
			++terms.mobile_vnum;
			break;
		case 6:
			terms.action_source.generation = id(99);
			break;
		case 7:
			terms.action_source.sequence = 5;
			break;
		default:
		{
			auto receipt = f->accepted;
			if (kind == 8)
				++receipt.root_item_uid;
			if (kind == 9)
				++receipt.item_count;
			if (kind == 10)
				++receipt.from_owner_revision;
			if (kind == 11)
				++receipt.to_owner_revision;
			if (kind == 12)
				++receipt.max_item_revision;
			assert(item_transfer_command_encode_result(
				receipt, &terms.original_acceptance_result));
		}
		}
		assert(!quest_fee_reward_trigger_binding_valid(terms, f->parent.command));
		passed("trigger-binding-" + std::to_string(kind));
	}
	for (int kind = 0; kind < 7; ++kind)
	{
		auto terms = f->terms;
		switch (kind)
		{
		case 0:
			terms.action_operation = terms.triggering_acceptance;
			break;
		case 1:
			terms.action_source.source = id(99);
			break;
		case 2:
			++terms.action_source.slot;
			break;
		case 3:
			terms.action_source.kind = economic_source_kind::npc_generation;
			break;
		case 4:
			terms.xp_award_count = 1;
			break;
		case 5:
			terms.xp_awards[0].amount = 69;
			break;
		case 6:
			terms.xp_awards[1].recipient_pid = 12;
			break;
		}
		std::vector<uint8_t> sentinel{ 0xa5 };
		assert(!quest_fee_reward_continuation_encode(terms, &sentinel) &&
		       sentinel == std::vector<uint8_t>{ 0xa5 });
		passed("continuation-refusal-" + std::to_string(kind));
	}
	for (bool corpse : { false, true })
	{
		auto receipt = f->accepted;
		if (corpse)
			receipt.corpse_revision = 1;
		else
			receipt.collector_catalog_changed = true;
		auto terms = f->terms;
		assert(item_transfer_command_encode_result(receipt,
							   &terms.original_acceptance_result));
		assert(!quest_fee_reward_trigger_binding_valid(terms, f->parent.command));
		std::vector<uint8_t> out{ 0xa5 };
		assert(!quest_fee_reward_continuation_encode(terms, &out) &&
		       out == std::vector<uint8_t>{ 0xa5 });
		passed(corpse ? "typed48-corpse" : "typed48-collector");
	}
	for (int kind = 0; kind < 4; ++kind)
	{
		auto parent = carrier(f->parent), child = carrier(f->child);
		if (kind == 0)
			++child.command.accepted_at_usec; // Same payload, wrong frozen frame.
		if (kind == 1)
			child.command.operation_id = id(99);
		if (kind == 2)
			child.phase = critical_native_recovery_phase::execution_pending;
		if (kind == 3)
			parent.revision = 0;
		const auto original_parent = frame(parent.command),
			   original_child = frame(child.command);
		assert(!native_quest_recovery_pair_context_valid(parent, child, nullptr));
		assert(frame(parent.command) == original_parent &&
		       frame(child.command) == original_child);
		passed("original-pair-refusal-" + std::to_string(kind));
	}
}

void projection_controls()
{
	const std::array<native_quest_cost_requirement, 4> costs{
		{ { 0, 25 }, { 2, 0 }, { 3, -1 }, { 4, 100 } }
	};
	native_quest_cost_projection valid;
	assert(native_quest_cost_project({ 0, 0, 1, 0 }, 17, costs, &valid) == cost_result::ok);
	assert(valid.attempts[0].outcome == native_quest_cost_attempt_outcome::charged &&
	       valid.attempts[1].outcome == native_quest_cost_attempt_outcome::nonpositive &&
	       valid.attempts[2].outcome == native_quest_cost_attempt_outcome::nonpositive &&
	       valid.attempts[3].outcome == native_quest_cost_attempt_outcome::insufficient &&
	       valid.after_revision == 18);
	std::vector<uint8_t> bytes;
	assert(native_quest_cost_projection_encode(valid, &bytes) == cost_result::ok);
	native_quest_cost_projection decoded;
	assert(native_quest_cost_projection_decode(bytes, &decoded) == cost_result::ok &&
	       decoded == valid);
	passed("ordered-charge-change-nonpositive-insufficient-valid");
	const std::array<native_quest_cost_requirement, 2> no_charge{ { { 0, 0 }, { 1, 101 } } };
	assert(native_quest_cost_project({ 0, 0, 1, 0 }, UINT64_MAX, no_charge, &decoded) ==
	       cost_result::ok);
	assert(decoded.before == decoded.after && decoded.after_revision == UINT64_MAX);
	passed("uncharged-max-revision-valid");
	for (int kind = 0; kind < 4; ++kind)
	{
		auto bad = valid;
		if (kind == 0)
			++bad.after[0];
		if (kind == 1)
			++bad.after_revision;
		if (kind == 2)
			++bad.attempts[0].requirement.copper;
		if (kind == 3)
			bad.attempts[1].requirement.slot = 0;
		std::vector<uint8_t> sentinel{ 0xa5 };
		assert(native_quest_cost_projection_encode(bad, &sentinel) != cost_result::ok &&
		       sentinel == std::vector<uint8_t>{ 0xa5 });
		passed("projection-encode-refusal-" + std::to_string(kind));
	}
	for (int kind = 0; kind < 4; ++kind)
	{
		auto before = valid.before;
		uint64_t revision = 17;
		if (kind == 0)
			before[0] = -1;
		if (kind == 1)
			before[3] = INT_MAX;
		if (kind == 2)
			revision = 0;
		if (kind == 3)
			revision = UINT64_MAX;
		decoded = valid;
		assert(native_quest_cost_project(before, revision, costs, &decoded) !=
			       cost_result::ok &&
		       decoded == valid);
		passed("projection-input-refusal-" + std::to_string(kind));
	}
	auto corrupt = bytes;
	corrupt.back() ^= 1;
	decoded = valid;
	assert(native_quest_cost_projection_decode(corrupt, &decoded) != cost_result::ok &&
	       decoded == valid);
	passed("projection-byte-refusal-preserves-output");
}

void receipt_and_transition_controls()
{
	auto f = modeled();
	auto child = carrier(f->child);
	critical_completion completion{};
	completion.operation_id = child.command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	completion.result_size = f->child.retained.receipt.result_size;
	completion.result_payload = f->child.retained.receipt.result_payload;
	completion.durable_revision = f->child.retained.receipt.durable_revision;
	for (auto outcome :
	     { critical_apply_outcome::applied, critical_apply_outcome::already_applied })
	{
		completion.outcome = outcome;
		assert(native_quest_recovery_fee_ack_context_valid(child, completion));
		passed(outcome == critical_apply_outcome::applied ? "fee-ack-applied" :
								    "fee-ack-already-applied");
	}
	for (bool phase : { false, true })
	{
		auto invalid = child;
		if (phase)
			invalid.phase = critical_native_recovery_phase::execution_pending;
		else
			invalid.revision = 0;
		assert(!native_quest_recovery_fee_ack_context_valid(invalid, completion));
		passed(phase ? "fee-ack-wrong-phase" : "fee-ack-zero-revision");
	}
	for (int kind = 0; kind < 9; ++kind)
	{
		auto bad = completion;
		switch (kind)
		{
		case 0:
			bad.result_payload[64] = 1;
			break;
		case 1:
			bad.result_payload[63] ^= 1;
			break;
		case 2:
			--bad.result_size;
			break;
		case 3:
			++bad.durable_revision;
			break;
		case 4:
			bad.operation_id = id(99);
			break;
		case 5:
			bad.outcome = critical_apply_outcome::terminal_failure;
			break;
		case 6:
			bad.error_code = ESTALE;
			break;
		case 7:
			bad.failure_stage = critical_failure_stage::coin_source_wallet_revision;
			break;
		case 8:
			bad.disposition = critical_completion_disposition::never_admitted;
			break;
		}
		assert(!native_quest_recovery_fee_ack_context_valid(child, bad));
		passed("fee-ack-full-receipt-refusal-" + std::to_string(kind));
	}
	for (int kind = 0; kind < 9; ++kind)
	{
		auto bad = f->child.retained;
		if (kind == 0)
			bad.receipt.result_payload[64] = 1;
		if (kind == 1)
			--bad.receipt.result_size;
		if (kind == 2)
			++bad.receipt.durable_revision;
		if (kind == 3)
			bad.receipt.outcome = critical_apply_outcome::terminal_failure;
		if (kind == 4)
			bad.receipt.result_payload[24] ^= 1;
		if (kind == 5)
			bad.receipt.result_payload[32] ^= 1;
		if (kind == 6)
			bad.receipt.result_payload[40] ^= 1;
		if (kind == 7)
			bad.receipt.result_payload[48] ^= 1;
		if (kind == 8)
			bad.receipt.result_payload[56] ^= 1;
		reject_context(f->child, bad, "NFR1-context-refusal-" + std::to_string(kind));
	}
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> encoded{};
	assert(item_native_mobile_fee_result_encode(f->fee_result, &encoded));
	for (int kind = 0; kind < 3; ++kind)
	{
		auto bytes = std::vector<uint8_t>(encoded.begin(), encoded.end());
		if (kind == 0)
			bytes.pop_back();
		if (kind == 1)
			bytes[6] = 1;
		if (kind == 2)
			bytes.push_back(0);
		auto sentinel = f->fee_result;
		assert(!item_native_mobile_fee_result_decode(bytes, &sentinel) &&
		       sentinel == f->fee_result);
		passed("NFR1-codec-refusal-" + std::to_string(kind));
	}
	fee_receipt(*f, true);
	f->child.payload.continuation = {};
	f->child.command =
		build(f->child.payload, f->terms.action_operation, &f->terms.action_source);
	f->parent.retained.next_child_command = frame(f->child.command);
	child = carrier(f->child);
	assert(native_quest_recovery_pair_context_valid(carrier(f->parent), child, nullptr));
	completion.outcome = critical_apply_outcome::terminal_failure;
	completion.error_code = ESTALE;
	completion.result_payload = f->child.retained.receipt.result_payload;
	completion.durable_revision = f->child.retained.receipt.durable_revision;
	assert(native_quest_recovery_fee_ack_context_valid(child, completion));
	passed("terminal-failure-precharge-clocks-no-reward-continuation-valid");
	for (int kind = 0; kind < 5; ++kind)
	{
		auto before = f->before;
		if (kind == 0)
			before.cash.reset();
		if (kind == 1)
			++before.cash->revision;
		if (kind == 2)
			++before.cash->denominations.amount[0];
		if (kind == 3)
			before.state = quest_mobile_lifetime_state::retired;
		if (kind == 4)
			++before.reference.mobile_revision;
		auto sentinel = f->before;
		std::vector<uint8_t> original, preserved;
		assert(quest_mobile_native_image_encode(sentinel, &original) == result::ok);
		assert(quest_mobile_native_fee_transition(before, f->child.payload,
							  f->child.command.operation_id,
							  &sentinel) != result::ok);
		assert(quest_mobile_native_image_encode(sentinel, &preserved) == result::ok &&
		       original == preserved);
		passed("native-transition-refusal-" + std::to_string(kind));
	}
}

void reward_literal_controls()
{
	auto f = modeled();
	quest_reward_continuation decoded;
	assert(quest_reward_continuation_decode(f->child.payload.continuation.data.data(),
						f->child.payload.continuation.data.size(),
						&decoded));
	assert(decoded.reward_count == 5 && decoded.rewards[0].number == 70018 &&
	       decoded.rewards[1].number == 70018 && decoded.rewards[2].number == 17 &&
	       decoded.rewards[3].frozen_amount == 70 &&
	       decoded.rewards[4].flags == QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION &&
	       decoded.xp_award_count == 2 && decoded.xp_awards[1].recipient_pid == 11 &&
	       decoded.xp_awards[1].amount == 30);
	const auto first = quest_fee_item_reward_source_id(decoded, 0),
		   second = quest_fee_item_reward_source_id(decoded, 1);
	assert(first && second && first != second &&
	       quest_fee_item_reward_source_id(decoded, 2) == 0 &&
	       quest_item_reward_source_id(decoded, 0) == first);
	passed("frozen-mixed-rewards-duplicate-ordinals-group-XP-skill-roundtrip");
	auto different = decoded;
	++different.rewards[0].number;
	std::vector<uint8_t> changed;
	assert(quest_fee_reward_continuation_encode(different, &changed) &&
	       changed != f->child.payload.continuation.data);
	assert(quest_fee_item_reward_source_id(different, 0) != first);
	passed("changed-valid-reward-literal-remains-distinct-not-owner-authenticated");
}
} // namespace

int main()
{
	valid_values();
	payload_controls();
	trigger_controls();
	projection_controls();
	receipt_and_transition_controls();
	reward_literal_controls();
	std::printf(
		"{\"classification\":\"modeled values through actual providers\",\"controls\":%zu,\"owner_authenticated\":false,\"SQL\":false,\"journal\":false,\"native_journey\":false}\n",
		controls);
}
