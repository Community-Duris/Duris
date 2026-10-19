#ifndef DURIS_PLAYER_CHECKPOINT_RAM_H
#define DURIS_PLAYER_CHECKPOINT_RAM_H
#include "player/player_snapshot.h"
#include "item/item_ownership_runtime.h"
#include <map>

struct player_death_ram_evidence
{
	critical_operation_id operation_id{};
	std::array<uint8_t, 32> digest{};
	uint64_t epoch = 0;
	std::map<uint64_t, player_death_custody_snapshot> custody;
};
struct player_checkpoint_ram_change
{
	item_ownership_runtime_patch custody;
	player_death_ram_evidence death;
	uint32_t pid = 0;
	uint64_t revision = 0;
	bool has_death = false;
};
bool player_checkpoint_ram_prepare(const player_snapshot &, uint64_t accepted_at_usec,
				   bool flatfile, player_checkpoint_ram_change *);
bool player_checkpoint_ram_publish(player_checkpoint_ram_change *);
bool player_checkpoint_ram_seed_revision(uint32_t pid, uint64_t revision);
bool player_checkpoint_ram_seed_death(uint32_t pid, uint64_t revision,
				      player_death_ram_evidence evidence);
bool player_checkpoint_ram_revision(uint32_t pid, uint64_t *revision);
const player_death_ram_evidence *player_checkpoint_ram_death(uint32_t pid, uint64_t revision);
void player_checkpoint_ram_reset();
bool player_checkpoint_ram_artifact_unique(int32_t vnum, uint64_t uid);
#endif
