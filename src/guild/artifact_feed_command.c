#include "guild/artifact_feed_rates.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "sql/sql.h"
#include "world/db.h"
#include "world/difficulty.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace
{
// "24 s", "3.0 min", "2.4 h": the timer an epic buys, at a glance.
void format_duration(double seconds, char *out, size_t size)
{
	if (seconds < 120.0)
		snprintf(out, size, "%.0f s", seconds);
	else if (seconds < 7200.0)
		snprintf(out, size, "%.1f min", seconds / 60.0);
	else
		snprintf(out, size, "%.1f h", seconds / 3600.0);
}

void show_setting_row(P_char ch, const artifact_feed_setting &setting, double per_epic_seconds)
{
	const double value = artifact_feed_setting_value(setting);
	char per_epic[32] = "";
	if (per_epic_seconds >= 0.0)
		format_duration(per_epic_seconds, per_epic, sizeof per_epic);
	char line[MAX_STRING_LENGTH];
	snprintf(line, sizeof line, "  %-11s %-27s %9.3f %10s %10.3f%s\r\n", setting.name,
		 setting.label, value, per_epic, setting.default_value,
		 std::fabs(value - setting.default_value) > 0.0005 ? "  *" : "");
	send_to_char(line, ch);
}

void show_rates(P_char ch)
{
	const double dial = difficulty_multiplier(DIFFICULTY_ARTIFACT_FEEDING);
	const int point = artifact_feed_point_seconds();
	const int thrill = get_property("epic.frag.thrill.duration", 45);
	char line[MAX_STRING_LENGTH];
	send_to_char("&+WArtifact feeding&n\r\n", ch);
	snprintf(line, sizeof line,
		 "  Time fed per epic = %d seconds x the source's rate x the difficulty dial "
		 "(%.2fx),\r\n  and x1.5 within %d seconds of a frag.\r\n",
		 point, dial, thrill);
	send_to_char(line, ch);
	snprintf(line, sizeof line,
		 "  Non-PvP feeds stop %.0f hours ahead; PvP fills the timer to %d days.\r\n\r\n",
		 artifact_feed_setting_value(ARTIFACT_FEED_NONPVP_CEILING_HOURS),
		 ARTIFACT_BLOOD_DAYS);
	send_to_char(line, ch);
	snprintf(line, sizeof line, "  %-11s %-27s %9s %10s %10s\r\n", "Setting", "Source", "Rate",
		 "Per epic", "Proposed");
	send_to_char(line, ch);
	for (size_t index = 0; index < ARTIFACT_FEED_SOURCE_COUNT; ++index)
	{
		const artifact_feed_setting &source = ARTIFACT_FEED_SOURCES[index];
		show_setting_row(ch, source, point * artifact_feed_setting_value(source) * dial);
	}
	send_to_char("\r\n", ch);
	show_setting_row(ch, ARTIFACT_FEED_POINT_SECONDS, -1.0);
	show_setting_row(ch, ARTIFACT_FEED_NONPVP_CEILING_HOURS, -1.0);
	send_to_char("\r\n  * differs from the proposed rate.\r\n"
		     "  artifeed set <setting> <value>   artifeed reset <setting|all>\r\n",
		     ch);
}

bool save_setting(P_char ch, const artifact_feed_setting &setting, double requested,
		  bool announce = true)
{
	// The properties file keeps three decimals; keep memory the same so a reboot changes nothing.
	const double value = std::round(requested * 1000.0) / 1000.0;
	if (!set_and_save_property(setting.property, static_cast<float>(value)))
	{
		send_to_char_f(ch,
			       "Could not save %s to lib/duris.properties; nothing changed.\r\n",
			       setting.property);
		return false;
	}
	wizlog(57, "%s set %s to %.3f", GET_NAME(ch), setting.property, value);
	logit(LOG_WIZ, "%s set %s to %.3f", GET_NAME(ch), setting.property, value);
	sql_log(ch, WIZLOG, "Set %s to %.3f", setting.property, value);
	if (announce)
		send_to_char_f(ch, "%s is now %.3f, saved to lib/duris.properties.\r\n",
			       setting.property, value);
	return true;
}
} // namespace

// artifeed                           show every feeding rate and what an epic buys
// artifeed set <setting> <value>     change one rate and save it (Forger and up)
// artifeed reset <setting|all>       restore the proposed rate and save it (Forger and up)
//
// Every rate is read live, so a change applies to the next epic award.
void do_artifeed(P_char ch, char *argument, int /*cmd*/)
{
	char command[16] = "", name[32] = "", value_text[32] = "";
	if (argument)
		sscanf(argument, " %15s %31s %31s", command, name, value_text);

	if (!*command || !strcasecmp(command, "show"))
	{
		show_rates(ch);
		return;
	}
	if (strcasecmp(command, "set") && strcasecmp(command, "reset"))
	{
		send_to_char(
			"Usage: artifeed [show | set <setting> <value> | reset <setting|all>]\r\n",
			ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Only a Forger or higher can change artifact feeding.\r\n", ch);
		return;
	}

	const bool reset = !strcasecmp(command, "reset");
	if (reset && !strcasecmp(name, "all"))
	{
		// One line for the whole reset, so the table that follows fits on a page.
		for (size_t index = 0; index < ARTIFACT_FEED_SOURCE_COUNT; ++index)
			if (!save_setting(ch, ARTIFACT_FEED_SOURCES[index],
					  ARTIFACT_FEED_SOURCES[index].default_value, false))
				return;
		if (!save_setting(ch, ARTIFACT_FEED_POINT_SECONDS,
				  ARTIFACT_FEED_POINT_SECONDS.default_value, false) ||
		    !save_setting(ch, ARTIFACT_FEED_NONPVP_CEILING_HOURS,
				  ARTIFACT_FEED_NONPVP_CEILING_HOURS.default_value, false))
			return;
		send_to_char("Every feeding rate is back to its proposed value, saved to "
			     "lib/duris.properties.\r\n",
			     ch);
		show_rates(ch);
		return;
	}

	const artifact_feed_setting *setting = artifact_feed_setting_find(name);
	if (!setting)
	{
		send_to_char_f(
			ch, "There is no feeding setting called '%s'; 'artifeed' lists them.\r\n",
			name);
		return;
	}

	double value = setting->default_value;
	if (!reset)
	{
		char *end = nullptr;
		value = strtod(value_text, &end);
		if (!*value_text || !end || *end || !std::isfinite(value) ||
		    value < setting->minimum || value > setting->maximum)
		{
			send_to_char_f(ch, "%s takes a number from %.3f to %.3f.\r\n",
				       setting->name, setting->minimum, setting->maximum);
			return;
		}
	}
	if (save_setting(ch, *setting, value))
		show_rates(ch);
}
