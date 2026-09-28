#ifndef DURIS_ECONOMIC_SQL_ACTIVATION_RECEIPT_H
#define DURIS_ECONOMIC_SQL_ACTIVATION_RECEIPT_H

#include "economy/economic_accounting_types.h"
#include "persistence/economic_sql_lifecycle_guard.h"
#include "persistence/economic_sql_source_snapshot.h"
#include <mysql/mysql.h>
#include <optional>
#include <string>
#include <vector>

constexpr uint8_t ECONOMIC_SQL_ACTIVATION_SCOPE_QUALIFICATION_WALLET_ROOT_V1 = 1;
constexpr uint16_t ECONOMIC_SQL_ACTIVATION_COVERAGE_CONTRACT_VERSION_V1 = 1;
constexpr uint16_t ECONOMIC_SQL_ACTIVATION_RECEIPT_VERSION_V1 = 1;

// Durable data only. This record is not an activation, gameplay, or coverage
// capability; a digest or successful validation cannot authorize any transition.
struct economic_sql_activation_receipt
{
	critical_operation_id operation_id = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	critical_operation_id baseline_operation_id = {};
	uint64_t baseline_revision = 0;
	economic_sql_source_digest source_capture_digest = {};
	economic_sql_source_digest native_boundary_digest = {};
	uint8_t activation_scope = 0;
	uint16_t coverage_contract_version = 0;
	economic_sql_source_digest coverage_evidence_digest = {};
	economic_sql_source_digest activation_digest = {};
	uint16_t receipt_version = 0;
	// SQL TIMESTAMP(6) text; audit metadata, intentionally outside EASR1.
	std::string created_at;
};

// Parsed result of one SQL join, exposed as data so malformed/stale row
// validation can be regression-tested without manufacturing SQL authority.
// Never accept caller-constructed instances as SQL provenance.
struct economic_sql_activation_receipt_readback_row
{
	economic_sql_activation_receipt receipt;

	bool installation_present = false;
	critical_operation_id installation_operation_id = {};
	critical_operation_id installation_lineage = {};
	critical_operation_id installation_epoch = {};
	bool installation_baseline_operation_present = false;
	critical_operation_id installation_baseline_operation_id = {};
	uint64_t installation_phase = 0;
	bool installation_selected_epoch_present = false;
	critical_operation_id installation_selected_epoch = {};
	uint64_t installation_revision = 0;
	economic_sql_source_digest installation_request_digest = {};
	economic_sql_source_digest installation_source_capture_digest = {};
	economic_sql_source_digest installation_native_boundary_digest = {};
	uint64_t installation_wallet_count = 0;
	uint64_t installation_bank_count = 0;

	bool installation_inbox_present = false;
	critical_operation_id installation_inbox_operation_id = {};
	economic_sql_source_digest installation_inbox_command_hash = {};
	economic_sql_source_digest installation_inbox_keys_hash = {};
	uint64_t installation_inbox_command_type = 0;
	uint64_t installation_inbox_schema_version = 0;
	uint64_t installation_inbox_payload_version = 0;
	uint64_t installation_inbox_status = 0;
	uint64_t installation_inbox_result_code = 0;
	uint64_t installation_inbox_failure_stage = 0;
	uint64_t installation_inbox_durable_revision = 0;
	uint64_t installation_inbox_result_payload_bytes = 0;
	bool installation_inbox_committed = false;

	bool epoch_present = false;
	critical_operation_id epoch_lineage = {};
	critical_operation_id epoch_id = {};
	critical_operation_id epoch_creating_operation_id = {};
	economic_sql_source_digest epoch_transition_digest = {};

	bool lineage_state_present = false;
	bool active_epoch_present = false;
	critical_operation_id active_epoch = {};
	uint64_t lineage_state_revision = 0;

	bool baseline_control_present = false;
	critical_operation_id baseline_control_epoch = {};
	uint64_t baseline_control_opening_account_bytes = 0;
	bool baseline_control_opening_account_present = false;
	economic_account_key baseline_control_opening_account = {};
	critical_operation_id baseline_control_creating_operation_id = {};
	uint64_t baseline_control_revision = 0;
	bool baseline_control_last_operation_present = false;
	critical_operation_id baseline_control_last_operation_id = {};

	bool baseline_witness_present = false;
	critical_operation_id baseline_witness_operation_id = {};
	critical_operation_id baseline_witness_lineage = {};
	critical_operation_id baseline_witness_epoch = {};
	uint64_t baseline_witness_revision = 0;
	uint64_t baseline_witness_version = 0;
	uint64_t baseline_witness_holding_count = 0;
	uint64_t baseline_witness_item_count = 0;
	economic_sql_source_digest baseline_witness_digest = {};
	std::vector<uint8_t> baseline_canonical_witness;

	bool baseline_operation_present = false;
	critical_operation_id baseline_operation_id = {};
	critical_operation_id baseline_operation_lineage = {};
	critical_operation_id baseline_operation_epoch = {};
	uint64_t baseline_operation_reason = 0;
	uint64_t baseline_operation_outcome = 0;
	uint64_t baseline_operation_result_code = 0;

	bool baseline_inbox_present = false;
	critical_operation_id baseline_inbox_operation_id = {};
	uint64_t baseline_inbox_command_type = 0;
	uint64_t baseline_inbox_schema_version = 0;
	uint64_t baseline_inbox_payload_version = 0;
	uint64_t baseline_inbox_status = 0;
	uint64_t baseline_inbox_result_code = 0;
	uint64_t baseline_inbox_failure_stage = 0;
	uint64_t baseline_inbox_durable_revision = 0;
	uint64_t baseline_inbox_result_payload_bytes = 0;
	bool baseline_inbox_committed = false;
};

// Domain-separated EASR1 digest; writes output only on success. The audit
// timestamp is not semantic and is intentionally excluded from the digest.
unsigned int economic_sql_activation_receipt_digest(const economic_sql_activation_receipt &,
						    economic_sql_source_digest *) noexcept;
unsigned int
economic_sql_activation_receipt_validate(const economic_sql_activation_receipt &) noexcept;

// Pure structural check of a projected SQL join. It is not proof that the row
// was read from SQL and is never authority. The SQL entrypoint below owns that
// provenance boundary and returns only the receipt data value.
unsigned int economic_sql_activation_receipt_validate_readback_row(
	const economic_sql_activation_receipt_readback_row &,
	const critical_operation_id &expected_lineage) noexcept;

// Read-only, single-SELECT consistent-snapshot validation. Requires a live
// maintenance authority bound to this exact MYSQL handle/session and verifies both
// lifecycle named locks there. Returns 0 and fills output only for a consistent
// receipt/phase-2-installation/pointer/baseline row set. Never mutates SQL state,
// installs mappings, or grants activation/gameplay authority. After transaction
// start, every failure attempts a checked rollback; if terminal idleness cannot
// be confirmed, the caller must treat the MYSQL session as unusable and discard
// it through its owner. This function never closes a guard-owned handle.
unsigned int economic_sql_activation_receipt_readback(MYSQL *, const economic_sql_lifecycle_guard &,
						      const critical_operation_id &lineage,
						      economic_sql_activation_receipt *) noexcept;

#endif
