#include "core/defines.h"
#include "economy/collector_codec.h"
#include "economy/collector_policy.h"
#include "economy/currency_command.h"
#include "item/item_transfer_command.h"
#include "persistence/corpse_lifecycle_command.h"
#include "persistence/critical_command_repository.h"

#include <mysql.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

namespace
{
MYSQL *database = nullptr;
constexpr uint32_t OWNER_PID = 2147000601U;
constexpr const char *OWNER_NAME = "CorpseHarness";
constexpr uint64_t INITIAL_ITEM_REVISION = 5;

struct corpse_fixture
{
	uint32_t corpse_id = 0;
	uint32_t root_row_id = 0;
	uint32_t child_row_id = 0;
	uint32_t save_id = 0;
	int32_t room_vnum = 0;
	int32_t root_vnum = 0;
	int32_t child_vnum = 0;
	uint64_t root_uid = 0;
	uint64_t child_uid = 0;
	uint64_t owner_id = 0;
	uint64_t source_owner_revision = 0;
	bool artifact = false;
	bool skipped_children = false;
};

struct applied_corpse
{
	critical_apply_result raw = {};
	corpse_lifecycle_result result = {};
};

void execute(const std::string &sql)
{
	if (mysql_real_query(database, sql.data(), sql.size()) != 0)
		fprintf(stderr, "corpse lifecycle SQL failed: %u %s\n%s\n", mysql_errno(database),
			mysql_error(database), sql.c_str());
	assert(mysql_errno(database) == 0);
}

uint64_t scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *rows = mysql_store_result(database);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const uint64_t value = strtoull(row[0], nullptr, 10);
	mysql_free_result(rows);
	return value;
}

std::string text(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *rows = mysql_store_result(database);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const std::string value = row[0];
	mysql_free_result(rows);
	return value;
}

std::string hex_bytes(const uint8_t *bytes, size_t size)
{
	static constexpr char HEX[] = "0123456789abcdef";
	std::string value(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		value[index * 2] = HEX[bytes[index] >> 4];
		value[index * 2 + 1] = HEX[bytes[index] & 0x0f];
	}
	return value;
}

std::string operation_hex(const critical_operation_id &operation)
{
	return hex_bytes(operation.bytes.data(), operation.bytes.size());
}

critical_operation_id operation()
{
	critical_operation_id value = {};
	assert(critical_operation_id_generate(&value));
	return value;
}

uint64_t owner_revision(const item_owner_identity &owner)
{
	execute("INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,"
		"revision) VALUES(" +
		std::to_string(static_cast<unsigned int>(owner.type)) + "," +
		std::to_string(owner.id) + "," + std::to_string(owner.context_id) + ",0)");
	return scalar("SELECT revision FROM item_owner_revision WHERE owner_type=" +
		      std::to_string(static_cast<unsigned int>(owner.type)) +
		      " AND owner_id=" + std::to_string(owner.id) +
		      " AND owner_context_id=" + std::to_string(owner.context_id));
}

void seed_owner(const item_owner_identity &owner, uint64_t revision)
{
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(" +
		std::to_string(static_cast<unsigned int>(owner.type)) + "," +
		std::to_string(owner.id) + "," + std::to_string(owner.context_id) + "," +
		std::to_string(revision) + ")");
}

void seed_authority(uint64_t uid, uint64_t root_uid, uint64_t parent_uid, int32_t vnum,
		    const item_owner_identity &owner, uint64_t revision)
{
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(uid) + "," + std::to_string(root_uid) + "," +
		(parent_uid ? std::to_string(parent_uid) : "NULL") + "," +
		std::to_string(static_cast<unsigned int>(owner.type)) + "," +
		std::to_string(owner.id) + "," + std::to_string(owner.context_id) + "," +
		std::to_string(revision) + "," + std::to_string(vnum) + ",1)");
}

void seed_artifact(const corpse_fixture &fixture)
{
	execute("INSERT INTO artifacts(vnum,owned,location,locType) VALUES(" +
		std::to_string(fixture.child_vnum) + ",'Y'," + std::to_string(OWNER_PID) + ",5)");
	execute("INSERT INTO artifacts_mortal(vnum,owned,location,locType) VALUES(" +
		std::to_string(fixture.child_vnum) + ",'Y'," + std::to_string(OWNER_PID) + ",5)");
	execute("INSERT INTO artifact_bind(vnum,owner_pid,timer) VALUES(" +
		std::to_string(fixture.child_vnum) + ",77,1700000000)");
	execute("INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,"
		"artifact_type,bind_owner_pid,bind_timer_epoch,item_uid,item_revision,revision) "
		"VALUES(" +
		std::to_string(fixture.child_vnum) + ",1,5," + std::to_string(OWNER_PID) +
		",0,0,77,1700000000," + std::to_string(fixture.child_uid) + "," +
		std::to_string(INITIAL_ITEM_REVISION) + ",9)");
}

