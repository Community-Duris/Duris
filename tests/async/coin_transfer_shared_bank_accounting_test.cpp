#include "../../src/economy/coin_transfer_accounting.c"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace
{
critical_operation_id test_operation_id(uint8_t value)
{
	critical_operation_id result = {};
	result.bytes[0] = value;
	return result;
}

void build_endpoint(coin_transfer_endpoint *endpoint, uint32_t pid, const char *account_name,
		    uint8_t operation_id, uint64_t wallet_revision, uint64_t bank_revision,
		    int64_t wallet_delta, int32_t wallet_before)
{
	currency_command_payload change = {};
	change.pid = pid;
	change.reason = currency_reason_type::coin_transfer;
	std::strncpy(change.account_name.data(), account_name, change.account_name.size() - 1);
	change.wallet_delta.amount[0] = wallet_delta;
	endpoint->before[0] = wallet_before;
	endpoint->after[0] = wallet_before + static_cast<int32_t>(wallet_delta);
	if (!currency_command_build(&endpoint->change, test_operation_id(operation_id), change,
				    wallet_revision, bank_revision, critical_source_site::command,
				    critical_deadline_class::interactive))
		std::abort();
}

coin_transfer_payload make_payload(const char *source_bank, const char *destination_bank,
				   uint64_t source_bank_revision,
				   uint64_t destination_bank_revision)
{
	coin_transfer_payload payload = {};
	build_endpoint(&payload.source, 101, source_bank, 1, 10, source_bank_revision, -5, 100);
	build_endpoint(&payload.destination, 202, destination_bank, 2, 20,
		       destination_bank_revision, 5, 200);
	return payload;
}

currency_command_result make_result(const coin_transfer_endpoint &endpoint, uint64_t bank_revision)
{
	currency_command_result result = {};
	result.wallet.amount[0] = endpoint.after[0];
	result.wallet_revision = endpoint.change.expected_revisions[0].revision + 1;
	result.bank_revision = bank_revision;
	return result;
}

unsigned int validation_error(const coin_transfer_payload &payload,
			      const currency_command_result (&results)[2],
			      economic_accounting_plan *plan)
{
	const economic_account_key source = { {}, economic_account_kind::wallet, 1, 0 };
	const economic_account_key destination = { {}, economic_account_kind::wallet, 2, 0 };
	try
	{
		validate_payload(payload, source, destination, results, plan);
	}
	catch (const failure &error)
	{
		return error.code;
	}
	return 0;
}

void test_valid_shared_bank_second_increment()
{
	const auto payload = make_payload("shared_bank", "shared_bank", 7, 7);
	const currency_command_result results[2] = { make_result(payload.source, 8),
						     make_result(payload.destination, 9) };
	economic_accounting_plan plan;
	assert(validation_error(payload, results, &plan) == 0);
	assert(plan.accounts.size() == 2);
	assert(plan.accounts[0].before_revision == 10);
	assert(plan.accounts[0].after_revision == 11);
	assert(plan.accounts[1].before_revision == 20);
	assert(plan.accounts[1].after_revision == 21);
}

void test_incorrect_shared_bank_destination_revision()
{
	const auto payload = make_payload("shared_bank", "shared_bank", 7, 7);
	const currency_command_result results[2] = { make_result(payload.source, 8),
						     make_result(payload.destination, 8) };
	economic_accounting_plan plan;
	assert(validation_error(payload, results, &plan) == EILSEQ);
}

void test_distinct_banks_keep_original_destination_fence()
{
	const auto payload = make_payload("source_bank", "destination_bank", 7, 41);
	const currency_command_result valid_results[2] = { make_result(payload.source, 8),
							   make_result(payload.destination, 42) };
	economic_accounting_plan plan;
	assert(validation_error(payload, valid_results, &plan) == 0);
	const currency_command_result wrong_rebase_results[2] = {
		make_result(payload.source, 8), make_result(payload.destination, 9)
	};
	assert(validation_error(payload, wrong_rebase_results, &plan) == EILSEQ);
}

void test_mismatched_original_shared_bank_fences()
{
	const auto payload = make_payload("shared_bank", "shared_bank", 7, 8);
	const currency_command_result results[2] = { make_result(payload.source, 8),
						     make_result(payload.destination, 9) };
	economic_accounting_plan plan;
	assert(validation_error(payload, results, &plan) == EILSEQ);
}

void test_effective_destination_revision_overflow()
{
	const auto payload =
		make_payload("shared_bank", "shared_bank", UINT64_MAX - 1, UINT64_MAX - 1);
	const currency_command_result results[2] = { make_result(payload.source, UINT64_MAX),
						     make_result(payload.destination, 0) };
	economic_accounting_plan plan;
	assert(validation_error(payload, results, &plan) == EILSEQ);
}
} // namespace

int main()
{
	test_valid_shared_bank_second_increment();
	test_incorrect_shared_bank_destination_revision();
	test_distinct_banks_keep_original_destination_fence();
	test_mismatched_original_shared_bank_fences();
	test_effective_destination_revision_overflow();
	return 0;
}
