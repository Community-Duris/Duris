#include "net/casting_pulse_policy.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

static void expect_near(float actual, float expected)
{
	assert(std::fabs(actual - expected) < 0.001f);
}

int main()
{
	const casting_pulse_config defaults = casting_pulse_make_config(
		CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER, CASTING_PULSE_DEFAULT_TANK_SUCCESS_PERCENT,
		CASTING_PULSE_DEFAULT_SKILL_BASE_PERCENT,
		CASTING_PULSE_DEFAULT_SKILL_PERCENT_PER_POINT,
		CASTING_PULSE_DEFAULT_ABORT_BASE_PERCENT,
		CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION,
		CASTING_PULSE_DEFAULT_ABORT_CAP_PERCENT);

	expect_near(casting_pulse_quick_chant_percent(defaults, -20), 0.0f);
	expect_near(casting_pulse_quick_chant_percent(defaults, 0), 0.0f);
	expect_near(casting_pulse_quick_chant_percent(defaults, 50), 50.0f);
	expect_near(casting_pulse_quick_chant_percent(defaults, 100), 100.0f);
	expect_near(casting_pulse_quick_chant_percent(defaults, 140), 100.0f);

	assert(!casting_pulse_percent_roll(0.0f, 1));
	assert(!casting_pulse_percent_roll(-10.0f, 1));
	assert(casting_pulse_percent_roll(0.01f, 1));
	assert(!casting_pulse_percent_roll(0.01f, 2));
	assert(casting_pulse_percent_roll(0.53f, 53));
	assert(!casting_pulse_percent_roll(0.53f, 54));
	assert(casting_pulse_percent_roll(12.34f, 1234));
	assert(!casting_pulse_percent_roll(12.34f, 1235));
	assert(casting_pulse_percent_roll(100.0f, CASTING_PULSE_ROLL_SCALE));
	assert(casting_pulse_percent_roll(500.0f, CASTING_PULSE_ROLL_SCALE));
	assert(!casting_pulse_percent_roll(100.0f, 0));
	assert(!casting_pulse_percent_roll(100.0f, CASTING_PULSE_ROLL_SCALE + 1));
	for (int hundredth = 1; hundredth <= CASTING_PULSE_ROLL_SCALE; ++hundredth)
	{
		const float percent = static_cast<float>(hundredth) / 100.0f;
		assert(casting_pulse_percent_roll(percent, hundredth));
		if (hundredth < CASTING_PULSE_ROLL_SCALE)
			assert(!casting_pulse_percent_roll(percent, hundredth + 1));
	}

	const casting_pulse_timing ordinary = casting_pulse_timing_for(defaults, 18, false);
	assert(ordinary.landing_beats == 18 && ordinary.command_gate_beats == 18);
	const casting_pulse_timing quick = casting_pulse_timing_for(defaults, 18, true);
	assert(quick.landing_beats == 9 && quick.command_gate_beats == 9);
	const casting_pulse_timing odd = casting_pulse_timing_for(defaults, 15, true);
	assert(odd.landing_beats == 7 && odd.command_gate_beats == 7);
	const casting_pulse_timing floor = casting_pulse_timing_for(defaults, 1, true);
	assert(floor.landing_beats == 1 && floor.command_gate_beats == 1);
	const casting_pulse_timing bad_base = casting_pulse_timing_for(defaults, -10, false);
	assert(bad_base.landing_beats == 1 && bad_base.command_gate_beats == 1);
	const casting_pulse_timing maximum =
		casting_pulse_timing_for(defaults, std::numeric_limits<int>::max(), true);
	assert(maximum.landing_beats == 1073741823 && maximum.command_gate_beats == 1073741823);

	expect_near(casting_pulse_max_circle_abort_percent(defaults, 0), 5.0f);
	expect_near(casting_pulse_max_circle_abort_percent(defaults, 60), 5.0f);
	expect_near(casting_pulse_max_circle_abort_percent(defaults, 90), 5.0f);
	expect_near(casting_pulse_max_circle_abort_percent(defaults, 99), 0.5f);
	expect_near(casting_pulse_max_circle_abort_percent(defaults, 100), 0.0f);

	const float nan = std::numeric_limits<float>::quiet_NaN();
	assert(!casting_pulse_percent_roll(nan, 1));
	assert(!casting_pulse_percent_roll(std::numeric_limits<float>::infinity(), 1));
	const casting_pulse_config bounded =
		casting_pulse_make_config(nan, 150.0f, -200.0f, 50.0f, -4.0f, nan, 220.0f);
	expect_near(bounded.quick_multiplier, CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER);
	expect_near(bounded.tank_success_percent, 100.0f);
	expect_near(bounded.skill_base_percent, -100.0f);
	expect_near(bounded.skill_percent_per_point, 5.0f);
	expect_near(bounded.abort_base_percent, 0.0f);
	expect_near(bounded.abort_agility_reduction, CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION);
	expect_near(bounded.abort_cap_percent, 100.0f);
	expect_near(casting_pulse_quick_chant_percent(bounded, 100), 100.0f);
	expect_near(casting_pulse_max_circle_abort_percent(bounded, 0), 0.0f);

	casting_pulse_config hostile = defaults;
	hostile.quick_multiplier = nan;
	hostile.skill_base_percent = nan;
	hostile.skill_percent_per_point = std::numeric_limits<float>::infinity();
	hostile.abort_base_percent = nan;
	hostile.abort_agility_reduction = std::numeric_limits<float>::infinity();
	hostile.abort_cap_percent = nan;
	const casting_pulse_timing hostile_timing = casting_pulse_timing_for(hostile, 18, true);
	assert(hostile_timing.landing_beats == 9 && hostile_timing.command_gate_beats == 9);
	expect_near(casting_pulse_quick_chant_percent(hostile, 100), 100.0f);
	expect_near(casting_pulse_max_circle_abort_percent(hostile, 60), 5.0f);

	std::puts("casting pulse policy: bounded config, exact percentage rolls, skill curve, "
		  "abort cap, and equal landing/gate timing passed");
}
