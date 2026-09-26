#include "persistence/critical_command_repository.h"
#include "persistence/player_death_restitution_command.h"

#include <mysql/mysql.h>
#include <openssl/sha.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
[[noreturn]] void fail(const std::string &message)
{
	std::cerr << "native restitution mysql check failed: " << message << '\n';
	std::exit(1);
}

void require(bool condition, const std::string &message)
{
	if (!condition)
		fail(message);
}

std::string hex_bytes(const uint8_t *bytes, size_t size)
{
	std::ostringstream output;
	output << std::hex << std::setfill('0');
	for (size_t index = 0; index < size; ++index)
		output << std::setw(2) << static_cast<unsigned int>(bytes[index]);
	return output.str();
}

template <typename Container> std::string hex_bytes(const Container &bytes)
{
	return hex_bytes(bytes.data(), bytes.size());
}

critical_operation_id operation_id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}

std::string operation_hex(const critical_operation_id &value)
{
	return hex_bytes(value.bytes);
}

void query_or_fail(MYSQL *connection, const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()) != 0)
		fail("SQL error " + std::to_string(mysql_errno(connection)) + ": " +
		     mysql_error(connection));
}

std::string scalar(MYSQL *connection, const std::string &sql)
{
	query_or_fail(connection, sql);
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		fail("missing result for scalar query: " + sql);
	MYSQL_ROW row = mysql_fetch_row(result);
	std::string value = row && row[0] ? row[0] : "";
	mysql_free_result(result);
	return value;
}

MYSQL *connect_database()
{
	MYSQL *connection = mysql_init(nullptr);
	require(connection != nullptr, "mysql_init");
	const char *host = std::getenv("DB_HOST");
	const char *port_text = std::getenv("DB_PORT");
	const char *user = std::getenv("DB_USER");
	const char *password = std::getenv("DB_PASSWD");
	const char *database = std::getenv("DB_NAME");
	const unsigned int port =
		port_text ? static_cast<unsigned int>(std::strtoul(port_text, nullptr, 10)) : 3306;
	require(mysql_real_connect(connection, host ? host : "127.0.0.1", user ? user : "root",
				   password ? password : std::getenv("MYSQL_PWD"),
				   database ? database : "duris_issue_331_test", port, nullptr,
				   0) != nullptr,
		std::string("mysql_real_connect: ") + mysql_error(connection));
	query_or_fail(connection, "SET time_zone='+00:00'");
	return connection;
}

void clear_database(MYSQL *connection)
{
	const char *statements[] = {
		"DELETE FROM player_death_restitution_runtime",
		"DELETE FROM player_death_restitution_delivery",
		"DELETE FROM player_death_restitution_item",
		"DELETE FROM player_death_restitution_receipt",
		"DELETE FROM item_ownership_ledger",
		"DELETE FROM player_item_affects",
		"DELETE FROM player_item_extra_descr",
		"DELETE FROM player_items",
		"DELETE FROM critical_outbox",
		"DELETE FROM critical_operation_inbox",
		"DELETE FROM artifact_domain_baseline",
		"DELETE FROM artifact_bind",
		"DELETE FROM artifacts",
		"DELETE FROM artifacts_mortal",
		"DELETE FROM artifact_domain_state",
		"DELETE FROM item_current_owner",
		"DELETE FROM player_death_custody",
		"DELETE FROM item_owner_revision",
		"DELETE FROM player_death_disposition",
		"DELETE FROM player_data",
	};
	for (const char *statement : statements)
		query_or_fail(connection, statement);
}

struct fixture
{
	player_death_restitution_plan plan;
	std::vector<uint8_t> death_payload;
};

