#include "persistence/critical_command_repository.h"
#include "persistence/quest_reward_obligation_repository.h"
#include "economic_sql_coordinator_fixture.h"
#include "economy/item_transfer_accounting.h"
#include "item/item_transfer_command.h"
#include "item/item_transfer_repository.h"
#include "item/item_uid_allocator.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "player/player_load_repository.h"
#include "persistence/persistence_observability.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <span>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
MYSQL *open_pool_test_connection()
{
	const char *host = getenv("DB_HOST");
	const char *user = getenv("DB_USER");
	const char *password = getenv("DB_PASSWD");
	const char *database = getenv("ITEM_TRANSFER_TEST_DB_NAME");
	const char *port_value = getenv("DB_PORT");
	if (!host || !user || !password || !database)
		return nullptr;
	MYSQL *pooled = mysql_init(nullptr);
	if (!pooled)
		return nullptr;
	const unsigned int port =
		port_value ? static_cast<unsigned int>(strtoul(port_value, nullptr, 10)) : 3306;
	if (!mysql_real_connect(pooled, host, user, password, database, port, nullptr, 0))
	{
		mysql_close(pooled);
		return nullptr;
	}
	return pooled;
}
} // namespace

unsigned long next_obj_uid = 1;
extern "C" MYSQL *sql_pool_acquire(void)
{
	return open_pool_test_connection();
}
extern "C" void sql_pool_release(MYSQL *pooled)
{
	if (pooled)
		mysql_close(pooled);
}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *pooled)
{
	if (pooled)
		mysql_close(pooled);
	return open_pool_test_connection();
}
extern "C" void sql_pool_discard_connection(MYSQL *pooled)
{
	if (pooled)
		mysql_close(pooled);
}

namespace
{
uint64_t root_uid = 0;
uint64_t child_uid = 0;
critical_operation_id run_operation = {};

critical_operation_id operation(uint8_t value)
{
	critical_operation_id id = {};
	id = run_operation;
	id.bytes[15] ^= value;
	return id;
}

std::string operation_hex(uint8_t value)
{
	char output[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation(value), output, sizeof(output)));
	return output;
}

std::string operation_hex(const critical_operation_id &value)
{
	char output[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(value, output, sizeof(output)));
	return output;
}

std::string bytes_hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result;
	result.reserve(bytes.size() * 2);
	for (uint8_t byte : bytes)
	{
		result.push_back(digits[byte >> 4]);
		result.push_back(digits[byte & 0x0f]);
	}
	return result;
}

void execute(MYSQL *connection, const char *sql)
{
	if (mysql_real_query(connection, sql, strlen(sql)) != 0)
	{
		fprintf(stderr, "SQL failed: %s\nquery: %s\n", mysql_error(connection), sql);
		assert(false);
	}
}

void execute(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql.c_str());
}

void ensure_collector_boundary_fixture(MYSQL *connection)
{
	execute(connection,
		"CREATE TABLE IF NOT EXISTS collector_listings("
		"listing_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,"
		"item_uid BIGINT UNSIGNED NOT NULL,status TINYINT UNSIGNED NOT NULL,"
		"KEY idx_collector_item_history(item_uid,status,listing_id)) ENGINE=InnoDB");
}

uint64_t scalar(MYSQL *connection, const char *sql)
{
	execute(connection, sql);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	uint64_t value = strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

uint64_t owner_revision(MYSQL *connection, const item_owner_identity &owner)
{
	char query[512];
	snprintf(
		query, sizeof(query),
		"INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(%u,%llu,%llu,0)",
		static_cast<unsigned int>(owner.type), static_cast<unsigned long long>(owner.id),
		static_cast<unsigned long long>(owner.context_id));
	execute(connection, query);
	snprintf(query, sizeof(query),
		 "SELECT revision FROM item_owner_revision WHERE owner_type=%u AND owner_id=%llu "
		 "AND owner_context_id=%llu",
		 static_cast<unsigned int>(owner.type), static_cast<unsigned long long>(owner.id),
		 static_cast<unsigned long long>(owner.context_id));
	return scalar(connection, query);
}

player_item_snapshot physical_item(uint64_t uid, int32_t vnum, int32_t parent_index)
{
	player_item_snapshot item = {};
	item.parent_index = parent_index;
	item.equipment_slot = 0;
	item.object_uid = uid;
	item.vnum = vnum;
	item.type = 15;
	item.string_mask = 0x0f;
	item.name = "durable item " + std::to_string(uid);
	item.short_description = "a durable item";
	item.description = "A durable item is here.";
	item.action_description = "durable";
	item.values = { 1, 2, 3, 4, 5, 6, 7, 8 };
	item.timers = { -1, 11, 12, 13, 14, 15 };
	item.wear_flags = 1;
	item.extra_flags = 2;
	item.weight = 3;
	item.material = 4;
	item.cost = 5;
	item.condition = 97;
	item.bitvectors = { 6, 7, 8, 9, 10 };
	item.affects[0] = { 1, 12 };
	item.extra_descriptions.push_back({ "mark", "persistent", false, {} });
	return item;
}

void attach_blob(item_transfer_payload *value, const std::vector<player_item_snapshot> &snapshots)
{
	std::vector<uint8_t> encoded;
	assert(value &&
	       player_item_snapshot_list_encode(snapshots, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       !encoded.empty() && encoded.size() <= value->item_blob.size());
	value->item_blob_size = static_cast<uint32_t>(encoded.size());
	std::copy(encoded.begin(), encoded.end(), value->item_blob.begin());
}

item_transfer_payload payload(item_owner_identity from, item_owner_identity to,
			      item_transfer_reason reason, uint64_t from_revision,
			      uint64_t to_revision, uint64_t item_revision, uint16_t count = 2)
{
	item_transfer_payload value = {};
	value.from_owner = from;
	value.to_owner = to;
	value.reason = reason;
	value.reason_id = 77;
	value.expected_from_revision = from_revision;
	value.expected_to_revision = to_revision;
	value.item_count = count;
	value.items[0] = { root_uid,
			   root_uid,
			   0,
			   item_revision,
			   1001,
			   reason == item_transfer_reason::creation ? item_custody_state::absent :
								      item_custody_state::active };
	if (count == 2)
		value.items[1] = { child_uid,
				   root_uid,
				   root_uid,
				   item_revision,
				   1002,
				   reason == item_transfer_reason::creation ?
					   item_custody_state::absent :
					   item_custody_state::active };
	std::vector<player_item_snapshot> snapshots = { physical_item(root_uid, 1001,
								      PLAYER_SNAPSHOT_NO_PARENT) };
	if (count == 2)
		snapshots.push_back(physical_item(child_uid, 1002, 0));
	attach_blob(&value, snapshots);
	return value;
}

item_transfer_payload multi_root_creation_payload(uint64_t first_root, uint64_t first_child,
						  uint64_t second_root, uint64_t from_revision,
						  uint64_t to_revision)
{
	item_transfer_payload value = {};
	value.from_owner = { item_owner_type::system, 0, 0 };
	value.to_owner = { item_owner_type::player, 4000000001, 0 };
	value.reason = item_transfer_reason::creation;
	value.reason_id = 181;
	value.expected_from_revision = from_revision;
	value.expected_to_revision = to_revision;
	value.multi_root = true;
	value.item_count = 3;
	value.items[0] = { first_root, first_root,
			   0,	       ITEM_TRANSFER_ABSENT_REVISION,
			   1101,       item_custody_state::absent };
	value.items[1] = { first_child, first_root,
			   first_root,	ITEM_TRANSFER_ABSENT_REVISION,
			   1102,	item_custody_state::absent };
	value.items[2] = { second_root, second_root,
			   0,		ITEM_TRANSFER_ABSENT_REVISION,
			   1103,	item_custody_state::absent };
	attach_blob(&value, { physical_item(first_root, 1101, PLAYER_SNAPSHOT_NO_PARENT),
			      physical_item(first_child, 1102, 0),
			      physical_item(second_root, 1103, PLAYER_SNAPSHOT_NO_PARENT) });
	return value;
}

critical_apply_result apply(MYSQL *connection, uint8_t id, const item_transfer_payload &value)
{
	critical_command command = {};
	assert(item_transfer_command_build(&command, operation(id), value,
					   critical_source_site::operator_repair,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return critical_command_repository_apply(connection, command);
}

critical_command accounted_item_transfer(critical_operation_id id,
					 const item_transfer_payload &value,
					 const critical_operation_id &lineage,
					 const critical_operation_id &epoch, uint32_t actor_pid,
					 economic_source_kind creation_source = {})
{
	critical_command command = {};
	assert(item_transfer_command_build(&command, id, value, critical_source_site::command,
					   critical_deadline_class::interactive));
	std::vector<uint8_t> intent;
	assert(item_transfer_accounting_intent(command, lineage, epoch, actor_pid, &intent,
					       creation_source) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.accepted_at_usec = 1;
	assert(item_transfer_accounting_command_supported(command));
	return command;
}

player_item_snapshot runtime_item(uint64_t uid, int64_t generated_key, int64_t timer)
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = -1;
	item.object_uid = uid;
	item.generated_key = generated_key;
	item.vnum = 1901;
	item.type = 1;
	item.string_mask = 0x0f;
	item.name = "restitution item";
	item.short_description = "restitution short";
	item.description = "restitution description";
	item.action_description = "restitution action";
	item.values = { 11, 12, 13, 14, 15, 16, 17, 18 };
	item.timers = { timer, timer + 1, timer + 2, timer + 3, timer + 4, timer + 5 };
	item.wear_flags = 0x10203040;
	item.extra_flags = 0x50607080;
	item.anti_flags = 0x11223344;
	item.anti2_flags = 0x55667788;
	item.extra2_flags = 0x99aabbcc;
	item.weight = 27;
	item.material = 3;
	item.cost = 4567;
	item.condition = 89;
	item.craftsmanship = 31;
	item.bitvectors = { 21, 22, 23, 24, 25 };
	for (size_t index = 0; index < item.affects.size(); ++index)
		item.affects[index] = { static_cast<int16_t>(index + 1),
					static_cast<int16_t>(index + 11) };
	item.dynamic_affects.push_back({ 7, 41, 42 });
	item.extra_descriptions.push_back({ "SPELLBOOK", "", true, { 4, 9, 15 } });
	return item;
}

std::vector<uint8_t> encode_runtime_item(const player_item_snapshot &item)
{
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode({ item }, &encoded) ==
	       player_snapshot_codec_result::ok);
	return encoded;
}

std::vector<uint8_t> read_blob(MYSQL *connection, const char *sql)
{
	execute(connection, sql);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const unsigned long *lengths = mysql_fetch_lengths(rows);
	assert(lengths);
	std::vector<uint8_t> blob(reinterpret_cast<const uint8_t *>(row[0]),
				  reinterpret_cast<const uint8_t *>(row[0]) + lengths[0]);
	mysql_free_result(rows);
	return blob;
}

void prepare_restitution_runtime_fixture(MYSQL *connection, uint64_t uid,
					 const std::vector<uint8_t> &initial_payload)
{
	execute(connection, "CREATE TABLE IF NOT EXISTS player_death_restitution_receipt ("
			    "restitution_id BINARY(16) NOT NULL PRIMARY KEY) ENGINE=InnoDB");
	execute(connection,
		"CREATE TABLE IF NOT EXISTS player_death_restitution_item ("
		"restitution_id BINARY(16) NOT NULL,item_uid BIGINT UNSIGNED NOT NULL,"
		"vnum INT NOT NULL,PRIMARY KEY(restitution_id,item_uid),"
		"FOREIGN KEY(restitution_id) REFERENCES player_death_restitution_receipt(restitution_id))"
		" ENGINE=InnoDB");
	execute(connection,
		"CREATE TABLE IF NOT EXISTS player_death_restitution_delivery ("
		"item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,restitution_id BINARY(16) NOT NULL,"
		"source_pid INT NOT NULL,death_revision BIGINT UNSIGNED NOT NULL,recipient_pid INT NOT NULL,"
		"source_item_revision BIGINT UNSIGNED NOT NULL,delivered_item_revision BIGINT UNSIGNED NOT NULL,"
		"delivered_item_id INT UNSIGNED NOT NULL,metadata_digest BINARY(32) NOT NULL,"
		"original_payload MEDIUMBLOB NOT NULL,"
		"FOREIGN KEY(restitution_id,item_uid) REFERENCES player_death_restitution_item(restitution_id,item_uid))"
		" ENGINE=InnoDB");
	execute(connection,
		"CREATE TABLE IF NOT EXISTS player_death_restitution_runtime ("
		"item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,recipient_pid INT NOT NULL,"
		"state_payload MEDIUMBLOB NOT NULL,state_digest BINARY(32) NOT NULL,"
		"FOREIGN KEY(item_uid) REFERENCES player_death_restitution_delivery(item_uid))"
		" ENGINE=InnoDB");
	const std::string payload_hex = [&]
	{
		static const char digits[] = "0123456789abcdef";
		std::string result;
		result.reserve(initial_payload.size() * 2);
		for (uint8_t byte : initial_payload)
		{
			result.push_back(digits[byte >> 4]);
			result.push_back(digits[byte & 0x0f]);
		}
		return result;
	}();
	const std::string id = "UNHEX('" + operation_hex(operation(0xa1)) + "')";
	const std::string death_operation_id = "UNHEX('" + operation_hex(operation(0xb1)) + "')";
	execute(connection,
		"INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,"
		"death_revision,recipient_pid,death_operation_id,evidence_digest,plan_digest,"
		"status,actor,reason) VALUES (" +
			id + ",99,1,41," + death_operation_id +
			",UNHEX(REPEAT('c1',32)),"
			"UNHEX(SHA2(" +
			id + ",256)),2,'item-transfer-test','runtime update')");
	execute(connection,
		"INSERT INTO player_death_restitution_item(restitution_id,item_uid,vnum,"
		"disposition,classification,metadata_digest,metadata_payload) VALUES (" +
			id + "," + std::to_string(uid) +
			",1901,1,'runtime',UNHEX(REPEAT('11',32)),UNHEX('" + payload_hex + "'))");
	execute(connection,
		"INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,"
		"death_revision,recipient_pid,source_item_revision,delivered_item_revision,"
		"delivered_item_id,metadata_digest,original_payload) VALUES (" +
			std::to_string(uid) + "," + id +
			",99,1,41,1,1,1901,REPEAT(0x11,32),UNHEX('" + payload_hex + "'))");
	execute(connection,
		"INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,"
		"state_digest) VALUES (" +
			std::to_string(uid) + ",41,UNHEX('" + payload_hex +
			"'),UNHEX(SHA2(UNHEX('" + payload_hex + "'),256)))");
}

void check_restitution_runtime_transfer(MYSQL *connection)
{
	uint64_t uid = 0;
	for (size_t index = 0; index < sizeof(uid); ++index)
		uid = (uid << 8) | run_operation.bytes[index];
	if (!uid)
		uid = 9000001;
	const item_owner_identity source = { item_owner_type::player, 41, 0 };
	const item_owner_identity target = { item_owner_type::player, 42, 0 };
	const player_item_snapshot initial = runtime_item(uid, 7, 1700000000);
	const player_item_snapshot mutated = runtime_item(uid, 7007, 1900000000);
	const std::vector<uint8_t> initial_payload = encode_runtime_item(initial);
	const std::vector<uint8_t> mutated_payload = encode_runtime_item(mutated);
	prepare_restitution_runtime_fixture(connection, uid, initial_payload);
	execute(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES (" +
			std::to_string(uid) + "," + std::to_string(uid) + ",NULL,1,41,0,1,1901,1)");
	const uint64_t source_revision = owner_revision(connection, source);
	const uint64_t target_revision = owner_revision(connection, target);
	item_transfer_payload craft = {};
	craft.from_owner = source;
	craft.to_owner = source;
	craft.reason = item_transfer_reason::craft;
	craft.reason_id = 9002;
	craft.expected_from_revision = source_revision;
	craft.expected_to_revision = source_revision;
	craft.selected_item_uid = uid;
	craft.target_root_item_uid = 0;
	craft.multi_root = true;
	craft.item_count = 1;
	craft.items[0] = { uid, uid, 0, 1, 1901, item_custody_state::active };
	const critical_apply_result rejected_craft = apply(connection, 16, craft);
	assert(rejected_craft.outcome == critical_apply_outcome::terminal_failure &&
	       rejected_craft.error_code == EPERM);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		       std::to_string(uid) + " AND owner_id=41 AND state=1 AND item_revision=1")
			      .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid=" +
		       std::to_string(uid))
			      .c_str()) == 1);
	const std::vector<uint8_t> preserved_payload = read_blob(
		connection,
		("SELECT state_payload FROM player_death_restitution_runtime WHERE item_uid=" +
		 std::to_string(uid))
			.c_str());
	assert(preserved_payload == initial_payload);
	assert(owner_revision(connection, source) == source_revision);
	item_transfer_payload transfer = {};
	transfer.from_owner = source;
	transfer.to_owner = target;
	transfer.reason = item_transfer_reason::player_give;
	transfer.reason_id = 9001;
	transfer.expected_from_revision = source_revision;
	transfer.expected_to_revision = target_revision;
	transfer.selected_item_uid = uid;
	transfer.target_root_item_uid = uid;
	transfer.item_count = 1;
	transfer.items[0] = { uid, uid, 0, 1, 1901, item_custody_state::active };
	transfer.item_blob_size = static_cast<uint32_t>(mutated_payload.size());
	std::copy(mutated_payload.begin(), mutated_payload.end(), transfer.item_blob.begin());
	const critical_apply_result moved = apply(connection, 15, transfer);
	assert(moved.outcome == critical_apply_outcome::applied && moved.error_code == 0);
	const std::vector<uint8_t> stored_payload = read_blob(
		connection,
		("SELECT state_payload FROM player_death_restitution_runtime WHERE item_uid=" +
		 std::to_string(uid))
			.c_str());
	std::vector<player_item_snapshot> stored;
	assert(player_item_snapshot_list_decode(stored_payload.data(), stored_payload.size(),
						&stored) == player_snapshot_codec_result::ok &&
	       stored.size() == 1);
	const player_item_snapshot &actual = stored[0];
	assert(actual.parent_index == PLAYER_SNAPSHOT_NO_PARENT && actual.equipment_slot == -1);
	assert(actual.object_uid == mutated.object_uid &&
	       actual.generated_key == mutated.generated_key && actual.vnum == mutated.vnum &&
	       actual.type == mutated.type && actual.string_mask == mutated.string_mask &&
	       actual.name == mutated.name &&
	       actual.short_description == mutated.short_description &&
	       actual.description == mutated.description &&
	       actual.action_description == mutated.action_description &&
	       actual.values == mutated.values && actual.timers == mutated.timers &&
	       actual.wear_flags == mutated.wear_flags &&
	       actual.extra_flags == mutated.extra_flags &&
	       actual.anti_flags == mutated.anti_flags &&
	       actual.anti2_flags == mutated.anti2_flags &&
	       actual.extra2_flags == mutated.extra2_flags && actual.weight == mutated.weight &&
	       actual.material == mutated.material && actual.cost == mutated.cost &&
	       actual.condition == mutated.condition &&
	       actual.craftsmanship == mutated.craftsmanship &&
	       actual.bitvectors == mutated.bitvectors && actual.affects == mutated.affects);
	assert(actual.dynamic_affects.size() == mutated.dynamic_affects.size() &&
	       actual.dynamic_affects[0].type == mutated.dynamic_affects[0].type &&
	       actual.dynamic_affects[0].data == mutated.dynamic_affects[0].data &&
	       actual.dynamic_affects[0].extra2 == mutated.dynamic_affects[0].extra2 &&
	       actual.extra_descriptions.size() == mutated.extra_descriptions.size() &&
	       actual.extra_descriptions[0].keyword == mutated.extra_descriptions[0].keyword &&
	       actual.extra_descriptions[0].description ==
		       mutated.extra_descriptions[0].description &&
	       actual.extra_descriptions[0].spellbook == mutated.extra_descriptions[0].spellbook &&
	       actual.extra_descriptions[0].spell_ids == mutated.extra_descriptions[0].spell_ids);
	assert(scalar(connection, ("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
				   std::to_string(uid))
					  .c_str()) == 42);
	// The fixture intentionally has custody and a restitution runtime payload but no
	// player_items row. A legitimate live give must repair that historical gap from
	// the command's exact snapshot instead of rejecting the move as an ownership
	// conflict or leaving the recipient with custody-only state.
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=42 AND obj_uid=" +
		       std::to_string(uid) + " AND vnum=1901 AND cost=4567 AND timer=1900000000")
			      .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE pid=41 AND obj_uid=" +
				   std::to_string(uid))
					  .c_str()) == 0);
}
} // namespace

