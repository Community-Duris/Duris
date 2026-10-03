// Real SQL admission/read contract. This does not qualify live terminal extraction.
#include "player/player_load_repository.h"
#include "player/player_death_conflict_repository.h"
#include "persistence/persistence_observability.h"
#include "classes/necromancy.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include "sql/sql_pool.h"

#include <cerrno>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// Deterministic barriers around REAL client queries. Only the observed load
// connection is paused; retention runs normally on a second SQL connection.
namespace load_barrier
{
MYSQL *connection = nullptr;
std::function<void()> after_start;
std::function<void()> after_identity_lock;
} // namespace load_barrier
namespace query_observation
{
MYSQL *connection = nullptr;
size_t count = 0;
std::map<MYSQL_STMT *, MYSQL *> statements;
} // namespace query_observation
extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" int __wrap_mysql_real_query(MYSQL *db, const char *text, unsigned long length)
{
	if (db == query_observation::connection)
		++query_observation::count;
	const int rc = __real_mysql_real_query(db, text, length);
	if (!rc && db == load_barrier::connection)
	{
		const std::string_view statement(text, length);
		if (statement.starts_with("START TRANSACTION") && load_barrier::after_start)
		{
			auto callback = std::move(load_barrier::after_start);
			load_barrier::after_start = {};
			callback();
		}
		if (statement.find("LOCK IN SHARE MODE") != std::string_view::npos &&
		    load_barrier::after_identity_lock)
		{
			auto callback = std::move(load_barrier::after_identity_lock);
			load_barrier::after_identity_lock = {};
			callback();
		}
	}
	return rc;
}
extern "C" MYSQL_STMT *__real_mysql_stmt_init(MYSQL *);
extern "C" MYSQL_STMT *__wrap_mysql_stmt_init(MYSQL *db)
{
	auto statement = __real_mysql_stmt_init(db);
	if (statement)
		query_observation::statements.emplace(statement, db);
	return statement;
}
extern "C" int __real_mysql_stmt_execute(MYSQL_STMT *);
extern "C" int __wrap_mysql_stmt_execute(MYSQL_STMT *statement)
{
	const auto found = query_observation::statements.find(statement);
	if (found != query_observation::statements.end() &&
	    found->second == query_observation::connection)
		++query_observation::count;
	return __real_mysql_stmt_execute(statement);
}
using statement_close_result = decltype(mysql_stmt_close(nullptr));
extern "C" statement_close_result __real_mysql_stmt_close(MYSQL_STMT *);
extern "C" statement_close_result __wrap_mysql_stmt_close(MYSQL_STMT *statement)
{
	query_observation::statements.erase(statement);
	return __real_mysql_stmt_close(statement);
}

MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
void sql_pool_release(MYSQL *) {}
void sql_pool_discard_connection(MYSQL *) {}
MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
char *sql_escape_string(const char *)
{
	std::abort();
}