fixture make_fixture(uint8_t operation_seed)
{
	fixture value = {};
	value.death_payload = { 0x42, 0x43, 0x44, 0x45, 0x46 };
	value.plan.source_pid = 42;
	value.plan.death_revision = 7;
	value.plan.recipient_pid = 43;
	value.plan.restitution_id = operation_id(operation_seed);
	value.plan.death_operation_id = operation_id(0x20);
	value.plan.evidence_digest.fill(static_cast<uint8_t>(operation_seed ^ 0xa5));
	SHA256(value.death_payload.data(), value.death_payload.size(),
	       value.plan.payload_digest.data());
	require(value.plan.evidence_digest != value.plan.payload_digest,
		"native fixture digests are not separated");
	value.plan.plan_digest.fill(static_cast<uint8_t>(operation_seed ^ 0x5a));
	value.plan.expected_recipient_save_revision = 1;
	value.plan.expected_source_owner_revision = 6;
	value.plan.expected_recipient_owner_revision = 0;
	value.plan.loss_epoch = 1700000000;
	value.plan.accepted_at_usec = 1700000000000000ULL;
	value.plan.actor = "native-mysql-test";
	value.plan.reason = "death restitution native integration";

	player_death_restitution_item_state state = {};
	state.item_uid = 1004;
	state.vnum = 104;
	state.equip_slot = -1;
	state.quantity = 2;
	state.weight = 17;
	state.cost = 913;
	state.timer = 321;
	state.extra_flags = 0x123456789ULL;
	state.wear_flags = 0x55;
	state.item_type = 2;
	state.values = { 11, 22, 33, 44, 55, 66, 77, 88 };
	state.string_present[0] = true;
	state.strings[0] = { 's', 'p', 'e', 'l', 'l', 'b', 'o', 'o', 'k' };
	state.string_present[1] = true;
	state.strings[1] = { 'a', 'r', 't', 'i', 'f', 'a', 'c', 't' };
	state.string_present[2] = true;
	state.strings[2] = { 'm', 'e', 't', 'a', 'd', 'a', 't', 'a' };
	state.bitvector_present[0] = true;
	state.bitvectors[0] = 0x1111;
	state.bitvector_present[1] = true;
	state.bitvectors[1] = 0x2222;
	state.material = 4;
	state.condition = 97;
	state.affects.push_back({ 7, -3 });
	state.extra_descriptions.push_back({ { 'r', 'u', 'n' }, { 'e', 's', 't' } });

	player_death_restitution_item item = {};
	require(player_death_restitution_item_state_encode(state, &item.metadata_payload),
		"encode artifact state");
	item.item_uid = state.item_uid;
	item.source_root_item_uid = state.item_uid;
	item.delivered_root_item_uid = state.item_uid;
	item.source_item_revision = 11;
	item.custody_item_revision = 10;
	item.expected_item_revision = 11;
	item.expected_owner_revision = 6;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = value.plan.source_pid;
	item.custody_owner_revision = 5;
	item.vnum = state.vnum;
	item.artifact_vnum = state.vnum;
	item.disposition = player_death_restitution_disposition::deliver;
	item.artifact_timing_evidence_present = true;
	item.artifact_loss_epoch = value.plan.loss_epoch;
	item.artifact_source_timer_epoch = 1700000456;
	item.artifact_usable_lifetime_seconds = 456;
	item.artifact_source_location_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_CORPSE;
	item.artifact_source_location = static_cast<int32_t>(value.plan.source_pid);
	item.artifact_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_UNIQUE;
	item.artifact_domain_present = true;
	item.artifact_domain_item_uid_present = true;
	item.artifact_domain_item_uid = item.item_uid;
	item.artifact_domain_item_revision = item.expected_item_revision;
	item.artifact_domain_revision = 4;
	item.artifact_bind_present = true;
	item.artifact_bind_owner_pid = static_cast<int32_t>(value.plan.source_pid);
	item.artifact_bind_timer_epoch = 654321;
	item.artifact_legacy_projection_mask = PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL;
	item.classification = "artifact_unique";
	item.note = "historical remaining lifetime";
	item.original_payload = { 0xde, 0xad, 0xbe, 0xef };
	SHA256(item.metadata_payload.data(), item.metadata_payload.size(),
	       item.metadata_digest.data());
	value.plan.items.push_back(std::move(item));
	require(player_death_restitution_plan_valid(value.plan), "native fixture plan valid");
	return value;
}

