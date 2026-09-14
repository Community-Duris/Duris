#ifndef DURIS_CASTING_PULSE_POLICY_H
#define DURIS_CASTING_PULSE_POLICY_H

constexpr int CASTING_PULSE_ROLL_SCALE = 10000;
constexpr float CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER = 0.5f;
constexpr float CASTING_PULSE_DEFAULT_TANK_SUCCESS_PERCENT = 75.0f;
constexpr float CASTING_PULSE_DEFAULT_SKILL_BASE_PERCENT = 0.0f;
constexpr float CASTING_PULSE_DEFAULT_SKILL_PERCENT_PER_POINT = 1.0f;
constexpr float CASTING_PULSE_DEFAULT_ABORT_BASE_PERCENT = 50.0f;
constexpr float CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION = 0.5f;
constexpr float CASTING_PULSE_DEFAULT_ABORT_CAP_PERCENT = 5.0f;

struct casting_pulse_config
{
	float quick_multiplier = CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER;
	float tank_success_percent = CASTING_PULSE_DEFAULT_TANK_SUCCESS_PERCENT;
	float skill_base_percent = CASTING_PULSE_DEFAULT_SKILL_BASE_PERCENT;
	float skill_percent_per_point = CASTING_PULSE_DEFAULT_SKILL_PERCENT_PER_POINT;
	float abort_base_percent = CASTING_PULSE_DEFAULT_ABORT_BASE_PERCENT;
	float abort_agility_reduction = CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION;
	float abort_cap_percent = CASTING_PULSE_DEFAULT_ABORT_CAP_PERCENT;
};

struct casting_pulse_timing
{
	int landing_beats;
	int command_gate_beats;
};

casting_pulse_config casting_pulse_make_config(float quick_multiplier, float tank_success_percent,
					       float skill_base_percent,
					       float skill_percent_per_point,
					       float abort_base_percent,
					       float abort_agility_reduction,
					       float abort_cap_percent);

float casting_pulse_quick_chant_percent(const casting_pulse_config &config, int skill);
float casting_pulse_max_circle_abort_percent(const casting_pulse_config &config, int agility);
bool casting_pulse_percent_roll(float percent, int roll);
casting_pulse_timing casting_pulse_timing_for(const casting_pulse_config &config, int base_beats,
					      bool quick_chant_succeeded);

#endif // DURIS_CASTING_PULSE_POLICY_H