corpse_fixture seed_corpse(uint32_t save_id, int32_t room_vnum, int32_t root_vnum,
			   uint64_t root_uid, uint64_t child_uid, uint64_t source_owner_revision,
			   bool artifact, bool skipped_children)
{
	corpse_fixture fixture;
	fixture.save_id = save_id;
	fixture.room_vnum = room_vnum;
	fixture.root_vnum = root_vnum;
	fixture.child_vnum = root_vnum + 1;
	fixture.root_uid = root_uid;
	fixture.child_uid = child_uid;
	fixture.owner_id = item_corpse_owner_id(OWNER_PID, save_id);
	fixture.source_owner_revision = source_owner_revision;
	fixture.artifact = artifact;
	fixture.skipped_children = skipped_children;

	execute("INSERT INTO corpses(player_name,save_id,corpse_revision,room_vnum,short_descr,"
		"description,name,weight,value3) VALUES('" +
		std::string(OWNER_NAME) + "'," + std::to_string(save_id) + ",1," +
		std::to_string(room_vnum) +
		",'the corpse of CorpseHarness',"
		"'The corpse of CorpseHarness is lying here.','corpse corpseharness',20," +
		std::to_string(OWNER_PID) + ")");
	fixture.corpse_id = static_cast<uint32_t>(mysql_insert_id(database));
	assert(fixture.corpse_id);

	execute("INSERT INTO corpse_items(corpse_id,vnum,item_type,quantity,weight,cost,timer,"
		"extra_flags,wear_flags,value0,value1,value2,value3,value4,value5,value6,value7,"
		"name,short_descr,description,action_descr,obj_uid,item_condition,item_material) "
		"VALUES(" +
		std::to_string(fixture.corpse_id) + "," + std::to_string(root_vnum) +
		",15,1,10,100,-1,0,17,1,2,3,4,5,6,7,8,'root relic','a root relic',"
		"'A root relic lies here.','root action'," +
		std::to_string(root_uid) + ",88,4)");
	fixture.root_row_id = static_cast<uint32_t>(mysql_insert_id(database));
	assert(fixture.root_row_id);

	execute("INSERT INTO corpse_items(corpse_id,vnum,item_type,container_id,quantity,weight,"
		"cost,timer,extra_flags,wear_flags,value0,value1,value2,value3,value4,value5,"
		"value6,value7,name,short_descr,description,action_descr,obj_uid,item_condition,"
		"item_material) VALUES(" +
		std::to_string(fixture.corpse_id) + "," + std::to_string(fixture.child_vnum) +
		",15," + std::to_string(fixture.root_row_id) + ",1,4,200,-1," +
		std::to_string(artifact ? ITEM_ARTIFACT : 0) +
		",19,9,8,7,6,5,4,3,2,'child relic','a child relic',"
		"'A child relic lies here.','child action'," +
		std::to_string(child_uid) + ",77,5)");
	fixture.child_row_id = static_cast<uint32_t>(mysql_insert_id(database));
	assert(fixture.child_row_id);
	execute("INSERT INTO corpse_item_affects(item_id,location,modifier) VALUES(" +
		std::to_string(fixture.child_row_id) + ",2,11)");
	execute("INSERT INTO corpse_item_extra_descr(item_id,keyword,description) VALUES(" +
		std::to_string(fixture.child_row_id) + ",'runes','Fine runes cover it.')");

	if (skipped_children)
	{
		execute("INSERT INTO corpse_items(corpse_id,vnum,item_type,container_id,weight,"
			"value0,value1,value2,value3,name,short_descr) VALUES(" +
			std::to_string(fixture.corpse_id) + ",3,11," +
			std::to_string(fixture.root_row_id) + ",2,1,2,3,4,'coins','some coins')");
		execute("INSERT INTO corpse_items(corpse_id,vnum,item_type,container_id,weight,"
			"extra_flags,name,short_descr) VALUES(" +
			std::to_string(fixture.corpse_id) + "," + std::to_string(root_vnum + 2) +
			",15," + std::to_string(fixture.root_row_id) + ",3," +
			std::to_string(ITEM_TRANSIENT) + ",'mist','some fading mist')");
	}

	const item_owner_identity corpse_owner = { item_owner_type::corpse, fixture.owner_id, 0 };
	seed_owner(corpse_owner, source_owner_revision);
	seed_authority(root_uid, root_uid, 0, root_vnum, corpse_owner, INITIAL_ITEM_REVISION);
	seed_authority(child_uid, root_uid, root_uid, fixture.child_vnum, corpse_owner,
		       INITIAL_ITEM_REVISION);
	if (artifact)
		seed_artifact(fixture);
	return fixture;
}

collector::rules collector_rules()
{
	collector::rules value;
	value.enabled = true;
	return value;
}

void seed_candidate(uint64_t listing, uint64_t uid, uint64_t item_revision)
{
	const critical_operation_id death = operation();
	const std::string death_hex = operation_hex(death);
	const uint64_t death_time = 1700000000ULL + listing;
	execute("INSERT INTO collector_deaths(death_operation_id,beneficiary_pid,death_time) "
		"VALUES(UNHEX('" +
		death_hex + "')," + std::to_string(OWNER_PID) + "," + std::to_string(death_time) +
		")");
	collector::record entry;
	assert(collector::enroll(listing, death_hex, OWNER_PID, uid, item_revision, death_time,
				 collector_rules(), &entry) == collector::outcome::applied);
	std::array<uint8_t, collector::encoded_record_bytes> encoded = {};
	assert(collector::record_encode(entry, &encoded) == collector::codec_result::ok);
	execute("INSERT INTO collector_listings(listing_id,death_operation_id,beneficiary_pid,"
		"item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,"
		"record_blob,item_blob) VALUES(" +
		std::to_string(listing) + ",UNHEX('" + death_hex + "')," +
		std::to_string(OWNER_PID) + "," + std::to_string(uid) + ",1,0," +
		std::to_string(entry.collect_at) + ",1," + std::to_string(item_revision) +
		",0,UNHEX('" + hex_bytes(encoded.data(), encoded.size()) + "'),NULL)");
}

void seed_player(uint32_t pid, const std::string &name, const std::string &account,
		 const std::array<int32_t, 4> &wallet, uint64_t wallet_revision = 0)
{
	execute("INSERT INTO accounts(account_name,password) VALUES('" + account + "','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,"
		"platinum,wallet_revision) VALUES(" +
		std::to_string(pid) + ",'" + name + "','" + account + "',1," +
		std::to_string(wallet[0]) + "," + std::to_string(wallet[1]) + "," +
		std::to_string(wallet[2]) + "," + std::to_string(wallet[3]) + "," +
		std::to_string(wallet_revision) + ")");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		"bank_platinum,bank_revision) VALUES('" +
		account + "',1,0,0,0,0,0)");
}

corpse_lifecycle_payload payload_for(const corpse_fixture &fixture, corpse_lifecycle_action action)
{
	corpse_lifecycle_payload payload;
	payload.action = action;
	payload.owner_pid = OWNER_PID;
	payload.save_id = fixture.save_id;
	payload.expected_corpse_revision = 1;
	payload.room_vnum = fixture.room_vnum;
	payload.owner_name = OWNER_NAME;
	return payload;
}

critical_command command_for(const corpse_lifecycle_payload &payload)
{
	critical_command command = {};
	assert(corpse_lifecycle_command_build(&command, operation(), payload,
					      critical_source_site::zone_event,
					      critical_deadline_class::background));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	return command;
}

