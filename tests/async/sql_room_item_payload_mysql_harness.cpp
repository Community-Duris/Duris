// Reuse the maintained native SQL fixture helpers and pooled coordinator owner.
// Its original main is linked but never executed by this focused payload case.
#define main maintained_item_transfer_fixture_main
#define __wrap_mysql_real_query maintained_room_mysql_real_query
#define sql_open_configured_connection maintained_room_configured_connection
#include "item_transfer_mysql_harness.cpp"
#undef sql_open_configured_connection
#undef __wrap_mysql_real_query
#undef main
#include "persistence/sql_room_item_payload.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "account/account_load.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string_view>

static bool partial_matrix_enabled()
{
	const char *value = std::getenv("DURIS_SQL_ROOM_PARTIAL_SAVE_MATRIX");
	return value && !std::strcmp(value, "1");
}

#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
static void partial_session(MYSQL *connection)
{
	assert(connection && !(connection->server_status & SERVER_STATUS_IN_TRANS));
	execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
	execute(connection, "SET SESSION innodb_lock_wait_timeout=1");
	const char *isolation = strstr(mysql_get_server_info(connection), "MariaDB") ?
					"SELECT @@SESSION.tx_isolation" :
					"SELECT @@SESSION.transaction_isolation";
	execute(connection, isolation);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0] && !std::strcmp(row[0], "READ-COMMITTED"));
	mysql_free_result(result);
	assert(scalar(connection, "SELECT @@SESSION.innodb_lock_wait_timeout") == 1);
	assert(scalar(connection, "SELECT @@SESSION.foreign_key_checks") == 1);
}

MYSQL *sql_open_configured_connection(unsigned long flags)
{
	MYSQL *connection = maintained_room_configured_connection(flags);
	if (connection && partial_matrix_enabled())
		partial_session(connection);
	return connection;
}
#endif

enum class partial_query_role
{
	none,
	save,
	drop
};
static thread_local partial_query_role partial_role = partial_query_role::none;

struct partial_query_barrier
{
	partial_query_role role;
	std::string prefix;
	std::mutex mutex;
	std::condition_variable changed;
	bool reached = false, released = false;
	unsigned long session = 0;
	partial_query_barrier(partial_query_role owner, std::string query_prefix)
		: role(owner)
		, prefix(std::move(query_prefix))
	{
	}

	void before(MYSQL *connection, partial_query_role caller, std::string_view query)
	{
		if (caller != role || !query.starts_with(prefix))
			return;
		std::unique_lock lock(mutex);
		if (reached)
			return;
		assert(connection && (connection->server_status & SERVER_STATUS_IN_TRANS));
		session = mysql_thread_id(connection);
		reached = true;
		changed.notify_all();
		assert(changed.wait_for(lock, std::chrono::seconds(10), [&] { return released; }));
	}
	void wait()
	{
		std::unique_lock lock(mutex);
		assert(changed.wait_for(lock, std::chrono::seconds(10), [&] { return reached; }));
	}
	void release()
	{
		std::lock_guard lock(mutex);
		released = true;
		changed.notify_all();
	}
};

static std::atomic<partial_query_barrier *> partial_barrier{ nullptr };

extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long size)
{
	if (partial_query_barrier *barrier = partial_barrier.load())
		barrier->before(connection, partial_role, std::string_view(query, size));
	// SQL always executes outside the test barrier mutex; keep the existing
	// actual COMMIT-reply-loss observer and every default fixture path intact.
	return maintained_room_mysql_real_query(connection, query, size);
}

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

// Opt-in native repository/real-pool compatibility proof. These independent
// synthetic forests do not change the default historical receipt matrix.
#ifdef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
using partial_rows = std::vector<std::vector<std::string>>;
static partial_rows partial_read_rows(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	partial_rows rows;
	while (MYSQL_ROW row = mysql_fetch_row(result))
	{
		const auto *lengths = mysql_fetch_lengths(result);
		assert(lengths);
		std::vector<std::string> fields;
		for (unsigned column = 0; column < mysql_num_fields(result); ++column)
			fields.push_back(row[column] ?
						 std::string("value:") +
							 std::string(row[column], lengths[column]) :
						 "null:");
		rows.push_back(std::move(fields));
	}
	mysql_free_result(result);
	return rows;
}

