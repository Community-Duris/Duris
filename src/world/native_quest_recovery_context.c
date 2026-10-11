#include "world/native_quest_recovery_context.h"
#include "item/item_transfer_command.h"
#include "world/quest_mobile_native.h"
#include "core/config.h"
#include "core/structs.h"
#include "item/quest_reward_continuation.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace
{
constexpr std::array<uint8_t, 4> magic = { 'N', 'Q', 'R', '1' };
constexpr std::array<uint8_t, 4> child_magic = { 'N', 'Q', 'R', '2' };
constexpr std::array<uint8_t, 4> latest_magic = { 'N', 'Q', 'R', '3' };
constexpr std::array<uint8_t, 4> money_magic = { 'N', 'Q', 'R', '4' };
constexpr std::array<uint8_t, 4> fee_magic = { 'N', 'Q', 'R', '5' };
constexpr size_t limit = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;

// Match the original freeze_program's existing branch/goal/string charges.
// Bounds apply before decoded containers allocate, as well as before encoding.
constexpr size_t branch_charge = sizeof(quest_complete_data) + 2 * sizeof(std::vector<goal_data>) +
				 3 * sizeof(std::string) + 2 * sizeof(void *);
constexpr size_t goal_charge = 2 * sizeof(goal_data);
bool charge(size_t bytes, size_t *retained)
{
	if (!retained || *retained > limit || bytes > limit - *retained)
		return false;
	*retained += bytes;
	return true;
}

struct writer
{
	std::vector<uint8_t> bytes;
	void raw(std::span<const uint8_t> value)
	{
		if (value.size() > limit - bytes.size())
			throw std::length_error("native recovery context");
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	template <typename T> void number(T value)
	{
		std::array<uint8_t, sizeof(T)> data{};
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i));
		raw(data);
	}
	void blob(std::span<const uint8_t> value)
	{
		if (value.size() > limit)
			throw std::length_error("native recovery context");
		number<uint32_t>(static_cast<uint32_t>(value.size()));
		raw(value);
	}
	void text(const std::string &value)
	{
		if (value.find('\0') != std::string::npos)
			throw std::invalid_argument("native recovery text");
		blob({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
};

struct reader
{
	std::span<const uint8_t> bytes;
	size_t cursor = 0;
	size_t retained_program = sizeof(native_quest_recovery_context);
	bool raw(size_t size, std::span<const uint8_t> *out)
	{
		if (!out || size > bytes.size() - cursor)
			return false;
		*out = bytes.subspan(cursor, size);
		cursor += size;
		return true;
	}
	template <typename T> bool number(T *out)
	{
		std::span<const uint8_t> value;
		if (!out || !raw(sizeof(T), &value))
			return false;
		uint64_t decoded = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			decoded |= static_cast<uint64_t>(value[i]) << (8 * i);
		*out = static_cast<T>(decoded);
		return true;
	}
	bool blob(std::span<const uint8_t> *out)
	{
		uint32_t size = 0;
		return number(&size) && raw(size, out);
	}
	bool text(std::string *out, size_t maximum)
	{
		std::span<const uint8_t> value;
		if (!blob(&value) || value.size() > maximum ||
		    std::find(value.begin(), value.end(), 0) != value.end() ||
		    !charge(value.size() + 1, &retained_program))
			return false;
		out->assign(reinterpret_cast<const char *>(value.data()), value.size());
		return true;
	}
	bool boolean(bool *out)
	{
		uint8_t value = 0;
		if (!number(&value) || value > 1)
			return false;
		*out = value != 0;
		return true;
	}
};

bool stages_valid(std::span<const uint8_t> values)
{
	return std::all_of(values.begin(), values.end(), [](uint8_t value) { return value <= 2; });
}

bool money_receipt_valid(const item_transfer_payload &payload,
			 const native_quest_recovery_receipt &receipt)
{
	if (!receipt.present)
		return receipt == native_quest_recovery_receipt{};
	if ((receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES ||
	    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
	     (receipt.error_code != 0)))
		return false;
	item_native_mobile_money_result actual{}, expected{};
	if (!item_native_mobile_money_result_decode(
		    { receipt.result_payload.data(), receipt.result_size }, &actual) ||
	    !item_native_mobile_money_result_build(payload, &expected))
		return false;
	if (receipt.outcome == critical_apply_outcome::terminal_failure)
	{
		expected.player_wallet_revision =
			payload.native_money.projection.player_before_revision;
		expected.mobile_cash_revision =
			payload.native_money.projection.mobile_before_revision;
		expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
	}
	return actual == expected &&
	       receipt.durable_revision ==
		       std::max({ actual.player_wallet_revision, actual.mobile_cash_revision,
				  actual.mobile_revision, actual.player_custody_revision,
				  actual.stock_revision }) &&
	       std::all_of(receipt.result_payload.begin() + receipt.result_size,
			   receipt.result_payload.end(), [](uint8_t b) { return b == 0; });
}

bool fee_receipt_valid(const item_transfer_payload &payload,
		       const native_quest_recovery_receipt &receipt)
{
	if (!receipt.present)
		return receipt == native_quest_recovery_receipt{};
	if ((receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES ||
	    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
	     (receipt.error_code != 0)))
		return false;
	item_native_mobile_fee_result actual{}, expected{};
	if (!item_native_mobile_fee_result_decode(
		    { receipt.result_payload.data(), receipt.result_size }, &actual) ||
	    !item_native_mobile_fee_result_build(payload, &expected))
		return false;
	if (receipt.outcome == critical_apply_outcome::terminal_failure)
	{
		expected.mobile_cash_revision = payload.native_cost.projection.before_revision;
		expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
	}
	return actual == expected &&
	       receipt.durable_revision ==
		       std::max({ actual.mobile_cash_revision, actual.mobile_revision,
				  actual.stock_revision, actual.native_custody_revision,
				  actual.player_custody_revision }) &&
	       std::all_of(receipt.result_payload.begin() + receipt.result_size,
			   receipt.result_payload.end(), [](uint8_t v) { return v == 0; });
}

bool receipt_valid(const native_quest_recovery_receipt &receipt)
{
	if (!receipt.present)
		return receipt == native_quest_recovery_receipt{};
	if ((receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
		return false;
	item_transfer_result result{};
	if (!item_transfer_command_decode_result(receipt.result_payload.data(), receipt.result_size,
						 &result) ||
	    receipt.durable_revision !=
		    std::max({ result.from_owner_revision, result.to_owner_revision,
			       result.max_item_revision }) ||
	    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
	     (receipt.error_code != 0)))
		return false;
	return std::all_of(receipt.result_payload.begin() + receipt.result_size,
			   receipt.result_payload.end(), [](uint8_t value) { return value == 0; });
}

bool context_shape(const item_transfer_payload &payload, const native_quest_recovery_context &value)
{
	if (!(payload.native_money.present ? money_receipt_valid(payload, value.receipt) :
	      payload.native_cost.fee_only ? fee_receipt_valid(payload, value.receipt) :
					     receipt_valid(value.receipt)) ||
	    static_cast<uint8_t>(value.publication_stage) > 2 ||
	    (value.publication_stage != native_quest_recovery_publication_stage::captured &&
	     !value.receipt.present) ||
	    !stages_valid(value.publication_steps) || !stages_valid(value.give_messages) ||
	    !stages_valid(value.give_hooks) || !stages_valid(value.consumed_root_steps) ||
	    value.consumed_root_steps.size() !=
		    payload.native_recovery.consumed_root_order.size() ||
	    value.next_branch > value.branches.size() || value.child_handoff_stage > 2 ||
	    value.next_child_command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    (!value.next_child_command.empty() &&
	     (!value.child_handoff_stage || !value.branch_program_frozen ||
	      value.next_branch >= value.branches.size())) ||
	    ((value.child_handoff_stage == 0) !=
	     critical_operation_id_is_zero(value.next_child_operation)) ||
	    (!value.branch_program_frozen && (!value.branches.empty() || value.next_branch)) ||
	    (value.branch_program_frozen &&
	     !std::all_of(value.give_hooks.begin(), value.give_hooks.end(),
			  [](uint8_t stage) { return stage == 2; })))
		return false;
	if (payload.native_money.present &&
	    (value.branch_program_frozen || !value.branches.empty() || value.next_branch ||
	     !critical_operation_id_is_zero(value.parent_acceptance) ||
	     !critical_operation_id_is_zero(value.next_child_operation) ||
	     value.child_handoff_stage || !value.next_child_command.empty() ||
	     value.latest_child_branch || value.latest_child_revision ||
	     !value.latest_child_command.empty() || !value.latest_child_attachment.empty() ||
	     !value.consumed_root_steps.empty() ||
	     std::any_of(value.give_hooks.begin(), value.give_hooks.end(),
			 [](uint8_t stage) { return stage != 0; }) ||
	     std::any_of(value.publication_steps.begin() + 1, value.publication_steps.end(),
			 [](uint8_t stage) { return stage != 0; }) ||
	     (value.publication_stage == native_quest_recovery_publication_stage::captured &&
	      value.publication_steps[0] != 0) ||
	     (value.publication_stage ==
		      native_quest_recovery_publication_stage::physically_proven &&
	      value.publication_steps[0] !=
		      (value.receipt.outcome == critical_apply_outcome::terminal_failure ? 0 : 2))))
		return false;
	if (payload.native_cost.fee_only &&
	    (value.branch_program_frozen || !value.branches.empty() || value.next_branch ||
	     !critical_operation_id_is_zero(value.parent_acceptance) ||
	     !critical_operation_id_is_zero(value.next_child_operation) ||
	     value.child_handoff_stage || !value.next_child_command.empty() ||
	     value.latest_child_branch || value.latest_child_revision ||
	     !value.latest_child_command.empty() || !value.latest_child_attachment.empty() ||
	     !value.consumed_root_steps.empty() ||
	     std::any_of(value.give_hooks.begin(), value.give_hooks.end(),
			 [](uint8_t v) { return v != 0; }) ||
	     std::any_of(value.give_messages.begin(), value.give_messages.end(),
			 [](uint8_t v) { return v != 0; }) ||
	     std::any_of(value.publication_steps.begin() + 1, value.publication_steps.begin() + 4,
			 [](uint8_t v) { return v != 0; }) ||
	     (value.publication_stage == native_quest_recovery_publication_stage::captured &&
	      std::any_of(value.publication_steps.begin(), value.publication_steps.end(),
			  [](uint8_t v) { return v != 0; })) ||
	     (value.publication_stage ==
		      native_quest_recovery_publication_stage::physically_proven &&
	      value.publication_steps[0] !=
		      (value.receipt.outcome == critical_apply_outcome::terminal_failure ? 0 : 2))))
		return false;
	size_t retained_program = sizeof(native_quest_recovery_context);
	if (!charge(value.next_child_command.size(), &retained_program) ||
	    !charge(value.latest_child_command.size(), &retained_program) ||
	    !charge(value.latest_child_attachment.size(), &retained_program) ||
	    value.latest_child_command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    value.latest_child_attachment.size() > limit ||
	    (value.latest_child_command.empty() ?
		     (value.latest_child_revision || value.latest_child_branch ||
		      !value.latest_child_attachment.empty()) :
		     (!value.latest_child_revision || !value.branch_program_frozen ||
		      value.latest_child_branch >= value.branches.size() ||
		      value.latest_child_branch >= value.next_branch ||
		      value.latest_child_attachment.empty())))
		return false;
	if (value.branches.size() > limit / branch_charge ||
	    !charge(value.branches.size() * branch_charge, &retained_program))
		return false;
	for (const auto &branch : value.branches)
	{
		if ((!branch.message_present && !branch.message.empty()) ||
		    (!branch.disappear_message_present && !branch.disappear_message.empty()) ||
		    branch.message.size() >= MAX_STRING_LENGTH ||
		    branch.disappear_message.size() >= MAX_STRING_LENGTH ||
		    branch.definition_id.size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
		    branch.message.find('\0') != std::string::npos ||
		    branch.disappear_message.find('\0') != std::string::npos ||
		    branch.definition_id.find('\0') != std::string::npos ||
		    !charge(branch.message.size() + 1, &retained_program) ||
		    !charge(branch.disappear_message.size() + 1, &retained_program) ||
		    !charge(branch.definition_id.size() + 1, &retained_program) ||
		    branch.give.size() > limit / goal_charge ||
		    branch.receive.size() > limit / goal_charge ||
		    !charge(branch.give.size() * goal_charge, &retained_program) ||
		    !charge(branch.receive.size() * goal_charge, &retained_program))
			return false;
	}
	return true;
}

player_snapshot_codec_result original_forests(const item_transfer_payload &payload,
					      const native_quest_recovery_context &value,
					      std::vector<uint8_t> *native_bytes,
					      std::vector<uint8_t> *player_bytes)
{
	auto result = player_item_snapshot_list_encode(value.native_before, native_bytes);
	if (result != player_snapshot_codec_result::ok)
		return result;
	result = player_item_snapshot_list_encode(value.player_before, player_bytes);
	if (result != player_snapshot_codec_result::ok)
		return result;
	if (payload.native_money.present || payload.native_cost.fee_only)
	{
		// These zero-item actions change neither forest; verify both role-bound player cuts
		// against the same original literal bytes, never a synthetic root UID.
		return shop_trade_recovery_forest_verify(
			       *player_bytes, shop_trade_recovery_forest_role::player_before,
			       payload.native_recovery.player_before) &&
				       shop_trade_recovery_forest_verify(
					       *player_bytes,
					       shop_trade_recovery_forest_role::player_after,
					       payload.native_recovery.player_after) ?
			       player_snapshot_codec_result::ok :
			       player_snapshot_codec_result::invalid_value;
	}
	std::vector<player_item_snapshot> after;
	result = quest_mobile_native_items_transition(
		value.native_before, payload.native_mobile.reference, payload, &after);
	if (result != player_snapshot_codec_result::ok)
		return result;
	if (payload.native_mobile.action == item_native_mobile_action::acceptance)
	{
		std::vector<player_item_snapshot> selected, remaining;
		std::vector<uint8_t> selected_bytes, after_bytes;
		if (!shop_trade_recovery_forest_verify(
			    *player_bytes, shop_trade_recovery_forest_role::player_before,
			    payload.native_recovery.player_before))
			return player_snapshot_codec_result::invalid_value;
		result = player_item_snapshot_extract_subtree(value.player_before,
							      item_transfer_result_root(payload),
							      &selected, &remaining);
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = player_item_snapshot_list_encode(selected, &selected_bytes);
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = player_item_snapshot_list_encode(remaining, &after_bytes);
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (selected_bytes.size() != payload.item_blob_size ||
		    !std::equal(selected_bytes.begin(), selected_bytes.end(),
				payload.item_blob.begin()) ||
		    !shop_trade_recovery_forest_verify(
			    after_bytes, shop_trade_recovery_forest_role::player_after,
			    payload.native_recovery.player_after))
			return player_snapshot_codec_result::invalid_value;
	}
	return player_snapshot_codec_result::ok;
}

bool original_command(const critical_command &command, item_transfer_payload *payload,
		      std::vector<uint8_t> *encoded)
{
	return command.type == critical_command_type::item_transfer &&
	       command.publication_required &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       item_transfer_native_mobile_acknowledged_version(command.payload_version) &&
	       critical_command_encode(command, encoded) == critical_command_codec_result::ok &&
	       item_transfer_command_decode_payload(command, payload) &&
	       item_transfer_native_mobile_recovery_shape_valid(*payload);
}

// Structural original-command correlation only. This never authenticates
// current native rows, a source issuer, execution, physical publication or ACK.
bool child_command_shape(const critical_command &parent, const item_transfer_payload &payload,
			 const native_quest_recovery_context &value)
{
	if (value.next_child_command.empty())
		return true; // Historical NQR1 remains exact; its cold owner must retain gaps.
	if (payload.native_mobile.action != item_native_mobile_action::acceptance ||
	    value.parent_acceptance.bytes != parent.operation_id.bytes ||
	    value.next_child_operation.bytes == parent.operation_id.bytes ||
	    value.publication_stage != native_quest_recovery_publication_stage::physically_proven)
		return false;
	critical_command child;
	item_transfer_payload child_payload{};
	std::vector<uint8_t> canonical;
	if (critical_command_decode(value.next_child_command.data(),
				    value.next_child_command.size(),
				    &child) != critical_command_codec_result::ok ||
	    child.operation_id.bytes != value.next_child_operation.bytes ||
	    !original_command(child, &child_payload, &canonical) ||
	    canonical != value.next_child_command ||
	    child_payload.native_mobile.action != item_native_mobile_action::consumption ||
	    child_payload.native_recovery.player_pid != payload.native_recovery.player_pid ||
	    child_payload.native_mobile.final_giver_pid != payload.native_recovery.player_pid)
		return false;
	// Birth/provenance must be the same original lifetime. Later revisions
	// remain literal child facts, to be proved by its own original native owner.
	auto parent_birth = payload.native_mobile.reference;
	parent_birth.mobile_revision = child_payload.native_mobile.reference.mobile_revision;
	parent_birth.stock_revision = child_payload.native_mobile.reference.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> parent_reference{},
		child_reference{};
	return quest_mobile_native_reference_encode(parent_birth, &parent_reference) ==
		       player_snapshot_codec_result::ok &&
	       quest_mobile_native_reference_encode(child_payload.native_mobile.reference,
						    &child_reference) ==
		       player_snapshot_codec_result::ok &&
	       parent_reference == child_reference;
}

bool latest_child_shape(const critical_command &parent, const item_transfer_payload &payload,
			const native_quest_recovery_context &value)
{
	if (value.latest_child_command.empty())
		return true;
	// Check the leaf marker BEFORE decode, so crafted nested NQR3 input cannot recurse.
	if (value.latest_child_attachment.size() < magic.size() ||
	    (!std::equal(magic.begin(), magic.end(), value.latest_child_attachment.begin()) &&
	     !std::equal(fee_magic.begin(), fee_magic.end(),
			 value.latest_child_attachment.begin())))
		return false;
	critical_command child;
	item_transfer_payload child_payload{};
	native_quest_recovery_context child_context, correlation;
	if (critical_command_decode(value.latest_child_command.data(),
				    value.latest_child_command.size(),
				    &child) != critical_command_codec_result::ok ||
	    !item_transfer_command_decode_payload(child, &child_payload) ||
	    child_payload.continuation.kind != item_transfer_continuation_kind::none ||
	    (std::equal(fee_magic.begin(), fee_magic.end(),
			value.latest_child_attachment.begin()) !=
	     child_payload.native_cost.fee_only) ||
	    native_quest_recovery_context_decode(child, value.latest_child_attachment,
						 &child_context) !=
		    player_snapshot_codec_result::ok ||
	    child_context.publication_stage !=
		    native_quest_recovery_publication_stage::physically_proven ||
	    !child_context.receipt.present || child_context.receipt.error_code ||
	    (child_context.receipt.outcome != critical_apply_outcome::applied &&
	     child_context.receipt.outcome != critical_apply_outcome::already_applied) ||
	    child_context.branch_program_frozen || child_context.child_handoff_stage ||
	    (!critical_operation_id_is_zero(child_context.parent_acceptance) &&
	     child_context.parent_acceptance.bytes != parent.operation_id.bytes))
		return false;
	correlation.parent_acceptance = value.parent_acceptance;
	correlation.publication_stage = value.publication_stage;
	correlation.next_child_operation = child.operation_id;
	correlation.next_child_command = value.latest_child_command;
	return child_command_shape(parent, payload, correlation);
}

void write_goals(writer &out, const std::vector<native_quest_recovery_goal> &values)
{
	if (values.size() > limit / 5)
		throw std::length_error("native recovery goals");
	out.number<uint32_t>(static_cast<uint32_t>(values.size()));
	for (const auto &goal : values)
	{
		out.number<uint8_t>(goal.type);
		out.number<int32_t>(goal.number);
	}
}

bool read_goals(reader &in, std::vector<native_quest_recovery_goal> *values)
{
	uint32_t count = 0;
	if (!in.number(&count) || count > (in.bytes.size() - in.cursor) / 5 ||
	    count > limit / goal_charge || !charge(count * goal_charge, &in.retained_program))
		return false;
	values->resize(count);
	for (auto &goal : *values)
		if (!in.number(&goal.type) || !in.number(&goal.number))
			return false;
	return true;
}
}

player_snapshot_codec_result
native_quest_recovery_context_encode(const critical_command &command,
				     const native_quest_recovery_context &value,
				     std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> encoded_command, native_bytes, player_bytes;
		if (!original_command(command, &payload, &encoded_command) ||
		    !context_shape(payload, value) ||
		    !child_command_shape(command, payload, value) ||
		    !latest_child_shape(command, payload, value))
			return player_snapshot_codec_result::invalid_value;
		auto result = original_forests(payload, value, &native_bytes, &player_bytes);
		if (result != player_snapshot_codec_result::ok)
			return result;
		writer out;
		out.raw(payload.native_cost.fee_only ?
				fee_magic :
			payload.native_money.present ?
				money_magic :
				(!value.latest_child_command.empty() ?
					 latest_magic :
					 (value.next_child_command.empty() ? magic : child_magic)));
		out.blob(encoded_command);
		out.blob(native_bytes);
		out.blob(player_bytes);
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
		if (value.branches.size() > limit / 24)
			return player_snapshot_codec_result::limit_exceeded;
		out.number<uint32_t>(static_cast<uint32_t>(value.branches.size()));
		for (const auto &branch : value.branches)
		{
			out.number<uint8_t>(branch.message_present);
			out.number<uint8_t>(branch.disappear_message_present);
			out.number<uint8_t>(branch.echo_all);
			out.number<uint8_t>(branch.disappear);
			out.text(branch.message);
			out.text(branch.disappear_message);
			out.text(branch.definition_id);
			write_goals(out, branch.give);
			write_goals(out, branch.receive);
		}
		if (!value.next_child_command.empty() || !value.latest_child_command.empty())
			out.blob(value.next_child_command);
		if (!value.latest_child_command.empty())
		{
			out.number<uint32_t>(value.latest_child_branch);
			out.number<uint64_t>(value.latest_child_revision);
			out.blob(value.latest_child_command);
			out.blob(value.latest_child_attachment);
		}
		*output = std::move(out.bytes);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	catch (const std::length_error &)
	{
		return player_snapshot_codec_result::limit_exceeded;
	}
	catch (...)
	{
		return player_snapshot_codec_result::invalid_value;
	}
}

player_snapshot_codec_result
native_quest_recovery_context_decode(const critical_command &command,
				     std::span<const uint8_t> bytes,
				     native_quest_recovery_context *output) noexcept
{
	if (!output || bytes.size() > limit)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		reader in{ bytes };
		std::span<const uint8_t> value, native_bytes, player_bytes;
		item_transfer_payload payload{};
		std::vector<uint8_t> encoded_command;
		native_quest_recovery_context candidate;
		uint8_t outcome = 0, publication = 0;
		uint16_t failure = 0;
		if (!original_command(command, &payload, &encoded_command) ||
		    !in.raw(magic.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		const bool with_latest =
			std::equal(value.begin(), value.end(), latest_magic.begin());
		const bool with_child = with_latest ||
					std::equal(value.begin(), value.end(), child_magic.begin());
		const bool with_money = std::equal(value.begin(), value.end(), money_magic.begin());
		const bool with_fee = std::equal(value.begin(), value.end(), fee_magic.begin());
		if (with_fee != payload.native_cost.fee_only ||
		    with_money != payload.native_money.present ||
		    (!with_fee && !with_money && !with_child &&
		     !std::equal(value.begin(), value.end(), magic.begin())) ||
		    !in.blob(&value) || value.size() != encoded_command.size() ||
		    !std::equal(value.begin(), value.end(), encoded_command.begin()) ||
		    !in.blob(&native_bytes) || !in.blob(&player_bytes) ||
		    !in.boolean(&candidate.receipt.present) || !in.number(&outcome) ||
		    !in.number(&candidate.receipt.durable_revision) ||
		    !in.number(&candidate.receipt.error_code) || !in.number(&failure) ||
		    !in.number(&candidate.receipt.result_size) ||
		    !in.raw(candidate.receipt.result_payload.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		std::copy(value.begin(), value.end(), candidate.receipt.result_payload.begin());
		candidate.receipt.outcome = static_cast<critical_apply_outcome>(outcome);
		candidate.receipt.failure_stage = static_cast<critical_failure_stage>(failure);
		if (!in.number(&publication) || !in.raw(candidate.publication_steps.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		candidate.publication_stage =
			static_cast<native_quest_recovery_publication_stage>(publication);
		std::copy(value.begin(), value.end(), candidate.publication_steps.begin());
		if (!in.raw(candidate.give_messages.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		std::copy(value.begin(), value.end(), candidate.give_messages.begin());
		if (!in.blob(&value) ||
		    value.size() != payload.native_recovery.consumed_root_order.size())
			return player_snapshot_codec_result::invalid_value;
		candidate.consumed_root_steps.assign(value.begin(), value.end());
		if (!in.raw(candidate.give_hooks.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		std::copy(value.begin(), value.end(), candidate.give_hooks.begin());
		if (!in.boolean(&candidate.branch_program_frozen) ||
		    !in.number(&candidate.next_branch) ||
		    !in.raw(candidate.parent_acceptance.bytes.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		std::copy(value.begin(), value.end(), candidate.parent_acceptance.bytes.begin());
		if (!in.raw(candidate.next_child_operation.bytes.size(), &value))
			return player_snapshot_codec_result::invalid_value;
		std::copy(value.begin(), value.end(), candidate.next_child_operation.bytes.begin());
		uint32_t branch_count = 0;
		if (!in.number(&candidate.child_handoff_stage) || !in.number(&branch_count) ||
		    branch_count > (bytes.size() - in.cursor) / 24 ||
		    branch_count > limit / branch_charge ||
		    !charge(branch_count * branch_charge, &in.retained_program) ||
		    (!candidate.branch_program_frozen && branch_count))
			return player_snapshot_codec_result::invalid_value;
		candidate.branches.resize(branch_count);
		for (auto &branch : candidate.branches)
			if (!in.boolean(&branch.message_present) ||
			    !in.boolean(&branch.disappear_message_present) ||
			    !in.boolean(&branch.echo_all) || !in.boolean(&branch.disappear) ||
			    !in.text(&branch.message, MAX_STRING_LENGTH - 1) ||
			    !in.text(&branch.disappear_message, MAX_STRING_LENGTH - 1) ||
			    !in.text(&branch.definition_id, QUEST_REWARD_MAX_DEFINITION_ID_BYTES) ||
			    !read_goals(in, &branch.give) || !read_goals(in, &branch.receive))
				return player_snapshot_codec_result::invalid_value;
		if (with_child)
		{
			if (!in.blob(&value) || (!with_latest && value.empty()) ||
			    value.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
			    !charge(value.size(), &in.retained_program))
				return player_snapshot_codec_result::invalid_value;
			candidate.next_child_command.assign(value.begin(), value.end());
		}
		if (with_latest)
		{
			if (!in.number(&candidate.latest_child_branch) ||
			    !in.number(&candidate.latest_child_revision) || !in.blob(&value) ||
			    value.empty() || value.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
			    !charge(value.size(), &in.retained_program))
				return player_snapshot_codec_result::invalid_value;
			candidate.latest_child_command.assign(value.begin(), value.end());
			if (!in.blob(&value) || value.empty() ||
			    !charge(value.size(), &in.retained_program))
				return player_snapshot_codec_result::invalid_value;
			candidate.latest_child_attachment.assign(value.begin(), value.end());
		}
		if (in.cursor != bytes.size() || !context_shape(payload, candidate))
			return player_snapshot_codec_result::invalid_value;
		auto decoded = player_item_snapshot_list_decode(
			native_bytes.data(), native_bytes.size(), &candidate.native_before);
		if (decoded != player_snapshot_codec_result::ok)
			return decoded;
		decoded = player_item_snapshot_list_decode(player_bytes.data(), player_bytes.size(),
							   &candidate.player_before);
		if (decoded != player_snapshot_codec_result::ok)
			return decoded;
		std::vector<uint8_t> canonical;
		auto result = native_quest_recovery_context_encode(command, candidate, &canonical);
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return player_snapshot_codec_result::invalid_value;
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	catch (...)
	{
		return player_snapshot_codec_result::invalid_value;
	}
}

bool native_quest_recovery_publication_context_valid(
	const critical_native_recovery_envelope &envelope,
	const critical_completion &completion) noexcept
{
	native_quest_recovery_context context;
	if (envelope.phase != critical_native_recovery_phase::execution_pending ||
	    !envelope.revision ||
	    completion.disposition != critical_completion_disposition::execution ||
	    completion.operation_id.bytes != envelope.command.operation_id.bytes ||
	    native_quest_recovery_context_decode(envelope.command, envelope.attachment, &context) !=
		    player_snapshot_codec_result::ok ||
	    context.publication_stage !=
		    native_quest_recovery_publication_stage::physically_proven ||
	    !context.receipt.present)
		return false;
	const bool same_outcome =
		context.receipt.outcome == completion.outcome ||
		((context.receipt.outcome == critical_apply_outcome::applied ||
		  context.receipt.outcome == critical_apply_outcome::already_applied) &&
		 (completion.outcome == critical_apply_outcome::applied ||
		  completion.outcome == critical_apply_outcome::already_applied));
	return same_outcome && context.receipt.durable_revision == completion.durable_revision &&
	       context.receipt.error_code == completion.error_code &&
	       context.receipt.failure_stage == completion.failure_stage &&
	       context.receipt.result_size == completion.result_size &&
	       context.receipt.result_payload == completion.result_payload;
}

// Exact fee ACK after the journal transition, while the original hold remains.
bool native_quest_recovery_fee_ack_context_valid(const critical_native_recovery_envelope &envelope,
						 const critical_completion &completion) noexcept
{
	native_quest_recovery_context context;
	item_transfer_payload payload{};
	if (envelope.phase != critical_native_recovery_phase::continuation_pending ||
	    envelope.command.payload_version !=
		    ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ||
	    !item_transfer_command_decode_payload(envelope.command, &payload) ||
	    !payload.native_cost.fee_only || !envelope.revision ||
	    completion.disposition != critical_completion_disposition::execution ||
	    completion.operation_id.bytes != envelope.command.operation_id.bytes ||
	    native_quest_recovery_context_decode(envelope.command, envelope.attachment, &context) !=
		    player_snapshot_codec_result::ok ||
	    context.publication_stage !=
		    native_quest_recovery_publication_stage::physically_proven ||
	    !context.receipt.present)
		return false;
	const bool same_outcome =
		context.receipt.outcome == completion.outcome ||
		((context.receipt.outcome == critical_apply_outcome::applied ||
		  context.receipt.outcome == critical_apply_outcome::already_applied) &&
		 (completion.outcome == critical_apply_outcome::applied ||
		  completion.outcome == critical_apply_outcome::already_applied));
	return same_outcome && context.receipt.durable_revision == completion.durable_revision &&
	       context.receipt.error_code == completion.error_code &&
	       context.receipt.failure_stage == completion.failure_stage &&
	       context.receipt.result_size == completion.result_size &&
	       context.receipt.result_payload == completion.result_payload;
}

bool native_quest_recovery_pair_context_valid(
	const critical_native_recovery_envelope &parent,
	const critical_native_recovery_envelope &child,
	const critical_native_recovery_envelope *successor) noexcept
{
	try
	{
		native_quest_recovery_context context, child_context;
		item_transfer_payload child_payload{};
		std::vector<uint8_t> child_bytes;
		if (!parent.revision || !child.revision ||
		    parent.phase != critical_native_recovery_phase::continuation_pending ||
		    child.phase != critical_native_recovery_phase::continuation_pending ||
		    parent.command.operation_id.bytes == child.command.operation_id.bytes ||
		    native_quest_recovery_context_decode(parent.command, parent.attachment,
							 &context) !=
			    player_snapshot_codec_result::ok ||
		    (context.child_handoff_stage != 1 && context.child_handoff_stage != 2) ||
		    context.next_child_operation.bytes != child.command.operation_id.bytes ||
		    critical_command_encode(child.command, &child_bytes) !=
			    critical_command_codec_result::ok ||
		    context.next_child_command != child_bytes ||
		    child.attachment.size() < magic.size() ||
		    (!std::equal(magic.begin(), magic.end(), child.attachment.begin()) &&
		     !(child.command.payload_version ==
			       ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION &&
		       std::equal(fee_magic.begin(), fee_magic.end(), child.attachment.begin()))) ||
		    !item_transfer_command_decode_payload(child.command, &child_payload) ||
		    native_quest_recovery_context_decode(child.command, child.attachment,
							 &child_context) !=
			    player_snapshot_codec_result::ok ||
		    child_context.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven ||
		    !child_context.receipt.present ||
		    (!critical_operation_id_is_zero(child_context.parent_acceptance) &&
		     child_context.parent_acceptance.bytes != parent.command.operation_id.bytes))
			return false;
		if (child_payload.native_cost.fee_only)
		{
			item_transfer_payload accepted{};
			item_transfer_result receipt{};
			if (parent.command.payload_version !=
				    ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ||
			    !item_transfer_command_decode_payload(parent.command, &accepted) ||
			    accepted.native_mobile.action !=
				    item_native_mobile_action::acceptance ||
			    child_payload.native_cost.completion_slot != context.next_branch ||
			    !context.receipt.present || context.receipt.error_code ||
			    (context.receipt.outcome != critical_apply_outcome::applied &&
			     context.receipt.outcome != critical_apply_outcome::already_applied) ||
			    context.receipt.result_size != ITEM_TRANSFER_RESULT_BYTES ||
			    !item_transfer_command_decode_result(
				    context.receipt.result_payload.data(),
				    context.receipt.result_size, &receipt) ||
			    receipt.root_item_uid != item_transfer_result_root(accepted) ||
			    receipt.item_count != accepted.item_count || receipt.corpse_revision ||
			    receipt.collector_catalog_changed ||
			    accepted.expected_from_revision == UINT64_MAX ||
			    accepted.expected_to_revision == UINT64_MAX ||
			    receipt.from_owner_revision != accepted.expected_from_revision + 1 ||
			    receipt.to_owner_revision != accepted.expected_to_revision + 1)
				return false;
			uint64_t maximum = 0;
			for (size_t i = 0; i < accepted.item_count; ++i)
			{
				if (accepted.items[i].expected_item_revision == UINT64_MAX)
					return false;
				maximum = std::max(maximum,
						   accepted.items[i].expected_item_revision + 1);
			}
			if (receipt.max_item_revision != maximum)
				return false;
			if (child_payload.continuation.kind ==
			    item_transfer_continuation_kind::quest_offering)
			{
				quest_reward_continuation terms;
				if (!quest_reward_continuation_decode(
					    child_payload.continuation.data.data(),
					    child_payload.continuation.data.size(), &terms) ||
				    !quest_fee_reward_trigger_binding_valid(terms,
									    parent.command) ||
				    terms.action_operation.bytes !=
					    child.command.operation_id.bytes ||
				    !std::equal(terms.original_acceptance_result.begin(),
						terms.original_acceptance_result.end(),
						context.receipt.result_payload.begin()))
					return false;
			}
			else if (child_payload.continuation.kind !=
				 item_transfer_continuation_kind::none)
				return false;
		}
		if (!successor)
			return child_payload.continuation.kind ==
				       item_transfer_continuation_kind::quest_offering ||
			       child_context.receipt.outcome ==
				       critical_apply_outcome::terminal_failure;
		// Terminal ACK/rejection can precede the parent handoff2 checkpoint.
		// Only cleanup accepts handoff1; prefix advancement still needs handoff2.
		if (context.child_handoff_stage != 2 || parent.revision == UINT64_MAX ||
		    successor->revision != parent.revision + 1 ||
		    successor->phase != parent.phase ||
		    !critical_command_equal(parent.command, successor->command) ||
		    child_payload.continuation.kind != item_transfer_continuation_kind::none ||
		    child_context.receipt.error_code ||
		    (child_context.receipt.outcome != critical_apply_outcome::applied &&
		     child_context.receipt.outcome != critical_apply_outcome::already_applied) ||
		    context.next_branch >= context.branches.size())
			return false;
		context.latest_child_branch = context.next_branch;
		context.latest_child_revision = child.revision;
		context.latest_child_command = std::move(child_bytes);
		context.latest_child_attachment = child.attachment;
		++context.next_branch;
		context.next_child_command.clear();
		context.next_child_operation = {};
		context.child_handoff_stage = 0;
		std::vector<uint8_t> canonical;
		return native_quest_recovery_context_encode(parent.command, context, &canonical) ==
			       player_snapshot_codec_result::ok &&
		       canonical == successor->attachment;
	}
	catch (...)
	{
		return false;
	}
}

#include <cerrno>
#include <functional>
#include <iterator>
namespace
{
constexpr size_t recovery_library_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t recovery_library_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t recovery_library_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t recovery_library_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint8_t);
constexpr size_t recovery_library_vector_frames =
	recovery_library_allocator_frames + recovery_library_copy_frames +
	recovery_library_relocate_frames + recovery_library_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t recovery_library_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + recovery_library_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t recovery_library_nontrivial_frames =
	// default_n_1<false>: first/n/cur/return; _Construct/addressof/placement.
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) +
	// Actual aggregate row and description this, four row strings and two
	// description strings: string()/allocator hider/use-local-data/set-length.
	2 * sizeof(void *) +
	6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	// row/description nested vector()/Vector_base()/Vector_impl()/data() and
	// allocator return carriers. Three source member vector types.
	3 * (5 * sizeof(void *) + sizeof(std::allocator<int32_t>)) +
	// Nontrivial _Destroy range/aux::__destroy/destroy_at/__addressof; actual
	// row/description destructor this then six string destructors/dispose/
	// _M_is_local/_M_destroy and three nested vector destroy/deallocate scopes.
	8 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	3 * recovery_library_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t recovery_library_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t recovery_library_string_frames =
	recovery_library_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	recovery_library_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
// Genuine vector(n,value,allocator) constructor scopes, before fill:
// vector this/n/value-reference/allocator-reference and default allocator;
// _S_check_init_len n/a/result and its _Tp allocator copy; _Vector_base
// this/n/a, _Vector_impl this/a and allocator copy, _Vector_impl_data this;
// _M_create_storage this/n. Existing allocator profile owns _S_max_size.
constexpr size_t recovery_library_size_constructor_frames =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<size_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<size_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Actual codec linked owner/row-vector observation and relay source scopes.
// These declared carriers complement the real T-specific CURRENT profiles.
constexpr size_t recovery_owner_source_frames =
	// add/rows/capacity + byte/row/payload/intent/context overload arguments,
	// genuine const row range iterators, row reference and current-heap scalar.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 4 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 10 * sizeof(void *) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + 2 * sizeof(size_t) +
	4 * sizeof(bool) +
	// current(this,out), bytes and actual linked cursor; prefix/peak this,
	// outputs, requests/frames/bytes plus callback arguments/results.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 5 * sizeof(void *) +
	7 * sizeof(size_t) + 4 * sizeof(bool) +
	// invoke this/F-reference/failure/frames/local value, actual lambda prefix;
	// watcher ctor this/owner/value and destructor this; observe formal pair.
	2 * sizeof(void *) + sizeof(bool) + 3 * sizeof(size_t) + 6 * sizeof(void *) + sizeof(bool);
}

namespace
{
// Actual installed span/array/initializer_list source constructor/accessors.
// The view object itself is a real caller local,priced separately.
constexpr size_t recovery_library_view_frames =
	// span(pointer,count) this/first/count; to_address/__to_address params+results;
	// dynamic extent constructor and accessor each actual receiver/size value.
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) +
	// span default/array/converting constructors,actual reference and extent;
	// begin/end actual normal_iterator return and ctor source references.
	4 * sizeof(void *) + sizeof(size_t) + 2 * (4 * sizeof(void *)) +
	// subspan this/offset/count/returned span; size/empty/data/index receiver/
	// result,where size reaches the real extent accessor.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::span<const uint8_t>) +
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// array begin/end/data/_S_ptr and actual operator[]/_S_ref argument/results.
	4 * (4 * sizeof(void *)) + 3 * sizeof(void *) + sizeof(size_t) +
	// initializer_list private compiler ctor and member begin/end/size.
	2 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
}
namespace
{

// Branch aggregate constructor/destructor: actual three strings and two goal
// vectors, excluding their inline members (owned by the vector request).
constexpr size_t recovery_branch_member_frames =
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	3 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	2 * (5 * sizeof(void *) + sizeof(std::allocator<native_quest_recovery_goal>)) +
	8 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(void *) +
	3 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	2 * recovery_library_allocator_frames;
// Goal aggregate default constructor is nontrivial (default initializers);
// _default_n_1<false> owns first/n/cur/result and _Construct/placement carriers.
// Its destructor is trivial. Goal scalar members live in the element request.
constexpr size_t recovery_goal_default_frames =
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) + sizeof(void *);
// Exact equal-allocator basic_string move source template, re-instantiated
// for the three genuine branch members. Vector moves transfer their actual
// three pointers and allocator; members remain part of the element request.
constexpr size_t recovery_branch_move_frames =
	2 * sizeof(void *) +
	3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	     12 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	     sizeof(char) + recovery_library_string_frames) +
	2 * recovery_library_move_frames;
// Generic byte copy/equality and pointer/normal-iterator iterator-base paths.
// std::copy/_copy_move_a/_a1/_a2/_copy_m and the actual count/memmove scopes
// are the same authenticated contiguous instantiations as the codec template.
constexpr size_t recovery_byte_algorithm_frames =
	recovery_library_copy_frames + 3 * sizeof(void *) +
	2 * (3 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	// all_of/any_of -> find_if(_not)/__find_if and predicate adapters, actual
	// random-access difference/trip-count/first/last/predicate/category/returns.
	4 * (3 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(std::ptrdiff_t) + sizeof(void *) +
	2 * sizeof(std::random_access_iterator_tag) + 4 * sizeof(bool) + sizeof(uint8_t) +
	// find(0), basic_string::find and char_traits::find/memchr scope formals.
	6 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(char) + sizeof(bool);

}
// Private source candidate: source-carrier closure still requires independent
// review. The original selected codecs are not changed by this companion.
namespace
{
using recovery_reserve = bool (*)(size_t, void *) noexcept;
bool recovery_add(size_t &bytes, size_t value) noexcept
{
	if (value > SIZE_MAX - bytes)
	{
		errno = EOVERFLOW;
		return false;
	}
	bytes += value;
	return true;
}
template <class T> bool recovery_rows(size_t &bytes, const std::vector<T> &value) noexcept
{
	if (value.capacity() > SIZE_MAX / sizeof(T))
	{
		errno = EOVERFLOW;
		return false;
	}
	return recovery_add(bytes, value.capacity() * sizeof(T));
}
bool recovery_heap(const std::vector<uint8_t> &value, size_t &bytes) noexcept
{
	return recovery_rows(bytes, value);
}
bool recovery_heap(const std::vector<player_item_snapshot> &value, size_t &bytes) noexcept
{
	if (!recovery_rows(bytes, value))
		return false;
	for (const auto &row : value)
	{
		size_t heap = 0;
		if (!player_item_snapshot_current_heap_bytes(row, &heap) ||
		    !recovery_add(bytes, heap))
			return false;
	}
	return true;
}
bool recovery_heap(const item_transfer_payload &value, size_t &bytes) noexcept
{
	size_t heap = 0;
	return item_transfer_payload_current_heap_bytes(value, &heap) && recovery_add(bytes, heap);
}

bool recovery_text(size_t &bytes, const std::string &text) noexcept
{
	return text.capacity() <= 15 ||
	       (text.capacity() != SIZE_MAX && recovery_add(bytes, text.capacity() + 1));
}
bool recovery_heap(const critical_command &value, size_t &bytes) noexcept
{
	size_t heap = 0;
	return critical_command_current_heap_bytes(value, &heap) && recovery_add(bytes, heap);
}
bool recovery_heap(const native_quest_recovery_context &value, size_t &bytes) noexcept
{
	if (!recovery_heap(value.native_before, bytes) ||
	    !recovery_heap(value.player_before, bytes) || !recovery_rows(bytes, value.branches) ||
	    !recovery_rows(bytes, value.consumed_root_steps) ||
	    !recovery_rows(bytes, value.next_child_command) ||
	    !recovery_rows(bytes, value.latest_child_command) ||
	    !recovery_rows(bytes, value.latest_child_attachment))
		return false;
	for (const auto &branch : value.branches)
		if (!recovery_text(bytes, branch.message) ||
		    !recovery_text(bytes, branch.disappear_message) ||
		    !recovery_text(bytes, branch.definition_id) ||
		    !recovery_rows(bytes, branch.give) || !recovery_rows(bytes, branch.receive))
			return false;
	return true;
}

struct recovery_codec_scope;
struct recovery_codec_live
{
	recovery_codec_scope &scope;
	recovery_codec_live *prior;
	const void *object;
	size_t inline_bytes;
	bool (*observe)(const void *, size_t &) noexcept;
	template <class T> recovery_codec_live(recovery_codec_scope &, const T &) noexcept;
	~recovery_codec_live();
};
struct recovery_codec_scope
{
	template <class F, class R> R invoke_codec(F &&body, R failure, size_t frames)
	{
		size_t value = 0;
		return peak(native_quest_recovery_codec_entry_inline_bytes(), frames) &&
				       prefix(&value, frames) ?
			       body(value) :
			       failure;
	}
	template <class F, class R> R invoke(F &&body, R failure, size_t frames)
	{
		size_t value = 0;
		return prefix(&value, frames) ? body(value) : failure;
	}
	recovery_reserve reserve;
	void *context;
	size_t outer;
	recovery_codec_live *live = nullptr;
	const recovery_codec_scope *parent = nullptr;
	size_t parent_frames = 0;
	bool current(size_t *output) const noexcept
	{
		size_t bytes = 0;
		if (parent && !parent->current(&bytes))
			return false;
		if (!recovery_add(bytes, outer) || !recovery_add(bytes, parent_frames))
			return false;
		if (!output || !recovery_add(bytes, sizeof(*this)))
			return false;
		for (const auto *entry = live; entry; entry = entry->prior)
			if (!recovery_add(bytes, sizeof(*entry)) ||
			    !recovery_add(bytes, entry->inline_bytes) ||
			    !entry->observe(entry->object, bytes))
				return false;
		*output = bytes;
		return true;
	}
	bool prefix(size_t *output, size_t frames) noexcept
	{
		size_t bytes = 0;
		if (!current(&bytes) || !recovery_add(bytes, frames) || !reserve ||
		    !reserve(bytes, context))
		{
			errno = ENOBUFS;
			return false;
		}
		*output = bytes;
		return true;
	}
	bool peak(size_t requests, size_t frames) noexcept
	{
		size_t bytes = 0;
		if (!current(&bytes) || !recovery_add(bytes, requests) ||
		    !recovery_add(bytes, frames) || !reserve || !reserve(bytes, context))
		{
			errno = ENOBUFS;
			return false;
		}
		return true;
	}
};
template <class T>
recovery_codec_live::recovery_codec_live(recovery_codec_scope &owner, const T &value) noexcept
	: scope(owner)
	, prior(owner.live)
	, object(&value)
	, inline_bytes(sizeof(T))
	, observe([](const void *pointer, size_t &bytes) noexcept
		  { return recovery_heap(*static_cast<const T *>(pointer), bytes); })
{
	owner.live = this;
}
recovery_codec_live::~recovery_codec_live()
{
	scope.live = prior;
}
}

namespace
{
struct nqr_bounded_reader
{
	recovery_codec_scope &scope;
	std::span<const uint8_t> bytes;
	player_snapshot_codec_result result = player_snapshot_codec_result::ok;
	size_t cursor = 0;
	size_t retained_program = sizeof(native_quest_recovery_context);
	bool raw(size_t size, std::span<const uint8_t> *out)
	{
		if (!out || size > bytes.size() - cursor)
			return false;
		*out = bytes.subspan(cursor, size);
		cursor += size;
		return true;
	}
	template <typename T> bool number(T *out)
	{
		std::span<const uint8_t> value;
		if (!out || !raw(sizeof(T), &value))
			return false;
		uint64_t decoded = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			decoded |= static_cast<uint64_t>(value[i]) << (8 * i);
		*out = static_cast<T>(decoded);
		return true;
	}
	bool blob(std::span<const uint8_t> *out)
	{
		uint32_t size = 0;
		return number(&size) && raw(size, out);
	}
	bool text(std::string *out, size_t maximum)
	{
		std::span<const uint8_t> value;
		if (!blob(&value) || value.size() > maximum ||
		    std::find(value.begin(), value.end(), 0) != value.end() ||
		    !charge(value.size() + 1, &retained_program))
			return false;

		if (value.size() > out->capacity())
		{
			size_t request = value.size();
			if (request < 2 * out->capacity())
				request = 2 * out->capacity();
			if (request == SIZE_MAX ||
			    !scope.peak(request + 1,
					native_quest_recovery_codec_source_frame_bytes()))
			{
				result = player_snapshot_codec_result::allocation_failure;
				return false;
			}
		}
		out->assign(reinterpret_cast<const char *>(value.data()), value.size());
		return true;
	}
	bool boolean(bool *out)
	{
		uint8_t value = 0;
		if (!number(&value) || value > 1)
			return false;
		*out = value != 0;
		return true;
	}
};

struct nqr_bounded_writer
{
	recovery_codec_scope &scope;
	std::vector<uint8_t> bytes;
	player_snapshot_codec_result result = player_snapshot_codec_result::ok;
	void raw(std::span<const uint8_t> value)
	{
		if (value.size() > limit - bytes.size())
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return;
		}

		if (value.size() > bytes.capacity() - bytes.size())
		{
			const size_t growth = std::max(bytes.size(), value.size());
			if (growth > SIZE_MAX - bytes.size() ||
			    !scope.peak(bytes.size() + growth,
					native_quest_recovery_codec_source_frame_bytes()))
			{
				result = player_snapshot_codec_result::allocation_failure;
				return;
			}
		}
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	template <typename T> void number(T value)
	{
		std::array<uint8_t, sizeof(T)> data{};
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i));
		raw(data);
	}
	void blob(std::span<const uint8_t> value)
	{
		if (value.size() > limit)
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return;
		}
		number<uint32_t>(static_cast<uint32_t>(value.size()));
		if (result != player_snapshot_codec_result::ok)
			return;
		raw(value);
	}
	void text(const std::string &value)
	{
		if (value.find('\0') != std::string::npos)
		{
			result = player_snapshot_codec_result::invalid_value;
			return;
		}
		blob({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
};

bool native_money_result_valid_owned(const item_native_mobile_money_result &value) noexcept
{
	return value.mobile_instance_id && value.mobile_instance_id != UINT64_MAX &&
	       value.player_pid && value.player_pid <= INT32_MAX && value.mobile_cash_revision &&
	       value.mobile_revision && value.stock_revision &&
	       value.player_custody_revision != UINT64_MAX;
}
bool item_native_mobile_money_result_build_owned(const item_transfer_payload &payload,
						 item_native_mobile_money_result *output,
						 recovery_reserve reserve, void *context,
						 recovery_codec_scope &parent_scope) noexcept
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	if (!output || !scope.invoke(
			       [&](size_t prefix)
			       {
				       return item_transfer_native_money_shape_valid_bounded(
					       payload, payload.native_recovery.present, reserve,
					       context, prefix);
			       },
			       false, native_quest_recovery_codec_source_frame_bytes()))
		return false;
	const auto &reference = payload.native_mobile.reference;
	const auto &projection = payload.native_money.projection;
	const item_native_mobile_money_result result{
		reference.mobile_instance_id,	  payload.native_mobile.final_giver_pid,
		projection.player_after_revision, projection.mobile_after_revision,
		reference.mobile_revision + 1,	  payload.expected_from_revision,
		reference.stock_revision
	};
	if (!native_money_result_valid_owned(result))
		return false;
	*output = result;
	return true;
}
bool item_native_mobile_fee_result_build_owned(const item_transfer_payload &payload,
					       item_native_mobile_fee_result *output,
					       recovery_reserve reserve, void *context,
					       recovery_codec_scope &parent_scope) noexcept
try
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	if (!output || !scope.invoke(
			       [&](size_t prefix) {
				       return item_transfer_native_fee_shape_valid_bounded(
					       payload, true, reserve, context, prefix);
			       },
			       false, native_quest_recovery_codec_source_frame_bytes()))
		return false;
	item_native_mobile_fee_result value{};
	value.mobile_instance_id = payload.native_mobile.reference.mobile_instance_id;
	value.player_pid = payload.native_mobile.final_giver_pid;
	value.mobile_cash_revision = payload.native_cost.projection.after_revision;
	value.mobile_revision = payload.native_mobile.reference.mobile_revision + 1;
	value.stock_revision = payload.native_mobile.reference.stock_revision;
	value.native_custody_revision = payload.expected_from_revision;
	value.player_custody_revision = payload.expected_to_revision;
	*output = value;
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}
bool money_receipt_valid_bounded(const item_transfer_payload &payload,
				 const native_quest_recovery_receipt &receipt,
				 recovery_reserve reserve, void *context,
				 recovery_codec_scope &parent_scope)
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	if (!receipt.present)
		return receipt == native_quest_recovery_receipt{};
	if ((receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES ||
	    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
	     (receipt.error_code != 0)))
		return false;
	item_native_mobile_money_result actual{}, expected{};
	if (!item_native_mobile_money_result_decode(
		    { receipt.result_payload.data(), receipt.result_size }, &actual) ||
	    !scope.invoke(
		    [&]([[maybe_unused]] size_t prefix)
		    {
			    return item_native_mobile_money_result_build_owned(
				    payload, &expected, reserve, context, scope);
		    },
		    false, native_quest_recovery_codec_source_frame_bytes()))
		return false;
	if (receipt.outcome == critical_apply_outcome::terminal_failure)
	{
		expected.player_wallet_revision =
			payload.native_money.projection.player_before_revision;
		expected.mobile_cash_revision =
			payload.native_money.projection.mobile_before_revision;
		expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
	}
	return actual == expected &&
	       receipt.durable_revision ==
		       std::max({ actual.player_wallet_revision, actual.mobile_cash_revision,
				  actual.mobile_revision, actual.player_custody_revision,
				  actual.stock_revision }) &&
	       std::all_of(receipt.result_payload.begin() + receipt.result_size,
			   receipt.result_payload.end(), [](uint8_t b) { return b == 0; });
}
bool fee_receipt_valid_bounded(const item_transfer_payload &payload,
			       const native_quest_recovery_receipt &receipt,
			       recovery_reserve reserve, void *context,
			       recovery_codec_scope &parent_scope)
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	if (!receipt.present)
		return receipt == native_quest_recovery_receipt{};
	if ((receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES ||
	    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
	     (receipt.error_code != 0)))
		return false;
	item_native_mobile_fee_result actual{}, expected{};
	if (!item_native_mobile_fee_result_decode(
		    { receipt.result_payload.data(), receipt.result_size }, &actual) ||
	    !scope.invoke(
		    [&]([[maybe_unused]] size_t prefix) {
			    return item_native_mobile_fee_result_build_owned(
				    payload, &expected, reserve, context, scope);
		    },
		    false, native_quest_recovery_codec_source_frame_bytes()))
		return false;
	if (receipt.outcome == critical_apply_outcome::terminal_failure)
	{
		expected.mobile_cash_revision = payload.native_cost.projection.before_revision;
		expected.mobile_revision = payload.native_mobile.reference.mobile_revision;
	}
	return actual == expected &&
	       receipt.durable_revision ==
		       std::max({ actual.mobile_cash_revision, actual.mobile_revision,
				  actual.stock_revision, actual.native_custody_revision,
				  actual.player_custody_revision }) &&
	       std::all_of(receipt.result_payload.begin() + receipt.result_size,
			   receipt.result_payload.end(), [](uint8_t v) { return v == 0; });
}
bool context_shape_bounded(const item_transfer_payload &payload,
			   const native_quest_recovery_context &value, recovery_reserve reserve,
			   void *context, recovery_codec_scope &parent_scope)
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	if (!(payload.native_money.present ?
		      scope.invoke(
			      [&]([[maybe_unused]] size_t prefix) {
				      return money_receipt_valid_bounded(payload, value.receipt,
									 reserve, context, scope);
			      },
			      false, native_quest_recovery_codec_source_frame_bytes()) :
	      payload.native_cost.fee_only ?
		      scope.invoke(
			      [&]([[maybe_unused]] size_t prefix) {
				      return fee_receipt_valid_bounded(payload, value.receipt,
								       reserve, context, scope);
			      },
			      false, native_quest_recovery_codec_source_frame_bytes()) :
		      receipt_valid(value.receipt)) ||
	    static_cast<uint8_t>(value.publication_stage) > 2 ||
	    (value.publication_stage != native_quest_recovery_publication_stage::captured &&
	     !value.receipt.present) ||
	    !stages_valid(value.publication_steps) || !stages_valid(value.give_messages) ||
	    !stages_valid(value.give_hooks) || !stages_valid(value.consumed_root_steps) ||
	    value.consumed_root_steps.size() !=
		    payload.native_recovery.consumed_root_order.size() ||
	    value.next_branch > value.branches.size() || value.child_handoff_stage > 2 ||
	    value.next_child_command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    (!value.next_child_command.empty() &&
	     (!value.child_handoff_stage || !value.branch_program_frozen ||
	      value.next_branch >= value.branches.size())) ||
	    ((value.child_handoff_stage == 0) !=
	     critical_operation_id_is_zero(value.next_child_operation)) ||
	    (!value.branch_program_frozen && (!value.branches.empty() || value.next_branch)) ||
	    (value.branch_program_frozen &&
	     !std::all_of(value.give_hooks.begin(), value.give_hooks.end(),
			  [](uint8_t stage) { return stage == 2; })))
		return false;
	if (payload.native_money.present &&
	    (value.branch_program_frozen || !value.branches.empty() || value.next_branch ||
	     !critical_operation_id_is_zero(value.parent_acceptance) ||
	     !critical_operation_id_is_zero(value.next_child_operation) ||
	     value.child_handoff_stage || !value.next_child_command.empty() ||
	     value.latest_child_branch || value.latest_child_revision ||
	     !value.latest_child_command.empty() || !value.latest_child_attachment.empty() ||
	     !value.consumed_root_steps.empty() ||
	     std::any_of(value.give_hooks.begin(), value.give_hooks.end(),
			 [](uint8_t stage) { return stage != 0; }) ||
	     std::any_of(value.publication_steps.begin() + 1, value.publication_steps.end(),
			 [](uint8_t stage) { return stage != 0; }) ||
	     (value.publication_stage == native_quest_recovery_publication_stage::captured &&
	      value.publication_steps[0] != 0) ||
	     (value.publication_stage ==
		      native_quest_recovery_publication_stage::physically_proven &&
	      value.publication_steps[0] !=
		      (value.receipt.outcome == critical_apply_outcome::terminal_failure ? 0 : 2))))
		return false;
	if (payload.native_cost.fee_only &&
	    (value.branch_program_frozen || !value.branches.empty() || value.next_branch ||
	     !critical_operation_id_is_zero(value.parent_acceptance) ||
	     !critical_operation_id_is_zero(value.next_child_operation) ||
	     value.child_handoff_stage || !value.next_child_command.empty() ||
	     value.latest_child_branch || value.latest_child_revision ||
	     !value.latest_child_command.empty() || !value.latest_child_attachment.empty() ||
	     !value.consumed_root_steps.empty() ||
	     std::any_of(value.give_hooks.begin(), value.give_hooks.end(),
			 [](uint8_t v) { return v != 0; }) ||
	     std::any_of(value.give_messages.begin(), value.give_messages.end(),
			 [](uint8_t v) { return v != 0; }) ||
	     std::any_of(value.publication_steps.begin() + 1, value.publication_steps.begin() + 4,
			 [](uint8_t v) { return v != 0; }) ||
	     (value.publication_stage == native_quest_recovery_publication_stage::captured &&
	      std::any_of(value.publication_steps.begin(), value.publication_steps.end(),
			  [](uint8_t v) { return v != 0; })) ||
	     (value.publication_stage ==
		      native_quest_recovery_publication_stage::physically_proven &&
	      value.publication_steps[0] !=
		      (value.receipt.outcome == critical_apply_outcome::terminal_failure ? 0 : 2))))
		return false;
	size_t retained_program = sizeof(native_quest_recovery_context);
	if (!charge(value.next_child_command.size(), &retained_program) ||
	    !charge(value.latest_child_command.size(), &retained_program) ||
	    !charge(value.latest_child_attachment.size(), &retained_program) ||
	    value.latest_child_command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    value.latest_child_attachment.size() > limit ||
	    (value.latest_child_command.empty() ?
		     (value.latest_child_revision || value.latest_child_branch ||
		      !value.latest_child_attachment.empty()) :
		     (!value.latest_child_revision || !value.branch_program_frozen ||
		      value.latest_child_branch >= value.branches.size() ||
		      value.latest_child_branch >= value.next_branch ||
		      value.latest_child_attachment.empty())))
		return false;
	if (value.branches.size() > limit / branch_charge ||
	    !charge(value.branches.size() * branch_charge, &retained_program))
		return false;
	for (const auto &branch : value.branches)
	{
		if ((!branch.message_present && !branch.message.empty()) ||
		    (!branch.disappear_message_present && !branch.disappear_message.empty()) ||
		    branch.message.size() >= MAX_STRING_LENGTH ||
		    branch.disappear_message.size() >= MAX_STRING_LENGTH ||
		    branch.definition_id.size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
		    branch.message.find('\0') != std::string::npos ||
		    branch.disappear_message.find('\0') != std::string::npos ||
		    branch.definition_id.find('\0') != std::string::npos ||
		    !charge(branch.message.size() + 1, &retained_program) ||
		    !charge(branch.disappear_message.size() + 1, &retained_program) ||
		    !charge(branch.definition_id.size() + 1, &retained_program) ||
		    branch.give.size() > limit / goal_charge ||
		    branch.receive.size() > limit / goal_charge ||
		    !charge(branch.give.size() * goal_charge, &retained_program) ||
		    !charge(branch.receive.size() * goal_charge, &retained_program))
			return false;
	}
	return true;
}
bool original_command_bounded(const critical_command &command, item_transfer_payload *payload,
			      std::vector<uint8_t> *encoded, recovery_reserve reserve,
			      void *context, recovery_codec_scope &parent_scope)
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	return command.type == critical_command_type::item_transfer &&
	       command.publication_required &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       item_transfer_native_mobile_acknowledged_version(command.payload_version) &&
	       scope.invoke(
		       [&](size_t prefix) {
			       return critical_command_encode_bounded(command, encoded, reserve,
								      context, prefix);
		       },
		       critical_command_codec_result::overflow,
		       native_quest_recovery_codec_source_frame_bytes()) ==
		       critical_command_codec_result::ok &&
	       scope.invoke(
		       [&](size_t prefix)
		       {
			       return item_transfer_command_decode_payload_bounded(
				       command, payload, reserve, context, prefix);
		       },
		       false, native_quest_recovery_codec_source_frame_bytes()) &&
	       scope.invoke(
		       [&](size_t prefix)
		       {
			       return item_transfer_native_mobile_recovery_shape_valid_bounded(
				       *payload, reserve, context, prefix);
		       },
		       false, native_quest_recovery_codec_source_frame_bytes());
}
player_snapshot_codec_result original_forests_bounded(const item_transfer_payload &payload,
						      const native_quest_recovery_context &value,
						      std::vector<uint8_t> *native_bytes,
						      std::vector<uint8_t> *player_bytes,
						      recovery_reserve reserve, void *context,
						      recovery_codec_scope &parent_scope)
{
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	auto result = scope.invoke(
		[&](size_t prefix)
		{
			return player_item_snapshot_list_encode_bounded(
				value.native_before, native_bytes, reserve, context, prefix);
		},
		player_snapshot_codec_result::allocation_failure,
		native_quest_recovery_codec_source_frame_bytes());
	if (result != player_snapshot_codec_result::ok)
		return result;
	result = scope.invoke(
		[&](size_t prefix)
		{
			return player_item_snapshot_list_encode_bounded(
				value.player_before, player_bytes, reserve, context, prefix);
		},
		player_snapshot_codec_result::allocation_failure,
		native_quest_recovery_codec_source_frame_bytes());
	if (result != player_snapshot_codec_result::ok)
		return result;
	if (payload.native_money.present || payload.native_cost.fee_only)
	{
		// These zero-item actions change neither forest; verify both role-bound player cuts
		// against the same original literal bytes, never a synthetic root UID.
		return scope.invoke(
			       [&](size_t prefix)
			       {
				       return shop_trade_recovery_forest_verify_bounded(
					       *player_bytes,
					       shop_trade_recovery_forest_role::player_before,
					       payload.native_recovery.player_before, reserve,
					       context, prefix);
			       },
			       false, native_quest_recovery_codec_source_frame_bytes()) &&
				       scope.invoke(
					       [&](size_t prefix)
					       {
						       return shop_trade_recovery_forest_verify_bounded(
							       *player_bytes,
							       shop_trade_recovery_forest_role::
								       player_after,
							       payload.native_recovery.player_after,
							       reserve, context, prefix);
					       },
					       false,
					       native_quest_recovery_codec_source_frame_bytes()) ?
			       player_snapshot_codec_result::ok :
			       player_snapshot_codec_result::invalid_value;
	}
	std::vector<player_item_snapshot> after;
	recovery_codec_live after_live(scope, after);
	result = scope.invoke(
		[&](size_t prefix)
		{
			return quest_mobile_native_items_transition_bounded(
				value.native_before, payload.native_mobile.reference, payload,
				&after, reserve, context, prefix);
		},
		player_snapshot_codec_result::allocation_failure,
		native_quest_recovery_codec_source_frame_bytes());
	if (result != player_snapshot_codec_result::ok)
		return result;
	if (payload.native_mobile.action == item_native_mobile_action::acceptance)
	{
		std::vector<player_item_snapshot> selected, remaining;
		std::vector<uint8_t> selected_bytes, after_bytes;
		recovery_codec_live selected_live(scope, selected),
			remaining_live(scope, remaining),
			selected_bytes_live(scope, selected_bytes),
			after_bytes_live(scope, after_bytes);
		if (!scope.invoke(
			    [&](size_t prefix)
			    {
				    return shop_trade_recovery_forest_verify_bounded(
					    *player_bytes,
					    shop_trade_recovery_forest_role::player_before,
					    payload.native_recovery.player_before, reserve, context,
					    prefix);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()))
			return player_snapshot_codec_result::invalid_value;
		result = scope.invoke(
			[&](size_t prefix)
			{
				return player_item_snapshot_extract_subtree_bounded(
					value.player_before, item_transfer_result_root(payload),
					&selected, &remaining, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = scope.invoke(
			[&](size_t prefix)
			{
				return player_item_snapshot_list_encode_bounded(
					selected, &selected_bytes, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = scope.invoke(
			[&](size_t prefix)
			{
				return player_item_snapshot_list_encode_bounded(
					remaining, &after_bytes, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (selected_bytes.size() != payload.item_blob_size ||
		    !std::equal(selected_bytes.begin(), selected_bytes.end(),
				payload.item_blob.begin()) ||
		    !scope.invoke(
			    [&](size_t prefix)
			    {
				    return shop_trade_recovery_forest_verify_bounded(
					    after_bytes,
					    shop_trade_recovery_forest_role::player_after,
					    payload.native_recovery.player_after, reserve, context,
					    prefix);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()))
			return player_snapshot_codec_result::invalid_value;
	}
	return player_snapshot_codec_result::ok;
}
bool child_command_shape_bounded(const critical_command &parent,
				 const item_transfer_payload &payload,
				 const native_quest_recovery_context &value,
				 recovery_reserve reserve, void *context,
				 recovery_codec_scope &parent_scope)
{
	if (value.next_child_command.empty())
		return true; // Historical NQR1 remains exact; its cold owner must retain gaps.
	if (payload.native_mobile.action != item_native_mobile_action::acceptance ||
	    value.parent_acceptance.bytes != parent.operation_id.bytes ||
	    value.next_child_operation.bytes == parent.operation_id.bytes ||
	    value.publication_stage != native_quest_recovery_publication_stage::physically_proven)
		return false;
	critical_command child;
	item_transfer_payload child_payload{};
	std::vector<uint8_t> canonical;
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	recovery_codec_live child_live(scope, child), payload_live(scope, child_payload),
		canonical_live(scope, canonical);
	if (scope.invoke(
		    [&](size_t prefix)
		    {
			    return critical_command_decode_bounded(value.next_child_command.data(),
								   value.next_child_command.size(),
								   &child, reserve, context,
								   prefix);
		    },
		    critical_command_codec_result::overflow,
		    native_quest_recovery_codec_source_frame_bytes()) !=
		    critical_command_codec_result::ok ||
	    child.operation_id.bytes != value.next_child_operation.bytes ||
	    !scope.invoke_codec(
		    [&]([[maybe_unused]] size_t prefix) {
			    return original_command_bounded(child, &child_payload, &canonical,
							    reserve, context, scope);
		    },
		    false, native_quest_recovery_codec_source_frame_bytes()) ||
	    canonical != value.next_child_command ||
	    child_payload.native_mobile.action != item_native_mobile_action::consumption ||
	    child_payload.native_recovery.player_pid != payload.native_recovery.player_pid ||
	    child_payload.native_mobile.final_giver_pid != payload.native_recovery.player_pid)
		return false;
	// Birth/provenance must be the same original lifetime. Later revisions
	// remain literal child facts, to be proved by its own original native owner.
	auto parent_birth = payload.native_mobile.reference;
	parent_birth.mobile_revision = child_payload.native_mobile.reference.mobile_revision;
	parent_birth.stock_revision = child_payload.native_mobile.reference.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> parent_reference{},
		child_reference{};
	return scope.invoke(
		       [&](size_t prefix)
		       {
			       return quest_mobile_native_reference_encode_bounded(
				       parent_birth, &parent_reference, reserve, context, prefix);
		       },
		       player_snapshot_codec_result::allocation_failure,
		       native_quest_recovery_codec_source_frame_bytes()) ==
		       player_snapshot_codec_result::ok &&
	       scope.invoke(
		       [&](size_t prefix)
		       {
			       return quest_mobile_native_reference_encode_bounded(
				       child_payload.native_mobile.reference, &child_reference,
				       reserve, context, prefix);
		       },
		       player_snapshot_codec_result::allocation_failure,
		       native_quest_recovery_codec_source_frame_bytes()) ==
		       player_snapshot_codec_result::ok &&
	       parent_reference == child_reference;
}
bool latest_child_shape_bounded(const critical_command &parent,
				const item_transfer_payload &payload,
				const native_quest_recovery_context &value,
				recovery_reserve reserve, void *context,
				recovery_codec_scope &parent_scope)
{
	if (value.latest_child_command.empty())
		return true;
	// Check the leaf marker BEFORE decode, so crafted nested NQR3 input cannot recurse.
	if (value.latest_child_attachment.size() < magic.size() ||
	    (!std::equal(magic.begin(), magic.end(), value.latest_child_attachment.begin()) &&
	     !std::equal(fee_magic.begin(), fee_magic.end(),
			 value.latest_child_attachment.begin())))
		return false;
	critical_command child;
	item_transfer_payload child_payload{};
	native_quest_recovery_context child_context, correlation;
	recovery_codec_scope scope{
		reserve, context,	0,
		nullptr, &parent_scope, native_quest_recovery_codec_source_frame_bytes()
	};
	recovery_codec_live child_live(scope, child), payload_live(scope, child_payload),
		context_live(scope, child_context), correlation_live(scope, correlation);
	if (scope.invoke(
		    [&](size_t prefix)
		    {
			    return critical_command_decode_bounded(
				    value.latest_child_command.data(),
				    value.latest_child_command.size(), &child, reserve, context,
				    prefix);
		    },
		    critical_command_codec_result::overflow,
		    native_quest_recovery_codec_source_frame_bytes()) !=
		    critical_command_codec_result::ok ||
	    !scope.invoke(
		    [&](size_t prefix)
		    {
			    return item_transfer_command_decode_payload_bounded(
				    child, &child_payload, reserve, context, prefix);
		    },
		    false, native_quest_recovery_codec_source_frame_bytes()) ||
	    child_payload.continuation.kind != item_transfer_continuation_kind::none ||
	    (std::equal(fee_magic.begin(), fee_magic.end(),
			value.latest_child_attachment.begin()) !=
	     child_payload.native_cost.fee_only) ||
	    scope.invoke_codec(
		    [&](size_t prefix)
		    {
			    return native_quest_recovery_context_decode_bounded(
				    child, value.latest_child_attachment, &child_context, reserve,
				    context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    native_quest_recovery_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    child_context.publication_stage !=
		    native_quest_recovery_publication_stage::physically_proven ||
	    !child_context.receipt.present || child_context.receipt.error_code ||
	    (child_context.receipt.outcome != critical_apply_outcome::applied &&
	     child_context.receipt.outcome != critical_apply_outcome::already_applied) ||
	    child_context.branch_program_frozen || child_context.child_handoff_stage ||
	    (!critical_operation_id_is_zero(child_context.parent_acceptance) &&
	     child_context.parent_acceptance.bytes != parent.operation_id.bytes))
		return false;
	correlation.parent_acceptance = value.parent_acceptance;
	correlation.publication_stage = value.publication_stage;
	correlation.next_child_operation = child.operation_id;
	if (!scope.peak(value.latest_child_command.size(),
			native_quest_recovery_codec_source_frame_bytes()))
		return false;
	correlation.next_child_command = value.latest_child_command;
	return scope.invoke_codec(
		[&]([[maybe_unused]] size_t prefix) {
			return child_command_shape_bounded(parent, payload, correlation, reserve,
							   context, scope);
		},
		false, native_quest_recovery_codec_source_frame_bytes());
}
void write_goals_bounded(nqr_bounded_writer &out,
			 const std::vector<native_quest_recovery_goal> &values)
{
	if (values.size() > limit / 5)
	{
		out.result = player_snapshot_codec_result::limit_exceeded;
		return;
	}
	out.number<uint32_t>(static_cast<uint32_t>(values.size()));
	if (out.result != player_snapshot_codec_result::ok)
		return;
	for (const auto &goal : values)
	{
		out.number<uint8_t>(goal.type);
		if (out.result != player_snapshot_codec_result::ok)
			return;
		out.number<int32_t>(goal.number);
		if (out.result != player_snapshot_codec_result::ok)
			return;
	}
}
bool read_goals_bounded(nqr_bounded_reader &in, std::vector<native_quest_recovery_goal> *values)
{
	uint32_t count = 0;
	if (!in.number(&count) || count > (in.bytes.size() - in.cursor) / 5 ||
	    count > limit / goal_charge || !charge(count * goal_charge, &in.retained_program))
		return false;
	if (count > values->capacity() &&
	    !in.scope.peak(count * sizeof(native_quest_recovery_goal),
			   native_quest_recovery_codec_source_frame_bytes()))
	{
		in.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
	values->resize(count);
	for (auto &goal : *values)
		if (!in.number(&goal.type) || !in.number(&goal.number))
			return false;
	return true;
}
}

// Pure storage observes the complete actual context, excluding its inline
// object and source-call frames. Game-thread/lifetime exclusion is caller-owned.
bool native_quest_recovery_context_current_heap_bytes(const native_quest_recovery_context &value,
						      size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	(void)value;
	(void)output;
	errno = ENOTSUP;
	return false;
#else
	size_t bytes = 0;
	if (!output || !recovery_heap(value, bytes))
		return false;
	*output = bytes;
	return true;
#endif
}
size_t native_quest_recovery_context_current_heap_observer_frame_bytes() noexcept
{
	using branches = decltype(std::declval<native_quest_recovery_context>().branches);
	using rows = std::vector<player_item_snapshot>;
	// Only the genuine pure context census, not codec owners/relays or restore.
	// Public value/output/bytes/return; recovery_heap context value/bytes/return;
	// nine actual vector capacity/checked-row calls (two forests, branches,
	// root steps,three command/attachment arrays,and both nested goal vectors). Each call owns
	// value/bytes, capacity receiver/result, checked-add value/reference/result.
	// Forest range owns its begin/end const iterators, row ref and heap scalar;
	// branch range owns begin/end, branch ref; normal-iterator begin/end,
	// dereference/increment/equality source scopes use the actual types below.
	// Three real string calls use the authenticated capacity pointer_to closure.
	return 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) +
	       sizeof(bool) + 9 * (4 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool)) +
	       2 * sizeof(rows::const_iterator) + sizeof(const player_item_snapshot *) +
	       sizeof(size_t) + 2 * sizeof(branches::const_iterator) +
	       sizeof(const native_quest_recovery_branch *) +
	       2 * (10 * sizeof(void *) + 2 * sizeof(bool)) +
	       3 * (2 * sizeof(void *) + 11 * sizeof(void *) + 2 * sizeof(size_t) +
		    2 * sizeof(bool)) +
	       sizeof(int *) + player_item_snapshot_current_heap_observer_frame_bytes();
}

// Complete lexical inventory paired to authentic contexts/helper validators
// and allocating lower providers. No exception reports a budget/content limit.
size_t native_quest_recovery_codec_source_frame_bytes() noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return SIZE_MAX;
#else

	using branches = std::vector<native_quest_recovery_branch>;
	using goals = std::vector<native_quest_recovery_goal>;
	size_t command_frames = 0;
	if (!critical_command_current_heap_observer_frame_bytes(&command_frames))
		return SIZE_MAX;
	// Watched payload/command/context/vector objects are charged by their actual
	// live scope. Writer's watched byte-vector inline object is not repeated.
	const size_t public_frames =
		2 * (5 * sizeof(void *) + sizeof(size_t) + sizeof(player_snapshot_codec_result)) +
		sizeof(std::span<const uint8_t>) + sizeof(nqr_bounded_reader) +
		sizeof(nqr_bounded_writer) - sizeof(std::vector<uint8_t>) +
		3 * sizeof(std::span<const uint8_t>) + 2 * sizeof(uint8_t) + sizeof(uint16_t) +
		sizeof(uint32_t) + 4 * sizeof(bool) + 2 * sizeof(player_snapshot_codec_result) +
		2 * sizeof(branches::const_iterator) +
		sizeof(const native_quest_recovery_branch *) + 2 * sizeof(branches::iterator) +
		sizeof(native_quest_recovery_branch *);
	const size_t helpers = 4 * (5 * sizeof(void *) + sizeof(bool)) +
			       sizeof(player_snapshot_codec_result) +
			       sizeof(quest_mobile_native_reference) +
			       2 * sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>);
	// Context_shape retained scalar/range/absent receipt; operation-zero actual
	// array16 backing pointers, scalar byte and array-accessor scopes.
	const size_t shape = 5 * sizeof(void *) + sizeof(bool) + sizeof(size_t) +
			     2 * sizeof(branches::const_iterator) +
			     sizeof(const native_quest_recovery_branch *) +
			     sizeof(native_quest_recovery_receipt) + sizeof(void *) +
			     sizeof(uint8_t) + 2 * sizeof(const uint8_t *) + sizeof(bool) +
			     4 * sizeof(void *);
	// Receipt validators: actual+expected typed results,five-word max backing
	// array/view,original equality. Builders hold typed values/projection refs.
	// Allocating shape/reference/SHA/fee continuation calls use real bounded
	// providers, never this nonallocating lexical subtotal as a replacement.
	const size_t receipts =
		2 * (5 * sizeof(void *) + sizeof(bool) + 5 * sizeof(uint64_t) +
		     sizeof(std::initializer_list<uint64_t>) + 3 * sizeof(void *) +
		     sizeof(uint64_t) + 3 * sizeof(void *) + sizeof(bool)) +
		3 * sizeof(item_native_mobile_money_result) +
		3 * sizeof(item_native_mobile_fee_result) +
		2 * (5 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) +
		// Generic item receipt/decode aggregate temporary and three-word max.
		2 * sizeof(item_transfer_result) + sizeof(void *) + sizeof(bool) +
		3 * sizeof(uint64_t) + sizeof(std::initializer_list<uint64_t>) +
		3 * sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(void *) + sizeof(size_t) +
		sizeof(bool) +
		// Fixed money/fee decode/encode original local values/canonical arrays.
		2 * sizeof(std::span<const uint8_t>) + 6 * sizeof(void *) + 4 * sizeof(bool) +
		sizeof(item_native_mobile_money_result) + sizeof(item_native_mobile_fee_result) +
		2 * sizeof(std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES>) +
		sizeof(std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES>) +
		2 * sizeof(void *) + sizeof(bool);
	// Reader/writer typed number/raw/blob/text/boolean,get/put u16/32/64;
	// actual span,count,decoded word,string growth request,eight-byte buffer.
	// max(initializer_list<uint64_t>) passes a true by-value list;
	// __max_element first/last/comparator/result and actual saved result,
	// iter_less_iter constructor/operator() and actual returned scalar.
	const size_t maximum = sizeof(std::initializer_list<uint64_t>) + sizeof(uint64_t) +
			       4 * sizeof(void *) + 2 * sizeof(__gnu_cxx::__ops::_Iter_less_iter) +
			       3 * sizeof(void *) + sizeof(bool);
	const size_t parser =
		18 * sizeof(void *) + 7 * sizeof(size_t) + 3 * sizeof(std::span<const uint8_t>) +
		2 * sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t) +
		sizeof(std::array<uint8_t, 8>) + 5 * sizeof(bool) +
		sizeof(player_snapshot_codec_result) + // Original put/get u16/32/64 scalar formals,return values and unsigned byte loops.
		sizeof(void *) + sizeof(uint16_t) + sizeof(void *) + 2 * sizeof(uint32_t) +
		sizeof(unsigned int) + sizeof(void *) + 2 * sizeof(uint64_t) +
		sizeof(unsigned int) + sizeof(void *) + sizeof(uint16_t) + sizeof(void *) +
		sizeof(uint32_t) + sizeof(unsigned int) + sizeof(void *) + sizeof(uint64_t) +
		sizeof(unsigned int) +
		// read/write_goals count,real goal range iterators and element references.
		4 * sizeof(void *) + sizeof(uint32_t) + sizeof(bool) +
		2 * sizeof(goals::const_iterator) + 2 * sizeof(goals::iterator) +
		sizeof(const native_quest_recovery_goal *) + sizeof(native_quest_recovery_goal *);
	// Payload six strings/five vectors; context seven vectors; one helper
	// command's four vectors. Branch/goal specializations remain separate.
	const size_t defaults = 8 * sizeof(void *) +
				6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) +
				     sizeof(std::allocator<char>)) +
				16 * (5 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
				6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
				16 * recovery_library_allocator_frames;
	// Scope.parent is a genuine live pointer,not copied storage; own current/
	// prefix arguments/results join the retained parent at every descendant.
	const size_t borrowed = 3 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool);
	return public_frames + helpers + shape + receipts + maximum + parser + defaults + borrowed +
	       player_item_snapshot_current_heap_observer_frame_bytes() +
	       item_transfer_payload_current_heap_observer_frame_bytes() +
	       native_quest_recovery_context_current_heap_observer_frame_bytes() +
	       native_quest_recovery_codec_source_profile_query_frames() +
	       recovery_owner_source_frames +
	       // Actual transient invoke_codec formals/local, two admissions and entry getter.
	       4 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
	       recovery_library_view_frames + recovery_library_vector_frames +
	       recovery_library_move_frames + recovery_library_string_frames +
	       recovery_branch_member_frames + recovery_branch_move_frames +
	       recovery_goal_default_frames + recovery_byte_algorithm_frames + command_frames +
	       sizeof(size_t *) + sizeof(bool);
#endif
}

namespace
{
// Reader/spans/results are lexical source, not watched physical inline.
constexpr size_t native_entry_encode =
	sizeof(recovery_codec_scope) + sizeof(item_transfer_payload) +
	3 * sizeof(std::vector<uint8_t>) + 4 * sizeof(recovery_codec_live);
constexpr size_t native_entry_decode =
	sizeof(recovery_codec_scope) + sizeof(item_transfer_payload) +
	sizeof(std::vector<uint8_t>) + sizeof(native_quest_recovery_context) +
	3 * sizeof(recovery_codec_live);
constexpr size_t native_entry_latest =
	sizeof(recovery_codec_scope) + sizeof(critical_command) + sizeof(item_transfer_payload) +
	2 * sizeof(native_quest_recovery_context) + 4 * sizeof(recovery_codec_live);
constexpr size_t native_entry_forests =
	sizeof(recovery_codec_scope) + 2 * sizeof(std::vector<player_item_snapshot>) +
	3 * sizeof(std::vector<uint8_t>) + 5 * sizeof(recovery_codec_live);
constexpr size_t native_entry_a = native_entry_encode > native_entry_decode ? native_entry_encode :
									      native_entry_decode;
constexpr size_t native_entry_b = native_entry_latest > native_entry_forests ? native_entry_latest :
									       native_entry_forests;
constexpr size_t native_entry_max = native_entry_a > native_entry_b ? native_entry_a :
								      native_entry_b;
// Child command/payload/one vector/three watchers is contained in latest.
// Original-command/context-shape scope-only entries are also contained here.
}
size_t native_quest_recovery_codec_entry_inline_bytes() noexcept
{
	return native_entry_max;
}

player_snapshot_codec_result
native_quest_recovery_context_encode_bounded(const critical_command &command,
					     const native_quest_recovery_context &value,
					     std::vector<uint8_t> *output, recovery_reserve reserve,
					     void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return player_snapshot_codec_result::allocation_failure;
#endif

	if (!output)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> encoded_command, native_bytes, player_bytes;
		recovery_codec_scope scope{ reserve, context, outer };
		recovery_codec_live payload_live(scope, payload),
			encoded_live(scope, encoded_command), native_live(scope, native_bytes),
			player_live(scope, player_bytes);
		if (!scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return original_command_bounded(command, &payload,
								    &encoded_command, reserve,
								    context, scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()) ||
		    !scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return context_shape_bounded(payload, value, reserve, context,
								 scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()) ||
		    !scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return child_command_shape_bounded(command, payload, value,
								       reserve, context, scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()) ||
		    !scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return latest_child_shape_bounded(command, payload, value,
								      reserve, context, scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()))
			return player_snapshot_codec_result::invalid_value;
		auto result = scope.invoke_codec(
			[&]([[maybe_unused]] size_t prefix)
			{
				return original_forests_bounded(payload, value, &native_bytes,
								&player_bytes, reserve, context,
								scope);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (result != player_snapshot_codec_result::ok)
			return result;
		nqr_bounded_writer out{ scope, {} };
		recovery_codec_live out_live(scope, out.bytes);
		out.raw(payload.native_cost.fee_only ?
				fee_magic :
			payload.native_money.present ?
				money_magic :
				(!value.latest_child_command.empty() ?
					 latest_magic :
					 (value.next_child_command.empty() ? magic : child_magic)));
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.blob(encoded_command);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.blob(native_bytes);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.blob(player_bytes);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint8_t>(value.receipt.present);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint8_t>(static_cast<uint8_t>(value.receipt.outcome));
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint64_t>(value.receipt.durable_revision);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint32_t>(value.receipt.error_code);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint16_t>(static_cast<uint16_t>(value.receipt.failure_stage));
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint16_t>(value.receipt.result_size);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.receipt.result_payload);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint8_t>(static_cast<uint8_t>(value.publication_stage));
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.publication_steps);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.give_messages);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.blob(value.consumed_root_steps);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.give_hooks);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint8_t>(value.branch_program_frozen);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint32_t>(value.next_branch);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.parent_acceptance.bytes);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.raw(value.next_child_operation.bytes);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		out.number<uint8_t>(value.child_handoff_stage);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		if (value.branches.size() > limit / 24)
			return player_snapshot_codec_result::limit_exceeded;
		out.number<uint32_t>(static_cast<uint32_t>(value.branches.size()));
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		for (const auto &branch : value.branches)
		{
			out.number<uint8_t>(branch.message_present);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.number<uint8_t>(branch.disappear_message_present);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.number<uint8_t>(branch.echo_all);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.number<uint8_t>(branch.disappear);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.text(branch.message);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.text(branch.disappear_message);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.text(branch.definition_id);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			write_goals_bounded(out, branch.give);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			write_goals_bounded(out, branch.receive);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
		}
		if (!value.next_child_command.empty() || !value.latest_child_command.empty())
			out.blob(value.next_child_command);
		if (out.result != player_snapshot_codec_result::ok)
			return out.result;
		if (!value.latest_child_command.empty())
		{
			out.number<uint32_t>(value.latest_child_branch);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.number<uint64_t>(value.latest_child_revision);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.blob(value.latest_child_command);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
			out.blob(value.latest_child_attachment);
			if (out.result != player_snapshot_codec_result::ok)
				return out.result;
		}
		*output = std::move(out.bytes);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	catch (const std::length_error &)
	{
		return player_snapshot_codec_result::limit_exceeded;
	}
	catch (...)
	{
		return player_snapshot_codec_result::invalid_value;
	}
}
player_snapshot_codec_result native_quest_recovery_context_decode_bounded(
	const critical_command &command, std::span<const uint8_t> bytes,
	native_quest_recovery_context *output, recovery_reserve reserve, void *context,
	size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return player_snapshot_codec_result::allocation_failure;
#endif

	if (!output || bytes.size() > limit)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		recovery_codec_scope scope{ reserve, context, outer };
		nqr_bounded_reader in{ scope, bytes };
		std::span<const uint8_t> value, native_bytes, player_bytes;
		item_transfer_payload payload{};
		std::vector<uint8_t> encoded_command;
		native_quest_recovery_context candidate;
		recovery_codec_live payload_live(scope, payload),
			encoded_live(scope, encoded_command), candidate_live(scope, candidate);
		uint8_t outcome = 0, publication = 0;
		uint16_t failure = 0;
		if (!scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return original_command_bounded(command, &payload,
								    &encoded_command, reserve,
								    context, scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()) ||
		    !in.raw(magic.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		const bool with_latest =
			std::equal(value.begin(), value.end(), latest_magic.begin());
		const bool with_child = with_latest ||
					std::equal(value.begin(), value.end(), child_magic.begin());
		const bool with_money = std::equal(value.begin(), value.end(), money_magic.begin());
		const bool with_fee = std::equal(value.begin(), value.end(), fee_magic.begin());
		if (with_fee != payload.native_cost.fee_only ||
		    with_money != payload.native_money.present ||
		    (!with_fee && !with_money && !with_child &&
		     !std::equal(value.begin(), value.end(), magic.begin())) ||
		    !in.blob(&value) || value.size() != encoded_command.size() ||
		    !std::equal(value.begin(), value.end(), encoded_command.begin()) ||
		    !in.blob(&native_bytes) || !in.blob(&player_bytes) ||
		    !in.boolean(&candidate.receipt.present) || !in.number(&outcome) ||
		    !in.number(&candidate.receipt.durable_revision) ||
		    !in.number(&candidate.receipt.error_code) || !in.number(&failure) ||
		    !in.number(&candidate.receipt.result_size) ||
		    !in.raw(candidate.receipt.result_payload.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		std::copy(value.begin(), value.end(), candidate.receipt.result_payload.begin());
		candidate.receipt.outcome = static_cast<critical_apply_outcome>(outcome);
		candidate.receipt.failure_stage = static_cast<critical_failure_stage>(failure);
		if (!in.number(&publication) || !in.raw(candidate.publication_steps.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		candidate.publication_stage =
			static_cast<native_quest_recovery_publication_stage>(publication);
		std::copy(value.begin(), value.end(), candidate.publication_steps.begin());
		if (!in.raw(candidate.give_messages.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		std::copy(value.begin(), value.end(), candidate.give_messages.begin());
		if (!in.blob(&value) ||
		    value.size() != payload.native_recovery.consumed_root_order.size())
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		if (value.size() > candidate.consumed_root_steps.capacity() &&
		    !scope.peak(value.size(), native_quest_recovery_codec_source_frame_bytes()))
			return player_snapshot_codec_result::allocation_failure;
		candidate.consumed_root_steps.assign(value.begin(), value.end());
		if (!in.raw(candidate.give_hooks.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		std::copy(value.begin(), value.end(), candidate.give_hooks.begin());
		if (!in.boolean(&candidate.branch_program_frozen) ||
		    !in.number(&candidate.next_branch) ||
		    !in.raw(candidate.parent_acceptance.bytes.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		std::copy(value.begin(), value.end(), candidate.parent_acceptance.bytes.begin());
		if (!in.raw(candidate.next_child_operation.bytes.size(), &value))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		std::copy(value.begin(), value.end(), candidate.next_child_operation.bytes.begin());
		uint32_t branch_count = 0;
		if (!in.number(&candidate.child_handoff_stage) || !in.number(&branch_count) ||
		    branch_count > (bytes.size() - in.cursor) / 24 ||
		    branch_count > limit / branch_charge ||
		    !charge(branch_count * branch_charge, &in.retained_program) ||
		    (!candidate.branch_program_frozen && branch_count))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		if (branch_count > candidate.branches.capacity() &&
		    !scope.peak(branch_count * sizeof(native_quest_recovery_branch),
				native_quest_recovery_codec_source_frame_bytes()))
			return player_snapshot_codec_result::allocation_failure;
		candidate.branches.resize(branch_count);
		for (auto &branch : candidate.branches)
			if (!in.boolean(&branch.message_present) ||
			    !in.boolean(&branch.disappear_message_present) ||
			    !in.boolean(&branch.echo_all) || !in.boolean(&branch.disappear) ||
			    !in.text(&branch.message, MAX_STRING_LENGTH - 1) ||
			    !in.text(&branch.disappear_message, MAX_STRING_LENGTH - 1) ||
			    !in.text(&branch.definition_id, QUEST_REWARD_MAX_DEFINITION_ID_BYTES) ||
			    !read_goals_bounded(in, &branch.give) ||
			    !read_goals_bounded(in, &branch.receive))
				return in.result == player_snapshot_codec_result::ok ?
					       player_snapshot_codec_result::invalid_value :
					       in.result;
		if (with_child)
		{
			if (!in.blob(&value) || (!with_latest && value.empty()) ||
			    value.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
			    !charge(value.size(), &in.retained_program))
				return in.result == player_snapshot_codec_result::ok ?
					       player_snapshot_codec_result::invalid_value :
					       in.result;
			if (value.size() > candidate.next_child_command.capacity() &&
			    !scope.peak(value.size(),
					native_quest_recovery_codec_source_frame_bytes()))
				return player_snapshot_codec_result::allocation_failure;
			candidate.next_child_command.assign(value.begin(), value.end());
		}
		if (with_latest)
		{
			if (!in.number(&candidate.latest_child_branch) ||
			    !in.number(&candidate.latest_child_revision) || !in.blob(&value) ||
			    value.empty() || value.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
			    !charge(value.size(), &in.retained_program))
				return in.result == player_snapshot_codec_result::ok ?
					       player_snapshot_codec_result::invalid_value :
					       in.result;
			if (value.size() > candidate.latest_child_command.capacity() &&
			    !scope.peak(value.size(),
					native_quest_recovery_codec_source_frame_bytes()))
				return player_snapshot_codec_result::allocation_failure;
			candidate.latest_child_command.assign(value.begin(), value.end());
			if (!in.blob(&value) || value.empty() ||
			    !charge(value.size(), &in.retained_program))
				return in.result == player_snapshot_codec_result::ok ?
					       player_snapshot_codec_result::invalid_value :
					       in.result;
			if (value.size() > candidate.latest_child_attachment.capacity() &&
			    !scope.peak(value.size(),
					native_quest_recovery_codec_source_frame_bytes()))
				return player_snapshot_codec_result::allocation_failure;
			candidate.latest_child_attachment.assign(value.begin(), value.end());
		}
		if (in.cursor != bytes.size() ||
		    !scope.invoke_codec(
			    [&]([[maybe_unused]] size_t prefix) {
				    return context_shape_bounded(payload, candidate, reserve,
								 context, scope);
			    },
			    false, native_quest_recovery_codec_source_frame_bytes()))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		auto decoded = scope.invoke(
			[&](size_t prefix)
			{
				return player_item_snapshot_list_decode_bounded(
					native_bytes.data(), native_bytes.size(),
					&candidate.native_before, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (decoded != player_snapshot_codec_result::ok)
			return decoded;
		decoded = scope.invoke(
			[&](size_t prefix)
			{
				return player_item_snapshot_list_decode_bounded(
					player_bytes.data(), player_bytes.size(),
					&candidate.player_before, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (decoded != player_snapshot_codec_result::ok)
			return decoded;
		std::vector<uint8_t> canonical;
		recovery_codec_live canonical_live(scope, canonical);
		auto result = scope.invoke_codec(
			[&](size_t prefix)
			{
				return native_quest_recovery_context_encode_bounded(
					command, candidate, &canonical, reserve, context, prefix);
			},
			player_snapshot_codec_result::allocation_failure,
			native_quest_recovery_codec_source_frame_bytes());
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       in.result;
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	catch (...)
	{
		return player_snapshot_codec_result::invalid_value;
	}
}
