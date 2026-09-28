#ifndef DURIS_ECONOMIC_SQL_BANK_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_BANK_TRANSACTION_H

#include "economy/economic_currency_adapter.h"
#include <mysql/mysql.h>
#include <memory>

// One root ATM transaction component. Only its own successful domain writer can
// produce the private applied state needed by finalize. No caller-supplied plan
// grants an append capability. Owns no connection and never commits or retries.
class economic_sql_bank_transaction
{
    public:
	~economic_sql_bank_transaction();
	economic_sql_bank_transaction(const economic_sql_bank_transaction &) = delete;
	economic_sql_bank_transaction &operator=(const economic_sql_bank_transaction &) = delete;
	static unsigned int prepare(MYSQL *connection, const critical_command &command,
				    std::unique_ptr<economic_sql_bank_transaction> *transaction);
	unsigned int apply();
	unsigned int finalize();
	// Required after inbox completion, immediately before the root owner commits.
	unsigned int verify_root_completion();
	const currency_command_result &result() const;
	unsigned int result_code() const;

    private:
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit economic_sql_bank_transaction(std::unique_ptr<implementation> state);
};

// Full frozen-policy validation; do not call before retained-root lookup.
bool economic_sql_bank_command_supported(const critical_command &command);
// Validate retained evidence/result without consulting current epoch or balances.
unsigned int economic_sql_bank_verify_retained(MYSQL *connection, const critical_command &command,
					       unsigned int result_code,
					       std::span<const uint8_t> result_payload);

#endif
