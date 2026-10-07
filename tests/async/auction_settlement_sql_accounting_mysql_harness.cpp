#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "persistence/economic_sql_auction_item_claim_transaction.h"
#include "persistence/economic_sql_auction_settlement_transaction.h"
#ifdef DURIS_TELEMETRY_AUCTION_QUALIFICATION
#include "economy/auction_money_claim_accounting.h"
#include "persistence/economic_sql_auction_money_claim_transaction.h"
#endif

#include <mysql.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
MYSQL *connection = nullptr;
constexpr uint32_t SELLER = 2147000711U;
constexpr uint32_t WINNER = 2147000712U;
constexpr uint32_t TRUSTED = 2147000713U;
constexpr uint64_t ITEM = 9900000711ULL;
#ifdef DURIS_TELEMETRY_AUCTION_QUALIFICATION
constexpr uint64_t INITIAL_PENDING = 0;

void require_telemetry_disposable_target()
{
	const auto equals = [](const char *name, const char *value)
	{
		const char *actual = std::getenv(name);
		return actual && std::strcmp(actual, value) == 0;
	};
	assert(equals("TEST_DB_DISPOSABLE", "1") &&
	       equals("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "1") &&
	       equals("DB_HOST", "127.0.0.1"));
	const char *socket = std::getenv("DB_SOCKET");
	assert(!socket || !*socket);
	const char *database = std::getenv("DB_NAME");
	constexpr char prefix[] = "economic_schema_test_";
	assert(database && std::strncmp(database, prefix, sizeof(prefix) - 1) == 0 &&
	       database[sizeof(prefix) - 1]);
	for (const char *cursor = database; *cursor; ++cursor)
		assert((*cursor >= 'a' && *cursor <= 'z') || (*cursor >= 'A' && *cursor <= 'Z') ||
		       (*cursor >= '0' && *cursor <= '9') || *cursor == '_');
	const char *port = std::getenv("DB_PORT");
	assert(port && *port);
	for (const char *cursor = port; *cursor; ++cursor)
		assert(*cursor >= '0' && *cursor <= '9');
	const auto number = std::strtoul(port, nullptr, 10);
	assert(number > 0 && number <= 65535);
	const char *sources = std::getenv("TELEMETRY_AUCTION_CLAIM_SOURCES");
	assert(!sources || std::strcmp(sources, "2") == 0 || std::strcmp(sources, "128") == 0 ||
	       std::strcmp(sources, "129") == 0);
}
#else
constexpr uint64_t INITIAL_PENDING = 100;
#endif

critical_operation_id id(uint32_t first)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < sizeof(first); ++index)
		value.bytes[index] = static_cast<uint8_t>(first >> (index * 8));
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
		std::fprintf(stderr, "auction settlement SQL error %u: %s\n%s\n",
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

uint32_t auction(const critical_operation_id &listing_op, const critical_operation_id &bid_op,
		 uint64_t uid, uint64_t end_time)
{
	inbox(listing_op, static_cast<uint16_t>(critical_command_type::auction), 1, 1);
	if (!critical_operation_id_is_zero(bid_op))
		inbox(bid_op, static_cast<uint16_t>(critical_command_type::auction), 1, 1);
	const bool won = !critical_operation_id_is_zero(bid_op);
	execute("INSERT INTO auctions(seller_pid,seller_name,status,winning_bidder_pid,"
		"winning_bidder_name,cur_price,buy_price,obj_short,obj_vnum,obj_blob_str,"
		"quantity,end_time,auction_revision,custody_state,listing_operation_id) VALUES(" +
		std::to_string(SELLER) + ",'AuctionSeller',1," + std::to_string(won ? WINNER : 0) +
		",'" + (won ? "AuctionWinner" : "") + "'," + std::to_string(won ? 3000 : 2000) +
		",0,'a relic',77,'relic',1,"
		"FROM_UNIXTIME(" +
		std::to_string(end_time) + ")," + std::to_string(won ? 2 : 1) + ",1," +
		literal(listing_op) + ")");
	const uint32_t auction_id = mysql_insert_id(connection);
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(uid) + "," + std::to_string(uid) + ",NULL,6," +
		std::to_string(auction_id) + ",0,1,77,1)");
	execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,"
		"obj_blob) VALUES(" +
		std::to_string(auction_id) + ",0," + std::to_string(uid) + ",1,77,'relic')");
	if (won)
		execute("INSERT INTO auction_ledger(operation_id,event_type,auction_id,"
			"auction_revision,actor_pid,counterparty_pid,final_price,item_count) "
			"VALUES(" +
			literal(bid_op) + ",2," + std::to_string(auction_id) + ",2," +
			std::to_string(WINNER) + "," + std::to_string(SELLER) + ",3000,0)");
	return auction_id;
}

