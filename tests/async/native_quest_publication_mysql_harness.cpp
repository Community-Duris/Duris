#include "economy/item_transfer_accounting.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "player/player_snapshot_codec.h"
#include "persistence/quest_mobile_native_sql.h"
#include "world/object_template.h"
#include "world/quest_mobile_native.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

// Integration fixture only. The parent must provide a fresh full-schema disposable
// database. These seeded values do not authenticate an inbox receipt, source,
// admission, native lifetime, publication, ACK, or an activation decision.
namespace
{
constexpr uint32_t PID = 910001;
constexpr uint64_t MOBILE = 910002, UID = 910003, SAVE = 23;
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

void query(MYSQL *db, const std::string &sql)
{
	// Never print a query, credentials, connection errors, or private row values.
	assert(mysql_real_query(db, sql.data(), sql.size()) == 0);
}

uint64_t scalar(MYSQL *db, const std::string &sql)
{
	query(db, sql);
	result_ptr result(mysql_store_result(db), mysql_free_result);
	assert(result && mysql_num_rows(result.get()) == 1 && mysql_num_fields(result.get()) == 1);
	MYSQL_ROW row = mysql_fetch_row(result.get());
	assert(row && row[0]);
	char *end = nullptr;
	const auto value = std::strtoull(row[0], &end, 10);
	assert(end && !*end);
	return value;
}

critical_operation_id id(uint8_t value)
{
	critical_operation_id result{};
	result.bytes[0] = value;
	return result;
}

std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output;
	output.reserve(bytes.size() * 2);
	for (const auto value : bytes)
	{
		output.push_back(digits[value >> 4]);
		output.push_back(digits[value & 15]);
	}
	return output;
}

player_item_snapshot item()
{
	player_item_snapshot result{};
	result.object_uid = UID;
	result.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	result.vnum = 9001;
	result.string_mask = 15;
	result.name = "synthetic publication root";
	result.short_description = "a synthetic publication root";
	result.description = "A synthetic publication root is here.";
	result.timers[0] = 23;
	result.condition = 17;
	return result;
}

std::vector<uint8_t> forest(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> result;
	assert(player_item_snapshot_list_encode(items, &result) ==
	       player_snapshot_codec_result::ok);
	return result;
}

std::vector<uint8_t> image_bytes(const quest_mobile_native_image &image)
{
	std::vector<uint8_t> result;
	assert(quest_mobile_native_image_encode(image, &result) ==
	       player_snapshot_codec_result::ok);
	return result;
}

struct original_cut
{
	item_transfer_payload payload{};
	critical_command command{};
	critical_completion completion{};
	std::vector<player_item_snapshot> native_before, player_before, player_after;
	quest_mobile_native_image current;
	uint64_t current_destination = 0;
};

