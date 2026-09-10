#ifndef DURIS_PASSWORD_ASYNC_H
#define DURIS_PASSWORD_ASYNC_H
#include "account/password_hash.h"
#include <functional>
struct descriptor_data;
/* Continuations and session snapshots live exclusively on the game thread.
 * The callback borrows the result hash for this call only. */
using password_completion = std::function<void(descriptor_data *, int, const char *)>;
bool password_async_start(descriptor_data *d, password_login_job *job, const char *expected_hash,
			  password_completion finish);
bool password_async_pulse(descriptor_data *d);
void password_async_cancel(descriptor_data *d);
#endif