static MYSQL *partial_open_connection()
{
	MYSQL *connection = open_pool_test_connection();
	assert(connection);
	partial_session(connection);
	return connection;
}

struct partial_forest
{
	int pid = 0, foreign_pid = 0, healthy_pid = 0;
	uint64_t root = 0, foreign_uid = 0;
	std::vector<uint64_t> uids;
	std::vector<player_item_snapshot> exact;
	player_snapshot save{};
	critical_command drop{};
};

using partial_durable_rows = std::vector<std::pair<std::string, partial_rows>>;
static partial_durable_rows partial_durable(MYSQL *connection, const partial_forest &forest)
{
	std::string uids;
	for (auto uid : forest.uids)
		uids += (uids.empty() ? "" : ",") + std::to_string(uid);
	const auto selected_uids = " IN (" + uids + ")";
	const auto pids = " IN (" + std::to_string(forest.pid) + "," +
			  std::to_string(forest.foreign_pid) + ")";
	const auto physical =
		"SELECT id FROM player_items WHERE pid" + pids + " OR obj_uid" + selected_uids;
	const auto operation = retained_where(forest.drop); // Frozen ID, never victim-row derived.
	partial_durable_rows state;
	const auto read = [&](const char *table, const std::string &filter, const char *order)
	{
		state.emplace_back(table, partial_read_rows(connection,
							    std::string("SELECT * FROM ") + table +
								    " WHERE " + filter +
								    " ORDER BY " + order));
	};
	read("player_data", "pid" + pids, "pid");
	read("item_current_owner", "item_uid" + selected_uids, "item_uid");
	read("item_owner_revision",
	     "(owner_type=1 AND owner_id" + pids + ") OR (owner_type=3 AND owner_id=22800)",
	     "owner_type,owner_id,owner_context_id");
	read("player_items", "pid" + pids + " OR obj_uid" + selected_uids, "id");
	for (const char *table : { "player_item_affects", "player_item_extra_descr" })
		read(table, "item_id IN (" + physical + ")", "id");
	read("player_item_runtime_state", "item_id IN (" + physical + ")", "item_id");
	read("sql_room_item_payload", "item_uid" + selected_uids + " OR " + operation,
	     "item_uid,item_revision");
	read("item_ownership_ledger", "item_uid" + selected_uids + " OR " + operation,
	     "operation_id,event_index");
	read("economic_accounting_item_reference", "item_uid" + selected_uids + " OR " + operation,
	     "operation_id,event_index");
	for (const char *table : { "economic_accounting_operation",
				   "economic_accounting_source_claim", "critical_operation_inbox" })
		read(table, operation, "operation_id");
	read("economic_accounting_account_effect", operation, "operation_id,account_index");
	read("economic_accounting_coin_posting", operation, "operation_id,line_index");
	read("economic_accounting_child", operation, "operation_id,child_index");
	read("critical_outbox", operation, "outbox_id");
	return state;
}

static partial_durable_rows partial_protected(MYSQL *connection, const partial_forest &forest)
{
	const auto uids = " IN (" + std::to_string(forest.uids[3]) + "," +
			  std::to_string(forest.foreign_uid) + ")";
	const auto physical = "SELECT id FROM player_items WHERE obj_uid" + uids;
	partial_durable_rows rows;
	for (const char *table : { "player_item_affects", "player_item_extra_descr" })
		rows.emplace_back(table, partial_read_rows(connection,
							   std::string("SELECT * FROM ") + table +
								   " WHERE item_id IN (" +
								   physical + ") ORDER BY id"));
	rows.emplace_back(
		"runtime",
		partial_read_rows(connection,
				  "SELECT * FROM player_item_runtime_state WHERE item_id IN (" +
					  physical + ") ORDER BY item_id"));
	rows.emplace_back("items",
			  partial_read_rows(connection, "SELECT * FROM player_items WHERE obj_uid" +
								uids + " ORDER BY id"));
	rows.emplace_back("custody",
			  partial_read_rows(connection,
					    "SELECT * FROM item_current_owner WHERE item_uid" +
						    uids + " ORDER BY item_uid"));
	return rows;
}