namespace
{
using query_outcome = player_death_recovery_query_outcome;
constexpr unsigned CASE_COUNT = 30;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}
const char *env(const char *name)
{
	const char *value = std::getenv(name);
	require(value && *value, std::string("missing environment: ") + name);
	return value;
}
void sql(MYSQL *db, const std::string &statement)
{
	require(mysql_real_query(db, statement.data(), statement.size()) == 0,
		"SQL operation failed, error=" + std::to_string(mysql_errno(db)));
}
using rows_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
std::string scalar(MYSQL *db, const std::string &statement)
{
	sql(db, statement);
	rows_ptr rows(mysql_store_result(db), mysql_free_result);
	require(bool(rows) && mysql_num_rows(rows.get()) == 1, "scalar cardinality");
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	const auto lengths = mysql_fetch_lengths(rows.get());
	require(row && row[0] && lengths, "scalar cell");
	return { row[0], lengths[0] };
}
MYSQL *connect()
{
	require(std::string(env("TEST_DB_DISPOSABLE")) == "1" &&
			std::string(env("DB_NAME")).starts_with("economic_schema_test_") &&
			std::string(env("DB_HOST")) == "127.0.0.1",
		"non-disposable target refused");
	MYSQL *db = mysql_init(nullptr);
	require(db, "client allocation");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect), "reconnect option");
	require(mysql_real_connect(db, env("DB_HOST"), env("DB_USER"), env("DB_PASSWD"),
				   env("DB_NAME"), std::stoul(env("DB_PORT")), nullptr, 0),
		"connection");
	sql(db, "SET NAMES utf8mb4");
	return db;
}
// All cells, including NULL/empty distinctions and binary payloads, stay in memory.
std::string native_state(MYSQL *db)
{
	std::string result;
	for (const auto *statement :
	     { "SELECT * FROM player_data ORDER BY pid", "SELECT * FROM player_items ORDER BY id",
	       "SELECT * FROM player_item_affects ORDER BY id",
	       "SELECT * FROM player_item_extra_descr ORDER BY id",
	       "SELECT * FROM player_pets ORDER BY id",
	       "SELECT * FROM player_pet_items ORDER BY id",
	       "SELECT * FROM player_pet_item_affects ORDER BY id",
	       "SELECT * FROM player_pet_item_extra_descr ORDER BY id",
	       "SELECT * FROM item_current_owner ORDER BY item_uid",
	       "SELECT * FROM item_owner_revision ORDER BY owner_type,owner_id,owner_context_id",
	       "SELECT * FROM player_death_conflict_evidence ORDER BY pid,save_revision",
	       "SELECT * FROM player_death_disposition ORDER BY pid,save_revision" })
	{
		sql(db, statement);
		rows_ptr rows(mysql_store_result(db), mysql_free_result);
		require(bool(rows), "source state query");
		result += std::string(statement) + ':' +
			  std::to_string(mysql_num_rows(rows.get())) + ':';
		while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
		{
			const auto lengths = mysql_fetch_lengths(rows.get());
			require(lengths, "source cell lengths");
			for (unsigned i = 0; i < mysql_num_fields(rows.get()); ++i)
				result += row[i] ? std::to_string(lengths[i]) + ':' +
							   std::string(row[i], lengths[i]) :
						   "NULL:";
		}
	}
	return result;
}
player_snapshot retained_request(unsigned index)
{
	player_snapshot request = {};
	request.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	request.pid = 1;
	request.revision = 100 + index;
	request.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	request.save_intent = 4;
	request.room_vnum = 1201;
	request.encoded_size_bound = 8192;
	request.recipes_are_external = true;
	request.status_strings.push_back({ player_status_string_field::name, "Probe" });
	request.status_integers.push_back({ player_status_field::level, 50, 0, false });
	for (auto field : { player_status_field::copper, player_status_field::silver,
			    player_status_field::gold, player_status_field::platinum })
		request.status_integers.push_back({ field, 0, 0, false });
	request.death.emplace();
	auto &death = *request.death;
	death.operation_id.bytes[0] = 0xad;
	death.operation_id.bytes[15] = static_cast<uint8_t>(index + 1);
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 10000 + index;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = 1;
	corpse.values[CORPSE_SAVEID] = 11000 + index;
	corpse.name = "a retained corpse";
	death.corpse.push_back(corpse);
	player_item_snapshot item = {};
	item.parent_index = 0;
	item.object_uid = 201;
	item.vnum = 501;
	item.name = "a blade\r\n\x1b[31m%s$n&r\xff";
	death.corpse.push_back(item);
	death.custody.push_back({ { 201, 201, 0, 3, 501, item_custody_state::active },
				  { item_owner_type::player, 1, 0 },
				  5 });
	return request;
}
void seed(MYSQL *db)
{
	require(scalar(db, "SELECT COUNT(*) FROM player_data") == "0", "fresh fixture required");
	sql(db, "INSERT INTO accounts(account_name) VALUES ('recovery_owner'),('recovery_other')");
	sql(db,
	    "INSERT INTO player_data(pid,name,account_name,active,save_revision,wallet_revision) VALUES (1,'Probe','recovery_owner',1,5,7),(2,'Other','recovery_other',1,2,0),(3,'Clean',NULL,1,2,0)");
	sql(db,
	    "INSERT INTO account_characters(account_name,char_name,pid) VALUES ('recovery_owner','Probe',1),('recovery_other','Other',2),('recovery_owner','Clean',3)");
	sql(db,
	    "INSERT INTO player_items(id,pid,vnum,obj_uid,name) VALUES (7101,1,501,201,'owned'),(7102,1,501,999,'foreign stray')");
	sql(db, "INSERT INTO player_item_affects(item_id,location,modifier) VALUES (7101,7,-40)");
	sql(db,
	    "INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES (7101,'private','SQL_ONLY_SECRET')");
	sql(db,
	    "INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES (1,1,5),(1,2,9)");
	sql(db,
	    "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum,state) VALUES (201,201,1,1,3,501,1),(999,999,1,2,6,501,1)");
	sql(db, "INSERT INTO player_pets(id,owner_pid,mob_vnum,pet_uid) VALUES (9101,1,100,5001)");
	sql(db, "INSERT INTO player_pet_items(id,pet_id,vnum,obj_uid) VALUES (9201,9101,501,5002)");
	for (unsigned index = 0; index < CASE_COUNT; ++index)
	{
		const auto result = player_death_conflict_retain(db, retained_request(index));
		require(result.outcome == player_death_conflict_outcome::retained,
			"native archive fixture rejected, error=" +
				std::to_string(result.error_code));
	}
	require(scalar(db, "SELECT COUNT(*) FROM player_death_disposition") == "0",
		"archive fixture advanced terminal state");
}
player_load_result load(MYSQL *db, int pid, const std::string &name, bool payload,
			bool by_name = false, bool recovery_query = false)
{
	player_load_request request = {};
	request.request_id = 123;
	request.pid = by_name ? 0 : pid;
	request.account_name = by_name ? "" : "recovery_owner";
	request.player_name = name;
	request.include_items = payload;
	request.include_pets = payload;
	if (recovery_query)
		request.death_recovery_query.kind = player_death_recovery_query_kind::list;
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	query_observation::connection = db;
	query_observation::count = 0;
	auto result = player_load_repository_execute(db, request);
	query_observation::connection = nullptr;
	if (!recovery_query && payload && result.outcome == player_load_outcome::applied)
	{
		const size_t query_limit = by_name ? PLAYER_LOAD_NAME_QUERY_MAX :
						     PLAYER_LOAD_PID_QUERY_MAX;
		// This request has no pending craft/spell receipts; empty obligation
		// results can also avoid witness reads. The budget is a ceiling.
		require(result.metrics.query_count == query_observation::count,
			"load query metrics disagree with actual client statement executions: reported=" +
				std::to_string(result.metrics.query_count) +
				" actual=" + std::to_string(query_observation::count));
		require(query_observation::count <= query_limit,
			"healthy full load exceeded query budget: actual=" +
				std::to_string(query_observation::count) +
				" limit=" + std::to_string(query_limit));
	}
	return result;
}
void refusal(const player_load_result &result, player_load_recovery_gate gate)
{
	require(result.outcome == player_load_outcome::component_failure &&
			result.recovery_gate == gate,
		"cold-load admission failed to refuse at recovery gate: outcome=" +
			std::to_string(static_cast<unsigned>(result.outcome)) +
			" error=" + std::to_string(result.error_code) + " component=" +
			(result.failed_component ? result.failed_component : "none"));
	require(result.snapshot.pid == 0 && result.snapshot.items.empty() &&
			result.snapshot.pets.empty() && result.snapshot.status_integers.empty() &&
			result.item_identities.empty() && result.pet_identities.empty(),
		"refused load returned materializable partial state");
}
constexpr int RACE_CASES = 8;
player_snapshot seed_race(MYSQL *db, int index)
{
	const int pid = 100 + index;
	const std::string id = std::to_string(pid);
	const std::string name = "Race" + std::string(1, static_cast<char>('a' + index));
	const uint64_t owned_uid = 600000 + 2 * pid;
	const uint64_t stray_uid = owned_uid + 1;
	sql(db,
	    "INSERT INTO player_data(pid,name,account_name,active,level,save_revision,wallet_revision) "
	    "VALUES(" +
		    id + ",'" + name + "','recovery_owner',1,50,2,7)");
	sql(db, "INSERT INTO account_characters(account_name,pid,char_name) VALUES"
		"('recovery_owner'," +
			id + ",'" + name + "')");
	sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,revision) "
		"VALUES(1," +
			id + ",5)");
	sql(db, "INSERT INTO player_items(pid,vnum,obj_uid,name) VALUES(" + id + ",501," +
			std::to_string(owned_uid) + ",'owned'),(" + id + ",501," +
			std::to_string(stray_uid) + ",'foreign stray')");
	sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,"
		"item_revision,vnum,state) VALUES(" +
			std::to_string(owned_uid) + "," + std::to_string(owned_uid) + ",1," + id +
			",3,501,1),(" + std::to_string(stray_uid) + "," +
			std::to_string(stray_uid) + ",1,2,6,501,1)");
	auto request = retained_request(40 + index);
	request.pid = pid;
	request.status_strings = { { player_status_string_field::name, name } };
	request.death->corpse[0].values[CORPSE_PID] = pid;
	request.death->corpse[1].object_uid = owned_uid;
	request.death->custody = { { { owned_uid, owned_uid, 0, 3, 501,
				       item_custody_state::active },
				     { item_owner_type::player, static_cast<uint64_t>(pid), 0 },
				     5 } };
	return request;
}

