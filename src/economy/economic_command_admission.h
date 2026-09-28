#ifndef DURIS_ECONOMIC_COMMAND_ADMISSION_H
#define DURIS_ECONOMIC_COMMAND_ADMISSION_H

#include "persistence/critical_command.h"

// Stateless schema-2 route validation for admission and durable replay. SQL
// supports bank, coin and item roots; flatfile supports bank and item roots.
// Repository owners still enforce source entitlement and current authority.
bool economic_command_admission_supported(const critical_command &) noexcept;
bool economic_flatfile_command_admission_supported(const critical_command &) noexcept;

#endif
