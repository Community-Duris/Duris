#include "economy/native_mobile_birth_recovery.h"
#include "economy/native_mobile_birth_result.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include "core/config.h"
#include "flatfile/flatfile_shopkeeper_repository.h"

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
// Historical callers never acquire prospective role support implicitly.
enum class recovery_policy
{
	historical,
	ordinary_cash_role,
	shared_shop
};
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
bool receipt_values_shape(bool present, const critical_completion &receipt,
			  recovery_policy policy = recovery_policy::historical) noexcept
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
	       ((receipt.durable_revision == 1 ||
		 (policy == recovery_policy::shared_shop && receipt.durable_revision > 0)) &&
		!receipt.error_code && receipt.failure_stage == critical_failure_stage::none &&
		receipt.result_size == (policy != recovery_policy::historical ?
						NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES :
						NATIVE_MOBILE_BIRTH_RESULT_BYTES));
}
bool receipt_shape(const critical_command &command, bool present,
		   const critical_completion &receipt,
		   recovery_policy policy = recovery_policy::historical) noexcept
{
	return (!present || receipt.operation_id.bytes == command.operation_id.bytes) &&
	       receipt_values_shape(present, receipt, policy);
}
bool receipt_result_valid(const critical_command &command, bool present,
			  const critical_completion &receipt,
			  recovery_policy policy = recovery_policy::historical)
{
	if (!receipt_shape(command, present, receipt, policy))
		return false;
	if (!present || !successful(receipt))
		return true;
	if (policy == recovery_policy::shared_shop)
	{
		native_mobile_birth_cash_role_result result;
		if (!native_mobile_birth_cash_role_result_decode(
			    { receipt.result_payload.data(), receipt.result_size }, &result) ||
		    result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    receipt.durable_revision !=
			    std::max(uint64_t{ 1 }, result.shared.owner_revision_after))
			return false;
		quest_mobile_native_image original_image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		if (native_mobile_birth_cash_role_command_decode(command, &original_image, &recipes,
								 &role) != error::ok ||
		    role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    result.shared.shop_before_present || result.shared.shop_revision_before != 0 ||
		    !result.shared.shop_after_present || result.shared.shop_revision_after != 1)
			return false;
		if (original_image.items.empty())
		{
			if (result.shared.owner_before_present !=
				    result.shared.owner_after_present ||
			    result.shared.owner_revision_before !=
				    result.shared.owner_revision_after)
				return false;
		}
		else if (!result.shared.owner_after_present ||
			 result.shared.owner_revision_before == UINT64_MAX ||
			 result.shared.owner_revision_after !=
				 result.shared.owner_revision_before + 1)
			return false;
		economic_accounting_plan plan;
		return native_mobile_birth_cash_role_accounting_compile(command, result.shared,
									&plan) == error::ok &&
		       native_mobile_birth_cash_role_result_matches(command, result.shared, plan,
								    result);
	}
	if (policy == recovery_policy::ordinary_cash_role)
	{
		native_mobile_birth_cash_role_result result;
		if (!native_mobile_birth_cash_role_result_decode(
			    { receipt.result_payload.data(), receipt.result_size }, &result) ||
		    result.role != native_mobile_birth_cash_role::ordinary_wallet)
			return false;
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) != error::ok)
			return false;
		const economic_account_key wallet{ intent.admission.metadata.lineage,
						   economic_account_kind::wallet,
						   result.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan plan;
		return native_mobile_birth_cash_role_accounting_compile(command, wallet, &plan) ==
			       error::ok &&
		       native_mobile_birth_cash_role_result_matches(command, wallet, plan, result);
	}
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
			 GetItem get_item, GetCount get_count, GetEffect get_effect,
			 recovery_policy policy = recovery_policy::historical)
{
	if (value.stage > native_mobile_birth_recovery_stage::physically_proven ||
	    count != recipes.size() || recipes.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !valid_action(value.whole_binding) || !valid_action(value.reference_install) ||
	    (value.mobile_publication.returned && !value.mobile_publication.started) ||
	    (value.mobile_publication.consumed && !value.mobile_publication.started) ||
	    !mobile_shape(value) ||
	    !receipt_result_valid(command, value.receipt_present, value.receipt, policy))
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
		   const native_mobile_birth_recovery_context &value,
		   recovery_policy policy = recovery_policy::historical)
{
	return context_valid_range(
		command, recipes, value, value.items.size(),
		[&](size_t i) -> const native_mobile_birth_recovery_item &
		{ return value.items[i]; }, [&](size_t i) { return value.items[i].effects.size(); },
		[&](size_t i, size_t e) { return value.items[i].effects[e]; }, policy);
}
bool command_values(const critical_command &command, std::vector<uint8_t> *canonical,
		    std::vector<native_mobile_birth_item_recipe> *recipes,
		    recovery_policy policy = recovery_policy::historical)
{
	quest_mobile_native_image image;
	if (policy != recovery_policy::historical)
	{
		native_mobile_birth_cash_role_recipe role;
		return command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
		       command.publication_required &&
		       critical_command_encode(command, canonical) ==
			       critical_command_codec_result::ok &&
		       canonical->size() <= CRITICAL_COMMAND_MAX_ENCODED_BYTES &&
		       native_mobile_birth_cash_role_command_decode(command, &image, recipes,
								    &role) == error::ok &&
		       role.role == (policy == recovery_policy::shared_shop ?
					     native_mobile_birth_cash_role::shared_shopkeeper :
					     native_mobile_birth_cash_role::ordinary_wallet);
	}
	return (command.payload_version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION ||
		command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION) &&
	       command.publication_required &&
	       critical_command_encode(command, canonical) == critical_command_codec_result::ok &&
	       canonical->size() <= CRITICAL_COMMAND_MAX_ENCODED_BYTES &&
	       native_mobile_birth_command_decode(command, &image, recipes) == error::ok;
}

// Canonical checkpoint observations are retained values only. The original
// producer and executing SQL owner authenticate their actual source/lifetime.
bool checkpoint_matches(const critical_command &command, std::span<const uint8_t> bytes)
{
	if (bytes.empty() || bytes.size() > LIMIT)
		return false;
	flatfile_shopkeeper_record record;
	std::vector<uint8_t> encoded(bytes.begin(), bytes.end());
	if (!flatfile_shopkeeper_initial_checkpoint_decode(encoded, &record))
		return false;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	if (native_mobile_birth_cash_role_command_decode(command, &image, &recipes, &role) !=
		    error::ok ||
	    role.role != native_mobile_birth_cash_role::shared_shopkeeper || !image.cash ||
	    record.shop_id != static_cast<uint32_t>(role.original.reset_shop_index) ||
	    record.mob_vnum != role.original.mobile_vnum ||
	    record.room_vnum != role.original.reset_room_vnum || record.revision != 1 ||
	    record.saved_at < 0 || record.cash < 0)
		return false;
	int64_t cash = 0;
	std::vector<uint8_t> original_items, checkpoint_items;
	return economic_coin_value(image.cash->denominations.amount, &cash) == error::ok &&
	       cash == record.cash &&
	       player_item_snapshot_list_encode(image.items, &original_items) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(record.items, &checkpoint_items) ==
		       player_snapshot_codec_result::ok &&
	       original_items == checkpoint_items;
}

struct wire_view
{
	std::span<const uint8_t> command;
	std::span<const uint8_t> checkpoint;
	const uint8_t *body = nullptr;
	size_t items_offset = 0;
	uint32_t count = 0;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_ROWS> item_offsets{};
};
// Validate every count and byte/retained-memory bound before any allocation,
// including before the existing bounded command/image/recipe decoders run.
error preflight(const critical_command *command, std::span<const uint8_t> bytes, wire_view *view,
		recovery_policy policy = recovery_policy::historical) noexcept
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
	if (!(command ? receipt_shape(*command, body[0] != 0, stored_receipt, policy) :
			receipt_values_shape(body[0] != 0, stored_receipt, policy)) ||
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
	size_t retained = (policy == recovery_policy::shared_shop ?
				   sizeof(native_mobile_birth_shared_shop_recovery_context) :
				   sizeof(native_mobile_birth_recovery_context)) +
			  command_size;
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
	if (policy == recovery_policy::shared_shop)
	{
		if (bytes.size() - offset < 4)
			return error::corrupt_evidence;
		const size_t length = static_cast<uint32_t>(get(bytes.data() + offset, 4));
		offset += 4;
		if (!length || length != bytes.size() - offset)
			return error::corrupt_evidence;
		if (length > LIMIT - retained)
			return error::capacity;
		candidate.checkpoint = bytes.subspan(offset, length);
		offset += length;
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
		     native_mobile_birth_recovery_context *context,
		     recovery_policy policy = recovery_policy::historical) noexcept
{
	return envelope.revision &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		envelope.phase == critical_native_recovery_phase::continuation_pending) &&
	       ([&]() {
		       if (policy == recovery_policy::shared_shop)
		       {
			       native_mobile_birth_shared_shop_recovery_context value;
			       const auto result = native_mobile_birth_shared_shop_recovery_decode(
				       envelope.command, envelope.attachment, &value);
			       if (result == error::ok)
				       *context = std::move(value.progress);
			       return result;
		       }
		       return policy == recovery_policy::ordinary_cash_role ?
				      native_mobile_birth_cash_role_recovery_decode(
					      envelope.command, envelope.attachment, context) :
				      native_mobile_birth_recovery_decode(
					      envelope.command, envelope.attachment, context);
	       }()) == error::ok &&
	       (envelope.revision != 1 ||
		(envelope.phase == critical_native_recovery_phase::execution_pending &&
		 !context->receipt_present && no_progress(*context))) &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		body_terminal(*context));
}
} // namespace