original_cut cut(bool acceptance, bool rejected, uint64_t receipt_destination,
		 uint64_t current_destination)
{
	original_cut result;
	auto &payload = result.payload;
	payload.from_owner = { acceptance ? item_owner_type::player :
					    item_owner_type::native_mobile,
			       acceptance ? PID : MOBILE, 0 };
	payload.to_owner = { acceptance ? item_owner_type::native_mobile :
					  item_owner_type::destruction,
			     acceptance ? MOBILE : 0, 0 };
	payload.reason = acceptance ? item_transfer_reason::quest_offering :
				      item_transfer_reason::quest_turnin;
	payload.reason_id = 9001;
	payload.expected_from_revision = acceptance ? 3 : 7;
	payload.expected_to_revision = acceptance ? 7 : 10;
	payload.multi_root = !acceptance;
	payload.selected_item_uid = payload.target_root_item_uid = acceptance ? UID : 0;
	payload.item_count = 1;
	payload.items[0] = { UID, UID, 0, 2, 9001, item_custody_state::active };
	const auto bytes = forest({ item() });
	payload.item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
	payload.native_mobile.present = true;
	payload.native_mobile.action = acceptance ? item_native_mobile_action::acceptance :
						    item_native_mobile_action::consumption;
	payload.native_mobile.final_giver_pid = PID;
	auto &reference = payload.native_mobile.reference;
	reference.mobile_instance_id = MOBILE;
	reference.birth_operation = id(1);
	reference.birth_source = { economic_source_kind::npc_generation, id(2), id(3), 4, 5 };
	reference.mobile_vnum = 9001;
	reference.reset_zone_vnum = 1;
	reference.provenance = quest_mobile_birth_provenance::reset;
	reference.mobile_revision = 5;
	reference.stock_revision = 7;
	if (acceptance)
	{
		result.player_before = { item() };
		assert(item_transfer_native_mobile_recovery_freeze(&payload, PID, SAVE,
								   forest(result.player_before)));
	}
	else
	{
		result.native_before = { item() };
		const std::array<uint64_t, 1> roots{ UID };
		assert(item_transfer_native_mobile_recovery_freeze(&payload, PID, SAVE, {}, roots));
	}
	assert(item_transfer_command_build_native_mobile_recovery(
		&result.command, id(6), payload, critical_source_site::command,
		critical_deadline_class::interactive));
	economic_source_event event{ economic_source_kind::quest_action, id(9), id(10), 1, 0 };
	assert(item_native_mobile_accounting_intent(
		       result.command, id(7), id(8), PID, acceptance ? nullptr : &event,
		       &result.command.accounting_intent) == economic_accounting_error::ok);
	result.command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	result.command.accepted_at_usec = 1;
	result.command.publication_required = true;
	assert(critical_command_envelope_valid(result.command));
	item_transfer_result receipt{};
	receipt.root_item_uid = item_transfer_result_root(payload);
	receipt.item_count = payload.item_count;
	receipt.from_owner_revision = rejected ? payload.expected_from_revision :
						 payload.expected_from_revision + 1;
	receipt.to_owner_revision = receipt_destination;
	receipt.max_item_revision = rejected ? 0 : 3;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded{};
	assert(item_transfer_command_encode_result(receipt, &encoded));
	auto &completion = result.completion;
	completion.operation_id = result.command.operation_id;
	completion.disposition = critical_completion_disposition::execution;
	completion.outcome = rejected ? critical_apply_outcome::terminal_failure :
					critical_apply_outcome::applied;
	completion.error_code = rejected ? ESTALE : 0;
	completion.failure_stage = critical_failure_stage::none;
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	completion.durable_revision =
		std::max({ receipt.from_owner_revision, receipt.to_owner_revision,
			   receipt.max_item_revision });
	result.current.reference = reference;
	result.current.state = quest_mobile_lifetime_state::live;
	result.current.last_transition_operation = rejected ? id(11) : result.command.operation_id;
	result.current.items = result.native_before;
	result.current.cash = quest_mobile_native_cash{ 4, { { 13, 2, 1, 0 } } };
	if (!rejected)
	{
		assert(quest_mobile_native_items_transition(result.native_before, reference,
							    payload, &result.current.items) ==
		       player_snapshot_codec_result::ok);
		++result.current.reference.mobile_revision;
		++result.current.reference.stock_revision;
	}
	result.current_destination = current_destination;
	return result;
}

void owner(MYSQL *db, const item_owner_identity &identity, uint64_t revision)
{
	query(db,
	      "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(" +
		      std::to_string(static_cast<uint8_t>(identity.type)) + "," +
		      std::to_string(identity.id) + ",0," + std::to_string(revision) + ")");
}

