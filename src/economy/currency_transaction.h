#ifndef CURRENCY_TRANSACTION_H
#define CURRENCY_TRANSACTION_H

#include "persistence/critical_command_coordinator.h"
#include "economy/currency_command.h"
#include "core/structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CURRENCY_PENDING_MAX = 1024;
constexpr size_t CURRENCY_PENDING_CONTEXT_MAX_BYTES = 64;

using currency_completion_fn = void (*)(P_char character, bool committed,
					const currency_command_result &result,
					unsigned int error_code, const uint8_t *context,
					size_t context_size);

struct currency_transaction_health
{
	uint64_t pending;
	uint64_t retained_offline;
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t submission_failures;
	uint64_t malformed_completions;
};

bool currency_transaction_can_submit(P_char character);
bool currency_transaction_publish_wallet(P_char character, const currency_vector &wallet,
					 uint64_t wallet_revision);
bool currency_transaction_publish_balances(P_char character, const char *account_name,
					   uint8_t racewar, const currency_vector &wallet,
					   const currency_vector &bank, uint64_t wallet_revision,
					   uint64_t bank_revision);
bool currency_transaction_submit(P_char character, const currency_vector &wallet_delta,
				 const currency_vector &bank_delta, currency_reason_type reason,
				 int64_t reason_id, critical_source_site source_site,
				 critical_deadline_class deadline_class,
				 currency_completion_fn completion, const void *context,
				 size_t context_size);
bool currency_transaction_submit_wallet_value(P_char character, int64_t value_delta,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size);
bool currency_transaction_submit_bank_reward(P_char character, int64_t value,
					     currency_reason_type reason, int64_t reason_id,
					     critical_source_site source_site,
					     critical_deadline_class deadline_class,
					     currency_completion_fn completion, const void *context,
					     size_t context_size);
bool currency_transaction_submit_bank_payment(P_char character, int64_t value,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size);
void currency_transaction_handle_completions(const critical_completion *completions, size_t count);
void currency_transaction_player_ready(P_char character);
currency_transaction_health currency_transaction_health_copy(void);
void currency_transaction_reset_for_tests(void);

#endif