static unsigned partial_failures = 0;
static void partial_expect(bool condition, const char *label)
{
	if (!condition)
	{
		++partial_failures;
		std::fprintf(stderr, "ASSERTION FAILED: room partial-save %s\n", label);
	}
}

static bool partial_custody_refusal(const player_save_apply_result &result)
{
	return result.outcome == player_save_apply_outcome::terminal_failure &&
	       result.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
	       result.custody_diagnosis != player_save_custody_diagnosis::none;
}

static void partial_healthy_progress(MYSQL *connection, const partial_forest &forest)
{
	player_snapshot healthy{};
	healthy.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	healthy.pid = forest.healthy_pid;
	healthy.revision = scalar(connection, ("SELECT save_revision FROM player_data WHERE pid=" +
					       std::to_string(healthy.pid))
						      .c_str()) +
			   1;
	healthy.components = PLAYER_COMPONENT_STATUS;
	healthy.status_integers.push_back({ player_status_field::wimpy, 41, 0, false });
	const auto applied = player_snapshot_repository_apply(connection, healthy);
	partial_expect(applied.outcome == player_save_apply_outcome::applied &&
			       applied.durable_revision == healthy.revision &&
			       scalar(connection, ("SELECT wimpy FROM player_data WHERE pid=" +
						   std::to_string(healthy.pid))
							  .c_str()) == 41,
		       "independent PID committed through an actual save transaction");
}

