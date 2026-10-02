// Disposable-SQL fixture for the real retained-evidence and web-delete journey.
#include "player/player_death_conflict_repository.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"
#include "sql/sql_pool.h"

#include <mysql/mysql.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
void sql_pool_release(MYSQL *) {}
void sql_pool_discard_connection(MYSQL *) {}
MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
char *sql_escape_string(const char *)
{
	std::abort();
}

namespace
{
constexpr int RECOVERY_PID = 74001;
constexpr int CLEAN_PID = 74002;
constexpr const char *ACCOUNT = "S10Test";
constexpr const char *RECOVERY_NAME = "S10Recovery";
constexpr const char *CLEAN_NAME = "S10Clean";

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}
const char *env(const char *name)
{
	const char *value = std::getenv(name);
	require(value && *value, std::string("missing environment: ") + name);
	return value;
}
void sql(MYSQL *db, const std::string &statement)
{
	require(mysql_real_query(db, statement.data(), statement.size()) == 0,
		"SQL statement failed, code=" + std::to_string(mysql_errno(db)));
}
using rows_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
std::string scalar(MYSQL *db, const std::string &statement)
{
	sql(db, statement);
	rows_ptr rows(mysql_store_result(db), mysql_free_result);
	require(bool(rows) && mysql_num_rows(rows.get()) == 1, "scalar result cardinality");
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	require(row && row[0], "scalar result cell");
	return row[0];
}
MYSQL *connect_db()
{
	const std::string db_name = env("DB_NAME");
	require(std::string(env("TEST_DB_DISPOSABLE")) == "1" &&
			std::string(env("DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY")) == "1" &&
			std::string(env("DB_HOST")) == "127.0.0.1" && db_name.size() == 32 &&
			db_name.starts_with("corpse_journey_test_") &&
			std::all_of(db_name.begin() + 20, db_name.end(), [](unsigned char c)
				    { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }),
		"non-disposable death-recovery selector refused");
	MYSQL *db = mysql_init(nullptr);
	require(db, "mysql_init failed");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect) == 0, "disable reconnect");
	require(mysql_real_connect(db, env("DB_HOST"), env("DB_USER"), env("DB_PASSWD"),
				   db_name.c_str(), std::stoul(env("DB_PORT")), nullptr, 0),
		"database connection failed");
	sql(db, "SET NAMES utf8mb4");
	return db;
}
player_snapshot retained_request()
{
	player_snapshot request = {};
	request.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	request.pid = RECOVERY_PID;
	request.revision = 100;
	request.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	request.save_intent = 4;
	request.room_vnum = 1201;
	request.encoded_size_bound = 8192;
	request.recipes_are_external = true;
	request.status_strings.push_back({ player_status_string_field::name, RECOVERY_NAME });
	request.status_integers.push_back({ player_status_field::level, 50, 0, false });
	for (auto field : { player_status_field::copper, player_status_field::silver,
			    player_status_field::gold, player_status_field::platinum })
		request.status_integers.push_back({ field, 0, 0, false });
	request.death.emplace();
	auto &death = *request.death;
	death.operation_id.bytes[0] = 0xad;
	death.operation_id.bytes[15] = 1;
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 10000;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = RECOVERY_PID;
	corpse.values[CORPSE_SAVEID] = 11000;
	corpse.name = "a retained corpse";
	death.corpse.push_back(corpse);
	player_item_snapshot item = {};
	item.parent_index = 0;
	item.object_uid = 201;
	item.vnum = 501;
	item.name = "a retained blade";
	death.corpse.push_back(item);
	death.custody.push_back({ { 201, 201, 0, 3, 501, item_custody_state::active },
				  { item_owner_type::player, RECOVERY_PID, 0 },
				  5 });
	return request;
}
void seed(MYSQL *db)
{
	require(scalar(db, "SELECT COUNT(*) FROM player_data") == "0", "fresh fixture required");
	require(scalar(db, "SELECT COUNT(*) FROM accounts") == "0",
		"fresh account fixture required");
	sql(db, "INSERT INTO accounts(account_name) VALUES ('S10Test')");
	sql(db,
	    "INSERT INTO player_data(pid,name,account_name,active,level,save_revision,wallet_revision,epic_revision,frag_revision) "
	    "VALUES (74001,'S10Recovery','S10Test',1,50,5,7,1,1),(74002,'S10Clean','S10Test',1,50,5,7,1,1)");
	sql(db, "INSERT INTO account_characters(account_name,char_name,pid) "
		"VALUES ('S10Test','S10Recovery',74001),('S10Test','S10Clean',74002)");
	sql(db, "INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision) "
		"VALUES (74001,0,1),(74002,0,1)");
	sql(db, "INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
		"opening_gold,opening_platinum,opening_revision) "
		"VALUES (74001,0,0,0,0,7),(74002,0,0,0,0,7)");
	sql(db, "INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision) "
		"VALUES (74001,0,1),(74002,0,1)");
	sql(db, "INSERT INTO player_items(id,pid,vnum,obj_uid,name) "
		"VALUES (7101,74001,501,201,'recovery blade'),(7102,74002,501,202,'clean blade')");
	sql(db, "INSERT INTO player_item_affects(item_id,location,modifier) VALUES (7101,7,-40)");
	sql(db, "INSERT INTO player_item_extra_descr(item_id,keyword,description) "
		"VALUES (7101,'fixture-note','retained test detail')");
	sql(db,
	    "INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES (1,74001,5),(1,74002,2)");
	sql(db,
	    "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum,state) "
	    "VALUES (201,201,1,74001,3,501,1),(202,202,1,74002,1,501,1)");
	const auto result = player_death_conflict_retain(db, retained_request());
	require(result.outcome == player_death_conflict_outcome::retained,
		"real retained-case repository rejected fixture; error=" +
			std::to_string(result.error_code));
	require(scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=74001") ==
			"1",
		"retained case is not present in SQL");
	require(scalar(db,
		       "SELECT COUNT(*) FROM item_current_owner WHERE item_uid=201 AND owner_type=1 AND owner_id=74001 AND item_revision=3") ==
			"1",
		"retained fixture custody missing");
	require(scalar(db, "SELECT COUNT(*) FROM player_death_disposition WHERE pid=74001") == "0",
		"retained case unexpectedly advanced to terminal disposition");
}
std::string state_bytes(MYSQL *db)
{
	std::string state;
	for (const char *statement :
	     { "SELECT * FROM accounts WHERE account_name='S10Test'",
	       "SELECT * FROM account_characters WHERE account_name='S10Test' ORDER BY id",
	       "SELECT * FROM player_data WHERE pid IN (74001,74002) ORDER BY pid",
	       "SELECT * FROM epic_balance_baseline WHERE pid IN (74001,74002) ORDER BY pid",
	       "SELECT * FROM currency_wallet_baseline WHERE pid IN (74001,74002) ORDER BY pid",
	       "SELECT * FROM combat_frag_baseline WHERE pid IN (74001,74002) ORDER BY pid",
	       "SELECT * FROM player_items WHERE pid IN (74001,74002) ORDER BY id",
	       "SELECT * FROM player_item_affects WHERE item_id IN (7101,7102) ORDER BY id",
	       "SELECT * FROM player_item_extra_descr WHERE item_id IN (7101,7102) ORDER BY id",
	       "SELECT * FROM item_current_owner WHERE item_uid IN (201,202) ORDER BY item_uid",
	       "SELECT * FROM item_owner_revision WHERE owner_type=1 AND owner_id IN (74001,74002) ORDER BY owner_id",
	       "SELECT * FROM player_death_conflict_evidence WHERE pid=74001 ORDER BY save_revision",
	       "SELECT * FROM player_death_disposition WHERE pid=74001 ORDER BY save_revision" })
	{
		sql(db, statement);
		rows_ptr rows(mysql_store_result(db), mysql_free_result);
		require(bool(rows), "state query failed");
		state.append(statement).push_back('\n');
		while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
		{
			const auto *lengths = mysql_fetch_lengths(rows.get());
			require(lengths, "state row lengths missing");
			for (unsigned int i = 0; i < mysql_num_fields(rows.get()); ++i)
			{
				if (!row[i])
					state.append("NULL;");
				else
				{
					state.append(std::to_string(lengths[i])).push_back(':');
					state.append(row[i], lengths[i]).push_back(';');
				}
			}
			state.push_back('\n');
		}
	}
	return state;
}
void print_hash(MYSQL *db)
{
	const auto state = state_bytes(db);
	std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
	SHA256(reinterpret_cast<const unsigned char *>(state.data()), state.size(), digest.data());
	for (unsigned char byte : digest)
		std::printf("%02x", byte);
	std::printf("\n");
}
} // namespace

int main(int argc, char **argv)
{
	require(argc == 2 && (std::string_view(argv[1]) == "--seed" ||
			      std::string_view(argv[1]) == "--snapshot"),
		"explicit mode required");
	require(mysql_library_init(0, nullptr, nullptr) == 0, "mysql client init");
	MYSQL *db = connect_db();
	if (std::string_view(argv[1]) == "--seed")
	{
		seed(db);
		std::cout
			<< "PASS: seeded an account, two players, exact item custody, and one real retained SQL case\n";
	}
	else
		print_hash(db);
	mysql_close(db);
	mysql_library_end();
	return 0;
}