auction_settlement_listing listing(uint32_t auction_id, uint64_t uid, uint64_t end_time,
				   const critical_operation_id &listing_op,
				   const critical_operation_id &bid_op)
{
	auction_settlement_listing state;
	state.auction_id = auction_id;
	state.seller_pid = SELLER;
	state.winner_pid = critical_operation_id_is_zero(bid_op) ? 0 : WINNER;
	state.status = 1;
	state.custody_state = 1;
	state.quantity = 1;
	state.current_price = state.winner_pid ? 3000 : 2000;
	state.revision = state.winner_pid ? 2 : 1;
	state.end_time = end_time;
	state.listing_operation = listing_op;
	state.winning_bid_operation = bid_op;
	state.item_count = 1;
	state.items[0] = { uid, 1, 0, 77, 0, false };
	return state;
}

critical_command closure(uint32_t operation, auction_action action,
			 const auction_settlement_listing &state,
			 const auction_settlement_accounts &accounts,
			 const critical_operation_id &epoch, bool trusted = false,
			 uint16_t fee_basis_points = 300)
{
	auction_command_payload payload = {};
	payload.action = action;
	payload.auction_id = state.auction_id;
	payload.closing_fee_basis_points = fee_basis_points;
	if (trusted)
	{
		payload.actor_pid = TRUSTED;
		payload.racewar = 1;
		std::memcpy(payload.account_name.data(), "auction_settle_trusted", 22);
		std::memcpy(payload.actor_name.data(), "AuctionTrusted", 14);
	}
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     trusted ? critical_source_site::command :
					       critical_source_site::zone_event,
				     trusted ? critical_deadline_class::interactive :
					       critical_deadline_class::background));
	command.accepted_at_usec = 1;
	assert(auction_settlement_accounting_intent(command, epoch, state, accounts,
						    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

critical_command seller_claim(uint8_t operation, const auction_item_claim_state &state,
			      const economic_account_key &wallet, const economic_account_key &bank,
			      const critical_operation_id &epoch, uint64_t uid)
{
	auction_command_payload payload = {};
	payload.action = auction_action::claim_item;
	payload.auction_id = state.auction_id;
	payload.actor_pid = SELLER;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "auction_settle_seller", 21);
	std::memcpy(payload.actor_name.data(), "AuctionSeller", 13);
	payload.item_count = 1;
	payload.items[0] = { uid, 1, 77 };
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_item_claim_accounting_intent(command, epoch, wallet, bank, state,
						    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
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

void reconnect()
{
	mysql_close(connection);
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
}

#ifdef DURIS_TELEMETRY_AUCTION_QUALIFICATION
void telemetry_money_claim(const critical_command &first, const economic_account_key &wallet,
			   const economic_account_key &bank, const economic_account_key &pending,
			   const critical_operation_id &epoch,
			   const critical_operation_id &bootstrap)
{
	const char *requested = std::getenv("TELEMETRY_AUCTION_CLAIM_SOURCES");
	const size_t source_count = requested ? std::strtoul(requested, nullptr, 10) : 2;
	auction_money_claim_state state;
	state.beneficiary_pid = SELLER;
	state.money = 2910;
	state.sources = { { first.operation_id, 2, SELLER, pending.authority_id, 2910 } };
	auction_command_result result = {};
	unsigned int code = 0;
	bool applied = false;
	// Listings/bids are explicit prerequisites; every credit uses the actual
	// typed settlement owner. Large journeys include its zero-fee SQL branch.
	for (size_t index = 1; index < source_count; ++index)
	{
		const uint32_t base = index == 1 ? 18 : 1000 + static_cast<uint32_t>(index) * 3;
		const uint64_t uid = ITEM + 500 + index;
		const bool zero_fee = source_count > 2 && index + 1 == source_count;
		const uint32_t auction_id = auction(id(base), id(base + 1), uid, 1700000000);
		auction_settlement_accounts accounts;
		accounts.escrow = mapping(pending.lineage, economic_account_kind::auction_escrow, 0,
					  4, auction_id, bootstrap);
		accounts.seller_claim = pending;
		const auto staged = listing(auction_id, uid, 1700000000, id(base), id(base + 1));
		const auto sale = closure(base + 2, auction_action::finalize, staged, accounts,
					  epoch, false, zero_fee ? 0 : 300);
		execute("START TRANSACTION");
		inbox(sale.operation_id, static_cast<uint16_t>(sale.type), 2, 0);
		economic_sql_auction_settlement_context settlement;
		assert(economic_sql_auction_settlement_lock(connection, sale, &settlement) == 0);
		assert(economic_sql_auction_settlement_execute_and_record(
			       connection, sale, settlement, &result, &code, &applied) == 0);
		assert(code == 0 && applied);
		receipt(sale, result);
		execute("COMMIT");
		const uint64_t amount = zero_fee ? 3000 : 2910;
		state.money += amount;
		state.sources.push_back(
			{ sale.operation_id, 2, SELLER, pending.authority_id, amount });
	}
	std::sort(state.sources.begin(), state.sources.end(),
		  [](const auto &left, const auto &right)
		  { return left.operation.bytes < right.operation.bytes; });
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == static_cast<uint64_t>(state.money));
	state.revision = scalar("SELECT claim_revision FROM auction_money_pickups WHERE pid=" +
				std::to_string(SELLER));
	auction_command_payload payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = SELLER;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "auction_settle_seller", 21);
	std::memcpy(payload.actor_name.data(), "AuctionSeller", 13);
	critical_command command = {};
	assert(auction_command_build(&command, id(21), payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_money_claim_accounting_intent(command, epoch, wallet, bank, pending, state,
						     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	economic_sql_auction_money_claim_context context;
	// Refuse a changed allocation and a real failure after wallet/posting writes.
	execute("UPDATE economic_pending_claim_source SET amount=2911 WHERE source_operation_id=" +
		literal(id(20)));
	execute("START TRANSACTION");
	inbox(command.operation_id, static_cast<uint16_t>(command.type), 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &code, &applied) != 0);
	execute("ROLLBACK");
	execute("UPDATE economic_pending_claim_source SET amount=2910 WHERE source_operation_id=" +
		literal(id(20)));
	execute("CREATE TRIGGER fail_telemetry_auction_claim BEFORE UPDATE ON "
		"economic_pending_claim_source FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced telemetry claim failure'");
	execute("START TRANSACTION");
	inbox(command.operation_id, static_cast<uint16_t>(command.type), 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &code, &applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_telemetry_auction_claim");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == static_cast<uint64_t>(state.money));
	assert(scalar("SELECT gold FROM player_data WHERE pid=" + std::to_string(SELLER)) == 0);
	assert(scalar("SELECT wallet_revision FROM player_data WHERE pid=" +
		      std::to_string(SELLER)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id IS NOT NULL") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	execute("START TRANSACTION");
	inbox(command.operation_id, static_cast<uint16_t>(command.type), 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &code, &applied) == 0);
	assert(code == 0 && applied && result.event_type == auction_event_type::money_claimed);
	receipt(command, result);
	execute("COMMIT");
	reconnect();
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE claim_operation_id=" +
		      literal(command.operation_id)) == source_count);
	std::printf(
		"TELEMETRY_AUCTION_READY %s",
		hex(command.operation_id.bytes.data(), command.operation_id.bytes.size()).c_str());
	for (const auto &source : state.sources)
		std::printf(
			" %s",
			hex(source.operation.bytes.data(), source.operation.bytes.size()).c_str());
	std::printf("\n");
	std::fflush(stdout);
	char acknowledgement[32] = {};
	assert(std::fgets(acknowledgement, sizeof(acknowledgement), stdin) &&
	       std::strcmp(acknowledgement, "continue\n") == 0);
}
#endif
} // namespace

int main()
{
#ifdef DURIS_TELEMETRY_AUCTION_QUALIFICATION
	require_telemetry_disposable_target();
#endif
	assert(getenv("DB_HOST") && getenv("DB_USER") && getenv("DB_PASSWD") && getenv("DB_NAME") &&
	       getenv("DB_PORT"));
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
	const auto bootstrap = id(1), lineage = id(2), epoch = id(3);
	inbox(bootstrap, 1, 1, 1);
	player(SELLER, "AuctionSeller", "auction_settle_seller");
	player(TRUSTED, "AuctionTrusted", "auction_settle_trusted");
	const uint64_t seller_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_settle_seller'");
	const uint64_t trusted_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_settle_trusted'");
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
		literal(bootstrap) + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ")");
	execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
		std::to_string(SELLER) + "," + std::to_string(INITIAL_PENDING) + ",1)");
	const auto seller_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, SELLER, bootstrap);
	const auto seller_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, seller_bank, bootstrap);
	const auto trusted_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, TRUSTED, bootstrap);
	const auto trusted_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, trusted_bank, bootstrap);
	const auto seller_claim_key =
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, SELLER, bootstrap);
	const uint32_t sale_auction = auction(id(4), id(5), ITEM, 1700000000);
	const auto sale_escrow = mapping(lineage, economic_account_kind::auction_escrow, 0, 4,
					 sale_auction, bootstrap);
	auto sale_listing = listing(sale_auction, ITEM, 1700000000, id(4), id(5));
	auction_settlement_accounts sale_accounts;
	sale_accounts.escrow = sale_escrow;
	sale_accounts.seller_claim = seller_claim_key;
	const auto sale = closure(6, auction_action::finalize, sale_listing, sale_accounts, epoch);
	execute("CREATE TRIGGER fail_auction_settlement_source BEFORE INSERT ON "
		"economic_pending_claim_source FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced settlement source failure'");
	execute("START TRANSACTION");
	inbox(sale.operation_id, static_cast<uint16_t>(sale.type), 2, 0);
	economic_sql_auction_settlement_context sale_context;
	assert(economic_sql_auction_settlement_lock(connection, sale, &sale_context) == 0);
	auction_command_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;
	assert(economic_sql_auction_settlement_execute_and_record(connection, sale, sale_context,
								  &result, &result_code,
								  &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_settlement_source");
	assert(scalar("SELECT status+0 FROM auctions WHERE id=" + std::to_string(sale_auction)) ==
	       1);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == INITIAL_PENDING);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(sale_auction) + " AND claim_pid IS NULL") == 1);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE operation_id=" +
		      literal(sale.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      literal(sale.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(sale.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(sale_escrow.authority_id) + " AND active_native_id=" +
		      std::to_string(sale_auction) + " AND retiring_operation_id IS NULL") == 1);
	execute("START TRANSACTION");
	inbox(sale.operation_id, static_cast<uint16_t>(sale.type), 2, 0);
	assert(economic_sql_auction_settlement_lock(connection, sale, &sale_context) == 0);
	assert(economic_sql_auction_settlement_execute_and_record(connection, sale, sale_context,
								  &result, &result_code,
								  &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::sold);
	receipt(sale, result);
	execute("COMMIT");
	assert(scalar("SELECT status+0 FROM auctions WHERE id=" + std::to_string(sale_auction)) ==
	       2);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == INITIAL_PENDING + 2910);
	assert(scalar("SELECT amount FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(sale.operation_id) +
		      " AND source_slot=2 AND beneficiary_pid=" + std::to_string(SELLER) +
		      " AND claim_mapping_id=" + std::to_string(seller_claim_key.authority_id)) ==
	       2910);
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(sale_auction)) == WINNER);
	assert(value("SELECT GROUP_CONCAT(copper_value ORDER BY event_index) FROM "
		     "economic_accounting_coin_posting WHERE operation_id=" +
		     literal(sale.operation_id)) == "-3000,2910,90");
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(sale.operation_id)) == 0);
	assert(scalar("SELECT before_witness_count FROM economic_accounting_operation WHERE "
		      "operation_id=" +
		      literal(sale.operation_id)) == 1);
	assert(value("SELECT LOWER(HEX(SUBSTR(source_event,5,16))) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(sale.operation_id)) == hex(id(5).bytes.data(), id(5).bytes.size()));
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(sale_escrow.authority_id) +
		      " AND active_native_id IS NULL "
		      "AND retiring_operation_id=" +
		      literal(sale.operation_id) + " AND revision=1") == 1);