std::vector<std::string> native_snapshot(MYSQL *connection)
{
	std::vector<std::string> snapshot;
	for (const char *table :
	     { "player_data", "player_items", "item_current_owner", "item_owner_revision",
	       "item_ownership_ledger", "critical_operation_inbox", "critical_outbox" })
	{
		execute(connection, std::string("SELECT * FROM ") + table);
		MYSQL_RES *rows = mysql_store_result(connection);
		assert(rows);
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(rows)))
		{
			const auto *lengths = mysql_fetch_lengths(rows);
			assert(lengths);
			std::string encoded = table;
			for (unsigned int index = 0; index < mysql_num_fields(rows); ++index)
			{
				if (!row[index])
					encoded += ":NULL;";
				else
				{
					encoded += ":" + std::to_string(lengths[index]) + ":";
					encoded.append(row[index], lengths[index]);
				}
			}
			snapshot.push_back(std::move(encoded));
		}
		mysql_free_result(rows);
	}
	std::sort(snapshot.begin(), snapshot.end());
	return snapshot;
}

void check_staged_accounting_refusal(MYSQL *connection, const item_transfer_payload &items)
{
	critical_command command = {};
	assert(item_transfer_command_build(&command, operation(83), items,
					   critical_source_site::operator_repair,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	const std::string inbox = "UNHEX('" + operation_hex(80) + "')";
	const std::string lineage = "UNHEX('" + operation_hex(81) + "')";
	const std::string epoch = "UNHEX('" + operation_hex(82) + "')";
	// Refusal fixtures only, not valid economic admission or permission to write.
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
			inbox + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32)," +
			std::to_string(static_cast<unsigned int>(command.type)) + ",1,1,1,'')");
	execute(connection, "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
			    "transition_digest,creating_operation_id) VALUES(" +
				    lineage + "," + epoch + ",1,1,REPEAT(CHAR(3),32)," + inbox +
				    ")");
	execute(connection, "INSERT INTO economic_lineage_state(lineage) VALUES(" + lineage + ")");
	execute(connection,
		"INSERT INTO economic_sql_lifecycle_installation(operation_id,lineage,epoch,"
		"request_digest,source_capture_digest,native_boundary_digest,wallet_count,bank_count,"
		"phase) VALUES(" +
			inbox + "," + lineage + "," + epoch +
			",REPEAT(CHAR(4),32),REPEAT(CHAR(5),32),REPEAT(CHAR(6),32),0,0,1)");
	auto refused = [&](const char *phase, unsigned int expected_error = EPERM)
	{
		const auto before = native_snapshot(connection);
		const auto result = critical_command_repository_apply(connection, command);
		if (result.error_code != expected_error ||
		    result.outcome == critical_apply_outcome::applied)
			fprintf(stderr, "legacy admission refusal failed: phase=%s error=%u\n",
				phase, result.error_code);
		assert(result.error_code == expected_error &&
		       result.outcome != critical_apply_outcome::applied);
		assert(native_snapshot(connection) == before);
		assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
		assert(scalar(connection,
			      "SELECT IS_FREE_LOCK('duris:economic_sql_currency_writers')") == 1);
	};
	refused("staged-1");
	execute(connection, "UPDATE economic_sql_lifecycle_installation SET phase=2,"
			    "selected_epoch=epoch,baseline_operation_id=operation_id,revision=1 "
			    "WHERE operation_id=" +
				    inbox);
	refused("staged-2");
	execute(connection,
		"DELETE FROM economic_sql_lifecycle_installation WHERE operation_id=" + inbox);
	execute(connection, "UPDATE economic_lineage_state SET active_epoch=" + epoch +
				    " WHERE lineage=" + lineage);
	refused("active-without-installation");
	execute(connection, "DELETE FROM economic_lineage_state WHERE lineage=" + lineage);
	execute(connection, "DELETE FROM economic_epoch WHERE lineage=" + lineage);
	execute(connection, "DELETE FROM critical_operation_inbox WHERE operation_id=" + inbox);
	execute(connection, "RENAME TABLE economic_sql_lifecycle_installation TO "
			    "economic_sql_lifecycle_installation_admission_probe");
	refused("missing-lifecycle-schema", 1146);
	execute(connection, "RENAME TABLE economic_sql_lifecycle_installation_admission_probe TO "
			    "economic_sql_lifecycle_installation");
	puts("PASS: staged phases and active epoch refuse legacy grants without native mutations");
}

