#ifndef DURIS_ARTIFACT_RESTITUTION_RAM_H
#define DURIS_ARTIFACT_RESTITUTION_RAM_H
#include "persistence/player_death_restitution_command.h"

struct artifact_restitution_location
{
	bool present = false, owned = false;
	uint8_t type = 0, location_type = 0;
	int32_t location = 0;
	int64_t timer = 0;
};
struct artifact_restitution_state
{
	uint32_t vnum = 0;
	artifact_restitution_location domain, god, mortal;
	bool uid_present = false, baseline_present = false, bind_present = false;
	bool item_revision_present = false;
	uint64_t uid = 0, item_revision = 0, revision = 0, opening_revision = 0;
	int32_t bind_owner = -1, opening_bind_owner = -1;
	int64_t bind_timer = 0, opening_bind_timer = 0, opening_timer = 0;
	int32_t bind_row_owner = -1;
	int64_t bind_row_timer = 0;
};
bool artifact_restitution_ram_seed(const artifact_restitution_state &);
const artifact_restitution_state *artifact_restitution_ram_lookup(uint32_t);
bool artifact_restitution_ram_prepare(const player_death_restitution_plan &, uint64_t);
void artifact_restitution_ram_publish();
void artifact_restitution_ram_reset();
void artifact_restitution_ram_update(const artifact_restitution_state &) noexcept;
void artifact_restitution_ram_feed(uint32_t vnum, int64_t timer, int32_t owner, int64_t bind_timer,
				   uint64_t revision);
#endif
