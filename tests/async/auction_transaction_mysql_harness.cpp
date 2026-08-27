#include "auction_command.h"
#include "critical_command_repository.h"

#include <mysql.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
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

MYSQL *connect_database()
{
	MYSQL *result = mysql_init(nullptr);
	assert(result);
	const char *port_value = getenv("DB_PORT");
	const unsigned int port = port_value ? static_cast<unsigned int>(atoi(port_value)) : 3306;
	assert(mysql_real_connect(result, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
				  getenv("AUCTION_TEST_DB_NAME"), port, nullptr, 0));
	return result;
}

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()) != 0)
		fprintf(stderr, "auction harness SQL failed: %u %s\n%s\n", mysql_errno(connection),
			mysql_error(connection), sql.c_str());
	assert(mysql_errno(connection) == 0);
}

long long scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *query = mysql_store_result(connection);
	assert(query);
	MYSQL_ROW row = mysql_fetch_row(query);
	assert(row && row[0]);
	const long long value = strtoll(row[0], nullptr, 10);
	mysql_free_result(query);
	return value;
}

std::string operation_hex(const critical_operation_id &operation_id)
{
	char value[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation_id, value, sizeof(value)));
	return value;
}

void fill_actor(auction_command_payload *payload, uint32_t pid, const char *account,
		const char *name, uint64_t revision)
{
	payload->actor_pid = pid;
	payload->racewar = 1;
	memcpy(payload->account_name.data(), account, strlen(account));
	memcpy(payload->actor_name.data(), name, strlen(name));
	payload->expected_wallet_revision = revision;
	payload->expected_bank_revision = revision;
	payload->closing_fee_basis_points = 300;
	payload->bid_extension_seconds = 300;
}

