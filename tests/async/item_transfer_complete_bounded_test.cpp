// Pure-value complete Item codec regression. No native lifetime, admission, gameplay,
// persistence, checkpoint, or release authority is established by these fixtures.
#include "item/item_transfer_command.h"
#include "item/lockpick_retirement_continuation.h"
#include "player/player_snapshot_codec.h"
#include "economy/native_quest_cost.h"
#include "core/defines.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include <cstring>
#include "world/quest_mobile_native_reference.h"
#include <vector>
#if !defined(__SANITIZE_ADDRESS__)
#include "economy/item_transfer_accounting.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cassert>
#include <utility>

// Synthetic pure values only: no native lifetime/source/checkpoint authority.
namespace
{
critical_operation_id id(uint8_t value)
{
	critical_operation_id result{};
	result.bytes[0] = value;
	return result;
}

player_item_snapshot item(uint64_t uid, int32_t parent, int16_t slot = 0)
{
	player_item_snapshot result{};
	result.object_uid = uid;
	result.parent_index = parent;
	result.equipment_slot = slot;
	result.vnum = 9001;
	result.string_mask = 15;
	result.name = "synthetic retained item";
	result.short_description = "a synthetic retained item";
	result.description = "A synthetic retained item is here.";
	result.timers[0] = 23;
	result.condition = 17;
	return result;
}

std::vector<uint8_t> encode(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> result;
	assert(player_item_snapshot_list_encode(items, &result) ==
	       player_snapshot_codec_result::ok);
	return result;
}

item_transfer_payload acceptance()
{
	item_transfer_payload result{};
	result.from_owner = { item_owner_type::player, 10, 0 };
	result.to_owner = { item_owner_type::native_mobile, 9000, 0 };
	result.reason = item_transfer_reason::quest_offering;
	result.reason_id = 9001;
	result.expected_from_revision = 3;
	result.expected_to_revision = 5;
	result.selected_item_uid = result.target_root_item_uid = 100;
	result.item_count = 2;
	result.items[0] = { 100, 100, 0, 2, 9001, item_custody_state::active };
	result.items[1] = { 101, 100, 100, 3, 9001, item_custody_state::active };
	const auto bytes = encode({ item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 0) });
	result.item_blob_size = static_cast<uint32_t>(bytes.size());
	std::copy(bytes.begin(), bytes.end(), result.item_blob.begin());
	result.native_mobile.present = true;
	result.native_mobile.action = item_native_mobile_action::acceptance;
	result.native_mobile.final_giver_pid = 10;
	auto &reference = result.native_mobile.reference;
	reference.mobile_instance_id = 9000;
	reference.birth_operation = id(1);
	reference.birth_source = { economic_source_kind::npc_generation, id(2), id(3), 4, 5 };
	reference.mobile_vnum = 9001;
	reference.reset_zone_vnum = 1;
	reference.provenance = quest_mobile_birth_provenance::reset;
	reference.mobile_revision = 5;
	reference.stock_revision = 7;
	return result;
}

}

#endif

