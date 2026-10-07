#include "economy/native_mobile_birth_recovery.h"
#include "economy/native_mobile_birth_result.h"
#include "core/config.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <utility>
#include <type_traits>

namespace
{
using error = economic_accounting_error;
constexpr uint32_t MAGIC = 0x31524d4e; // NMR1.
constexpr size_t HEADER_BYTES = 12;
constexpr size_t RECEIPT_BYTES = 97 + CRITICAL_COMPLETION_RESULT_MAX_BYTES;
constexpr size_t STATE_BYTES = 48;
constexpr size_t BODY_BYTES = STATE_BYTES + RECEIPT_BYTES + 4;
constexpr size_t ITEM_BYTES = 20;
constexpr size_t LIMIT = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;

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
uint8_t bits(const native_mobile_birth_recovery_action &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0));
}
uint8_t bits(const native_mobile_birth_recovery_mobile &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.consumed ? 4 : 0));
}
uint8_t bits(const native_mobile_birth_recovery_effect &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0) | (a.periodic ? 8 : 0));
}
native_mobile_birth_recovery_action action(uint8_t value) noexcept
{
	return { bool(value & 1), bool(value & 2), bool(value & 4) };
}
native_mobile_birth_recovery_mobile mobile(uint8_t value) noexcept
{
	return { bool(value & 1), bool(value & 2), bool(value & 4) };
}
native_mobile_birth_recovery_effect effect(uint8_t value) noexcept
{
	return { bool(value & 1), bool(value & 2), bool(value & 4), bool(value & 8) };
}
bool valid_action(const native_mobile_birth_recovery_action &a) noexcept
{
	return (!a.returned || a.started) && (!a.succeeded || a.returned);
}
bool complete(const native_mobile_birth_recovery_action &a) noexcept
{
	return a.started && a.returned && a.succeeded;
}
bool complete(const native_mobile_birth_recovery_mobile &a) noexcept
{
	return a.started && a.returned && a.consumed;
}
bool complete(const native_mobile_birth_recovery_effect &e) noexcept
{
	return e.started && e.returned && e.succeeded;
}
bool choice_valid(size_t index, const native_mobile_birth_recovery_choice &choice) noexcept
{
	if (!choice.chosen)
		return !choice.requested && !choice.delay;
	if (!choice.requested)
		return index != 0 && !choice.delay;
	switch (index)
	{
	case 0:
		return choice.delay > 0;
	case 1:
		return choice.delay >= PULSE_MOBILE - 4 && choice.delay <= PULSE_MOBILE + 4;
	case 2:
		return choice.delay == WAIT_SEC;
	case 3:
		return choice.delay >= 1 && choice.delay <= 5;
	}
	return false;
}
native_mobile_birth_recovery_choice read_choice(const uint8_t *input) noexcept
{
	return { bool(input[0] & 1), bool(input[0] & 2),
		 std::bit_cast<int32_t>(static_cast<uint32_t>(get(input + 4, 4))) };
}
bool mobile_shape(const native_mobile_birth_recovery_context &value) noexcept
{
	bool previous_done = true, all_done = true;
	for (size_t i = 0; i < value.mobile_effects.size(); ++i)
	{
		const auto &e = value.mobile_effects[i];
		if (!valid_action({ e.started, e.returned, e.succeeded }) ||
		    (e.periodic && (!e.returned || i != 3)) || (e.started && !previous_done))
			return false;
		previous_done = complete(e);
		all_done = all_done && previous_done;
	}
	constexpr std::array<size_t, 4> schedule_steps{ 2, 4, 5, 6 };
	for (size_t i = 0; i < value.mobile_choices.size(); ++i)
	{
		const auto &choice = value.mobile_choices[i];
		const size_t step = schedule_steps[i];
		if (!choice_valid(i, choice) ||
		    (choice.chosen && !complete(value.mobile_effects[step - 1])) ||
		    (value.mobile_effects[step].started && !choice.chosen) ||
		    (i == 1 && choice.chosen &&
		     choice.requested != value.mobile_effects[3].periodic))
			return false;
	}
	return value.mobile_publication.started == value.mobile_effects[0].started &&
	       value.mobile_publication.consumed == complete(value.mobile_effects[0]) &&
	       (!value.mobile_publication.returned || all_done);
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
	return !successful(receipt) || (receipt.durable_revision == 1 && !receipt.error_code &&
					receipt.failure_stage == critical_failure_stage::none &&
					receipt.result_size == NATIVE_MOBILE_BIRTH_RESULT_BYTES);
}
bool receipt_shape(const critical_command &command, bool present,
		   const critical_completion &receipt) noexcept
{
	return (!present || receipt.operation_id.bytes == command.operation_id.bytes) &&
	       receipt_values_shape(present, receipt);
}
bool receipt_result_valid(const critical_command &command, bool present,
			  const critical_completion &receipt)
{
	if (!receipt_shape(command, present, receipt))
		return false;
	if (!present || !successful(receipt))
		return true;
	native_mobile_birth_result result;
	if (!native_mobile_birth_result_decode(
		    { receipt.result_payload.data(), receipt.result_size }, &result))
		return false;
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) != error::ok)
		return false;
	const economic_account_key wallet{ intent.admission.metadata.lineage,
					   economic_account_kind::wallet, result.wallet_mapping_id,
					   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	economic_accounting_plan plan;
	return native_mobile_birth_accounting_compile(command, wallet, &plan) == error::ok &&
	       native_mobile_birth_result_matches(command, wallet, plan, result);
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
bool item_done_fields(const native_mobile_birth_recovery_item &item, size_t effect_count) noexcept
{
	return item.admitted && item.published && complete(item.publication) &&
	       complete(item.enrollment) && !item.current_step_started &&
	       item.next_step == effect_count;
}
bool item_done(const native_mobile_birth_recovery_item &item) noexcept
{
	return item_done_fields(item, item.effects.size());
}
bool body_terminal(const native_mobile_birth_recovery_context &value) noexcept
{
	return value.receipt_present && successful(value.receipt) &&
	       value.stage == native_mobile_birth_recovery_stage::physically_proven &&
	       complete(value.whole_binding) && complete(value.reference_install) &&
	       complete(value.mobile_publication) && value.runtime_applied &&
	       std::all_of(value.items.begin(), value.items.end(), item_done);
}
bool no_progress(const native_mobile_birth_recovery_context &value) noexcept
{
	if (value.stage != native_mobile_birth_recovery_stage::captured ||
	    bits(value.whole_binding) || bits(value.reference_install) ||
	    bits(value.mobile_publication) || value.runtime_applied)
		return false;
	for (const auto &e : value.mobile_effects)
		if (bits(e))
			return false;
	for (const auto &choice : value.mobile_choices)
		if (choice.chosen || choice.requested || choice.delay)
			return false;
	for (const auto &item : value.items)
	{
		if (item.next_step || item.current_step_started || item.admitted ||
		    item.published || bits(item.publication) || bits(item.enrollment))
			return false;
		for (const auto &e : item.effects)
			if (bits(e))
				return false;
	}
	return true;
}
size_t recipe_index(std::span<const native_mobile_birth_item_recipe> recipes,
		    uint64_t object_uid) noexcept
{
	for (size_t index = 0; index < recipes.size(); ++index)
		if (recipes[index].object_uid == object_uid)
			return index;
	return recipes.size();
}
template <typename GetItem, typename GetCount, typename GetEffect>
bool context_valid_range(const critical_command &command,
			 std::span<const native_mobile_birth_item_recipe> recipes,
			 const native_mobile_birth_recovery_context &value, size_t count,
			 GetItem get_item, GetCount get_count, GetEffect get_effect)
{
	if (value.stage > native_mobile_birth_recovery_stage::physically_proven ||
	    count != recipes.size() || recipes.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !valid_action(value.whole_binding) || !valid_action(value.reference_install) ||
	    (value.mobile_publication.returned && !value.mobile_publication.started) ||
	    (value.mobile_publication.consumed && !value.mobile_publication.started) ||
	    !mobile_shape(value) ||
	    !receipt_result_valid(command, value.receipt_present, value.receipt))
		return false;
	if ((!value.receipt_present || !successful(value.receipt)) && !no_progress(value))
		return false;
	if (value.stage == native_mobile_birth_recovery_stage::captured && !no_progress(value))
		return false;
	if (value.reference_install.started && !complete(value.whole_binding))
		return false;
	bool all_published = true, previous_item_done = true, all_done = true;
	const bool must_zero = !value.receipt_present || !successful(value.receipt) ||
			       value.stage == native_mobile_birth_recovery_stage::captured;
	std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS> seen{};
	for (size_t i = 0; i < count; ++i)
	{
		const auto &item = get_item(i);
		const size_t effect_count = get_count(i);
		const size_t index = recipe_index(recipes, item.object_uid);
		// Count equality plus complete unique membership binds every image UID,
		// while these prefix guards retain the actual original publication order.
		if (index == recipes.size() || seen[index])
			return false;
		seen[index] = 1;
		if (!valid_action(item.publication) || !valid_action(item.enrollment) ||
		    !effects_valid(recipes[index], item.next_step, item.current_step_started,
				   effect_count, [&](size_t e) { return get_effect(i, e); }) ||
		    (must_zero &&
		     (item.next_step || item.current_step_started || item.admitted ||
		      item.published || bits(item.publication) || bits(item.enrollment))) ||
		    item.published != complete(item.publication) ||
		    (item.publication.started &&
		     (!item.admitted || !complete(value.reference_install) || !all_published)) ||
		    (item.enrollment.started &&
		     (!complete(value.mobile_publication) || !previous_item_done)) ||
		    ((item.next_step || item.current_step_started) && !complete(item.enrollment)))
			return false;
		all_published = all_published && item.published;
		previous_item_done = item_done_fields(item, effect_count);
		all_done = all_done && previous_item_done;
	}
	if ((value.mobile_publication.started &&
	     (!all_published || !complete(value.reference_install))) ||
	    (value.runtime_applied && (!complete(value.mobile_publication) || !all_done)))
		return false;
	return value.stage != native_mobile_birth_recovery_stage::physically_proven ||
	       (value.receipt_present && successful(value.receipt) &&
		complete(value.whole_binding) && complete(value.reference_install) &&
		complete(value.mobile_publication) && value.runtime_applied && all_done);
}
bool context_valid(const critical_command &command,
		   std::span<const native_mobile_birth_item_recipe> recipes,
		   const native_mobile_birth_recovery_context &value)
{
	return context_valid_range(
		command, recipes, value, value.items.size(),
		[&](size_t i) -> const native_mobile_birth_recovery_item &
		{ return value.items[i]; }, [&](size_t i) { return value.items[i].effects.size(); },
		[&](size_t i, size_t e) { return value.items[i].effects[e]; });
}
bool command_values(const critical_command &command, std::vector<uint8_t> *canonical,
		    std::vector<native_mobile_birth_item_recipe> *recipes)
{
	quest_mobile_native_image image;
	return (command.payload_version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION ||
		command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION) &&
	       command.publication_required &&
	       critical_command_encode(command, canonical) == critical_command_codec_result::ok &&
	       canonical->size() <= CRITICAL_COMMAND_MAX_ENCODED_BYTES &&
	       native_mobile_birth_command_decode(command, &image, recipes) == error::ok;
}

struct wire_view
{
	std::span<const uint8_t> command;
	const uint8_t *body = nullptr;
	size_t items_offset = 0;
	uint32_t count = 0;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_ROWS> item_offsets{};
};
// Validate every count and byte/retained-memory bound before any allocation,
// including before the existing bounded command/image/recipe decoders run.
error preflight(const critical_command *command, std::span<const uint8_t> bytes,
		wire_view *view) noexcept
{
	if (!view || bytes.size() > LIMIT)
		return error::capacity;
	if (bytes.size() < HEADER_BYTES + BODY_BYTES || get(bytes.data(), 4) != MAGIC ||
	    get(bytes.data() + 4, 2) != NATIVE_MOBILE_BIRTH_RECOVERY_VERSION)
		return error::invalid_version;
	const size_t command_size = static_cast<uint32_t>(get(bytes.data() + 8, 4));
	if (get(bytes.data() + 6, 2) || !command_size ||
	    command_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    command_size > bytes.size() - HEADER_BYTES - BODY_BYTES)
		return error::corrupt_evidence;
	wire_view candidate;
	candidate.command = bytes.subspan(HEADER_BYTES, command_size);
	candidate.body = bytes.data() + HEADER_BYTES + command_size;
	const auto *body = candidate.body;
	if (body[0] > 1 || body[1] > 2 || body[2] > 7 || body[3] > 7 || body[4] > 7 ||
	    body[5] > 1 || get(body + 6, 2) || get(body + STATE_BYTES + 62, 2))
		return error::corrupt_evidence;
	const auto stored_receipt = read_receipt(body + STATE_BYTES);
	const auto stored_mobile = mobile(body[4]);
	if (!(command ? receipt_shape(*command, body[0] != 0, stored_receipt) :
			receipt_values_shape(body[0] != 0, stored_receipt)) ||
	    !valid_action(action(body[2])) || !valid_action(action(body[3])) ||
	    (stored_mobile.returned && !stored_mobile.started) ||
	    (stored_mobile.consumed && !stored_mobile.started))
		return error::corrupt_evidence;
	for (size_t i = 0; i < 8; ++i)
	{
		const auto e = effect(body[8 + i]);
		if (body[8 + i] > 15 || !valid_action({ e.started, e.returned, e.succeeded }) ||
		    (e.periodic && (!e.returned || i != 3)))
			return error::corrupt_evidence;
	}
	for (size_t i = 0; i < 4; ++i)
	{
		const auto *choice = body + 16 + i * 8;
		if (choice[0] > 3 || get(choice + 1, 3) || !choice_valid(i, read_choice(choice)))
			return error::corrupt_evidence;
	}
	candidate.count = static_cast<uint32_t>(get(body + STATE_BYTES + RECEIPT_BYTES, 4));
	candidate.items_offset = HEADER_BYTES + command_size + BODY_BYTES;
	size_t offset = candidate.items_offset;
	if (candidate.count > PLAYER_SNAPSHOT_MAX_ROWS ||
	    candidate.count > (bytes.size() - offset) / ITEM_BYTES)
		return error::corrupt_evidence;
	size_t retained = sizeof(native_mobile_birth_recovery_context) + command_size;
	if (candidate.count > (LIMIT - retained) / sizeof(native_mobile_birth_recovery_item))
		return error::capacity;
	retained += candidate.count * sizeof(native_mobile_birth_recovery_item);
	for (size_t i = 0; i < candidate.count; ++i)
	{
		candidate.item_offsets[i] = offset;
		if (bytes.size() - offset < ITEM_BYTES)
			return error::corrupt_evidence;
		const auto *item = bytes.data() + offset;
		const size_t effects = static_cast<uint32_t>(get(item + 16, 4));
		if (item[12] > 7 || item[13] > 7 || item[14] > 7 || item[15] ||
		    effects > 2 * PLAYER_SNAPSHOT_MAX_ROWS + 3 || get(item + 8, 4) > effects ||
		    !valid_action(action(item[13])) || !valid_action(action(item[14])))
			return error::corrupt_evidence;
		offset += ITEM_BYTES;
		const size_t remaining = candidate.count - i - 1;
		if (remaining > (bytes.size() - offset) / ITEM_BYTES ||
		    effects > bytes.size() - offset - remaining * ITEM_BYTES ||
		    effects > (LIMIT - retained) / sizeof(native_mobile_birth_recovery_effect))
			return error::capacity;
		retained += effects * sizeof(native_mobile_birth_recovery_effect);
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
	*view = candidate;
	return error::ok;
}
void read_body(const uint8_t *body, native_mobile_birth_recovery_context *value) noexcept
{
	value->receipt_present = body[0] != 0;
	value->stage = static_cast<native_mobile_birth_recovery_stage>(body[1]);
	value->whole_binding = action(body[2]);
	value->reference_install = action(body[3]);
	value->mobile_publication = mobile(body[4]);
	value->runtime_applied = body[5] != 0;
	for (size_t i = 0; i < value->mobile_effects.size(); ++i)
		value->mobile_effects[i] = effect(body[8 + i]);
	for (size_t i = 0; i < value->mobile_choices.size(); ++i)
		value->mobile_choices[i] = read_choice(body + 16 + i * 8);
	value->receipt = read_receipt(body + STATE_BYTES);
}
native_mobile_birth_recovery_item read_item(const uint8_t *item) noexcept
{
	native_mobile_birth_recovery_item value;
	value.object_uid = get(item, 8);
	value.next_step = static_cast<uint32_t>(get(item + 8, 4));
	value.current_step_started = (item[12] & 1) != 0;
	value.admitted = (item[12] & 2) != 0;
	value.published = (item[12] & 4) != 0;
	value.publication = action(item[13]);
	value.enrollment = action(item[14]);
	return value;
}
bool successor_action(const native_mobile_birth_recovery_action &a,
		      const native_mobile_birth_recovery_action &b) noexcept
{
	if (a == b)
		return true;
	if (!a.started)
		return b.started && !b.returned && !b.succeeded;
	if (!a.returned)
		return b.started && b.returned;
	return false;
}
bool successor_mobile(const native_mobile_birth_recovery_mobile &a,
		      const native_mobile_birth_recovery_mobile &b) noexcept
{
	if (a == b)
		return true;
	if (!a.started)
		return b.started && !b.returned && !b.consumed;
	if (a.returned || (a.consumed && !b.consumed) || !b.started)
		return false;
	return !b.returned || b.consumed == a.consumed;
}
bool successor_effect(const native_mobile_birth_recovery_effect &a,
		      const native_mobile_birth_recovery_effect &b) noexcept
{
	if (a == b)
		return true;
	return successor_action({ a.started, a.returned, a.succeeded },
				{ b.started, b.returned, b.succeeded }) &&
	       (!a.returned || a.periodic == b.periodic);
}
bool successor_context(const native_mobile_birth_recovery_context &a,
		       const native_mobile_birth_recovery_context &b) noexcept
{
	if ((a.receipt_present && !b.receipt_present) ||
	    (a.receipt_present && !completion_equal(a.receipt, b.receipt) &&
	     !receipt_core_equal(a.receipt, b.receipt)) ||
	    b.stage < a.stage ||
	    static_cast<unsigned>(b.stage) > static_cast<unsigned>(a.stage) + 1 ||
	    (a.runtime_applied && !b.runtime_applied) ||
	    !successor_action(a.whole_binding, b.whole_binding) ||
	    !successor_action(a.reference_install, b.reference_install) ||
	    !successor_mobile(a.mobile_publication, b.mobile_publication) ||
	    a.items.size() != b.items.size())
		return false;
	for (size_t i = 0; i < a.mobile_effects.size(); ++i)
		if (!successor_effect(a.mobile_effects[i], b.mobile_effects[i]))
			return false;
	for (size_t i = 0; i < a.mobile_choices.size(); ++i)
		if (a.mobile_choices[i].chosen && a.mobile_choices[i] != b.mobile_choices[i])
			return false;
	for (size_t i = 0; i < a.items.size(); ++i)
	{
		const auto &left = a.items[i];
		const auto &right = b.items[i];
		if (left.object_uid != right.object_uid || right.next_step < left.next_step ||
		    right.next_step > static_cast<uint64_t>(left.next_step) + 1 ||
		    (left.admitted && !right.admitted) || (left.published && !right.published) ||
		    !successor_action(left.publication, right.publication) ||
		    !successor_action(left.enrollment, right.enrollment) ||
		    left.effects.size() != right.effects.size())
			return false;
		for (size_t step = 0; step < left.effects.size(); ++step)
			if (!successor_effect(left.effects[step], right.effects[step]))
				return false;
		if (left.current_step_started && !right.current_step_started &&
		    right.next_step != left.next_step + 1)
			return false;
	}
	return true;
}
bool envelope_decode(const critical_native_recovery_envelope &envelope,
		     native_mobile_birth_recovery_context *context) noexcept
{
	return envelope.revision &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		envelope.phase == critical_native_recovery_phase::continuation_pending) &&
	       native_mobile_birth_recovery_decode(envelope.command, envelope.attachment,
						   context) == error::ok &&
	       (envelope.revision != 1 ||
		(envelope.phase == critical_native_recovery_phase::execution_pending &&
		 !context->receipt_present && no_progress(*context))) &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		body_terminal(*context));
}
} // namespace

