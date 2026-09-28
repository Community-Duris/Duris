#include "economy/account_bank_balances.h"
#include "economy/economic_currency_adapter.h"
#include "flatfile/flatfile_accounting_bank_transaction.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_dispatch.h"
#include "flatfile/flatfile_item_repository.h"
#include "persistence/economic_sql_bank_transaction.h"

#include <cassert>
#include <cerrno>
#include <cstring>

const char *persistence_mode_flatfile_root()
{
	return nullptr;
}

critical_apply_result flatfile_critical_command_repository_apply_selected(const critical_command &,
									  void *)
{
	return { critical_apply_outcome::applied, 1, 0 };
}

critical_apply_result flatfile_accounting_bank_transaction::apply(const std::string &,
								  const critical_command &)
{
	return { critical_apply_outcome::applied, 2, 0 };
}
critical_apply_result flatfile_accounting_coin_transaction::apply(const std::string &,
								  const critical_command &)
{
	return { critical_apply_outcome::applied, 3, 0 };
}
critical_apply_result flatfile_item_repository_apply(const std::string &, const critical_command &)
{
	return { critical_apply_outcome::retryable_failure, 0, ENOTSUP };
}

int main()
{
	AccountBankBalances b = { 10, 20, 30, 40 };
	int64_t total_copper =
		static_cast<int64_t>(b.copper) * 1 + static_cast<int64_t>(b.silver) * 10 +
		static_cast<int64_t>(b.gold) * 100 + static_cast<int64_t>(b.platinum) * 1000;
	assert(total_copper == 43210);
	assert(b.total_copper() == 43210);
	assert(b.is_valid());
	AccountBankBalances b2 = { 10, 20, 30, 40 };
	assert(b == b2);
	AccountBankBalances invalid_b = { -1, 0, 0, 0 };
	assert(!invalid_b.is_valid());

	critical_operation_id root = {}, lineage = {}, epoch = {};
	root.bytes[0] = 0xaa;
	lineage.bytes[0] = 0xbb;
	epoch.bytes[0] = 0xcc;

	currency_command_payload payload = {};
	payload.pid = 42;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_deposit;
	strcpy(payload.account_name.data(), "test_dual_account");
	payload.wallet_delta.amount[0] = -50;
	payload.bank_delta.amount[0] = 50;

	critical_command command;
	assert(currency_command_build(&command, root, payload, UINT64_MAX, UINT64_MAX,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1000;

	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 42, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 100, 1 };

	assert(economic_bank_transfer_intent(command, epoch, wallet, bank,
					     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = 2;

	// 1. Verify MariaDB side
	assert(economic_sql_bank_command_supported(command));
	std::unique_ptr<economic_sql_bank_transaction> sql_tx;
	assert(economic_sql_bank_transaction::prepare(nullptr, command, &sql_tx) == ENOTSUP);
	assert(!sql_tx);
	assert(economic_sql_bank_verify_retained(nullptr, command, 0, {}) == ENOTSUP);

	// 2. Verify Flatfile dispatch side without root fails gracefully with ENOENT
	critical_apply_result flatfile_res = flatfile_accounting_apply_selected(command, nullptr);
	assert(flatfile_res.outcome == critical_apply_outcome::retryable_failure);
	assert(flatfile_res.error_code == ENOENT);

	// 3. Verify unsupported schema-2 command types return ENOTSUP in flatfile dispatch
	critical_command unsupported_cmd = command;
	unsupported_cmd.type = critical_command_type::test;
	critical_apply_result unsupp_res =
		flatfile_accounting_apply_selected(unsupported_cmd, (void *)"/tmp");
	assert(unsupp_res.outcome == critical_apply_outcome::retryable_failure);
	assert(unsupp_res.error_code == ENOTSUP);

	// 4. Corrupt intent detection
	critical_command corrupted_cmd = command;
	corrupted_cmd.accounting_intent.back() ^= 0xff;
	assert(!economic_sql_bank_command_supported(corrupted_cmd));

	return 0;
}