applied_corpse apply_success(const critical_command &command)
{
	applied_corpse applied;
	applied.raw = critical_command_repository_apply(database, command);
	if (applied.raw.outcome != critical_apply_outcome::applied || applied.raw.error_code)
	{
		corpse_lifecycle_payload failed_payload;
		assert(corpse_lifecycle_command_decode_payload(command, &failed_payload));
		fprintf(stderr, "corpse apply mismatch: outcome=%u error=%u mysql=%u %s errno=%d\n",
			static_cast<unsigned int>(applied.raw.outcome), applied.raw.error_code,
			mysql_errno(database), mysql_error(database), errno);
		fprintf(stderr,
			"action=%u owner=%u save=%u room=%d destination=%u old_room=%d "
			"corpse_revision=%llu room_revision=%llu player_revision=%llu "
			"wallet_revision=%llu\n",
			static_cast<unsigned int>(failed_payload.action), failed_payload.owner_pid,
			failed_payload.save_id, failed_payload.room_vnum,
			failed_payload.destination_player_pid, failed_payload.old_room_vnum,
			static_cast<unsigned long long>(failed_payload.expected_corpse_revision),
			static_cast<unsigned long long>(failed_payload.expected_room_revision),
			static_cast<unsigned long long>(failed_payload.expected_player_revision),
			static_cast<unsigned long long>(failed_payload.expected_wallet_revision));
	}
	assert(applied.raw.outcome == critical_apply_outcome::applied);
	assert(applied.raw.error_code == 0);
	assert(applied.raw.result_size == CORPSE_LIFECYCLE_RESULT_BYTES);
	assert(corpse_lifecycle_command_decode_result(applied.raw.result_payload.data(),
						      applied.raw.result_size, &applied.result));
	return applied;
}

void assert_exact_replay(const critical_command &command, const applied_corpse &first)
{
	const critical_apply_result replay = critical_command_repository_apply(database, command);
	assert(replay.outcome == critical_apply_outcome::already_applied);
	assert(replay.error_code == 0 && replay.result_size == first.raw.result_size);
	assert(std::equal(first.raw.result_payload.begin(),
			  first.raw.result_payload.begin() + first.raw.result_size,
			  replay.result_payload.begin()));
}

void assert_outbox(const critical_command &command, bool collector_changed)
{
	const std::string expected = collector_changed ? "0:12:1:1,1:11:1:2" : "0:12:1:1";
	assert(text("SELECT GROUP_CONCAT(CONCAT(event_index,':',destination,':',event_type,':',"
		    "payload_version) ORDER BY event_index) FROM critical_outbox WHERE "
		    "operation_id=UNHEX('" +
		    operation_hex(command.operation_id) + "')") == expected);
}

