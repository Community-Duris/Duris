// Focus the established SQL item harness on accounted item transitions.
// Its broader legacy fence checks run in the original executable.
#define main item_transfer_full_harness_main
#include "item_transfer_mysql_harness.cpp"
#undef main

#include <barrier>
#include <thread>

static void check_simultaneous_first_claimants(MYSQL *connection)
{
	const auto creator = operation(200);
	const auto lineage = operation(201);
	const auto epoch = operation(202);
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
			operation_hex(creator) +
			"'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
	execute(connection,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
			"'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(creator) + "'))");
	execute(connection,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) + "'),0)");
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity first_owner = { item_owner_type::player, 41, 0 };
	const item_owner_identity second_owner = { item_owner_type::player, 42, 0 };
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 1));
	root_uid = item_uid_allocator_next();
	const uint64_t uid = root_uid;
	const uint64_t source_revision = owner_revision(connection, system);
	const auto first_payload = payload(system, first_owner, item_transfer_reason::creation,
					   source_revision, owner_revision(connection, first_owner),
					   ITEM_TRANSFER_ABSENT_REVISION, 1);
	const auto second_payload =
		payload(system, second_owner, item_transfer_reason::creation, source_revision,
			owner_revision(connection, second_owner), ITEM_TRANSFER_ABSENT_REVISION, 1);
	const std::array<critical_command, 2> commands = {
		accounted_item_transfer(operation(203), first_payload, lineage, epoch, 41,
					economic_source_kind::world_generation),
		accounted_item_transfer(operation(204), second_payload, lineage, epoch, 42,
					economic_source_kind::world_generation),
	};
	std::array<critical_apply_result, 2> outcomes = {};
	std::barrier start(3);
	auto claimant = [&](size_t index)
	{
		MYSQL *candidate = mysql_init(nullptr);
		assert(candidate &&
		       mysql_real_connect(
			       candidate, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
			       getenv("ITEM_TRANSFER_TEST_DB_NAME"),
			       static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)),
			       nullptr, 0));
		start.arrive_and_wait();
		outcomes[index] = critical_command_repository_apply(candidate, commands[index]);
		mysql_close(candidate);
	};
	std::thread first(claimant, 0);
	std::thread second(claimant, 1);
	start.arrive_and_wait();
	first.join();
	second.join();
	const auto applied = [](const critical_apply_result &result)
	{ return result.outcome == critical_apply_outcome::applied; };
	assert(static_cast<int>(applied(outcomes[0])) + static_cast<int>(applied(outcomes[1])) ==
	       1);
	const std::string item = std::to_string(uid);
	const auto count = [&](const std::string &query)
	{ return scalar(connection, query.c_str()); };
	assert(count("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" + item) == 1);
	assert(count("SELECT COUNT(*) FROM player_items WHERE obj_uid=" + item) == 1);
	assert(count("SELECT COUNT(*) FROM item_ownership_ledger WHERE item_uid=" + item) == 1);
	assert(count("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE item_uid=" +
		     item) == 1);
	assert(count("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage=UNHEX('" +
		     operation_hex(lineage) + "')") == 1);
	execute(connection, "DELETE FROM economic_lineage_state WHERE lineage=UNHEX('" +
				    operation_hex(lineage) + "')");
	puts("PASS: simultaneous first claimants retained one owner, native row, event, reference and source claim");
}

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("ITEM_TRANSFER_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	execute(connection, "INSERT IGNORE INTO player_data(pid,name) VALUES"
			    "(41,'RestitutionSource'),(42,'RestitutionTarget')");
	assert(critical_operation_id_generate(&run_operation));
	check_sql_accounted_item_transfer(connection);
	check_simultaneous_first_claimants(connection);
	mysql_close(connection);
}
