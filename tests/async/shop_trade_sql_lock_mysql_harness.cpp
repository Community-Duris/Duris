#include "persistence/economic_sql_shop_trade_transaction.h"

#include <mysql.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

namespace
{
constexpr uint32_t PLAYER = 2147000731U;
constexpr uint32_t SHOP = 3;
constexpr uint32_t KEEPER_ROW = 9;
constexpr uint64_t ITEM = 9900000731ULL;
constexpr uint64_t PRODUCED = 9900000732ULL;
MYSQL *connection = nullptr;

critical_operation_id id(uint8_t first)
{
	critical_operation_id result = {};
	result.bytes[0] = first;
	return result;
}

std::string literal(const critical_operation_id &value)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	for (auto byte : value.bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result + "'";
}

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()))
		std::fprintf(stderr, "shop SQL lock error %u: %s\n%s\n", mysql_errno(connection),
			     mysql_error(connection), sql.c_str());
	assert(mysql_errno(connection) == 0);
}

uint64_t scalar(const std::string &sql)
{
	execute(sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	assert(rows && mysql_num_rows(rows.get()) == 1);
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	assert(row && row[0]);
	return strtoull(row[0], nullptr, 10);
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

shop_trade_payload payload_for(shop_trade_action action)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = PLAYER;
	payload.shop_id = SHOP;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "shop_sql_player", 16);
	payload.price = action == shop_trade_action::discard_invalid ? 0 : 200;
	payload.keeper_vnum = 12345;
	payload.expected_keeper_cash = 500;
	payload.expected_wallet_revision = 4;
	payload.expected_bank_revision = 7;
	payload.expected_shop_revision = 9;
	payload.selected_item_uid = action == shop_trade_action::buy_produced ? PRODUCED : ITEM;
	payload.target_root_item_uid = payload.selected_item_uid;
	payload.item_count = 1;
	payload.items[0] = {
		payload.selected_item_uid,
		payload.selected_item_uid,
		0,
		action == shop_trade_action::buy_produced ? ITEM_TRANSFER_ABSENT_REVISION : 4,
		77,
		action == shop_trade_action::buy_produced ? item_custody_state::absent :
							    item_custody_state::active
	};
	if (action == shop_trade_action::buy_existing ||
	    action == shop_trade_action::discard_invalid ||
	    action == shop_trade_action::buy_produced)
	{
		payload.stock_item_uid = ITEM;
		payload.expected_stock_item_revision = 4;
		payload.stock_vnum = 77;
	}
	payload.item_blob[0] = 1;
	payload.item_blob_size = 1;
	return payload;
}

