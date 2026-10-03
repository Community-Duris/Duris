#include "persistence/critical_command_repository.h"
#include "economic_sql_coordinator_fixture.h"
#include "economic_sql_commit_reply_loss_fixture.h"
#include "economy/currency_command.h"
#include "economy/coin_transfer_command.h"
#include "economy/coin_transfer_accounting.h"
#include "player/player_snapshot_codec.h"
#include "player/player_load_repository.h"
#include "persistence/persistence_observability.h"
#include "core/structs.h"
#include "world/vnum.obj.h"

#include <mysql.h>

#include <cassert>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long length)
{
	return economic_sql_commit_reply_loss_fixture::query_result(
		connection, query, length, __real_mysql_real_query(connection, query, length));
}
extern "C" unsigned int __wrap_mysql_errno(MYSQL *connection)
{
	return economic_sql_commit_reply_loss_fixture::lost_reply(connection) ?
		       2013 :
		       __real_mysql_errno(connection);
}

namespace
{
MYSQL *open_pool_test_connection()
{
	const char *host = std::getenv("DB_HOST");
	const char *user = std::getenv("DB_USER");
	const char *password = std::getenv("DB_PASSWD");
	const char *database = std::getenv("CURRENCY_TEST_DB_NAME");
	const char *port_value = std::getenv("DB_PORT");
	if (!host || !user || !password || !database)
		return nullptr;
	MYSQL *pooled = mysql_init(nullptr);
	if (!pooled)
		return nullptr;
	const unsigned int port = port_value ? static_cast<unsigned int>(atoi(port_value)) : 3306;
	if (!mysql_real_connect(pooled, host, user, password, database, port, nullptr, 0))
	{
		mysql_close(pooled);
		return nullptr;
	}
	return pooled;
}
} // namespace

extern "C" MYSQL *sql_pool_acquire(void)
{
	auto *pooled = open_pool_test_connection();
	economic_sql_commit_reply_loss_fixture::acquired(pooled);
	return pooled;
}
extern "C" void sql_pool_release(MYSQL *pooled)
{
	economic_sql_commit_reply_loss_fixture::closing(pooled, false);
	if (pooled)
		mysql_close(pooled);
}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *pooled)
{
	economic_sql_commit_reply_loss_fixture::closing(pooled, true);
	if (pooled)
		mysql_close(pooled);
	return open_pool_test_connection();
}
extern "C" void sql_pool_discard_connection(MYSQL *pooled)
{
	economic_sql_commit_reply_loss_fixture::closing(pooled, false);
	if (pooled)
		mysql_close(pooled);
}

namespace
{
MYSQL *connection = nullptr;

critical_apply_result pooled_apply(const critical_command &command)
{
	critical_apply_result applied = {};
	std::thread worker(
		[&] { applied = critical_command_repository_apply_from_pool(command, nullptr); });
	worker.join();
	return applied;
}

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()) != 0)
	{
		fprintf(stderr, "fixture SQL failed: %s\n%s\n", sql.c_str(),
			mysql_error(connection));
		abort();
	}
}

