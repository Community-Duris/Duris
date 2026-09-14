#ifndef DURIS_FALLING_POLICY_H
#define DURIS_FALLING_POLICY_H

enum class falling_route
{
	stay,
	vertical,
	downward
};

enum class falling_motion
{
	continue_falling,
	stopped
};

struct falling_speed_decision
{
	int speed;
	falling_motion motion;
};

bool falling_climb_catches(bool climb_active, int climb_skill, int roll);
falling_route falling_choose_route(bool has_vertical_distance, bool has_open_down_exit);
bool falling_should_land(bool has_down_exit, bool has_valid_down_destination, bool down_closed,
			 bool down_breakable, bool completed_vertical_descent);
falling_speed_decision falling_advance_speed(int speed, bool levitating, bool flying);
int falling_impact_damage(int max_hit, int speed, int agility, int impact_roll, int safe_fall_skill,
			  int safe_fall_roll);
int falling_injury_percent(int damage, int max_hit);
bool falling_breaks_floor(int speed, int wall_strength);
int falling_event_delay(int speed);

#endif // DURIS_FALLING_POLICY_H
