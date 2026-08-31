#ifndef MAINTENANCE_SNAPSHOT_H
#define MAINTENANCE_SNAPSHOT_H

#include "persistence/maintenance_scheduler.h"

bool maintenance_prepare_request(maintenance_request &request, void *context);

#endif
