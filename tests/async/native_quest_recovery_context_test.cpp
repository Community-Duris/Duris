#include "world/native_quest_recovery_context.h"
#include "core/config.h"
#include "core/structs.h"
#include "economy/item_transfer_accounting.h"
#include "item/quest_reward_continuation.h"
#include "world/quest_mobile_native.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

// Observe genuine C++ allocations; always delegate to the actual allocator.
// No world/domain/source/ACK provider or private owner is fabricated.
static bool observe_allocations = false;
static size_t allocation_threshold = 0, large_allocations = 0;
static bool observe_child_allocations = false;
static size_t exact_child_allocation = 0, child_allocations = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (observe_allocations && size >= allocation_threshold)
		++large_allocations;
	if (observe_child_allocations && size == exact_child_allocation)
		++child_allocations;
	return __real__Znwm(size);
}

namespace
{
using result = player_snapshot_codec_result;
using context = native_quest_recovery_context;
using stage = native_quest_recovery_publication_stage;
constexpr size_t limit = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;
constexpr size_t branch_charge = sizeof(quest_complete_data) + 2 * sizeof(std::vector<goal_data>) +
				 3 * sizeof(std::string) + 2 * sizeof(void *);
constexpr size_t goal_charge = 2 * sizeof(goal_data);

void allocator_observer_calibration()
{
	allocation_threshold = 1024 * 1024;
	large_allocations = 0;
	observe_allocations = true;
	std::vector<uint8_t> actual(2 * allocation_threshold, 0x5a);
	observe_allocations = false;
	assert(actual.size() == 2 * allocation_threshold && actual[100] == 0x5a &&
	       large_allocations > 0);
}

critical_operation_id id(uint8_t tag)
{
	critical_operation_id value{};
	value.bytes[0] = tag;
	return value;
}

player_item_snapshot item(uint64_t uid, int32_t parent = PLAYER_SNAPSHOT_NO_PARENT,
			  int16_t slot = 0, int32_t vnum = 9001)
{
	player_item_snapshot value{};
	value.object_uid = uid;
	value.parent_index = parent;
	value.equipment_slot = slot;
	value.vnum = vnum;
	value.string_mask = 15;
	value.name = "synthetic context item";
	value.short_description = "a synthetic context item";
	value.description = "A synthetic context item is here.";
	value.timers[0] = 23;
	value.timers[1] = 19;
	value.condition = 17;
	return value;
}

std::vector<uint8_t> forest(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> bytes;
	assert(player_item_snapshot_list_encode(items, &bytes) == result::ok);
	return bytes;
}

struct original
{
	item_transfer_payload payload{};
	critical_command command{};
	context value;
};

original make(bool consumption)
{
	original value;
	auto &payload = value.payload;
	payload.from_owner = { consumption ? item_owner_type::native_mobile :
					     item_owner_type::player,
			       consumption ? uint64_t{ 9000 } : uint64_t{ 10 }, 0 };
	payload.to_owner = { consumption ? item_owner_type::destruction :
					   item_owner_type::native_mobile,
			     consumption ? uint64_t{ 0 } : uint64_t{ 9000 }, 0 };
	payload.reason = consumption ? item_transfer_reason::quest_turnin :
				       item_transfer_reason::quest_offering;
	payload.reason_id = 9001;
	payload.expected_from_revision = consumption ? 7 : 3;
	payload.expected_to_revision = consumption ? 10 : 7;
	payload.multi_root = consumption;
	payload.selected_item_uid = payload.target_root_item_uid = consumption ? 0 : 100;
	payload.item_count = consumption ? 3 : 2;
	payload.items[0] = { 100, 100, 0, 2, 9001, item_custody_state::active };
	payload.items[1] = { 101, 100, 100, 3, 9001, item_custody_state::active };
	std::vector<player_item_snapshot> selected{ item(100), item(101, 0) };
	if (consumption)
	{
		payload.items[2] = { 200, 200, 0, 4, 9001, item_custody_state::active };
		selected.push_back(item(200));
		value.value.native_before = { item(500, PLAYER_SNAPSHOT_NO_PARENT, 1, 9002),
					      item(100), item(101, 1),
					      item(700, PLAYER_SNAPSHOT_NO_PARENT, 0, 9002),
					      item(200) };
		value.value.player_before = { item(900, PLAYER_SNAPSHOT_NO_PARENT, 1, 9002),
					      item(800), item(801, 1) };
	}
	else
	{
		value.value.native_before = { item(500, PLAYER_SNAPSHOT_NO_PARENT, 1, 9002),
					      item(600) };
		value.value.player_before = { item(900, PLAYER_SNAPSHOT_NO_PARENT, 1, 9002),
					      item(100), item(101, 1), item(800) };
	}
	const auto bytes = forest(selected);
	payload.item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
	payload.native_mobile.present = true;
	payload.native_mobile.action = consumption ? item_native_mobile_action::consumption :
						     item_native_mobile_action::acceptance;
	payload.native_mobile.final_giver_pid = 10;
	auto &reference = payload.native_mobile.reference;
	reference.mobile_instance_id = 9000;
	reference.birth_operation = id(1);
	reference.birth_source = { economic_source_kind::npc_generation, id(2), id(3), 4, 5 };
	reference.mobile_vnum = 9001;
	reference.reset_zone_vnum = 1;
	reference.provenance = quest_mobile_birth_provenance::reset;
	reference.mobile_revision = 5;
	reference.stock_revision = 7;
	const std::array<uint64_t, 2> roots{ 200, 100 };
	assert(consumption ?
		       item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, {}, roots) :
		       item_transfer_native_mobile_recovery_freeze(
			       &payload, 10, 23, forest(value.value.player_before)));
	assert(item_transfer_command_build_native_mobile_recovery(
		&value.command, id(consumption ? 16 : 6), payload, critical_source_site::command,
		critical_deadline_class::interactive));
	economic_source_event source{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	assert(item_native_mobile_accounting_intent(
		       value.command, id(7), id(8), 10, consumption ? &source : nullptr,
		       &value.command.accounting_intent) == economic_accounting_error::ok);
	value.command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	value.command.publication_required = true;
	value.command.accepted_at_usec = 1700000000000000ULL;
	assert(critical_command_envelope_valid(value.command));
	value.value.consumed_root_steps.resize(consumption ? 2 : 0);
	value.value.give_hooks.fill(2);
	value.value.branch_program_frozen = true;
	native_quest_recovery_branch a, b;
	a.definition_id = "fixture:original-first";
	a.give = { { 1, 9001 }, { 2, 3 }, { 1, 9001 } };
	a.receive = { { 7, 23 }, { 6, -1 } };
	a.disappear_message_present = true;
	a.disappear = true;
	b.definition_id = "fixture:original-second";
	b.message_present = true; // Present empty differs from absent empty in a.
	b.echo_all = true;
	b.give = { { 2, 4 }, { 1, 9001 } };
	b.receive = { { 6, -1 }, { 7, 23 } };
	value.value.branches = { a, b };
	value.value.parent_acceptance = consumption ? id(6) : critical_operation_id{};
	return value;
}

bool equal(const context &a, const context &b)
{
	return forest(a.native_before) == forest(b.native_before) &&
	       forest(a.player_before) == forest(b.player_before) && a.receipt == b.receipt &&
	       a.publication_stage == b.publication_stage &&
	       a.publication_steps == b.publication_steps && a.give_messages == b.give_messages &&
	       a.consumed_root_steps == b.consumed_root_steps && a.give_hooks == b.give_hooks &&
	       a.branch_program_frozen == b.branch_program_frozen && a.branches == b.branches &&
	       a.next_branch == b.next_branch &&
	       a.parent_acceptance.bytes == b.parent_acceptance.bytes &&
	       a.next_child_operation.bytes == b.next_child_operation.bytes &&
	       a.child_handoff_stage == b.child_handoff_stage &&
	       a.next_child_command == b.next_child_command &&
	       a.latest_child_branch == b.latest_child_branch &&
	       a.latest_child_revision == b.latest_child_revision &&
	       a.latest_child_command == b.latest_child_command &&
	       a.latest_child_attachment == b.latest_child_attachment;
}

std::vector<uint8_t> encode(const critical_command &command, const context &value)
{
	std::vector<uint8_t> bytes;
	assert(native_quest_recovery_context_encode(command, value, &bytes) == result::ok);
	return bytes;
}

void roundtrip(const critical_command &command, const context &value)
{
	const auto bytes = encode(command, value);
	context actual;
	assert(native_quest_recovery_context_decode(command, bytes, &actual) == result::ok);
	assert(equal(actual, value) && encode(command, actual) == bytes);
}

void reject_encode(const critical_command &command, const context &value)
{
	std::vector<uint8_t> output{ 0xa5, 0x5a };
	const auto before = output;
	assert(native_quest_recovery_context_encode(command, value, &output) != result::ok);
	assert(output == before);
}

void reject_decode(const critical_command &command, std::span<const uint8_t> bytes,
		   const context &sentinel, size_t forbidden_allocation = 0)
{
	auto output = sentinel;
	large_allocations = 0;
	allocation_threshold = forbidden_allocation;
	observe_allocations = forbidden_allocation != 0;
	const auto code = native_quest_recovery_context_decode(command, bytes, &output);
	observe_allocations = false;
	assert(code != result::ok && equal(output, sentinel));
	if (forbidden_allocation)
		assert(large_allocations ==
		       0); // Observed actual allocator requests, not provider stubs.
}

uint32_t u32(const std::vector<uint8_t> &bytes, size_t offset)
{
	assert(offset + 4 <= bytes.size());
	uint32_t value = 0;
	for (size_t i = 0; i < 4; ++i)
		value |= uint32_t{ bytes[offset + i] } << (8 * i);
	return value;
}

template <typename T> void set(std::vector<uint8_t> &bytes, size_t offset, T value)
{
	assert(offset + sizeof(T) <= bytes.size());
	for (size_t i = 0; i < sizeof(T); ++i)
		bytes[offset + i] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i));
}

