#ifndef DURIS_COIN_TRANSFER_ACCOUNTING_H
#define DURIS_COIN_TRANSFER_ACCOUNTING_H

#include "economy/coin_transfer_command.h"
#include "economy/economic_accounting_plan.h"
#include "persistence/economic_accounting_repository.h"

#include <mysql/mysql.h>

// Stable typed writer capability. IDs 1/2 are ATM and 4 is the baseline;
// 3 remains reserved by the historical disconnected wallet-to-pile recorder.
// This writer supports wallet and UID-keyed physical pile endpoints.
constexpr uint32_t ECONOMIC_WRITER_WALLET_COIN_TRANSFER = 5;
constexpr size_t ECONOMIC_COIN_TRANSFER_FACT_BYTES = 16;

struct coin_transfer_accounting_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
};

// Freeze a schema-2 root from retained wallet lifetimes and/or physical coin
// piles. Pile keys use their item UID; existing piles need a prior effect in the
// active epoch, while a newly created pile must be funded by the other endpoint.
economic_accounting_error
coin_transfer_accounting_intent(const critical_command &root, const critical_operation_id &epoch,
				const economic_account_key &source_account,
				const economic_account_key &destination_account,
				std::vector<uint8_t> *encoded);

// Structural and immutable-intent validation; no SQL or authority lookup.
bool coin_transfer_accounting_command_supported(const critical_command &root) noexcept;

// Stable identity for coin-pile creation/retirement, derived from the debit
// account and its expected revision so a retry may use a new root ID and a new
// destination UID without becoming a second source event.
bool coin_transfer_accounting_source_event(economic_account_kind source_kind, uint64_t source_id,
					   uint64_t source_revision, bool source_retired,
					   bool destination_created,
					   economic_source_event *event) noexcept;

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

// Read-only indexed-row/source-claim verification for an independently authenticated
// stored canonical coin plan. No original command hash/binding, native authority,
// item child ledger, inbox or outbox proof is granted here. The caller owns the
// transaction/session and supplies the already authenticated plan operation ID.
unsigned int coin_transfer_accounting_verify_indexed_plan(MYSQL *connection,
							  const economic_accounting_plan &plan);

// Validate retained exact-ID evidence, without consulting the current active
// epoch. The repository has already matched the exact root command hash.
unsigned int coin_transfer_accounting_verify_retained(
	MYSQL *connection, const critical_command &root, unsigned int result_code,
	const uint8_t *result_payload, size_t result_size,
	critical_failure_stage failure_stage = critical_failure_stage::none);

// Complete original structural coin intent builder. Full wallet/pile decoder,
// source lifecycle, facts and canonical fixed-context freeze are retained.
// Inputs, old output and every sibling owner remain authentic outer state.
economic_accounting_error
coin_transfer_accounting_intent_bounded(const critical_command &, const critical_operation_id &,
					const economic_account_key &, const economic_account_key &,
					std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
					void *, size_t outer_live) noexcept;

// Complete original immutable schema-2 coin proof, including both endpoints,
// facts/authority, actual full admission projection and canonical intent rebuild.
// Inputs, old outputs and all sibling owner storage remain authentic outer.
bool coin_transfer_accounting_command_supported_bounded(const critical_command &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t outer_live) noexcept;

#endif
