#ifndef CREATION_AVAILABILITY_CONFIG_H
#define CREATION_AVAILABILITY_CONFIG_H

#include <stdbool.h>

void boot_creation_availability_config(void);
bool creation_class_enabled(int class_id);
bool creation_class_normally_available(int race_id, int class_id);
bool creation_race_enabled(int race_id);
bool creation_all_races_enabled(void);
bool creation_all_classes_enabled(void);
int  creation_class_align(int race_id, int class_id);

#endif
