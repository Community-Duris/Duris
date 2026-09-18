#include "artifact/artifact_control.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <cstdlib>
#include <string>

namespace
{
void say(P_char ch, const std::string &text)
{
	send_to_char(text.c_str(), ch);
	send_to_char("\n\r", ch);
}

void show_help(P_char ch)
{
	say(ch, "Artifact control commands:\n\r"
		"  artifact control status\n\r"
		"  artifact control list [name|vnum|classification]\n\r"
		"  artifact control inspect <vnum>\n\r"
		"  artifact control preview <vnum>\n\r"
		"  artifact control setmode <vnum> <player|npc|controlled> <variant>\n\r"
		"  artifact control enable <vnum> <0|1>\n\r"
		"  artifact control publish\n\r"
		"  artifact control discard\n\r"
		"  artifact control reload\n\r"
		"Changes are revisioned and written atomically to the artifact catalog.");
}
}

void artifact_control_command(P_char ch, char *argument)
{
	using namespace artifact_control;
	char command[MAX_INPUT_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char *rest = one_argument(argument ? argument : const_cast<char *>(""), command);
	rest = one_argument(rest, arg1);
	rest = one_argument(rest, arg2);
	rest = one_argument(rest, arg3);

	if (!*command || !strcmp(command, "help") || !strcmp(command, "?"))
	{
		show_help(ch);
		return;
	}
	const status current = control_status();
	if (!current.initialized)
	{
		say(ch, "Artifact control is unavailable: " +
				(current.last_error.empty() ? "catalog is not initialized" :
							      current.last_error));
		return;
	}

	if (is_abbrev(command, "status"))
	{
		const status state = control_status();
		say(ch, "Artifact catalog " + std::to_string(state.active_revision) +
				" hash=" + state.active_hash +
				(state.dirty ? " (draft changes pending)" : " (active)") +
				" path=" + state.source_path);
		return;
	}
	if (is_abbrev(command, "list"))
	{
		const std::string output = control_list(*arg1 ? arg1 : nullptr);
		say(ch, output.empty() ? "No artifacts matched." : output);
		return;
	}
	if (is_abbrev(command, "inspect") || is_abbrev(command, "show"))
	{
		if (!*arg1)
		{
			say(ch, "Syntax: artifact control inspect <vnum>");
			return;
		}
		const std::string output = control_inspect(std::atoi(arg1));
		say(ch, output == "UNKNOWN_ARTIFACT" ? "Unknown artifact vnum." : output);
		return;
	}
	if (is_abbrev(command, "setmode"))
	{
		if (GET_LEVEL(ch) < FORGER || !*arg1 || !*arg2 || !*arg3)
		{
			say(ch,
			    GET_LEVEL(ch) < FORGER ?
				    "You need Forger rank to publish artifact changes." :
				    "Syntax: artifact control setmode <vnum> <player|npc|controlled> <variant>");
			return;
		}
		holder_kind holder;
		if (!parse_holder(arg2, &holder))
		{
			say(ch, "Holder must be player, npc, or controlled.");
			return;
		}
		std::string error;
		if (control_set_variant(std::atoi(arg1), holder, arg3, &error) !=
		    control_result::ok)
		{
			say(ch, "Artifact change rejected: " + error);
			return;
		}
		say(ch, "Artifact mode staged. Use 'artifact control preview " + std::string(arg1) +
				"' then 'artifact control publish' to activate it.");
		return;
	}
	if (is_abbrev(command, "enable"))
	{
		if (GET_LEVEL(ch) < FORGER || !*arg1 || !*arg2)
		{
			say(ch, GET_LEVEL(ch) < FORGER ?
					"You need Forger rank to publish artifact changes." :
					"Syntax: artifact control enable <vnum> <0|1>");
			return;
		}
		const int enabled = std::atoi(arg2);
		if (enabled != 0 && enabled != 1)
		{
			say(ch, "Enable must be exactly 0 or 1.");
			return;
		}
		std::string error;
		if (control_set_enabled(std::atoi(arg1), enabled == 1, &error) !=
		    control_result::ok)
		{
			say(ch, "Artifact change rejected: " + error);
			return;
		}
		say(ch,
		    "Artifact enablement staged. Use 'artifact control publish' to activate it.");
		return;
	}
	if (is_abbrev(command, "preview"))
	{
		if (!*arg1)
		{
			say(ch, "Syntax: artifact control preview <vnum>");
			return;
		}
		const std::string output = control_preview(std::atoi(arg1));
		say(ch, output == "UNKNOWN_ARTIFACT" ? "Unknown artifact vnum." : output);
		return;
	}
	if (is_abbrev(command, "publish"))
	{
		if (GET_LEVEL(ch) < FORGER)
		{
			say(ch, "You need Forger rank to publish artifact changes.");
			return;
		}
		std::string error;
		if (!control_publish(&error))
			say(ch, "Artifact publish rejected: " + error);
		else
			say(ch, "Published artifact catalog revision " +
					std::to_string(control_status().active_revision) + ".");
		return;
	}
	if (is_abbrev(command, "discard"))
	{
		if (GET_LEVEL(ch) < FORGER)
		{
			say(ch, "You need Forger rank to discard artifact changes.");
			return;
		}
		std::string error;
		if (!control_discard(&error))
			say(ch, "Artifact draft discard rejected: " + error);
		else
			say(ch, "Unpublished artifact changes discarded.");
		return;
	}
	if (is_abbrev(command, "reload"))
	{
		if (GET_LEVEL(ch) < FORGER)
		{
			say(ch, "You need Forger rank to reload artifact configuration.");
			return;
		}
		std::string error;
		if (!control_reload(&error))
			say(ch, "Artifact catalog reload rejected: " + error);
		else
			say(ch, "Artifact catalog reloaded at revision " +
					std::to_string(control_status().active_revision) + ".");
		return;
	}
	show_help(ch);
}
