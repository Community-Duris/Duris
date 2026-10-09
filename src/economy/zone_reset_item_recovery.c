#include "economy/zone_reset_item_recovery.h"
#include "item/item_transfer_command.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{
using error = economic_accounting_error;
constexpr uint32_t MAGIC = 0x3152525a; // ZRR1, little endian.
constexpr size_t HEADER_BYTES = 12;
constexpr size_t RECEIPT_BYTES = 97 + CRITICAL_COMPLETION_RESULT_MAX_BYTES;
constexpr size_t STATE_BYTES = 8;
constexpr size_t BODY_BYTES = STATE_BYTES + RECEIPT_BYTES + 4;
constexpr size_t ITEM_BYTES = 20;
constexpr size_t LIMIT = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;
constexpr size_t MAX_ITEMS = CRITICAL_COMMAND_MAX_KEYS - 2;

void put(uint8_t *output, uint64_t value, size_t bytes) noexcept
{
	for (size_t i = 0; i < bytes; ++i)
		output[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(const uint8_t *input, size_t bytes) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < bytes; ++i)
		value |= static_cast<uint64_t>(input[i]) << (8 * i);
	return value;
}
uint8_t bits(const zone_reset_item_recovery_action &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0));
}
uint8_t bits(const zone_reset_item_recovery_effect &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0) | (a.periodic ? 8 : 0));
}
zone_reset_item_recovery_action action(uint8_t value) noexcept
{
	return { bool(value & 1), bool(value & 2), bool(value & 4) };
}
zone_reset_item_recovery_effect effect(uint8_t value) noexcept
{
	return { bool(value & 1), bool(value & 2), bool(value & 4), bool(value & 8) };
}
bool valid_action(const zone_reset_item_recovery_action &a) noexcept
{
	return (!a.returned || a.started) && (!a.succeeded || a.returned);
}
bool complete(const zone_reset_item_recovery_action &a) noexcept
{
	return a.started && a.returned && a.succeeded;
}
bool completion_equal(const critical_completion &a, const critical_completion &b) noexcept
{
	return a.operation_id.bytes == b.operation_id.bytes && a.outcome == b.outcome &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.attempt == b.attempt && a.queued_at_usec == b.queued_at_usec &&
	       a.started_at_usec == b.started_at_usec &&
	       a.completed_at_usec == b.completed_at_usec && a.failure_stage == b.failure_stage &&
	       a.result_size == b.result_size && a.result_payload == b.result_payload &&
	       a.recovery_correlation == b.recovery_correlation && a.disposition == b.disposition;
}
bool successful(const critical_completion &receipt) noexcept
{
	return receipt.outcome == critical_apply_outcome::applied ||
	       receipt.outcome == critical_apply_outcome::already_applied;
}
bool receipt_core_equal(const critical_completion &a, const critical_completion &b) noexcept
{
	return successful(a) && successful(b) && a.operation_id.bytes == b.operation_id.bytes &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload &&
	       a.disposition == critical_completion_disposition::execution &&
	       b.disposition == critical_completion_disposition::execution;
}
bool receipt_values_shape(bool present, const critical_completion &receipt) noexcept
{
	if (!present)
		return completion_equal(receipt, critical_completion{});
	if (receipt.disposition != critical_completion_disposition::execution ||
	    receipt.outcome > critical_apply_outcome::terminal_failure ||
	    !critical_failure_stage_valid(receipt.failure_stage) ||
	    receipt.result_size > CRITICAL_COMPLETION_RESULT_MAX_BYTES)
		return false;
	for (size_t i = receipt.result_size; i < receipt.result_payload.size(); ++i)
		if (receipt.result_payload[i])
			return false;
	return !successful(receipt) ||
	       (!receipt.error_code && receipt.failure_stage == critical_failure_stage::none &&
		receipt.result_size == ITEM_TRANSFER_RESULT_BYTES);
}
bool receipt_shape(const critical_command &command, bool present,
		   const critical_completion &receipt) noexcept
{
	return (!present || receipt.operation_id.bytes == command.operation_id.bytes) &&
	       receipt_values_shape(present, receipt);
}
bool receipt_result_valid(const critical_command &command, const zone_reset_item_image &image,
			  bool present, const critical_completion &receipt)
{
	if (!receipt_shape(command, present, receipt))
		return false;
	if (!present || !successful(receipt))
		return true;
	if (image.items.empty() || image.items.size() > UINT16_MAX ||
	    image.expected_room_revision == UINT64_MAX ||
	    receipt.durable_revision != image.expected_room_revision + 1)
		return false;
	item_transfer_result expected{};
	expected.root_item_uid = image.items.front().object_uid;
	expected.item_count = static_cast<uint16_t>(image.items.size());
	expected.to_owner_revision = image.expected_room_revision + 1;
	expected.max_item_revision = 1;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical{};
	return item_transfer_command_encode_result(expected, &canonical) &&
	       std::equal(canonical.begin(), canonical.end(), receipt.result_payload.begin());
}
void write_receipt(uint8_t *output, const critical_completion &r) noexcept
{
	std::copy(r.operation_id.bytes.begin(), r.operation_id.bytes.end(), output);
	output[16] = static_cast<uint8_t>(r.outcome);
	output[17] = static_cast<uint8_t>(r.disposition);
	put(output + 18, static_cast<uint16_t>(r.failure_stage), 2);
	put(output + 20, r.durable_revision, 8);
	put(output + 28, r.error_code, 4);
	put(output + 32, r.attempt, 4);
	put(output + 36, r.queued_at_usec, 8);
	put(output + 44, r.started_at_usec, 8);
	put(output + 52, r.completed_at_usec, 8);
	put(output + 60, r.result_size, 2);
	std::memcpy(output + 64, r.recovery_correlation.data(), r.recovery_correlation.size());
	std::copy(r.result_payload.begin(), r.result_payload.end(), output + 97);
}
critical_completion read_receipt(const uint8_t *input) noexcept
{
	critical_completion r{};
	std::copy_n(input, r.operation_id.bytes.size(), r.operation_id.bytes.begin());
	r.outcome = static_cast<critical_apply_outcome>(input[16]);
	r.disposition = static_cast<critical_completion_disposition>(input[17]);
	r.failure_stage = static_cast<critical_failure_stage>(get(input + 18, 2));
	r.durable_revision = get(input + 20, 8);
	r.error_code = static_cast<unsigned int>(get(input + 28, 4));
	r.attempt = static_cast<unsigned int>(get(input + 32, 4));
	r.queued_at_usec = get(input + 36, 8);
	r.started_at_usec = get(input + 44, 8);
	r.completed_at_usec = get(input + 52, 8);
	r.result_size = static_cast<uint16_t>(get(input + 60, 2));
	std::memcpy(r.recovery_correlation.data(), input + 64, r.recovery_correlation.size());
	std::copy_n(input + 97, r.result_payload.size(), r.result_payload.begin());
	return r;
}
size_t step_count(const native_mobile_birth_item_recipe &recipe) noexcept
{
	return 2 * recipe.libraries.size() + 3;
}
bool periodic_at(const native_mobile_birth_item_recipe &recipe, size_t index) noexcept
{
	const size_t general = 2 * recipe.libraries.size();
	return index < general ? !(index & 1) && recipe.libraries[index / 2].periodic_requested :
				 index == general && recipe.general_periodic;
}
template <typename GetEffect> bool effects_valid(const native_mobile_birth_item_recipe &recipe,
						 uint32_t next_step, bool current_started,
						 size_t count, GetEffect get_effect) noexcept
{
	if (count != step_count(recipe) || next_step > count)
		return false;
	for (size_t i = 0; i < count; ++i)
	{
		const auto e = get_effect(i);
		if ((e.returned && !e.started) || (e.succeeded && !e.returned) ||
		    e.periodic != (e.returned && periodic_at(recipe, i)))
			return false;
		if (i < next_step)
		{
			if (!e.started || !e.returned || !e.succeeded)
				return false;
		}
		else if (i == next_step)
		{
			if (current_started != e.started || (e.returned && e.succeeded))
				return false;
		}
		else if (bits(e))
			return false;
	}
	return next_step != count || !current_started;
}
bool item_done_fields(const zone_reset_item_recovery_item &item, size_t count) noexcept
{
	return item.admitted && item.published && !item.current_step_started &&
	       item.next_step == count;
}
bool item_done(const zone_reset_item_recovery_item &item) noexcept
{
	return item_done_fields(item, item.effects.size());
}
bool body_terminal(const zone_reset_item_recovery_context &v) noexcept
{
	return v.receipt_present && successful(v.receipt) &&
	       v.stage == zone_reset_item_recovery_stage::physically_proven &&
	       complete(v.whole_binding) && complete(v.batch_publication) &&
	       complete(v.room_placement) && v.runtime_applied &&
	       std::all_of(v.items.begin(), v.items.end(), item_done);
}
bool no_progress(const zone_reset_item_recovery_context &v) noexcept
{
	if (v.stage != zone_reset_item_recovery_stage::captured || bits(v.whole_binding) ||
	    bits(v.batch_publication) || bits(v.room_placement) || v.runtime_applied)
		return false;
	for (const auto &item : v.items)
	{
		if (item.next_step || item.current_step_started || item.admitted || item.published)
			return false;
		for (const auto &e : item.effects)
			if (bits(e))
				return false;
	}
	return true;
}
template <typename GetItem, typename GetCount, typename GetEffect>
bool context_valid_range(const critical_command &command, const zone_reset_item_image &image,
			 const zone_reset_item_recovery_context &v, size_t count, GetItem get_item,
			 GetCount get_count, GetEffect get_effect)
{
	if (v.stage > zone_reset_item_recovery_stage::physically_proven ||
	    count != image.recipes.size() || count > MAX_ITEMS || !valid_action(v.whole_binding) ||
	    !valid_action(v.batch_publication) || !valid_action(v.room_placement) ||
	    !receipt_result_valid(command, image, v.receipt_present, v.receipt))
		return false;
	const bool must_zero = !v.receipt_present || !successful(v.receipt) ||
			       v.stage == zone_reset_item_recovery_stage::captured;
	if (must_zero &&
	    (v.stage != zone_reset_item_recovery_stage::captured || bits(v.whole_binding) ||
	     bits(v.batch_publication) || bits(v.room_placement) || v.runtime_applied))
		return false;
	if (v.batch_publication.started && !complete(v.whole_binding))
		return false;
	bool all_admitted = true, previous_done = true, all_done = true;
	for (size_t i = 0; i < count; ++i)
	{
		const auto &item = get_item(i);
		const size_t effects = get_count(i);
		// Exact complete command order: no missing item, UID substitution or reorder.
		if (item.object_uid != image.recipes[i].object_uid ||
		    !effects_valid(image.recipes[i], item.next_step, item.current_step_started,
				   effects, [&](size_t e) { return get_effect(i, e); }) ||
		    (must_zero && (item.next_step || item.current_step_started || item.admitted ||
				   item.published)) ||
		    item.published != complete(v.batch_publication) ||
		    ((item.next_step || item.current_step_started) &&
		     (!complete(v.batch_publication) || !previous_done)))
			return false;
		all_admitted = all_admitted && item.admitted;
		previous_done = item_done_fields(item, effects);
		all_done = all_done && previous_done;
	}
	if ((v.whole_binding.started && !all_admitted) ||
	    (v.room_placement.started && (!complete(v.batch_publication) || !all_done)) ||
	    (v.runtime_applied && !complete(v.room_placement)))
		return false;
	return v.stage != zone_reset_item_recovery_stage::physically_proven ||
	       (v.receipt_present && successful(v.receipt) && complete(v.whole_binding) &&
		complete(v.batch_publication) && complete(v.room_placement) && v.runtime_applied &&
		all_done);
}
bool context_valid(const critical_command &command, const zone_reset_item_image &image,
		   const zone_reset_item_recovery_context &v)
{
	return context_valid_range(
		command, image, v, v.items.size(),
		[&](size_t i) -> const zone_reset_item_recovery_item & { return v.items[i]; },
		[&](size_t i) { return v.items[i].effects.size(); },
		[&](size_t i, size_t e) { return v.items[i].effects[e]; });
}
bool command_values(const critical_command &command, std::vector<uint8_t> *canonical,
		    zone_reset_item_image *image)
{
	return critical_command_encode(command, canonical) == critical_command_codec_result::ok &&
	       canonical->size() <= CRITICAL_COMMAND_MAX_ENCODED_BYTES &&
	       zone_reset_item_command_decode(command, image) == error::ok;
}
struct wire_view
{
	std::span<const uint8_t> command;
	const uint8_t *body = nullptr;
	size_t items_offset = 0;
	uint32_t count = 0;
	std::array<size_t, MAX_ITEMS> item_offsets{};
};
// Bound the complete packet and retained attachment containers before any decoder allocates.
error preflight(const critical_command *command, std::span<const uint8_t> bytes,
		wire_view *view) noexcept
{
	if (!view || bytes.size() > LIMIT)
		return error::capacity;
	if (bytes.size() < HEADER_BYTES + BODY_BYTES || get(bytes.data(), 4) != MAGIC ||
	    get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_RECOVERY_VERSION)
		return error::invalid_version;
	const size_t command_size = static_cast<uint32_t>(get(bytes.data() + 8, 4));
	if (get(bytes.data() + 6, 2) || !command_size ||
	    command_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    command_size > bytes.size() - HEADER_BYTES - BODY_BYTES)
		return error::corrupt_evidence;
	wire_view v;
	v.command = bytes.subspan(HEADER_BYTES, command_size);
	v.body = bytes.data() + HEADER_BYTES + command_size;
	const auto *body = v.body;
	if (body[0] > 1 || body[1] > 2 || body[2] > 7 || body[3] > 7 || body[4] > 7 ||
	    body[5] > 1 || get(body + 6, 2) || get(body + STATE_BYTES + 62, 2) ||
	    !valid_action(action(body[2])) || !valid_action(action(body[3])) ||
	    !valid_action(action(body[4])))
		return error::corrupt_evidence;
	const auto receipt = read_receipt(body + STATE_BYTES);
	if (!(command ? receipt_shape(*command, body[0] != 0, receipt) :
			receipt_values_shape(body[0] != 0, receipt)))
		return error::corrupt_evidence;
	v.count = static_cast<uint32_t>(get(body + STATE_BYTES + RECEIPT_BYTES, 4));
	v.items_offset = HEADER_BYTES + command_size + BODY_BYTES;
	size_t offset = v.items_offset;
	if (v.count > MAX_ITEMS || v.count > (bytes.size() - offset) / ITEM_BYTES)
		return error::corrupt_evidence;
	size_t retained = sizeof(zone_reset_item_recovery_context) + command_size;
	if (retained > LIMIT ||
	    v.count > (LIMIT - retained) / sizeof(zone_reset_item_recovery_item))
		return error::capacity;
	retained += v.count * sizeof(zone_reset_item_recovery_item);
	for (size_t i = 0; i < v.count; ++i)
	{
		v.item_offsets[i] = offset;
		if (bytes.size() - offset < ITEM_BYTES)
			return error::corrupt_evidence;
		const auto *item = bytes.data() + offset;
		const size_t effects = static_cast<uint32_t>(get(item + 16, 4));
		if (item[12] > 7 || get(item + 13, 3) ||
		    effects > 2 * PLAYER_SNAPSHOT_MAX_ROWS + 3 || get(item + 8, 4) > effects)
			return error::corrupt_evidence;
		offset += ITEM_BYTES;
		const size_t remaining = v.count - i - 1;
		if (remaining > (bytes.size() - offset) / ITEM_BYTES ||
		    effects > bytes.size() - offset - remaining * ITEM_BYTES ||
		    effects > (LIMIT - retained) / sizeof(zone_reset_item_recovery_effect))
			return error::capacity;
		retained += effects * sizeof(zone_reset_item_recovery_effect);
		for (size_t e = 0; e < effects; ++e)
		{
			const auto decoded = effect(bytes[offset + e]);
			if (bytes[offset + e] > 15 ||
			    !valid_action(
				    { decoded.started, decoded.returned, decoded.succeeded }) ||
			    (decoded.periodic && !decoded.returned))
				return error::corrupt_evidence;
		}
		offset += effects;
	}
	if (offset != bytes.size())
		return error::corrupt_evidence;
	*view = v;
	return error::ok;
}
void read_body(const uint8_t *body, zone_reset_item_recovery_context *v) noexcept
{
	v->receipt_present = body[0] != 0;
	v->stage = static_cast<zone_reset_item_recovery_stage>(body[1]);
	v->whole_binding = action(body[2]);
	v->batch_publication = action(body[3]);
	v->room_placement = action(body[4]);
	v->runtime_applied = body[5] != 0;
	v->receipt = read_receipt(body + STATE_BYTES);
}
zone_reset_item_recovery_item read_item(const uint8_t *item) noexcept
{
	zone_reset_item_recovery_item v;
	v.object_uid = get(item, 8);
	v.next_step = static_cast<uint32_t>(get(item + 8, 4));
	v.current_step_started = (item[12] & 1) != 0;
	v.admitted = (item[12] & 2) != 0;
	v.published = (item[12] & 4) != 0;
	return v;
}
bool successor_action(const zone_reset_item_recovery_action &a,
		      const zone_reset_item_recovery_action &b) noexcept
{
	if (a == b)
		return true;
	if (!a.started)
		return b.started && !b.returned && !b.succeeded;
	if (!a.returned)
		return b.started && b.returned;
	return false;
}
bool successor_effect(const zone_reset_item_recovery_effect &a,
		      const zone_reset_item_recovery_effect &b) noexcept
{
	if (a == b)
		return true;
	return successor_action({ a.started, a.returned, a.succeeded },
				{ b.started, b.returned, b.succeeded }) &&
	       (!a.returned || a.periodic == b.periodic);
}
bool successor_context(const zone_reset_item_recovery_context &a,
		       const zone_reset_item_recovery_context &b) noexcept
{
	if ((a.receipt_present && !b.receipt_present) ||
	    (a.receipt_present && !completion_equal(a.receipt, b.receipt) &&
	     !receipt_core_equal(a.receipt, b.receipt)) ||
	    b.stage < a.stage ||
	    static_cast<unsigned>(b.stage) > static_cast<unsigned>(a.stage) + 1 ||
	    (a.runtime_applied && !b.runtime_applied) ||
	    !successor_action(a.whole_binding, b.whole_binding) ||
	    !successor_action(a.batch_publication, b.batch_publication) ||
	    !successor_action(a.room_placement, b.room_placement) ||
	    a.items.size() != b.items.size())
		return false;
	for (size_t i = 0; i < a.items.size(); ++i)
	{
		const auto &left = a.items[i];
		const auto &right = b.items[i];
		if (left.object_uid != right.object_uid || right.next_step < left.next_step ||
		    right.next_step > static_cast<uint64_t>(left.next_step) + 1 ||
		    (left.admitted && !right.admitted) || (left.published && !right.published) ||
		    left.effects.size() != right.effects.size())
			return false;
		for (size_t step = 0; step < left.effects.size(); ++step)
			if (!successor_effect(left.effects[step], right.effects[step]))
				return false;
		// Advancing the cursor settles exactly the already-started original step.
		// No started latch may disappear without that exact returned-success step.
		if ((right.next_step != left.next_step &&
		     (!left.current_step_started || right.current_step_started)) ||
		    (left.current_step_started && !right.current_step_started &&
		     right.next_step != static_cast<uint64_t>(left.next_step) + 1))
			return false;
	}
	return true;
}
bool envelope_decode(const critical_native_recovery_envelope &envelope,
		     zone_reset_item_recovery_context *context) noexcept
{
	return envelope.revision &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		envelope.phase == critical_native_recovery_phase::continuation_pending) &&
	       zone_reset_item_recovery_decode(envelope.command, envelope.attachment, context) ==
		       error::ok &&
	       (envelope.revision != 1 ||
		(envelope.phase == critical_native_recovery_phase::execution_pending &&
		 !context->receipt_present && no_progress(*context))) &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		body_terminal(*context));
}
} // namespace

