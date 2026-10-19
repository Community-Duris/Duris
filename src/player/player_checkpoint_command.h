#ifndef DURIS_PLAYER_CHECKPOINT_COMMAND_H
#define DURIS_PLAYER_CHECKPOINT_COMMAND_H
#include "persistence/critical_command_completion.h"
#include "player/player_save_worker.h"
bool player_checkpoint_command_build(const player_snapshot &, critical_command *);
critical_apply_result player_checkpoint_command_project(const critical_command &,
							player_save_apply_fn, void *);
// Set only while projecting a sealed checkpoint on this thread. Legacy saves
// return zero and retain their original timestamp behavior.
inline thread_local uint64_t player_checkpoint_projecting_epoch = 0;
inline uint64_t player_checkpoint_projection_epoch()
{
	return player_checkpoint_projecting_epoch;
}
#endif