static economic_accounting_error recovery_encode(const critical_command &command,
						 const native_mobile_birth_recovery_context &value,
						 std::vector<uint8_t> *output,
						 recovery_policy policy,
						 std::span<const uint8_t> checkpoint = {}) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	try
	{
		std::vector<uint8_t> canonical;
		std::vector<native_mobile_birth_item_recipe> recipes;
		if (!command_values(command, &canonical, &recipes, policy) ||
		    (policy == recovery_policy::shared_shop &&
		     !checkpoint_matches(command, checkpoint)) ||
		    (policy != recovery_policy::shared_shop && !checkpoint.empty()) ||
		    !context_valid(command, recipes, value, policy))
			return error::corrupt_evidence;
		size_t size = HEADER_BYTES + canonical.size() + BODY_BYTES;
		for (const auto &item : value.items)
		{
			if (size > LIMIT - ITEM_BYTES ||
			    item.effects.size() > LIMIT - size - ITEM_BYTES)
				return error::capacity;
			size += ITEM_BYTES + item.effects.size();
		}
		if (policy == recovery_policy::shared_shop)
		{
			if (size > LIMIT - 4 || checkpoint.size() > LIMIT - size - 4)
				return error::capacity;
			size += 4 + checkpoint.size();
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
		if (policy == recovery_policy::shared_shop)
		{
			put(bytes.data() + offset, checkpoint.size(), 4);
			std::copy(checkpoint.begin(), checkpoint.end(), bytes.begin() + offset + 4);
		}
		*output = std::move(bytes);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

static economic_accounting_error recovery_decode(const critical_command &command,
						 std::span<const uint8_t> bytes,
						 native_mobile_birth_recovery_context *output,
						 recovery_policy policy) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	const auto checked = preflight(&command, bytes, &view, policy);
	if (checked != error::ok)
		return checked;
	try
	{
		std::vector<uint8_t> canonical;
		std::vector<native_mobile_birth_item_recipe> recipes;
		if (!command_values(command, &canonical, &recipes, policy) ||
		    !std::equal(canonical.begin(), canonical.end(), view.command.begin(),
				view.command.end()) ||
		    view.count != recipes.size())
			return error::payload_conflict;
		if (policy == recovery_policy::shared_shop &&
		    !checkpoint_matches(command, view.checkpoint))
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
			    { return effect(bytes[view.item_offsets[i] + ITEM_BYTES + e]); },
			    policy))
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

static economic_accounting_error recovery_original_command_decode(std::span<const uint8_t> bytes,
								  critical_command *output,
								  recovery_policy policy) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	// The SAME original parser bounds every header/body/count/effect before
	// even the command codec can allocate. No caller-invented command identity.
	const auto checked = preflight(nullptr, bytes, &view, policy);
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
		if (original.payload_version !=
		    (policy != recovery_policy::historical ?
			     NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION :
			     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION))
			return error::invalid_version;
		native_mobile_birth_recovery_context context;
		const auto valid = recovery_decode(original, bytes, &context, policy);
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
static bool recovery_successor(const critical_native_recovery_envelope &expected,
			       const critical_native_recovery_envelope &successor,
			       recovery_policy policy) noexcept
{
	try
	{
		native_mobile_birth_recovery_context before, after;
		if (expected.revision == UINT64_MAX ||
		    successor.revision != expected.revision + 1 ||
		    !critical_command_equal(expected.command, successor.command) ||
		    !envelope_decode(expected, &before, policy) ||
		    !envelope_decode(successor, &after, policy))
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

economic_accounting_error
native_mobile_birth_recovery_encode(const critical_command &command,
				    const native_mobile_birth_recovery_context &value,
				    std::vector<uint8_t> *output) noexcept
{
	return recovery_encode(command, value, output, recovery_policy::historical);
}
economic_accounting_error
native_mobile_birth_recovery_decode(const critical_command &command, std::span<const uint8_t> bytes,
				    native_mobile_birth_recovery_context *output) noexcept
{
	return recovery_decode(command, bytes, output, recovery_policy::historical);
}
economic_accounting_error
native_mobile_birth_recovery_original_command_decode(std::span<const uint8_t> bytes,
						     critical_command *output) noexcept
{
	return recovery_original_command_decode(bytes, output, recovery_policy::historical);
}
bool native_mobile_birth_recovery_successor(const critical_native_recovery_envelope &before,
					    const critical_native_recovery_envelope &after) noexcept
{
	return recovery_successor(before, after, recovery_policy::historical);
}

economic_accounting_error
native_mobile_birth_cash_role_recovery_encode(const critical_command &command,
					      const native_mobile_birth_recovery_context &value,
					      std::vector<uint8_t> *output) noexcept
{
	return recovery_encode(command, value, output, recovery_policy::ordinary_cash_role);
}
economic_accounting_error
native_mobile_birth_cash_role_recovery_decode(const critical_command &command,
					      std::span<const uint8_t> bytes,
					      native_mobile_birth_recovery_context *output) noexcept
{
	return recovery_decode(command, bytes, output, recovery_policy::ordinary_cash_role);
}
economic_accounting_error
native_mobile_birth_cash_role_recovery_original_command_decode(std::span<const uint8_t> bytes,
							       critical_command *output) noexcept
{
	return recovery_original_command_decode(bytes, output, recovery_policy::ordinary_cash_role);
}
bool native_mobile_birth_cash_role_recovery_valid(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope_decode(envelope, &value, recovery_policy::ordinary_cash_role);
}
bool native_mobile_birth_cash_role_recovery_initial(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope.revision == 1 &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope_decode(envelope, &value, recovery_policy::ordinary_cash_role) &&
	       !value.receipt_present && no_progress(value);
}
bool native_mobile_birth_cash_role_recovery_successor(
	const critical_native_recovery_envelope &before,
	const critical_native_recovery_envelope &after) noexcept
{
	return recovery_successor(before, after, recovery_policy::ordinary_cash_role);
}
bool native_mobile_birth_cash_role_recovery_publication(
	const critical_native_recovery_envelope &envelope,
	const critical_completion &current) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope_decode(envelope, &value, recovery_policy::ordinary_cash_role) &&
	       body_terminal(value) && receipt_core_equal(value.receipt, current);
}
bool native_mobile_birth_cash_role_recovery_terminal(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context value;
	return envelope.revision > 1 &&
	       envelope.phase == critical_native_recovery_phase::continuation_pending &&
	       envelope_decode(envelope, &value, recovery_policy::ordinary_cash_role) &&
	       body_terminal(value);
}

economic_accounting_error native_mobile_birth_shared_shop_recovery_encode(
	const critical_command &command,
	const native_mobile_birth_shared_shop_recovery_context &value,
	std::vector<uint8_t> *output) noexcept
{
	return recovery_encode(command, value.progress, output, recovery_policy::shared_shop,
			       value.original_checkpoint);
}
economic_accounting_error native_mobile_birth_shared_shop_recovery_decode(
	const critical_command &command, std::span<const uint8_t> bytes,
	native_mobile_birth_shared_shop_recovery_context *output) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	wire_view view;
	const auto checked = preflight(&command, bytes, &view, recovery_policy::shared_shop);
	if (checked != error::ok)
		return checked;
	try
	{
		native_mobile_birth_shared_shop_recovery_context value;
		const auto decoded = recovery_decode(command, bytes, &value.progress,
						     recovery_policy::shared_shop);
		if (decoded != error::ok)
			return decoded;
		value.original_checkpoint.assign(view.checkpoint.begin(), view.checkpoint.end());
		static_assert(std::is_nothrow_move_assignable_v<
			      native_mobile_birth_shared_shop_recovery_context>);
		*output = std::move(value);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
economic_accounting_error
native_mobile_birth_shared_shop_recovery_original_command_decode(std::span<const uint8_t> bytes,
								 critical_command *output) noexcept
{
	return recovery_original_command_decode(bytes, output, recovery_policy::shared_shop);
}
bool native_mobile_birth_shared_shop_recovery_valid(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context progress;
	return envelope_decode(envelope, &progress, recovery_policy::shared_shop);
}
bool native_mobile_birth_shared_shop_recovery_initial(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context progress;
	return envelope.revision == 1 &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope_decode(envelope, &progress, recovery_policy::shared_shop) &&
	       !progress.receipt_present && no_progress(progress);
}
bool native_mobile_birth_shared_shop_recovery_successor(
	const critical_native_recovery_envelope &before,
	const critical_native_recovery_envelope &after) noexcept
{
	wire_view a, b;
	return preflight(&before.command, before.attachment, &a, recovery_policy::shared_shop) ==
		       error::ok &&
	       preflight(&after.command, after.attachment, &b, recovery_policy::shared_shop) ==
		       error::ok &&
	       std::equal(a.checkpoint.begin(), a.checkpoint.end(), b.checkpoint.begin(),
			  b.checkpoint.end()) &&
	       recovery_successor(before, after, recovery_policy::shared_shop);
}
bool native_mobile_birth_shared_shop_recovery_publication(
	const critical_native_recovery_envelope &envelope,
	const critical_completion &current) noexcept
{
	native_mobile_birth_recovery_context progress;
	return envelope_decode(envelope, &progress, recovery_policy::shared_shop) &&
	       body_terminal(progress) && receipt_core_equal(progress.receipt, current);
}
bool native_mobile_birth_shared_shop_recovery_terminal(
	const critical_native_recovery_envelope &envelope) noexcept
{
	native_mobile_birth_recovery_context progress;
	return envelope.revision > 1 &&
	       envelope.phase == critical_native_recovery_phase::continuation_pending &&
	       envelope_decode(envelope, &progress, recovery_policy::shared_shop) &&
	       body_terminal(progress);
}

bool native_mobile_birth_shared_shop_recovery_execution_valid(const critical_command &command,
							      std::span<const uint8_t> attachment,
							      uint64_t revision) noexcept
{
	native_mobile_birth_shared_shop_recovery_context value;
	return revision &&
	       native_mobile_birth_shared_shop_recovery_decode(command, attachment, &value) ==
		       error::ok &&
	       (revision != 1 || (!value.progress.receipt_present && no_progress(value.progress)));
}

namespace
{
using recovery_reserve_fn = bool (*)(size_t, void *) noexcept;
bool recovery_add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}
bool recovery_rows(size_t &value, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && recovery_add(value, count * width);
}
bool recovery_admit(size_t base, size_t extra, recovery_reserve_fn reserve, void *context) noexcept
{
	return recovery_add(base, extra) && reserve && reserve(base, context);
}
struct recovery_reservation
{
	recovery_reserve_fn reserve;
	void *context;
	bool refused = false;
	static bool forward(size_t bytes, void *opaque) noexcept
	{
		auto &self = *static_cast<recovery_reservation *>(opaque);
		if (!self.reserve || !self.reserve(bytes, self.context))
		{
			self.refused = true;
			return false;
		}
		return true;
	}
};
bool recovery_storage_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	return true;
#else
	return false;
#endif
}
// Pure original parser: candidate view and stored receipt persist through all
// row checks. read_receipt's local/returned objects and the absent-receipt
// comparison object have sequential peaks. These are named project objects;
// scalar call frames and library-private implementation objects are excluded.
size_t recovery_wire_preflight_bytes() noexcept
{
	const size_t receipt_phase = 2 * sizeof(critical_completion);
	const size_t row_phase = sizeof(native_mobile_birth_recovery_effect) +
				 sizeof(native_mobile_birth_recovery_action);
	const size_t choice_phase = sizeof(native_mobile_birth_recovery_choice);
	return sizeof(wire_view) + sizeof(std::span<const uint8_t>) +
	       std::max(receipt_phase, sizeof(critical_completion) +
					       sizeof(native_mobile_birth_recovery_mobile) +
					       std::max(sizeof(critical_completion),
							std::max(row_phase, choice_phase)));
}
error recovery_preflight_bounded(const critical_command *command,
				 const std::span<const uint8_t> &bytes, wire_view *view,
				 recovery_policy policy, recovery_reserve_fn reserve, void *context,
				 size_t outer) noexcept
{
	if (!recovery_admit(outer, recovery_wire_preflight_bytes(), reserve, context))
		return error::capacity;
	return preflight(command, bytes, view, policy);
}
struct recovery_command_workspace
{
	quest_mobile_native_image image;
	native_mobile_birth_cash_role_recipe role;
};
bool recovery_command_values_bounded(const critical_command &command,
				     std::vector<uint8_t> *canonical,
				     std::vector<native_mobile_birth_item_recipe> *recipes,
				     recovery_policy policy, recovery_reserve_fn reserve,
				     void *context, size_t outer, size_t *recipe_heap)
{
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_command_workspace)) ||
	    !recovery_admit(base, 0, reserve, context))
		return false;
	recovery_command_workspace work;
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    !command.publication_required ||
	    critical_command_encode_bounded(command, canonical, reserve, context, base) !=
		    critical_command_codec_result::ok ||
	    canonical->size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    !recovery_add(base, canonical->capacity()))
		return false;
	size_t image_heap = 0, recipes_heap = 0;
	if (native_mobile_birth_cash_role_command_decode_bounded(
		    command, &work.image, recipes, &work.role, reserve, context, base, &image_heap,
		    &recipes_heap) != error::ok ||
	    work.role.role != (policy == recovery_policy::shared_shop ?
				       native_mobile_birth_cash_role::shared_shopkeeper :
				       native_mobile_birth_cash_role::ordinary_wallet))
		return false;
	*recipe_heap = recipes_heap;
	return true;
}
struct recovery_receipt_workspace
{
	native_mobile_birth_cash_role_result result;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	economic_account_key wallet;
	economic_accounting_plan plan;
	std::span<const uint8_t> payload, intent_bytes;
};
bool recovery_receipt_result_valid_bounded(const critical_command &command, bool present,
					   const critical_completion &receipt,
					   recovery_policy policy, recovery_reserve_fn reserve,
					   void *context, size_t outer)
{
	if (present && receipt.operation_id.bytes != command.operation_id.bytes)
		return false;
	if ((!present && !recovery_admit(outer, sizeof(critical_completion), reserve, context)) ||
	    !receipt_shape(command, present, receipt, policy))
		return false;
	if (!present || !successful(receipt))
		return true;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_receipt_workspace)) ||
	    !recovery_admit(base, 0, reserve, context))
		return false;
	recovery_receipt_workspace work;
	// The original fixed result decoder retains its result, canonical array and
	// encoder array together; encoder fields/clocks and coin conversion coexist.
	const size_t result_decode =
		sizeof(std::span<const uint8_t>) + sizeof(native_mobile_birth_cash_role_result) +
		2 * sizeof(std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
		std::max(sizeof(uint64_t[7]) + sizeof(uint64_t[4]), sizeof(economic_coin_vector));
	if (!recovery_admit(base, result_decode, reserve, context))
		return false;
	work.payload = { receipt.result_payload.data(), receipt.result_size };
	if (!native_mobile_birth_cash_role_result_decode(work.payload, &work.result))
		return false;
	size_t image_heap = 0, recipe_heap = 0, plan_heap = 0;
	if (policy == recovery_policy::shared_shop)
	{
		if (work.result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    receipt.durable_revision !=
			    std::max(uint64_t{ 1 }, work.result.shared.owner_revision_after))
			return false;
		if (native_mobile_birth_cash_role_command_decode_bounded(
			    command, &work.image, &work.recipes, &work.role, reserve, context, base,
			    &image_heap, &recipe_heap) != error::ok ||
		    work.role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    work.result.shared.shop_before_present ||
		    work.result.shared.shop_revision_before != 0 ||
		    !work.result.shared.shop_after_present ||
		    work.result.shared.shop_revision_after != 1)
			return false;
		if (work.image.items.empty())
		{
			if (work.result.shared.owner_before_present !=
				    work.result.shared.owner_after_present ||
			    work.result.shared.owner_revision_before !=
				    work.result.shared.owner_revision_after)
				return false;
		}
		else if (!work.result.shared.owner_after_present ||
			 work.result.shared.owner_revision_before == UINT64_MAX ||
			 work.result.shared.owner_revision_after !=
				 work.result.shared.owner_revision_before + 1)
			return false;
		if (!recovery_add(base, image_heap) || !recovery_add(base, recipe_heap) ||
		    native_mobile_birth_cash_role_accounting_compile_bounded(
			    command, work.result.shared, &work.plan, reserve, context, base,
			    &plan_heap) != error::ok ||
		    !recovery_add(base, plan_heap))
			return false;
		return native_mobile_birth_cash_role_result_matches_bounded(command,
									    work.result.shared,
									    work.plan, work.result,
									    reserve, context, base);
	}
	if (work.result.role != native_mobile_birth_cash_role::ordinary_wallet)
		return false;
	if (!recovery_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return false;
	work.intent_bytes = command.accounting_intent;
	if (economic_intent_decode_bounded(work.intent_bytes, &work.intent, reserve, context,
					   base) != error::ok ||
	    !recovery_add(base, work.intent.admission.facts.capacity()))
		return false;
	work.wallet.lineage = work.intent.admission.metadata.lineage;
	work.wallet.kind = economic_account_kind::wallet;
	work.wallet.authority_id = work.result.wallet_mapping_id;
	work.wallet.context_id = ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT;
	if (native_mobile_birth_cash_role_accounting_compile_bounded(
		    command, work.wallet, &work.plan, reserve, context, base, &plan_heap) !=
		    error::ok ||
	    !recovery_add(base, plan_heap))
		return false;
	return native_mobile_birth_cash_role_result_matches_bounded(
		command, work.wallet, work.plan, work.result, reserve, context, base);
}
bool recovery_string_heap(size_t &heap, const std::string &value) noexcept
{
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && recovery_add(heap, value.capacity() + 1));
}
bool recovery_item_heap(size_t &heap, const std::vector<player_item_snapshot> &items) noexcept
{
	if (!recovery_rows(heap, items.capacity(), sizeof(player_item_snapshot)))
		return false;
	for (const auto &item : items)
	{
		if (!recovery_string_heap(heap, item.name) ||
		    !recovery_string_heap(heap, item.short_description) ||
		    !recovery_string_heap(heap, item.description) ||
		    !recovery_string_heap(heap, item.action_description) ||
		    !recovery_rows(heap, item.dynamic_affects.capacity(),
				   sizeof(player_item_dynamic_affect_snapshot)) ||
		    !recovery_rows(heap, item.extra_descriptions.capacity(),
				   sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &extra : item.extra_descriptions)
			if (!recovery_string_heap(heap, extra.keyword) ||
			    !recovery_string_heap(heap, extra.description) ||
			    !recovery_rows(heap, extra.spell_ids.capacity(), sizeof(int32_t)))
				return false;
	}
	return true;
}
struct recovery_checkpoint_workspace
{
	flatfile_shopkeeper_record record;
	std::vector<uint8_t> encoded, original_items, checkpoint_items;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
};
bool recovery_checkpoint_matches_bounded(const critical_command &command,
					 const std::span<const uint8_t> &bytes,
					 recovery_reserve_fn reserve, void *context, size_t outer)
{
	if (bytes.empty() || bytes.size() > LIMIT)
		return false;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_checkpoint_workspace)) ||
	    !recovery_admit(base, bytes.size(), reserve, context))
		return false;
	recovery_checkpoint_workspace work;
	work.encoded.assign(bytes.begin(), bytes.end());
	if (!recovery_add(base, work.encoded.capacity()) ||
	    flatfile_shopkeeper_initial_checkpoint_decode_bounded(work.encoded, &work.record,
								  reserve, context, base) !=
		    flatfile_shopkeeper_result::ok)
		return false;
	size_t record_heap = 0;
	if (!recovery_rows(record_heap, work.record.affects.capacity(),
			   sizeof(flatfile_shopkeeper_affect_record)) ||
	    !recovery_item_heap(record_heap, work.record.items) || !recovery_add(base, record_heap))
		return false;
	size_t image_heap = 0, recipe_heap = 0;
	if (native_mobile_birth_cash_role_command_decode_bounded(
		    command, &work.image, &work.recipes, &work.role, reserve, context, base,
		    &image_heap, &recipe_heap) != error::ok ||
	    work.role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
	    !work.image.cash ||
	    work.record.shop_id != static_cast<uint32_t>(work.role.original.reset_shop_index) ||
	    work.record.mob_vnum != work.role.original.mobile_vnum ||
	    work.record.room_vnum != work.role.original.reset_room_vnum ||
	    work.record.revision != 1 || work.record.saved_at < 0 || work.record.cash < 0)
		return false;
	int64_t cash = 0;
	if (!recovery_add(base, image_heap) || !recovery_add(base, recipe_heap) ||
	    !recovery_admit(base, sizeof(economic_coin_vector), reserve, context) ||
	    economic_coin_value(work.image.cash->denominations.amount, &cash) != error::ok ||
	    cash != work.record.cash ||
	    player_item_snapshot_list_encode_bounded(work.image.items, &work.original_items,
						     reserve, context,
						     base) != player_snapshot_codec_result::ok ||
	    !recovery_add(base, work.original_items.capacity()) ||
	    player_item_snapshot_list_encode_bounded(work.record.items, &work.checkpoint_items,
						     reserve, context,
						     base) != player_snapshot_codec_result::ok)
		return false;
	return work.original_items == work.checkpoint_items;
}
template <typename GetItem, typename GetCount, typename GetEffect>
bool recovery_context_valid_range_bounded(
	const critical_command &command,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const native_mobile_birth_recovery_context &value, size_t count, const GetItem &get_item,
	const GetCount &get_count, const GetEffect &get_effect, recovery_policy policy,
	recovery_reserve_fn reserve, void *context, size_t outer)
{
	if (value.stage > native_mobile_birth_recovery_stage::physically_proven ||
	    count != recipes.size() || recipes.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !valid_action(value.whole_binding) || !valid_action(value.reference_install) ||
	    (value.mobile_publication.returned && !value.mobile_publication.started) ||
	    (value.mobile_publication.consumed && !value.mobile_publication.started) ||
	    (!recovery_admit(outer,
			     std::max(sizeof(std::array<size_t, 4>),
				      sizeof(native_mobile_birth_recovery_action)),
			     reserve, context) ||
	     !mobile_shape(value)) ||
	    !recovery_receipt_result_valid_bounded(command, value.receipt_present, value.receipt,
						   policy, reserve, context, outer))
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
	// Two no-NRVO read_item DTOs cover the returned temporary and its callee;
	// encode accessors return a reference and make no row/vector copy.
	const size_t row_object = std::is_reference_v<decltype(get_item(size_t{}))> ?
					  0 :
					  sizeof(native_mobile_birth_recovery_item);
	const size_t proof_objects =
		sizeof(std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS>) +
		std::max(2 * row_object,
			 row_object +
				 std::max(sizeof(std::span<const native_mobile_birth_item_recipe>),
					  sizeof(native_mobile_birth_recovery_effect)));
	if (!recovery_admit(outer, proof_objects, reserve, context))
		return false;
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
struct recovery_context_item_access
{
	const native_mobile_birth_recovery_context &value;
	const native_mobile_birth_recovery_item &operator()(size_t i) const noexcept
	{
		return value.items[i];
	}
};
struct recovery_context_count_access
{
	const native_mobile_birth_recovery_context &value;
	size_t operator()(size_t i) const noexcept { return value.items[i].effects.size(); }
};
struct recovery_context_effect_access
{
	const native_mobile_birth_recovery_context &value;
	native_mobile_birth_recovery_effect operator()(size_t i, size_t e) const noexcept
	{
		return value.items[i].effects[e];
	}
};
struct recovery_wire_item_access
{
	const std::span<const uint8_t> &bytes;
	const wire_view &view;
	native_mobile_birth_recovery_item operator()(size_t i) const noexcept
	{
		return read_item(bytes.data() + view.item_offsets[i]);
	}
};
struct recovery_wire_count_access
{
	const std::span<const uint8_t> &bytes;
	const wire_view &view;
	size_t operator()(size_t i) const noexcept
	{
		return static_cast<size_t>(get(bytes.data() + view.item_offsets[i] + 16, 4));
	}
};
struct recovery_wire_effect_access
{
	const std::span<const uint8_t> &bytes;
	const wire_view &view;
	native_mobile_birth_recovery_effect operator()(size_t i, size_t e) const noexcept
	{
		return effect(bytes[view.item_offsets[i] + ITEM_BYTES + e]);
	}
};
struct recovery_encode_workspace
{
	std::vector<uint8_t> canonical, bytes;
	std::vector<native_mobile_birth_item_recipe> recipes;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	std::span<const uint8_t> checkpoint;
};
struct recovery_decode_workspace
{
	wire_view view;
	std::vector<uint8_t> canonical;
	std::vector<native_mobile_birth_item_recipe> recipes;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	native_mobile_birth_recovery_context candidate;
};

error recovery_encode_bounded_impl(const critical_command &command,
				   const native_mobile_birth_recovery_context &value,
				   std::vector<uint8_t> *output, recovery_policy policy,
				   const std::span<const uint8_t> &checkpoint,
				   recovery_reserve_fn reserve, void *context,
				   size_t outer) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_encode_workspace)) ||
	    !recovery_add(base, sizeof(recovery_reservation)) ||
	    !recovery_add(base, sizeof(recovery_context_item_access)) ||
	    !recovery_add(base, sizeof(recovery_context_count_access)) ||
	    !recovery_add(base, sizeof(recovery_context_effect_access)) ||
	    !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		recovery_encode_workspace work;
		work.checkpoint = checkpoint;
		recovery_context_item_access get_item{ value };
		recovery_context_count_access get_count{ value };
		recovery_context_effect_access get_effect{ value };
		size_t recipe_heap = 0;
		if (!recovery_command_values_bounded(command, &work.canonical, &work.recipes,
						     policy, reserve, context, base, &recipe_heap))
			return admission.refused ? error::capacity : error::corrupt_evidence;
		size_t current = base;
		if (!recovery_add(current, work.canonical.capacity()) ||
		    !recovery_add(current, recipe_heap))
			return error::capacity;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		work.recipe_values = work.recipes;
		if ((policy == recovery_policy::shared_shop &&
		     !recovery_checkpoint_matches_bounded(command, work.checkpoint, reserve,
							  context, current)) ||
		    (policy != recovery_policy::shared_shop && !work.checkpoint.empty()) ||
		    !recovery_context_valid_range_bounded(
			    command, work.recipe_values, value, value.items.size(), get_item,
			    get_count, get_effect, policy, reserve, context, current))
			return admission.refused ? error::capacity : error::corrupt_evidence;
		size_t size = HEADER_BYTES + work.canonical.size() + BODY_BYTES;
		for (const auto &item : value.items)
		{
			if (size > LIMIT - ITEM_BYTES ||
			    item.effects.size() > LIMIT - size - ITEM_BYTES)
				return error::capacity;
			size += ITEM_BYTES + item.effects.size();
		}
		if (policy == recovery_policy::shared_shop)
		{
			if (size > LIMIT - 4 || work.checkpoint.size() > LIMIT - size - 4)
				return error::capacity;
			size += 4 + work.checkpoint.size();
		}
		if (!recovery_admit(current, size, reserve, context))
			return error::capacity;
		work.bytes.assign(size, 0);
		auto &bytes = work.bytes;
		put(bytes.data(), MAGIC, 4);
		put(bytes.data() + 4, NATIVE_MOBILE_BIRTH_RECOVERY_VERSION, 2);
		put(bytes.data() + 8, work.canonical.size(), 4);
		std::copy(work.canonical.begin(), work.canonical.end(),
			  bytes.begin() + HEADER_BYTES);
		auto *body = bytes.data() + HEADER_BYTES + work.canonical.size();
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
		size_t offset = HEADER_BYTES + work.canonical.size() + BODY_BYTES;
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
		if (policy == recovery_policy::shared_shop)
		{
			put(bytes.data() + offset, work.checkpoint.size(), 4);
			std::copy(work.checkpoint.begin(), work.checkpoint.end(),
				  bytes.begin() + offset + 4);
		}
		static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
		*output = std::move(work.bytes);
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

