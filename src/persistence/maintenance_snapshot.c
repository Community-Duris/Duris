#include "persistence/maintenance_snapshot.h"

#include "net/comm.h"
#include "config.h"
#include "ctf.h"
#include "db.h"
#include "epic.h"
#include "frag_cap_config.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "ships/ships.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
#include <set>
#include <string>

extern P_char character_list;
extern P_desc descriptor_list;
extern int max_descs;
extern long boot_time;
extern const char *month_name[];
extern struct time_info_data time_info;
extern const char *weekdays[];
extern P_room world;

namespace
{
bool set_content(maintenance_request &request, const char *content)
{
	if (!content)
		return false;
	const size_t size = strlen(content);
	if (size >= request.content.size())
		return false;
	memcpy(request.content.data(), content, size);
	request.content[size] = '\0';
	request.content_size = size;
	return true;
}

bool prepare_web_status(maintenance_request &request)
{
	char content[MAINTENANCE_CONTENT_MAX] = {};
	const int weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
	const int day = time_info.day + 1;
	const char *suffix = "th";
	if (day == 1 || (day > 20 && day % 10 == 1))
		suffix = "st";
	else if (day == 2 || (day > 20 && day % 10 == 2))
		suffix = "nd";
	else if (day == 3 || (day > 20 && day % 10 == 3))
		suffix = "rd";
	const long now = time(nullptr);
	const struct time_info_data uptime = real_time_passed(now, boot_time);
	const int written = snprintf(content, sizeof(content),
				     "It is %d%s, on %s\nThe %d%s Day of the %s, Year %d.\n"
				     "Time elapsed since boot-up: %d:%02d:%02d\n"
				     "&+RRecord number of connections this boot: &+W%d&n\n",
				     (time_info.hour % 12) ? (time_info.hour % 12) : 12,
				     (time_info.hour > 11) ? "pm" : "am", weekdays[weekday], day,
				     suffix, month_name[time_info.month], time_info.year,
				     uptime.day * 24 + uptime.hour, uptime.minute, uptime.second,
				     max_descs);
	return written >= 0 && static_cast<size_t>(written) < sizeof(content) &&
	       set_content(request, content);
}

bool prepare_statistics(maintenance_request &request)
{
	int64_t goodies = 0;
	int64_t evils = 0;
	int64_t undeads = 0;
	int64_t illithids = 0;
	int64_t gods = 0;
	int64_t inhalls = 0;
	int64_t goodies_level = 0;
	int64_t evils_level = 0;
	int64_t undeads_level = 0;
	int64_t illithids_level = 0;
	std::set<std::string> unique_hosts;
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		P_char character = descriptor->character;
		if (descriptor->connected != CON_PLAYING || !character)
			continue;
		if (IS_TRUSTED(character))
		{
			++gods;
			continue;
		}
		if (character->in_room >= 0 && world[character->in_room].zone == 50)
			++inhalls;
		if (IS_RACEWAR_GOOD(character))
		{
			++goodies;
			goodies_level += GET_LEVEL(character);
		}
		else if (IS_RACEWAR_EVIL(character) && !IS_ILLITHID(character))
		{
			++evils;
			evils_level += GET_LEVEL(character);
		}
		else if (IS_RACEWAR_UNDEAD(character))
		{
			++undeads;
			undeads_level += GET_LEVEL(character);
		}
		else if (IS_ILLITHID(character))
		{
			++illithids;
			illithids_level += GET_LEVEL(character);
		}
		if (descriptor->host[0])
			unique_hosts.emplace(descriptor->host);
	}
	request.value_count = 12;
	request.values[0] = time(nullptr);
	request.values[1] = goodies;
	request.values[2] = evils;
	request.values[3] = illithids;
	request.values[4] = undeads;
	request.values[5] = gods;
	request.values[6] = inhalls;
	request.values[7] = goodies_level;
	request.values[8] = evils_level;
	request.values[9] = undeads_level;
	request.values[10] = illithids_level;
	request.values[11] = static_cast<int64_t>(unique_hosts.size());
	return true;
}

bool prepare_boon_scan(maintenance_request &request)
{
	request.values[0] = time(nullptr);
	const size_t epic_start = 2;
	const size_t epic_values = epic_zone_completion_snapshot(
		request.values.data() + epic_start, request.values.size() - epic_start - 1);
	if (epic_values == SIZE_MAX || epic_values % 2)
		return false;
	request.values[1] = static_cast<int64_t>(epic_values / 2);
	const size_t ctf_count_index = epic_start + epic_values;
	const size_t ctf_values =
		ctf_boon_state_snapshot(request.values.data() + ctf_count_index + 1,
					request.values.size() - ctf_count_index - 1);
	if (ctf_values == SIZE_MAX || ctf_values % 3)
		return false;
	request.values[ctf_count_index] = static_cast<int64_t>(ctf_values / 3);
	request.value_count = ctf_count_index + 1 + ctf_values;
	return true;
}

int64_t scaled(double value)
{
	return static_cast<int64_t>(llround(value * 1000000.0));
}
} // namespace

bool maintenance_prepare_request(maintenance_request &request, void *)
{
	switch (request.job_id)
	{
	case maintenance_job_id::web_status:
		return prepare_web_status(request);
	case maintenance_job_id::operational_statistics:
		return prepare_statistics(request);
	case maintenance_job_id::epic_zone_balance:
		request.value_count = 2;
		request.values[0] = time(nullptr);
		request.values[1] =
			static_cast<int64_t>(get_property("epic.alignment.reset.hour", 7 * 24)) *
			3600;
		return request.values[1] >= 0;
	case maintenance_job_id::epic_zone_modifiers:
		request.value_count = 5;
		request.values[0] = time(nullptr);
		request.values[1] = get_property("epic.freqMod.tick.waitSecs", 3600);
		request.values[2] = scaled(get_property("epic.freqMod.tick.add", 0.002));
		request.values[3] = scaled(get_property("epic.freqMod.max", 2.00));
		request.values[4] = scaled(get_property("epic.freqMod.min", 0.40));
		return request.values[1] >= 0 && request.values[3] >= request.values[4];
	case maintenance_job_id::zone_trophy:
		request.value_count = 4;
		request.values[0] = time(nullptr);
		request.values[1] = get_property("exp.zoneTrophy.enabled", 0) ? 1 : 0;
		request.values[2] = get_property("exp.zoneTrophy.update.secs", 3600);
		request.values[3] = scaled(get_property("exp.zoneTrophy.update.multiplier", 1.0));
		return request.values[2] >= 0 && request.values[3] >= 0;
	case maintenance_job_id::level_cap:
		request.value_count = 1;
		request.values[0] = time(nullptr);
		return true;
	case maintenance_job_id::boon_scan:
		return prepare_boon_scan(request);
	case maintenance_job_id::cargo_market:
		request.value_count = cargo_maintenance_snapshot(
			request.work_id, request.values.data(), request.values.size());
		return request.value_count == 3 + NUM_PORTS * NUM_PORTS * 4;
	default:
		return true;
	}
}