economic_accounting_error
native_mobile_birth_recovery_encode(const critical_command &command,
				    const native_mobile_birth_recovery_context &value,
				    std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	try
	{
		std::vector<uint8_t> canonical;
		std::vector<native_mobile_birth_item_recipe> recipes;
		if (!command_values(command, &canonical, &recipes) ||
		    !context_valid(command, recipes, value))
			return error::corrupt_evidence;
		size_t size = HEADER_BYTES + canonical.size() + BODY_BYTES;
		for (const auto &item : value.items)
		{
			if (size > LIMIT - ITEM_BYTES ||
			    item.effects.size() > LIMIT - size - ITEM_BYTES)
				return error::capacity;
			size += ITEM_BYTES + item.effects.size();
		}
		std::vector<uint8_t> bytes(size, 0);
		put(bytes.data(), MAGIC, 4);
		put(bytes.data() + 4, NATIVE_MOBILE_BIRTH_RECOVERY_VERSION, 2);
		put(bytes.data() + 8, canonical.size(), 4);
		std::copy(canonical.begin(), canonical.end(), bytes.begin() + HEADER_BYTES);
		auto *body = bytes.data() + HEADER_BYTES + canonical.size();
		body[0] = value.receipt_present ? 1 : 0;
		body[1] = static_cast<uint8_t>(value.stage);
		body[2] = bits(value.whole_binding);
		body[3] = bits(value.reference_install);
		body[4] = bits(value.mobile_publication);
		body[5] = value.runtime_applied ? 1 : 0;
		for (size_t i = 0; i < value.mobile_effects.size(); ++i)
			body[8 + i] = bits(value.mobile_effects[i]);
		for (size_t i = 0; i < value.mobile_choices.size(); ++i)
		{
			auto *choice = body + 16 + i * 8;
			choice[0] =
				static_cast<uint8_t>((value.mobile_choices[i].chosen ? 1 : 0) |
						     (value.mobile_choices[i].requested ? 2 : 0));
			put(choice + 4, static_cast<uint32_t>(value.mobile_choices[i].delay), 4);
		}
		write_receipt(body + STATE_BYTES, value.receipt);
		put(body + STATE_BYTES + RECEIPT_BYTES, value.items.size(), 4);
		size_t offset = HEADER_BYTES + canonical.size() + BODY_BYTES;
		for (const auto &item : value.items)
		{
			auto *encoded = bytes.data() + offset;
			put(encoded, item.object_uid, 8);
			put(encoded + 8, item.next_step, 4);
			encoded[12] = static_cast<uint8_t>((item.current_step_started ? 1 : 0) |
							   (item.admitted ? 2 : 0) |
							   (item.published ? 4 : 0));
			encoded[13] = bits(item.publication);
			encoded[14] = bits(item.enrollment);
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
}

economic_accounting_error
native_mobile_birth_recovery_decode(const critical_command &command, std::span<const uint8_t> bytes,
				    native_mobile_birth_recovery_context *output) noexcept
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
		std::vector<native_mobile_birth_item_recipe> recipes;
		if (!command_values(command, &canonical, &recipes) ||
		    !std::equal(canonical.begin(), canonical.end(), view.command.begin(),
				view.command.end()) ||
		    view.count != recipes.size())
			return error::payload_conflict;
		// The complete wire bounds preceded all allocation. Check recipe-derived
		// counts before creating any attachment container, not only afterwards.
		size_t offset = view.items_offset;
		for (size_t i = 0; i < recipes.size(); ++i)
		{
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			const size_t index = recipe_index(recipes, get(bytes.data() + offset, 8));
			if (index == recipes.size() || count != step_count(recipes[index]))
				return error::corrupt_evidence;
			offset += ITEM_BYTES + count;
		}
		native_mobile_birth_recovery_context candidate;
		read_body(view.body, &candidate);
		if (!context_valid_range(
			    command, recipes, candidate, view.count, [&](size_t i)
			    { return read_item(bytes.data() + view.item_offsets[i]); },
			    [&](size_t i) {
				    return static_cast<size_t>(
					    get(bytes.data() + view.item_offsets[i] + 16, 4));
			    },
			    [&](size_t i, size_t e)
			    { return effect(bytes[view.item_offsets[i] + ITEM_BYTES + e]); }))
			return error::corrupt_evidence;
		// All attachment semantics, including ordering, cursor/latches and exact
		// recipe/result correlation, now passed before attachment containers allocate.
		candidate.items.reserve(view.count);
		offset = view.items_offset;
		for (size_t i = 0; i < view.count; ++i)
		{
			auto item = read_item(bytes.data() + offset);
			// Already proved against this UID's recipe before any container allocation.
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			offset += ITEM_BYTES;
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset++]));
			candidate.items.push_back(std::move(item));
		}
		*output = std::move(candidate);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

