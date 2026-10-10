#ifndef DURIS_LOCKPICK_RETIREMENT_CONTINUATION_H
#define DURIS_LOCKPICK_RETIREMENT_CONTINUATION_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

enum class lockpick_retirement_branch : uint8_t
{
	failed_container = 1,
	failed_door = 2,
	wear = 3,
};

// Original already-selected break values only; no RNG, custody or ACK authority.
struct lockpick_retirement_terms
{
	lockpick_retirement_branch branch = {};
	uint64_t item_uid = 0;
	uint32_t actor_pid = 0;
	int32_t item_vnum = 0;
};

constexpr size_t LOCKPICK_RETIREMENT_CONTINUATION_BYTES = 20;
bool lockpick_retirement_encode(const lockpick_retirement_terms &, std::vector<uint8_t> *) noexcept;
bool lockpick_retirement_decode(std::span<const uint8_t>, lockpick_retirement_terms *) noexcept;
struct item_transfer_payload;
// The shared codec separately selects the reserved kind8 and v9+ transport.
// This validates the exact one-item original held-pick graph and literal binding.
bool lockpick_retirement_payload_valid(const item_transfer_payload &) noexcept;
// Complete unselected original held-pick literal predicate. Authentic outer
// owns input/caller/global storage; this owns the full private decoded item.
bool lockpick_retirement_payload_valid_bounded(const item_transfer_payload &,
					       bool (*)(size_t, void *) noexcept, void *,
					       size_t) noexcept;
#endif
