#include "economy/auction_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "persistence/economic_sql_auction_item_claim_transaction.h"
#include "persistence/economic_sql_auction_settlement_transaction.h"

#include <mysql.h>
#include <openssl/sha.h>
#include "persistence/economic_sql_pending_claim_source.h"
#include <cerrno>

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

void inbox(const critical_command &command)
{
	inbox(command.operation_id, static_cast<uint16_t>(command.type), command.schema_version, 0);
	std::vector<uint8_t> bytes, keys;
	std::array<uint8_t, 32> digest{}, keys_digest{};
	assert(critical_command_encode(command, &bytes) == critical_command_codec_result::ok);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (unsigned shift = 0; shift < 8; ++shift)
			keys.push_back(static_cast<uint8_t>(key.id >> (shift * 8)));
	}
	SHA256(bytes.data(), bytes.size(), digest.data());
	SHA256(keys.data(), keys.size(), keys_digest.data());
	execute("UPDATE critical_operation_inbox SET command_hash=X'" +
		hex(digest.data(), digest.size()) + "',keys_hash=X'" +
		hex(keys_digest.data(), keys_digest.size()) +
		"' WHERE operation_id=" + literal(command.operation_id));
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

critical_command closure(uint8_t operation, auction_action action,
			 const auction_settlement_listing &state,
			 const auction_settlement_accounts &accounts,
			 const critical_operation_id &epoch, bool trusted = false,
			 uint32_t fee = 300)
{
	auction_command_payload payload = {};
	payload.action = action;
	payload.auction_id = state.auction_id;
	payload.closing_fee_basis_points = fee;
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

// Original committed creator evidence must reject corruption without changing output.
void retained_sale_corruption_controls(const critical_command &command,
				       const auction_command_result &original, uint32_t beneficiary,
				       const economic_account_key &claim, bool zero)
{
	const auto scope = "operation_id=" + literal(command.operation_id);
	const auto refused = [&]()
	{
		economic_account_key sentinel = claim;
		sentinel.authority_id = 999;
		const auto unchanged = sentinel;
		assert(economic_sql_pending_claim_endpoint_readback(
			       connection, command, beneficiary, &sentinel) == EILSEQ);
		assert(economic_account_key_equal(sentinel, unchanged));
		if (zero)
			assert(economic_sql_pending_claim_endpoint_verify_zero_creator(
				       connection, command.operation_id, claim, beneficiary) ==
			       EILSEQ);
	};
	const auto corrupt = [&](const std::string &sql)
	{
		execute("START TRANSACTION");
		execute(sql);
		assert(mysql_affected_rows(connection) == 1);
		refused();
		execute("ROLLBACK");
	};
	corrupt("UPDATE critical_operation_inbox SET result_payload=X'' WHERE " + scope);
	corrupt("UPDATE critical_operation_inbox SET durable_revision=durable_revision+1 WHERE " +
		scope);
	corrupt("UPDATE critical_outbox SET payload_version=2 WHERE " + scope);
	corrupt("UPDATE critical_outbox SET destination=11 WHERE " + scope);
	// Canonical but changed original results remain inconsistent with native witnesses.
	std::vector<auction_command_result> changed;
	auto candidate = original;
	++candidate.final_price;
	changed.push_back(candidate);
	candidate = original;
	++candidate.wallet_revision;
	changed.push_back(candidate);
	candidate = original;
	++candidate.bank.amount[0];
	changed.push_back(candidate);
	candidate = original;
	++candidate.claim_credit_used;
	changed.push_back(candidate);
	candidate = original;
	++candidate.previous_bidder_pid;
	changed.push_back(candidate);
	candidate = original;
	++candidate.status;
	changed.push_back(candidate);
	for (const auto &result : changed)
	{
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
		assert(auction_command_encode_result(result, &encoded));
		const auto bytes = "X'" + hex(encoded.data(), encoded.size()) + "'";
		// First alter only the inbox: independent native outbox must disagree.
		corrupt("UPDATE critical_operation_inbox SET result_payload=" + bytes + " WHERE " +
			scope);
		// Matching altered receipt copies cannot replace native ledger/plan witnesses.
		execute("START TRANSACTION");
		execute("UPDATE critical_operation_inbox SET result_payload=" + bytes + " WHERE " +
			scope);
		assert(mysql_affected_rows(connection) == 1);
		execute("UPDATE critical_outbox SET payload=" + bytes + " WHERE " + scope);
		assert(mysql_affected_rows(connection) == 1);
		refused();
		execute("ROLLBACK");
	}
	economic_account_key sink{ claim.lineage, economic_account_kind::sink,
				   ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID, 0 };
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
	assert(economic_account_key_encode(sink, &encoded) == economic_accounting_error::ok);
	const auto fee_index =
		scalar("SELECT account_index FROM economic_accounting_account_effect WHERE " +
		       scope + " AND account_key=X'" + hex(encoded.data(), encoded.size()) + "'");
	const auto fee = scope + " AND account_index=" + std::to_string(fee_index);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE " + fee) == 1);
	corrupt("UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 WHERE " +
		fee);
	corrupt("UPDATE economic_accounting_coin_posting SET delta_copper=delta_copper+1 WHERE " +
		fee);
	corrupt("UPDATE economic_accounting_coin_posting SET child_index=1 WHERE " + fee);
	corrupt("DELETE FROM economic_accounting_coin_posting WHERE " + fee);
	// The selected claim is unchanged while a different original account is corrupt.
	corrupt("UPDATE economic_accounting_account_effect SET after_revision=after_revision+1 WHERE " +
		scope + " AND account_index=" + std::to_string(fee_index));
	// Respect the existing posting->effect FK while testing a missing effect.
	execute("START TRANSACTION");
	execute("DELETE FROM economic_accounting_coin_posting WHERE " + fee);
	assert(mysql_affected_rows(connection) == 1);
	execute("DELETE FROM economic_accounting_account_effect WHERE " + fee);
	assert(mysql_affected_rows(connection) == 1);
	refused();
	execute("ROLLBACK");
	corrupt("UPDATE auction_ledger SET value_delta=value_delta+1 WHERE " + scope);
	execute("START TRANSACTION");
	economic_account_key retained;
	assert(economic_sql_pending_claim_endpoint_readback(connection, command, beneficiary,
							    &retained) == 0);
	assert(economic_account_key_equal(retained, claim));
	if (zero)
		assert(economic_sql_pending_claim_endpoint_verify_zero_creator(
			       connection, command.operation_id, claim, beneficiary) == 0);
	execute("ROLLBACK");
}

void receipt(const critical_command &command, const auction_command_result &result)
{
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded = {};
	assert(auction_command_encode_result(result, &encoded));
	execute("INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,"
		"payload_version,payload) VALUES(" +
		literal(command.operation_id) + ",0,5,1,1,X'" +
		hex(encoded.data(), encoded.size()) + "')");
	execute("UPDATE critical_operation_inbox SET status=1,result_code=0,durable_revision=" +
		std::to_string(std::max({ result.auction_revision, result.wallet_revision,
					  result.bank_revision, result.player_owner_revision,
					  result.auction_owner_revision })) +
		",result_payload=X'" + hex(encoded.data(), encoded.size()) +
		"',committed_at=CURRENT_TIMESTAMP(6) WHERE operation_id=" +
		literal(command.operation_id) + " AND status=0");
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
} // namespace

int main(int argc, char **argv)
{
	const bool zero = argc == 2 && !std::strcmp(argv[1], "--first-seller-zero");
	const bool first_seller = zero || (argc == 2 && !std::strcmp(argv[1], "--first-seller"));
	assert(argc == 1 || first_seller);
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
	if (!first_seller)
		execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
			std::to_string(SELLER) + ",100,1)");
	const auto seller_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, SELLER, bootstrap);
	const auto seller_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, seller_bank, bootstrap);
	const auto trusted_wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, TRUSTED, bootstrap);
	const auto trusted_bank_key =
		mapping(lineage, economic_account_kind::bank, 1, 2, trusted_bank, bootstrap);
	auto seller_claim_key = first_seller ?
					economic_account_key{} :
					mapping(lineage, economic_account_kind::pending_claim, 0, 5,
						SELLER, bootstrap);
	const uint32_t sale_auction = auction(id(4), id(5), ITEM, 1700000000);
	const auto sale_escrow = mapping(lineage, economic_account_kind::auction_escrow, 0, 4,
					 sale_auction, bootstrap);
	auto sale_listing = listing(sale_auction, ITEM, 1700000000, id(4), id(5));
	auction_settlement_accounts sale_accounts;
	sale_accounts.escrow = sale_escrow;
	sale_accounts.seller_claim = seller_claim_key;
	if (first_seller)
		sale_accounts.absent_seller_pid = SELLER;
	const auto sale = closure(6, auction_action::finalize, sale_listing, sale_accounts, epoch,
				  false, zero ? 10000 : 300);
	if (first_seller)
	{
		economic_sql_auction_settlement_context context;
		auction_command_result native{};
		unsigned int code = 0;
		bool applied = false;
		execute("START TRANSACTION");
		inbox(sale);
		execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
			std::to_string(SELLER) + ",0,1)");
		assert(economic_sql_auction_settlement_lock(connection, sale, &context) == EILSEQ);
		execute("ROLLBACK");
		execute("START TRANSACTION");
		inbox(sale);
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, SELLER, bootstrap);
		assert(economic_sql_auction_settlement_lock(connection, sale, &context) == EILSEQ);
		execute("ROLLBACK");
		execute("CREATE TRIGGER fail_first_endpoint_effect BEFORE INSERT ON economic_accounting_account_effect FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='first endpoint evidence failure'");
		execute("START TRANSACTION");
		inbox(sale);
		assert(economic_sql_auction_settlement_lock(connection, sale, &context) == 0);
		assert(economic_sql_auction_settlement_execute_and_record(
			       connection, sale, context, &native, &code, &applied) != 0);
		execute("ROLLBACK");
		execute("DROP TRIGGER fail_first_endpoint_effect");
		assert(scalar("SELECT COUNT(*) FROM auction_money_pickups WHERE pid=" +
			      std::to_string(SELLER)) == 0);
		assert(scalar("SELECT COUNT(*) FROM economic_account_mapping WHERE account_kind=5 AND lineage=" +
			      literal(lineage)) == 0);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
			      literal(sale.operation_id)) == 0);
		assert(scalar("SELECT status+0 FROM auctions WHERE id=" +
			      std::to_string(sale_auction)) == 1);
		execute("START TRANSACTION");
		inbox(sale);
		assert(economic_sql_auction_settlement_lock(connection, sale, &context) == 0);
		assert(economic_sql_auction_settlement_execute_and_record(
			       connection, sale, context, &native, &code, &applied) == 0);
		assert(!code && applied);
		receipt(sale, native);
		execute("COMMIT");
		assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
			      std::to_string(SELLER)) == (zero ? 0U : 2910U));
		assert(scalar("SELECT claim_revision FROM auction_money_pickups WHERE pid=" +
			      std::to_string(SELLER)) == 1);
		execute("START TRANSACTION");
		assert(economic_sql_pending_claim_endpoint_readback(connection, sale, SELLER,
								    &seller_claim_key) == 0);
		if (zero)
			assert(economic_sql_pending_claim_endpoint_verify_zero_creator(
				       connection, sale.operation_id, seller_claim_key, SELLER) ==
			       0);
		execute("COMMIT");
		retained_sale_corruption_controls(sale, native, SELLER, seller_claim_key, zero);
		assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE source_operation_id=" +
			      literal(sale.operation_id)) == (zero ? 0U : 1U));
		if (!zero)
		{
			execute("UPDATE economic_pending_claim_source SET source_slot=3 WHERE source_operation_id=" +
				literal(sale.operation_id));
			execute("START TRANSACTION");
			economic_account_key sentinel;
			sentinel.authority_id = 999;
			assert(economic_sql_pending_claim_endpoint_readback(
				       connection, sale, SELLER, &sentinel) == EILSEQ);
			assert(sentinel.authority_id == 999);
			execute("ROLLBACK");
			execute("UPDATE economic_pending_claim_source SET source_slot=2 WHERE source_operation_id=" +
				literal(sale.operation_id));
		}
		execute("UPDATE economic_account_mapping SET creating_operation_id=" +
			literal(bootstrap) +
			" WHERE mapping_id=" + std::to_string(seller_claim_key.authority_id));
		execute("START TRANSACTION");
		economic_account_key sentinel;
		assert(economic_sql_pending_claim_endpoint_readback(connection, sale, SELLER,
								    &sentinel) == EILSEQ);
		if (zero)
			assert(economic_sql_pending_claim_endpoint_verify_zero_creator(
				       connection, sale.operation_id, seller_claim_key, SELLER) ==
			       EILSEQ);
		execute("ROLLBACK");
		execute("UPDATE economic_account_mapping SET creating_operation_id=" +
			literal(sale.operation_id) +
			" WHERE mapping_id=" + std::to_string(seller_claim_key.authority_id));
		reconnect();
		execute("START TRANSACTION");
		economic_account_key retained;
		assert(economic_sql_pending_claim_endpoint_readback(connection, sale, SELLER,
								    &retained) == 0);
		assert(retained.authority_id == seller_claim_key.authority_id);
		if (zero)
			assert(economic_sql_pending_claim_endpoint_verify_zero_creator(
				       connection, sale.operation_id, retained, SELLER) == 0);
		execute("COMMIT");
		mysql_close(connection);
		return 0;
	}
	execute("CREATE TRIGGER fail_auction_settlement_source BEFORE INSERT ON "
		"economic_pending_claim_source FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced settlement source failure'");
	execute("START TRANSACTION");
	inbox(sale);
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
		      std::to_string(SELLER)) == 100);
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
	inbox(sale);
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
		      std::to_string(SELLER)) == 3010);
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
	inbox(removal);
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
	inbox(removal);
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
	inbox(claim);
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
	inbox(remove_no_bid);
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
	inbox(expired);
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