economic_accounting_error
native_mobile_birth_recovery_original_command_decode(std::span<const uint8_t> bytes,
						     critical_command *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	// The SAME original parser bounds every header/body/count/effect before
	// even the command codec can allocate. No caller-invented command identity.
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
		// Legacy stock-only commands remain readable through the original API,
		// but cannot provide unseen NPC constructor recovery inputs.
		if (original.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
			return error::invalid_version;
		native_mobile_birth_recovery_context context;
		const auto valid = native_mobile_birth_recovery_decode(original, bytes, &context);
		if (valid != error::ok)
			return valid;
		if (!body_terminal(context))
			return error::unresolved;
		// Every original field came from the retained canonical command bytes.
		// Envelope phase/revision and authenticated lifetime storage are separate.
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*output = std::move(original);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

bool native_mobile_birth_recovery_valid(const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope_decode(envelope, &value);
}
bool native_mobile_birth_recovery_initial(const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope.revision == 1 &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope_decode(envelope, &value) && !value.receipt_present && no_progress(value);
}
bool native_mobile_birth_recovery_successor(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	try
	{
		native_mobile_birth_recovery_context before, after;
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
bool native_mobile_birth_recovery_publication(const critical_native_recovery_envelope &envelope,
					      const critical_completion &current) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope_decode(envelope, &value) && body_terminal(value) &&
	       receipt_core_equal(value.receipt, current);
}
bool native_mobile_birth_recovery_terminal(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope.phase == critical_native_recovery_phase::continuation_pending &&
	       envelope_decode(envelope, &value) && body_terminal(value);
}
