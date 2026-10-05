#ifndef CRITICAL_COMMAND_REPOSITORY_H
#define CRITICAL_COMMAND_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <mysql/mysql.h>

struct item_transfer_result;

constexpr size_t CRITICAL_COMMAND_RESULT_MAX_BYTES = 4096;
static_assert(CRITICAL_COMMAND_RESULT_MAX_BYTES <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
constexpr size_t CRITICAL_OUTBOX_PAYLOAD_MAX_BYTES = 65535;

critical_apply_result critical_command_repository_apply(MYSQL *connection,
							const critical_command &command);
critical_apply_result critical_command_repository_reconcile(MYSQL *connection,
							    const critical_command &command);
// Read-only, exact creation evidence for the isolated player recovery owner.
// Caller owns the transaction. Never executes a missing operation.
critical_apply_result
critical_command_repository_verify_creation_in_transaction(MYSQL *connection,
							   const critical_command &command);
// Read-only retained definitive schema-2 coin receipt. Caller must own a
// reconnect-disabled transaction and acquire current native/accounting authority
// locks in native order BEFORE invoking this historical proof. Reads a nonlocking
// committed inbox snapshot; never STARTs/COMMITs/ROLLBACKs or executes a missing
// operation. Missing/uncommitted receipts retain retry; changed identity refuses.
// Successful and authentic rejected roots preserve their exact retained result,
// durable revision and source/destination failure stage after accounting/outbox
// verification. This does not establish current native custody or grant ACK.
critical_apply_result
critical_command_repository_verify_coin_in_transaction(MYSQL *connection,
						       const critical_command &command) noexcept;
// Read-only retained definitive schema-2 ordinary-drop receipt. Caller must own
// a reconnect-disabled transaction and acquire current lineage/season/room/
// custody/payload locks in native order BEFORE invoking this historical proof.
// Uses a nonlocking committed inbox snapshot; it never takes a late inbox lock,
// STARTs/COMMITs/ROLLBACKs, applies a missing command, or grants publication ACK.
// Missing/uncommitted receipts retain retry; changed identity/corruption refuse.
// A proven rejected root returns its exact original terminal_failure result;
// rejected accounting/root/outbox/native movement/payload absence is verified.
// This does not establish current source custody or permit publication/ACK.
critical_apply_result critical_command_repository_verify_ordinary_drop_in_transaction(
	MYSQL *connection, const critical_command &command) noexcept;
// Historical collector purchase proof only. Caller already owns current
// lifetime/catalog/listing/wallet/custody locks on its original transaction.
// Only a fully verified stored rejection returns terminal_failure.
critical_apply_result critical_command_repository_verify_collector_purchase_in_transaction(
	MYSQL *connection, const critical_command &command) noexcept;
// Historical v6 shop proof only. Caller owns an original reconnect-disabled
// transaction and acquires current authority/domain/custody locks first.
// No current projection, transaction lifecycle, application or ACK authority.
// Proof failures retain retry; only an authentic retained rejection is terminal.
critical_apply_result critical_command_repository_verify_shop_trade_in_transaction(
	MYSQL *connection, const critical_command &command) noexcept;
critical_apply_result critical_command_repository_apply_from_pool(const critical_command &command,
								  void *context);

// Shared transaction-finalization helpers for command-specific repositories.
bool critical_command_repository_insert_outbox_event(MYSQL *connection,
						     const critical_operation_id &operation_id,
						     uint16_t event_index, uint16_t destination,
						     uint16_t event_type, uint16_t payload_version,
						     const uint8_t *payload, size_t payload_size);
bool critical_command_repository_finish_inbox(
	MYSQL *connection, const critical_command &command, uint64_t durable_revision,
	unsigned int result_code, const uint8_t *payload, size_t payload_size,
	critical_failure_stage failure_stage = critical_failure_stage::none);
// Transaction-scoped support for compound administrative item operations. The
// caller owns START/COMMIT/ROLLBACK; these preserve the normal inbox and outbox
// audit contract around a nested item-transfer repository call.
bool critical_command_repository_begin_inbox_in_transaction(MYSQL *connection,
							    const critical_command &command);
bool critical_command_repository_finish_item_transfer_in_transaction(
	MYSQL *connection, const critical_command &command, const item_transfer_result &result);

#endif