void seed(MYSQL *db, const original_cut &original)
{
	const auto &payload = original.payload;
	const bool acceptance = payload.native_mobile.action ==
				item_native_mobile_action::acceptance;
	const bool rejected = original.completion.outcome ==
			      critical_apply_outcome::terminal_failure;
	// Acceptance controls below use the committed empty-player AFTER, so no
	// player physical row/sidecar is fabricated or omitted from an expected forest.
	assert(!acceptance || !rejected);
	query(db, "INSERT INTO player_data(pid,name,account_name,save_revision) VALUES(" +
			  std::to_string(PID) + ",'NativePublication','NativePublicationFixture'," +
			  std::to_string(SAVE) + ")");
	owner(db, payload.from_owner,
	      rejected ? payload.expected_from_revision : payload.expected_from_revision + 1);
	owner(db, payload.to_owner, original.current_destination);
	if (!acceptance)
		owner(db, { item_owner_type::player, PID, 0 }, 19);
	const auto &image = original.current;
	query(db,
	      "INSERT INTO quest_mobile_native(mobile_instance_id,mobile_revision,stock_revision,lifetime_state,canonical_image) VALUES(" +
		      std::to_string(MOBILE) + "," +
		      std::to_string(image.reference.mobile_revision) + "," +
		      std::to_string(image.reference.stock_revision) + ",1,X'" +
		      hex(image_bytes(image)) + "')");
	const auto custody = rejected ? payload.from_owner : payload.to_owner;
	query(db,
	      "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(" +
		      std::to_string(UID) + "," + std::to_string(UID) + ",NULL," +
		      std::to_string(static_cast<uint8_t>(custody.type)) + "," +
		      std::to_string(custody.id) + ",0," + (rejected ? "2" : "3") + ",9001," +
		      (!acceptance && !rejected ? "2" : "1") + ",0)");
}

economic_sql_native_quest_publication sentinel(const original_cut &original)
{
	economic_sql_native_quest_publication result;
	result.session_id = 777;
	result.native = original.current;
	result.from_owner_revision = 888;
	result.to_owner_revision = 889;
	result.player_owner_revision = 999;
	result.acknowledged_save_revision = 666;
	result.custody.push_back({ UID + 50,
				   UID + 50,
				   0,
				   { item_owner_type::player, PID, 0 },
				   999,
				   998,
				   9002,
				   item_custody_state::active });
	return result;
}

void unchanged(const economic_sql_native_quest_publication &output,
	       const economic_sql_native_quest_publication &before)
{
	assert(output.session_id == before.session_id && output.custody.size() == 1 &&
	       output.custody[0].item_uid == before.custody[0].item_uid &&
	       output.custody[0].root_item_uid == before.custody[0].root_item_uid &&
	       output.custody[0].parent_item_uid == before.custody[0].parent_item_uid &&
	       item_owner_identity_equal(output.custody[0].owner, before.custody[0].owner) &&
	       output.custody[0].item_revision == before.custody[0].item_revision &&
	       output.custody[0].owner_revision == before.custody[0].owner_revision &&
	       output.custody[0].vnum == before.custody[0].vnum &&
	       output.custody[0].state == before.custody[0].state &&
	       output.from_owner_revision == before.from_owner_revision &&
	       output.to_owner_revision == before.to_owner_revision &&
	       output.player_owner_revision == before.player_owner_revision &&
	       output.acknowledged_save_revision == before.acknowledged_save_revision &&
	       image_bytes(output.native) == image_bytes(before.native));
}

