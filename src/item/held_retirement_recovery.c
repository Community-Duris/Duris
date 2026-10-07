#include "item/held_retirement_recovery.h"
#include "economy/item_transfer_accounting.h"
#include "core/defines.h"
#include <algorithm>
#include <openssl/sha.h>

namespace
{
constexpr size_t HEADER = 148;
void put(std::vector<uint8_t> &out, size_t at, uint64_t value, size_t count)
{
	for (size_t i = 0; i < count; ++i)
		out[at + i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(std::span<const uint8_t> in, size_t at, size_t count)
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(in[at + i]) << (8 * i);
	return value;
}
bool canonical_body(std::span<const uint8_t> bytes, std::vector<player_item_snapshot> *out)
{
	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_decode(bytes.data(), bytes.size(), &items) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() != bytes.size() ||
	    !std::equal(encoded.begin(), encoded.end(), bytes.begin()))
		return false;
	if (out)
		*out = std::move(items);
	return true;
}
bool receipt_valid(const held_retirement_receipt &receipt)
{
	if (receipt.result_size > receipt.result.size())
		return false;
	if (!receipt.present)
		return receipt.outcome == critical_apply_outcome::applied &&
		       !receipt.durable_revision && !receipt.error_code &&
		       receipt.failure_stage == critical_failure_stage::none &&
		       !receipt.result_size &&
		       std::all_of(receipt.result.begin(), receipt.result.end(),
				   [](uint8_t v) { return !v; });
	return (receipt.outcome == critical_apply_outcome::applied ||
		receipt.outcome == critical_apply_outcome::already_applied ||
		receipt.outcome == critical_apply_outcome::terminal_failure) &&
	       receipt.failure_stage == critical_failure_stage::none &&
	       std::all_of(receipt.result.begin() + receipt.result_size, receipt.result.end(),
			   [](uint8_t v) { return !v; });
}
}

bool held_retirement_command_identity(const critical_command &command, item_transfer_payload *out,
				      lockpick_retirement_terms *terms_out) noexcept
