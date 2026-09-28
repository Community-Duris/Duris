#include "economy/auction_listing_accounting.h"
#include "persistence/economic_sql_auction_listing_transaction.h"

#include <mysql.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
MYSQL *connection = nullptr;
constexpr uint32_t SELLER = 2147000711U;
constexpr uint64_t ITEM = 9900000711ULL;
constexpr uint64_t SECOND_ITEM = 9900000712ULL;

critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

std::string hex(const uint8_t *bytes, size_t size)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		output[index * 2] = digits[bytes[index] >> 4];
		output[index * 2 + 1] = digits[bytes[index] & 15];
	}
	return output;
}

std::string literal(const critical_operation_id &operation)
{
	return "X'" + hex(operation.bytes.data(), operation.bytes.size()) + "'";
}

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()))
		std::fprintf(stderr, "auction listing accounting SQL error %u: %s\n%s\n",
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
	const uint64_t result = strtoull(row[0], nullptr, 10);
	mysql_free_result(rows);
	return result;
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

void inbox(const critical_operation_id &operation, uint16_t type, uint16_t schema, uint8_t status)
{
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		literal(operation) + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32)," +
		std::to_string(type) + "," + std::to_string(schema) + ",1," +
		std::to_string(status) + ",X'')");
}

economic_account_key mapping(const critical_operation_id &lineage, economic_account_kind kind,
			     uint64_t context, uint16_t locator, uint64_t native,
			     const critical_operation_id &bootstrap)
{
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		literal(lineage) + "," + std::to_string(static_cast<uint16_t>(kind)) + "," +
		std::to_string(context) + ",1," + std::to_string(locator) + "," +
		std::to_string(native) + "," + std::to_string(native) + "," + literal(bootstrap) +
		")");
	return { lineage, kind, mysql_insert_id(connection), context };
}

critical_command listing(uint8_t operation, const critical_operation_id &epoch,
			 const economic_account_key &wallet, const economic_account_key &bank,
			 uint64_t item_revision)
{
	auction_command_payload payload = {};
	payload.action = auction_action::list;
	payload.actor_pid = SELLER;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "auction_listing_seller", 22);
	std::memcpy(payload.actor_name.data(), "AuctionSeller", 13);
	payload.start_price = 2000;
	payload.buy_price = 5000;
	payload.listing_fee = 200;
	payload.end_time = 2000000000;
	payload.item_count = 2;
	payload.items[0] = { ITEM, item_revision, 77 };
	payload.items[1] = { SECOND_ITEM, item_revision, 78 };
	std::memcpy(payload.object_blob.data(), "relic", 5);
	payload.object_blob_size = 5;
	std::memcpy(payload.object_short.data(), "a relic", 7);
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_listing_accounting_intent(command, epoch, wallet, bank,
						 &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

void receipt(const critical_command &command, const auction_command_result &result)
{
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded = {};
	assert(auction_command_encode_result(result, &encoded));
	execute("INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,"
		"payload_version,payload) VALUES(" +
		literal(command.operation_id) + ",0,11,1,1,X'" +
		hex(encoded.data(), encoded.size()) + "')");
	execute("UPDATE critical_operation_inbox SET status=1,result_code=0,durable_revision=" +
		std::to_string(result.auction_revision) + ",result_payload=X'" +
		hex(encoded.data(), encoded.size()) +
		"' WHERE operation_id=" + literal(command.operation_id) + " AND status=0");
	assert(mysql_affected_rows(connection) == 1);
}

void connect()
{
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
}
} // namespace

