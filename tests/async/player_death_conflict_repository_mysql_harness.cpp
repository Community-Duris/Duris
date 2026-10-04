#include "player/player_death_conflict_repository.h"
#include "player/player_snapshot_repository.h"
#include "player/player_snapshot_codec.h"
#include "classes/necromancy.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include "sql/sql_pool.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include <algorithm>
#include <barrier>
#include <thread>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <type_traits>

MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
void sql_pool_release(MYSQL *) {}
bool sql_pool_retire_owned_connection(MYSQL *)
{
	// This direct-repository fixture has no owned pool slots to retire.
	return false;
}
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
constexpr int PROBE_PID = 1;
using outcome = player_death_conflict_outcome;
void require(bool ok, const std::string &message)
{
	if (!ok)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}
std::string env(const char *key, const char *fallback = "")
{
	auto value = std::getenv(key);
	return value && *value ? value : fallback;
}
void execute(MYSQL *db, const std::string &sql)
{
	const int code = mysql_real_query(db, sql.data(), sql.size());
	require(!code, "SQL error " + std::to_string(mysql_errno(db)) + ": " + mysql_error(db) +
			       " statement=" + sql.substr(0, 160));
}
std::string scalar(MYSQL *db, const std::string &sql)
{
	execute(db, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(mysql_store_result(db),
									mysql_free_result);
	require(bool(result), "missing result");
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths && row[0], "missing scalar");
	return { row[0], lengths[0] };
}
MYSQL *connect()
{
	auto db = mysql_init(nullptr);
	require(db, "mysql_init");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect), "reconnect option");
	const auto database = env("DB_NAME");
	require(env("TEST_DB_DISPOSABLE") == "1" && database.starts_with("economic_schema_test_"),
		"non-disposable database refused");
	require(mysql_real_connect(db, env("DB_HOST", "127.0.0.1").c_str(),
				   env("DB_USER", "root").c_str(), env("DB_PASSWD").c_str(),
				   database.c_str(), std::stoul(env("DB_PORT", "3306")), nullptr,
				   0),
		"connect");
	execute(db,
		"SET NAMES latin1"); // Deliberate hostile connection charset: raw UTF-8 must survive.
	execute(db, "SET SESSION sql_mode='STRICT_TRANS_TABLES,NO_BACKSLASH_ESCAPES'");
	return db;
}
player_snapshot make_death(player_revision_t revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = PROBE_PID;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4; // RENT_DEATH
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 50, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::silver, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::gold, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::platinum, 0, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Probe" });
	snapshot.recipes_are_external = true;

	snapshot.death.emplace();
	player_death_snapshot &death = *snapshot.death;
	death.operation_id.bytes.fill(0);
	death.operation_id.bytes[0] = 0xa5;
	death.operation_id.bytes[15] = 0x5a;
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	death.wallet_before = { 11, 12, 13, 14 };
	death.wallet_pile_uid = 202;

	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 200;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = PROBE_PID;
	corpse.values[CORPSE_SAVEID] = 9001;
	death.corpse.push_back(corpse);

	player_item_snapshot refused = {};
	refused.parent_index = 0;
	refused.object_uid = 201;
	refused.vnum = 501;
	death.corpse.push_back(refused);

	player_item_snapshot captured_child = {};
	captured_child.parent_index = 0;
	captured_child.object_uid = 203;
	captured_child.vnum = 501;
	death.corpse.push_back(captured_child);

	player_item_snapshot wallet = {};
	wallet.parent_index = 0;
	wallet.object_uid = death.wallet_pile_uid;
	wallet.vnum = VOBJ_COINS;
	wallet.type = ITEM_MONEY;
	for (size_t denomination = 0; denomination < death.wallet_before.size(); ++denomination)
		wallet.values[denomination] = death.wallet_before[denomination];
	death.corpse.push_back(wallet);

	death.custody.push_back({ { 201, 201, 0, 3, 501, item_custody_state::active },
				  { item_owner_type::player, PROBE_PID, 0 },
				  5 });
	death.custody.push_back({ { 203, 201, 201, 1, 501, item_custody_state::active },
				  { item_owner_type::player, PROBE_PID, 0 },
				  5 });
	death.custody.push_back(
		{ { death.wallet_pile_uid, death.wallet_pile_uid, 0, ITEM_TRANSFER_ABSENT_REVISION,
		    VOBJ_COINS, item_custody_state::absent },
		  {},
		  0 });
	return snapshot;
}

