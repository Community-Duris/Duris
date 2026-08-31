#ifndef REDIS_DONATION_RUNTIME_H
#define REDIS_DONATION_RUNTIME_H

#include "core/structs.h"

bool redis_donation_runtime_enabled(void);
void redis_donation_runtime_set_enabled(bool enabled);
void event_check_donation_messages(P_char ch, P_char victim, P_obj obj, void *data);

#endif
