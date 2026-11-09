#ifndef DURIS_COLLECTOR_RAM_H
#define DURIS_COLLECTOR_RAM_H

#include "economy/collector_command.h"
#include "economy/collector_custody_boundary.h"
#include "persistence/critical_command_coordinator.h"

critical_apply_result collector_ram_prepare(const critical_command &command);
unsigned int collector_ram_prepare_item(const critical_command &command,
					const item_transfer_payload &payload,
					item_transfer_result *result);
unsigned int
collector_ram_prepare_boundary(const critical_command &command, collector::reason reason,
			       const std::vector<collector_custody_boundary_item> &items,
			       bool *changed, const char *excluded_death = nullptr);
void collector_ram_begin();
uint64_t collector_ram_revision();
bool collector_ram_seal(const critical_command &command, const critical_apply_result &result);
bool collector_ram_can_publish(const critical_command &command);
bool collector_ram_publish(const critical_command &command);
void collector_ram_set_flatfile(bool flatfile);

#endif