void check(MYSQL *db, const original_cut &original, unsigned int expected)
{
	auto output = sentinel(original);
	const auto before = output;
	const auto session = mysql_thread_id(db);
	// This existing receipt-cut suite deliberately locks an empty current player
	// forest: last-item acceptance AFTER or consumption's empty original forest.
	// The real catalog provider remains linked but no populated template route is
	// qualified here; do not turn unavailable boot provenance into a fake owner.
	assert((original.completion.outcome == critical_apply_outcome::terminal_failure ?
			original.player_before :
			original.player_after)
		       .empty());
	const auto code = economic_sql_native_quest_lock_publication(
		db, original.command, original.completion, original.native_before,
		original.player_before, original.player_after, SAVE, &output);
	assert(code == expected);
	assert(mysql_thread_id(db) == session && (db->server_status & SERVER_STATUS_IN_TRANS) &&
	       (db->server_status & SERVER_STATUS_AUTOCOMMIT));
	if (expected)
		unchanged(output, before);
	else
	{
		const bool acceptance = original.payload.native_mobile.action ==
					item_native_mobile_action::acceptance;
		assert(output.session_id == session && output.acknowledged_save_revision == SAVE &&
		       output.player_owner_revision == (acceptance ? 4 : 19) &&
		       output.to_owner_revision == original.current_destination &&
		       output.from_owner_revision ==
			       (original.completion.outcome ==
						critical_apply_outcome::terminal_failure ?
					original.payload.expected_from_revision :
					original.payload.expected_from_revision + 1) &&
		       output.custody.size() == 1 && output.custody[0].item_uid == UID &&
		       output.custody[0].owner_revision ==
			       (original.completion.outcome ==
						critical_apply_outcome::terminal_failure ?
					original.payload.expected_from_revision :
					original.current_destination) &&
		       image_bytes(output.native) == image_bytes(original.current));
	}
	// Same original session still owns all seeded rows; provider did not commit.
	assert(scalar(db, "SELECT COUNT(*) FROM quest_mobile_native") == 1);
}

// This extends the original SQL publication-cut component, not native birth or
// admission authority. The original seed() facts remain explicit fixture data;
// successful publication below is the actual existing production API call.
struct catalog_cut
{
	economic_sql_physical_source_snapshot physical;
	sql_room_item_source_snapshot rooms;
	quest_mobile_native_sql_catalog native;
};

catalog_cut capture_catalog_cut(MYSQL *db)
{
	catalog_cut result;
	const economic_sql_source_limits limits;
	const auto session = mysql_thread_id(db);
	assert((db->server_status & SERVER_STATUS_IN_TRANS) && session);
	assert(!economic_sql_capture_physical_sources_in_transaction(db, limits, &result.physical));
	assert(!sql_room_item_payload_capture_sources_in_transaction(db, limits, result.physical,
								     &result.rooms));
	assert(!quest_mobile_native_sql_capture_catalog_in_transaction(
		db, result.physical, result.rooms, limits, &result.native));
	assert(mysql_thread_id(db) == session && (db->server_status & SERVER_STATUS_IN_TRANS) &&
	       (db->server_status & SERVER_STATUS_AUTOCOMMIT));
	assert(result.native.original_session == session &&
	       result.native.physical_digest == result.physical.digest);
	return result;
}

void catalog_publication_check(MYSQL *db, const original_cut &original)
{
	const auto observed = capture_catalog_cut(db);
	const auto &report = observed.native;
	assert(report.catalog.size() == 1 && report.catalog[0].image &&
	       report.catalog[0].mobile_instance_id == MOBILE &&
	       report.catalog[0].mobile_revision == original.current.reference.mobile_revision &&
	       report.catalog[0].stock_revision == original.current.reference.stock_revision &&
	       report.catalog[0].lifetime_state == uint8_t(quest_mobile_lifetime_state::live) &&
	       image_bytes(*report.catalog[0].image) == image_bytes(original.current));
	for (const auto &cell : report.catalog[0].cells)
		assert(cell);
	assert(report.catalog[0].flags == 0 && report.catalog[0].owner_revision &&
	       report.items.size() == original.current.items.size() &&
	       report.native_custody.size() == report.items.size() &&
	       report.unmatched_active_custody.empty() && report.malformed_custody.empty() &&
	       report.extra_custody.empty() && report.findings.empty());
	const auto clock = *report.catalog[0].owner_revision;
	assert(clock.row < observed.physical.source2.tables[12].rows.size() &&
	       clock.digest == observed.physical.source2.tables[12].rows[clock.row].digest);
	for (const auto &witness : report.items)
	{
		assert(witness.current_field_correspondence && witness.flags == 0 &&
		       witness.catalog_row == 0 && witness.custody && witness.equipment &&
		       witness.owner_revision && witness.observed_item_revision &&
		       witness.root_item_uid == UID && !witness.parent_item_uid);
		const auto custody = *witness.custody, equipment = *witness.equipment;
		assert(custody.row < observed.physical.source2.tables[11].rows.size() &&
		       custody.digest ==
			       observed.physical.source2.tables[11].rows[custody.row].digest &&
		       equipment.row <
			       observed.physical.source2.item_equipment_sources[0].rows.size() &&
		       equipment.digest == observed.physical.source2.item_equipment_sources[0]
						   .rows[equipment.row]
						   .digest);
		// This original publication fixture observes item2/3 and native stock7/8:
		// the item revision is borrowed SQL evidence, never stock-derived.
		assert(*witness.observed_item_revision != *report.catalog[0].stock_revision);
	}
	assert(report.rows == observed.rooms.rows + 1 && report.cells == observed.rooms.cells + 6);
	// No origin/lineage tables or authenticated birth facts were provided to the
	// inspector; original publication assertions still belong to check().
}

