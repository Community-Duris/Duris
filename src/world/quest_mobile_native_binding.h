#ifndef QUEST_MOBILE_NATIVE_BINDING_H
#define QUEST_MOBILE_NATIVE_BINDING_H

#include "world/quest_mobile_native_reference.h"
#include "economy/native_quest_cost.h"
#include "economy/native_quest_coin_give.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

struct char_data;
struct quest_mobile_native_reference;
class quest_mobile_native_birth_owner;
class item_native_quest_publication_owner;
class quest_mobile_native_publication_binding;

constexpr std::size_t QUEST_MOBILE_NATIVE_BINDING_BYTES = 148;
constexpr std::size_t QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES = 48;

// Observed process-local metadata plus actual native denominations. Birth epoch
// identifies the authenticated original install, not today's activation authority.
// This copy is not an admission, source, mapping or SQL capability.
struct quest_mobile_native_cash_reference
{
	quest_mobile_native_reference reference;
	critical_operation_id lineage{}, birth_epoch{};
	uint64_t wallet_mapping_id = 0, cash_revision = 0;
	std::array<int64_t, 4> denominations{};
};

// Runtime-only zeroable storage, not a birth proof or persistence record. The
// original birth/restore/retirement owner alone may install, revise or clear it.
// No constructor is allowed: char_data is allocated and reused by zeroing pools.
struct quest_mobile_native_binding
{
    private:
	std::uint8_t encoded_reference_[QUEST_MOBILE_NATIVE_BINDING_BYTES];
	// Byte arrays preserve zeroable pool layout without padding/constructors.
	// Zero means unobserved; never cash revision1 or a native-ID wallet fallback.
	std::uint8_t cash_revision_[8], wallet_mapping_id_[8], lineage_[16], birth_epoch_[16];

	friend class quest_mobile_native_birth_owner;
	friend class quest_mobile_native_stage;
	friend class quest_mobile_native_publication_binding;
	friend bool quest_mobile_native_reference_copy(const char_data *, std::uint64_t,
						       quest_mobile_native_reference *) noexcept;
	friend player_snapshot_codec_result quest_mobile_native_reference_copy_bounded(
		const char_data *, std::uint64_t, quest_mobile_native_reference *,
		bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	friend bool
	quest_mobile_native_cash_reference_copy(const char_data *, std::uint64_t,
						quest_mobile_native_cash_reference *) noexcept;
};

static_assert(sizeof(quest_mobile_native_binding) ==
	      QUEST_MOBILE_NATIVE_BINDING_BYTES + QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES);
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

// Same complete indexed observation/canonical policy, with genuine prospective
// codec/query storage admission and strong output. Outer includes actual input,
// prior output and every other caller owner. Resource refusals retain their
// existing codec result; invalid_value never proves that a binding is absent.
// No binding write, birth, UID, wallet, custody, replay or admission capability.
player_snapshot_codec_result quest_mobile_native_reference_copy_bounded(
	const char_data *, uint64_t expected_runtime_id, quest_mobile_native_reference *,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept;

// Exact indexed runtime and original reference are observed before private cash
// metadata/literal access. Unknown legacy metadata refuses; output preserved.
bool quest_mobile_native_cash_reference_copy(const char_data *, std::uint64_t expected_runtime_id,
					     quest_mobile_native_cash_reference *) noexcept;

// One original item-transition publication capability. Only the real owner may
// invoke it after original SQL/held-body/world proof; it grants no birth or ACK.
struct quest_mobile_native_image;
struct native_mobile_wallet_origin;
class quest_mobile_native_publication_binding final
{
    private:
	friend class item_native_quest_publication_owner;
	// Only after authenticated original receipt/current SQL/full world cut and
	// confirmed read-only rollback. Existing indexed NPC only; no construction,
	// birth issuance or epoch inference. Mixed metadata refuses without writes.
	static bool restore_money_metadata(char_data *, uint64_t actual_runtime,
					   const quest_mobile_native_image &,
					   const native_mobile_wallet_origin &) noexcept;
	static bool advance(char_data *, std::uint64_t expected_runtime_id,
			    const quest_mobile_native_reference &before,
			    const quest_mobile_native_reference &after) noexcept;
	// Original fee publication owner only, after genuine same-root SQL/held
	// proof and started native cash/binding action. All comparisons precede
	// one nonallocating cash+reference+revision write. No retry of uncertainty.
	static bool apply_cost(char_data *, std::uint64_t expected_runtime_id,
			       const quest_mobile_native_cash_reference &before,
			       const native_quest_cost_projection &cost,
			       const quest_mobile_native_reference &after) noexcept;
	// Original money publication owner only: authentic wallet/root/held-world
	// proof and one started cash/binding journal action precede this call.
	// All checks occur before player+native nonallocating writes. Mixed state
	// refuses; no independent endpoint retry or fabricated rollback.
	static bool apply_money(char_data *player, uint64_t player_runtime, uint32_t player_pid,
				char_data *mobile, uint64_t mobile_runtime,
				const quest_mobile_native_cash_reference &native_before,
				const native_quest_coin_give_projection &,
				const quest_mobile_native_reference &native_after) noexcept;
};

#endif
