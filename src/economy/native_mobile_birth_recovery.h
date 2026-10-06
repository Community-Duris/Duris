#ifndef NATIVE_MOBILE_BIRTH_RECOVERY_H
#define NATIVE_MOBILE_BIRTH_RECOVERY_H

#include "economy/native_mobile_birth_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <array>
#include <span>
#include <vector>

// Actual original once-only action observations. Started/unreturned is retained
// uncertainty, never permission to rerun. Returned failure cannot become success.
struct native_mobile_birth_recovery_action
{
	bool started = false, returned = false, succeeded = false;
	bool operator==(const native_mobile_birth_recovery_action &) const = default;
};
struct native_mobile_birth_recovery_effect
{
	bool started = false, returned = false, succeeded = false, periodic = false;
	bool operator==(const native_mobile_birth_recovery_effect &) const = default;
};
struct native_mobile_birth_recovery_mobile
{
	// Native ownership is consumed before the original room/special callbacks.
	// Consumed does not imply those callbacks returned, or permit their replay.
	bool started = false, returned = false, consumed = false;
	bool operator==(const native_mobile_birth_recovery_mobile &) const = default;
};
enum class native_mobile_birth_recovery_mobile_step : uint8_t
{
	ownership = 0,
	room = 1,
	mundane = 2,
	special_probe = 3,
	special_event = 4,
	patrol = 5,
	skin = 6,
	maintenance = 7,
};
enum class native_mobile_birth_recovery_mobile_choice : uint8_t
{
	mundane = 0,
	special_event = 1,
	patrol = 2,
	skin = 3,
};
struct native_mobile_birth_recovery_choice
{
	bool chosen = false, requested = false;
	int32_t delay = 0;
	bool operator==(const native_mobile_birth_recovery_choice &) const = default;
};
struct native_mobile_birth_recovery_item
{
	uint64_t object_uid = 0;
	uint32_t next_step = 0;
	bool current_step_started = false, admitted = false, published = false;
	// succeeded means the original detached item ownership was published.
	native_mobile_birth_recovery_action publication;
	// succeeded means the original local stock enrollment really returned success.
	native_mobile_birth_recovery_action enrollment;
	// Exact original count/order: 2*libraries+3, from the immutable recipe.
	std::vector<native_mobile_birth_recovery_effect> effects;
	bool operator==(const native_mobile_birth_recovery_item &) const = default;
};
enum class native_mobile_birth_recovery_stage : uint8_t
{
	captured = 0,
	publishing = 1,
	physically_proven = 2,
};
struct native_mobile_birth_recovery_context
{
	bool receipt_present = false;
	// All genuine completion fields survive, including delivery metadata. This
	// value does not authenticate receipt delivery or current SQL/world proof.
	critical_completion receipt{};
	native_mobile_birth_recovery_stage stage = native_mobile_birth_recovery_stage::captured;
	// succeeded means committed/installed respectively.
	native_mobile_birth_recovery_action whole_binding, reference_install;
	native_mobile_birth_recovery_mobile mobile_publication;
	// Original fixed publication order. Only special_probe's periodic flag may
	// hold its actual returned callback decision; all other periodic flags are zero.
	std::array<native_mobile_birth_recovery_effect, 8> mobile_effects{};
	// Retain the actual chosen scheduling decisions/delays BEFORE schedule intent.
	// Skip decisions are chosen=true/requested=false/delay=0, then no-op success.
	std::array<native_mobile_birth_recovery_choice, 4> mobile_choices{};
	// Actual original stock construction/publication order, which may differ
	// from the final image's equipment/carry forest order. UID binds each recipe.
	// Every successor preserves this complete unique UID order exactly.
	std::vector<native_mobile_birth_recovery_item> items;
	// Original successful cache projection observation only. Cold owners must
	// prove and rebuild the current projection; this flag grants no bypass.
	bool runtime_applied = false;
};

constexpr uint16_t NATIVE_MOBILE_BIRTH_RECOVERY_VERSION = 1;

// Complete canonical attachment bound to every byte of the original v2 birth
// command, including its final literal image and constructor recipe. No pointer,
// process generation, later native image or new NPC-stat ledger is persisted.
// The existing native recovery journal owns persistence. These pure functions
// grant no admission, SQL, cleanup, world publication or ACK capability.
// Strong output guarantee: every refusal leaves the output unchanged.
economic_accounting_error
native_mobile_birth_recovery_encode(const critical_command &,
				    const native_mobile_birth_recovery_context &,
				    std::vector<uint8_t> *) noexcept;
economic_accounting_error
native_mobile_birth_recovery_decode(const critical_command &, std::span<const uint8_t>,
				    native_mobile_birth_recovery_context *) noexcept;

bool native_mobile_birth_recovery_valid(const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_recovery_initial(const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_recovery_successor(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept;
// Retained full receipt and CURRENT successful receipt must have the same
// immutable economic core. Replay may genuinely change applied/already_applied,
// attempts, times and diagnostic correlation. The shared private ACK owner must
// independently authenticate EVERY current field against coordinator delivery;
// this predicate neither synthesizes a receipt nor authenticates that authority.
bool native_mobile_birth_recovery_publication(const critical_native_recovery_envelope &,
					      const critical_completion &) noexcept;
bool native_mobile_birth_recovery_terminal(const critical_native_recovery_envelope &) noexcept;

#endif
