#include "persistence/critical_command_repository.h"
#include "economy/currency_command.h"
#include "economy/coin_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "player/player_load_repository.h"
#include "persistence/persistence_observability.h"
#include "core/structs.h"
#include "world/vnum.obj.h"

#include <mysql.h>

#include <cassert>
#include <algorithm>
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

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()) != 0)
	{
		fprintf(stderr, "fixture SQL failed: %s\n%s\n", sql.c_str(),
			mysql_error(connection));
		abort();
	}
}

long long scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	const long long value = strtoll(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

std::string operation_hex(const critical_operation_id &operation_id)
{
	char value[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation_id, value, sizeof(value)));
	return value;
}

critical_command command_for(uint32_t pid, const char *account_name,
			     const currency_vector &wallet_delta, const currency_vector &bank_delta,
			     currency_reason_type reason, uint64_t wallet_revision = UINT64_MAX,
			     uint64_t bank_revision = UINT64_MAX)
{
	critical_operation_id operation_id = {};
	assert(critical_operation_id_generate(&operation_id));
	currency_command_payload payload = { .pid = pid,
					     .racewar = 1,
					     .reason = reason,
					     .reason_id = 77,
					     .account_name = {},
					     .wallet_delta = wallet_delta,
					     .bank_delta = bank_delta };
	assert(strlen(account_name) < payload.account_name.size());
	memcpy(payload.account_name.data(), account_name, strlen(account_name));
	critical_command command = {};
	assert(currency_command_build(&command, operation_id, payload, wallet_revision,
				      bank_revision, critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return command;
}

currency_command_result currency_result_of(const critical_apply_result &applied)
{
	currency_command_result result = {};
	assert(currency_command_decode_result(applied.result_payload.data(), applied.result_size,
					      &result));
	return result;
}

using coins = std::array<int32_t, 4>;

uint64_t owner_revision(const item_owner_identity &owner)
{
	return scalar(
		"SELECT COALESCE(MAX(revision),0) FROM item_owner_revision WHERE owner_type=" +
		std::to_string(static_cast<unsigned int>(owner.type)) +
		" AND owner_id=" + std::to_string(owner.id) +
		" AND owner_context_id=" + std::to_string(owner.context_id));
}

coin_transfer_endpoint coin_wallet(uint32_t pid, const char *account, coins before, coins after)
{
	coin_transfer_endpoint endpoint;
	endpoint.before = before;
	endpoint.after = after;
	currency_vector delta = {};
	for (size_t index = 0; index < 4; ++index)
		delta.amount[index] = static_cast<int64_t>(after[index]) - before[index];
	endpoint.change = command_for(
		pid, account, delta, {}, currency_reason_type::coin_transfer,
		scalar("SELECT wallet_revision FROM player_data WHERE pid=" + std::to_string(pid)),
		scalar("SELECT COALESCE(MAX(bank_revision),0) FROM account_banks WHERE account_name='" +
		       std::string(account) + "' AND racewar=1"));
	return endpoint;
}

coin_transfer_endpoint coin_pile(item_owner_identity owner, uint64_t uid, uint64_t parent,
				 coins before, coins after)
{
	coin_transfer_endpoint endpoint;
	endpoint.before = before;
	endpoint.after = after;
	const bool created = before == coins{}, consumed = after == coins{};
	item_transfer_payload payload = {};
	payload.from_owner = created ? item_owner_identity{ item_owner_type::system, 0, 0 } : owner;
	payload.to_owner = consumed ? item_owner_identity{ item_owner_type::destruction, 0, 0 } :
				      owner;
	payload.expected_from_revision = owner_revision(payload.from_owner);
	payload.expected_to_revision = owner_revision(payload.to_owner);
	payload.reason = created  ? item_transfer_reason::creation :
			 consumed ? item_transfer_reason::destruction :
				    item_transfer_reason::player_put;
	payload.selected_item_uid = uid;
	payload.target_root_item_uid = consumed || !parent ? uid : parent;
	payload.target_parent_item_uid = consumed ? 0 : parent;
	payload.expected_target_parent_revision =
		payload.target_parent_item_uid ?
			scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=" +
			       std::to_string(parent)) :
			0;
	payload.item_count = 1;
	payload.items[0] = {
		uid,
		created || !parent ? uid : parent,
		created ? 0 : parent,
		created ? ITEM_TRANSFER_ABSENT_REVISION :
			  static_cast<uint64_t>(scalar(
				  "SELECT item_revision FROM item_current_owner WHERE item_uid=" +
				  std::to_string(uid))),
		VOBJ_COINS,
		created ? item_custody_state::absent : item_custody_state::active
	};
	player_item_snapshot snapshot = {};
	snapshot.object_uid = uid;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.vnum = VOBJ_COINS;
	snapshot.type = ITEM_MONEY;
	snapshot.string_mask = 7;
	snapshot.name = "coins";
	snapshot.short_description = "a pile of coins";
	snapshot.description = "A pile of coins lies here.";
	const auto &amount = consumed ? before : after;
	std::copy(amount.begin(), amount.end(), snapshot.values.begin());
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ snapshot }, &blob) ==
	       player_snapshot_codec_result::ok);
	payload.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	critical_operation_id id = {};
	assert(critical_operation_id_generate(&id));
	assert(item_transfer_command_build(&endpoint.change, id, payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	return endpoint;
}

coin_transfer_endpoint coin_pile(uint32_t pid, uint64_t uid, uint64_t parent, coins before,
				 coins after)
{
	return coin_pile({ item_owner_type::player, pid, 0 }, uid, parent, before, after);
}

critical_command coin_command(coin_transfer_endpoint source, coin_transfer_endpoint destination)
{
	critical_operation_id id = {};
	assert(critical_operation_id_generate(&id));
	critical_command command;
	assert(coin_transfer_command_build(&command, id, { source, destination },
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return command;
}

coins pile_amount(uint64_t uid)
{
	execute("SELECT coin_payload FROM item_current_owner WHERE item_uid=" +
		std::to_string(uid));
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	const auto *lengths = mysql_fetch_lengths(rows);
	assert(row && row[0] && lengths);
	std::vector<player_item_snapshot> items;
	assert(player_item_snapshot_list_decode(reinterpret_cast<const uint8_t *>(row[0]),
						lengths[0],
						&items) == player_snapshot_codec_result::ok);
	assert(items.size() == 1 && items[0].object_uid == uid && items[0].name == "coins");
	coins amounts;
	std::copy_n(items[0].values.begin(), 4, amounts.begin());
	mysql_free_result(rows);
	return amounts;
}

void coin_failure_matrix()
{
	constexpr uint64_t bag = 900000001, pile = 900000002;
	const char *account = "coin_matrix_account";
	execute("INSERT INTO accounts(account_name,password) VALUES('coin_matrix_account','')");
	execute("INSERT INTO player_data(name,account_name,racewar,copper,last_room) VALUES('CoinMatrix','coin_matrix_account',1,1000,100)");
	const uint32_t pid = mysql_insert_id(connection);
	const std::string pid_text = std::to_string(pid);
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES(1," +
		pid_text + ",1)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum,state) "
		"VALUES(900000001,900000001,1," +
		pid_text + ",1,96443,1)");
	execute("INSERT INTO player_items(pid,vnum,obj_uid) VALUES(" + pid_text +
		",96443,900000001)");
	const auto bag_row = mysql_insert_id(connection);
	auto verify_reload = [&](int32_t wallet, int32_t amount)
	{
		player_load_request request;
		request.request_id = 1;
		request.pid = pid;
		request.account_name = account;
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		const auto loaded = player_load_repository_execute(connection, request);
		if (loaded.outcome != player_load_outcome::applied)
			fprintf(stderr,
				"coin reload failed: outcome=%u error=%u stage=%s queries=%u mysql=%s\n",
				unsigned(loaded.outcome), loaded.error_code,
				loaded.failed_component ? loaded.failed_component : "none",
				loaded.metrics.query_count, mysql_error(connection));
		assert(loaded.outcome == player_load_outcome::applied);
		assert(loaded.domains.wallet[0] == static_cast<uint64_t>(wallet));
		assert(loaded.missing_payload_rows == 0);
		assert(loaded.stale_item_rows == 0);
		assert(loaded.snapshot.items.size() == (amount ? 2u : 1u));
		assert(loaded.authoritative_item_count == loaded.snapshot.items.size());
		const auto found = std::find_if(loaded.snapshot.items.begin(),
						loaded.snapshot.items.end(),
						[&](const player_item_snapshot &item)
						{ return item.object_uid == pile; });
		if (!amount)
			assert(found == loaded.snapshot.items.end());
		else
		{
			assert(found != loaded.snapshot.items.end());
			assert(found->values[0] == amount && found->values[1] == 0 &&
			       found->values[2] == 0 && found->values[3] == 0);
			assert(found->name == "coins");
			assert(found->parent_index >= 0 &&
			       static_cast<size_t>(found->parent_index) <
				       loaded.snapshot.items.size());
			assert(loaded.snapshot.items[found->parent_index].object_uid == bag);
		}
	};
	auto make_put = [&](int32_t wallet_before, int32_t pile_before, int32_t amount)
	{
		return coin_command(coin_wallet(pid, account, { wallet_before, 0, 0, 0 },
						{ wallet_before - amount, 0, 0, 0 }),
				    coin_pile(pid, pile, bag, { pile_before, 0, 0, 0 },
					      { pile_before + amount, 0, 0, 0 }));
	};
	critical_command put = make_put(1000, 0, 100);
	auto applied = critical_command_repository_apply(connection, put);
	if (applied.error_code)
		fprintf(stderr, "coin put failed: outcome=%u error=%u mysql=%s\n",
			unsigned(applied.outcome), applied.error_code, mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 900);
	assert((pile_amount(pile) == coins{ 100, 0, 0, 0 }));
	assert(scalar("SELECT parent_item_uid FROM item_current_owner WHERE item_uid=900000002") ==
	       bag);
	coin_transfer_payload decoded;
	coin_transfer_result result;
	assert(coin_transfer_command_decode_payload(put, &decoded));
	assert(coin_transfer_command_decode_result(decoded, applied.result_payload.data(),
						   applied.result_size, &result));
	assert(result.wallets[0].wallet.amount[0] == 900 && result.piles[1].max_item_revision == 1);
	// A crash before the inventory snapshot leaves no player_items coin row.
	verify_reload(900, 100);
	// A later stale projection must not replace the committed amount or metadata.
	execute("INSERT INTO player_items(pid,vnum,obj_uid,container_id,value0,name) VALUES(" +
		pid_text + ",3,900000002," + std::to_string(bag_row) + ",1,'stale coins')");

	critical_command merge = make_put(900, 100, 200);
	assert(critical_command_repository_apply(connection, merge).outcome ==
	       critical_apply_outcome::applied);
	// Discard the first acknowledgement, then replay its operation ID.
	assert(critical_command_repository_apply(connection, merge).outcome ==
	       critical_apply_outcome::already_applied);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	verify_reload(700, 300);
	const auto ledger_count =
		scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text);
	const auto item_revision =
		scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=900000002");
	critical_command stale = make_put(700, 299, 50);
	applied = critical_command_repository_apply(connection, stale);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ESTALE);
	assert(critical_command_repository_apply(connection, stale).error_code == ESTALE);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
	assert(scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=900000002") ==
	       item_revision);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));

	critical_command fault = make_put(700, 300, 50);
	execute("CREATE TRIGGER coin_injected_failure BEFORE UPDATE ON item_current_owner FOR EACH ROW "
		"SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected coin write failure'");
	applied = critical_command_repository_apply(connection, fault);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == 1644);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	execute("DROP TRIGGER coin_injected_failure");
	assert(critical_command_repository_apply(connection, fault).outcome ==
	       critical_apply_outcome::applied);
	assert((pile_amount(pile) == coins{ 350, 0, 0, 0 }));
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 650);

	critical_command partial =
		coin_command(coin_pile(pid, pile, bag, { 350, 0, 0, 0 }, { 125, 0, 0, 0 }),
			     coin_wallet(pid, account, { 650, 0, 0, 0 }, { 875, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, partial).outcome ==
	       critical_apply_outcome::applied);
	assert((pile_amount(pile) == coins{ 125, 0, 0, 0 }));
	verify_reload(875, 125);
	critical_command take_all =
		coin_command(coin_pile(pid, pile, bag, { 125, 0, 0, 0 }, {}),
			     coin_wallet(pid, account, { 875, 0, 0, 0 }, { 1000, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, take_all).outcome ==
	       critical_apply_outcome::applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 1000);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=900000002 AND state=2 "
		      "AND coin_payload IS NULL AND parent_item_uid IS NULL AND root_item_uid=item_uid") ==
	       1);
	verify_reload(1000, 0);
	// Repeated completed pickups before an inventory snapshot must not look like
	// a damaged inventory and trip the materializer's stale-row refusal threshold.
	for (size_t index = 0; index <= PLAYER_LOAD_ITEM_SKIP_MAX; ++index)
	{
		const uint64_t uid = pile + 1 + index;
		auto small_put =
			coin_command(coin_wallet(pid, account, { 1000, 0, 0, 0 }, { 999, 0, 0, 0 }),
				     coin_pile(pid, uid, bag, {}, { 1, 0, 0, 0 }));
		assert(critical_command_repository_apply(connection, small_put).outcome ==
		       critical_apply_outcome::applied);
		execute("INSERT INTO player_items(pid,vnum,obj_uid,container_id,value0) VALUES(" +
			pid_text + ",3," + std::to_string(uid) + "," + std::to_string(bag_row) +
			",1)");
		auto small_get = coin_command(coin_pile(pid, uid, bag, { 1, 0, 0, 0 }, {}),
					      coin_wallet(pid, account, { 999, 0, 0, 0 },
							  { 1000, 0, 0, 0 }));
		assert(critical_command_repository_apply(connection, small_get).outcome ==
		       critical_apply_outcome::applied);
	}
	verify_reload(1000, 0);

	// The emptied bag now has exactly its live subtree: the consumed pile cannot
	// become the extra authoritative descendant that rejects its next handoff.
	item_transfer_payload drop = {};
	drop.from_owner = { item_owner_type::player, pid, 0 };
	drop.to_owner = { item_owner_type::room, 100, 0 };
	drop.expected_from_revision = owner_revision(drop.from_owner);
	drop.reason = item_transfer_reason::player_drop;
	drop.selected_item_uid = bag;
	drop.target_root_item_uid = bag;
	drop.item_count = 1;
	drop.items[0] = { bag, bag, 0, 1, 96443, item_custody_state::active };
	critical_operation_id drop_id = {};
	assert(critical_operation_id_generate(&drop_id));
	critical_command drop_command;
	assert(item_transfer_command_build(&drop_command, drop_id, drop,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	drop_command.accepted_at_usec = 1;
	assert(critical_command_repository_apply(connection, drop_command).outcome ==
	       critical_apply_outcome::applied);

	// Giving between characters on the same account advances the shared bank
	// fence within this commit instead of rejecting after the first debit.
	execute("INSERT INTO player_data(name,account_name,racewar,copper) VALUES('CoinRecipient','coin_matrix_account',1,0)");
	const uint32_t recipient = mysql_insert_id(connection);
	critical_command give =
		coin_command(coin_wallet(pid, account, { 1000, 0, 0, 0 }, { 990, 0, 0, 0 }),
			     coin_wallet(recipient, account, {}, { 10, 0, 0, 0 }));
	applied = critical_command_repository_apply(connection, give);
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, give).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 990);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(recipient)) ==
	       10);
	// Pre-upgrade piles have no canonical blob. Establish their amounts from the
	// actual owner-specific payload stores, including current private/public lockers.
	execute("INSERT INTO lockers(locker_name,owner_pid) VALUES('CoinLocker'," + pid_text + ")");
	const uint64_t locker_id = mysql_insert_id(connection);
	execute("INSERT INTO private_chests(locker_id,chest_name) VALUES(" +
		std::to_string(locker_id) + ",'coins')");
	const uint64_t chest_id = mysql_insert_id(connection);
	execute("INSERT INTO corpses(player_name,save_id,room_vnum) VALUES('CoinMatrix',77,100)");
	const uint64_t corpse_id = mysql_insert_id(connection);
	const std::array<item_owner_identity, 4> owners = {
		{ { item_owner_type::player, pid, 0 },
		  { item_owner_type::room, 100, 0 },
		  { item_owner_type::corpse, item_corpse_owner_id(pid, 77), 0 },
		  { item_owner_type::locker, locker_id, chest_id } }
	};
	int32_t wallet = 990;
	for (size_t index = 0; index < owners.size(); ++index)
	{
		const auto &owner = owners[index];
		const uint64_t uid = 900000100 + index;
		const std::string uid_text = std::to_string(uid);
		execute("INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(" +
			std::to_string(unsigned(owner.type)) + "," + std::to_string(owner.id) +
			"," + std::to_string(owner.context_id) + ",1)");
		execute("INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
			uid_text + "," + uid_text + "," + std::to_string(unsigned(owner.type)) +
			"," + std::to_string(owner.id) + "," + std::to_string(owner.context_id) +
			",1,3,1)");
		switch (owner.type)
		{
		case item_owner_type::player:
			execute("INSERT INTO player_items(pid,vnum,obj_uid,value0) VALUES(" +
				pid_text + ",3," + uid_text + ",30)");
			break;
		case item_owner_type::room:
			execute("INSERT INTO saved_items(item_key,room_vnum,vnum,obj_uid,value0) VALUES('coin_fixture',100,3," +
				uid_text + ",30)");
			break;
		case item_owner_type::corpse:
			execute("INSERT INTO corpse_items(corpse_id,vnum,obj_uid,value0) VALUES(" +
				std::to_string(corpse_id) + ",3," + uid_text + ",30)");
			break;
		case item_owner_type::locker:
			execute("INSERT INTO locker_items(locker_id,chest_id,vnum,obj_uid,value0) VALUES(" +
				std::to_string(locker_id) + "," + std::to_string(chest_id) + ",3," +
				uid_text + ",30)");
			break;
		default:
			assert(false);
		}
		auto pickup = coin_command(
			coin_pile(owner, uid, 0, { 30, 0, 0, 0 }, { 20, 0, 0, 0 }),
			coin_wallet(pid, account, { wallet, 0, 0, 0 }, { wallet + 10, 0, 0, 0 }));
		assert(critical_command_repository_apply(connection, pickup).outcome ==
		       critical_apply_outcome::applied);
		assert((pile_amount(uid) == coins{ 20, 0, 0, 0 }));
		wallet += 10;
		auto remainder = coin_command(coin_pile(owner, uid, 0, { 20, 0, 0, 0 }, {}),
					      coin_wallet(pid, account, { wallet, 0, 0, 0 },
							  { wallet + 20, 0, 0, 0 }));
		assert(critical_command_repository_apply(connection, remainder).outcome ==
		       critical_apply_outcome::applied);
		wallet += 20;
	}
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 1110);
	// Merge two physical piles in the dropped bag. Both legs share the room's
	// owner revision; the second must use the revision advanced by the first.
	const item_owner_identity room_owner = { item_owner_type::room, 100, 0 };
	constexpr uint64_t merge_source = 900000200, merge_destination = 900000201;
	auto first_pile =
		coin_command(coin_wallet(pid, account, { 1110, 0, 0, 0 }, { 1090, 0, 0, 0 }),
			     coin_pile(room_owner, merge_source, bag, {}, { 20, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, first_pile).outcome ==
	       critical_apply_outcome::applied);
	auto second_pile =
		coin_command(coin_wallet(pid, account, { 1090, 0, 0, 0 }, { 1060, 0, 0, 0 }),
			     coin_pile(room_owner, merge_destination, bag, {}, { 30, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, second_pile).outcome ==
	       critical_apply_outcome::applied);
	auto combine = coin_command(coin_pile(room_owner, merge_source, bag, { 20, 0, 0, 0 }, {}),
				    coin_pile(room_owner, merge_destination, bag, { 30, 0, 0, 0 },
					      { 50, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, combine).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, combine).outcome ==
	       critical_apply_outcome::already_applied);
	assert((pile_amount(merge_destination) == coins{ 50, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=900000200 "
		      "AND state=2 AND coin_payload IS NULL AND parent_item_uid IS NULL") == 1);
	auto combined_pickup =
		coin_command(coin_pile(room_owner, merge_destination, bag, { 50, 0, 0, 0 }, {}),
			     coin_wallet(pid, account, { 1060, 0, 0, 0 }, { 1110, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, combined_pickup).outcome ==
	       critical_apply_outcome::applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 1110);
	// Admit a pre-existing NPC pile with the ordinary item command. Its wallet
	// remains untouched until the following atomic coin command commits.
	const item_owner_identity npc_room = { item_owner_type::room, 101, 0 };
	constexpr uint64_t npc_uid = 900000300;
	auto admission = coin_pile(npc_room, npc_uid, 0, {}, { 50, 0, 0, 0 }).change;
	admission.accepted_at_usec = 1;
	assert(critical_command_repository_apply(connection, admission).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, admission).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 1110);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=900000300 "
		      "AND owner_type=3 AND owner_id=101 AND root_item_uid=900000300 AND parent_item_uid IS NULL") ==
	       1);
	assert((pile_amount(npc_uid) == coins{ 50, 0, 0, 0 }));
	auto reject_admission = [&]
	{
		auto conflict = coin_pile(npc_room, npc_uid, 0, {}, { 50, 0, 0, 0 }).change;
		conflict.accepted_at_usec = 1;
		assert(critical_command_repository_apply(connection, conflict).error_code ==
		       EEXIST);
		assert(critical_command_repository_apply(connection, conflict).error_code ==
		       EEXIST);
	};
	reject_admission();
	auto npc_pickup =
		coin_command(coin_pile(npc_room, npc_uid, 0, { 50, 0, 0, 0 }, {}),
			     coin_wallet(pid, account, { 1110, 0, 0, 0 }, { 1160, 0, 0, 0 }));
	assert(critical_command_repository_apply(connection, npc_pickup).outcome ==
	       critical_apply_outcome::applied);
	assert(critical_command_repository_apply(connection, npc_pickup).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 1160);
	reject_admission(); // A missing runtime entry cannot revive retired durable custody.
	puts("coin SQL: atomic conversion, rollback/replay, reload, retired custody and legacy player/room/corpse/locker piles passed");
}
} // namespace