void kill_if_present(unsigned long thread_id)
{
	const std::string sql = "KILL CONNECTION " + std::to_string(thread_id);
	if (mysql_query(connection, sql.c_str()) == 0)
		return;
	// The sleep can finish between the processlist observation and KILL. Treat
	// that race as a test assertion below, not as an unrelated fixture abort.
	if (mysql_errno(connection) != 1094)
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

long long failure_stage_of(const critical_command &command)
{
	return scalar(
		"SELECT failure_stage FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		operation_hex(command.operation_id) + "')");
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
		402013,
		created ? item_custody_state::absent : item_custody_state::active
	};
	player_item_snapshot snapshot = {};
	snapshot.object_uid = uid;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.vnum = 402013;
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

coins pile_amount(uint64_t uid);
void check_active_pile_split_merge(uint64_t pile_uid, item_owner_identity room,
				   const critical_operation_id &lineage,
				   const critical_operation_id &epoch);

void check_active_coin_item_accounting(uint32_t pid, const char *account,
				       const critical_operation_id &lineage,
				       const critical_operation_id &epoch)
{
	constexpr uint64_t pile_uid = 900001000;
	constexpr uint64_t room_id = 987654321;
	const item_owner_identity room = { item_owner_type::room, room_id, 0 };
	const auto source = coin_wallet(pid, account, { 1, 0, 0, 5 }, { 0, 0, 0, 5 });
	const auto destination = coin_pile(room, pile_uid, 0, {}, { 1, 0, 0, 0 });
	const coin_transfer_payload payload = { source, destination };
	critical_operation_id operation = {};
	assert(critical_operation_id_generate(&operation));
	critical_command command = {};
	assert(coin_transfer_command_build(&command, operation, payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	const economic_account_key wallet_account = { lineage, economic_account_kind::wallet, pid,
						      0 };
	const economic_account_key pile_account = { lineage, economic_account_kind::pile, pile_uid,
						    0 };
	std::vector<uint8_t> intent;
	assert(coin_transfer_accounting_intent(command, epoch, wallet_account, pile_account,
					       &intent) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.accepted_at_usec = 1;
	command.publication_required = true;
	assert(critical_command_envelope_valid(command));
	economic_sql_commit_reply_loss_fixture::arm();
	const critical_apply_result applied = exercise_sql_coordinator(
		command, "coin-native-commit-reply", false,
		critical_apply_outcome::already_applied);
	economic_sql_commit_reply_loss_fixture::verify();
	if (applied.outcome != critical_apply_outcome::already_applied || applied.error_code)
		fprintf(stderr, "pooled typed coin transfer failed outcome=%u error=%u\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code);
	assert(applied.outcome == critical_apply_outcome::already_applied && !applied.error_code);
	const auto replayed = critical_command_repository_apply(connection, command);
	assert(replayed.outcome == critical_apply_outcome::already_applied &&
	       replayed.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  replayed.result_payload.begin()));
	auto changed = command;
	++changed.accepted_at_usec;
	const auto conflict = pooled_apply(changed);
	assert(conflict.outcome == critical_apply_outcome::terminal_failure &&
	       conflict.error_code == EEXIST);
	const auto reconciled = critical_command_repository_reconcile(connection, command);
	assert(reconciled.outcome == critical_apply_outcome::already_applied &&
	       reconciled.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  reconciled.result_payload.begin()));

	critical_operation_id wallet_child = {}, pile_child = {};
	assert(critical_operation_id_derive(operation, COIN_TRANSFER_OPERATION_DOMAIN, 0,
					    &wallet_child));
	assert(critical_operation_id_derive(operation, COIN_TRANSFER_OPERATION_DOMAIN, 1,
					    &pile_child));
	const std::string root_id = operation_hex(operation);
	const std::string wallet_id = operation_hex(wallet_child);
	const std::string pile_id = operation_hex(pile_child);
	const std::string uid = std::to_string(pile_uid);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid) +
		      " AND copper=0 AND silver=0 AND gold=0 AND platinum=5 AND "
		      "wallet_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert((pile_amount(pile_uid) == coins{ 1, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" + uid +
		      " AND item_revision=1 AND state=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      root_id + "') AND status=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=UNHEX('" +
		      root_id +
		      "') AND outcome=1 AND account_count=2 AND posting_count=2 AND "
		      "child_count=2 AND item_event_count=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE operation_id=UNHEX('" +
		      root_id + "')") == 2);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE operation_id=UNHEX('" +
		      root_id + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=0 AND "
		      "before_copper=1 AND after_copper=0 AND before_platinum=5 AND "
		      "after_platinum=5 AND before_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision) +
		      " AND after_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=1 AND "
		      "before_copper=0 AND after_copper=1 AND before_revision=0 AND "
		      "after_revision=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=0 AND "
		      "delta_copper=-1 AND copper_value=-1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=1 AND "
		      "delta_copper=1 AND copper_value=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_child WHERE operation_id=UNHEX('" +
		      root_id +
		      "') AND receipt_operation_id=child_operation_id AND "
		      "child_operation_id IN (UNHEX('" +
		      wallet_id + "'),UNHEX('" + pile_id + "'))") == 2);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		      "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND "
		      "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND "
		      "l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		      root_id + "') AND r.item_uid=" + uid +
		      " AND r.before_revision=0 AND "
		      "r.after_revision=1 AND r.legacy_operation_id=UNHEX('" +
		      pile_id + "')") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id IN "
		      "(UNHEX('" +
		      root_id + "'),UNHEX('" + wallet_id + "'),UNHEX('" + pile_id + "'))") == 3);

	const auto pickup_source = coin_pile(room, pile_uid, 0, { 1, 0, 0, 0 }, {});
	const auto pickup_destination = coin_wallet(pid, account, { 0, 0, 0, 5 }, { 1, 0, 0, 5 });
	auto pickup = coin_command(pickup_source, pickup_destination);
	std::vector<uint8_t> pickup_intent;
	assert(coin_transfer_accounting_intent(pickup, epoch, pile_account, wallet_account,
					       &pickup_intent) == economic_accounting_error::ok);
	pickup.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	pickup.accounting_intent = std::move(pickup_intent);
	pickup.publication_required = true;
	assert(critical_command_envelope_valid(pickup));
	const std::string pickup_root_id = operation_hex(pickup.operation_id);
	execute("CREATE TRIGGER accounted_coin_endpoint_failure BEFORE UPDATE ON "
		"item_current_owner FOR EACH ROW SIGNAL SQLSTATE '45000' "
		"SET MESSAGE_TEXT='injected accounted coin failure'");
	const auto failed_pickup = pooled_apply(pickup);
	assert(failed_pickup.outcome == critical_apply_outcome::retryable_failure &&
	       failed_pickup.error_code == 1644);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid) +
		      " AND copper=0 AND wallet_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(pile_uid) + " AND item_revision=1 AND state=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      pickup_root_id + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id + "')") == 0);
	execute("DROP TRIGGER accounted_coin_endpoint_failure");
	const auto picked_up = pooled_apply(pickup);
	if (picked_up.outcome != critical_apply_outcome::applied || picked_up.error_code)
		fprintf(stderr, "typed coin pickup failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(picked_up.outcome), picked_up.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(picked_up.outcome == critical_apply_outcome::applied && !picked_up.error_code);
	const auto pickup_replay = pooled_apply(pickup);
	assert(pickup_replay.outcome == critical_apply_outcome::already_applied &&
	       pickup_replay.result_size == picked_up.result_size &&
	       std::equal(picked_up.result_payload.begin(),
			  picked_up.result_payload.begin() + picked_up.result_size,
			  pickup_replay.result_payload.begin()));
	const auto pickup_reconciled = critical_command_repository_reconcile(connection, pickup);
	assert(pickup_reconciled.outcome == critical_apply_outcome::already_applied &&
	       pickup_reconciled.result_size == picked_up.result_size);
	critical_operation_id pickup_pile_child = {}, pickup_wallet_child = {};
	assert(critical_operation_id_derive(pickup.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 0,
					    &pickup_pile_child));
	assert(critical_operation_id_derive(pickup.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 1,
					    &pickup_wallet_child));
	const std::string pickup_pile_id = operation_hex(pickup_pile_child);
	const std::string pickup_wallet_id = operation_hex(pickup_wallet_child);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid) +
		      " AND copper=1 AND platinum=5 AND wallet_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 2)) == 1);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" + uid +
		      " AND item_revision=2 AND state=2 AND coin_payload IS NULL") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id +
		      "') AND outcome=1 AND account_count=2 AND posting_count=2 AND "
		      "child_count=2 AND item_event_count=1") == 1);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id +
		      "') AND account_index=1 AND before_copper=1 AND after_copper=0 "
		      "AND before_revision=1 AND after_revision=2") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id +
		      "') AND account_index=0 AND before_copper=0 AND after_copper=1 "
		      "AND before_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1) +
		      " AND after_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 2)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      pickup_root_id +
		      "') AND ((account_index=1 AND delta_copper=-1 AND copper_value=-1) "
		      "OR (account_index=0 AND delta_copper=1 AND copper_value=1))") == 2);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference r JOIN "
		      "item_ownership_ledger l ON l.operation_id=r.legacy_operation_id "
		      "AND l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid "
		      "AND l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
		      pickup_root_id + "') AND r.item_uid=" + uid +
		      " AND r.before_revision=1 AND r.after_revision=2 "
		      "AND r.legacy_operation_id=UNHEX('" +
		      pickup_pile_id + "')") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      pickup_root_id + "') AND status=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id IN "
		      "(UNHEX('" +
		      pickup_root_id + "'),UNHEX('" + pickup_pile_id + "'),UNHEX('" +
		      pickup_wallet_id + "'))") == 3);

	// This fixture runs inside a disposable SQL schema. Remove only its rows and
	// restore the synthetic player's pre-test balances before legacy coin cases.
	const std::string operation_ids = "UNHEX('" + root_id + "'),UNHEX('" + wallet_id +
					  "'),UNHEX('" + pile_id + "'),UNHEX('" + pickup_root_id +
					  "'),UNHEX('" + pickup_pile_id + "'),UNHEX('" +
					  pickup_wallet_id + "')";
	execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
		"ON o.outbox_id=d.outbox_id WHERE o.operation_id IN (" +
		operation_ids + ")");
	execute("DELETE FROM critical_outbox WHERE operation_id IN (" + operation_ids + ")");
	execute("DELETE FROM economic_accounting_item_reference WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM economic_accounting_child WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM economic_accounting_account_effect WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM economic_accounting_source_claim WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM economic_accounting_operation WHERE operation_id IN "
		"(UNHEX('" +
		root_id + "'),UNHEX('" + pickup_root_id + "'))");
	execute("DELETE FROM currency_ledger WHERE operation_id IN "
		"(UNHEX('" +
		wallet_id + "'),UNHEX('" + pickup_wallet_id + "'))");
	execute("DELETE FROM item_ownership_ledger WHERE operation_id IN "
		"(UNHEX('" +
		pile_id + "'),UNHEX('" + pickup_pile_id + "'))");
	execute("DELETE FROM saved_items WHERE obj_uid=" + uid);
	execute("DELETE FROM item_current_owner WHERE item_uid=" + uid);
	execute("DELETE FROM item_owner_revision WHERE owner_type=" +
		std::to_string(static_cast<unsigned int>(item_owner_type::room)) +
		" AND owner_id=" + std::to_string(room_id) + " AND owner_context_id=0");
	execute("DELETE FROM critical_operation_inbox WHERE operation_id IN (" + operation_ids +
		")");
	execute("UPDATE player_data SET copper=1,wallet_revision=" +
		std::to_string(source.change.expected_revisions[0].revision) +
		" WHERE pid=" + std::to_string(pid));
	execute("UPDATE account_banks SET bank_revision=" +
		std::to_string(source.change.expected_revisions[1].revision) + " WHERE id=" +
		std::to_string(scalar("SELECT id FROM account_banks WHERE account_name='" +
				      std::string(account) + "' AND racewar=1")));
	puts("PASS: activated SQL wallet-to-pile drop and full pickup commit balanced postings, custody references, and retained replay");
}

