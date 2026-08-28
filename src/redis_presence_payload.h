#ifndef REDIS_PRESENCE_PAYLOAD_H
#define REDIS_PRESENCE_PAYLOAD_H

#include <cstdint>

struct redis_presence_fields
{
	const char *name;
	const char *account;
	const char *race;
	const char *player_class;
	const char *ip;
	const char *client;
	const char *client_version;
	int level;
	int racewar;
	int64_t login_time;
	bool include_private;
};

char *redis_presence_payload_encode(const redis_presence_fields &fields);

#endif