error recovery_decode_bounded_impl(const critical_command &command,
				   const std::span<const uint8_t> &bytes,
				   native_mobile_birth_recovery_context *output,
				   recovery_policy policy, recovery_reserve_fn reserve,
				   void *context, size_t outer, size_t *retained_heap) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_decode_workspace)) ||
	    !recovery_add(base, sizeof(recovery_reservation)) ||
	    !recovery_add(base, sizeof(recovery_wire_item_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_count_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_effect_access)) ||
	    !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		recovery_decode_workspace work;
		recovery_wire_item_access get_item{ bytes, work.view };
		recovery_wire_count_access get_count{ bytes, work.view };
		recovery_wire_effect_access get_effect{ bytes, work.view };
		const auto checked = recovery_preflight_bounded(&command, bytes, &work.view, policy,
								reserve, context, base);
		if (checked != error::ok)
			return checked;
		size_t recipe_heap = 0;
		if (!recovery_command_values_bounded(command, &work.canonical, &work.recipes,
						     policy, reserve, context, base,
						     &recipe_heap) ||
		    !std::equal(work.canonical.begin(), work.canonical.end(),
				work.view.command.begin(), work.view.command.end()) ||
		    work.view.count != work.recipes.size())
			return admission.refused ? error::capacity : error::payload_conflict;
		size_t current = base;
		if (!recovery_add(current, work.canonical.capacity()) ||
		    !recovery_add(current, recipe_heap))
			return error::capacity;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		work.recipe_values = work.recipes;
		if (policy == recovery_policy::shared_shop &&
		    !recovery_checkpoint_matches_bounded(command, work.view.checkpoint, reserve,
							 context, current))
			return admission.refused ? error::capacity : error::payload_conflict;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		// The complete wire bounds preceded all allocation. Check recipe-derived
		// counts before creating any attachment container, not only afterwards.
		size_t offset = work.view.items_offset;
		for (size_t i = 0; i < work.recipes.size(); ++i)
		{
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			const size_t index =
				recipe_index(work.recipe_values, get(bytes.data() + offset, 8));
			if (index == work.recipes.size() ||
			    count != step_count(work.recipes[index]))
				return error::corrupt_evidence;
			offset += ITEM_BYTES + count;
		}
		if (!recovery_admit(current, 2 * sizeof(critical_completion), reserve, context))
			return error::capacity;
		read_body(work.view.body, &work.candidate);
		if (!recovery_context_valid_range_bounded(
			    command, work.recipe_values, work.candidate, work.view.count, get_item,
			    get_count, get_effect, policy, reserve, context, current))
			return admission.refused ? error::capacity : error::corrupt_evidence;
		// All attachment semantics, including ordering, cursor/latches and exact
		// recipe/result correlation, now passed before attachment containers allocate.
		size_t retained = 0;
		if (!recovery_rows(retained, work.view.count,
				   sizeof(native_mobile_birth_recovery_item)) ||
		    !recovery_admit(current, retained, reserve, context))
			return error::capacity;
		work.candidate.items.reserve(work.view.count);
		offset = work.view.items_offset;
		for (size_t i = 0; i < work.view.count; ++i)
		{
			size_t row_live = current;
			if (!recovery_add(row_live, retained) ||
			    !recovery_admit(row_live, 2 * sizeof(native_mobile_birth_recovery_item),
					    reserve, context))
				return error::capacity;
			auto item = read_item(bytes.data() + offset);
			// Already proved against this UID's recipe before any container allocation.
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			offset += ITEM_BYTES;
			size_t effect_heap = 0, effect_live = row_live;
			if (!recovery_rows(effect_heap, count,
					   sizeof(native_mobile_birth_recovery_effect)) ||
			    !recovery_add(effect_live, sizeof(native_mobile_birth_recovery_item)) ||
			    !recovery_add(effect_live, effect_heap) ||
			    !recovery_admit(effect_live,
					    sizeof(native_mobile_birth_recovery_effect), reserve,
					    context))
				return error::capacity;
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset++]));
			work.candidate.items.push_back(std::move(item));
			if (!recovery_add(retained, effect_heap))
				return error::capacity;
		}
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_recovery_context>);
		*output = std::move(work.candidate);
		if (retained_heap)
			*retained_heap = retained;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
} // namespace

economic_accounting_error native_mobile_birth_cash_role_recovery_encode_bounded(
	const critical_command &command, const native_mobile_birth_recovery_context &value,
	std::vector<uint8_t> *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	// Actual named empty input span remains live through the private encoder.
	if (!recovery_admit(outer_live, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	const std::span<const uint8_t> checkpoint;
	return recovery_encode_bounded_impl(command, value, output,
					    recovery_policy::ordinary_cash_role, checkpoint,
					    reserve, context, outer_live + sizeof(checkpoint));
}
economic_accounting_error native_mobile_birth_cash_role_recovery_decode_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_recovery_context *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live, size_t *retained_context_heap_bytes) noexcept
{
	return recovery_decode_bounded_impl(command, bytes, output,
					    recovery_policy::ordinary_cash_role, reserve, context,
					    outer_live, retained_context_heap_bytes);
}
economic_accounting_error native_mobile_birth_shared_shop_recovery_encode_bounded(
	const critical_command &command,
	const native_mobile_birth_shared_shop_recovery_context &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_admit(outer_live, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	const std::span<const uint8_t> checkpoint(value.original_checkpoint);
	return recovery_encode_bounded_impl(command, value.progress, output,
					    recovery_policy::shared_shop, checkpoint, reserve,
					    context, outer_live + sizeof(checkpoint));
}
economic_accounting_error native_mobile_birth_shared_shop_recovery_decode_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_shared_shop_recovery_context *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_context_heap_bytes) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	struct workspace
	{
		wire_view view;
		native_mobile_birth_shared_shop_recovery_context candidate;
	};
	size_t base = outer_live;
	if (!recovery_add(base, sizeof(workspace)) ||
	    !recovery_add(base, sizeof(recovery_reservation)) ||
	    !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		workspace work;
		// Preserve the shared wrapper's first full preflight and the inner decoder's
		// second complete preflight/command/receipt/progress proof in original order.
		const auto checked = recovery_preflight_bounded(&command, bytes, &work.view,
								recovery_policy::shared_shop,
								reserve, context, base);
		if (checked != error::ok)
			return checked;
		size_t retained = 0;
		const auto decoded = recovery_decode_bounded_impl(
			command, bytes, &work.candidate.progress, recovery_policy::shared_shop,
			reserve, context, base, &retained);
		if (decoded != error::ok)
			return decoded;
		size_t current = base;
		if (!recovery_add(current, retained) ||
		    !recovery_admit(current, work.view.checkpoint.size(), reserve, context))
			return error::capacity;
		work.candidate.original_checkpoint.assign(work.view.checkpoint.begin(),
							  work.view.checkpoint.end());
		if (!recovery_add(retained, work.candidate.original_checkpoint.capacity()))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<
			      native_mobile_birth_shared_shop_recovery_context>);
		*output = std::move(work.candidate);
		if (retained_context_heap_bytes)
			*retained_context_heap_bytes = retained;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
namespace
{
bool recovery_envelope_valid_bounded(const critical_native_recovery_envelope &envelope,
				     recovery_policy policy, unsigned predicate,
				     recovery_reserve_fn reserve, void *context,
				     size_t outer) noexcept
{
	// Original initial/terminal wrappers short-circuit their envelope guards
	// before any decode; ordinary validity preserves the full phase predicate.
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    (predicate == 1 &&
	     (envelope.revision != 1 ||
	      envelope.phase != critical_native_recovery_phase::execution_pending)) ||
	    (predicate == 2 &&
	     (envelope.revision <= 1 ||
	      envelope.phase != critical_native_recovery_phase::continuation_pending)))
		return false;
	struct workspace
	{
		native_mobile_birth_recovery_context progress;
		native_mobile_birth_shared_shop_recovery_context shared;
		std::span<const uint8_t> attachment;
	};
	size_t base = outer;
	if (!recovery_add(base, sizeof(workspace)) || !recovery_admit(base, 0, reserve, context))
		return false;
	workspace work;
	if (!recovery_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return false;
	work.attachment = envelope.attachment;
	if (policy == recovery_policy::shared_shop)
	{
		if (native_mobile_birth_shared_shop_recovery_decode_bounded(
			    envelope.command, work.attachment, &work.shared, reserve, context,
			    base) != error::ok)
			return false;
		work.progress = std::move(work.shared.progress);
	}
	else if (native_mobile_birth_cash_role_recovery_decode_bounded(
			 envelope.command, work.attachment, &work.progress, reserve, context,
			 base) != error::ok)
		return false;
	return (envelope.revision != 1 ||
		(envelope.phase == critical_native_recovery_phase::execution_pending &&
		 !work.progress.receipt_present && no_progress(work.progress))) &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		body_terminal(work.progress)) &&
	       (predicate != 1 || (!work.progress.receipt_present && no_progress(work.progress))) &&
	       (predicate != 2 || body_terminal(work.progress));
}
} // namespace
bool native_mobile_birth_cash_role_recovery_valid_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::ordinary_cash_role, 0,
					       reserve, context, outer_live);
}
bool native_mobile_birth_cash_role_recovery_initial_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::ordinary_cash_role, 1,
					       reserve, context, outer_live);
}
bool native_mobile_birth_cash_role_recovery_terminal_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::ordinary_cash_role, 2,
					       reserve, context, outer_live);
}
bool native_mobile_birth_shared_shop_recovery_valid_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::shared_shop, 0, reserve,
					       context, outer_live);
}
bool native_mobile_birth_shared_shop_recovery_initial_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::shared_shop, 1, reserve,
					       context, outer_live);
}
bool native_mobile_birth_shared_shop_recovery_terminal_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	return recovery_envelope_valid_bounded(envelope, recovery_policy::shared_shop, 2, reserve,
					       context, outer_live);
}

bool native_mobile_birth_shared_shop_recovery_execution_valid_bounded(
	const critical_command &command, const std::span<const uint8_t> &attachment,
	uint64_t revision, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	// Full original nonzero revision and shared decode/no-progress proof. This
	// execution predicate deliberately grants no continuation/terminal phase gate.
	if (!revision)
		return false;
	struct execution_workspace
	{
		native_mobile_birth_shared_shop_recovery_context value;
		std::span<const uint8_t> attachment;
	};
	size_t base = outer_live;
	if (!recovery_add(base, sizeof(execution_workspace)) || !recovery_add(base, sizeof(base)) ||
	    !recovery_admit(base, 0, reserve, context))
		return false;
	execution_workspace work;
	work.attachment = attachment;
	return native_mobile_birth_shared_shop_recovery_decode_bounded(
		       command, work.attachment, &work.value, reserve, context, base) ==
		       economic_accounting_error::ok &&
	       (revision != 1 ||
		(!work.value.progress.receipt_present && no_progress(work.value.progress)));
}

