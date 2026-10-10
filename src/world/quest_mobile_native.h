#ifndef QUEST_MOBILE_NATIVE_H
#define QUEST_MOBILE_NATIVE_H

#include "economy/economic_accounting_plan.h"
#include "economy/currency_command.h"
#include "world/quest_mobile_native_reference.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"

#include <array>
#include <optional>
#include <span>

// Keep the original image and 148-byte reference formats readable verbatim.
constexpr size_t QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD = 216;
constexpr uint16_t QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION = 2;
constexpr size_t QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD = 256;

enum class quest_mobile_lifetime_state : uint8_t
{
	live = 1,
	retired = 2
};

struct quest_mobile_native_cash
{
	uint64_t revision = 0;
	currency_vector denominations{};
};

struct quest_mobile_native_image
{
	quest_mobile_native_reference reference;
	quest_mobile_lifetime_state state = {};
	critical_operation_id last_transition_operation = {};
	// Equipment roots in ascending one-based slot order, then carried roots in
	// native linked-list order; each complete subtree is contiguous depth-first.
	// The existing parent/slot representation preserves order without a second
	// UID/custody catalog. Every row freezes all four literal strings.
	std::vector<player_item_snapshot> items;
	// Absent in historical v1 images: unknown cash, never an authoritative zero.
	// A v2 image freezes the actual native denominations, not converted value.
	std::optional<quest_mobile_native_cash> cash;
};

player_snapshot_codec_result
quest_mobile_native_image_encode(const quest_mobile_native_image &,
				 std::vector<uint8_t> *output) noexcept;
player_snapshot_codec_result
quest_mobile_native_image_decode(std::span<const uint8_t>,
				 quest_mobile_native_image *output) noexcept;

// Passive allocation-free whole native-image framing/storage shape. It retains
// v1 unknown cash and v2 literal cash; no item/UID/source/world authority.
// Storage totals require the explicit pinned libstdc++13/C++11 ABI policy.
struct quest_mobile_native_image_allocation_profile
{
	player_item_snapshot_list_allocation_profile items;
	size_t decoded_image_payload_bytes = 0;
	size_t forest_validation_heap_peak_bytes = 0;
	size_t forest_validation_inline_bytes = 0;
	size_t canonical_image_bytes = 0;
	size_t canonical_encode_working_bytes = 0;
	size_t decode_working_bytes = 0;
	bool storage_policy_supported = false;
};
// Untrusted shape only: no image/reference digest check or authentication.
// Strong output. Full existing semantic decode and byte-for-byte canonical
// comparison still follow before publication. OpenSSL/system allocator storage
// is outside this explicit C++ payload/request profile.
player_snapshot_codec_result
quest_mobile_native_image_preflight(std::span<const uint8_t>,
				    quest_mobile_native_image_allocation_profile *) noexcept;

// Pure native cash revision policy, not admitted source/transition authority.
// Missing BEFORE means birth; unknown historical cash cannot be adopted here.
// A cash change advances both its revision and mobile revision exactly once.
bool quest_mobile_native_cash_transition_valid(const quest_mobile_native_image *before,
					       const quest_mobile_native_image &after) noexcept;

// Read-only complete NPC forest, including NORENT items: this is native stock,
// not a player save/filtering decision. No UID adoption, runtime/custody write,
// birth decision, rebind, retirement or ACK. The supplied reference/operation
// remain caller facts; matching prototype/birthplace is not durable authority.
// Capture requires explicit LIVE input. RETIRED images are values constructed
// by the eventual native transition owner and must have an empty forest.
// Every output remains unchanged on failure. Game-thread serialization remains
// the caller's obligation, as for existing literal tree capture.
player_snapshot_capture_result quest_mobile_native_capture(
	P_char, const quest_mobile_native_reference &, quest_mobile_lifetime_state,
	const critical_operation_id &last_transition, quest_mobile_native_image *output) noexcept;

// Cash-aware capture for the original economic owner. The caller supplies its
// retained cash revision; this reads the actual post-conversion native wallet.
// The compatibility overload above still returns a v1 image with unknown cash.
player_snapshot_capture_result
quest_mobile_native_capture(P_char, const quest_mobile_native_reference &,
			    quest_mobile_lifetime_state,
			    const critical_operation_id &last_transition, uint64_t cash_revision,
			    quest_mobile_native_image *output) noexcept;

// Read-only full native item forest on the game thread. No previous-transition
// operation or cash revision is fabricated merely to observe the original stock.
// Reference remains an already-authenticated caller fact; output is unchanged
// on refusal. The existing image capture/codec policies remain unchanged.
player_snapshot_capture_result
quest_mobile_native_items_observe(P_char, const quest_mobile_native_reference &,
				  std::vector<player_item_snapshot> *output) noexcept;

struct item_transfer_payload;
// Pure ordered item values only. Original reference/forest are caller evidence;
// no historical last-transition or cash image is constructed to transform them.
// The cash-aware image participant retains its separate exact cash policy.
player_snapshot_codec_result quest_mobile_native_items_transition(
	std::span<const player_item_snapshot>, const quest_mobile_native_reference &,
	const item_transfer_payload &, std::vector<player_item_snapshot> *after) noexcept;
