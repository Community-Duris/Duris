#include "persistence/ram_runtime.h"
#include "persistence/operation_log.h"
#include "persistence/critical_command_repository.h"
#include "persistence/persistence_mode.h"
#include "player/player_save_worker.h"
#include "economy/auction_ram.h"
#include "economy/shop_trade_runtime.h"
#include "economy/shop_trade_command.h"
#include "economy/auction_website.h"
#include "economy/auction_transaction.h"
#include "economy/economic_state.h"
#include "item/item_ownership_runtime.h"
#include "item/item_transfer_repository.h"
#include "player/player_snapshot_codec.h"
#include "player/player_load_repository.h"
#include "player/player_load_materialize.h"
#include "persistence/persistence_observability.h"
#include "core/utils.h"
#include "guild/artifact_guild_state.h"
#include "player/artifact_restitution_ram.h"
#include "sql/sql.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>

namespace
{
std::atomic<bool> hold{ false };
MYSQL *connect_db()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(connection, "127.0.0.1", getenv("DB_USER"), getenv("DB_PASSWD"),
				  getenv("DB_NAME"), atoi(getenv("DB_PORT")), nullptr, 0));
	assert(!mysql_query(connection, "SET time_zone='+00:00'"));
	return connection;
}
void sql(MYSQL *connection, const std::string &query)
{
	if (mysql_real_query(connection, query.data(), query.size()))
	{
		fprintf(stderr, "SQL failed: %s\n%s\n", mysql_error(connection), query.c_str());
		abort();
	}
}
uint64_t scalar(MYSQL *connection, const std::string &query)
{
	sql(connection, query);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const uint64_t value = strtoull(row[0], nullptr, 10);
	mysql_free_result(rows);
	return value;
}
critical_apply_result project(const critical_command &command, void *)
{
	while (hold.load())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(!mysql_thread_init());
	MYSQL *connection = connect_db();
	const auto result = critical_command_repository_apply(connection, command);
	if (result.outcome != critical_apply_outcome::applied &&
	    result.outcome != critical_apply_outcome::already_applied)
		fprintf(stderr, "Projection failed: type=%u outcome=%u code=%u mysql=%s\n",
			unsigned(command.type), unsigned(result.outcome), result.error_code,
			mysql_error(connection));
	mysql_close(connection);
	mysql_thread_end();
	return result;
}
player_save_apply_result unused_snapshot(const player_snapshot &, void *)
{
	abort();
}
critical_command bid(uint32_t pid, const char *account, int64_t amount, const char *request)
{
	critical_command command;
	assert(auction_website_build_bid(account, pid, 7, amount, request, &command));
	return command;
}
critical_completion submit(critical_command command)
{
	const auto admitted = command.source_site == critical_source_site::website ?
				      auction_transaction_submit_website(command) :
				      critical_command_coordinator_submit(command);
	assert(critical_submit_result_keeps_operation(admitted));
	critical_completion completion{};
	if (admitted == critical_submit_result::attached &&
	    critical_command_coordinator_get_completed(command.operation_id, &completion))
		return completion;
	for (unsigned i = 0; i < 100; ++i)
	{
		if (critical_command_coordinator_pulse(&completion, 1))
		{
			auction_transaction_handle_completions(&completion, 1);
			return completion;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	fprintf(stderr, "No completion for type=%u\n", unsigned(command.type));
	abort();
}
critical_command auction_request(auction_command_payload payload, uint8_t id)
{
	critical_operation_id operation{};
	operation.bytes[0] = id;
	critical_command command{};
	assert(auction_command_build(&command, operation, payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	return command;
}
auction_command_payload listing_payload()
{
	auction_command_payload p{};
	p.action = auction_action::list;
	p.actor_pid = 1;
	p.racewar = 1;
	strcpy(p.account_name.data(), "seller");
	strcpy(p.actor_name.data(), "Seller");
	strcpy(p.object_short.data(), "two distinct swords");
	p.expected_wallet_revision = 4;
	p.expected_bank_revision = 8;
	p.start_price = 1000;
	p.buy_price = 2000;
	p.listing_fee = 100;
	p.end_time = time(nullptr) + 3600;
	p.item_count = 2;
	p.items[0] = { 992, 3, 1234 };
	p.items[1] = { 991, 2, 1234 };
	memcpy(p.object_blob.data(), "legacy", 7);
	p.object_blob_size = 7;
	std::vector<player_item_snapshot> snapshots(2);
	for (size_t i = 0; i < 2; ++i)
	{
		auto &item = snapshots[1 - i];
		item.object_uid = 991 + i;
		item.vnum = 1234;
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		item.equipment_slot = -1;
		item.condition = 71 + i;
		item.craftsmanship = 32 + i;
		item.values[3] = 789 + i;
		item.timers[4] = 123456 + i;
		item.name = "different sword " + std::to_string(i);
		item.string_mask = 1;
	}
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode(snapshots, &encoded) ==
	       player_snapshot_codec_result::ok);
	assert(auction_blob_pack(&p, encoded));
	return p;
}
shop_trade_payload shop_payload(shop_trade_action action)
{
	shop_trade_payload p{};
	p.action = action;
	p.shop_id = 9;
	p.player_pid = 2;
	p.racewar = 1;
	strcpy(p.account_name.data(), "bidder");
	p.selected_item_uid = action == shop_trade_action::buy_produced ||
					      action == shop_trade_action::sell_destroy ?
				      10002 :
				      10001;
	p.target_root_item_uid = p.selected_item_uid;
	p.price = action == shop_trade_action::buy_existing ? 1000 :
		  action == shop_trade_action::sell_store   ? 500 :
		  action == shop_trade_action::buy_produced ? 200 :
		  action == shop_trade_action::sell_destroy ? 100 :
							      0;
	currency_command_result money;
	assert(economic_state_balances(2, "bidder", 1, &money));
	p.expected_wallet_revision = money.wallet_revision;
	p.expected_bank_revision = money.bank_revision;
	assert(shop_trade_runtime_revision(9, &p.expected_shop_revision));
	item_ownership_runtime_entry item{};
	if (action != shop_trade_action::buy_produced)
		assert(item_ownership_runtime_lookup(p.selected_item_uid, &item));
	else
	{
		item_ownership_runtime_entry stock{};
		assert(item_ownership_runtime_lookup(10001, &stock));
		p.stock_item_uid = 10001;
		p.expected_stock_item_revision = stock.item_revision;
		p.stock_vnum = 2345;
	}
	p.item_count = 1;
	p.items[0] = { p.selected_item_uid,	  p.selected_item_uid, 0, item.item_revision, 2345,
		       item_custody_state::active };
	if (action == shop_trade_action::buy_produced)
	{
		p.items[0].expected_item_revision = ITEM_TRANSFER_ABSENT_REVISION;
		p.items[0].expected_state = item_custody_state::absent;
	}
	if (action == shop_trade_action::buy_existing ||
	    action == shop_trade_action::discard_invalid)
	{
		p.stock_item_uid = item.item_uid;
		p.expected_stock_item_revision = item.item_revision;
		p.stock_vnum = item.vnum;
	}
	player_item_snapshot snapshot{};
	snapshot.object_uid = p.selected_item_uid;
	snapshot.vnum = 2345;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.condition = 88;
	snapshot.craftsmanship = 44;
	snapshot.equipment_slot = 0;
	snapshot.values[2] = 37;
	std::vector<uint8_t> bytes;
	assert(player_item_snapshot_list_encode({ snapshot }, &bytes) ==
	       player_snapshot_codec_result::ok);
	p.item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), p.item_blob.begin());
	return p;
}
void caught_up()
{
	assert(persistence_writer_wait_idle(15000));
	assert(operation_log_projection_caught_up());
}
void wallet(uint32_t pid, const char *account, int64_t platinum, uint64_t revision)
{
	currency_command_result value{};
	assert(economic_state_balances(pid, account, 1, &value));
	assert(value.wallet.amount[3] == platinum && value.wallet_revision == revision);
}
}
int main(int argc, char **argv)
{
	assert(argc == 3 && getenv("DB_NAME") &&
	       std::string(getenv("DB_NAME")).starts_with("ram_authority_test_"));
	assert(!mysql_library_init(0, nullptr, nullptr));
	char error[256]{};
	assert(persistence_mode_configure(error, sizeof(error)));
	MYSQL *connection = connect_db();
	DB = connection;
	const bool crash = !strcmp(argv[2], "crash");
	const std::string mode = argv[2];
	if (mode == "shop-buy-crash")
	{
		sql(connection, "INSERT INTO shopkeepers(shop_id) VALUES(9)");
		sql(connection, "INSERT INTO shop_trade_state(shop_id,revision) VALUES(9,1)");
		sql(connection,
		    "INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES(" +
			    std::to_string(static_cast<unsigned>(item_owner_type::shopkeeper)) +
			    ",10,4)");
		sql(connection,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum) VALUES(10001,10001," +
			    std::to_string(static_cast<unsigned>(item_owner_type::shopkeeper)) +
			    ",10,2,2345)");
		item_transfer_payload materialization{};
		materialization.to_owner = { item_owner_type::shopkeeper, 10, 0 };
		materialization.item_count = 1;
		materialization.selected_item_uid = materialization.target_root_item_uid = 10001;
		materialization.items[0] = { 10001, 10001, 0, 2, 2345, item_custody_state::active };
		player_item_snapshot snapshot{};
		snapshot.object_uid = 10001;
		snapshot.vnum = 2345;
		snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		snapshot.condition = 88;
		snapshot.craftsmanship = 44;
		snapshot.values[2] = 37;
		std::vector<uint8_t> bytes;
		assert(player_item_snapshot_list_encode({ snapshot }, &bytes) ==
		       player_snapshot_codec_result::ok);
		materialization.item_blob_size = bytes.size();
		std::copy(bytes.begin(), bytes.end(), materialization.item_blob.begin());
		assert(item_transfer_repository_materialize(connection, materialization));
	}
	const bool item_crash = !strcmp(argv[2], "item-crash"),
		   item_recover = !strcmp(argv[2], "item-recover");
	if (crash)
	{
		sql(connection,
		    "INSERT INTO artifacts(vnum,owned,locType,location,timer,type) VALUES(1234,'Y',3,2,FROM_UNIXTIME(2000000000),1)");
		sql(connection,
		    "INSERT INTO accounts(account_name) VALUES('seller'),('bidder'),('other'),('unbanked')");
		sql(connection,
		    "INSERT INTO account_banks(account_name,racewar,bank_platinum,bank_revision) VALUES('seller',1,3,8),('bidder',1,3,8),('other',1,3,8)");
		sql(connection,
		    "INSERT INTO player_data(pid,name,account_name,racewar,platinum,wallet_revision,save_revision) VALUES(1,'Seller','seller',1,10,4,1),(2,'Bidder','bidder',1,10,4,1),(3,'Other','other',1,10,4,1),(4,'Alt','bidder',1,10,4,1),(5,'Unbanked','unbanked',1,10,4,1)");
		sql(connection,
		    "INSERT INTO auctions(id,seller_pid,seller_name,cur_price,buy_price,obj_blob_str,end_time,auction_revision,custody_state) VALUES(7,1,'Seller',1000,5000,'fixture',FROM_UNIXTIME(UNIX_TIMESTAMP()+3600),1,1)");
	}
	if (item_crash)
	{
		sql(connection,
		    "INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES(1,1,4)");
		sql(connection,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum) VALUES(991,991,1,1,2,1234),(992,992,1,1,3,1234)");
		item_transfer_payload transfer{};
		auto p = listing_payload();
		p.action = auction_action::claim_item;
		assert(auction_item_payload(p, 8, &transfer));
		assert(item_transfer_repository_materialize(connection, transfer));
	}
	assert(operation_log_init(argv[1], project, nullptr));
	std::string hydration_error;
	if (!ram_runtime_hydrate(connection, nullptr, &hydration_error))
	{
		fprintf(stderr, "RAM hydration failed: %s, mysql=%s\n", hydration_error.c_str(),
			mysql_error(connection));
		abort();
	}
	currency_command_result unbanked{};
	assert(economic_state_balances(5, "unbanked", 1, &unbanked) && !unbanked.bank_revision &&
	       !unbanked.bank.amount[3]);
	assert(player_save_worker_init(unused_snapshot, nullptr));
	assert(ram_runtime_start());
	assert(artifact_guild_state_hydrate());
	if (mode.starts_with("shop-"))
	{
		if (mode == "shop-sell-crash")
		{
			assert(scalar(connection,
				      "SELECT item_condition FROM player_items WHERE obj_uid=10001") ==
			       88);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM shopkeeper_items WHERE obj_uid=10001") ==
			       0);
		}
		if (mode == "shop-produced-crash")
		{
			assert(scalar(connection,
				      "SELECT item_condition FROM shopkeeper_items WHERE obj_uid=10001") ==
			       88);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE obj_uid=10001") ==
			       0);
		}
		if (mode == "shop-destroy-crash")
		{
			assert(scalar(connection,
				      "SELECT item_condition FROM player_items WHERE obj_uid=10002") ==
			       88);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM shopkeeper_items WHERE obj_uid=10001") ==
			       1);
		}
		if (mode == "shop-discard-crash" || mode == "shop-recover")
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE obj_uid IN(10001,10002)") ==
			       0);
		if (mode == "shop-recover")
		{
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM shopkeeper_items WHERE obj_uid=10001") ==
			       0);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN(10001,10002) AND state=2") ==
			       2);
			currency_command_result final;
			assert(economic_state_balances(2, "bidder", 1, &final));
			assert(final.wallet.amount[3] == 1 && final.wallet.amount[2] == 4);
			puts("RAM runtime: shop buy, sell, produced stock, destroy and invalid stock crash recovery passed");
			critical_command_coordinator_shutdown();
			player_save_worker_shutdown();
			ram_runtime_shutdown();
			mysql_close(connection);
			return 0;
		}
		auto action = mode == "shop-buy-crash"	    ? shop_trade_action::buy_existing :
			      mode == "shop-sell-crash"	    ? shop_trade_action::sell_store :
			      mode == "shop-produced-crash" ? shop_trade_action::buy_produced :
			      mode == "shop-destroy-crash"  ? shop_trade_action::sell_destroy :
							      shop_trade_action::discard_invalid;
		auto payload = shop_payload(action);
		critical_operation_id id{};
		id.bytes[0] = 100 + static_cast<unsigned>(action);
		critical_command request;
		assert(shop_trade_command_build(&request, id, payload,
						critical_source_site::command,
						critical_deadline_class::interactive));
		hold = true;
		auto outcome = submit(request);
		if (outcome.error_code)
			fprintf(stderr, "shop prepare failed: %u\n", outcome.error_code);
		assert(outcome.outcome == critical_apply_outcome::applied && !outcome.error_code);
		_exit(0);
	}
	if (item_crash || item_recover)
	{
		auto payload = listing_payload();
		auto listed_command = auction_request(payload, 90);
		const uint32_t id = auction_command_listing_id(listed_command.operation_id);
		if (item_crash)
		{
			auto listed = submit(listed_command);
			assert(listed.outcome == critical_apply_outcome::applied &&
			       !listed.error_code);
			caught_up();
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE obj_uid IN(991,992)") ==
			       0);
			critical_command buy;
			assert(auction_website_build_bid("bidder", 2, id, 2000, "item-buyout",
							 &buy));
			assert(submit(buy).outcome == critical_apply_outcome::applied);
			caught_up();
			payload.action = auction_action::claim_item;
			payload.actor_pid = 2;
			payload.auction_id = id;
			strcpy(payload.account_name.data(), "bidder");
			strcpy(payload.actor_name.data(), "Bidder");
			payload.expected_wallet_revision = payload.expected_bank_revision =
				UINT64_MAX;
			++payload.items[0].expected_item_revision;
			++payload.items[1].expected_item_revision;
			hold = true;
			const auto claimed = submit(auction_request(payload, 91));
			assert(claimed.outcome == critical_apply_outcome::applied &&
			       !claimed.error_code);
			assert(scalar(connection,
				      "SELECT COUNT(*) FROM player_items WHERE obj_uid IN(991,992)") ==
			       0);
			item_ownership_runtime_entry owner{};
			assert(item_ownership_runtime_lookup(991, &owner) && owner.owner.id == 2 &&
			       owner.owner.type == item_owner_type::player);
			_exit(0);
		}
		assert(scalar(connection,
			      "SELECT COUNT(*) FROM player_items WHERE pid=2 AND obj_uid IN(991,992) AND equip_slot=0") ==
		       2);
		assert(scalar(connection,
			      "SELECT item_condition FROM player_items WHERE obj_uid=991") == 71);
		assert(scalar(connection, "SELECT value3 FROM player_items WHERE obj_uid=992") ==
		       790);
		sql(connection,
		    "SELECT r.payload FROM player_item_runtime_state r JOIN player_items i ON i.id=r.item_id WHERE i.obj_uid=992");
		MYSQL_RES *rows = mysql_store_result(connection);
		MYSQL_ROW row = mysql_fetch_row(rows);
		std::vector<player_item_snapshot> decoded;
		assert(row &&
		       player_item_snapshot_list_decode(reinterpret_cast<const uint8_t *>(row[0]),
							mysql_fetch_lengths(rows)[0], &decoded) ==
			       player_snapshot_codec_result::ok);
		assert(decoded.size() == 1 && decoded[0].object_uid == 992 &&
		       decoded[0].craftsmanship == 33 && decoded[0].timers[4] == 123457);
		mysql_free_result(rows);
		puts("RAM runtime: distinct stacked item listing, buyout, claim crash and physical recovery passed");
		critical_command_coordinator_shutdown();
		player_save_worker_shutdown();
		ram_runtime_shutdown();
		mysql_close(connection);
		return 0;
	}
	if (crash)
	{
		player_load_request request;
		request.request_id = 1;
		request.pid = 2;
		request.account_name = "bidder";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		auto stale_login = player_load_repository_execute(connection, request);
		// This isolated persistence process has no booted world rooms.
		stale_login.snapshot.room_vnum = NOWHERE;
		assert(stale_login.domains.wallet[3] == 10);
		hold = true;
		const auto accepted = submit(bid(2, "bidder", 1000, "offline-first"));
		assert(accepted.outcome == critical_apply_outcome::applied);
		wallet(2, "bidder", 9, 5);
		assert(scalar(connection, "SELECT platinum FROM player_data WHERE pid=2") == 10);
		assert(scalar(connection, "SELECT winning_bidder_pid FROM auctions WHERE id=7") ==
		       0);
		// A load that began before the web bid must attach to the same RAM owner.
		char_data character{};
		pc_only_data pc{};
		character.only.pc = &pc;
		character.in_room = NOWHERE;
		assert(player_load_materialize(&character, stale_login));
		assert(GET_PLATINUM(&character) == 9 && pc.wallet_revision == 5 &&
		       pc.bank_revision == 9);
		// Abrupt process death after the durable acknowledgement and RAM publication.
		// No destructor or shutdown is allowed to project the pending bid.
		_exit(0);
	}
	wallet(2, "bidder", 9, 5);
	assert(scalar(connection, "SELECT platinum FROM player_data WHERE pid=2") == 9);
	assert(scalar(connection, "SELECT winning_bidder_pid FROM auctions WHERE id=7") == 2);
	assert(scalar(connection, "SELECT COUNT(*) FROM auction_bid_history") == 1);
	auto duplicate = submit(bid(2, "BIDDER", 1000, "offline-first"));
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	wallet(2, "bidder", 9, 5);
	assert(auction_transaction_submit_website(bid(2, "bidder", 2000, "offline-first")) ==
	       critical_submit_result::identity_conflict);
	critical_command unauthorized;
	assert(!auction_website_build_bid("other", 2, 7, 2000, "spoof", &unauthorized));
	// A second offline account outbids; bidder escrow and the refund recover whole.
	hold = true;
	auto replaced = submit(bid(3, "other", 2000, "offline-second"));
	assert(replaced.outcome == critical_apply_outcome::applied);
	wallet(3, "other", 8, 5);
	auction_ram_pickup refund{};
	assert(auction_ram_find_pickup(2, &refund) && refund.amount == 1000);
	assert(scalar(connection, "SELECT winning_bidder_pid FROM auctions WHERE id=7") == 2);
	hold = false;
	caught_up();
	assert(scalar(connection, "SELECT money FROM auction_money_pickups WHERE pid=2") == 1000);
	assert(scalar(connection, "SELECT winning_bidder_pid FROM auctions WHERE id=7") == 3);
	// Synchronous buyout includes the prior bidder's refund and seller proceeds.
	auto sold = submit(bid(2, "bidder", 5000, "offline-buyout"));
	assert(sold.outcome == critical_apply_outcome::applied);
	caught_up();
	wallet(2, "bidder", 4, 6);
	assert(scalar(connection, "SELECT money FROM auction_money_pickups WHERE pid=3") == 2000);
	assert(scalar(connection, "SELECT money FROM auction_money_pickups WHERE pid=1") == 4850);
	assert(scalar(connection, "SELECT status+0 FROM auctions WHERE id=7") == 2);
	// Maintenance cannot read old rows and overwrite a durably logged feed.
	artifact_guild_payload feed{};
	feed.actor_pid = 2;
	feed.parent_operation_id = bid(2, "bidder", 5000, "offline-buyout").operation_id;
	feed.artifact_count = 1;
	const auto before = *artifact_restitution_ram_lookup(1234);
	feed.artifacts[0] = { 1234,
			      ARTIFACT_DELTA_FEED,
			      before.revision,
			      before.domain.timer,
			      before.domain.timer + 600,
			      before.bind_owner,
			      before.bind_owner,
			      before.bind_timer,
			      before.bind_timer };
	critical_operation_id feed_id{};
	feed_id.bytes[0] = 92;
	critical_command feed_command;
	assert(artifact_guild_command_build(&feed_command, feed_id, feed));
	hold = true;
	assert(submit(feed_command).outcome == critical_apply_outcome::applied);
	assert(artifact_restitution_ram_lookup(1234)->domain.timer == before.domain.timer + 600);
	std::thread release_projection(
		[]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			hold = false;
		});
	{
		artifact_maintenance_scope maintenance;
		assert(maintenance);
		assert(scalar(connection,
			      "SELECT UNIX_TIMESTAMP(timer) FROM artifacts WHERE vnum=1234") ==
		       static_cast<uint64_t>(before.domain.timer + 600));
		sql(connection,
		    "UPDATE artifacts SET timer=DATE_ADD(timer,INTERVAL 60 SECOND) WHERE vnum=1234");
	}
	release_projection.join();
	assert(artifact_restitution_ram_lookup(1234)->domain.timer == before.domain.timer + 660);
	assert(artifact_restitution_ram_lookup(1234)->revision == before.revision + 2);
	// Closing the auction rejects further intent durably, including after restart.
	auto rejected_request = bid(3, "other", 6000, "closed-rejection");
	auto rejected = submit(rejected_request);
	assert(rejected.outcome == critical_apply_outcome::terminal_failure && rejected.error_code);
	critical_command_coordinator_shutdown();
	player_save_worker_shutdown();
	ram_runtime_shutdown();
	item_ownership_runtime_reset();
	assert(operation_log_init(argv[1], project, nullptr));
	assert(ram_runtime_hydrate(connection, nullptr, &hydration_error));
	assert(player_save_worker_init(unused_snapshot, nullptr));
	assert(ram_runtime_start());
	auto again = submit(rejected_request);
	assert(again.outcome == rejected.outcome && again.error_code == rejected.error_code);
	wallet(2, "bidder", 4, 6);
	assert(auction_ram_find_pickup(1, &refund) && refund.amount == 4850);
	assert(scalar(connection, "SELECT COUNT(*) FROM auction_bid_history") == 3);
	critical_command_coordinator_shutdown();
	player_save_worker_shutdown();
	ram_runtime_shutdown();
	mysql_close(connection);
	puts("RAM runtime: offline bid, lag, crash, cold recovery, duplicate identity, outbid, buyout and negative receipt passed");
}