namespace
{

struct historical_initial_command_workspace
{
	quest_mobile_native_image image;
};
bool historical_initial_command_values_bounded(
	const critical_command &command, std::vector<uint8_t> *canonical,
	std::vector<native_mobile_birth_item_recipe> *recipes, recovery_reserve_fn reserve,
	void *context, size_t outer, size_t *recipe_heap)
{
	size_t base = outer;
	if (!recovery_add(base, sizeof(historical_initial_command_workspace)) ||
	    !recovery_admit(base, 0, reserve, context))
		return false;
	historical_initial_command_workspace work;
	if ((command.payload_version != NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION &&
	     command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION) ||
	    !command.publication_required ||
	    critical_command_encode_bounded(command, canonical, reserve, context, base) !=
		    critical_command_codec_result::ok ||
	    canonical->size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    !recovery_add(base, canonical->capacity()))
		return false;
	size_t image_heap = 0, recipes_heap = 0;
	if (native_mobile_birth_command_decode_bounded(command, &work.image, recipes, reserve,
						       context, base, &image_heap,
						       &recipes_heap) != error::ok)
		return false;
	*recipe_heap = recipes_heap;
	return true;
}

error historical_initial_decode_bounded(const critical_command &command,
					const std::span<const uint8_t> &bytes,
					native_mobile_birth_recovery_context *output,
					recovery_reserve_fn reserve, void *context, size_t outer,
					size_t *retained_heap) noexcept
{
	constexpr auto policy = recovery_policy::historical;
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_decode_workspace)) ||
	    !recovery_add(base, sizeof(recovery_reservation)) ||
	    !recovery_add(base, sizeof(recovery_wire_item_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_count_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_effect_access)) ||
	    !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		recovery_decode_workspace work;
		recovery_wire_item_access get_item{ bytes, work.view };
		recovery_wire_count_access get_count{ bytes, work.view };
		recovery_wire_effect_access get_effect{ bytes, work.view };
		const auto checked = recovery_preflight_bounded(&command, bytes, &work.view, policy,
								reserve, context, base);
		if (checked != error::ok)
			return checked;
		// INITIAL only. A successful receipt requires a separate historical result
		// compiler; refuse every receipt before entering the bounded progress path.
		if (work.view.body[0])
			return error::corrupt_evidence;
		size_t recipe_heap = 0;
		if (!historical_initial_command_values_bounded(command, &work.canonical,
							       &work.recipes, reserve, context,
							       base, &recipe_heap) ||
		    !std::equal(work.canonical.begin(), work.canonical.end(),
				work.view.command.begin(), work.view.command.end()) ||
		    work.view.count != work.recipes.size())
			return admission.refused ? error::capacity : error::payload_conflict;
		size_t current = base;
		if (!recovery_add(current, work.canonical.capacity()) ||
		    !recovery_add(current, recipe_heap))
			return error::capacity;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		work.recipe_values = work.recipes;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		// The complete wire bounds preceded all allocation. Check recipe-derived
		// counts before creating any attachment container, not only afterwards.
		size_t offset = work.view.items_offset;
		for (size_t i = 0; i < work.recipes.size(); ++i)
		{
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			const size_t index =
				recipe_index(work.recipe_values, get(bytes.data() + offset, 8));
			if (index == work.recipes.size() ||
			    count != step_count(work.recipes[index]))
				return error::corrupt_evidence;
			offset += ITEM_BYTES + count;
		}
		if (!recovery_admit(current, 2 * sizeof(critical_completion), reserve, context))
			return error::capacity;
		read_body(work.view.body, &work.candidate);
		if (!recovery_context_valid_range_bounded(
			    command, work.recipe_values, work.candidate, work.view.count, get_item,
			    get_count, get_effect, policy, reserve, context, current))
			return admission.refused ? error::capacity : error::corrupt_evidence;
		// All attachment semantics, including ordering, cursor/latches and exact
		// recipe/result correlation, now passed before attachment containers allocate.
		size_t retained = 0;
		if (!recovery_rows(retained, work.view.count,
				   sizeof(native_mobile_birth_recovery_item)) ||
		    !recovery_admit(current, retained, reserve, context))
			return error::capacity;
		work.candidate.items.reserve(work.view.count);
		offset = work.view.items_offset;
		for (size_t i = 0; i < work.view.count; ++i)
		{
			size_t row_live = current;
			if (!recovery_add(row_live, retained) ||
			    !recovery_admit(row_live, 2 * sizeof(native_mobile_birth_recovery_item),
					    reserve, context))
				return error::capacity;
			auto item = read_item(bytes.data() + offset);
			// Already proved against this UID's recipe before any container allocation.
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			offset += ITEM_BYTES;
			size_t effect_heap = 0, effect_live = row_live;
			if (!recovery_rows(effect_heap, count,
					   sizeof(native_mobile_birth_recovery_effect)) ||
			    !recovery_add(effect_live, sizeof(native_mobile_birth_recovery_item)) ||
			    !recovery_add(effect_live, effect_heap) ||
			    !recovery_admit(effect_live,
					    sizeof(native_mobile_birth_recovery_effect), reserve,
					    context))
				return error::capacity;
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset++]));
			work.candidate.items.push_back(std::move(item));
			if (!recovery_add(retained, effect_heap))
				return error::capacity;
		}
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_recovery_context>);
		*output = std::move(work.candidate);
		if (retained_heap)
			*retained_heap = retained;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
} // namespace

bool native_mobile_birth_recovery_initial_bounded(const critical_native_recovery_envelope &envelope,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	if (envelope.revision != 1 ||
	    envelope.phase != critical_native_recovery_phase::execution_pending)
		return false;
	struct initial_workspace
	{
		native_mobile_birth_recovery_context value;
		std::span<const uint8_t> attachment;
	};
	size_t base = outer_live;
	if (!recovery_add(base, sizeof(initial_workspace)) ||
	    !recovery_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
		return false;
	initial_workspace work;
	work.attachment = envelope.attachment;
	return historical_initial_decode_bounded(envelope.command, work.attachment, &work.value,
						 reserve, context, base, nullptr) == error::ok &&
	       !work.value.receipt_present && no_progress(work.value);
}

namespace
{
// Only ordinary resource/profile outcomes change classification. Every other
// failed proof retains its original enclosing command/receipt error category.
error recovery_ordinary_proof_status(error code, error invalid) noexcept
{
	if (code == error::capacity || code == error::overflow)
		return error::capacity;
	if (code == error::unresolved)
		return error::unresolved;
	return code == error::ok ? error::ok : invalid;
}
struct recovery_ordinary_command_workspace
{
	recovery_command_workspace decoded;
	size_t base = 0, image_heap = 0, recipes_heap = 0;
	critical_command_codec_result encoded = critical_command_codec_result::invalid;
	error status = error::ok;
};
error recovery_ordinary_command_values_status_bounded(
	const critical_command &command, std::vector<uint8_t> *canonical,
	std::vector<native_mobile_birth_item_recipe> *recipes, recovery_reserve_fn reserve,
	void *context, size_t outer, size_t *recipe_heap)
{
	// The workspace owns the actual decoder DTO, both enum results and heap
	// scalars. Two mapping enum parameters are prospective nested source frames.
	// The caller already retains the output vector objects and old heap.
	if (!recovery_admit(outer, sizeof(recovery_ordinary_command_workspace) + 2 * sizeof(error),
			    reserve, context))
		return error::capacity;
	recovery_ordinary_command_workspace work;
	work.base = outer;
	if (!recovery_add(work.base, sizeof(work)) || !recovery_add(work.base, 2 * sizeof(error)))
		return error::capacity;
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    !command.publication_required)
		return error::payload_conflict;
	work.encoded =
		critical_command_encode_bounded(command, canonical, reserve, context, work.base);
	if (work.encoded == critical_command_codec_result::overflow)
		return error::capacity;
	if (work.encoded != critical_command_codec_result::ok ||
	    canonical->size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
		return error::payload_conflict;
	if (!recovery_add(work.base, canonical->capacity()))
		return error::capacity;
	work.status = native_mobile_birth_cash_role_command_decode_bounded(
		command, &work.decoded.image, recipes, &work.decoded.role, reserve, context,
		work.base, &work.image_heap, &work.recipes_heap);
	if (work.status != error::ok)
		return recovery_ordinary_proof_status(work.status, error::payload_conflict);
	if (work.decoded.role.role != native_mobile_birth_cash_role::ordinary_wallet)
		return error::payload_conflict;
	*recipe_heap = work.recipes_heap;
	return error::ok;
}
struct recovery_ordinary_receipt_workspace
{
	native_mobile_birth_cash_role_result result, expected;
	economic_frozen_intent intent;
	economic_account_key wallet;
	economic_accounting_plan plan;
	std::span<const uint8_t> payload, intent_bytes;
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> actual_bytes{},
		expected_bytes{};
	size_t base = 0, plan_heap = 0;
	error status = error::ok;
};
error recovery_ordinary_receipt_status_bounded(const critical_command &command, bool present,
					       const critical_completion &receipt,
					       recovery_reserve_fn reserve, void *context,
					       size_t outer)
{
	if (present && receipt.operation_id.bytes != command.operation_id.bytes)
		return error::corrupt_evidence;
	if (!present && !recovery_admit(outer, sizeof(critical_completion), reserve, context))
		return error::capacity;
	if (!receipt_shape(command, present, receipt, recovery_policy::ordinary_cash_role))
		return error::corrupt_evidence;
	if (!present || !successful(receipt))
		return error::ok;
	if (!recovery_admit(outer, sizeof(recovery_ordinary_receipt_workspace) + 2 * sizeof(error),
			    reserve, context))
		return error::capacity;
	recovery_ordinary_receipt_workspace work;
	work.base = outer;
	if (!recovery_add(work.base, sizeof(work)) || !recovery_add(work.base, 2 * sizeof(error)))
		return error::capacity;
	// The complete fixed decoder retains its own result/canonical/encoder DTOs.
	// These coexist with both strong result outputs and equality arrays in work.
	if (!recovery_admit(
		    work.base,
		    sizeof(std::span<const uint8_t>) +
			    sizeof(native_mobile_birth_cash_role_result) +
			    2 * sizeof(std::array<uint8_t,
						  NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
			    std::max(sizeof(uint64_t[7]) + sizeof(uint64_t[4]),
				     sizeof(economic_coin_vector)),
		    reserve, context))
		return error::capacity;
	work.payload = { receipt.result_payload.data(), receipt.result_size };
	if (!native_mobile_birth_cash_role_result_decode(work.payload, &work.result) ||
	    work.result.role != native_mobile_birth_cash_role::ordinary_wallet)
		return error::corrupt_evidence;
	if (!recovery_admit(work.base, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	work.intent_bytes = command.accounting_intent;
	work.status = economic_intent_decode_bounded(work.intent_bytes, &work.intent, reserve,
						     context, work.base);
	if (work.status != error::ok)
		return recovery_ordinary_proof_status(work.status, error::corrupt_evidence);
	if (!recovery_add(work.base, work.intent.admission.facts.capacity()))
		return error::capacity;
	work.wallet.lineage = work.intent.admission.metadata.lineage;
	work.wallet.kind = economic_account_kind::wallet;
	work.wallet.authority_id = work.result.wallet_mapping_id;
	work.wallet.context_id = ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT;
	work.status = native_mobile_birth_cash_role_accounting_compile_bounded(
		command, work.wallet, &work.plan, reserve, context, work.base, &work.plan_heap);
	if (work.status != error::ok)
		return recovery_ordinary_proof_status(work.status, error::corrupt_evidence);
	if (!recovery_add(work.base, work.plan_heap))
		return error::capacity;
	// Match the original complete builder and canonical equality, retaining its
	// enum before comparison. This includes command/image/role and plan digests.
	work.status = native_mobile_birth_cash_role_result_build_bounded(
		command, work.wallet, work.plan, &work.expected, reserve, context, work.base);
	if (work.status != error::ok)
		return recovery_ordinary_proof_status(work.status, error::corrupt_evidence);
	// a,b are already real admitted workspace members; the encoder's own bytes,
	// complete fields and clocks remain live together in either sequential call.
	if (!recovery_admit(work.base,
			    sizeof(std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
				    sizeof(uint64_t[7]) + sizeof(uint64_t[4]),
			    reserve, context))
		return error::capacity;
	if (!native_mobile_birth_cash_role_result_encode(work.expected, &work.expected_bytes) ||
	    !native_mobile_birth_cash_role_result_encode(work.result, &work.actual_bytes) ||
	    work.expected_bytes != work.actual_bytes)
		return error::corrupt_evidence;
	return error::ok;
}
template <typename GetItem, typename GetCount, typename GetEffect>
error recovery_ordinary_context_status_bounded(
	const critical_command &command,
	const std::span<const native_mobile_birth_item_recipe> &recipes,
	const native_mobile_birth_recovery_context &value, size_t count, const GetItem &get_item,
	const GetCount &get_count, const GetEffect &get_effect, recovery_reserve_fn reserve,
	void *context, size_t outer)
{
	// Admit the actual status scalar before it exists. Original row DTO/lambda
	// and mobile/effect proof frames remain separately admitted below.
	if (!recovery_admit(outer, sizeof(error), reserve, context) ||
	    !recovery_add(outer, sizeof(error)))
		return error::capacity;
	if (value.stage > native_mobile_birth_recovery_stage::physically_proven ||
	    count != recipes.size() || recipes.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !valid_action(value.whole_binding) || !valid_action(value.reference_install) ||
	    (value.mobile_publication.returned && !value.mobile_publication.started) ||
	    (value.mobile_publication.consumed && !value.mobile_publication.started))
		return error::corrupt_evidence;
	if (!recovery_admit(outer,
			    std::max(sizeof(std::array<size_t, 4>),
				     sizeof(native_mobile_birth_recovery_action)),
			    reserve, context))
		return error::capacity;
	if (!mobile_shape(value))
		return error::corrupt_evidence;
	const auto receipt_status = recovery_ordinary_receipt_status_bounded(
		command, value.receipt_present, value.receipt, reserve, context, outer);
	if (receipt_status != error::ok)
		return receipt_status;
	if ((!value.receipt_present || !successful(value.receipt)) && !no_progress(value))
		return error::corrupt_evidence;
	if (value.stage == native_mobile_birth_recovery_stage::captured && !no_progress(value))
		return error::corrupt_evidence;
	if (value.reference_install.started && !complete(value.whole_binding))
		return error::corrupt_evidence;
	bool all_published = true, previous_item_done = true, all_done = true;
	const bool must_zero = !value.receipt_present || !successful(value.receipt) ||
			       value.stage == native_mobile_birth_recovery_stage::captured;
	// Two no-NRVO read_item DTOs cover the returned temporary and its callee;
	// encode accessors return a reference and make no row/vector copy.
	const size_t row_object = std::is_reference_v<decltype(get_item(size_t{}))> ?
					  0 :
					  sizeof(native_mobile_birth_recovery_item);
	const size_t proof_objects =
		sizeof(std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS>) +
		std::max(2 * row_object,
			 row_object +
				 std::max(sizeof(std::span<const native_mobile_birth_item_recipe>),
					  sizeof(native_mobile_birth_recovery_effect)));
	if (!recovery_admit(outer, proof_objects, reserve, context))
		return error::capacity;
	std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS> seen{};
	for (size_t i = 0; i < count; ++i)
	{
		const auto &item = get_item(i);
		const size_t effect_count = get_count(i);
		const size_t index = recipe_index(recipes, item.object_uid);
		// Count equality plus complete unique membership binds every image UID,
		// while these prefix guards retain the actual original publication order.
		if (index == recipes.size() || seen[index])
			return error::corrupt_evidence;
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
			return error::corrupt_evidence;
		all_published = all_published && item.published;
		previous_item_done = item_done_fields(item, effect_count);
		all_done = all_done && previous_item_done;
	}
	if ((value.mobile_publication.started &&
	     (!all_published || !complete(value.reference_install))) ||
	    (value.runtime_applied && (!complete(value.mobile_publication) || !all_done)))
		return error::corrupt_evidence;
	return value.stage != native_mobile_birth_recovery_stage::physically_proven ||
			       (value.receipt_present && successful(value.receipt) &&
				complete(value.whole_binding) &&
				complete(value.reference_install) &&
				complete(value.mobile_publication) && value.runtime_applied &&
				all_done) ?
		       error::ok :
		       error::corrupt_evidence;
}
error recovery_ordinary_decode_status_bounded(const critical_command &command,
					      const std::span<const uint8_t> &bytes,
					      native_mobile_birth_recovery_context *output,
					      recovery_reserve_fn reserve, void *context,
					      size_t outer, size_t *retained_heap) noexcept
{
	constexpr auto policy = recovery_policy::ordinary_cash_role;
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	size_t base = outer;
	if (!recovery_add(base, sizeof(recovery_decode_workspace)) ||
	    !recovery_add(base, sizeof(recovery_reservation)) ||
	    !recovery_add(base, sizeof(recovery_wire_item_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_count_access)) ||
	    !recovery_add(base, sizeof(recovery_wire_effect_access)) ||
	    !recovery_add(base, 3 * sizeof(error) + sizeof(recovery_policy)) ||
	    !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		recovery_decode_workspace work;
		recovery_wire_item_access get_item{ bytes, work.view };
		recovery_wire_count_access get_count{ bytes, work.view };
		recovery_wire_effect_access get_effect{ bytes, work.view };
		const auto checked = recovery_preflight_bounded(&command, bytes, &work.view, policy,
								reserve, context, base);
		if (checked != error::ok)
			return checked;
		size_t recipe_heap = 0;
		const auto command_status = recovery_ordinary_command_values_status_bounded(
			command, &work.canonical, &work.recipes, reserve, context, base,
			&recipe_heap);
		if (admission.refused)
			return error::capacity;
		if (command_status != error::ok)
			return command_status;
		if (!std::equal(work.canonical.begin(), work.canonical.end(),
				work.view.command.begin(), work.view.command.end()) ||
		    work.view.count != work.recipes.size())
			return error::payload_conflict;
		size_t current = base;
		if (!recovery_add(current, work.canonical.capacity()) ||
		    !recovery_add(current, recipe_heap))
			return error::capacity;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		work.recipe_values = work.recipes;
		if (!recovery_admit(current,
				    sizeof(std::span<const native_mobile_birth_item_recipe>),
				    reserve, context))
			return error::capacity;
		// The complete wire bounds preceded all allocation. Check recipe-derived
		// counts before creating any attachment container, not only afterwards.
		size_t offset = work.view.items_offset;
		for (size_t i = 0; i < work.recipes.size(); ++i)
		{
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			const size_t index =
				recipe_index(work.recipe_values, get(bytes.data() + offset, 8));
			if (index == work.recipes.size() ||
			    count != step_count(work.recipes[index]))
				return error::corrupt_evidence;
			offset += ITEM_BYTES + count;
		}
		if (!recovery_admit(current, 2 * sizeof(critical_completion), reserve, context))
			return error::capacity;
		read_body(work.view.body, &work.candidate);
		const auto context_status = recovery_ordinary_context_status_bounded(
			command, work.recipe_values, work.candidate, work.view.count, get_item,
			get_count, get_effect, reserve, context, current);
		if (admission.refused)
			return error::capacity;
		if (context_status != error::ok)
			return context_status;
		// All attachment semantics, including ordering, cursor/latches and exact
		// recipe/result correlation, now passed before attachment containers allocate.
		size_t retained = 0;
		if (!recovery_rows(retained, work.view.count,
				   sizeof(native_mobile_birth_recovery_item)) ||
		    !recovery_admit(current, retained, reserve, context))
			return error::capacity;
		work.candidate.items.reserve(work.view.count);
		offset = work.view.items_offset;
		for (size_t i = 0; i < work.view.count; ++i)
		{
			size_t row_live = current;
			if (!recovery_add(row_live, retained) ||
			    !recovery_admit(row_live, 2 * sizeof(native_mobile_birth_recovery_item),
					    reserve, context))
				return error::capacity;
			auto item = read_item(bytes.data() + offset);
			// Already proved against this UID's recipe before any container allocation.
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 16, 4));
			offset += ITEM_BYTES;
			size_t effect_heap = 0, effect_live = row_live;
			if (!recovery_rows(effect_heap, count,
					   sizeof(native_mobile_birth_recovery_effect)) ||
			    !recovery_add(effect_live, sizeof(native_mobile_birth_recovery_item)) ||
			    !recovery_add(effect_live, effect_heap) ||
			    !recovery_admit(effect_live,
					    sizeof(native_mobile_birth_recovery_effect), reserve,
					    context))
				return error::capacity;
			item.effects.reserve(count);
			for (size_t e = 0; e < count; ++e)
				item.effects.push_back(effect(bytes[offset++]));
			work.candidate.items.push_back(std::move(item));
			if (!recovery_add(retained, effect_heap))
				return error::capacity;
		}
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_recovery_context>);
		*output = std::move(work.candidate);
		if (retained_heap)
			*retained_heap = retained;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
} // namespace

economic_accounting_error native_mobile_birth_cash_role_recovery_decode_status_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_recovery_context *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live, size_t *retained_context_heap_bytes) noexcept
{
	return recovery_ordinary_decode_status_bounded(command, bytes, output, reserve, context,
						       outer_live, retained_context_heap_bytes);
}

economic_accounting_error native_mobile_birth_cash_role_recovery_validate_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	// Complete original ordinary validity predicate, preserving the decoded
	// error rather than flattening resource/profile refusals into false.
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending))
		return economic_accounting_error::corrupt_evidence;
	struct workspace
	{
		native_mobile_birth_recovery_context progress;
		std::span<const uint8_t> attachment;
	};
	// Actual wrapper objects, references, scalar results and full predicates.
	// Nested decoder admits its own complete temporary storage; outer retains
	// input command/attachment, prior caller output and borrowed lock storage.
	constexpr size_t own = sizeof(workspace) + 4 * sizeof(void *) + 3 * sizeof(size_t) +
			       sizeof(economic_accounting_error) + 5 * sizeof(bool);
	size_t base = outer_live;
	if (!recovery_add(base, own) || !recovery_admit(base, 0, reserve, context))
		return economic_accounting_error::capacity;
	workspace work;
	work.attachment = envelope.attachment;
	const auto decoded = native_mobile_birth_cash_role_recovery_decode_status_bounded(
		envelope.command, work.attachment, &work.progress, reserve, context, base);
	if (decoded != economic_accounting_error::ok)
		return decoded;
	if ((envelope.revision == 1 &&
	     (envelope.phase != critical_native_recovery_phase::execution_pending ||
	      work.progress.receipt_present || !no_progress(work.progress))) ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     !body_terminal(work.progress)))
		return economic_accounting_error::corrupt_evidence;
	return economic_accounting_error::ok;
}

