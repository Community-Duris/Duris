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

critical_command build(const item_transfer_payload &payload)
{
	critical_command result{};
	assert(item_transfer_command_build_native_mobile_recovery(
		&result, id(6), payload, critical_source_site::command,
		critical_deadline_class::interactive));
	assert(result.payload_version == 12);
	assert(!critical_command_legacy_execution_supported(result));
	assert(!critical_command_valid(result));
	assert(!item_transfer_accounting_command_supported(result));
	assert(result.keys.size() <= CRITICAL_COMMAND_MAX_KEYS);
	assert(result.payload.size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES);
	return result;
}

void roundtrip_and_original_literal_cut()
{
	auto payload = acceptance();
	critical_command old{};
	assert(item_transfer_command_build_native_mobile(&old, id(6), payload,
							 critical_source_site::command,
							 critical_deadline_class::interactive));
	const auto original = encode({ item(900, PLAYER_SNAPSHOT_NO_PARENT, 43),
				       item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 1),
				       item(800, PLAYER_SNAPSHOT_NO_PARENT) });
	assert(item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, original));
	assert((payload.native_recovery.player_before.ordered_item_uids ==
		std::vector<uint64_t>{ 900, 100, 101, 800 }));
	assert((payload.native_recovery.player_after.ordered_item_uids ==
		std::vector<uint64_t>{ 900, 800 }));
	assert(shop_trade_recovery_forest_verify(encode({ item(900, PLAYER_SNAPSHOT_NO_PARENT, 43),
							  item(800, PLAYER_SNAPSHOT_NO_PARENT) }),
						 shop_trade_recovery_forest_role::player_after,
						 payload.native_recovery.player_after));
	const auto command = build(payload);
	assert(command.payload.size() == old.payload.size() +
						 ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MIN_BYTES +
						 6 * sizeof(uint64_t));
	assert(std::equal(old.payload.begin(), old.payload.end(), command.payload.begin()));
	item_transfer_payload decoded{};
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.native_recovery.player_before == payload.native_recovery.player_before);
	assert(decoded.native_recovery.player_after == payload.native_recovery.player_after);
	assert(decoded.native_recovery.player_pid == 10 &&
	       decoded.native_recovery.acknowledged_save_revision == 23);
	std::vector<uint8_t> bytes;
	assert(item_transfer_command_encode_native_mobile_recovery(decoded, &bytes));
	assert(bytes == command.payload);
	assert(!item_transfer_command_encode_native_mobile(payload, &bytes));
	assert(!item_transfer_command_encode_payload(payload, &bytes));

	const auto before_binding = payload.native_recovery.player_before;
	const auto after_binding = payload.native_recovery.player_after;
	auto altered = original;
	altered.pop_back();
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, altered));
	assert(payload.native_recovery.player_before == before_binding &&
	       payload.native_recovery.player_after == after_binding);
	auto literal = std::vector{ item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 0) };
	literal[1].timers[0]++;
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, encode(literal)));
	assert(payload.native_recovery.player_before == before_binding);
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 11, 23, original));
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 10, 0, original));

	std::vector<uint8_t> intent;
	assert(item_native_mobile_accounting_intent(command, id(7), id(8), 10, nullptr, &intent) ==
	       economic_accounting_error::ok);
	economic_frozen_intent frozen;
	assert(economic_intent_decode(intent, &frozen) == economic_accounting_error::ok);
	assert(economic_intent_verify_binding(command, frozen) == economic_accounting_error::ok);
	auto changed = payload;
	changed.native_recovery.acknowledged_save_revision++;
	assert(economic_intent_verify_binding(build(changed), frozen) ==
	       economic_accounting_error::payload_conflict);
	changed = payload;
	changed.native_recovery.player_before.canonical_digest[0] ^= 1;
	assert(economic_intent_verify_binding(build(changed), frozen) ==
	       economic_accounting_error::payload_conflict);
	economic_source_event event{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	assert(item_native_mobile_accounting_intent(command, id(7), id(8), 10, &event, &intent) ==
	       economic_accounting_error::unauthorized);
}

