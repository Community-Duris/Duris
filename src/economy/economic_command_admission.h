#ifndef DURIS_ECONOMIC_COMMAND_ADMISSION_H
#define DURIS_ECONOMIC_COMMAND_ADMISSION_H

#include "persistence/critical_command.h"

// Stateless schema-2 route validation for admission and durable replay. Only
// the implemented typed SQL bank owner qualifies. Does not grant source entitlement, resolve
// current mappings or activate a domain; repository owners still enforce those.
bool economic_command_admission_supported(const critical_command &) noexcept;

#endif