void check_active_coin_change_accounting(uint32_t pid, const char *account,
					 const critical_operation_id &lineage,
					 const critical_operation_id &epoch)
{
	constexpr uint64_t pile_uid = 900001001;
	constexpr uint64_t room_id = 987654322;
	const item_owner_identity room = { item_owner_type::room, room_id, 0 };
	const auto source = coin_wallet(pid, account, { 1, 0, 0, 5 }, { 1, 9, 9, 4 });
	const auto destination = coin_pile(room, pile_uid, 0, {}, { 10, 0, 0, 0 });
	auto command = coin_command(source, destination);
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, pid, 0 };
	const economic_account_key pile = { lineage, economic_account_kind::pile, pile_uid, 0 };
	std::vector<uint8_t> intent;
	assert(coin_transfer_accounting_intent(command, epoch, wallet, pile, &intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.publication_required = true;
	assert(critical_command_envelope_valid(command));
	// A typed root must retain the same bounded recovery receipt as schema v1,
	// with a no-posting refusal and no committed child or outbox side effects.
	auto stale_command = coin_command(source, destination);
	assert(coin_transfer_accounting_intent(stale_command, epoch, wallet, pile,
					       &stale_command.accounting_intent) ==
	       economic_accounting_error::ok);
	stale_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	stale_command.publication_required = true;
	execute("UPDATE player_data SET copper=2,wallet_revision=wallet_revision+1 WHERE pid=" +
		std::to_string(pid));
	const auto refused = pooled_apply(stale_command);
	coin_transfer_payload stale_payload;
	assert(coin_transfer_command_decode_payload(stale_command, &stale_payload));
	coin_transfer_stale_result repair;
	assert(refused.outcome == critical_apply_outcome::terminal_failure &&
	       refused.error_code == ESTALE &&
	       refused.failure_stage == critical_failure_stage::coin_source_wallet_revision &&
	       coin_transfer_command_decode_stale_result(stale_payload, refused.failure_stage,
							 refused.result_payload.data(),
							 refused.result_size, &repair));
	assert(repair.current.wallet.amount[0] == 2 && repair.wallet_stale && !repair.bank_stale);
	for (const auto &replay :
	     { pooled_apply(stale_command),
	       critical_command_repository_reconcile(connection, stale_command) })
		assert(replay.outcome == refused.outcome && replay.error_code == ESTALE &&
		       replay.failure_stage == refused.failure_stage &&
		       replay.result_size == refused.result_size &&
		       replay.result_payload == refused.result_payload);
	assert(coin_transfer_accounting_verify_retained(
		       connection, stale_command, ESTALE, refused.result_payload.data(),
		       refused.result_size,
		       critical_failure_stage::coin_source_bank_revision) == EILSEQ);
	auto malformed = refused.result_payload;
	malformed[0] = 0;
	assert(coin_transfer_accounting_verify_retained(connection, stale_command, ESTALE,
							malformed.data(), refused.result_size,
							refused.failure_stage) == EILSEQ);
	const std::string refused_id = "UNHEX('" + operation_hex(stale_command.operation_id) + "')";
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id=" +
		      refused_id + " AND result_code=116 AND posting_count=0 AND child_count=0") ==
	       1);
	for (const char *table :
	     { "economic_accounting_coin_posting", "economic_accounting_account_effect",
	       "economic_accounting_child", "economic_accounting_source_claim", "critical_outbox" })
		assert(scalar("SELECT COUNT(*) FROM " + std::string(table) +
			      " WHERE operation_id=" + refused_id) == 0);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(pile_uid)) == 0);
	execute("DELETE FROM economic_accounting_operation WHERE operation_id=" + refused_id);
	execute("DELETE FROM critical_operation_inbox WHERE operation_id=" + refused_id);
	execute("UPDATE player_data SET copper=1,wallet_revision=" +
		std::to_string(source.change.expected_revisions[0].revision) +
		" WHERE pid=" + std::to_string(pid));
	const auto applied = pooled_apply(command);
	if (applied.outcome != critical_apply_outcome::applied || applied.error_code)
		fprintf(stderr, "typed coin change failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied && !applied.error_code);
	const auto replayed = pooled_apply(command);
	assert(replayed.outcome == critical_apply_outcome::already_applied &&
	       replayed.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  replayed.result_payload.begin()));
	const auto reconciled = critical_command_repository_reconcile(connection, command);
	assert(reconciled.outcome == critical_apply_outcome::already_applied &&
	       reconciled.result_size == applied.result_size);
	critical_operation_id wallet_child = {}, pile_child = {};
	assert(critical_operation_id_derive(command.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 0,
					    &wallet_child));
	assert(critical_operation_id_derive(command.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 1,
					    &pile_child));
	const std::string root_id = operation_hex(command.operation_id);
	const std::string wallet_id = operation_hex(wallet_child);
	const std::string pile_id = operation_hex(pile_child);
	const std::string uid = std::to_string(pile_uid);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid) +
		      " AND copper=1 AND silver=9 AND gold=9 AND platinum=4 AND "
		      "wallet_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert((pile_amount(pile_uid) == coins{ 10, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" + uid +
		      " AND item_revision=1 AND state=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND outcome=1 AND account_count=2 AND posting_count=2 AND "
		      "child_count=2 AND item_event_count=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=0 AND before_copper=1 AND before_silver=0 "
		      "AND before_gold=0 AND before_platinum=5 AND after_copper=1 "
		      "AND after_silver=9 AND after_gold=9 AND after_platinum=4 "
		      "AND before_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision) +
		      " AND after_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=1 AND before_copper=0 AND after_copper=10 "
		      "AND before_revision=0 AND after_revision=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=0 AND delta_copper=0 AND delta_silver=9 "
		      "AND delta_gold=9 AND delta_platinum=-1 AND copper_value=-10") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id + "') AND account_index=1 AND delta_copper=10 AND copper_value=10") ==
	       1);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference WHERE "
		      "operation_id=UNHEX('" +
		      root_id + "') AND item_uid=" + uid +
		      " AND before_revision=0 AND after_revision=1 "
		      "AND legacy_operation_id=UNHEX('" +
		      pile_id + "')") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      root_id + "') AND status=1") == 1);
	const std::string ids =
		"UNHEX('" + root_id + "'),UNHEX('" + wallet_id + "'),UNHEX('" + pile_id + "')";
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id IN (" + ids + ")") ==
	       3);
	check_active_pile_split_merge(pile_uid, room, lineage, epoch);

	execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
		"ON o.outbox_id=d.outbox_id WHERE o.operation_id IN (" +
		ids + ")");
	execute("DELETE FROM critical_outbox WHERE operation_id IN (" + ids + ")");
	execute("DELETE FROM economic_accounting_item_reference WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_child WHERE operation_id=UNHEX('" + root_id +
		"')");
	execute("DELETE FROM economic_accounting_account_effect WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_source_claim WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_operation WHERE operation_id=UNHEX('" + root_id +
		"')");
	execute("DELETE FROM currency_ledger WHERE operation_id=UNHEX('" + wallet_id + "')");
	execute("DELETE FROM item_ownership_ledger WHERE operation_id=UNHEX('" + pile_id + "')");
	execute("DELETE FROM saved_items WHERE obj_uid=" + uid);
	execute("DELETE FROM item_current_owner WHERE item_uid=" + uid);
	execute("DELETE FROM item_owner_revision WHERE owner_type=" +
		std::to_string(static_cast<unsigned int>(item_owner_type::room)) +
		" AND owner_id=" + std::to_string(room_id) + " AND owner_context_id=0");
	execute("DELETE FROM critical_operation_inbox WHERE operation_id IN (" + ids + ")");
	execute("UPDATE player_data SET copper=1,silver=0,gold=0,platinum=5,wallet_revision=" +
		std::to_string(source.change.expected_revisions[0].revision) +
		" WHERE pid=" + std::to_string(pid));
	execute("UPDATE account_banks SET bank_revision=" +
		std::to_string(source.change.expected_revisions[1].revision) + " WHERE id=" +
		std::to_string(scalar("SELECT id FROM account_banks WHERE account_name='" +
				      std::string(account) + "' AND racewar=1")));
	puts("PASS: activated SQL coin drop records the exact change-making denomination vector");
}

