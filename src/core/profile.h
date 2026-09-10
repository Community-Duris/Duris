#ifndef __PROFILE_H__
#define __PROFILE_H__

#include "core/clock_utils.h"

#include <stdint.h>

#define DO_PROFILE

#ifdef DO_PROFILE

#ifndef PROFILE_CLOCK_GETTIME
#define PROFILE_CLOCK_GETTIME clock_gettime
#endif

typedef struct
{
	uint64_t started_us;
	uint64_t ended_us;
	uint64_t last_us;
	uint64_t total_inside_us;
	uint64_t total_outside_us;
	uint64_t calls;
	bool start_valid;
	bool end_valid;
	bool skip_next_end;
} profile_timer;

static inline bool profile_monotonic_us(uint64_t *result)
{
	return clock_read_microseconds(CLOCK_MONOTONIC, result, PROFILE_CLOCK_GETTIME);
}

static inline void profile_timer_rebase(profile_timer *timer)
{
	uint64_t now_us = 0;
	const bool valid = profile_monotonic_us(&now_us);

	timer->started_us = now_us;
	timer->ended_us = now_us;
	timer->last_us = 0;
	timer->start_valid = valid;
	timer->end_valid = valid;
	timer->skip_next_end = true;
}

static inline void profile_timer_reset(profile_timer *timer)
{
	timer->total_inside_us = 0;
	timer->total_outside_us = 0;
	timer->calls = 0;
	profile_timer_rebase(timer);
}

static inline void profile_timer_start(profile_timer *timer)
{
	uint64_t now_us = 0;
	timer->skip_next_end = false;
	timer->start_valid = profile_monotonic_us(&now_us);
	if (!timer->start_valid)
		return;
	timer->started_us = now_us;
	if (timer->end_valid && now_us >= timer->ended_us)
		timer->total_outside_us += now_us - timer->ended_us;
}

static inline void profile_timer_end(profile_timer *timer)
{
	if (timer->skip_next_end)
	{
		timer->skip_next_end = false;
		timer->last_us = 0;
		return;
	}
	uint64_t now_us = 0;
	const bool valid = profile_monotonic_us(&now_us);
	timer->last_us = 0;
	timer->calls++;
	if (!valid)
	{
		timer->end_valid = false;
		return;
	}
	timer->ended_us = now_us;
	timer->end_valid = true;
	if (timer->start_valid && now_us >= timer->started_us)
	{
		timer->last_us = now_us - timer->started_us;
		timer->total_inside_us += timer->last_us;
	}
}

// list of active profiles
#define PROFILES(action)                                                                                                \
	PROFILE_##action(connections) PROFILE_##action(commands) PROFILE_##action(prompts) PROFILE_##action(            \
		activities) PROFILE_##action(combat) PROFILE_##action(pulse_reset) PROFILE_##action(event_loop)         \
		PROFILE_##action(event_func) PROFILE_##action(mundane_quest) PROFILE_##action(                          \
			mundane_autoinvis) PROFILE_##action(mundane_wagon) PROFILE_##action(mundane_wakeup)             \
			PROFILE_##action(mundane_justice) PROFILE_##action(mundane_commune) PROFILE_##action(           \
				mundane_autostand) PROFILE_##action(mundane_specproc) PROFILE_##action(mundane_mobcast) \
				PROFILE_##action(mundane_track) PROFILE_##action(                                       \
					mundane_track_1) PROFILE_##action(mundane_track_2)                              \
					PROFILE_##action(mundane_track_3) PROFILE_##action(                             \
						mundane_track_4) PROFILE_##action(mundane_charmbreak)                   \
						PROFILE_##action(mundane_curepoison) PROFILE_##action(                  \
							mundane_wallbreak) PROFILE_##action(mundane_picktarget)         \
							PROFILE_##action(mundane_attack) PROFILE_##action(              \
								mundane_assist) PROFILE_##action(mundane_wander)        \
								PROFILE_##action(mundane_newevent)                      \
									PROFILE_##action(                               \
										mobhunt_dijkstra)                       \
										PROFILE_##action(                       \
											random_mob_create)

#define PROFILE_DEFINE(var) profile_timer var##_profile;
#define PROFILE_DECLARE(var) extern profile_timer var##_profile;
#define PROFILE_RESET(var) profile_timer_reset(&var##_profile);
#define PROFILE_REBASE(var) profile_timer_rebase(&var##_profile);

#define PROFILE_COUNT(var)                     \
	{                                      \
		if (do_profile)                \
			var##_profile.calls++; \
	}

#define PROFILE_START(var)                                   \
	{                                                    \
		if (do_profile)                              \
			profile_timer_start(&var##_profile); \
	}

#define PROFILE_END(var)                                   \
	{                                                  \
		if (do_profile)                            \
			profile_timer_end(&var##_profile); \
	}

#define PROFILE_LAST_US(var) (var##_profile.last_us)

#define PROFILE_SAVE(var)                                                                      \
	save_profile_data(#var, var##_profile.total_inside_us, var##_profile.total_outside_us, \
			  var##_profile.calls);

#define PROFILE_REGISTER_CALL(func, duration_us)                           \
	{                                                                  \
		if (do_profile)                                            \
			register_func_call((void *)(func), (duration_us)); \
	}

extern void save_profile_data(const char *name, uint64_t total_inside_us, uint64_t total_outside_us,
			      uint64_t total);
extern void register_func_call(void *func, uint64_t duration_us);
extern bool do_profile;

extern void init_func_call_info();
extern void save_func_call_info();
extern void reset_func_call_info();

#else

#define PROFILES(action)
#define PROFILE_DEFINE(var)
#define PROFILE_DECLARE(var)
#define PROFILE_RESET(var)
#define PROFILE_REBASE(var)
#define PROFILE_COUNT(var)
#define PROFILE_START(var)
#define PROFILE_END(var)
#define PROFILE_LAST_US(var) 0
#define PROFILE_SAVE(var)
#define PROFILE_REGISTER_CALL(func, duration_us)

#endif

PROFILES(DECLARE);

#endif // __PROFILE_H__
