// epic_bonus.c
//
// created by: Venthix 9-11-11
//
// Selectable bonuses granted based on epics received within the last week.
// Choosing a new bonus will reset your counter, you can only accumulate epics towards
// the bonus chosen, if a new bonus is chosen your accumulated epics will be reset to
// the date and time you chose.

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "epic.h"
#include "epic_bonus.h"
#include <string.h>
#include "config.h"
#include "sql.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

struct epic_bonus_data ebd[] = {
	{ EPIC_BONUS_NONE, "none", "No Epic Bonus" },
	{ EPIC_BONUS_CARGO, "cargo", "Cargo Discount" },
	{ EPIC_BONUS_SHOP, "shop", "Shop Discount" },
	{ EPIC_BONUS_EXP, "exp", "Experience Bonus" },
	{ EPIC_BONUS_EPIC_POINT, "epic", "Epic Points Bonus" },
	{ EPIC_BONUS_HEALTH_REG, "health", "Health Regen Bonus" },
	{ EPIC_BONUS_MOVE_REG, "moves", "Movement Regen Bonus" },
	{},
};

static_assert(EPIC_BONUS_MOVE_REG == EPIC_BONUS_STATE_MAX_TYPE,
	      "epic bonus state type bound must match the public bonus types");

static bool epic_bonus_config(int type, int *window_days, double *contribution_cap,
			      double *maximum_modifier);

// command interpreter for epic_bonus
void do_epic_bonus(P_char ch, char *arg, int /*cmd*/)
{
	char buff[MAX_STRING_LENGTH];
	int type = 0;

	// Clear "bonus" from args
	arg = one_argument(arg, buff);
	// Get first argument
	arg = one_argument(arg, buff);

	for (int i = 1; ebd[i].type; i++)
	{
		if (!str_cmp(buff, ebd[i].name))
		{
			type = ebd[i].type;
			break;
		}
	}

	if (type || !str_cmp(buff, "none"))
	{
		epic_bonus_set(ch, type);
		return;
	}
	else
	{
		epic_bonus_help(ch);
		return;
	}
}

// Display help on epic bonus
void epic_bonus_help(P_char ch)
{
	EpicBonusData ebdata;
	if (!get_epic_bonus_data(ch, &ebdata))
	{
		ebdata.type = EPIC_BONUS_NONE;
	}

	send_to_char("&+WEpic Bonus:&n\r\n\r\n", ch);
	send_to_char_f(ch,
		       "&+cYou are currently benefiting from the &+C%s &+c(&+C%.2f%&+c).\r\n\r\n",
		       ebd[ebdata.type].description, get_epic_bonus(ch, ebdata.type) * 100);
	send_to_char("&+CYou can choose from the following bonuses:&n\r\n", ch);
	send_to_char_f(ch, "&+C%10s &+c(&+CMAX: %3d%%&+c) &+w- &+c%s\r\n", ebd[0].name,
		       (int)(get_epic_bonus_max(0) * 100), ebd[0].description);
	for (int i = 1; ebd[i].type; i++)
	{
		if (ebd[i].type == EPIC_BONUS_HEALTH_REG)
			send_to_char_f(ch, "&+C%10s &+c(&+CMax: %3d &+c) &+w- &+c%s\r\n",
				       ebd[i].name,
				       (int)(get_epic_bonus_max(i) * EPIC_HEALTH_REGEN_MOD),
				       ebd[i].description);
		else
			send_to_char_f(ch, "&+C%10s &+c(&+CMax: %3d%%&+c) &+w- &+c%s\r\n",
				       ebd[i].name, (int)(get_epic_bonus_max(i) * 100),
				       ebd[i].description);
	}
	send_to_char("\r\n", ch);
	return;
}

// Set the new epic bonus on the character, and reset their timer to now()
void epic_bonus_set(P_char ch, int type)
{
	if (!IS_PC(ch) || type < EPIC_BONUS_NONE || type > EPIC_BONUS_MOVE_REG)
		return;

	if (!qry("INSERT INTO epic_bonus (pid, type, time) VALUES ('%i', '%i', NOW()) "
		 "ON DUPLICATE KEY UPDATE type=VALUES(type), time=VALUES(time)",
		 GET_PID(ch), type))
	{
		send_to_char(
			"Your epic bonus could not be changed right now. Please try again.\r\n",
			ch);
		return;
	}

	int window_days = 0;
	double contribution_cap = 0.0;
	double maximum_modifier = 0.0;
	if (!epic_bonus_config(type, &window_days, &contribution_cap, &maximum_modifier) ||
	    !epic_bonus_state_select(&ch->only.pc->epic_bonus_state, type, time(NULL),
				     contribution_cap, maximum_modifier))
		epic_bonus_state_mark_unavailable(&ch->only.pc->epic_bonus_state);
	else
		ch->only.pc->epic_bonus_state.window_days = window_days;

	send_to_char_f(ch, "Your epic bonus has been changed to %s.\r\n", ebd[type].description);
	send_to_char("&+RPlease be aware your timer has been reset to now.&n\r\n", ch);

	return;
}

