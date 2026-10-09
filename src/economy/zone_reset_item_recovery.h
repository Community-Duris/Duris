#ifndef ZONE_RESET_ITEM_RECOVERY_H
#define ZONE_RESET_ITEM_RECOVERY_H

#include "economy/zone_reset_item_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <span>
#include <vector>

// Observations of the original once-only action, never replay entitlement.
// Started/unreturned is uncertainty; returned failure cannot become success.
struct zone_reset_item_recovery_action
{
	bool started = false, returned = false, succeeded = false;
	bool operator==(const zone_reset_item_recovery_action &) const = default;
};
struct zone_reset_item_recovery_effect
{
	bool started = false, returned = false, succeeded = false, periodic = false;
	bool operator==(const zone_reset_item_recovery_effect &) const = default;
};
struct zone_reset_item_recovery_item
{
	uint64_t object_uid = 0;
	uint32_t next_step = 0;
	bool current_step_started = false, admitted = false, published = false;
	// Exact original order/count: 2*recipe.libraries.size()+3, including real no-ops.
	std::vector<zone_reset_item_recovery_effect> effects;
	bool operator==(const zone_reset_item_recovery_item &) const = default;
};
enum class zone_reset_item_recovery_stage : uint8_t
{
	captured = 0,
	publishing = 1,
	physically_proven = 2,
};
struct zone_reset_item_recovery_context
{
	bool receipt_present = false;
	// Preserve every actual completion field, including delivery metadata.
	// The private coordinator must independently authenticate current delivery.
	critical_completion receipt{};
	zone_reset_item_recovery_stage stage = zone_reset_item_recovery_stage::captured;
	zone_reset_item_recovery_action whole_binding, batch_publication, room_placement;
	// Complete immutable command recipe/forest order, never just surviving UIDs.
	std::vector<zone_reset_item_recovery_item> items;
	// Final original cache projection observed after all services and room placement.
	// Early batch hydration is not this final proof; no current-world bypass.
	bool runtime_applied = false;
};
constexpr uint16_t ZONE_RESET_ITEM_RECOVERY_VERSION = 1;

// ZRR1 retains every byte of the actual type22 command, including full literal
// forest and constructor recipes. Shared journal attachment limits are unchanged.
// Pure predicates grant no admission, SQL, world, cleanup, UID or ACK authority.
// Only the private critical_zone_reset_item_publication_owner and actual
// zone_reset_room_publication_owner may authenticate and act on observations.
// Every refusal leaves the output unchanged.
economic_accounting_error zone_reset_item_recovery_encode(const critical_command &,
							  const zone_reset_item_recovery_context &,
							  std::vector<uint8_t> *) noexcept;
economic_accounting_error
zone_reset_item_recovery_decode(const critical_command &, std::span<const uint8_t>,
				zone_reset_item_recovery_context *) noexcept;
// Read the original command from a complete terminal BODY. Envelope phase,
// revision, authentic lifetime retention and current physical proof are separate.
economic_accounting_error
zone_reset_item_recovery_original_command_decode(std::span<const uint8_t>,
						 critical_command *) noexcept;
bool zone_reset_item_recovery_valid(const critical_native_recovery_envelope &) noexcept;
bool zone_reset_item_recovery_initial(const critical_native_recovery_envelope &) noexcept;
// Bounded companions preserve complete original command/context validation.
// Caller owns input, old outputs and its other live storage in outer_live;
// reservations precede all allocating phases and survive output transfer.
// No source, factory, storage/publication or recovery entitlement is granted.
economic_accounting_error zone_reset_item_recovery_encode_bounded(
	const critical_command &, const zone_reset_item_recovery_context &,
	std::vector<uint8_t> *, bool (*reserve_scratch_peak)(size_t, void *) noexcept,
	void *context, size_t outer_live_scratch) noexcept;
economic_accounting_error zone_reset_item_recovery_decode_bounded(
	const critical_command &, std::span<const uint8_t>, zone_reset_item_recovery_context *,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch, size_t *retained_context_heap_bytes = nullptr) noexcept;
bool zone_reset_item_recovery_initial_bounded(
	const critical_native_recovery_envelope &,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept;
bool zone_reset_item_recovery_successor(const critical_native_recovery_envelope &expected,
					const critical_native_recovery_envelope &successor) noexcept;
bool zone_reset_item_recovery_valid_bounded(const critical_native_recovery_envelope &,
					    bool (*reserve_scratch_peak)(size_t, void *) noexcept,
					    void *context, size_t outer_live_scratch) noexcept;
// Current successful delivery may change attempts/times/replay outcome, but the
// complete economic receipt core must match. This does not authenticate delivery.
bool zone_reset_item_recovery_publication(const critical_native_recovery_envelope &,
					  const critical_completion &) noexcept;
bool zone_reset_item_recovery_terminal(const critical_native_recovery_envelope &) noexcept;
// Complete original terminal envelope predicate with owning prospective decode.
// Caller owns original input/prior live state in outer; no ACK authority.
bool zone_reset_item_recovery_terminal_bounded(const critical_native_recovery_envelope &,
					       bool (*)(size_t, void *) noexcept, void *,
					       size_t outer_live) noexcept;

// Complete saved terminal BODY against a separately authenticated immutable
// original command. This returns observations only: no envelope, revision,
// generation, delivery or current physical/ACK entitlement is constructed.
// Same original terminal predicate; strong context and retained-heap outputs.
// Caller owns wire/input/prior output and span objects in outer_live_scratch.
economic_accounting_error zone_reset_item_recovery_terminal_body_decode_bounded(
	const critical_command &, const std::span<const uint8_t> &,
	zone_reset_item_recovery_context *, bool (*reserve_scratch_peak)(size_t, void *) noexcept,
	void *context, size_t outer_live_scratch,
	size_t *retained_context_heap_bytes = nullptr) noexcept;

#endif
