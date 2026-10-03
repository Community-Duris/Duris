// C14 manual SQL harness. Include the production TU so this calls its real
// static sql_save_player_items definition rather than a copied implementation.
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "player/player_snapshot_codec.h"
#include "sql/sql.h"

#include <mysql.h>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "sql/sql_player.c"

MYSQL *DB = nullptr;
P_index obj_index = nullptr;
P_acct account_list = nullptr;
P_obj save_equip[MAX_WEAR] = {};

namespace
{
constexpr unsigned long long UID_BASE = 900000000000000000ULL;
constexpr int ROOT_VNUM = 15001;
constexpr int CHILD_VNUM = 15002;
constexpr int LEGACY_AFFECT_LOCATION = 1;
constexpr int LEGACY_AFFECT_MODIFIER = 7;
constexpr int DYNAMIC_AFFECT_TYPE = 37;
constexpr int DYNAMIC_AFFECT_DATA = 5;

struct saved_item
{
	unsigned long long row_id = 0;
	unsigned long long uid = 0;
	int pid = 0;
	int vnum = 0;
	int equip_slot = 0;
	unsigned long long container_id = 0;
	unsigned long long extra_flags = 0;
	bool has_properties = false;
	uint32_t extra2_flags = 0;
	std::vector<player_item_dynamic_affect_snapshot> dynamic_affects;
};

struct saved_state
{
	std::vector<saved_item> items;
	std::vector<std::pair<int, int>> legacy_affects;
};

void fail(const char *message)
{
	std::cerr << "ASSERTION FAILED: " << message << '\n';
	std::exit(1);
}

const char *required(const char *name)
{
	const char *value = std::getenv(name);
	if (!value || !*value)
		fail("missing fixture database environment");
	return value;
}

void exec_sql(const std::string &statement)
{
	if (!DB || mysql_real_query(DB, statement.data(),
				    static_cast<unsigned long>(statement.size())) != 0)
	{
		std::cerr << "fixture SQL error=" << (DB ? mysql_errno(DB) : 0) << '\n';
		std::exit(2);
	}
	if (MYSQL_RES *result = mysql_store_result(DB))
		mysql_free_result(result);
}

MYSQL_RES *query(const std::string &statement)
{
	if (!DB || mysql_real_query(DB, statement.data(),
				    static_cast<unsigned long>(statement.size())) != 0)
	{
		std::cerr << "fixture query error=" << (DB ? mysql_errno(DB) : 0) << '\n';
		std::exit(2);
	}
	MYSQL_RES *result = mysql_store_result(DB);
	if (!result)
		fail("fixture readback returned no result set");
	return result;
}

unsigned long long number(const char *value)
{
	if (!value)
		return 0;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed = std::strtoull(value, &end, 10);
	if (errno || !end || *end)
		fail("fixture SQL returned a malformed number");
	return parsed;
}

MYSQL *connect_db()
{
	const std::string host = required("DB_HOST");
	const std::string database = required("DB_NAME");
	if (host != "127.0.0.1" || !database.starts_with("c14_sync_item_state_test_"))
		fail("refusing a non-disposable database target");

	MYSQL *connection = mysql_init(nullptr);
	if (!connection || !mysql_real_connect(connection, host.c_str(), required("DB_USER"),
					       required("DB_PASSWD"), database.c_str(),
					       static_cast<unsigned int>(std::strtoul(
						       required("DB_PORT"), nullptr, 10)),
					       nullptr, 0))
	{
		std::cerr
			<< "fixture connection error=" << (connection ? mysql_errno(connection) : 0)
			<< '\n';
		std::exit(2);
	}
	return connection;
}

saved_state read_state(int pid)
{
	saved_state state;
	MYSQL_RES *items =
		query("SELECT id,obj_uid,pid,vnum,equip_slot,COALESCE(container_id,0),extra_flags,"
		      "item_properties,OCTET_LENGTH(item_properties) "
		      "FROM player_items WHERE pid=" +
		      std::to_string(pid) + " ORDER BY obj_uid");
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(items)))
	{
		saved_item item;
		item.row_id = number(row[0]);
		item.uid = number(row[1]);
		item.pid = static_cast<int>(number(row[2]));
		item.vnum = static_cast<int>(number(row[3]));
		item.equip_slot = static_cast<int>(number(row[4]));
		item.container_id = number(row[5]);
		item.extra_flags = number(row[6]);
		bool has_payload = false;
		const player_snapshot_codec_result decoded = player_item_properties_decode_sql_row(
			row[7], row[8], &item.extra2_flags, &item.dynamic_affects, &has_payload);
		if (decoded != player_snapshot_codec_result::ok)
			fail("production item_properties decoder rejected the SQL row");
		item.has_properties = has_payload;
		state.items.push_back(std::move(item));
	}
	mysql_free_result(items);

	MYSQL_RES *affects = query(
		"SELECT ia.location,ia.modifier FROM player_item_affects ia "
		"JOIN player_items pi ON pi.id=ia.item_id WHERE pi.pid=" +
		std::to_string(pid) + " AND pi.obj_uid=" +
		std::to_string(UID_BASE + static_cast<unsigned long long>(pid) * 10ULL + 1ULL) +
		" ORDER BY ia.location,ia.modifier");
	while ((row = mysql_fetch_row(affects)))
		state.legacy_affects.emplace_back(static_cast<int>(number(row[0])),
						  static_cast<int>(number(row[1])));
	mysql_free_result(affects);
	return state;
}

