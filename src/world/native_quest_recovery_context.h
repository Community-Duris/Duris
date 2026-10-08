#ifndef NATIVE_QUEST_RECOVERY_CONTEXT_H
#define NATIVE_QUEST_RECOVERY_CONTEXT_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"
#include "player/player_snapshot_codec.h"

#include <array>
#include <span>
#include <string>

// Pointer-free original values. These records carry no source, SQL, physical
// publication or ACK authority; only the existing private owners can persist
// their actual observations. Runtime IDs and process generations are rebound.
struct native_quest_recovery_goal
{
	uint8_t type = 0;
	int32_t number = 0;
	bool operator==(const native_quest_recovery_goal &) const = default;
};

struct native_quest_recovery_branch
{
	std::vector<native_quest_recovery_goal> give, receive;
	std::string message, disappear_message, definition_id;
	bool message_present = false, disappear_message_present = false;
	bool echo_all = false, disappear = false;
	bool operator==(const native_quest_recovery_branch &) const = default;
};

enum class native_quest_recovery_publication_stage : uint8_t
{
	captured = 0,
	publishing = 1,
	physically_proven = 2,
};

struct native_quest_recovery_receipt
{
	bool present = false;
	critical_apply_outcome outcome = critical_apply_outcome::applied;
	uint64_t durable_revision = 0;
	uint32_t error_code = 0;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	uint16_t result_size = 0;
	std::array<uint8_t, CRITICAL_COMPLETION_RESULT_MAX_BYTES> result_payload{};
	bool operator==(const native_quest_recovery_receipt &) const = default;
};

struct native_quest_recovery_context
{
	// Full literal stock and final-giver inventory from the original checkpoint.
	// AFTER is derived with the existing pure transforms, never reconstructed
	// from a later player or native stock image.
	std::vector<player_item_snapshot> native_before, player_before;
	native_quest_recovery_receipt receipt;
	native_quest_recovery_publication_stage publication_stage =
		native_quest_recovery_publication_stage::captured;
	// 0 = not started, 1 = started/unreturned, 2 = returned. An unreturned
	// notice/hook is retained uncertainty, never permission to repeat it.
	// detach, place, runtime registry, reference binding, completion notice,
	// original reward continuation, respectively.
	std::array<uint8_t, 6> publication_steps{};
	std::array<uint8_t, 3> give_messages{};
	std::vector<uint8_t> consumed_root_steps;
	std::array<uint8_t, 9> give_hooks{};
	bool branch_program_frozen = false;
	std::vector<native_quest_recovery_branch> branches;
	uint32_t next_branch = 0;
	// Original parent/child identities only. Allocation and durable handoff
	// remain the existing gameplay/command owners' responsibility.
	critical_operation_id parent_acceptance{}, next_child_operation{};
	uint8_t child_handoff_stage = 0;
	// Exact FINAL acknowledged child command, frozen by the original owner
	// before child admission. Empty historical contexts remain NQR1 and cannot
	// reconstruct a missing child from later native/player images.
	std::vector<uint8_t> next_child_command;
	// Latest authentic completed prefix carrier, retained across cursor advancement.
	// A leaf NQR1 consumption body only: no recursive parent/child histories.
	// Values grant no current SQL/world authority or reward-completion permission.
	uint32_t latest_child_branch = 0;
	uint64_t latest_child_revision = 0;
	std::vector<uint8_t> latest_child_command, latest_child_attachment;
};

// Bounded canonical transport bound to every byte of the original command.
// No SQL, world census, source allocation, native mutation or ACK is performed.
// Strong output guarantee: refusal leaves the output unchanged.
player_snapshot_codec_result
native_quest_recovery_context_encode(const critical_command &,
				     const native_quest_recovery_context &,
				     std::vector<uint8_t> *) noexcept;
player_snapshot_codec_result
native_quest_recovery_context_decode(const critical_command &, std::span<const uint8_t>,
				     native_quest_recovery_context *) noexcept;

// Pure structural receipt/context check for the coordinator. The separate
// original guarded publication owner still supplies physical/ACK authority.
// Pure fee-only ACK retry proof for the actual phase2 envelope. No effects or ACK.
bool native_quest_recovery_fee_ack_context_valid(const critical_native_recovery_envelope &,
						 const critical_completion &) noexcept;

bool native_quest_recovery_publication_context_valid(const critical_native_recovery_envelope &,
						     const critical_completion &) noexcept;

// Pure exact parent/child correlation and prefix successor shape. Terminal calls
// still require the original domain owner's genuine completion/ACK proof.
bool native_quest_recovery_pair_context_valid(
	const critical_native_recovery_envelope &parent,
	const critical_native_recovery_envelope &child,
	const critical_native_recovery_envelope *parent_successor) noexcept;

#endif