int main()
{
	assert(getenv("DB_HOST") && getenv("DB_USER") && getenv("DB_PASSWD") && getenv("DB_NAME") &&
	       getenv("DB_PORT"));
	connect();
	const auto bootstrap = id(41), lineage = id(42), epoch = id(43);
	inbox(bootstrap, 1, 1, 1);
	execute("INSERT INTO accounts(account_name,password) VALUES('auction_listing_seller','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,"
		"platinum,wallet_revision) VALUES(" +
		std::to_string(SELLER) + ",'AuctionSeller','auction_listing_seller',1,0,0,0,10,0)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,"
		"bank_gold,bank_platinum,bank_revision) VALUES('auction_listing_seller',1,0,0,0,0,0)");
	const auto native_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_listing_seller'");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ITEM) + "," + std::to_string(ITEM) + ",NULL,1," +
		std::to_string(SELLER) + ",0,4,77,1)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(SECOND_ITEM) + "," + std::to_string(SECOND_ITEM) + ",NULL,1," +
		std::to_string(SELLER) + ",0,4,78,1)");
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
		literal(bootstrap) + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ")");
	const auto wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, SELLER, bootstrap);
	const auto bank =
		mapping(lineage, economic_account_kind::bank, 1, 2, native_bank, bootstrap);
	const auto command = listing(44, epoch, wallet, bank, 4);
	// Force failure after the native listing, wallet charge, and escrow creation.
	execute("CREATE TRIGGER fail_listing_reference BEFORE INSERT ON "
		"economic_accounting_item_reference FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced listing reference failure'");
	execute("START TRANSACTION");
	inbox(command.operation_id, static_cast<uint16_t>(command.type), 2, 0);
	economic_sql_auction_listing_context context;
	assert(economic_sql_auction_listing_lock(connection, command, &context) == 0);
	auction_command_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;
	const auto failed = economic_sql_auction_listing_execute_and_record(
		connection, command, context, &result, &result_code, &mutation_applied);
	if (!failed)
		std::fprintf(stderr, "forced listing reference failure was not observed\n");
	assert(failed != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_listing_reference");
	assert(scalar("SELECT COUNT(*) FROM auctions WHERE listing_operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE "
		      "creating_operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 1);
	assert(scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 4);
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(SECOND_ITEM)) == 1);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(SELLER)) ==
	       10);
	assert(scalar("SELECT wallet_revision FROM player_data WHERE pid=" +
		      std::to_string(SELLER)) == 0);
	assert(scalar("SELECT bank_revision FROM account_banks WHERE account_name="
		      "'auction_listing_seller'") == 0);
	assert(scalar("SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=1 AND owner_id=" +
		      std::to_string(SELLER)) == 0);
	execute("START TRANSACTION");
	inbox(command.operation_id, static_cast<uint16_t>(command.type), 2, 0);
	assert(economic_sql_auction_listing_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_listing_execute_and_record(connection, command, context,
							       &result, &result_code,
							       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.auction_id &&
	       result.item_count == 2 && result.item_uids[0] == ITEM &&
	       result.item_revisions[0] == 5 && result.item_uids[1] == SECOND_ITEM &&
	       result.item_revisions[1] == 5);
	receipt(command, result);
	execute("COMMIT");
	const auto auction_id = result.auction_id;
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 6);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == auction_id);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(SECOND_ITEM)) == auction_id);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(SELLER)) == 0);
	assert(scalar("SELECT gold FROM player_data WHERE pid=" + std::to_string(SELLER)) == 8);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(SELLER)) == 9);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id) + " AND item_uid=" + std::to_string(ITEM) +
		      " AND item_revision=5") == 1);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id) + " AND item_uid=" + std::to_string(SECOND_ITEM) +
		      " AND item_revision=5 AND slot=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE account_kind=4 "
		      "AND locator_kind=4 AND native_id=" +
		      std::to_string(auction_id) +
		      " AND creating_operation_id=" + literal(command.operation_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 3);
	assert(value("SELECT GROUP_CONCAT(copper_value ORDER BY event_index) FROM "
		     "economic_accounting_coin_posting WHERE operation_id=" +
		     literal(command.operation_id)) == "-200,200");
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(command.operation_id) + " AND item_uid=" + std::to_string(ITEM) +
		      " AND before_revision=4 AND after_revision=5 AND legacy_operation_id=" +
		      literal(command.operation_id) + " AND legacy_event_index=0") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(command.operation_id) +
		      " AND item_uid=" + std::to_string(SECOND_ITEM) +
		      " AND before_revision=4 AND after_revision=5 AND legacy_operation_id=" +
		      literal(command.operation_id) + " AND legacy_event_index=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
		      literal(command.operation_id) +
		      " AND event_index=0 AND item_uid=" + std::to_string(ITEM) +
		      " AND from_owner_type=1 AND from_owner_id=" + std::to_string(SELLER) +
		      " AND to_owner_type=6 AND to_owner_id=" + std::to_string(auction_id) +
		      " AND item_revision=5") == 1);
	assert(scalar("SELECT item_event_count FROM economic_accounting_operation WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 2);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
		      literal(command.operation_id)) == 1);
	assert(scalar("SELECT LENGTH(result_payload) FROM critical_operation_inbox WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == AUCTION_RESULT_PAYLOAD_BYTES);
	// An old lock cannot run after the active epoch is paused.
	execute("UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 "
		"WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_auction_listing_execute_and_record(connection, command, context,
							       &result, &result_code,
							       &mutation_applied) != 0);
	execute("ROLLBACK");
	mysql_close(connection);
	connect();
	// The persisted inbox key rejects a duplicate after reconnect, and the
	// original receipt still identifies the same auction and item revisions.
	execute("INSERT IGNORE INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		literal(command.operation_id) +
		",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,2,1,0,X'')");
	assert(mysql_affected_rows(connection) == 0);
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded_result = {};
	assert(auction_command_encode_result(result, &encoded_result));
	assert(value("SELECT LOWER(HEX(result_payload)) FROM critical_operation_inbox WHERE "
		     "operation_id=" +
		     literal(command.operation_id)) ==
	       hex(encoded_result.data(), encoded_result.size()));
	assert(scalar("SELECT COUNT(*) FROM auctions WHERE listing_operation_id=" +
		      literal(command.operation_id)) == 1);
	assert(scalar("SELECT LENGTH(canonical_plan) FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(command.operation_id)) > 0);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(command.operation_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 2);
	mysql_close(connection);
	return 0;
}
