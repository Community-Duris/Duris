#include "persistence/critical_command_repository.h"
#include "item/item_transfer_command.h"
#include "item/item_uid_allocator.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <string>

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

critical_apply_result apply(MYSQL *connection, uint8_t id, const item_transfer_payload &value)
{
	critical_command command = {};
	assert(item_transfer_command_build(&command, operation(id), value,
					   critical_source_site::operator_repair,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return critical_command_repository_apply(connection, command);
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
	critical_apply_result moved =
		apply(connection, 4,
		      payload(player_one, player_two, item_transfer_reason::synthetic,
			      created_result.to_owner_revision, player_two_revision, 1));
	assert(moved.outcome == critical_apply_outcome::applied);
	item_transfer_result moved_result = {};
	assert(item_transfer_command_decode_result(moved.result_payload.data(), moved.result_size,
						   &moved_result));
	assert(moved_result.max_item_revision == 2);

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
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const uint64_t container_uid = item_uid_allocator_next();
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

	root_uid = container_uid - 2;
	child_uid = container_uid - 1;
	auto reparent = payload(player_one, player_one, item_transfer_reason::player_put,
				container_result.to_owner_revision,
				container_result.to_owner_revision, 1);
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

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 2));
	assert(item_uid_allocator_next() == allocator_start + 5);
	assert(item_uid_allocator_next() == allocator_start + 6);
	for (uint8_t id = 1; id <= 12; ++id)
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