std::string encoded(const player_snapshot &snapshot)
{
	std::vector<uint8_t> data;
	require(player_snapshot_encode(snapshot, &data) == player_snapshot_codec_result::ok,
		"encode fixture");
	return { reinterpret_cast<const char *>(data.data()), data.size() };
}
const std::optional<std::string> &cell(const player_death_evidence_table &table, size_t row,
				       const char *name)
{
	const auto i = std::find(table.columns.begin(), table.columns.end(), name);
	require(i != table.columns.end(), std::string("missing column ") + name);
	return table.rows.at(row).at(static_cast<size_t>(i - table.columns.begin()));
}
std::string source_state(MYSQL *db)
{
	std::string result;
	for (const auto *query :
	     { "SELECT CONCAT_WS(':',save_revision,wallet_revision,copper,silver,gold,platinum) FROM player_data WHERE pid=1",
	       "SELECT GROUP_CONCAT(CONCAT_WS(':',id,COALESCE(obj_uid,0),COALESCE(container_id,0),extra_flags) ORDER BY id) FROM player_items",
	       "SELECT GROUP_CONCAT(CONCAT_WS(':',item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_id,item_revision,state) ORDER BY item_uid) FROM item_current_owner",
	       "SELECT GROUP_CONCAT(CONCAT_WS(':',owner_type,owner_id,owner_context_id,revision) ORDER BY owner_type,owner_id,owner_context_id) FROM item_owner_revision",
	       "SELECT GROUP_CONCAT(CONCAT_WS(':',id,item_id,COALESCE(location,'NULL'),modifier) ORDER BY id) FROM player_item_affects",
	       "SELECT GROUP_CONCAT(CONCAT_WS(':',id,item_id,HEX(keyword),COALESCE(HEX(description),'NULL')) ORDER BY id) FROM player_item_extra_descr",
	       "SELECT COUNT(*) FROM player_death_disposition" })
		result += scalar(db, query) + '\n';
	return result;
}
player_snapshot fresh(unsigned n)
{
	auto request = make_death(100 + n);
	request.death->operation_id.bytes[14] = static_cast<uint8_t>(n);
	request.death->corpse.front().object_uid = 10000 + n;
	request.death->corpse.front().values[CORPSE_SAVEID] = 11000 + n;
	return request;
}
void unchanged_failure(MYSQL *db, const player_snapshot &request, outcome expected)
{
	const auto before = source_state(db);
	const auto count = scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence");
	const auto result = player_death_conflict_retain(db, request);
	require(result.outcome == expected,
		"expected refusal: got " + std::to_string(static_cast<unsigned>(result.outcome)) +
			" error=" + std::to_string(result.error_code));
	require(source_state(db) == before &&
			scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence") == count,
		"refusal mutated source or retained a partial record");
	require(!(db->server_status & SERVER_STATUS_IN_TRANS), "transaction leaked");
}
player_snapshot terminal_request(bool spell_receipt = false, bool quest_receipt = false)
{
	auto request = make_death(7);
	request.death->wallet_before.fill(0);
	request.death->wallet_pile_uid = 0;
	std::erase_if(request.death->corpse,
		      [](const auto &item) { return item.object_uid == 202; });
	std::erase_if(request.death->custody,
		      [](const auto &item) { return item.item.item_uid == 202; });
	request.status_integers.push_back({ player_status_field::hit_difference, -321, 0, false });
	if (spell_receipt)
	{
		request.schema_version = PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
		player_spell_effect_receipt_snapshot receipt = {};
		receipt.operation_id.bytes[0] = 77;
		receipt.effect_id = 6;
		request.spell_effect_receipts.push_back(receipt);
		player_affect_snapshot affect = {};
		affect.type = 77;
		affect.duration = 23;
		request.affects.push_back(affect);
	}
	if (quest_receipt)
	{
		request.schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
		player_quest_xp_receipt_snapshot receipt = {};
		receipt.offering_operation.bytes[0] = 88;
		receipt.amount = 75;
		request.quest_xp_receipts.push_back(receipt);
		request.status_integers.push_back(
			{ player_status_field::experience, 75, 0, false });
	}
	return request;
}

void seed_terminal_quest(MYSQL *db)
{
	std::vector<uint8_t> bytes;
	const auto add = [&](uint64_t value, unsigned width)
	{
		for (unsigned index = 0; index < width; ++index)
			bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
	};
	// Version-5 solo continuation: one offered root and one frozen XP award.
	for (uint32_t value : { 5, PROBE_PID, 1, 0, 77, 1201 })
		add(value, 4);
	add(123456789, 8);
	add(1, 4);
	add(9001, 8);
	add(1, 4);
	for (uint32_t value : { 5, 75, 0, 75, 1, 50, 0, 1, 50, 1, PROBE_PID })
		add(value, 4);
	add(5, 4);
	for (char value : std::string("Probe"))
		bytes.push_back(value);
	add(1, 4);
	bytes.push_back('d');
	for (uint32_t value : { 1, PROBE_PID, 0, 75 })
		add(value, 4);
	static constexpr char digits[] = "0123456789abcdef";
	std::string hex;
	for (uint8_t byte : bytes)
	{
		hex += digits[byte >> 4];
		hex += digits[byte & 15];
	}
	execute(db,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('58000000000000000000000000000000'),UNHEX(REPEAT('00',32)),UNHEX(REPEAT('00',32)),2,2,1,1,X'')");
	execute(db,
		"INSERT INTO quest_reward_obligation(offering_operation_id,player_pid,continuation) VALUES(UNHEX('58000000000000000000000000000000'),1,UNHEX('" +
			hex + "'))");
	execute(db,
		"INSERT INTO quest_reward_xp_entitlement(offering_operation_id,recipient_pid,reward_index,amount) VALUES(UNHEX('58000000000000000000000000000000'),1,0,75)");
}

