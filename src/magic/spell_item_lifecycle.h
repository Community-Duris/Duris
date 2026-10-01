#ifndef DURIS_SPELL_ITEM_LIFECYCLE_H
#define DURIS_SPELL_ITEM_LIFECYCLE_H

#include "item/item_movement_transaction.h"

#include <cstddef>
#include <cstdint>
#include <array>

enum class spell_component_effect_status : uint8_t
{
	retry,
	waiting_for_owner,
	complete,
};

// Stable replay dispatch for component-consuming spell effects.
using spell_component_effect_completion_fn = spell_component_effect_status (*)(
	const critical_operation_id &operation_id, P_char actor, bool committed,
	const item_transfer_result &result, unsigned int error_code, const uint8_t *context,
	size_t context_size);

constexpr size_t SPELL_COMPONENT_EFFECT_CONTEXT_MAX_BYTES = 48;

struct spell_component_context_writer
{
	bool put_u8(uint8_t value)
	{
		if (size >= bytes.size())
			return false;
		bytes[size++] = value;
		return true;
	}
	bool put_u32(uint32_t value)
	{
		for (size_t shift = 0; shift < 32; shift += 8)
			if (!put_u8(static_cast<uint8_t>(value >> shift)))
				return false;
		return true;
	}
	bool put_i32(int32_t value) { return put_u32(static_cast<uint32_t>(value)); }
	bool put_u64(uint64_t value)
	{
		for (size_t shift = 0; shift < 64; shift += 8)
			if (!put_u8(static_cast<uint8_t>(value >> shift)))
				return false;
		return true;
	}
	const uint8_t *data() const { return bytes.data(); }
	size_t size = 0;
	std::array<uint8_t, SPELL_COMPONENT_EFFECT_CONTEXT_MAX_BYTES> bytes = {};
};

struct spell_component_context_reader
{
	spell_component_context_reader(const uint8_t *encoded, size_t encoded_size)
		: data(encoded)
		, size(encoded_size)
	{
	}
	bool get_u8(uint8_t *value)
	{
		if (!value || !data || offset >= size)
			return false;
		*value = data[offset++];
		return true;
	}
	bool get_u32(uint32_t *value)
	{
		if (!value)
			return false;
		uint32_t decoded = 0;
		for (size_t shift = 0; shift < 32; shift += 8)
		{
			uint8_t byte = 0;
			if (!get_u8(&byte))
				return false;
			decoded |= static_cast<uint32_t>(byte) << shift;
		}
		*value = decoded;
		return true;
	}
	bool get_i32(int32_t *value)
	{
		uint32_t decoded = 0;
		if (!value || !get_u32(&decoded))
			return false;
		*value = static_cast<int32_t>(decoded);
		return true;
	}
	bool get_u64(uint64_t *value)
	{
		if (!value)
			return false;
		uint64_t decoded = 0;
		for (size_t shift = 0; shift < 64; shift += 8)
		{
			uint8_t byte = 0;
			if (!get_u8(&byte))
				return false;
			decoded |= static_cast<uint64_t>(byte) << shift;
		}
		*value = decoded;
		return true;
	}
	bool finished() const { return data && offset == size; }
	const uint8_t *data;
	size_t size;
	size_t offset = 0;
};

// Consume up to max_components matching player-carried roots as one custody
// transaction. Require the full count when require_exact_count is true. The
// continuation runs only after committed items are removed.
bool spell_consume_components(P_char actor, int vnum, size_t max_components, uint32_t reason_id,
			      item_spell_component_effect effect,
			      spell_component_effect_completion_fn continuation,
			      const void *context, size_t context_size,
			      bool require_exact_count = false);

spell_component_effect_status
spell_faerie_sight_component_completed(const critical_operation_id &operation_id, P_char actor,
				       bool committed, const item_transfer_result &result,
				       unsigned int error_code, const uint8_t *context,
				       size_t context_size);
spell_component_effect_status spell_spore_burst_initial_components_completed(
	const critical_operation_id &operation_id, P_char actor, bool committed,
	const item_transfer_result &result, unsigned int error_code, const uint8_t *context,
	size_t context_size);
spell_component_effect_status spell_spore_burst_repeat_components_completed(
	const critical_operation_id &operation_id, P_char actor, bool committed,
	const item_transfer_result &result, unsigned int error_code, const uint8_t *context,
	size_t context_size);
spell_component_effect_status
spell_summon_insects_component_completed(const critical_operation_id &operation_id, P_char actor,
					 bool committed, const item_transfer_result &result,
					 unsigned int error_code, const uint8_t *context,
					 size_t context_size);
spell_component_effect_status
spell_wall_of_bones_scales_completed(const critical_operation_id &operation_id, P_char actor,
				     bool committed, const item_transfer_result &result,
				     unsigned int error_code, const uint8_t *context,
				     size_t context_size);
spell_component_effect_status
spell_vines_component_retirement_completed(const critical_operation_id &operation_id, P_char actor,
					   bool committed, const item_transfer_result &result,
					   unsigned int error_code, const uint8_t *context,
					   size_t context_size);

// Rebuild the bounded live callback context from a retained command. Failed
// commands can publish their failure callback; committed effects remain held
// until the effect owner has an operation-scoped receipt.
bool spell_component_retirement_restore_context(
	const item_transfer_payload &payload,
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> *context, size_t *context_size,
	uint32_t *effect_id = nullptr, uint32_t *receipt_owner_pid = nullptr);
bool spell_component_retirement_replayed_publication(const critical_operation_id &operation_id,
						     P_char actor, bool committed,
						     const item_transfer_result &result,
						     unsigned int error_code,
						     const uint8_t *context, size_t context_size);
bool spell_component_retirement_waiting_for_effect(const critical_operation_id &operation_id);
struct player_load_spell_effect_receipt;
struct player_spell_effect_receipt_snapshot;
bool spell_component_retirement_restore_replayed_effect(const critical_operation_id &operation_id,
							uint32_t actor_pid, uint32_t effect_id,
							uint32_t receipt_owner_pid = 0);
bool spell_component_retirement_bind_effect_owner(const critical_operation_id &operation_id,
						  uint32_t actor_pid, uint32_t owner_pid,
						  item_spell_component_effect effect);
bool spell_component_retirement_append_owner_operations(
	uint32_t owner_pid, std::vector<critical_operation_id> *operations);
bool spell_component_retirement_pending_save_receipts(
	uint32_t owner_pid, std::vector<player_spell_effect_receipt_snapshot> *receipts);
void spell_component_retirement_recover_receipts(uint32_t actor_pid,
						 const player_load_spell_effect_receipt *receipts,
						 size_t count);
void spell_component_retirement_save_completed(int32_t actor_pid, bool acknowledged,
					       const player_spell_effect_receipt_snapshot *receipts,
					       size_t count);
bool spell_component_retirement_effect_applied(const critical_operation_id &operation_id);
bool spell_component_retirement_effect_applied_once(const critical_operation_id &operation_id);
spell_component_effect_status
spell_component_retirement_save_effect(const critical_operation_id &operation_id, P_char actor,
				       item_spell_component_effect effect);
// Restore durable soulbind publication from the serialized item command.
bool spell_item_lifecycle_restore_replayed_command(const critical_command &command);

#endif