void verify_load_races(MYSQL *db, bool seed_cases)
{
	MYSQL *writer = connect();
	sql(writer, "SET SESSION innodb_lock_wait_timeout=1");
	for (int index = 0; index < RACE_CASES; ++index)
	{
		const int pid = 100 + index;
		const bool payload = index % 2 != 0;
		const bool by_name = (index / 2) % 2 != 0;
		const std::string name = "Race" + std::string(1, static_cast<char>('a' + index));
		if (!seed_cases)
		{
			refusal(load(db, pid, name, payload, by_name),
				player_load_recovery_gate::retained_conflict);
			continue;
		}
		const auto request = seed_race(writer, index);
		bool barrier_seen = false;
		load_barrier::connection = db;
		std::string retained_state;
		if (index < RACE_CASES / 2)
		{
			// The old WITH CONSISTENT SNAPSHOT admission misses this commit.
			// No reads or responses are faked: the real retention owner commits
			// on writer while the load is paused immediately after its START.
			load_barrier::after_start = [&]
			{
				barrier_seen = true;
				const auto saved = player_death_conflict_retain(writer, request);
				require(saved.outcome == player_death_conflict_outcome::retained,
					"barrier writer did not durably retain: " +
						std::to_string(saved.error_code));
				retained_state = native_state(writer);
			};
			const auto loaded = load(db, pid, name, payload, by_name);
			require(barrier_seen, "missing transaction-start barrier");
			require(loaded.recovery_gate ==
					player_load_recovery_gate::retained_conflict,
				"consistent-snapshot race admitted retained case; pid=" +
					std::to_string(pid) + " outcome=" +
					std::to_string(static_cast<int>(loaded.outcome)));
			refusal(loaded, player_load_recovery_gate::retained_conflict);
			require(native_state(writer) == retained_state,
				"racing refused load mutated durable state");
		}
		else
		{
			// In the reverse ordering, the shared player identity lock must
			// prevent retention committing until this read decision completes.
			const std::string before = native_state(writer);
			load_barrier::after_identity_lock = [&]
			{
				barrier_seen = true;
				const auto blocked = player_death_conflict_retain(writer, request);
				require(blocked.outcome == player_death_conflict_outcome::failed &&
						blocked.error_code == 1205,
					"retention crossed live load identity lock: " +
						std::to_string(blocked.error_code));
				require(!(writer->server_status & SERVER_STATUS_IN_TRANS),
					"blocked writer leaked a transaction");
			};
			const auto loaded = load(db, pid, name, payload, by_name);
			require(barrier_seen &&
					loaded.recovery_gate == player_load_recovery_gate::clear,
				"reverse-order load did not reach its serialized clear decision");
			require(!loaded.snapshot.status_integers.empty(),
				"healthy reverse-order load lost its status");
			require(native_state(writer) == before,
				"blocked retention mutated source state");
			const auto saved = player_death_conflict_retain(writer, request);
			require(saved.outcome == player_death_conflict_outcome::retained,
				"retention did not recover after load released its lock");
			refusal(load(db, pid, name, payload, by_name),
				player_load_recovery_gate::retained_conflict);
		}
		load_barrier::connection = nullptr;
		require(!load_barrier::after_start && !load_barrier::after_identity_lock,
			"unconsumed race barrier");
		require(!(db->server_status & SERVER_STATUS_IN_TRANS),
			"racing load leaked transaction");
	}
	mysql_close(writer);
	std::cout << "PASS: " << RACE_CASES
		  << (seed_cases ? " two-connection load/retention orderings" :
				   " race cases refused after restart")
		  << "; PID/name and full/metadata-only; retained source preserved\n";
}

