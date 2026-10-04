#include "core/defines.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_repository.h"
#include "player/player_snapshot_codec.h"
#include "player/player_save_journal.h"
#include "player/player_death_conflict_repository.h"
#include "sql/sql_pool.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

extern "C" MYSQL *__real_sql_pool_acquire();
extern "C" void __real_sql_pool_release(MYSQL *);

// Real SQL repository, pool, journal, codec, conflict and lifecycle owners.
// The fixture factory is configured for an externally supplied private schema.
// Fault observers delegate actual C-client work, except explicitly refused
// ROLLBACK/update replies. No SQL outcome is inferred from the observer alone.
namespace
{
constexpr int base_pid = 68850;
constexpr uint64_t first_uid = 9000000000000688501ULL;
uint64_t item_uid = first_uid, pet_uid = first_uid + 1;
constexpr player_component_mask_t receipt_components =
	PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS |
	PLAYER_COMPONENT_TROPHIES;
enum class phase
{
	none,
	start,
	dml,
	commit_readback,
	rollback_failure,
	replacement_null,
	description,
	pet_result,
	conflict_rollback,
	retained_start,
	retained_insert,
	retained_insert_rollback,
	retained_commit_readback
};
thread_local phase mode = phase::none;
thread_local bool observing = false, arm_next = false, allocation_hit = false, phase_hit = false,
		  resource_tracking = false, native_query_active = false,
		  retained_record_query = false;
thread_local MYSQL *synthetic_error = nullptr;
thread_local unsigned synthetic_code = 0, starts = 0, dmls = 0, commits = 0, rollback_attempts = 0,
		      rollback_successes = 0;
thread_local std::array<void *, 4> buffers{};
thread_local size_t buffer_count = 0;
thread_local MYSQL_RES *pet_result = nullptr;
thread_local bool pet_query = false, readback_query = false;
MYSQL *observer = nullptr, *direct = nullptr, *lease = nullptr;
unsigned long original_session = 0, closed_original_session = 0, replacement_session = 0;
unsigned acquired = 0, released = 0, replaced = 0, discarded = 0, foreign_release = 0,
	 factories = 0;
bool factory_failure = false;
int pid = 0;
thread_local unsigned evidence_inserts = 0, retained_readbacks = 0, event_sequence = 0,
		      first_rollback_event = 0, second_start_event = 0, second_rollback_event = 0,
		      retained_fence_release_event = 0, retained_fence_release_count = 0;
thread_local unsigned long first_rollback_session = 0, second_start_session = 0,
			   second_rollback_session = 0, retained_fence_session = 0;
bool release_idle = false;
bool retained_mode() noexcept
{
	return mode == phase::retained_start || mode == phase::retained_insert ||
	       mode == phase::retained_insert_rollback || mode == phase::retained_commit_readback;
}

void require(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}
const char *env(const char *name)
{
	const auto *value = std::getenv(name);
	require(value && *value, "required disposable environment missing");
	return value;
}
void guard()
{
	require(std::strcmp(env("TEST_DB_DISPOSABLE"), "1") == 0 &&
			std::strcmp(env("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1") == 0,
		"both disposable markers required");
	require(std::strcmp(env("DB_HOST"), "127.0.0.1") == 0 && !std::getenv("DB_SOCKET") &&
			!std::getenv("TEST_DB_SOCKET"),
		"loopback TCP only");
	const std::string schema = env("DB_NAME"), prefix = "economic_schema_test_se_";
	require(schema.starts_with(prefix) && schema.size() == prefix.size() + 8 &&
			schema.find_first_not_of("0123456789abcdef", prefix.size()) ==
				std::string::npos &&
			schema.size() + std::strlen("duris.player.death.restitution.") <= 64,
		"exact bounded private schema");
	require(env("DB_ALLOWED_TARGETS") == "127.0.0.1/" + schema, "explicit target allowlist");
}
MYSQL *connect(unsigned long flags = 0)
{
	guard();
	char *end = nullptr;
	const auto port = std::strtoul(env("DB_PORT"), &end, 10);
	require(end && !*end && port > 0 && port <= 65535, "explicit bounded port");
	MYSQL *connection = mysql_init(nullptr);
	require(connection, "mysql_init");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	unsigned timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	require(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
			!mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) &&
			!mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout) &&
			!mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout) &&
			!mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol),
		"real configured factory options");
	require(mysql_real_connect(connection, "127.0.0.1", env("DB_USER"), env("DB_PASSWD"),
				   env("DB_NAME"), static_cast<unsigned>(port), nullptr, flags),
		"guarded native connect");
	require(!mysql_set_character_set(connection, "utf8mb4"), "utf8mb4 native session");
	return connection;
}
void execute(MYSQL *connection, const std::string &sql)
{
	require(mysql_real_query(connection, sql.data(), sql.size()) == 0,
		"fixture SQL statement failed (sanitized)");
}
using table = std::vector<std::vector<std::string>>;
table rows(MYSQL *connection, const std::string &sql)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), "fixture actual row result");
	table values;
	while (auto row = mysql_fetch_row(result.get()))
	{
		const auto *lengths = mysql_fetch_lengths(result.get());
		require(lengths, "actual result lengths");
		std::vector<std::string> cells;
		for (unsigned index = 0; index < mysql_num_fields(result.get()); ++index)
			cells.emplace_back(row[index] ? std::string(row[index], lengths[index]) :
							"<NULL>");
		values.push_back(std::move(cells));
	}
	return values;
}
uint64_t scalar(MYSQL *connection, const std::string &sql)
{
	const auto value = rows(connection, sql);
	require(value.size() == 1 && value[0].size() == 1, "one real scalar");
	return std::stoull(value[0][0]);
}
bool idle(MYSQL *connection)
{
	// SELECT1 refreshes actual protocol bits on both engines. This does not
	// replace a successful original ROLLBACK as cleanup authority.
	require(scalar(connection, "SELECT 1") == 1, "protocol status refresh");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) == 0,
		"actual reconnect readback");
	return !(connection->server_status & SERVER_STATUS_IN_TRANS) &&
	       (connection->server_status & SERVER_STATUS_AUTOCOMMIT) && !reconnect;
}
std::vector<uint8_t> encode(const player_snapshot &snapshot)
{
	std::vector<uint8_t> bytes;
	require(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok,
		"real canonical request codec");
	return bytes;
}
std::string hex(const uint8_t *bytes, size_t count)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value;
	value.reserve(count * 2);
	for (size_t i = 0; i < count; ++i)
	{
		value.push_back(digits[bytes[i] >> 4]);
		value.push_back(digits[bytes[i] & 15]);
	}
	return value;
}
std::vector<uint8_t> raw_journal(const std::string &directory)
{
	std::ifstream stream(directory + "/player-save.journal", std::ios::binary);
	require(bool(stream), "actual durable journal file");
	return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
}
std::vector<table> protected_rows()
{
	const auto p = std::to_string(pid);
	return {
		rows(observer, "SELECT * FROM player_data WHERE pid=" + p),
		rows(observer, "SELECT * FROM player_items WHERE pid=" + p + " ORDER BY id"),
		rows(observer,
		     "SELECT a.* FROM player_item_affects a JOIN player_items i ON i.id=a.item_id WHERE i.pid=" +
			     p + " ORDER BY a.id"),
		rows(observer,
		     "SELECT e.* FROM player_item_extra_descr e JOIN player_items i ON i.id=e.item_id WHERE i.pid=" +
			     p + " ORDER BY e.id"),
		rows(observer,
		     "SELECT s.* FROM player_item_runtime_state s JOIN player_items i ON i.id=s.item_id WHERE i.pid=" +
			     p + " ORDER BY s.item_id"),
		rows(observer,
		     "SELECT * FROM item_current_owner WHERE item_uid=" + std::to_string(item_uid)),
		rows(observer, "SELECT * FROM item_owner_revision WHERE owner_id=" + p),
		rows(observer, "SELECT * FROM player_pets WHERE owner_pid=" + p + " ORDER BY id"),
		rows(observer,
		     "SELECT i.* FROM player_pet_items i JOIN player_pets p ON p.id=i.pet_id WHERE p.owner_pid=" +
			     p + " ORDER BY i.id"),
		rows(observer, "SELECT * FROM player_craft_progression WHERE pid=" + p),
		rows(observer,
		     "SELECT * FROM critical_operation_inbox WHERE operation_id=(SELECT operation_id FROM player_craft_progression WHERE pid=" +
			     p + " LIMIT 1)")
	};
}
player_snapshot request(bool receipt = true)
{
	player_snapshot snapshot{};
	snapshot.pid = pid;
	snapshot.revision = 2;
	snapshot.components = receipt ? receipt_components : PLAYER_COMPONENT_STATUS;
	snapshot.schema_version = receipt ? PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION :
					    PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 22800;
	snapshot.encoded_size_bound = 32768;
	snapshot.status_integers.push_back({ player_status_field::level, 42, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name,
					    "SyntheticSqlException" + std::to_string(pid) });
	if (receipt)
	{
		player_craft_receipt_snapshot proof{};
		proof.operation_id.bytes[0] = 0x73;
		proof.operation_id.bytes[1] = static_cast<uint8_t>(pid - base_pid);
		proof.discipline = 2;
		proof.experience = 61;
		snapshot.craft_receipts.push_back(proof);
	}
	(void)encode(snapshot);
	return snapshot;
}
void seed(const player_snapshot &snapshot)
{
	require(scalar(observer,
		       "SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid)) == 0,
		"fresh PID collision refusal");
	execute(observer, "INSERT INTO accounts(account_name) VALUES('SqlExceptionFixture" +
				  std::to_string(pid) + "')");
	execute(observer,
		"INSERT INTO player_data(pid,name,account_name,save_revision,wallet_revision,level,copper,silver,gold,platinum) VALUES(" +
			std::to_string(pid) + ",'SyntheticSqlException" + std::to_string(pid) +
			"','SqlExceptionFixture" + std::to_string(pid) + "',1,1,10,0,0,0,0)");
	if (!snapshot.craft_receipts.empty())
	{
		const auto &proof = snapshot.craft_receipts[0];
		const auto id =
			hex(proof.operation_id.bytes.data(), proof.operation_id.bytes.size());
		execute(observer,
			"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('" +
				id +
				"'),UNHEX(REPEAT('00',32)),UNHEX(REPEAT('00',32)),5,2,10,1,X'')");
		execute(observer,
			"INSERT INTO player_craft_progression(operation_id,pid,discipline,experience) VALUES(UNHEX('" +
				id + "')," + std::to_string(pid) + ",2,61)");
	}
}
void arm(phase selected)
{
	mode = selected;
	observing = true;
	arm_next = allocation_hit = phase_hit = resource_tracking = false;
	synthetic_error = nullptr;
	synthetic_code = starts = dmls = commits = rollback_attempts = rollback_successes = 0;
	evidence_inserts = retained_readbacks = event_sequence = first_rollback_event =
		second_start_event = second_rollback_event = retained_fence_release_event =
			retained_fence_release_count = 0;
	first_rollback_session = second_start_session = second_rollback_session =
		retained_fence_session = 0;
	release_idle = false;
	buffers.fill(nullptr);
	buffer_count = 0;
	pet_result = nullptr;
}
void disarm()
{
	observing = arm_next = resource_tracking = false;
	mode = phase::none;
	synthetic_error = nullptr;
}
struct application
{
	player_save_apply_result result{};
	bool escaped = false;
};
application call(const player_snapshot &snapshot, bool pooled, bool conflict = false)
{
	application value;
	try
	{
		value.result =
			conflict ? player_death_conflict_apply_from_pool(snapshot, nullptr) :
			pooled	 ? player_snapshot_repository_apply_from_pool(snapshot, nullptr) :
				   player_snapshot_repository_apply(direct, snapshot);
	}
	catch (const std::bad_alloc &)
	{
		value.escaped = true;
	}
	disarm();
	return value;
}
void assert_frame(const std::string &directory, const std::vector<uint8_t> &original)
{
	require(raw_journal(directory) == original && !player_save_journal_pid_quarantined(pid),
		"unresolved exact original raw frame retained without fabricated ACK/quarantine");
}
void healthy()
{
	const int saved = pid;
	pid += 100;
	auto snapshot = request(false);
	seed(snapshot);
	const auto result = player_snapshot_repository_apply_from_pool(snapshot, nullptr);
	require(result.outcome == player_save_apply_outcome::applied &&
			result.durable_revision == 2 && sql_pool_in_use() == 0,
		"independent PID progress with zero native pool leases");
	pid = saved;
}
void pool_ready()
{
	require(sql_pool_init(1) == 0 && sql_pool_in_use() == 0 && sql_pool_available() == 1,
		"real one-slot pool init");
}
void calibration()
{
	require(idle(direct), "direct session initially idle/autocommit/reconnectOFF");
	execute(direct, "START TRANSACTION");
	require(!idle(direct), "protocol IN_TRANS calibration");
	execute(direct, "ROLLBACK");
	require(idle(direct), "real rollback calibrates idle session");
}
player_snapshot death_request()
{
	auto snapshot = request(false);
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.death.emplace();
	auto &death = *snapshot.death;
	death.operation_id.bytes[0] = 0x74;
	death.operation_id.bytes[1] = static_cast<uint8_t>(pid - base_pid);
	death.corpse_room_vnum = 22800;
	death.wallet_revision = 1;
	snapshot.recipes_are_external = true;
	for (auto field : { player_status_field::copper, player_status_field::silver,
			    player_status_field::gold, player_status_field::platinum })
		snapshot.status_integers.push_back({ field, 0, 0, false });
	player_item_snapshot corpse{};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = item_uid + 5;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = pid;
	corpse.values[CORPSE_SAVEID] = 42;
	death.corpse.push_back(corpse);
	player_item_snapshot item{};
	item.parent_index = 0;
	item.object_uid = item_uid;
	item.vnum = 48;
	item.type = ITEM_CONTAINER;
	death.corpse.push_back(item);
	death.custody.push_back({ { item_uid, item_uid, 0, 1, 48, item_custody_state::active },
				  { item_owner_type::player, static_cast<uint64_t>(pid), 0 },
				  1 });
	(void)encode(snapshot);
	return snapshot;
}
void custody_seed(bool pet = false)
{
	const auto owner_type =
		static_cast<unsigned>(pet ? item_owner_type::pet : item_owner_type::player);
	const auto owner_id = pet ? pet_uid : static_cast<uint64_t>(pid);
	const auto context = pet ? pid : 0;
	execute(observer,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(" +
			std::to_string(owner_type) + ',' + std::to_string(owner_id) + ',' +
			std::to_string(context) + ",1)");
	execute(observer,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
			std::to_string(item_uid) + ',' + std::to_string(item_uid) + ",NULL," +
			std::to_string(owner_type) + ',' + std::to_string(owner_id) + ',' +
			std::to_string(context) + ",1,48,1)");
}
auto witness_key(const persistence_custody_witness &value)
{
	return std::tuple(value.item_uid, value.expected_root, value.expected_parent,
			  value.observed_root, value.observed_parent, value.observed_item_revision,
			  value.source_line, value.expected_vnum, value.observed_vnum,
			  value.expected_slot, value.observed_slot, value.expected_present,
			  value.observed_present);
}
std::vector<table> terminal_rows(const player_snapshot &snapshot)
{
	const auto p = std::to_string(snapshot.pid);
	const auto op = hex(snapshot.death->operation_id.bytes.data(),
			    snapshot.death->operation_id.bytes.size());
	return {
		rows(observer,
		     "SELECT HEX(operation_id),pid,save_revision,source_revision,corpse_item_uid,HEX(request_hash),HEX(payload_hash),HEX(payload),recorded_at FROM player_death_conflict_evidence WHERE pid=" +
			     p + " ORDER BY save_revision"),
		rows(observer,
		     "SELECT pid,save_revision,HEX(operation_id),corpse_item_uid,corpse_room_vnum,wallet_revision,wallet_copper,wallet_silver,wallet_gold,wallet_platinum,wallet_pile_uid,HEX(payload),recorded_at FROM player_death_disposition WHERE pid=" +
			     p + " ORDER BY save_revision"),
		rows(observer, "SELECT * FROM player_death_custody WHERE pid=" + p +
				       " ORDER BY save_revision,item_uid"),
		rows(observer,
		     "SELECT HEX(operation_id),HEX(command_hash),HEX(keys_hash),command_type,schema_version,payload_version,status,HEX(result_payload) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			     op + "')"),
		rows(observer,
		     "SELECT HEX(offering_operation_id),recipient_pid,reward_index,amount,applied_at,created_at FROM quest_reward_xp_entitlement WHERE recipient_pid=" +
			     p + " ORDER BY offering_operation_id,reward_index"),
		rows(observer,
		     "SELECT pid,HEX(operation_id),effect_id,created_at FROM player_spell_effect_receipt WHERE pid=" +
			     p + " ORDER BY operation_id"),
		rows(observer,
		     "SELECT outbox_id,HEX(operation_id),event_index,destination,event_type,payload_version,HEX(payload),status,attempt_count,next_attempt_at FROM critical_outbox WHERE operation_id=UNHEX('" +
			     op + "') ORDER BY outbox_id")
	};
}
void verify_terminal(const player_snapshot &snapshot)
{
	const auto actual = terminal_rows(snapshot);
	const auto op = hex(snapshot.death->operation_id.bytes.data(),
			    snapshot.death->operation_id.bytes.size());
	require(actual[0].size() == 1 && actual[1].size() == 1 &&
			actual[0][0][0] == actual[1][0][2] && actual[0][0][0] == op &&
			actual[0][0][1] == std::to_string(snapshot.pid) &&
			actual[0][0][2] == std::to_string(snapshot.revision) &&
			actual[0][0][3] == "1" &&
			actual[1][0][1] == std::to_string(snapshot.revision) &&
			actual[0][0][7] == actual[1][0][11],
		"actual exact original-ID archive and terminal disposition share complete payload");
	player_snapshot retained;
	require(player_death_conflict_read(observer, snapshot.pid, snapshot.death->operation_id,
					   &retained)
					.outcome == player_death_conflict_outcome::read &&
			retained.death && retained.death->conflict_evidence &&
			retained.revision == snapshot.revision &&
			retained.death->operation_id.bytes == snapshot.death->operation_id.bytes,
		"actual hash-checked retained evidence readback");
	retained.schema_version = player_snapshot_death_request_schema(retained.schema_version);
	retained.death->conflict_evidence.reset();
	require(encode(retained) == encode(snapshot),
		"full canonical original request preserved in archive/disposition");
	require(scalar(observer, "SELECT save_revision FROM player_data WHERE pid=" +
					 std::to_string(snapshot.pid)) == snapshot.revision,
		"actual terminal SQL save revision");
}
void retained_case(const std::string &name, const std::string &directory)
{
	auto snapshot = death_request();
	const auto canonical = encode(snapshot);
	seed(snapshot);
	custody_seed();
	execute(observer, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(" +
				  std::to_string(pid) + ",49," + std::to_string(item_uid) + ')');
	const auto before = protected_rows();
	const auto before_terminal = terminal_rows(snapshot);
	pool_ready();
	require(player_save_journal_append(snapshot) == player_save_journal_result::ok,
		"actual original durable death request");
	const auto frame = raw_journal(directory);
	arm(phase::none);
	const auto baseline = call(snapshot, false);
	require(!baseline.escaped &&
			baseline.result.outcome == player_save_apply_outcome::terminal_failure &&
			baseline.result.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
			baseline.result.custody_diagnosis != player_save_custody_diagnosis::none &&
			rollback_successes == 1 && idle(direct) && before == protected_rows(),
		"calibrated actual ordinary custody mismatch plus confirmed original rollback");
	const auto selected = name == "retained_start_oom"  ? phase::retained_start :
			      name == "retained_insert_oom" ? phase::retained_insert :
			      name == "retained_insert_rollback_failure" ?
							      phase::retained_insert_rollback :
							      phase::retained_commit_readback;
	arm(selected);
	const auto blocked = call(snapshot, true, true);
	const auto after = protected_rows();
	const auto after_terminal = terminal_rows(snapshot);
	std::cout << "RETAINED phase_hit=" << phase_hit << " allocation_hit=" << allocation_hit
		  << " escaped=" << blocked.escaped << " starts=" << starts
		  << " first_rollback_event=" << first_rollback_event
		  << " first_rollback_session=" << first_rollback_session
		  << " second_start_event=" << second_start_event
		  << " second_start_session=" << second_start_session
		  << " rollback_attempts=" << rollback_attempts
		  << " confirmed_rollbacks=" << rollback_successes
		  << " second_rollback_event=" << second_rollback_event
		  << " second_rollback_session=" << second_rollback_session
		  << " fence_release_event=" << retained_fence_release_event
		  << " fence_release_count=" << retained_fence_release_count
		  << " fence_session=" << retained_fence_session
		  << " evidence_inserts=" << evidence_inserts << " real_commits=" << commits
		  << " replacement_session=" << replacement_session
		  << " readback_hits=" << retained_readbacks
		  << " original_session=" << original_session
		  << " closed_original=" << closed_original_session
		  << " outcome=" << static_cast<unsigned>(blocked.result.outcome)
		  << " error=" << blocked.result.error_code
		  << " diagnosis=" << static_cast<unsigned>(blocked.result.custody_diagnosis)
		  << " witness_exact="
		  << (witness_key(blocked.result.custody_witness) ==
		      witness_key(baseline.result.custody_witness))
		  << " durable=" << blocked.result.durable_revision
		  << " leases=" << sql_pool_in_use() << " released=" << released
		  << " discarded=" << discarded << " release_idle=" << release_idle << std::endl;
	require(phase_hit && allocation_hit && !blocked.escaped && starts == 2 &&
			first_rollback_event && second_start_event > first_rollback_event &&
			first_rollback_session == original_session &&
			second_start_session == original_session && rollback_attempts >= 1 &&
			encode(snapshot) == canonical,
		"real first rollback precedes faulted retained participant on original native session");
	require(sql_pool_in_use() == 0 && foreign_release == 0 &&
			(release_idle || closed_original_session == original_session),
		"lease returned only idle or original unsafe connection retired");
	const bool committed = selected == phase::retained_commit_readback;
	if (committed)
	{
		require(commits == 1 && replaced == 1 && replacement_session &&
				replacement_session != original_session &&
				retained_readbacks == 1 &&
				blocked.result.outcome ==
					player_save_apply_outcome::ambiguous_commit &&
				blocked.result.durable_revision < snapshot.revision,
			"actual hidden terminal COMMIT and distinct replacement readback OOM retain ambiguity");
		verify_terminal(snapshot);
	}
	else
	{
		require(commits == 0 && second_rollback_event > second_start_event &&
				retained_fence_release_event > second_rollback_event &&
				second_rollback_session == original_session &&
				retained_fence_session == original_session && after == before &&
				after_terminal == before_terminal &&
				blocked.result.durable_revision < snapshot.revision &&
				(blocked.result.outcome ==
					 player_save_apply_outcome::retryable_failure ||
				 blocked.result.outcome ==
					 player_save_apply_outcome::terminal_failure ||
				 blocked.result.outcome ==
					 player_save_apply_outcome::ambiguous_commit),
			"retained rollback attempt before writer fence release; no false receipt or durable projection");
		require(evidence_inserts == (selected == phase::retained_start ? 0U : 1U),
			"actual targeted retained DML boundary");
		if (selected == phase::retained_insert_rollback)
			require(rollback_successes == 1 && rollback_attempts == 2 &&
					discarded >= 1 &&
					closed_original_session == original_session &&
					blocked.result.error_code == baseline.result.error_code &&
					blocked.result.custody_diagnosis ==
						baseline.result.custody_diagnosis &&
					witness_key(blocked.result.custody_witness) ==
						witness_key(baseline.result.custody_witness),
				"failed second rollback retires original lease and preserves exact original10001 diagnosis/witness");
		else
			require(rollback_successes == 2 && release_idle,
				"confirmed second rollback permits clean original lease reuse");
	}
	assert_frame(directory, frame);
	if (committed)
		require(std::equal(before.begin() + 1, before.end(), after.begin() + 1) &&
				after_terminal[3] == before_terminal[3] &&
				after_terminal[4] == before_terminal[4] &&
				after_terminal[5] == before_terminal[5] &&
				after_terminal[6] == before_terminal[6],
			"actual retained terminal commit preserves current custody, item graph and unrelated typed receipt/inbox state");
	if (sql_pool_available())
	{
		MYSQL *fresh = __real_sql_pool_acquire();
		require(fresh && idle(fresh),
			"actual reborrowed slot idle/autocommit/reconnectOFF");
		__real_sql_pool_release(fresh);
	}
	arm(phase::none);
	const auto repaired = call(snapshot, true, true);
	require(!repaired.escaped &&
			(repaired.result.outcome == player_save_apply_outcome::applied ||
			 repaired.result.outcome == player_save_apply_outcome::already_applied) &&
			repaired.result.durable_revision == snapshot.revision &&
			repaired.result.error_code == 0 && sql_pool_in_use() == 0,
		"same exact operation/revision retry settles after fault removal");
	verify_terminal(snapshot);
	if (committed)
		require(protected_rows() == after && terminal_rows(snapshot) == after_terminal,
			"committed original immutable archive/disposition and projections never republished on retry");
	require(encode(snapshot) == canonical &&
			player_save_journal_worker_ack(snapshot, repaired.result.durable_revision,
						       nullptr) &&
			raw_journal(directory).empty(),
		"real original canonical journal ACK only after qualified exact retry");
	const auto settled = terminal_rows(snapshot);
	const auto repeated = call(snapshot, true, true);
	require(!repeated.escaped &&
			repeated.result.outcome == player_save_apply_outcome::already_applied &&
			repeated.result.durable_revision == snapshot.revision &&
			terminal_rows(snapshot) == settled,
		"repeated exact operation leaves single original archive/disposition unchanged");
	healthy();
}

} // namespace

void logit(const char *, const char *, ...) {}
MYSQL *sql_open_configured_connection(unsigned long flags)
{
	++factories;
	if (factory_failure)
	{
		factory_failure = false;
		return nullptr;
	}
	return connect(flags);
}
char *sql_escape_string(const char *value)
{
	require(observer, "escape needs actual supplied connection");
	const auto length = std::strlen(value);
	char *copy = static_cast<char *>(std::malloc(length * 2 + 1));
	if (copy)
		mysql_real_escape_string(observer, copy, value, length);
	return copy;
}
extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" int __real_mysql_query(MYSQL *, const char *);
extern "C" unsigned __real_mysql_errno(MYSQL *);
extern "C" MYSQL_RES *__real_mysql_store_result(MYSQL *);
extern "C" void __real_mysql_free_result(MYSQL_RES *);
extern "C" void __real_mysql_close(MYSQL *);
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real_malloc(size_t);
extern "C" void __real_free(void *);
extern "C" MYSQL *__real_sql_pool_acquire();
extern "C" void __real_sql_pool_release(MYSQL *);
extern "C" void __real_sql_pool_discard_connection(MYSQL *);
extern "C" MYSQL *__real_sql_pool_replace_connection(MYSQL *);
int observe_mysql_query(MYSQL *connection, const char *query, unsigned long length,
			bool via_mysql_query)
{
	if (native_query_active)
		return via_mysql_query ? __real_mysql_query(connection, query) :
					 __real_mysql_real_query(connection, query, length);
	const std::string_view text(query, length);
	if (observing)
	{
		pet_query = text.starts_with(
			"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),vnum FROM item_current_owner WHERE owner_type=");
		retained_record_query = text.starts_with(
			"SELECT operation_id,pid,save_revision,source_revision,corpse_item_uid,request_hash,payload_hash,");
		readback_query = text.starts_with("SELECT save_revision FROM player_data");
		if (retained_mode() && starts >= 2 &&
		    text == "SELECT RELEASE_LOCK('duris:economic_sql_currency_writers')")
		{
			++retained_fence_release_count;
			if (!retained_fence_release_event)
			{
				retained_fence_release_event = ++event_sequence;
				retained_fence_session = mysql_thread_id(connection);
			}
		}
		if (text == "ROLLBACK")
		{
			++rollback_attempts;
			if (retained_mode() && starts >= 2)
			{
				second_rollback_event = ++event_sequence;
				second_rollback_session = mysql_thread_id(connection);
			}
			if (mode == phase::retained_insert_rollback && rollback_attempts == 2)
			{
				synthetic_error = connection;
				synthetic_code = 2013;
				return 1;
			}
			if (mode == phase::rollback_failure || mode == phase::conflict_rollback)
			{
				phase_hit = true;
				synthetic_error = connection;
				synthetic_code = 2013;
				return 1;
			}
		}
		if (mode == phase::rollback_failure &&
		    text.starts_with("UPDATE player_data SET save_revision="))
		{
			synthetic_error = connection;
			synthetic_code = 1205;
			return 1;
		}
	}
	native_query_active = true;
	const int result = via_mysql_query ? __real_mysql_query(connection, query) :
					     __real_mysql_real_query(connection, query, length);
	native_query_active = false;
	if (observing && !result)
	{
		if (text == "START TRANSACTION")
		{
			++starts;
			if (retained_mode() && starts == 2 && rollback_successes == 1)
			{
				second_start_event = ++event_sequence;
				second_start_session = mysql_thread_id(connection);
				if (mode == phase::retained_start)
				{
					phase_hit = true;
					arm_next = true;
				}
			}
			if (mode == phase::start)
			{
				phase_hit = true;
				arm_next = true;
			}
		}
		if (text.starts_with("UPDATE player_data SET "))
		{
			++dmls;
			if (mode == phase::dml)
			{
				phase_hit = true;
				arm_next = true;
			}
		}
		if (text == "ROLLBACK")
		{
			++rollback_successes;
			if (retained_mode() && rollback_successes == 1)
			{
				first_rollback_event = ++event_sequence;
				first_rollback_session = mysql_thread_id(connection);
			}
		}
		if (retained_mode() &&
		    text.starts_with("INSERT INTO player_death_conflict_evidence"))
		{
			++evidence_inserts;
			if (mode == phase::retained_insert ||
			    mode == phase::retained_insert_rollback)
			{
				phase_hit = true;
				arm_next = true;
			}
		}
		if (text == "COMMIT")
		{
			++commits;
			if (mode == phase::commit_readback || mode == phase::replacement_null ||
			    mode == phase::retained_commit_readback)
			{
				phase_hit = true;
				synthetic_error = connection;
				synthetic_code = 2013;
				if (mode == phase::replacement_null)
					factory_failure = true;
				return 1; // The real server COMMIT succeeded; only its reply is hidden.
			}
		}
		if (mode == phase::description &&
		    text.starts_with("INSERT INTO player_item_runtime_state"))
			resource_tracking = true;
	}
	return result;
}
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long length)
{
	return observe_mysql_query(connection, query, length, false);
}
extern "C" int __wrap_mysql_query(MYSQL *connection, const char *query)
{
	return observe_mysql_query(connection, query,
				   static_cast<unsigned long>(std::strlen(query)), true);
}
extern "C" unsigned __wrap_mysql_errno(MYSQL *connection)
{
	if (connection == synthetic_error)
	{
		synthetic_error = nullptr;
		return synthetic_code;
	}
	return __real_mysql_errno(connection);
}
extern "C" MYSQL_RES *__wrap_mysql_store_result(MYSQL *connection)
{
	auto *result = __real_mysql_store_result(connection);
	if (observing && result)
	{
		if (mode == phase::pet_result && pet_query)
		{
			pet_result = result;
			phase_hit = true;
			arm_next = true;
		}
		if (replacement_session &&
		    ((mode == phase::commit_readback && readback_query) ||
		     (mode == phase::retained_commit_readback && retained_record_query)))
		{
			phase_hit = true;
			arm_next = true;
			if (mode == phase::retained_commit_readback)
				++retained_readbacks;
		}
	}
	return result;
}
extern "C" void __wrap_mysql_free_result(MYSQL_RES *result)
{
	if (result == pet_result)
		pet_result = nullptr;
	__real_mysql_free_result(result);
}
extern "C" void *__wrap_malloc(size_t bytes)
{
	void *value = __real_malloc(bytes);
	if (resource_tracking && value)
	{
		if (buffer_count < buffers.size())
			buffers[buffer_count++] = value;
		if (buffer_count == 2)
		{
			resource_tracking = false;
			phase_hit = true;
			arm_next = true;
		}
	}
	return value;
}
extern "C" void __wrap_free(void *value)
{
	for (auto &pointer : buffers)
		if (pointer == value)
			pointer = nullptr;
	__real_free(value);
}
extern "C" void *__wrap__Znwm(size_t bytes)
{
	if (arm_next)
	{
		arm_next = false;
		allocation_hit = true;
		throw std::bad_alloc();
	}
	return __real__Znwm(bytes);
}
extern "C" MYSQL *__wrap_sql_pool_acquire()
{
	auto *value = __real_sql_pool_acquire();
	if (value)
	{
		++acquired;
		lease = value;
		if (!original_session)
			original_session = mysql_thread_id(value);
	}
	return value;
}
extern "C" void __wrap_sql_pool_release(MYSQL *connection)
{
	if (connection)
	{
		++released;
		if (observing && retained_mode() && connection == lease)
		{
			using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
			flag reconnect = false;
			release_idle =
				!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
				!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect;
		}
		if (connection != lease)
			++foreign_release;
	}
	__real_sql_pool_release(connection);
	if (connection == lease)
		lease = nullptr;
}
extern "C" void __wrap_sql_pool_discard_connection(MYSQL *connection)
{
	++discarded;
	__real_sql_pool_discard_connection(connection);
}
extern "C" MYSQL *__wrap_sql_pool_replace_connection(MYSQL *connection)
{
	++replaced;
	const auto before = mysql_thread_id(connection);
	lease = nullptr;
	auto *value = __real_sql_pool_replace_connection(connection);
	if (value)
	{
		lease = value;
		replacement_session = mysql_thread_id(value);
		require(replacement_session != before, "real distinct replacement session");
	}
	return value;
}
extern "C" void __wrap_mysql_close(MYSQL *connection)
{
	if (connection && mysql_thread_id(connection) == original_session)
		closed_original_session = original_session;
	if (connection == lease)
		lease = nullptr;
	__real_mysql_close(connection);
}
int main(int argc, char **argv)
{
	bool library = false;
	int code = 0;
	try
	{
		require(argc == 3 && std::filesystem::path(argv[2]).is_absolute(),
			"named case and fresh absolute journal path required");
		const std::string name = argv[1], directory = argv[2];
		guard();
		require(mysql_library_init(0, nullptr, nullptr) == 0, "native client library init");
		library = true;
		observer = connect();
		direct = connect();
		calibration();
		require(scalar(observer,
			       "SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL") ==
					0 &&
				scalar(observer,
				       "SELECT COUNT(*) FROM economic_sql_global_activation") == 0,
			"fixture remains globally inactive");
		static const std::array<std::string_view, 4> names = {
			"retained_start_oom", "retained_insert_oom",
			"retained_insert_rollback_failure", "retained_commit_readback_oom"
		};
		const auto found = std::find(names.begin(), names.end(), name);
		require(found != names.end(), "known case only");
		pid = base_pid + static_cast<int>(found - names.begin());
		item_uid = first_uid + static_cast<uint64_t>(pid - base_pid) * 10;
		pet_uid = item_uid + 1;
		require(player_save_journal_init(directory.c_str()),
			"fresh private durable journal");
		retained_case(name, directory);
		require(sql_pool_in_use() == 0,
			"all actual leases returned before pool/library shutdown");
		std::cout << "PASS " << name << '\n';
	}
	catch (const std::exception &error)
	{
		disarm();
		code = 1;
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << " leases=" << sql_pool_in_use()
			  << " rollback_attempts=" << rollback_attempts
			  << " rollback_successes=" << rollback_successes
			  << " native_buffer_allocations=" << buffer_count
			  << " live_result=" << (pet_result != nullptr) << '\n';
	}
	// Cleanup is fixture-owned AFTER observations, never a manufactured proof
	// for production. It prevents a known old leaked lease hanging shutdown.
	if (lease)
	{
		__real_sql_pool_discard_connection(lease);
		__real_sql_pool_release(lease);
		lease = nullptr;
	}
	sql_pool_shutdown();
	player_save_journal_shutdown();
	if (direct)
		mysql_close(direct);
	if (observer)
		mysql_close(observer);
	if (library)
		mysql_library_end();
	return code;
}