void check_dispatch_fence_commit_and_rollback(MYSQL *connection)
{
	const uint64_t operation_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation");
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity player = { item_owner_type::player, 4000000001, 0 };
	critical_command command = {};
	assert(item_transfer_command_build(
		&command, operation(84),
		payload(system, player, item_transfer_reason::creation,
			owner_revision(connection, system), owner_revision(connection, player),
			ITEM_TRANSFER_ABSENT_REVISION),
		critical_source_site::operator_repair, critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	// Observe the real transaction's maintenance lock from inside its native
	// ledger write. Session variables survive the deliberately injected rollback.
	execute(connection,
		"CREATE TRIGGER legacy_writer_fence_probe BEFORE INSERT ON item_ownership_ledger "
		"FOR EACH ROW BEGIN SET @legacy_writer_fence_seen="
		"(IS_USED_LOCK('duris:economic_sql_currency_writers') <=> CONNECTION_ID()); "
		"IF @legacy_writer_fence_fault THEN SIGNAL SQLSTATE '45000' "
		"SET MYSQL_ERRNO=1644,MESSAGE_TEXT='legacy writer rollback probe'; END IF; END");
	execute(connection, "SET @legacy_writer_fence_seen=0,@legacy_writer_fence_fault=1");
	const auto before = native_snapshot(connection);
	const auto failed = critical_command_repository_apply(connection, command);
	assert(failed.outcome != critical_apply_outcome::applied && failed.error_code == 1644);
	assert(scalar(connection, "SELECT @legacy_writer_fence_seen") == 1);
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
	assert(scalar(connection, "SELECT IS_FREE_LOCK('duris:economic_sql_currency_writers')") ==
	       1);
	assert(native_snapshot(connection) == before);
	execute(connection, "SET @legacy_writer_fence_seen=0,@legacy_writer_fence_fault=0");
	const auto committed = critical_command_repository_apply(connection, command);
	assert(committed.outcome == critical_apply_outcome::applied && committed.error_code == 0);
	assert(scalar(connection, "SELECT @legacy_writer_fence_seen") == 1);
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
	assert(scalar(connection, "SELECT IS_FREE_LOCK('duris:economic_sql_currency_writers')") ==
	       1);
	const auto committed_state = native_snapshot(connection);
	const auto replay = critical_command_repository_apply(connection, command);
	assert(replay.outcome == critical_apply_outcome::already_applied && replay.error_code == 0);
	assert(native_snapshot(connection) == committed_state);
	assert(scalar(connection, "SELECT IS_FREE_LOCK('duris:economic_sql_currency_writers')") ==
	       1);
	execute(connection, "DROP TRIGGER legacy_writer_fence_probe");
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation") ==
	       operation_count);
	puts("PASS: real dispatch holds the maintenance fence and releases it after rollback, commit and replay");
}

void check_deferred_accounting_reference_rollback(MYSQL *connection)
{
	const uint64_t item_reference_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference");
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity player = { item_owner_type::player, 4000000001, 0 };
	const uint64_t from_revision = owner_revision(connection, system);
	const uint64_t to_revision = owner_revision(connection, player);
	critical_command command = {};
	assert(item_transfer_command_build(
		&command, operation(91),
		payload(system, player, item_transfer_reason::creation, from_revision, to_revision,
			ITEM_TRANSFER_ABSENT_REVISION),
		critical_source_site::operator_repair, critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	execute(connection, "START TRANSACTION");
	// Supply only the legacy inbox required by the native ledger. Deliberately
	// do not create an accounting root: its failure must roll back all custody.
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
			operation_hex(91) + "'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32)," +
			std::to_string(static_cast<unsigned int>(command.type)) + ",1,1,0,'')");
	item_transfer_accounting_context context = {};
	item_transfer_result result = {};
	unsigned int result_code = 0;
	bool mutated = false;
	assert(!item_transfer_repository_execute(connection, command, &result, &result_code,
						 &mutated, nullptr, &context));
	assert(errno == EINVAL && !mutated);
	context.root_operation_id = operation(92);
	context.child_index = 65;
	assert(!item_transfer_repository_execute(connection, command, &result, &result_code,
						 &mutated, nullptr, &context));
	assert(errno == EINVAL && !mutated);
	context.child_index = 1;
	context.line_index_base = UINT16_MAX;
	assert(!item_transfer_repository_execute(connection, command, &result, &result_code,
						 &mutated, nullptr, &context));
	assert(errno == EINVAL && !mutated);
	context.line_index_base = 0;
	const bool applied = item_transfer_repository_execute(
		connection, command, &result, &result_code, &mutated, nullptr, &context);
	assert(applied && !result_code && mutated);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference") ==
	       item_reference_count);
	// The admitted parent writes this FK row after its accounting operation.
	// The enclosing coin owner must insert and verify that reference before commit.
	execute(connection, "ROLLBACK");
	assert(owner_revision(connection, system) == from_revision);
	assert(owner_revision(connection, player) == to_revision);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				   std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					  .c_str()) == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
				   std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					  .c_str()) == 0);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(91) + "')")
			      .c_str()) == 0);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		       operation_hex(91) + "')")
			      .c_str()) == 0);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference") ==
	       item_reference_count);
	puts("PASS: deferred reference and enclosing rollback preserve native custody, revisions and inbox");
}

