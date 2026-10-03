// Reuse the maintained native SQL fixture helpers and pooled coordinator owner.
// Its original main is linked but never executed by this focused payload case.
#define main maintained_item_transfer_fixture_main
#include "item_transfer_mysql_harness.cpp"
#undef main
#include "persistence/sql_room_item_payload.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "account/account_load.h"

static void guard_room_fixture()
{
	const auto value = [](const char *name)
	{
		const char *result = std::getenv(name);
		assert(result && *result);
		return result;
	};
	assert(!std::strcmp(value("TEST_DB_DISPOSABLE"), "1") &&
	       !std::strcmp(value("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1") &&
	       !std::strcmp(value("DB_HOST"), "127.0.0.1"));
	assert(!std::getenv("DB_SOCKET") && !std::getenv("TEST_DB_SOCKET"));
	const std::string schema = value("DB_NAME");
	const std::string prefix = "economic_schema_test_";
	assert(schema.starts_with(prefix) && schema.size() > prefix.size() &&
	       schema.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	assert(std::strlen("duris.player.death.restitution.") + schema.size() <= 64);
	assert(value("DB_ALLOWED_TARGETS") == "127.0.0.1/" + schema);
	assert(value("ITEM_TRANSFER_TEST_DB_NAME") == schema);
	char *end = nullptr;
	const auto port = std::strtoul(value("DB_PORT"), &end, 10);
	assert(end && !*end && port > 0 && port <= 65535);
}

static uint64_t room_scalar(MYSQL *connection, const std::string &sql)
{
	return scalar(connection, sql.c_str());
}

static std::vector<uint8_t> encode_exact_graph(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode(items, &encoded) ==
	       player_snapshot_codec_result::ok);
	return encoded;
}

static void check_cold_payload(MYSQL *connection, const std::vector<player_item_snapshot> &expected)
{
	execute(connection, "START TRANSACTION");
	std::vector<uint64_t> roots;
	assert(sql_room_item_payload_roots(connection, &roots));
	assert(std::find(roots.begin(), roots.end(), expected[0].object_uid) != roots.end());
	sql_room_item_graph graph;
	const bool read = sql_room_item_payload_read(connection, expected[0].object_uid, &graph);
	if (!read)
		fprintf(stderr, "exact payload read refused errno=%d sql=%s\n", errno,
			mysql_error(connection));
	assert(read);
	assert(graph.items.size() == expected.size() && graph.identities.size() == expected.size());
	for (size_t index = 0; index < expected.size(); ++index)
	{
		graph.items[index].equipment_slot = 0;
		const auto &identity = graph.identities[index];
		assert(identity.item_uid == expected[index].object_uid &&
		       identity.item_revision == 2);
		assert(identity.root_item_uid == expected[0].object_uid &&
		       identity.owner.type == item_owner_type::room);
		assert(identity.owner.id == 22800 && identity.owner.context_id == 0 &&
		       identity.quantity == 1);
	}
	assert(encode_exact_graph(graph.items) == encode_exact_graph(expected));
	execute(connection, "ROLLBACK");
}

// Only the private fixture observer damages history. Pool workers retain their
// normal FK/session settings, and each probe restores the exact original rows.
static std::string retained_where(const critical_command &command)
{
	return "operation_id=UNHEX('" + operation_hex(command.operation_id) + "')";
}

static void retain_backup(MYSQL *connection, const critical_command &command)
{
	for (const char *table : { "sql_room_item_payload", "item_ownership_ledger",
				   "economic_accounting_item_reference" })
		execute(connection, std::string("CREATE TEMPORARY TABLE retained_") + table +
					    " AS SELECT * FROM " + table + " WHERE " +
					    retained_where(command));
}

static void retain_restore(MYSQL *connection, const critical_command &command)
{
	execute(connection, "SET FOREIGN_KEY_CHECKS=0");
	execute(connection, "START TRANSACTION");
	for (const char *table : { "economic_accounting_item_reference", "sql_room_item_payload",
				   "item_ownership_ledger" })
		execute(connection,
			std::string("DELETE FROM ") + table + " WHERE " + retained_where(command));
	for (const char *table : { "item_ownership_ledger", "sql_room_item_payload",
				   "economic_accounting_item_reference" })
		execute(connection,
			std::string("INSERT INTO ") + table + " SELECT * FROM retained_" + table);
	execute(connection, "COMMIT");
	execute(connection, "SET FOREIGN_KEY_CHECKS=1");
}

static bool retained_result_equal(const critical_apply_result &actual,
				  const critical_apply_result &expected)
{
	return actual.outcome == critical_apply_outcome::already_applied && !actual.error_code &&
	       actual.durable_revision == expected.durable_revision &&
	       actual.result_size == expected.result_size &&
	       actual.result_payload == expected.result_payload;
}

static unsigned retained_failures = 0;
static void retained_check(bool ok, const char *label)
{
	std::printf("%s: retained receipt %s\n", ok ? "PASS" : "FAIL", label);
	if (!ok)
		++retained_failures;
}

static void retained_replay_checks(MYSQL *connection, const critical_command &command,
				   const critical_apply_result &original, bool damaged,
				   const char *label)
{
	const auto direct = critical_command_repository_apply(connection, command);
	MYSQL *cold = open_pool_test_connection();
	assert(cold && mysql_thread_id(cold) != mysql_thread_id(connection));
	const auto reconciled = critical_command_repository_reconcile(cold, command);
	mysql_close(cold);
	const auto pooled = critical_command_repository_apply_from_pool(command, nullptr);
	const auto valid = [&](const critical_apply_result &value)
	{
		return damaged ? value.outcome == critical_apply_outcome::retryable_failure &&
					 value.error_code && !value.result_size :
				 retained_result_equal(value, original);
	};
	const bool ok = valid(direct) && valid(reconciled) && valid(pooled);
	if (!ok)
		std::fprintf(stderr, "retained case=%s direct=%u/%u reconcile=%u/%u pooled=%u/%u\n",
			     label, static_cast<unsigned>(direct.outcome), direct.error_code,
			     static_cast<unsigned>(reconciled.outcome), reconciled.error_code,
			     static_cast<unsigned>(pooled.outcome), pooled.error_code);
	retained_check(ok, label);
}

static critical_completion retained_completion()
{
	critical_completion completion{};
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	while (!critical_command_coordinator_pulse(&completion, 1))
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return completion;
}

static void retained_retry_exhaustion(MYSQL *connection, const critical_command &command,
				      const critical_apply_result &original)
{
	execute(connection, "DELETE FROM sql_room_item_payload WHERE " + retained_where(command) +
				    " AND item_uid=" + std::to_string(root_uid));
	std::string directory = "/tmp/duris-room-retained-retry-XXXXXX";
	assert(mkdtemp(directory.data()));
	const auto apply = [](const critical_command &value, void *)
	{ return critical_command_repository_apply_from_pool(value, nullptr); };
	const auto start = [&]
	{
		assert(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 1,
							 nullptr, nullptr,
							 economic_command_admission_supported));
	};
	start();
	assert(critical_command_coordinator_submit_for_publication(command) ==
	       critical_submit_result::awaiting_durability);
	const auto failed = retained_completion();
	assert(failed.outcome == critical_apply_outcome::retryable_failure && failed.error_code &&
	       !failed.result_size);
	const auto health = critical_command_coordinator_health_copy();
	assert(health.blocked == 1 && health.retries == CRITICAL_COORDINATOR_MAX_RETRIES &&
	       health.publication_pending == 0 && health.completed == 0);
	for (const auto &key : command.keys)
	{
		critical_operation_id owner{};
		assert(critical_command_coordinator_is_fenced(key, &owner) &&
		       critical_operation_id_equal(owner, command.operation_id));
	}
	assert(!critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_coordinator_shutdown());
	retain_restore(connection, command);
	start();
	const auto replayed = retained_completion();
	assert(replayed.outcome == critical_apply_outcome::already_applied &&
	       !replayed.error_code && replayed.durable_revision == original.durable_revision &&
	       replayed.result_size == original.result_size &&
	       replayed.result_payload == original.result_payload);
	assert(critical_command_coordinator_health_copy().publication_pending == 1 &&
	       critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert(critical_command_coordinator_shutdown());
	std::filesystem::remove_all(directory);
	std::puts(
		"PASS: retained retry exhaustion preserves original fences and journal; repaired restart ACK");
}

static bool check_retained_receipt(MYSQL *connection, const critical_command &command,
				   const critical_apply_result &original,
				   const item_transfer_payload &drop,
				   const critical_operation_id &lineage,
				   const critical_operation_id &epoch, bool export_seed)
{
	retain_backup(connection, command);
	retained_replay_checks(connection, command, original, false, "intact original proof");
	const auto where = retained_where(command);
	const auto first = " AND item_uid=" + std::to_string(root_uid);
	const std::vector<std::pair<std::string, std::string>> mutations = {
		{ "missing payload", "DELETE FROM sql_room_item_payload WHERE " + where + first },
		{ "corrupt payload",
		  "UPDATE sql_room_item_payload SET payload=X'00000000' WHERE " + where + first },
		{ "wrong payload revision",
		  "UPDATE sql_room_item_payload SET item_revision=item_revision+20 WHERE " + where +
			  first },
		{ "inconsistent historical season",
		  "UPDATE sql_room_item_payload SET season_epoch=season_epoch+1 WHERE " + where +
			  first },
		{ "extra payload",
		  "INSERT INTO sql_room_item_payload SELECT item_uid+100000,item_revision,payload_version,operation_id,season_epoch,payload FROM retained_sql_room_item_payload WHERE item_uid=" +
			  std::to_string(root_uid) },
		{ "missing native ledger",
		  "DELETE FROM item_ownership_ledger WHERE " + where + first },
		{ "corrupt native topology",
		  "UPDATE item_ownership_ledger SET parent_item_uid=" + std::to_string(child_uid) +
			  " WHERE " + where + first },
		{ "corrupt native source owner",
		  "UPDATE item_ownership_ledger SET from_owner_id=from_owner_id+1 WHERE " + where +
			  first },
		{ "corrupt native destination owner",
		  "UPDATE item_ownership_ledger SET to_owner_id=to_owner_id+1 WHERE " + where +
			  first },
		{ "corrupt native item revision",
		  "UPDATE item_ownership_ledger SET item_revision=item_revision+20 WHERE " + where +
			  first },
		{ "corrupt native owner revision",
		  "UPDATE item_ownership_ledger SET from_owner_revision=from_owner_revision+1 WHERE " +
			  where + first },
		{ "corrupt native destination revision",
		  "UPDATE item_ownership_ledger SET to_owner_revision=to_owner_revision+1 WHERE " +
			  where + first },
		{ "corrupt native reason",
		  "UPDATE item_ownership_ledger SET reason_id=reason_id+1 WHERE " + where + first },
		{ "corrupt native source site",
		  "UPDATE item_ownership_ledger SET source_site=source_site+1 WHERE " + where +
			  first },
		{ "corrupt native equipment slot",
		  "UPDATE item_ownership_ledger SET from_equipment_slot=1 WHERE " + where + first },
		{ "extra native ledger",
		  "INSERT INTO item_ownership_ledger SELECT operation_id,99,item_uid+100000,root_item_uid,parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site,created_at,from_equipment_slot,to_equipment_slot FROM retained_item_ownership_ledger WHERE item_uid=" +
			  std::to_string(root_uid) },
		{ "missing accounting reference",
		  "DELETE FROM economic_accounting_item_reference WHERE " + where + first },
		{ "corrupt accounting reference",
		  "UPDATE economic_accounting_item_reference SET before_revision=0 WHERE " + where +
			  first },
		{ "extra accounting reference",
		  "INSERT INTO economic_accounting_item_reference SELECT operation_id,99,99,child_index,item_uid,before_revision,after_revision,legacy_operation_id,99 FROM retained_economic_accounting_item_reference WHERE item_uid=" +
			  std::to_string(root_uid) }
	};
	for (const auto &[label, mutation] : mutations)
	{
		// Simulate externally damaged native history while preserving accounting
		// references, so a successful old verifier cannot hide behind the FK.
		execute(connection, "SET FOREIGN_KEY_CHECKS=0");
		execute(connection, mutation);
		execute(connection, "SET FOREIGN_KEY_CHECKS=1");
		retained_replay_checks(connection, command, original, true, label.c_str());
		retain_restore(connection, command);
		retained_replay_checks(connection, command, original, false, "exact repair");
	}
	if (retained_failures)
		return false; // Preserve all before-fix failures without claiming later checks ran.
	retained_retry_exhaustion(connection, command, original);

	// A real persisted domain refusal owns no successful room payload. Its
	// original rejection must survive duplicate and reconcile unchanged.
	auto rejected_command = accounted_item_transfer(operation(41), drop, lineage, epoch, 60491);
	rejected_command.publication_required = true;
	const auto rejected = critical_command_repository_apply(connection, rejected_command);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure && rejected.error_code);
	assert(room_scalar(connection, "SELECT COUNT(*) FROM economic_accounting_operation WHERE " +
					       retained_where(rejected_command)) == 1);
	assert(room_scalar(connection, "SELECT COUNT(*) FROM sql_room_item_payload WHERE " +
					       retained_where(rejected_command)) == 0);
	for (const auto &again :
	     { critical_command_repository_apply(connection, rejected_command),
	       critical_command_repository_reconcile(connection, rejected_command),
	       critical_command_repository_apply_from_pool(rejected_command, nullptr) })
		assert(again.outcome == rejected.outcome &&
		       again.error_code == rejected.error_code &&
		       again.failure_stage == rejected.failure_stage &&
		       again.result_size == rejected.result_size &&
		       again.result_payload == rejected.result_payload);
	std::puts(
		"PASS: legitimate retained rejected drop has no room payload and preserves rejection");

#ifdef DURIS_SQL_ROOM_ITEM_RETAINED_VERIFIER_TEST
	// Public helper contract: establish a stale RR view, then repair on another
	// native connection. Both metadata and blob must use current locking reads.
	const auto original_blob = read_blob(
		connection,
		("SELECT payload FROM sql_room_item_payload WHERE " + where + first).c_str());
	execute(connection,
		"UPDATE sql_room_item_payload SET payload=X'00000000' WHERE " + where + first);
	execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	execute(connection, "START TRANSACTION");
	assert(read_blob(
		       connection,
		       ("SELECT payload FROM sql_room_item_payload WHERE " + where + first).c_str())
		       .size() == 4);
	MYSQL *repair = open_pool_test_connection();
	assert(repair);
	execute(repair, "UPDATE sql_room_item_payload SET payload=UNHEX('" +
				bytes_hex(original_blob) + "') WHERE " + where + first);
	mysql_close(repair);
	item_transfer_result decoded{};
	assert(item_transfer_command_decode_result(original.result_payload.data(),
						   original.result_size, &decoded));
	assert(sql_room_item_payload_verify_retained(connection, command, decoded));
	execute(connection, "ROLLBACK");
	std::puts(
		"PASS: public retained verifier ignores stale REPEATABLE READ view after independent repair");
#endif

	if (!export_seed)
	{
		// Move the real original UIDs with a later native typed transaction.
		// Historical receipt verification must not require current room custody.
		auto get = drop;
		get.from_owner = drop.to_owner;
		get.to_owner = drop.from_owner;
		get.reason = item_transfer_reason::player_get;
		get.expected_from_revision = owner_revision(connection, get.from_owner);
		get.expected_to_revision = owner_revision(connection, get.to_owner);
		for (size_t index = 0; index < get.item_count; ++index)
			get.items[index].expected_item_revision = 2;
		auto moved = accounted_item_transfer(operation(42), get, lineage, epoch, 60491);
		const auto movement = critical_command_repository_apply(connection, moved);
		assert(movement.outcome == critical_apply_outcome::applied && !movement.error_code);
		assert(room_scalar(
			       connection,
			       "SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id=60491 AND item_revision=3") ==
		       3);
		execute(connection,
			"UPDATE season_reset_state SET season_epoch=season_epoch+1 WHERE state_id=1");
		retained_replay_checks(connection, command, original, false,
				       "later real UID movement and season advance");
	}
	else
		std::puts(
			"SKIP: later custody/season mutation preserves existing world seed export fixture");
	return retained_failures == 0;
}

