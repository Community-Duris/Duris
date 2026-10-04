#ifndef DURIS_ECONOMIC_COMMAND_ADMISSION_H
#define DURIS_ECONOMIC_COMMAND_ADMISSION_H

#include "persistence/critical_command.h"

// Stateless schema-2 route validation for admission and durable replay. SQL
// supports bank, coin, item and singleton collector purchase roots; flatfile
// supports bank and item roots. Other collector actions remain unregistered.
// Repository owners still enforce source entitlement and current authority.
bool economic_command_admission_supported(const critical_command &) noexcept;
bool economic_flatfile_command_admission_supported(const critical_command &) noexcept;

#endif
