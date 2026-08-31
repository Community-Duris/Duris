#ifndef EPIC_BONUS_STATE_H
#define EPIC_BONUS_STATE_H

#include <stdint.h>
#include <time.h>

#define EPIC_BONUS_STATE_MAX_BUCKETS 32
#define EPIC_BONUS_STATE_MAX_WINDOW_DAYS 31
#define EPIC_BONUS_STATE_MAX_TYPE 6

enum EpicBonusStateStatus
{
	EPIC_BONUS_STATE_UNINITIALIZED = 0,
	EPIC_BONUS_STATE_READY = 1,
	EPIC_BONUS_STATE_UNAVAILABLE = 2
};

struct EpicBonusContributionBucket
{
	time_t expires_at;
	int64_t amount;
};

struct EpicBonusState
{
	int status;
	int selected_type;
	int window_days;
	time_t selected_at;
	double contribution_cap;
	double maximum_modifier;
	int64_t total_contributions;
	time_t next_expiry;
	int bucket_count;
	struct EpicBonusContributionBucket buckets[EPIC_BONUS_STATE_MAX_BUCKETS];
};

void epic_bonus_state_reset(struct EpicBonusState *state);
void epic_bonus_state_mark_unavailable(struct EpicBonusState *state);
bool epic_bonus_state_publish(struct EpicBonusState *state, int selected_type, time_t selected_at,
			      double contribution_cap, double maximum_modifier,
			      const struct EpicBonusContributionBucket *buckets, int bucket_count,
			      time_t now);
bool epic_bonus_state_select(struct EpicBonusState *state, int selected_type, time_t selected_at,
			     double contribution_cap, double maximum_modifier);
bool epic_bonus_state_add(struct EpicBonusState *state, int64_t amount, time_t expires_at,
			  time_t now);
void epic_bonus_state_expire(struct EpicBonusState *state, time_t now);
double epic_bonus_state_modifier(struct EpicBonusState *state, int requested_type, time_t now);

#endif