int main()
{
	guard_room_fixture();
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	std::vector<player_item_snapshot> seed_expected;
	MYSQL *connection = open_pool_test_connection();
	assert(connection);
#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
	{
		economic_sql_real_pool_lifecycle pool_lifecycle;
#endif
		assert(critical_operation_id_generate(&run_operation));
		ensure_collector_boundary_fixture(connection);
		execute(connection,
			"INSERT INTO player_data(pid,name,account_name) VALUES(60491,'ExactPayloadFixture','ExactPayloadFixture')");
		// Install the real zero-revision login baselines before retaining history.
		// The complete native boot keeps its baseline readiness guards unchanged.
		assert(account_load_repair(connection, "ExactPayloadFixture") >= 0);
		for (const char *table : { "currency_wallet_baseline", "epic_balance_baseline",
					   "combat_frag_baseline" })
			assert(room_scalar(connection, std::string("SELECT COUNT(*) FROM ") +
							       table + " WHERE pid=60491") == 1);
		const auto creator = operation(1), lineage = operation(2), epoch = operation(3);
		execute(connection,
			"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
				operation_hex(creator) +
				"'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
		execute(connection,
			"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(UNHEX('" +
				operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
				"'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(creator) +
				"'))");
		execute(connection,
			"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
				operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
				"'),0)");
		item_uid_allocator_reset_for_tests();
		assert(item_uid_allocator_reserve(connection, 3));
		root_uid = item_uid_allocator_next();
		child_uid = item_uid_allocator_next();
		const uint64_t grandchild = item_uid_allocator_next();
		const item_owner_identity system = { item_owner_type::system, 0, 0 };
		const item_owner_identity player = { item_owner_type::player, 60491, 0 };
		const item_owner_identity room = { item_owner_type::room, 22800, 0 };
		std::vector<player_item_snapshot> exact;
		for (size_t index = 0; index < 3; ++index)
		{
			auto next = runtime_item(index == 0 ? root_uid :
						 index == 1 ? child_uid :
							      grandchild,
						 INT64_C(9007199254740993) + index, 60 + index);
			// This route is ordinary inventory only; artifact custody is excluded.
			next.extra_flags &= ~ITEM_ARTIFACT;
			// Match actual set_obj_affected_extra state: the retained baseline
			// precedes the effect, and live flags include that effect's bits.
			// An incoherent synthetic flag/list pair correctly refuses restore.
			next.dynamic_affects.insert(next.dynamic_affects.begin(),
						    { TAG_ALTERED_EXTRA2, 0, next.extra2_flags });
			next.extra2_flags |= next.dynamic_affects[1].extra2;
			next.vnum = index < 2 ? 48 : 5;
			next.type = index < 2 ? 15 : 1;
			next.equipment_slot = 0;
			next.parent_index = index ? index - 1 : -1;
			exact.push_back(std::move(next));
		}
		auto creation = payload(system, player, item_transfer_reason::creation,
					owner_revision(connection, system),
					owner_revision(connection, player),
					ITEM_TRANSFER_ABSENT_REVISION);
		creation.item_count = 3;
		creation.items[2] = { grandchild, root_uid,
				      child_uid,  ITEM_TRANSFER_ABSENT_REVISION,
				      5,	  item_custody_state::absent };
		for (size_t index = 0; index < creation.item_count; ++index)
			creation.items[index].vnum = exact[index].vnum;
		attach_blob(&creation, exact);
		const auto created = critical_command_repository_apply(
			connection,
			accounted_item_transfer(operation(4), creation, lineage, epoch, 60491,
						economic_source_kind::starter_grant));
		assert(created.outcome == critical_apply_outcome::applied && !created.error_code);
		// Use the ordinary native save writer, then read its actual physical state.
		// The drop source is these existing native rows and original UIDs, never a
		// sidecar/legacy projection or a prototype used to fill absent fields.
		player_snapshot checkpoint{};
		checkpoint.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		checkpoint.pid = 60491;
		checkpoint.revision = 1;
		checkpoint.components = PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_EQUIPMENT;
		checkpoint.items = exact;
		assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
		       player_save_apply_outcome::applied);
		player_load_request request{};
		request.request_id = 60491;
		request.pid = 60491;
		request.account_name = "ExactPayloadFixture";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		auto source = player_load_repository_execute(connection, request);
		assert(source.outcome == player_load_outcome::applied &&
		       source.snapshot.items.size() == exact.size());
		for (size_t index = 0; index < exact.size(); ++index)
		{
			// SQL transports spellbook metadata as JSON text. Verify those physical
			// bytes before converting to the typed representation used by runtime
			// capture; no other captured field may be filled from a prototype.
			auto &item = source.snapshot.items[index];
			assert(item.extra_descriptions.size() == 1);
			auto &book = item.extra_descriptions[0];
			assert(book.keyword == "SPELLBOOK" && book.spellbook &&
			       book.description == "[4,9,15]" && book.spell_ids.empty());
			book.description.clear();
			book.spell_ids = { 4, 9, 15 };
		}
		// Encode the complete graph: a singleton cannot retain a child's parent index.
		assert(encode_exact_graph(source.snapshot.items) == encode_exact_graph(exact));

		auto drop = creation;
		drop.from_owner = player;
		drop.to_owner = room;
		drop.reason = item_transfer_reason::player_drop;
		drop.reason_id = room.id;
		drop.selected_item_uid = root_uid;
		drop.target_root_item_uid = root_uid;
		drop.expected_from_revision = owner_revision(connection, player);
		drop.expected_to_revision = owner_revision(connection, room);
		for (size_t index = 0; index < drop.item_count; ++index)
		{
			drop.items[index].expected_item_revision = 1;
			drop.items[index].expected_state = item_custody_state::active;
		}
		attach_blob(&drop, source.snapshot.items);
		execute(connection, "START TRANSACTION");
		sql_room_item_payload_batch prepared;
		const bool prepared_ok = sql_room_item_payload_prepare(connection, drop, &prepared);
		if (!prepared_ok)
			fprintf(stderr, "room prepare refused errno=%d sql=%s\n", errno,
				mysql_error(connection));
		assert(prepared_ok);
		execute(connection, "ROLLBACK");
		auto altered = drop;
		altered.reason_id = 0;
		execute(connection, "START TRANSACTION");
		assert(!sql_room_item_payload_prepare(connection, altered, &prepared));
		execute(connection, "ROLLBACK");
		// A missing physical properties projection cannot prove that the native
		// literal checkpoint wrote its extra2/dynamic-effect payload.
		execute(connection, "START TRANSACTION");
		execute(connection, "UPDATE player_items SET item_properties=NULL WHERE obj_uid=" +
					    std::to_string(root_uid));
		assert(!sql_room_item_payload_prepare(connection, drop, &prepared) &&
		       errno == ESTALE);
		execute(connection, "ROLLBACK");
		// Fail after earlier payload rows, then after physical deletion begins. Each
		// failure must roll back custody, immutable rows, source projection and root
		// evidence together before a successful lost-reply run.
		const std::vector<std::pair<std::string, std::string>> faults = {
			{ "BEFORE INSERT ON sql_room_item_payload",
			  "NEW.item_uid=" + std::to_string(grandchild) },
			{ "AFTER DELETE ON player_items",
			  "OLD.obj_uid=" + std::to_string(root_uid) }
		};
		for (size_t index = 0; index < faults.size(); ++index)
		{
			const auto failed = accounted_item_transfer(operation(6 + index), drop,
								    lineage, epoch, 60491);
			execute(connection,
				"CREATE TRIGGER sql_room_item_fault " + faults[index].first +
					" FOR EACH ROW BEGIN IF " + faults[index].second +
					" THEN SIGNAL SQLSTATE '45000' SET MYSQL_ERRNO=1644,MESSAGE_TEXT='room payload rollback fixture'; END IF; END");
			const auto refused = critical_command_repository_apply(connection, failed);
			assert(refused.outcome != critical_apply_outcome::applied &&
			       refused.outcome != critical_apply_outcome::already_applied);
			execute(connection, "DROP TRIGGER sql_room_item_fault");
			assert(owner_revision(connection, player) == drop.expected_from_revision &&
			       owner_revision(connection, room) == drop.expected_to_revision);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id=60491 AND item_revision=1 AND state=1") ==
			       3);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE pid=60491") == 3);
			assert(scalar(connection, "SELECT COUNT(*) FROM sql_room_item_payload") ==
			       0);
			for (const char *table :
			     { "item_ownership_ledger", "critical_operation_inbox",
			       "critical_outbox", "economic_accounting_operation" })
				assert(room_scalar(connection,
						   std::string("SELECT COUNT(*) FROM ") + table +
							   " WHERE operation_id=UNHEX('" +
							   operation_hex(failed.operation_id) +
							   "')") == 0);
		}
		auto command = accounted_item_transfer(operation(5), drop, lineage, epoch, 60491);
		command.publication_required = true;
		economic_sql_commit_reply_loss_fixture::arm();
		const auto applied =
			exercise_sql_coordinator(command, "exact-room-payload", false,
						 critical_apply_outcome::already_applied);
		economic_sql_commit_reply_loss_fixture::verify();
		assert(applied.outcome == critical_apply_outcome::already_applied);
		assert(scalar(connection, "SELECT COUNT(*) FROM sql_room_item_payload") == 3);
		assert(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE pid=60491") ==
		       0);
		for (const char *table : { "player_item_runtime_state", "player_item_affects",
					   "player_item_extra_descr" })
			assert(room_scalar(connection,
					   std::string("SELECT COUNT(*) FROM ") + table) == 0);
		// The coordinator fixture ACKs and restarts its journal before returning.
		check_cold_payload(connection, exact);
		mysql_close(connection);
		connection = open_pool_test_connection();
		assert(connection);
		check_cold_payload(connection, exact);
		const auto refusal = [&]
		{
			execute(connection, "START TRANSACTION");
			sql_room_item_graph sentinel;
			sentinel.owner_revision = 999;
			assert(!sql_room_item_payload_read(connection, root_uid, &sentinel));
			assert(sentinel.owner_revision == 999 && sentinel.items.empty());
			execute(connection, "ROLLBACK");
		};
		execute(connection, "START TRANSACTION");
		execute(connection,
			"UPDATE item_current_owner SET item_revision=item_revision+1 WHERE item_uid=" +
				std::to_string(root_uid));
		std::vector<uint64_t> stale_roots;
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) !=
			       stale_roots.end());
		sql_room_item_graph graph;
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		const auto retained = read_blob(
			connection, ("SELECT payload FROM sql_room_item_payload WHERE item_uid=" +
				     std::to_string(grandchild))
					    .c_str());
		execute(connection,
			"UPDATE sql_room_item_payload SET payload=X'00000000' WHERE item_uid=" +
				std::to_string(grandchild));
		refusal();
		execute(connection, "UPDATE sql_room_item_payload SET payload=UNHEX('" +
					    bytes_hex(retained) +
					    "') WHERE item_uid=" + std::to_string(grandchild));
		execute(connection,
			"UPDATE critical_operation_inbox SET failure_stage=1 WHERE operation_id=UNHEX('" +
				operation_hex(command.operation_id) + "')");
		refusal();
		execute(connection,
			"UPDATE critical_operation_inbox SET failure_stage=0 WHERE operation_id=UNHEX('" +
				operation_hex(command.operation_id) + "')");
		execute(connection, "START TRANSACTION");
		execute(connection, "DELETE FROM sql_room_item_payload");
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) !=
			       stale_roots.end());
		bool enrolled = false;
		assert(sql_room_item_payload_present(connection, grandchild, &enrolled) &&
		       enrolled);
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		execute(connection, "START TRANSACTION");
		execute(connection,
			"UPDATE season_reset_state SET season_epoch=season_epoch+1 WHERE state_id=1");
		assert(sql_room_item_payload_roots(connection, &stale_roots) &&
		       std::find(stale_roots.begin(), stale_roots.end(), root_uid) ==
			       stale_roots.end());
		assert(!sql_room_item_payload_read(connection, root_uid, &graph));
		execute(connection, "ROLLBACK");
		check_cold_payload(connection, exact);
		const char *export_value = std::getenv("DURIS_SQL_ROOM_ITEM_SEED_EXPORT");
		if (!check_retained_receipt(connection, command, applied, drop, lineage, epoch,
					    export_value && !std::strcmp(export_value, "1")))
		{
			std::fprintf(stderr, "FAIL: %u retained immutable-proof assertions\n",
				     retained_failures);
			mysql_close(connection);
			return 1;
		}
		seed_expected = exact;
		mysql_close(connection);
#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
	}
#endif
	// Export only after durable ACK and the production pool's clean shutdown.
	// This isolated fixture retains historical lineage and payload evidence;
	// complete active-epoch lifecycle installation is a separate qualification.
	if (const char *export_seed = std::getenv("DURIS_SQL_ROOM_ITEM_SEED_EXPORT");
	    export_seed && std::strcmp(export_seed, "1") == 0)
	{
		connection = open_pool_test_connection();
		assert(connection);
		execute(connection, "UPDATE economic_lineage_state SET active_epoch=NULL");
		check_cold_payload(connection, seed_expected);
		mysql_close(connection);
		const auto encoded = encode_exact_graph(seed_expected);
		std::printf("ROOM_ITEM_PAYLOAD_SEED uid=%llu payload=%s\n",
			    static_cast<unsigned long long>(seed_expected[0].object_uid),
			    bytes_hex(encoded).c_str());
	}
	std::puts(
		"PASS: exact native source, atomic schema2 drop and two rollback faults, ACK, two cold SQL connections, corrupt/stale/missing provenance and season refusal; no gameplay publication claim");
}