void test_room_release()
{
	constexpr int32_t ROOM = 4101;
	constexpr uint64_t LISTING = 9101, ROOT_UID = 810000001, CHILD_UID = 810000002,
			   TRANSIENT_UID = 810000003;
	const corpse_fixture fixture =
		seed_corpse(1001, ROOM, 1801, ROOT_UID, CHILD_UID, 2, true, true);
	execute("UPDATE corpse_items SET container_id=NULL,obj_uid=" +
		std::to_string(TRANSIENT_UID) + ",extra_flags=" +
		std::to_string(ITEM_TRANSIENT) +
		" WHERE corpse_id=" + std::to_string(fixture.corpse_id) +
		" AND vnum=3");
	seed_authority(TRANSIENT_UID, TRANSIENT_UID, 0, 3,
		       { item_owner_type::corpse, fixture.owner_id, 0 }, INITIAL_ITEM_REVISION);
	seed_owner({ item_owner_type::room, ROOM, 0 }, 3);
	seed_candidate(LISTING, ROOT_UID, INITIAL_ITEM_REVISION);
	const uint64_t catalog_before =
		scalar("SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1");

	corpse_lifecycle_payload payload = payload_for(fixture, corpse_lifecycle_action::release);
	payload.expected_room_revision = 3;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.action == corpse_lifecycle_action::release &&
	       applied.result.catalog_revision == catalog_before + 1 &&
	       applied.result.corpse_owner_revision == 4 &&
	       applied.result.room_owner_revision == 4 && applied.result.item_count == 2 &&
	       applied.result.max_item_revision == 6 &&
	       applied.result.destruction_owner_revision == 1 &&
	       applied.result.discarded_item_count == 1 &&
	       applied.result.max_discarded_item_revision == 6 &&
	       !applied.result.collector_catalog_changed);
	assert(scalar("SELECT COUNT(*) FROM corpses WHERE id=" +
		      std::to_string(fixture.corpse_id)) == 0);
	assert(text("SELECT GROUP_CONCAT(CONCAT(item_uid,':',root_item_uid,':',"
		    "COALESCE(parent_item_uid,0),':',owner_type,':',owner_id,':',item_revision,"
		    "':',state) ORDER BY item_uid) FROM item_current_owner WHERE item_uid IN (" +
		    std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") ==
	       "810000001:810000001:0:3:4101:6:1,"
	       "810000002:810000001:810000001:3:4101:6:1");
	assert(text("SELECT CONCAT(r.weight,':',c.weight,':',c.container_id=r.id) FROM "
		    "saved_items r JOIN saved_items c ON c.container_id=r.id WHERE r.obj_uid=" +
		    std::to_string(ROOT_UID) + " AND c.obj_uid=" + std::to_string(CHILD_UID)) ==
	       "7:4:1");
	assert(scalar("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN (" +
		      std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") == 2);
	assert(text("SELECT CONCAT(owner_type,':',state,':',item_revision) FROM "
		    "item_current_owner WHERE item_uid=" +
		    std::to_string(TRANSIENT_UID)) == "8:2:6");
	assert(scalar("SELECT COUNT(*) FROM saved_item_affects a JOIN saved_items i ON "
		      "i.id=a.item_id WHERE i.obj_uid=" +
		      std::to_string(CHILD_UID) + " AND a.location=2 AND a.modifier=11") == 1);
	assert(scalar("SELECT COUNT(*) FROM saved_item_extra_descr e JOIN saved_items i ON "
		      "i.id=e.item_id WHERE i.obj_uid=" +
		      std::to_string(CHILD_UID) + " AND e.keyword='runes'") == 1);
	assert(text("SELECT CONCAT(owned,':',loc_type,':',location,':',item_uid,':',"
		    "item_revision,':',revision) FROM artifact_domain_state WHERE vnum=" +
		    std::to_string(fixture.child_vnum)) == "1:4:4101:810000002:6:10");
	assert(text("SELECT CONCAT(status,':',listing_revision,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM collector_listings WHERE "
		    "listing_id=" +
		    std::to_string(LISTING)) == "1:1:5:00");
	assert_outbox(command, false);
	assert_exact_replay(command, applied);
	assert(scalar("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN (" +
		      std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") == 2);
	assert(scalar("SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1") ==
	       catalog_before + 1);
}

void test_destruction()
{
	constexpr int32_t ROOM = 4102;
	constexpr uint64_t LISTING = 9102, ROOT_UID = 820000001, CHILD_UID = 820000002;
	const corpse_fixture fixture =
		seed_corpse(1002, ROOM, 1811, ROOT_UID, CHILD_UID, 4, true, false);
	seed_candidate(LISTING, CHILD_UID, INITIAL_ITEM_REVISION);
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	const uint64_t destruction_before = owner_revision(destruction);

	corpse_lifecycle_payload payload = payload_for(fixture, corpse_lifecycle_action::destroy);
	payload.expected_room_revision = destruction_before;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.action == corpse_lifecycle_action::destroy &&
	       applied.result.corpse_owner_revision == 5 &&
	       applied.result.room_owner_revision == destruction_before + 1 &&
	       applied.result.item_count == 2 && applied.result.max_item_revision == 6 &&
	       applied.result.collector_catalog_changed);
	assert(text("SELECT GROUP_CONCAT(CONCAT(owner_type,':',state,':',item_revision) "
		    "ORDER BY item_uid) FROM item_current_owner WHERE item_uid IN (" +
		    std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") ==
	       "8:2:6,8:2:6");
	assert(text("SELECT CONCAT(status,':',listing_revision,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM collector_listings WHERE "
		    "listing_id=" +
		    std::to_string(LISTING)) == "5:2:6:02");
	assert(text("SELECT CONCAT(owned,':',loc_type,':',location,':',bind_owner_pid,':',"
		    "bind_timer_epoch,':',item_revision,':',revision) FROM artifact_domain_state "
		    "WHERE vnum=" +
		    std::to_string(fixture.child_vnum)) == "0:1:-1:-1:0:6:10");
	assert(text("SELECT CONCAT(owned,':',locType,':',location) FROM artifacts WHERE vnum=" +
		    std::to_string(fixture.child_vnum)) == "N:1:-1");
	assert(text("SELECT CONCAT(owner_pid,':',timer) FROM artifact_bind WHERE vnum=" +
		    std::to_string(fixture.child_vnum)) == "-1:0");
	assert(scalar("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN (" +
		      std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") == 0);
	assert(text("SELECT CONCAT(action,':',actor_pid,':',closed_reason) FROM collector_ledger "
		    "WHERE operation_id=UNHEX('" +
		    operation_hex(command.operation_id) + "')") == "5:0:2");
	assert_outbox(command, true);
	assert_exact_replay(command, applied);
	assert(scalar("SELECT COUNT(*) FROM collector_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 1);
}

void test_resurrection()
{
	constexpr uint32_t PLAYER = 2147000602U;
	constexpr int32_t ROOM = 4103, OLD_ROOM = 4104;
	constexpr uint64_t LISTING = 9103, ROOT_UID = 830000001, CHILD_UID = 830000002;
	const std::array<int32_t, 4> wallet = { 10, 20, 30, 40 };
	seed_player(PLAYER, "ResurrectedHarness", "corpse_resurrect", wallet);
	seed_owner({ item_owner_type::player, PLAYER, 0 }, 1);
	seed_owner({ item_owner_type::room, OLD_ROOM, 0 }, 2);
	const corpse_fixture fixture =
		seed_corpse(1003, ROOM, 1821, ROOT_UID, CHILD_UID, 3, true, true);
	seed_candidate(LISTING, ROOT_UID, INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload = payload_for(fixture, corpse_lifecycle_action::resurrect);
	payload.destination_player_pid = PLAYER;
	payload.old_room_vnum = OLD_ROOM;
	payload.expected_room_revision = 2;
	payload.expected_player_revision = 1;
	payload.expected_wallet_revision = 0;
	payload.money = wallet;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.action == corpse_lifecycle_action::resurrect &&
	       applied.result.corpse_owner_revision == 4 &&
	       applied.result.room_owner_revision == 3 &&
	       applied.result.player_owner_revision == 2 && applied.result.wallet_revision == 1 &&
	       applied.result.bank_revision == 1 && applied.result.wallet[0] == 1 &&
	       applied.result.wallet[1] == 2 && applied.result.wallet[2] == 3 &&
	       applied.result.wallet[3] == 4 && applied.result.collector_catalog_changed);
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		    "FROM player_data WHERE pid=" +
		    std::to_string(PLAYER)) == "1:2:3:4:1");
	assert(scalar("SELECT bank_revision FROM account_banks WHERE account_name='corpse_resurrect' "
		      "AND racewar=1") == 1);
	assert(text("SELECT CONCAT(r.weight,':',c.container_id=r.id) FROM player_items r JOIN "
		    "player_items c ON c.container_id=r.id WHERE r.pid=" +
		    std::to_string(PLAYER) + " AND r.obj_uid=" + std::to_string(ROOT_UID) +
		    " AND c.obj_uid=" + std::to_string(CHILD_UID)) == "5:1");
	assert(text("SELECT CONCAT(status,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM "
		    "collector_listings WHERE listing_id=" +
		    std::to_string(LISTING)) == "5:6:01");
	assert(text("SELECT CONCAT(owned,':',loc_type,':',location,':',item_revision,':',"
		    "revision) FROM artifact_domain_state WHERE vnum=" +
		    std::to_string(fixture.child_vnum)) == "1:3:2147000602:6:10");
	assert(scalar("SELECT reason_type FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") ==
	       static_cast<unsigned int>(currency_reason_type::corpse_lifecycle));
	assert(text("SELECT CONCAT(action,':',actor_pid,':',closed_reason) FROM collector_ledger "
		    "WHERE operation_id=UNHEX('" +
		    operation_hex(command.operation_id) + "')") == "5:2147000602:1");
	assert_outbox(command, true);
	assert_exact_replay(command, applied);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 1);
}

void test_raise_follower()
{
	constexpr uint32_t PLAYER = 2147000603U;
	constexpr int32_t ROOM = 4105;
	constexpr uint64_t LISTING = 9104, ROOT_UID = 840000001, CHILD_UID = 840000002,
			   TRANSIENT_UID = 840000003;
	const std::array<int32_t, 4> wallet = { 5, 6, 7, 8 };
	seed_player(PLAYER, "RaisedHarness", "corpse_raise", wallet);
	seed_owner({ item_owner_type::player, PLAYER, 0 }, 1);
	const corpse_fixture fixture =
		seed_corpse(1004, ROOM, 1831, ROOT_UID, CHILD_UID, 1, false, true);
	execute("UPDATE corpse_items SET obj_uid=" + std::to_string(TRANSIENT_UID) +
		" WHERE corpse_id=" + std::to_string(fixture.corpse_id) + " AND vnum=1833");
	seed_authority(TRANSIENT_UID, ROOT_UID, ROOT_UID, 1833,
		       { item_owner_type::corpse, fixture.owner_id, 0 }, INITIAL_ITEM_REVISION);
	seed_candidate(LISTING, CHILD_UID, INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload =
		payload_for(fixture, corpse_lifecycle_action::raise_follower);
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.money = wallet;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.corpse_owner_revision == 3 &&
	       applied.result.discarded_item_count == 1 &&
	       applied.result.destruction_owner_revision ==
		       owner_revision({ item_owner_type::destruction, 0, 0 }) &&
	       applied.result.max_discarded_item_revision == 6);
	assert(applied.result.player_owner_revision == 2 && applied.result.wallet_revision == 1 &&
	       applied.result.bank_revision == 1 && applied.result.wallet[0] == 6 &&
	       applied.result.wallet[1] == 8 && applied.result.wallet[2] == 10 &&
	       applied.result.wallet[3] == 12 && applied.result.collector_catalog_changed);
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		    "FROM player_data WHERE pid=" +
		    std::to_string(PLAYER)) == "6:8:10:12:1");
	assert(text("SELECT CONCAT(status,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM "
		    "collector_listings WHERE listing_id=" +
		    std::to_string(LISTING)) == "5:6:01");
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(PLAYER) +
		      " AND obj_uid IN (" + std::to_string(ROOT_UID) + "," +
		      std::to_string(CHILD_UID) + ")") == 2);
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
		      std::to_string(TRANSIENT_UID)) == 0);
	assert(text("SELECT CONCAT(owner_type,':',item_revision,':',state) "
		    "FROM item_current_owner WHERE item_uid=" +
		    std::to_string(TRANSIENT_UID)) == "8:6:2");
	assert_outbox(command, true);
}

