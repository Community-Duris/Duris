#ifndef DURIS_ECONOMIC_SQL_AUCTION_RETAINED_H
#define DURIS_ECONOMIC_SQL_AUCTION_RETAINED_H
#include "persistence/economic_accounting_repository.h"
// Borrowed trusted transaction; compares immutable original receipt and accounting.
// Never starts/commits a transaction, executes gameplay or captures current balances.
unsigned int economic_sql_auction_verify_retained(MYSQL *, const critical_command &,
						  uint32_t result_code, critical_failure_stage,
						  uint64_t durable_revision,
						  std::span<const uint8_t> result);
#endif