// Locate field boundaries in real encoder output solely for corruption tests.
struct offsets
{
	size_t native_blob, player_blob, receipt, publication, steps, messages, roots, hooks,
		frozen, next_branch, parent, child, handoff, branches, first_branch;
	explicit offsets(const std::vector<uint8_t> &bytes)
	{
		native_blob = 8 + u32(bytes, 4);
		player_blob = native_blob + 4 + u32(bytes, native_blob);
		receipt = player_blob + 4 + u32(bytes, player_blob);
		publication = receipt + 18 + CRITICAL_COMPLETION_RESULT_MAX_BYTES;
		steps = publication + 1;
		messages = steps + 6;
		roots = messages + 3;
		hooks = roots + 4 + u32(bytes, roots);
		frozen = hooks + 9;
		next_branch = frozen + 1;
		parent = next_branch + 4;
		child = parent + 16;
		handoff = child + 16;
		branches = handoff + 1;
		first_branch = branches + 4;
		assert(first_branch <= bytes.size());
	}
};

void forests_program_and_binding()
{
	for (const bool consumption : { false, true })
	{
		const auto original = make(consumption);
		roundtrip(original.command, original.value);
		std::vector<player_item_snapshot> after;
		assert(quest_mobile_native_items_transition(
			       original.value.native_before,
			       original.payload.native_mobile.reference, original.payload,
			       &after) == result::ok);
		if (consumption)
		{
			assert(after.size() == 2 && after[0].object_uid == 500 &&
			       after[1].object_uid == 700);
			assert((original.payload.native_recovery.consumed_root_order ==
				std::vector<uint64_t>{ 200, 100 }));
			// An ancestral frozen program is retained before this child's publication.
			auto child = original.value;
			child.next_branch = 1;
			child.next_child_operation = id(22);
			child.child_handoff_stage = 1;
			assert(!child.receipt.present &&
			       child.publication_stage == stage::captured);
			roundtrip(original.command, child);
		}
		else
			assert(after.size() == 4 && after[0].object_uid == 500 &&
			       after[1].object_uid == 100 && after[2].object_uid == 101 &&
			       after[3].object_uid == 600 && after[2].parent_index == 1);
		auto missing = original.value;
		if (consumption)
			missing.native_before.pop_back();
		else
			missing.player_before.pop_back();
		reject_encode(original.command, missing);
		const auto bytes = encode(original.command, original.value);
		auto other_command = original.command;
		++other_command.accepted_at_usec;
		std::vector<uint8_t> canonical;
		assert(critical_command_encode(other_command, &canonical) ==
		       critical_command_codec_result::ok);
		reject_decode(other_command, bytes, original.value);
		other_command = original.command;
		other_command.operation_id = id(25);
		reject_decode(other_command, bytes, original.value);
		other_command = original.command;
		other_command.accounting_intent.back() ^= 1;
		reject_decode(other_command, bytes, original.value);
		other_command = original.command;
		other_command.publication_required = false;
		reject_decode(other_command, bytes, original.value);
		reject_encode(other_command, original.value);
		assert(native_quest_recovery_context_encode(original.command, original.value,
							    nullptr) == result::invalid_value);
		assert(native_quest_recovery_context_decode(original.command, bytes, nullptr) ==
		       result::invalid_value);
	}
}

