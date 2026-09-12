// Run only against an explicitly provisioned disposable pet_state_test schema.
#include "player/player_snapshot_repository.h"
#include "player/player_load_repository.h"
#include "player/pet_restore_state.h"
#include "persistence/persistence_observability.h"
#include <mysql/mysql.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
	const char *host = std::getenv("TEST_DB_HOST");
	const char *password = std::getenv("TEST_DB_PASSWORD");
	assert(host && password);
	MYSQL *db = mysql_init(nullptr);
	assert(mysql_real_connect(db, host, "root", password, "pet_state_test", 3306, nullptr, 0));
	auto sql = [&](const std::string &query)
	{
		if (mysql_real_query(db, query.data(), query.size()))
		{
			std::cerr << mysql_error(db) << '\n';
			std::abort();
		}
	};
	sql("INSERT INTO player_data(pid,name,account_name,level,last_room,base_hit,save_revision) "
	    "VALUES(212212,'Petfixture','Petfixture',56,123,100,1)");
	sql("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,212212,0,1)");
	sql("INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) "
	    "VALUES(212212,212212,1,212212,0,1,100,1)");
	player_snapshot snapshot = {};
	snapshot.pid = 212212;
	snapshot.room_vnum = 123;
	snapshot.components = PLAYER_COMPONENT_PETS;
	snapshot.revision = 1;
	player_pet_snapshot pet = {};
	pet.mob_vnum = 1201;
	pet.room_vnum = 123;
	pet.hit = pet.max_hit = 1234;
	pet.mana = pet.max_mana = 55;
	pet.vitality = pet.max_vitality = 100;
	pet.charm_duration = -1;
	pet_restore_state state;
	state.kind = summoned_pet_kind::undead_first;
	state.name = "skeleton _Petfixture_";
	state.short_description = "the skeleton of a victim";
	state.long_description = "The skeleton of a victim waits here.\r\n";
	state.level = 21;
	state.base_points = { 1234, 55, 100, -25, 17, 19, 0 };
	state.base_stats.fill(95);
	state.charm_expires_at = 2000000000;
	state.death_expires_at = 2000000060;
	assert(pet_restore_state_encode(state, &pet.restore_state));
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = 212212;
	item.vnum = 100;
	item.type = 9;
	item.equipment_slot = 1;
	item.condition = 100;
	item.affects[0] = { 13, 19 };
	pet.items.push_back(item);
	snapshot.pets.push_back(pet);
	for (int cycle = 0; cycle < 3; ++cycle)
	{
		if (cycle == 1)
		{
			snapshot.pets[0].restore_state = "unknown-version'\\opaque";
			snapshot.pets[0].hold_reason = pet_hold_reason::invalid_state;
		}
		sql("START TRANSACTION");
		assert(player_snapshot_repository_write_pets(db, snapshot));
		sql("COMMIT");
		player_load_request request = {};
		request.request_id = cycle + 1;
		request.pid = snapshot.pid;
		request.account_name = "Petfixture";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		auto loaded = player_load_repository_execute(db, request);
		if (loaded.outcome != player_load_outcome::applied)
			std::cerr << "load failed component="
				  << (loaded.failed_component ? loaded.failed_component : "none")
				  << " error=" << loaded.error_code << '\n';
		assert(loaded.outcome == player_load_outcome::applied);
		assert(loaded.snapshot.pets.size() == 1);
		const auto &restored = loaded.snapshot.pets[0];
		assert(restored.restore_state == snapshot.pets[0].restore_state);
		assert(restored.hold_reason == snapshot.pets[0].hold_reason);
		assert(restored.items.size() == 1 && restored.items[0].object_uid == 212212);
		assert(restored.items[0].affects[0] == item.affects[0]);
		assert(loaded.authoritative_item_count == 1);
		snapshot.pets = loaded.snapshot.pets;
	}
	sql("DELETE FROM item_current_owner WHERE item_uid=212212");
	sql("DELETE FROM item_owner_revision WHERE owner_type=1 AND owner_id=212212");
	sql("DELETE FROM player_data WHERE pid=212212");
	mysql_close(db);
	std::cout
		<< "production SQL pet save/load: generated state, opaque held state, equipped UID and repeated replacement passed\n";
}