void test_coinless_raise_follower()
{
	constexpr uint32_t PLAYER = 2147000610U;
	constexpr uint64_t ROOT_UID = 840000011, CHILD_UID = 840000012, TRANSIENT_UID = 840000013;
	const std::array<int32_t, 4> wallet = {};
	seed_player(PLAYER, "CoinlessRaisedHarness", "corpse_raise_coinless", wallet);
	seed_owner({ item_owner_type::player, PLAYER, 0 }, 1);
	const corpse_fixture fixture =
		seed_corpse(1008, 4108, 1871, ROOT_UID, CHILD_UID, 1, false, true);
	execute("DELETE FROM corpse_items WHERE corpse_id=" + std::to_string(fixture.corpse_id) +
		" AND vnum=3");
	execute("UPDATE corpse_items SET obj_uid=" + std::to_string(TRANSIENT_UID) +
		" WHERE corpse_id=" + std::to_string(fixture.corpse_id) + " AND vnum=1873");
	seed_authority(TRANSIENT_UID, ROOT_UID, ROOT_UID, 1873,
		       { item_owner_type::corpse, fixture.owner_id, 0 }, INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload =
		payload_for(fixture, corpse_lifecycle_action::raise_follower);
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.money = wallet;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.item_count == 2 && applied.result.discarded_item_count == 1);
	assert(applied.result.wallet == wallet && applied.result.wallet_revision == 1 &&
	       applied.result.bank_revision == 1);
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		    "FROM player_data WHERE pid=" +
		    std::to_string(PLAYER)) == "0:0:0:0:1");
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 1);
	assert(text("SELECT CONCAT(owner_type,':',state) FROM item_current_owner WHERE item_uid=" +
		    std::to_string(TRANSIENT_UID)) == "8:2");
	assert_exact_replay(command, applied);
}

void test_nested_room_release()
{
	constexpr int32_t ROOM = 4106;
	constexpr uint64_t LISTING = 9105, PARENT_UID = 850000000, ROOT_UID = 850000001,
			   CHILD_UID = 850000002;
	const item_owner_identity room_owner = { item_owner_type::room, ROOM, 0 };
	seed_owner(room_owner, 1);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,weight,name,short_descr,obj_uid) "
		"VALUES('nested-room-parent'," +
		std::to_string(ROOM) + ",1840,20,'parent chest','a parent chest'," +
		std::to_string(PARENT_UID) + ")");
	const uint64_t parent_row = mysql_insert_id(database);
	seed_authority(PARENT_UID, PARENT_UID, 0, 1840, room_owner, 9);
	const corpse_fixture fixture =
		seed_corpse(1005, ROOM, 1841, ROOT_UID, CHILD_UID, 2, false, false);
	seed_candidate(LISTING, ROOT_UID, INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload =
		payload_for(fixture, corpse_lifecycle_action::release_nested);
	payload.expected_room_revision = 1;
	payload.target_root_item_uid = PARENT_UID;
	payload.target_parent_item_uid = PARENT_UID;
	payload.expected_target_parent_revision = 9;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.room_owner_revision == 2 && !applied.result.player_owner_revision &&
	       !applied.result.collector_catalog_changed);
	assert(text("SELECT CONCAT(root_item_uid,':',parent_item_uid,':',item_revision) FROM "
		    "item_current_owner WHERE item_uid=" +
		    std::to_string(ROOT_UID)) == "850000000:850000000:6");
	assert(text("SELECT CONCAT(root_item_uid,':',parent_item_uid,':',item_revision) FROM "
		    "item_current_owner WHERE item_uid=" +
		    std::to_string(CHILD_UID)) == "850000000:850000001:6");
	assert(scalar("SELECT container_id FROM saved_items WHERE obj_uid=" +
		      std::to_string(ROOT_UID)) == parent_row);
	assert(text("SELECT CONCAT(status,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM "
		    "collector_listings WHERE listing_id=" +
		    std::to_string(LISTING)) == "1:5:00");
	assert_outbox(command, false);
}

