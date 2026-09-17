#include "persistence/critical_command_repository.h"
#include "item/item_transfer_command.h"
#include "item/item_uid_allocator.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <string>
#include <vector>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}

unsigned long next_obj_uid = 1;
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
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

void execute(MYSQL *connection, const char *sql)
{
	assert(mysql_real_query(connection, sql, strlen(sql)) == 0);
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
	const std::string id = "UNHEX(REPEAT('a1',16))";
	execute(connection,
		"INSERT INTO player_death_restitution_receipt(restitution_id) VALUES (" + id + ")");
	execute(connection,
		"INSERT INTO player_death_restitution_item(restitution_id,item_uid,vnum) VALUES (" +
			id + "," + std::to_string(uid) + ",1901)");
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
	const uint64_t uid = 9000001;
	const item_owner_identity source = { item_owner_type::player, 41, 0 };
	const item_owner_identity target = { item_owner_type::player, 42, 0 };
	const player_item_snapshot initial = runtime_item(uid, 7, 1700000000);
	const player_item_snapshot mutated = runtime_item(uid, 7007, 1900000000);
	const std::vector<uint8_t> initial_payload = encode_runtime_item(initial);
	const std::vector<uint8_t> mutated_payload = encode_runtime_item(mutated);
	prepare_restitution_runtime_fixture(connection, uid, initial_payload);
	execute(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES (9000001,9000001,NULL,1,41,0,1,1901,1)");
	const uint64_t source_revision = owner_revision(connection, source);
	const uint64_t target_revision = owner_revision(connection, target);
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
		"SELECT state_payload FROM player_death_restitution_runtime WHERE item_uid=9000001");
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
	assert(scalar(connection,
		      "SELECT owner_id FROM item_current_owner WHERE item_uid=9000001") == 42);
}
} // namespace

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("ITEM_TRANSFER_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
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
	assert(created.outcome == critical_apply_outcome::applied && created.error_code == 0);
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
	critical_apply_result moved = apply(connection, 4, give_payload);
	assert(moved.outcome == critical_apply_outcome::applied);
	item_transfer_result moved_result = {};
	assert(item_transfer_command_decode_result(moved.result_payload.data(), moved.result_size,
						   &moved_result));
	assert(moved_result.max_item_revision == 2);
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
	critical_apply_result detached = apply(connection, 12, detach);
	assert(detached.outcome == critical_apply_outcome::applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(child_uid) + " AND root_item_uid=" +
				   std::to_string(child_uid) + " AND parent_item_uid IS NULL")
					  .c_str()) == 1);

	check_restitution_runtime_transfer(connection);

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	assert(item_uid_allocator_next() == allocator_start + 9);
	assert(item_uid_allocator_next() == allocator_start + 10);
	for (uint8_t id = 1; id <= 15; ++id)
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
	mysql_close(connection);
	return 0;
}
