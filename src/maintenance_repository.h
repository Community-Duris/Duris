#ifndef MAINTENANCE_REPOSITORY_H
#define MAINTENANCE_REPOSITORY_H

#include "maintenance_scheduler.h"

maintenance_result maintenance_repository_execute(const maintenance_request &request,
						  void *context);

#endif
