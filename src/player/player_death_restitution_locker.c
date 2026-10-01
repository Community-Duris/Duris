#include "player/player_death_restitution_locker.h"

#include "account/account.h"
#include "core/defines.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/vnum.obj.h"

#ifndef __NO_MYSQL__
#include "sql/sql.h"
#include "sql/sql_player.h"
#endif

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

bool player_death_restitution_is_locker_bag(P_obj obj)
{
	if (!obj || OBJ_VNUM(obj) != PLAYER_DEATH_RESTITUTION_BAG_VNUM ||
	    !IS_SET(obj->extra_flags, ITEM_TRANSIENT))
		return false;
	for (extra_descr_data *description = obj->ex_description; description;
	     description = description->next)
		if (description->keyword &&
		    !strcmp(description->keyword, PLAYER_DEATH_RESTITUTION_BAG_MARKER))
			return true;
	return false;
}

#ifndef __NO_MYSQL__
static std::string safe_character_name(const char *short_description)
{
	static const char prefix[] = "a restitution lost items bag for ";
	if (!short_description || strncmp(short_description, prefix, sizeof(prefix) - 1))
		return "an earlier character";
	std::string result;
	for (const unsigned char *cursor = reinterpret_cast<const unsigned char *>(
		     short_description + sizeof(prefix) - 1);
	     *cursor && result.size() < 64; ++cursor)
	{
		if (std::isalnum(*cursor) || *cursor == '-' || *cursor == '_')
			result.push_back(static_cast<char>(*cursor));
	}
	return result.empty() ? "an earlier character" : result;
}
#endif

void player_death_restitution_locker_notice(P_char ch)
{
#ifdef __NO_MYSQL__
	(void)ch;
#else
	if (!DB || !ch || !IS_PC(ch))
		return;
	const char *account = get_account_name_safe(ch);
	if (!account || !*account || !strcasecmp(account, "Unknown"))
		return;
	char *escaped_account = sql_escape_string(account);
	if (!escaped_account)
		return;
	char locker_names[2048];
	size_t used = 0;
	for (unsigned int slot = 0; slot < 5; ++slot)
	{
		const int written = snprintf(locker_names + used, sizeof(locker_names) - used,
					     "%sLOWER(CONCAT('account.','%s','.%u.locker'))",
					     slot ? "," : "", escaped_account, slot);
		if (written < 0 || static_cast<size_t>(written) >= sizeof(locker_names) - used)
		{
			free(escaped_account);
			return;
		}
		used += static_cast<size_t>(written);
	}
	free(escaped_account);

	char query[4096];
	const int written = snprintf(
		query, sizeof(query),
		"SELECT DISTINCT li.id,li.short_descr,l.racewar FROM locker_items li "
		"JOIN lockers l ON l.id=li.locker_id "
		"JOIN locker_item_extra_descr ed ON ed.item_id=li.id "
		"WHERE LOWER(l.locker_name) IN (%s) AND li.vnum=%d "
		"AND (li.extra_flags & %lu)<>0 AND BINARY ed.keyword=BINARY '%s' "
		"ORDER BY li.id LIMIT 20",
		locker_names, PLAYER_DEATH_RESTITUTION_BAG_VNUM,
		static_cast<unsigned long>(ITEM_TRANSIENT), PLAYER_DEATH_RESTITUTION_BAG_MARKER);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(query))
		return;
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return;
	if (mysql_num_rows(result) == 0)
	{
		mysql_free_result(result);
		return;
	}

	send_to_char(
		"\r\n&+R=================================================================&n\r\n"
		"&+W*** ITEM RESTITUTION IS WAITING IN YOUR ACCOUNT LOCKER ***&n\r\n"
		"&+YStaff recovered equipment for a deleted character on this account.&n\r\n"
		"&+YLook for the clearly named restitution lost items bag in the locker&n\r\n"
		"&+Yfor that character's race-war side. The bag is TRANSIENT: if you take&n\r\n"
		"&+Yit out and drop it, the bag will dissolve.&n\r\n",
		ch);
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(result)))
	{
		const std::string character = safe_character_name(row[1]);
		const int racewar = row[2] ? atoi(row[2]) : 0;
		send_to_char_f(ch, "&+C  - Restitution bag for %s (account locker side %d)&n\r\n",
			       character.c_str(), racewar);
	}
	send_to_char(
		"&+R=================================================================&n\r\n\r\n",
		ch);
	mysql_free_result(result);
#endif
}