try
{
	item_transfer_payload payload{};
	lockpick_retirement_terms terms;
	economic_frozen_intent intent;
	if (!command.publication_required || !command.accepted_at_usec ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !item_transfer_accounting_command_supported(command) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    payload.continuation.kind != static_cast<item_transfer_continuation_kind>(8) ||
	    !lockpick_retirement_payload_valid(payload) ||
	    !lockpick_retirement_decode(payload.continuation.data, &terms) ||
	    economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    !intent.admission.metadata.source_event ||
	    intent.admission.metadata.source_event->kind !=
		    economic_source_kind::intentional_destruction ||
	    intent.admission.metadata.source_event->sequence != terms.item_uid ||
	    intent.admission.metadata.source_event->slot !=
		    static_cast<uint32_t>(terms.item_vnum) ||
	    intent.admission.metadata.actor_id != terms.actor_pid)
		return false;
	if (out)
		*out = std::move(payload);
	if (terms_out)
		*terms_out = terms;
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_body_pair(const item_transfer_payload &payload,
			       std::span<const uint8_t> before,
			       std::vector<uint8_t> *after) noexcept
try
{
	if (!after || !lockpick_retirement_payload_valid(payload))
		return false;
	std::vector<player_item_snapshot> full, selected, remaining;
	std::vector<uint8_t> selected_bytes, after_bytes;
	if (!canonical_body(before, &full) ||
	    player_item_snapshot_extract_subtree(full, payload.selected_item_uid, &selected,
						 &remaining) != player_snapshot_codec_result::ok ||
	    selected.size() != 1 || selected[0].equipment_slot != HOLD + 1 ||
	    player_item_snapshot_list_encode(selected, &selected_bytes) !=
		    player_snapshot_codec_result::ok ||
	    selected_bytes.size() != payload.item_blob_size ||
	    !std::equal(selected_bytes.begin(), selected_bytes.end(), payload.item_blob.begin()) ||
	    player_item_snapshot_list_encode(remaining, &after_bytes) !=
		    player_snapshot_codec_result::ok)
		return false;
	*after = std::move(after_bytes);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_encode(const critical_command &command,
				     const held_retirement_recovery &context,
				     std::vector<uint8_t> *output) noexcept
try
{
	if (!output || !context.save_revision || context.save_revision == UINT64_MAX ||
	    context.physical_stage > 2 || (!context.receipt.present && context.physical_stage) ||
	    !receipt_valid(context.receipt))
		return false;
	item_transfer_payload payload{};
	std::vector<uint8_t> frozen, before, after, expected_after;
	if (!held_retirement_command_identity(command, &payload) ||
	    critical_command_encode(command, &frozen) != critical_command_codec_result::ok ||
	    player_item_snapshot_list_encode(context.before, &before) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(context.after, &after) !=
		    player_snapshot_codec_result::ok ||
	    !held_retirement_body_pair(payload, before, &expected_after) || after != expected_after)
		return false;
	size_t bytes = HEADER;
	for (size_t size :
	     { before.size(), after.size(), static_cast<size_t>(context.receipt.result_size) })
	{
		if (size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES - bytes)
			return false;
		bytes += size;
	}
	std::vector<uint8_t> encoded(bytes, 0);
	std::copy_n(reinterpret_cast<const uint8_t *>("HRT1"), 4, encoded.begin());
	encoded[4] = 1;
	encoded[5] = context.physical_stage;
	encoded[6] = context.receipt.present;
	encoded[7] = static_cast<uint8_t>(context.receipt.outcome);
	put(encoded, 8, context.save_revision, 8);
	put(encoded, 16, before.size(), 4);
	put(encoded, 20, after.size(), 4);
	put(encoded, 24, context.receipt.durable_revision, 8);
	put(encoded, 32, context.receipt.error_code, 4);
	put(encoded, 36, context.receipt.result_size, 2);
	encoded[38] = static_cast<uint8_t>(context.receipt.failure_stage);
	SHA256(frozen.data(), frozen.size(), encoded.data() + 40);
	SHA256(before.data(), before.size(), encoded.data() + 72);
	SHA256(after.data(), after.size(), encoded.data() + 104);
	// Reserved tail is zero. Full original bodies, then exact delivered result.
	auto at = std::copy(before.begin(), before.end(), encoded.begin() + HEADER);
	at = std::copy(after.begin(), after.end(), at);
	std::copy_n(context.receipt.result.begin(), context.receipt.result_size, at);
	*output = std::move(encoded);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_decode(const critical_command &command,
				     std::span<const uint8_t> bytes,
				     held_retirement_recovery *output) noexcept
try
{
	if (!output || bytes.size() < HEADER ||
	    bytes.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
	    !std::equal(bytes.begin(), bytes.begin() + 4,
			reinterpret_cast<const uint8_t *>("HRT1")) ||
	    bytes[4] != 1 || bytes[5] > 2 || bytes[6] > 1 || bytes[39] ||
	    std::any_of(bytes.begin() + 136, bytes.begin() + HEADER,
			[](uint8_t v) { return v != 0; }))
		return false;
	const size_t before_size = get(bytes, 16, 4), after_size = get(bytes, 20, 4),
		     result_size = get(bytes, 36, 2);
	if (before_size > bytes.size() - HEADER ||
	    after_size > bytes.size() - HEADER - before_size ||
	    result_size != bytes.size() - HEADER - before_size - after_size ||
	    result_size > CRITICAL_COMPLETION_RESULT_MAX_BYTES)
		return false;
	held_retirement_recovery decoded;
	decoded.save_revision = get(bytes, 8, 8);
	decoded.physical_stage = bytes[5];
	decoded.receipt.present = bytes[6];
	decoded.receipt.outcome = static_cast<critical_apply_outcome>(bytes[7]);
	decoded.receipt.durable_revision = get(bytes, 24, 8);
	decoded.receipt.error_code = static_cast<uint32_t>(get(bytes, 32, 4));
	decoded.receipt.result_size = static_cast<uint16_t>(result_size);
	decoded.receipt.failure_stage = static_cast<critical_failure_stage>(bytes[38]);
	if (!canonical_body(bytes.subspan(HEADER, before_size), &decoded.before) ||
	    !canonical_body(bytes.subspan(HEADER + before_size, after_size), &decoded.after))
		return false;
	std::copy_n(bytes.begin() + HEADER + before_size + after_size, result_size,
		    decoded.receipt.result.begin());
	std::vector<uint8_t> canonical;
	if (!held_retirement_recovery_encode(command, decoded, &canonical) ||
	    canonical.size() != bytes.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = std::move(decoded);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_receipt_matches(const held_retirement_receipt &receipt,
					      const critical_completion &completion) noexcept
{
	return receipt.present && receipt_valid(receipt) &&
	       critical_completion_disposition_valid(completion) &&
	       completion.disposition == critical_completion_disposition::execution &&
	       receipt.outcome == completion.outcome &&
	       receipt.durable_revision == completion.durable_revision &&
	       receipt.error_code == completion.error_code &&
	       receipt.failure_stage == completion.failure_stage &&
	       receipt.result_size == completion.result_size &&
	       std::equal(receipt.result.begin(), receipt.result.end(),
			  completion.result_payload.begin());
}

bool held_retirement_recovery_transition_valid(
	const critical_native_recovery_envelope &original,
	const critical_native_recovery_envelope &successor) noexcept
try
{
	std::vector<uint8_t> a, b;
	held_retirement_recovery before, after;
	if (!original.revision || original.revision == UINT64_MAX ||
	    successor.revision != original.revision + 1 ||
	    original.phase != critical_native_recovery_phase::execution_pending ||
	    successor.phase != original.phase ||
	    critical_command_encode(original.command, &a) != critical_command_codec_result::ok ||
	    critical_command_encode(successor.command, &b) != critical_command_codec_result::ok ||
	    a != b ||
	    !held_retirement_recovery_decode(original.command, original.attachment, &before) ||
	    !held_retirement_recovery_decode(successor.command, successor.attachment, &after) ||
	    before.save_revision != after.save_revision ||
	    after.physical_stage < before.physical_stage ||
	    after.physical_stage > before.physical_stage + 1 ||
	    (before.receipt.present && before.receipt != after.receipt))
		return false;
	std::vector<uint8_t> old_body, new_body;
	for (const auto *pair : { &before.before, &before.after })
	{
		const auto &target = pair == &before.before ? after.before : after.after;
		if (player_item_snapshot_list_encode(*pair, &old_body) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(target, &new_body) !=
			    player_snapshot_codec_result::ok ||
		    old_body != new_body)
			return false;
	}
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_publication_context_valid(
	const critical_native_recovery_envelope &envelope,
	const critical_completion &completion) noexcept
{
	held_retirement_recovery context;
	return envelope.revision &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope.command.operation_id.bytes == completion.operation_id.bytes &&
	       held_retirement_recovery_decode(envelope.command, envelope.attachment, &context) &&
	       context.physical_stage == 2 &&
	       held_retirement_recovery_receipt_matches(context.receipt, completion);
}

bool held_retirement_publication_snapshot_valid(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &snapshot) noexcept
try
{
	std::vector<uint8_t> supplied, retained;
	held_retirement_recovery context;
	if (!critical_completion_disposition_valid(completion) ||
	    completion.operation_id.bytes != command.operation_id.bytes ||
	    !snapshot.envelope.revision ||
	    snapshot.envelope.phase != critical_native_recovery_phase::execution_pending ||
	    critical_command_encode(command, &supplied) != critical_command_codec_result::ok ||
	    critical_command_encode(snapshot.envelope.command, &retained) !=
		    critical_command_codec_result::ok ||
	    supplied != retained ||
	    !held_retirement_recovery_decode(command, snapshot.envelope.attachment, &context))
		return false;
	switch (snapshot.mode)
	{
	case held_retirement_publication_mode::original_refusal:
		return completion.disposition == critical_completion_disposition::never_admitted &&
		       !context.receipt.present && context.physical_stage == 0;
	case held_retirement_publication_mode::terminal_ack_retry:
		return held_retirement_recovery_publication_context_valid(snapshot.envelope,
									  completion);
	case held_retirement_publication_mode::execution:
		return completion.disposition == critical_completion_disposition::execution &&
		       (!context.receipt.present ||
			held_retirement_recovery_receipt_matches(context.receipt, completion));
	}
	return false;
}
catch (...)
{
	return false;
}