void test_nested_player_release()
{
	constexpr uint32_t PLAYER = 2147000604U;
	constexpr int32_t ROOM = 4107;
	constexpr uint64_t LISTING = 9106, PARENT_UID = 860000000, ROOT_UID = 860000001,
			   CHILD_UID = 860000002;
	const std::array<int32_t, 4> wallet = { 2, 3, 4, 5 };
	seed_player(PLAYER, "NestedHarness", "corpse_nested", wallet);
	const item_owner_identity player_owner = { item_owner_type::player, PLAYER, 0 };
	seed_owner(player_owner, 1);
	execute("INSERT INTO player_items(pid,vnum,weight,name,short_descr,obj_uid) VALUES(" +
		std::to_string(PLAYER) + ",1850,20,'parent pack','a parent pack'," +
		std::to_string(PARENT_UID) + ")");
	const uint64_t parent_row = mysql_insert_id(database);
	seed_authority(PARENT_UID, PARENT_UID, 0, 1850, player_owner, 9);
	const corpse_fixture fixture =
		seed_corpse(1006, ROOM, 1851, ROOT_UID, CHILD_UID, 2, false, true);
	seed_candidate(LISTING, ROOT_UID, INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload =
		payload_for(fixture, corpse_lifecycle_action::release_nested);
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.expected_wallet_revision = 0;
	payload.money = wallet;
	payload.target_root_item_uid = PARENT_UID;
	payload.target_parent_item_uid = PARENT_UID;
	payload.expected_target_parent_revision = 9;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.player_owner_revision == 2 && applied.result.wallet_revision == 1 &&
	       applied.result.bank_revision == 1 && applied.result.collector_catalog_changed);
	assert(scalar("SELECT container_id FROM player_items WHERE obj_uid=" +
		      std::to_string(ROOT_UID)) == parent_row);
	assert(text("SELECT CONCAT(root_item_uid,':',parent_item_uid,':',item_revision) FROM "
		    "item_current_owner WHERE item_uid=" +
		    std::to_string(ROOT_UID)) == "860000000:860000000:6");
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum) FROM player_data "
		    "WHERE pid=" +
		    std::to_string(PLAYER)) == "3:5:7:9");
	assert(text("SELECT CONCAT(status,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM "
		    "collector_listings WHERE listing_id=" +
		    std::to_string(LISTING)) == "5:6:01");
	assert_outbox(command, true);
}

void test_equipped_world_corpse_raise()
{
	constexpr uint32_t PLAYER = 2147000611U;
	constexpr int32_t ROOM = 4111;
	constexpr uint64_t ROOT_UID = 900000001, ORDINARY_UID = 900000002,
			   NO_TAKE_UID = 900000003, NO_SHOW_UID = 900000004,
			   TRANSIENT_UID = 900000005, MONEY_UID = 900000006;
	const item_owner_identity room_owner = { item_owner_type::room, ROOM, 0 };
	const item_owner_identity player_owner = { item_owner_type::player, PLAYER, 0 };
	seed_player(PLAYER, "WorldRaiseHarness", "corpse_world_raise", {});
	seed_owner(room_owner, 1);
	seed_owner(player_owner, 1);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,item_type,obj_uid,weight,timer,"
		"value1,value2,name,short_descr,description) VALUES('world-raise'," +
		std::to_string(ROOM) + ",2,24," + std::to_string(ROOT_UID) +
		",12,1000000,4,56,'ordinary beast corpse _npcorpse_',"
		"'the corpse of an ordinary beast','An ordinary corpse lies here.')");
	const uint64_t root_row = mysql_insert_id(database);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,container_id,obj_uid,weight,"
		"wear_flags,name,short_descr) VALUES('world-raise-ordinary'," +
		std::to_string(ROOM) + ",48," + std::to_string(root_row) + "," +
		std::to_string(ORDINARY_UID) + ",5,1,'ordinary backpack','an ordinary backpack')");
	const uint64_t ordinary_row = mysql_insert_id(database);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,container_id,obj_uid,weight,"
		"wear_flags,name,short_descr) VALUES('world-raise-no-take'," +
		std::to_string(ROOM) + ",48," + std::to_string(ordinary_row) + "," +
		std::to_string(NO_TAKE_UID) + ",2,0,'no-take token','a no-take token')");
	const uint64_t no_take_row = mysql_insert_id(database);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,container_id,obj_uid,weight,"
		"extra_flags,wear_flags,name,short_descr) VALUES('world-raise-no-show'," +
		std::to_string(ROOM) + ",5," + std::to_string(no_take_row) + "," +
		std::to_string(NO_SHOW_UID) +
		",1,2050,0,'no-show token','a no-show token')");
	const uint64_t no_show_row = mysql_insert_id(database);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,container_id,obj_uid,weight,"
		"extra_flags,wear_flags,name,short_descr) VALUES('world-raise-transient'," +
		std::to_string(ROOM) + ",5," + std::to_string(ordinary_row) + "," +
		std::to_string(TRANSIENT_UID) + ",1," + std::to_string(ITEM_TRANSIENT) +
		",1,'fading token','a fading token')");
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,item_type,container_id,obj_uid,"
		"weight,value0,value1,value2,value3,name,short_descr) VALUES('world-raise-money'," +
		std::to_string(ROOM) + ",3,20," + std::to_string(root_row) + "," +
		std::to_string(MONEY_UID) + ",2,1,2,3,4,'coins','some coins')");
	execute("INSERT INTO saved_item_affects(item_id,location,modifier) VALUES(" +
		std::to_string(no_take_row) + ",1,7)");
	execute("INSERT INTO saved_item_extra_descr(item_id,keyword,description) VALUES(" +
		std::to_string(no_show_row) + ",'restricted-mark','A restricted mark.')");
	for (const auto &[uid, parent, vnum] :
	     std::array<std::array<uint64_t, 3>, 6>{ {
		     { ROOT_UID, 0, 2 },
		     { ORDINARY_UID, ROOT_UID, 48 },
		     { NO_TAKE_UID, ORDINARY_UID, 48 },
		     { NO_SHOW_UID, NO_TAKE_UID, 5 },
		     { TRANSIENT_UID, ORDINARY_UID, 5 },
		     { MONEY_UID, ROOT_UID, 3 },
	     } })
		seed_authority(uid, ROOT_UID, parent, static_cast<int32_t>(vnum), room_owner,
			       INITIAL_ITEM_REVISION);

	corpse_lifecycle_payload payload = {};
	payload.action = corpse_lifecycle_action::raise_world_follower;
	payload.owner_pid = static_cast<uint32_t>(ROOT_UID >> 32);
	payload.save_id = static_cast<uint32_t>(ROOT_UID);
	payload.expected_corpse_revision = INITIAL_ITEM_REVISION;
	payload.expected_room_revision = 1;
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.room_vnum = ROOM;
	payload.owner_name = "ordinary beast corpse _npcorpse_";
	payload.pet_uid = ROOT_UID;
	payload.pet_mob_vnum = 701;
	payload.pet_hit = payload.pet_max_hit = 20;
	payload.pet_mana = payload.pet_max_mana = 10;
	payload.pet_vitality = payload.pet_max_vitality = 5;
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.action == corpse_lifecycle_action::raise_world_follower &&
	       applied.result.catalog_revision == 5 &&
	       applied.result.corpse_owner_revision == 5 &&
	       applied.result.pet_owner_revision == 1 && applied.result.item_count == 3 &&
	       applied.result.max_item_revision == 7 &&
	       applied.result.destruction_owner_revision ==
		       owner_revision({ item_owner_type::destruction, 0, 0 }) &&
	       applied.result.discarded_item_count == 3 &&
	       applied.result.max_discarded_item_revision == 7 &&
	       !applied.result.wallet_revision && !applied.result.player_owner_revision);
	assert(scalar("SELECT COUNT(*) FROM saved_items WHERE obj_uid BETWEEN " +
		      std::to_string(ROOT_UID) + " AND " + std::to_string(MONEY_UID)) == 0);
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
		      std::to_string(ORDINARY_UID) + "," + std::to_string(NO_TAKE_UID) + "," +
		      std::to_string(NO_SHOW_UID) + ")") == 0);
	assert(scalar("SELECT COUNT(*) FROM player_pets WHERE owner_pid=" +
		      std::to_string(PLAYER) + " AND pet_uid=" + std::to_string(ROOT_UID)) == 1);
	assert(scalar("SELECT COUNT(*) FROM player_pet_items WHERE obj_uid IN (" +
		      std::to_string(ORDINARY_UID) + "," + std::to_string(NO_TAKE_UID) + "," +
		      std::to_string(NO_SHOW_UID) + ")") == 3);
	assert(text("SELECT CONCAT(root_item_uid,':',COALESCE(parent_item_uid,0),':',"
		    "owner_type,':',owner_id,':',owner_context_id,':',item_revision,':',state) "
		    "FROM item_current_owner WHERE item_uid=" +
		    std::to_string(NO_SHOW_UID)) ==
	       "900000002:900000003:11:900000001:" + std::to_string(PLAYER) + ":7:1");
	assert(text("SELECT CONCAT(extra_flags,':',wear_flags) FROM player_pet_items WHERE "
		    "obj_uid=" +
		    std::to_string(NO_SHOW_UID)) == "2050:0");
	assert(scalar("SELECT COUNT(*) FROM player_pet_item_affects a JOIN player_pet_items i "
		      "ON i.id=a.item_id WHERE i.obj_uid=" +
		      std::to_string(NO_TAKE_UID) + " AND a.location=1 AND a.modifier=7") == 1);
	assert(scalar("SELECT COUNT(*) FROM player_pet_item_extra_descr e JOIN player_pet_items i "
		      "ON i.id=e.item_id WHERE i.obj_uid=" +
		      std::to_string(NO_SHOW_UID) + " AND e.keyword='restricted-mark'") == 1);
	assert(text("SELECT GROUP_CONCAT(CONCAT(item_uid,':',owner_type,':',state,':',"
		    "item_revision) ORDER BY item_uid) FROM item_current_owner WHERE item_uid IN (" +
		    std::to_string(ROOT_UID) + "," + std::to_string(TRANSIENT_UID) + "," +
		    std::to_string(MONEY_UID) + ")") ==
	       "900000001:8:2:6,900000005:8:2:7,900000006:8:2:6");
	assert_exact_replay(command, applied);
	assert_outbox(command, false);
}