void check_sql_accounted_item_transfer(MYSQL *connection)
{
	const uint64_t epoch_count = scalar(connection, "SELECT COUNT(*) FROM economic_epoch");
	const uint64_t lineage_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_lineage_state");
	const uint64_t operation_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation");
	const uint64_t item_reference_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference");
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity player_one = { item_owner_type::player, 41, 0 };
	const item_owner_identity player_two = { item_owner_type::player, 42, 0 };
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const auto creation = payload(system, player_one, item_transfer_reason::creation,
				      owner_revision(connection, system),
				      owner_revision(connection, player_one),
				      ITEM_TRANSFER_ABSENT_REVISION, 2);
	const critical_apply_result created = apply(connection, 127, creation);
	assert(created.outcome == critical_apply_outcome::applied);
	item_transfer_result created_result = {};
	assert(item_transfer_command_decode_result(created.result_payload.data(),
						   created.result_size, &created_result));

	const critical_operation_id creator = operation(129);
	const critical_operation_id lineage = operation(130);
	const critical_operation_id epoch = operation(131);
	execute(connection,
		("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		 "command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
		 operation_hex(creator) + "'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')")
			.c_str());
	execute(connection,
		("INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,"
		 "transition_digest,creating_operation_id) VALUES(UNHEX('" +
		 operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
		 "'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(creator) + "'))")
			.c_str());
	execute(connection,
		("INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
		 operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) + "'),0)")
			.c_str());

	const uint64_t original_root_uid = root_uid;
	const uint64_t original_child_uid = child_uid;
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const auto creation_payload = payload(system, player_one, item_transfer_reason::creation,
					      owner_revision(connection, system),
					      owner_revision(connection, player_one),
					      ITEM_TRANSFER_ABSENT_REVISION, 2);
	const critical_command creation_command =
		accounted_item_transfer(operation(137), creation_payload, lineage, epoch, 41,
					economic_source_kind::starter_grant);
	const critical_apply_result sourced_creation =
		critical_command_repository_apply(connection, creation_command);
	assert(sourced_creation.outcome == critical_apply_outcome::applied &&
	       sourced_creation.error_code == 0);
	item_transfer_result sourced_creation_result = {};
	assert(item_transfer_command_decode_result(sourced_creation.result_payload.data(),
						   sourced_creation.result_size,
						   &sourced_creation_result));
	economic_source_event source_event = { economic_source_kind::starter_grant, lineage,
					       lineage, root_uid, 77 };
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded_source_event = {};
	assert(economic_source_event_encode(source_event, &encoded_source_event) ==
	       economic_accounting_error::ok);
	const std::string encoded_source_event_hex = bytes_hex(encoded_source_event);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage=UNHEX('" +
		       operation_hex(lineage) + "') AND source_event=UNHEX('" +
		       encoded_source_event_hex + "') AND operation_id=UNHEX('" +
		       operation_hex(creation_command.operation_id) + "') AND outcome=1")
			      .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(creation_command.operation_id) + "') AND r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=0 AND r.after_revision=1")
			      .c_str()) == 2);
	const critical_apply_result replayed_creation =
		critical_command_repository_apply(connection, creation_command);
	assert(replayed_creation.outcome == critical_apply_outcome::already_applied &&
	       replayed_creation.result_size == sourced_creation.result_size);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE operation_id=UNHEX('" +
		       operation_hex(creation_command.operation_id) + "')")
			      .c_str()) == 1);
	const uint64_t sourced_root_uid = root_uid;
	const uint64_t sourced_child_uid = child_uid;
	item_transfer_payload nested_get = {};
	nested_get.from_owner = player_one;
	nested_get.to_owner = player_one;
	nested_get.reason = item_transfer_reason::player_get;
	nested_get.expected_from_revision = owner_revision(connection, player_one);
	nested_get.expected_to_revision = nested_get.expected_from_revision;
	nested_get.selected_item_uid = sourced_child_uid;
	nested_get.target_root_item_uid = sourced_child_uid;
	nested_get.item_count = 1;
	nested_get.items[0] = {
		sourced_child_uid,	   sourced_root_uid, sourced_root_uid, 1, 1002,
		item_custody_state::active
	};
	attach_blob(&nested_get,
		    { physical_item(sourced_child_uid, 1002, PLAYER_SNAPSHOT_NO_PARENT) });
	const auto nested_get_command =
		accounted_item_transfer(operation(140), nested_get, lineage, epoch, 41);
	const auto nested_got = critical_command_repository_apply(connection, nested_get_command);
	assert(nested_got.outcome == critical_apply_outcome::applied && !nested_got.error_code);
	assert(critical_command_repository_apply(connection, nested_get_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(nested_get_command.operation_id) +
		       "') AND r.item_uid=" + std::to_string(sourced_child_uid) +
		       " AND r.before_revision=1 AND r.after_revision=2")
			      .c_str()) == 1);
	item_transfer_payload nested_put = nested_get;
	nested_put.reason = item_transfer_reason::player_put;
	nested_put.reason_id = static_cast<int64_t>(sourced_root_uid);
	nested_put.expected_from_revision = owner_revision(connection, player_one);
	nested_put.expected_to_revision = nested_put.expected_from_revision;
	nested_put.target_root_item_uid = sourced_root_uid;
	nested_put.target_parent_item_uid = sourced_root_uid;
	nested_put.expected_target_parent_revision = 1;
	nested_put.items[0] = { sourced_child_uid,	   sourced_child_uid, 0, 2, 1002,
				item_custody_state::active };
	const auto nested_put_command =
		accounted_item_transfer(operation(141), nested_put, lineage, epoch, 41);
	const auto nested_put_result =
		critical_command_repository_apply(connection, nested_put_command);
	assert(nested_put_result.outcome == critical_apply_outcome::applied &&
	       !nested_put_result.error_code);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(sourced_child_uid) + " AND root_item_uid=" +
				   std::to_string(sourced_root_uid) + " AND parent_item_uid=" +
				   std::to_string(sourced_root_uid) + " AND item_revision=3")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(nested_put_command.operation_id) +
		       "') AND r.item_uid=" + std::to_string(sourced_child_uid) +
		       " AND r.before_revision=2 AND r.after_revision=3")
			      .c_str()) == 1);
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const uint64_t equipment_root_uid = root_uid;
	const uint64_t equipment_child_uid = child_uid;
	const auto equipment_creation = accounted_item_transfer(
		operation(151),
		payload(system, player_one, item_transfer_reason::creation,
			owner_revision(connection, system), owner_revision(connection, player_one),
			ITEM_TRANSFER_ABSENT_REVISION),
		lineage, epoch, 41, economic_source_kind::starter_grant);
	const auto equipment_created =
		critical_command_repository_apply(connection, equipment_creation);
	assert(equipment_created.outcome == critical_apply_outcome::applied &&
	       !equipment_created.error_code);
	auto wear_item = payload(player_one, player_one, item_transfer_reason::player_wear,
				 owner_revision(connection, player_one),
				 owner_revision(connection, player_one), 1);
	wear_item.reason_id = 5;
	wear_item.selected_item_uid = equipment_root_uid;
	wear_item.target_root_item_uid = equipment_root_uid;
	auto equipped_root = physical_item(equipment_root_uid, 1001, PLAYER_SNAPSHOT_NO_PARENT);
	equipped_root.equipment_slot = 5;
	attach_blob(&wear_item, { equipped_root, physical_item(equipment_child_uid, 1002, 0) });
	const auto wear_command =
		accounted_item_transfer(operation(152), wear_item, lineage, epoch, 41);
	const auto worn = critical_command_repository_apply(connection, wear_command);
	assert(worn.outcome == critical_apply_outcome::applied && !worn.error_code);
	assert(critical_command_repository_apply(connection, wear_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(equipment_root_uid) +
				   " AND equipment_slot=5 AND item_revision=2")
					  .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
				   std::to_string(equipment_root_uid) + " AND equip_slot=5")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(wear_command.operation_id) +
		       "') AND l.from_equipment_slot=0 AND l.to_equipment_slot=5")
			      .c_str()) == 1);
	auto stale_wear = wear_item;
	stale_wear.expected_from_revision = owner_revision(connection, player_one);
	stale_wear.expected_to_revision = stale_wear.expected_from_revision;
	stale_wear.items[0].expected_item_revision = 2;
	stale_wear.items[1].expected_item_revision = 2;
	const auto stale_wear_command =
		accounted_item_transfer(operation(153), stale_wear, lineage, epoch, 41);
	const auto stale_worn = critical_command_repository_apply(connection, stale_wear_command);
	assert(stale_worn.outcome == critical_apply_outcome::terminal_failure &&
	       stale_worn.error_code == ESTALE);
	auto remove_item = payload(player_one, player_one, item_transfer_reason::player_remove,
				   owner_revision(connection, player_one),
				   owner_revision(connection, player_one), 2);
	remove_item.reason_id = 5;
	remove_item.selected_item_uid = equipment_root_uid;
	remove_item.target_root_item_uid = equipment_root_uid;
	const auto remove_command =
		accounted_item_transfer(operation(154), remove_item, lineage, epoch, 41);
	const auto removed = critical_command_repository_apply(connection, remove_command);
	assert(removed.outcome == critical_apply_outcome::applied && !removed.error_code);
	assert(critical_command_repository_apply(connection, remove_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(equipment_root_uid) +
				   " AND equipment_slot=0 AND item_revision=3")
					  .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
				   std::to_string(equipment_root_uid) + " AND equip_slot=0")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(remove_command.operation_id) +
		       "') AND l.from_equipment_slot=5 AND l.to_equipment_slot=0")
			      .c_str()) == 1);

	auto rewear_item = wear_item;
	rewear_item.expected_from_revision = owner_revision(connection, player_one);
	rewear_item.expected_to_revision = rewear_item.expected_from_revision;
	rewear_item.items[0].expected_item_revision = 3;
	rewear_item.items[1].expected_item_revision = 3;
	const auto rewear_command =
		accounted_item_transfer(operation(155), rewear_item, lineage, epoch, 41);
	const auto reworn = critical_command_repository_apply(connection, rewear_command);
	assert(reworn.outcome == critical_apply_outcome::applied && !reworn.error_code);

	auto forced_item = payload(player_one, { item_owner_type::room, 50, 0 },
				   item_transfer_reason::critical_disarm,
				   owner_revision(connection, player_one), 0, 4);
	forced_item.reason_id = 6;
	forced_item.selected_item_uid = equipment_root_uid;
	forced_item.target_root_item_uid = equipment_root_uid;
	const auto stale_disarm_command =
		accounted_item_transfer(operation(156), forced_item, lineage, epoch, 41);
	const auto stale_disarm =
		critical_command_repository_apply(connection, stale_disarm_command);
	assert(stale_disarm.outcome == critical_apply_outcome::terminal_failure &&
	       stale_disarm.error_code == ESTALE);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(equipment_root_uid) +
				   " AND owner_type=1 AND equipment_slot=5 AND item_revision=4")
					  .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
				   std::to_string(equipment_root_uid) + " AND equip_slot=5")
					  .c_str()) == 1);
	forced_item.reason_id = 5;
	execute(connection, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(41,1002," +
				    std::to_string(equipment_child_uid) + ")");
	const uint64_t duplicate_child_row_id = mysql_insert_id(connection);
	assert(duplicate_child_row_id);
	const auto duplicate_disarm_command =
		accounted_item_transfer(operation(158), forced_item, lineage, epoch, 41);
	const auto duplicate_disarm =
		critical_command_repository_apply(connection, duplicate_disarm_command);
	assert(duplicate_disarm.outcome == critical_apply_outcome::terminal_failure &&
	       duplicate_disarm.error_code == ESTALE);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_disarm_command.operation_id) + "')")
			      .c_str()) == 0);
	execute(connection,
		"DELETE FROM player_items WHERE id=" + std::to_string(duplicate_child_row_id));
	const auto forced_command =
		accounted_item_transfer(operation(157), forced_item, lineage, epoch, 41);
	const auto disarmed = critical_command_repository_apply(connection, forced_command);
	assert(disarmed.outcome == critical_apply_outcome::applied && !disarmed.error_code);
	assert(critical_command_repository_apply(connection, forced_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				   std::to_string(equipment_root_uid) + "," +
				   std::to_string(equipment_child_uid) +
				   ") AND owner_type=3 AND owner_id=50 AND equipment_slot=0 AND "
				   "item_revision=5")
					  .c_str()) == 2);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
				   std::to_string(equipment_root_uid) + "," +
				   std::to_string(equipment_child_uid) + ")")
					  .c_str()) == 0);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(forced_command.operation_id) + "') AND l.reason_type=" +
		       std::to_string(
			       static_cast<unsigned int>(item_transfer_reason::critical_disarm)) +
		       " AND l.from_equipment_slot IN (0,5) AND l.to_equipment_slot=0 "
		       "AND r.before_revision=4 AND r.after_revision=5")
			      .c_str()) == 2);
	root_uid = original_root_uid;
	child_uid = original_child_uid;

	const auto give = payload(
		player_one, player_two, item_transfer_reason::player_give,
		owner_revision(connection, player_one), owner_revision(connection, player_two),
		scalar(connection, ("SELECT item_revision FROM item_current_owner WHERE item_uid=" +
				    std::to_string(root_uid))
					   .c_str()),
		2);
	critical_command admitted =
		accounted_item_transfer(operation(133), give, lineage, epoch, 41);
	admitted.publication_required = true;
	const critical_apply_result moved = exercise_sql_coordinator(admitted, "item", true);
	if (moved.outcome != critical_apply_outcome::already_applied || moved.error_code)
		fprintf(stderr, "accounted item transfer failed: outcome=%u error=%u\n",
			static_cast<unsigned int>(moved.outcome), moved.error_code);
	assert(moved.outcome == critical_apply_outcome::already_applied && moved.error_code == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
				   "operation_id=UNHEX('" +
				   operation_hex(admitted.operation_id) +
				   "') AND writer_id=6 AND outcome=1 AND item_event_count=2 AND "
				   "before_witness_count=2 AND after_witness_count=2")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(admitted.operation_id) +
		       "') AND r.line_index=r.event_index AND "
		       "r.event_index IN (0,1) AND r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=1 AND r.after_revision=2")
			      .c_str()) == 2);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_ownership_ledger WHERE "
				   "operation_id=UNHEX('" +
				   operation_hex(admitted.operation_id) + "')")
					  .c_str()) == 2);

	auto trusted_steal_payload = payload(player_two, player_one,
					     item_transfer_reason::trusted_steal,
					     owner_revision(connection, player_two),
					     owner_revision(connection, player_one), 2, 2);
	trusted_steal_payload.reason_id = static_cast<int64_t>(player_two.id);
	const critical_command stolen =
		accounted_item_transfer(operation(134), trusted_steal_payload, lineage, epoch,
					static_cast<uint32_t>(player_one.id));
	const critical_apply_result stolen_result =
		critical_command_repository_apply(connection, stolen);
	assert(stolen_result.outcome == critical_apply_outcome::applied &&
	       stolen_result.error_code == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
				   "operation_id=UNHEX('" +
				   operation_hex(stolen.operation_id) +
				   "') AND writer_id=6 AND outcome=1 AND item_event_count=2 AND "
				   "before_witness_count=2 AND after_witness_count=2")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(stolen.operation_id) +
		       "') AND r.line_index=r.event_index AND r.event_index IN (0,1) AND "
		       "r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=2 AND "
		       "r.after_revision=3")
			      .c_str()) == 2);
	const critical_apply_result stolen_replay =
		critical_command_repository_apply(connection, stolen);
	assert(stolen_replay.outcome == critical_apply_outcome::already_applied &&
	       stolen_replay.result_size == stolen_result.result_size);

	const critical_operation_id next_creator = operation(136);
	const critical_operation_id next_epoch = operation(132);
	execute(connection,
		("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		 "command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
		 operation_hex(next_creator) +
		 "'),REPEAT(CHAR(3),32),REPEAT(CHAR(4),32),1,1,1,1,X'')")
			.c_str());
	execute(connection,
		("INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,"
		 "transition_digest,creating_operation_id) VALUES(UNHEX('" +
		 operation_hex(lineage) + "'),UNHEX('" + operation_hex(next_epoch) +
		 "'),2,UNHEX('" + operation_hex(epoch) + "'),1,REPEAT(CHAR(0),32),UNHEX('" +
		 operation_hex(next_creator) + "'))")
			.c_str());
	execute(connection,
		("UPDATE economic_lineage_state SET active_epoch=UNHEX('" +
		 operation_hex(next_epoch) + "'),revision=revision+1 WHERE lineage=UNHEX('" +
		 operation_hex(lineage) + "')")
			.c_str());

	// Reopen the SQL session before replay. Retained evidence is verified under the
	// command's original epoch even after the active lineage advances.
	MYSQL *restarted = mysql_init(nullptr);
	assert(restarted);
	assert(mysql_real_connect(
		restarted, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("ITEM_TRANSFER_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	const critical_apply_result replayed =
		critical_command_repository_apply(restarted, admitted);
	assert(replayed.outcome == critical_apply_outcome::already_applied &&
	       replayed.result_size == moved.result_size);
	const critical_apply_result reconciled =
		critical_command_repository_reconcile(restarted, admitted);
	assert(reconciled.outcome == critical_apply_outcome::already_applied &&
	       reconciled.result_size == moved.result_size);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(admitted.operation_id) + "')")
					 .c_str()) == 2);
	const auto stale_payload = payload(player_two, player_one,
					   item_transfer_reason::player_give,
					   owner_revision(restarted, player_two),
					   owner_revision(restarted, player_one), 2, 1);
	const critical_command stale_epoch =
		accounted_item_transfer(operation(135), stale_payload, lineage, epoch, 42);
	const critical_apply_result refused =
		critical_command_repository_apply(restarted, stale_epoch);
	assert(refused.outcome == critical_apply_outcome::retryable_failure &&
	       refused.error_code == ESTALE);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM critical_operation_inbox WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(stale_epoch.operation_id) + "')")
					 .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) +
				  ") AND owner_type=1 AND owner_id=41 AND item_revision=3")
					 .c_str()) == 2);
	const item_owner_identity destroyed_owner = { item_owner_type::destruction, 0, 0 };
	const auto retirement_payload = payload(player_one, destroyed_owner,
						item_transfer_reason::destruction,
						owner_revision(restarted, player_one),
						owner_revision(restarted, destroyed_owner), 3, 2);
	const critical_command retirement =
		accounted_item_transfer(operation(138), retirement_payload, lineage, next_epoch, 41,
					economic_source_kind::item_action);
	const critical_apply_result retirement_result =
		critical_command_repository_apply(restarted, retirement);
	if (retirement_result.outcome != critical_apply_outcome::applied ||
	    retirement_result.error_code)
		fprintf(stderr, "accounted item retirement failed: outcome=%u error=%u\n",
			static_cast<unsigned int>(retirement_result.outcome),
			retirement_result.error_code);
	assert(retirement_result.outcome == critical_apply_outcome::applied &&
	       retirement_result.error_code == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND owner_type=8 AND owner_id=0 AND state=2 AND item_revision=4")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(retirement.operation_id) + "') AND r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=3 AND "
		       "r.after_revision=4")
			      .c_str()) == 2);
	economic_source_event retirement_source_event = { economic_source_kind::item_action,
							  lineage, lineage, root_uid, 77 };
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded_retirement_source_event = {};
	assert(economic_source_event_encode(retirement_source_event,
					    &encoded_retirement_source_event) ==
	       economic_accounting_error::ok);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage=UNHEX('" +
		       operation_hex(lineage) + "') AND source_event=UNHEX('" +
		       bytes_hex(encoded_retirement_source_event) + "') AND operation_id=UNHEX('" +
		       operation_hex(retirement.operation_id) + "') AND outcome=1")
			      .c_str()) == 1);
	const critical_apply_result retirement_replay =
		critical_command_repository_apply(restarted, retirement);
	assert(retirement_replay.outcome == critical_apply_outcome::already_applied &&
	       retirement_replay.result_size == retirement_result.result_size);
	const item_owner_identity reward_room = { item_owner_type::room, 150, 0 };
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(restarted, 1));
	const uint64_t room_reward_uid = item_uid_allocator_next();
	root_uid = room_reward_uid;
	const auto room_creation_payload =
		payload(system, reward_room, item_transfer_reason::creation,
			owner_revision(restarted, system), owner_revision(restarted, reward_room),
			ITEM_TRANSFER_ABSENT_REVISION, 1);
	root_uid = original_root_uid;
	const critical_command room_creation =
		accounted_item_transfer(operation(139), room_creation_payload, lineage, next_epoch,
					41, economic_source_kind::quest_completion);
	const critical_apply_result room_created =
		critical_command_repository_apply(restarted, room_creation);
	assert(room_created.outcome == critical_apply_outcome::applied &&
	       room_created.error_code == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				  std::to_string(room_reward_uid) +
				  " AND owner_type=3 AND owner_id=150 AND state=1 AND "
				  "item_revision=1")
					 .c_str()) == 1);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE operation_id=UNHEX('" +
		       operation_hex(room_creation.operation_id) +
		       "') AND item_uid=" + std::to_string(room_reward_uid) +
		       " AND before_revision=0 AND after_revision=1")
			      .c_str()) == 1);
	const critical_apply_result room_creation_replay =
		critical_command_repository_apply(restarted, room_creation);
	assert(room_creation_replay.outcome == critical_apply_outcome::already_applied &&
	       room_creation_replay.result_size == room_created.result_size);
	// A root has exactly one publication obligation at event index zero.
	execute(restarted, "INSERT INTO critical_outbox(operation_id,event_index,destination,"
			   "event_type,payload_version,payload) VALUES(UNHEX('" +
				   operation_hex(admitted.operation_id) + "'),1,4,1,1,'extra')");
	const auto extra_outbox = critical_command_repository_reconcile(restarted, admitted);
	assert(extra_outbox.outcome == critical_apply_outcome::retryable_failure &&
	       extra_outbox.error_code == EILSEQ);
	execute(restarted, "DELETE FROM critical_outbox WHERE operation_id=UNHEX('" +
				   operation_hex(admitted.operation_id) + "') AND event_index=1");
	// A retained accounting row cannot stand in for its publication obligation.
	execute(restarted, "DELETE FROM critical_outbox WHERE operation_id=UNHEX('" +
				   operation_hex(admitted.operation_id) + "')");
	const auto missing_outbox = critical_command_repository_reconcile(restarted, admitted);
	assert(missing_outbox.outcome == critical_apply_outcome::retryable_failure &&
	       missing_outbox.error_code == EILSEQ);
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(restarted, 1));
	root_uid = item_uid_allocator_next();
	const uint64_t retried_reward_uid = root_uid;
	const auto duplicate_reward_payload =
		payload(system, reward_room, item_transfer_reason::creation,
			owner_revision(restarted, system), owner_revision(restarted, reward_room),
			ITEM_TRANSFER_ABSENT_REVISION, 1);
	const auto duplicate_reward =
		accounted_item_transfer(operation(145), duplicate_reward_payload, lineage,
					next_epoch, 41, economic_source_kind::quest_completion);
	const auto refused_duplicate_reward =
		critical_command_repository_apply(restarted, duplicate_reward);
	assert(refused_duplicate_reward.outcome == critical_apply_outcome::terminal_failure &&
	       refused_duplicate_reward.error_code == EEXIST);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_reward.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				  std::to_string(retried_reward_uid))
					 .c_str()) == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_reward.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(duplicate_reward.operation_id) + "')")
					 .c_str()) == 0);
	root_uid = original_root_uid;
	const item_owner_identity generated_room = { item_owner_type::room, 151, 0 };
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(restarted, 1));
	root_uid = item_uid_allocator_next();
	const uint64_t generated_uid = root_uid;
	auto generated_payload = payload(system, generated_room, item_transfer_reason::creation,
					 owner_revision(restarted, system),
					 owner_revision(restarted, generated_room),
					 ITEM_TRANSFER_ABSENT_REVISION, 1);
	generated_payload.logical_source_id = 90001;
	const auto generated = accounted_item_transfer(operation(159), generated_payload, lineage,
						       next_epoch, 41,
						       economic_source_kind::world_generation);
	assert(critical_command_repository_apply(restarted, generated).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(restarted, generated).outcome ==
	       critical_apply_outcome::already_applied);
	economic_source_event generated_source_event = { economic_source_kind::world_generation,
							 lineage, lineage, 90001, 0 };
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> encoded_generated_source_event = {};
	assert(economic_source_event_encode(generated_source_event,
					    &encoded_generated_source_event) ==
	       economic_accounting_error::ok);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage=UNHEX('" +
		       operation_hex(lineage) + "') AND source_event=UNHEX('" +
		       bytes_hex(encoded_generated_source_event) + "') AND operation_id=UNHEX('" +
		       operation_hex(generated.operation_id) + "') AND outcome=1")
			      .c_str()) == 1);
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(restarted, 1));
	root_uid = item_uid_allocator_next();
	const uint64_t duplicate_generated_uid = root_uid;
	auto duplicate_generated_payload = payload(system, generated_room,
						   item_transfer_reason::creation,
						   owner_revision(restarted, system),
						   owner_revision(restarted, generated_room),
						   ITEM_TRANSFER_ABSENT_REVISION, 1);
	duplicate_generated_payload.logical_source_id = 90001;
	const auto duplicate_generated =
		accounted_item_transfer(operation(160), duplicate_generated_payload, lineage,
					next_epoch, 41, economic_source_kind::world_generation);
	const auto refused_duplicate_generated =
		critical_command_repository_apply(restarted, duplicate_generated);
	assert(refused_duplicate_generated.outcome == critical_apply_outcome::terminal_failure &&
	       refused_duplicate_generated.error_code == EEXIST);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_generated.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				  std::to_string(duplicate_generated_uid))
					 .c_str()) == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_generated.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(duplicate_generated.operation_id) + "')")
					 .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				  std::to_string(generated_uid) +
				  " AND owner_type=3 AND owner_id=151 AND state=1")
					 .c_str()) == 1);
	root_uid = original_root_uid;

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(restarted, 3));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const uint64_t pet_uid = item_uid_allocator_next();
	const auto pet_tree_creation = accounted_item_transfer(
		operation(142),
		payload(system, player_one, item_transfer_reason::creation,
			owner_revision(restarted, system), owner_revision(restarted, player_one),
			ITEM_TRANSFER_ABSENT_REVISION),
		lineage, next_epoch, 41, economic_source_kind::starter_grant);
	const auto pet_tree_created =
		critical_command_repository_apply(restarted, pet_tree_creation);
	assert(pet_tree_created.outcome == critical_apply_outcome::applied &&
	       !pet_tree_created.error_code);
	execute(restarted,
		"INSERT INTO player_pets(owner_pid,mob_vnum,pet_uid,hold_reason) VALUES(41,1001," +
			std::to_string(pet_uid) + ",0)");
	const uint64_t pet_row_id = mysql_insert_id(restarted);
	assert(pet_row_id);
	const item_owner_identity pet_owner = { item_owner_type::pet, pet_uid, 41 };
	auto pet_give = payload(player_one, pet_owner, item_transfer_reason::pet_give,
				owner_revision(restarted, player_one),
				owner_revision(restarted, pet_owner), 1);
	pet_give.reason_id = static_cast<int64_t>(pet_uid);
	pet_give.selected_item_uid = root_uid;
	const auto pet_give_command =
		accounted_item_transfer(operation(143), pet_give, lineage, next_epoch, 41);
	const auto pet_given = critical_command_repository_apply(restarted, pet_give_command);
	assert(pet_given.outcome == critical_apply_outcome::applied && !pet_given.error_code);
	assert(critical_command_repository_apply(restarted, pet_give_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM player_pet_items WHERE pet_id=" +
				  std::to_string(pet_row_id) + " AND obj_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					 .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_pet_items child JOIN player_pet_items "
		       "parent ON parent.id=child.container_id WHERE child.pet_id=" +
		       std::to_string(pet_row_id) +
		       " AND child.obj_uid=" + std::to_string(child_uid) +
		       " AND parent.obj_uid=" + std::to_string(root_uid))
			      .c_str()) == 1);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM player_pet_item_affects a JOIN "
				  "player_pet_items i ON i.id=a.item_id WHERE i.pet_id=" +
				  std::to_string(pet_row_id) + " AND i.obj_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					 .c_str()) == 2);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM player_pet_item_extra_descr d JOIN "
				  "player_pet_items i ON i.id=d.item_id WHERE i.pet_id=" +
				  std::to_string(pet_row_id) + " AND i.obj_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					 .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=41 AND obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(pet_give_command.operation_id) + "') AND r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=1 AND "
		       "r.after_revision=2")
			      .c_str()) == 2);
	auto pet_return = payload(pet_owner, player_one, item_transfer_reason::pet_return,
				  owner_revision(restarted, pet_owner),
				  owner_revision(restarted, player_one), 2);
	pet_return.reason_id = static_cast<int64_t>(pet_uid);
	pet_return.selected_item_uid = root_uid;
	const auto pet_return_command =
		accounted_item_transfer(operation(144), pet_return, lineage, next_epoch, 41);
	const auto pet_returned = critical_command_repository_apply(restarted, pet_return_command);
	assert(pet_returned.outcome == critical_apply_outcome::applied && !pet_returned.error_code);
	assert(critical_command_repository_apply(restarted, pet_return_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_items child JOIN player_items parent ON "
		       "parent.id=child.container_id WHERE child.pid=41 AND "
		       "child.obj_uid=" +
		       std::to_string(child_uid) +
		       " AND parent.obj_uid=" + std::to_string(root_uid))
			      .c_str()) == 1);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_item_affects a JOIN player_items i ON "
		       "i.id=a.item_id WHERE i.pid=41 AND i.obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_item_extra_descr d JOIN player_items i "
		       "ON i.id=d.item_id WHERE i.pid=41 AND i.obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 2);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM player_pet_items WHERE pet_id=" +
				  std::to_string(pet_row_id) + " AND obj_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
					 .c_str()) == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND owner_type=1 AND owner_id=41 AND state=1 AND item_revision=3")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(pet_return_command.operation_id) + "') AND r.item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) +
		       ") AND r.before_revision=2 AND "
		       "r.after_revision=3")
			      .c_str()) == 2);
	execute(restarted,
		"INSERT INTO lockers(locker_name,owner_pid) VALUES('ItemProvenanceLocker',41)");
	const uint64_t locker_id = mysql_insert_id(restarted);
	assert(locker_id);
	execute(restarted, "INSERT INTO private_chests(locker_id,chest_name,is_public) VALUES(" +
				   std::to_string(locker_id) + ",'public',1)");
	const uint64_t chest_id = mysql_insert_id(restarted);
	assert(chest_id);
	const item_owner_identity locker_owner = { item_owner_type::locker, locker_id, chest_id };
	const item_owner_identity missing_chest_owner = { item_owner_type::locker, locker_id,
							  chest_id + 1 };
	auto missing_chest = payload(player_one, missing_chest_owner,
				     item_transfer_reason::locker_deposit,
				     owner_revision(restarted, player_one),
				     owner_revision(restarted, missing_chest_owner), 3);
	missing_chest.selected_item_uid = root_uid;
	missing_chest.reason_id = static_cast<int64_t>(missing_chest_owner.context_id);
	const auto missing_chest_command =
		accounted_item_transfer(operation(148), missing_chest, lineage, next_epoch, 41);
	const auto refused_chest =
		critical_command_repository_apply(restarted, missing_chest_command);
	assert(refused_chest.outcome == critical_apply_outcome::retryable_failure &&
	       refused_chest.error_code == EILSEQ);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) +
				  ") AND owner_type=1 AND owner_id=41 AND item_revision=3")
					 .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(missing_chest_command.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(missing_chest_command.operation_id) + "')")
					 .c_str()) == 0);
	execute(restarted, "UPDATE player_items SET equip_slot=5 WHERE pid=41 AND obj_uid=" +
				   std::to_string(root_uid));
	auto stale_slot = payload(player_one, locker_owner, item_transfer_reason::locker_deposit,
				  owner_revision(restarted, player_one),
				  owner_revision(restarted, locker_owner), 3);
	stale_slot.selected_item_uid = root_uid;
	stale_slot.reason_id = static_cast<int64_t>(chest_id);
	const auto stale_slot_command =
		accounted_item_transfer(operation(150), stale_slot, lineage, next_epoch, 41);
	const auto refused_slot = critical_command_repository_apply(restarted, stale_slot_command);
	assert(refused_slot.outcome == critical_apply_outcome::retryable_failure &&
	       refused_slot.error_code == EILSEQ);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				  std::to_string(root_uid) + "," + std::to_string(child_uid) +
				  ") AND owner_type=1 AND owner_id=41 AND item_revision=3")
					 .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(stale_slot_command.operation_id) + "')")
			      .c_str()) == 0);
	execute(restarted, "UPDATE player_items SET equip_slot=0 WHERE pid=41 AND obj_uid=" +
				   std::to_string(root_uid));
	auto locker_deposit = payload(player_one, locker_owner,
				      item_transfer_reason::locker_deposit,
				      owner_revision(restarted, player_one),
				      owner_revision(restarted, locker_owner), 3);
	locker_deposit.selected_item_uid = root_uid;
	locker_deposit.reason_id = static_cast<int64_t>(chest_id);
	const auto locker_deposit_command =
		accounted_item_transfer(operation(146), locker_deposit, lineage, next_epoch, 41);
	const auto deposited = critical_command_repository_apply(restarted, locker_deposit_command);
	assert(deposited.outcome == critical_apply_outcome::applied && !deposited.error_code);
	assert(critical_command_repository_apply(restarted, locker_deposit_command).outcome ==
	       critical_apply_outcome::already_applied);
	const std::string selected_uids =
		std::to_string(root_uid) + "," + std::to_string(child_uid);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM locker_items WHERE locker_id=" +
		       std::to_string(locker_id) + " AND chest_id=" + std::to_string(chest_id) +
		       " AND obj_uid IN (" + selected_uids + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM locker_items child JOIN locker_items parent ON "
		       "parent.id=child.container_id WHERE child.obj_uid=" +
		       std::to_string(child_uid) +
		       " AND parent.obj_uid=" + std::to_string(root_uid))
			      .c_str()) == 1);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM locker_item_affects a JOIN locker_items i ON "
		       "i.id=a.item_id WHERE i.obj_uid IN (" +
		       selected_uids + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM locker_item_extra_descr d JOIN locker_items i ON "
		       "i.id=d.item_id WHERE i.obj_uid IN (" +
		       selected_uids + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=41 AND obj_uid IN (" +
		       selected_uids + ")")
			      .c_str()) == 0);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(locker_deposit_command.operation_id) +
		       "') AND r.item_uid IN (" + selected_uids +
		       ") AND r.before_revision=3 AND r.after_revision=4")
			      .c_str()) == 2);
	auto locker_withdraw = payload(locker_owner, player_one,
				       item_transfer_reason::locker_withdraw,
				       owner_revision(restarted, locker_owner),
				       owner_revision(restarted, player_one), 4);
	locker_withdraw.selected_item_uid = root_uid;
	locker_withdraw.reason_id = static_cast<int64_t>(chest_id);
	const auto locker_withdraw_command =
		accounted_item_transfer(operation(147), locker_withdraw, lineage, next_epoch, 41);
	const auto withdrawn =
		critical_command_repository_apply(restarted, locker_withdraw_command);
	assert(withdrawn.outcome == critical_apply_outcome::applied && !withdrawn.error_code);
	assert(critical_command_repository_apply(restarted, locker_withdraw_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_items child JOIN player_items parent ON "
		       "parent.id=child.container_id WHERE child.pid=41 AND child.obj_uid=" +
		       std::to_string(child_uid) +
		       " AND parent.obj_uid=" + std::to_string(root_uid))
			      .c_str()) == 1);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_item_affects a JOIN player_items i ON "
		       "i.id=a.item_id WHERE i.pid=41 AND i.obj_uid IN (" +
		       selected_uids + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM player_item_extra_descr d JOIN player_items i ON "
		       "i.id=d.item_id WHERE i.pid=41 AND i.obj_uid IN (" +
		       selected_uids + ")")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM locker_items WHERE locker_id=" +
		       std::to_string(locker_id) + " AND obj_uid IN (" + selected_uids + ")")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				  selected_uids +
				  ") AND owner_type=1 AND owner_id=41 AND state=1 "
				  "AND item_revision=5")
					 .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		       "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		       "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(locker_withdraw_command.operation_id) +
		       "') AND r.item_uid IN (" + selected_uids +
		       ") AND r.before_revision=4 AND r.after_revision=5")
			      .c_str()) == 2);
	execute(restarted, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(42,1001," +
				   std::to_string(root_uid) + ")");
	const uint64_t duplicate_physical_id = mysql_insert_id(restarted);
	assert(duplicate_physical_id);
	auto duplicate_physical = payload(player_one, locker_owner,
					  item_transfer_reason::locker_deposit,
					  owner_revision(restarted, player_one),
					  owner_revision(restarted, locker_owner), 5);
	duplicate_physical.selected_item_uid = root_uid;
	duplicate_physical.reason_id = static_cast<int64_t>(chest_id);
	const auto duplicate_physical_command = accounted_item_transfer(
		operation(149), duplicate_physical, lineage, next_epoch, 41);
	const auto refused_physical =
		critical_command_repository_apply(restarted, duplicate_physical_command);
	assert(refused_physical.outcome == critical_apply_outcome::retryable_failure &&
	       refused_physical.error_code == EILSEQ);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
		       selected_uids + ") AND owner_type=1 AND owner_id=41 AND item_revision=5")
			      .c_str()) == 2);
	assert(scalar(restarted,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_physical_command.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(restarted, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				  "operation_id=UNHEX('" +
				  operation_hex(duplicate_physical_command.operation_id) + "')")
					 .c_str()) == 0);
	execute(restarted,
		"DELETE FROM player_items WHERE id=" + std::to_string(duplicate_physical_id));
	root_uid = original_root_uid;
	child_uid = original_child_uid;
	// Keep the applied roots, epochs, and custody references as retained evidence.
	// Deactivate this test lineage so the next run starts with legacy gameplay allowed.
	execute(connection, "DELETE FROM economic_lineage_state WHERE lineage=UNHEX('" +
				    operation_hex(lineage) + "')");
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_epoch") == epoch_count + 2);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_lineage_state") == lineage_count);
	const uint64_t actual_operations =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation");
	const uint64_t actual_references =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference");
	if (actual_operations != operation_count + 21 ||
	    actual_references != item_reference_count + 32)
		fprintf(stderr,
			"accounted item totals: operations=%llu expected=%llu references=%llu "
			"expected=%llu\n",
			static_cast<unsigned long long>(actual_operations - operation_count), 21ULL,
			static_cast<unsigned long long>(actual_references - item_reference_count),
			32ULL);
	assert(actual_operations == operation_count + 21);
	assert(actual_references == item_reference_count + 32);
	mysql_close(restarted);
	puts("PASS: sourced creation, nested transfers and native pet/locker rows, duplicate quest and world source refusal, theft and retirement retain exact SQL references through replay and epoch change");
}