void verify_name_resolution_race(MYSQL *db, bool seed_cases)
{
	MYSQL *writer = connect();
	if (seed_cases)
	{
		sql(writer,
		    "INSERT INTO player_data(pid,name,account_name,active,save_revision) VALUES "
		    "(200,'Turnover','recovery_owner',1,2),(201,'Replacement','recovery_owner',1,2)");
		for (bool payload : { false, true })
		{
			bool barrier_seen = false;
			std::string changed_state;
			load_barrier::connection = db;
			load_barrier::after_start = [&]
			{
				barrier_seen = true;
				sql(writer,
				    "UPDATE player_data SET name='Movedaway' WHERE pid=200");
				sql(writer, "UPDATE player_data SET name='Turnover' WHERE pid=201");
				changed_state = native_state(writer);
			};
			const auto reassigned = load(db, 0, "Turnover", payload, true);
			load_barrier::connection = nullptr;
			require(barrier_seen &&
					reassigned.outcome ==
						player_load_outcome::component_failure &&
					reassigned.error_code == ESTALE &&
					reassigned.snapshot.pid == 0 &&
					reassigned.snapshot.status_integers.empty() &&
					reassigned.snapshot.items.empty() &&
					reassigned.snapshot.pets.empty(),
				"name reassignment admitted an unlocked identity");
			require(!(db->server_status & SERVER_STATUS_IN_TRANS) &&
					native_state(writer) == changed_state,
				"name reassignment refusal leaked lock or mutated state");
			sql(writer, "UPDATE player_data SET name='Replacement' WHERE pid=201");
			sql(writer, "UPDATE player_data SET name='Turnover' WHERE pid=200");
		}
	}
	for (bool payload : { false, true })
	{
		const auto healthy = load(db, 0, "tUrNoVeR", payload, true);
		require(healthy.pid == 200 && healthy.snapshot.pid == 200 &&
				healthy.recovery_gate == player_load_recovery_gate::clear,
			"healthy case-insensitive name resolution regressed");
	}
	mysql_close(writer);
	std::cout << "PASS: "
		  << (seed_cases ? "name reassignment refused; " : "name identity after restart; ")
		  << "healthy case-insensitive full/metadata load preserved\n";
}