void test_hostile_equipped_world_corpse_raise()
{
	constexpr uint32_t PLAYER = 2147000612U;
	constexpr int32_t ROOM = 4112;
	constexpr uint64_t ROOT_UID = 901000001, GEAR_UID = 901000002;
	const item_owner_identity room_owner = { item_owner_type::room, ROOM, 0 };
	const item_owner_identity player_owner = { item_owner_type::player, PLAYER, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	seed_player(PLAYER, "HostileWorldRaiseHarness", "corpse_hostile_world_raise", {});
	seed_owner(room_owner, 1);
	seed_owner(player_owner, 1);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,item_type,obj_uid,weight,timer,"
		"value1,value2,name,short_descr,description) VALUES('hostile-world-raise'," +
		std::to_string(ROOM) + ",2,24," + std::to_string(ROOT_UID) +
		",12,1000000,4,56,'hostile beast corpse _npcorpse_',"
		"'the corpse of a hostile beast','A hostile corpse lies here.')");
	const uint64_t root_row = mysql_insert_id(database);
	execute("INSERT INTO saved_items(item_key,room_vnum,vnum,item_type,container_id,obj_uid,"
		"weight,wear_flags,name,short_descr) VALUES('hostile-world-raise-gear'," +
		std::to_string(ROOM) + ",5,9," + std::to_string(root_row) + "," +
		std::to_string(GEAR_UID) + ",2,0,'no-take armor','some no-take armor')");
	seed_authority(ROOT_UID, ROOT_UID, 0, 2, room_owner, INITIAL_ITEM_REVISION);
	seed_authority(GEAR_UID, ROOT_UID, ROOT_UID, 5, room_owner, INITIAL_ITEM_REVISION);
	const uint64_t destruction_before = owner_revision(destruction);

	corpse_lifecycle_payload payload = {};
	payload.action = corpse_lifecycle_action::raise_world_follower;
	payload.owner_pid = static_cast<uint32_t>(ROOT_UID >> 32);
	payload.save_id = static_cast<uint32_t>(ROOT_UID);
	payload.expected_corpse_revision = INITIAL_ITEM_REVISION;
	payload.expected_room_revision = 1;
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.room_vnum = ROOM;
	payload.owner_name = "hostile beast corpse _npcorpse_";
	const critical_command command = command_for(payload);
	const applied_corpse applied = apply_success(command);
	assert(applied.result.action == corpse_lifecycle_action::raise_world_follower &&
	       applied.result.catalog_revision == 2 &&
	       applied.result.corpse_owner_revision == 2 && !applied.result.pet_owner_revision &&
	       !applied.result.item_count && !applied.result.max_item_revision &&
	       applied.result.destruction_owner_revision == destruction_before + 1 &&
	       applied.result.discarded_item_count == 2 &&
	       applied.result.max_discarded_item_revision == INITIAL_ITEM_REVISION + 1);
	assert(scalar("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN (" +
		      std::to_string(ROOT_UID) + "," + std::to_string(GEAR_UID) + ")") == 0);
	assert(scalar("SELECT COUNT(*) FROM player_pets WHERE owner_pid=" +
		      std::to_string(PLAYER)) == 0);
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
		      std::to_string(GEAR_UID)) == 0);
	assert(scalar("SELECT COUNT(*) FROM player_pet_items WHERE obj_uid=" +
		      std::to_string(GEAR_UID)) == 0);
	assert(text("SELECT GROUP_CONCAT(CONCAT(item_uid,':',owner_type,':',state,':',"
		    "item_revision) ORDER BY item_uid) FROM item_current_owner WHERE item_uid IN (" +
		    std::to_string(ROOT_UID) + "," + std::to_string(GEAR_UID) + ")") ==
	       "901000001:8:2:6,901000002:8:2:6");
	assert_exact_replay(command, applied);
	assert_outbox(command, false);
}

