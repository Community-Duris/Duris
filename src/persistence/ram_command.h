#ifndef DURIS_RAM_COMMAND_H
#define DURIS_RAM_COMMAND_H

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_coordinator.h"

// Called on the world thread after the preceding outcome has been published.
// Preparation reads live state only and never publishes or performs storage I/O.
critical_apply_result ram_command_prepare(const critical_command &, void *);
critical_submit_result ram_command_submit(const critical_command &);
void ram_command_set_combat_sequence(uint64_t next, bool flatfile);
struct zone_touch_result;
bool ram_command_seed_stone(const zone_touch_result &result);

#endif
