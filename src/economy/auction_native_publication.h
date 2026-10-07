#ifndef AUCTION_NATIVE_PUBLICATION_H
#define AUCTION_NATIVE_PUBLICATION_H

#include "economy/auction_transaction.h"
#include "economy/auction_recovery_context.h"
#include "player/inert_item_stage.h"

// Distinct private capability over the genuine full-literal allocator. The
// wrapper never offers a public allocator, enrollment callback or ACK permit.
class auction_original_item_stage final
{
	friend class auction_native_publication_owner;
	shop_trade_original_item_stage original_;
	static bool prepare(const object_template &, const player_item_snapshot &,
			    auction_original_item_stage &) noexcept;
	static bool reload_step(P_obj, const object_template &, unsigned int,
				shop_trade_original_reload_effect &) noexcept;
	static bool proclib_probe(P_obj, const object_template &, size_t,
				  shop_trade_original_reload_effect &) noexcept;
	P_obj get() const noexcept;
	P_obj consume() noexcept;
};

// Pure whole-player transform for the original owner/SQL writer. BEFORE is the
// acknowledged original saved body; source literals are separately authenticated.
// The real native primitive inserts before the first matching R_num root,
// otherwise at the inventory head. For sealed templates VNUM binds that R_num.
// This grants no source, custody, publication or ACK authority. Strong output.
bool auction_native_expected_player_forest(const auction_command_payload &,
					   std::span<const player_item_snapshot> original_before,
					   std::span<const player_item_snapshot> original_selected,
					   bool rejected, uint32_t original_actor_level,
					   std::vector<player_item_snapshot> *) noexcept;
bool auction_native_selected_forest_valid(const auction_command_payload &,
					  std::span<const player_item_snapshot>) noexcept;

enum class auction_native_world_location : uint8_t
{
	absent,
	carried,
	detached
};
struct auction_native_world_uid
{
	uint64_t uid = 0;
	auction_native_world_location location = auction_native_world_location::absent;
};
struct auction_native_world_observation
{
	P_char actor = nullptr;
	std::vector<player_item_snapshot> player_items;
	std::vector<P_obj> selected;
	// Exact observed full tree for each selected root; absent roots have empty
	// trees. Original SQL/world proof must compare these values separately.
	std::vector<std::vector<player_item_snapshot>> selected_trees;
};
// Complete original world link/UID/actor census, including room, descriptor,
// detached, equipment and omitted NORENT bodies. Actor runtime zero requires
// actual complete PID-body absence; no pointer is retained across callbacks.
// Values/pointers grant neither SQL custody proof nor publication/ACK authority.
bool auction_native_world_observe(uint32_t actor_pid, uint64_t actor_runtime_id,
				  std::span<const player_item_snapshot> expected_player,
				  std::span<const auction_native_world_uid> selected,
				  auction_native_world_observation *) noexcept;

// These are the domain owner's entry points. They preserve original operation
// identities on uncertainty; legacy/inactive submissions remain separate.
bool auction_native_publication_submit(P_char, const auction_command_payload &,
				       auction_completion_fn, critical_source_site,
				       critical_deadline_class) noexcept;
void auction_native_publication_completions(const critical_completion *, size_t) noexcept;
void auction_native_publication_pulse() noexcept;
void auction_native_publication_player_ready(P_char) noexcept;
bool auction_native_publication_player_busy(P_char) noexcept;
// Passive preparation-only registration. Neither callback admits, executes,
// invokes native effects nor releases a player hold or publication reservation.
bool auction_native_publication_restore(const critical_native_recovery_envelope &) noexcept;
bool auction_native_publication_restore_replayed_command(const critical_command &) noexcept;

#endif
