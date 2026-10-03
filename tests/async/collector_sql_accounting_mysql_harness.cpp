#include "economy/collector_accounting.h"
#include "persistence/economic_sql_collector_transaction.h"
#include "persistence/critical_command_repository.h"
#include "player/player_snapshot_codec.h"

#include <mysql.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

namespace
{
MYSQL *connection = nullptr;
constexpr uint32_t PID = 2147000880;
constexpr const char *ACCOUNT = "collector_accounting_fixture";

std::string hex(const uint8_t *bytes, size_t size)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value;
	value.reserve(size * 2);
	for (size_t index = 0; index < size; ++index)
	{
		value += digits[bytes[index] >> 4];
		value += digits[bytes[index] & 15];
	}
	return value;
}

std::string literal(const critical_operation_id &operation)
{
	return "X'" + hex(operation.bytes.data(), operation.bytes.size()) + "'";
}

critical_operation_id new_id()
{
	critical_operation_id value = {};
	assert(critical_operation_id_generate(&value));
	return value;
}

std::string id_hex(const critical_operation_id &operation)
{
	return hex(operation.bytes.data(), operation.bytes.size());
}

void execute(const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		fprintf(stderr, "collector accounting SQL failed: %u %s\n%s\n",
			mysql_errno(connection), mysql_error(connection), sql.c_str());
	assert(mysql_errno(connection) == 0);
}

uint64_t scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows && mysql_num_rows(rows) == 1);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const uint64_t value = strtoull(row[0], nullptr, 10);
	mysql_free_result(rows);
	return value;
}

std::string value(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows && mysql_num_rows(rows) == 1);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const std::string result = row[0];
	mysql_free_result(rows);
	return result;
}

void seed_inbox(const critical_operation_id &operation, uint16_t type, uint16_t schema,
		uint8_t status)
{
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		literal(operation) + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32)," +
		std::to_string(type) + "," + std::to_string(schema) + ",1," +
		std::to_string(status) + ",X'')");
}

std::vector<uint8_t> item_blob(uint64_t uid)
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = uid;
	item.vnum = 1201;
	item.type = 15;
	item.string_mask = 15;
	item.name = "collector accounting relic";
	item.short_description = "a collector accounting relic";
	item.description = "A collector accounting relic rests here.";
	item.action_description = "collector-accounting-action";
	item.timers.fill(-1);
	item.weight = 5;
	item.cost = 250;
	item.condition = 77;
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode({ item }, &encoded) ==
	       player_snapshot_codec_result::ok);
	return encoded;
}

collector::record seed_listing(uint64_t listing, uint64_t uid, bool available,
			       const std::vector<uint8_t> &blob)
{
	const critical_operation_id death = new_id();
	seed_inbox(death, 16, 1, 1);
	const uint64_t death_time = 1700000000 + listing;
	execute("INSERT INTO collector_deaths(death_operation_id,beneficiary_pid,death_time) "
		"VALUES(" +
		literal(death) + "," + std::to_string(PID) + "," + std::to_string(death_time) +
		")");
	collector::rules rules;
	rules.enabled = true;
	collector::record record;
	assert(collector::enroll(listing, id_hex(death), PID, uid, 5, death_time, rules, &record) ==
	       collector::outcome::applied);
	assert(collector::collect(&record, 1, 5, 5, true, 250, record.collect_at) ==
	       collector::outcome::applied);
	if (available)
		assert(collector::activate(&record, 2, record.sale_at) ==
		       collector::outcome::applied);
	std::array<uint8_t, collector::encoded_record_bytes> encoded = {};
	assert(collector::record_encode(record, &encoded) == collector::codec_result::ok);
	const uint64_t due = available ? record.expires_at : record.sale_at;
	execute("INSERT INTO collector_listings(listing_id,death_operation_id,beneficiary_pid,"
		"item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,"
		"record_blob,item_blob) VALUES(" +
		std::to_string(listing) + "," + literal(death) + "," + std::to_string(PID) + "," +
		std::to_string(uid) + "," +
		std::to_string(static_cast<unsigned int>(record.status)) + ",0," +
		std::to_string(due) + "," + std::to_string(record.revision) + "," +
		std::to_string(record.item_revision) + "," + std::to_string(record.price_value) +
		",X'" + hex(encoded.data(), encoded.size()) + "',X'" +
		hex(blob.data(), blob.size()) + "')");
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(10," +
		std::to_string(listing) + ",0,0)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(uid) + "," + std::to_string(uid) + ",NULL,10," +
		std::to_string(listing) + ",0," + std::to_string(record.item_revision) +
		",1201,1)");
	return record;
}

