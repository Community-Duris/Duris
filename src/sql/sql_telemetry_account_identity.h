#ifndef DURIS_SQL_TELEMETRY_ACCOUNT_IDENTITY_H
#define DURIS_SQL_TELEMETRY_ACCOUNT_IDENTITY_H

#include <cstdint>

struct st_mysql;

/* Account-load preparation on the native account-persistence connection.
 * The caller owns no transaction. A committed result is an opaque token for
 * this environment and season; failures leave token zero and never gate login.
 * Preparation itself is not evidence that a player authenticated or played. */
bool sql_prepare_telemetry_account_token(const char *account_name, std::uint64_t environment_id,
					 std::uint64_t season_id, std::uint64_t *token);

/* Optional preparation on an exclusively borrowed worker connection, after its
 * account snapshot transaction commits. Never touches the main SQL handle.
 * The monotonic deadline bounds admission of each query; failure leaves token
 * zero. The caller retires a connection with uncertain transaction/transport state. */
bool sql_prepare_telemetry_account_token_on(struct st_mysql *connection, const char *account_name,
					    std::uint64_t environment_id, std::uint64_t season_id,
					    std::uint64_t deadline_usec, std::uint64_t *token);

#endif