void shape_and_wire_refusals()
{
	auto payload = acceptance();
	const auto original = encode({ item(900, PLAYER_SNAPSHOT_NO_PARENT),
				       item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 1) });
	assert(item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, original));
	const auto command = build(payload);
	const size_t tail = command.payload.size() -
			    ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MIN_BYTES - 4 * sizeof(uint64_t);
	for (const size_t offset : { size_t{ 0 }, size_t{ 2 }, size_t{ 4 }, size_t{ 8 },
				     size_t{ 12 }, size_t{ 24 }, size_t{ 25 }, size_t{ 26 } })
	{
		auto malformed = command;
		malformed.payload[tail + offset] ^= 0xff;
		item_transfer_payload unchanged = payload;
		assert(!item_transfer_command_decode_payload(malformed, &unchanged));
		assert(unchanged.native_recovery.acknowledged_save_revision == 23);
	}
	auto zero_save = command;
	std::fill_n(zero_save.payload.begin() + tail + 16, 8, 0);
	item_transfer_payload zero_decoded{};
	assert(!item_transfer_command_decode_payload(zero_save, &zero_decoded));
	for (const size_t size : { tail, tail + 23, tail + 63, command.payload.size() - 1 })
	{
		auto malformed = command;
		malformed.payload.resize(size);
		item_transfer_payload decoded{};
		assert(!item_transfer_command_decode_payload(malformed, &decoded));
	}
	auto trailing = command;
	trailing.payload.push_back(0);
	item_transfer_payload decoded{};
	assert(!item_transfer_command_decode_payload(trailing, &decoded));
	auto wrong_version = command;
	wrong_version.payload_version = 11;
	assert(!item_transfer_command_decode_payload(wrong_version, &decoded));

	auto changed = payload;
	changed.native_recovery.player_before.ordered_item_uids[0] = 100;
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	changed = payload;
	changed.native_recovery.player_after.ordered_item_uids[0] = 101;
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	changed = payload;
	changed.native_recovery.player_after = {};
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	changed = payload;
	changed.native_recovery.player_before.ordered_item_uids.resize(4097);
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	for (uint64_t invalid_uid : { uint64_t{ 0 }, UINT64_MAX })
	{
		changed = payload;
		changed.native_recovery.player_before.ordered_item_uids[0] = invalid_uid;
		assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	}
	changed = payload;
	changed.native_recovery.player_before.canonical_digest = {};
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));
	changed = payload;
	changed.native_recovery.player_before.canonical_bytes = PLAYER_SNAPSHOT_MAX_BYTES + 1;
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));

	// Present empty AFTER is distinct from absent and is derived from real bytes.
	payload = acceptance();
	assert(item_transfer_native_mobile_recovery_freeze(
		&payload, 10, 23, encode({ item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 0) })));
	assert(payload.native_recovery.player_after.present &&
	       payload.native_recovery.player_after.canonical_bytes == 4);
	assert(payload.native_recovery.player_after.ordered_item_uids.empty());
	build(payload);

	std::vector<player_item_snapshot> full;
	full.push_back(item(100, PLAYER_SNAPSHOT_NO_PARENT));
	full.push_back(item(101, 0));
	for (uint64_t index = 2; index < 4096; ++index)
		full.push_back(item(10000 + index, PLAYER_SNAPSHOT_NO_PARENT));
	payload = acceptance();
	assert(item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, encode(full)));
	assert(payload.native_recovery.player_before.ordered_item_uids.size() == 4096);
	build(payload);
	full.push_back(item(50000, PLAYER_SNAPSHOT_NO_PARENT));
	std::vector<uint8_t> over;
	assert(player_item_snapshot_list_encode(full, &over) != player_snapshot_codec_result::ok);
}

