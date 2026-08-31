#include "redis/redis_presence_runtime.h"

#include "presence_policy.h"
#include "prototypes.h"
#include "redis/redis_presence_payload.h"
#include "redis/redis_presence_worker.h"
#include "utils.h"

#include <cstdlib>
#include <ctime>

extern const struct race_names race_names_table[];

namespace
{
bool presence_enabled = false;
} // namespace

bool redis_presence_runtime_enabled(void)
{
	return presence_enabled;
}

void redis_presence_runtime_set_enabled(bool enabled)
{
	presence_enabled = enabled;
}

void redis_player_online(P_char character)
{
#ifndef __NO_REDIS__
	if (!presence_enabled || !character || IS_NPC(character))
		return;
	if (!durisweb_presence_character_visible(character))
	{
		redis_presence_worker_submit_offline(GET_PID(character), false);
		return;
	}

	const char *account = get_account_name_safe(character);
	const char *race = race_names_table[GET_RACE(character)].ansi;
	const char *character_class = get_class_name(character, NULL);
	const char *ip = (character->desc && character->desc->host[0]) ? character->desc->host : "";
	const char *client = (character->desc && character->desc->client_name[0]) ?
				     character->desc->client_name :
				     "";
	const char *client_version = (character->desc && character->desc->client_version[0]) ?
					     character->desc->client_version :
					     "";
	const time_t login_time = character->player.time.logon;

	const redis_presence_fields fields = { GET_NAME(character),
					       account,
					       race,
					       character_class,
					       ip,
					       client,
					       client_version,
					       GET_LEVEL(character),
					       GET_RACEWAR(character),
					       static_cast<int64_t>(login_time),
					       durisweb_private_presence_enabled() };
	char *json = redis_presence_payload_encode(fields);
	if (!json)
		return;

	redis_presence_worker_submit_online(GET_PID(character), json, true);
	free(json);
#endif
}

void redis_player_offline(P_char character)
{
#ifndef __NO_REDIS__
	if (!presence_enabled || !character || IS_NPC(character))
		return;
	redis_presence_worker_submit_offline(GET_PID(character),
					     durisweb_presence_character_visible(character));
#endif
}

void redis_clear_online_players(void)
{
#ifndef __NO_REDIS__
	if (presence_enabled)
		redis_presence_worker_submit_clear();
#endif
}
