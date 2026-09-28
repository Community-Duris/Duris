#include "economy/auction_money_claim_accounting.h"
#include "persistence/economic_sql_auction_money_claim_transaction.h"
#include "persistence/economic_sql_pending_claim_source.h"

#include <mysql.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
MYSQL *connection = nullptr;
constexpr uint32_t PLAYER = 2147000711U;

critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

std::string hex(const uint8_t *bytes, size_t size)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		result[index * 2] = digits[bytes[index] >> 4];
		result[index * 2 + 1] = digits[bytes[index] & 15];
	}
	return result;
}

std::string literal(const critical_operation_id &value)
{
	return "X'" + hex(value.bytes.data(), value.bytes.size()) + "'";
}

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()))
		std::fprintf(stderr, "auction claim SQL error %u: %s\n%s\n",
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

void inbox(const critical_operation_id &operation, uint16_t schema, uint8_t status)
{
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		literal(operation) + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32)," +
		std::to_string(static_cast<uint16_t>(critical_command_type::auction)) + "," +
		std::to_string(schema) + ",1," + std::to_string(status) + ",X'')");
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

void source_root(const critical_operation_id &operation, const critical_operation_id &lineage,
		 const critical_operation_id &epoch, const economic_account_key &claim,
		 uint64_t amount)
{
	inbox(operation, 2, 1);
	execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,"
		"original_operation_id,accounting_version,writer_id,policy_version,compiler_version,"
		"actor_kind,actor_id,reason,intent_digest,domain_digest,plan_digest,"
		"canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,"
		"child_count,item_event_count,before_witness_count,after_witness_count) VALUES(" +
		literal(operation) + "," + literal(lineage) + "," + literal(epoch) +
		",NULL,1,99,1,1,2," + std::to_string(PLAYER) +
		",1,REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),REPEAT(CHAR(3),32),"
		"REPEAT(CHAR(4),256),REPEAT(CHAR(5),256),1,0,1,0,0,0,0,0)");
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	assert(economic_account_key_encode(claim, &key) == economic_accounting_error::ok);
	execute("INSERT INTO economic_accounting_account_effect(operation_id,account_index,"
		"account_key,before_copper,before_silver,before_gold,before_platinum,"
		"after_copper,after_silver,after_gold,after_platinum,before_revision,"
		"after_revision) VALUES(" +
		literal(operation) + ",0,X'" + hex(key.data(), key.size()) + "',0,0,0,0," +
		std::to_string(amount) + ",0,0,0,0,1)");
}