std::string terminal_quest_state(MYSQL *db)
{
	return scalar(db, "SELECT exp FROM player_data WHERE pid=1") + ':' +
	       scalar(db,
		      "SELECT xp_applied_mask FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')") +
	       ':' +
	       scalar(db,
		      "SELECT CONCAT_WS(':',amount,COALESCE(CAST(applied_at AS CHAR),'NULL')) FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')");
}

// Length-prefix every SQL cell, preserving all columns and NULL independently
// of empty strings. This is a source-state assertion, never a restore format.
std::string protected_state(MYSQL *db)
{
	std::string output;
	for (const auto *query :
	     { "SELECT wallet_revision,copper,silver,gold,platinum FROM player_data ORDER BY pid",
	       "SELECT * FROM player_items ORDER BY id",
	       "SELECT * FROM player_item_affects ORDER BY id",
	       "SELECT * FROM player_item_extra_descr ORDER BY id",
	       "SELECT * FROM item_current_owner ORDER BY item_uid",
	       "SELECT * FROM item_owner_revision ORDER BY owner_type,owner_id,owner_context_id",
	       "SELECT * FROM player_pets ORDER BY id",
	       "SELECT * FROM player_pet_items ORDER BY id",
	       "SELECT * FROM player_pet_item_affects ORDER BY id",
	       "SELECT * FROM player_pet_item_extra_descr ORDER BY id" })
	{
		execute(db, query);
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
			mysql_store_result(db), mysql_free_result);
		require(bool(rows), "protected-state result");
		output +=
			std::string(query) + ':' + std::to_string(mysql_num_rows(rows.get())) + ':';
		while (auto row = mysql_fetch_row(rows.get()))
		{
			const auto lengths = mysql_fetch_lengths(rows.get());
			require(lengths, "protected-state lengths");
			for (unsigned index = 0; index < mysql_num_fields(rows.get()); ++index)
				output += row[index] ?
						  std::to_string(lengths[index]) + ':' +
							  std::string(row[index], lengths[index]) :
						  "NULL:";
		}
	}
	return output;
}

void terminal_receipt(MYSQL *db, const player_snapshot &request)
{
	require(scalar(db,
		       "SELECT CONCAT_WS(':',save_revision,level,hit_diff,wallet_revision,copper,silver,gold,platinum) FROM player_data WHERE pid=1") ==
			"7:50:-321:7:0:0:0:0",
		"terminal player state/wallet was not committed");
	require(scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence") == "1" &&
			scalar(db, "SELECT COUNT(*) FROM player_death_disposition") == "1",
		"terminal records missing/duplicated");
	player_snapshot retained;
	require(player_death_conflict_read(db, request.pid, request.death->operation_id, &retained)
				.outcome == outcome::read,
		"terminal archive cannot read back");
	require(encoded(retained) ==
			scalar(db,
			       "SELECT payload FROM player_death_disposition WHERE pid=1 AND save_revision=7"),
		"terminal disposition differs from hash-checked evidence");
	auto original = retained;
	original.schema_version = player_snapshot_death_request_schema(retained.schema_version);
	original.death->conflict_evidence.reset();
	require(encoded(original) == encoded(request),
		"terminal archive changed request identity/body");
	require(scalar(db,
		       "SELECT COUNT(*) FROM player_death_custody WHERE pid=1 AND save_revision=7") ==
			std::to_string(request.death->custody.size()),
		"terminal custody observations missing");
}

