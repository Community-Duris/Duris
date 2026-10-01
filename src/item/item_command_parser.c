#include "item/item_command_parser.h"

#include "core/prototypes.h"
#include "core/utils.h"

#include <cstdio>
#include <cstring>

namespace
{
bool parse_all_dot(char *first_arg, char *filter)
{
	if (!first_arg || !filter || strn_cmp(first_arg, "all", 3) ||
	    sscanf(first_arg, "all.%1023s", filter) != 1)
		return false;
	snprintf(first_arg, MAX_INPUT_LENGTH, "all");
	return true;
}
} // namespace

bool item_get_command_parse(const char *argument, item_get_command *command)
{
	if (!command)
		return false;
	*command = {};

	if (!argument || strlen(argument) >= MAX_INPUT_LENGTH)
		return true;

	char bounded_argument[MAX_INPUT_LENGTH] = {};
	char first_arg[MAX_INPUT_LENGTH] = {};
	char second_arg[MAX_INPUT_LENGTH] = {};
	snprintf(bounded_argument, sizeof(bounded_argument), "%s", argument);
	argument_interpreter(bounded_argument, first_arg, second_arg);
	snprintf(command->object, sizeof(command->object), "%s", first_arg);
	snprintf(command->container, sizeof(command->container), "%s", second_arg);

	if (!*first_arg)
		return true;

	if (!*second_arg)
	{
		command->alldot = parse_all_dot(command->object, command->filter);
		command->kind = !str_cmp(command->object, "all") ?
					item_get_command_kind::floor_all :
					item_get_command_kind::floor_item;
		return true;
	}

	command->alldot = parse_all_dot(command->object, command->filter);
	if (!str_cmp(command->object, "all"))
		command->kind = !str_cmp(command->container, "all") ?
					item_get_command_kind::all_from_all :
					item_get_command_kind::all_from_container;
	else
		command->kind = !str_cmp(command->container, "all") ?
					item_get_command_kind::item_from_all :
					item_get_command_kind::item_from_container;
	return true;
}
