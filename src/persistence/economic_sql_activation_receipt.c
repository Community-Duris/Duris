#include "persistence/economic_sql_activation_receipt.h"
#include "economy/economic_baseline_adapter.h"
#include "persistence/critical_command.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <openssl/sha.h>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
bool zero(const critical_operation_id &value)
{
	return std::all_of(value.bytes.begin(), value.bytes.end(),
			   [](uint8_t byte) { return byte == 0; });
}
bool zero(const economic_sql_source_digest &value)
{
	return std::all_of(value.begin(), value.end(), [](uint8_t byte) { return byte == 0; });
}
bool same_id(const critical_operation_id &left, const critical_operation_id &right)
{
	return left.bytes == right.bytes;
}
void append_number(std::vector<uint8_t> &bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
}
void append_frame(std::vector<uint8_t> &bytes, std::span<const uint8_t> value)
{
	append_number(bytes, value.size());
	bytes.insert(bytes.end(), value.begin(), value.end());
}
economic_sql_source_digest sha(std::span<const uint8_t> bytes)
{
	economic_sql_source_digest digest = {};
	const uint8_t empty = 0;
	const auto *data = bytes.empty() ? &empty : bytes.data();
	require(SHA256(data, bytes.size(), digest.data()) != nullptr, EIO);
	return digest;
}
economic_sql_source_digest empty_sha()
{
	return sha({});
}

#ifndef __NO_MYSQL__
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
using sql_row = std::vector<std::optional<std::string>>;
unsigned int sql_error(MYSQL *connection)
{
	const auto code = mysql_errno(connection);
	return code ? code : EIO;
}
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ sql_error(connection) };
}
std::vector<sql_row> query(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	require(bool(result), sql_error(connection));
	require(mysql_num_fields(result.get()) == columns);
	std::vector<sql_row> rows;
	while (auto raw = mysql_fetch_row(result.get()))
	{
		const auto lengths = mysql_fetch_lengths(result.get());
		require(lengths, sql_error(connection));
		sql_row fields;
		fields.reserve(columns);
		for (size_t index = 0; index < columns; ++index)
			fields.emplace_back(raw[index] ? std::optional<std::string>(std::string(
								 raw[index], lengths[index])) :
							 std::nullopt);
		rows.push_back(std::move(fields));
	}
	require(!mysql_errno(connection), sql_error(connection));
	return rows;
}
const std::string &text(const sql_row &row, size_t index)
{
	require(index < row.size() && row[index] && !row[index]->empty());
	return *row[index];
}
template <typename T> T integer(const sql_row &row, size_t index)
{
	const auto &value = text(row, index);
	T result = 0;
	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
	require(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size());
	return result;
}
bool boolean(const sql_row &row, size_t index)
{
	const auto value = integer<unsigned int>(row, index);
	require(value <= 1);
	return value == 1;
}
critical_operation_id parse_id(const sql_row &row, size_t index)
{
	const auto &value = text(row, index);
	require(value.size() == CRITICAL_COMMAND_ID_BYTES * 2);
	critical_operation_id id{};
	auto nibble = [](char character) -> uint8_t
	{
		if (character >= '0' && character <= '9')
			return static_cast<uint8_t>(character - '0');
		if (character >= 'a' && character <= 'f')
			return static_cast<uint8_t>(character - 'a' + 10);
		if (character >= 'A' && character <= 'F')
			return static_cast<uint8_t>(character - 'A' + 10);
		throw failure{ EILSEQ };
	};
	for (size_t offset = 0; offset < id.bytes.size(); ++offset)
		id.bytes[offset] = static_cast<uint8_t>((nibble(value[offset * 2]) << 4) |
							nibble(value[offset * 2 + 1]));
	return id;
}
economic_sql_source_digest parse_digest(const sql_row &row, size_t index)
{
	const auto &value = text(row, index);
	require(value.size() == SHA256_DIGEST_LENGTH * 2);
	economic_sql_source_digest digest = {};
	auto nibble = [](char character) -> uint8_t
	{
		if (character >= '0' && character <= '9')
			return static_cast<uint8_t>(character - '0');
		if (character >= 'a' && character <= 'f')
			return static_cast<uint8_t>(character - 'a' + 10);
		if (character >= 'A' && character <= 'F')
			return static_cast<uint8_t>(character - 'A' + 10);
		throw failure{ EILSEQ };
	};
	for (size_t offset = 0; offset < digest.size(); ++offset)
		digest[offset] = static_cast<uint8_t>((nibble(value[offset * 2]) << 4) |
						      (nibble(value[offset * 2 + 1])));
	return digest;
}
economic_account_key parse_account_key(const sql_row &row, size_t index)
{
	const auto &value = text(row, index);
	require(value.size() == ECONOMIC_ACCOUNT_KEY_BYTES * 2);
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
	auto nibble = [](char character) -> uint8_t
	{
		if (character >= '0' && character <= '9')
			return static_cast<uint8_t>(character - '0');
		if (character >= 'a' && character <= 'f')
			return static_cast<uint8_t>(character - 'a' + 10);
		if (character >= 'A' && character <= 'F')
			return static_cast<uint8_t>(character - 'A' + 10);
		throw failure{ EILSEQ };
	};
	for (size_t offset = 0; offset < encoded.size(); ++offset)
		encoded[offset] = static_cast<uint8_t>((nibble(value[offset * 2]) << 4) |
						       nibble(value[offset * 2 + 1]));
	economic_account_key key{};
	require(economic_account_key_decode(encoded, &key) == economic_accounting_error::ok);
	return key;
}
std::string binary(const critical_operation_id &value)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(value.bytes.size() * 2 + 3);
	for (const auto byte : value.bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 0x0f];
	}
	result += '\'';
	return result;
}

