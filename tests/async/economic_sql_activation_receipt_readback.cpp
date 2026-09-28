#include "persistence/economic_sql_activation_receipt.h"
#include "economy/economic_baseline_adapter.h"
#include "persistence/critical_command.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <openssl/sha.h>

namespace
{
critical_operation_id id(uint8_t value)
{
	critical_operation_id result{};
	for (size_t index = 0; index < result.bytes.size(); ++index)
		result.bytes[index] = static_cast<uint8_t>(value + index);
	return result;
}
economic_sql_source_digest digest(uint8_t value)
{
	economic_sql_source_digest result{};
	for (size_t index = 0; index < result.size(); ++index)
		result[index] = static_cast<uint8_t>(value + index);
	return result;
}
economic_sql_source_digest hash(const std::vector<uint8_t> &bytes)
{
	economic_sql_source_digest result{};
	const uint8_t empty = 0;
	assert(SHA256(bytes.empty() ? &empty : bytes.data(), bytes.size(), result.data()));
	return result;
}
economic_sql_activation_receipt_readback_row consistent_row()
{
	economic_sql_activation_receipt_readback_row row;
	auto &receipt = row.receipt;
	receipt.operation_id = id(1);
	receipt.lineage = id(41);
	receipt.epoch = id(81);
	assert(critical_operation_id_derive(receipt.operation_id,
					    ECONOMIC_BASELINE_OPERATION_DOMAIN, 0,
					    &receipt.baseline_operation_id));
	receipt.baseline_revision = 1;
	receipt.source_capture_digest = digest(11);
	receipt.native_boundary_digest = digest(71);
	receipt.activation_scope = ECONOMIC_SQL_ACTIVATION_SCOPE_QUALIFICATION_WALLET_ROOT_V1;
	receipt.coverage_contract_version = ECONOMIC_SQL_ACTIVATION_COVERAGE_CONTRACT_VERSION_V1;
	receipt.coverage_evidence_digest = digest(131);
	receipt.receipt_version = ECONOMIC_SQL_ACTIVATION_RECEIPT_VERSION_V1;
	receipt.created_at = "fixture-timestamp";
	assert(economic_sql_activation_receipt_digest(receipt, &receipt.activation_digest) == 0);

	row.installation_present = true;
	row.installation_operation_id = receipt.operation_id;
	row.installation_lineage = receipt.lineage;
	row.installation_epoch = receipt.epoch;
	row.installation_baseline_operation_present = true;
	row.installation_baseline_operation_id = receipt.baseline_operation_id;
	row.installation_phase = 2;
	row.installation_selected_epoch_present = true;
	row.installation_selected_epoch = receipt.epoch;
	row.installation_revision = 1;
	row.installation_request_digest = digest(201);
	row.installation_source_capture_digest = receipt.source_capture_digest;
	row.installation_native_boundary_digest = receipt.native_boundary_digest;
	row.installation_wallet_count = 1;
	row.installation_bank_count = 0;

	row.installation_inbox_present = true;
	row.installation_inbox_operation_id = receipt.operation_id;
	row.installation_inbox_command_hash = row.installation_request_digest;
	const uint8_t empty = 0;
	assert(SHA256(&empty, 0, row.installation_inbox_keys_hash.data()));
	row.installation_inbox_command_type =
		static_cast<uint16_t>(critical_command_type::economic_baseline);
	row.installation_inbox_schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	row.installation_inbox_payload_version = 1;
	row.installation_inbox_status = 0;
	row.installation_inbox_result_code = 0;
	row.installation_inbox_failure_stage = 0;
	row.installation_inbox_durable_revision = 0;
	row.installation_inbox_result_payload_bytes = 0;
	row.installation_inbox_committed = true;

	row.epoch_present = true;
	row.epoch_lineage = receipt.lineage;
	row.epoch_id = receipt.epoch;
	row.epoch_creating_operation_id = receipt.operation_id;
	row.epoch_transition_digest = receipt.native_boundary_digest;
	row.lineage_state_present = true;
	row.active_epoch_present = true;
	row.active_epoch = receipt.epoch;
	row.lineage_state_revision = 1;

	row.baseline_control_present = true;
	row.baseline_control_epoch = receipt.epoch;
	row.baseline_control_opening_account_bytes = ECONOMIC_ACCOUNT_KEY_BYTES;
	row.baseline_control_opening_account_present = true;
	row.baseline_control_opening_account = { receipt.lineage, economic_account_kind::opening, 1,
						 0 };
	row.baseline_control_creating_operation_id = receipt.operation_id;
	row.baseline_control_revision = receipt.baseline_revision;
	row.baseline_control_last_operation_present = true;
	row.baseline_control_last_operation_id = receipt.baseline_operation_id;

	row.baseline_witness_present = true;
	row.baseline_witness_operation_id = receipt.baseline_operation_id;
	row.baseline_witness_lineage = receipt.lineage;
	row.baseline_witness_epoch = receipt.epoch;
	row.baseline_witness_revision = receipt.baseline_revision;
	row.baseline_witness_version = 1;
	row.baseline_witness_holding_count = 1;
	row.baseline_witness_item_count = 0;
	economic_baseline_batch baseline;
	baseline.lineage = receipt.lineage;
	baseline.epoch = receipt.epoch;
	baseline.preparation_id = receipt.operation_id;
	baseline.actor_id = 7;
	baseline.batch_index = 0;
	baseline.opening_account = { receipt.lineage, economic_account_kind::opening, 1, 0 };
	baseline.boundary_digest = receipt.native_boundary_digest;
	baseline.coverage_digest = digest(151);
	const economic_account_key wallet{ receipt.lineage, economic_account_kind::wallet, 901, 0 };
	baseline.holdings.push_back({ wallet, {}, 0, digest(211) });
	std::optional<economic_prepared_baseline> prepared;
	assert(economic_baseline_prepare(baseline, &prepared) == economic_accounting_error::ok);
	assert(prepared.has_value());
	assert(economic_baseline_encode(*prepared, &row.baseline_canonical_witness) ==
	       economic_accounting_error::ok);
	row.baseline_witness_digest = hash(row.baseline_canonical_witness);
	row.baseline_operation_present = true;
	row.baseline_operation_id = receipt.baseline_operation_id;
	row.baseline_operation_lineage = receipt.lineage;
	row.baseline_operation_epoch = receipt.epoch;
	row.baseline_operation_reason = static_cast<uint16_t>(economic_reason::baseline);
	row.baseline_operation_outcome = 1;
	row.baseline_operation_result_code = 0;

	row.baseline_inbox_present = true;
	row.baseline_inbox_operation_id = receipt.baseline_operation_id;
	row.baseline_inbox_command_type =
		static_cast<uint16_t>(critical_command_type::economic_baseline);
	row.baseline_inbox_schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	row.baseline_inbox_payload_version = 1;
	row.baseline_inbox_status = 1;
	row.baseline_inbox_result_code = 0;
	row.baseline_inbox_failure_stage = 0;
	row.baseline_inbox_durable_revision = receipt.baseline_revision;
	row.baseline_inbox_result_payload_bytes = 0;
	row.baseline_inbox_committed = true;
	return row;
}
bool same_receipt(const economic_sql_activation_receipt &left,
		  const economic_sql_activation_receipt &right)
{
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.lineage.bytes == right.lineage.bytes && left.epoch.bytes == right.epoch.bytes &&
	       left.baseline_operation_id.bytes == right.baseline_operation_id.bytes &&
	       left.baseline_revision == right.baseline_revision &&
	       left.source_capture_digest == right.source_capture_digest &&
	       left.native_boundary_digest == right.native_boundary_digest &&
	       left.activation_scope == right.activation_scope &&
	       left.coverage_contract_version == right.coverage_contract_version &&
	       left.coverage_evidence_digest == right.coverage_evidence_digest &&
	       left.activation_digest == right.activation_digest &&
	       left.receipt_version == right.receipt_version && left.created_at == right.created_at;
}
} // namespace