int main()
{
	const char *host = getenv("DB_HOST"), *user = getenv("DB_USER"),
		   *password = getenv("DB_PASSWD"), *database = getenv("CURRENCY_TEST_DB_NAME"),
		   *port_value = getenv("DB_PORT");
	assert(host && user && password && database);
	connection = mysql_init(nullptr);
	assert(connection);
	const unsigned int port = port_value ? static_cast<unsigned int>(atoi(port_value)) : 3306;
	assert(mysql_real_connect(connection, host, user, password, database, port, nullptr, 0));

	const std::string account = "currency_harness_account";
	execute("DELETE FROM currency_wallet_baseline WHERE pid IN (SELECT pid FROM player_data "
		"WHERE name='CurrencyHarness')");
	execute("DELETE FROM currency_bank_baseline WHERE bank_id IN (SELECT id FROM account_banks "
		"WHERE account_name='" +
		account + "')");
	execute("DELETE FROM player_data WHERE name='CurrencyHarness'");
	execute("DELETE FROM account_banks WHERE account_name='" + account + "'");
	execute("DELETE FROM accounts WHERE account_name='" + account + "'");
	execute("INSERT INTO accounts(account_name,password) VALUES('" + account +
		"','') ON DUPLICATE KEY UPDATE account_name=VALUES(account_name)");
	execute("INSERT INTO player_data(name,account_name,racewar,copper,silver,gold,platinum) "
		"VALUES('CurrencyHarness','" +
		account + "',1,9,8,7,6)");
	const uint32_t pid = static_cast<uint32_t>(mysql_insert_id(connection));
	assert(scalar("SELECT COUNT(*) FROM account_banks WHERE account_name='" + account + "'") ==
	       0);
	uint32_t bank_id = 0;

	const currency_vector wallet_deposit = { { -9, -8, -7, -6 } };
	const currency_vector bank_deposit = { { 9, 8, 7, 6 } };
	critical_command deposit = command_for(pid, account.c_str(), wallet_deposit, bank_deposit,
					       currency_reason_type::atm_deposit);
	currency_command_payload decoded_deposit = {};
	assert(currency_command_decode_payload(deposit, &decoded_deposit));
	assert(critical_command_valid(deposit));
	std::vector<std::string> operations = { operation_hex(deposit.operation_id) };
	critical_apply_result applied = critical_command_repository_apply(connection, deposit);
	if (applied.outcome != critical_apply_outcome::applied || applied.error_code != 0)
		fprintf(stderr, "currency deposit failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	currency_command_result result = currency_result_of(applied);
	const currency_vector empty_balances = {};
	const currency_vector deposited_balances = { { 9, 8, 7, 6 } };
	assert(result.wallet.amount == empty_balances.amount);
	assert(result.bank.amount == deposited_balances.amount);
	assert(result.wallet_revision == 1 && result.bank_revision == 1);
	bank_id = static_cast<uint32_t>(scalar("SELECT id FROM account_banks WHERE account_name='" +
					       account + "' AND racewar=1"));
	assert(bank_id != 0);
	critical_apply_result duplicate = critical_command_repository_apply(connection, deposit);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(currency_result_of(duplicate).bank_revision == 1);

	const currency_vector wallet_withdraw = { { 0, 0, 0, 5 } };
	const currency_vector bank_withdraw = { { 0, 0, 0, -5 } };
	critical_command withdrawal = command_for(pid, account.c_str(), wallet_withdraw,
						  bank_withdraw,
						  currency_reason_type::atm_withdraw);
	operations.push_back(operation_hex(withdrawal.operation_id));
	applied = critical_command_repository_apply(connection, withdrawal);
	assert(applied.outcome == critical_apply_outcome::applied);
	result = currency_result_of(applied);
	assert(result.wallet.amount[3] == 5 && result.bank.amount[3] == 1);

	const currency_vector empty = {};
	const currency_vector excessive = { { 0, 0, 0, -100 } };
	critical_command rejected = command_for(pid, account.c_str(), empty, excessive,
						currency_reason_type::atm_withdraw);
	operations.push_back(operation_hex(rejected.operation_id));
	applied = critical_command_repository_apply(connection, rejected);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ENOSPC);
	duplicate = critical_command_repository_apply(connection, rejected);
	assert(duplicate.outcome == critical_apply_outcome::terminal_failure &&
	       duplicate.error_code == ENOSPC);

	const currency_vector reward = { { 3, 0, 0, 0 } };
	critical_command rebased_reward = command_for(pid, account.c_str(), reward, empty,
						      currency_reason_type::wallet_reward, 0, 2);
	operations.push_back(operation_hex(rebased_reward.operation_id));
	applied = critical_command_repository_apply(connection, rebased_reward);
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	result = currency_result_of(applied);
	assert(result.wallet.amount[0] == 3 && result.wallet.amount[3] == 5);
	assert(result.wallet_revision == 3 && result.bank_revision == 3);

	const currency_vector coin_drop_debit = { { -2, 0, 0, 0 } };
	critical_command coin_drop = command_for(pid, account.c_str(), coin_drop_debit, empty,
						 currency_reason_type::wallet_spend);
	operations.push_back(operation_hex(coin_drop.operation_id));
	applied = critical_command_repository_apply(connection, coin_drop);
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	result = currency_result_of(applied);
	assert(result.wallet.amount[0] == 1 && result.wallet.amount[3] == 5);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=UNHEX('" +
		      operation_hex(coin_drop.operation_id) + "') AND reason_type=" +
		      std::to_string(
			      static_cast<unsigned int>(currency_reason_type::wallet_spend))) == 1);

	execute("UPDATE account_banks SET bank_copper=2147483647 WHERE id=" +
		std::to_string(bank_id));
	const currency_vector overflow_delta = { { 1, 0, 0, 0 } };
	critical_command overflow = command_for(pid, account.c_str(), empty, overflow_delta,
						currency_reason_type::bank_reward);
	operations.push_back(operation_hex(overflow.operation_id));
	applied = critical_command_repository_apply(connection, overflow);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ERANGE);
	execute("UPDATE account_banks SET bank_copper=10 WHERE id=" + std::to_string(bank_id));

	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid)) == 1);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(pid)) == 5);
	assert(scalar("SELECT bank_platinum FROM account_banks WHERE id=" +
		      std::to_string(bank_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + std::to_string(pid)) ==
	       4);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox o JOIN currency_ledger l "
		      "ON l.operation_id=o.operation_id WHERE l.pid=" +
		      std::to_string(pid)) == 4);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(rejected.operation_id) + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(overflow.operation_id) + "')") == 0);

	for (const std::string &operation : operations)
	{
		execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
			"ON o.outbox_id=d.outbox_id WHERE o.operation_id=UNHEX('" +
			operation + "')");
		execute("DELETE FROM critical_outbox WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM currency_ledger WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			operation + "')");
	}
	execute("DELETE FROM currency_wallet_baseline WHERE pid=" + std::to_string(pid));
	execute("DELETE FROM currency_bank_baseline WHERE bank_id=" + std::to_string(bank_id));
	execute("DELETE FROM player_data WHERE pid=" + std::to_string(pid));
	execute("DELETE FROM account_banks WHERE id=" + std::to_string(bank_id));
	execute("DELETE FROM accounts WHERE account_name='" + account + "'");
	coin_failure_matrix();
	mysql_close(connection);
	return 0;
}
