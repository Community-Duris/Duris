#ifndef DURIS_BOON_RAM_H
#define DURIS_BOON_RAM_H

#include "flatfile/flatfile_boon_repository.h"
#include <map>

struct boon_ram_state
{
	std::map<uint32_t, flatfile_boon_definition> definitions;
	std::map<std::pair<uint32_t, uint32_t>, double> progress;
	std::map<uint32_t, flatfile_boon_player_projection> shops;
	std::map<uint32_t, uint64_t> stat_revisions;
	std::map<std::pair<uint32_t, uint8_t>, std::pair<uint64_t, int16_t>> stat_changes;
};

void boon_ram_seed(boon_ram_state state);
void boon_ram_overlay_stats(uint32_t pid, uint64_t observed_revision,
			    std::array<int16_t, 10> *stats);
bool boon_ram_player(uint32_t pid, flatfile_boon_player_projection *result);
bool boon_ram_definition(uint32_t id, flatfile_boon_definition *result);
bool boon_ram_progress(uint32_t boon, uint32_t pid, double *counter);
// Definition changes are configuration, serialized after outstanding operations.
bool boon_ram_configure(const flatfile_boon_definition &definition);
critical_apply_result boon_ram_prepare(const critical_command &command, int16_t current_stat = 0);
bool boon_ram_publish(const critical_command &command) noexcept;

#endif
