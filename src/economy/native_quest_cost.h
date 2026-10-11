#ifndef DURIS_NATIVE_QUEST_COST_H
#define DURIS_NATIVE_QUEST_COST_H

#include "persistence/critical_command.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>

// Original first-pass COINS attempts only, in original give-list slot order.
// The quest producer separately proves preliminary availability and which
// attempts actually precede an overlapping ITEM/TYPE failure.
struct native_quest_cost_requirement
{
	uint32_t slot = 0;
	int32_t copper = 0;
	bool operator==(const native_quest_cost_requirement &) const = default;
};
enum class native_quest_cost_attempt_outcome : uint8_t
{
	charged = 1,
	nonpositive = 2,
	insufficient = 3,
};
struct native_quest_cost_attempt
{
	native_quest_cost_requirement requirement;
	native_quest_cost_attempt_outcome outcome = {};
	std::array<int64_t, 4> before{}, after{};
	bool operator==(const native_quest_cost_attempt &) const = default;
};
struct native_quest_cost_projection
{
	uint64_t before_revision = 0, after_revision = 0;
	std::array<int64_t, 4> before{}, after{};
	std::vector<native_quest_cost_attempt> attempts;
	bool operator==(const native_quest_cost_projection &) const = default;
};
enum class native_quest_cost_projection_result : uint8_t
{
	ok,
	invalid,
	overflow,
	allocation_failure,
};

// Pure literal projection of original NPC SUB_MONEY(mode0)/ADD_MONEY change.
// Native int cash and GET_MONEY must have defined nonnegative arithmetic.
// No caller identity, wallet mapping, source, epoch, SQL, native write, receipt,
// or publication authority is issued. Every refusal preserves output.
native_quest_cost_projection_result
native_quest_cost_project(const std::array<int64_t, 4> &before, uint64_t original_cash_revision,
			  std::span<const native_quest_cost_requirement> original_attempts,
			  native_quest_cost_projection *output) noexcept;

// Canonical value transport for the original cash/attempt sequence. The bound
// is the existing critical-command payload budget; the parent also checks its
// complete combined payload. Decode verifies every projected step, not merely
// a net copper total. Neither codec grants monetary/source authority.
constexpr size_t NATIVE_QUEST_COST_HEADER_BYTES = 96;
constexpr size_t NATIVE_QUEST_COST_ATTEMPT_BYTES = 80;
native_quest_cost_projection_result
native_quest_cost_projection_encode(const native_quest_cost_projection &,
				    std::vector<uint8_t> *) noexcept;
native_quest_cost_projection_result
native_quest_cost_projection_decode(std::span<const uint8_t>,
				    native_quest_cost_projection *) noexcept;

// Unselected complete pure projection/codec companions. Outer owns authentic
// input, prior output and caller storage; these own every private allocation.
native_quest_cost_projection_result native_quest_cost_project_bounded(
	const std::array<int64_t, 4> &, uint64_t, std::span<const native_quest_cost_requirement>,
	native_quest_cost_projection *, bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
native_quest_cost_projection_result native_quest_cost_projection_encode_bounded(
	const native_quest_cost_projection &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
native_quest_cost_projection_result native_quest_cost_projection_decode_bounded(
	std::span<const uint8_t>, native_quest_cost_projection *, bool (*)(size_t, void *) noexcept,
	void *, size_t) noexcept;

// Actual original project/encode/decode call cost_bound_initial before creating
// their private projection/vector/controller owners. The full SOURCE query
// separately admits pre-callback scalar/call scopes; initial owner inline is
// therefore zero, not the mixed original frames allowance or a cached baseline.
bool native_quest_cost_project_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_project_initial_query_frame_bytes() noexcept
{
	// Getter output parameter and returned bool; scalar result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
bool native_quest_cost_projection_encode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_projection_encode_initial_query_frame_bytes() noexcept
{
	// Getter output parameter and returned bool; scalar result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
bool native_quest_cost_projection_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_projection_decode_initial_query_frame_bytes() noexcept
{
	// Getter output parameter and returned bool; scalar result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}

// Complete Source of the matching original *_bounded runtime only.
// Full is used for transient pre-entry admission. Supplement is the
// conservative complete retained residual after exact persistent original
// Source credits in those bounded frames become live. Unknown callback
// bodies, captures, caller input/prior output and host-library emitted
// correspondence remain caller/qualification obligations.
// Initial owner INLINE remains zero before cost_bound_initial. Original
// initial and prefix already own the real cost_bound_budget INLINE;
// decode additionally owns its uint32 materialized bit_cast argument.
// External lvalue-to-value span copying belongs the actual caller Source.
// Verify constructs its requirement span prvalue directly into the project
// formal; pointer/count construction is included, no intervening copy.
// Explicit span destruction is included; no implicit trivial extent dtor.
// Do not retain full and supplement together or infer callsite selection
// from these scalar exports. All refusals preserve scalar output.
bool native_quest_cost_project_source_frame_bytes(size_t *) noexcept;
bool native_quest_cost_project_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_project_source_query_frame_bytes() noexcept
{
	// Direct getter output parameter and bool; returned size_t caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
bool native_quest_cost_projection_encode_source_frame_bytes(size_t *) noexcept;
bool native_quest_cost_projection_encode_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_projection_encode_source_query_frame_bytes() noexcept
{
	// Direct getter output parameter and bool; returned size_t caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
bool native_quest_cost_projection_decode_source_frame_bytes(size_t *) noexcept;
bool native_quest_cost_projection_decode_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t native_quest_cost_projection_decode_source_query_frame_bytes() noexcept
{
	// Direct getter output parameter and bool; returned size_t caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
#endif