static partial_forest partial_seed(MYSQL *connection, unsigned index,
				   const critical_operation_id &lineage,
				   const critical_operation_id &epoch)
{
	partial_forest forest;
	forest.pid = 60510 + static_cast<int>(index) * 3;
	forest.foreign_pid = forest.pid + 1;
	forest.healthy_pid = forest.pid + 2;
	for (int pid : { forest.pid, forest.foreign_pid, forest.healthy_pid })
	{
		const auto name = "RoomPartialFixture" + std::to_string(pid);
		execute(connection, "INSERT INTO player_data(pid,name,account_name) VALUES(" +
					    std::to_string(pid) + ",'" + name + "','" + name +
					    "')");
		assert(account_load_repair(connection, name.c_str()) >= 0);
	}
	assert(item_uid_allocator_reserve(connection, 5));
	for (unsigned item = 0; item < 5; ++item)
		forest.uids.push_back(item_uid_allocator_next());
	forest.root = forest.uids[0];
	forest.foreign_uid = forest.uids[4];
	root_uid = forest.root;
	child_uid = forest.uids[1];
	for (size_t item = 0; item < 4; ++item)
	{
		auto snapshot = runtime_item(forest.uids[item], INT64_C(9007199254740993) + item,
					     60 + static_cast<int64_t>(item));
		snapshot.extra_flags &= ~ITEM_ARTIFACT;
		snapshot.dynamic_affects.insert(snapshot.dynamic_affects.begin(),
						{ TAG_ALTERED_EXTRA2, 0, snapshot.extra2_flags });
		snapshot.extra2_flags |= snapshot.dynamic_affects[1].extra2;
		snapshot.vnum = item < 2 ? 48 : 5;
		snapshot.type = item < 2 ? 15 : 1;
		snapshot.equipment_slot = item == 3 ? 4 : 0;
		snapshot.parent_index = item == 0 || item == 3 ? -1 :
								 static_cast<int32_t>(item - 1);
		forest.exact.push_back(std::move(snapshot));
	}
	const item_owner_identity system{ item_owner_type::system, 0, 0 };
	const item_owner_identity player{ item_owner_type::player,
					  static_cast<uint64_t>(forest.pid), 0 };
	const item_owner_identity room{ item_owner_type::room, 22800, 0 };
	auto creation = payload(system, player, item_transfer_reason::creation,
				owner_revision(connection, system),
				owner_revision(connection, player), ITEM_TRANSFER_ABSENT_REVISION);
	creation.item_count = 3;
	creation.items[2] = { forest.uids[2],
			      forest.root,
			      forest.uids[1],
			      ITEM_TRANSFER_ABSENT_REVISION,
			      5,
			      item_custody_state::absent };
	for (size_t item = 0; item < creation.item_count; ++item)
		creation.items[item].vnum = forest.exact[item].vnum;
	std::vector<player_item_snapshot> inventory(forest.exact.begin(), forest.exact.begin() + 3);
	attach_blob(&creation, inventory);
	const auto created = critical_command_repository_apply(
		connection,
		accounted_item_transfer(operation(static_cast<uint8_t>(90 + index * 3)), creation,
					lineage, epoch, static_cast<uint32_t>(forest.pid),
					economic_source_kind::starter_grant));
	assert(created.outcome == critical_apply_outcome::applied && !created.error_code);
	// Independent protected equipment and adversarial foreign-child custody are
	// private fixture baselines, not additional qualified accounting producers.
	for (size_t item : { size_t{ 3 }, size_t{ 4 } })
		execute(connection,
			"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
			"owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(" +
				std::to_string(forest.uids[item]) + "," +
				std::to_string(forest.uids[item]) + ",NULL,1," +
				std::to_string(item == 3 ? forest.pid : forest.foreign_pid) +
				",0,1,5,1," + std::to_string(item == 3 ? 4 : 0) + ")");
	forest.save.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	forest.save.pid = forest.pid;
	forest.save.revision = 1;
	forest.save.components = PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_EQUIPMENT;
	forest.save.items = forest.exact;
	assert(player_snapshot_repository_apply(connection, forest.save).outcome ==
	       player_save_apply_outcome::applied);
	// Verify the real physical writer/loader bytes before freezing a drop.
	player_load_request load{};
	load.request_id = static_cast<uint64_t>(forest.pid);
	load.pid = forest.pid;
	load.account_name = "RoomPartialFixture" + std::to_string(forest.pid);
	load.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	auto captured = player_load_repository_execute(connection, load);
	assert(captured.outcome == player_load_outcome::applied &&
	       captured.snapshot.items.size() == 4);
	for (auto &item : captured.snapshot.items)
	{
		assert(item.extra_descriptions.size() == 1);
		auto &book = item.extra_descriptions[0];
		assert(book.keyword == "SPELLBOOK" && book.spellbook &&
		       book.description == "[4,9,15]");
		book.description.clear();
		book.spell_ids = { 4, 9, 15 };
	}
	assert(encode_exact_graph(captured.snapshot.items) == encode_exact_graph(forest.exact));
	forest.save.items = inventory;
	forest.save.revision = 2;
	forest.save.components = PLAYER_COMPONENT_INVENTORY | PLAYER_COMPONENT_STATUS;
	forest.save.status_integers.push_back({ player_status_field::wimpy, 47, 0, false });
	auto drop = creation;
	drop.from_owner = player;
	drop.to_owner = room;
	drop.reason = item_transfer_reason::player_drop;
	drop.reason_id = 22800;
	drop.selected_item_uid = forest.root;
	drop.target_root_item_uid = forest.root;
	drop.expected_from_revision = owner_revision(connection, player);
	drop.expected_to_revision = owner_revision(connection, room);
	for (size_t item = 0; item < drop.item_count; ++item)
	{
		drop.items[item].expected_item_revision = 1;
		drop.items[item].expected_state = item_custody_state::active;
	}
	attach_blob(&drop, inventory);
	forest.drop = accounted_item_transfer(operation(static_cast<uint8_t>(91 + index * 3)), drop,
					      lineage, epoch, static_cast<uint32_t>(forest.pid));
	forest.drop.publication_required = true;
	assert(economic_command_admission_supported(forest.drop));
	return forest;
}

