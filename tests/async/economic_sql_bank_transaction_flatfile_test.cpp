#include "persistence/economic_sql_bank_transaction.h"
#include <cassert>
#include <cerrno>
#include <cstring>

int main()
{
	critical_operation_id root = {}, lineage = {}, epoch = {};
	root.bytes[0] = 1;
	lineage.bytes[0] = 2;
	epoch.bytes[0] = 3;
	currency_command_payload payload = {};
	payload.pid = 1;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_deposit;
	strcpy(payload.account_name.data(), "synthetic_account");
	payload.wallet_delta.amount[0] = -10;
	payload.bank_delta.amount[0] = 10;
	critical_command command;
	assert(currency_command_build(&command, root, payload, UINT64_MAX, UINT64_MAX,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 1, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 2, 1 };
	assert(economic_bank_transfer_intent(command, epoch, wallet, bank,
					     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = 2;
	assert(economic_sql_bank_command_supported(command));
	std::unique_ptr<economic_sql_bank_transaction> transaction;
	assert(economic_sql_bank_transaction::prepare(nullptr, command, &transaction) == ENOTSUP &&
	       !transaction);
	assert(economic_sql_bank_verify_retained(nullptr, command, 0, {}) == ENOTSUP);
	command.accounting_intent.back() ^= 1;
	assert(!economic_sql_bank_command_supported(command));
}