void check_active_pile_split_merge(uint64_t pile_uid, item_owner_identity room,
				   const critical_operation_id &lineage,
				   const critical_operation_id &epoch)
{
	constexpr uint64_t split_uid = 900001002;
	const economic_account_key original = { lineage, economic_account_kind::pile, pile_uid, 0 };
	const economic_account_key separated = { lineage, economic_account_kind::pile, split_uid,
						 0 };
	auto accounted = [&](coin_transfer_endpoint source, coin_transfer_endpoint destination,
			     const economic_account_key &from, const economic_account_key &to)
	{
		auto root = coin_command(source, destination);
		std::vector<uint8_t> intent;
		assert(coin_transfer_accounting_intent(root, epoch, from, to, &intent) ==
		       economic_accounting_error::ok);
		root.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		root.accounting_intent = std::move(intent);
		root.publication_required = true;
		assert(critical_command_envelope_valid(root));
		return root;
	};
	auto split = accounted(coin_pile(room, pile_uid, 0, { 10, 0, 0, 0 }, { 6, 0, 0, 0 }),
			       coin_pile(room, split_uid, 0, {}, { 4, 0, 0, 0 }), original,
			       separated);
	auto split_result = pooled_apply(split);
	if (split_result.outcome != critical_apply_outcome::applied || split_result.error_code)
		fprintf(stderr, "typed coin pile split failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(split_result.outcome), split_result.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(split_result.outcome == critical_apply_outcome::applied && !split_result.error_code);
	assert((pile_amount(pile_uid) == coins{ 6, 0, 0, 0 }));
	assert((pile_amount(split_uid) == coins{ 4, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(pile_uid) + " AND item_revision=2 AND state=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(split_uid) + " AND item_revision=1 AND state=1") == 1);

	auto merge = accounted(coin_pile(room, split_uid, 0, { 4, 0, 0, 0 }, {}),
			       coin_pile(room, pile_uid, 0, { 6, 0, 0, 0 }, { 10, 0, 0, 0 }),
			       separated, original);
	auto merge_result = pooled_apply(merge);
	if (merge_result.outcome != critical_apply_outcome::applied || merge_result.error_code)
		fprintf(stderr, "typed coin pile merge failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(merge_result.outcome), merge_result.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(merge_result.outcome == critical_apply_outcome::applied && !merge_result.error_code);
	assert((pile_amount(pile_uid) == coins{ 10, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(pile_uid) + " AND item_revision=3 AND state=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
		      std::to_string(split_uid) + " AND item_revision=2 AND state=2") == 1);

	const critical_command roots[] = { split, merge };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &root = roots[index];
		const auto &applied = index ? merge_result : split_result;
		const auto replay = pooled_apply(root);
		const auto reconciled = critical_command_repository_reconcile(connection, root);
		for (const auto &retained : { replay, reconciled })
			assert(retained.outcome == critical_apply_outcome::already_applied &&
			       retained.result_size == applied.result_size &&
			       std::equal(applied.result_payload.begin(),
					  applied.result_payload.begin() + applied.result_size,
					  retained.result_payload.begin()));
		const std::string root_id = operation_hex(root.operation_id);
		assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE "
			      "operation_id=UNHEX('" +
			      root_id + "') AND status=1") == 1);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      "') AND outcome=1 AND account_count=2 AND posting_count=2 "
			      "AND child_count=2 AND item_event_count=2") == 1);
		assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting "
			      "WHERE operation_id=UNHEX('" +
			      root_id + "')") == 0);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
			      "operation_id=UNHEX('" +
			      root_id + "') AND ABS(copper_value)=4") == 2);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_item_reference r "
			      "JOIN item_ownership_ledger l ON "
			      "l.operation_id=r.legacy_operation_id AND "
			      "l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid "
			      "AND l.item_revision=r.after_revision WHERE r.operation_id=UNHEX('" +
			      root_id + "')") == 2);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      (index ? "') AND ((before_copper=4 AND after_copper=0 AND "
				       "before_revision=1 AND after_revision=2) OR "
				       "(before_copper=6 AND after_copper=10 AND "
				       "before_revision=2 AND after_revision=3))" :
				       "') AND ((before_copper=10 AND after_copper=6 AND "
				       "before_revision=1 AND after_revision=2) OR "
				       "(before_copper=0 AND after_copper=4 AND "
				       "before_revision=0 AND after_revision=1))")) == 2);
	}
	std::vector<std::string> all_ids;
	std::vector<std::string> root_ids;
	std::vector<std::string> child_ids;
	for (const auto &root : roots)
	{
		root_ids.push_back("UNHEX('" + operation_hex(root.operation_id) + "')");
		all_ids.push_back(root_ids.back());
		for (uint64_t index = 0; index < 2; ++index)
		{
			critical_operation_id child = {};
			assert(critical_operation_id_derive(
				root.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, index, &child));
			child_ids.push_back("UNHEX('" + operation_hex(child) + "')");
			all_ids.push_back(child_ids.back());
		}
	}
	auto joined = [](const std::vector<std::string> &values)
	{
		std::string result;
		for (const auto &value : values)
		{
			if (!result.empty())
				result += ',';
			result += value;
		}
		return result;
	};
	const auto ids = joined(all_ids), roots_sql = joined(root_ids);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id IN (" + ids + ")") ==
	       6);
	execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
		"ON o.outbox_id=d.outbox_id WHERE o.operation_id IN (" +
		ids + ")");
	execute("DELETE FROM critical_outbox WHERE operation_id IN (" + ids + ")");
	execute("DELETE FROM economic_accounting_item_reference WHERE operation_id IN (" +
		roots_sql + ")");
	execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id IN (" + roots_sql +
		")");
	execute("DELETE FROM economic_accounting_child WHERE operation_id IN (" + roots_sql + ")");
	execute("DELETE FROM economic_accounting_account_effect WHERE operation_id IN (" +
		roots_sql + ")");
	execute("DELETE FROM economic_accounting_source_claim WHERE operation_id IN (" + roots_sql +
		")");
	execute("DELETE FROM economic_accounting_operation WHERE operation_id IN (" + roots_sql +
		")");
	execute("DELETE FROM item_ownership_ledger WHERE operation_id IN (" + joined(child_ids) +
		")");
	execute("DELETE FROM saved_items WHERE obj_uid=" + std::to_string(split_uid));
	execute("DELETE FROM item_current_owner WHERE item_uid=" + std::to_string(split_uid));
	execute("DELETE FROM critical_operation_inbox WHERE operation_id IN (" + ids + ")");
	puts("PASS: activated SQL pile split and merge retain exact effects, custody links, and replay");
}