economic_accounting_error zone_reset_item_recovery_encode(const critical_command &command,
							  const zone_reset_item_recovery_context &v,
							  std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	try
	{
		std::vector<uint8_t> canonical;
		zone_reset_item_image image;
		if (!command_values(command, &canonical, &image) ||
		    !context_valid(command, image, v))
			return error::corrupt_evidence;
		size_t size = HEADER_BYTES + canonical.size() + BODY_BYTES;
		size_t retained = sizeof(zone_reset_item_recovery_context) + canonical.size();
		if (size > LIMIT || retained > LIMIT ||
		    v.items.size() > (LIMIT - retained) / sizeof(zone_reset_item_recovery_item))
			return error::capacity;
		retained += v.items.size() * sizeof(zone_reset_item_recovery_item);
		for (const auto &item : v.items)
		{
			if (size > LIMIT - ITEM_BYTES ||
			    item.effects.size() > LIMIT - size - ITEM_BYTES ||
			    item.effects.size() >
				    (LIMIT - retained) / sizeof(zone_reset_item_recovery_effect))
				return error::capacity;
			size += ITEM_BYTES + item.effects.size();
			retained += item.effects.size() * sizeof(zone_reset_item_recovery_effect);
		}
		std::vector<uint8_t> bytes(size, 0);
		put(bytes.data(), MAGIC, 4);
		put(bytes.data() + 4, ZONE_RESET_ITEM_RECOVERY_VERSION, 2);
		put(bytes.data() + 8, canonical.size(), 4);
		std::copy(canonical.begin(), canonical.end(), bytes.begin() + HEADER_BYTES);
		auto *body = bytes.data() + HEADER_BYTES + canonical.size();
		body[0] = v.receipt_present ? 1 : 0;
		body[1] = static_cast<uint8_t>(v.stage);
		body[2] = bits(v.whole_binding);
		body[3] = bits(v.batch_publication);
		body[4] = bits(v.room_placement);
		body[5] = v.runtime_applied ? 1 : 0;
		write_receipt(body + STATE_BYTES, v.receipt);
		put(body + STATE_BYTES + RECEIPT_BYTES, v.items.size(), 4);
		size_t offset = HEADER_BYTES + canonical.size() + BODY_BYTES;
		for (const auto &item : v.items)
		{
			auto *encoded = bytes.data() + offset;
			put(encoded, item.object_uid, 8);
			put(encoded + 8, item.next_step, 4);
			encoded[12] = static_cast<uint8_t>((item.current_step_started ? 1 : 0) |
							   (item.admitted ? 2 : 0) |
							   (item.published ? 4 : 0));
			put(encoded + 16, item.effects.size(), 4);
			offset += ITEM_BYTES;
			for (const auto &e : item.effects)
				bytes[offset++] = bits(e);
		}
		static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
		*output = std::move(bytes);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
economic_accounting_error
zone_reset_item_recovery_decode(const critical_command &command, std::span<const uint8_t> bytes,
				zone_reset_item_recovery_context *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	const auto checked = preflight(&command, bytes, &view);
	if (checked != error::ok)
		return checked;
	try
	{
		std::vector<uint8_t> canonical;
		zone_reset_item_image image;
		if (!command_values(command, &canonical, &image) ||
		    !std::equal(canonical.begin(), canonical.end(), view.command.begin(),
				view.command.end()) ||
		    view.count != image.recipes.size())
			return error::payload_conflict;
		zone_reset_item_recovery_context candidate;
		read_body(view.body, &candidate);
		if (!context_valid_range(
			    command, image, candidate, view.count, [&](size_t i)
			    { return read_item(bytes.data() + view.item_offsets[i]); },
			    [&](size_t i) {
				    return static_cast<size_t>(
					    get(bytes.data() + view.item_offsets[i] + 16, 4));
			    },
			    [&](size_t i, size_t e)
			    { return effect(bytes[view.item_offsets[i] + ITEM_BYTES + e]); }))
			return error::corrupt_evidence;
		// Validate complete semantics and recipe/count correlation before containers allocate.
		candidate.items.reserve(view.count);
		for (size_t i = 0; i < view.count; ++i)
		{
			const size_t offset = view.item_offsets[i];
			auto item = read_item(bytes.data() + offset);
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset + ITEM_BYTES + e]));
			candidate.items.push_back(std::move(item));
		}
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_recovery_context>);
		*output = std::move(candidate);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
