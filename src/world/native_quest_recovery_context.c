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
	if (!receipt_valid(value.receipt) || static_cast<uint8_t>(value.publication_stage) > 2 ||
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
	       command.payload_version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
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
	    !std::equal(magic.begin(), magic.end(), value.latest_child_attachment.begin()))
		return false;
	critical_command child;
	item_transfer_payload child_payload{};
	native_quest_recovery_context child_context, correlation;
	if (critical_command_decode(value.latest_child_command.data(),
				    value.latest_child_command.size(),
				    &child) != critical_command_codec_result::ok ||
	    !item_transfer_command_decode_payload(child, &child_payload) ||
	    child_payload.continuation.kind != item_transfer_continuation_kind::none ||
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
		out.raw(!value.latest_child_command.empty() ?
				latest_magic :
				(value.next_child_command.empty() ? magic : child_magic));
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
		if ((!with_child && !std::equal(value.begin(), value.end(), magic.begin())) ||
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
		    !std::equal(magic.begin(), magic.end(), child.attachment.begin()) ||
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