critical_command command_for(shop_trade_payload payload, uint8_t operation,
			     const critical_operation_id &epoch, const economic_account_key &wallet,
			     const economic_account_key &bank, const economic_account_key &keeper)
{
	critical_command command = {};
	assert(shop_trade_command_build(&command, id(operation), payload,
					critical_source_site::command,
					critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(shop_trade_accounting_intent(command, epoch, wallet, bank, keeper,
					    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(command));
	return command;
}

shop_trade_result result_for(const shop_trade_payload &payload,
			     const shop_trade_accounting_authority &before)
{
	shop_trade_result result = {};
	result.action = payload.action;
	result.wallet.amount = payload.action == shop_trade_action::discard_invalid ?
				       economic_coin_vector{ 0, 0, 0, 1 } :
			       payload.action == shop_trade_action::buy_existing ||
					       payload.action == shop_trade_action::buy_produced ?
				       economic_coin_vector{ 0, 0, 8, 0 } :
				       economic_coin_vector{ 0, 0, 2, 1 };
	result.bank.amount = { 2, 0, 0, 0 };
	result.wallet_revision = 4 + (payload.action != shop_trade_action::discard_invalid);
	result.bank_revision = 7 + (payload.action != shop_trade_action::discard_invalid);
	result.shop_revision = 10;
	result.keeper_cash = payload.action == shop_trade_action::discard_invalid ? 500 :
			     payload.action == shop_trade_action::buy_existing ||
					     payload.action == shop_trade_action::buy_produced ?
										    700 :
										    300;
	result.keeper_cash_recorded = true;
	result.player_owner_revision = before.player_owner_revision_before + 1;
	result.counterparty_owner_revision = before.counterparty_owner_revision_before + 1;
	result.item_count = 1;
	result.item_uids[0] = payload.selected_item_uid;
	result.item_revisions[0] = payload.action == shop_trade_action::buy_produced ? 1 : 5;
	return result;
}

void plan_case(shop_trade_action action, uint8_t operation, const critical_operation_id &epoch,
	       const economic_account_key &wallet, const economic_account_key &bank,
	       const economic_account_key &keeper)
{
	const auto payload = payload_for(action);
	const auto command = command_for(payload, operation, epoch, wallet, bank, keeper);
	execute("START TRANSACTION");
	economic_sql_shop_trade_context context;
	const auto status = economic_sql_shop_trade_lock(connection, command, &context);
	if (status)
		std::fprintf(stderr, "shop SQL lock action %u rejected: %u\n",
			     static_cast<unsigned>(action), status);
	assert(status == 0);
	assert(context.bank_id && context.keeper_id == KEEPER_ROW &&
	       context.before.shop_id == SHOP && context.before.keeper_cash_before == 500 &&
	       context.before.shop_revision_before == 9 &&
	       context.before.balances_before.wallet.amount ==
		       (economic_coin_vector{ 0, 0, 0, 1 }));
	economic_frozen_intent intent;
	assert(economic_intent_decode(command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	economic_accounting_plan plan;
	const auto result = result_for(payload, context.before);
	assert(shop_trade_accounting_plan(command, intent, context.before, result, &plan) ==
	       economic_accounting_error::ok);
	assert(plan.item_events.size() == 1 &&
	       plan.item_events[0].uid == payload.selected_item_uid &&
	       plan.items_before.size() == (action == shop_trade_action::buy_produced ? 2U : 1U));
	execute("ROLLBACK");
}
} // namespace

int main()
{
	assert(getenv("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") &&
	       !std::strcmp(getenv("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(getenv("DB_HOST") && !std::strcmp(getenv("DB_HOST"), "127.0.0.1") &&
	       getenv("DB_NAME") &&
	       std::string(getenv("DB_NAME")).starts_with("economic_schema_test_") &&
	       getenv("DB_USER") && getenv("DB_PASSWD") && getenv("DB_PORT") &&
	       !getenv("DB_SOCKET"));
	connection = mysql_init(nullptr);
	assert(connection);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	assert(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect));
	assert(mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
				  getenv("DB_PASSWD"), getenv("DB_NAME"),
				  static_cast<unsigned>(strtoul(getenv("DB_PORT"), nullptr, 10)),
				  nullptr, 0));
	const auto bootstrap = id(71), lineage = id(72), epoch = id(73);
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		literal(bootstrap) + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'')");
	execute("INSERT INTO accounts(account_name,password) VALUES('shop_sql_player','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,"
		"platinum,wallet_revision) VALUES(" +
		std::to_string(PLAYER) + ",'ShopPlayer','shop_sql_player',1,0,0,0,1,4)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		"bank_platinum,bank_revision) VALUES('shop_sql_player',1,2,0,0,0,7)");
	const auto native_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='shop_sql_player'");
	execute("INSERT INTO shopkeepers(id,shop_id,mob_vnum,room_vnum,cash,shop_revision) "
		"VALUES(9,3,12345,100,500,9)");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ITEM) + "," + std::to_string(ITEM) + ",NULL,9,4,0,4,77,1)");
	execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1," +
		std::to_string(PLAYER) + ",0,11),(9,4,0,12)");
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ",1,1,REPEAT(CHAR(1),32)," +
		literal(bootstrap) + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" +
		literal(lineage) + "," + literal(epoch) + ")");
	const auto wallet =
		mapping(lineage, economic_account_kind::wallet, 0, 1, PLAYER, bootstrap);
	const auto bank =
		mapping(lineage, economic_account_kind::bank, 1, 2, native_bank, bootstrap);
	const auto keeper =
		mapping(lineage, economic_account_kind::treasury, 0, 6, KEEPER_ROW, bootstrap);
	plan_case(shop_trade_action::buy_existing, 74, epoch, wallet, bank, keeper);
	plan_case(shop_trade_action::buy_produced, 75, epoch, wallet, bank, keeper);
	const auto buy = command_for(payload_for(shop_trade_action::buy_existing), 76, epoch,
				     wallet, bank, keeper);
	execute("UPDATE shopkeepers SET cash=NULL WHERE id=9");
	execute("START TRANSACTION");
	economic_sql_shop_trade_context unchanged;
	unchanged.keeper_id = 123;
	assert(economic_sql_shop_trade_lock(connection, buy, &unchanged) == ENODATA &&
	       unchanged.keeper_id == 123);
	execute("ROLLBACK");
	execute("UPDATE shopkeepers SET cash=501 WHERE id=9");
	execute("START TRANSACTION");
	assert(economic_sql_shop_trade_lock(connection, buy, &unchanged) == ESTALE &&
	       unchanged.keeper_id == 123);
	execute("ROLLBACK");
	execute("UPDATE shopkeepers SET cash=500 WHERE id=9");
	execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
		"owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
		std::to_string(ITEM + 10) + "," + std::to_string(ITEM) + "," +
		std::to_string(ITEM) + ",9,4,0,1,78,1)");
	execute("START TRANSACTION");
	assert(economic_sql_shop_trade_lock(connection, buy, &unchanged) == EMSGSIZE &&
	       unchanged.keeper_id == 123);
	execute("ROLLBACK");
	execute("DELETE FROM item_current_owner WHERE item_uid=" + std::to_string(ITEM + 10));
	execute("UPDATE item_current_owner SET owner_type=1,owner_id=" + std::to_string(PLAYER) +
		" WHERE item_uid=" + std::to_string(ITEM));
	const auto sell = command_for(payload_for(shop_trade_action::sell_store), 82, epoch, wallet,
				      bank, keeper);
	execute("UPDATE item_current_owner SET equipment_slot=1 WHERE item_uid=" +
		std::to_string(ITEM));
	execute("START TRANSACTION");
	assert(economic_sql_shop_trade_lock(connection, sell, &unchanged) == ESTALE &&
	       unchanged.keeper_id == 123);
	execute("ROLLBACK");
	execute("UPDATE item_current_owner SET equipment_slot=0 WHERE item_uid=" +
		std::to_string(ITEM));
	plan_case(shop_trade_action::sell_store, 77, epoch, wallet, bank, keeper);
	plan_case(shop_trade_action::sell_destroy, 78, epoch, wallet, bank, keeper);
	execute("UPDATE shopkeepers SET cash=50,mob_vnum=11005 WHERE id=9");
	auto exception = payload_for(shop_trade_action::sell_store);
	exception.expected_keeper_cash = 50;
	exception.keeper_vnum = 11005;
	exception.keeper_roaming = 1;
	const auto exceptional_command = command_for(exception, 80, epoch, wallet, bank, keeper);
	execute("START TRANSACTION");
	economic_sql_shop_trade_context exceptional_context;
	assert(economic_sql_shop_trade_lock(connection, exceptional_command,
					    &exceptional_context) == 0);
	economic_frozen_intent exceptional_intent;
	assert(economic_intent_decode(exceptional_command.accounting_intent, &exceptional_intent) ==
	       economic_accounting_error::ok);
	auto exceptional_result = result_for(exception, exceptional_context.before);
	exceptional_result.keeper_cash = 50;
	economic_accounting_plan exceptional_plan;
	assert(shop_trade_accounting_plan(exceptional_command, exceptional_intent,
					  exceptional_context.before, exceptional_result,
					  &exceptional_plan) == economic_accounting_error::ok);
	bool issued = false;
	for (const auto &account : exceptional_plan.accounts)
		issued |= account.key.kind == economic_account_kind::issuance;
	assert(issued);
	execute("ROLLBACK");
	execute("UPDATE shopkeepers SET mob_vnum=12345 WHERE id=9");
	exception.keeper_vnum = 12345;
	const auto roaming_command = command_for(exception, 81, epoch, wallet, bank, keeper);
	execute("START TRANSACTION");
	economic_sql_shop_trade_context roaming_context;
	assert(economic_sql_shop_trade_lock(connection, roaming_command, &roaming_context) == 0);
	economic_frozen_intent roaming_intent;
	assert(economic_intent_decode(roaming_command.accounting_intent, &roaming_intent) ==
	       economic_accounting_error::ok);
	auto roaming_result = result_for(exception, roaming_context.before);
	roaming_result.keeper_cash = 50;
	economic_accounting_plan roaming_plan;
	assert(shop_trade_accounting_plan(roaming_command, roaming_intent, roaming_context.before,
					  roaming_result, &roaming_plan) ==
	       economic_accounting_error::negative_holding);
	execute("ROLLBACK");
	execute("UPDATE shopkeepers SET cash=500 WHERE id=9");
	execute("UPDATE item_current_owner SET owner_type=9,owner_id=4 WHERE item_uid=" +
		std::to_string(ITEM));
	plan_case(shop_trade_action::discard_invalid, 79, epoch, wallet, bank, keeper);
	execute("UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_shop_trade_lock(connection, buy, &unchanged) == ENODATA &&
	       unchanged.keeper_id == 123);
	execute("ROLLBACK");
	mysql_close(connection);
	std::puts("SQL shop lock: five actions, cash exceptions, stale/unknown cash, hidden "
		  "child, equipped item, inactive epoch PASS");
}