void shapes_aliases_and_bounds()
{
	const auto original = make(true);
	const auto bytes = encode(original.command, original.value);
	const offsets at(bytes);
	for (size_t cut : { size_t{ 0 }, size_t{ 3 }, size_t{ 7 }, at.native_blob, at.receipt,
			    at.first_branch, bytes.size() - 1 })
		reject_decode(original.command, std::span(bytes).first(cut), original.value);
	auto trailing = bytes;
	trailing.push_back(0);
	reject_decode(original.command, trailing, original.value);
	for (size_t offset :
	     { size_t{ 0 }, size_t{ 4 }, at.native_blob, at.player_blob, at.receipt, at.publication,
	       at.steps, at.messages, at.hooks, at.frozen, at.handoff, at.first_branch })
	{
		auto corrupt = bytes;
		corrupt[offset] ^= 0xff;
		reject_decode(original.command, corrupt, original.value);
	}
	auto corrupt = bytes;
	set<uint32_t>(corrupt, at.roots,
		      1); // Original ordered two-root stage count must remain exact.
	reject_decode(original.command, corrupt, original.value);
	corrupt = bytes;
	set<uint32_t>(corrupt, at.next_branch, 3);
	reject_decode(original.command, corrupt, original.value);
	corrupt = bytes;
	corrupt[at.child] = 1; // No handoff cannot carry a next-child identity alias.
	reject_decode(original.command, corrupt, original.value);
	corrupt = bytes;
	corrupt[at.receipt + 18 + ITEM_TRANSFER_RESULT_BYTES] = 1;
	reject_decode(original.command, corrupt, original.value);
	for (int kind = 0; kind < 15; ++kind)
	{
		auto bad = original.value;
		if (kind == 0)
			bad.branches[0].message = "absent cannot carry text";
		if (kind == 1)
		{
			bad.branches[0].message_present = true;
			bad.branches[0].message = std::string(1, '\0');
		}
		if (kind == 2)
			bad.branches[0].definition_id.push_back('\0');
		if (kind == 3)
			bad.branches[0].disappear_message = std::string(MAX_STRING_LENGTH, 'd');
		if (kind == 4)
			bad.branches[0].definition_id =
				std::string(QUEST_REWARD_MAX_DEFINITION_ID_BYTES + 1, 'i');
		if (kind == 5)
			bad.give_hooks[0] =
				1; // Frozen ancestral program requires returned hook cut.
		if (kind == 6)
			bad.branch_program_frozen = false;
		if (kind == 7)
			bad.consumed_root_steps[0] = 3;
		if (kind == 8)
			bad.child_handoff_stage = 1;
		if (kind == 9)
			bad.publication_stage = stage::publishing;
		if (kind == 10)
			bad.receipt.outcome = critical_apply_outcome::already_applied;
		if (kind == 11)
			bad.receipt.durable_revision = 1;
		if (kind == 12)
			bad.publication_steps[0] = 3;
		if (kind == 13)
			bad.give_messages[0] = 3;
		if (kind == 14)
			bad.branches[1].disappear_message = "absent disappear cannot carry text";
		reject_encode(original.command, bad);
	}
	auto boundary = original.value;
	boundary.branches[0].message_present = true;
	boundary.branches[0].message = std::string(MAX_STRING_LENGTH - 1, 'm');
	boundary.branches[0].disappear_message = std::string(MAX_STRING_LENGTH - 1, 'd');
	boundary.branches[0].definition_id = std::string(QUEST_REWARD_MAX_DEFINITION_ID_BYTES, 'i');
	roundtrip(original.command, boundary);
	boundary.branches[0].message.push_back('m');
	reject_encode(original.command, boundary);
	// Declared branch count fits the wire but its ORIGINAL cumulative retained
	// program charge does not fit; refuse before allocating decoded branches.
	const size_t branch_count = limit / branch_charge;
	corrupt.assign(bytes.begin(), bytes.begin() + at.first_branch);
	set<uint32_t>(corrupt, at.branches, branch_count);
	corrupt.resize(at.first_branch + 24 * branch_count, 0);
	assert(corrupt.size() < limit &&
	       branch_count * sizeof(native_quest_recovery_branch) > 1024 * 1024);
	reject_decode(original.command, corrupt, original.value, 1024 * 1024);
	// A single branch's goal count likewise fits physical bytes and the isolated
	// count bound, but exceeds the retained budget after its existing charges.
	const size_t goal_count = limit / goal_charge;
	corrupt.assign(bytes.begin(), bytes.begin() + at.first_branch);
	set<uint32_t>(corrupt, at.branches, 1);
	corrupt.resize(at.first_branch + 24 + 5 * goal_count, 0);
	set<uint32_t>(corrupt, at.first_branch + 16, goal_count);
	assert(corrupt.size() < limit &&
	       goal_count * sizeof(native_quest_recovery_goal) > 1024 * 1024);
	reject_decode(original.command, corrupt, original.value, 1024 * 1024);
	// Oversized original message is physically present, not just truncated.
	// Refusal/output preservation is measured; shared-library string allocations
	// are outside the calibrated vector-allocation observer's coverage.
	corrupt.assign(bytes.begin(), bytes.begin() + at.first_branch);
	set<uint32_t>(corrupt, at.branches, 1);
	corrupt.resize(at.first_branch + 8, 0);
	corrupt[at.first_branch] = 1;
	set<uint32_t>(corrupt, at.first_branch + 4, MAX_STRING_LENGTH);
	corrupt.resize(corrupt.size() + MAX_STRING_LENGTH, 'm');
	reject_decode(original.command, corrupt, original.value);
	// Definition bound is exercised on physically supplied bytes after the two
	// empty message blobs, with strong refusal and no global heap claim.
	corrupt.assign(bytes.begin(), bytes.begin() + at.first_branch);
	set<uint32_t>(corrupt, at.branches, 1);
	corrupt.resize(at.first_branch + 16, 0);
	set<uint32_t>(corrupt, at.first_branch + 12, QUEST_REWARD_MAX_DEFINITION_ID_BYTES + 1);
	corrupt.resize(corrupt.size() + QUEST_REWARD_MAX_DEFINITION_ID_BYTES + 1, 'i');
	reject_decode(original.command, corrupt, original.value);
	corrupt.assign(limit + 1, 0);
	reject_decode(original.command, corrupt, original.value, 1024 * 1024);
}

