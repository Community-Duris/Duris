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
	bool current_private_heap_bytes(const player_item_snapshot &, size_t *) const noexcept;
	static size_t current_private_heap_observer_frame_bytes() noexcept;
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

// Pure actual retained native-auction owner storage on the game thread.
// Includes the real registry/body/command/context/stage capacities; pooled object
// and affect pages are owned by the caller's paired global census. No allocation,
// callbacks, cached attachment estimate, readiness/route selection or authority.
// False/null/unsupported preserves the scalar output. Frames are separately
// admitted by the caller before observation and excluded from retained CURRENT.
bool auction_native_publication_current_storage_bytes(size_t *) noexcept;
size_t auction_native_publication_current_storage_observer_frame_bytes() noexcept;

class player_save_coin_replay_budget_scope_owner;
// Full genuine native owner; ROOT excludes this CURRENT and same-scope pipeline
// CURRENT from outer. Every nested reservation refreshes both actual owners.
size_t auction_native_publication_replay_source_frame_bytes() noexcept;
bool auction_native_expected_player_forest_bounded(const auction_command_payload &,
						   std::span<const player_item_snapshot>,
						   std::span<const player_item_snapshot>, bool,
						   uint32_t, std::vector<player_item_snapshot> *,
						   bool (*)(size_t, void *) noexcept, void *,
						   size_t) noexcept;
bool auction_native_selected_forest_valid_bounded(const auction_command_payload &,
						  std::span<const player_item_snapshot>,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept;
bool auction_repository_frozen_accounting_valid_bounded(const critical_command &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t) noexcept;
bool auction_native_publication_restore_bounded(const critical_native_recovery_envelope &,
						player_save_coin_replay_budget_scope_owner &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t) noexcept;
bool auction_native_publication_restore_replayed_command_bounded(
	const critical_command &, player_save_coin_replay_budget_scope_owner &,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

// Additive complete original five-arm fixed validator and genuine profiles.
// The original/default APIs and the real SQL/flat caller guards are untouched.
#include "economy/auction_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_money_claim_accounting.h"
bool auction_repository_frozen_accounting_valid_fixed_bounded(const critical_command &,
							      bool (*)(size_t, void *) noexcept,
							      void *, size_t) noexcept;
bool auction_repository_frozen_accounting_valid_own_source_frame_bytes(size_t *) noexcept;
bool auction_repository_frozen_accounting_valid_source_frame_bytes(size_t *) noexcept;
bool auction_repository_frozen_accounting_valid_initial_inline_bytes(size_t *) noexcept;
bool auction_repository_frozen_accounting_valid_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_repository_frozen_accounting_valid_own_source_query_frame_bytes() noexcept
{
	return sizeof(void *) + 2 * sizeof(bool);
}
constexpr size_t auction_repository_frozen_accounting_valid_source_query_frame_bytes() noexcept
{
	return auction_repository_frozen_accounting_valid_own_source_query_frame_bytes() +
	       auction_bid_accounting_decode_source_query_frame_bytes() +
	       auction_settlement_accounting_decode_source_query_frame_bytes() +
	       auction_listing_accounting_decode_source_query_frame_bytes() +
	       auction_item_claim_accounting_decode_source_query_frame_bytes() +
	       auction_money_claim_accounting_decode_source_query_frame_bytes() +
	       // Actual full getter output/result, seven locals, each child bool
	       // and checked-add reference/value/result. Accessor return is caller-owned.
	       2 * sizeof(void *) + 8 * sizeof(size_t) + 8 * sizeof(bool);
}

#endif
