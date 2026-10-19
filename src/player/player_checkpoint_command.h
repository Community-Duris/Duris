#ifndef DURIS_PLAYER_CHECKPOINT_COMMAND_H
#define DURIS_PLAYER_CHECKPOINT_COMMAND_H
#include "persistence/critical_command_completion.h"
#include "player/player_save_worker.h"
bool player_checkpoint_command_build(const player_snapshot &, critical_command *);
critical_apply_result player_checkpoint_command_project(const critical_command &,
							player_save_apply_fn, void *);
#endif
