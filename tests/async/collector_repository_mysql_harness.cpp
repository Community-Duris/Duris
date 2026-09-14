#include "economy/collector_command.h"
#include "economy/collector_repository.h"
#include "persistence/critical_command_repository.h"
#include "player/player_snapshot_codec.h"

#include <mysql.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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
constexpr uint32_t PLAYER_PID = 2147000801U;
constexpr const char *PLAYER_NAME = "CollectorHarness";
constexpr const char *ACCOUNT_NAME = "collector_harness";
constexpr uint64_t DEATH_TIME = 1700000000;

void execute(const std::string &sql)
{
	if (mysql_real_query(database, sql.data(), sql.size()) != 0)
		fprintf(stderr, "collector harness SQL failed: %u %s\n%s\n", mysql_errno(database),
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
		value[index * 2 + 1] = HEX[bytes[index] & 0xf];
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

uint64_t due_at(const collector::record &entry)
{
	switch (entry.status)
	{
	case collector::state::candidate:
		return entry.collect_at;
	case collector::state::collected:
		return entry.sale_at;
	case collector::state::available:
		return entry.holding_paused ? 0 : entry.expires_at;
	case collector::state::purchased:
	case collector::state::cancelled:
	case collector::state::expired:
		return 0;
	}
	return 0;
}

collector::rules rules()
{
	collector::rules value;
	value.enabled = true;
	return value;
}

void seed_death(const critical_operation_id &death, uint64_t death_time = DEATH_TIME)
{
	const std::string id = operation_hex(death);
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
		id + "'),UNHEX(REPEAT('11',32)),UNHEX(REPEAT('22',32)),16,1,1,1,X'')");
	execute("INSERT INTO collector_deaths(death_operation_id,beneficiary_pid,death_time) "
		"VALUES(UNHEX('" +
		id + "')," + std::to_string(PLAYER_PID) + "," + std::to_string(death_time) + ")");
}

void seed_listing(const collector::record &entry, const std::vector<uint8_t> &item_blob = {})
{
	std::array<uint8_t, collector::encoded_record_bytes> record = {};
	assert(collector::record_encode(entry, &record) == collector::codec_result::ok);
	const uint64_t due = due_at(entry);
	const std::string blob =
		item_blob.empty() ?
			"NULL" :
			"UNHEX('" + hex_bytes(item_blob.data(), item_blob.size()) + "')";
	execute("INSERT INTO collector_listings(listing_id,death_operation_id,beneficiary_pid,"
		"item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,"
		"record_blob,item_blob) VALUES(" +
		std::to_string(entry.listing) + ",UNHEX('" + entry.death_operation.data() + "')," +
		std::to_string(entry.beneficiary) + "," + std::to_string(entry.uid) + "," +
		std::to_string(static_cast<unsigned int>(entry.status)) + "," +
		std::to_string(entry.holding_paused ? 1 : 0) + "," +
		(due ? std::to_string(due) : "NULL") + "," + std::to_string(entry.revision) + "," +
		std::to_string(entry.item_revision) + "," + std::to_string(entry.price_value) +
		",UNHEX('" + hex_bytes(record.data(), record.size()) + "')," + blob + ")");
}

collector::record candidate(uint64_t listing, uint64_t uid, uint64_t item_revision,
			    critical_operation_id *death)
{
	*death = operation();
	seed_death(*death);
	collector::record entry;
	assert(collector::enroll(listing, operation_hex(*death), PLAYER_PID, uid, item_revision,
				 DEATH_TIME, rules(), &entry) == collector::outcome::applied);
	return entry;
}

player_item_snapshot item_snapshot(uint64_t uid, int32_t vnum, int32_t cost = 250)
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	item.object_uid = uid;
	item.generated_key = 77;
	item.vnum = vnum;
	item.type = 15;
	item.string_mask = 15;
	item.name = "collector harness relic";
	item.short_description = "a collector harness relic";
	item.description = "A collector harness relic rests here.";
	item.action_description = "collector-harness-action";
	item.values[1] = 9;
	item.values[2] = 7;
	item.timers.fill(-1);
	item.wear_flags = 17;
	item.extra_flags = 33;
	item.weight = 5;
	item.material = 4;
	item.cost = cost;
	item.condition = 77;
	item.craftsmanship = 8;
	item.bitvectors[0] = 3;
	item.affects[0] = { 1, 7 };
	item.extra_descriptions.push_back({ "runes", "Fine runes cover it.", false, {} });
	return item;
}

