#include "economy/currency_command.h"

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstring>

static critical_command make_command(currency_reason_type reason, int64_t amount,
				     uint64_t wallet_revision, uint64_t bank_revision)
{
	currency_command_payload payload{};
	payload.pid = 42;
	payload.racewar = 1;
	payload.reason = reason;
	strcpy(payload.account_name.data(), "ram-account");
	payload.wallet_delta.amount[0] = amount;
	critical_command command;
	critical_operation_id id;
	assert(critical_operation_id_generate(&id));
	assert(currency_command_build(&command, id, payload, wallet_revision, bank_revision,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	return command;
}

int main()
{
	const currency_command_result current{ { { 100, 2, 3, 4 } }, { { 1, 2, 3, 4 } }, 7, 9 };
	currency_command_result after{};
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_spend,
							     -40, 7, 9),
						current, &after) == 0);
	assert(after.wallet.amount[0] == 60 && after.wallet_revision == 8 &&
	       after.bank_revision == 10);
	assert(after.bank.amount == current.bank.amount && current.wallet.amount[0] == 100);
	const auto check_unchanged = [&]
	{
		assert(after.wallet.amount[0] == 60 && after.wallet_revision == 8 &&
		       after.bank_revision == 10);
	};
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_spend,
							     -101, 7, 9),
						current, &after) == ENOSPC);
	check_unchanged();
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_spend, -1,
							     6, 9),
						current, &after) == ESTALE);
	check_unchanged();
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_reward,
							     INT_MAX, 7, 9),
						current, &after) == ERANGE);
	check_unchanged();
	auto exhausted = current;
	exhausted.wallet_revision = UINT64_MAX;
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_reward, 1,
							     7, 9),
						exhausted, &after) == ERANGE);
	check_unchanged();
	// A queued reward rebases on the live result of its predecessor.
	assert(currency_command_prepare_outcome(make_command(currency_reason_type::wallet_reward, 5,
							     1, 1),
						current, &after) == 0);
	assert(after.wallet.amount[0] == 105 && after.wallet_revision == 8 &&
	       after.bank_revision == 10);
	const auto next = after;
	assert(currency_command_prepare_outcome(
		       make_command(currency_reason_type::wallet_reward, 7, 1, 1), next, &after) ==
	       0);
	assert(after.wallet.amount[0] == 112 && after.wallet_revision == 9 &&
	       after.bank_revision == 11);
}
