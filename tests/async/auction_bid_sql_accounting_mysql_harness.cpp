#include "economy/auction_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "persistence/economic_sql_auction_bid_transaction.h"
#include "persistence/economic_sql_auction_item_claim_transaction.h"
#include "persistence/economic_sql_auction_money_claim_transaction.h"
#include "persistence/economic_sql_auction_settlement_transaction.h"

#include <mysql.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
MYSQL *connection = nullptr;
constexpr uint32_t SELLER = 2147000611U;
constexpr uint32_t FIRST = 2147000612U;
constexpr uint32_t SECOND = 2147000613U;
constexpr uint64_t ITEM = 9900000611ULL;

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
		std::fprintf(stderr, "auction bid accounting SQL error %u: %s\n%s\n",
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

void player(uint32_t pid, const char *name, const char *account)
{
	execute("INSERT INTO accounts(account_name,password) VALUES('" + std::string(account) +
		"','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,"
		"platinum,wallet_revision) VALUES(" +
		std::to_string(pid) + ",'" + name + "','" + account + "',1,0,0,0,10,0)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,"
		"bank_gold,bank_platinum,bank_revision) VALUES('" +
		std::string(account) + "',1,0,0,0,0,0)");
}

critical_command bid(uint8_t operation, uint32_t pid, const char *account, const char *name,
		     int64_t amount, const auction_bid_accounting_listing &listing,
		     const auction_bid_accounting_accounts &keys,
		     const critical_operation_id &epoch, uint64_t wallet_revision = 0,
		     uint64_t bank_revision = 0)
{
	auction_command_payload payload = {};
	payload.action = auction_action::bid;
	payload.auction_id = listing.auction_id;
	payload.actor_pid = pid;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), account, std::strlen(account));
	std::memcpy(payload.actor_name.data(), name, std::strlen(name));
	payload.value = amount;
	payload.expected_wallet_revision = wallet_revision;
	payload.expected_bank_revision = bank_revision;
	payload.closing_fee_basis_points = 300;
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_bid_accounting_intent(command, epoch, listing, keys,
					     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

critical_command claim_item(uint8_t operation, uint32_t auction_id,
			    const auction_item_claim_state &state,
			    const economic_account_key &wallet, const economic_account_key &bank,
			    const critical_operation_id &epoch, uint32_t actor_pid,
			    const char *account, const char *name, uint64_t item_uid,
			    uint64_t wallet_revision, uint64_t bank_revision)
{
	auction_command_payload payload = {};
	payload.action = auction_action::claim_item;
	payload.auction_id = auction_id;
	payload.actor_pid = actor_pid;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), account, std::strlen(account));
	std::memcpy(payload.actor_name.data(), name, std::strlen(name));
	payload.expected_wallet_revision = wallet_revision;
	payload.expected_bank_revision = bank_revision;
	payload.item_count = 1;
	payload.items[0] = { item_uid, 1, 77 };
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_item_claim_accounting_intent(command, epoch, wallet, bank, state,
						    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

critical_command claim_money(uint8_t operation, uint32_t pid, const char *account, const char *name,
			     uint64_t wallet_revision, uint64_t bank_revision,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     const economic_account_key &pending,
			     const auction_money_claim_state &state,
			     const critical_operation_id &epoch)
{
	auction_command_payload payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = pid;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), account, std::strlen(account));
	std::memcpy(payload.actor_name.data(), name, std::strlen(name));
	payload.expected_wallet_revision = wallet_revision;
	payload.expected_bank_revision = bank_revision;
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_money_claim_accounting_intent(command, epoch, wallet, bank, pending, state,
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
} // namespace

