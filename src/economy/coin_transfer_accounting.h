#ifndef DURIS_COIN_TRANSFER_ACCOUNTING_H
#define DURIS_COIN_TRANSFER_ACCOUNTING_H

#include "economy/coin_transfer_command.h"
#include "persistence/economic_accounting_repository.h"

#include <mysql/mysql.h>

// Stable typed writer capability. IDs 1/2 are ATM and 4 is the baseline;
// 3 remains reserved by the historical, disconnected wallet-to-pile recorder.
constexpr uint32_t ECONOMIC_WRITER_WALLET_COIN_TRANSFER = 5;
constexpr size_t ECONOMIC_WALLET_COIN_TRANSFER_FACT_BYTES = 16;

struct coin_transfer_accounting_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
};

// Freeze a schema-2 root from two retained native wallet lifetimes. Pile/source
// item variants are intentionally not admitted by this writer.
economic_accounting_error
coin_transfer_accounting_intent(const critical_command &root, const critical_operation_id &epoch,
				const economic_account_key &source_wallet,
				const economic_account_key &destination_wallet,
				std::vector<uint8_t> *encoded);

// Structural and immutable-intent validation; no SQL or authority lookup.
bool coin_transfer_accounting_command_supported(const critical_command &root) noexcept;

// Called only after the parent root inbox is inserted and while its transaction
// is active. It locks existing lineage/lifetime mappings; it never creates them.
unsigned int coin_transfer_accounting_lock(MYSQL *connection, const critical_command &root,
					   const coin_transfer_payload &payload,
					   coin_transfer_accounting_context *context);

// Append the balanced plan/effects/postings (or a no-posting terminal business
// rejection) inside the caller's existing root transaction. Never commits.
unsigned int coin_transfer_accounting_record(MYSQL *connection, const critical_command &root,
					     const coin_transfer_result &result,
					     unsigned int result_code,
					     const coin_transfer_accounting_context &context);

// Validate retained exact-ID evidence, without consulting the current active
// epoch. The repository has already matched the exact root command hash.
unsigned int coin_transfer_accounting_verify_retained(MYSQL *connection,
						      const critical_command &root,
						      unsigned int result_code,
						      const uint8_t *result_payload,
						      size_t result_size);

#endif
