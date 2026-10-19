#ifndef DURIS_PLAYER_RESTITUTION_RAM_H
#define DURIS_PLAYER_RESTITUTION_RAM_H
#include "persistence/critical_command_completion.h"
#include "persistence/player_death_restitution_command.h"
using restitution_artifact_prepare_fn = bool (*)(const player_death_restitution_plan &, uint64_t);
using restitution_artifact_publish_fn = void (*)();
void player_restitution_ram_artifact_callbacks(restitution_artifact_prepare_fn,
					       restitution_artifact_publish_fn);
bool player_restitution_ram_seed_delivery(uint64_t uid);
void player_restitution_ram_reset();
critical_apply_result player_restitution_ram_prepare(const critical_command &);
bool player_restitution_ram_can_publish(const critical_command &);
bool player_restitution_ram_publish(const critical_command &);
#endif