float get_epic_bonus_max(int id)
{
	char buff[MAX_STRING_LENGTH];

	int i;
	for (i = 1; ebd[i].type; i++)
		;

	if (id < 0 || id >= i)
	{
		debug("get_epic_bonus_max(): called with invalid id: %d", id);
		return 0;
	}

	snprintf(buff, MAX_STRING_LENGTH, "epic.bonus.max.%s", ebd[id].name);

	return get_property(buff, 0.000);
}

bool get_epic_bonus_data(P_char ch, EpicBonusData *ebdata)
{
	if (!ebdata || !IS_PC(ch))
		return false;

	struct EpicBonusState *state = &ch->only.pc->epic_bonus_state;
	if (state->status != EPIC_BONUS_STATE_READY)
		return false;

	ebdata->pid = GET_PID(ch);
	ebdata->type = state->selected_type;
	ebdata->time[0] = '\0';
	struct tm selected_tm;
	if (state->selected_at > 0 && localtime_r(&state->selected_at, &selected_tm))
		strftime(ebdata->time, sizeof(ebdata->time), "%Y-%m-%d %H:%M:%S", &selected_tm);

	return true;
}

float get_epic_bonus(P_char ch, int type)
{
	if (!IS_PC(ch))
		return 0;

	struct EpicBonusState *state = &ch->only.pc->epic_bonus_state;
	if (state->status != EPIC_BONUS_STATE_READY)
		return 0;
	int window_days = 0;
	double contribution_cap = 0.0;
	double maximum_modifier = 0.0;
	if (!epic_bonus_config(state->selected_type, &window_days, &contribution_cap,
			       &maximum_modifier) ||
	    state->window_days != window_days)
	{
		epic_bonus_state_mark_unavailable(state);
		return 0;
	}
	state->contribution_cap = contribution_cap;
	state->maximum_modifier = maximum_modifier;
	return (float)epic_bonus_state_modifier(state, type, time(NULL));
}

static bool parse_int64(const char *value, int64_t *out)
{
	if (!value || !*value || !out)
		return false;
	errno = 0;
	char *end = NULL;
	const long long parsed = strtoll(value, &end, 10);
	if (errno || !end || *end != '\0')
		return false;
	*out = (int64_t)parsed;
	return true;
}

static bool epic_bonus_config(int type, int *window_days, double *contribution_cap,
			      double *maximum_modifier)
{
	if (type < EPIC_BONUS_NONE || type > EPIC_BONUS_MOVE_REG || !window_days ||
	    !contribution_cap || !maximum_modifier)
		return false;

	const double configured_window = get_property("epic.bonus.time", 7.0);
	*contribution_cap = get_property("epic.bonus.epic.cap", 1.0);
	*maximum_modifier = get_epic_bonus_max(type);
	if (!isfinite(configured_window) || configured_window < 1.0 ||
	    configured_window > EPIC_BONUS_STATE_MAX_WINDOW_DAYS ||
	    configured_window != floor(configured_window) || !isfinite(*contribution_cap) ||
	    *contribution_cap <= 0.0 || !isfinite(*maximum_modifier) || *maximum_modifier < 0.0)
		return false;

	*window_days = (int)configured_window;
	return true;
}

static bool epic_bonus_expiry(time_t when, int window_days, time_t *expires_at)
{
	if (when < 0 || window_days < 1 || window_days > EPIC_BONUS_STATE_MAX_WINDOW_DAYS ||
	    !expires_at)
		return false;

	struct tm local;
	if (!localtime_r(&when, &local))
		return false;
	const bool exact_midnight = local.tm_hour == 0 && local.tm_min == 0 && local.tm_sec == 0;
	local.tm_hour = 0;
	local.tm_min = 0;
	local.tm_sec = 0;
	local.tm_isdst = -1;
	local.tm_mday += window_days + (exact_midnight ? 0 : 1);
	const time_t expiry = mktime(&local);
	if (expiry <= when)
		return false;
	*expires_at = expiry;
	return true;
}

