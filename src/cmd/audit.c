#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "item/economic_accounting_item_reference.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "persistence/persistence_mode.h"
#ifndef __NO_MYSQL__
#include "sql/sql_pool.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr const char *AUDIT_SYNTAX =
	"Syntax: audit item <uid> | audit item <name> | audit help\r\n";

std::string format_operation_id(const critical_operation_id &id)
{
	char hex[33] = {};
	for (size_t i = 0; i < 16; ++i)
	{
		std::snprintf(hex + i * 2, 3, "%02x", id.bytes[i]);
	}
	return std::string(hex);
}

void audit_item(P_char ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Usage: audit item <uid> or audit item <name>\r\n", ch);
		return;
	}

	uint64_t target_uid = 0;
	char *end = nullptr;
	unsigned long long parsed = strtoull(arg, &end, 10);
	if (!*end && parsed > 0)
	{
		target_uid = static_cast<uint64_t>(parsed);
	}
	else
	{
		P_obj obj = get_obj_vis(ch, arg);
		if (!obj)
		{
			send_to_char("No such item visible, and argument is not a numeric UID.\r\n",
				     ch);
			return;
		}
		if (obj->obj_uid == 0)
		{
			send_to_char(
				"That item has no unique object identifier (obj_uid == 0).\r\n",
				ch);
			return;
		}
		target_uid = static_cast<uint64_t>(obj->obj_uid);
	}

	char header[MAX_STRING_LENGTH];
	std::snprintf(header, sizeof(header), "&+Y=== Accounting Audit for Item UID %llu ===&n\r\n",
		      static_cast<unsigned long long>(target_uid));
	send_to_char(header, ch);

	std::vector<economic_accounting_item_reference> history;
	bool queried = false;

#ifndef __NO_MYSQL__
	if (persistence_mode_requires_mysql() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
	{
		MYSQL *conn = sql_pool_acquire();
		if (conn)
		{
			if (economic_accounting_item_reference_find_history(conn, target_uid,
									    &history))
			{
				queried = true;
			}
			sql_pool_release(conn);
		}
	}
#endif

	if (!queried)
	{
		const char *root = persistence_mode_flatfile_root();
		if (!root || !*root)
			root = ".";
		std::string err;
		if (flatfile_item_accounting_reference_find_history(root, target_uid, &history,
								    &err) ==
		    flatfile_item_accounting_status::ok)
		{
			queried = true;
		}
	}

	if (!queried || history.empty())
	{
		send_to_char("No accounting reference records found for that item UID.\r\n", ch);
		return;
	}

	for (size_t i = 0; i < history.size(); ++i)
	{
		const auto &ref = history[i];
		char line[MAX_STRING_LENGTH];
		std::snprintf(line, sizeof(line),
			      "[%zu] op=%s rev=%llu->%llu line=%u event=%u child=%u\r\n", i + 1,
			      format_operation_id(ref.operation_id).c_str(),
			      static_cast<unsigned long long>(ref.before_revision),
			      static_cast<unsigned long long>(ref.after_revision),
			      static_cast<unsigned>(ref.line_index),
			      static_cast<unsigned>(ref.event_index),
			      static_cast<unsigned>(ref.child_index));
		send_to_char(line, ch);
	}
}

} // namespace

void do_audit(P_char ch, char *argument, int /*cmd*/)
{
	if (IS_NPC(ch) || !IS_TRUSTED(ch))
		return;

	char subcmd[MAX_INPUT_LENGTH];
	argument = one_argument(argument, subcmd);

	if (!*subcmd || isname(subcmd, "help"))
	{
		send_to_char(AUDIT_SYNTAX, ch);
		return;
	}

	if (isname(subcmd, "item"))
	{
		audit_item(ch, argument);
		return;
	}

	send_to_char(AUDIT_SYNTAX, ch);
}
