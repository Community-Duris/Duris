#ifndef SKILL_NOTCH_H
#define SKILL_NOTCH_H

#include <cstdint>

struct char_data;
// Frozen skill result. Planning consumes the existing RNG without changing
// skills, timers or messages. Application never rolls again.
struct skill_notch_outcome
{
	uint32_t learned_before = 0;
	uint32_t learned_after = 0;
	uint32_t timer_tag = 0;
	uint32_t timer_duration = 0;
};

skill_notch_outcome skill_notch_prepare(char_data *, int skill, float chance);
bool skill_notch_apply(char_data *, int skill, const skill_notch_outcome &);

#endif
