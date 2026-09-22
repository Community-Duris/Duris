#include "economy/auction_repository.h"
#include "economy/collector_repository.h"
#include "economy/boon_reward_repository.h"
#include "persistence/corpse_lifecycle_repository.h"
#include "persistence/player_death_restitution_repository.h"
#include "combat/combat_outcome_repository.h"
#include "guild/artifact_guild_repository.h"
#include "world/zone_touch_repository.h"
#include "account/session_audit_repository.h"
#include "economy/currency_repository.h"
#include "economy/economic_currency_adapter.h"
#include "persistence/critical_command_repository.h"
#include "item/item_transfer_repository.h"
#include "sql/sql_pool.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>

// These must remain unreachable for every unsupported envelope variant.
extern "C" MYSQL *sql_pool_acquire()
{
	assert(false && "unsupported envelope acquired a SQL connection");
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *)
{
	assert(false && "unsupported envelope released a SQL connection");
}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	assert(false && "unsupported envelope replaced a SQL connection");
	return nullptr;
}
extern "C" int __wrap_mysql_server_init(int, char **, char **)
{
	assert(false && "unsupported envelope initialized SQL library");
	return 1;
}
extern "C" decltype(mysql_thread_init()) __wrap_mysql_thread_init()
{
	assert(false && "unsupported envelope initialized SQL thread");
	return 1;
}

void check_rejected(const critical_command &command)
{
	auto *unusable_connection = reinterpret_cast<MYSQL *>(uintptr_t{ 1 });
	currency_command_result currency = {};
	currency.wallet_revision = 99;
	item_transfer_result item = {};
	unsigned int result_code = 123;
	bool mutated = true;
	errno = 0;
	assert(!currency_repository_execute(unusable_connection, command, &currency, &result_code,
					    &mutated));
	assert(errno == EPROTONOSUPPORT && result_code == 123 && mutated &&
	       currency.wallet_revision == 99);
	errno = 0;
	assert(!item_transfer_repository_execute_at_offset(unusable_connection, command, 0, &item,
							   &result_code, &mutated));
	assert(errno == EPROTONOSUPPORT && result_code == 123 && mutated);
	errno = 0;
	assert(!item_transfer_repository_execute(unusable_connection, command, &item, &result_code,
						 &mutated));
	assert(errno == EPROTONOSUPPORT && result_code == 123 && mutated);
	errno = 0;
	assert(!item_transfer_repository_execute_coin(unusable_connection, command, {}, &item,
						      &result_code, &mutated));
	assert(errno == EPROTONOSUPPORT && result_code == 123 && mutated);

	// Every public command-bearing SQL mutation helper shares the same gate.
	auto rejected = [&](bool result)
	{
		assert(!result && errno == EPROTONOSUPPORT);
		errno = 0;
	};
	errno = 0;
	rejected(critical_command_repository_finish_inbox(unusable_connection, command, 1, 0,
							  nullptr, 0));
	rejected(auction_repository_execute(unusable_connection, command, nullptr, &result_code,
					    &mutated));
	rejected(collector_repository_execute(unusable_connection, command, nullptr, &result_code,
					      &mutated));
	rejected(boon_reward_repository_execute(unusable_connection, command, nullptr, &result_code,
						&mutated));
	rejected(corpse_lifecycle_repository_execute(unusable_connection, command, nullptr,
						     &result_code, &mutated, nullptr, nullptr));
	rejected(player_death_restitution_repository_execute(unusable_connection, command, nullptr,
							     &result_code, &mutated));
	rejected(combat_outcome_repository_execute(unusable_connection, command, nullptr,
						   &result_code, &mutated));
	rejected(artifact_guild_repository_execute(unusable_connection, command, nullptr,
						   &result_code, &mutated));
	rejected(zone_touch_repository_execute(unusable_connection, command, nullptr, &result_code,
					       &mutated));
	rejected(session_audit_repository_execute(unusable_connection, command, nullptr));
	rejected(collector_repository_apply_item_boundary(unusable_connection, command, {}, nullptr,
							  nullptr));
	rejected(collector_repository_apply_death_enrollment(unusable_connection, command, {}, {},
							     {}));
	const auto transaction = player_death_restitution_repository_apply_in_transaction(
		unusable_connection, command);
	assert(transaction.outcome == critical_apply_outcome::retryable_failure);
	assert(transaction.error_code == EPROTONOSUPPORT);
	assert(result_code == 123 && mutated);
	const auto pooled = critical_command_repository_apply_from_pool(command, nullptr);
	assert(pooled.outcome == critical_apply_outcome::retryable_failure);
	assert(pooled.error_code == EPROTONOSUPPORT);
	// Only the direct schema-2 bank root now owns a SQL transaction. Its null
	// connection check must still reject before SQL; every closed root retains
	// the poison-pointer proof that the connection is never inspected.
	const bool bank_root = command.schema_version ==
				       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			       command.type == critical_command_type::account_bank &&
			       critical_command_envelope_valid(command);
	auto *root_connection = bank_root ? nullptr : unusable_connection;
	for (const auto &top_level :
	     { critical_command_repository_apply(root_connection, command),
	       critical_command_repository_reconcile(root_connection, command) })
	{
		assert(top_level.outcome == critical_apply_outcome::terminal_failure);
		assert(top_level.error_code == EINVAL);
	}
}