critical_command claim_command(const economic_account_key &wallet, const economic_account_key &bank,
			       const economic_account_key &pending,
			       const auction_money_claim_state &state,
			       const critical_operation_id &epoch)
{
	auction_command_payload payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = PLAYER;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "auction_claim", 13);
	std::memcpy(payload.actor_name.data(), "Claimant", 8);
	critical_command command = {};
	assert(auction_command_build(&command, id(20), payload, critical_source_site::command,
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
	const auto bootstrap = id(1), lineage = id(2), epoch = id(3);
	inbox(bootstrap, 1, 1);
	execute("INSERT INTO accounts(account_name,password) VALUES('auction_claim','')");
	execute("INSERT INTO player_data(pid,name,account_name,racewar,copper,silver,gold,"
		"platinum,wallet_revision) VALUES(" +
		std::to_string(PLAYER) + ",'Claimant','auction_claim',1,0,0,0,10,0)");
	execute("INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,"
		"bank_gold,bank_platinum,bank_revision) VALUES('auction_claim',1,0,0,0,0,0)");
	const auto native_bank =
		scalar("SELECT id FROM account_banks WHERE account_name='auction_claim'");
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
	const auto pending =
		mapping(lineage, economic_account_kind::pending_claim, 0, 5, PLAYER, bootstrap);
	source_root(id(10), lineage, epoch, pending, 300);
	source_root(id(11), lineage, epoch, pending, 200);
	execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(" +
		std::to_string(PLAYER) + ",500,2)");
	execute("START TRANSACTION");
	assert(economic_sql_pending_claim_source_stage(connection, id(10), 1, pending, PLAYER,
						       300) == 0);
	assert(economic_sql_pending_claim_source_stage(connection, id(11), 2, pending, PLAYER,
						       200) == 0);
	execute("COMMIT");
	auction_money_claim_state state;
	state.beneficiary_pid = PLAYER;
	state.money = 500;
	state.revision = 2;
	state.sources = { { id(10), 1, PLAYER, pending.authority_id, 300 },
			  { id(11), 2, PLAYER, pending.authority_id, 200 } };
	const auto command = claim_command(wallet, bank, pending, state, epoch);
	// A direct legacy credit in the shared aggregate must not be attributed to
	// these two source rows or picked up by the typed claim.
	execute("UPDATE auction_money_pickups SET money=600 WHERE pid=" + std::to_string(PLAYER));
	execute("START TRANSACTION");
	inbox(command.operation_id, 2, 0);
	economic_sql_auction_money_claim_context context;
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	auction_command_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &result_code,
								   &mutation_applied) != 0);
	execute("ROLLBACK");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(PLAYER)) == 600);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(PLAYER)) ==
	       10);
	execute("UPDATE auction_money_pickups SET money=500 WHERE pid=" + std::to_string(PLAYER));
	// Changing one source after admission also blocks the claim before pickup.
	execute("UPDATE economic_pending_claim_source SET amount=201 WHERE "
		"source_operation_id=" +
		literal(id(11)));
	execute("START TRANSACTION");
	inbox(command.operation_id, 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &result_code,
								   &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("UPDATE economic_pending_claim_source SET amount=200 WHERE "
		"source_operation_id=" +
		literal(id(11)));
	// Force failure after native wallet credit and accounting postings.
	execute("CREATE TRIGGER fail_auction_money_claim_source BEFORE UPDATE ON "
		"economic_pending_claim_source FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='forced claim source failure'");
	execute("START TRANSACTION");
	inbox(command.operation_id, 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &result_code,
								   &mutation_applied) != 0);
	execute("ROLLBACK");
	execute("DROP TRIGGER fail_auction_money_claim_source");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(PLAYER)) == 500);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(PLAYER)) ==
	       10);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id IS NOT NULL") == 0);
	execute("START TRANSACTION");
	inbox(command.operation_id, 2, 0);
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) == 0);
	assert(economic_sql_auction_money_claim_execute_and_record(connection, command, context,
								   &result, &result_code,
								   &mutation_applied) == 0);
	assert(result_code == 0 && mutation_applied &&
	       result.event_type == auction_event_type::money_claimed);
	receipt(command, result);
	execute("COMMIT");
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=" +
		      std::to_string(PLAYER)) == 0);
	assert(scalar("SELECT claim_revision FROM auction_money_pickups WHERE pid=" +
		      std::to_string(PLAYER)) == 3);
	assert(scalar("SELECT gold FROM player_data WHERE pid=" + std::to_string(PLAYER)) == 5);
	assert(scalar("SELECT platinum FROM player_data WHERE pid=" + std::to_string(PLAYER)) ==
	       10);
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id=" +
		      literal(command.operation_id)) == 2);
	assert(value("SELECT GROUP_CONCAT(amount ORDER BY source_operation_id,source_slot) "
		     "FROM economic_pending_claim_source WHERE claim_operation_id=" +
		     literal(command.operation_id)) == "300,200");
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting "
		      "WHERE operation_id=" +
		      literal(command.operation_id)) == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=" +
		      literal(command.operation_id)) == 2);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id=" +
		      literal(command.operation_id)) == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=" +
		      literal(command.operation_id)) == 1);
	assert(scalar("SELECT status FROM critical_operation_inbox WHERE operation_id=" +
		      literal(command.operation_id)) == 1);
	mysql_close(connection);
	connection = mysql_init(nullptr);
	assert(connection && mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
						getenv("DB_PASSWD"), getenv("DB_NAME"),
						static_cast<unsigned int>(
							strtoul(getenv("DB_PORT"), nullptr, 10)),
						nullptr, 0));
	assert(scalar("SELECT COUNT(*) FROM economic_pending_claim_source WHERE "
		      "claim_operation_id=" +
		      literal(command.operation_id)) == 2);
	assert(scalar("SELECT OCTET_LENGTH(result_payload) FROM critical_operation_inbox "
		      "WHERE operation_id=" +
		      literal(command.operation_id)) == AUCTION_RESULT_PAYLOAD_BYTES);
	execute("UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=" +
		literal(lineage));
	execute("START TRANSACTION");
	assert(economic_sql_auction_money_claim_lock(connection, command, &context) != 0);
	execute("ROLLBACK");
	mysql_close(connection);
	return 0;
}
