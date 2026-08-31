#include "player/player_load_repository.h"
#include "persistence/persistence_observability.h"

#include <mysql/mysql.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <strings.h>

namespace
{
const char *required_env(const char *name)
{
	const char *value = std::getenv(name);
	assert(value && *value);
	return value;
}

void execute_sql(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) != 0)
	{
		std::cerr << "fixture SQL failed: error=" << mysql_errno(connection) << '\n';
		std::abort();
	}
}

player_load_result execute_load(MYSQL *connection, player_load_request request, uint64_t request_id)
{
	request.request_id = request_id;
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	return player_load_repository_execute(connection, request);
}
}

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, required_env("DB_HOST"), required_env("DB_USER"),
		required_env("DB_PASSWD"), required_env("DB_NAME"),
		static_cast<unsigned int>(std::strtoul(required_env("DB_PORT"), nullptr, 10)),
		nullptr, CLIENT_MULTI_STATEMENTS));
	std::string character = required_env("GAME_ACCOUNT_CHARACTER_NAME");
	std::string escaped(character.size() * 2 + 1, '\0');
	escaped.resize(mysql_real_escape_string(connection, escaped.data(), character.data(),
						character.size()));
	const std::string pid_sql = "SELECT pid FROM player_data WHERE name='" + escaped + "'";
	assert(mysql_real_query(connection, pid_sql.data(), pid_sql.size()) == 0);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const int pid = std::atoi(row[0]);
	mysql_free_result(rows);

	const uint64_t now = persistence_observability_now_usec();
	player_load_request request = {
		PLAYER_LOAD_SCHEMA_VERSION,	77, pid, required_env("GAME_ACCOUNT_NAME"),
		now + PLAYER_LOAD_TIMEOUT_USEC, {}
	};
	assert(player_load_request_valid(request, now));
	player_load_result result = player_load_repository_execute(connection, request);
	if (result.outcome != player_load_outcome::applied)
		std::cerr << "outcome=" << static_cast<int>(result.outcome)
			  << " error=" << result.error_code << " mysql=" << mysql_errno(connection)
			  << " queries=" << result.metrics.query_count << " component="
			  << (result.failed_component ? result.failed_component : "none") << '\n';
	assert(result.outcome == player_load_outcome::applied);
	assert(result.request_id == request.request_id && result.pid == pid);
	assert(result.snapshot.pid == pid);
	assert(result.snapshot.components == PLAYER_LOAD_SESSION03_COMPONENTS);
	assert(result.metrics.query_count == 22);
	assert(result.read_components == PLAYER_LOAD_SESSION04_READS);
	assert(result.recent_pvp_deaths.size() <= GAMEPLAY_READ_RECENT_DURABLE_MAX);
	assert(result.completed_epic_zones.size() <= GAMEPLAY_READ_COMPLETED_ZONE_MAX);
	assert(result.metrics.row_count > 0 &&
	       result.metrics.row_count <= PLAYER_SNAPSHOT_MAX_ROWS);
	assert(result.metrics.byte_count > 0 &&
	       result.metrics.byte_count <= PLAYER_SNAPSHOT_MAX_BYTES);
	assert(result.metrics.transaction_usec <= PLAYER_LOAD_TIMEOUT_USEC);
	assert(result.snapshot.status_strings.size() == 7);
	assert(result.snapshot.status_integers.size() == 63);
	assert(strcasecmp(result.account_name.c_str(), required_env("GAME_ACCOUNT_NAME")) == 0);

	const std::string values_sql =
		"SELECT copper,silver,gold,platinum,epics,frags,oldfrags,wallet_revision,"
		"epic_revision,frag_revision FROM player_data WHERE pid=" +
		std::to_string(pid);
	assert(mysql_real_query(connection, values_sql.data(), values_sql.size()) == 0);
	rows = mysql_store_result(connection);
	assert(rows);
	row = mysql_fetch_row(rows);
	assert(row);
	assert(result.domains.wallet[0] == std::strtoull(row[0], nullptr, 10));
	assert(result.domains.wallet[1] == std::strtoull(row[1], nullptr, 10));
	assert(result.domains.wallet[2] == std::strtoull(row[2], nullptr, 10));
	assert(result.domains.wallet[3] == std::strtoull(row[3], nullptr, 10));
	assert(result.domains.epics == std::strtoll(row[4], nullptr, 10));
	assert(result.domains.frags == std::strtoll(row[5], nullptr, 10));
	assert(result.domains.old_frags == std::strtoll(row[6], nullptr, 10));
	assert(result.domains.wallet_revision == std::strtoull(row[7], nullptr, 10));
	assert(result.domains.epic_revision == std::strtoull(row[8], nullptr, 10));
	assert(result.domains.frag_revision == std::strtoull(row[9], nullptr, 10));
	mysql_free_result(rows);

	player_load_request by_name = {};
	by_name.request_id = 78;
	by_name.player_name = character;
	by_name.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result name_result = player_load_repository_execute(connection, by_name);
	assert(name_result.outcome == player_load_outcome::applied);
	assert(name_result.pid == pid && name_result.request_id == by_name.request_id);

	player_load_request wrong_account = request;
	wrong_account.request_id = 79;
	wrong_account.account_name = "definitely-not-the-configured-account";
	wrong_account.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result rejected = player_load_repository_execute(connection, wrong_account);
	assert(rejected.outcome == player_load_outcome::component_failure);

	player_load_request missing = request;
	missing.request_id = 80;
	missing.pid = 2147483647;
	missing.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result absent = player_load_repository_execute(connection, missing);
	assert(absent.outcome == player_load_outcome::not_found);
	if (std::getenv("PLAYER_LOAD_REAL_ONLY"))
	{
		mysql_close(connection);
		std::cout << "configured player-load repository snapshot passed\n";
		return 0;
	}

	// Shadow the touched domains for deterministic, connection-local fixtures.
	// Temporary tables vanish on connection close and cannot alter configured player data.
	for (const char *table :
	     { "player_items", "player_item_affects", "player_item_extra_descr", "player_pets",
	       "player_pet_items", "player_pet_item_affects", "player_pet_item_extra_descr",
	       "item_current_owner", "item_owner_revision", "pkill_event", "pkill_info",
	       "epic_gain", "epic_ledger" })
	{
		const std::string temporary = std::string("fixture_") + table;
		execute_sql(connection, "CREATE TEMPORARY TABLE " + temporary + " LIKE " + table);
		execute_sql(connection, "ALTER TABLE " + temporary + " RENAME TO " + table);
	}
	for (int index = 0; index < 25; ++index)
	{
		const int event_id = 4001 + index;
		execute_sql(connection,
			    "INSERT INTO pkill_event(id,stamp,room_vnum,room_name) VALUES(" +
				    std::to_string(event_id) + ",FROM_UNIXTIME(" +
				    std::to_string(2000000000 - index) + "),1,'fixture')");
		execute_sql(
			connection,
			"INSERT INTO pkill_info(event_id,pid,level,pk_type,equip,inroom) VALUES(" +
				std::to_string(event_id) + "," + std::to_string(pid) +
				",1,'VICTIM','',1)");
	}
	execute_sql(connection, "INSERT INTO epic_gain(pid,time,type,type_id,epics) VALUES(" +
					std::to_string(pid) + ",NOW(),0,10,1),(" +
					std::to_string(pid) + ",NOW(),0,20,1)");
	execute_sql(
		connection,
		"INSERT INTO epic_ledger(operation_id,pid,delta,balance_after,epic_revision,"
		"reason_type,reason_id,source_site) VALUES(UNHEX('00000000000000000000000000000001')," +
			std::to_string(pid) +
			",1,1,1,1,20,1),(UNHEX('00000000000000000000000000000002')," +
			std::to_string(pid) + ",1,2,2,1,30,1)");
	execute_sql(
		connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1," +
			std::to_string(pid) + ",0,7)");
	execute_sql(connection,
		    "INSERT INTO player_items(id,pid,vnum,equip_slot,container_id,quantity,weight,"
		    "cost,timer,extra_flags,value0,value1,value2,value3,value4,value5,value6,"
		    "value7,obj_uid,item_condition) VALUES"
		    "(1001," +
			    std::to_string(pid) +
			    ",100,0,NULL,1,2,3,-1,0,0,0,0,0,0,0,0,0,900001,100),"
			    "(1002," +
			    std::to_string(pid) +
			    ",101,0,1001,1,3,4,-1,0,0,0,0,0,0,0,0,0,900002,99),"
			    "(1003," +
			    std::to_string(pid) +
			    ",102,1,NULL,1,4,5,-1,0,0,0,0,0,0,0,0,0,900003,98)");
	execute_sql(connection,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		    "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES"
		    "(900001,900001,NULL,1," +
			    std::to_string(pid) + ",0,2,100,1),(900002,900001,900001,1," +
			    std::to_string(pid) + ",0,3,101,1),(900003,900003,NULL,1," +
			    std::to_string(pid) + ",0,4,102,1)");
	execute_sql(connection, "INSERT INTO player_item_affects(item_id,location,modifier) VALUES"
				"(1002,1,2),(1002,2,-3)");
	execute_sql(connection,
		    "INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES"
		    "(1002,'SPELLBOOK','[1,7,31]'),(1003,'detail','fixture')");

	player_load_result fixture = execute_load(connection, request, 81);
	assert(fixture.outcome == player_load_outcome::applied);
	assert(fixture.metrics.query_count == 22 && fixture.snapshot.items.size() == 3);
	assert(fixture.recent_pvp_deaths.size() == 20);
	assert(fixture.recent_pvp_deaths.front() == 2000000000 &&
	       fixture.recent_pvp_deaths.back() == 1999999981);
	assert((fixture.completed_epic_zones == std::vector<int32_t>{ 10, 20, 30 }));
	assert(fixture.item_identities.size() == 3 && fixture.item_owner_revision == 7);
	assert(fixture.snapshot.items[1].parent_index == 0);
	assert(fixture.item_identities[1].root_item_uid == 900001);
	assert(fixture.item_identities[1].parent_item_uid == 900001);
	assert(fixture.snapshot.items[1].extra_descriptions.size() == 1);
	assert(fixture.snapshot.items[1].affects[0][0] == 1);
	assert(fixture.stale_item_rows == 0);

	// The saved projection can lag a committed reparent in either direction. Custody
	// is authoritative, so both cases rebuild placement instead of refusing login.
	execute_sql(connection,
		    "UPDATE item_current_owner SET root_item_uid=900002,parent_item_uid=NULL "
		    "WHERE item_uid=900002");
	player_load_result stale_nested_projection = execute_load(connection, request, 93);
	assert(stale_nested_projection.outcome == player_load_outcome::applied);
	assert(stale_nested_projection.repaired_item_rows == 1 &&
	       stale_nested_projection.snapshot.items[1].parent_index ==
		       PLAYER_SNAPSHOT_NO_PARENT &&
	       !stale_nested_projection.item_identities[1].serialized_parent_id);
	execute_sql(connection,
		    "UPDATE item_current_owner SET root_item_uid=900001,parent_item_uid=900001 "
		    "WHERE item_uid=900002");
	execute_sql(connection, "UPDATE player_items SET container_id=NULL WHERE id=1002");
	player_load_result stale_flat_projection = execute_load(connection, request, 94);
	assert(stale_flat_projection.outcome == player_load_outcome::applied);
	assert(stale_flat_projection.repaired_item_rows == 1 &&
	       stale_flat_projection.snapshot.items[1].parent_index == 0 &&
	       stale_flat_projection.item_identities[1].serialized_parent_id == 1001);
	execute_sql(connection, "UPDATE player_items SET container_id=1001 WHERE id=1002");

	// A committed ownership move can outrun the replacement player snapshot. The
	// ownership ledger is authoritative, so the stale payload row is skipped and its
	// metadata cannot make the whole character unloadable.
	execute_sql(
		connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(3,1200,0,1)");
	execute_sql(connection, "UPDATE item_current_owner SET owner_type=3,owner_id=1200,"
				"owner_context_id=0 WHERE item_uid=900003");
	player_load_result stale_payload = execute_load(connection, request, 90);
	assert(stale_payload.outcome == player_load_outcome::applied);
	assert(stale_payload.snapshot.items.size() == 2 && stale_payload.stale_item_rows == 1);
	assert(stale_payload.authoritative_item_count == 2);
	execute_sql(connection,
		    "UPDATE item_current_owner SET owner_type=1,owner_id=" + std::to_string(pid) +
			    ",owner_context_id=0 WHERE item_uid=900003");
	execute_sql(connection,
		    "DELETE FROM item_owner_revision WHERE owner_type=3 AND owner_id=1200 AND "
		    "owner_context_id=0");

	// Losing a container's payload row must not take its contents down with it: item
	// 1002 sits inside 1001, and orphaning 1001 promotes 1002 to the top level.
	execute_sql(connection, "DELETE FROM item_current_owner WHERE item_uid=900001");
	player_load_result promoted = execute_load(connection, request, 92);
	assert(promoted.outcome == player_load_outcome::applied);
	assert(promoted.snapshot.items.size() == 2 && promoted.authoritative_item_count == 2);
	assert(promoted.stale_item_rows == 1 && promoted.promoted_item_rows == 1);
	for (size_t index = 0; index < promoted.item_identities.size(); ++index)
	{
		assert(promoted.snapshot.items[index].parent_index == PLAYER_SNAPSHOT_NO_PARENT);
		assert(!promoted.item_identities[index].parent_item_uid);
		assert(promoted.item_identities[index].root_item_uid ==
		       promoted.item_identities[index].item_uid);
	}
	execute_sql(connection,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		    "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES"
		    "(900001,900001,NULL,1," +
			    std::to_string(pid) + ",0,2,100,1)");

	// Pet payload and metadata share the player owner revision and add three queries,
	// independent of pet and pet-item count.
	execute_sql(connection,
		    "INSERT INTO player_pets(id,owner_pid,mob_vnum,pet_order,hit,max_hit,mana,"
		    "max_mana,vitality,max_vitality,charm_duration,room_vnum) VALUES(3001," +
			    std::to_string(pid) + ",100,0,10,20,3,5,4,6,12," +
			    std::to_string(fixture.snapshot.room_vnum) + ")");
	execute_sql(connection,
		    "INSERT INTO player_pet_items(id,pet_id,vnum,equip_slot,container_id,weight,"
		    "cost,timer,extra_flags,value0,value1,value2,value3,value4,value5,value6,"
		    "value7,obj_uid,item_condition) VALUES"
		    "(3101,3001,100,0,NULL,2,3,-1,0,0,0,0,0,0,0,0,0,910001,100),"
		    "(3102,3001,101,1,NULL,3,4,-1,0,0,0,0,0,0,0,0,0,910002,99)");
	execute_sql(connection,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		    "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES"
		    "(910001,910001,NULL,1," +
			    std::to_string(pid) + ",0,5,100,1),(910002,910002,NULL,1," +
			    std::to_string(pid) + ",0,6,101,1)");
	execute_sql(connection,
		    "INSERT INTO player_pet_item_affects(item_id,location,modifier) VALUES"
		    "(3102,1,2)");
	execute_sql(connection,
		    "INSERT INTO player_pet_item_extra_descr(item_id,keyword,description) VALUES"
		    "(3101,'detail','pet fixture')");
	player_load_result pet_fixture = execute_load(connection, request, 88);
	assert(pet_fixture.outcome == player_load_outcome::applied);
	assert(pet_fixture.metrics.query_count == 22 && pet_fixture.snapshot.pets.size() == 1);
	assert(pet_fixture.pet_identities.size() == 1 &&
	       pet_fixture.pet_identities[0].database_id == 3001);
	assert(pet_fixture.snapshot.pets[0].items.size() == 2 &&
	       pet_fixture.pet_identities[0].item_identities.size() == 2);
	assert(pet_fixture.snapshot.pets[0].items[0].extra_descriptions.size() == 1);
	assert(pet_fixture.snapshot.pets[0].items[1].affects[0][0] == 1);
	assert(pet_fixture.authoritative_item_count == 5);

	// A pet payload row whose custody row was reassigned is skipped, exactly as the
	// character's own inventory rows are.
	execute_sql(
		connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(3,1201,0,1)");
	execute_sql(connection, "UPDATE item_current_owner SET owner_type=3,owner_id=1201,"
				"owner_context_id=0 WHERE item_uid=910002");
	player_load_result pet_reassigned = execute_load(connection, request, 91);
	assert(pet_reassigned.outcome == player_load_outcome::applied);
	assert(pet_reassigned.snapshot.pets.size() == 1 &&
	       pet_reassigned.snapshot.pets[0].items.size() == 1);
	assert(pet_reassigned.stale_item_rows == 1 && pet_reassigned.authoritative_item_count == 4);
	execute_sql(connection,
		    "DELETE FROM item_owner_revision WHERE owner_type=3 AND owner_id=1201 AND "
		    "owner_context_id=0");

	// Pet payload without any custody row at all is one skippable row, not a refusal:
	// an orphan must never lock the owning character out of the game.
	execute_sql(connection, "DELETE FROM item_current_owner WHERE item_uid=910002");
	player_load_result pet_orphan = execute_load(connection, request, 89);
	assert(pet_orphan.outcome == player_load_outcome::applied);
	assert(pet_orphan.snapshot.pets.size() == 1 &&
	       pet_orphan.snapshot.pets[0].items.size() == 1);
	assert(pet_orphan.stale_item_rows == 1 && pet_orphan.missing_payload_rows == 0);
	assert(pet_orphan.authoritative_item_count == 4);
	execute_sql(connection, "DELETE FROM player_pet_item_affects");
	execute_sql(connection, "DELETE FROM player_pet_item_extra_descr");
	execute_sql(connection, "DELETE FROM player_pet_items");
	execute_sql(connection, "DELETE FROM player_pets");
	execute_sql(connection, "DELETE FROM item_current_owner WHERE item_uid>=910000");

	// Active custody whose payload row is gone cannot be rebuilt, but the rest of the
	// inventory still loads; the count is reported instead of refusing the character.
	execute_sql(connection, "DELETE FROM player_items WHERE id=1003");
	player_load_result missing_payload = execute_load(connection, request, 82);
	assert(missing_payload.outcome == player_load_outcome::applied);
	assert(missing_payload.snapshot.items.size() == 2 &&
	       missing_payload.authoritative_item_count == 2);
	assert(missing_payload.missing_payload_rows == 1 && missing_payload.stale_item_rows == 0);
	execute_sql(connection,
		    "INSERT INTO player_items(id,pid,vnum,equip_slot,quantity,weight,cost,timer,"
		    "extra_flags,value0,value1,value2,value3,value4,value5,value6,value7,obj_uid,"
		    "item_condition) VALUES(1003," +
			    std::to_string(pid) + ",102,1,1,4,5,-1,0,0,0,0,0,0,0,0,0,900003,98)");

	// Payload/custody vnum disagreement fails before publication.
	execute_sql(connection, "UPDATE item_current_owner SET vnum=999 WHERE item_uid=900003");
	assert(execute_load(connection, request, 83).outcome ==
	       player_load_outcome::component_failure);
	execute_sql(connection, "UPDATE item_current_owner SET vnum=102 WHERE item_uid=900003");

	// More than four distinct static affects is an explicit limit outcome.
	execute_sql(connection, "INSERT INTO player_item_affects(item_id,location,modifier) VALUES"
				"(1002,3,1),(1002,4,1),(1002,5,1)");
	assert(execute_load(connection, request, 84).outcome ==
	       player_load_outcome::limit_exceeded);
	execute_sql(connection, "DELETE FROM player_item_affects WHERE location>=3");

	// Empty ownership still carries and validates its owner revision.
	execute_sql(connection, "DELETE FROM player_item_extra_descr");
	execute_sql(connection, "DELETE FROM player_item_affects");
	execute_sql(connection, "DELETE FROM player_items");
	execute_sql(connection, "DELETE FROM item_current_owner");
	player_load_result empty = execute_load(connection, request, 85);
	assert(empty.outcome == player_load_outcome::applied && empty.snapshot.items.empty());
	assert(empty.item_owner_revision == 7 && empty.metrics.query_count == 22);
	execute_sql(connection, "DELETE FROM item_owner_revision");
	player_load_result never_owned = execute_load(connection, request, 86);
	assert(never_owned.outcome == player_load_outcome::applied &&
	       never_owned.snapshot.items.empty());
	assert(never_owned.item_owner_revision == 0 && never_owned.metrics.query_count == 22);

	// Serialized payload without any custody row is the orphan that used to lock the
	// character out for good. It is skipped and counted, and the load still applies.
	execute_sql(connection,
		    "INSERT INTO player_items(id,pid,vnum,equip_slot,quantity,obj_uid) VALUES"
		    "(2001," +
			    std::to_string(pid) + ",101,0,1,900101)");
	player_load_result orphan_payload = execute_load(connection, request, 87);
	assert(orphan_payload.outcome == player_load_outcome::applied);
	assert(orphan_payload.snapshot.items.empty() && orphan_payload.item_identities.empty());
	assert(orphan_payload.stale_item_rows == 1 && orphan_payload.missing_payload_rows == 0);
	assert(orphan_payload.authoritative_item_count == 0);

	request.request_id = 0;
	assert(!player_load_request_valid(request, persistence_observability_now_usec()));
	mysql_close(connection);
	std::cout << "consistent player-load repository snapshot passed\n";
}