// Pure ordered stock transition for an already LIVE original native image.
// The enclosing participant validates the complete native-v11 payload contract;
// this helper validates only the original image/reference and literal stock transform.
// Values only: no birth, admission, custody, native mutation or ACK authority.
// Item-only transitions preserve the exact known wallet and its revision.
player_snapshot_codec_result quest_mobile_native_item_transition(
	const quest_mobile_native_image &before, const item_transfer_payload &payload,
	const critical_operation_id &operation, quest_mobile_native_image *after) noexcept;

// Explicit15/16 finite cash acceptance preserves the complete original stock,
// stock/custody revision and native lifetime. Only mobile/cash revisions advance.
// Original root proves both actual wallets and the before image independently;
// this pure projection supplies no source, SQL or publication authority.
// Pure no-item original fee action: actual NPC cash/mobile revision changes,
// full stock and custody are unchanged. Caller owes source/root/SQL publication.
player_snapshot_codec_result
quest_mobile_native_fee_transition(const quest_mobile_native_image &, const item_transfer_payload &,
				   const critical_operation_id &,
				   quest_mobile_native_image *) noexcept;

player_snapshot_codec_result
quest_mobile_native_money_transition(const quest_mobile_native_image &,
				     const item_transfer_payload &, const critical_operation_id &,
				     quest_mobile_native_image *) noexcept;

// Prospective complete native-image companions. The caller includes inputs,
// old outputs and inline output objects in outer_live, and retains the admitted
// absolute simultaneous peak through output transfer. Full original reference,
// checksum, forest/item semantics and exact canonical bytes remain required.
// Requests require GCC13 libstdc++ C++11 ABI; unsupported policy refuses.
// All outputs, including optional actual transferred heap scalar, are strong.
// The scalar excludes the caller's inline image; no source/runtime authority.
player_snapshot_codec_result
quest_mobile_native_image_encode_bounded(const quest_mobile_native_image &, std::vector<uint8_t> *,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t outer_live) noexcept;
player_snapshot_codec_result quest_mobile_native_image_decode_bounded(
	const std::span<const uint8_t> &, quest_mobile_native_image *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_image_heap_bytes = nullptr) noexcept;

// Full original physical forest/cash capture with prospective admission.
// Caller serializes the actual native graph and owns input/prior output/ROOT
// storage in outer_live. Both outputs are strong on every refusal. The retained
// heap excludes the inline image. Historical overload preserves unknown cash.
player_snapshot_capture_result
quest_mobile_native_capture_bounded(P_char, const quest_mobile_native_reference &,
				    quest_mobile_lifetime_state, const critical_operation_id &,
				    quest_mobile_native_image *, bool (*)(size_t, void *) noexcept,
				    void *, size_t, size_t *retained_image_heap = nullptr) noexcept;
player_snapshot_capture_result
quest_mobile_native_capture_bounded(P_char, const quest_mobile_native_reference &,
				    quest_mobile_lifetime_state, const critical_operation_id &,
				    uint64_t cash_revision, quest_mobile_native_image *,
				    bool (*)(size_t, void *) noexcept, void *, size_t,
				    size_t *retained_image_heap = nullptr) noexcept;

// Complete acceptance/consumption transform; original validation and DFS/UID/order
// policy remain authoritative. Outer owns caller inputs/prior output; the callee
// owns actual private copies, codecs, growth and hash/forest validation lifetimes.
size_t quest_mobile_native_items_transition_frame_bytes() noexcept;
player_snapshot_codec_result quest_mobile_native_items_transition_bounded(
	std::span<const player_item_snapshot>, const quest_mobile_native_reference &,
	const item_transfer_payload &, std::vector<player_item_snapshot> *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

// Pure selected SOURCE contracts. Whole SOURCE + early inline is a transient
// preentry admission; the actual parent retains only *_source_supplement across
// existing byte-exact bounded entries. Real child workspaces/requests stay
// child-owned. Queries allocate/scan nothing, strong output on refusal; no
// factory, publication, activation or emitted/native qualification follows.
bool quest_mobile_native_image_encode_source_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_image_decode_source_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_image_encode_source_supplement_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_image_decode_source_supplement_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_image_encode_initial_inline_bytes(size_t *) noexcept;
bool quest_mobile_native_image_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t quest_mobile_native_image_source_query_frame_bytes() noexcept
{
	// Largest real query is decode: own output/result/policy + seven size_t
	// locals, six lower queries (including nested encode's four locals and
	// three queries), six checked adds. Accessor size_t result is separate.
	return 19 * sizeof(void *) + 20 * sizeof(size_t) + 29 * sizeof(bool);
}
bool quest_mobile_native_image_lifetime_source_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_image_current_heap_source_frame_bytes(size_t *) noexcept;
constexpr size_t quest_mobile_native_image_lifetime_source_query_frame_bytes() noexcept
{
	// Own output/result/policy, two locals, list query and checked add.
	return 3 * sizeof(void *) + 3 * sizeof(size_t) + 5 * sizeof(bool);
}
constexpr size_t quest_mobile_native_image_current_heap_source_query_frame_bytes() noexcept
{
	return 3 * sizeof(void *) + 3 * sizeof(size_t) + 5 * sizeof(bool);
}
#endif