economic_accounting_error
zone_reset_item_recovery_original_command_decode(std::span<const uint8_t> bytes,
						 critical_command *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	const auto checked = preflight(nullptr, bytes, &view);
	if (checked != error::ok)
		return checked;
	try
	{
		critical_command original{};
		const auto decoded = critical_command_decode(view.command.data(),
							     view.command.size(), &original);
		if (decoded != critical_command_codec_result::ok)
			return decoded == critical_command_codec_result::overflow ?
				       error::capacity :
			       decoded == critical_command_codec_result::unsupported_version ?
				       error::invalid_version :
				       error::corrupt_evidence;
		zone_reset_item_recovery_context context;
		const auto valid = zone_reset_item_recovery_decode(original, bytes, &context);
		if (valid != error::ok)
			return valid;
		if (!body_terminal(context))
			return error::unresolved;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*output = std::move(original);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
bool zone_reset_item_recovery_valid(const critical_native_recovery_envelope &envelope) noexcept
{
	zone_reset_item_recovery_context v;
	return envelope_decode(envelope, &v);
}
namespace
{
[[maybe_unused]] bool recovery_bound_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
[[maybe_unused]] bool recovery_bound_array(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && recovery_bound_add(bytes, count * width);
}
[[maybe_unused]] bool recovery_image_heap(const zone_reset_item_image &image,
					 size_t *output) noexcept
{
	size_t bytes = 0;
	const auto text = [&](const std::string &value)
	{
		// Supported fresh decoder strings use the pinned local capacity 15.
		// Local characters already belong to the row's inline object storage.
		return value.capacity() <= 15 ||
		       (value.capacity() != SIZE_MAX &&
			recovery_bound_add(bytes, value.capacity() + 1));
	};
	if (!recovery_bound_array(bytes, image.items.capacity(), sizeof(player_item_snapshot)) ||
	    !recovery_bound_array(bytes, image.recipes.capacity(),
				  sizeof(native_mobile_birth_item_recipe)) ||
	    !recovery_bound_array(bytes, image.coins.capacity(), sizeof(zone_reset_coin_output)))
		return false;
	for (const auto &item : image.items)
	{
		if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description) ||
		    !recovery_bound_array(bytes, item.dynamic_affects.capacity(),
					  sizeof(player_item_dynamic_affect_snapshot)) ||
		    !recovery_bound_array(bytes, item.extra_descriptions.capacity(),
					  sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : item.extra_descriptions)
			if (!text(description.keyword) || !text(description.description) ||
			    !recovery_bound_array(bytes, description.spell_ids.capacity(), sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : image.recipes)
		if (!recovery_bound_array(bytes, recipe.libraries.capacity(),
					  sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}
[[maybe_unused]] size_t recovery_validation_objects() noexcept
{
	// Receipt validation, and the later per-item/effect validation, are separate
	// phases. read_item's local and returned value may coexist without NRVO.
	return std::max({ sizeof(critical_completion),
		sizeof(item_transfer_result) + sizeof(std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES>),
		2 * sizeof(zone_reset_item_recovery_item) +
			2 * sizeof(zone_reset_item_recovery_effect) +
			sizeof(zone_reset_item_recovery_action) });
}
[[maybe_unused]] size_t recovery_preflight_objects() noexcept
{
	// Caller view is separate. preflight's local view survives its receipt/row
	// checks; read_receipt can retain its local and returned completion objects.
	return sizeof(wire_view) + std::max(
		2 * sizeof(critical_completion),
		2 * sizeof(zone_reset_item_recovery_effect) +
			2 * sizeof(zone_reset_item_recovery_action));
}
[[maybe_unused]] error recovery_canonical_bounded(const critical_command &command,
	std::vector<uint8_t> *canonical, zone_reset_item_image *image,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t live,
	size_t *retained) noexcept
{
	const auto encoded = critical_command_encode_bounded(command, canonical, reserve, context, live);
	if (encoded != critical_command_codec_result::ok)
		return encoded == critical_command_codec_result::overflow ||
			encoded == critical_command_codec_result::unsupported_version ?
				error::capacity : error::corrupt_evidence;
	if (!recovery_bound_add(live, canonical->capacity()))
		return error::capacity;
	const auto decoded = zone_reset_item_command_decode_bounded(command, image,
		reserve, context, live);
	if (decoded != error::ok)
		return decoded == error::capacity ? error::capacity : error::corrupt_evidence;
	size_t image_heap = 0;
	if (!recovery_image_heap(*image, &image_heap) || !recovery_bound_add(live, image_heap))
		return error::capacity;
	*retained = live;
	return error::ok;
}
} // namespace

economic_accounting_error zone_reset_item_recovery_encode_bounded(
	const critical_command &command, const zone_reset_item_recovery_context &v,
	std::vector<uint8_t> *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!reserve)
		return error::capacity;
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)v;
	(void)context;
	(void)outer_live;
	return error::capacity;
#else
	try
	{
		size_t live = outer_live;
		if (!recovery_bound_add(live, sizeof(std::vector<uint8_t>)) ||
		    !recovery_bound_add(live, sizeof(zone_reset_item_image)) || !reserve(live, context))
			return error::capacity;
		std::vector<uint8_t> canonical;
		zone_reset_item_image image;
		auto status = recovery_canonical_bounded(command, &canonical, &image,
			reserve, context, live, &live);
		if (status != error::ok)
			return status;
		size_t validation = live;
		if (!recovery_bound_add(validation, recovery_validation_objects()) ||
		    !reserve(validation, context))
			return error::capacity;
		if (!context_valid(command, image, v))
			return error::corrupt_evidence;
		size_t size = HEADER_BYTES + canonical.size() + BODY_BYTES;
		size_t retained = sizeof(zone_reset_item_recovery_context) + canonical.size();
		if (size > LIMIT || retained > LIMIT ||
		    v.items.size() > (LIMIT - retained) / sizeof(zone_reset_item_recovery_item))
			return error::capacity;
		retained += v.items.size() * sizeof(zone_reset_item_recovery_item);
		for (const auto &item : v.items)
		{
			if (size > LIMIT - ITEM_BYTES || item.effects.size() > LIMIT - size - ITEM_BYTES ||
			    item.effects.size() > (LIMIT - retained) / sizeof(zone_reset_item_recovery_effect))
				return error::capacity;
			size += ITEM_BYTES + item.effects.size();
			retained += item.effects.size() * sizeof(zone_reset_item_recovery_effect);
		}
		// Fresh vector(size,0) requests exactly size under the supported policy.
		if (!recovery_bound_add(live, sizeof(std::vector<uint8_t>)) ||
		    !recovery_bound_add(live, size) || !reserve(live, context))
			return error::capacity;
		std::vector<uint8_t> bytes(size, 0);
		put(bytes.data(), MAGIC, 4);
		put(bytes.data() + 4, ZONE_RESET_ITEM_RECOVERY_VERSION, 2);
		put(bytes.data() + 8, canonical.size(), 4);
		std::copy(canonical.begin(), canonical.end(), bytes.begin() + HEADER_BYTES);
		auto *body = bytes.data() + HEADER_BYTES + canonical.size();
		body[0] = v.receipt_present ? 1 : 0;
		body[1] = static_cast<uint8_t>(v.stage);
		body[2] = bits(v.whole_binding);
		body[3] = bits(v.batch_publication);
		body[4] = bits(v.room_placement);
		body[5] = v.runtime_applied ? 1 : 0;
		write_receipt(body + STATE_BYTES, v.receipt);
		put(body + STATE_BYTES + RECEIPT_BYTES, v.items.size(), 4);
		size_t offset = HEADER_BYTES + canonical.size() + BODY_BYTES;
		for (const auto &item : v.items)
		{
			auto *encoded = bytes.data() + offset;
			put(encoded, item.object_uid, 8);
			put(encoded + 8, item.next_step, 4);
			encoded[12] = static_cast<uint8_t>((item.current_step_started ? 1 : 0) |
							   (item.admitted ? 2 : 0) |
							   (item.published ? 4 : 0));
			put(encoded + 16, item.effects.size(), 4);
			offset += ITEM_BYTES;
			for (const auto &e : item.effects)
				bytes[offset++] = bits(e);
		}
		*output = std::move(bytes);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
#endif
}

economic_accounting_error zone_reset_item_recovery_decode_bounded(
	const critical_command &command, std::span<const uint8_t> bytes,
	zone_reset_item_recovery_context *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live, size_t *retained_context_heap_bytes) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!reserve)
		return error::capacity;
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)bytes;
	(void)context;
	(void)outer_live;
	(void)retained_context_heap_bytes;
	return error::capacity;
#else
	try
	{
		size_t live = outer_live;
		if (!recovery_bound_add(live, sizeof(wire_view)) ||
		    !recovery_bound_add(live, sizeof(std::vector<uint8_t>)) ||
		    !recovery_bound_add(live, sizeof(zone_reset_item_image)) ||
		    !recovery_bound_add(live, sizeof(zone_reset_item_recovery_context)))
			return error::capacity;
		size_t preliminary = live;
		if (!recovery_bound_add(preliminary, recovery_preflight_objects()) ||
		    !reserve(preliminary, context))
			return error::capacity;
		wire_view view;
		std::vector<uint8_t> canonical;
		zone_reset_item_image image;
		zone_reset_item_recovery_context candidate;
		const auto checked = preflight(&command, bytes, &view);
		if (checked != error::ok)
			return checked;
		auto status = recovery_canonical_bounded(command, &canonical, &image,
			reserve, context, live, &live);
		if (status != error::ok)
			return status == error::capacity ? error::capacity : error::payload_conflict;
		if (!std::equal(canonical.begin(), canonical.end(), view.command.begin(),
				view.command.end()) || view.count != image.recipes.size())
			return error::payload_conflict;
		size_t validation = live;
		if (!recovery_bound_add(validation, std::max(recovery_validation_objects(),
			2 * sizeof(critical_completion))) || !reserve(validation, context))
			return error::capacity;
		read_body(view.body, &candidate);
		if (!context_valid_range(command, image, candidate, view.count,
			[&](size_t i) { return read_item(bytes.data() + view.item_offsets[i]); },
			[&](size_t i) { return static_cast<size_t>(
				get(bytes.data() + view.item_offsets[i] + 16, 4)); },
			[&](size_t i, size_t e) {
				return effect(bytes[view.item_offsets[i] + ITEM_BYTES + e]); }))
			return error::corrupt_evidence;
		// All wire semantics and recipe correlations are proven before reserve.
		size_t heap = 0;
		if (!recovery_bound_array(heap, view.count, sizeof(zone_reset_item_recovery_item)))
			return error::capacity;
		for (size_t i = 0; i < view.count; ++i)
			if (!recovery_bound_array(heap, static_cast<size_t>(
				get(bytes.data() + view.item_offsets[i] + 16, 4)),
				sizeof(zone_reset_item_recovery_effect)))
				return error::capacity;
		if (!recovery_bound_add(live, heap) ||
		    !recovery_bound_add(live, 2 * sizeof(zone_reset_item_recovery_item) +
			2 * sizeof(zone_reset_item_recovery_effect)) || !reserve(live, context))
			return error::capacity;
		candidate.items.reserve(view.count);
		for (size_t i = 0; i < view.count; ++i)
		{
			const size_t offset = view.item_offsets[i];
			auto item = read_item(bytes.data() + offset);
			const size_t count = static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset + ITEM_BYTES + e]));
			candidate.items.push_back(std::move(item));
		}
		*output = std::move(candidate);
		if (retained_context_heap_bytes)
			*retained_context_heap_bytes = heap;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
