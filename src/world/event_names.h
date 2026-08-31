#ifndef DURIS_EVENT_NAMES_H
#define DURIS_EVENT_NAMES_H

#include <stddef.h>
#include <stdint.h>

struct event_name_load_result
{
	bool opened;
	bool read_error;
	size_t symbols_loaded;
	size_t duplicate_addresses;
	size_t malformed_lines;
	int error_number;
};

typedef void (*event_name_registry_visitor)(const void *func, const char *name, void *context);

event_name_load_result event_name_registry_load(const char *path, uintptr_t base_address);
const char *event_name_registry_lookup(const void *func);
size_t event_name_registry_size();
void event_name_registry_visit(event_name_registry_visitor visitor, void *context);
void event_name_registry_clear();

#endif