static void partial_refuse_unchanged(MYSQL *connection, const partial_forest &forest)
{
	const auto before = partial_durable(connection, forest);
	const auto refused = player_snapshot_repository_apply(connection, forest.save);
	partial_expect(partial_custody_refusal(refused),
		       "stale partial frame refused on locked custody");
	partial_expect(
		partial_durable(connection, forest) == before,
		"refusal rolls back status/revision/native payload and fixed-ID economic history");
	const auto repeated = player_snapshot_repository_apply(connection, forest.save);
	partial_expect(partial_custody_refusal(repeated),
		       "same immutable partial frame refuses again");
	partial_expect(partial_durable(connection, forest) == before,
		       "repeat refusal preserves original UID graph and all durable receipts");
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
}

static std::thread partial_save_thread(const partial_forest &forest,
				       player_save_apply_result *result)
{
	return std::thread(
		[&forest, result]
		{
			assert(mysql_thread_init() == 0);
			MYSQL *connection = partial_open_connection();
			partial_role = partial_query_role::save;
			*result = player_snapshot_repository_apply(connection, forest.save);
			partial_role = partial_query_role::none;
			assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
			mysql_close(connection);
			mysql_thread_end();
		});
}

static critical_apply_result partial_pooled_drop(const critical_command &command)
{
	partial_role = partial_query_role::drop;
	const auto result = critical_command_repository_apply_from_pool(command, nullptr);
	partial_role = partial_query_role::none;
	return result;
}

