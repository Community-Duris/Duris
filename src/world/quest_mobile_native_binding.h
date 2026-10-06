#ifndef QUEST_MOBILE_NATIVE_BINDING_H
#define QUEST_MOBILE_NATIVE_BINDING_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct char_data;
struct quest_mobile_native_reference;
class quest_mobile_native_birth_owner;
class item_native_quest_publication_owner;
class quest_mobile_native_publication_binding;

constexpr std::size_t QUEST_MOBILE_NATIVE_BINDING_BYTES = 148;

// Runtime-only zeroable storage, not a birth proof or persistence record. The
// original birth/restore/retirement owner alone may install, revise or clear it.
// No constructor is allowed: char_data is allocated and reused by zeroing pools.
struct quest_mobile_native_binding
{
    private:
	std::uint8_t encoded_reference_[QUEST_MOBILE_NATIVE_BINDING_BYTES];

	friend class quest_mobile_native_birth_owner;
	friend class quest_mobile_native_publication_binding;
	friend bool quest_mobile_native_reference_copy(const char_data *, std::uint64_t,
						       quest_mobile_native_reference *) noexcept;
};

static_assert(sizeof(quest_mobile_native_binding) == QUEST_MOBILE_NATIVE_BINDING_BYTES);
static_assert(std::is_trivial_v<quest_mobile_native_binding>);
static_assert(std::is_trivially_copyable_v<quest_mobile_native_binding>);
static_assert(std::is_standard_layout_v<quest_mobile_native_binding>);

// Observation only, on the game thread and for the same indexed live NPC.
// Carry the originally observed runtime generation across events; never infer it
// from a borrowed pointer that may have been reused by the character pool.
// Preserves output on refusal; never derives a native identity from a prototype,
// pet/keeper state, a probe, runtime_id, or the ordinary idnum allocator.
bool quest_mobile_native_reference_copy(const char_data *, std::uint64_t expected_runtime_id,
					quest_mobile_native_reference *output) noexcept;

// One original item-transition publication capability. Only the real owner may
// invoke it after original SQL/held-body/world proof; it grants no birth or ACK.
class quest_mobile_native_publication_binding final
{
    private:
	friend class item_native_quest_publication_owner;
	static bool advance(char_data *, std::uint64_t expected_runtime_id,
			    const quest_mobile_native_reference &before,
			    const quest_mobile_native_reference &after) noexcept;
};

#endif