void receipt_and_publication()
{
	for (const bool consumption : { false, true })
	{
		auto original = make(consumption);
		item_transfer_result receipt{};
		receipt.root_item_uid = item_transfer_result_root(original.payload);
		receipt.item_count = original.payload.item_count;
		receipt.from_owner_revision = original.payload.expected_from_revision + 1;
		receipt.to_owner_revision = original.payload.expected_to_revision + 1;
		receipt.max_item_revision = consumption ? 5 : 4;
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> result_bytes{};
		assert(item_transfer_command_encode_result(receipt, &result_bytes));
		auto &retained = original.value.receipt;
		retained.present = true;
		retained.durable_revision =
			std::max({ receipt.from_owner_revision, receipt.to_owner_revision,
				   receipt.max_item_revision });
		retained.result_size = result_bytes.size();
		std::copy(result_bytes.begin(), result_bytes.end(),
			  retained.result_payload.begin());
		original.value.publication_stage = stage::physically_proven;
		original.value.publication_steps.fill(2);
		original.value.give_messages = { 0, 1, 2 };
		for (auto &step : original.value.consumed_root_steps)
			step = 2;
		critical_completion completion{};
		completion.operation_id = original.command.operation_id;
		completion.durable_revision = retained.durable_revision;
		completion.result_size = retained.result_size;
		completion.result_payload = retained.result_payload;
		critical_native_recovery_envelope envelope;
		envelope.command = original.command;
		envelope.revision = 2;
		for (auto saved :
		     { critical_apply_outcome::applied, critical_apply_outcome::already_applied })
		{
			retained.outcome = saved;
			roundtrip(original.command, original.value);
			envelope.attachment = encode(original.command, original.value);
			for (auto delivered : { critical_apply_outcome::applied,
						critical_apply_outcome::already_applied })
			{
				completion.outcome = delivered;
				assert(native_quest_recovery_publication_context_valid(envelope,
										       completion));
			}
		}
		for (int kind = 0; kind < 9; ++kind)
		{
			auto mismatch = completion;
			if (kind == 0)
				mismatch.operation_id = id(99);
			if (kind == 1)
				mismatch.disposition =
					critical_completion_disposition::never_admitted;
			if (kind == 2)
				mismatch.outcome = critical_apply_outcome::terminal_failure;
			if (kind == 3)
				++mismatch.durable_revision;
			if (kind == 4)
				++mismatch.error_code;
			if (kind == 5)
				mismatch.failure_stage =
					critical_failure_stage::coin_source_wallet_revision;
			if (kind == 6)
				--mismatch.result_size;
			if (kind == 7)
				mismatch.result_payload[0] ^= 1;
			if (kind == 8)
				mismatch.result_payload.back() = 1;
			assert(!native_quest_recovery_publication_context_valid(envelope,
										mismatch));
		}
		for (auto outcome : { critical_apply_outcome::retryable_failure,
				      critical_apply_outcome::ambiguous_commit })
		{
			auto mismatch = completion;
			mismatch.outcome = outcome;
			assert(!native_quest_recovery_publication_context_valid(envelope,
										mismatch));
		}
		auto bad_envelope = envelope;
		bad_envelope.phase = critical_native_recovery_phase::continuation_pending;
		assert(!native_quest_recovery_publication_context_valid(bad_envelope, completion));
		bad_envelope = envelope;
		bad_envelope.revision = 0;
		assert(!native_quest_recovery_publication_context_valid(bad_envelope, completion));
		auto pending = original.value;
		pending.publication_stage = stage::publishing;
		bad_envelope = envelope;
		bad_envelope.attachment = encode(original.command, pending);
		assert(!native_quest_recovery_publication_context_valid(bad_envelope, completion));
		// Delivery diagnostics are outside the retained receipt identity contract.
		completion.attempt = 9;
		completion.queued_at_usec = completion.started_at_usec =
			completion.completed_at_usec = 100;
		completion.recovery_correlation[0] = 'd';
		assert(native_quest_recovery_publication_context_valid(envelope, completion));
		auto invalid_receipt = original.value;
		invalid_receipt.receipt.result_payload[ITEM_TRANSFER_RESULT_BYTES] = 1;
		reject_encode(original.command, invalid_receipt);
		invalid_receipt = original.value;
		++invalid_receipt.receipt.durable_revision;
		reject_encode(original.command, invalid_receipt);
		invalid_receipt = original.value;
		invalid_receipt.receipt.failure_stage =
			critical_failure_stage::coin_source_wallet_revision;
		reject_encode(original.command, invalid_receipt);
		invalid_receipt = original.value;
		invalid_receipt.receipt.outcome = critical_apply_outcome::retryable_failure;
		reject_encode(original.command, invalid_receipt);
		invalid_receipt = original.value;
		--invalid_receipt.receipt.result_size;
		reject_encode(original.command, invalid_receipt);
		// An actual typed terminal result retains its original lower revisions.
		receipt.from_owner_revision = original.payload.expected_from_revision;
		receipt.to_owner_revision = original.payload.expected_to_revision;
		receipt.max_item_revision = 0;
		assert(item_transfer_command_encode_result(receipt, &result_bytes));
		retained.result_payload.fill(0);
		std::copy(result_bytes.begin(), result_bytes.end(),
			  retained.result_payload.begin());
		retained.durable_revision =
			std::max(receipt.from_owner_revision, receipt.to_owner_revision);
		retained.outcome = critical_apply_outcome::terminal_failure;
		retained.error_code = ESTALE;
		roundtrip(original.command, original.value);
		completion.outcome = retained.outcome;
		completion.error_code = retained.error_code;
		completion.durable_revision = retained.durable_revision;
		completion.result_payload = retained.result_payload;
		envelope.attachment = encode(original.command, original.value);
		assert(native_quest_recovery_publication_context_valid(envelope, completion));
		completion.outcome = critical_apply_outcome::already_applied;
		assert(!native_quest_recovery_publication_context_valid(envelope, completion));
	}
}

