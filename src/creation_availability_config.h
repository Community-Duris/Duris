#ifndef CREATION_AVAILABILITY_CONFIG_H
#define CREATION_AVAILABILITY_CONFIG_H

#include <stdbool.h>

void boot_creation_availability_config(void);
bool creation_class_enabled(int class_id);
bool creation_race_enabled(int race_id);

#endif