std::vector<uint8_t> encode_item(const player_item_snapshot &item)
{
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode({ item }, &encoded) ==
	       player_snapshot_codec_result::ok);
	assert(!encoded.empty() && encoded.size() <= COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES);
	return encoded;
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

void seed_authority(uint64_t uid, int32_t vnum, const item_owner_identity &owner, uint64_t revision)
{
	(void)owner_revision(owner);
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(uid) + "," + std::to_string(uid) + ",NULL," +
		std::to_string(static_cast<unsigned int>(owner.type)) + "," +
		std::to_string(owner.id) + "," + std::to_string(owner.context_id) + "," +
		std::to_string(revision) + "," + std::to_string(vnum) + ",1)");
}

critical_command command_for(const collector_command_payload &payload)
{
	critical_command command = {};
	assert(collector_command_build(&command, operation(), payload,
				       payload.actor_pid ? critical_source_site::command :
							   critical_source_site::zone_event,
				       payload.actor_pid ? critical_deadline_class::interactive :
							   critical_deadline_class::background));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	return command;
}

collector_command_result apply(const critical_command &command,
			       critical_apply_outcome expected = critical_apply_outcome::applied,
			       unsigned int expected_error = 0)
{
	const critical_apply_result applied = critical_command_repository_apply(database, command);
	if (applied.outcome != expected || applied.error_code != expected_error)
		fprintf(stderr, "collector apply mismatch: outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(database), mysql_error(database));
	assert(applied.outcome == expected && applied.error_code == expected_error);
	collector_command_result result;
	assert(collector_command_decode_result(applied.result_payload.data(), applied.result_size,
					       &result));
	return result;
}

collector_command_payload metadata(collector_action action, const collector::record &entry,
				   uint64_t observed_at)
{
	collector_command_payload payload;
	payload.action = action;
	payload.listing = entry.listing;
	payload.expected_listing_revision = entry.revision;
	payload.observed_at = observed_at;
	return payload;
}

collector::record seed_held(uint64_t listing, uint64_t uid, int32_t vnum,
			    const std::vector<uint8_t> &blob, bool available)
{
	critical_operation_id death;
	collector::record entry = candidate(listing, uid, 5, &death);
	assert(collector::collect(&entry, 1, 5, 5, true, 250, entry.collect_at) ==
	       collector::outcome::applied);
	if (available)
		assert(collector::activate(&entry, 2, entry.sale_at) ==
		       collector::outcome::applied);
	seed_listing(entry, blob);
	seed_authority(uid, vnum, { item_owner_type::collector, listing, 0 }, entry.item_revision);
	return entry;
}
} // namespace