// Frozen NQR1 wire layout from the actual parent codec (before NQR2).
// This compatibility oracle never invokes the current context encoder.
struct historical_nqr1_writer
{
	std::vector<uint8_t> bytes{ 'N', 'Q', 'R', '1' };
	void raw(std::span<const uint8_t> value)
	{
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	template <typename T> void number(T value)
	{
		for (size_t i = 0; i < sizeof(T); ++i)
			bytes.push_back(
				static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i)));
	}
	void blob(std::span<const uint8_t> value)
	{
		number<uint32_t>(value.size());
		raw(value);
	}
	void text(const std::string &value)
	{
		blob({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
	void goals(const std::vector<native_quest_recovery_goal> &values)
	{
		number<uint32_t>(values.size());
		for (const auto &goal : values)
		{
			number<uint8_t>(goal.type);
			number<int32_t>(goal.number);
		}
	}
};

std::vector<uint8_t> historical_nqr1(const critical_command &command, const context &value)
{
	assert(value.next_child_command.empty());
	std::vector<uint8_t> command_bytes;
	assert(critical_command_encode(command, &command_bytes) ==
	       critical_command_codec_result::ok);
	historical_nqr1_writer out;
	out.blob(command_bytes);
	out.blob(forest(value.native_before));
	out.blob(forest(value.player_before));
	out.number<uint8_t>(value.receipt.present);
	out.number<uint8_t>(static_cast<uint8_t>(value.receipt.outcome));
	out.number<uint64_t>(value.receipt.durable_revision);
	out.number<uint32_t>(value.receipt.error_code);
	out.number<uint16_t>(static_cast<uint16_t>(value.receipt.failure_stage));
	out.number<uint16_t>(value.receipt.result_size);
	out.raw(value.receipt.result_payload);
	out.number<uint8_t>(static_cast<uint8_t>(value.publication_stage));
	out.raw(value.publication_steps);
	out.raw(value.give_messages);
	out.blob(value.consumed_root_steps);
	out.raw(value.give_hooks);
	out.number<uint8_t>(value.branch_program_frozen);
	out.number<uint32_t>(value.next_branch);
	out.raw(value.parent_acceptance.bytes);
	out.raw(value.next_child_operation.bytes);
	out.number<uint8_t>(value.child_handoff_stage);
	out.number<uint32_t>(value.branches.size());
	for (const auto &branch : value.branches)
	{
		out.number<uint8_t>(branch.message_present);
		out.number<uint8_t>(branch.disappear_message_present);
		out.number<uint8_t>(branch.echo_all);
		out.number<uint8_t>(branch.disappear);
		out.text(branch.message);
		out.text(branch.disappear_message);
		out.text(branch.definition_id);
		out.goals(branch.give);
		out.goals(branch.receive);
	}
	return out.bytes;
}

// Synthetic receipt/stage values exercise structure, never physical authority.
void physical_receipt(original *value)
{
	item_transfer_result receipt{};
	receipt.root_item_uid = item_transfer_result_root(value->payload);
	receipt.item_count = value->payload.item_count;
	receipt.from_owner_revision = value->payload.expected_from_revision + 1;
	receipt.to_owner_revision = value->payload.expected_to_revision + 1;
	receipt.max_item_revision = 4;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes{};
	assert(item_transfer_command_encode_result(receipt, &bytes));
	auto &retained = value->value.receipt;
	retained.present = true;
	retained.durable_revision =
		std::max({ receipt.from_owner_revision, receipt.to_owner_revision,
			   receipt.max_item_revision });
	retained.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), retained.result_payload.begin());
	value->value.publication_stage = stage::physically_proven;
	value->value.publication_steps.fill(2);
}

std::vector<uint8_t> command_frame(const critical_command &value)
{
	std::vector<uint8_t> bytes;
	assert(critical_command_encode(value, &bytes) == critical_command_codec_result::ok);
	return bytes;
}