static void partial_verify_drop(MYSQL *observer, const partial_forest &forest)
{
	const auto where = retained_where(forest.drop);
	assert(scalar(observer,
		      ("SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
		       std::to_string(forest.root) +
		       " AND owner_type=3 AND owner_id=22800 AND owner_context_id=0 AND item_revision=2 AND state=1")
			      .c_str()) == 3);
	for (const char *table : { "item_ownership_ledger", "economic_accounting_item_reference",
				   "sql_room_item_payload" })
		assert(scalar(observer,
			      (std::string("SELECT COUNT(*) FROM ") + table + " WHERE " + where)
				      .c_str()) == 3);
	for (const char *table : { "critical_operation_inbox", "economic_accounting_operation" })
		assert(scalar(observer,
			      (std::string("SELECT COUNT(*) FROM ") + table + " WHERE " + where)
				      .c_str()) == 1);
	assert(scalar(observer, ("SELECT COUNT(*) FROM critical_outbox WHERE " + where).c_str()) >
	       0);
	assert(scalar(observer,
		      ("SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(forest.pid))
			      .c_str()) == 1);
	assert(scalar(observer, ("SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
				 std::to_string(forest.uids[3]) + " AND equip_slot=4")
					.c_str()) == 1);
	MYSQL *cold = partial_open_connection();
	assert(mysql_thread_id(cold) != mysql_thread_id(observer));
	const std::vector<player_item_snapshot> selected(forest.exact.begin(),
							 forest.exact.begin() + 3);
	check_cold_payload(cold, selected);
	mysql_close(cold);
}

static std::string partial_foreign_insert(const partial_forest &forest, uint64_t parent)
{
	return "INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,obj_uid,item_type,"
	       "name,short_descr,description,action_descr) VALUES(" +
	       std::to_string(forest.foreign_pid) + ",5,0," + std::to_string(parent) + ",1," +
	       std::to_string(forest.foreign_uid) +
	       ",1,'Foreign FK fixture','Foreign FK fixture','Foreign FK fixture','Foreign FK fixture')";
}

static void partial_guarded_foreign_rows(MYSQL *connection, const partial_forest &forest)
{
	const auto id = scalar(connection, ("SELECT id FROM player_items WHERE obj_uid=" +
					    std::to_string(forest.foreign_uid))
						   .c_str());
	execute(connection, "INSERT INTO player_item_affects(item_id,location,modifier) VALUES(" +
				    std::to_string(id) + ",8,7)");
	execute(connection,
		"INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES(" +
			std::to_string(id) + ",'foreign','preserved foreign child description')");
	execute(connection, "INSERT INTO player_item_runtime_state(item_id,payload) VALUES(" +
				    std::to_string(id) + ",X'01020304')");
}
#endif

static int partial_save_matrix()
{
#ifndef DURIS_ECONOMIC_SQL_REAL_POOL_TEST
	std::fputs("room partial-save matrix requires the actual production pool build\n", stderr);
	mysql_library_end();
	return 2;
#else
	const char *seed = std::getenv("DURIS_SQL_ROOM_ITEM_SEED_EXPORT");
	assert(!seed || std::strcmp(seed, "1"));
	MYSQL *observer = partial_open_connection();
	assert(critical_operation_id_generate(&run_operation));
	ensure_collector_boundary_fixture(observer);
	const auto creator = operation(70), lineage = operation(71), epoch = operation(72);
	execute(observer,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
			operation_hex(creator) +
			"'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
	execute(observer,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) +
			"'),1,NULL,1,REPEAT(CHAR(0),32),UNHEX('" + operation_hex(creator) + "'))");
	execute(observer,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(UNHEX('" +
			operation_hex(lineage) + "'),UNHEX('" + operation_hex(epoch) + "'),0)");
	item_uid_allocator_reset_for_tests();
	const char *names[] = { "pooled_drop_stale_partial",  "save_first_drop_retry",
				"drop_first_save_retry",      "independent_pid_under_guard",
				"foreign_child_before_guard", "parent_locked_foreign_insert" };
	{
		economic_sql_real_pool_lifecycle pool_lifecycle;
		for (unsigned index = 0; index < 6; ++index)
		{
			const unsigned failures_before = partial_failures;
			auto forest = partial_seed(observer, index, lineage, epoch);
			const auto protected_before = partial_protected(observer, forest);
			if (index == 0)
			{
				economic_sql_commit_reply_loss_fixture::arm();
				const auto drop = exercise_sql_coordinator(
					forest.drop, "room-partial-stale", false,
					critical_apply_outcome::already_applied);
				economic_sql_commit_reply_loss_fixture::verify();
				assert(drop.outcome == critical_apply_outcome::already_applied);
				partial_verify_drop(observer, forest);
				partial_refuse_unchanged(observer, forest);
				partial_healthy_progress(observer, forest);
				partial_expect(
					partial_protected(observer, forest) == protected_before,
					"pooled drop and stale refusal preserve protected equipment/foreign custody");
			}
			else if (index == 1 || index == 3)
			{
				partial_query_barrier barrier(
					partial_query_role::save,
					index == 1 ? "COMMIT" :
						     "DELETE FROM player_items WHERE pid=");
				partial_barrier = &barrier;
				player_save_apply_result saved{};
				auto saver = partial_save_thread(forest, &saved);
				barrier.wait();
				assert(barrier.session != mysql_thread_id(observer));
				partial_healthy_progress(observer, forest);
				if (index == 1)
				{
					const auto waiting_drop = partial_pooled_drop(forest.drop);
					partial_expect(
						waiting_drop.outcome == critical_apply_outcome::
										retryable_failure &&
							waiting_drop.error_code == 1205,
						"save-first actual pooled drop lock wait returned 1205");
				}
				barrier.release();
				saver.join();
				partial_barrier = nullptr;
				partial_expect(
					saved.outcome == player_save_apply_outcome::applied &&
						saved.durable_revision == forest.save.revision,
					"identical selected graph partial save committed");
				partial_expect(
					partial_protected(observer, forest) == protected_before,
					"valid partial save leaves unselected physical bytes and original IDs intact");
				const auto committed = exercise_sql_coordinator(
					forest.drop, "room-partial-save-first");
				assert(committed.outcome == critical_apply_outcome::applied);
				partial_verify_drop(observer, forest);
				// This is a genuinely new save revision, not already-applied replay.
				++forest.save.revision;
				partial_refuse_unchanged(observer, forest);
				partial_expect(
					partial_protected(observer, forest) == protected_before,
					"save-first retry/drop and later refusal preserve protected native graph");
			}
			else if (index == 2)
			{
				partial_query_barrier barrier(partial_query_role::drop, "COMMIT");
				partial_barrier = &barrier;
				critical_apply_result dropped{};
				std::thread dropper(
					[&] { dropped = partial_pooled_drop(forest.drop); });
				barrier.wait();
				assert(barrier.session != mysql_thread_id(observer));
				const auto before = partial_durable(observer, forest);
				partial_healthy_progress(observer, forest);
				const auto waiting_save =
					player_snapshot_repository_apply(observer, forest.save);
				partial_expect(
					waiting_save.outcome ==
							player_save_apply_outcome::retryable_failure &&
						waiting_save.error_code == 1205,
					"drop-first actual partial save lock wait returned 1205");
				partial_expect(partial_durable(observer, forest) == before,
					       "blocked save rolled back before drop commit");
				barrier.release();
				dropper.join();
				partial_barrier = nullptr;
				assert(dropped.outcome == critical_apply_outcome::applied &&
				       !dropped.error_code);
				const auto replayed = exercise_sql_coordinator(
					forest.drop, "room-partial-drop-first", false,
					critical_apply_outcome::already_applied);
				assert(retained_result_equal(replayed, dropped));
				partial_verify_drop(observer, forest);
				partial_refuse_unchanged(observer, forest);
				partial_expect(
					partial_protected(observer, forest) == protected_before,
					"drop-first retry preserves protected original rows and custody");
			}
			else
			{
				const auto parent = scalar(
					observer, ("SELECT id FROM player_items WHERE obj_uid=" +
						   std::to_string(forest.root))
							  .c_str());
				const auto insert = partial_foreign_insert(forest, parent);
				MYSQL *foreign = partial_open_connection();
				assert(mysql_thread_id(foreign) != mysql_thread_id(observer));
				if (index == 4)
				{
					execute(foreign,
						insert); // Valid FK, deliberately foreign PID; no FK bypass.
					partial_guarded_foreign_rows(foreign, forest);
					partial_refuse_unchanged(observer, forest);
					partial_healthy_progress(observer, forest);
				}
				else
				{
					partial_query_barrier barrier(
						partial_query_role::save,
						"DELETE FROM player_items WHERE pid=");
					partial_barrier = &barrier;
					player_save_apply_result saved{};
					auto saver = partial_save_thread(forest, &saved);
					barrier.wait();
					assert(barrier.session != mysql_thread_id(observer) &&
					       barrier.session != mysql_thread_id(foreign));
					partial_healthy_progress(observer, forest);
					const auto before = partial_durable(observer, forest);
					const int inserted = mysql_real_query(
						foreign, insert.data(), insert.size());
					partial_expect(
						inserted != 0 && mysql_errno(foreign) == 1205,
						"real FK child insertion blocked by validated parent lock");
					partial_expect(
						partial_durable(observer, forest) == before,
						"blocked foreign insert created no row and changed no native history");
					barrier.release();
					saver.join();
					partial_barrier = nullptr;
					partial_expect(
						saved.outcome == player_save_apply_outcome::applied,
						"parent-locked valid partial save committed");
					partial_expect(
						partial_protected(observer, forest) ==
							protected_before,
						"parent-lock save preserves unselected equipment and foreign UID custody");
					const auto settled = partial_durable(observer, forest);
					const int retried = mysql_real_query(foreign, insert.data(),
									     insert.size());
					partial_expect(
						retried != 0 && mysql_errno(foreign) == 1452,
						"same original FK parent ID refused after replacement commit");
					partial_expect(
						partial_durable(observer, forest) == settled,
						"postcommit FK refusal preserves replacement tree and foreign custody");
				}
				mysql_close(foreign);
			}
			std::printf("CASE %s result=%s source=actual_pool_drop_and_direct_save\n",
				    names[index],
				    partial_failures == failures_before ? "pass" : "fail");
			std::fflush(stdout);
		}
	}
	mysql_close(observer);
	mysql_library_end();
	std::printf("%s: six room partial-save native pool/SQL cases; no gameplay route claim\n",
		    partial_failures ? "FAIL" : "PASS");
	return partial_failures ? 1 : 0;
#endif
}

int main()
{
	guard_room_fixture();
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	if (partial_matrix_enabled())
		return partial_save_matrix();
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