void terminal_tests(MYSQL *&db, const std::string &requested_mode)
{
	using save = player_save_apply_outcome;
	const bool quest_receipt = requested_mode == "--terminal-quest-matrix";
	const bool spell_receipt = quest_receipt || requested_mode == "--terminal-spell-matrix";
	const std::string mode = spell_receipt ? "--terminal-matrix" : requested_mode;
	const auto request = terminal_request(spell_receipt, quest_receipt);
	if (quest_receipt)
		seed_terminal_quest(db);
	const auto protected_before = protected_state(db);
	const auto native_before = source_state(db);
	const auto refuse = [&](const player_snapshot &input)
	{
		const auto before =
			protected_state(db) + source_state(db) +
			scalar(db,
			       "SELECT CONCAT_WS(':',level,hit_diff) FROM player_data WHERE pid=1");
		const auto count =
			scalar(db, "SELECT COUNT(*) FROM player_death_conflict_evidence");
		const auto spell_before =
			scalar(db, "SELECT COUNT(*) FROM player_spell_effect_receipt");
		const auto affects_before = scalar(db, "SELECT COUNT(*) FROM player_affects");
		const auto xp_before = quest_receipt ? terminal_quest_state(db) : std::string{};
		const auto result = player_death_conflict_apply(db, input);
		require(result.outcome != save::applied &&
				result.outcome != save::already_applied &&
				result.outcome != save::stale_revision,
			"failed terminal attempt was acknowledged");
		require(before == protected_state(db) + source_state(db) +
						scalar(db,
						       "SELECT CONCAT_WS(':',level,hit_diff) FROM player_data WHERE pid=1") &&
				count ==
					scalar(db,
					       "SELECT COUNT(*) FROM player_death_conflict_evidence"),
			"terminal refusal changed source, state, or archive");
		require(spell_before == scalar(db,
					       "SELECT COUNT(*) FROM player_spell_effect_receipt") &&
				affects_before == scalar(db, "SELECT COUNT(*) FROM player_affects"),
			"terminal refusal partially committed spell effect or receipt");
		require(!quest_receipt || terminal_quest_state(db) == xp_before,
			"terminal refusal partially committed XP or its application marker");
		require(!(db->server_status & SERVER_STATUS_IN_TRANS),
			"terminal refusal leaked transaction");
		return result;
	};
	if (mode == "--terminal-matrix")
	{
		auto control = connect();
		{
			economic_sql_lifecycle_guard maintenance;
			require(economic_sql_lifecycle_guard::acquire_maintenance(
					control, &maintenance) == 0,
				"terminal maintenance guard");
			require(refuse(request).error_code == EPERM,
				"terminal owner bypassed maintenance");
		}
		mysql_close(control);
		execute(db, "START TRANSACTION");
		execute(db, "UPDATE player_data SET level=77 WHERE pid=1");
		require(player_death_conflict_apply(db, request).error_code == EBUSY &&
				(db->server_status & SERVER_STATUS_IN_TRANS) &&
				scalar(db, "SELECT level FROM player_data WHERE pid=1") == "77",
			"terminal owner committed or rolled back a borrowed transaction");
		execute(db, "ROLLBACK");
		for (const auto *trigger :
		     { "CREATE TRIGGER terminal_fail AFTER INSERT ON player_death_conflict_evidence FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected archive failure'",
		       "CREATE TRIGGER terminal_fail AFTER INSERT ON player_death_disposition FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected disposition failure'",
		       "CREATE TRIGGER terminal_fail BEFORE UPDATE ON player_data FOR EACH ROW BEGIN IF NEW.save_revision<>OLD.save_revision THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected revision failure'; END IF; END" })
		{
			execute(db, trigger);
			require(refuse(request).error_code == 1644,
				"fault did not reach intended terminal write");
			execute(db, "DROP TRIGGER terminal_fail");
		}
		auto invalid = request;
		invalid.death->wallet_before[0] = 1;
		refuse(invalid);
		execute(db, "UPDATE player_data SET copper=1 WHERE pid=1");
		refuse(request);
		execute(db, "UPDATE player_data SET copper=0 WHERE pid=1");
		// A bare revision, including a newer unrelated save, is not a receipt.
		execute(db, "UPDATE player_data SET save_revision=7 WHERE pid=1");
		refuse(request);
		execute(db, "UPDATE player_data SET save_revision=8 WHERE pid=1");
		refuse(request);
		execute(db, "UPDATE player_data SET save_revision=5 WHERE pid=1");
		// Upgrade an archive-only crash boundary without recapturing evidence.
		require(player_death_conflict_retain(db, request).outcome == outcome::retained,
			"archive-only fixture");
		require(source_state(db) == native_before, "archive alone advanced terminal state");
		execute(db, "UPDATE player_data SET save_revision=6 WHERE pid=1");
		refuse(request);
		execute(db, "UPDATE player_data SET save_revision=5 WHERE pid=1");
	}
	if (mode == "--terminal-concurrent")
	{
		std::array<player_save_apply_result, 2> results = {};
		std::barrier ready(3);
		const auto writer = [&](size_t index)
		{
			require(mysql_thread_init() == 0, "terminal thread init");
			auto connection = connect();
			ready.arrive_and_wait();
			results[index] = player_death_conflict_apply(connection, request);
			require(!(connection->server_status & SERVER_STATUS_IN_TRANS),
				"concurrent terminal leaked transaction");
			mysql_close(connection);
			mysql_thread_end();
		};
		std::thread first(writer, 0), second(writer, 1);
		ready.arrive_and_wait();
		first.join();
		second.join();
		require(std::count_if(results.begin(), results.end(), [](auto result)
				      { return result.outcome == save::applied; }) == 1 &&
				std::count_if(
					results.begin(), results.end(), [](auto result)
					{ return result.outcome == save::already_applied; }) == 1,
			"concurrent terminal did not commit exactly once: " +
				std::to_string(results[0].error_code) + ',' +
				std::to_string(results[1].error_code));
	}
	else
	{
		const auto result = player_death_conflict_apply(db, request);
		if (mode == "--terminal-ambiguous")
		{
			require(result.outcome == save::ambiguous_commit,
				"terminal COMMIT loss not reported ambiguous");
			mysql_close(db);
			db = connect();
		}
		else
			require(result.outcome == save::applied &&
					result.durable_revision == request.revision,
				"terminal owner did not commit: outcome=" +
					std::to_string(static_cast<unsigned>(result.outcome)) +
					" error=" + std::to_string(result.error_code));
	}
	require(player_death_conflict_apply(db, request).outcome == save::already_applied,
		"terminal exact replay failed");
	terminal_receipt(db, request);
	if (quest_receipt)
	{
		require(scalar(db, "SELECT exp FROM player_data WHERE pid=1") == "75" &&
				scalar(db,
				       "SELECT xp_applied_mask FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')") ==
					"1" &&
				scalar(db,
				       "SELECT applied_at IS NOT NULL FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')") ==
					"1",
			"terminal XP and application markers did not commit together");
		player_snapshot retained;
		require(player_death_conflict_read(db, 1, request.death->operation_id, &retained)
						.outcome == outcome::read &&
				retained.schema_version ==
					PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION &&
				retained.quest_xp_receipts.size() == 1 &&
				retained.spell_effect_receipts.size() == 1,
			"terminal archive dropped the coupled receipts");
		execute(db,
			"UPDATE quest_reward_xp_entitlement SET applied_at=NULL WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')");
		refuse(request);
		execute(db,
			"UPDATE quest_reward_xp_entitlement SET applied_at=CURRENT_TIMESTAMP(6) WHERE offering_operation_id=UNHEX('58000000000000000000000000000000')");
		auto invalid = request;
		invalid.quest_xp_receipts[0].amount = 74;
		refuse(invalid);
		require(player_death_conflict_apply(db, request).outcome == save::already_applied,
			"restored exact XP receipt did not replay");
	}
	if (spell_receipt)
	{
		require(scalar(db,
			       "SELECT effect_id FROM player_spell_effect_receipt WHERE pid=1 AND operation_id=UNHEX('4d000000000000000000000000000000')") ==
					"6" &&
				scalar(db,
				       "SELECT duration FROM player_affects WHERE pid=1 AND type=77") ==
					"23",
			"terminal affect and receipt did not commit together");
		execute(db, "DELETE FROM player_spell_effect_receipt WHERE pid=1");
		refuse(request);
		execute(db,
			"INSERT INTO player_spell_effect_receipt(pid,operation_id,effect_id) VALUES(1,UNHEX('4d000000000000000000000000000000'),1)");
		refuse(request);
		execute(db, "UPDATE player_spell_effect_receipt SET effect_id=6 WHERE pid=1");
		require(player_death_conflict_apply(db, request).outcome == save::already_applied,
			"restored exact spell receipt did not replay");
	}
	require(protected_state(db) == protected_before,
		"terminal commit changed payload/custody/pet/wallet bytes");
	auto changed = request;
	changed.status_integers.front().signed_value = 51;
	refuse(changed);
	changed = request;
	changed.death->operation_id.bytes[2] = 99;
	refuse(changed);
	changed = request;
	changed.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	changed.death.reset();
	changed.spell_effect_receipts.clear();
	changed.quest_xp_receipts.clear();
	changed.revision = 8;
	require(refuse(changed).error_code == EBUSY,
		"unresolved case allowed a later partial checkpoint");
	const auto receipt =
		scalar(db, "SELECT HEX(payload) FROM player_death_disposition WHERE pid=1");
	execute(db, "UPDATE player_death_disposition SET payload='invalid' WHERE pid=1");
	refuse(request);
	execute(db,
		"UPDATE player_death_disposition SET payload=UNHEX('" + receipt + "') WHERE pid=1");
	terminal_receipt(db, request);
	std::cout
		<< "PASS: " << requested_mode
		<< "; exact terminal receipt/replay, unchanged complete payload/custody/pet/wallet state, wrong identity/counter/partial-save refusal\n";
}
}
int main(int argc, char **argv)
{
	require(!mysql_library_init(0, nullptr, nullptr), "client init");
	auto db = connect();
	const auto death = make_death(7);
	if (argc > 1 && std::string(argv[1]) == "--terminal-restart")
	{
		const auto request = terminal_request();
		const auto before = protected_state(db);
		require(player_death_conflict_apply(db, request).outcome ==
				player_save_apply_outcome::already_applied,
			"restart did not verify the exact terminal receipt");
		terminal_receipt(db, request);
		require(protected_state(db) == before, "restart replay changed source state");
		std::cout
			<< "PASS: terminal restart exact receipt/replay with unchanged source state\n";
		mysql_close(db);
		mysql_library_end();
		return 0;
	}
	if (argc > 1 && std::string(argv[1]) == "--verify-restart")
	{
		player_snapshot readback = {};
		require(player_death_conflict_read(db, 1, death.death->operation_id, &readback)
					.outcome == outcome::read,
			"restart read");
		require(player_death_conflict_retain(db, death).outcome ==
				outcome::already_retained,
			"restart replay");
		require(scalar(db, "SELECT save_revision FROM player_data WHERE pid=1") == "5",
			"restart changed player revision");
		std::cout
			<< "PASS: restart read/replay with original operation and unchanged player revision\n";
		mysql_close(db);
		mysql_library_end();
		return 0;
	}
	if (argc > 1 && std::string(argv[1]) == "--concurrent-replay")
	{
		constexpr unsigned rounds = 8;
		const auto before = source_state(db);
		for (unsigned round = 0; round < rounds; ++round)
		{
			const auto request = fresh(60 + round);
			std::array<player_death_conflict_result, 2> results = {};
			std::barrier ready(3);
			const auto writer = [&](size_t index)
			{
				require(mysql_thread_init() == 0, "worker client init");
				auto connection = connect();
				ready.arrive_and_wait();
				results[index] = player_death_conflict_retain(connection, request);
				require(!(connection->server_status & SERVER_STATUS_IN_TRANS),
					"concurrent caller leaked a transaction");
				mysql_close(connection);
				mysql_thread_end();
			};
			std::thread first(writer, 0), second(writer, 1);
			ready.arrive_and_wait();
			first.join();
			second.join();
			const auto retained_count =
				std::count_if(results.begin(), results.end(),
					      [](const auto &result) {
						      return result.outcome == outcome::retained &&
							     !result.error_code;
					      });
			const auto replay_count = std::count_if(
				results.begin(), results.end(),
				[](const auto &result) {
					return result.outcome == outcome::already_retained &&
					       !result.error_code;
				});
			require(retained_count == 1 && replay_count == 1,
				"concurrent identity did not serialize: " +
					std::to_string(results[0].error_code) + "," +
					std::to_string(results[1].error_code));
			require(scalar(db,
				       "SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=1 AND save_revision=" +
					       std::to_string(request.revision)) == "1",
				"concurrent identity duplicated archive");
			require(player_death_conflict_retain(db, request).outcome ==
					outcome::already_retained,
				"same-operation replay after concurrent submission failed");
			player_snapshot archived = {};
			require(player_death_conflict_read(db, 1, request.death->operation_id,
							   &archived)
						.outcome == outcome::read,
				"concurrent archive did not read back");
			archived.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
			archived.death->conflict_evidence.reset();
			require(encoded(archived) == encoded(request),
				"concurrent archive changed request identity/body");
		}
		require(source_state(db) == before,
			"concurrent archive mutated authoritative sources");
		mysql_close(db);
		mysql_library_end();
		std::cout
			<< "PASS: " << rounds
			<< " concurrent request pairs, one retained + one already-retained per pair; "
			   "exact replay/readback, no duplicate record, source mutation or leaked transaction\n";
		return 0;
	}
	if (argc > 1 && std::string(argv[1]) == "--ambiguous-commit")
	{
		const auto request = fresh(90);
		const auto result = player_death_conflict_retain(db, request);
		require(result.outcome == outcome::commit_unknown,
			"lost COMMIT acknowledgement was treated as success/failure: " +
				std::to_string(static_cast<unsigned>(result.outcome)) +
				" error=" + std::to_string(result.error_code));
		mysql_close(db);
		db = connect();
		require(player_death_conflict_retain(db, request).outcome ==
				outcome::already_retained,
			"ambiguous commit replay failed");
		require(scalar(db,
			       "SELECT COUNT(*) FROM player_death_conflict_evidence WHERE save_revision=190") ==
				"1",
			"ambiguous commit duplicate");
		std::cout
			<< "PASS: lost COMMIT reply -> unknown -> reconnect -> identical replay, one record\n";
		mysql_close(db);
		mysql_library_end();
		return 0;
	}
	execute(db, "INSERT INTO accounts(account_name) VALUES ('conflict_probe')");
	execute(db,
		"INSERT INTO player_data(pid,name,account_name,save_revision,wallet_revision,copper,silver,gold,platinum) VALUES (1,'Probe','conflict_probe',5,7,11,12,13,14),(2,'Other','conflict_probe',2,0,0,0,0,0)");
	execute(db,
		"INSERT INTO player_items(id,pid,vnum,obj_uid,container_id,extra_flags) VALUES (7101,1,501,201,NULL,9223372036854775808),(7102,1,501,301,7101,17),(7103,1,501,NULL,7101,0),(7104,1,501,301,NULL,18),(8101,2,501,999,NULL,19)");
	execute(db,
		"INSERT INTO player_item_affects(id,item_id,location,modifier) VALUES (7201,7101,7,-40),(7202,7102,NULL,0),(8201,8101,1,99)");
	execute(db,
		"INSERT INTO player_item_extra_descr(id,item_id,keyword,description) VALUES (7301,7101,'sigil',CONVERT(0x6d61726b00275cc3a9f09f9880 USING utf8mb4)),(7302,7102,'empty',''),(7303,7103,'null',NULL),(8301,8101,'private','foreign secret')");
	execute(db,
		"INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES (1,1,5),(1,2,9)");
	execute(db,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,item_revision,vnum,state) VALUES (201,201,NULL,1,1,3,501,1),(203,201,201,1,1,1,501,1),(300,300,NULL,1,2,7,501,1),(301,300,300,1,2,18446744073709551615,501,1),(302,300,300,1,2,8,501,1),(999,999,NULL,1,2,9,501,1)");
	if (argc > 1 && std::string(argv[1]).starts_with("--terminal-"))
	{
		execute(db,
			"UPDATE player_data SET copper=0,silver=0,gold=0,platinum=0 WHERE pid=1");
		execute(db,
			"INSERT INTO player_pets(id,owner_pid,mob_vnum,pet_uid) VALUES (9101,1,100,5001)");
		execute(db,
			"INSERT INTO player_pet_items(id,pet_id,vnum,obj_uid) VALUES (9201,9101,501,5002)");
		execute(db,
			"INSERT INTO player_pet_item_affects(item_id,location,modifier) VALUES (9201,3,-21)");
		execute(db,
			"INSERT INTO player_pet_item_extra_descr(item_id,keyword,description) VALUES (9201,'pet sigil',NULL)");
		terminal_tests(db, argv[1]);
		mysql_close(db);
		mysql_library_end();
		return 0;
	}
	const auto before = source_state(db);
	auto ordinary = player_snapshot_repository_apply(db, death);
	require(ordinary.outcome == player_save_apply_outcome::terminal_failure &&
			ordinary.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH,
		"fixture no longer reproduces death blocker");
	require(source_state(db) == before, "blocked death changed source");
	auto retained = player_death_conflict_retain(db, death);
	require(retained.outcome == outcome::retained && retained.source_revision == 5,
		"retain failed error=" + std::to_string(retained.error_code));
	require(source_state(db) == before,
		"retention changed inventory, wallet, custody or terminal revision");
	require(scalar(db, "SELECT COUNT(*) FROM player_death_disposition") == "0",
		"retention masqueraded as death save");
	player_snapshot readback = {};
	require(player_death_conflict_read(db, 1, death.death->operation_id, &readback).outcome ==
			outcome::read,
		"read own record");
	const auto frozen = encoded(readback);
	const auto &evidence = *readback.death->conflict_evidence;
	require(evidence.player_items.rows.size() == 4 &&
			evidence.player_item_affects.rows.size() == 2 &&
			evidence.player_item_extra_descr.rows.size() == 3,
		"payload or auxiliaries missing / foreign payload leaked");
	require(cell(evidence.player_items, 0, "extra_flags") == "9223372036854775808" &&
			!cell(evidence.player_items, 2, "obj_uid"),
		"flags or NULL UID lost");
	require(cell(evidence.player_items, 1, "obj_uid") == "301" &&
			cell(evidence.player_items, 3, "obj_uid") == "301",
		"duplicate evidence normalized");
	require(cell(evidence.player_items, 1, "container_id") == "7101" &&
			!cell(evidence.player_items, 3, "container_id"),
		"topology lost");
	const char special[] = "mark\0'\\\xc3\xa9\xf0\x9f\x98\x80";
	require(cell(evidence.player_item_extra_descr, 0, "description") ==
			std::string(special, sizeof(special) - 1),
		"raw byte/charset/NUL fidelity lost");
	require(cell(evidence.player_item_extra_descr, 1, "description") == "" &&
			!cell(evidence.player_item_extra_descr, 2, "description"),
		"NULL and empty conflated");
	require(!cell(evidence.player_item_affects, 1, "location") &&
			cell(evidence.player_item_affects, 0, "modifier") == "-40",
		"affect evidence lost");
	std::set<std::string> uids;
	for (size_t i = 0; i < evidence.item_current_owner.rows.size(); ++i)
		uids.insert(*cell(evidence.item_current_owner, i, "item_uid"));
	require(uids == std::set<std::string>{ "201", "203", "300", "301", "302" },
		"custody closure incomplete / unrelated foreign custody leaked");
	require(evidence.item_owner_revision.rows.size() == 2, "owner fence evidence incomplete");
	require(readback.death->wallet_before == death.death->wallet_before &&
			readback.items.empty(),
		"wallet or non-loadable boundary changed");
	player_snapshot sentinel = {};
	sentinel.pid = 999;
	require(player_death_conflict_read(db, 2, death.death->operation_id, &sentinel).outcome ==
				outcome::not_found &&
			sentinel.pid == 999,
		"cross-player detail leak/output mutation");
	std::vector<player_death_conflict_case> cases;
	require(player_death_conflict_list(db, 2, 0, &cases).outcome == outcome::read &&
			cases.empty(),
		"cross-player list leak");
	auto changed = death;
	changed.death->wallet_revision++;
	unchanged_failure(db, changed, outcome::identity_conflict);
	changed = death;
	changed.death->operation_id.bytes[1] = 1;
	unchanged_failure(db, changed, outcome::identity_conflict);
	changed = death;
	changed.death->operation_id.bytes[1] = 2;
	changed.death->corpse.front().object_uid = 8888;
	unchanged_failure(db, changed, outcome::identity_conflict);
	changed = fresh(1);
	changed.revision = 5;
	unchanged_failure(db, changed, outcome::stale_revision);
	changed = fresh(1);
	changed.death->corpse.clear();
	unchanged_failure(db, changed, outcome::failed);
	execute(db,
		"UPDATE player_item_extra_descr SET description='new observation' WHERE id=7301");
	execute(db,
		"UPDATE item_current_owner SET item_revision=item_revision+1 WHERE item_uid=300");
	const auto later = source_state(db);
	{
		auto control = connect();
		{
			economic_sql_lifecycle_guard maintenance;
			require(economic_sql_lifecycle_guard::acquire_maintenance(
					control, &maintenance) == 0,
				"maintenance acquire");
			require(player_death_conflict_retain(db, fresh(2)).error_code == EPERM,
				"retention raced the maintenance fence");
		}
		mysql_close(control);
	}
	require(source_state(db) == later, "maintenance refusal changed source");
	require(player_death_conflict_retain(db, death).outcome == outcome::already_retained,
		"replay failed after source changed");
	require(player_death_conflict_read(db, 1, death.death->operation_id, &readback).outcome ==
				outcome::read &&
			encoded(readback) == frozen,
		"replay recaptured/overwrote first observations");
	require(source_state(db) == later, "replay mutated source");
	execute(db, "START TRANSACTION");
	execute(db, "UPDATE player_data SET copper=99 WHERE pid=1");
	require(player_death_conflict_retain(db, fresh(2)).error_code == EBUSY &&
			(db->server_status & SERVER_STATUS_IN_TRANS),
		"borrowed caller transaction was committed/rolled back");
	execute(db, "ROLLBACK");
	require(source_state(db) == later, "caller rollback");
	execute(db, "SET autocommit=0");
	require(player_death_conflict_retain(db, fresh(2)).error_code == EBUSY,
		"autocommit-off accepted");
	execute(db, "SET autocommit=1");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	require(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect), "enable reconnect");
	require(player_death_conflict_retain(db, fresh(2)).error_code == EINVAL,
		"automatic reconnect accepted");
	reconnect = false;
	require(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect), "disable reconnect");
	execute(db,
		"CREATE TRIGGER death_conflict_fail AFTER INSERT ON player_death_conflict_evidence FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic late failure'");
	unchanged_failure(db, fresh(2), outcome::failed);
	execute(db, "DROP TRIGGER death_conflict_fail");
	require(player_death_conflict_retain(db, fresh(2)).outcome == outcome::retained,
		"subsequent write after failed insert");
	const auto payload_hex = scalar(
		db,
		"SELECT HEX(payload) FROM player_death_conflict_evidence WHERE save_revision=7");
	execute(db,
		"UPDATE player_death_conflict_evidence SET payload=CONCAT(payload,UNHEX('00')) WHERE save_revision=7");
	require(player_death_conflict_read(db, 1, death.death->operation_id, &sentinel).outcome ==
				outcome::failed &&
			sentinel.pid == 999,
		"corrupt payload published");
	unchanged_failure(db, death, outcome::failed);
	execute(db, "UPDATE player_death_conflict_evidence SET payload=UNHEX('" + payload_hex +
			    "') WHERE save_revision=7");
	// Both bounds fail before an archive or authoritative source mutation.
	std::string insert =
		"INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES ";
	for (unsigned i = 0; i < 80; ++i)
		insert += (i ? "," : "") + std::string("(7101,'oversize_") + std::to_string(i) +
			  "',REPEAT('x',65000))";
	execute(db, insert);
	unchanged_failure(db, fresh(3), outcome::failed);
	execute(db, "DELETE FROM player_item_extra_descr WHERE keyword LIKE 'oversize_%'");
	for (unsigned chunk = 0; chunk < 9; ++chunk)
	{
		insert = "INSERT INTO player_item_affects(item_id,location,modifier) VALUES ";
		for (unsigned i = 0; i < 1000; ++i)
			insert += (i ? "," : "") + std::string("(7101,99,") +
				  std::to_string(chunk * 1000 + i) + ")";
		execute(db, insert);
	}
	unchanged_failure(db, fresh(3), outcome::failed);
	execute(db, "DELETE FROM player_item_affects WHERE location=99");
	for (unsigned i = 3; i < 30; ++i)
		require(player_death_conflict_retain(db, fresh(i)).outcome == outcome::retained,
			"pagination seed");
	std::set<std::string> operations;
	player_revision_t cursor = 0;
	for (;;)
	{
		require(player_death_conflict_list(db, 1, cursor, &cases).outcome ==
					outcome::read &&
				cases.size() <= PLAYER_DEATH_CONFLICT_LIST_LIMIT,
			"list page");
		if (cases.empty())
			break;
		for (const auto &c : cases)
		{
			require(c.save_revision > cursor &&
					operations
						.insert(std::string(
							reinterpret_cast<const char *>(
								c.operation_id.bytes.data()),
							c.operation_id.bytes.size()))
						.second,
				"duplicate/out-of-order page");
			cursor = c.save_revision;
		}
	}
	require(std::to_string(operations.size()) ==
			scalar(db,
			       "SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=1"),
		"pagination omitted cases");
	const auto blocked = player_snapshot_repository_apply(db, death);
	require(blocked.outcome == player_save_apply_outcome::terminal_failure &&
			blocked.error_code == EBUSY,
		"archive-only case bypassed the ordinary checkpoint fence");
	require(source_state(db) == later, "final authoritative source drift");
	require(scalar(db,
		       "SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL") ==
			"0",
		"accounting activated");
	mysql_close(db);
	db = connect();
	require(player_death_conflict_read(db, 1, death.death->operation_id, &readback).outcome ==
				outcome::read &&
			encoded(readback) == frozen,
		"cold reconnect lost evidence");
	mysql_close(db);
	mysql_library_end();
	std::cout
		<< "PASS: raw evidence, auxiliaries, custody closure, immutable replay, collision/privacy guards, rollback, byte/row bounds, pagination and cold reconnect; death release still blocked\n";
}
