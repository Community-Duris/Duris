#include "world/falling_policy.h"

#include <algorithm>

/* Keep all legacy arithmetic and boundary rules here so they can be verified
 * without constructing characters, rooms, or events. */

bool falling_climb_catches(bool climb_active, int climb_skill, int roll)
{
	if (!climb_active)
		return false;
	const int chance = std::clamp(climb_skill, 0, 100) / 2;
	return roll <= chance;
}

falling_route falling_choose_route(bool has_vertical_distance, bool has_open_down_exit)
{
	if (has_vertical_distance)
		return falling_route::vertical;
	if (has_open_down_exit)
		return falling_route::downward;
	return falling_route::stay;
}

bool falling_should_land(bool has_down_exit, bool has_valid_down_destination, bool down_closed,
			 bool down_breakable, bool completed_vertical_descent)
{
	return !has_down_exit || !has_valid_down_destination || down_closed || down_breakable ||
	       completed_vertical_descent;
}

falling_speed_decision falling_advance_speed(int speed, bool levitating, bool flying)
{
	if (levitating || flying)
	{
		speed -= flying ? 4 : 8;
		if (speed <= 0)
			return { 0, falling_motion::stopped };
		return { speed, falling_motion::continue_falling };
	}

	if (speed == 1)
		speed = 31;
	else if (speed == 31)
		speed = 43;
	else
		speed += 8;

	if (speed > 250)
		speed = 250;
	else if (speed < 31)
		speed = 31;

	return { speed, falling_motion::continue_falling };
}

int falling_impact_damage(int max_hit, int speed, int agility, int impact_roll, int safe_fall_skill,
			  int safe_fall_roll)
{
	int damage = static_cast<int>(max_hit * ((speed / 2.5) / 100)) + impact_roll - agility;
	if (damage < 2)
		damage = 2;
	if (safe_fall_skill && safe_fall_skill > safe_fall_roll)
		damage /= 2;
	return damage;
}

bool falling_breaks_floor(int speed, int wall_strength)
{
	return speed > 43 || wall_strength / 2 < 10;
}

int falling_event_delay(int speed)
{
	if (speed == 31)
		return 4;
	if (speed == 43)
		return 2;
	return 1;
}