bool epic_bonus_hydrate(P_char ch)
{
	if (!IS_PC(ch) || GET_PID(ch) <= 0 || !DB)
		return false;

	struct EpicBonusState *state = &ch->only.pc->epic_bonus_state;
	int window_days = 0;
	double contribution_cap = 0.0;
	double maximum_modifier = 0.0;
	const double configured_window = get_property("epic.bonus.time", 7.0);
	if (!isfinite(configured_window) || configured_window < 1.0 ||
	    configured_window > EPIC_BONUS_STATE_MAX_WINDOW_DAYS ||
	    configured_window != floor(configured_window))
	{
		epic_bonus_state_mark_unavailable(state);
		logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=invalid_window");
		return false;
	}
	window_days = (int)configured_window;

	MYSQL_RES *res = db_query(
		"SELECT eb.type, UNIX_TIMESTAMP(eb.time), "
		"UNIX_TIMESTAMP(CASE WHEN TIME(eg.time) = '00:00:00' "
		"THEN DATE_ADD(DATE(eg.time), INTERVAL %d DAY) "
		"ELSE DATE_ADD(DATE(eg.time), INTERVAL %d DAY) END) AS expires_at, "
		"SUM(eg.epics) "
		"FROM epic_bonus eb LEFT JOIN epic_gain eg ON eg.pid=eb.pid AND eg.type != %d "
		"AND eg.epics > 0 AND eg.time > DATE_SUB(CURDATE(), INTERVAL %d DAY) "
		"AND eg.time > eb.time WHERE eb.pid=%d "
		"GROUP BY eb.type, eb.time, expires_at ORDER BY expires_at",
		window_days, window_days + 1, EPIC_BOTTLE, window_days, GET_PID(ch));
	if (!res)
	{
		epic_bonus_state_mark_unavailable(state);
		logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=query_failure");
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		mysql_free_result(res);
		if (!epic_bonus_config(EPIC_BONUS_NONE, &window_days, &contribution_cap,
				       &maximum_modifier) ||
		    !epic_bonus_state_publish(state, EPIC_BONUS_NONE, 0, contribution_cap,
					      maximum_modifier, NULL, 0, time(NULL)))
		{
			epic_bonus_state_mark_unavailable(state);
			return false;
		}
		state->window_days = window_days;
		return true;
	}

	int64_t parsed_type = 0;
	int64_t selected_at = 0;
	if (!parse_int64(row[0], &parsed_type) || !parse_int64(row[1], &selected_at) ||
	    parsed_type < EPIC_BONUS_NONE || parsed_type > EPIC_BONUS_MOVE_REG || selected_at < 0 ||
	    !epic_bonus_config((int)parsed_type, &window_days, &contribution_cap,
			       &maximum_modifier))
	{
		mysql_free_result(res);
		epic_bonus_state_mark_unavailable(state);
		logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=invalid_header");
		return false;
	}

	struct EpicBonusContributionBucket buckets[EPIC_BONUS_STATE_MAX_BUCKETS] = {};
	int bucket_count = 0;
	do
	{
		int64_t row_type = 0;
		int64_t row_selected_at = 0;
		if (!parse_int64(row[0], &row_type) || !parse_int64(row[1], &row_selected_at) ||
		    row_type != parsed_type || row_selected_at != selected_at)
		{
			mysql_free_result(res);
			epic_bonus_state_mark_unavailable(state);
			logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=inconsistent_rows");
			return false;
		}

		if (row[2] || row[3])
		{
			int64_t expires_at = 0;
			int64_t amount = 0;
			if (!row[2] || !row[3] || !parse_int64(row[2], &expires_at) ||
			    !parse_int64(row[3], &amount) || expires_at <= 0 || amount <= 0 ||
			    bucket_count >= EPIC_BONUS_STATE_MAX_BUCKETS)
			{
				mysql_free_result(res);
				epic_bonus_state_mark_unavailable(state);
				logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=invalid_bucket");
				return false;
			}
			buckets[bucket_count].expires_at = (time_t)expires_at;
			buckets[bucket_count].amount = amount;
			bucket_count++;
		}
	} while ((row = mysql_fetch_row(res)));
	mysql_free_result(res);

	if (!epic_bonus_state_publish(state, (int)parsed_type, (time_t)selected_at,
				      contribution_cap, maximum_modifier, buckets, bucket_count,
				      time(NULL)))
	{
		epic_bonus_state_mark_unavailable(state);
		logit(LOG_DEBUG, "epic_bonus_hydrate: outcome=publish_failure");
		return false;
	}
	state->window_days = window_days;
	return true;
}

void epic_bonus_record_gain(P_char ch, int type, int amount)
{
	if (!IS_PC(ch) || type == EPIC_BOTTLE || amount <= 0)
		return;

	struct EpicBonusState *state = &ch->only.pc->epic_bonus_state;
	if (state->status != EPIC_BONUS_STATE_READY || state->selected_type == EPIC_BONUS_NONE)
		return;

	int window_days = 0;
	double contribution_cap = 0.0;
	double maximum_modifier = 0.0;
	if (!epic_bonus_config(state->selected_type, &window_days, &contribution_cap,
			       &maximum_modifier))
	{
		epic_bonus_state_mark_unavailable(state);
		return;
	}
	if (state->window_days != window_days)
	{
		epic_bonus_state_mark_unavailable(state);
		return;
	}

	const time_t now = time(NULL);
	if (now <= state->selected_at)
		return;
	time_t expires_at = 0;
	if (!epic_bonus_expiry(now, window_days, &expires_at) ||
	    !epic_bonus_state_add(state, amount, expires_at, now))
	{
		epic_bonus_state_mark_unavailable(state);
		return;
	}
	state->contribution_cap = contribution_cap;
	state->maximum_modifier = maximum_modifier;
}