void check_active_peer_accounting(uint32_t pid, const char *account,
				  const critical_operation_id &lineage,
				  const critical_operation_id &epoch)
{
	const std::string lineage_sql = "UNHEX('" + operation_hex(lineage) + "')";
	execute("INSERT INTO player_data(name,account_name,racewar,copper) "
		"VALUES('CurrencyPeer','" +
		std::string(account) + "',1,0)");
	const uint32_t peer_pid = static_cast<uint32_t>(mysql_insert_id(connection));
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		lineage_sql + ",1,0,1,1," + std::to_string(peer_pid) + "," +
		std::to_string(peer_pid) + ",UNHEX('cccccccccccccccccccccccccccccccc'))");
	const uint64_t source_lifetime = static_cast<uint64_t>(scalar(
		"SELECT mapping_id FROM economic_account_mapping WHERE lineage=" + lineage_sql +
		" AND account_kind=1 AND native_id=" + std::to_string(pid)));
	const uint64_t peer_lifetime = static_cast<uint64_t>(scalar(
		"SELECT mapping_id FROM economic_account_mapping WHERE lineage=" + lineage_sql +
		" AND account_kind=1 AND native_id=" + std::to_string(peer_pid)));
	const uint64_t bank_revision = static_cast<uint64_t>(
		scalar("SELECT bank_revision FROM account_banks WHERE account_name='" +
		       std::string(account) + "' AND racewar=1"));
	const auto source = coin_wallet(pid, account, { 1, 0, 0, 5 }, { 0, 0, 0, 5 });
	const auto destination = coin_wallet(peer_pid, account, {}, { 1, 0, 0, 0 });
	critical_command command = coin_command(source, destination);
	std::vector<uint8_t> intent;
	assert(coin_transfer_accounting_intent(
		       command, epoch,
		       { lineage, economic_account_kind::wallet, source_lifetime, 0 },
		       { lineage, economic_account_kind::wallet, peer_lifetime, 0 },
		       &intent) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	command.publication_required = true;
	assert(critical_command_envelope_valid(command));
	const auto applied = pooled_apply(command);
	if (applied.outcome != critical_apply_outcome::applied || applied.error_code)
		fprintf(stderr, "typed peer transfer failed outcome=%u error=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			mysql_errno(connection), mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::applied && !applied.error_code);
	const auto replayed = pooled_apply(command);
	assert(replayed.outcome == critical_apply_outcome::already_applied &&
	       replayed.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  replayed.result_payload.begin()));
	const auto reconciled = critical_command_repository_reconcile(connection, command);
	assert(reconciled.outcome == critical_apply_outcome::already_applied &&
	       reconciled.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  reconciled.result_payload.begin()));
	const std::string root_id = operation_hex(command.operation_id);
	critical_operation_id source_child = {}, destination_child = {};
	assert(critical_operation_id_derive(command.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 0,
					    &source_child));
	assert(critical_operation_id_derive(command.operation_id, COIN_TRANSFER_OPERATION_DOMAIN, 1,
					    &destination_child));
	const std::string ids = "UNHEX('" + root_id + "'),UNHEX('" + operation_hex(source_child) +
				"'),UNHEX('" + operation_hex(destination_child) + "')";
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(pid) +
		      " AND copper=0 AND wallet_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + std::to_string(peer_pid) +
		      " AND copper=1 AND wallet_revision=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM account_banks WHERE account_name='" +
		      std::string(account) + "' AND racewar=1 AND bank_revision=" +
		      std::to_string(bank_revision + 2)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND outcome=1 AND account_count=2 AND posting_count=2 AND "
		      "child_count=2 AND item_event_count=0") == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=0 AND before_copper=1 AND after_copper=0 AND "
		      "before_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision) +
		      " AND after_revision=" +
		      std::to_string(source.change.expected_revisions[0].revision + 1)) == 1);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND account_index=1 AND before_copper=0 AND after_copper=1 AND "
		      "before_revision=0 AND after_revision=1") == 1);
	assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id + "')") == 0);
	assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
		      "operation_id=UNHEX('" +
		      root_id +
		      "') AND ((account_index=0 AND delta_copper=-1 AND copper_value=-1) "
		      "OR (account_index=1 AND delta_copper=1 AND copper_value=1))") == 2);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      root_id + "') AND status=1") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id IN (" + ids + ")") ==
	       3);

	// The disposable fixture leaves the legacy coin matrix at its original state.
	execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
		"ON o.outbox_id=d.outbox_id WHERE o.operation_id IN (" +
		ids + ")");
	execute("DELETE FROM critical_outbox WHERE operation_id IN (" + ids + ")");
	execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_child WHERE operation_id=UNHEX('" + root_id +
		"')");
	execute("DELETE FROM economic_accounting_account_effect WHERE operation_id=UNHEX('" +
		root_id + "')");
	execute("DELETE FROM economic_accounting_operation WHERE operation_id=UNHEX('" + root_id +
		"')");
	execute("DELETE FROM currency_ledger WHERE operation_id IN (UNHEX('" +
		operation_hex(source_child) + "'),UNHEX('" + operation_hex(destination_child) +
		"'))");
	execute("DELETE FROM critical_operation_inbox WHERE operation_id IN (" + ids + ")");
	execute("DELETE FROM economic_account_mapping WHERE lineage=" + lineage_sql +
		" AND mapping_id=" + std::to_string(peer_lifetime));
	execute("DELETE FROM player_data WHERE pid=" + std::to_string(peer_pid));
	execute("UPDATE player_data SET copper=1,wallet_revision=" +
		std::to_string(source.change.expected_revisions[0].revision) +
		" WHERE pid=" + std::to_string(pid));
	execute("UPDATE account_banks SET bank_revision=" + std::to_string(bank_revision) +
		" WHERE account_name='" + std::string(account) + "' AND racewar=1");
	puts("PASS: activated SQL peer transfer preserves shared-bank revisions, balanced postings, and replay");
}