economic_accounting_error
native_mobile_birth_cash_role_recovery_original_command_decode_status_bounded(
	std::span<const uint8_t> bytes, critical_command *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_command_heap_bytes) noexcept
{
	// Reuse the SAME complete original preflight before the command codec can
	// allocate. This is the original terminal extraction, not a framing parser.
	if (!output)
		return error::corrupt_evidence;
	if (!recovery_storage_policy())
		return error::unresolved;
	struct workspace
	{
		wire_view view;
		critical_command original;
		native_mobile_birth_recovery_context progress;
		size_t command_heap = 0, context_heap = 0, live = 0;
		error checked = error::ok;
		critical_command_codec_result decoded = critical_command_codec_result::ok;
	};
	size_t base = outer_live;
	constexpr size_t own = sizeof(workspace) + sizeof(recovery_reservation) +
			       5 * sizeof(void *) + 3 * sizeof(size_t) +
			       sizeof(std::span<const uint8_t>);
	if (!recovery_add(base, own) || !recovery_admit(base, 0, reserve, context))
		return error::capacity;
	recovery_reservation admission{ reserve, context };
	reserve = &recovery_reservation::forward;
	context = &admission;
	try
	{
		workspace work;
		work.checked = recovery_preflight_bounded(nullptr, bytes, &work.view,
							  recovery_policy::ordinary_cash_role,
							  reserve, context, base);
		if (work.checked != error::ok)
			return work.checked;
		work.decoded = critical_command_decode_bounded(work.view.command.data(),
							       work.view.command.size(),
							       &work.original, reserve, context,
							       base, &work.command_heap);
		if (admission.refused)
			return error::capacity;
		if (work.decoded != critical_command_codec_result::ok)
			return work.decoded == critical_command_codec_result::overflow ?
				       error::capacity :
			       work.decoded == critical_command_codec_result::unsupported_version ?
				       error::invalid_version :
				       error::corrupt_evidence;
		if (work.original.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
			return error::invalid_version;
		work.live = base;
		if (!recovery_add(work.live, work.command_heap))
			return error::capacity;
		work.checked = native_mobile_birth_cash_role_recovery_decode_status_bounded(
			work.original, bytes, &work.progress, reserve, context, work.live,
			&work.context_heap);
		if (admission.refused)
			return error::capacity;
		if (work.checked != error::ok)
			return work.checked;
		// The real decoded context heap is still retained during the original
		// terminal predicate. Account its full all_of/find_if_not predicate and
		// iterator carriers, complete/successful/item_done scopes before entry.
		constexpr size_t terminal_frames =
			6 * (3 * sizeof(void *) + sizeof(bool)) +
			4 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(size_t) +
			sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
			4 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
		if (!recovery_add(work.live, work.context_heap) ||
		    !recovery_admit(work.live, terminal_frames, reserve, context))
			return error::capacity;
		if (!body_terminal(work.progress))
			return error::unresolved;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*output = std::move(work.original);
		if (retained_command_heap_bytes)
			*retained_command_heap_bytes = work.command_heap;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

#include <new>
namespace
{
// This private budget owns only this decoder's scopes and actual transferred
// child payloads. Caller input/prior-output storage stays in outer unchanged.
struct birth_historical_budget
{
	recovery_reserve_fn reserve;
	void *context;
	size_t outer, source, inline_bytes, heap = 0;
	bool denied = false;
	bool prefix(size_t &result, size_t extra = 0) noexcept
	{
		result = outer;
		if (!recovery_add(result, source) || !recovery_add(result, inline_bytes) ||
		    !recovery_add(result, heap) || !recovery_add(result, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool admit(size_t extra = 0) noexcept
	{
		size_t peak = 0;
		if (!prefix(peak, extra) || !reserve || !reserve(peak, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool retain(size_t extra) noexcept
	{
		if (!recovery_add(heap, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool forward(size_t peak, void *opaque) noexcept
	{
		auto &self = *static_cast<birth_historical_budget *>(opaque);
		if (!self.reserve || !self.reserve(peak, self.context))
		{
			self.denied = true;
			return false;
		}
		return true;
	}
};
using birth_historical_query = bool (*)(size_t *) noexcept;
bool birth_historical_child_source(birth_historical_budget &budget, birth_historical_query getter,
				   size_t query, size_t &source, size_t caller_extra = 0) noexcept
{
	// The getter output and accessor return already belong to this helper's
	// admitted source. Admit the genuine lower getter itself before evaluation.
	size_t peak = caller_extra;
	if (!recovery_add(peak, query) || !budget.admit(peak) || !getter(&source))
	{
		budget.denied = true;
		return false;
	}
	peak = caller_extra;
	if (!recovery_add(peak, source))
	{
		budget.denied = true;
		return false;
	}
	return budget.admit(peak);
}
bool birth_historical_owned_child_entry(size_t base, birth_historical_query source_getter,
					birth_historical_query inline_getter, size_t query,
					birth_historical_budget &budget) noexcept
{
	size_t peak = base, source = 0, initial = 0;
	if (!recovery_add(peak, query) || !budget.reserve ||
	    !budget.reserve(peak, budget.context) || !source_getter(&source) ||
	    !inline_getter(&initial))
	{
		budget.denied = true;
		return false;
	}
	peak = base;
	if (!recovery_add(peak, source) || !recovery_add(peak, initial) ||
	    !budget.reserve(peak, budget.context))
	{
		budget.denied = true;
		return false;
	}
	// Real callee adds its own source/work to every callback; the temporary
	// entry admission does not become part of base passed into that child.
	return true;
}
bool birth_historical_retained_child_source(size_t base, birth_historical_query getter,
					    size_t query, size_t &source,
					    birth_historical_budget &budget) noexcept
{
	size_t peak = base;
	if (!recovery_add(peak, query) || !budget.reserve ||
	    !budget.reserve(peak, budget.context) || !getter(&source))
	{
		budget.denied = true;
		return false;
	}
	peak = base;
	if (!recovery_add(peak, source) || !budget.reserve(peak, budget.context))
	{
		budget.denied = true;
		return false;
	}
	return true;
}
struct birth_historical_command_work
{
	quest_mobile_native_image image;
	size_t source = 0, initial = 0, image_heap = 0, recipe_heap = 0, base = 0, lifetime = 0;
};
bool birth_historical_command_values(const critical_command &command,
				     std::vector<uint8_t> *canonical,
				     std::vector<native_mobile_birth_item_recipe> *recipes,
				     birth_historical_budget &budget)
{
	// Preserve the original two-version/publication short circuit.
	if ((command.payload_version != NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION &&
	     command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION) ||
	    !command.publication_required)
		return false;
	// Required constant evaluation: the full query accessor composes nested
	// pure query accessors; only this real caller-owned size_t remains live.
	constexpr size_t command_query_frames =
		native_mobile_birth_command_source_query_frame_bytes();
	size_t lifetime = 0, helper_inline = sizeof(birth_historical_command_work);
	if (!budget.admit(quest_mobile_native_image_lifetime_source_query_frame_bytes()) ||
	    !quest_mobile_native_image_lifetime_source_frame_bytes(&lifetime) ||
	    !recovery_add(helper_inline, lifetime) || !budget.admit(helper_inline))
	{
		budget.denied = true;
		return false;
	}
	birth_historical_command_work work;
	work.lifetime = lifetime;
	if (!birth_historical_child_source(
		    budget, critical_command_startup_codec_source_frame_bytes,
		    critical_command_startup_codec_source_query_frame_bytes(), work.source,
		    helper_inline) ||
	    !budget.prefix(work.base, helper_inline) || !recovery_add(work.base, work.source))
	{
		budget.denied = true;
		return false;
	}
	if (critical_command_encode_bounded(command, canonical, birth_historical_budget::forward,
					    &budget,
					    work.base) != critical_command_codec_result::ok)
		return false;
	if (!budget.retain(canonical->capacity()))
		return false;
	if (canonical->size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
		return false;
	if (!birth_historical_child_source(
		    budget, native_mobile_birth_command_historical_decode_source_frame_bytes,
		    command_query_frames, work.source, helper_inline) ||
	    !budget.prefix(work.base, helper_inline) ||
	    !recovery_add(work.base, command_query_frames) || !budget.reserve ||
	    !budget.reserve(work.base, budget.context) ||
	    !native_mobile_birth_command_historical_decode_initial_inline_bytes(&work.initial) ||
	    !budget.prefix(work.base, helper_inline))
	{
		budget.denied = true;
		return false;
	}
	// The fixed child owns its actual local SOURCE and image/recipe/constructor/
	// rebuild DTOs. Caller source and entry preflight are transient only.
	size_t entry = work.base;
	if (!recovery_add(entry, work.source) || !recovery_add(entry, work.initial) ||
	    !budget.reserve || !budget.reserve(entry, budget.context))
	{
		budget.denied = true;
		return false;
	}
	if (native_mobile_birth_command_historical_decode_fixed_bounded(
		    command, &work.image, recipes, birth_historical_budget::forward, &budget,
		    work.base, &work.image_heap, &work.recipe_heap) != error::ok)
		return false;
	// The image dies here, as in original command_values. Only the actual
	// recipe payload lives with the decoder after this helper returns.
	return budget.retain(work.recipe_heap);
}
struct birth_historical_receipt_work
{
	native_mobile_birth_result result;
	economic_frozen_intent intent;
	economic_account_key wallet;
	economic_accounting_plan plan;
	std::span<const uint8_t> payload, intent_bytes;
	size_t base = 0, source = 0, initial = 0, plan_heap = 0;
	bool matches = false;
};
bool birth_historical_receipt_valid(const critical_command &command, bool present,
				    const critical_completion &receipt,
				    birth_historical_budget &budget)
{
	if (!receipt_shape(command, present, receipt, recovery_policy::historical))
		return false;
	if (!present || !successful(receipt))
		return true;
	if (!budget.admit(sizeof(birth_historical_receipt_work)))
		return false;
	birth_historical_receipt_work work;
	work.payload = { receipt.result_payload.data(), receipt.result_size };
	if (!budget.prefix(work.base, sizeof(work)))
		return false;
	if (!birth_historical_owned_child_entry(
		    work.base, native_mobile_birth_result_source_frame_bytes,
		    native_mobile_birth_result_initial_inline_bytes,
		    native_mobile_birth_result_source_query_frame_bytes(), budget))
		return false;
	if (native_mobile_birth_result_decode_bounded(work.payload, &work.result,
						      birth_historical_budget::forward, &budget,
						      work.base) != error::ok)
		return false;
	work.intent_bytes = command.accounting_intent;
	if (!birth_historical_owned_child_entry(
		    work.base, economic_intent_decode_source_frame_bytes,
		    economic_intent_decode_initial_inline_bytes,
		    economic_intent_decode_source_query_frame_bytes(), budget))
		return false;
	if (!birth_historical_child_source(
		    budget, economic_intent_decode_source_supplement_frame_bytes,
		    economic_intent_decode_source_query_frame_bytes(), work.source, sizeof(work)))
		return false;
	size_t intent_outer = work.base;
	if (!recovery_add(intent_outer, work.source))
	{
		budget.denied = true;
		return false;
	}
	if (economic_intent_decode_bounded(work.intent_bytes, &work.intent,
					   birth_historical_budget::forward, &budget,
					   intent_outer) != error::ok)
		return false;
	if (!recovery_add(work.base, work.intent.admission.facts.capacity()))
	{
		budget.denied = true;
		return false;
	}
	work.wallet = { work.intent.admission.metadata.lineage, economic_account_kind::wallet,
			work.result.wallet_mapping_id, ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	if (!birth_historical_owned_child_entry(
		    work.base, native_mobile_birth_accounting_compile_own_source_frame_bytes,
		    native_mobile_birth_accounting_compile_initial_inline_bytes,
		    native_mobile_birth_accounting_compile_profile_query_frame_bytes(), budget))
		return false;
	if (native_mobile_birth_accounting_compile_bounded(
		    command, work.wallet, &work.plan, birth_historical_budget::forward, &budget,
		    work.base, &work.plan_heap) != error::ok)
		return false;
	if (!recovery_add(work.base, work.plan_heap))
	{
		budget.denied = true;
		return false;
	}
	if (!birth_historical_owned_child_entry(
		    work.base, native_mobile_birth_result_source_frame_bytes,
		    native_mobile_birth_result_initial_inline_bytes,
		    native_mobile_birth_result_source_query_frame_bytes(), budget))
		return false;
	return native_mobile_birth_result_matches_bounded(command, work.wallet, work.plan,
							  work.result, &work.matches,
							  birth_historical_budget::forward, &budget,
							  work.base) == error::ok &&
	       work.matches;
}

namespace
{
struct birth_fixed_v4_command_work
{
	quest_mobile_native_image image;
	native_mobile_birth_cash_role_recipe role;
	size_t base = 0, lifetime = 0, source = 0, initial = 0, image_heap = 0, recipe_heap = 0;
};
bool birth_fixed_v4_decode_entry(size_t base, birth_historical_budget &budget) noexcept
{
	// Required constant evaluation, with genuine caller-owned local N.
	constexpr size_t command_query_frames =
		native_mobile_birth_cash_role_command_source_query_frame_bytes();
	return birth_historical_owned_child_entry(
		base, native_mobile_birth_cash_role_command_decode_source_frame_bytes,
		native_mobile_birth_cash_role_command_decode_initial_inline_bytes,
		command_query_frames, budget);
}
bool birth_fixed_command_values(const critical_command &command, std::vector<uint8_t> *canonical,
				std::vector<native_mobile_birth_item_recipe> *recipes,
				recovery_policy policy, birth_historical_budget &budget)
{
	if (policy == recovery_policy::historical)
		return birth_historical_command_values(command, canonical, recipes, budget);
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    !command.publication_required)
		return false;
	size_t lifetime = 0, helper = sizeof(birth_fixed_v4_command_work);
	if (!budget.admit(quest_mobile_native_image_lifetime_source_query_frame_bytes()) ||
	    !quest_mobile_native_image_lifetime_source_frame_bytes(&lifetime) ||
	    !recovery_add(helper, lifetime) || !budget.admit(helper))
	{
		budget.denied = true;
		return false;
	}
	birth_fixed_v4_command_work work;
	work.lifetime = lifetime;
	if (!birth_historical_child_source(
		    budget, critical_command_startup_codec_source_frame_bytes,
		    critical_command_startup_codec_source_query_frame_bytes(), work.source,
		    helper) ||
	    !budget.prefix(work.base, helper) || !recovery_add(work.base, work.source))
	{
		budget.denied = true;
		return false;
	}
	if (critical_command_encode_bounded(command, canonical, birth_historical_budget::forward,
					    &budget,
					    work.base) != critical_command_codec_result::ok)
		return false;
	if (!budget.retain(canonical->capacity()))
		return false;
	if (canonical->size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
		return false;
	if (!budget.prefix(work.base, helper) || !birth_fixed_v4_decode_entry(work.base, budget))
		return false;
	if (native_mobile_birth_cash_role_command_decode_fixed_bounded(
		    command, &work.image, recipes, &work.role, birth_historical_budget::forward,
		    &budget, work.base, &work.image_heap, &work.recipe_heap) != error::ok ||
	    work.role.role != (policy == recovery_policy::shared_shop ?
				       native_mobile_birth_cash_role::shared_shopkeeper :
				       native_mobile_birth_cash_role::ordinary_wallet))
		return false;
	return budget.retain(work.recipe_heap);
}
struct birth_fixed_v4_receipt_work
{
	native_mobile_birth_cash_role_result result;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	economic_account_key wallet;
	economic_accounting_plan plan;
	std::span<const uint8_t> payload, intent_bytes;
	size_t base = 0, lifetime = 0, image_heap = 0, recipe_heap = 0, plan_heap = 0, source = 0;
	bool matches = false;
};
bool birth_fixed_receipt_valid(const critical_command &command, bool present,
			       const critical_completion &receipt, recovery_policy policy,
			       birth_historical_budget &budget)
{
	if (policy == recovery_policy::historical)
		return birth_historical_receipt_valid(command, present, receipt, budget);
	if (!receipt_shape(command, present, receipt, policy))
		return false;
	if (!present || !successful(receipt))
		return true;
	size_t lifetime = 0, helper = sizeof(birth_fixed_v4_receipt_work);
	if (!budget.admit(quest_mobile_native_image_lifetime_source_query_frame_bytes()) ||
	    !quest_mobile_native_image_lifetime_source_frame_bytes(&lifetime) ||
	    !recovery_add(helper, lifetime) || !budget.admit(helper))
	{
		budget.denied = true;
		return false;
	}
	birth_fixed_v4_receipt_work work;
	if (!budget.prefix(work.base, helper))
		return false;
	work.payload = { receipt.result_payload.data(), receipt.result_size };
	if (!birth_historical_owned_child_entry(
		    work.base, native_mobile_birth_cash_role_result_fixed_source_frame_bytes,
		    native_mobile_birth_cash_role_result_fixed_initial_inline_bytes,
		    native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(), budget))
		return false;
	if (native_mobile_birth_cash_role_result_decode_fixed_bounded(
		    work.payload, &work.result, birth_historical_budget::forward, &budget,
		    work.base) != error::ok)
		return false;
	if (policy == recovery_policy::shared_shop)
	{
		if (work.result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    receipt.durable_revision !=
			    std::max(uint64_t{ 1 }, work.result.shared.owner_revision_after))
			return false;
		if (!birth_fixed_v4_decode_entry(work.base, budget))
			return false;
		if (native_mobile_birth_cash_role_command_decode_fixed_bounded(
			    command, &work.image, &work.recipes, &work.role,
			    birth_historical_budget::forward, &budget, work.base, &work.image_heap,
			    &work.recipe_heap) != error::ok ||
		    work.role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    work.result.shared.shop_before_present ||
		    work.result.shared.shop_revision_before != 0 ||
		    !work.result.shared.shop_after_present ||
		    work.result.shared.shop_revision_after != 1)
			return false;
		if (work.image.items.empty())
		{
			if (work.result.shared.owner_before_present !=
				    work.result.shared.owner_after_present ||
			    work.result.shared.owner_revision_before !=
				    work.result.shared.owner_revision_after)
				return false;
		}
		else if (!work.result.shared.owner_after_present ||
			 work.result.shared.owner_revision_before == UINT64_MAX ||
			 work.result.shared.owner_revision_after !=
				 work.result.shared.owner_revision_before + 1)
			return false;
		if (!recovery_add(work.base, work.image_heap) ||
		    !recovery_add(work.base, work.recipe_heap))
		{
			budget.denied = true;
			return false;
		}
	}
	else
	{
		if (work.result.role != native_mobile_birth_cash_role::ordinary_wallet)
			return false;
		work.intent_bytes = command.accounting_intent;
		if (!birth_historical_owned_child_entry(
			    work.base, economic_intent_decode_source_frame_bytes,
			    economic_intent_decode_initial_inline_bytes,
			    economic_intent_decode_source_query_frame_bytes(), budget))
			return false;
		size_t intent_outer = work.base;
		if (!birth_historical_child_source(
			    budget, economic_intent_decode_source_supplement_frame_bytes,
			    economic_intent_decode_source_query_frame_bytes(), work.source,
			    helper) ||
		    !recovery_add(intent_outer, work.source))
		{
			budget.denied = true;
			return false;
		}
		if (economic_intent_decode_bounded(work.intent_bytes, &work.intent,
						   birth_historical_budget::forward, &budget,
						   intent_outer) != error::ok)
			return false;
		if (!recovery_add(work.base, work.intent.admission.facts.capacity()))
		{
			budget.denied = true;
			return false;
		}
		work.wallet = { work.intent.admission.metadata.lineage,
				economic_account_kind::wallet, work.result.wallet_mapping_id,
				ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
	}
	if (!birth_historical_owned_child_entry(
		    work.base,
		    native_mobile_birth_cash_role_accounting_compile_own_source_frame_bytes,
		    native_mobile_birth_cash_role_accounting_compile_initial_inline_bytes,
		    native_mobile_birth_cash_role_accounting_compile_profile_query_frame_bytes(),
		    budget))
		return false;
	const auto compiled =
		policy == recovery_policy::shared_shop ?
			native_mobile_birth_cash_role_accounting_compile_fixed_bounded(
				command, work.result.shared, &work.plan,
				birth_historical_budget::forward, &budget, work.base,
				&work.plan_heap) :
			native_mobile_birth_cash_role_accounting_compile_fixed_bounded(
				command, work.wallet, &work.plan, birth_historical_budget::forward,
				&budget, work.base, &work.plan_heap);
	if (compiled != error::ok)
		return false;
	if (!recovery_add(work.base, work.plan_heap))
	{
		budget.denied = true;
		return false;
	}
	if (!birth_historical_owned_child_entry(
		    work.base, native_mobile_birth_cash_role_result_fixed_source_frame_bytes,
		    native_mobile_birth_cash_role_result_fixed_initial_inline_bytes,
		    native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(), budget))
		return false;
	const auto matched =
		policy == recovery_policy::shared_shop ?
			native_mobile_birth_cash_role_result_matches_fixed_bounded(
				command, work.result.shared, work.plan, work.result, &work.matches,
				birth_historical_budget::forward, &budget, work.base) :
			native_mobile_birth_cash_role_result_matches_fixed_bounded(
				command, work.wallet, work.plan, work.result, &work.matches,
				birth_historical_budget::forward, &budget, work.base);
	return matched == error::ok && work.matches;
}
// Full original checkpoint matcher, with actual fresh wire/forest requests and
// fixed command child. Lower flat/list codec SOURCE remains an explicit join.
bool birth_fixed_checkpoint_matches(const critical_command &command,
				    const std::span<const uint8_t> &bytes,
				    birth_historical_budget &budget)
{
	if (bytes.empty() || bytes.size() > LIMIT)
		return false;
	size_t lifetime = 0, helper = sizeof(recovery_checkpoint_workspace), base = 0, source = 0;
	if (!budget.admit(quest_mobile_native_image_lifetime_source_query_frame_bytes()) ||
	    !quest_mobile_native_image_lifetime_source_frame_bytes(&lifetime) ||
	    !recovery_add(helper, lifetime) || !budget.admit(helper) ||
	    !budget.prefix(base, helper))
	{
		budget.denied = true;
		return false;
	}
	// Before decoded record exists, retain its complete physical observer source
	// through the child's admission and the first allocation-free CURRENT scan.
	// No callback drops its unknown returned heap between decode and that scan.
	source = player_item_snapshot_current_heap_observer_frame_bytes();
	if (!recovery_add(base, source))
	{
		budget.denied = true;
		return false;
	}
	size_t peak = base;
	if (!recovery_add(peak, bytes.size()) || !budget.reserve ||
	    !budget.reserve(peak, budget.context))
	{
		budget.denied = true;
		return false;
	}
	recovery_checkpoint_workspace work;
	work.encoded.assign(bytes.begin(), bytes.end());
	if (!recovery_add(base, work.encoded.capacity()))
	{
		budget.denied = true;
		return false;
	}
	if (!birth_historical_owned_child_entry(
		    base, flatfile_shopkeeper_initial_checkpoint_decode_fixed_source_frame_bytes,
		    flatfile_shopkeeper_initial_checkpoint_decode_fixed_initial_inline_bytes,
		    flatfile_shopkeeper_initial_checkpoint_decode_fixed_source_query_frame_bytes(),
		    budget))
		return false;
	if (flatfile_shopkeeper_initial_checkpoint_decode_fixed_bounded(
		    work.encoded, &work.record, birth_historical_budget::forward, &budget, base) !=
	    flatfile_shopkeeper_result::ok)
		return false;
	size_t record_heap = 0, items_heap = 0, image_heap = 0, recipe_heap = 0;
	if (!recovery_rows(record_heap, work.record.affects.capacity(),
			   sizeof(flatfile_shopkeeper_affect_record)) ||
	    !player_item_snapshot_list_current_heap_bytes(work.record.items, &items_heap) ||
	    !recovery_add(record_heap, items_heap) || !recovery_add(base, record_heap))
	{
		budget.denied = true;
		return false;
	}
	if (!birth_fixed_v4_decode_entry(base, budget))
		return false;
	if (native_mobile_birth_cash_role_command_decode_fixed_bounded(
		    command, &work.image, &work.recipes, &work.role,
		    birth_historical_budget::forward, &budget, base, &image_heap,
		    &recipe_heap) != error::ok ||
	    work.role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
	    !work.image.cash ||
	    work.record.shop_id != static_cast<uint32_t>(work.role.original.reset_shop_index) ||
	    work.record.mob_vnum != work.role.original.mobile_vnum ||
	    work.record.room_vnum != work.role.original.reset_room_vnum ||
	    work.record.revision != 1 || work.record.saved_at < 0 || work.record.cash < 0)
		return false;
	int64_t cash = 0;
	if (!recovery_add(base, image_heap) || !recovery_add(base, recipe_heap))
	{
		budget.denied = true;
		return false;
	}
	if (economic_coin_value(work.image.cash->denominations.amount, &cash) != error::ok ||
	    cash != work.record.cash)
		return false;
	if (!birth_historical_owned_child_entry(
		    base, player_item_snapshot_list_encode_source_frame_bytes,
		    player_item_snapshot_list_encode_initial_inline_bytes,
		    player_item_snapshot_list_encode_source_query_frame_bytes(), budget) ||
	    !birth_historical_retained_child_source(
		    base, player_item_snapshot_list_encode_source_frame_bytes,
		    player_item_snapshot_list_encode_source_query_frame_bytes(), source, budget))
		return false;
	size_t encode_outer = base;
	if (!recovery_add(encode_outer, source))
	{
		budget.denied = true;
		return false;
	}
	if (player_item_snapshot_list_encode_bounded(
		    work.image.items, &work.original_items, birth_historical_budget::forward,
		    &budget, encode_outer) != player_snapshot_codec_result::ok)
		return false;
	if (!recovery_add(base, work.original_items.capacity()))
	{
		budget.denied = true;
		return false;
	}
	if (!birth_historical_owned_child_entry(
		    base, player_item_snapshot_list_encode_source_frame_bytes,
		    player_item_snapshot_list_encode_initial_inline_bytes,
		    player_item_snapshot_list_encode_source_query_frame_bytes(), budget))
		return false;
	encode_outer = base;
	if (!recovery_add(encode_outer, source))
	{
		budget.denied = true;
		return false;
	}
	return player_item_snapshot_list_encode_bounded(work.record.items, &work.checkpoint_items,
							birth_historical_budget::forward, &budget,
							encode_outer) ==
		       player_snapshot_codec_result::ok &&
	       work.original_items == work.checkpoint_items;
}
}

template <typename GetItem, typename GetCount, typename GetEffect>
bool birth_historical_context_valid_range(const critical_command &command,
					  std::span<const native_mobile_birth_item_recipe> recipes,
					  const native_mobile_birth_recovery_context &value,
					  size_t count, GetItem get_item, GetCount get_count,
					  GetEffect get_effect, birth_historical_budget &budget,
					  recovery_policy policy)
{
	if (value.stage > native_mobile_birth_recovery_stage::physically_proven ||
	    count != recipes.size() || recipes.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !valid_action(value.whole_binding) || !valid_action(value.reference_install) ||
	    (value.mobile_publication.returned && !value.mobile_publication.started) ||
	    (value.mobile_publication.consumed && !value.mobile_publication.started) ||
	    !mobile_shape(value) ||
	    !birth_fixed_receipt_valid(command, value.receipt_present, value.receipt, policy,
				       budget))
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

struct birth_historical_decode_work
{
	wire_view view;
	std::vector<uint8_t> canonical;
	std::vector<native_mobile_birth_item_recipe> recipes;
	std::span<const native_mobile_birth_item_recipe> recipe_values;
	native_mobile_birth_recovery_context candidate;
	size_t offset = 0, request = 0, count = 0, index = 0, transferred = 0;
	error checked = error::ok;
};
}

namespace
{
template <class T> constexpr size_t birth_historical_vector_lifetime_source_frames() noexcept
{
	// Actual default constructor chain and full typed destroy/deallocate chain;
	// no reserve, resize, element insertion or generic copy profile is aliased.
	using vector = std::vector<T>;
	using allocator = typename vector::allocator_type;
	using pointer = typename vector::pointer;
	constexpr size_t defaults = 6 * sizeof(void *);
	constexpr size_t cleanup = 20 * sizeof(void *) + 6 * sizeof(size_t) + 5 * sizeof(pointer) +
				   2 * sizeof(allocator) + sizeof(bool);
	return defaults + cleanup;
}
// Selected ordinary GNU13 vector subgraphs. This source sum is independent of
// payload requests and row counts. It does not grant allocator/emitted proof.
template <class T> constexpr size_t birth_historical_vector_source_frames() noexcept
{
	using vector = std::vector<T>;
	using allocator = typename vector::allocator_type;
	using pointer = typename vector::pointer;
	using iterator = typename vector::iterator;
	using difference = typename vector::difference_type;
	// vector/_Vector_base/_Vector_impl/allocator/new_allocator/data default.
	// reserve this/n/old_size/tmp; max_size/_S_max_size diffmax/allocmax/min;
	// allocator_traits/max_size/new_allocator and actual size/capacity scopes.
	constexpr size_t reserve = 10 * sizeof(void *) + 11 * sizeof(size_t) + sizeof(pointer) +
				   2 * sizeof(difference) + 3 * sizeof(bool);
	// _M_allocate -> traits allocate -> allocator::allocate -> new_allocator:
	// this/allocator/result/count, alignment and literal new call carriers.
	constexpr size_t allocate = 7 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(pointer) +
				    sizeof(std::align_val_t) + 2 * sizeof(bool);
	// _S_relocate/__relocate_a/__relocate_a_1/__relocate_object_a; typed pointer
	// first/last/result/current, allocator and object reference forwarding.
	constexpr size_t relocate = 17 * sizeof(pointer) + 5 * sizeof(void *) + sizeof(bool);
	// Reserved push_back(T&&)->emplace_back->traits construct->construct_at:
	// no _M_realloc_insert can execute after exact full count reserve. Actual
	// returned reference/argument forwarding and placement-new are retained.
	constexpr size_t append =
		12 * sizeof(void *) + sizeof(pointer) + sizeof(iterator) + sizeof(bool);
	// vector move constructor -> base allocator/default -> impl_data move;
	// move assignment true_type -> tmp/get_allocator/swap_data/_M_copy_data.
	constexpr size_t move = sizeof(vector) + sizeof(allocator) + 25 * sizeof(void *) +
				sizeof(std::true_type) + sizeof(std::false_type);
	// ~vector/_Destroy allocator -> _Destroy -> _Destroy_aux -> destroy_at;
	// base cleanup/get_allocator/deallocate -> traits/allocator/new_allocator.
	// Nested element destructor itself is included by its owning type below.
	return birth_historical_vector_lifetime_source_frames<T>() + reserve + allocate + relocate +
	       append + move;
}
constexpr size_t birth_historical_byte_algorithms =
	// 4-iterator equal/__equal4 -> distance/subtraction/__equal_aux/_equal:
	16 * sizeof(const uint8_t *) + 4 * sizeof(std::ptrdiff_t) + 7 * sizeof(bool) +
	// read_receipt copy_n -> __copy_n -> copy -> copy_move_a2 and copy move
	// trivial memmove; array begin/end/data/operator[] forwarding and byte get.
	18 * sizeof(void *) + 9 * sizeof(size_t) + 2 * sizeof(uint64_t) + 2 * sizeof(uint8_t) +
	3 * sizeof(std::random_access_iterator_tag) + 3 * sizeof(bool);
constexpr size_t birth_historical_predicate_frames =
	// context_valid_range signature and seen array, original six flags, UID
	// indices/effect count/getter row and lambda forwarding/reference scopes.
	sizeof(std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS>) +
	2 * sizeof(native_mobile_birth_recovery_item) + sizeof(recovery_wire_item_access) +
	sizeof(recovery_wire_count_access) + sizeof(recovery_wire_effect_access) +
	18 * sizeof(void *) + 9 * sizeof(size_t) + 12 * sizeof(bool) +
	sizeof(std::span<const native_mobile_birth_item_recipe>) +
	// effects_valid/periodic_at/step_count/recipe_index real iterative scopes.
	9 * sizeof(void *) + 12 * sizeof(size_t) + 4 * sizeof(uint32_t) + 9 * sizeof(bool) +
	3 * sizeof(native_mobile_birth_recovery_effect) +
	// mobile_shape schedule_steps and action/choice/complete typed DTOs.
	sizeof(std::array<size_t, 4>) + 8 * sizeof(void *) + 5 * sizeof(size_t) + 7 * sizeof(bool) +
	3 * sizeof(native_mobile_birth_recovery_action) +
	2 * sizeof(native_mobile_birth_recovery_choice) +
	// no_progress/body_terminal and typed all_of -> find_if_not -> __find_if,
	// _Iter_negate<function pointer>, iterator/result/count/trip_count scopes.
	8 * sizeof(std::vector<native_mobile_birth_recovery_item>::const_iterator) +
	8 * sizeof(void *) + 3 * sizeof(std::ptrdiff_t) + 10 * sizeof(bool) +
	2 * sizeof(std::vector<native_mobile_birth_recovery_effect>::const_iterator);
constexpr size_t birth_historical_wire_frames =
	// Original preflight candidate is real, not inferred encoded storage.
	sizeof(wire_view) + 3 * sizeof(critical_completion) +
	sizeof(native_mobile_birth_recovery_mobile) + sizeof(native_mobile_birth_recovery_effect) +
	sizeof(native_mobile_birth_recovery_action) + sizeof(native_mobile_birth_recovery_choice) +
	// preflight all header/count/remaining/retained/offset/effects/index locals;
	// read_body/read_item/read_receipt/get/read_choice/receipt_shape signatures.
	20 * sizeof(void *) + 23 * sizeof(size_t) + 4 * sizeof(uint64_t) + 3 * sizeof(uint32_t) +
	sizeof(uint16_t) + 4 * sizeof(uint8_t) + 12 * sizeof(bool) + sizeof(recovery_policy) +
	6 * sizeof(std::span<const uint8_t>);
constexpr size_t birth_historical_budget_source_frames =
	// prefix/admit/retain/forward and actual checked add/rows descendants.
	11 * sizeof(void *) + 13 * sizeof(size_t) + 9 * sizeof(bool);
constexpr size_t birth_historical_local_scopes =
	// Actual public decoder wrapper + private body signatures/query locals.
	12 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(error) + sizeof(recovery_policy) +
	// budget prefix(this,result&,extra), admit(this,extra,peak), retain and
	// forward(peak,opaque,self&); checked recovery_add/rows original helpers.
	birth_historical_budget_source_frames +
	// child_source(budget&,getter,query,source&,extra,peak) and entry helper
	// (base,getters,query,budget&,peak,source,initial). The full common command
	// query is now a required constexpr local N, replacing its accessor-return
	// N; no nested runtime query-accessor graph executes. Other simple accessor
	// return carriers remain genuine.
	6 * sizeof(void *) + 12 * sizeof(size_t) + 4 * sizeof(bool) +
	// command_values refs/lifetime/helper/entry; receipt refs/present and
	// intent_outer, typed lower results and temporary wallet assignment.
	7 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool) + 4 * sizeof(error) +
	sizeof(critical_command_codec_result) + sizeof(economic_account_key) + 2 * sizeof(void *) +
	// Actual generic command/receipt dispatch wrappers around historical arms.
	7 * sizeof(void *) + 3 * sizeof(bool) + 2 * sizeof(recovery_policy) +
	// Four actual byte-vector const capacity()/size() observer scopes used for
	// canonical and intent-facts handoff; member fields remain object-owned.
	4 * sizeof(void *) + 4 * sizeof(size_t) +
	// Original local command image destructor and recipe nested-library vector
	// cleanup have their generic library source in child command profiles.
	// Own public output move implicit member action/receipt/arrays references.
	16 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(std::true_type) +
	sizeof(std::span<const native_mobile_birth_item_recipe>);
constexpr size_t birth_historical_plan_lifetime_source =
	// All six actual vectors in receipt-owned plan; items_before/items_after
	// are two independent named objects with sequential destructor scopes.
	birth_historical_vector_lifetime_source_frames<economic_account_effect>() +
	birth_historical_vector_lifetime_source_frames<economic_coin_posting>() +
	birth_historical_vector_lifetime_source_frames<economic_child_link>() +
	2 * birth_historical_vector_lifetime_source_frames<economic_item_snapshot>() +
	birth_historical_vector_lifetime_source_frames<economic_item_event>() +
	4 * sizeof(void *); // actual aggregate plan/frozen-intent default/cleanup this.
// First query executes before work/budget construction. Its actual public
// arguments, four scalar locals, query helper pointer/bool and accessor result.
constexpr size_t birth_historical_entry_query_bytes = 12 * sizeof(void *) + 7 * sizeof(size_t) +
						      2 * sizeof(bool) + 2 * sizeof(error) +
						      sizeof(recovery_policy);
constexpr size_t birth_historical_progress_source_frames() noexcept
{
	using item_iterator = std::vector<native_mobile_birth_recovery_item>::const_iterator;
	using effect_iterator = std::vector<native_mobile_birth_recovery_effect>::const_iterator;
	using predicate = decltype(&item_done);
	using iter_predicate = __gnu_cxx::__ops::_Iter_pred<predicate>;
	using negated_predicate = __gnu_cxx::__ops::_Iter_negate<predicate>;
	using category = std::random_access_iterator_tag;
	// Only the original postdecode no_progress/body_terminal descendants.
	// body_terminal/successful/complete(action,mobile), item_done/fields,
	// no_progress and bits(action,mobile,effect): actual refs/results/count.
	constexpr size_t local =
		10 * sizeof(void *) + sizeof(size_t) + 7 * sizeof(bool) + 3 * sizeof(uint8_t);
	// Four actual range-for scopes: range refs, two array pointer pairs,
	// two vector iterator pairs and their bound element references.
	constexpr size_t ranges =
		12 * sizeof(void *) + 2 * sizeof(item_iterator) + 2 * sizeof(effect_iterator);
	// array<T,N>::const begin/end/data for effect and choice arrays.
	// GNU13 data directly casts _M_elems; no unused _S_ref/_S_ptr branch.
	constexpr size_t arrays = 12 * sizeof(void *);
	// Both vector const begin/end constructor calls, plus only the genuine
	// effect size() reached from item_done. Returned iterators are separate.
	constexpr size_t vector_observers = 13 * sizeof(void *) + 2 * sizeof(item_iterator) +
					    2 * sizeof(effect_iterator) + sizeof(size_t);
	// Normal iterator dereference/increment/base/equality for both types;
	// subtraction belongs only to the item all_of random-access path.
	constexpr size_t iterators =
		18 * sizeof(void *) + 2 * sizeof(bool) + sizeof(std::ptrdiff_t);
	// Exact selected GNU13 all_of -> find_if_not -> __find_if_not ->
	// __find_if(random_access), _Iter_pred/_Iter_negate constructors and
	// std::move pointer references. Every result/adapter/tag is typed.
	constexpr size_t all = 3 * sizeof(item_iterator) + sizeof(predicate) + sizeof(bool);
	constexpr size_t find_not = 3 * sizeof(item_iterator) + sizeof(predicate);
	constexpr size_t adapt =
		2 * sizeof(predicate) + sizeof(iter_predicate) + 3 * sizeof(void *);
	constexpr size_t inner = 3 * sizeof(item_iterator) + sizeof(iter_predicate);
	constexpr size_t negate = sizeof(iter_predicate) + sizeof(negated_predicate) +
				  sizeof(predicate) + 3 * sizeof(void *);
	constexpr size_t query_category = sizeof(void *) + sizeof(category);
	constexpr size_t find = 3 * sizeof(item_iterator) + sizeof(negated_predicate) +
				sizeof(category) + sizeof(std::ptrdiff_t);
	constexpr size_t invoke = sizeof(void *) + sizeof(item_iterator) + sizeof(bool);
	return local + ranges + arrays + vector_observers + iterators + all + find_not + adapt +
	       inner + negate + query_category + find + invoke;
}
// Original span(vector const&) selected member-data/member-size delegation.
// span range ctor(this,range); ranges::_Data(this,range,pointer-result);
// vector::data(this,pointer-result)->_M_data_ptr(this,pointer,pointer-result);
// ranges::_Size(this,range,size-result)->vector::size(this,size-result);
// span(pointer,count)(this,pointer,count)->to_address->__to_address (each
// pointer argument/result), then dynamic extent-storage(this,count).
// No ADL/begin-data/sentinel-size/noexcept-only bodies are runtime-selected.
constexpr size_t birth_historical_attachment_span_source =
	2 * sizeof(void *) + 3 * sizeof(void *) + 5 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	sizeof(void *) + sizeof(size_t);
constexpr size_t birth_historical_validator_local_source =
	// typed validator formals(envelope,reserve,opaque,outer), base/status;
	// context aggregate default+cleanup receivers; recovery_add(ref,extra)
	// and actual bool result. Context/span representation belongs initial.
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(error) + 2 * sizeof(void *) +
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	birth_historical_attachment_span_source;
constexpr size_t birth_historical_validate_initial =
	sizeof(native_mobile_birth_recovery_context) + sizeof(std::span<const uint8_t>);
constexpr size_t birth_historical_validate_source =
	birth_historical_validator_local_source +
	// Actual validator owns its postdecode progress/terminal/cleanup subset.
	birth_historical_progress_source_frames() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_item>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_effect>();
// Bool valid wrapper formals, actual base scalar, checked-add descendants and
// bool return are separate from the typed validator context/progress lifetime.
constexpr size_t birth_historical_valid_wrapper_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t birth_historical_validate_entry_bytes =
	birth_historical_validate_source + birth_historical_validate_initial;
#if defined(__linux__) && defined(__x86_64__) && defined(_GLIBCXX_RELEASE) &&                  \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI && \
	__cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR) &&                   \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
constexpr bool birth_historical_source_policy =
	sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	sizeof(std::vector<uint8_t>) == 3 * sizeof(void *) &&
	sizeof(std::vector<native_mobile_birth_recovery_item>::iterator) == sizeof(void *) &&
	sizeof(std::vector<native_mobile_birth_recovery_effect>::iterator) == sizeof(void *) &&
	std::is_nothrow_move_constructible_v<native_mobile_birth_recovery_item> &&
	std::is_trivially_copyable_v<native_mobile_birth_recovery_effect>;
#else
constexpr bool birth_historical_source_policy = false;
#endif
constexpr size_t birth_historical_complete_own_source =
	birth_historical_wire_frames + birth_historical_predicate_frames +
	birth_historical_byte_algorithms + birth_historical_local_scopes +
	birth_historical_plan_lifetime_source +
	birth_historical_vector_lifetime_source_frames<uint8_t>() +
	birth_historical_vector_source_frames<native_mobile_birth_recovery_item>() +
	birth_historical_vector_source_frames<native_mobile_birth_recovery_effect>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_item_recipe>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_library_recipe>();
constexpr size_t birth_historical_initial_inline =
	sizeof(birth_historical_budget) + sizeof(birth_historical_decode_work);
}
bool native_mobile_birth_recovery_historical_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_complete_own_source;
	return true;
}
bool native_mobile_birth_recovery_historical_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_initial_inline;
	return true;
}

namespace
{
struct birth_fixed_shared_decode_work
{
	wire_view view;
	native_mobile_birth_shared_shop_recovery_context candidate;
	size_t retained = 0;
	error checked = error::ok;
};
// Real byte-vector assign(first,last): forward-iterator dispatch, len/capacity,
// _M_allocate_and_copy -> allocate -> __uninitialized_copy_a -> copy/memmove,
// fitting copy and erase/destroy. Complete selected source, no fresh-copy alias.
constexpr size_t birth_fixed_byte_assign_source =
	18 * sizeof(void *) + 12 * sizeof(const uint8_t *) + 8 * sizeof(size_t) +
	4 * sizeof(std::ptrdiff_t) + 4 * sizeof(bool) +
	3 * sizeof(std::random_access_iterator_tag) + 2 * sizeof(std::input_iterator_tag) +
	sizeof(std::allocator<uint8_t>);
constexpr size_t birth_fixed_coin_value_source =
	// Actual economic_coin_value(vector&,value*) + total/index/error result,
	// narrow(wide value,value*) + bool result and numeric_limits min/max
	// int64 returns. COPPER_UNITS is global, not a prospective local vector.
	2 * sizeof(__int128_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(error) +
	sizeof(bool) + 2 * sizeof(int64_t) +
	// array<int64_t,4>::const size and two const operator[] calls; GNU13's
	// actual operator[] directly indexes _M_elems, with no _S_ref child.
	sizeof(void *) + sizeof(size_t) + 2 * (2 * sizeof(void *) + sizeof(size_t));
constexpr size_t birth_fixed_v4_helper_source =
	// Actual v4 command/receipt/checkpoint helper arguments, query/entry/local
	// scalars, result status enums and shared max's uint64 references/result.
	// Full cash command query uses required constexpr local N in place of its
	// former scalar accessor-return N; nested query bodies are not runtime.
	30 * sizeof(void *) + 32 * sizeof(size_t) + 3 * sizeof(error) + sizeof(recovery_policy) +
	12 * sizeof(bool) + 3 * sizeof(uint64_t) + 3 * sizeof(int64_t) +
	birth_fixed_coin_value_source +
	// retained_child_source base/getter/query/source-ref/budget-ref/peak.
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// original vector byte equality and physical record affects/count query.
	8 * sizeof(std::vector<uint8_t>::const_iterator) + 6 * sizeof(void *) + 3 * sizeof(size_t) +
	birth_fixed_byte_assign_source;
constexpr size_t birth_fixed_v4_decode_source =
	// Genuine common wire/context scopes used by this selected v4 algorithm,
	// not a call to a whole historical profile or guessed domain maximum.
	birth_historical_wire_frames + birth_historical_predicate_frames +
	birth_historical_byte_algorithms + birth_historical_local_scopes +
	birth_fixed_v4_helper_source + birth_historical_plan_lifetime_source +
	birth_historical_vector_source_frames<native_mobile_birth_recovery_item>() +
	birth_historical_vector_source_frames<native_mobile_birth_recovery_effect>() +
	birth_historical_vector_lifetime_source_frames<uint8_t>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_item_recipe>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_library_recipe>() +
	birth_historical_vector_lifetime_source_frames<flatfile_shopkeeper_affect_record>();
constexpr size_t birth_fixed_shared_wrapper_source =
	// First original full preflight, actual wrapper signatures/query/scalars,
	// byte checkpoint range assignment, aggregate strong moves and cleanup.
	birth_historical_wire_frames + birth_historical_byte_algorithms +
	birth_fixed_byte_assign_source + birth_historical_budget_source_frames +
	14 * sizeof(void *) + 14 * sizeof(size_t) + 4 * sizeof(bool) + 2 * sizeof(error) +
	birth_historical_vector_source_frames<uint8_t>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_item>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_effect>();
constexpr size_t birth_fixed_shared_query_bytes =
	7 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t birth_fixed_shared_validate_initial =
	sizeof(native_mobile_birth_shared_shop_recovery_context) + sizeof(std::span<const uint8_t>);
constexpr size_t birth_fixed_shared_validate_source =
	birth_historical_validator_local_source + birth_historical_progress_source_frames() +
	birth_historical_vector_lifetime_source_frames<uint8_t>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_item>() +
	birth_historical_vector_lifetime_source_frames<native_mobile_birth_recovery_effect>();
constexpr size_t birth_fixed_shared_validate_entry_bytes =
	birth_fixed_shared_validate_source + birth_fixed_shared_validate_initial;
}
bool native_mobile_birth_cash_role_recovery_fixed_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_v4_decode_source;
	return true;
}
bool native_mobile_birth_cash_role_recovery_fixed_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_initial_inline;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_shared_wrapper_source;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = sizeof(birth_fixed_shared_decode_work) + sizeof(birth_historical_budget);
	return true;
}

// Exact validator/valid entry queries, distinct from decoder work.
bool native_mobile_birth_recovery_historical_validate_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_source;
	return true;
}
bool native_mobile_birth_recovery_historical_validate_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_initial;
	return true;
}
bool native_mobile_birth_recovery_historical_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_source + birth_historical_valid_wrapper_source;
	return true;
}
bool native_mobile_birth_recovery_historical_valid_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_initial;
	return true;
}
bool native_mobile_birth_cash_role_recovery_fixed_validate_source_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_source;
	return true;
}
bool native_mobile_birth_cash_role_recovery_fixed_validate_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_initial;
	return true;
}
bool native_mobile_birth_cash_role_recovery_fixed_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_source + birth_historical_valid_wrapper_source;
	return true;
}
bool native_mobile_birth_cash_role_recovery_fixed_valid_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_historical_validate_initial;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_validate_source_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_shared_validate_source;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_validate_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_shared_validate_initial;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_shared_validate_source + birth_historical_valid_wrapper_source;
	return true;
}
bool native_mobile_birth_shared_shop_recovery_fixed_valid_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !birth_historical_source_policy)
		return false;
	*output = birth_fixed_shared_validate_initial;
	return true;
}

static economic_accounting_error
birth_recovery_decode_fixed_impl(const critical_command &command,
				 const std::span<const uint8_t> &bytes,
				 native_mobile_birth_recovery_context *output,
				 recovery_policy policy, recovery_reserve_fn reserve, void *context,
				 size_t outer_live, size_t *retained_context_heap_bytes) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	// Admit every initial signature/getter/budget scope before querying or
	// constructing the actual work. Inputs and previous destination stay outer.
	size_t source = 0, initial = 0, query_peak = outer_live;
	if (!recovery_add(query_peak, birth_historical_entry_query_bytes) || !reserve ||
	    !reserve(query_peak, context) ||
	    !(policy == recovery_policy::historical ?
		      native_mobile_birth_recovery_historical_source_frame_bytes(&source) :
		      native_mobile_birth_cash_role_recovery_fixed_source_frame_bytes(&source)) ||
	    !native_mobile_birth_recovery_historical_initial_inline_bytes(&initial))
		return error::capacity;
	query_peak = outer_live;
	if (!recovery_add(query_peak, source) || !recovery_add(query_peak, initial) ||
	    !reserve(query_peak, context))
		return error::capacity;
	birth_historical_budget budget{ reserve, context, outer_live, source, initial };
	if (!budget.admit())
		return error::capacity;
	try
	{
		birth_historical_decode_work work;
		work.checked = preflight(&command, bytes, &work.view, policy);
		if (work.checked != error::ok)
			return work.checked;
		if (!birth_fixed_command_values(command, &work.canonical, &work.recipes, policy,
						budget) ||
		    !std::equal(work.canonical.begin(), work.canonical.end(),
				work.view.command.begin(), work.view.command.end()) ||
		    work.view.count != work.recipes.size())
			return budget.denied ? error::capacity : error::payload_conflict;
		work.recipe_values = work.recipes;
		if (policy == recovery_policy::shared_shop &&
		    !birth_fixed_checkpoint_matches(command, work.view.checkpoint, budget))
			return budget.denied ? error::capacity : error::payload_conflict;
		work.offset = work.view.items_offset;
		for (size_t i = 0; i < work.recipes.size(); ++i)
		{
			work.count = static_cast<uint32_t>(get(bytes.data() + work.offset + 16, 4));
			work.index = recipe_index(work.recipe_values,
						  get(bytes.data() + work.offset, 8));
			if (work.index == work.recipes.size() ||
			    work.count != step_count(work.recipes[work.index]))
				return error::corrupt_evidence;
			work.offset += ITEM_BYTES + work.count;
		}
		read_body(work.view.body, &work.candidate);
		if (!birth_historical_context_valid_range(
			    command, work.recipe_values, work.candidate, work.view.count,
			    recovery_wire_item_access{ bytes, work.view },
			    recovery_wire_count_access{ bytes, work.view },
			    recovery_wire_effect_access{ bytes, work.view }, budget, policy))
			return budget.denied ? error::capacity : error::corrupt_evidence;
		work.request = 0;
		if (!recovery_rows(work.request, work.view.count,
				   sizeof(native_mobile_birth_recovery_item)) ||
		    !budget.admit(work.request))
			return error::capacity;
		work.candidate.items.reserve(work.view.count);
		if (!budget.retain(work.request))
			return error::capacity;
		work.transferred = work.request;
		work.offset = work.view.items_offset;
		for (size_t i = 0; i < work.view.count; ++i)
		{
			auto item = read_item(bytes.data() + work.offset);
			work.count = static_cast<uint32_t>(get(bytes.data() + work.offset + 16, 4));
			work.offset += ITEM_BYTES;
			work.request = 0;
			if (!recovery_rows(work.request, work.count,
					   sizeof(native_mobile_birth_recovery_effect)) ||
			    !budget.admit(work.request))
				return error::capacity;
			item.effects.reserve(work.count);
			if (!budget.retain(work.request) ||
			    !recovery_add(work.transferred, work.request))
				return error::capacity;
			for (size_t e = 0; e < work.count; ++e)
				item.effects.push_back(effect(bytes[work.offset++]));
			work.candidate.items.push_back(std::move(item));
		}
		static_assert(
			std::is_nothrow_move_assignable_v<native_mobile_birth_recovery_context>);
		*output = std::move(work.candidate);
		if (retained_context_heap_bytes)
			*retained_context_heap_bytes = work.transferred;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}

economic_accounting_error
native_mobile_birth_recovery_validate_bounded(const critical_native_recovery_envelope &envelope,
					      recovery_reserve_fn reserve, void *context,
					      size_t outer_live) noexcept
{
	// Same revision/two-phase guard precedes decode in the original envelope.
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending))
		return error::corrupt_evidence;
	if (!birth_historical_source_policy)
		return error::capacity;
	size_t base = outer_live;
	if (!recovery_add(base, birth_historical_validate_entry_bytes) || !reserve ||
	    !reserve(base, context))
		return error::capacity;
	native_mobile_birth_recovery_context value;
	std::span<const uint8_t> attachment = envelope.attachment;
	const auto status = native_mobile_birth_recovery_decode_bounded(
		envelope.command, attachment, &value, reserve, context, base);
	if (status != error::ok)
		return status;
	if (envelope.revision == 1 &&
	    (envelope.phase != critical_native_recovery_phase::execution_pending ||
	     value.receipt_present || !no_progress(value)))
		return error::corrupt_evidence;
	if (envelope.phase == critical_native_recovery_phase::continuation_pending &&
	    !body_terminal(value))
		return error::corrupt_evidence;
	return error::ok;
}
bool native_mobile_birth_recovery_valid_bounded(const critical_native_recovery_envelope &envelope,
						recovery_reserve_fn reserve, void *context,
						size_t outer_live) noexcept
{
	size_t base = outer_live;
	if (!recovery_add(base, birth_historical_valid_wrapper_source))
		return false;
	return native_mobile_birth_recovery_validate_bounded(envelope, reserve, context, base) ==
	       error::ok;
}