#ifdef DURIS_TELEMETRY_AUCTION_QUALIFICATION
	telemetry_money_claim(sale, seller_wallet, seller_bank_key, seller_claim_key, epoch,
			      bootstrap);
	mysql_close(connection);
	return 0;
#endif
	const uint32_t removed_auction = auction(id(7), id(8), ITEM + 100, 2000000000);
	const auto removal_escrow = mapping(lineage, economic_account_kind::auction_escrow, 0, 4,
					    removed_auction, bootstrap);
	const auto removal_listing = listing(removed_auction, ITEM + 100, 2000000000, id(7), id(8));
	auction_settlement_accounts removal_accounts;
	removal_accounts.escrow = removal_escrow;
	removal_accounts.actor_wallet = trusted_wallet;
	removal_accounts.actor_bank = trusted_bank_key;
	const auto removal =
		closure(9, auction_action::remove, removal_listing, removal_accounts, epoch, true);
	execute("CREATE TRIGGER fail_auction_removal_effect BEFORE INSERT ON "
		"economic_accounting_account_effect FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced removal accounting failure'");
	execute("START TRANSACTION");
	inbox(removal.operation_id, static_cast<uint16_t>(removal.type), 2, 0);
	economic_sql_auction_settlement_context removal_context;
	assert(economic_sql_auction_settlement_lock(connection, removal, &removal_context) == 0);
	assert(economic_sql_auction_settlement_execute_and_record(
		       connection, removal, removal_context, &result, &result_code,
		       &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_removal_effect");
	assert(scalar("SELECT status+0 FROM auctions WHERE id=" +
		      std::to_string(removed_auction)) == 1);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(removed_auction) + " AND claim_pid IS NULL") == 1);
	execute("START TRANSACTION");
	inbox(removal.operation_id, static_cast<uint16_t>(removal.type), 2, 0);
	assert(economic_sql_auction_settlement_lock(connection, removal, &removal_context) == 0);
	assert(economic_sql_auction_settlement_execute_and_record(
		       connection, removal, removal_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::removed);
	receipt(removal, result);
	execute("COMMIT");
	assert(scalar("SELECT status+0 FROM auctions WHERE id=" +
		      std::to_string(removed_auction)) == 3);
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(removed_auction)) == SELLER);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(SELLER)) == 3010);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "source_operation_id=" +
		      literal(removal.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(removal.operation_id)) == 0);
	assert(scalar("SELECT before_copper FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(removal.operation_id)) == 3000);
	assert(scalar("SELECT after_copper FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(removal.operation_id)) == 3000);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(removal_escrow.authority_id) + " AND active_native_id=" +
		      std::to_string(removed_auction) + " AND retiring_operation_id IS NULL") == 1);
	auction_item_claim_state staged;
	staged.auction_id = removed_auction;
	staged.seller_pid = SELLER;
	staged.winner_pid = WINNER;
	staged.claimant_pid = SELLER;
	staged.status = 3;
	staged.custody_state = 1;
	staged.auction_revision = 3;
	staged.listing_operation = id(7);
	staged.claim_source_operation = removal.operation_id;
	staged.item_count = 1;
	staged.rows[0] = { ITEM + 100, 1, 0, 77, SELLER, false };
	const auto claim =
		seller_claim(10, staged, seller_wallet, seller_bank_key, epoch, ITEM + 100);
	execute("START TRANSACTION");
	inbox(claim.operation_id, static_cast<uint16_t>(claim.type), 2, 0);
	economic_sql_auction_item_claim_context claim_context;
	assert(economic_sql_auction_item_claim_lock(connection, claim, &claim_context) == 0);
	assert(economic_sql_auction_item_claim_execute_and_record(connection, claim, claim_context,
								  &result, &result_code,
								  &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied && result.item_uids[0] == ITEM + 100);
	receipt(claim, result);
	execute("COMMIT");
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM + 100)) == 1);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(ITEM + 100)) == SELLER);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(claim.operation_id) + " AND item_uid=" + std::to_string(ITEM + 100) +
		      " AND before_revision=1 AND after_revision=2") == 1);
	assert(value("SELECT LOWER(HEX(SUBSTR(source_event,5,16))) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(claim.operation_id)) ==
	       hex(removal.operation_id.bytes.data(), removal.operation_id.bytes.size()));
	assert(value("SELECT LOWER(HEX(original_operation_id)) FROM "
		     "economic_accounting_operation WHERE operation_id=" +
		     literal(claim.operation_id)) ==
	       hex(staged.listing_operation.bytes.data(), staged.listing_operation.bytes.size()));
	const uint32_t removed_no_bid_auction = auction(id(14), {}, ITEM + 300, 2000000000);
	const auto removed_no_bid_escrow = mapping(lineage, economic_account_kind::auction_escrow,
						   0, 4, removed_no_bid_auction, bootstrap);
	const auto removed_no_bid_listing =
		listing(removed_no_bid_auction, ITEM + 300, 2000000000, id(14), {});
	auction_settlement_accounts removed_no_bid_accounts;
	removed_no_bid_accounts.escrow = removed_no_bid_escrow;
	removed_no_bid_accounts.actor_wallet = trusted_wallet;
	removed_no_bid_accounts.actor_bank = trusted_bank_key;
	const auto remove_no_bid = closure(15, auction_action::remove, removed_no_bid_listing,
					   removed_no_bid_accounts, epoch, true);
	execute("START TRANSACTION");
	inbox(remove_no_bid.operation_id, static_cast<uint16_t>(remove_no_bid.type), 2, 0);
	economic_sql_auction_settlement_context remove_no_bid_context;
	assert(economic_sql_auction_settlement_lock(connection, remove_no_bid,
						    &remove_no_bid_context) == 0);
	assert(economic_sql_auction_settlement_execute_and_record(
		       connection, remove_no_bid, remove_no_bid_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::removed);
	receipt(remove_no_bid, result);
	execute("COMMIT");
	assert(scalar("SELECT after_copper FROM economic_accounting_account_effect WHERE "
		      "operation_id=" +
		      literal(remove_no_bid.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(remove_no_bid.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(removed_no_bid_escrow.authority_id) +
		      " AND active_native_id IS NULL AND retiring_operation_id=" +
		      literal(remove_no_bid.operation_id) + " AND revision=1") == 1);
	const uint32_t expired_auction = auction(id(11), {}, ITEM + 200, 1700000000);
	const auto expired_escrow = mapping(lineage, economic_account_kind::auction_escrow, 0, 4,
					    expired_auction, bootstrap);
	const auto expired_listing = listing(expired_auction, ITEM + 200, 1700000000, id(11), {});
	auction_settlement_accounts expired_accounts;
	expired_accounts.escrow = expired_escrow;
	const auto expired =
		closure(12, auction_action::finalize, expired_listing, expired_accounts, epoch);
	execute("START TRANSACTION");
	inbox(expired.operation_id, static_cast<uint16_t>(expired.type), 2, 0);
	economic_sql_auction_settlement_context expired_context;
	assert(economic_sql_auction_settlement_lock(connection, expired, &expired_context) == 0);
	assert(economic_sql_auction_settlement_execute_and_record(
		       connection, expired, expired_context, &result, &result_code,
		       &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::expired);
	receipt(expired, result);
	execute("COMMIT");
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(expired_auction)) == SELLER);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(expired.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=" +
		      std::to_string(expired_escrow.authority_id) +
		      " AND active_native_id IS NULL "
		      "AND retiring_operation_id=" +
		      literal(expired.operation_id) + " AND revision=1") == 1);
	// Retained contexts must reject a changed epoch without a second mutation.
	execute("UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 "
		"WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_auction_settlement_execute_and_record(
		       connection, removal, removal_context, &result, &result_code,
		       &mutation_applied) != 0);
	execute("ROLLBACK");
	reconnect();
	for (const auto operation : { sale.operation_id, removal.operation_id,
				      remove_no_bid.operation_id, expired.operation_id })
	{
		assert(scalar("SELECT LENGTH(canonical_intent) FROM "
			      "economic_accounting_operation WHERE operation_id=" +
			      literal(operation)) > 0);
		assert(scalar("SELECT LENGTH(canonical_plan) FROM "
			      "economic_accounting_operation WHERE operation_id=" +
			      literal(operation)) > 0);
		assert(scalar("SELECT LENGTH(result_payload) FROM critical_operation_inbox "
			      "WHERE operation_id=" +
			      literal(operation)) == AUCTION_RESULT_PAYLOAD_BYTES);
		assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
			      literal(operation)) == 1);
	}
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=" +
		      literal(claim.operation_id)) == 1);
	mysql_close(connection);
	return 0;
}