// Every persisted field is selected in one joined consistent snapshot. Left
// joins preserve orphan/mismatched rows as explicit missing evidence rather
// than turning them into an accidental successful subset.
constexpr const char *readback_projection =
	"SELECT "
	"HEX(r.operation_id),HEX(r.lineage),HEX(r.epoch),HEX(r.baseline_operation_id),"
	"r.baseline_revision,HEX(r.source_capture_digest),HEX(r.native_boundary_digest),"
	"r.activation_scope,r.coverage_contract_version,HEX(r.coverage_evidence_digest),"
	"HEX(r.activation_digest),r.receipt_version,"
	"HEX(i.operation_id),HEX(i.lineage),HEX(i.epoch),HEX(i.baseline_operation_id),"
	"i.phase,HEX(i.selected_epoch),i.revision,HEX(i.request_digest),"
	"HEX(i.source_capture_digest),HEX(i.native_boundary_digest),"
	"HEX(ri.operation_id),HEX(ri.command_hash),HEX(ri.keys_hash),ri.command_type,"
	"ri.schema_version,ri.payload_version,ri.status,ri.result_code,ri.failure_stage,"
	"ri.durable_revision,OCTET_LENGTH(ri.result_payload),ri.committed_at IS NOT NULL,"
	"HEX(e.lineage),HEX(e.epoch),HEX(e.creating_operation_id),HEX(e.transition_digest),"
	"HEX(s.lineage),HEX(s.active_epoch),s.revision,"
	"HEX(c.lineage),HEX(c.epoch),OCTET_LENGTH(c.opening_account),"
	"HEX(c.creating_operation_id),c.revision,HEX(c.last_operation_id),"
	"HEX(w.operation_id),HEX(w.lineage),HEX(w.epoch),w.book_revision,w.witness_version,"
	"w.holding_count,w.item_count,HEX(w.witness_digest),w.canonical_witness,"
	"HEX(bo.operation_id),HEX(bo.lineage),HEX(bo.epoch),bo.reason,bo.outcome,"
	"bo.result_code,"
	"HEX(bi.operation_id),bi.command_type,bi.schema_version,bi.payload_version,"
	"bi.status,bi.result_code,bi.failure_stage,bi.durable_revision,"
	"OCTET_LENGTH(bi.result_payload),bi.committed_at IS NOT NULL,"
	"i.wallet_count,i.bank_count,HEX(c.opening_account),r.created_at "
	"FROM economic_sql_activation_receipt AS r "
	"LEFT JOIN economic_sql_lifecycle_installation AS i ON i.lineage=r.lineage "
	"LEFT JOIN critical_operation_inbox AS ri ON ri.operation_id=i.operation_id "
	"LEFT JOIN economic_epoch AS e ON e.lineage=r.lineage AND e.epoch=r.epoch "
	"LEFT JOIN economic_lineage_state AS s ON s.lineage=r.lineage "
	"LEFT JOIN economic_baseline_control AS c ON c.lineage=r.lineage AND c.epoch=r.epoch "
	"LEFT JOIN economic_baseline_witness AS w ON w.lineage=r.lineage AND w.epoch=r.epoch "
	" AND w.operation_id=r.baseline_operation_id AND w.book_revision=r.baseline_revision "
	"LEFT JOIN economic_accounting_operation AS bo ON bo.operation_id=r.baseline_operation_id "
	"LEFT JOIN critical_operation_inbox AS bi ON bi.operation_id=r.baseline_operation_id "
	"WHERE r.lineage=";