economic_accounting_error native_mobile_birth_recovery_decode_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_recovery_context *output, recovery_reserve_fn reserve, void *context,
	size_t outer, size_t *retained) noexcept
{
	return birth_recovery_decode_fixed_impl(command, bytes, output, recovery_policy::historical,
						reserve, context, outer, retained);
}
economic_accounting_error native_mobile_birth_cash_role_recovery_decode_fixed_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_recovery_context *output, recovery_reserve_fn reserve, void *context,
	size_t outer, size_t *retained) noexcept
{
	return birth_recovery_decode_fixed_impl(command, bytes, output,
						recovery_policy::ordinary_cash_role, reserve,
						context, outer, retained);
}
economic_accounting_error native_mobile_birth_shared_shop_recovery_decode_fixed_bounded(
	const critical_command &command, const std::span<const uint8_t> &bytes,
	native_mobile_birth_shared_shop_recovery_context *output, recovery_reserve_fn reserve,
	void *context, size_t outer, size_t *retained) noexcept
{
	if (!output)
		return error::corrupt_evidence;
	size_t source = 0, initial = 0, base = outer, query = outer;
	if (!recovery_add(query, birth_fixed_shared_query_bytes) || !reserve ||
	    !reserve(query, context) ||
	    !native_mobile_birth_shared_shop_recovery_fixed_source_frame_bytes(&source) ||
	    !native_mobile_birth_shared_shop_recovery_fixed_initial_inline_bytes(&initial) ||
	    !recovery_add(base, source) || !recovery_add(base, initial) || !reserve(base, context))
		return error::capacity;
	birth_historical_budget budget{ reserve, context, outer, source, initial };
	try
	{
		birth_fixed_shared_decode_work work;
		work.checked = preflight(&command, bytes, &work.view, recovery_policy::shared_shop);
		if (work.checked != error::ok)
			return work.checked;
		// Original first preflight is distinct from inner full preflight/command/
		// checkpoint/receipt/progress proof; do not collapse either pass.
		work.checked = birth_recovery_decode_fixed_impl(
			command, bytes, &work.candidate.progress, recovery_policy::shared_shop,
			birth_historical_budget::forward, &budget, base, &work.retained);
		if (work.checked != error::ok)
			return work.checked;
		if (!budget.retain(work.retained) || !budget.admit(work.view.checkpoint.size()))
			return error::capacity;
		work.candidate.original_checkpoint.assign(work.view.checkpoint.begin(),
							  work.view.checkpoint.end());
		if (!recovery_add(work.retained, work.candidate.original_checkpoint.capacity()))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<
			      native_mobile_birth_shared_shop_recovery_context>);
		*output = std::move(work.candidate);
		if (retained)
			*retained = work.retained;
		return error::ok;
	}
	catch (...)
	{
		return error::capacity;
	}
}
economic_accounting_error native_mobile_birth_cash_role_recovery_validate_fixed_bounded(
	const critical_native_recovery_envelope &envelope, recovery_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending))
		return error::corrupt_evidence;
	if (!birth_historical_source_policy)
		return error::capacity;
	size_t base = outer;
	if (!recovery_add(base, birth_historical_validate_entry_bytes) || !reserve ||
	    !reserve(base, context))
		return error::capacity;
	native_mobile_birth_recovery_context value;
	std::span<const uint8_t> bytes = envelope.attachment;
	const auto status = native_mobile_birth_cash_role_recovery_decode_fixed_bounded(
		envelope.command, bytes, &value, reserve, context, base);
	if (status != error::ok)
		return status;
	if (envelope.revision == 1 &&
	    (envelope.phase != critical_native_recovery_phase::execution_pending ||
	     value.receipt_present || !no_progress(value)))
		return error::corrupt_evidence;
	if (envelope.phase == critical_native_recovery_phase::continuation_pending &&
	    !body_terminal(value))
		return error::corrupt_evidence;
	return error::ok;
}
economic_accounting_error native_mobile_birth_shared_shop_recovery_validate_fixed_bounded(
	const critical_native_recovery_envelope &envelope, recovery_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending))
		return error::corrupt_evidence;
	if (!birth_historical_source_policy)
		return error::capacity;
	size_t base = outer;
	if (!recovery_add(base, birth_fixed_shared_validate_entry_bytes) || !reserve ||
	    !reserve(base, context))
		return error::capacity;
	native_mobile_birth_shared_shop_recovery_context value;
	std::span<const uint8_t> bytes = envelope.attachment;
	const auto status = native_mobile_birth_shared_shop_recovery_decode_fixed_bounded(
		envelope.command, bytes, &value, reserve, context, base);
	if (status != error::ok)
		return status;
	if (envelope.revision == 1 &&
	    (envelope.phase != critical_native_recovery_phase::execution_pending ||
	     value.progress.receipt_present || !no_progress(value.progress)))
		return error::corrupt_evidence;
	if (envelope.phase == critical_native_recovery_phase::continuation_pending &&
	    !body_terminal(value.progress))
		return error::corrupt_evidence;
	return error::ok;
}
bool native_mobile_birth_cash_role_recovery_valid_fixed_bounded(
	const critical_native_recovery_envelope &envelope, recovery_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	size_t base = outer;
	if (!recovery_add(base, birth_historical_valid_wrapper_source))
		return false;
	return native_mobile_birth_cash_role_recovery_validate_fixed_bounded(
		       envelope, reserve, context, base) == error::ok;
}
bool native_mobile_birth_shared_shop_recovery_valid_fixed_bounded(
	const critical_native_recovery_envelope &envelope, recovery_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	size_t base = outer;
	if (!recovery_add(base, birth_historical_valid_wrapper_source))
		return false;
	return native_mobile_birth_shared_shop_recovery_validate_fixed_bounded(
		       envelope, reserve, context, base) == error::ok;
}