void check_quest_reward_obligation(MYSQL *&connection)
{
	const uint64_t previous_root_uid = root_uid;
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 1));
	root_uid = item_uid_allocator_next();
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity player = { item_owner_type::player, 4000000002, 0 };
	const item_owner_identity destroyed = { item_owner_type::destruction, 0, 0 };
	auto create = payload(system, player, item_transfer_reason::creation,
			      owner_revision(connection, system),
			      owner_revision(connection, player), ITEM_TRANSFER_ABSENT_REVISION, 1);
	const auto created = apply(connection, 240, create);
	assert(created.outcome == critical_apply_outcome::applied);
	item_transfer_result created_result = {};
	assert(item_transfer_command_decode_result(created.result_payload.data(),
						   created.result_size, &created_result));
	auto offering = payload(player, destroyed, item_transfer_reason::quest_turnin,
				created_result.to_owner_revision,
				owner_revision(connection, destroyed),
				created_result.max_item_revision, 1);
	offering.multi_root = true;
	offering.continuation.kind = item_transfer_continuation_kind::quest_offering;
	offering.continuation.data.assign(56, 0);
	auto put32 = [&](size_t offset, uint32_t value)
	{
		for (size_t index = 0; index < 4; ++index)
			offering.continuation.data[offset + index] = value >> (index * 8);
	};
	auto put64 = [&](size_t offset, uint64_t value)
	{
		for (size_t index = 0; index < 8; ++index)
			offering.continuation.data[offset + index] = value >> (index * 8);
	};
	put32(0, 1);
	put32(4, 4000000002U);
	put32(8, 3);
	put32(12, 0);
	put32(16, 77);
	put32(20, 4200);
	put64(24, 123456789);
	put32(32, 1);
	put64(36, root_uid);
	put32(44, 1);
	put32(48, 1);
	put32(52, 1005);
	auto stale_offering = offering;
	--stale_offering.expected_from_revision;
	const auto rejected = apply(connection, 242, stale_offering);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure);
	assert(scalar(connection, ("SELECT COUNT(*) FROM quest_reward_obligation WHERE "
				   "offering_operation_id=UNHEX('" +
				   operation_hex(242) + "')")
					  .c_str()) == 0);
	// Simulate the game process dying after SQL commits the offering but before
	// it can publish the result or acknowledge the reward. The parent reconnects
	// and verifies both custody and the retained obligation.
	mysql_close(connection);
	connection = nullptr;
	const pid_t writer = fork();
	assert(writer >= 0);
	if (writer == 0)
	{
		MYSQL *writer_connection = open_pool_test_connection();
		if (!writer_connection)
			_exit(80);
		const auto consumed = apply(writer_connection, 241, offering);
		if (consumed.outcome != critical_apply_outcome::applied)
			_exit(81);
		kill(getpid(), SIGKILL);
		_exit(82);
	}
	int writer_status = 0;
	assert(waitpid(writer, &writer_status, 0) == writer);
	assert(WIFSIGNALED(writer_status) && WTERMSIG(writer_status) == SIGKILL);
	connection = open_pool_test_connection();
	assert(connection);
	const std::string operation_id = operation_hex(241);
	const std::string continuation_hex = bytes_hex(offering.continuation.data);
	assert(scalar(connection, ("SELECT COUNT(*) FROM quest_reward_obligation WHERE "
				   "offering_operation_id=UNHEX('" +
				   operation_id +
				   "') AND player_pid=4000000002 AND acknowledged_at IS NULL AND "
				   "continuation=UNHEX('" +
				   continuation_hex + "')")
					  .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_ownership_ledger WHERE "
				   "operation_id=UNHEX('" +
				   operation_id + "')")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		       std::to_string(root_uid) + " AND state=" +
		       std::to_string(static_cast<unsigned>(item_custody_state::destroyed)) +
		       " AND owner_type=" +
		       std::to_string(static_cast<unsigned>(item_owner_type::destruction)))
			      .c_str()) == 1);
	MYSQL *reopened = open_pool_test_connection();
	assert(reopened);
	std::vector<quest_reward_obligation_record> pending_rewards;
	unsigned int database_error = 0;
	assert(quest_reward_obligation_repository_pending(reopened, 4000000002U, &pending_rewards,
							  &database_error) ==
	       quest_reward_obligation_result::ok);
	assert(database_error == 0 && pending_rewards.size() == 1 &&
	       critical_operation_id_equal(pending_rewards[0].offering_operation, operation(241)) &&
	       pending_rewards[0].continuation == offering.continuation.data &&
	       pending_rewards[0].terms.rewards[0].number == 1005);
	execute(connection, "UPDATE quest_reward_obligation SET continuation=X'00' WHERE "
			    "offering_operation_id=UNHEX('" +
				    operation_id + "')");
	assert(quest_reward_obligation_repository_pending(reopened, 4000000002U, &pending_rewards,
							  &database_error) ==
		       quest_reward_obligation_result::corrupt &&
	       pending_rewards.size() == 1);
	execute(connection, "UPDATE quest_reward_obligation SET continuation=UNHEX('" +
				    continuation_hex + "') WHERE offering_operation_id=UNHEX('" +
				    operation_id + "')");
	assert(quest_reward_obligation_repository_pending(reopened, 4000000001U, &pending_rewards,
							  &database_error) ==
		       quest_reward_obligation_result::ok &&
	       pending_rewards.empty());
	assert(quest_reward_obligation_repository_acknowledge(reopened, 4000000001U, operation(241),
							      &database_error) ==
	       quest_reward_obligation_result::not_found);
	assert(quest_reward_obligation_repository_acknowledge(reopened, 4000000002U, operation(241),
							      &database_error) ==
	       quest_reward_obligation_result::ok);
	assert(quest_reward_obligation_repository_acknowledge(reopened, 4000000002U, operation(241),
							      &database_error) ==
	       quest_reward_obligation_result::already_acknowledged);
	assert(quest_reward_obligation_repository_pending(reopened, 4000000002U, &pending_rewards,
							  &database_error) ==
		       quest_reward_obligation_result::ok &&
	       pending_rewards.empty());
	mysql_close(reopened);
	assert(apply(connection, 241, offering).outcome == critical_apply_outcome::already_applied);
	critical_command command = {};
	assert(item_transfer_command_build(&command, operation(241), offering,
					   critical_source_site::operator_repair,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_repository_reconcile(connection, command).outcome ==
	       critical_apply_outcome::already_applied);
	root_uid = previous_root_uid;
	puts("PASS: committed SQL quest offering retains exact reward obligation and replays once");
}

