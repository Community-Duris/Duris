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

static void check_spell_component_batch_retirement(MYSQL *connection)
{
	const auto creator = operation(210);
	const auto lineage = operation(211);
	const auto epoch = operation(212);
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
	const item_owner_identity player = { item_owner_type::player, 41, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	const uint64_t saved_root_uid = root_uid;
	const uint64_t saved_child_uid = child_uid;
	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 3));
	root_uid = item_uid_allocator_next();
	child_uid = item_uid_allocator_next();
	const uint64_t second_root_uid = item_uid_allocator_next();
	auto creation = multi_root_creation_payload(root_uid, child_uid, second_root_uid,
						    owner_revision(connection, system),
						    owner_revision(connection, player));
	creation.to_owner = player;
	creation.logical_source_id = 70002;
	const auto issue = accounted_item_transfer(operation(213), creation, lineage, epoch, 41,
						   economic_source_kind::spell_creation);
	assert(critical_command_repository_apply(connection, issue).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, issue).outcome ==
	       critical_apply_outcome::already_applied);

	item_uid_allocator_reset_for_tests();
	assert(item_uid_allocator_reserve(connection, 1));
	root_uid = item_uid_allocator_next();
	const uint64_t duplicate_uid = root_uid;
	auto duplicate = payload(system, player, item_transfer_reason::creation,
				 owner_revision(connection, system),
				 owner_revision(connection, player), ITEM_TRANSFER_ABSENT_REVISION,
				 1);
	duplicate.logical_source_id = creation.logical_source_id;
	const auto duplicate_issue = accounted_item_transfer(operation(214), duplicate, lineage,
							     epoch, 41,
							     economic_source_kind::spell_creation);
	const auto refused = critical_command_repository_apply(connection, duplicate_issue);
	// The logical source was already committed. This new command is a
	// terminal semantic refusal, not a retryable SQL duplicate-key fault.
	assert(refused.outcome == critical_apply_outcome::terminal_failure &&
	       refused.error_code == EEXIST);
	const auto duplicate_retry = critical_command_repository_apply(connection, duplicate_issue);
	assert(duplicate_retry.outcome == critical_apply_outcome::terminal_failure &&
	       duplicate_retry.error_code == EEXIST);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage=UNHEX('" +
		       operation_hex(lineage) + "')")
			      .c_str()) == 1);
	for (const char *table :
	     { "economic_accounting_operation", "economic_accounting_item_reference",
	       "critical_operation_inbox", "critical_outbox" })
		assert(scalar(connection, (std::string("SELECT COUNT(*) FROM ") + table +
					   " WHERE operation_id=UNHEX('" +
					   operation_hex(duplicate_issue.operation_id) + "')")
						  .c_str()) == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(duplicate_uid))
					  .c_str()) == 0);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(duplicate_issue.operation_id) + "')")
			      .c_str()) == 0);
	root_uid = creation.items[0].item_uid;

	auto consume = creation;
	consume.from_owner = player;
	consume.to_owner = destruction;
	consume.reason = item_transfer_reason::destruction;
	consume.reason_id = 3401;
	consume.logical_source_id = 0;
	consume.expected_from_revision = owner_revision(connection, player);
	consume.expected_to_revision = owner_revision(connection, destruction);
	for (size_t index = 0; index < consume.item_count; ++index)
	{
		consume.items[index].expected_item_revision = 1;
		consume.items[index].expected_state = item_custody_state::active;
	}
	const uint64_t native_root_row = scalar(
		connection,
		("SELECT id FROM player_items WHERE obj_uid=" + std::to_string(root_uid)).c_str());
	execute(connection, "INSERT INTO player_items(pid,vnum,container_id) VALUES(41,999," +
				    std::to_string(native_root_row) + ")");
	const uint64_t hidden_child_row = mysql_insert_id(connection);
	assert(hidden_child_row);
	const auto hidden_child_retire =
		accounted_item_transfer(operation(216), consume, lineage, epoch, 41,
					economic_source_kind::spell_consumption);
	const auto refused_hidden_child =
		critical_command_repository_apply(connection, hidden_child_retire);
	assert(refused_hidden_child.outcome == critical_apply_outcome::retryable_failure &&
	       refused_hidden_child.error_code == EILSEQ);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(hidden_child_retire.operation_id) + "')")
			      .c_str()) == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
				   "operation_id=UNHEX('" +
				   operation_hex(hidden_child_retire.operation_id) + "')")
					  .c_str()) == 0);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				   std::to_string(root_uid) + "," + std::to_string(child_uid) +
				   "," + std::to_string(second_root_uid) +
				   ") AND owner_type=1 AND state=1 AND item_revision=1")
					  .c_str()) == 3);
	execute(connection,
		"DELETE FROM player_items WHERE id=" + std::to_string(hidden_child_row));
	const auto retire = accounted_item_transfer(operation(215), consume, lineage, epoch, 41,
						    economic_source_kind::spell_consumption);
	assert(critical_command_repository_apply(connection, retire).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, retire).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
				   std::to_string(root_uid) + "," + std::to_string(child_uid) +
				   "," + std::to_string(second_root_uid) +
				   ") AND owner_type=8 AND state=2 AND item_revision=2")
					  .c_str()) == 3);
	assert(scalar(connection, ("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
				   std::to_string(child_uid) +
				   " AND root_item_uid=" + std::to_string(root_uid) +
				   " AND parent_item_uid=" + std::to_string(root_uid))
					  .c_str()) == 1);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=UNHEX('" +
		       operation_hex(retire.operation_id) + "')")
			      .c_str()) == 3);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		       "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id "
		       "AND l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid "
		       "AND l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		       operation_hex(retire.operation_id) +
		       "') AND r.before_revision=1 "
		       "AND r.after_revision=2")
			      .c_str()) == 3);
	assert(scalar(connection,
		      ("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE lineage="
		       "UNHEX('" +
		       operation_hex(lineage) + "')")
			      .c_str()) == 2);
	assert(scalar(connection, ("SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
				   std::to_string(root_uid) + "," + std::to_string(child_uid) +
				   "," + std::to_string(second_root_uid) + ")")
					  .c_str()) == 0);
	execute(connection, "DELETE FROM economic_lineage_state WHERE lineage=UNHEX('" +
				    operation_hex(lineage) + "')");
	root_uid = saved_root_uid;
	child_uid = saved_child_uid;
	puts("PASS: spell component batch retirement preserved every source, event, reference and child tombstone");
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
	check_spell_component_batch_retirement(connection);
	mysql_close(connection);
}
