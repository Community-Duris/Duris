// A death whose corpse handoff the ledger refused is only durable if the record
// commits with the death itself. This drives player_snapshot_repository_apply()
// against a real server: the player is left empty-handed, and the corpse
// identity, the wallet a rejected conversion never took, the refused item
// payload and the disputed custody rows are all still there afterwards.
#include "player/player_snapshot_repository.h"
#include "player/player_snapshot_codec.h"
#include "classes/necromancy.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include "sql/sql_pool.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <mysql/mysql.h>
#include <string>

// player_snapshot_repository.c also offers a pooled entry point this harness
// does not exercise; the pool itself belongs to the running server.
MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
void sql_pool_release(MYSQL *)
{
}
MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

namespace
{
constexpr int PROBE_PID = 1;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

std::string environment(const char *name, const char *fallback)
{
	const char *value = getenv(name);
	return value && *value ? value : fallback;
}

player_snapshot make_death(player_revision_t revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = PROBE_PID;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4; // RENT_DEATH
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 50, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::silver, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::gold, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::platinum, 0, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Probe" });
	snapshot.recipes_are_external = true;

	snapshot.death.emplace();
	player_death_snapshot &death = *snapshot.death;
	death.operation_id.bytes.fill(0);
	death.operation_id.bytes[0] = 0xa5;
	death.operation_id.bytes[15] = 0x5a;
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	death.wallet_before = { 11, 12, 13, 14 };
	death.wallet_pile_uid = 202;

	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 200;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = PROBE_PID;
	corpse.values[CORPSE_SAVEID] = 9001;
	death.corpse.push_back(corpse);

	player_item_snapshot refused = {};
	refused.parent_index = 0;
	refused.object_uid = 201;
	refused.vnum = 501;
	death.corpse.push_back(refused);

	player_item_snapshot wallet = {};
	wallet.parent_index = 0;
	wallet.object_uid = death.wallet_pile_uid;
	wallet.vnum = VOBJ_COINS;
	wallet.type = ITEM_MONEY;
	for (size_t denomination = 0; denomination < death.wallet_before.size(); ++denomination)
		wallet.values[denomination] = death.wallet_before[denomination];
	death.corpse.push_back(wallet);

	death.custody.push_back({ { 201, 201, 0, 3, 501, item_custody_state::active },
				  { item_owner_type::player, PROBE_PID, 0 },
				  5 });
	death.custody.push_back({ { death.wallet_pile_uid, death.wallet_pile_uid, 0,
				    ITEM_TRANSFER_ABSENT_REVISION, VOBJ_COINS,
				    item_custody_state::absent },
				  {},
				  0 });
	return snapshot;
}

std::string scalar(MYSQL *connection, const std::string &sql)
{
	require(!mysql_real_query(connection, sql.data(), sql.size()),
		std::string("query failed: ") + sql + ": " + mysql_error(connection));
	MYSQL_RES *result = mysql_store_result(connection);
	require(result != nullptr, "no result set for: " + sql);
	MYSQL_ROW row = mysql_fetch_row(result);
	const std::string value = row && row[0] ? row[0] : "";
	mysql_free_result(result);
	return value;
}

void execute(MYSQL *connection, const std::string &sql)
{
	require(!mysql_real_query(connection, sql.data(), sql.size()),
		std::string("statement failed: ") + sql + ": " + mysql_error(connection));
}
} // namespace