void consumption_and_existing_frame_identity()
{
	auto payload = acceptance();
	payload.from_owner = payload.to_owner;
	payload.to_owner = { item_owner_type::destruction, 0, 0 };
	payload.reason = item_transfer_reason::quest_turnin;
	payload.multi_root = true;
	payload.selected_item_uid = payload.target_root_item_uid = 0;
	payload.native_mobile.action = item_native_mobile_action::consumption;
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, encode({})));
	const std::array<uint64_t, 1> original_order{ 100 };
	assert(!item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, {}));
	assert(item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, {}, original_order));
	assert(!payload.native_recovery.player_before.present &&
	       !payload.native_recovery.player_after.present);
	const auto command = build(payload);
	item_transfer_payload decoded{};
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(!decoded.native_recovery.player_before.present &&
	       !decoded.native_recovery.player_after.present &&
	       decoded.native_recovery.acknowledged_save_revision == 23);
	const auto giver = std::find_if(
		command.keys.begin(), command.keys.end(), [](const critical_entity_key &key)
		{ return key.type == critical_entity_type::player && key.id == 10; });
	assert(giver != command.keys.end());
	const auto revision = std::find_if(
		command.expected_revisions.begin(), command.expected_revisions.end(),
		[](const critical_expected_revision &entry)
		{ return entry.key.type == critical_entity_type::player && entry.key.id == 10; });
	assert(revision != command.expected_revisions.end() && revision->revision == 0);
	auto missing_giver = command;
	missing_giver.keys.erase(missing_giver.keys.begin() + (giver - command.keys.begin()));
	missing_giver.expected_revisions.erase(missing_giver.expected_revisions.begin() +
					       (revision - command.expected_revisions.begin()));
	assert(!item_transfer_command_decode_payload(missing_giver, &decoded));
	std::vector<uint8_t> intent;
	assert(item_native_mobile_accounting_intent(command, id(7), id(8), 10, nullptr, &intent) ==
	       economic_accounting_error::unauthorized);
	economic_source_event event{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	assert(item_native_mobile_accounting_intent(command, id(7), id(8), 10, &event, &intent) ==
	       economic_accounting_error::ok);
	auto changed = payload;
	assert(shop_trade_recovery_forest_freeze(encode({}),
						 shop_trade_recovery_forest_role::player_before,
						 &changed.native_recovery.player_before));
	assert(!item_transfer_native_mobile_recovery_shape_valid(changed));

	shop_trade_recovery_manifest manifest;
	const auto empty = encode({});
	assert(shop_trade_recovery_forest_freeze(
		empty, shop_trade_recovery_forest_role::player_before, &manifest.player_before));
	assert(shop_trade_recovery_forest_freeze(
		empty, shop_trade_recovery_forest_role::player_after, &manifest.player_after));
	assert(shop_trade_recovery_forest_freeze(
		empty, shop_trade_recovery_forest_role::keeper_before, &manifest.keeper_before));
	assert(shop_trade_recovery_forest_freeze(
		empty, shop_trade_recovery_forest_role::keeper_after, &manifest.keeper_after));
	std::vector<uint8_t> full, frame;
	assert(shop_trade_recovery_manifest_encode(manifest, &full));
	assert(shop_trade_recovery_forest_encode(
		manifest.player_before, shop_trade_recovery_forest_role::player_before, &frame));
	assert(std::equal(frame.begin(), frame.end(),
			  full.begin() + SHOP_TRADE_RECOVERY_MANIFEST_HEADER_BYTES));
	shop_trade_recovery_forest_binding binding;
	assert(shop_trade_recovery_forest_decode(
		frame, shop_trade_recovery_forest_role::player_before, &binding));
	assert(binding == manifest.player_before);
	assert(!shop_trade_recovery_forest_decode(
		frame, shop_trade_recovery_forest_role::player_after, &binding));
	frame.push_back(0);
	assert(!shop_trade_recovery_forest_decode(
		frame, shop_trade_recovery_forest_role::player_before, &binding));
}

// A failed original ITEM/TYPE prefix may differ from the native forest order.
// Its order is retained even when no reward continuation exists.
item_transfer_payload two_root_consumption()
{
	auto payload = acceptance();
	payload.from_owner = payload.to_owner;
	payload.to_owner = { item_owner_type::destruction, 0, 0 };
	payload.reason = item_transfer_reason::quest_turnin;
	payload.multi_root = true;
	payload.selected_item_uid = payload.target_root_item_uid = 0;
	payload.native_mobile.action = item_native_mobile_action::consumption;
	payload.item_count = 3;
	payload.items[2] = { 200, 200, 0, 4, 9001, item_custody_state::active };
	const auto literal = encode({ item(100, PLAYER_SNAPSHOT_NO_PARENT), item(101, 0),
				      item(200, PLAYER_SNAPSHOT_NO_PARENT) });
	payload.item_blob_size = static_cast<uint32_t>(literal.size());
	std::copy(literal.begin(), literal.end(), payload.item_blob.begin());
	return payload;
}