void verify(MYSQL *db)
{
	const auto before = native_state(db);
	const auto healthy = load(db, 3, "Clean", true);
	require(healthy.outcome == player_load_outcome::applied && healthy.snapshot.pid == 3 &&
			healthy.recovery_gate == player_load_recovery_gate::clear,
		"healthy ordinary load refused: outcome=" +
			std::to_string(static_cast<unsigned>(healthy.outcome)) +
			" error=" + std::to_string(healthy.error_code) + " component=" +
			(healthy.failed_component ? healthy.failed_component : "none"));
	for (bool payload : { false, true })
		for (bool by_name : { false, true })
			refusal(load(db, 1, "Probe", payload, by_name),
				player_load_recovery_gate::retained_conflict);
	const auto dispatched = load(db, 1, "Probe", false, false, true);
	require(dispatched.outcome == player_load_outcome::applied &&
			dispatched.snapshot.pid == 0 &&
			dispatched.death_recovery_query.outcome == query_outcome::read &&
			dispatched.request_account_name == "recovery_owner" &&
			dispatched.request_player_name == "Probe",
		"worker repository did not route a metadata-only recovery query");
	player_death_recovery_query_request request = {};
	request.kind = player_death_recovery_query_kind::list;
	const auto legacy_null =
		player_death_recovery_query_execute(db, 3, "recovery_owner", "Clean", request);
	require(legacy_null.outcome == query_outcome::read && legacy_null.cases.empty(),
		"legacy NULL-account membership fallback refused");
	auto page = player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(page.outcome == query_outcome::read && page.cases.size() == 25, "first SQL page");
	for (size_t i = 0; i < page.cases.size(); ++i)
		require(page.cases[i].operation_id.bytes ==
					retained_request(i).death->operation_id.bytes &&
				page.cases[i].save_revision == 100 + i,
			"first-page stable identity/order");
	request.after_revision = page.cases.back().save_revision;
	page = player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(page.outcome == query_outcome::read && page.cases.size() == CASE_COUNT - 25 &&
			page.cases.front().save_revision == 125 &&
			page.cases.back().save_revision == 129,
		"second page overlaps or omits cases");
	request.after_revision = page.cases.back().save_revision;
	page = player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(page.outcome == query_outcome::read && page.cases.empty(), "pagination end");
	request.after_revision = 0;
	for (const auto &identity :
	     { std::pair{ "recovery_other", "Probe" }, std::pair{ "recovery_owner", "Other" },
	       std::pair{ "recovery_owner' OR 1=1 -- ", "Probe" } })
	{
		const auto denied = player_death_recovery_query_execute(db, 1, identity.first,
									identity.second, request);
		require(denied.outcome == query_outcome::unauthorized && denied.cases.empty() &&
				denied.detail_summary.empty(),
			"cross-account/name/injection query admitted");
	}
	request.kind = player_death_recovery_query_kind::detail;
	request.operation_id = retained_request(0).death->operation_id;
	const auto detail =
		player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(detail.outcome == query_outcome::read &&
			detail.detail_identity.operation_id.bytes == request.operation_id.bytes &&
			detail.detail_identity.save_revision == 100 &&
			!detail.detail_summary.empty() &&
			detail.detail_summary.size() <= PLAYER_DEATH_RECOVERY_SUMMARY_MAX,
		"SQL detail identity/size");
	for (const auto *forbidden : { "SQL_ONLY_SECRET", "%", "$", "&", "\x1b", "\xff" })
		require(detail.detail_summary.find(forbidden) == std::string::npos,
			"detail leaks raw evidence/control bytes");
	const auto foreign =
		player_death_recovery_query_execute(db, 2, "recovery_other", "Other", request);
	require(foreign.outcome == query_outcome::not_found && foreign.detail_summary.empty(),
		"cross-character case leaked");
	// A stale association must not override the account in the character record.
	// Normal load_status() already prefers that non-NULL authoritative account.
	for (const auto kind :
	     { player_death_recovery_query_kind::list, player_death_recovery_query_kind::detail })
	{
		player_death_recovery_query_request drifted_request = {};
		drifted_request.kind = kind;
		if (kind == player_death_recovery_query_kind::detail)
			drifted_request.operation_id = retained_request(0).death->operation_id;
		sql(db, "UPDATE account_characters SET account_name='recovery_other' WHERE pid=1");
		const auto denied = player_death_recovery_query_execute(db, 1, "recovery_other",
									"Probe", drifted_request);
		sql(db, "UPDATE account_characters SET account_name='recovery_owner' WHERE pid=1");
		require(denied.outcome == query_outcome::unauthorized && denied.cases.empty() &&
				denied.detail_summary.empty(),
			"authoritative account disagreement exposed retained evidence");
	}
	sql(db, "UPDATE account_characters SET deleted_at=CURRENT_TIMESTAMP WHERE pid=1");
	const auto deleted =
		player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(deleted.outcome == query_outcome::unauthorized && deleted.detail_summary.empty(),
		"deleted membership admitted");
	sql(db, "UPDATE account_characters SET deleted_at=NULL WHERE pid=1");
	const auto old_hash = scalar(
		db,
		"SELECT HEX(payload_hash) FROM player_death_conflict_evidence WHERE pid=1 AND save_revision=100");
	sql(db,
	    "UPDATE player_death_conflict_evidence SET payload_hash=UNHEX(REPEAT('00',32)) WHERE pid=1 AND save_revision=100");
	const auto corrupt =
		player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(corrupt.outcome == query_outcome::failed && corrupt.detail_summary.empty(),
		"corrupt archive admitted");
	refusal(load(db, 1, "Probe", false), player_load_recovery_gate::retained_conflict);
	sql(db, "UPDATE player_death_conflict_evidence SET payload_hash=UNHEX('" + old_hash +
			"') WHERE pid=1 AND save_revision=100");
	sql(db, "START TRANSACTION");
	sql(db, "UPDATE player_data SET level=77 WHERE pid=1");
	for (bool payload : { false, true })
	{
		const auto refused = load(db, 1, "Probe", payload);
		require(refused.outcome == player_load_outcome::component_failure &&
				refused.error_code == EBUSY &&
				refused.snapshot.status_integers.empty() &&
				(db->server_status & SERVER_STATUS_IN_TRANS) &&
				scalar(db, "SELECT level FROM player_data WHERE pid=1") == "77",
			"normal load altered caller-owned transaction");
	}
	const auto borrowed =
		player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(borrowed.outcome == query_outcome::failed && borrowed.error_code == EBUSY &&
			(db->server_status & SERVER_STATUS_IN_TRANS) &&
			scalar(db, "SELECT level FROM player_data WHERE pid=1") == "77",
		"adapter committed or rolled back caller-owned transaction");
	sql(db, "ROLLBACK");
	sql(db,
	    "RENAME TABLE player_death_conflict_evidence TO fixture_parked_death_conflict_evidence");
	const auto missing =
		player_death_recovery_query_execute(db, 1, "recovery_owner", "Probe", request);
	require(missing.outcome == query_outcome::failed && missing.error_code == 1146 &&
			missing.detail_summary.empty(),
		"missing schema was presented as empty/success");
	for (bool payload : { false, true })
		refusal(load(db, 1, "Probe", payload), player_load_recovery_gate::unavailable);
	sql(db,
	    "RENAME TABLE fixture_parked_death_conflict_evidence TO player_death_conflict_evidence");
	require(!(db->server_status & SERVER_STATUS_IN_TRANS), "adapter/load leaked transaction");
	require(native_state(db) == before,
		"read/admission checks mutated native payload, custody, wallet or archive");
	std::cout
		<< "PASS: native SQL healthy load; full/metadata-only PID/name conflict refusal; async repository dispatch; "
		<< CASE_COUNT
		<< " ordered cases; self-scoped list/detail; sanitization; deleted membership; hash/schema failure; "
		<< "borrowed transaction preserved; byte-exact native state preservation\n";
}
} // namespace
int main(int argc, char **argv)
{
	require(argc == 2 && (std::string(argv[1]) == "--seed-and-check" ||
			      std::string(argv[1]) == "--verify-restart"),
		"explicit mode required");
	require(mysql_library_init(0, nullptr, nullptr) == 0, "client library init");
	MYSQL *db = connect();
	if (std::string(argv[1]) == "--seed-and-check")
		seed(db);
	require(scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=1") ==
			std::to_string(CASE_COUNT),
		"case fixture count");
	verify(db);
	verify_load_races(db, std::string(argv[1]) == "--seed-and-check");
	verify_name_resolution_race(db, std::string(argv[1]) == "--seed-and-check");
	mysql_close(db);
	mysql_library_end();
	return 0;
}
