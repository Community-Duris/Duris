#ifndef AUCTION_RECOVERY_CONTEXT_H
#define AUCTION_RECOVERY_CONTEXT_H

#include "economy/auction_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"
#include "player/player_snapshot_codec.h"
#include <span>

// Original pointer-free observations only. Matching these bytes cannot grant
// SQL, physical publication, player-save release or coordinator ACK authority.
enum class auction_recovery_stage : uint8_t
{
	captured,
	publishing,
	physically_proven,
	// Actual passive loader AFTER + original SQL/world cut, not callback returns.
	// Only the privately restored/rebound save participant may checkpoint this.
	restored_after_proven
};
struct auction_recovery_effect
{
	// Zero: untouched; one: started/unreturned; two: returned successfully.
	uint8_t state = 0;
	bool periodic = false;
	bool operator==(const auction_recovery_effect &) const = default;
};
struct auction_recovery_context
{
	uint32_t actor_pid = 0;
	uint32_t original_level = 0;
	uint64_t before_save_revision = 0;
	bool before_present = false, after_present = false, receipt_present = false;
	std::vector<player_item_snapshot> player_before, player_after, selected_literals;
	critical_apply_outcome outcome = critical_apply_outcome::applied;
	uint32_t result_code = 0;
	critical_failure_stage failure_stage = critical_failure_stage::none;
	uint64_t durable_revision = 0;
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result{};
	auction_recovery_stage stage = auction_recovery_stage::captured;
	// Detach, destroy, enroll, place, runtime registry, in original item order.
	std::array<std::array<auction_recovery_effect, 5>, AUCTION_COMMAND_MAX_ITEMS> roots{};
	// One record per actual selected literal node, including descendants.
	std::vector<std::array<auction_recovery_effect, 4>> reload;
	std::vector<std::vector<auction_recovery_effect>> proclib;
	// BID/settlement publish existing current owner/custody cache, not item moves.
	// Actorless settlement may use this effect; balances remain player-only.
	auction_recovery_effect ownership{}, balances{}, notice{};
};

player_snapshot_codec_result auction_recovery_context_encode(const critical_command &,
							     const auction_recovery_context &,
							     std::vector<uint8_t> *) noexcept;
player_snapshot_codec_result auction_recovery_context_decode(const critical_command &,
							     std::span<const uint8_t>,
							     auction_recovery_context *) noexcept;
bool auction_recovery_envelope_valid(const critical_native_recovery_envelope &) noexcept;
bool auction_recovery_initial_valid(const critical_native_recovery_envelope &) noexcept;
bool auction_recovery_successor_valid(const critical_native_recovery_envelope &,
				      const critical_native_recovery_envelope &) noexcept;
bool auction_recovery_publication_context_valid(const critical_native_recovery_envelope &,
						const critical_completion &) noexcept;
bool auction_recovery_terminal_valid(const critical_native_recovery_envelope &) noexcept;

size_t auction_recovery_shape_source_frame_bytes() noexcept;
size_t auction_recovery_wire_source_frame_bytes() noexcept;
player_snapshot_codec_result
auction_recovery_context_encode_bounded(const critical_command &, const auction_recovery_context &,
					std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
					void *, size_t) noexcept;
player_snapshot_codec_result
auction_recovery_context_decode_bounded(const critical_command &, std::span<const uint8_t>,
					auction_recovery_context *,
					bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
bool auction_recovery_envelope_valid_bounded(const critical_native_recovery_envelope &,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t) noexcept;

#endif