void catalog_reader_components(MYSQL *db)
{
	using namespace quest_mobile_native_catalog_flags;
	const auto original = cut(true, false, 8, 8);
	query(db, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	query(db, "START TRANSACTION");
	seed(db, original);
	check(db, original, 0);
	catalog_publication_check(db, original);
	// These rolled-back perturbations are component reader facts, never a native
	// producer/admission pass. Each captures a new same-session RR cut after the
	// fixture's own writes; no pre-write packet is reused as post-write evidence.
	auto perturb = [&](const std::string &sql, uint64_t flag, bool current,
			   size_t related_count = 1, size_t extra_count = 0)
	{
		query(db, "SAVEPOINT native_catalog_reader");
		query(db, sql);
		const auto observed = capture_catalog_cut(db);
		assert(observed.native.items.size() == 1 &&
		       observed.native.items[0].current_field_correspondence == current &&
		       observed.physical.source2.tables[11].rows.size() == related_count &&
		       observed.native.related_custody.size() == related_count &&
		       observed.native.native_custody.size() == 1 &&
		       observed.native.extra_custody.size() == extra_count);
		for (const auto &source : observed.native.related_custody)
			assert(source.row < observed.physical.source2.tables[11].rows.size() &&
			       source.digest ==
				       observed.physical.source2.tables[11].rows[source.row].digest);
		if (flag)
			assert(std::any_of(observed.native.findings.begin(),
					   observed.native.findings.end(), [&](const auto &finding)
					   { return finding.flag & flag; }));
		else
			assert(observed.native.findings.empty());
		query(db, "ROLLBACK TO SAVEPOINT native_catalog_reader");
		query(db, "RELEASE SAVEPOINT native_catalog_reader");
		catalog_publication_check(db, original);
	};
	perturb("UPDATE quest_mobile_native SET mobile_revision=mobile_revision+1 WHERE mobile_instance_id=" +
			std::to_string(MOBILE),
		invalid_row, false);
	perturb("UPDATE item_owner_revision SET revision=revision+1 WHERE owner_type=12 AND owner_id=" +
			std::to_string(MOBILE) + " AND owner_context_id=0",
		owner_clock_mismatch, false);
	perturb("UPDATE item_current_owner SET equipment_slot=1 WHERE item_uid=" +
			std::to_string(UID),
		custody_mismatch, false);
	const std::string extra =
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(" +
		std::to_string(UID + 50) + "," + std::to_string(UID) + "," + std::to_string(UID) +
		"," + std::to_string(uint8_t(item_owner_type::player)) + "," + std::to_string(PID) +
		",0,1,9001,";
	perturb(extra + "1,0)", extra_related_custody, false, 2, 1);
	perturb(extra + "2,0)", 0, true, 2);
	perturb(extra + "3,0)", 0, true, 2);
	// CHECK(state BETWEEN1 AND3) makes malformed state a separate raw-reader
	// component case, not a legal SQL perturbation of this genuine schema.

	query(db, "SAVEPOINT native_catalog_terminal");
	auto terminal = original.current;
	terminal.reference.mobile_revision = terminal.reference.stock_revision = UINT64_MAX;
	const auto terminal_bytes = image_bytes(terminal); // Original native canonical codec.
	query(db,
	      "UPDATE quest_mobile_native SET mobile_revision=18446744073709551615,stock_revision=18446744073709551615,canonical_image=X'" +
		      hex(terminal_bytes) + "' WHERE mobile_instance_id=" + std::to_string(MOBILE));
	query(db,
	      "UPDATE item_owner_revision SET revision=18446744073709551615 WHERE owner_type=12 AND owner_id=" +
		      std::to_string(MOBILE) + " AND owner_context_id=0");
	quest_mobile_native_sql_row locked;
	assert(!quest_mobile_native_sql_lock(db, MOBILE, &locked) && locked.present &&
	       image_bytes(locked.image) == terminal_bytes);
	const auto terminal_cut = capture_catalog_cut(db);
	assert(terminal_cut.native.findings.empty() && terminal_cut.native.items.size() == 1 &&
	       terminal_cut.native.items[0].current_field_correspondence &&
	       terminal_cut.native.catalog[0].mobile_revision == UINT64_MAX &&
	       terminal_cut.native.catalog[0].stock_revision == UINT64_MAX &&
	       terminal_cut.native.items[0].observed_item_revision == 3);
	query(db, "ROLLBACK TO SAVEPOINT native_catalog_terminal");
	query(db, "RELEASE SAVEPOINT native_catalog_terminal");
	catalog_publication_check(db, original);
	const auto retained = capture_catalog_cut(db);
	query(db, "ROLLBACK");
	assert(scalar(db, "SELECT COUNT(*) FROM quest_mobile_native") == 0);
	quest_mobile_native_sql_catalog output;
	output.original_session = 777;
	output.rows = 888;
	output.catalog.resize(1);
	output.catalog[0].cells[0] = "catalog output sentinel";
	assert(quest_mobile_native_sql_capture_catalog_in_transaction(
		       db, retained.physical, retained.rooms, {}, &output) == EBUSY);
	assert(output.original_session == 777 && output.rows == 888 && output.catalog.size() == 1 &&
	       output.catalog[0].cells[0] == "catalog output sentinel" && output.items.empty());
}

void example(MYSQL *db, const original_cut &original, unsigned int expected)
{
	query(db, "START TRANSACTION");
	seed(db, original);
	check(db, original, expected);
	if (!expected)
		catalog_publication_check(db, original);
	query(db, "ROLLBACK");
	assert(scalar(db, "SELECT COUNT(*) FROM quest_mobile_native") == 0);
}
}

int main()
{
	// Actual db.c catalog is unbootstrapped in this bounded SQL component.
	assert(!recovery_object_templates_ready());
	assert(find_recovery_object_template(9001) == nullptr);
	const char *database = std::getenv("NATIVE_QUEST_PUBLICATION_TEST_DB_NAME");
	const char *disposable = std::getenv("NATIVE_QUEST_PUBLICATION_DISPOSABLE");
	const char *host = std::getenv("DB_HOST"), *user = std::getenv("DB_USER");
	const char *password = std::getenv("DB_PASSWD"), *port = std::getenv("DB_PORT");
	const char *socket = std::getenv("DB_SOCKET");
	assert(socket && *socket && socket[0] == '/');
	assert(database && disposable && !std::strcmp(disposable, "1") && host && user &&
	       password &&
	       std::regex_match(database,
				std::regex("native_quest_publication_test_[a-f0-9]{16}")));
	MYSQL *db = mysql_init(nullptr);
	assert(db);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	const unsigned int timeout = 3;
	assert(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect));
	assert(!mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(mysql_real_connect(db, host, user, password, database,
				  port ? std::strtoul(port, nullptr, 10) : 3306, socket, 0));
	query(db, "SET SESSION innodb_lock_wait_timeout=1");
	query(db, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	for (const char *table :
	     { "quest_mobile_native", "item_current_owner", "item_owner_revision", "player_data",
	       "player_items", "player_item_runtime_state" })
		assert(scalar(db, "SELECT COUNT(*) FROM " + std::string(table)) == 0);

	// Regression: stale rejection observed destination15, not merely expected10.
	// The unfixed provider incorrectly accepts14;15 and later16 must remain valid.
	example(db, cut(false, true, 15, 14), ESTALE);
	example(db, cut(false, true, 15, 15), 0);
	example(db, cut(false, true, 15, 16), 0);
	// Early receipt0 cannot weaken the immutable original expected destination10.
	example(db, cut(false, true, 0, 9), ESTALE);
	example(db, cut(false, true, 0, 10), 0);
	// Successful destruction permits a later shared sink, keeping the native cut exact.
	example(db, cut(false, false, 11, 11), 0);
	example(db, cut(false, false, 11, 12), 0);
	example(db, cut(false, false, 11, 10), ESTALE);
	// Offering acceptance consumes the last giver item. Empty AFTER still carries
	// the actual locked player owner revision4, never a made-up zero.
	example(db, cut(true, false, 8, 8), 0);
	example(db, cut(true, false, 8, 9), ESTALE);

	const auto original = cut(false, true, 15, 15);
	query(db, "START TRANSACTION");
	seed(db, original);
	auto malformed = original;
	malformed.completion.result_payload[ITEM_TRANSFER_RESULT_BYTES] = 1;
	check(db, malformed, EILSEQ);
	auto absent_original = original;
	absent_original.native_before.clear();
	check(db, absent_original, ESTALE);
	query(db, "UPDATE player_data SET save_revision=24 WHERE pid=" + std::to_string(PID));
	check(db, original, ESTALE);
	query(db, "UPDATE player_data SET save_revision=23 WHERE pid=" + std::to_string(PID));
	query(db, "UPDATE item_current_owner SET item_revision=3 WHERE item_uid=" +
			  std::to_string(UID));
	check(db, original, ESTALE);
	query(db, "UPDATE item_current_owner SET item_revision=2 WHERE item_uid=" +
			  std::to_string(UID));
	query(db,
	      "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(910004," +
		      std::to_string(UID) + ",NULL,3,100,0,1,9001,1,0)");
	check(db, original, ESTALE);
	query(db, "ROLLBACK");

	auto output = sentinel(original);
	const auto before = output;
	assert(economic_sql_native_quest_lock_publication(db, original.command, original.completion,
							  original.native_before, {}, {}, SAVE,
							  &output) == ENOTCONN);
	unchanged(output, before);
	query(db, "START TRANSACTION");
	seed(db, original);
	reconnect = true;
	assert(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect));
	check(db, original, ENOTCONN);
	reconnect = false;
	assert(!mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect));
	check(db, original, 0);
	catalog_publication_check(db, original);
	query(db, "ROLLBACK");
	query(db, "SET autocommit=0");
	query(db, "START TRANSACTION");
	seed(db, original);
	assert(economic_sql_native_quest_lock_publication(db, original.command, original.completion,
							  original.native_before, {}, {}, SAVE,
							  &output) == ENOTCONN);
	unchanged(output, before);
	query(db, "ROLLBACK");
	query(db, "SET autocommit=1");
	assert(scalar(db, "SELECT COUNT(*) FROM quest_mobile_native") == 0);
	catalog_reader_components(db);
	mysql_close(db);
	// Actual db.c catalog is unbootstrapped in this bounded SQL component.
	assert(!recovery_object_templates_ready());
	assert(find_recovery_object_template(9001) == nullptr);
	std::puts(
		"native quest current SQL publication cut: rejection lower bounds, success, empty giver, custody/save fences and borrowed session passed");
}