int main()
{
	require(!mysql_library_init(0, nullptr, nullptr), "could not initialize the MySQL client");
	MYSQL *connection = mysql_init(nullptr);
	require(connection != nullptr, "could not allocate a MySQL connection");
	const std::string host = environment("DB_HOST", "127.0.0.1");
	const std::string user = environment("DB_USER", "root");
	const std::string password = environment("DB_PASSWD", "");
	const std::string database = environment("DB_NAME", "death_disposition_test");
	const unsigned int port =
		static_cast<unsigned int>(std::strtoul(environment("DB_PORT", "3306").c_str(),
						       nullptr, 10));
	require(mysql_real_connect(connection, host.c_str(), user.c_str(), password.c_str(),
				   database.c_str(), port, nullptr, 0) != nullptr,
		std::string("could not connect: ") + mysql_error(connection));
	execute(connection, "SET SESSION sql_mode='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,"
			    "NO_ENGINE_SUBSTITUTION'");

	// The character still holds the refused payload from an ordinary earlier save.
	execute(connection, "DELETE FROM player_items WHERE pid=1");
	execute(connection,
		"INSERT INTO player_items (pid,vnum,equip_slot,container_id,quantity,item_type,"
		"obj_uid) VALUES (1,501,0,NULL,1,0,201)");

	const player_snapshot death = make_death(5);
	player_save_apply_result applied = player_snapshot_repository_apply(connection, death);
	require(applied.outcome == player_save_apply_outcome::applied &&
			applied.durable_revision == 5,
		"the death disposition was refused by the MariaDB backend: error=" +
			std::to_string(applied.error_code) + " outcome=" +
			std::to_string(static_cast<unsigned>(applied.outcome)));

	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE pid=1") == "0",
		"the death left assets in the player's active inventory");
	require(scalar(connection, "SELECT save_revision FROM player_data WHERE pid=1") == "5",
		"the death did not advance the durable player revision");
	require(scalar(connection,
		       "SELECT CONCAT_WS(':',LOWER(HEX(operation_id)),corpse_item_uid,"
		       "corpse_room_vnum,wallet_revision,wallet_copper,wallet_silver,wallet_gold,"
		       "wallet_platinum,wallet_pile_uid,LENGTH(payload)>0) FROM "
		       "player_death_disposition WHERE pid=1 AND save_revision=5") ==
			"a500000000000000000000000000005a:200:1201:7:11:12:13:14:202:1",
		"the death disposition lost corpse identity, wallet or payload");
	require(scalar(connection,
		       "SELECT GROUP_CONCAT(CONCAT_WS(':',item_uid,root_item_uid,item_revision,"
		       "vnum,state,owner_type,owner_id,owner_revision) ORDER BY item_uid) FROM "
		       "player_death_custody WHERE pid=1 AND save_revision=5") ==
			"201:201:3:501:1:1:1:5,202:202:18446744073709551615:3:0:0:0:0",
		"the death disposition lost its disputed custody evidence");

	// The record decodes back to the same corpse topology, UIDs and wallet.
	MYSQL_RES *payload_rows = nullptr;
	execute(connection, "SELECT payload FROM player_death_disposition WHERE pid=1 AND "
			    "save_revision=5");
	payload_rows = mysql_store_result(connection);
	require(payload_rows != nullptr, "could not read the stored death payload");
	MYSQL_ROW payload_row = mysql_fetch_row(payload_rows);
	const unsigned long *lengths = mysql_fetch_lengths(payload_rows);
	require(payload_row && payload_row[0] && lengths, "the stored death payload was empty");
	player_snapshot decoded = {};
	require(player_snapshot_decode(reinterpret_cast<const uint8_t *>(payload_row[0]),
				       lengths[0],
				       &decoded) == player_snapshot_codec_result::ok &&
			decoded.death.has_value(),
		"the stored death payload did not decode");
	mysql_free_result(payload_rows);
	require(decoded.death->corpse.size() == 3 && decoded.death->corpse[0].object_uid == 200 &&
			decoded.death->corpse[0].values[CORPSE_SAVEID] == 9001 &&
			decoded.death->corpse[1].object_uid == 201 &&
			decoded.death->corpse[2].object_uid == 202 &&
			decoded.death->wallet_before == std::array<int32_t, 4>{ 11, 12, 13, 14 } &&
			decoded.death->custody.size() == 2 && decoded.items.empty(),
		"the stored death payload lost the refused corpse contents");

	// Replay must not repeat the death, and must not duplicate the record.
	applied = player_snapshot_repository_apply(connection, death);
	require(applied.outcome == player_save_apply_outcome::already_applied &&
			applied.durable_revision == 5,
		"replaying the death did not report it as already applied");
	require(scalar(connection, "SELECT COUNT(*) FROM player_death_disposition WHERE pid=1") ==
			"1",
		"replaying the death duplicated its disposition");

	// An ordinary save on a later revision leaves the record standing.
	player_snapshot ordinary = make_death(6);
	ordinary.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	ordinary.save_intent = 1;
	ordinary.death.reset();
	require(player_snapshot_repository_apply(connection, ordinary).outcome ==
			player_save_apply_outcome::applied,
		"an ordinary save after the death was refused");
	require(scalar(connection, "SELECT COUNT(*) FROM player_death_disposition WHERE pid=1") ==
			"1",
		"an ordinary save discarded the death disposition");

	// A death record the codec cannot accept must never reach the tables.
	player_snapshot malformed = make_death(7);
	malformed.death->corpse.clear();
	require(player_snapshot_repository_apply(connection, malformed).outcome ==
			player_save_apply_outcome::terminal_failure,
		"a death record without its corpse was accepted");
	require(scalar(connection, "SELECT save_revision FROM player_data WHERE pid=1") == "6",
		"a refused death record still advanced the durable player revision");

	mysql_close(connection);
	mysql_library_end();
	std::cout << "player death disposition schema and apply passed\n";
	return 0;
}