void check_accounted_craft_conservation(MYSQL *connection)
{
	const item_owner_identity owner = { item_owner_type::player, 552, 0 };
	const auto lineage = operation(67);
	const auto epoch = operation(68);
	execute(connection,
		"INSERT INTO player_data(pid,name,account_name) VALUES(552,'AccountedCraft','CraftFixture')");
	execute(connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,552,0,1)");
	execute(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(55201,55201,NULL,1,552,0,1,102,1,7),(55202,55201,55201,1,552,0,1,103,1,0)");
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
			operation_hex(66) +
			"'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
	execute(connection,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
			"'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(66) + "'))");
	execute(connection,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) + "'),0)");
	player_item_snapshot output = runtime_item(55203, 771, 1900001234);
	output.craftsmanship = 4;
	output.dynamic_affects.push_back({ 1, 2, 3 });
	player_item_snapshot child = runtime_item(55204, 772, 1900001235);
	child.parent_index = 0;
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode({ output, child }, &encoded) ==
	       player_snapshot_codec_result::ok);
	item_transfer_payload craft = {};
	craft.from_owner = craft.to_owner = owner;
	craft.expected_from_revision = craft.expected_to_revision = 1;
	craft.reason = item_transfer_reason::craft;
	craft.reason_id = 552;
	craft.multi_root = true;
	craft.selected_item_uid = output.object_uid;
	craft.item_count = 2;
	craft.items[0] = { 55201, 55201, 0, 1, 102, item_custody_state::active };
	craft.items[1] = { 55202, 55201, 55201, 1, 103, item_custody_state::active };
	craft.item_blob_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), craft.item_blob.begin());
	const auto success = accounted_item_transfer(operation(69), craft, lineage, epoch, 552,
						     economic_source_kind::crafting);
	auto applied = critical_command_repository_apply(connection, success);
	if (applied.outcome != critical_apply_outcome::applied)
		fprintf(stderr, "accounted craft outcome=%u error=%u stage=%u sql=%s\n",
			(unsigned)applied.outcome, applied.error_code,
			(unsigned)applied.failure_stage, mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	assert(owner_revision(connection, owner) == 2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN(55201,55202) AND owner_type=8 AND state=2 AND item_revision=2 AND equipment_slot=0") ==
	       2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner WHERE item_uid=55204 AND root_item_uid=55203 AND parent_item_uid=55203 AND state=1") ==
	       1);
	const auto canonical = read_blob(
		connection,
		("SELECT canonical_plan FROM economic_accounting_operation WHERE operation_id=UNHEX('" +
		 operation_hex(success.operation_id) + "')")
			.c_str());
	economic_accounting_plan plan;
	assert(economic_plan_decode(canonical, &plan) == economic_accounting_error::ok);
	assert(plan.item_events.size() == 4 && plan.item_events[0].before.equipment_slot == 7 &&
	       plan.item_events[0].after.equipment_slot == 0 &&
	       plan.item_events[1].after.parent_uid == 55201);
	for (uint16_t index = 0; index < 4; ++index)
	{
		economic_accounting_item_reference ref = {};
		assert(economic_accounting_item_reference_find_by_legacy(
			connection, success.operation_id, index, &ref));
		assert(ref.event_index == index && ref.item_uid == 55201u + index &&
		       ref.before_revision == (index < 2 ? 1u : 0u) &&
		       ref.after_revision == (index < 2 ? 2u : 1u));
	}
	MYSQL *reopened = open_pool_test_connection();
	assert(reopened);
	assert(critical_command_repository_reconcile(reopened, success).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(reopened, success).outcome ==
	       critical_apply_outcome::already_applied);
	player_load_request request = {};
	request.request_id = 552;
	request.pid = 552;
	request.account_name = "CraftFixture";
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	const auto loaded = player_load_repository_execute(reopened, request);
	assert(loaded.outcome == player_load_outcome::applied && loaded.snapshot.items.size() == 2);
	assert(loaded.snapshot.items[0].object_uid == output.object_uid &&
	       loaded.snapshot.items[0].timers == output.timers &&
	       loaded.snapshot.items[0].craftsmanship == output.craftsmanship &&
	       loaded.snapshot.items[0].dynamic_affects.size() == output.dynamic_affects.size() &&
	       loaded.snapshot.items[1].object_uid == child.object_uid &&
	       loaded.snapshot.items[1].parent_index == 0);
	execute(reopened,
		"INSERT INTO player_spell_effect_receipt(pid,operation_id,effect_id) VALUES(552,UNHEX('" +
			operation_hex(success.operation_id) + "'),1)");
	request.pid = 0;
	request.account_name.clear();
	request.player_name = "AccountedCraft";
	request.pending_spell_effect_operations = { success.operation_id };
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	const auto pending_load = player_load_repository_execute(reopened, request);
	assert(pending_load.outcome == player_load_outcome::applied &&
	       pending_load.metrics.query_count == PLAYER_LOAD_QUERY_MAX &&
	       pending_load.spell_effect_receipts.size() == 1 &&
	       pending_load.spell_effect_receipts[0].effect_id ==
		       static_cast<uint32_t>(item_spell_component_effect::faerie_sight));
	// A rebuilt command cannot issue another result for consumed inputs.
	const auto duplicate = accounted_item_transfer(operation(70), craft, lineage, epoch, 552,
						       economic_source_kind::crafting);
	assert(critical_command_repository_apply(reopened, duplicate).outcome ==
	       critical_apply_outcome::terminal_failure);
	assert(owner_revision(reopened, owner) == 2);
	craft.expected_from_revision = craft.expected_to_revision = 2;
	craft.selected_item_uid = 55203;
	craft.items[0] = { 55203, 55203, 0, 1, output.vnum, item_custody_state::active };
	craft.items[1] = { 55204, 55203, 55203, 1, child.vnum, item_custody_state::active };
	craft.item_blob_size = 0;
	const auto failure = accounted_item_transfer(operation(71), craft, lineage, epoch, 552,
						     economic_source_kind::crafting);
	assert(critical_command_repository_apply(reopened, failure).outcome ==
	       critical_apply_outcome::applied);
	assert(owner_revision(reopened, owner) == 3);
	assert(scalar(reopened, "SELECT COUNT(*) FROM player_items WHERE pid=552") == 0);
	assert(critical_command_repository_reconcile(reopened, failure).outcome ==
	       critical_apply_outcome::already_applied);
	mysql_close(reopened);
	puts("PASS: accounted SQL craft preserves exact consumed/output witnesses, rich nested state, fresh-session replay, duplicate refusal and intended failure");
}