void seed_database(MYSQL *connection, const fixture &value)
{
	const std::string death_operation = operation_hex(value.plan.death_operation_id);
	query_or_fail(connection, "INSERT INTO player_data(pid,name,save_revision) VALUES"
				  "(43,'RestitutionRecipient',1)");
	query_or_fail(
		connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1,42,0,6)");
	query_or_fail(
		connection,
		"INSERT INTO player_death_disposition(pid,save_revision,operation_id,corpse_item_uid,"
		"corpse_room_vnum,wallet_revision,payload,recorded_at) VALUES(42,7,UNHEX('" +
			death_operation + "'),900,1234,9,UNHEX('" + hex_bytes(value.death_payload) +
			"'),FROM_UNIXTIME(1700000000))");
	query_or_fail(
		connection,
		"INSERT INTO player_death_custody(pid,save_revision,item_uid,root_item_uid,parent_item_uid,"
		"item_revision,vnum,state,owner_type,owner_id,owner_context_id,owner_revision) "
		"VALUES(42,7,1004,1004,0,10,104,1,1,42,0,5)");
	query_or_fail(
		connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
		"owner_context_id,vnum,item_revision,state) VALUES(1004,1004,NULL,1,42,0,104,11,3)");
	query_or_fail(connection, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(43,300,3000)");
	query_or_fail(
		connection,
		"INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,artifact_type,"
		"bind_owner_pid,bind_timer_epoch,item_uid,item_revision,revision) VALUES(104,1,5,42,"
		"1700000456,2,42,654321,1004,11,4)");
	query_or_fail(connection,
		      "INSERT INTO artifact_bind(vnum,owner_pid,timer) VALUES(104,42,654321)");
	query_or_fail(
		connection,
		"INSERT INTO artifacts_mortal(vnum,owned,location,timer,type,lastUpdate,locType) VALUES"
		"(104,'Y',42,FROM_UNIXTIME(1700000456),2,FROM_UNIXTIME(1700000456),5)");
}

critical_command build_command(const fixture &value)
{
	critical_command command = {};
	require(player_death_restitution_command_build(value.plan, &command),
		"build native command");
	return command;
}

// Rejection must preserve the state established immediately before apply.  Some
// fixtures deliberately alter one authority row to make the command stale.
void assert_unchanged_after_rejection(MYSQL *connection, uint32_t expected_owner_id,
				      uint64_t expected_domain_revision)
{
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=1004") == "0",
		"rejection did not project item");
	require(scalar(connection, "SELECT owner_id FROM item_current_owner WHERE item_uid=1004") ==
			std::to_string(expected_owner_id),
		"rejection changed owner identity");
	require(scalar(connection,
		       "SELECT item_revision FROM item_current_owner WHERE item_uid=1004") == "11",
		"rejection changed item revision");
	require(scalar(connection, "SELECT revision FROM artifact_domain_state WHERE vnum=104") ==
			std::to_string(expected_domain_revision),
		"rejection changed artifact domain");
	require(scalar(connection, "SELECT location FROM artifacts_mortal WHERE vnum=104") == "42",
		"rejection changed legacy artifact projection");
}

