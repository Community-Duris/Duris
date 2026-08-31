#ifndef CRITICAL_COMMAND_REPOSITORY_H
#define CRITICAL_COMMAND_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <mysql/mysql.h>

constexpr size_t CRITICAL_COMMAND_RESULT_MAX_BYTES = 4096;
constexpr size_t CRITICAL_OUTBOX_PAYLOAD_MAX_BYTES = 65535;

critical_apply_result critical_command_repository_apply(MYSQL *connection,
							const critical_command &command);
critical_apply_result critical_command_repository_reconcile(MYSQL *connection,
							    const critical_command &command);
critical_apply_result critical_command_repository_apply_from_pool(const critical_command &command,
								  void *context);

#endif
