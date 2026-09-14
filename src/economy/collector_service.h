#ifndef DURIS_COLLECTOR_SERVICE_H
#define DURIS_COLLECTOR_SERVICE_H

#include "core/structs.h"

#include <cstddef>
#include <cstdint>

struct collector_service_health
{
	size_t pending_details = 0;
	uint64_t submitted_details = 0;
	uint64_t completed_details = 0;
	uint64_t abandoned_offline = 0;
	uint64_t stale_results = 0;
	uint64_t rejected_details = 0;
	uint64_t submitted_purchases = 0;
	uint64_t rejected_purchases = 0;
	uint64_t committed_purchases = 0;
	uint64_t materialization_failures = 0;
};

void collector_service_command(P_char character, char *arguments, int command);
void collector_service_pulse(void);
bool collector_service_player_busy(P_char character);
collector_service_health collector_service_health_copy(void);
void collector_service_reset_for_tests(void);

#endif