critical_command purchase_command(const collector::record &listing,
				  const std::vector<uint8_t> &blob,
				  const critical_operation_id &epoch,
				  const economic_account_key &wallet,
				  const economic_account_key &bank, uint64_t wallet_revision = 0,
				  uint64_t bank_revision = 0, uint64_t player_owner_revision = 0)
{
	collector_command_payload payload;
	payload.action = collector_action::purchase;
	payload.listing = listing.listing;
	payload.expected_listing_revision = listing.revision;
	payload.observed_at = listing.available_at;
	payload.actor_pid = PID;
	payload.racewar = 1;
	memcpy(payload.account_name.data(), ACCOUNT, strlen(ACCOUNT));
	payload.capacity_admitted = true;
	payload.expected_wallet_revision = wallet_revision;
	payload.expected_bank_revision = bank_revision;
	payload.from_owner = { item_owner_type::collector, listing.listing, 0 };
	payload.to_owner = { item_owner_type::player, PID, 0 };
	payload.expected_from_owner_revision = 0;
	payload.expected_to_owner_revision = player_owner_revision;
	payload.selected_item_uid = listing.uid;
	payload.target_state = item_custody_state::active;
	payload.item_count = 1;
	payload.items[0] = { listing.uid,	    listing.uid, 0,
			     listing.item_revision, 1201,	 item_custody_state::active };
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	critical_command command;
	assert(collector_command_build(&command, new_id(), payload, critical_source_site::command,
				       critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(collector_purchase_accounting_intent(command, epoch, wallet, bank, listing,
						    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

critical_command held_command(const collector::record &listing, const std::vector<uint8_t> &blob,
			      const critical_operation_id &lineage,
			      const critical_operation_id &epoch, collector_action action,
			      collector::reason reason = collector::reason::none)
{
	collector_command_payload payload;
	payload.action = action;
	payload.cancel_reason = reason;
	payload.listing = listing.listing;
	payload.expected_listing_revision = listing.revision;
	payload.observed_at = action == collector_action::expire ? listing.expires_at :
								   listing.available_at;
	payload.from_owner = { item_owner_type::collector, listing.listing, 0 };
	payload.to_owner = reason == collector::reason::quarantined ?
				   item_owner_identity{ item_owner_type::system, 0, 0 } :
				   item_owner_identity{ item_owner_type::destruction, 0, 0 };
	payload.selected_item_uid = listing.uid;
	payload.target_state = reason == collector::reason::quarantined ?
				       item_custody_state::quarantined :
				       item_custody_state::destroyed;
	payload.item_count = 1;
	payload.items[0] = { listing.uid,	    listing.uid, 0,
			     listing.item_revision, 1201,	 item_custody_state::active };
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	critical_command command;
	assert(collector_command_build(&command, new_id(), payload,
				       critical_source_site::zone_event,
				       critical_deadline_class::background));
	command.accepted_at_usec = 1;
	assert(collector_held_accounting_intent(command, lineage, epoch, listing,
						&command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

} // namespace

int main()
{
	connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("DB_NAME"),
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
	execute("INSERT INTO accounts(account_name,password) VALUES('" + std::string(ACCOUNT) +
		"','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES(" +
		std::to_string(PID) + ",'CollectorAccounting','" + ACCOUNT + "',1,0,0,10,0)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper) VALUES('" +
		std::string(ACCOUNT) + "',1,0)");
	const uint64_t bank_id = mysql_insert_id(connection);
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1," +
		std::to_string(PID) + ",0,0)");
	execute("UPDATE collector_catalog_state SET next_listing=10000 WHERE state_id=1");
	const auto lineage = new_id(), epoch = new_id(), bootstrap = new_id();
	seed_inbox(bootstrap, 1, 1, 1);
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
		literal(bootstrap) + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ")");
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		literal(lineage) + ",1,0,1,1," + std::to_string(PID) + "," + std::to_string(PID) +
		"," + literal(bootstrap) + ")");
	const economic_account_key wallet = { lineage, economic_account_kind::wallet,
					      mysql_insert_id(connection), 0 };
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		literal(lineage) + ",2,1,1,2," + std::to_string(bank_id) + "," +
		std::to_string(bank_id) + "," + literal(bootstrap) + ")");
	const economic_account_key bank = { lineage, economic_account_kind::bank,
					    mysql_insert_id(connection), 1 };
	const auto blob = item_blob(9400000880ULL);
	const auto listing = seed_listing(8001, 9400000880ULL, true, blob);
	const auto command = purchase_command(listing, blob, epoch, wallet, bank);
	const critical_apply_result applied =
		critical_command_repository_apply(connection, command);
	if (applied.outcome != critical_apply_outcome::applied)
		fprintf(stderr, "collector root apply failed: %u outcome=%u mysql=%u %s\n",
			applied.error_code, static_cast<unsigned int>(applied.outcome),
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(scalar("SELECT realized_price_copper FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(command.operation_id)) == static_cast<uint64_t>(listing.price_value));
	collector_command_result result = {};
	assert(collector_command_decode_result(applied.result_payload.data(), applied.result_size,
					       &result));
	economic_sql_collector_context context;
	unsigned int result_code = 0;
	bool mutation_applied = false;
	const std::string where = " WHERE operation_id=" + literal(command.operation_id);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation" + where) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect" + where) == 3);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting" + where) == 2);
	assert(scalar("SELECT COALESCE(SUM(copper_value),1) FROM "
		      "economic_accounting_coin_posting" +
		      where) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference" + where) == 1);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger" + where) == 1);
	assert(value("SELECT CONCAT(item_uid,':',before_revision,':',after_revision,':',"
		     "legacy_event_index) FROM economic_accounting_item_reference" +
		     where) == "9400000880:6:7:0");
	assert(scalar("SELECT COUNT(*) FROM collector_ledger" + where) == 1);
	mysql_close(connection);
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
	assert(scalar("SELECT COUNT(*) FROM critical_outbox" + where) == 1);
	assert(scalar("SELECT status FROM critical_operation_inbox" + where) == 1);
	assert(scalar("SELECT status FROM collector_listings WHERE listing_id=8001") ==
	       static_cast<unsigned int>(collector::state::purchased));
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=9400000880") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation" + where) == 1);
	assert(value("SELECT LOWER(HEX(canonical_intent)) FROM "
		     "economic_accounting_operation" +
		     where) ==
	       hex(command.accounting_intent.data(), command.accounting_intent.size()));
	assert(value("SELECT LOWER(HEX(original_operation_id)) FROM "
		     "economic_accounting_operation" +
		     where) == listing.death_operation.data());
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> retained_purchase = {};
	assert(collector_command_encode_result(result, &retained_purchase));
	assert(economic_sql_collector_verify_retained(connection, command, 0,
						      retained_purchase.data(),
						      retained_purchase.size()) == 0);
	assert(critical_command_repository_reconcile(connection, command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, command).outcome ==
	       critical_apply_outcome::already_applied);
	execute("UPDATE economic_accounting_operation SET realized_price_copper=" +
		std::to_string(listing.price_value + 1) +
		" WHERE operation_id=" + literal(command.operation_id));
	assert(economic_sql_collector_verify_retained(connection, command, 0,
						      retained_purchase.data(),
						      retained_purchase.size()) != 0);
	execute("UPDATE economic_accounting_operation SET realized_price_copper=" +
		std::to_string(listing.price_value) +
		" WHERE operation_id=" + literal(command.operation_id));
	assert(economic_sql_collector_verify_retained(connection, command, 0,
						      retained_purchase.data(),
						      retained_purchase.size()) == 0);
	execute("UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 "
		"WHERE operation_id=" +
		literal(command.operation_id) + " AND line_index=0");
	assert(economic_sql_collector_verify_retained(connection, command, 0,
						      retained_purchase.data(),
						      retained_purchase.size()) != 0);
	assert(critical_command_repository_apply(connection, command).outcome ==
	       critical_apply_outcome::retryable_failure);
	execute("UPDATE economic_accounting_coin_posting SET copper_value=copper_value-1 "
		"WHERE operation_id=" +
		literal(command.operation_id) + " AND line_index=0");
	assert(economic_sql_collector_verify_retained(connection, command, 0,
						      retained_purchase.data(),
						      retained_purchase.size()) == 0);
	assert(critical_command_repository_reconcile(connection, command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(value("SELECT LOWER(HEX(result_payload)) FROM critical_operation_inbox" + where) ==
	       hex(retained_purchase.data(), retained_purchase.size()));
	const std::string wallet_after_purchase =
		value("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',wallet_revision) "
		      "FROM player_data WHERE pid=" +
		      std::to_string(PID));
	const auto stale = purchase_command(listing, blob, epoch, wallet, bank);
	const critical_apply_result rejected = critical_command_repository_apply(connection, stale);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure);
	assert(rejected.error_code == ESTALE);
	assert(critical_command_repository_apply(connection, stale).outcome ==
	       critical_apply_outcome::terminal_failure);
	const std::string stale_where = " WHERE operation_id=" + literal(stale.operation_id);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation" + stale_where) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting" + stale_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference" + stale_where) ==
	       0);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox" + stale_where) == 0);

	// Held expiry is a separate linked root with one destruction event and no money.
	const auto expire_blob = item_blob(9400000882ULL);
	const auto expiring = seed_listing(8003, 9400000882ULL, true, expire_blob);
	execute("INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,"
		"revision) VALUES(8,0,0,0)");
	const auto expire =
		held_command(expiring, expire_blob, lineage, epoch, collector_action::expire);
	const critical_apply_result expired_root =
		critical_command_repository_apply(connection, expire);
	assert(expired_root.outcome == critical_apply_outcome::applied);
	assert(collector_command_decode_result(expired_root.result_payload.data(),
					       expired_root.result_size, &result));
	const std::string expire_where = " WHERE operation_id=" + literal(expire.operation_id);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect" + expire_where) ==
	       0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting" + expire_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference" + expire_where) ==
	       1);
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> retained_expire = {};
	assert(collector_command_encode_result(result, &retained_expire));
	assert(economic_sql_collector_verify_retained(connection, expire, 0, retained_expire.data(),
						      retained_expire.size()) == 0);
	assert(critical_command_repository_reconcile(connection, expire).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, expire).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox" + expire_where) == 1);
	assert(value("SELECT LOWER(HEX(original_operation_id)) FROM "
		     "economic_accounting_operation" +
		     expire_where) == expiring.death_operation.data());
	assert(value("SELECT CONCAT(owner_type,':',state,':',item_revision) FROM "
		     "item_current_owner WHERE item_uid=9400000882") == "8:2:7");
	assert(value("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',"
		     "wallet_revision) FROM player_data WHERE pid=" +
		     std::to_string(PID)) == wallet_after_purchase);

	// Cancellation is its own item-only root and preserves the original UID in
	// quarantine. It must not debit the buyer or issue collector proceeds.
	const auto cancel_blob = item_blob(9400000883ULL);
	const auto cancellable = seed_listing(8004, 9400000883ULL, true, cancel_blob);
	execute("INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,"
		"revision) VALUES(7,0,0,0)");
	const auto cancel = held_command(cancellable, cancel_blob, lineage, epoch,
					 collector_action::cancel, collector::reason::quarantined);
	const critical_apply_result cancelled_root =
		critical_command_repository_apply(connection, cancel);
	assert(cancelled_root.outcome == critical_apply_outcome::applied);
	const std::string cancel_where = " WHERE operation_id=" + literal(cancel.operation_id);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect" + cancel_where) ==
	       0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting" + cancel_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference" + cancel_where) ==
	       1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox" + cancel_where) == 1);
	assert(value("SELECT CONCAT(owner_type,':',state,':',item_revision) FROM "
		     "item_current_owner WHERE item_uid=9400000883") == "7:3:7");
	assert(critical_command_repository_reconcile(connection, cancel).outcome ==
	       critical_apply_outcome::already_applied);
	assert(critical_command_repository_apply(connection, cancel).outcome ==
	       critical_apply_outcome::already_applied);
	assert(value("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',"
		     "wallet_revision) FROM player_data WHERE pid=" +
		     std::to_string(PID)) == wallet_after_purchase);

	const auto second_blob = item_blob(9400000881ULL);
	const auto second = seed_listing(8002, 9400000881ULL, true, second_blob);
	const auto failure = purchase_command(second, second_blob, epoch, wallet, bank, 1, 1, 1);
	execute("CREATE TRIGGER fail_accounting_reference BEFORE INSERT ON "
		"economic_accounting_item_reference FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced accounting failure'");
	const critical_apply_result failed_root =
		critical_command_repository_apply(connection, failure);
	assert(failed_root.outcome == critical_apply_outcome::retryable_failure);
	execute("DROP TRIGGER fail_accounting_reference");
	const auto failed_where = " WHERE operation_id=" + literal(failure.operation_id);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox" + failed_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM collector_ledger" + failed_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation" + failed_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger" + failed_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=9400000881") == 0);
	assert(scalar("SELECT status FROM collector_listings WHERE listing_id=8002") ==
	       static_cast<unsigned int>(collector::state::available));
	assert(value("SELECT CONCAT(copper,':',silver,':',gold,':',platinum,':',"
		     "wallet_revision) FROM player_data WHERE pid=" +
		     std::to_string(PID)) == wallet_after_purchase);
	// A context retained across transactions cannot bypass an epoch transition.
	execute("START TRANSACTION");
	assert(economic_sql_collector_lock(connection, failure, &context) == 0);
	execute("ROLLBACK");
	execute("UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 "
		"WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_collector_execute_and_record(connection, failure, context, &result,
							 &result_code, &mutation_applied) != 0);
	execute("ROLLBACK");
	assert(scalar("SELECT COUNT(*) FROM collector_ledger" + failed_where) == 0);
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=9400000881") == 0);
	mysql_close(connection);
	return 0;
}