void check_craft_conservation(MYSQL *connection)
{
	const item_owner_identity owner = { item_owner_type::player, 551, 0 };
	execute(connection,
		"INSERT INTO player_data(pid,name,account_name) VALUES(551,'CraftConservation','CraftFixture')");
	execute(connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,551,0,1)");
	execute(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(55101,55101,NULL,1,551,0,1,102,1),(55102,55102,NULL,1,551,0,1,103,1)");
	player_item_snapshot output = runtime_item(55103, 771, 1900001234);
	output.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	output.craftsmanship = 4;
	output.anti_flags = 11;
	output.anti2_flags = 12;
	output.extra2_flags = 13;
	output.generated_key = 551;
	output.dynamic_affects.push_back({ 1, 2, 3 });
	const auto encoded = encode_runtime_item(output);
	item_transfer_payload craft = {};
	craft.from_owner = craft.to_owner = owner;
	craft.expected_from_revision = craft.expected_to_revision = 1;
	craft.reason = item_transfer_reason::craft;
	craft.reason_id = 551;
	craft.multi_root = true;
	craft.selected_item_uid = output.object_uid;
	craft.item_count = 2;
	craft.items[0] = { 55101, 55101, 0, 1, 102, item_custody_state::active };
	craft.items[1] = { 55102, 55102, 0, 1, 103, item_custody_state::active };
	craft.item_blob_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), craft.item_blob.begin());
	auto applied = apply(connection, 31, craft);
	if (applied.outcome != critical_apply_outcome::applied)
		fprintf(stderr, "craft outcome=%u error=%u stage=%u sql=%s\n",
			(unsigned)applied.outcome, applied.error_code,
			(unsigned)applied.failure_stage, mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner WHERE owner_id=551 AND owner_type=1 AND state=1") ==
	       1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN(55101,55102) AND owner_type=8 AND state=2") ==
	       2);
	assert(owner_revision(connection, owner) == 2);
	const auto stored = read_blob(
		connection,
		"SELECT state.payload FROM player_item_runtime_state state JOIN player_items pi ON pi.id=state.item_id WHERE pi.obj_uid=55103");
	assert(stored == encoded);
	// Exercise the ordinary save writer and a fresh repository load, independently
	// of the craft receipt. Rich state must survive replacing physical item rows.
	player_snapshot checkpoint = {};
	checkpoint.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	checkpoint.pid = 551;
	checkpoint.revision = 1;
	checkpoint.components = PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_EQUIPMENT;
	checkpoint.items = { output };
	// Published carried slots use zero on this branch; the frozen detached craft
	// output uses -1 until publication.
	checkpoint.items[0].equipment_slot = 0;
	const auto checkpoint_result = player_snapshot_repository_apply(connection, checkpoint);
	if (checkpoint_result.outcome != player_save_apply_outcome::applied)
		fprintf(stderr, "craft checkpoint outcome=%u error=%u\n",
			(unsigned)checkpoint_result.outcome, checkpoint_result.error_code);
	assert(checkpoint_result.outcome == player_save_apply_outcome::applied);
	player_load_request request = {};
	request.request_id = 551;
	request.pid = 551;
	request.account_name = "CraftFixture";
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	auto reloaded = player_load_repository_execute(connection, request);
	if (reloaded.outcome != player_load_outcome::applied)
		fprintf(stderr, "craft reload outcome=%u error=%u component=%s\n",
			(unsigned)reloaded.outcome, reloaded.error_code,
			reloaded.failed_component ? reloaded.failed_component : "none");
	assert(reloaded.outcome == player_load_outcome::applied &&
	       reloaded.snapshot.items.size() == 1);
	assert(reloaded.metrics.query_count == PLAYER_LOAD_PID_QUERY_MAX);
	player_load_request by_name = request;
	by_name.request_id = 552;
	by_name.pid = 0;
	by_name.account_name.clear();
	by_name.player_name = "CraftConservation";
	by_name.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	const auto named = player_load_repository_execute(connection, by_name);
	assert(named.outcome == player_load_outcome::applied && named.pid == 551 &&
	       named.metrics.query_count == reloaded.metrics.query_count + 1 &&
	       named.metrics.query_count == PLAYER_LOAD_NAME_QUERY_MAX);
	const auto &loaded = reloaded.snapshot.items[0];
	assert(loaded.object_uid == output.object_uid &&
	       loaded.craftsmanship == output.craftsmanship);
	assert(loaded.generated_key == output.generated_key && loaded.timers == output.timers);
	assert(loaded.anti_flags == output.anti_flags && loaded.anti2_flags == output.anti2_flags &&
	       loaded.extra2_flags == output.extra2_flags);
	assert(loaded.dynamic_affects.size() == output.dynamic_affects.size() &&
	       loaded.extra_descriptions.size() == output.extra_descriptions.size());
	applied = apply(connection, 31, craft);
	assert(applied.outcome == critical_apply_outcome::already_applied);
	assert(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=55103") == 1);
	// A later stale command cannot consume the committed output or admit another.
	craft.expected_from_revision = craft.expected_to_revision = 1;
	craft.selected_item_uid = 55104;
	output.object_uid = 55104;
	const auto stale_bytes = encode_runtime_item(output);
	craft.item_blob_size = stale_bytes.size();
	std::copy(stale_bytes.begin(), stale_bytes.end(), craft.item_blob.begin());
	applied = apply(connection, 32, craft);
	assert(applied.outcome == critical_apply_outcome::terminal_failure);
	assert(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=55104") == 0);
	assert(owner_revision(connection, owner) == 2);
	craft = {};
	craft.from_owner = craft.to_owner = owner;
	craft.expected_from_revision = craft.expected_to_revision = 2;
	craft.reason = item_transfer_reason::craft;
	craft.reason_id = 551;
	craft.multi_root = true;
	craft.selected_item_uid = 55103;
	craft.item_count = 1;
	craft.items[0] = { 55103, 55103, 0, 1, output.vnum, item_custody_state::active };
	applied = apply(connection, 33, craft);
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(owner_revision(connection, owner) == 3);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner WHERE owner_id=551 AND owner_type=1 AND state=1") ==
	       0);
	assert(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=55103") == 0);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM player_item_runtime_state state LEFT JOIN player_items pi ON pi.id=state.item_id WHERE pi.id IS NULL") ==
	       0);
}