unsigned long long logical_parent_uid(const saved_item &item, const std::vector<saved_item> &items)
{
	if (!item.container_id)
		return 0;
	for (const saved_item &candidate : items)
		if (candidate.row_id == item.container_id)
			return candidate.uid;
	return ULLONG_MAX;
}

bool verify_state(const saved_state &state, int pid, unsigned long long root_uid,
		  unsigned long long child_uid)
{
	if (state.items.size() != 2)
	{
		std::cerr << "ASSERTION FAILED: expected exactly two player_items rows; got "
			  << state.items.size() << '\n';
		return false;
	}

	const saved_item *root = nullptr;
	const saved_item *child = nullptr;
	for (const saved_item &item : state.items)
	{
		if (item.uid == root_uid)
			root = &item;
		if (item.uid == child_uid)
			child = &item;
	}
	if (!root || !child || root->pid != pid || child->pid != pid || root->vnum != ROOT_VNUM ||
	    child->vnum != CHILD_VNUM || root->equip_slot != 0 || child->equip_slot != 0 ||
	    logical_parent_uid(*root, state.items) != 0 ||
	    logical_parent_uid(*child, state.items) != root_uid)
	{
		std::cerr
			<< "ASSERTION FAILED: UID, player custody, or container binding changed\n";
		return false;
	}

	if (!root->has_properties)
	{
		std::cerr << "ASSERTION FAILED: item_properties missing from SQL readback\n";
		return false;
	}
	if ((root->extra_flags & ITEM_LIT) == 0 || root->extra2_flags != ITEM2_BLESS ||
	    root->dynamic_affects.size() != 1 ||
	    root->dynamic_affects[0].type != DYNAMIC_AFFECT_TYPE ||
	    root->dynamic_affects[0].data != DYNAMIC_AFFECT_DATA ||
	    root->dynamic_affects[0].extra2 != ITEM2_BLESS)
	{
		std::cerr << "ASSERTION FAILED: dynamic item properties/flags/affects changed\n";
		return false;
	}
	if (state.legacy_affects.size() != 1 ||
	    state.legacy_affects[0].first != LEGACY_AFFECT_LOCATION ||
	    state.legacy_affects[0].second != LEGACY_AFFECT_MODIFIER)
	{
		std::cerr << "ASSERTION FAILED: player_item_affects row changed\n";
		return false;
	}
	return true;
}

bool same_persisted_state(const saved_state &left, const saved_state &right)
{
	if (left.items.size() != right.items.size() || left.legacy_affects != right.legacy_affects)
		return false;
	for (size_t index = 0; index < left.items.size(); ++index)
	{
		const saved_item &a = left.items[index];
		const saved_item &b = right.items[index];
		if (a.uid != b.uid || a.pid != b.pid || a.vnum != b.vnum ||
		    a.equip_slot != b.equip_slot ||
		    logical_parent_uid(a, left.items) != logical_parent_uid(b, right.items) ||
		    a.extra_flags != b.extra_flags || a.has_properties != b.has_properties ||
		    a.extra2_flags != b.extra2_flags ||
		    a.dynamic_affects.size() != b.dynamic_affects.size())
			return false;
		for (size_t affect = 0; affect < a.dynamic_affects.size(); ++affect)
			if (a.dynamic_affects[affect].type != b.dynamic_affects[affect].type ||
			    a.dynamic_affects[affect].data != b.dynamic_affects[affect].data ||
			    a.dynamic_affects[affect].extra2 != b.dynamic_affects[affect].extra2)
				return false;
	}
	return true;
}
}

// Small test adapters for the SQL execution/transaction services referenced by
// the production TU. The saver, its item SQL, and its transaction boundaries
// remain the included production definitions.
MYSQL_RES *db_query_at(struct persistence_query_site, const char *format, ...)
{
	if (!DB || !format)
		return nullptr;
	char statement[2048];
	va_list arguments;
	va_start(arguments, format);
	const int written = std::vsnprintf(statement, sizeof(statement), format, arguments);
	va_end(arguments);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(statement) ||
	    mysql_real_query(DB, statement, static_cast<unsigned long>(written)) != 0)
		return nullptr;
	return mysql_store_result(DB);
}

