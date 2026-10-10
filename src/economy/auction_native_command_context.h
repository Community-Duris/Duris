#ifndef DURIS_AUCTION_NATIVE_COMMAND_CONTEXT_H
#define DURIS_AUCTION_NATIVE_COMMAND_CONTEXT_H

#include "economy/auction_command.h"
#include "economy/economic_accounting_types.h"
#include "player/player_snapshot_codec.h"

#include <array>
#include <span>

constexpr uint16_t AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION = 2;

// Pointer-free original values only. Digests/counts grant no SQL, checkpoint,
// physical publication, completion or ACK authority. Complete BEFORE/AFTER and
// selected forests remain with the original private retained owner/SQL source;
// this bounded command never fabricates decoded selected literal values.
struct auction_native_command_context
{
	auction_command_payload payload{};
	std::vector<uint8_t> base_v1_payload;
	// Exact acknowledged whole-forest DFS order; values remain digest-bound.
	std::vector<uint64_t> before_item_uids;
	uint32_t original_level = 0;
	uint64_t acknowledged_save_revision = 0;
	std::array<uint8_t, 32> before_digest{}, after_digest{}, selected_digest{};
	uint32_t selected_node_count = 0;
	uint16_t selected_root_count = 0;
};

// Fresh schema1/v1/no-intent preparation only. The original v1 payload and all
// envelope identities/fences/site/deadline are preserved; only payload/version
// gain the v2 trailer. The root-owned accounting binder must subsequently
// authenticate admission and assign schema2. Refusal preserves the full command.
economic_accounting_error
auction_native_command_bind(critical_command *, std::span<const player_item_snapshot> before,
			    std::span<const player_item_snapshot> after,
			    std::span<const player_item_snapshot> selected_literals,
			    uint32_t original_level, uint64_t acknowledged_save_revision) noexcept;

// Structural schema1 preparation/accounting projection or schema2 transport
// decode, never execution admission. A stamped schema1 projection may be decoded
// for the original pure accounting compiler; fresh bind still requires time0,
// and root's shared legacy execution gate refuses v2. Refusal preserves output.
// Original v1/inactive decoders are intact.
economic_accounting_error auction_native_command_decode(const critical_command &,
							auction_native_command_context *) noexcept;

size_t auction_native_command_uid_set_source_frame_bytes() noexcept;
economic_accounting_error auction_native_command_decode_bounded(const critical_command &,
								auction_native_command_context *,
								bool (*)(size_t, void *) noexcept,
								void *, size_t) noexcept;

#endif