economic_sql_activation_receipt_readback_row parse_record(const sql_row &fields)
{
	require(fields.size() == 76);
	economic_sql_activation_receipt_readback_row row;
	auto &receipt = row.receipt;
	receipt.operation_id = parse_id(fields, 0);
	receipt.lineage = parse_id(fields, 1);
	receipt.epoch = parse_id(fields, 2);
	receipt.baseline_operation_id = parse_id(fields, 3);
	receipt.baseline_revision = integer<uint64_t>(fields, 4);
	receipt.source_capture_digest = parse_digest(fields, 5);
	receipt.native_boundary_digest = parse_digest(fields, 6);
	receipt.activation_scope = integer<uint8_t>(fields, 7);
	receipt.coverage_contract_version = integer<uint16_t>(fields, 8);
	receipt.coverage_evidence_digest = parse_digest(fields, 9);
	receipt.activation_digest = parse_digest(fields, 10);
	receipt.receipt_version = integer<uint16_t>(fields, 11);
	receipt.created_at = text(fields, 75);

	row.installation_present = fields[12].has_value();
	if (row.installation_present)
	{
		row.installation_operation_id = parse_id(fields, 12);
		row.installation_lineage = parse_id(fields, 13);
		row.installation_epoch = parse_id(fields, 14);
		row.installation_baseline_operation_present = fields[15].has_value();
		if (row.installation_baseline_operation_present)
			row.installation_baseline_operation_id = parse_id(fields, 15);
		row.installation_phase = integer<uint64_t>(fields, 16);
		row.installation_selected_epoch_present = fields[17].has_value();
		if (row.installation_selected_epoch_present)
			row.installation_selected_epoch = parse_id(fields, 17);
		row.installation_revision = integer<uint64_t>(fields, 18);
		row.installation_request_digest = parse_digest(fields, 19);
		row.installation_source_capture_digest = parse_digest(fields, 20);
		row.installation_native_boundary_digest = parse_digest(fields, 21);
		row.installation_wallet_count = integer<uint64_t>(fields, 72);
		row.installation_bank_count = integer<uint64_t>(fields, 73);
	}

	row.installation_inbox_present = fields[22].has_value();
	if (row.installation_inbox_present)
	{
		row.installation_inbox_operation_id = parse_id(fields, 22);
		row.installation_inbox_command_hash = parse_digest(fields, 23);
		row.installation_inbox_keys_hash = parse_digest(fields, 24);
		row.installation_inbox_command_type = integer<uint64_t>(fields, 25);
		row.installation_inbox_schema_version = integer<uint64_t>(fields, 26);
		row.installation_inbox_payload_version = integer<uint64_t>(fields, 27);
		row.installation_inbox_status = integer<uint64_t>(fields, 28);
		row.installation_inbox_result_code = integer<uint64_t>(fields, 29);
		row.installation_inbox_failure_stage = integer<uint64_t>(fields, 30);
		row.installation_inbox_durable_revision = integer<uint64_t>(fields, 31);
		row.installation_inbox_result_payload_bytes = integer<uint64_t>(fields, 32);
		row.installation_inbox_committed = boolean(fields, 33);
	}

	row.epoch_present = fields[34].has_value();
	if (row.epoch_present)
	{
		row.epoch_lineage = parse_id(fields, 34);
		row.epoch_id = parse_id(fields, 35);
		row.epoch_creating_operation_id = parse_id(fields, 36);
		row.epoch_transition_digest = parse_digest(fields, 37);
	}
	row.lineage_state_present = fields[38].has_value();
	if (row.lineage_state_present)
	{
		(void)parse_id(fields, 38);
		row.active_epoch_present = fields[39].has_value();
		if (row.active_epoch_present)
			row.active_epoch = parse_id(fields, 39);
		row.lineage_state_revision = integer<uint64_t>(fields, 40);
	}
	row.baseline_control_present = fields[41].has_value();
	if (row.baseline_control_present)
	{
		(void)parse_id(fields, 41);
		row.baseline_control_epoch = parse_id(fields, 42);
		row.baseline_control_opening_account_bytes = integer<uint64_t>(fields, 43);
		row.baseline_control_creating_operation_id = parse_id(fields, 44);
		row.baseline_control_revision = integer<uint64_t>(fields, 45);
		row.baseline_control_last_operation_present = fields[46].has_value();
		if (row.baseline_control_last_operation_present)
			row.baseline_control_last_operation_id = parse_id(fields, 46);
	}
	row.baseline_control_opening_account_present = fields[74].has_value();
	if (row.baseline_control_opening_account_present)
		row.baseline_control_opening_account = parse_account_key(fields, 74);
	row.baseline_witness_present = fields[47].has_value();
	if (row.baseline_witness_present)
	{
		row.baseline_witness_operation_id = parse_id(fields, 47);
		row.baseline_witness_lineage = parse_id(fields, 48);
		row.baseline_witness_epoch = parse_id(fields, 49);
		row.baseline_witness_revision = integer<uint64_t>(fields, 50);
		row.baseline_witness_version = integer<uint64_t>(fields, 51);
		row.baseline_witness_holding_count = integer<uint64_t>(fields, 52);
		row.baseline_witness_item_count = integer<uint64_t>(fields, 53);
		row.baseline_witness_digest = parse_digest(fields, 54);
		const auto &canonical = text(fields, 55);
		row.baseline_canonical_witness.assign(canonical.begin(), canonical.end());
	}
	row.baseline_operation_present = fields[56].has_value();
	if (row.baseline_operation_present)
	{
		row.baseline_operation_id = parse_id(fields, 56);
		row.baseline_operation_lineage = parse_id(fields, 57);
		row.baseline_operation_epoch = parse_id(fields, 58);
		row.baseline_operation_reason = integer<uint64_t>(fields, 59);
		row.baseline_operation_outcome = integer<uint64_t>(fields, 60);
		row.baseline_operation_result_code = integer<uint64_t>(fields, 61);
	}
	row.baseline_inbox_present = fields[62].has_value();
	if (row.baseline_inbox_present)
	{
		row.baseline_inbox_operation_id = parse_id(fields, 62);
		row.baseline_inbox_command_type = integer<uint64_t>(fields, 63);
		row.baseline_inbox_schema_version = integer<uint64_t>(fields, 64);
		row.baseline_inbox_payload_version = integer<uint64_t>(fields, 65);
		row.baseline_inbox_status = integer<uint64_t>(fields, 66);
		row.baseline_inbox_result_code = integer<uint64_t>(fields, 67);
		row.baseline_inbox_failure_stage = integer<uint64_t>(fields, 68);
		row.baseline_inbox_durable_revision = integer<uint64_t>(fields, 69);
		row.baseline_inbox_result_payload_bytes = integer<uint64_t>(fields, 70);
		row.baseline_inbox_committed = boolean(fields, 71);
	}
	return row;
}
struct read_transaction
{
	MYSQL *connection = nullptr;
	unsigned long session = 0;
	bool started = false;
	unsigned int rollback() noexcept
	{
		if (!started)
			return 0;
		if (!connection || mysql_thread_id(connection) != session)
			return ENOTCONN;
		if (mysql_real_query(connection, "ROLLBACK", 8))
			return mysql_errno(connection) ? mysql_errno(connection) : EIO;
		if (mysql_thread_id(connection) != session ||
		    (connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT))
			return ENOTCONN;
		started = false;
		return 0;
	}
};
bool owns_maintenance_session(MYSQL *connection, unsigned long session)
{
	const auto rows =
		query(connection,
		      "SELECT CONNECTION_ID(),IS_USED_LOCK('duris:economic_sql_boot_maintenance'),"
		      "IS_USED_LOCK('duris:economic_sql_currency_writers')",
		      3);
	if (rows.size() != 1)
		return false;
	const auto connection_id = integer<uint64_t>(rows.front(), 0);
	return integer<uint64_t>(rows.front(), 1) == connection_id &&
	       integer<uint64_t>(rows.front(), 2) == connection_id &&
	       connection_id == static_cast<uint64_t>(session);
}
#endif
} // namespace