#endif
}

bool zone_reset_item_recovery_initial_bounded(const critical_native_recovery_envelope &envelope,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || envelope.revision != 1 ||
	    envelope.phase != critical_native_recovery_phase::execution_pending ||
	    !recovery_bound_add(outer_live, sizeof(zone_reset_item_recovery_context)) ||
	    !reserve(outer_live, context))
		return false;
	zone_reset_item_recovery_context v;
	return zone_reset_item_recovery_decode_bounded(envelope.command, envelope.attachment,
		&v, reserve, context, outer_live) == error::ok &&
		!v.receipt_present && no_progress(v);
}

bool zone_reset_item_recovery_initial(const critical_native_recovery_envelope &envelope) noexcept
{
	zone_reset_item_recovery_context v;
	return envelope.revision == 1 &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope_decode(envelope, &v) && !v.receipt_present && no_progress(v);
}
bool zone_reset_item_recovery_successor(const critical_native_recovery_envelope &expected,
					const critical_native_recovery_envelope &successor) noexcept
{
	try
	{
		zone_reset_item_recovery_context before, after;
		if (expected.revision == UINT64_MAX ||
		    successor.revision != expected.revision + 1 ||
		    !critical_command_equal(expected.command, successor.command) ||
		    !envelope_decode(expected, &before) || !envelope_decode(successor, &after))
			return false;
		if (expected.phase == critical_native_recovery_phase::continuation_pending ||
		    successor.phase == critical_native_recovery_phase::continuation_pending)
			return successor.phase ==
				       critical_native_recovery_phase::continuation_pending &&
			       expected.attachment == successor.attachment && body_terminal(after);
		return successor_context(before, after);
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_item_recovery_publication(const critical_native_recovery_envelope &envelope,
					  const critical_completion &current) noexcept
{
	try
	{
		zone_reset_item_recovery_context v;
		zone_reset_item_image image;
		return envelope_decode(envelope, &v) && body_terminal(v) &&
		       zone_reset_item_command_decode(envelope.command, &image) == error::ok &&
		       receipt_result_valid(envelope.command, image, true, current) &&
		       receipt_core_equal(v.receipt, current);
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_item_recovery_terminal(const critical_native_recovery_envelope &envelope) noexcept
{
	zone_reset_item_recovery_context v;
	return envelope.phase == critical_native_recovery_phase::continuation_pending &&
	       envelope_decode(envelope, &v) && body_terminal(v);
}
