#ifndef DURIS_RAM_COMMAND_H
#define DURIS_RAM_COMMAND_H

#include "persistence/critical_command_completion.h"

// Called on the world thread after the preceding outcome has been published.
// Preparation reads live state only and never publishes or performs storage I/O.
critical_apply_result ram_command_prepare(const critical_command &, void *);

#endif
