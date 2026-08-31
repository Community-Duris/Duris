#include "redis/redis_presence_payload.h"

#include <cjson/cJSON.h>

char *redis_presence_payload_encode(const redis_presence_fields &fields)
{
	cJSON *root = cJSON_CreateObject();
	if (!root)
		return nullptr;

	cJSON_AddStringToObject(root, "name", fields.name ? fields.name : "");
	cJSON_AddNumberToObject(root, "level", fields.level);
	cJSON_AddStringToObject(root, "race", fields.race ? fields.race : "");
	cJSON_AddStringToObject(root, "class", fields.player_class ? fields.player_class : "");
	cJSON_AddNumberToObject(root, "racewar", fields.racewar);
	cJSON_AddNumberToObject(root, "login_time", static_cast<double>(fields.login_time));
	if (fields.include_private)
	{
		cJSON_AddStringToObject(root, "account", fields.account ? fields.account : "");
		cJSON_AddStringToObject(root, "ip", fields.ip ? fields.ip : "");
		cJSON_AddStringToObject(root, "client", fields.client ? fields.client : "");
		cJSON_AddStringToObject(root, "client_version",
					fields.client_version ? fields.client_version : "");
	}

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}
