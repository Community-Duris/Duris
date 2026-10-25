#ifndef DURIS_ECONOMIC_STATE_REPOSITORY_H
#define DURIS_ECONOMIC_STATE_REPOSITORY_H

#include <mysql/mysql.h>
#include <string>

// Startup only, after all accepted operations have been replayed, before login
// or command admission. Failure leaves the server closed to economic activity.
bool economic_state_repository_load(MYSQL *connection, const char *flatfile_root,
				    std::string *error);
// Compatibility boundary for legacy artifact maintenance. Caller has drained
// the ordered writer; reconcile its old projection and rehydrate before any
// subsequent RAM decision. This never runs from the command projector.
bool economic_state_repository_refresh_artifacts(MYSQL *connection);

#endif
