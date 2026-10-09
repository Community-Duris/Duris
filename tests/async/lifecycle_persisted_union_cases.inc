// Additive SQL controls in the original lifecycle fixture. These observations
// exercise its real fresh activation owner; synthetic route coverage remains
// fixture-only, and this does not qualify production activation or origin.
#include "economy/auction_listing_accounting.h"
#include "economy/auction_native_command_context.h"
#include "economy/collector_repository.h"
#include "economy/item_transfer_accounting.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_auction_listing_transaction.h"
#include "player/player_snapshot_repository.h"

static player_item_snapshot lifecycle_union_item(uint64_t uid)
{
	player_item_snapshot item{};
	item.object_uid = uid;
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	item.vnum = 501;
	item.type = 1;
	item.string_mask = 15;
	item.name = "lifecycle union item";
	item.short_description = "a lifecycle union item";
	item.description = "A lifecycle union item is here.";
	item.action_description = "";
	return item;
}
static uint64_t lifecycle_union_revision(MYSQL *db, unsigned kind, uint64_t owner)
{
	const auto found = rows(db, "SELECT revision FROM item_owner_revision WHERE owner_type=" +
					    std::to_string(kind) + " AND owner_id=" +
					    std::to_string(owner) + " AND owner_context_id=0");
	return found.empty() ? 0 : std::stoull(found.at(0).at(0));
}
static critical_command lifecycle_union_drop(const economic_sql_lifecycle_request &request,
					     MYSQL *db, const player_item_snapshot &item)
{
	item_transfer_payload payload{};
	payload.from_owner = { item_owner_type::player, 21002, 0 };
	payload.to_owner = { item_owner_type::room, 22800, 0 };
	payload.expected_from_revision = lifecycle_union_revision(db, 1, 21002);
	payload.expected_to_revision = lifecycle_union_revision(db, 3, 22800);
	payload.reason = item_transfer_reason::player_drop;
	payload.reason_id = 22800;
	payload.selected_item_uid = payload.target_root_item_uid = item.object_uid;
	payload.item_count = 1;
	payload.items[0] = { item.object_uid, item.object_uid,		 0, 1,
			     item.vnum,	      item_custody_state::active };
	std::vector<uint8_t> bytes;
	if (player_item_snapshot_list_encode({ item }, &bytes) !=
		    player_snapshot_codec_result::ok ||
	    bytes.size() > payload.item_blob.size())
		throw std::runtime_error("lifecycle room literal did not encode");
	payload.item_blob_size = static_cast<uint32_t>(bytes.size());
	std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
	critical_command command{};
	if (!item_transfer_command_build(&command, ident(202), payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive) ||
	    item_transfer_accounting_intent(command, request.lineage, request.epoch, 21002,
					    &command.accounting_intent) !=
		    economic_accounting_error::ok)
		throw std::runtime_error("lifecycle real room command did not bind");
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accepted_at_usec = 1234567;
	return command;
}
static void lifecycle_union_seed(MYSQL *db, const economic_sql_lifecycle_request &request)
{
	// Existing baseline controls have completed. Materialize two selected native
	// player items through the original borrowed projection, then use the real
	// native listing owner and ordinary accounted room-drop repository.
	const auto auction_item = lifecycle_union_item(6);
	const auto room_item = lifecycle_union_item(8);
	execute(db, "START TRANSACTION");
	execute(db, "UPDATE player_data SET level=60 WHERE pid=21002");
	execute(db, "UPDATE item_uid_allocator SET next_uid=9 WHERE allocator_id=1");
	execute(db,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1,21002,0,1)");
	execute(db,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(6,6,NULL,1,21002,0,1,501,1),(8,8,NULL,1,21002,0,1,501,1)");
	const std::vector<player_item_snapshot> before{ auction_item, room_item };
	if (const auto code =
		    player_snapshot_repository_project_items_in_transaction(db, 21002, 15, before))
		throw std::runtime_error("lifecycle native player projection refused " +
					 std::to_string(code));
	execute(db, "COMMIT");

	const auto key =
		[&](economic_account_kind kind, unsigned locator, uint64_t native, uint64_t context)
	{
		return economic_account_key{
			request.lineage, kind,
			std::stoull(scalar(
				db,
				"SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
					sql_id(request.lineage) + " AND account_kind=" +
					std::to_string(static_cast<unsigned>(kind)) +
					" AND locator_kind=" + std::to_string(locator) +
					" AND native_id=" + std::to_string(native) +
					" AND context_id=" + std::to_string(context))),
			context
		};
	};
	const auto wallet = key(economic_account_kind::wallet, 1, 21002, 0);
	const auto bank_id = std::stoull(scalar(
		db, "SELECT id FROM account_banks WHERE account_name='lifecycle_b' AND racewar=1"));
	const auto bank = key(economic_account_kind::bank, 2, bank_id, 1);
	auction_command_payload payload{};
	payload.action = auction_action::list;
	payload.actor_pid = 21002;
	payload.racewar = 1;
	std::strcpy(payload.account_name.data(), "lifecycle_b");
	std::strcpy(payload.actor_name.data(), "LifecycleTwo");
	payload.expected_wallet_revision =
		std::stoull(scalar(db, "SELECT wallet_revision FROM player_data WHERE pid=21002"));
	payload.expected_bank_revision = std::stoull(scalar(
		db, "SELECT bank_revision FROM account_banks WHERE id=" + std::to_string(bank_id)));
	payload.start_price = 500;
	payload.end_time = 2000000000;
	payload.item_count = 1;
	payload.items[0] = { 6, 1, 501 };
	payload.object_blob[0] = 'x';
	payload.object_blob_size = 1;
	critical_command command{};
	if (!auction_command_build(&command, ident(201), payload, critical_source_site::command,
				   critical_deadline_class::interactive) ||
	    auction_native_command_bind(&command, before,
					std::vector<player_item_snapshot>{ room_item },
					std::vector<player_item_snapshot>{ auction_item }, 60,
					15) != economic_accounting_error::ok)
		throw std::runtime_error("lifecycle native auction source did not bind");
	command.accepted_at_usec = 1234566;
	if (auction_listing_accounting_intent(command, request.epoch, wallet, bank,
					      &command.accounting_intent) !=
	    economic_accounting_error::ok)
		throw std::runtime_error("lifecycle actual auction intent refused");
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	execute(db, "START TRANSACTION");
	retain_auction_inbox(db, command);
	economic_sql_auction_listing_context context;
	auction_command_result listed{};
	unsigned int result = 0;
	bool applied = false;
	const auto locked = economic_sql_auction_listing_lock(db, command, &context);
	const auto recorded = locked ? locked :
				       economic_sql_auction_listing_execute_and_record(
					       db, command, context, &listed, &result, &applied);
	if (recorded || result || !applied)
		throw std::runtime_error("lifecycle actual native listing refused " +
					 std::to_string(recorded) + ":" + std::to_string(result));
	retain_auction_result(db, command, listed);
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> receipt{};
	if (!auction_command_encode_result(listed, &receipt) ||
	    !critical_command_repository_insert_outbox_event(db, command.operation_id, 0, 5, 1, 1,
							     receipt.data(), receipt.size()))
		throw std::runtime_error("lifecycle actual auction receipt/outbox refused");
	execute(db, "COMMIT");
	std::vector<player_item_snapshot> decoded;
	std::vector<uint64_t> revisions;
	const auto blob = rows(db, "SELECT obj_blob FROM auction_item_custody WHERE auction_id=" +
					   std::to_string(listed.auction_id) + " AND slot=0")
				  .at(0)
				  .at(0);
	if (!auction_repository_decode_native_tree_blob(
		    std::span(reinterpret_cast<const uint8_t *>(blob.data()), blob.size()),
		    &decoded, &revisions) ||
	    decoded.size() != 1 || decoded[0].object_uid != 6 ||
	    revisions != std::vector<uint64_t>{ 2 })
		throw std::runtime_error("actual native listing literal readback refused");
	const auto dropped =
		critical_command_repository_apply(db, lifecycle_union_drop(request, db, room_item));
	if (dropped.outcome != critical_apply_outcome::applied || dropped.error_code)
		throw std::runtime_error("lifecycle original room drop refused " +
					 std::to_string(dropped.error_code));
	// Fixture-only completed delivery; no real game/publication route is invoked.
	execute(db,
		"UPDATE critical_outbox SET delivered_at=CURRENT_TIMESTAMP(6) WHERE operation_id IN(" +
			sql_id(ident(201)) + "," + sql_id(ident(202)) + ")");

	// Original collector metadata/literal codecs provide a legitimate retained
	// native boundary. No accounting receipt or original entitlement is invented.
	collector::record record;
	collector::rules rules;
	rules.enabled = true;
	char death[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	if (!critical_operation_id_to_hex(ident(203), death, sizeof(death)) ||
	    collector::enroll(1, death, 21002, 7, 1, 1000, rules, &record) !=
		    collector::outcome::applied ||
	    collector::collect(&record, record.revision, record.item_revision, record.item_revision,
			       true, 75, record.collect_at) != collector::outcome::applied)
		throw std::runtime_error("lifecycle original collector metadata refused");
	std::array<uint8_t, collector::encoded_record_bytes> encoded{};
	std::vector<uint8_t> literal;
	if (collector::record_encode(record, &encoded) != collector::codec_result::ok ||
	    player_item_snapshot_list_encode({ lifecycle_union_item(7) }, &literal) !=
		    player_snapshot_codec_result::ok)
		throw std::runtime_error("lifecycle collector retained literal refused");
	execute(db, "START TRANSACTION");
	execute(db, "UPDATE collector_catalog_state SET next_listing=2 WHERE state_id=1");
	execute(db,
		"INSERT INTO collector_deaths(death_operation_id,beneficiary_pid,death_time,collection_delay,sale_delay,holding_duration,price_percent,minimum_value) VALUES(" +
			sql_id(ident(203)) + ",21002,1000," +
			std::to_string(rules.collection_delay) + "," +
			std::to_string(rules.sale_delay) + "," +
			std::to_string(rules.holding_duration) + "," +
			std::to_string(rules.price_percent) + "," +
			std::to_string(rules.minimum_value) + ")");
	execute(db,
		"INSERT INTO collector_listings(listing_id,death_operation_id,beneficiary_pid,item_uid,status,due_at,listing_revision,item_revision,price_value,record_blob,item_blob) VALUES(1," +
			sql_id(ident(203)) + ",21002,7,2," + std::to_string(record.sale_at) + "," +
			std::to_string(record.revision) + "," +
			std::to_string(record.item_revision) + "," +
			std::to_string(record.price_value) + "," + fixture_bytes(encoded) + "," +
			fixture_bytes(literal) + ")");
	execute(db,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(7,7,NULL,10,1,0," +
			std::to_string(record.item_revision) + ",501,1)");
	execute(db,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(10,1,0,1)");
	execute(db, "COMMIT");
}
static size_t lifecycle_union_callback_entries = 0;
static unsigned int lifecycle_union_callback(MYSQL *db,
					     const economic_sql_lifecycle_request &request,
					     const economic_sql_activation_evidence &coverage,
					     const economic_sql_source_snapshot &snapshot)
{
	++lifecycle_union_callback_entries;
	const auto session = mysql_thread_id(db);
	economic_sql_physical_source_snapshot base;
	sql_room_item_source_snapshot room;
	economic_sql_persisted_correspondence report;
	if (economic_sql_capture_physical_sources_in_transaction(db, {}, &base) ||
	    sql_room_item_payload_capture_sources_in_transaction(db, {}, base, &room) ||
	    economic_sql_normalize_persisted_correspondence(base, room, {}, 1, &report) !=
		    economic_accounting_error::ok ||
	    base.source2.digest != snapshot.digest ||
	    base.source2.custody_digest != snapshot.custody_digest ||
	    !report.unmatched_active_custody_indices.empty() ||
	    !report.multiply_matched_active_custody_indices.empty())
		return EILSEQ;
	std::array<bool, 4> providers{};
	for (const auto &custody : report.custody)
		for (const auto &match : custody.matches)
			providers.at(static_cast<size_t>(match.provider)) = true;
	if (providers != std::array<bool, 4>{ true, true, true, true } ||
	    mysql_thread_id(db) != session || !(db->server_status & SERVER_STATUS_IN_TRANS))
		return EILSEQ;
	return verify_synthetic_routes(db, request, coverage, snapshot);
}

static unsigned int lifecycle_season_callback(MYSQL *db,
					      const economic_sql_lifecycle_request &request,
					      const economic_sql_activation_evidence &coverage,
					      const economic_sql_source_snapshot &snapshot)
{
	++lifecycle_union_callback_entries;
	return verify_synthetic_routes(db, request, coverage, snapshot);
}
template <class Cutover> static void
lifecycle_union_season_cases(MYSQL *setup, MYSQL *db, const economic_sql_lifecycle_request &request,
			     const economic_sql_activation_evidence &coverage, Cutover &cutover)
{
	// No modern payload/graph exists here: this reaches the season singleton
	// finding that previously survived only in a capped SIZE_MAX diagnostic.
	assert_scalar(setup, "SELECT COUNT(*) FROM sql_room_item_payload", "0");
	const auto run = [&](bool valid)
	{
		lifecycle_union_callback_entries = 0;
		const auto code = cutover(
			[&](auto &owner)
			{
				const auto session = mysql_thread_id(db);
				const auto result = economic_sql_accounting_lifecycle_transaction::
					activate_verified(db, owner, request, coverage,
							  lifecycle_season_callback);
				if (!owner.is_valid() || mysql_thread_id(db) != session ||
				    !(db->server_status & SERVER_STATUS_IN_TRANS))
					throw std::runtime_error(
						"season refusal changed original transaction/session");
				return result;
			},
			false);
		if (code != (valid ? 0U : static_cast<unsigned int>(ENODATA)) ||
		    lifecycle_union_callback_entries != (valid ? 1U : 0U))
			throw std::runtime_error(
				"missing/resetting empty-room season reached verifier");
		assert_scalar(setup,
			      "SELECT COUNT(*) FROM economic_sql_global_activation WHERE lineage=" +
				      sql_id(request.lineage),
			      "0");
	};
	run(true);
	execute(setup,
		"CREATE TEMPORARY TABLE lifecycle_union_season AS SELECT * FROM season_reset_state");
	execute(setup, "DELETE FROM season_reset_state");
	run(false);
	execute(setup, "INSERT INTO season_reset_state SELECT * FROM lifecycle_union_season");
	execute(setup, "UPDATE season_reset_state SET reset_status='resetting' WHERE state_id=1");
	run(false);
	execute(setup,
		"UPDATE season_reset_state s JOIN lifecycle_union_season b ON b.state_id=s.state_id SET s.reset_status=b.reset_status");
	run(true);
}

template <class Cutover> static void
lifecycle_union_cases(MYSQL *setup, MYSQL *db, const economic_sql_lifecycle_request &request,
		      const economic_sql_activation_evidence &coverage, Cutover &cutover)
{
	const auto decision = [&]
	{
		return rows(
			setup,
			"SELECT state,revision FROM economic_sql_global_activation WHERE lineage=" +
				sql_id(request.lineage));
	};
	const auto before = decision();
	const auto run = [&](bool success)
	{
		lifecycle_union_callback_entries = 0;
		const auto code = cutover(
			[&](auto &owner)
			{
				const auto session = mysql_thread_id(db);
				const auto result = economic_sql_accounting_lifecycle_transaction::
					activate_verified(db, owner, request, coverage,
							  lifecycle_union_callback);
				if (!owner.is_valid() || mysql_thread_id(db) != session ||
				    !(db->server_status & SERVER_STATUS_IN_TRANS))
					throw std::runtime_error(
						"fresh union changed caller transaction/session");
				return result;
			},
			false);
		if (lifecycle_union_callback_entries != (success ? 1U : 0U))
			throw std::runtime_error(
				"fresh prerequisite did not refuse before verifier callback");
		if ((code == 0) != success || decision() != before)
			throw std::runtime_error("fresh union refusal/rollback decision differs " +
						 std::to_string(code));
	};
	run(true);
	execute(setup, "RENAME TABLE player_items TO lifecycle_union_missing_player");
	run(false);
	execute(setup, "RENAME TABLE lifecycle_union_missing_player TO player_items");
	// Original native physical rows are repaired exactly after each refusal.
	execute(setup,
		"CREATE TEMPORARY TABLE lifecycle_union_player AS SELECT * FROM player_items WHERE obj_uid=3");
	execute(setup, "DELETE FROM player_items WHERE obj_uid=3");
	run(false);
	execute(setup, "INSERT INTO player_items SELECT * FROM lifecycle_union_player");
	execute(setup, "UPDATE player_items SET obj_uid=NULL WHERE obj_uid=3");
	run(false);
	execute(setup,
		"UPDATE player_items SET obj_uid=3 WHERE id=(SELECT id FROM lifecycle_union_player)");
	execute(setup, "UPDATE player_items SET equip_slot=-1 WHERE obj_uid=3");
	run(false);
	execute(setup, "UPDATE player_items SET equip_slot=0 WHERE obj_uid=3");
	execute(setup,
		"INSERT INTO player_items(pid,obj_uid,vnum,equip_slot) VALUES(21001,3,501,0)");
	const auto duplicate = mysql_insert_id(setup);
	run(false);
	execute(setup, "DELETE FROM player_items WHERE id=" + std::to_string(duplicate));
	execute(setup,
		"CREATE TEMPORARY TABLE lifecycle_union_room AS SELECT * FROM sql_room_item_payload WHERE item_uid=8");
	execute(setup, "DELETE FROM sql_room_item_payload WHERE item_uid=8");
	run(false);
	execute(setup, "INSERT INTO sql_room_item_payload SELECT * FROM lifecycle_union_room");
	execute(setup,
		"CREATE TEMPORARY TABLE lifecycle_union_collector AS SELECT * FROM collector_listings WHERE listing_id=1");
	execute(setup, "UPDATE collector_listings SET item_blob=NULL WHERE listing_id=1");
	run(false);
	execute(setup,
		"UPDATE collector_listings l JOIN lifecycle_union_collector b ON b.listing_id=l.listing_id SET l.item_blob=b.item_blob");
	execute(setup,
		"CREATE TEMPORARY TABLE lifecycle_union_auction AS SELECT * FROM auction_item_custody WHERE item_uid=6 AND claimed_at IS NULL");
	execute(setup,
		"UPDATE auction_item_custody SET obj_blob=X'00000000' WHERE item_uid=6 AND claimed_at IS NULL");
	run(false);
	execute(setup,
		"UPDATE auction_item_custody c JOIN lifecycle_union_auction b ON b.auction_id=c.auction_id AND b.slot=c.slot SET c.obj_blob=b.obj_blob");
	run(true);
	puts("PASS actual lifecycle fresh-cut all4 current correspondence, missing/NULL/duplicate/equipment/literal refusals; real SQL/native execution qualification separate");
}