int main()
{
	critical_command legacy = {};
	legacy.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	legacy.operation_id.bytes[0] = 1;
	legacy.type = critical_command_type::account_bank;
	legacy.payload_version = 1;
	legacy.source_site = critical_source_site::command;
	legacy.deadline_class = critical_deadline_class::interactive;
	legacy.accepted_at_usec = 1;
	legacy.keys = { { critical_entity_type::player, 7 } };
	legacy.payload = { 1, 2, 3 };
	assert(critical_command_valid(legacy));
	for (uint32_t schema : { 2u, 1u, 99u })
	{
		auto command = legacy;
		command.schema_version = schema;
		if (schema != 99)
			command.accounting_intent = { 1 };
		assert(!critical_command_valid(command));
		// All unsupported command families retain direct-root no-touch coverage.
		for (uint16_t type = static_cast<uint16_t>(critical_command_type::test);
		     type <= static_cast<uint16_t>(critical_command_type::player_death_restitution);
		     ++type)
		{
			command.type = static_cast<critical_command_type>(type);
			check_rejected(command);
		}
	}
	// Malformed bank envelopes must still reject even a poison connection.
	auto malformed = legacy;
	malformed.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	malformed.accounting_intent = { 1 };
	malformed.accepted_at_usec = 0;
	assert(!critical_command_envelope_valid(malformed));
	check_rejected(malformed);

	// Exercise the null-connection boundary with a fully formed typed bank
	// request too. Successful SQL execution belongs to the native DB harnesses.
	currency_command_payload payload = {};
	payload.pid = 7;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_deposit;
	std::strcpy(payload.account_name.data(), "fixture");
	payload.wallet_delta.amount = { -1, 0, 0, 0 };
	payload.bank_delta.amount = { 1, 0, 0, 0 };
	critical_command bank;
	assert(currency_command_build(&bank, legacy.operation_id, payload, 0, 0,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	bank.accepted_at_usec = 1;
	critical_operation_id lineage = {}, epoch = {};
	lineage.bytes[0] = 2;
	epoch.bytes[0] = 3;
	assert(economic_bank_transfer_intent(
		       bank, epoch, { lineage, economic_account_kind::wallet, 1001, 0 },
		       { lineage, economic_account_kind::bank, 2001, 1 },
		       &bank.accounting_intent) == economic_accounting_error::ok);
	bank.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(bank));
	check_rejected(bank);

	std::cout
		<< "closed SQL helpers, pools and roots reject before connection access; typed bank roots reject null connections\n";
}