critical_command rebuild_child(const original &child, const item_transfer_payload &payload)
{
	critical_command command;
	assert(item_transfer_command_build_native_mobile_recovery(
		&command, child.command.operation_id, payload, child.command.source_site,
		child.command.deadline_class));
	economic_source_event source{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	assert(item_native_mobile_accounting_intent(
		       command, id(7), id(8), payload.native_mobile.final_giver_pid, &source,
		       &command.accounting_intent) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.publication_required = true;
	command.accepted_at_usec = child.command.accepted_at_usec;
	assert(critical_command_envelope_valid(command));
	return command;
}

// Real codec values only. These stages/receipts do not grant SQL, world or ACK authority.
original latest_child_leaf()
{
	auto child = make(true);
	physical_receipt(&child);
	child.value.branch_program_frozen = false;
	child.value.branches.clear();
	child.value.give_hooks.fill(0);
	std::fill(child.value.consumed_root_steps.begin(), child.value.consumed_root_steps.end(),
		  2);
	item_transfer_result result{};
	assert(item_transfer_command_decode_result(child.value.receipt.result_payload.data(),
						   child.value.receipt.result_size, &result));
	result.max_item_revision = 5;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes{};
	assert(item_transfer_command_encode_result(result, &bytes));
	std::copy(bytes.begin(), bytes.end(), child.value.receipt.result_payload.begin());
	return child;
}

critical_native_recovery_envelope carrier(const original &value, uint64_t revision)
{
	critical_native_recovery_envelope envelope;
	envelope.command = value.command;
	envelope.revision = revision;
	envelope.phase = critical_native_recovery_phase::continuation_pending;
	envelope.attachment = encode(value.command, value.value);
	return envelope;
}

void reject_pair(const critical_native_recovery_envelope &parent,
		 const critical_native_recovery_envelope &child,
		 const critical_native_recovery_envelope *successor)
{
	const auto parent_frame = command_frame(parent.command),
		   child_frame = command_frame(child.command);
	const auto parent_bytes = parent.attachment, child_bytes = child.attachment;
	const auto parent_revision = parent.revision, child_revision = child.revision;
	const auto successor_frame = successor ? command_frame(successor->command) :
						 std::vector<uint8_t>{};
	const auto successor_bytes = successor ? successor->attachment : std::vector<uint8_t>{};
	assert(!native_quest_recovery_pair_context_valid(parent, child, successor));
	assert(command_frame(parent.command) == parent_frame &&
	       command_frame(child.command) == child_frame && parent.attachment == parent_bytes &&
	       child.attachment == child_bytes && parent.revision == parent_revision &&
	       child.revision == child_revision);
	if (successor)
		assert(command_frame(successor->command) == successor_frame &&
		       successor->attachment == successor_bytes);
}

void latest_child_frames()
{
	auto parent = make(false);
	physical_receipt(&parent);
	auto leaf = latest_child_leaf();
	parent.value.parent_acceptance = parent.command.operation_id;
	parent.value.next_branch = static_cast<uint32_t>(parent.value.branches.size() - 1);
	parent.value.next_child_operation = leaf.command.operation_id;
	parent.value.next_child_command = command_frame(leaf.command);
	parent.value.child_handoff_stage = 2;
	const auto pending = carrier(parent, 11), child = carrier(leaf, 7);
	assert(pending.attachment[3] == '2' && child.attachment[3] == '1');
	auto returned = parent.value;
	returned.latest_child_branch = returned.next_branch;
	returned.latest_child_revision = child.revision;
	returned.latest_child_command = command_frame(child.command);
	returned.latest_child_attachment = child.attachment;
	++returned.next_branch;
	returned.next_child_operation = {};
	returned.next_child_command.clear();
	returned.child_handoff_stage = 0;
	auto successor = pending;
	++successor.revision;
	successor.attachment = encode(parent.command, returned);
	assert(successor.attachment[3] == '3');
	assert(returned.next_branch == returned.branches.size() &&
	       returned.next_child_command.empty());
	roundtrip(parent.command, returned);
	assert(native_quest_recovery_pair_context_valid(pending, child, &successor));
	reject_pair(pending, child,
		    nullptr); // A successful prefix requires actual correlated advancement.
	// A nonzero latest revision is value-only; the actual supplied pair binds it exactly.
	auto other_revision = returned;
	++other_revision.latest_child_revision;
	roundtrip(parent.command, other_revision);
	auto bad_successor = successor;
	bad_successor.attachment = encode(parent.command, other_revision);
	reject_pair(pending, child, &bad_successor);
	for (int kind = 0; kind < 12; ++kind)
	{
		auto bad = returned;
		if (kind == 0)
			bad.latest_child_revision = 0;
		if (kind == 1)
			bad.latest_child_branch = bad.next_branch;
		if (kind == 2)
			--bad.next_branch;
		if (kind == 3)
			bad.latest_child_command.clear();
		if (kind == 4)
			bad.latest_child_attachment.clear();
		if (kind == 5)
			bad.latest_child_attachment[3] = '3'; // No recursive latest history.
		if (kind == 6)
			bad.latest_child_attachment.push_back(0);
		if (kind == 7)
			bad.latest_child_command.push_back(0);
		if (kind == 8)
			bad.latest_child_attachment[3] = '2'; // Required leaf is NQR1.
		const offsets leaf_at(child.attachment);
		if (kind == 9)
			bad.latest_child_attachment[leaf_at.receipt + 2] ^= 1;
		if (kind == 10)
			bad.latest_child_attachment[leaf_at.receipt + 14] ^= 1;
		if (kind == 11)
			bad.latest_child_attachment[leaf_at.receipt + 18 +
						    ITEM_TRANSFER_RESULT_BYTES] = 1;
		reject_encode(parent.command, bad);
	}
	for (int kind = 0; kind < 4; ++kind)
	{
		auto bad_leaf = leaf;
		if (kind == 0)
			bad_leaf.value.publication_stage = stage::publishing;
		if (kind == 1)
		{
			bad_leaf.value.receipt = {};
			bad_leaf.value.publication_stage = stage::captured;
		}
		if (kind == 2)
		{
			bad_leaf.value.receipt.outcome = critical_apply_outcome::terminal_failure;
			bad_leaf.value.receipt.error_code = ESTALE;
		}
		if (kind == 3)
			bad_leaf.value.parent_acceptance = id(99);
		auto bad = returned;
		bad.latest_child_attachment = encode(bad_leaf.command, bad_leaf.value);
		reject_encode(parent.command, bad);
	}
	// Strong decode preservation now includes ALL latest fields in its nonempty sentinel.
	const auto historical = historical_nqr1(parent.command, returned);
	const size_t tail = historical.size() + 4; // NQR3 empty next-child blob.
	for (int kind = 0; kind < 8; ++kind)
	{
		auto malformed = successor.attachment;
		if (kind == 0)
			malformed[3] = '2';
		if (kind == 1)
			malformed.resize(tail);
		if (kind == 2)
			set<uint32_t>(malformed, tail, returned.next_branch);
		if (kind == 3)
			set<uint64_t>(malformed, tail + 4, 0);
		if (kind == 4)
			set<uint32_t>(malformed, tail + 12, CRITICAL_COMMAND_MAX_ENCODED_BYTES + 1);
		if (kind == 5)
			malformed[tail + 16] ^= 0xff;
		if (kind == 6)
			malformed.pop_back();
		if (kind == 7)
			malformed.push_back(0);
		reject_decode(parent.command, malformed, returned);
	}
	for (int kind = 0; kind < 9; ++kind)
	{
		auto bad_parent = pending, bad_child = child, bad_next = successor;
		if (kind == 0)
			bad_parent.phase = critical_native_recovery_phase::execution_pending;
		if (kind == 1)
			bad_child.phase = critical_native_recovery_phase::execution_pending;
		if (kind == 2)
			bad_parent.revision = 0;
		if (kind == 3)
			bad_child.revision = 0;
		if (kind == 4)
			++bad_next.revision;
		if (kind == 5)
			bad_next.phase = critical_native_recovery_phase::execution_pending;
		if (kind == 6)
			bad_child.command.operation_id = parent.command.operation_id;
		if (kind == 7)
		{
			auto before = parent.value;
			before.child_handoff_stage = 1;
			bad_parent.attachment = encode(parent.command, before);
		}
		if (kind == 8)
		{
			auto wrong = returned;
			wrong.latest_child_branch =
				0; // Legal prior-branch value; wrong original pair.
			bad_next.attachment = encode(parent.command, wrong);
		}
		reject_pair(bad_parent, bad_child, &bad_next);
	}
	// Genuine-domain terminal proof remains separate; this validates its codec correlation only.
	auto rejected = leaf;
	rejected.value.receipt.outcome = critical_apply_outcome::terminal_failure;
	rejected.value.receipt.error_code = ESTALE;
	item_transfer_result rejection{};
	rejection.root_item_uid = item_transfer_result_root(rejected.payload);
	rejection.item_count = rejected.payload.item_count;
	rejection.from_owner_revision = rejected.payload.expected_from_revision;
	rejection.to_owner_revision = rejected.payload.expected_to_revision;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> rejected_bytes{};
	assert(item_transfer_command_encode_result(rejection, &rejected_bytes));
	rejected.value.receipt.result_payload.fill(0);
	std::copy(rejected_bytes.begin(), rejected_bytes.end(),
		  rejected.value.receipt.result_payload.begin());
	rejected.value.receipt.durable_revision =
		std::max(rejection.from_owner_revision, rejection.to_owner_revision);
	std::fill(rejected.value.consumed_root_steps.begin(),
		  rejected.value.consumed_root_steps.end(), 0);
	const auto rejected_child = carrier(rejected, child.revision);
	for (uint8_t handoff : { uint8_t{ 1 }, uint8_t{ 2 } })
	{
		auto before = parent.value;
		before.child_handoff_stage = handoff;
		auto terminal_parent = pending;
		terminal_parent.attachment = encode(parent.command, before);
		assert(native_quest_recovery_pair_context_valid(terminal_parent, rejected_child,
								nullptr));
		reject_pair(terminal_parent, rejected_child, &successor);
	}
}

void exact_child_frames()
{
	for (const bool consumption : { false, true })
	{
		const auto old = make(consumption);
		const auto historical = historical_nqr1(old.command, old.value);
		assert(encode(old.command, old.value) == historical);
		context restored;
		assert(native_quest_recovery_context_decode(old.command, historical, &restored) ==
		       result::ok);
		assert(equal(restored, old.value) && restored.next_child_command.empty());
	}
	auto parent = make(false);
	physical_receipt(&parent);
	const auto child = make(true);
	parent.value.parent_acceptance = parent.command.operation_id;
	parent.value.next_child_operation = child.command.operation_id;
	parent.value.child_handoff_stage = 1;
	parent.value.next_branch = 1;
	const auto historical = historical_nqr1(parent.command, parent.value);
	assert(encode(parent.command, parent.value) == historical);
	parent.value.next_child_command = command_frame(child.command);
	const auto bytes = encode(parent.command, parent.value);
	assert(bytes[3] == '2' &&
	       bytes.size() == historical.size() + 4 + parent.value.next_child_command.size());
	auto prefix = bytes;
	prefix.resize(historical.size());
	prefix[3] = '1';
	assert(prefix == historical);
	roundtrip(parent.command, parent.value);
	auto returned = parent.value;
	returned.child_handoff_stage = 2;
	roundtrip(parent.command, returned);
	context restored;
	assert(native_quest_recovery_context_decode(parent.command, bytes, &restored) ==
	       result::ok);
	assert(restored.next_child_command == command_frame(child.command));
	critical_command restored_child;
	assert(critical_command_decode(restored.next_child_command.data(),
				       restored.next_child_command.size(),
				       &restored_child) == critical_command_codec_result::ok);
	assert(command_frame(restored_child) == command_frame(child.command));
	item_transfer_payload restored_payload{};
	assert(item_transfer_command_decode_payload(restored_child, &restored_payload));
	assert(restored_payload.native_recovery.acknowledged_save_revision == 23 &&
	       (restored_payload.native_recovery.consumed_root_order ==
		std::vector<uint64_t>{ 200, 100 }));
	const size_t blob = historical.size();
	for (int kind = 0; kind < 8; ++kind)
	{
		auto malformed = bytes;
		if (kind == 0)
			malformed.push_back(0); // Context trailing alias.
		if (kind == 1)
		{
			malformed.resize(blob + 4);
			set<uint32_t>(malformed, blob, 0);
		}
		if (kind == 2)
			malformed[3] = '1'; // NQR1 cannot carry a child tail.
		if (kind == 3)
			malformed.resize(blob); // NQR2 requires a blob.
		if (kind == 4)
			malformed.pop_back();
		if (kind == 5)
		{
			malformed.push_back(0);
			set<uint32_t>(malformed, blob, parent.value.next_child_command.size() + 1);
		}
		if (kind == 6)
			malformed[blob + 4] ^= 0xff; // Actual command magic.
		if (kind == 7)
		{
			set<uint32_t>(malformed, blob, CRITICAL_COMMAND_MAX_ENCODED_BYTES + 1);
			malformed.resize(blob + 4 + CRITICAL_COMMAND_MAX_ENCODED_BYTES + 1, 0);
		}
		reject_decode(parent.command, malformed, parent.value);
	}
	for (int kind = 0; kind < 12; ++kind)
	{
		auto mismatch = parent.value;
		if (kind == 0)
			mismatch.next_child_operation = id(99);
		if (kind == 1)
		{
			mismatch.next_child_operation = parent.command.operation_id;
			auto self = child.command;
			self.operation_id = parent.command.operation_id;
			mismatch.next_child_command = command_frame(self);
		}
		if (kind == 2)
			mismatch.parent_acceptance = id(99);
		if (kind == 3)
			mismatch.publication_stage = stage::publishing;
		if (kind == 4)
			mismatch.next_branch = mismatch.branches.size();
		if (kind == 5)
			mismatch.child_handoff_stage = 0;
		if (kind == 6)
			mismatch.branch_program_frozen = false;
		if (kind == 7)
			mismatch.next_child_command.assign(CRITICAL_COMMAND_MAX_ENCODED_BYTES + 1,
							   0);
		if (kind == 8)
		{
			auto bad = child.payload;
			bad.native_mobile.reference.birth_operation = id(99);
			mismatch.next_child_command = command_frame(rebuild_child(child, bad));
		}
		if (kind == 9)
		{
			auto bad = child.payload;
			++bad.native_mobile.reference.birth_source.sequence;
			mismatch.next_child_command = command_frame(rebuild_child(child, bad));
		}
		if (kind == 10)
		{
			auto bad = child.payload;
			bad.native_mobile.final_giver_pid = bad.native_recovery.player_pid = 11;
			mismatch.next_child_command = command_frame(rebuild_child(child, bad));
		}
		if (kind == 11)
		{
			auto acceptance = parent.command;
			acceptance.operation_id = child.command.operation_id;
			mismatch.next_child_command = command_frame(acceptance);
		}
		reject_encode(parent.command, mismatch);
		auto child_bytes = mismatch.next_child_command;
		mismatch.next_child_command.clear();
		auto malformed = historical_nqr1(parent.command, mismatch);
		malformed[3] = '2';
		const size_t end = malformed.size();
		malformed.resize(end + 4);
		set<uint32_t>(malformed, end, child_bytes.size());
		malformed.insert(malformed.end(), child_bytes.begin(), child_bytes.end());
		reject_decode(parent.command, malformed, parent.value);
	}
	// Canonical command frames whose envelope properties independently refuse.
	for (int kind = 0; kind < 3; ++kind)
	{
		auto bad = child.command;
		if (kind == 0)
			bad.publication_required = false;
		if (kind == 1)
		{
			bad.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			bad.accounting_intent.clear();
			bad.publication_required = false;
		}
		if (kind == 2)
			bad.payload_version = ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
		auto mismatch = parent.value;
		mismatch.next_child_command = command_frame(bad);
		reject_encode(parent.command, mismatch);
	}
	// Keep the complete command frame canonical while corrupting one literal
	// payload PID. The actual item decoder, not a fixture stub, must refuse.
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference_bytes{};
	assert(quest_mobile_native_reference_encode(child.payload.native_mobile.reference,
						    &reference_bytes) == result::ok);
	const auto reference = std::search(child.command.payload.begin(),
					   child.command.payload.end(), reference_bytes.begin(),
					   reference_bytes.end());
	assert(reference != child.command.payload.end());
	const size_t reference_offset = reference - child.command.payload.begin();
	assert(reference_offset >= 16 &&
	       std::search(reference + reference_bytes.size(), child.command.payload.end(),
			   reference_bytes.begin(),
			   reference_bytes.end()) == child.command.payload.end());
	for (const size_t pid :
	     { reference_offset - 8,
	       reference_offset + ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES - 16 + 8 })
	{
		auto bad = child.command;
		set<uint32_t>(bad.payload, pid, 11);
		item_transfer_payload rejected{};
		assert(!item_transfer_command_decode_payload(bad, &rejected));
		auto mismatch = parent.value;
		mismatch.next_child_command = command_frame(bad);
		reject_encode(parent.command, mismatch);
	}
	// Revision differences are original child facts, not a static +1 alias.
	auto later = child.payload;
	++later.native_mobile.reference.mobile_revision;
	++later.native_mobile.reference.stock_revision;
	auto permitted = parent.value;
	permitted.next_child_command = command_frame(rebuild_child(child, later));
	roundtrip(parent.command, permitted);
	// An actual consumption parent cannot own an acceptance->child handoff.
	auto consumption_parent = child.value;
	consumption_parent.parent_acceptance = child.command.operation_id;
	consumption_parent.next_child_operation = id(99);
	consumption_parent.child_handoff_stage = 1;
	consumption_parent.next_child_command = parent.value.next_child_command;
	reject_encode(child.command, consumption_parent);
	assert(native_quest_recovery_context_encode(parent.command, parent.value, nullptr) ==
	       result::invalid_value);
	assert(native_quest_recovery_context_decode(parent.command, bytes, nullptr) ==
	       result::invalid_value);
	latest_child_frames();
}

void child_combined_retained_budget()
{
	auto parent = make(false);
	physical_receipt(&parent);
	const auto child = make(true);
	parent.value.parent_acceptance = parent.command.operation_id;
	parent.value.next_child_operation = child.command.operation_id;
	parent.value.child_handoff_stage = 1;
	parent.value.next_branch = 0;
	parent.value.branches.resize(1);
	parent.value.branches[0] = {};
	// Large synthetic blob deliberately fails canonical command parsing.
	// Positive observation calibrates this exact vector-copy size first.
	auto bytes = historical_nqr1(parent.command, parent.value);
	bytes[3] = '2';
	const size_t blob_size = CRITICAL_COMMAND_MAX_ENCODED_BYTES;
	const size_t old_end = bytes.size();
	bytes.resize(old_end + 4 + blob_size, 0);
	set<uint32_t>(bytes, old_end, blob_size);
	exact_child_allocation = blob_size;
	child_allocations = 0;
	observe_child_allocations = true;
	reject_decode(parent.command, bytes, parent.value);
	observe_child_allocations = false;
	assert(child_allocations > 0);
	// Physically present goals fit their isolated bound and are charged before
	// decoding. The subsequent child blob fits its own bound, but not their sum.
	const size_t base = sizeof(context) + branch_charge + 3;
	const size_t count = (limit - base - blob_size) / goal_charge + 1;
	assert(base + count * goal_charge <= limit &&
	       base + count * goal_charge + blob_size > limit);
	assert(count * sizeof(native_quest_recovery_goal) != blob_size);
	bytes = historical_nqr1(parent.command, parent.value);
	bytes[3] = '2';
	const offsets at(bytes);
	set<uint32_t>(bytes, at.first_branch + 16, count);
	bytes.insert(bytes.begin() + at.first_branch + 20, 5 * count, 0);
	const size_t end = bytes.size();
	bytes.resize(end + 4 + blob_size, 0);
	set<uint32_t>(bytes, end, blob_size);
	assert(bytes.size() < limit);
	child_allocations = 0;
	observe_child_allocations = true;
	reject_decode(parent.command, bytes, parent.value);
	observe_child_allocations = false;
	assert(child_allocations == 0); // Refusal precedes exact child-vector assign.

	// The NQR3 latest-command charge shares the original cumulative retained budget.
	// Physically supplied goals fit alone, but leave no room for its actual next blob.
	auto latest_parent = make(false);
	physical_receipt(&latest_parent);
	latest_parent.value.parent_acceptance = latest_parent.command.operation_id;
	latest_parent.value.branches.resize(1);
	latest_parent.value.branches[0] = {};
	latest_parent.value.next_branch = 1;
	bytes = historical_nqr1(latest_parent.command, latest_parent.value);
	bytes[3] = '3';
	const offsets latest_at(bytes);
	set<uint32_t>(bytes, latest_at.first_branch + 16, count);
	bytes.insert(bytes.begin() + latest_at.first_branch + 20, 5 * count, 0);
	historical_nqr1_writer tail;
	tail.bytes.clear();
	tail.number<uint32_t>(0); // Empty pending child is legal beside retained latest.
	tail.number<uint32_t>(0); // Original completed branch.
	tail.number<uint64_t>(7);
	tail.blob(std::vector<uint8_t>(blob_size, 0)); // Deliberately malformed real byte input.
	const auto bounded_leaf = latest_child_leaf();
	tail.blob(encode(bounded_leaf.command, bounded_leaf.value));
	bytes.insert(bytes.end(), tail.bytes.begin(), tail.bytes.end());
	assert(bytes.size() < limit && base + count * goal_charge + blob_size > limit);
	child_allocations = 0;
	observe_child_allocations = true;
	reject_decode(latest_parent.command, bytes, latest_parent.value);
	observe_child_allocations = false;
	assert(child_allocations == 0); // Charge refuses before latest-command vector assign.
}
}

int main()
{
	allocator_observer_calibration();
	forests_program_and_binding();
	shapes_aliases_and_bounds();
	receipt_and_publication();
	exact_child_frames();
	child_combined_retained_budget();
	std::puts(
		"native quest recovery context structural transport, original forests/program, bounds, receipt equivalence and strong output checks passed");
}
