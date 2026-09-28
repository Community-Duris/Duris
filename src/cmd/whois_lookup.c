/*
 * Staff and login IP history lookup.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "sql/sql.h"

#include <string.h>

void whois_ip(P_char ch, char *ip_address)
{
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row;
	P_char targ;

	if (!(res = db_query(
		      "SELECT player_name FROM log_entries WHERE ip_address LIKE '%s' GROUP BY player_name ORDER BY player_name",
		      escape_str(ip_address).c_str())))
	{
		send_to_char_f(ch, "Could not find ip_address '%s' in database.\n", ip_address);
		return;
	}
	if (!(row = mysql_fetch_row(res)))
	{
		send_to_char_f(ch, "Could not find any names matching ip address '%s'.\n",
			       ip_address);
		mysql_free_result(res);
		return;
	}
	send_to_char("&+YNames:&N ", ch);
	if ((targ = get_char_online(row[0])) != NULL)
	{
		if (targ->desc != NULL)
		{
			send_to_char_f(ch, "&+C%s&n", row[0]);
		}
		else
		{
			send_to_char_f(ch, "&+B%s&n", row[0]);
		}
	}
	else
	{
		// If they're not in game per above check, but have a desc, then they're at menu somewhere.
		if (get_descriptor_from_name(row[0]))
		{
			send_to_char_f(ch, "&+y%s&n", row[0]);
		}
		else
		{
			send_to_char_f(ch, "%s", row[0]);
		}
	}
	while ((row = mysql_fetch_row(res)))
	{
		if ((targ = get_char_online(row[0])) != NULL)
		{
			if (targ->desc != NULL)
			{
				send_to_char_f(ch, ", &+C%s&n", row[0]);
			}
			else
			{
				send_to_char_f(ch, ", &+B%s&n", row[0]);
			}
		}
		else
		{
			if (get_descriptor_from_name(row[0]))
			{
				send_to_char_f(ch, ", &+y%s&n", row[0]);
			}
			else
			{
				send_to_char_f(ch, ", %s", row[0]);
			}
		}
	}
	mysql_free_result(res);
	send_to_char(".\n", ch);
#else
	(void)ip_address;
	send_to_char("This command requires MySQL support which is not compiled in.\n", ch);
#endif
}

void do_whois(P_char ch, char *arg, int /*cmd*/)
{
	char ip_address[MAX_STRING_LENGTH];
	char name[MAX_INPUT_LENGTH];
#ifndef __NO_MYSQL__
	int pid;
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	arg = one_argument(arg, name);

	if (*name == '\0' || !strcmp(name, "?") || !strcmp(name, "help"))
	{
		send_to_char(
			"&+YSyntax: &+wwhois <player_name>|ip <ip_address>&n\n"
			"Where &+w<player_name>&n is the name of the player to look up,\n"
			"Or &+w[ip_address]&n is the ip address to look up (use % as a wildcard).\n",
			ch);
		send_to_char(
			"i.e. &+wwhois Lohrr&n or &+wwhois ip 173.224.193.243&n or &+wwhois ip 173.224.%.%&n.\n",
			ch);
		send_to_char(
			"Those in &+CCyan&n are online and connected, and those in &+BBlue&n are linkdead,"
			" and those in &+yBrown&n are at the menu.\n",
			ch);
		return;
	}
	if (!strcmp(name, "ip"))
	{
		arg = one_argument(arg, ip_address);
		if (*ip_address == '\0')
		{
			send_to_char("Please enter a valid ip address.\n", ch);
			return;
		}
	}
	else
	{
#ifndef __NO_MYSQL__
		if ((pid = get_player_pid_from_name(name)) < 1)
		{
			send_to_char_f(ch, "Name '%s' not found.\n", name);
			return;
		}
		if ((res = db_query(
			     "SELECT ip_address FROM log_entries WHERE pid=%d AND ip_address!=\"\" GROUP BY ip_address ORDER BY date DESC",
			     pid)) != NULL)
		{
			CAP(name);
			send_to_char_f(ch, "&=LWIP Addresses used by %s:&N\n", name);
			while ((row = mysql_fetch_row(res)))
			{
				send_to_char_f(ch, "%s, ", row[0]);
			}
			mysql_free_result(res);
			send_to_char("\n\n", ch);
		}
		if (!(res = db_query("SELECT last_ip FROM ip_info WHERE pid = %d", pid)))
		{
			send_to_char_f(ch, "Could not find pid %d in database!\n", pid);
			return;
		}
		if (!(row = mysql_fetch_row(res)))
		{
			send_to_char_f(ch, "Could not find last_ip in database (pid = %d)!\n", pid);
			mysql_free_result(res);
			return;
		}
		strcpy(ip_address, row[0]);
		mysql_free_result(res);
#else
		send_to_char("This command requires MySQL support which is not compiled in.\n", ch);
		return;
#endif
	}

#ifndef __NO_MYSQL__
	send_to_char_f(ch, "&=LWIP Address: '%s'&N\n", ip_address);

	whois_ip(ch, ip_address);
#endif
}