uint64_t positive_delivery(MYSQL *connection, const fixture &value, const critical_command &command)
{
	const critical_apply_result applied =
		critical_command_repository_apply(connection, command);
	require(applied.outcome == critical_apply_outcome::applied,
		"positive command was not applied (outcome=" +
			std::to_string(static_cast<int>(applied.outcome)) +
			", error=" + std::to_string(applied.error_code) + ")");
	require(applied.error_code == 0, "positive command returned an error");
	player_death_restitution_result decoded = {};
	require(applied.result_size > 0 &&
			player_death_restitution_command_decode_result(
				applied.result_payload.data(), applied.result_size, &decoded),
		"positive result payload was not decodable");
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=1004") == "1",
		"artifact player item was not projected");
	const std::string delivery_digest =
		scalar(connection,
		       "SELECT LOWER(HEX(metadata_digest)) FROM player_death_restitution_delivery "
		       "WHERE item_uid=1004");
	const std::string original_payload_digest = scalar(
		connection,
		"SELECT LOWER(SHA2(original_payload,256)) FROM player_death_restitution_delivery "
		"WHERE item_uid=1004");
	const std::string native_ist1_digest = scalar(
		connection, "SELECT LOWER(HEX(metadata_digest)) FROM player_death_restitution_item "
			    "WHERE item_uid=1004");
	require(scalar(connection,
		       "SELECT LEFT(HEX(metadata_payload),8) FROM player_death_restitution_item "
		       "WHERE item_uid=1004") == "49535431",
		"receipt item metadata payload is not native IST1");
	std::array<uint8_t, SHA256_DIGEST_LENGTH> expected_original_digest = {};
	SHA256(value.plan.items[0].original_payload.data(),
	       value.plan.items[0].original_payload.size(), expected_original_digest.data());
	const std::string expected_original_digest_hex = hex_bytes(expected_original_digest);
	const std::string expected_native_ist1_digest =
		hex_bytes(value.plan.items[0].metadata_digest);
	require(original_payload_digest == expected_original_digest_hex,
		"fixture original payload digest was not reproduced by SQL");
	require(native_ist1_digest == expected_native_ist1_digest,
		"fixture IST1 metadata digest was not reproduced by SQL");
	require(original_payload_digest != native_ist1_digest,
		"native fixture did not keep original and IST1 digests distinct");
	require(delivery_digest == original_payload_digest,
		"delivery metadata digest does not authenticate immutable original_payload");
	require(delivery_digest != native_ist1_digest,
		"delivery metadata digest reused the native IST1 metadata digest");
	require(scalar(connection,
		       "SELECT LOWER(HEX(metadata_digest))=LOWER(SHA2(metadata_payload,256)) "
		       "FROM player_death_restitution_item WHERE item_uid=1004") == "1",
		"receipt item metadata digest no longer authenticates IST1 metadata_payload");
	require(scalar(connection, "SELECT pid FROM player_items WHERE obj_uid=1004") == "43",
		"artifact projected to wrong player");
	require(scalar(connection, "SELECT HEX(name) FROM player_items WHERE obj_uid=1004") ==
			"7370656C6C626F6F6B",
		"spellbook metadata was not preserved");
	require(scalar(connection,
		       "SELECT HEX(short_descr) FROM player_items WHERE obj_uid=1004") ==
			"6172746966616374",
		"item description metadata was not preserved");
	require(scalar(connection, "SELECT extra_flags FROM player_items WHERE obj_uid=1004") ==
			std::to_string(0x123456789ULL),
		"item flags were not preserved");
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=3000") == "1",
		"current inventory was overwritten");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM player_item_affects WHERE item_id=(SELECT id FROM player_items WHERE obj_uid=1004)") ==
			"1",
		"item affects were not preserved");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM player_item_extra_descr WHERE item_id=(SELECT id FROM player_items WHERE obj_uid=1004)") ==
			"1",
		"item extra descriptions were not preserved");
	require(scalar(connection, "SELECT owner_id FROM item_current_owner WHERE item_uid=1004") ==
			"43",
		"current owner was not updated");
	require(scalar(connection,
		       "SELECT item_revision FROM item_current_owner WHERE item_uid=1004") == "12",
		"delivered item revision is wrong");
	require(scalar(connection, "SELECT revision FROM artifact_domain_state WHERE vnum=104") ==
			"5",
		"artifact domain revision was not fenced and advanced");
	require(scalar(connection, "SELECT location FROM artifact_domain_state WHERE vnum=104") ==
			"43",
		"artifact domain location is wrong");
	require(scalar(connection, "SELECT loc_type FROM artifact_domain_state WHERE vnum=104") ==
			"3",
		"artifact domain location type is wrong");
	const uint64_t delivered_timer = std::stoull(scalar(
		connection,
		"SELECT artifact_delivered_timer_epoch FROM player_death_restitution_item WHERE item_uid=1004"));
	const uint64_t delivered_domain_timer = std::stoull(
		scalar(connection, "SELECT timer_epoch FROM artifact_domain_state WHERE vnum=104"));
	require(decoded.delivery_epoch + 456 == delivered_timer &&
			delivered_timer == delivered_domain_timer,
		"receipt/domain timer disagree");
	require(decoded.delivery_epoch > 1700000000ULL,
		"delivered timer is not based on the current delivery epoch");
	require(scalar(connection,
		       "SELECT UNIX_TIMESTAMP(timer) FROM artifacts_mortal WHERE vnum=104") ==
			std::to_string(delivered_domain_timer),
		"legacy artifact timer does not match domain timer");
	require(scalar(connection,
		       "SELECT artifact_timing_basis FROM player_death_restitution_item WHERE item_uid=1004") ==
			"historical_loss_remainder",
		"historical timing basis was not audited");
	require(scalar(connection,
		       "SELECT artifact_usable_lifetime_seconds FROM player_death_restitution_item WHERE item_uid=1004") ==
			"456",
		"remaining lifetime was not preserved");
	require(scalar(connection, "SELECT owner_pid FROM artifact_bind WHERE vnum=104") == "42",
		"artifact bind projection was modified");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
			       operation_hex(command.operation_id) + "')") == "1",
		"restitution outbox event missing");
	require(scalar(connection,
		       "SELECT status FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			       operation_hex(command.operation_id) + "')") == "1",
		"inbox was not committed");
	return delivered_domain_timer;
}