int main()
{
	const auto valid = consistent_row();
	const auto lineage = valid.receipt.lineage;
	assert(economic_sql_activation_receipt_validate(valid.receipt) == 0);
	const economic_sql_source_digest canonical_vector = {
		0x46, 0x3a, 0x04, 0x29, 0x76, 0x8c, 0x01, 0xdb, 0xfa, 0x83, 0x50,
		0x9f, 0xe4, 0xd9, 0x86, 0x1d, 0xc7, 0x70, 0xec, 0xe8, 0x5d, 0xe2,
		0x73, 0x73, 0xa4, 0xfd, 0x31, 0xeb, 0x07, 0x63, 0xca, 0xe3
	};
	assert(valid.receipt.activation_digest == canonical_vector);
	assert(economic_sql_activation_receipt_validate_readback_row(valid, lineage) == 0);

	auto malformed = valid;
	malformed.receipt.receipt_version = 2;
	assert(economic_sql_activation_receipt_validate_readback_row(malformed, lineage) != 0);
	malformed = valid;
	malformed.receipt.created_at.clear();
	assert(economic_sql_activation_receipt_validate_readback_row(malformed, lineage) == EILSEQ);

	auto tampered = valid;
	tampered.receipt.activation_digest[0] ^= 0x80;
	assert(economic_sql_activation_receipt_validate_readback_row(tampered, lineage) == EILSEQ);

	auto orphan = valid;
	orphan.installation_present = false;
	assert(economic_sql_activation_receipt_validate_readback_row(orphan, lineage) == EILSEQ);

	auto stale = valid;
	++stale.baseline_control_revision;
	assert(economic_sql_activation_receipt_validate_readback_row(stale, lineage) == EILSEQ);
	stale = valid;
	stale.baseline_witness_revision = 0;
	assert(economic_sql_activation_receipt_validate_readback_row(stale, lineage) == EILSEQ);

	auto malformed_witness = valid;
	malformed_witness.baseline_canonical_witness.assign(ECONOMIC_BASELINE_HEADER_BYTES, 0);
	malformed_witness.baseline_canonical_witness[0] = 'E';
	malformed_witness.baseline_canonical_witness[1] = 'A';
	malformed_witness.baseline_canonical_witness[2] = 'B';
	malformed_witness.baseline_canonical_witness[3] = '1';
	malformed_witness.baseline_witness_digest =
		hash(malformed_witness.baseline_canonical_witness);
	assert(economic_sql_activation_receipt_validate_readback_row(malformed_witness, lineage) !=
	       0);
	malformed_witness = valid;
	// The codec's preparation_id follows its 16-byte fixed header and two IDs.
	malformed_witness.baseline_canonical_witness[48] ^= 0x40;
	malformed_witness.baseline_witness_digest =
		hash(malformed_witness.baseline_canonical_witness);
	assert(economic_sql_activation_receipt_validate_readback_row(malformed_witness, lineage) !=
	       0);
	malformed_witness = valid;
	malformed_witness.baseline_canonical_witness[120] ^= 1;
	malformed_witness.baseline_witness_digest =
		hash(malformed_witness.baseline_canonical_witness);
	assert(economic_sql_activation_receipt_validate_readback_row(malformed_witness, lineage) !=
	       0);

	auto count_mismatch = valid;
	count_mismatch.installation_wallet_count = 0;
	assert(economic_sql_activation_receipt_validate_readback_row(count_mismatch, lineage) != 0);
	count_mismatch = valid;
	count_mismatch.installation_wallet_count = 0;
	count_mismatch.installation_bank_count = 1;
	assert(economic_sql_activation_receipt_validate_readback_row(count_mismatch, lineage) != 0);
	count_mismatch = valid;
	count_mismatch.baseline_witness_holding_count = 0;
	assert(economic_sql_activation_receipt_validate_readback_row(count_mismatch, lineage) != 0);
	count_mismatch = valid;
	count_mismatch.baseline_witness_item_count = 1;
	assert(economic_sql_activation_receipt_validate_readback_row(count_mismatch, lineage) != 0);
	count_mismatch = valid;
	count_mismatch.installation_wallet_count = std::numeric_limits<uint64_t>::max();
	count_mismatch.installation_bank_count = std::numeric_limits<uint64_t>::max();
	assert(economic_sql_activation_receipt_validate_readback_row(count_mismatch, lineage) != 0);
	auto wrong_opening = valid;
	wrong_opening.baseline_control_opening_account.context_id = 1;
	assert(economic_sql_activation_receipt_validate_readback_row(wrong_opening, lineage) != 0);

	auto mismatched = valid;
	mismatched.installation_source_capture_digest[0] ^= 1;
	assert(economic_sql_activation_receipt_validate_readback_row(mismatched, lineage) ==
	       EILSEQ);
	mismatched = valid;
	mismatched.installation_native_boundary_digest[0] ^= 1;
	assert(economic_sql_activation_receipt_validate_readback_row(mismatched, lineage) ==
	       EILSEQ);
	mismatched = valid;
	mismatched.active_epoch = id(177);
	assert(economic_sql_activation_receipt_validate_readback_row(mismatched, lineage) ==
	       EILSEQ);
	mismatched = valid;
	mismatched.installation_baseline_operation_id = id(231);
	assert(economic_sql_activation_receipt_validate_readback_row(mismatched, lineage) ==
	       EILSEQ);
	mismatched = valid;
	mismatched.baseline_witness_digest[0] ^= 1;
	assert(economic_sql_activation_receipt_validate_readback_row(mismatched, lineage) ==
	       EILSEQ);

	// This exact all-columns-consistent projection is the only positive readback
	// case; all error cases above must fail without a candidate SQL row subset.
	assert(economic_sql_activation_receipt_validate_readback_row(valid, lineage) == 0);

	economic_sql_activation_receipt sentinel;
	sentinel.operation_id = id(171);
	sentinel.lineage = id(191);
	sentinel.created_at = "unchanged sentinel";
	const auto before_failed_readback = sentinel;
	economic_sql_lifecycle_guard invalid_authority;
	assert(economic_sql_activation_receipt_readback(nullptr, invalid_authority, lineage,
							&sentinel) != 0);
	assert(same_receipt(sentinel, before_failed_readback));
	std::cout << "activation receipt canonical/readback row contract PASS\n";
}