#if !defined(__SANITIZE_ADDRESS__)
namespace
{
struct reservation_trace
{
	size_t calls = 0, fail_at = 0, largest = 0;
	bool refused = false;
};
bool reserve(size_t requested, void *context) noexcept
{
	auto &trace = *static_cast<reservation_trace *>(context);
	++trace.calls;
	trace.largest = std::max(trace.largest, requested);
	if (trace.fail_at && trace.calls == trace.fail_at)
	{
		trace.refused = true;
		return false;
	}
	return true;
}
size_t cases = 0, successful_reservations = 0, refusal_points = 0;
template <class Call, class Stable> void sweep(Call call, Stable stable)
{
	reservation_trace success;
	const bool baseline = call(success);
	if (!baseline)
	{
		const char *const names[] = { "lockpick.valid",
					      "generic.valid",
					      "generic.encode",
					      "generic.decode",
					      "public.decode",
					      "native.acceptance.encode",
					      "native.acceptance.decode",
					      "native.recovery.encode",
					      "native.recovery.decode",
					      "native.consumption.encode",
					      "native.consumption.decode",
					      "nqf1.13.encode",
					      "nqf1.13.decode",
					      "nqf1.14.encode",
					      "nqf1.14.decode" };
		using getter = bool (*)(size_t *) noexcept;
		const getter getters[] = {
			lockpick_retirement_payload_valid_complete_source_frame_bytes,
			player_item_snapshot_list_decode_bounded_source_frame_bytes,
			item_transfer_payload_valid_complete_source_frame_bytes,
			item_transfer_payload_encode_version_complete_source_frame_bytes,
			item_transfer_payload_decode_generic_complete_source_frame_bytes,
			item_transfer_command_decode_payload_complete_source_frame_bytes
		};
		std::fprintf(
			stderr,
			"QUALIFICATION_SWEEP_FAILURE ordinal=%zu name=%s calls=%zu largest=%zu refused=%d\n",
			cases + 1, cases < 15 ? names[cases] : "out-of-inventory", success.calls,
			success.largest, success.refused ? 1 : 0);
		for (size_t index = 0; index < 6; ++index)
		{
			size_t source = SIZE_MAX;
			const bool supported = getters[index](&source);
			std::fprintf(
				stderr,
				"QUALIFICATION_SWEEP_SOURCE index=%zu supported=%d source=%zu\n",
				index, supported ? 1 : 0, source);
		}
		std::fflush(stderr);
	}
	assert(baseline);
	assert(success.calls && !success.refused);
	successful_reservations += success.calls;
	for (size_t nth = 1; nth <= success.calls; ++nth)
	{
		reservation_trace refused;
		refused.fail_at = nth;
		assert(!call(refused));
		assert(refused.refused && refused.calls >= nth);
		stable();
		++refusal_points;
	}
	++cases;
}
item_transfer_payload sentinel()
{
	item_transfer_payload value{};
	value.reason_id = 123456;
	value.corpse.owner_name = "prior caller-owned string";
	value.continuation.data = { 1, 2, 3, 4 };
	value.collector.eligible_item_uids = { 77, 88 };
	return value;
}
void unchanged(const item_transfer_payload &value)
{
	assert(value.reason_id == 123456 && value.corpse.owner_name == "prior caller-owned string");
	assert((value.continuation.data == std::vector<uint8_t>{ 1, 2, 3, 4 }));
	assert((value.collector.eligible_item_uids == std::vector<uint64_t>{ 77, 88 }));
}
item_transfer_payload held_pick()
{
	item_transfer_payload payload{};
	payload.from_owner = { item_owner_type::player, 42, 0 };
	payload.to_owner = { item_owner_type::destruction, 0, 0 };
	payload.reason = item_transfer_reason::destruction;
	payload.reason_id = 9001;
	payload.selected_item_uid = 100;
	payload.item_count = 1;
	payload.expected_from_revision = 3;
	payload.items[0] = { 100, 100, 0, 2, 9001, item_custody_state::active };
	auto pick = item(100, PLAYER_SNAPSHOT_NO_PARENT, HOLD + 1);
	pick.type = ITEM_PICK;
	const auto bytes = encode({ pick });
	assert(bytes.size() <= payload.item_blob.size());
	payload.item_blob_size = static_cast<uint32_t>(bytes.size());
	std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
	payload.continuation.kind = item_transfer_continuation_kind::lockpick_retirement;
	const lockpick_retirement_terms terms{ lockpick_retirement_branch::wear, 100, 42, 9001 };
	assert(lockpick_retirement_encode(terms, &payload.continuation.data));
	return payload;
}
// Check all caller object bytes, including nested container capacity, after refusal.
template <class Value, class Call> bool preserving_output_on_refusal(Value &output, Call call)
{
	std::array<unsigned char, sizeof(Value)> before{};
	std::memcpy(before.data(), &output, before.size());
	const bool ok = call();
	if (!ok)
		assert(std::memcmp(before.data(), &output, before.size()) == 0);
	return ok;
}
void generic_and_lockpick()
{
	const auto value = held_pick();
	reservation_trace reference;
	assert(lockpick_retirement_payload_valid(value));
	sweep(
		[&](reservation_trace &trace) {
			return lockpick_retirement_payload_valid_complete_bounded(value, reserve,
										  &trace, 4096);
		},
		[] {});
	assert(!lockpick_retirement_payload_valid_complete_bounded(value, nullptr, nullptr, 4096));
	auto changed = value;
	changed.items[0].vnum++;
	assert(!lockpick_retirement_payload_valid_complete_bounded(changed, reserve, &reference,
								   4096));
	auto literal = item(100, PLAYER_SNAPSHOT_NO_PARENT, HOLD + 1);
	literal.type = ITEM_PICK;
	literal.equipment_slot = 0;
	const auto wrong = encode({ literal });
	changed = value;
	changed.item_blob_size = static_cast<uint32_t>(wrong.size());
	std::copy(wrong.begin(), wrong.end(), changed.item_blob.begin());
	assert(!lockpick_retirement_payload_valid_complete_bounded(changed, reserve, &reference,
								   4096));
	changed = value;
	changed.continuation.data.pop_back();
	assert(!lockpick_retirement_payload_valid_complete_bounded(changed, reserve, &reference,
								   4096));
	sweep(
		[&](reservation_trace &trace)
		{
			return item_transfer_payload_valid_complete_bounded(
				value, ITEM_TRANSFER_PAYLOAD_VERSION, reserve, &trace, 4096);
		},
		[] {});
	std::vector<uint8_t> encoded, prior{ 0xa1, 0xb2 };
	size_t retained = 12345;
	sweep(
		[&](reservation_trace &trace)
		{
			encoded = prior;
			retained = 12345;
			const bool ok = preserving_output_on_refusal(
				encoded,
				[&]
				{
					return item_transfer_payload_encode_version_complete_bounded(
						value, ITEM_TRANSFER_PAYLOAD_VERSION, &encoded,
						reserve, &trace, 4096, &retained);
				});
			if (!ok)
			{
				assert(encoded == prior);
				assert(retained == 12345);
			}
			return ok;
		},
		[&] { assert(encoded == prior && retained == 12345); });
	critical_command command{};
	assert(item_transfer_command_build(&command, id(33), value, critical_source_site::command,
					   critical_deadline_class::interactive));
	// Test the real builder output: no manually edited positive payload.
	reference = {};
	assert(item_transfer_payload_encode_version_complete_bounded(
		value, ITEM_TRANSFER_PAYLOAD_VERSION, &encoded, reserve, &reference, 4096,
		&retained));
	assert(encoded == command.payload);
	std::vector<uint8_t> original_bounded;
	assert(item_transfer_payload_encode_version_bounded(value, ITEM_TRANSFER_PAYLOAD_VERSION,
							    &original_bounded, reserve, &reference,
							    4096));
	assert(original_bounded == command.payload);
	constexpr size_t target_root_offset = 72; // Original generic wire field.
	assert(std::all_of(command.payload.begin() + target_root_offset,
			   command.payload.begin() + target_root_offset + sizeof(uint64_t),
			   [](uint8_t byte) { return byte == 0; }));
	item_transfer_payload legacy{};
	assert(item_transfer_command_decode_payload(command, &legacy));
	assert(lockpick_retirement_payload_valid(legacy));
	item_transfer_payload output = sentinel();
	sweep(
		[&](reservation_trace &trace)
		{
			output = sentinel();
			retained = 12345;
			const bool ok = preserving_output_on_refusal(
				output,
				[&]
				{
					return item_transfer_payload_decode_generic_complete_bounded(
						command, &output, reserve, &trace, 4096, &retained);
				});
			if (ok)
			{
				assert(lockpick_retirement_payload_valid(output));
				assert(output.target_root_item_uid == 0);
			}
			return ok;
		},
		[&]
		{
			unchanged(output);
			assert(retained == 12345);
		});
	sweep(
		[&](reservation_trace &trace)
		{
			output = sentinel();
			const bool ok = preserving_output_on_refusal(
				output,
				[&]
				{
					return item_transfer_command_decode_payload_complete_bounded(
						command, &output, reserve, &trace, 4096);
				});
			if (ok)
			{
				assert(lockpick_retirement_payload_valid(output));
				assert(output.target_root_item_uid == 0);
			}
			return ok;
		},
		[&] { unchanged(output); });
	for (size_t length = 0; length < command.payload.size(); ++length)
	{
		auto truncated = command;
		truncated.payload.resize(length);
		output = sentinel();
		retained = 12345;
		assert(!preserving_output_on_refusal(
			output,
			[&]
			{
				return item_transfer_payload_decode_generic_complete_bounded(
					truncated, &output, reserve, &reference, 4096, &retained);
			}));
		assert(retained == 12345);
		assert(!preserving_output_on_refusal(
			output,
			[&]
			{
				return item_transfer_command_decode_payload_complete_bounded(
					truncated, &output, reserve, &reference, 4096);
			}));
		unchanged(output);
	}
	for (const uint64_t wrong_root_uid : std::array<uint64_t, 2>{ 100, 999 })
	{
		auto wrong_root = command;
		for (size_t index = 0; index < sizeof(uint64_t); ++index)
			wrong_root.payload[target_root_offset + index] =
				static_cast<uint8_t>(wrong_root_uid >> (8 * index));
		output = sentinel();
		assert(!item_transfer_command_decode_payload(wrong_root, &output));
		output = sentinel();
		retained = 12345;
		assert(!preserving_output_on_refusal(
			output,
			[&]
			{
				return item_transfer_payload_decode_generic_complete_bounded(
					wrong_root, &output, reserve, &reference, 4096, &retained);
			}));
		assert(retained == 12345);
		assert(!preserving_output_on_refusal(
			output,
			[&]
			{
				return item_transfer_command_decode_payload_complete_bounded(
					wrong_root, &output, reserve, &reference, 4096);
			}));
		unchanged(output);
	}
	encoded = prior;
	retained = 12345;
	assert(!item_transfer_payload_encode_version_complete_bounded(
		value, ITEM_TRANSFER_PAYLOAD_VERSION, &encoded, reserve, &reference, SIZE_MAX,
		&retained));
	assert(encoded == prior && retained == 12345);
	assert(!item_transfer_payload_valid_complete_bounded(value, ITEM_TRANSFER_PAYLOAD_VERSION,
							     reserve, &reference, SIZE_MAX));
	assert(!lockpick_retirement_payload_valid_complete_bounded(value, reserve, &reference,
								   SIZE_MAX));
	changed = value;
	changed.item_blob_size = UINT32_MAX;
	assert(!item_transfer_payload_valid_complete_bounded(changed, ITEM_TRANSFER_PAYLOAD_VERSION,
							     reserve, &reference, 4096));
	encoded = prior;
	retained = 12345;
	assert(!item_transfer_payload_encode_version_complete_bounded(
		changed, ITEM_TRANSFER_PAYLOAD_VERSION, &encoded, reserve, &reference, 4096,
		&retained));
	assert(encoded == prior && retained == 12345);
	assert(!item_transfer_payload_valid_complete_bounded(value, ITEM_TRANSFER_PAYLOAD_VERSION,
							     nullptr, nullptr, 4096));
	reference = {};
	output = sentinel();
	retained = 12345;
	assert(!preserving_output_on_refusal(
		output,
		[&]
		{
			return item_transfer_payload_decode_generic_complete_bounded(
				command, &output, reserve, &reference, SIZE_MAX, &retained);
		}));
	assert(retained == 12345);
	assert(!preserving_output_on_refusal(
		output,
		[&]
		{
			return item_transfer_command_decode_payload_complete_bounded(
				command, &output, reserve, &reference, SIZE_MAX);
		}));
	unchanged(output);
}
void native_roundtrip(item_transfer_payload payload, bool recovery)
{
	reservation_trace trace;
	std::vector<uint8_t> golden;
	assert(recovery ? item_transfer_command_encode_native_mobile_recovery(payload, &golden) :
			  item_transfer_command_encode_native_mobile(payload, &golden));
	std::vector<uint8_t> result, prior{ 0xa1, 0xb2 };
	sweep(
		[&](reservation_trace &probe)
		{
			result = prior;
			const bool ok = preserving_output_on_refusal(
				result,
				[&]
				{
					return recovery ?
						       item_transfer_command_encode_native_mobile_recovery_complete_bounded(
							       payload, &result, reserve, &probe,
							       4096) :
						       item_transfer_command_encode_native_mobile_complete_bounded(
							       payload, &result, reserve, &probe,
							       4096);
				});
			if (ok)
				assert(result == golden);
			else
				assert(result == prior);
			return ok;
		},
		[&] { assert(result == prior); });
	critical_command command{};
	assert(recovery ? item_transfer_command_build_native_mobile_recovery(
				  &command, id(33), payload, critical_source_site::command,
				  critical_deadline_class::interactive) :
			  item_transfer_command_build_native_mobile(
				  &command, id(33), payload, critical_source_site::command,
				  critical_deadline_class::interactive));
	item_transfer_payload output = sentinel();
	sweep(
		[&](reservation_trace &probe)
		{
			output = sentinel();
			return preserving_output_on_refusal(
				output,
				[&]
				{
					return item_transfer_command_decode_payload_complete_bounded(
						command, &output, reserve, &probe, 4096);
				});
		},
		[&] { unchanged(output); });
	output = {};
	assert(item_transfer_command_decode_payload_complete_bounded(command, &output, reserve,
								     &trace, 4096));
	assert(output.item_count == payload.item_count &&
	       output.native_cost.present == payload.native_cost.present);
	std::vector<uint8_t> decoded_wire;
	assert(recovery ?
		       item_transfer_command_encode_native_mobile_recovery(output, &decoded_wire) :
		       item_transfer_command_encode_native_mobile(output, &decoded_wire));
	assert(decoded_wire == golden);
	item_transfer_payload legacy{};
	assert(item_transfer_command_decode_payload(command, &legacy));
	std::vector<uint8_t> legacy_wire;
	assert(recovery ?
		       item_transfer_command_encode_native_mobile_recovery(legacy, &legacy_wire) :
		       item_transfer_command_encode_native_mobile(legacy, &legacy_wire));
	assert(legacy_wire == golden);
}
void native_and_nqf1()
{
	auto payload = acceptance();
	native_roundtrip(payload, false);
	assert(item_transfer_native_mobile_recovery_freeze(
		&payload, 10, 23, encode({ item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 0) })));
	native_roundtrip(payload, true);
	// Same real consumption transformation used by the original native harness.
	payload = acceptance();
	payload.from_owner = payload.to_owner;
	payload.to_owner = { item_owner_type::destruction, 0, 0 };
	payload.reason = item_transfer_reason::quest_turnin;
	payload.multi_root = true;
	payload.selected_item_uid = payload.target_root_item_uid = 0;
	payload.native_mobile.action = item_native_mobile_action::consumption;
	native_roundtrip(payload, false);
	payload.native_cost.present = true;
	payload.native_cost.wallet_mapping_id = 17;
	const std::array<native_quest_cost_requirement, 1> requirements{ { { 0, 1 } } };
	assert(native_quest_cost_project({ 100, 0, 0, 0 }, 31, requirements,
					 &payload.native_cost.projection) ==
	       native_quest_cost_projection_result::ok);
	native_roundtrip(payload, false); // Original NQF1 version13 -> version11.
	const std::array<uint64_t, 1> ordered{ 100 };
	assert(item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, {}, ordered));
	native_roundtrip(payload, true); // Original NQF1 version14 -> version12.
}
}
#endif
#if defined(__SANITIZE_ADDRESS__)
namespace
{
size_t reservations = 0;
bool admit(size_t, void *) noexcept
{
	++reservations;
	return true;
}
}