bool sql_trace_exec_at(struct persistence_query_site, const char *, const char *statement,
		       size_t length, bool, bool)
{
	return DB && mysql_real_query(DB, statement, static_cast<unsigned long>(length)) == 0;
}

void sql_clear_results() {}

void logit(const char *, const char *, ...) {}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::fputs("ASSERTION FAILED: production saver reached corruption panic\n", stderr);
	std::abort();
}

P_obj read_object(int, int)
{
	return nullptr;
}

void extract_obj(P_obj, int) {}

int main(int argc, char **argv)
{
	const bool sabotage = argc == 2 &&
			      std::string(argv[1]) == "--sabotage-drop-item-properties";
	if (argc > 2 || (argc == 2 && !sabotage))
		fail("unknown fixture argument");

	DB = connect_db();
	const std::string player_name = sabotage ? "C14SyncSabotage" : "C14SyncRoundtrip";
	exec_sql("INSERT INTO player_data(name,account_name,save_revision) VALUES('" + player_name +
		 "','c14-sync-fixture',1)");
	const int pid = static_cast<int>(mysql_insert_id(DB));
	if (pid <= 0)
		fail("disposable player fixture insert failed");

	static struct index_data test_obj_index[3] = {};
	test_obj_index[1].virtual_number = ROOT_VNUM;
	test_obj_index[2].virtual_number = CHILD_VNUM;
	obj_index = test_obj_index;

	char_data character{};
	pc_only_data pc{};
	pc.pid = pid;
	character.only.pc = &pc;
	character.runtime_flags = CHAR_RFLAG_DIRTY_INVENTORY;

	obj_data root{};
	obj_data child{};
	obj_affect dynamic_affect{};
	root.obj_uid = static_cast<unsigned long>(
		UID_BASE + static_cast<unsigned long long>(pid) * 10ULL + 1ULL);
	child.obj_uid = static_cast<unsigned long>(
		UID_BASE + static_cast<unsigned long long>(pid) * 10ULL + 2ULL);
	root.R_num = 1;
	child.R_num = 2;
	root.type = ITEM_WEAPON;
	child.type = ITEM_OTHER;
	root.weight = 2;
	child.weight = 1;
	root.condition = child.condition = 100;
	root.extra_flags = ITEM_LIT;
	root.extra2_flags = ITEM2_BLESS;
	root.affected[0].location = LEGACY_AFFECT_LOCATION;
	root.affected[0].modifier = LEGACY_AFFECT_MODIFIER;
	dynamic_affect.type = DYNAMIC_AFFECT_TYPE;
	dynamic_affect.data = DYNAMIC_AFFECT_DATA;
	dynamic_affect.extra2 = ITEM2_BLESS;
	root.affects = &dynamic_affect;
	root.contains = &child;
	character.carrying = &root;

	const unsigned long long root_uid = root.obj_uid;
	const unsigned long long child_uid = child.obj_uid;
	if (!sql_save_player_items(&character))
		fail("real synchronous sql_save_player_items first save failed");
	if (root.obj_uid != root_uid || child.obj_uid != child_uid)
		fail("first synchronous save changed an original object UID");

	if (sabotage)
	{
		exec_sql("UPDATE player_items SET item_properties=NULL WHERE pid=" +
			 std::to_string(pid) + " AND obj_uid=" + std::to_string(root_uid));
		if (verify_state(read_state(pid), pid, root_uid, child_uid))
			fail("sabotage was not detected: item_properties loss went unnoticed");
		mysql_close(DB);
		DB = nullptr;
		return 1;
	}

	const saved_state first = read_state(pid);
	if (!verify_state(first, pid, root_uid, child_uid))
		return 1;
	const int root_db_id_after_first = root.db_item_id;

	// The same dirty full-save boundary must overwrite, not duplicate, the rows.
	if (!sql_save_player_items(&character))
		fail("real synchronous sql_save_player_items repeat save failed");
	const saved_state second = read_state(pid);
	if (!verify_state(second, pid, root_uid, child_uid) || !same_persisted_state(first, second))
		fail("repeat synchronous save changed item properties, UID, or player custody");
	if (root.obj_uid != root_uid || child.obj_uid != child_uid || root.db_item_id <= 0 ||
	    root.db_item_id == root_db_id_after_first)
		fail("repeat save changed an object UID or did not replace the SQL row");

	std::cout << "CASE sync_save_readback items=" << second.items.size()
		  << " root_uid=stable child_uid=stable player_pid=stable"
		  << " parent_binding=stable item_lit=yes item2_bless=yes"
		  << " dynamic_affects=1 legacy_affects=1\n";
	std::cout << "[PASS] C14 synchronous item dynamic-state save/readback/repeat\n";
	mysql_close(DB);
	DB = nullptr;
	return 0;
}