struct delayed_apply
{
	critical_apply_result result = {};
	std::atomic<bool> finished = false;
};

void run_lock_delayed_delivery_epoch(MYSQL *connection)
{
	const fixture value = make_fixture(0x35);
	seed_database(connection, value);
	const critical_command command = build_command(value);
	MYSQL *locker = connect_database();
	query_or_fail(locker, "START TRANSACTION");
	require(scalar(locker, "SELECT save_revision FROM player_data WHERE pid=43 FOR UPDATE") ==
			"1",
		"lock-delay fixture did not hold the target validation row");

	delayed_apply applied;
	const auto started = std::chrono::steady_clock::now();
	std::thread worker(
		[&]
		{
			MYSQL *apply_connection = connect_database();
			applied.result =
				critical_command_repository_apply(apply_connection, command);
			mysql_close(apply_connection);
			applied.finished.store(true, std::memory_order_release);
		});

	// Keep the validation row locked long enough to cross multiple wall-clock
	// epochs. The apply must still be waiting here, before any projection commit.
	std::this_thread::sleep_for(std::chrono::milliseconds(2200));
	require(!applied.finished.load(std::memory_order_acquire),
		"apply bypassed the induced database lock delay");
	const uint64_t release_epoch =
		std::stoull(scalar(locker, "SELECT FLOOR(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(6)))"));
	query_or_fail(locker, "ROLLBACK");
	worker.join();
	mysql_close(locker);
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				     std::chrono::steady_clock::now() - started)
				     .count();
	require(elapsed >= 2000, "induced database lock delay was shorter than the test window");
	require(applied.result.outcome == critical_apply_outcome::applied &&
			applied.result.error_code == 0,
		"lock-delay command did not apply");
	player_death_restitution_result decoded = {};
	require(applied.result.result_size > 0 && player_death_restitution_command_decode_result(
							  applied.result.result_payload.data(),
							  applied.result.result_size, &decoded),
		"lock-delay result payload was not decodable");
	require(decoded.delivery_epoch >= release_epoch,
		"delivery epoch was captured before the blocking validation lock released");
	const uint64_t delivered_timer = std::stoull(scalar(
		connection,
		"SELECT artifact_delivered_timer_epoch FROM player_death_restitution_item WHERE item_uid=1004"));
	require(delivered_timer == decoded.delivery_epoch + 456,
		"lock-delay artifact lifetime was not based on post-lock delivery epoch");
}

void run_positive_and_replay(MYSQL *connection)
{
	const fixture value = make_fixture(0x30);
	seed_database(connection, value);
	const critical_command command = build_command(value);
	const uint64_t timer = positive_delivery(connection, value, command);
	require(timer > 1700000000ULL, "delivery timer was not based on current delivery epoch");
	const critical_apply_result replay = critical_command_repository_apply(connection, command);
	require(replay.outcome == critical_apply_outcome::already_applied,
		"replay was not idempotent");
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=1004") == "1",
		"replay duplicated original UID");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid=1004") ==
			"1",
		"replay duplicated delivery receipt");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
			       operation_hex(command.operation_id) + "')") == "1",
		"replay duplicated outbox event");
}