std::vector<uint8_t> original_completion(const std::array<uint64_t, 2> &roots)
{
	std::vector<uint8_t> result;
	const auto u32 = [&](uint32_t value)
	{
		for (size_t i = 0; i < 4; ++i)
			result.push_back(static_cast<uint8_t>(value >> (8 * i)));
	};
	const auto u64 = [&](uint64_t value)
	{
		for (size_t i = 0; i < 8; ++i)
			result.push_back(static_cast<uint8_t>(value >> (8 * i)));
	};
	u32(5);
	u32(10);
	u32(9001);
	u32(0);
	u32(9001);
	u32(1234);
	u64(1);
	u32(2);
	u64(roots[0]);
	u64(roots[1]);
	u32(0);
	u32(1);
	u32(30);
	u32(1);
	u32(1);
	u32(30);
	u32(1);
	u32(10);
	const std::string name = "Synthetic", definition = "fixture:ordered-quest";
	u32(static_cast<uint32_t>(name.size()));
	result.insert(result.end(), name.begin(), name.end());
	u32(static_cast<uint32_t>(definition.size()));
	result.insert(result.end(), definition.begin(), definition.end());
	u32(0);
	return result;
}

void ordered_failed_prefix_and_message_binding()
{
	const std::array<uint64_t, 2> original_order{ 200, 100 }, sorted_order{ 100, 200 };
	auto prefix = two_root_consumption();
	assert(item_transfer_native_mobile_recovery_freeze(&prefix, 10, 23, {}, original_order));
	const auto command = build(prefix);
	item_transfer_payload decoded{};
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert((decoded.native_recovery.consumed_root_order == std::vector<uint64_t>{ 200, 100 }));
	assert(decoded.continuation.kind == item_transfer_continuation_kind::none);
	economic_source_event source{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	std::vector<uint8_t> intent;
	assert(item_native_mobile_accounting_intent(command, id(7), id(8), 10, &source, &intent) ==
	       economic_accounting_error::ok);
	economic_frozen_intent frozen;
	assert(economic_intent_decode(intent, &frozen) == economic_accounting_error::ok);
	auto reordered = two_root_consumption();
	assert(item_transfer_native_mobile_recovery_freeze(&reordered, 10, 23, {}, sorted_order));
	assert(economic_intent_verify_binding(build(reordered), frozen) ==
	       economic_accounting_error::payload_conflict);
	for (const auto &bad : std::vector<std::vector<uint64_t>>{ {},
								   { 200 },
								   { 200, 200 },
								   { 200, 101 },
								   { 200, 999 },
								   { 200, 0 },
								   { 200, UINT64_MAX },
								   { 200, 100, 999 } })
	{
		auto rejected = prefix;
		assert(!item_transfer_native_mobile_recovery_freeze(&rejected, 10, 23, {}, bad));
		assert(rejected.native_recovery.consumed_root_order ==
		       prefix.native_recovery.consumed_root_order);
	}
	item_native_quest_publication_terms prefix_terms;
	prefix_terms.message = "The original preliminary quest message.";
	prefix_terms.echo_all = true;
	auto notified_prefix = two_root_consumption();
	assert(item_transfer_native_mobile_recovery_freeze(&notified_prefix, 10, 23, {},
							   original_order, prefix_terms));
	const auto notified_command = build(notified_prefix);
	assert(item_transfer_command_decode_payload(notified_command, &decoded));
	assert(decoded.continuation.kind == item_transfer_continuation_kind::none);
	assert(decoded.native_recovery.publication_terms.message == prefix_terms.message);
	assert(decoded.native_recovery.publication_terms.echo_all);
	assert(!decoded.native_recovery.publication_terms.disappear &&
	       decoded.native_recovery.publication_terms.disappear_message.empty());
	std::vector<uint8_t> notified_intent;
	assert(item_native_mobile_accounting_intent(notified_command, id(7), id(8), 10, &source,
						    &notified_intent) ==
	       economic_accounting_error::ok);
	economic_frozen_intent notified_frozen;
	assert(economic_intent_decode(notified_intent, &notified_frozen) ==
	       economic_accounting_error::ok);
	auto changed_message = two_root_consumption();
	auto changed_terms = prefix_terms;
	changed_terms.message = "A changed message cannot replace the original.";
	assert(item_transfer_native_mobile_recovery_freeze(&changed_message, 10, 23, {},
							   original_order, changed_terms));
	assert(economic_intent_verify_binding(build(changed_message), notified_frozen) ==
	       economic_accounting_error::payload_conflict);
	for (unsigned int fault = 0; fault < 3; ++fault)
	{
		auto invalid_terms = prefix_terms;
		if (fault == 0)
			invalid_terms.disappear = true;
		else if (fault == 1)
			invalid_terms.disappear_message = "No retirement on a failed prefix.";
		else
			invalid_terms.message.push_back('\0');
		auto rejected = notified_prefix;
		assert(!item_transfer_native_mobile_recovery_freeze(&rejected, 10, 23, {},
								    original_order, invalid_terms));
		assert(rejected.native_recovery.publication_terms.message == prefix_terms.message &&
		       rejected.native_recovery.publication_terms.echo_all &&
		       !rejected.native_recovery.publication_terms.disappear &&
		       rejected.native_recovery.publication_terms.disappear_message.empty());
		assert(rejected.native_recovery.consumed_root_order ==
		       notified_prefix.native_recovery.consumed_root_order);
	}

	auto completed = two_root_consumption();
	completed.continuation.kind = item_transfer_continuation_kind::quest_offering;
	completed.continuation.data = original_completion(original_order);
	item_native_quest_publication_terms terms;
	terms.message = "The original quest is complete.";
	terms.disappear_message = "The original giver disappears.";
	terms.echo_all = true;
	terms.disappear = true;
	assert(item_transfer_native_mobile_recovery_freeze(&completed, 10, 23, {}, original_order,
							   terms));
	const auto completed_command = build(completed);
	assert(item_transfer_command_decode_payload(completed_command, &decoded));
	assert(decoded.native_recovery.publication_terms.message == terms.message);
	assert(decoded.native_recovery.publication_terms.disappear_message ==
	       terms.disappear_message);
	assert(decoded.native_recovery.publication_terms.echo_all &&
	       decoded.native_recovery.publication_terms.disappear);
	source.kind = economic_source_kind::quest_completion;
	assert(item_native_mobile_accounting_intent(completed_command, id(7), id(8), 10, &source,
						    &intent) == economic_accounting_error::ok);
	assert(economic_intent_decode(intent, &frozen) == economic_accounting_error::ok);
	for (size_t field = 0; field < 4; ++field)
	{
		auto changed = completed;
		auto &changed_terms = changed.native_recovery.publication_terms;
		if (field == 0)
			changed_terms.message = "Today's mutable catalog message";
		if (field == 1)
			changed_terms.disappear_message = "Today's mutable disappearance";
		if (field == 2)
			changed_terms.echo_all = false;
		if (field == 3)
			changed_terms.disappear = false;
		assert(economic_intent_verify_binding(build(changed), frozen) ==
		       economic_accounting_error::payload_conflict);
	}
	auto mismatched = completed;
	mismatched.continuation.data = original_completion(sorted_order);
	assert(!item_transfer_native_mobile_recovery_freeze(&mismatched, 10, 23, {}, original_order,
							    terms));
	for (bool disappearance : { false, true })
	{
		auto invalid = terms;
		auto &value = disappearance ? invalid.disappear_message : invalid.message;
		value = std::string("before\0after", 12);
		auto changed = completed;
		assert(!item_transfer_native_mobile_recovery_freeze(&changed, 10, 23, {},
								    original_order, invalid));
		value.assign(ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES, 'x');
		assert(!item_transfer_native_mobile_recovery_freeze(&changed, 10, 23, {},
								    original_order, invalid));
		value.pop_back();
		assert(item_transfer_native_mobile_recovery_freeze(&changed, 10, 23, {},
								   original_order, invalid));
		build(changed);
	}
	const size_t text_bytes = terms.message.size() + terms.disappear_message.size();
	const size_t publication_offset = completed_command.payload.size() - text_bytes -
					  ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES;
	auto unknown_flags = completed_command;
	unknown_flags.payload[publication_offset] |= 4;
	assert(!item_transfer_command_decode_payload(unknown_flags, &decoded));
	auto embedded_nul = completed_command;
	embedded_nul.payload[publication_offset +
			     ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES] = 0;
	assert(!item_transfer_command_decode_payload(embedded_nul, &decoded));
	auto truncated = completed_command;
	truncated.payload.pop_back();
	assert(!item_transfer_command_decode_payload(truncated, &decoded));
}

} // namespace

int main()
{
	roundtrip_and_original_literal_cut();
	shape_and_wire_refusals();
	consumption_and_existing_frame_identity();
	ordered_failed_prefix_and_message_binding();
}