void test_stale_wallet_rolls_back()
{
	constexpr uint32_t PLAYER = 2147000605U;
	constexpr int32_t ROOM = 4108;
	constexpr uint64_t LISTING = 9107, ROOT_UID = 870000001, CHILD_UID = 870000002;
	const std::array<int32_t, 4> wallet = { 7, 8, 9, 10 };
	seed_player(PLAYER, "StaleHarness", "corpse_stale", wallet, 1);
	seed_owner({ item_owner_type::player, PLAYER, 0 }, 1);
	const corpse_fixture fixture =
		seed_corpse(1007, ROOM, 1861, ROOT_UID, CHILD_UID, 6, false, true);
	seed_candidate(LISTING, ROOT_UID, INITIAL_ITEM_REVISION);
	const uint64_t corpse_catalog_before =
		scalar("SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1");
	const uint64_t collector_catalog_before =
		scalar("SELECT catalog_revision FROM collector_catalog_state WHERE state_id=1");

	corpse_lifecycle_payload payload =
		payload_for(fixture, corpse_lifecycle_action::raise_follower);
	payload.destination_player_pid = PLAYER;
	payload.expected_player_revision = 1;
	payload.expected_wallet_revision = 0;
	payload.money = wallet;
	const critical_command command = command_for(payload);
	critical_apply_result rejected = critical_command_repository_apply(database, command);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure &&
	       rejected.error_code == ESTALE && rejected.result_size == 0);
	assert(scalar("SELECT COUNT(*) FROM corpses WHERE id=" +
		      std::to_string(fixture.corpse_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM corpse_items WHERE corpse_id=" +
		      std::to_string(fixture.corpse_id)) == 4);
	assert(text("SELECT GROUP_CONCAT(CONCAT(owner_type,':',owner_id,':',item_revision,':',"
		    "state) ORDER BY item_uid) FROM item_current_owner WHERE item_uid IN (" +
		    std::to_string(ROOT_UID) + "," + std::to_string(CHILD_UID) + ")") ==
	       "4:" + std::to_string(fixture.owner_id) +
		       ":5:1,4:" + std::to_string(fixture.owner_id) + ":5:1");
	assert(text("SELECT CONCAT(status,':',listing_revision,':',item_revision,':',"
		    "HEX(SUBSTRING(record_blob,154,1))) FROM collector_listings WHERE "
		    "listing_id=" +
		    std::to_string(LISTING)) == "1:1:5:00");
	assert(scalar("SELECT catalog_revision FROM corpse_catalog_state WHERE state_id=1") ==
	       corpse_catalog_before);
	assert(scalar("SELECT catalog_revision FROM collector_catalog_state WHERE state_id=1") ==
	       collector_catalog_before);
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		    "FROM player_data WHERE pid=" +
		    std::to_string(PLAYER)) == "7:8:9:10:1");
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM collector_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(command.operation_id) + "')") == 0);
	rejected = critical_command_repository_apply(database, command);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure &&
	       rejected.error_code == ESTALE && rejected.durable_revision == 1 &&
	       rejected.result_size == 0);
}

void test_stale_corpse_revision_reports_authority()
{
	constexpr int32_t ROOM = 4109;
	constexpr uint64_t ROOT_UID = 880000001, CHILD_UID = 880000002;
	seed_player(OWNER_PID, "CorpseHarnessOwner", "corpse_owner_stale", {});
	const corpse_fixture fixture =
		seed_corpse(1009, ROOM, 1881, ROOT_UID, CHILD_UID, 1, false, false);
	corpse_lifecycle_payload payload = payload_for(fixture, corpse_lifecycle_action::release);
	payload.expected_corpse_revision = 2;
	const critical_apply_result rejected =
		critical_command_repository_apply(database, command_for(payload));
	assert(rejected.outcome == critical_apply_outcome::terminal_failure &&
	       rejected.error_code == ESTALE && rejected.durable_revision == 1 &&
	       rejected.result_size == 0);
	assert(scalar("SELECT COUNT(*) FROM corpses WHERE id=" +
		      std::to_string(fixture.corpse_id)) == 1);
}

void test_stale_corpse_owner_missing_is_quarantined()
{
	execute("DELETE FROM player_data WHERE pid=" + std::to_string(OWNER_PID));
	constexpr int32_t ROOM = 4110;
	constexpr uint64_t ROOT_UID = 890000001, CHILD_UID = 890000002;
	const corpse_fixture fixture =
		seed_corpse(1010, ROOM, 1891, ROOT_UID, CHILD_UID, 1, false, false);
	corpse_lifecycle_payload payload = payload_for(fixture, corpse_lifecycle_action::release);
	payload.expected_corpse_revision = 2;
	const critical_apply_result rejected =
		critical_command_repository_apply(database, command_for(payload));
	assert(rejected.outcome == critical_apply_outcome::terminal_failure &&
	       rejected.error_code == ESRCH && rejected.durable_revision == 0 &&
	       rejected.result_size == 0);
	assert(scalar("SELECT COUNT(*) FROM corpses WHERE id=" +
		      std::to_string(fixture.corpse_id)) == 1);
}
} // namespace

int main()
{
	database = mysql_init(nullptr);
	assert(database);
	assert(mysql_real_connect(
		database, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("CORPSE_LIFECYCLE_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	execute("UPDATE collector_catalog_state SET next_listing=100000 WHERE state_id=1");
	test_room_release();
	test_destruction();
	test_resurrection();
	test_raise_follower();
	test_coinless_raise_follower();
	test_nested_room_release();
	test_nested_player_release();
	test_equipped_world_corpse_raise();
	test_hostile_equipped_world_corpse_raise();
	test_stale_wallet_rolls_back();
	test_stale_corpse_revision_reports_authority();
	test_stale_corpse_owner_missing_is_quarantined();
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=" +
		      std::to_string(static_cast<unsigned int>(
			      critical_command_type::corpse_lifecycle))) == 12);
	mysql_close(database);
	return 0;
}
