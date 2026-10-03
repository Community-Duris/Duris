#ifndef DURIS_ACCOUNT_ASYNC_H
#define DURIS_ACCOUNT_ASYNC_H

#include "account/account_load.h"
#include <functional>

struct descriptor_data;
// Continuation and session identity remain on the game thread.
using account_load_completion = std::function<void(descriptor_data *, account_load_outcome)>;
bool account_async_start(descriptor_data *d, account_load_completion finish);
bool account_async_pulse(descriptor_data *d);
void account_async_cancel(descriptor_data *d);

#endif