unsigned int economic_sql_activation_receipt_digest(const economic_sql_activation_receipt &receipt,
						    economic_sql_source_digest *output) noexcept
{
	try
	{
		require(output, EINVAL);
		require(!zero(receipt.operation_id) && !zero(receipt.lineage) &&
				!zero(receipt.epoch) && !zero(receipt.baseline_operation_id) &&
				receipt.operation_id.bytes != receipt.lineage.bytes &&
				receipt.operation_id.bytes != receipt.epoch.bytes &&
				receipt.operation_id.bytes != receipt.baseline_operation_id.bytes &&
				receipt.lineage.bytes != receipt.epoch.bytes &&
				receipt.lineage.bytes != receipt.baseline_operation_id.bytes &&
				receipt.epoch.bytes != receipt.baseline_operation_id.bytes &&
				receipt.baseline_revision > 0 &&
				!zero(receipt.source_capture_digest) &&
				!zero(receipt.native_boundary_digest) &&
				!zero(receipt.coverage_evidence_digest) &&
				receipt.activation_scope ==
					ECONOMIC_SQL_ACTIVATION_SCOPE_QUALIFICATION_WALLET_ROOT_V1 &&
				receipt.coverage_contract_version ==
					ECONOMIC_SQL_ACTIVATION_COVERAGE_CONTRACT_VERSION_V1 &&
				receipt.receipt_version ==
					ECONOMIC_SQL_ACTIVATION_RECEIPT_VERSION_V1,
			EINVAL);
		static constexpr uint8_t domain[] = "DURIS-SQL-ACTIVATION-RECEIPT-V1";
		std::vector<uint8_t> canonical;
		canonical.insert(canonical.end(), std::begin(domain), std::end(domain) - 1);
		append_frame(canonical, receipt.operation_id.bytes);
		append_frame(canonical, receipt.lineage.bytes);
		append_frame(canonical, receipt.epoch.bytes);
		append_frame(canonical, receipt.baseline_operation_id.bytes);
		append_number(canonical, receipt.baseline_revision);
		append_frame(canonical, receipt.source_capture_digest);
		append_frame(canonical, receipt.native_boundary_digest);
		append_number(canonical, receipt.activation_scope);
		append_number(canonical, receipt.coverage_contract_version);
		append_frame(canonical, receipt.coverage_evidence_digest);
		append_number(canonical, receipt.receipt_version);
		const auto digest = sha(canonical);
		*output = digest;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int
economic_sql_activation_receipt_validate(const economic_sql_activation_receipt &receipt) noexcept
{
	economic_sql_source_digest expected = {};
	const auto status = economic_sql_activation_receipt_digest(receipt, &expected);
	if (status)
		return status;
	return expected == receipt.activation_digest ? 0 : EILSEQ;
}

unsigned int economic_sql_activation_receipt_validate_readback_row(
	const economic_sql_activation_receipt_readback_row &row,
	const critical_operation_id &expected_lineage) noexcept
{
	try
	{
		const auto &receipt = row.receipt;
		require(!zero(expected_lineage) && same_id(receipt.lineage, expected_lineage) &&
			!receipt.created_at.empty());
		require(economic_sql_activation_receipt_validate(receipt) == 0);
		require(row.installation_present &&
			same_id(row.installation_operation_id, receipt.operation_id) &&
			same_id(row.installation_lineage, receipt.lineage) &&
			same_id(row.installation_epoch, receipt.epoch) &&
			row.installation_baseline_operation_present &&
			same_id(row.installation_baseline_operation_id,
				receipt.baseline_operation_id) &&
			row.installation_phase == 2 && row.installation_revision == 1 &&
			row.installation_selected_epoch_present &&
			same_id(row.installation_selected_epoch, receipt.epoch) &&
			row.installation_source_capture_digest == receipt.source_capture_digest &&
			row.installation_native_boundary_digest == receipt.native_boundary_digest);

		critical_operation_id derived_baseline = {};
		require(critical_operation_id_derive(receipt.operation_id,
						     ECONOMIC_BASELINE_OPERATION_DOMAIN, 0,
						     &derived_baseline) &&
			same_id(derived_baseline, receipt.baseline_operation_id));
		require(row.installation_inbox_present &&
			same_id(row.installation_inbox_operation_id, receipt.operation_id) &&
			!zero(row.installation_request_digest) &&
			row.installation_inbox_command_hash == row.installation_request_digest &&
			row.installation_inbox_keys_hash == empty_sha() &&
			row.installation_inbox_command_type ==
				static_cast<uint16_t>(critical_command_type::economic_baseline) &&
			row.installation_inbox_schema_version ==
				CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			row.installation_inbox_payload_version == 1 &&
			row.installation_inbox_status == 0 &&
			row.installation_inbox_result_code == 0 &&
			row.installation_inbox_failure_stage == 0 &&
			row.installation_inbox_durable_revision == 0 &&
			row.installation_inbox_result_payload_bytes == 0 &&
			row.installation_inbox_committed);

		require(row.epoch_present && same_id(row.epoch_lineage, receipt.lineage) &&
			same_id(row.epoch_id, receipt.epoch) &&
			same_id(row.epoch_creating_operation_id, receipt.operation_id) &&
			row.epoch_transition_digest == receipt.native_boundary_digest);
		require(row.lineage_state_present && row.active_epoch_present &&
			same_id(row.active_epoch, receipt.epoch) &&
			row.lineage_state_revision == 1);

		require(row.baseline_control_present &&
			same_id(row.baseline_control_epoch, receipt.epoch) &&
			row.baseline_control_opening_account_bytes == ECONOMIC_ACCOUNT_KEY_BYTES &&
			same_id(row.baseline_control_creating_operation_id, receipt.operation_id) &&
			row.baseline_control_revision == receipt.baseline_revision &&
			row.baseline_control_last_operation_present &&
			same_id(row.baseline_control_last_operation_id,
				receipt.baseline_operation_id));

		require(row.baseline_witness_present &&
			same_id(row.baseline_witness_operation_id, receipt.baseline_operation_id) &&
			same_id(row.baseline_witness_lineage, receipt.lineage) &&
			same_id(row.baseline_witness_epoch, receipt.epoch) &&
			row.baseline_witness_revision == receipt.baseline_revision &&
			(row.baseline_witness_version == 1 || row.baseline_witness_version == 2) &&
			row.baseline_witness_holding_count <= ECONOMIC_BASELINE_MAX_HOLDINGS &&
			row.baseline_witness_item_count <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES &&
			row.baseline_canonical_witness.size() >= ECONOMIC_BASELINE_HEADER_BYTES &&
			sha(row.baseline_canonical_witness) == row.baseline_witness_digest);
		std::optional<economic_prepared_baseline> decoded_baseline;
		require(economic_baseline_decode(row.baseline_canonical_witness,
						 &decoded_baseline) ==
				economic_accounting_error::ok &&
			decoded_baseline.has_value());
		const auto &witness = decoded_baseline->witness();
		require(row.baseline_witness_version == witness.witness_version);
		require(same_id(witness.lineage, receipt.lineage) &&
			same_id(witness.epoch, receipt.epoch) &&
			same_id(witness.preparation_id, receipt.operation_id) &&
			witness.boundary_digest == receipt.native_boundary_digest &&
			row.baseline_witness_holding_count == witness.holdings.size() &&
			row.baseline_witness_item_count == witness.items.size() &&
			witness.items.empty());
		require(row.installation_wallet_count <= ECONOMIC_BASELINE_MAX_HOLDINGS &&
			row.installation_bank_count <= ECONOMIC_BASELINE_MAX_HOLDINGS &&
			row.installation_wallet_count <=
				ECONOMIC_BASELINE_MAX_HOLDINGS - row.installation_bank_count);
		const uint64_t installed_holding_count =
			row.installation_wallet_count + row.installation_bank_count;
		require(installed_holding_count == witness.holdings.size());
		uint64_t decoded_wallet_count = 0;
		uint64_t decoded_bank_count = 0;
		for (const auto &holding : witness.holdings)
		{
			if (holding.account.kind == economic_account_kind::wallet)
				++decoded_wallet_count;
			else if (holding.account.kind == economic_account_kind::bank)
				++decoded_bank_count;
			else
				require(false);
		}
		require(decoded_wallet_count == row.installation_wallet_count &&
			decoded_bank_count == row.installation_bank_count);
		const economic_account_key expected_opening{ receipt.lineage,
							     economic_account_kind::opening, 1, 0 };
		require(row.baseline_control_opening_account_present &&
			economic_account_key_equal(witness.opening_account, expected_opening) &&
			economic_account_key_equal(row.baseline_control_opening_account,
						   expected_opening) &&
			economic_account_key_equal(witness.opening_account,
						   row.baseline_control_opening_account));

		require(row.baseline_operation_present &&
			same_id(row.baseline_operation_id, receipt.baseline_operation_id) &&
			same_id(row.baseline_operation_lineage, receipt.lineage) &&
			same_id(row.baseline_operation_epoch, receipt.epoch) &&
			row.baseline_operation_reason ==
				static_cast<uint16_t>(economic_reason::baseline) &&
			row.baseline_operation_outcome == 1 &&
			row.baseline_operation_result_code == 0);

		require(row.baseline_inbox_present &&
			same_id(row.baseline_inbox_operation_id, receipt.baseline_operation_id) &&
			row.baseline_inbox_command_type ==
				static_cast<uint16_t>(critical_command_type::economic_baseline) &&
			row.baseline_inbox_schema_version ==
				CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			row.baseline_inbox_payload_version == 1 && row.baseline_inbox_status == 1 &&
			row.baseline_inbox_result_code == 0 &&
			row.baseline_inbox_failure_stage == 0 &&
			row.baseline_inbox_durable_revision == receipt.baseline_revision &&
			row.baseline_inbox_result_payload_bytes == 0 &&
			row.baseline_inbox_committed);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_activation_receipt_readback(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority,
	const critical_operation_id &lineage, economic_sql_activation_receipt *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)authority;
	(void)lineage;
	(void)output;
	return ENOTSUP;
#else
	read_transaction transaction;
	try
	{
		require(connection && output && !zero(lineage), EINVAL);
		require(authority.is_maintenance_authority() && authority.is_valid_authority() &&
				authority.connection_ == connection &&
				authority.session_ == mysql_thread_id(connection),
			EPERM);
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
			EBUSY);
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect,
			EPERM);
		const auto session = mysql_thread_id(connection);
		transaction.connection = connection;
		transaction.session = session;
		require(session && owns_maintenance_session(connection, session) &&
				mysql_thread_id(connection) == session,
			EPERM);
		execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		transaction.started = true;
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY");
		require(mysql_thread_id(connection) == session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		const auto rows =
			query(connection, std::string(readback_projection) + binary(lineage), 76);
		require(rows.size() == 1, rows.empty() ? ENOENT : EILSEQ);
		auto parsed = parse_record(rows.front());
		require(economic_sql_activation_receipt_validate_readback_row(parsed, lineage) ==
			0);
		const auto cleanup = transaction.rollback();
		if (cleanup)
			return cleanup;
		static_assert(std::is_nothrow_move_assignable_v<economic_sql_activation_receipt>);
		*output = std::move(parsed.receipt);
		return 0;
	}
	catch (const failure &error)
	{
		const auto cleanup = transaction.rollback();
		return cleanup ? cleanup : (error.code ? error.code : EIO);
	}
	catch (const std::bad_alloc &)
	{
		const auto cleanup = transaction.rollback();
		return cleanup ? cleanup : ENOMEM;
	}
	catch (...)
	{
		const auto cleanup = transaction.rollback();
		return cleanup ? cleanup : EIO;
	}
#endif
}