int sanitizer_refusal()
{
	// Same valid shop-buy shape as the original compatibility harness's first call.
	item_transfer_payload input{};
	input.from_owner = { item_owner_type::shopkeeper, item_shopkeeper_owner_id(7), 0 };
	input.to_owner = { item_owner_type::player, 42, 0 };
	input.reason = item_transfer_reason::shop_buy;
	input.reason_id = 7;
	input.expected_from_revision = 3;
	input.expected_to_revision = 9;
	input.selected_item_uid = 100;
	input.target_root_item_uid = 100;
	input.item_count = 1;
	input.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	input.item_blob_size = 3;
	input.item_blob[0] = 0x12;
	input.item_blob[1] = 0x34;
	input.item_blob[2] = 0x56;
	critical_operation_id operation{};
	operation.bytes[0] = 0xa5;
	operation.bytes.back() = 0x67;
	critical_command command{};
	assert(item_transfer_command_build(&command, operation, input,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	item_transfer_payload legacy{};
	assert(item_transfer_command_decode_payload(command, &legacy));
	assert(legacy.reason == input.reason && legacy.item_blob_size == 3 &&
	       legacy.item_blob[2] == 0x56);

	using getter = bool (*)(size_t *) noexcept;
	const getter getters[] = {
		quest_mobile_native_reference_encode_source_frame_bytes,
		item_transfer_payload_valid_complete_source_frame_bytes,
		item_transfer_payload_encode_version_complete_source_frame_bytes,
		item_transfer_payload_decode_generic_complete_source_frame_bytes,
		item_transfer_command_decode_payload_complete_source_frame_bytes
	};
	for (auto get : getters)
	{
		size_t value = 45678;
		assert(!get(&value));
		assert(value == 45678);
	}

	item_transfer_payload output{};
	output.reason_id = 12345;
	output.continuation.data = { 0x98, 0x76 };
	output.corpse.owner_name = "prior owner";
	std::array<unsigned char, sizeof(item_transfer_payload)> before{};
	std::memcpy(before.data(), &output, before.size());
	std::vector<uint8_t> bytes{ 0xab, 0xcd };
	size_t heap = 34567;
	assert(!item_transfer_payload_valid_complete_bounded(legacy, command.payload_version, admit,
							     nullptr, 0));
	assert(!item_transfer_payload_encode_version_complete_bounded(
		legacy, command.payload_version, &bytes, admit, nullptr, 0, &heap));
	assert(!item_transfer_payload_decode_generic_complete_bounded(command, &output, admit,
								      nullptr, 0, &heap));
	assert(!item_transfer_command_decode_payload_complete_bounded(command, &output, admit,
								      nullptr, 0));
	assert(output.reason_id == 12345 && output.corpse.owner_name == "prior owner");
	assert((output.continuation.data == std::vector<uint8_t>{ 0x98, 0x76 }));
	assert((bytes == std::vector<uint8_t>{ 0xab, 0xcd }));
	assert(heap == 34567);
	assert(std::memcmp(before.data(), &output, before.size()) == 0);
	// Unlike compiled-out fallback profiles, genuine preentry may call admit.
	std::printf(
		"ITEM_TRANSFER_COMPLETE_SANITIZER_REFUSAL {\"legacy_success\":true,\"source_refusal\":true,"
		"\"no_writes\":true,\"reservations\":%zu}\n",
		reservations);
	return 0;
}

#endif
int main()
{
#if defined(__SANITIZE_ADDRESS__)
	return sanitizer_refusal();
#else
	generic_and_lockpick();
	native_and_nqf1();
	assert(cases == 15 && refusal_points && successful_reservations);
	std::printf(
		"ITEM_TRANSFER_COMPLETE_CASES {\"successful_cases\":%zu,\"refusal_points\":%zu,\"reservations\":%zu,"
		"\"lockpick_success\":true,\"malformed_binding\":true,\"nqf1_13_14\":true}\n",
		cases, refusal_points, successful_reservations);
#endif
}