int main()
{
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("ITEM_TRANSFER_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	const uint64_t baseline_epoch_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_epoch");
	const uint64_t baseline_lineage_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_lineage_state");
	const uint64_t baseline_operation_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation");
	const uint64_t baseline_item_reference_count =
		scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference");
	execute(connection, "INSERT IGNORE INTO player_data(pid,name) VALUES"
			    "(4000000001,'ItemTransferOne'),(4000000002,'ItemTransferTwo'),"
			    "(41,'RestitutionSource'),(42,'RestitutionTarget')");
	ensure_collector_boundary_fixture(connection);
	assert(critical_operation_id_generate(&run_operation));
	const uint64_t allocator_start =
		scalar(connection, "SELECT next_uid FROM item_uid_allocator WHERE allocator_id=1");
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	assert(root_uid == allocator_start && child_uid == allocator_start + 1);

	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity player_one = { item_owner_type::player, 4000000001, 0 };
	const item_owner_identity player_two = { item_owner_type::player, 4000000002, 0 };
	const item_owner_identity destroyed = { item_owner_type::destruction, 0, 0 };
	uint64_t system_revision = owner_revision(connection, system);
	uint64_t player_one_revision = owner_revision(connection, player_one);
	check_staged_accounting_refusal(connection,
					payload(system, player_one, item_transfer_reason::creation,
						system_revision, player_one_revision,
						ITEM_TRANSFER_ABSENT_REVISION));
	auto cyclic_payload = payload(system, player_one, item_transfer_reason::creation,
				      system_revision, player_one_revision,
				      ITEM_TRANSFER_ABSENT_REVISION);
	cyclic_payload.items[1].parent_item_uid = child_uid;
	critical_command invalid = {};
	assert(!item_transfer_command_build(&invalid, operation(8), cyclic_payload,
					    critical_source_site::operator_repair,
					    critical_deadline_class::interactive));
	critical_apply_result created =
		apply(connection, 1,
		      payload(system, player_one, item_transfer_reason::creation, system_revision,
			      player_one_revision, ITEM_TRANSFER_ABSENT_REVISION));
	if (created.outcome != critical_apply_outcome::applied || created.error_code)
		fprintf(stderr, "initial item creation failed: outcome=%u error=%u\n",
			static_cast<unsigned int>(created.outcome), created.error_code);
	assert(created.outcome == critical_apply_outcome::applied && created.error_code == 0);
	// A legacy grant retains its native custody/inbox guarantees without inventing
	// accounting evidence or activating a lineage. Link the real reference writer.
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_epoch") == baseline_epoch_count);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_lineage_state") ==
	       baseline_lineage_count);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation") ==
	       baseline_operation_count);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference") ==
	       baseline_item_reference_count);
	item_transfer_result created_result = {};
	assert(item_transfer_command_decode_result(created.result_payload.data(),
						   created.result_size, &created_result));
	assert(created_result.item_count == 2 && created_result.max_item_revision == 1);
	critical_command duplicate_command = {};
	auto create_payload = payload(system, player_one, item_transfer_reason::creation,
				      system_revision, player_one_revision,
				      ITEM_TRANSFER_ABSENT_REVISION);
	assert(item_transfer_command_build(&duplicate_command, operation(1), create_payload,
					   critical_source_site::operator_repair,
					   critical_deadline_class::interactive));
	duplicate_command.accepted_at_usec = 1;
	critical_apply_result duplicate =
		critical_command_repository_apply(connection, duplicate_command);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				   std::to_string(root_uid))
					  .c_str()) == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=4000000001 AND obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items child JOIN player_items parent ON "
		       "parent.id=child.container_id WHERE child.obj_uid=" +
		       std::to_string(child_uid) +
		       " AND parent.obj_uid=" + std::to_string(root_uid))
			      .c_str()) == 1);

	uint64_t player_two_revision = owner_revision(connection, player_two);
	critical_apply_result incomplete =
		apply(connection, 2,
		      payload(player_one, player_two, item_transfer_reason::synthetic,
			      created_result.to_owner_revision, player_two_revision, 1, 1));
	assert(incomplete.outcome == critical_apply_outcome::terminal_failure &&
	       incomplete.error_code == EMSGSIZE);
	critical_apply_result stale =
		apply(connection, 3,
		      payload(player_one, player_two, item_transfer_reason::synthetic,
			      created_result.to_owner_revision - 1, player_two_revision, 1));
	assert(stale.outcome == critical_apply_outcome::terminal_failure &&
	       stale.error_code == ESTALE);
	const auto give_payload = payload(player_one, player_two, item_transfer_reason::player_give,
					  created_result.to_owner_revision, player_two_revision, 1);
	const uint64_t native_root_id = scalar(
		connection,
		("SELECT id FROM player_items WHERE obj_uid=" + std::to_string(root_uid)).c_str());
	execute(connection, "INSERT INTO player_items(pid,vnum,container_id) VALUES(4000000001,"
			    "1003," +
				    std::to_string(native_root_id) + ")");
	const uint64_t unexpected_child_id = mysql_insert_id(connection);
	const critical_apply_result hidden_child = apply(connection, 249, give_payload);
	assert(hidden_child.outcome == critical_apply_outcome::terminal_failure &&
	       hidden_child.error_code == EILSEQ);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_ownership_ledger WHERE "
				   "operation_id=UNHEX('" +
				   operation_hex(249) + "')")
					  .c_str()) == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE id=" +
				   std::to_string(native_root_id) + " AND pid=4000000001")
					  .c_str()) == 1);
	execute(connection,
		"DELETE FROM player_items WHERE id=" + std::to_string(unexpected_child_id));
	execute(connection, "UPDATE player_items SET pid=4000000002 WHERE id=" +
				    std::to_string(native_root_id));
	const critical_apply_result foreign_row = apply(connection, 250, give_payload);
	assert(foreign_row.outcome == critical_apply_outcome::terminal_failure &&
	       foreign_row.error_code == EILSEQ);
	execute(connection, "UPDATE player_items SET pid=4000000001 WHERE id=" +
				    std::to_string(native_root_id));
	execute(connection,
		"UPDATE player_items SET vnum=1009 WHERE id=" + std::to_string(native_root_id));
	const critical_apply_result wrong_template = apply(connection, 251, give_payload);
	assert(wrong_template.outcome == critical_apply_outcome::terminal_failure &&
	       wrong_template.error_code == EILSEQ);
	execute(connection,
		"UPDATE player_items SET vnum=1001 WHERE id=" + std::to_string(native_root_id));
	critical_apply_result moved = apply(connection, 4, give_payload);
	assert(moved.outcome == critical_apply_outcome::applied);
	item_transfer_result moved_result = {};
	assert(item_transfer_command_decode_result(moved.result_payload.data(), moved.result_size,
						   &moved_result));
	assert(moved_result.max_item_revision == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=4000000002 AND obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=4000000001 AND obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(child_uid) + ")")
			      .c_str()) == 0);
	critical_apply_result replayed_give = apply(connection, 4, give_payload);
	item_transfer_result replayed_give_result = {};
	// The populated cross-owner give must apply exactly once when its operation is replayed.
	assert(replayed_give.outcome == critical_apply_outcome::already_applied &&
	       item_transfer_command_decode_result(replayed_give.result_payload.data(),
						   replayed_give.result_size,
						   &replayed_give_result) &&
	       replayed_give_result.from_owner_revision == moved_result.from_owner_revision &&
	       replayed_give_result.to_owner_revision == moved_result.to_owner_revision &&
	       replayed_give_result.max_item_revision == moved_result.max_item_revision);

	uint64_t destruction_revision = owner_revision(connection, destroyed);
	execute(connection, ("UPDATE item_current_owner SET item_revision=18446744073709551615 "
			     "WHERE root_item_uid=" +
			     std::to_string(root_uid))
				    .c_str());
	critical_apply_result overflow =
		apply(connection, 5,
		      payload(player_two, destroyed, item_transfer_reason::destruction,
			      moved_result.to_owner_revision, destruction_revision,
			      std::numeric_limits<uint64_t>::max()));
	assert(overflow.outcome == critical_apply_outcome::terminal_failure &&
	       overflow.error_code == ERANGE);
	execute(connection, ("UPDATE item_current_owner SET item_revision=2 WHERE root_item_uid=" +
			     std::to_string(root_uid))
				    .c_str());
	critical_apply_result destruction =
		apply(connection, 6,
		      payload(player_two, destroyed, item_transfer_reason::destruction,
			      moved_result.to_owner_revision, destruction_revision, 2));
	assert(destruction.outcome == critical_apply_outcome::applied);
	const std::string uid_list = std::to_string(root_uid) + "," + std::to_string(child_uid);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				   std::to_string(root_uid) +
				   " AND owner_type=8 AND state=2 AND item_revision=3")
					  .c_str()) == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE item_uid IN (" + uid_list +
		       ")")
			      .c_str()) == 6);
	assert(scalar(connection,
		      ("SELECT COUNT(DISTINCT outbox.operation_id) FROM critical_outbox outbox "
		       "JOIN item_ownership_ledger ledger ON ledger.operation_id=outbox.operation_id "
		       "WHERE ledger.item_uid IN (" +
		       uid_list + ")")
			      .c_str()) == 3);
	critical_apply_result collision = apply(
		connection, 7,
		payload(system, player_one, item_transfer_reason::creation,
			owner_revision(connection, system), owner_revision(connection, player_one),
			ITEM_TRANSFER_ABSENT_REVISION));
	assert(collision.outcome == critical_apply_outcome::terminal_failure &&
	       collision.error_code == EEXIST);

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 3));
	const uint64_t batch_first_root = item_uid_allocator_next();
	const uint64_t batch_first_child = item_uid_allocator_next();
	const uint64_t batch_second_root = item_uid_allocator_next();
	system_revision = owner_revision(connection, system);
	player_one_revision = owner_revision(connection, player_one);
	const item_transfer_payload batch_payload =
		multi_root_creation_payload(batch_first_root, batch_first_child, batch_second_root,
					    system_revision, player_one_revision);
	critical_apply_result batch_created = apply(connection, 14, batch_payload);
	assert(batch_created.outcome == critical_apply_outcome::applied &&
	       batch_created.error_code == 0);
	item_transfer_result batch_created_result = {};
	assert(item_transfer_command_decode_result(batch_created.result_payload.data(),
						   batch_created.result_size,
						   &batch_created_result));
	assert(batch_created_result.root_item_uid == batch_first_root &&
	       batch_created_result.item_count == 3 &&
	       batch_created_result.from_owner_revision == system_revision + 1 &&
	       batch_created_result.to_owner_revision == player_one_revision + 1 &&
	       batch_created_result.max_item_revision == 1);
	const std::string batch_uid_list = std::to_string(batch_first_root) + "," +
					   std::to_string(batch_first_child) + "," +
					   std::to_string(batch_second_root);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				   batch_uid_list + ") AND owner_type=1 AND owner_id=" +
				   std::to_string(player_one.id) + " AND state=1")
					  .c_str()) == 3);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(batch_first_child) +
				   " AND root_item_uid=" + std::to_string(batch_first_root) +
				   " AND parent_item_uid=" + std::to_string(batch_first_root))
					  .c_str()) == 1);
	batch_created = apply(connection, 14, batch_payload);
	item_transfer_result batch_replayed_result = {};
	assert(batch_created.outcome == critical_apply_outcome::already_applied &&
	       item_transfer_command_decode_result(batch_created.result_payload.data(),
						   batch_created.result_size,
						   &batch_replayed_result) &&
	       batch_replayed_result.to_owner_revision == batch_created_result.to_owner_revision);

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 4));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const uint64_t container_uid = item_uid_allocator_next();
	const uint64_t nested_created_uid = item_uid_allocator_next();
	system_revision = owner_revision(connection, system);
	player_one_revision = owner_revision(connection, player_one);
	critical_apply_result reparent_items_created =
		apply(connection, 9,
		      payload(system, player_one, item_transfer_reason::creation, system_revision,
			      player_one_revision, ITEM_TRANSFER_ABSENT_REVISION));
	assert(reparent_items_created.outcome == critical_apply_outcome::applied);
	item_transfer_result reparent_items_result = {};
	assert(item_transfer_command_decode_result(reparent_items_created.result_payload.data(),
						   reparent_items_created.result_size,
						   &reparent_items_result));
	root_uid = container_uid;
	critical_apply_result container_created = apply(
		connection, 10,
		payload(system, player_one, item_transfer_reason::creation,
			reparent_items_result.from_owner_revision,
			reparent_items_result.to_owner_revision, ITEM_TRANSFER_ABSENT_REVISION, 1));
	assert(container_created.outcome == critical_apply_outcome::applied);
	item_transfer_result container_result = {};
	assert(item_transfer_command_decode_result(container_created.result_payload.data(),
						   container_created.result_size,
						   &container_result));
	root_uid = nested_created_uid;
	child_uid = nested_created_uid;
	auto nested_creation = payload(system, player_one, item_transfer_reason::creation,
				       container_result.from_owner_revision,
				       container_result.to_owner_revision,
				       ITEM_TRANSFER_ABSENT_REVISION, 1);
	nested_creation.selected_item_uid = nested_created_uid;
	nested_creation.target_root_item_uid = container_uid;
	nested_creation.target_parent_item_uid = container_uid;
	nested_creation.expected_target_parent_revision = 1;
	critical_apply_result nested_created = apply(connection, 13, nested_creation);
	assert(nested_created.outcome == critical_apply_outcome::applied);
	item_transfer_result nested_created_result = {};
	assert(item_transfer_command_decode_result(nested_created.result_payload.data(),
						   nested_created.result_size,
						   &nested_created_result));
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(nested_created_uid) +
				   " AND root_item_uid=" + std::to_string(container_uid) +
				   " AND parent_item_uid=" + std::to_string(container_uid))
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items child JOIN player_items parent ON "
		       "parent.id=child.container_id WHERE child.obj_uid=" +
		       std::to_string(nested_created_uid) +
		       " AND parent.obj_uid=" + std::to_string(container_uid))
			      .c_str()) == 1);

	root_uid = container_uid - 2;
	child_uid = container_uid - 1;
	auto reparent = payload(player_one, player_one, item_transfer_reason::player_put,
				nested_created_result.to_owner_revision,
				nested_created_result.to_owner_revision, 1);
	reparent.selected_item_uid = root_uid;
	reparent.target_root_item_uid = container_uid;
	reparent.target_parent_item_uid = container_uid;
	reparent.expected_target_parent_revision = 1;
	critical_apply_result reparented = apply(connection, 11, reparent);
	assert(reparented.outcome == critical_apply_outcome::applied);
	item_transfer_result reparented_result = {};
	assert(item_transfer_command_decode_result(reparented.result_payload.data(),
						   reparented.result_size, &reparented_result));
	assert(reparented_result.from_owner_revision == reparented_result.to_owner_revision);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(root_uid) +
				   " AND root_item_uid=" + std::to_string(container_uid) +
				   " AND parent_item_uid=" + std::to_string(container_uid))
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items child JOIN player_items parent ON "
		       "parent.id=child.container_id WHERE child.obj_uid=" +
		       std::to_string(root_uid) +
		       " AND parent.obj_uid=" + std::to_string(container_uid))
			      .c_str()) == 1);

	item_transfer_payload detach = {};
	detach.from_owner = player_one;
	detach.to_owner = player_one;
	detach.reason = item_transfer_reason::player_get;
	detach.reason_id = 78;
	detach.expected_from_revision = reparented_result.to_owner_revision;
	detach.expected_to_revision = reparented_result.to_owner_revision;
	detach.selected_item_uid = child_uid;
	detach.target_root_item_uid = child_uid;
	detach.item_count = 1;
	detach.items[0] = {
		child_uid, container_uid, root_uid, 2, 1002, item_custody_state::active
	};
	attach_blob(&detach, { physical_item(child_uid, 1002, PLAYER_SNAPSHOT_NO_PARENT) });
	critical_apply_result detached = apply(connection, 12, detach);
	assert(detached.outcome == critical_apply_outcome::applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(child_uid) + " AND root_item_uid=" +
				   std::to_string(child_uid) + " AND parent_item_uid IS NULL")
					  .c_str()) == 1);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
				   std::to_string(child_uid) + " AND container_id IS NULL")
					  .c_str()) == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner own LEFT JOIN player_items payload "
		      "ON payload.obj_uid=own.item_uid AND payload.pid=own.owner_id WHERE "
		      "own.owner_type=1 AND own.state=1 AND payload.id IS NULL") == 0);

	// Administrative removal of a durable container must advance custody before
	// its saved projection disappears. Its contents are detached, not destroyed.
	execute(connection, "START TRANSACTION");
	const bool revoked = item_transfer_repository_revoke_roots_preserving_children(
		connection, &container_uid, 1);
	assert(revoked);
	execute(connection,
		("UPDATE player_items child JOIN player_items reward ON "
		 "child.container_id=reward.id SET child.container_id=reward.container_id WHERE "
		 "reward.obj_uid=" +
		 std::to_string(container_uid)));
	execute(connection,
		"DELETE FROM player_items WHERE obj_uid=" + std::to_string(container_uid));
	execute(connection, "COMMIT");
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(container_uid) + " AND owner_type=8 AND state=2")
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(nested_created_uid) +
		       ") AND owner_type=1 AND owner_id=4000000001 AND state=1 AND "
		       "root_item_uid=item_uid AND parent_item_uid IS NULL")
			      .c_str()) == 2);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
		       std::to_string(root_uid) + "," + std::to_string(nested_created_uid) +
		       ") AND pid=4000000001 AND container_id IS NULL")
			      .c_str()) == 2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM item_current_owner own LEFT JOIN player_items payload "
		      "ON payload.obj_uid=own.item_uid AND payload.pid=own.owner_id WHERE "
		      "own.owner_type=1 AND own.state=1 AND payload.id IS NULL") == 0);

	check_restitution_runtime_transfer(connection);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_epoch") == baseline_epoch_count);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation") ==
	       baseline_operation_count);
	assert(scalar(connection, "SELECT COUNT(*) FROM economic_accounting_item_reference") ==
	       baseline_item_reference_count);
	puts("PASS: legacy grants, transfers and replay retain custody without implicit accounting");

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	assert(item_uid_allocator_next() == allocator_start + 9);
	assert(item_uid_allocator_next() == allocator_start + 10);
	check_deferred_accounting_reference_rollback(connection);
	check_dispatch_fence_commit_and_rollback(connection);
	check_sql_accounted_item_transfer(connection);
	check_quest_reward_obligation(connection);
	for (uint8_t id = 1; id <= 16; ++id)
	{
		const std::string hex = operation_hex(id);
		execute(connection,
			("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o ON "
			 "o.outbox_id=d.outbox_id WHERE o.operation_id=UNHEX('" +
			 hex + "')")
				.c_str());
		execute(connection,
			("DELETE FROM critical_outbox WHERE operation_id=UNHEX('" + hex + "')")
				.c_str());
	}
	check_craft_conservation(connection);
	check_accounted_craft_conservation(connection);
	mysql_close(connection);
	return 0;
}