void run_stale_owner(MYSQL *connection)
{
	const fixture value = make_fixture(0x40);
	seed_database(connection, value);
	query_or_fail(connection, "UPDATE item_current_owner SET owner_id=99 WHERE item_uid=1004");
	const critical_apply_result result =
		critical_command_repository_apply(connection, build_command(value));
	require(result.outcome == critical_apply_outcome::terminal_failure &&
			result.error_code == ESTALE,
		"stale owner identity was accepted");
	assert_unchanged_after_rejection(connection, 99, 4);
}

void run_stale_domain_revision(MYSQL *connection)
{
	const fixture value = make_fixture(0x41);
	seed_database(connection, value);
	query_or_fail(connection, "UPDATE artifact_domain_state SET revision=5 WHERE vnum=104");
	const critical_apply_result result =
		critical_command_repository_apply(connection, build_command(value));
	require(result.outcome == critical_apply_outcome::terminal_failure &&
			result.error_code == ESTALE,
		"stale artifact domain revision was accepted");
	assert_unchanged_after_rejection(connection, 42, 5);
	require(scalar(connection,
		       "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			       operation_hex(value.plan.restitution_id) + "')") == "1",
		"stale command was not journaled");
}

void run_original_uid_guard(MYSQL *connection)
{
	const fixture value = make_fixture(0x42);
	seed_database(connection, value);
	query_or_fail(connection, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(43,104,1004)");
	const critical_apply_result result =
		critical_command_repository_apply(connection, build_command(value));
	require(result.outcome == critical_apply_outcome::terminal_failure &&
			result.error_code == EEXIST,
		"original UID collision was accepted");
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=1004") == "1",
		"original UID guard changed the pre-existing row");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid=1004") ==
			"0",
		"UID collision created a delivery row");
}

void run_atomic_rollback(MYSQL *connection)
{
	const fixture value = make_fixture(0x50);
	seed_database(connection, value);
	const std::string operation = operation_hex(value.plan.restitution_id);
	query_or_fail(
		connection,
		"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,parent_item_uid,"
		"from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
		"item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) VALUES(UNHEX('" +
			operation + "'),0,1004,1004,NULL,1,42,0,1,43,0,12,6,1,1,7,5)");
	const critical_apply_result result =
		critical_command_repository_apply(connection, build_command(value));
	require(result.outcome != critical_apply_outcome::applied &&
			result.outcome != critical_apply_outcome::already_applied,
		"forced ledger failure did not fail the command");
	require(scalar(connection, "SELECT COUNT(*) FROM player_items WHERE obj_uid=1004") == "0",
		"rollback left projected player item");
	require(scalar(connection, "SELECT COUNT(*) FROM player_death_restitution_receipt") == "0",
		"rollback left restitution receipt");
	require(scalar(connection, "SELECT owner_id FROM item_current_owner WHERE item_uid=1004") ==
			"42",
		"rollback left owner mutation");
	require(scalar(connection, "SELECT location FROM artifact_domain_state WHERE vnum=104") ==
			"42",
		"rollback left domain mutation");
	require(scalar(connection, "SELECT location FROM artifacts_mortal WHERE vnum=104") == "42",
		"rollback left legacy mutation");
	require(scalar(connection,
		       "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			       operation + "')") == "0",
		"rollback left inbox row");
}
}

int main()
{
	MYSQL *connection = connect_database();
	clear_database(connection);
	run_positive_and_replay(connection);
	clear_database(connection);
	run_lock_delayed_delivery_epoch(connection);
	clear_database(connection);
	run_stale_owner(connection);
	clear_database(connection);
	run_stale_domain_revision(connection);
	clear_database(connection);
	run_original_uid_guard(connection);
	clear_database(connection);
	run_atomic_rollback(connection);
	mysql_close(connection);
	std::cout << "native player death restitution MySQL transactional checks passed\n";
	return 0;
}
