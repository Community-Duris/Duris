#ifndef DONATION_EVENT_H
#define DONATION_EVENT_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct donation_event
{
	char event_id[65];
	int64_t issued_at;
	int64_t amount_cents;
	char currency[4];
	bool is_public;
	char character_name[33];
	char message[257];
};

bool donation_event_decode(const char *json, size_t length, const char *secret, time_t now,
			   struct donation_event *event);

#endif
