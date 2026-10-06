#ifndef NATIVE_MOBILE_BIRTH_RESET_TAIL_H
#define NATIVE_MOBILE_BIRTH_RESET_TAIL_H

#include <array>
#include <cstdint>

using native_mobile_birth_reset_tail_digest = std::array<uint8_t, 32>;

// Current selected original shop values only, never a saved-index binding permit.
struct shop_native_mobile_birth_reset_selection
{
	int32_t index = -1, shop_slot = -1, keeper_vnum = 0, room_vnum = 0;
	bool replicated = false;
};

// Pure game-thread observation before the original reset M tail. NMT1 binds
// actual room/owning-zone mapping, cached effective HP dial, bounded difficulty,
// only reached zone-factor floats, and recomputed configured shop selection.
// The original owner separately proves its CURRENT executable witness, original
// constructor/source, and before/after lifecycle. No stats snapshot, RNG, clock,
// constructor/modifier replay, shop assignment, callbacks, SQL or ACK here.
// Strong output: every refusal leaves output unchanged. Default IEEE float/double
// ABI; values are canonical bit patterns, not locale/decimal text or raw structs.
bool native_mobile_birth_reset_tail_capture(int32_t mobile_vnum, int32_t destination_room_vnum,
					    int configured_shop,
					    native_mobile_birth_reset_tail_digest *output) noexcept;

#endif
