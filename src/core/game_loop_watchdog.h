#ifndef DURIS_GAME_LOOP_WATCHDOG_H
#define DURIS_GAME_LOOP_WATCHDOG_H

// Called only by the world thread. With no launcher-provided fd these are no-ops.
bool game_loop_watchdog_init();
// A persistent supervisor authenticates the world PID; it emits no loop credit.
void game_loop_watchdog_delegate(int world_pid);
void game_loop_watchdog_completed();
// R=resume, C=copyover, S=intentional stop, D=restart/pwipe drain.
void game_loop_watchdog_lifecycle(char phase);
bool game_loop_watchdog_prepare_copyover();

#endif