int main()
{
	database = mysql_init(nullptr);
	assert(database);
	assert(mysql_real_connect(
		database, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("COLLECTOR_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	execute("INSERT INTO accounts(account_name,password) VALUES('" + std::string(ACCOUNT_NAME) +
		"','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES(" +
		std::to_string(PLAYER_PID) + ",'" + PLAYER_NAME + "','" + ACCOUNT_NAME +
		"',1,0,0,10,0)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		"bank_platinum) VALUES('" +
		std::string(ACCOUNT_NAME) + "',1,0,0,0,0)");
	execute("UPDATE collector_catalog_state SET next_listing=20000 WHERE state_id=1");
	collector::catalog empty_catalog;
	assert(collector_repository_read_catalog(database, &empty_catalog));
	assert(empty_catalog.revision == 0 && empty_catalog.next_listing == 20000 &&
	       empty_catalog.records.empty());

	// Collect a container shell from the middle of a corpse tree. Its direct child remains
	// in the corpse, reparented to the shell's parent, and only the shell's own weight leaves.
	constexpr uint64_t LISTING = 7001, ROOT_UID = 910000001, ITEM_UID = 910000002,
			   CHILD_UID = 910000003, SIBLING_UID = 910000004;
	constexpr uint32_t SAVE_ID = 12345;
	critical_operation_id death;
	collector::record entry = candidate(LISTING, ITEM_UID, 5, &death);
	seed_listing(entry);
	execute("INSERT INTO corpses(player_name,save_id,room_vnum) VALUES('" +
		std::string(PLAYER_NAME) + "'," + std::to_string(SAVE_ID) + ",3001)");
	const uint64_t corpse_id = mysql_insert_id(database);
	execute("INSERT INTO corpse_items(corpse_id,vnum,container_id,quantity,weight,cost,obj_uid,"
		"item_condition) VALUES(" +
		std::to_string(corpse_id) + ",1001,NULL,1,30,100," + std::to_string(ROOT_UID) +
		",90)");
	const uint64_t root_row = mysql_insert_id(database);
	execute("INSERT INTO corpse_items(corpse_id,vnum,container_id,quantity,weight,cost,obj_uid,"
		"item_condition) VALUES(" +
		std::to_string(corpse_id) + ",1002," + std::to_string(root_row) + ",1,20,250," +
		std::to_string(ITEM_UID) + ",77)");
	const uint64_t selected_row = mysql_insert_id(database);
	execute("INSERT INTO corpse_items(corpse_id,vnum,container_id,quantity,weight,cost,obj_uid,"
		"item_condition) VALUES(" +
		std::to_string(corpse_id) + ",1003," + std::to_string(selected_row) + ",1,15,50," +
		std::to_string(CHILD_UID) + ",80)");
	const uint64_t child_row = mysql_insert_id(database);
	execute("INSERT INTO corpse_items(corpse_id,vnum,container_id,quantity,weight,cost,obj_uid,"
		"item_condition) VALUES(" +
		std::to_string(corpse_id) + ",1004," + std::to_string(root_row) + ",1,5,20," +
		std::to_string(SIBLING_UID) + ",100)");
	const item_owner_identity corpse = { item_owner_type::corpse,
					     item_corpse_owner_id(PLAYER_PID, SAVE_ID), 0 };
	const item_owner_identity collector = { item_owner_type::collector, LISTING, 0 };
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(4," +
		std::to_string(corpse.id) + ",0,3),(10," + std::to_string(LISTING) + ",0,0)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ROOT_UID) + "," + std::to_string(ROOT_UID) + ",NULL,4," +
		std::to_string(corpse.id) + ",0,5,1001,1),(" + std::to_string(ITEM_UID) + "," +
		std::to_string(ROOT_UID) + "," + std::to_string(ROOT_UID) + ",4," +
		std::to_string(corpse.id) + ",0,5,1002,1),(" + std::to_string(CHILD_UID) + "," +
		std::to_string(ROOT_UID) + "," + std::to_string(ITEM_UID) + ",4," +
		std::to_string(corpse.id) + ",0,5,1003,1),(" + std::to_string(SIBLING_UID) + "," +
		std::to_string(ROOT_UID) + "," + std::to_string(ROOT_UID) + ",4," +
		std::to_string(corpse.id) + ",0,5,1004,1)");
	const player_item_snapshot exact = item_snapshot(ITEM_UID, 1002);
	const std::vector<uint8_t> blob = encode_item(exact);
	collector_command_payload collect;
	collect.action = collector_action::collect;
	collect.listing = LISTING;
	collect.expected_listing_revision = 1;
	collect.observed_at = entry.collect_at;
	collect.from_owner = corpse;
	collect.to_owner = collector;
	collect.expected_from_owner_revision = 3;
	collect.expected_to_owner_revision = 0;
	collect.selected_item_uid = ITEM_UID;
	collect.target_state = item_custody_state::active;
	collect.items[collect.item_count++] = { ROOT_UID, ROOT_UID, 0,
						5,	  1001,	    item_custody_state::active };
	collect.items[collect.item_count++] = { ITEM_UID, ROOT_UID, ROOT_UID,
						5,	  1002,	    item_custody_state::active };
	collect.items[collect.item_count++] = { CHILD_UID, ROOT_UID, ITEM_UID,
						5,	   1003,     item_custody_state::active };
	collect.items[collect.item_count++] = { SIBLING_UID, ROOT_UID, ROOT_UID,
						5,	     1004,     item_custody_state::active };
	collect.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), collect.item_blob.begin());
	const critical_command collect_command = command_for(collect);
	collector_command_result result = apply(collect_command);
	entry = result.entry;
	assert(entry.status == collector::state::collected && entry.revision == 2 &&
	       entry.item_revision == 6 && entry.price_value == 500);
	assert(scalar("SELECT COUNT(*) FROM corpse_items WHERE id=" +
		      std::to_string(selected_row)) == 0);
	assert(scalar("SELECT container_id FROM corpse_items WHERE id=" +
		      std::to_string(child_row)) == root_row);
	assert(scalar("SELECT weight FROM corpse_items WHERE id=" + std::to_string(root_row)) ==
	       25);
	assert(text("SELECT CONCAT(owner_type,':',owner_id,':',root_item_uid,':',"
		    "IFNULL(parent_item_uid,0),':',item_revision) FROM item_current_owner WHERE "
		    "item_uid=" +
		    std::to_string(ITEM_UID)) == "10:7001:910000002:0:6");
	assert(text("SELECT CONCAT(root_item_uid,':',parent_item_uid,':',item_revision) FROM "
		    "item_current_owner WHERE item_uid=" +
		    std::to_string(CHILD_UID)) == "910000001:910000001:6");
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(collect_command.operation_id) + "')") == 4);
	assert(scalar("SELECT OCTET_LENGTH(item_blob) FROM collector_listings WHERE listing_id=" +
		      std::to_string(LISTING)) == blob.size());
	const critical_apply_result duplicate =
		critical_command_repository_apply(database, collect_command);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(scalar("SELECT COUNT(*) FROM collector_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(collect_command.operation_id) + "')") == 1);

	// Timing fences are durable terminal receipts and do not advance the catalog.
	const uint64_t catalog_after_collect = result.catalog_revision;
	apply(command_for(metadata(collector_action::activate, entry, entry.sale_at - 1)),
	      critical_apply_outcome::terminal_failure, EAGAIN);
	assert(scalar("SELECT catalog_revision FROM collector_catalog_state WHERE state_id=1") ==
	       catalog_after_collect);
	result = apply(command_for(metadata(collector_action::activate, entry, entry.sale_at)));
	entry = result.entry;
	assert(entry.status == collector::state::available && entry.revision == 3 &&
	       entry.available_at == entry.sale_at && entry.expires_at > entry.available_at);

	// Purchase debits carried currency, advances both revision fences, persists the exact
	// recoverable item row, and transfers custody to the permanent beneficiary.
	collector_command_payload purchase;
	purchase.action = collector_action::purchase;
	purchase.listing = LISTING;
	purchase.expected_listing_revision = entry.revision;
	purchase.observed_at = entry.available_at;
	purchase.actor_pid = PLAYER_PID;
	purchase.racewar = 1;
	memcpy(purchase.account_name.data(), ACCOUNT_NAME, strlen(ACCOUNT_NAME));
	purchase.expected_wallet_revision = 0;
	purchase.expected_bank_revision = 0;
	purchase.capacity_admitted = true;
	purchase.from_owner = collector;
	purchase.to_owner = { item_owner_type::player, PLAYER_PID, 0 };
	purchase.expected_from_owner_revision = 1;
	purchase.expected_to_owner_revision = owner_revision(purchase.to_owner);
	purchase.selected_item_uid = ITEM_UID;
	purchase.target_state = item_custody_state::active;
	purchase.item_count = 1;
	purchase.items[0] = { ITEM_UID, ITEM_UID, 0, 6, 1002, item_custody_state::active };
	purchase.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), purchase.item_blob.begin());
	const critical_command purchase_command = command_for(purchase);
	result = apply(purchase_command);
	entry = result.entry;
	assert(entry.status == collector::state::purchased && entry.revision == 4 &&
	       entry.item_revision == 7);
	assert(text("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		    "FROM player_data WHERE pid=" +
		    std::to_string(PLAYER_PID)) == "0:0:5:0:1");
	assert(scalar("SELECT bank_revision FROM account_banks WHERE account_name='" +
		      std::string(ACCOUNT_NAME) + "' AND racewar=1") == 1);
	assert(text("SELECT CONCAT(vnum,':',weight,':',cost,':',value2,':',item_condition) FROM "
		    "player_items WHERE obj_uid=" +
		    std::to_string(ITEM_UID)) == "1002:5:250:7:77");
	assert(scalar("SELECT COUNT(*) FROM player_item_affects a JOIN player_items i ON "
		      "i.id=a.item_id WHERE i.obj_uid=" +
		      std::to_string(ITEM_UID) + " AND a.location=1 AND a.modifier=7") == 1);
	assert(scalar("SELECT COUNT(*) FROM player_item_extra_descr e JOIN player_items i ON "
		      "i.id=e.item_id WHERE i.obj_uid=" +
		      std::to_string(ITEM_UID) + " AND e.keyword='runes'") == 1);
	assert(scalar("SELECT reason_type FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(purchase_command.operation_id) + "')") ==
	       static_cast<unsigned int>(currency_reason_type::collector_purchase));
	assert(critical_command_repository_apply(database, purchase_command).outcome ==
	       critical_apply_outcome::already_applied);

	// Pause/resume extends the holding interval, and early expiry cannot mutate custody.
	constexpr uint64_t EXPIRE_LISTING = 7002, EXPIRE_UID = 920000001;
	const std::vector<uint8_t> expire_blob = encode_item(item_snapshot(EXPIRE_UID, 1101));
	collector::record expiring = seed_held(EXPIRE_LISTING, EXPIRE_UID, 1101, expire_blob, true);
	result = apply(command_for(
		metadata(collector_action::pause, expiring, expiring.available_at + 10)));
	expiring = result.entry;
	assert(expiring.holding_paused && expiring.paused_at == expiring.available_at + 10);
	result = apply(
		command_for(metadata(collector_action::resume, expiring, expiring.paused_at + 20)));
	expiring = result.entry;
	assert(!expiring.holding_paused && !expiring.paused_at);
	const uint64_t extended_expiry = expiring.expires_at;
	collector_command_payload expire;
	expire.action = collector_action::expire;
	expire.listing = EXPIRE_LISTING;
	expire.expected_listing_revision = expiring.revision;
	expire.observed_at = extended_expiry - 1;
	expire.from_owner = { item_owner_type::collector, EXPIRE_LISTING, 0 };
	expire.to_owner = { item_owner_type::destruction, 0, 0 };
	expire.expected_from_owner_revision = 0;
	expire.expected_to_owner_revision = owner_revision(expire.to_owner);
	expire.selected_item_uid = EXPIRE_UID;
	expire.target_state = item_custody_state::destroyed;
	expire.item_count = 1;
	expire.items[0] = { EXPIRE_UID, EXPIRE_UID,
			    0,		expiring.item_revision,
			    1101,	item_custody_state::active };
	expire.item_blob_size = static_cast<uint32_t>(expire_blob.size());
	std::copy(expire_blob.begin(), expire_blob.end(), expire.item_blob.begin());
	apply(command_for(expire), critical_apply_outcome::terminal_failure, EAGAIN);
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(EXPIRE_UID)) == 10);
	expire.observed_at = extended_expiry;
	result = apply(command_for(expire));
	assert(result.entry.status == collector::state::expired &&
	       result.entry.closed_reason == collector::reason::holding_elapsed);
	assert(text("SELECT CONCAT(owner_type,':',state,':',item_revision) FROM item_current_owner "
		    "WHERE item_uid=" +
		    std::to_string(EXPIRE_UID)) == "8:2:7");

	// A claimant cancels a candidate without moving the item. A held corruption closure moves
	// the exact singleton into quarantine, with both cases preserving their audit reason.
	constexpr uint64_t CLAIM_LISTING = 7003, CLAIM_UID = 930000001;
	critical_operation_id claim_death;
	collector::record claimed = candidate(CLAIM_LISTING, CLAIM_UID, 2, &claim_death);
	seed_listing(claimed);
	collector_command_payload claim =
		metadata(collector_action::cancel, claimed, claimed.death_time + 1);
	claim.cancel_reason = collector::reason::claimed;
	result = apply(command_for(claim));
	assert(result.entry.status == collector::state::cancelled &&
	       result.entry.closed_reason == collector::reason::claimed &&
	       result.entry.item_revision == claimed.item_revision);

	constexpr uint64_t QUARANTINE_LISTING = 7004, QUARANTINE_UID = 940000001;
	const std::vector<uint8_t> quarantine_blob =
		encode_item(item_snapshot(QUARANTINE_UID, 1201));
	collector::record quarantined =
		seed_held(QUARANTINE_LISTING, QUARANTINE_UID, 1201, quarantine_blob, false);
	collector_command_payload quarantine;
	quarantine.action = collector_action::cancel;
	quarantine.cancel_reason = collector::reason::quarantined;
	quarantine.listing = QUARANTINE_LISTING;
	quarantine.expected_listing_revision = quarantined.revision;
	quarantine.observed_at = quarantined.collect_at + 1;
	quarantine.from_owner = { item_owner_type::collector, QUARANTINE_LISTING, 0 };
	quarantine.to_owner = { item_owner_type::system, 0, 0 };
	quarantine.expected_from_owner_revision = 0;
	quarantine.expected_to_owner_revision = owner_revision(quarantine.to_owner);
	quarantine.selected_item_uid = QUARANTINE_UID;
	quarantine.target_state = item_custody_state::quarantined;
	quarantine.item_count = 1;
	quarantine.items[0] = { QUARANTINE_UID,
				QUARANTINE_UID,
				0,
				quarantined.item_revision,
				1201,
				item_custody_state::active };
	quarantine.item_blob_size = static_cast<uint32_t>(quarantine_blob.size());
	std::copy(quarantine_blob.begin(), quarantine_blob.end(), quarantine.item_blob.begin());
	result = apply(command_for(quarantine));
	assert(result.entry.status == collector::state::cancelled &&
	       result.entry.closed_reason == collector::reason::quarantined);
	assert(text("SELECT CONCAT(owner_type,':',state,':',item_revision) FROM item_current_owner "
		    "WHERE item_uid=" +
		    std::to_string(QUARANTINE_UID)) == "7:3:7");

	assert(scalar("SELECT COUNT(*) FROM collector_ledger") == 8);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE destination=11 AND event_type=1") ==
	       8);

	// Restart bootstrap observes one canonical catalog snapshot and leaves the
	// caller's prior snapshot untouched if a fixed-width record is corrupted.
	collector::catalog catalog;
	assert(collector_repository_read_catalog(database, &catalog));
	assert(catalog.revision == 8 && catalog.next_listing == 20000 &&
	       catalog.records.size() == 4);
	assert(catalog.records[0].listing == LISTING &&
	       catalog.records[0].status == collector::state::purchased);
	assert(catalog.records[1].listing == EXPIRE_LISTING &&
	       catalog.records[1].status == collector::state::expired);
	assert(catalog.records[2].listing == CLAIM_LISTING &&
	       catalog.records[2].closed_reason == collector::reason::claimed);
	assert(catalog.records[3].listing == QUARANTINE_LISTING &&
	       catalog.records[3].closed_reason == collector::reason::quarantined);
	execute("UPDATE collector_listings SET record_blob=REPEAT(CHAR(0),154) WHERE listing_id=" +
		std::to_string(QUARANTINE_LISTING));
	errno = 0;
	assert(!collector_repository_read_catalog(database, &catalog) && errno == EBADMSG);
	assert(catalog.revision == 8 && catalog.records.size() == 4 &&
	       catalog.records.back().listing == QUARANTINE_LISTING);
	mysql_close(database);
	return 0;
}