void check_active_split_children(const char *account, const critical_operation_id &lineage,
				 const critical_operation_id &epoch)
{
	const std::string lineage_sql = "UNHEX('" + operation_hex(lineage) + "')";
	const uint64_t bank_revision = static_cast<uint64_t>(
		scalar("SELECT bank_revision FROM account_banks WHERE account_name='" +
		       std::string(account) + "' AND racewar=1"));
	const std::array<const char *, 3> names = { "SplitSender", "SplitRecipientOne",
						    "SplitRecipientTwo" };
	std::array<uint32_t, 3> pids = {};
	std::array<uint64_t, 3> lifetimes = {};
	for (size_t index = 0; index < pids.size(); ++index)
	{
		execute("INSERT INTO player_data(name,account_name,racewar,copper) VALUES('" +
			std::string(names[index]) + "','" + account + "',1," +
			std::to_string(index == 0 ? 11 : 0) + ")");
		pids[index] = static_cast<uint32_t>(mysql_insert_id(connection));
		execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
			"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
			"VALUES(" +
			lineage_sql + ",1,0,1,1," + std::to_string(pids[index]) + "," +
			std::to_string(pids[index]) +
			",UNHEX('cccccccccccccccccccccccccccccccc'))");
		lifetimes[index] = static_cast<uint64_t>(
			scalar("SELECT mapping_id FROM economic_account_mapping WHERE lineage=" +
			       lineage_sql +
			       " AND account_kind=1 AND native_id=" + std::to_string(pids[index])));
	}
	std::array<critical_command, 2> children;
	std::array<critical_apply_result, 2> results;
	for (size_t index = 0; index < children.size(); ++index)
	{
		const int32_t before = 11 - 3 * static_cast<int32_t>(index);
		const auto source =
			coin_wallet(pids[0], account, { before, 0, 0, 0 }, { before - 3, 0, 0, 0 });
		const auto destination = coin_wallet(pids[index + 1], account, {}, { 3, 0, 0, 0 });
		auto &command = children[index];
		command = coin_command(source, destination);
		assert(coin_transfer_accounting_intent(
			       command, epoch,
			       { lineage, economic_account_kind::wallet, lifetimes[0], 0 },
			       { lineage, economic_account_kind::wallet, lifetimes[index + 1], 0 },
			       &command.accounting_intent) == economic_accounting_error::ok);
		command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		command.publication_required = true;
		assert(critical_command_envelope_valid(command));
		results[index] = pooled_apply(command);
		assert(results[index].outcome == critical_apply_outcome::applied &&
		       !results[index].error_code);
		const std::string root_id = operation_hex(command.operation_id);
		assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" +
			      std::to_string(pids[0]) +
			      " AND copper=" + std::to_string(before - 3) +
			      " AND wallet_revision=" + std::to_string(index + 1)) == 1);
		assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" +
			      std::to_string(pids[index + 1]) +
			      " AND copper=3 AND wallet_revision=1") == 1);
		assert(scalar("SELECT COUNT(*) FROM account_banks WHERE account_name='" +
			      std::string(account) + "' AND racewar=1 AND bank_revision=" +
			      std::to_string(bank_revision + 2 * (index + 1))) == 1);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      "') AND outcome=1 AND account_count=2 AND posting_count=2 AND "
			      "child_count=2 AND item_event_count=0") == 1);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      "') AND account_index=0 AND before_copper=" + std::to_string(before) +
			      " AND after_copper=" + std::to_string(before - 3) +
			      " AND before_revision=" + std::to_string(index) +
			      " AND after_revision=" + std::to_string(index + 1)) == 1);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_account_effect WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      "') AND account_index=1 AND before_copper=0 AND after_copper=3 "
			      "AND before_revision=0 AND after_revision=1") == 1);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_coin_posting WHERE "
			      "operation_id=UNHEX('" +
			      root_id +
			      "') AND ((account_index=0 AND delta_copper=-3 AND copper_value=-3) "
			      "OR (account_index=1 AND delta_copper=3 AND copper_value=3))") == 2);
		assert(scalar("SELECT SUM(copper_value) FROM economic_accounting_coin_posting "
			      "WHERE operation_id=UNHEX('" +
			      root_id + "')") == 0);
		assert(scalar("SELECT COUNT(*) FROM economic_accounting_source_claim WHERE "
			      "operation_id=UNHEX('" +
			      root_id + "')") == 0);
		assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE "
			      "operation_id=UNHEX('" +
			      root_id + "') AND status=1 AND OCTET_LENGTH(result_payload)>0") == 1);
		assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE operation_id IN "
			      "(SELECT child_operation_id FROM economic_accounting_child WHERE "
			      "operation_id=UNHEX('" +
			      root_id + "'))") == 2);
	}
	assert(scalar("SELECT SUM(copper) FROM player_data WHERE pid IN (" +
		      std::to_string(pids[0]) + "," + std::to_string(pids[1]) + "," +
		      std::to_string(pids[2]) + ")") == 11);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pids[0])) ==
	       5); // 11 / 3 gives two shares of 3; the splitter keeps the extra 2.
	for (size_t index = 0; index < children.size(); ++index)
	{
		const auto replayed = pooled_apply(children[index]);
		assert(replayed.outcome == critical_apply_outcome::already_applied &&
		       replayed.result_size == results[index].result_size &&
		       std::equal(results[index].result_payload.begin(),
				  results[index].result_payload.begin() +
					  results[index].result_size,
				  replayed.result_payload.begin()));
		const std::string root_id = operation_hex(children[index].operation_id);
		critical_operation_id source_child = {}, destination_child = {};
		assert(critical_operation_id_derive(children[index].operation_id,
						    COIN_TRANSFER_OPERATION_DOMAIN, 0,
						    &source_child));
		assert(critical_operation_id_derive(children[index].operation_id,
						    COIN_TRANSFER_OPERATION_DOMAIN, 1,
						    &destination_child));
		const std::string ids = "UNHEX('" + root_id + "'),UNHEX('" +
					operation_hex(source_child) + "'),UNHEX('" +
					operation_hex(destination_child) + "')";
		execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
			"ON o.outbox_id=d.outbox_id WHERE o.operation_id IN (" +
			ids + ")");
		execute("DELETE FROM critical_outbox WHERE operation_id IN (" + ids + ")");
		execute("DELETE FROM economic_accounting_coin_posting WHERE "
			"operation_id=UNHEX('" +
			root_id + "')");
		execute("DELETE FROM economic_accounting_child WHERE operation_id=UNHEX('" +
			root_id + "')");
		execute("DELETE FROM economic_accounting_account_effect WHERE "
			"operation_id=UNHEX('" +
			root_id + "')");
		execute("DELETE FROM economic_accounting_operation WHERE operation_id=UNHEX('" +
			root_id + "')");
		execute("DELETE FROM currency_ledger WHERE operation_id IN (UNHEX('" +
			operation_hex(source_child) + "'),UNHEX('" +
			operation_hex(destination_child) + "'))");
		execute("DELETE FROM critical_operation_inbox WHERE operation_id IN (" + ids + ")");
	}
	execute("DELETE FROM economic_account_mapping WHERE lineage=" + lineage_sql +
		" AND native_id IN (" + std::to_string(pids[0]) + "," + std::to_string(pids[1]) +
		"," + std::to_string(pids[2]) + ")");
	execute("DELETE FROM player_data WHERE pid IN (" + std::to_string(pids[0]) + "," +
		std::to_string(pids[1]) + "," + std::to_string(pids[2]) + ")");
	execute("UPDATE account_banks SET bank_revision=" + std::to_string(bank_revision) +
		" WHERE account_name='" + std::string(account) + "' AND racewar=1");
	puts("PASS: activated SQL split children retain separate balanced roots and the sender remainder");
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

