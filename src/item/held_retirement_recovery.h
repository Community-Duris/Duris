#ifndef DURIS_HELD_RETIREMENT_RECOVERY_H
#define DURIS_HELD_RETIREMENT_RECOVERY_H
#include "item/lockpick_retirement_continuation.h"
#include "item/item_ownership_runtime.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_command_journal.h"
#include "player/player_snapshot_codec.h"
#include "player/player_revision_state.h"

struct held_retirement_receipt
{
	bool present = false;
	critical_apply_outcome outcome = critical_apply_outcome::applied;
	uint64_t durable_revision = 0;
	uint32_t error_code = 0;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	uint16_t result_size = 0;
	std::array<uint8_t, CRITICAL_COMPLETION_RESULT_MAX_BYTES> result{};
	bool operator==(const held_retirement_receipt &) const = default;
};
// Pointer-free original values only. No source, SQL, native or ACK authority.
struct held_retirement_recovery
{
	player_revision_t save_revision = 0;
	std::vector<player_item_snapshot> before, after;
	held_retirement_receipt receipt;
	// 0 original capture; 1 original physical attempt started; 2 returned/proven.
	// Restart may complete an authentic graph, but never repeat a started notice.
	uint8_t physical_stage = 0;
};
bool held_retirement_command_identity(const critical_command &, item_transfer_payload * = nullptr,
				      lockpick_retirement_terms * = nullptr) noexcept;
bool held_retirement_body_pair(const item_transfer_payload &, std::span<const uint8_t> before,
			       std::vector<uint8_t> *after) noexcept;
bool held_retirement_recovery_encode(const critical_command &, const held_retirement_recovery &,
				     std::vector<uint8_t> *) noexcept;
bool held_retirement_recovery_decode(const critical_command &, std::span<const uint8_t>,
				     held_retirement_recovery *) noexcept;
bool held_retirement_recovery_receipt_matches(const held_retirement_receipt &,
					      const critical_completion &) noexcept;
bool held_retirement_recovery_transition_valid(const critical_native_recovery_envelope &,
					       const critical_native_recovery_envelope &) noexcept;
bool held_retirement_recovery_publication_context_valid(const critical_native_recovery_envelope &,
							const critical_completion &) noexcept;
enum class held_retirement_publication_mode : uint8_t
{
	execution = 0,
	original_refusal = 1,
	terminal_ack_retry = 2,
};
// Exact retained coordinator values. No ACK authority; only typed owners use it.
struct held_retirement_publication_snapshot
{
	critical_native_recovery_envelope envelope;
	held_retirement_publication_mode mode = held_retirement_publication_mode::execution;
};
bool held_retirement_publication_snapshot_valid(
	const critical_command &, const critical_completion &,
	const held_retirement_publication_snapshot &) noexcept;

class player_save_held_retirement_checkpoint_owner;
class player_save_held_retirement_publication_owner;
// Additive coordinator hook contracts: definitions belong to primary integration.
// No quest/SHOP capability is borrowed by the held-retirement owner.
class critical_held_retirement_submission_owner final
{
	friend class player_save_held_retirement_checkpoint_owner;
	static critical_submit_result submit(critical_native_recovery_envelope);
};
class critical_held_retirement_publication_owner final
{
	friend class player_save_held_retirement_publication_owner;
	static bool copy_context(const critical_command &,
				 critical_native_recovery_envelope *) noexcept;
	// Original refusal or terminal uncertain-ACK copy is a distinct exact-receipt
	// contract. It never weakens native_context_copy or clears uncertainty.
	static bool copy_publication(const critical_command &, const critical_completion &,
				     held_retirement_publication_snapshot *) noexcept;
	static bool checkpoint_context(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
};
struct held_retirement_current_observation
{
	player_revision_t save_revision = 0;
	std::vector<player_item_snapshot> items;
	item_ownership_runtime_entry selected{};
	uint64_t from_owner_revision = 0, to_owner_revision = 0;
	bool rollback_confirmed = false;
};
// Additive original-receipt/current-cut hook. Real implementation must authenticate
// immutable command/receipt, complete held BEFORE/AFTER, source UID lifetime and
// current SQL rows under installed locks, and confirm read-only rollback.
// This declaration/value cannot establish authority or activate an unsupported route.
bool held_retirement_repository_observe_current(const critical_command &,
						const critical_completion &,
						const held_retirement_recovery &,
						held_retirement_current_observation *) noexcept;
class player_save_restored_publication_owner;
bool critical_command_coordinator_cancel_held_retirement_publication(
	player_save_restored_publication_owner &, const critical_native_recovery_envelope &,
	bool (*)(const critical_command &, const critical_completion &, void *) noexcept,
	void *) noexcept;

// Additive complete original pure recovery value companions. Outer owns actual
// command/input/prior output/caller state. No replay or publication authority.
// Bodies remain unselected until the complete source/native gate is joined.
size_t held_retirement_codec_source_frame_bytes() noexcept;
// First preadmit this fixed query source, then query source+prospective entry
// inline and admit both before selecting any bounded entry. The entry inline
// is transient prospective storage: actual child CURRENT owns it after entry.
constexpr size_t held_retirement_codec_source_profile_query_frames() noexcept
{
	return sizeof(void *) + 17 * sizeof(size_t);
}
constexpr size_t held_retirement_codec_entry_inline_query_frames() noexcept
{
	return sizeof(size_t);
}
size_t held_retirement_codec_entry_inline_bytes() noexcept;
bool held_retirement_command_identity_bounded(const critical_command &, item_transfer_payload *,
					      lockpick_retirement_terms *,
					      bool (*)(size_t, void *) noexcept, void *,
					      size_t) noexcept;
bool held_retirement_body_pair_bounded(const item_transfer_payload &, std::span<const uint8_t>,
				       std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
				       void *, size_t) noexcept;
bool held_retirement_recovery_encode_bounded(const critical_command &,
					     const held_retirement_recovery &,
					     std::vector<uint8_t> *,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t) noexcept;
bool held_retirement_recovery_decode_bounded(const critical_command &, std::span<const uint8_t>,
					     held_retirement_recovery *,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t) noexcept;
#endif
