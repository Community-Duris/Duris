#ifndef DURIS_SHOP_TRADE_FLAT_NATIVE_CHECKPOINT_H
#define DURIS_SHOP_TRADE_FLAT_NATIVE_CHECKPOINT_H

#include "flatfile/flatfile_shop_native_checkpoint.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "player/player_save_pipeline.h"

// A complete original source cut retained before the first journal handoff.
// These public values are never write, hold, readiness, publication or ACK
// capabilities. Only the existing preparation/native owners change phases.
struct shop_trade_flat_native_checkpoint_stage
{
	critical_operation_id operation_id{};
	player_flat_shop_checkpoint_token player_token{};
	player_flat_shop_checkpoint_cut player_hold_cut;
	economic_shop_checkpoint_projection mapping{};
	player_shop_checkpoint_stage acknowledged_status{};
	player_snapshot original_queued_ack;
	flatfile_shop_native_player_cut player;
	uint64_t keeper_owner_revision = 0;
	std::vector<flatfile_item_ownership_record> keeper_custody;
	flatfile_shopkeeper_record keeper_before, keeper_after;
	flatfile_authority_after_image catalog_before, catalog_after;
};

// Original flat journal semantics. A proved unpublished BEFORE is terminal:
// it never resets the retained operation to a fresh native source attempt.
enum class shop_trade_flat_native_checkpoint_phase : uint8_t
{
	preparing,
	sealed,
	attempt_started,
	uncertain,
	ready,
	unpublished_before
};

class shop_trade_native_checkpoint_owner;
class shop_trade_preparation_owner;
// Only the genuine native owner constructs this after checking the complete
// retained source cut under its original lock. A public enum/boolean cannot
// mark an original preparation ready or release its held player checkpoint.
class shop_trade_flat_native_checkpoint_resolution final
{
    private:
	friend class shop_trade_native_checkpoint_owner;
	friend class shop_trade_preparation_owner;
	enum class cut : uint8_t
	{
		before,
		after
	};
	shop_trade_flat_native_checkpoint_resolution(
		const shop_trade_flat_native_checkpoint_stage *original, cut proved) noexcept
		: original_(original)
		, proved_(proved)
	{
	}
	const shop_trade_flat_native_checkpoint_stage *original_;
	cut proved_;
};

#endif
