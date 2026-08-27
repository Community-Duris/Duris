#ifndef SESSION_AUDIT_REPOSITORY_H
#define SESSION_AUDIT_REPOSITORY_H

#include "session_audit_command.h"

struct st_mysql;
typedef struct st_mysql MYSQL;

bool session_audit_repository_execute(MYSQL *connection, const critical_command &command,
				      session_audit_result *result);

#endif