int main()
{
	assert(getenv("DB_HOST") && getenv("DB_USER") && getenv("DB_PASSWD") && getenv("DB_NAME") &&
	       getenv("DB_PORT"));
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
	const auto bootstrap = id(1), lineage = id(2), epoch = id(3), listing_op = id(4);
	inbox(bootstrap, 1, 1, 1);
	inbox(listing_op, static_cast<uint16_t>(critical_command_type::auction), 1, 1);
	player(SELLER, "AuctionSeller", "auction_bid_seller");
	const uint64_t seller_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_bid_seller'");
	player(FIRST, "AuctionFirst", "auction_bid_first");
	const uint64_t first_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_bid_first'");
	player(SECOND, "AuctionSecond", "auction_bid_second");
	const uint64_t second_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_bid_second'");
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
		literal(bootstrap) + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ")");
	execute("INSERT INTO auctions(seller_pid,seller_name,status,cur_price,buy_price,"
		"obj_short,obj_vnum,obj_blob_str,quantity,end_time,auction_revision,"
		"custody_state,listing_operation_id) VALUES(" +
		std::to_string(SELLER) +
		",'AuctionSeller',1,2000,5000,'a relic',77,'relic',"
		"1,FROM_UNIXTIME(2000000000),1,1," +
		literal(listing_op) + ")");
	const uint32_t auction_id = mysql_insert_id(connection);
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ITEM) + "," + std::to_string(ITEM) + ",NULL,6," +
		std::to_string(auction_id) + ",0,1,77,1)");
	execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,"
		"obj_blob) VALUES(" +
		std::to_string(auction_id) + ",0," + std::to_string(ITEM) + ",1,77,'relic')");
	execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
		std::to_string(FIRST) + ",0,0),(" + std::to_string(SELLER) + ",0,0)");
	const auto escrow = mapping(lineage, economic_account_kind::auction_escrow, 0, 4,
				    auction_id, bootstrap);
	const auto first_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, FIRST, bootstrap);
	const auto first_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, first_bank, bootstrap);
	const auto second_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, SECOND, bootstrap);
	const auto second_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, second_bank, bootstrap);
	const auto seller_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, SELLER, bootstrap);
	const auto seller_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, seller_bank, bootstrap);
	const auto first_claim =
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, FIRST, bootstrap);
	const auto second_claim =
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, SECOND, bootstrap);
	const auto seller_claim =
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, SELLER, bootstrap);
	auction_bid_accounting_listing before;
	before.auction_id = auction_id;
	before.seller_pid = SELLER;
	before.status = 1;
	before.custody_state = 1;
	before.current_price = 2000;
	before.buy_price = 5000;
	before.revision = 1;
	before.listing_operation = listing_op;
	auction_bid_accounting_accounts first_accounts;
	first_accounts.wallet = first_wallet;
	first_accounts.bank = first_bank_key;
	first_accounts.escrow = escrow;
	first_accounts.bidder_claim = first_claim;
	const auto first = bid(5, FIRST, "auction_bid_first", "AuctionFirst", 3000, before,
			       first_accounts, epoch);
	// Force failure after the native bid and root/effects have been written.
	execute("CREATE TRIGGER fail_auction_accounting_posting BEFORE INSERT ON "
		"economic_accounting_coin_posting FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced accounting failure'");
	execute("START TRANSACTION");
	inbox(first.operation_id, static_cast<uint16_t>(first.type), 2, 0);
	economic_sql_auction_bid_context context;
	assert(economic_sql_auction_bid_lock(connection, first, &context) == 0);
	auction_command_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;
	assert(economic_sql_auction_bid_execute_and_record(connection, first, context, &result,
							   &result_code, &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_accounting_posting");
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
		      literal(first.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(first.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=" +
		      literal(first.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      literal(first.operation_id)) == 0);
	assert(scalar("SELECT cur_price FROM auctions WHERE id=" + std::to_string(auction_id)) ==
	       2000);
	assert(scalar("SELECT auction_revision FROM auctions WHERE id=" +
		      std::to_string(auction_id)) == 1);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(FIRST)) == 10);
	execute("START TRANSACTION");
	inbox(first.operation_id, static_cast<uint16_t>(first.type), 2, 0);
	assert(economic_sql_auction_bid_lock(connection, first, &context) == 0);
	assert(economic_sql_auction_bid_execute_and_record(connection, first, context, &result,
							   &result_code, &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.final_price == 3000);
	receipt(first, result);
	execute("COMMIT");
	assert(scalar("SELECT realized_price_copper FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(first.operation_id)) == 3000);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(first.operation_id)) == 2);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(first.operation_id)) == 2);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(first.operation_id)) == 0);
	before.winning_bidder_pid = FIRST;
	before.current_price = 3000;
	before.revision = 2;
	before.previous_bid_operation = first.operation_id;
	auction_bid_accounting_accounts second_accounts;
	second_accounts.wallet = second_wallet;
	second_accounts.bank = second_bank_key;
	second_accounts.escrow = escrow;
	second_accounts.bidder_claim = second_claim;
	second_accounts.previous_claim = first_claim;
	second_accounts.seller_claim = seller_claim;
	const auto second = bid(6, SECOND, "auction_bid_second", "AuctionSecond", 5000, before,
				second_accounts, epoch);
	execute("CREATE TRIGGER fail_auction_source BEFORE INSERT ON "
		"economic_pending_claim_source FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced pending claim source failure'");
	execute("START TRANSACTION");
	inbox(second.operation_id, static_cast<uint16_t>(second.type), 2, 0);
	assert(economic_sql_auction_bid_lock(connection, second, &context) == 0);
	assert(economic_sql_auction_bid_execute_and_record(connection, second, context, &result,
							   &result_code, &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_source");
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(second.operation_id)) == 0);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(FIRST)) == 0);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == 0);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(SECOND)) ==
	       10);
	execute("CREATE TRIGGER fail_auction_escrow_retirement BEFORE UPDATE ON "
		"economic_account_mapping FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced escrow retirement failure'");
	execute("START TRANSACTION");
	inbox(second.operation_id, static_cast<uint16_t>(second.type), 2, 0);
	assert(economic_sql_auction_bid_lock(connection, second, &context) == 0);
	assert(economic_sql_auction_bid_execute_and_record(connection, second, context, &result,
							   &result_code, &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_escrow_retirement");
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(second.operation_id)) == 0);
	assert(scalar("SELECT cur_price FROM auctions WHERE id=" + std::to_string(auction_id)) ==
	       3000);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(escrow.authority_id) + " AND active_native_id=" +
		      std::to_string(auction_id) + " AND retiring_operation_id IS NULL") == 1);
	execute("START TRANSACTION");
	inbox(second.operation_id, static_cast<uint16_t>(second.type), 2, 0);
	assert(economic_sql_auction_bid_lock(connection, second, &context) == 0);
	assert(economic_sql_auction_bid_execute_and_record(connection, second, context, &result,
							   &result_code, &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::sold);
	receipt(second, result);
	execute("COMMIT");
	assert(scalar("SELECT realized_price_copper FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(second.operation_id)) == 5000);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(FIRST)) == 3000);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == 4850);
	assert(value("SELECT GROUP_CONCAT(CONCAT(source_slot,':',beneficiary_pid,':',amount) "
		     "ORDER BY source_slot) FROM economic_pending_claim_source WHERE "
		     "source_operation_id=" +
		     literal(second.operation_id)) ==
	       "1:" + std::to_string(FIRST) + ":3000,2:" + std::to_string(SELLER) + ":4850");
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(SECOND)) == 5);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(second.operation_id)) == 5);
	assert(value("SELECT GROUP_CONCAT(copper_value ORDER BY event_index) FROM "
		     "economic_accounting_coin_posting WHERE operation_id=" +
		     literal(second.operation_id)) == "-5000,5000,-3000,3000,-5000,4850,150");
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id)) == SECOND);
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 6);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(second.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(second.operation_id)) == 7);
	assert(value("SELECT LOWER(HEX(original_operation_id)) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(second.operation_id)) ==
	       hex(listing_op.bytes.data(), listing_op.bytes.size()));
	assert(value("SELECT LOWER(HEX(SUBSTR(source_event,5,16))) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(second.operation_id)) ==
	       hex(first.operation_id.bytes.data(), first.operation_id.bytes.size()));
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
		      literal(second.operation_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(escrow.authority_id) +
		      " AND active_native_id IS NULL "
		      "AND retiring_operation_id=" +
		      literal(second.operation_id) + " AND revision=1") == 1);
	auction_item_claim_state staged;
	staged.auction_id = auction_id;
	staged.seller_pid = SELLER;
	staged.winner_pid = SECOND;
	staged.claimant_pid = SECOND;
	staged.status = 2;
	staged.custody_state = 1;
	staged.auction_revision = 3;
	staged.listing_operation = listing_op;
	staged.claim_source_operation = second.operation_id;
	staged.item_count = 1;
	staged.rows[0] = { ITEM, 1, 0, 77, SECOND, false };
	const auto item_claim = claim_item(7, auction_id, staged, second_wallet, second_bank_key,
					   epoch, SECOND, "auction_bid_second", "AuctionSecond",
					   ITEM, 1, 1);
	// The native root handoff does not move descendants. Refuse one before it
	// can mutate the parent or record incomplete custody evidence.
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ITEM + 1) + "," + std::to_string(ITEM) + "," + std::to_string(ITEM) +
		",6," + std::to_string(auction_id) + ",0,1,78,1)");
	execute("START TRANSACTION");
	inbox(item_claim.operation_id, static_cast<uint16_t>(item_claim.type), 2, 0);
	economic_sql_auction_item_claim_context claim_context;
	assert(economic_sql_auction_item_claim_lock(connection, item_claim, &claim_context) == 0);
	assert(economic_sql_auction_item_claim_execute_and_record(
		       connection, item_claim, claim_context, &result, &result_code,
		       &mutation_applied) != 0);
	execute("ROLLBACK");
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 6);
	execute("DELETE FROM item_current_owner WHERE item_uid=" + std::to_string(ITEM + 1));
	// Break the final accounting reference after the native custody handoff.
	execute("CREATE TRIGGER fail_auction_claim_reference BEFORE INSERT ON "
		"economic_accounting_item_reference FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced item reference failure'");
	execute("START TRANSACTION");
	inbox(item_claim.operation_id, static_cast<uint16_t>(item_claim.type), 2, 0);
	assert(economic_sql_auction_item_claim_lock(connection, item_claim, &claim_context) == 0);
	assert(economic_sql_auction_item_claim_execute_and_record(
		       connection, item_claim, claim_context, &result, &result_code,
		       &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_claim_reference");
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 6);
	assert(scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 1);
	assert(scalar("SELECT auction_revision FROM auctions WHERE id=" +
		      std::to_string(auction_id)) == 3);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
		      literal(item_claim.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE item_uid=" +
		      std::to_string(ITEM) +
		      " AND claim_operation_id IS NULL AND claimed_at IS NULL") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(item_claim.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      literal(item_claim.operation_id)) == 0);
	execute("START TRANSACTION");
	inbox(item_claim.operation_id, static_cast<uint16_t>(item_claim.type), 2, 0);
	assert(economic_sql_auction_item_claim_lock(connection, item_claim, &claim_context) == 0);
	assert(economic_sql_auction_item_claim_execute_and_record(
		       connection, item_claim, claim_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.item_count == 1 &&
	       result.item_uids[0] == ITEM && result.item_revisions[0] == 2);
	receipt(item_claim, result);
	execute("COMMIT");
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == 1);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM)) == SECOND);
	assert(scalar("SELECT item_revision FROM auction_item_custody WHERE item_uid=" +
		      std::to_string(ITEM)) == 2);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE item_uid=" +
		      std::to_string(ITEM) + " AND claim_operation_id=" +
		      literal(item_claim.operation_id) + " AND claimed_at IS NOT NULL") == 1);
	assert(scalar("SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
		      literal(item_claim.operation_id) +
		      " AND event_index=0 AND item_uid=" + std::to_string(ITEM) +
		      " AND from_owner_type=6 AND from_owner_id=" + std::to_string(auction_id) +
		      " AND to_owner_type=1 AND to_owner_id=" + std::to_string(SECOND) +
		      " AND item_revision=2") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(item_claim.operation_id) + " AND item_uid=" + std::to_string(ITEM) +
		      " AND before_revision=1 AND after_revision=2 "
		      "AND legacy_operation_id=" +
		      literal(item_claim.operation_id) + " AND legacy_event_index=0") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(item_claim.operation_id)) == 0);
	assert(scalar("SELECT item_event_count FROM economic_accounting_operation WHERE "
		      "operation_id=" +
		      literal(item_claim.operation_id)) == 1);
	assert(value("SELECT LOWER(HEX(SUBSTR(source_event,5,16))) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(item_claim.operation_id)) ==
	       hex(second.operation_id.bytes.data(), second.operation_id.bytes.size()));
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
		      literal(item_claim.operation_id)) == 1);
	// Collect both positive claim legs produced by the same buy-now bid. The
	// source operation is shared; its slots distinguish refund from proceeds.
	auction_money_claim_state refund_state;
	refund_state.beneficiary_pid = FIRST;
	refund_state.money = 3000;
	refund_state.revision = 1;
	refund_state.sources = { { second.operation_id, 1, FIRST, first_claim.authority_id,
				   3000 } };
	const auto refund = claim_money(8, FIRST, "auction_bid_first", "AuctionFirst", 1, 1,
					first_wallet, first_bank_key, first_claim, refund_state,
					epoch);
	execute("START TRANSACTION");
	inbox(refund.operation_id, static_cast<uint16_t>(refund.type), 2, 0);
	economic_sql_auction_money_claim_context money_context;
	assert(economic_sql_auction_money_claim_lock(connection, refund, &money_context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(
		       connection, refund, money_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.wallet_value_delta == 3000);
	receipt(refund, result);
	execute("COMMIT");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(FIRST)) == 0);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(FIRST)) == 10);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id=" +
		      literal(refund.operation_id) + " AND source_operation_id=" +
		      literal(second.operation_id) + " AND source_slot=1") == 1);
	auction_money_claim_state proceeds_state;
	proceeds_state.beneficiary_pid = SELLER;
	proceeds_state.money = 4850;
	proceeds_state.revision = 1;
	proceeds_state.sources = { { second.operation_id, 2, SELLER, seller_claim.authority_id,
				     4850 } };
	const auto proceeds = claim_money(9, SELLER, "auction_bid_seller", "AuctionSeller", 0, 0,
					  seller_wallet, seller_bank_key, seller_claim,
					  proceeds_state, epoch);
	execute("START TRANSACTION");
	inbox(proceeds.operation_id, static_cast<uint16_t>(proceeds.type), 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, proceeds, &money_context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(
		       connection, proceeds, money_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.wallet_value_delta == 4850);
	receipt(proceeds, result);
	execute("COMMIT");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == 0);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(SELLER)) ==
	       14);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id=" +
		      literal(proceeds.operation_id) + " AND source_operation_id=" +
		      literal(second.operation_id) + " AND source_slot=2") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE "
		      "operation_id IN (" +
		      literal(refund.operation_id) + "," + literal(proceeds.operation_id) + ")") ==
	       2);
	assert(value("SELECT HEX(source_event) FROM economic_accounting_operation WHERE "
		     "operation_id=" +
		     literal(refund.operation_id)) !=
	       value("SELECT HEX(source_event) FROM economic_accounting_operation WHERE "
		     "operation_id=" +
		     literal(proceeds.operation_id)));
	for (const auto &collection : { refund.operation_id, proceeds.operation_id })
	{
		assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting "
			      "WHERE operation_id=" +
			      literal(collection)) == 0);
		assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
			      literal(collection)) == 1);
	}
	// A retained context cannot run after the active epoch changes.
	execute("UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 "
		"WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_auction_bid_execute_and_record(connection, second, context, &result,
							   &result_code, &mutation_applied) != 0);
	assert(economic_sql_auction_item_claim_execute_and_record(
		       connection, item_claim, claim_context, &result, &result_code,
		       &mutation_applied) != 0);
	assert(economic_sql_auction_money_claim_execute_and_record(
		       connection, proceeds, money_context, &result, &result_code,
		       &mutation_applied) != 0);
	execute("ROLLBACK");
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(second.operation_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(second.operation_id) + " AND claim_operation_id IS NOT NULL") == 2);
	assert(scalar("SELECT LENGTH(canonical_plan) FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(item_claim.operation_id)) > 0);
	assert(scalar("SELECT LENGTH(result_payload) FROM critical_operation_inbox "
		      "WHERE operation_id=" +
		      literal(item_claim.operation_id)) == AUCTION_RESULT_PAYLOAD_BYTES);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(item_claim.operation_id)) == 1);
	mysql_close(connection);
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
	assert(scalar("SELECT LENGTH(canonical_intent) FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(second.operation_id)) > 0);
	assert(scalar("SELECT LENGTH(canonical_plan) FROM economic_accounting_operation "
		      "WHERE operation_id=" +
		      literal(second.operation_id)) > 0);
	assert(scalar("SELECT LENGTH(result_payload) FROM critical_operation_inbox "
		      "WHERE operation_id=" +
		      literal(second.operation_id)) == AUCTION_RESULT_PAYLOAD_BYTES);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(second.operation_id)) == 1);
	mysql_close(connection);
	return 0;
}
