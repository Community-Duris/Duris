#ifndef ITEM_COMMAND_PARSER_H
#define ITEM_COMMAND_PARSER_H

#include "core/structs.h"

#include <cstdint>

enum class item_get_command_kind : uint8_t
{
	invalid = 0,
	floor_all = 1,
	floor_item = 2,
	all_from_all = 3,
	all_from_container = 4,
	item_from_all = 5,
	item_from_container = 6,
};

struct item_get_command
{
	item_get_command_kind kind = item_get_command_kind::invalid;
	bool alldot = false;
	char object[MAX_INPUT_LENGTH] = {};
	char container[MAX_INPUT_LENGTH] = {};
	char filter[MAX_INPUT_LENGTH] = {};
};

/*
 * Parse the GET grammar into bounded command data.  An invalid or empty
 * command is represented by kind=invalid; false is reserved for a null
 * output object so callers cannot accidentally consume partial results.
 */
bool item_get_command_parse(const char *argument, item_get_command *command);

#endif
