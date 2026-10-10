#ifndef DURIS_NATIVE_QUEST_COIN_GIVE_H
#define DURIS_NATIVE_QUEST_COIN_GIVE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// The original GIVE command selects one denomination and positive native int
// quantity. This value is neither an account mapping nor a publication permit.
struct native_quest_coin_give_projection
{
	uint8_t denomination = 0;
	int32_t quantity = 0;
	uint64_t player_before_revision = 0, player_after_revision = 0;
	uint64_t mobile_before_revision = 0, mobile_after_revision = 0;
	std::array<int64_t, 4> player_before{}, player_after{}, mobile_before{}, mobile_after{};
	bool operator==(const native_quest_coin_give_projection &) const = default;
};
enum class native_quest_coin_give_result : uint8_t
{
	ok,
	invalid,
	insufficient,
	overflow,
	allocation_failure,
};

// Opening player wallet revision zero is legitimate. Native mobile revision
// zero is unobserved and refused. The player loses the selected denomination;
// actual begin_coin_give_credit credits its copper value through original
// ADD_MONEY decomposition. Value above INT_MAX refuses before any change.
// This does not convert existing recipient holdings or supply quest, issuance,
// identity, source or cash authority. GIVE does not evaluate player GET_MONEY.
native_quest_coin_give_result native_quest_coin_give_project(
	const std::array<int64_t, 4> &player_before, uint64_t player_revision,
	const std::array<int64_t, 4> &mobile_before, uint64_t mobile_revision, uint8_t denomination,
	int32_t quantity, native_quest_coin_give_projection *output) noexcept;

constexpr size_t NATIVE_QUEST_COIN_GIVE_BYTES = 176;
native_quest_coin_give_result
native_quest_coin_give_encode(const native_quest_coin_give_projection &,
			      std::vector<uint8_t> *) noexcept;
native_quest_coin_give_result
native_quest_coin_give_decode(std::span<const uint8_t>,
			      native_quest_coin_give_projection *) noexcept;

// Unselected full NQG1 value codecs; authentic outer owns input, old output
// and callers. No native money/source/publication authority is supplied.
native_quest_coin_give_result
native_quest_coin_give_encode_bounded(const native_quest_coin_give_projection &,
				      std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
				      void *, size_t) noexcept;
native_quest_coin_give_result
native_quest_coin_give_decode_bounded(std::span<const uint8_t>, native_quest_coin_give_projection *,
				      bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

#endif