critical_command command_for(const auction_command_payload &payload,
			     std::vector<std::string> *operations)
{
	critical_operation_id operation_id = {};
	assert(critical_operation_id_generate(&operation_id));
	critical_command command = {};
	assert(auction_command_build(&command, operation_id, payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	operations->push_back(operation_hex(operation_id));
	return command;
}

auction_command_result apply(const critical_command &command,
			     critical_apply_outcome expected = critical_apply_outcome::applied,
			     unsigned int error = 0)
{
	const critical_apply_result applied =
		critical_command_repository_apply(connection, command);
	if (applied.outcome != expected || applied.error_code != error)
		fprintf(stderr, "auction apply failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == expected && applied.error_code == error);
	auction_command_result result = {};
	if (expected == critical_apply_outcome::applied)
		assert(auction_command_decode_result(applied.result_payload.data(),
						     applied.result_size, &result));
	return result;
}

uint32_t insert_player(uint32_t pid, const char *name, const char *account)
{
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES(" +
		std::to_string(pid) + ",'" + name + "','" + account + "',1,0,0,0,10)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		"bank_platinum) VALUES('" +
		std::string(account) + "',1,0,0,0,0)");
	return pid;
}
} // namespace

int main()
{
	const char *host = getenv("DB_HOST"), *user = getenv("DB_USER"),
		   *password = getenv("DB_PASSWD"), *database = getenv("AUCTION_TEST_DB_NAME"),
		   *port_value = getenv("DB_PORT");
	assert(host && user && password && database);
	(void)port_value;
	connection = connect_database();

	const char *seller_account = "auction_harness_seller";
	const char *bidder_account = "auction_harness_bidder";
	const char *buyer_account = "auction_harness_buyer";
	for (const char *account : { seller_account, bidder_account, buyer_account })
	{
		execute("DELETE FROM player_data WHERE account_name='" + std::string(account) +
			"'");
		execute("DELETE FROM account_banks WHERE account_name='" + std::string(account) +
			"'");
		execute("DELETE FROM accounts WHERE account_name='" + std::string(account) + "'");
		execute("INSERT INTO accounts(account_name,password) VALUES('" +
			std::string(account) + "','')");
	}
	const uint32_t seller = insert_player(2147000301U, "AuctionSeller", seller_account);
	const uint32_t bidder = insert_player(2147000302U, "AuctionBidder", bidder_account);
	const uint32_t buyer = insert_player(2147000303U, "AuctionBuyer", buyer_account);
	const uint64_t item_uid = 990000001;
	execute("DELETE FROM item_current_owner WHERE item_uid=" + std::to_string(item_uid));
	execute("DELETE FROM item_owner_revision WHERE owner_type IN (1,7) AND owner_id IN (" +
		std::to_string(seller) + "," + std::to_string(buyer) + ")");
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1," +
		std::to_string(seller) + ",0,0)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(item_uid) + "," + std::to_string(item_uid) + ",NULL,1," +
		std::to_string(seller) + ",0,0,77,1)");

	std::vector<std::string> operations;
	auction_command_payload listing = {};
	listing.action = auction_action::list;
	fill_actor(&listing, seller, seller_account, "AuctionSeller", 0);
	listing.start_price = 2000;
	listing.buy_price = 5000;
	listing.listing_fee = 1000;
	listing.end_time = 2000000000;
	listing.item_count = 1;
	listing.items[0] = { item_uid, 0, 77 };
	memcpy(listing.object_blob.data(), "object-blob", 12);
	listing.object_blob_size = 12;
	memcpy(listing.object_short.data(), "a harness blade", 15);
	critical_command list_command = command_for(listing, &operations);
	auction_command_result result = apply(list_command);
	const uint32_t auction_id = result.auction_id;
	assert(auction_id && result.event_type == auction_event_type::listed);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(seller)) == 9);
	assert(scalar("SELECT owner_type FROM item_current_owner WHERE item_uid=" +
		      std::to_string(item_uid)) == 6);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id)) == 1);
	const critical_apply_result duplicate =
		critical_command_repository_apply(connection, list_command);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(scalar("SELECT COUNT(*) FROM auctions WHERE id=" + std::to_string(auction_id)) == 1);
	auction_command_payload early_finalize = {};
	early_finalize.action = auction_action::finalize;
	early_finalize.auction_id = auction_id;
	early_finalize.closing_fee_basis_points = 300;
	apply(command_for(early_finalize, &operations), critical_apply_outcome::terminal_failure,
	      EAGAIN);

	auction_command_payload stale_bid = {};
	stale_bid.action = auction_action::bid;
	stale_bid.auction_id = auction_id;
	stale_bid.value = 3000;
	fill_actor(&stale_bid, bidder, bidder_account, "AuctionBidder", 99);
	apply(command_for(stale_bid, &operations), critical_apply_outcome::terminal_failure,
	      ESTALE);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(bidder)) ==
	       10);

	auction_command_payload bid = stale_bid;
	fill_actor(&bid, bidder, bidder_account, "AuctionBidder", 0);
	result = apply(command_for(bid, &operations));
	assert(result.event_type == auction_event_type::bid_placed && result.final_price == 3000);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(bidder)) == 7);

	auction_command_payload buy = {};
	buy.action = auction_action::bid;
	buy.auction_id = auction_id;
	buy.value = 5000;
	fill_actor(&buy, buyer, buyer_account, "AuctionBuyer", 0);
	result = apply(command_for(buy, &operations));
	assert(result.event_type == auction_event_type::sold && result.final_price == 5000);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(bidder)) == 3000);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(seller)) == 4850);
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id)) == buyer);

	auction_command_payload money_claim = {};
	money_claim.action = auction_action::claim_money;
	fill_actor(&money_claim, seller, seller_account, "AuctionSeller", 1);
	critical_command money_commands[2] = { command_for(money_claim, &operations),
					       command_for(money_claim, &operations) };
	critical_apply_result money_results[2] = {};
	MYSQL *money_connections[2] = { connect_database(), connect_database() };
	std::thread money_workers[2] = {
		std::thread(
			[&] {
				money_results[0] = critical_command_repository_apply(
					money_connections[0], money_commands[0]);
			}),
		std::thread(
			[&] {
				money_results[1] = critical_command_repository_apply(
					money_connections[1], money_commands[1]);
			})
	};
	for (std::thread &worker : money_workers)
		worker.join();
	mysql_close(money_connections[0]);
	mysql_close(money_connections[1]);
	const int money_winner = money_results[0].outcome == critical_apply_outcome::applied ? 0 :
											       1;
	const int money_loser = 1 - money_winner;
	assert(money_results[money_winner].outcome == critical_apply_outcome::applied);
	assert(money_results[money_loser].outcome == critical_apply_outcome::terminal_failure &&
	       money_results[money_loser].error_code == ESTALE);
	assert(auction_command_decode_result(money_results[money_winner].result_payload.data(),
					     money_results[money_winner].result_size, &result));
	assert(result.wallet_value_delta == 4850);
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(seller)) == 0);

	auction_command_payload item_claim = {};
	item_claim.action = auction_action::claim_item;
	item_claim.auction_id = auction_id;
	fill_actor(&item_claim, buyer, buyer_account, "AuctionBuyer", 1);
	item_claim.item_count = 1;
	item_claim.items[0] = { item_uid, 1, 77 };
	memcpy(item_claim.object_blob.data(), "object-blob", 12);
	item_claim.object_blob_size = 12;
	result = apply(command_for(item_claim, &operations));
	assert(result.event_type == auction_event_type::item_claimed &&
	       result.item_revisions[0] == 2);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(item_uid)) == buyer);
	assert(scalar("SELECT COUNT(*) FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(auction_id) + " AND claimed_at IS NOT NULL") == 1);
	auction_command_payload duplicate_item_claim = item_claim;
	apply(command_for(duplicate_item_claim, &operations),
	      critical_apply_outcome::terminal_failure, ESTALE);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger WHERE auction_id=" +
		      std::to_string(auction_id)) == 4);

	const uint64_t expired_uid = item_uid + 1;
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(expired_uid) + "," + std::to_string(expired_uid) + ",NULL,1," +
		std::to_string(seller) + ",0,0,78,1)");
	auction_command_payload expired_listing = {};
	expired_listing.action = auction_action::list;
	fill_actor(&expired_listing, seller, seller_account, "AuctionSeller", 2);
	expired_listing.start_price = 1000;
	expired_listing.listing_fee = 1000;
	expired_listing.end_time = static_cast<uint64_t>(time(nullptr) - 1);
	expired_listing.item_count = 1;
	expired_listing.items[0] = { expired_uid, 0, 78 };
	memcpy(expired_listing.object_blob.data(), "expired-blob", 13);
	expired_listing.object_blob_size = 13;
	memcpy(expired_listing.object_short.data(), "an expired blade", 16);
	result = apply(command_for(expired_listing, &operations));
	const uint32_t expired_auction_id = result.auction_id;
	auction_command_payload finalize = {};
	finalize.action = auction_action::finalize;
	finalize.auction_id = expired_auction_id;
	finalize.closing_fee_basis_points = 300;
	result = apply(command_for(finalize, &operations));
	assert(result.event_type == auction_event_type::expired && result.winner_pid == 0);
	assert(scalar("SELECT claim_pid FROM auction_item_custody WHERE auction_id=" +
		      std::to_string(expired_auction_id)) == seller);
	auction_command_payload expired_claim = {};
	expired_claim.action = auction_action::claim_item;
	expired_claim.auction_id = expired_auction_id;
	fill_actor(&expired_claim, seller, seller_account, "AuctionSeller", 3);
	expired_claim.item_count = 1;
	expired_claim.items[0] = { expired_uid, 1, 78 };
	memcpy(expired_claim.object_blob.data(), "expired-blob", 13);
	expired_claim.object_blob_size = 13;
	result = apply(command_for(expired_claim, &operations));
	assert(result.event_type == auction_event_type::item_claimed);
	assert(scalar("SELECT owner_id FROM item_current_owner WHERE item_uid=" +
		      std::to_string(expired_uid)) == seller);

	execute("DELETE FROM auction_bid_history WHERE auction_id=" + std::to_string(auction_id));
	for (const std::string &operation : operations)
	{
		execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o ON "
			"o.outbox_id=d.outbox_id WHERE o.operation_id=UNHEX('" +
			operation + "')");
		execute("DELETE FROM critical_outbox WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM auction_ledger WHERE operation_id=UNHEX('" + operation + "')");
		execute("DELETE FROM currency_ledger WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM item_ownership_ledger WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			operation + "')");
	}
	execute("DELETE FROM auction_item_custody WHERE auction_id IN (" +
		std::to_string(auction_id) + "," + std::to_string(expired_auction_id) + ")");
	execute("DELETE FROM auctions WHERE id IN (" + std::to_string(auction_id) + "," +
		std::to_string(expired_auction_id) + ")");
	execute("DELETE FROM auction_money_pickups WHERE pid IN (" + std::to_string(seller) + "," +
		std::to_string(bidder) + "," + std::to_string(buyer) + ")");
	execute("DELETE FROM currency_wallet_baseline WHERE pid IN (" + std::to_string(seller) +
		"," + std::to_string(bidder) + "," + std::to_string(buyer) + ")");
	execute("DELETE FROM currency_bank_baseline WHERE bank_id IN (SELECT id FROM account_banks "
		"WHERE account_name LIKE 'auction_harness_%')");
	execute("DELETE FROM item_current_owner WHERE item_uid IN (" + std::to_string(item_uid) +
		"," + std::to_string(expired_uid) + ")");
	execute("DELETE FROM item_owner_revision WHERE (owner_type=6 AND owner_id IN (" +
		std::to_string(auction_id) + "," + std::to_string(expired_auction_id) +
		")) OR (owner_type=1 AND owner_id IN (" + std::to_string(seller) + "," +
		std::to_string(buyer) + "))");
	for (const char *account : { seller_account, bidder_account, buyer_account })
	{
		execute("DELETE FROM player_data WHERE account_name='" + std::string(account) +
			"'");
		execute("DELETE FROM account_banks WHERE account_name='" + std::string(account) +
			"'");
		execute("DELETE FROM accounts WHERE account_name='" + std::string(account) + "'");
	}
	mysql_close(connection);
	return 0;
}