// Run the custody/crash matrix with a real area-authored money prototype (#213).
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
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(put.operation_id) +
		      "') AND destination=10 AND OCTET_LENGTH(payload)=32") == 1);
	assert(critical_command_repository_apply(connection, put).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox i WHERE status=1 "
		      "AND NOT EXISTS (SELECT 1 FROM critical_outbox o WHERE o.operation_id=i.operation_id)") ==
	       0);
	// Simulate a missing legacy inventory projection. Custody still owns the pile.
	execute("DELETE FROM player_items WHERE pid=" + pid_text + " AND obj_uid=900000002");
	verify_reload(900, 100);
	// A later stale projection must not replace the committed amount or metadata.
	execute("INSERT INTO player_items(pid,vnum,obj_uid,container_id,value0,name,item_type) VALUES(" +
		pid_text + ",402013,900000002," + std::to_string(bag_row) + ",1,'stale coins',20)");
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE pid=" + pid_text +
		      " AND obj_uid=900000002") == 1);

	critical_command merge = make_put(900, 100, 200);
	const auto merged = critical_command_repository_apply(connection, merge);
	if (merged.outcome != critical_apply_outcome::applied)
		fprintf(stderr,
			"coin merge failed: outcome=%u error=%u stage=%u mysql_errno=%u errno=%d mysql=%s\n",
			unsigned(merged.outcome), merged.error_code, unsigned(merged.failure_stage),
			mysql_errno(connection), errno, mysql_error(connection));
	assert(merged.outcome == critical_apply_outcome::applied);
	// Discard the first acknowledgement, then replay its operation ID.
	assert(critical_command_repository_apply(connection, merge).outcome ==
	       critical_apply_outcome::already_applied);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM player_items WHERE pid=" + pid_text +
		      " AND obj_uid=900000002 AND value0=300 AND name='coins' AND item_type=20 AND container_id=" +
		      std::to_string(bag_row)) == 1);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	verify_reload(700, 300);
	const auto ledger_count =
		scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text);
	const auto item_revision =
		scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=900000002");
	critical_command stale = make_put(700, 299, 50);
	applied = critical_command_repository_apply(connection, stale);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ESTALE &&
	       applied.failure_stage ==
		       critical_failure_stage::coin_destination_coin_payload_revision);
	assert(failure_stage_of(stale) ==
	       static_cast<unsigned int>(
		       critical_failure_stage::coin_destination_coin_payload_revision));
	assert(scalar("SELECT OCTET_LENGTH(result_payload) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      operation_hex(stale.operation_id) + "')") == 0);
	const auto stale_replay = critical_command_repository_apply(connection, stale);
	assert(stale_replay.error_code == ESTALE &&
	       stale_replay.failure_stage == applied.failure_stage);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
	assert(scalar("SELECT item_revision FROM item_current_owner WHERE item_uid=900000002") ==
	       item_revision);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));

	// A death-style full-wallet conversion captured before another wallet
	// publication must not debit the new balance or create its stale coin pile.
	critical_command conflicted_wallet = make_put(700, 300, 700);
	execute("UPDATE player_data SET copper=701,wallet_revision=wallet_revision+1 WHERE pid=" +
		pid_text);
	applied = critical_command_repository_apply(connection, conflicted_wallet);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ESTALE &&
	       applied.failure_stage == critical_failure_stage::coin_source_wallet_revision);
	coin_transfer_stale_result stale_wallet = {};
	assert(applied.result_size == COIN_TRANSFER_STALE_RESULT_BYTES &&
	       coin_transfer_command_decode_stale_result(decoded, applied.failure_stage,
							 applied.result_payload.data(),
							 applied.result_size, &stale_wallet) &&
	       applied.durable_revision == stale_wallet.current.wallet_revision &&
	       stale_wallet.endpoint_index == 0 && stale_wallet.wallet_stale &&
	       !stale_wallet.bank_stale && stale_wallet.current.wallet.amount[0] == 701 &&
	       stale_wallet.current.wallet_revision ==
		       static_cast<uint64_t>(scalar(
			       "SELECT wallet_revision FROM player_data WHERE pid=" + pid_text)) &&
	       stale_wallet.current.bank_revision == 0 &&
	       std::all_of(stale_wallet.current.bank.amount.begin(),
			   stale_wallet.current.bank.amount.end(),
			   [](int64_t amount) { return amount == 0; }));
	assert(failure_stage_of(conflicted_wallet) ==
	       static_cast<unsigned int>(critical_failure_stage::coin_source_wallet_revision));
	assert(scalar("SELECT OCTET_LENGTH(result_payload) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      operation_hex(conflicted_wallet.operation_id) + "')") ==
	       COIN_TRANSFER_STALE_RESULT_BYTES);
	const auto conflicted_replay =
		critical_command_repository_apply(connection, conflicted_wallet);
	assert(conflicted_replay.error_code == ESTALE &&
	       conflicted_replay.failure_stage == applied.failure_stage &&
	       conflicted_replay.result_size == applied.result_size &&
	       std::equal(applied.result_payload.begin(),
			  applied.result_payload.begin() + applied.result_size,
			  conflicted_replay.result_payload.begin()));
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 701);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	// Restore only the synthetic concurrent credit for the remaining existing
	// crash-window matrix. Its builders read the new authoritative revision.
	execute("UPDATE player_data SET copper=700,wallet_revision=wallet_revision+1 WHERE pid=" +
		pid_text);

	critical_command fault = make_put(700, 300, 50);
	execute("CREATE TRIGGER coin_injected_failure BEFORE UPDATE ON item_current_owner FOR EACH ROW "
		"SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected coin write failure'");
	applied = critical_command_repository_apply(connection, fault);
	if (applied.outcome != critical_apply_outcome::terminal_failure ||
	    applied.error_code != 1644)
		fprintf(stderr, "coin injected failure outcome=%u error=%u stage=%u mysql=%u %s\n",
			static_cast<unsigned int>(applied.outcome), applied.error_code,
			static_cast<unsigned int>(applied.failure_stage), mysql_errno(connection),
			mysql_error(connection));
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == 1644);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	execute("DROP TRIGGER coin_injected_failure");
	// Disconnect the real transaction after its wallet debit, while the pile
	// UPDATE is paused inside the engine. Uncommitted endpoints must both roll
	// back, and retrying the same operation must commit exactly once.
	execute("CREATE TRIGGER coin_crash_pause BEFORE UPDATE ON item_current_owner "
		"FOR EACH ROW SET @coin_pause=SLEEP(10)");
	MYSQL *interrupted = mysql_init(nullptr);
	assert(interrupted &&
	       mysql_real_connect(
		       interrupted, std::getenv("DB_HOST"), std::getenv("DB_USER"),
		       std::getenv("DB_PASSWD"), std::getenv("DB_NAME"),
		       static_cast<unsigned int>(std::strtoul(std::getenv("DB_PORT"), nullptr, 10)),
		       nullptr, 0));
	const auto interrupted_id = mysql_thread_id(interrupted);
	critical_apply_result interrupted_result = {};
	std::thread transaction(
		[&]
		{
			assert(mysql_thread_init() == 0);
			interrupted_result = critical_command_repository_apply(interrupted, fault);
			mysql_close(interrupted);
			mysql_thread_end();
		});
	const auto pause_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
	bool paused = false;
	while (!(paused = scalar("SELECT COUNT(*) FROM information_schema.processlist WHERE ID=" +
				 std::to_string(interrupted_id) +
				 " AND LOWER(STATE)='user sleep'") == 1) &&
	       std::chrono::steady_clock::now() < pause_deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	if (paused)
		kill_if_present(interrupted_id);
	transaction.join();
	assert(paused && interrupted_result.outcome == critical_apply_outcome::retryable_failure);
	execute("DROP TRIGGER coin_crash_pause");
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + pid_text) == 700);
	assert((pile_amount(pile) == coins{ 300, 0, 0, 0 }));
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + pid_text) ==
	       ledger_count);
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
		execute("INSERT INTO player_items(pid,vnum,obj_uid,container_id,value0,item_type) VALUES(" +
			pid_text + ",402013," + std::to_string(uid) + "," +
			std::to_string(bag_row) + ",1,20)");
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
			",1,402013,1)");
		switch (owner.type)
		{
		case item_owner_type::player:
			execute("INSERT INTO player_items(pid,vnum,obj_uid,value0) VALUES(" +
				pid_text + ",402013," + uid_text + ",30)");
			break;
		case item_owner_type::room:
			execute("INSERT INTO saved_items(item_key,room_vnum,vnum,obj_uid,value0) VALUES('coin_fixture',100,402013," +
				uid_text + ",30)");
			break;
		case item_owner_type::corpse:
			execute("INSERT INTO corpse_items(corpse_id,vnum,obj_uid,value0) VALUES(" +
				std::to_string(corpse_id) + ",402013," + uid_text + ",30)");
			break;
		case item_owner_type::locker:
			execute("INSERT INTO locker_items(locker_id,chest_id,vnum,obj_uid,value0) VALUES(" +
				std::to_string(locker_id) + "," + std::to_string(chest_id) +
				",402013," + uid_text + ",30)");
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
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
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

	// Once a wallet/bank lineage has an active epoch, an old accepted operation
	// still replays, but a new ATM operation cannot mutate native holdings
	// outside the double-entry root. This epoch is scoped to this disposable DB.
	const std::string lineage = "UNHEX('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')";
	const std::string epoch = "UNHEX('bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb')";
	const std::string activation = "UNHEX('cccccccccccccccccccccccccccccccc')";
	execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) VALUES(" +
		activation + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,'')");
	execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,"
		"transition_digest,creating_operation_id) VALUES(" +
		lineage + "," + epoch + ",1,1,REPEAT(CHAR(1),32)," + activation + ")");
	execute("INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" + lineage + "," +
		epoch + ")");
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		lineage + ",1,0,1,1," + std::to_string(pid) + "," + std::to_string(pid) + "," +
		activation + ")");
	execute("INSERT INTO economic_account_mapping(lineage,account_kind,context_id,"
		"backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) "
		"VALUES(" +
		lineage + ",2,1,1,2," + std::to_string(bank_id) + "," + std::to_string(bank_id) +
		"," + activation + ")");
	critical_operation_id lineage_id = {}, epoch_id = {};
	lineage_id.bytes.fill(0xaa);
	epoch_id.bytes.fill(0xbb);
	check_active_coin_item_accounting(pid, account.c_str(), lineage_id, epoch_id);
	check_active_coin_change_accounting(pid, account.c_str(), lineage_id, epoch_id);
	check_active_peer_accounting(pid, account.c_str(), lineage_id, epoch_id);
	check_active_split_children(account.c_str(), lineage_id, epoch_id);
	assert(critical_command_repository_apply(connection, deposit).outcome ==
	       critical_apply_outcome::already_applied);
	const auto wallet_before =
		scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid));
	const auto bank_before =
		scalar("SELECT bank_copper FROM account_banks WHERE id=" + std::to_string(bank_id));
	const auto ledger_before =
		scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + std::to_string(pid));
	const currency_vector wallet_after_activation = { { 1, 0, 0, 0 } };
	const currency_vector bank_after_activation = { { -1, 0, 0, 0 } };
	const auto bypass = command_for(
		pid, account.c_str(), wallet_after_activation, bank_after_activation,
		currency_reason_type::atm_withdraw,
		scalar("SELECT wallet_revision FROM player_data WHERE pid=" + std::to_string(pid)),
		scalar("SELECT bank_revision FROM account_banks WHERE id=" +
		       std::to_string(bank_id)));
	const auto blocked = critical_command_repository_apply(connection, bypass);
	if (blocked.outcome != critical_apply_outcome::retryable_failure)
		fprintf(stderr,
			"post-activation legacy ATM outcome=%u error=%u wallet_before=%lld wallet_after=%lld bank_before=%lld bank_after=%lld ledger_before=%lld ledger_after=%lld\n",
			static_cast<unsigned int>(blocked.outcome), blocked.error_code,
			wallet_before,
			scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid)),
			bank_before,
			scalar("SELECT bank_copper FROM account_banks WHERE id=" +
			       std::to_string(bank_id)),
			ledger_before,
			scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" +
			       std::to_string(pid)));
	assert(blocked.outcome == critical_apply_outcome::retryable_failure);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
		      operation_hex(bypass.operation_id) + "')") == 0);
	assert(scalar("SELECT copper FROM player_data WHERE pid=" + std::to_string(pid)) ==
	       wallet_before);
	assert(scalar("SELECT bank_copper FROM account_banks WHERE id=" +
		      std::to_string(bank_id)) == bank_before);
	assert(scalar("SELECT COUNT(*) FROM currency_ledger WHERE pid=" + std::to_string(pid)) ==
	       ledger_before);
	// Tear down only the synthetic activation from this disposable schema; the
	// independent legacy coin/custody matrix below exercises its original
	// pre-activation contract and global inbox/outbox assertions.
	execute("DELETE FROM economic_account_mapping WHERE lineage=" + lineage);
	execute("DELETE FROM economic_lineage_state WHERE lineage=" + lineage);
	execute("DELETE FROM economic_epoch WHERE lineage=" + lineage);
	execute("DELETE FROM critical_operation_inbox WHERE operation_id=" + activation);

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
	const char *selection = std::getenv("CURRENCY_TEST_ATM_ONLY");
	if (selection && std::strcmp(selection, "1") == 0)
		puts("currency ATM legacy/activated/replay checks passed; coin matrix not run");
	else
		coin_failure_matrix();
	mysql_close(connection);
	return 0;
}
