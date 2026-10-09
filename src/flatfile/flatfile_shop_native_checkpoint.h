#ifndef DURIS_FLATFILE_SHOP_NATIVE_CHECKPOINT_H
#define DURIS_FLATFILE_SHOP_NATIVE_CHECKPOINT_H

#include "economy/economic_gameplay_authority.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "player/player_snapshot.h"
#include <cstddef>

struct player_shop_checkpoint_stage;
class shop_trade_native_checkpoint_owner;

// Actual persisted modern-pet owner cut; body/state/properties remain in the
// unchanged full player file at this exact native index. Values grant no source
// or runtime-pet publication authority.
struct flatfile_shop_native_pet_cut
{
	size_t native_pet_index = 0;
	uint64_t pet_uid = 0;
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
};

// Actual current stored values, not source, save-hold, mutation or publication
// capabilities. Custody lacks general literal properties and is retained whole.
struct flatfile_shop_native_player_cut
{
	flatfile_economic_authority_snapshot authority;
	flatfile_player_domain_record native_money;
	player_snapshot player;
	uint64_t player_owner_revision = 0;
	std::vector<flatfile_item_ownership_record> player_custody;
	std::vector<flatfile_shop_native_pet_cut> pet_custody;
};

class flatfile_shop_native_checkpoint_storage final
{
    private:
	friend class shop_trade_native_checkpoint_owner;
	// Original owner resolves journals and proves genuine held-save exclusion,
	// runtime identity and the original queued ACK before invoking this read.
	// Never recovers, acquires another lock, stages, writes or hydrates caches.
	// Output remains unchanged on any refusal/failure. Returns errno-style codes.
	static unsigned int
	read_current_player_locked(const std::string &root, const flatfile_authority_lock &lock,
				   const economic_shop_checkpoint_projection &projection,
				   int32_t pid, const std::string &account_name, int8_t racewar,
				   const player_snapshot &original_queued_ack,
				   const player_shop_checkpoint_stage &original_status,
				   flatfile_shop_native_player_cut *output, std::string *error);
};

#endif
