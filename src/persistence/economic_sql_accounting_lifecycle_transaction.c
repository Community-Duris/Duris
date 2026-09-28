#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "economy/economic_baseline_command.h"
#include "economy/economic_sql_source_normalize.h"
#include "persistence/economic_sql_baseline_transaction.h"
#include "persistence/economic_accounting_repository.h"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <openssl/sha.h>
#include <set>
#include <string_view>
#include <type_traits>

#ifdef __NO_MYSQL__
unsigned int economic_sql_accounting_lifecycle_transaction::install(
	MYSQL *, const economic_sql_lifecycle_guard &, const economic_sql_lifecycle_request &,
	economic_sql_lifecycle_receipt *) noexcept
{
	return ENOTSUP;
}
#else
namespace
{
struct failure
{
	unsigned int code;
};
void require(bool valid, unsigned int code = EILSEQ)
{
	if (!valid)
		throw failure{ code };
}
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
using row = std::vector<std::optional<std::string>>;
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
std::string binary(std::span<const uint8_t> value)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string text = "X'";
	text.reserve(value.size() * 2 + 3);
	for (auto byte : value)
	{
		text += digits[byte >> 4];
		text += digits[byte & 15];
	}
	text += '\'';
	return text;
}
std::string id(const critical_operation_id &value)
{
	return binary(value.bytes);
}
std::string digest_sql(const economic_sql_source_digest &value)
{
	return binary(value);
}
economic_sql_source_digest sha(std::span<const uint8_t> bytes)
{
	economic_sql_source_digest value = {};
	SHA256(bytes.data(), bytes.size(), value.data());
	return value;
}
void frame(std::vector<uint8_t> &output, std::span<const uint8_t> value)
{
	const uint64_t size = value.size();
	for (size_t index = 0; index < 8; ++index)
		output.push_back(static_cast<uint8_t>(size >> (index * 8)));
	output.insert(output.end(), value.begin(), value.end());
}
void number(std::vector<uint8_t> &output, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		output.push_back(static_cast<uint8_t>(value >> (index * 8)));
}
economic_sql_source_digest request_digest(const economic_sql_lifecycle_request &request)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'S', 'Q', 'L', '-', 'L',
				   'I', 'F', 'E', 'C', 'Y', 'C', 'L', 'E', '-', 'V', '1' };
	frame(data, request.operation_id.bytes);
	frame(data, request.lineage.bytes);
	frame(data, request.epoch.bytes);
	number(data, request.actor_id);
	number(data, request.accepted_at_usec);
	return sha(data);
}
economic_sql_source_digest native_digest(const economic_sql_source_snapshot &snapshot)
{
	const auto wallet = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					 [&](const auto &table)
					 { return table.name == "player_data"; });
	const auto bank = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
				       [&](const auto &table)
				       { return table.name == "account_banks"; });
	require(wallet != snapshot.tables.end() && bank != snapshot.tables.end());
	std::vector<uint8_t> data{ 'E', 'S', 'N', '1' };
	frame(data, wallet->content_digest);
	frame(data, bank->content_digest);
	return sha(data);
}
std::vector<row> query(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == columns);
	std::vector<row> rows;
	while (auto raw = mysql_fetch_row(result.get()))
	{
		auto lengths = mysql_fetch_lengths(result.get());
		require(lengths);
		row values;
		values.reserve(columns);
		for (size_t index = 0; index < columns; ++index)
			values.emplace_back(raw[index] ? std::optional<std::string>(std::string(
								 raw[index], lengths[index])) :
							 std::nullopt);
		rows.push_back(std::move(values));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	return rows;
}
template <typename T> T integer(const std::optional<std::string> &value)
{
	require(value && !value->empty());
	T output = 0;
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), output);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size());
	return output;
}
row one(MYSQL *connection, const std::string &sql, size_t columns)
{
	auto rows = query(connection, sql, columns);
	require(rows.size() == 1, rows.empty() ? ENOENT : EILSEQ);
	return std::move(rows.front());
}
uint64_t scalar(MYSQL *connection, const std::string &sql)
{
	return integer<uint64_t>(one(connection, sql, 1)[0]);
}
void count(MYSQL *connection, const std::string &table, const std::string &where, uint64_t expected)
{
	require(scalar(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + where) ==
		expected);
}
size_t table_index(const economic_sql_source_snapshot &snapshot, std::string_view name)
{
	const auto found = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[&](const auto &table) { return table.name == name; });
	require(found != snapshot.tables.end());
	return static_cast<size_t>(found - snapshot.tables.begin());
}
struct holding_source
{
	bool bank = false;
	uint64_t native_id = 0;
	uint8_t racewar = 0;
	std::string name;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	economic_sql_source_digest digest = {};
};
std::vector<holding_source> read_native_holdings(const economic_sql_source_snapshot &snapshot,
						 const economic_sql_normalized_sources &normalized)
{
	const auto wallets = table_index(snapshot, "player_data");
	const auto banks = table_index(snapshot, "account_banks");
	std::vector<holding_source> output;
	std::set<std::pair<std::string, uint8_t>> bank_names;
	for (const auto &holding : normalized.holdings)
	{
		if (holding.kind != economic_sql_holding_kind::wallet &&
		    holding.kind != economic_sql_holding_kind::bank)
			continue;
		require(holding.disposition == economic_sql_holding_disposition::current &&
			holding.balance && holding.native_revision.has_value() &&
			holding.native_id > 0);
		require(holding.source.row != SIZE_MAX &&
			(holding.kind == economic_sql_holding_kind::wallet ?
				 holding.source.table == wallets :
				 holding.source.table == banks));
		const auto &source = snapshot.tables[holding.source.table].rows[holding.source.row];
		for (auto amount : *holding.balance)
			require(amount >= 0, ERANGE);
		holding_source native;
		native.bank = holding.kind == economic_sql_holding_kind::bank;
		native.native_id = holding.native_id;
		native.balance = *holding.balance;
		native.native_revision = *holding.native_revision;
		native.digest = source.digest;
		if (native.bank)
		{
			require(holding.native_id <= UINT32_MAX && holding.native_context >= 0 &&
					holding.native_context <= 1 && source.cells.size() == 8,
				EINVAL);
			require(source.cells[1] && !source.cells[1]->empty(), EINVAL);
			native.name = *source.cells[1];
			native.racewar = static_cast<uint8_t>(holding.native_context);
			require(bank_names.emplace(native.name, native.racewar).second, EEXIST);
		}
		else
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 9,
				ERANGE);
		}
		output.push_back(std::move(native));
	}
	std::sort(output.begin(), output.end(),
		  [](const auto &left, const auto &right)
		  {
			  if (left.bank != right.bank)
				  return left.bank < right.bank;
			  return left.native_id < right.native_id;
		  });
	const auto wallet_rows = snapshot.tables[wallets].rows.size();
	const auto bank_rows = snapshot.tables[banks].rows.size();
	const auto selected_wallets = static_cast<uint64_t>(std::count_if(
		output.begin(), output.end(), [](const auto &value) { return !value.bank; }));
	const auto selected_banks = static_cast<uint64_t>(output.size()) - selected_wallets;
	require(selected_wallets == static_cast<uint64_t>(wallet_rows) &&
			selected_banks == static_cast<uint64_t>(bank_rows),
		EILSEQ);
	return output;
}
void reject_cutover_defects(const economic_sql_normalized_sources &normalized)
{
	using issue = economic_sql_normalization_issue;
	for (const auto kind :
	     { issue::incomplete_receipt, issue::pending_publication, issue::open_quarantine })
		require(normalized.issue_counts[static_cast<size_t>(kind)] == 0, EBUSY);
}
economic_sql_source_digest coverage_digest(const std::vector<holding_source> &holdings,
					   const std::vector<uint64_t> &mapping_ids)
{
	require(holdings.size() == mapping_ids.size());
	std::vector<uint8_t> data{ 'E', 'S', 'C', '1' };
	number(data, holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		data.push_back(holdings[index].bank ? 2 : 1);
		number(data, holdings[index].native_id);
		number(data, mapping_ids[index]);
		data.push_back(holdings[index].racewar);
		frame(data, holdings[index].digest);
	}
	return sha(data);
}
std::string lower_hex(const std::optional<std::string> &value)
{
	require(value && value->size() % 2 == 0);
	std::string output = *value;
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char character)
		       { return static_cast<char>(std::tolower(character)); });
	return output;
}
critical_operation_id parse_id(const std::optional<std::string> &value)
{
	const auto text = lower_hex(value);
	require(text.size() == 32);
	critical_operation_id output{};
	for (size_t index = 0; index < output.bytes.size(); ++index)
	{
		auto nibble = [](char ch) -> uint8_t
		{
			if (ch >= '0' && ch <= '9')
				return static_cast<uint8_t>(ch - '0');
			if (ch >= 'a' && ch <= 'f')
				return static_cast<uint8_t>(ch - 'a' + 10);
			throw failure{ EILSEQ };
		};
		output.bytes[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
							   nibble(text[index * 2 + 1]));
	}
	return output;
}
void create_operation_receipt(MYSQL *connection, const economic_sql_lifecycle_request &request,
			      const economic_sql_source_digest &request_hash)
{
	const auto empty_keys = sha({});
	const auto command = static_cast<uint16_t>(critical_command_type::economic_baseline);
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
		"schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at) VALUES(" +
			id(request.operation_id) + "," + digest_sql(request_hash) + "," +
			digest_sql(empty_keys) + "," + std::to_string(command) + "," +
			std::to_string(CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) +
			",1,1,0,0,0,X'',CURRENT_TIMESTAMP(6))");
}
struct stored_installation
{
	bool exists = false;
	critical_operation_id operation = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	economic_sql_source_digest request_hash = {};
	economic_sql_source_digest capture_hash = {};
	economic_sql_source_digest native_hash = {};
	std::optional<critical_operation_id> baseline_operation;
	uint64_t wallet_count = 0;
	uint64_t bank_count = 0;
	uint64_t phase = 0;
	std::optional<critical_operation_id> selected_epoch;
	uint64_t revision = 0;
};
stored_installation load_installation(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto rows = query(
		connection,
		"SELECT HEX(operation_id),HEX(lineage),HEX(epoch),HEX(request_digest),"
		"HEX(source_capture_digest),HEX(native_boundary_digest),HEX(baseline_operation_id),"
		"wallet_count,bank_count,phase,HEX(selected_epoch),revision "
		"FROM economic_sql_lifecycle_installation WHERE lineage=" +
			id(lineage) + " FOR UPDATE",
		12);
	require(rows.size() <= 1);
	stored_installation result;
	if (rows.empty())
		return result;
	const auto &record = rows.front();
	result.exists = true;
	result.operation = parse_id(record[0]);
	result.lineage = parse_id(record[1]);
	result.epoch = parse_id(record[2]);
	const auto copy_digest =
		[](const std::optional<std::string> &value, economic_sql_source_digest *target)
	{
		const auto text = lower_hex(value);
		require(text.size() == target->size() * 2);
		for (size_t i = 0; i < target->size(); ++i)
		{
			auto digit = [](char ch) -> uint8_t
			{ return static_cast<uint8_t>(ch <= '9' ? ch - '0' : ch - 'a' + 10); };
			(*target)[i] = static_cast<uint8_t>((digit(text[2 * i]) << 4) |
							    digit(text[2 * i + 1]));
		}
	};
	copy_digest(record[3], &result.request_hash);
	copy_digest(record[4], &result.capture_hash);
	copy_digest(record[5], &result.native_hash);
	if (record[6])
		result.baseline_operation = parse_id(record[6]);
	result.wallet_count = integer<uint64_t>(record[7]);
	result.bank_count = integer<uint64_t>(record[8]);
	result.phase = integer<uint64_t>(record[9]);
	if (record[10])
		result.selected_epoch = parse_id(record[10]);
	result.revision = integer<uint64_t>(record[11]);
	return result;
}
void verify_request(const stored_installation &stored,
		    const economic_sql_lifecycle_request &request,
		    const economic_sql_source_digest &request_hash,
		    const economic_sql_source_digest &native_hash, uint64_t wallets, uint64_t banks)
{
	require(stored.operation.bytes == request.operation_id.bytes &&
			stored.lineage.bytes == request.lineage.bytes &&
			stored.epoch.bytes == request.epoch.bytes &&
			stored.request_hash == request_hash && stored.native_hash == native_hash &&
			stored.wallet_count == wallets && stored.bank_count == banks &&
			stored.phase >= 1 && stored.phase <= 2,
		EEXIST);
}
std::vector<uint64_t> create_or_verify_mappings(MYSQL *connection,
						const economic_sql_lifecycle_request &request,
						const std::vector<holding_source> &holdings,
						bool create)
{
	if (create)
	{
		for (const auto &holding : holdings)
		{
			const uint16_t account_kind =
				holding.bank ? static_cast<uint16_t>(economic_account_kind::bank) :
					       static_cast<uint16_t>(economic_account_kind::wallet);
			const uint16_t locator = holding.bank ? 2 : 1;
			execute(connection,
				"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,"
				"locator_kind,native_id,active_native_id,creating_operation_id,retiring_operation_id,revision) VALUES(" +
					id(request.lineage) + "," + std::to_string(account_kind) +
					"," + std::to_string(holding.bank ? holding.racewar : 0) +
					",1," + std::to_string(locator) + "," +
					std::to_string(holding.native_id) + "," +
					std::to_string(holding.native_id) + "," +
					id(request.operation_id) + ",NULL,0)");
		}
	}
	const auto rows = query(
		connection,
		"SELECT mapping_id,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,HEX(creating_operation_id),HEX(retiring_operation_id),revision "
		"FROM economic_account_mapping WHERE lineage=" +
			id(request.lineage) + " ORDER BY locator_kind,native_id FOR UPDATE",
		10);
	require(rows.size() == holdings.size(), EILSEQ);
	std::vector<uint64_t> mapping_ids;
	mapping_ids.reserve(rows.size());
	for (size_t index = 0; index < rows.size(); ++index)
	{
		const auto &record = rows[index];
		const auto &source = holdings[index];
		const auto account_kind = integer<uint16_t>(record[1]);
		const auto context = integer<uint64_t>(record[2]);
		const auto locator = integer<uint16_t>(record[4]);
		const auto expected_kind =
			source.bank ? static_cast<uint16_t>(economic_account_kind::bank) :
				      static_cast<uint16_t>(economic_account_kind::wallet);
		require(account_kind == expected_kind &&
				context == (source.bank ? source.racewar : 0) &&
				integer<uint8_t>(record[3]) == ECONOMIC_MAPPING_BACKEND_SQL &&
				locator == (source.bank ? 2 : 1) &&
				integer<uint64_t>(record[5]) == source.native_id &&
				integer<uint64_t>(record[6]) == source.native_id &&
				parse_id(record[7]).bytes == request.operation_id.bytes &&
				!record[8] && integer<uint64_t>(record[9]) == 0,
			EILSEQ);
		const auto mapping = integer<uint64_t>(record[0]);
		require(mapping > 0, EILSEQ);
		mapping_ids.push_back(mapping);
	}
	return mapping_ids;
}
economic_baseline_batch make_batch(const economic_sql_lifecycle_request &request,
				   const std::vector<holding_source> &holdings,
				   const std::vector<uint64_t> &mapping_ids,
				   const economic_sql_source_digest &native_hash)
{
	require(holdings.size() == mapping_ids.size());
	economic_baseline_batch batch;
	batch.lineage = request.lineage;
	batch.epoch = request.epoch;
	batch.preparation_id = request.operation_id;
	batch.actor_id = request.actor_id;
	batch.batch_index = 0;
	batch.opening_account = { request.lineage, economic_account_kind::opening, 1, 0 };
	batch.boundary_digest = native_hash;
	batch.coverage_digest = coverage_digest(holdings, mapping_ids);
	batch.holdings.reserve(holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		economic_account_key account{
			request.lineage,
			source.bank ? economic_account_kind::bank : economic_account_kind::wallet,
			mapping_ids[index], static_cast<uint64_t>(source.bank ? source.racewar : 0)
		};
		batch.holdings.push_back(
			{ account, source.balance, source.native_revision, source.digest });
	}
	return batch;
}
critical_operation_id baseline_id(const economic_sql_lifecycle_request &request)
{
	critical_operation_id value{};
	require(critical_operation_id_derive(request.operation_id,
					     ECONOMIC_BASELINE_OPERATION_DOMAIN, 0, &value),
		EINVAL);
	return value;
}
using baseline_initializer = unsigned int (*)(MYSQL *, const critical_operation_id &,
					      const critical_operation_id &,
					      const economic_account_key &,
					      const critical_operation_id &);
void create_installation(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const economic_sql_source_digest &request_hash,
			 const economic_sql_source_digest &capture_hash,
			 const economic_sql_source_digest &native_hash, uint64_t wallets,
			 uint64_t banks, const std::vector<holding_source> &holdings,
			 baseline_initializer initialize)
{
	count(connection, "economic_lineage_state", "lineage=" + id(request.lineage), 0);
	count(connection, "economic_account_mapping", "lineage=" + id(request.lineage), 0);
	count(connection, "critical_operation_inbox", "operation_id=" + id(request.operation_id),
	      0);
	create_operation_receipt(connection, request, request_hash);
	execute(connection,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
			id(request.lineage) + ",NULL,0)");
	execute(connection,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(" +
			id(request.lineage) + "," + id(request.epoch) + ",1,NULL,1," +
			digest_sql(native_hash) + "," + id(request.operation_id) + ")");
	(void)create_or_verify_mappings(connection, request, holdings, true);
	const economic_account_key opening{ request.lineage, economic_account_kind::opening, 1, 0 };
	const auto initialized = initialize(connection, request.lineage, request.epoch, opening,
					    request.operation_id);
	require(initialized == 0, initialized);
	execute(connection,
		"INSERT INTO economic_sql_lifecycle_installation(operation_id,lineage,epoch,request_digest,"
		"source_capture_digest,native_boundary_digest,baseline_operation_id,wallet_count,bank_count,phase,selected_epoch,revision) VALUES(" +
			id(request.operation_id) + "," + id(request.lineage) + "," +
			id(request.epoch) + "," + digest_sql(request_hash) + "," +
			digest_sql(capture_hash) + "," + digest_sql(native_hash) + ",NULL," +
			std::to_string(wallets) + "," + std::to_string(banks) + ",1,NULL,0)");
}
void ensure_no_preexisting_mapping(const economic_sql_source_snapshot &snapshot,
				   const critical_operation_id &lineage)
{
	const auto index = table_index(snapshot, "economic_account_mapping");
	for (const auto &record : snapshot.tables[index].rows)
	{
		require(record.cells.size() == 11);
		if (record.cells[1] && record.cells[1]->size() == lineage.bytes.size() &&
		    std::equal(record.cells[1]->begin(), record.cells[1]->end(),
			       reinterpret_cast<const char *>(lineage.bytes.data())))
			throw failure{ EEXIST };
	}
}
void fill_export(const economic_sql_lifecycle_request &request,
		 const std::vector<holding_source> &holdings,
		 const std::vector<uint64_t> &mapping_ids, const stored_installation &stored,
		 uint64_t revision, economic_sql_lifecycle_receipt *output)
{
	economic_sql_lifecycle_receipt receipt;
	receipt.operation_id = request.operation_id;
	receipt.lineage = request.lineage;
	receipt.epoch = request.epoch;
	receipt.baseline_operation_id = stored.baseline_operation.value_or(baseline_id(request));
	receipt.source_capture_digest = stored.capture_hash;
	receipt.native_boundary_digest = stored.native_hash;
	receipt.baseline_revision = revision;
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		economic_account_key account{
			request.lineage,
			source.bank ? economic_account_kind::bank : economic_account_kind::wallet,
			mapping_ids[index], static_cast<uint64_t>(source.bank ? source.racewar : 0)
		};
		if (source.bank)
			receipt.banks.push_back({ source.name, source.racewar, account });
		else
			receipt.wallets.push_back(
				{ static_cast<uint32_t>(source.native_id), account });
	}
	*output = std::move(receipt);
}
void verify_staged_epoch(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const critical_operation_id &baseline_operation, uint64_t revision)
{
	const auto lineage =
		one(connection,
		    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
			    id(request.lineage) + " LOCK IN SHARE MODE",
		    1);
	require(!lineage[0], EPERM);
	const auto row =
		one(connection,
		    "SELECT phase,HEX(selected_epoch),HEX(baseline_operation_id),revision FROM "
		    "economic_sql_lifecycle_installation WHERE operation_id=" +
			    id(request.operation_id) + " LOCK IN SHARE MODE",
		    4);
	require(integer<uint8_t>(row[0]) == 2 && parse_id(row[1]).bytes == request.epoch.bytes &&
			parse_id(row[2]).bytes == baseline_operation.bytes &&
			integer<uint64_t>(row[3]) == 1 && revision == 1,
		EILSEQ);
}
void select_staged_epoch(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const economic_sql_source_digest &request_hash,
			 const critical_operation_id &baseline_operation)
{
	execute(connection,
		"UPDATE economic_sql_lifecycle_installation SET baseline_operation_id=" +
			id(baseline_operation) +
			",phase=2,selected_epoch=epoch,revision=revision+1 WHERE operation_id=" +
			id(request.operation_id) + " AND lineage=" + id(request.lineage) +
			" AND epoch=" + id(request.epoch) +
			" AND request_digest=" + digest_sql(request_hash) +
			" AND phase=1 AND revision=0 AND selected_epoch IS NULL");
	require(mysql_affected_rows(connection) == 1);
}
void check_active_epoch_null(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto state = one(
		connection,
		"SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" + id(lineage),
		1);
	require(!state[0], EPERM);
}
struct transaction
{
	MYSQL *connection;
	unsigned long session;
	bool started = false;
	~transaction()
	{
		if (started && mysql_thread_id(connection) == session)
			(void)mysql_real_query(connection, "ROLLBACK", 8);
	}
};
} // namespace

unsigned int economic_sql_accounting_lifecycle_transaction::install(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority,
	const economic_sql_lifecycle_request &request,
	economic_sql_lifecycle_receipt *output) noexcept
{
	try
	{
		require(connection && output, EINVAL);
		require(authority.is_maintenance_authority() &&
				authority.connection_ == connection &&
				authority.session_ == mysql_thread_id(connection),
			EPERM);
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
			EBUSY);
		require(!critical_operation_id_is_zero(request.operation_id) &&
				!critical_operation_id_is_zero(request.lineage) &&
				!critical_operation_id_is_zero(request.epoch) && request.actor_id &&
				request.accepted_at_usec &&
				request.operation_id.bytes != request.lineage.bytes &&
				request.operation_id.bytes != request.epoch.bytes &&
				request.lineage.bytes != request.epoch.bytes,
			EINVAL);
		const auto expected_request_hash = request_digest(request);
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
		economic_sql_source_snapshot snapshot;
		const auto captured = economic_sql_capture_sources(connection, {}, &snapshot);
		require(captured == 0, captured);
		economic_sql_normalized_sources normalized;
		const auto normalized_result =
			economic_sql_normalize_sources(snapshot, 512, &normalized);
		require(normalized_result == economic_accounting_error::ok,
			normalized_result == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
		reject_cutover_defects(normalized);
		const auto holdings = read_native_holdings(snapshot, normalized);
		const auto native_hash = native_digest(snapshot);
		const auto capture_hash = snapshot.digest;
		const auto wallets = static_cast<uint64_t>(
			std::count_if(holdings.begin(), holdings.end(),
				      [](const auto &value) { return !value.bank; }));
		const auto banks = holdings.size() - wallets;
		require(wallets + banks == holdings.size() &&
				wallets + banks <= ECONOMIC_BASELINE_MAX_HOLDINGS,
			E2BIG);
		const auto existing = load_installation(connection, request.lineage);
		std::vector<uint64_t> mapping_ids;
		if (!existing.exists)
		{
			ensure_no_preexisting_mapping(snapshot, request.lineage);
			const auto session = mysql_thread_id(connection);
			transaction owner{ connection, session, true };
			execute(connection, "START TRANSACTION");
			const auto initialize_baseline = [](MYSQL *database,
							    const critical_operation_id &lineage,
							    const critical_operation_id &epoch,
							    const economic_account_key &opening,
							    const critical_operation_id &operation)
			{
				return economic_sql_baseline_transaction::initialize(
					database, lineage, epoch, opening, operation);
			};
			create_installation(connection, request, expected_request_hash,
					    capture_hash, native_hash, wallets, banks, holdings,
					    initialize_baseline);
			require(mysql_thread_id(connection) == session &&
					(connection->server_status & SERVER_STATUS_IN_TRANS),
				ENOTCONN);
			execute(connection, "COMMIT");
			owner.started = false;
			mapping_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		else
		{
			verify_request(existing, request, expected_request_hash, native_hash,
				       wallets, banks);
			mapping_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		const auto stored = load_installation(connection, request.lineage);
		require(stored.exists);
		verify_request(stored, request, expected_request_hash, native_hash, wallets, banks);
		auto batch = make_batch(request, holdings, mapping_ids, native_hash);
		std::optional<economic_prepared_baseline> prepared;
		require(economic_baseline_prepare(batch, &prepared) ==
				economic_accounting_error::ok,
			EINVAL);
		critical_command command{};
		require(economic_baseline_command_build(*prepared, request.accepted_at_usec,
							&command) == economic_accounting_error::ok,
			EINVAL);
		const auto expected_baseline_id = baseline_id(request);
		require(command.operation_id.bytes == expected_baseline_id.bytes, EILSEQ);
		critical_apply_result applied =
			economic_sql_baseline_transaction::apply(connection, command, *prepared);
		if (applied.outcome == critical_apply_outcome::ambiguous_commit ||
		    applied.outcome == critical_apply_outcome::retryable_failure)
		{
			const auto reconciled =
				economic_sql_baseline_transaction::reconcile(connection, command);
			if (reconciled.outcome != critical_apply_outcome::already_applied)
				throw failure{ reconciled.error_code ? reconciled.error_code :
					       applied.error_code    ? applied.error_code :
								       EAGAIN };
			applied = reconciled;
		}
		require(applied.outcome == critical_apply_outcome::applied ||
				applied.outcome == critical_apply_outcome::already_applied,
			applied.error_code ? applied.error_code : EIO);
		const auto receipt =
			economic_sql_baseline_transaction::reconcile(connection, command);
		require(receipt.outcome == critical_apply_outcome::already_applied &&
				receipt.durable_revision == applied.durable_revision,
			EILSEQ);
		const auto session = mysql_thread_id(connection);
		transaction selection{ connection, session, true };
		execute(connection, "START TRANSACTION");
		if (stored.phase == 1)
			select_staged_epoch(connection, request, expected_request_hash,
					    expected_baseline_id);
		else
			require(stored.phase == 2 && stored.baseline_operation &&
					stored.baseline_operation->bytes ==
						expected_baseline_id.bytes &&
					stored.selected_epoch &&
					stored.selected_epoch->bytes == request.epoch.bytes &&
					stored.revision == 1,
				EILSEQ);
		check_active_epoch_null(connection, request.lineage);
		if (stored.phase == 1)
			verify_staged_epoch(connection, request, expected_baseline_id,
					    receipt.durable_revision);
		execute(connection, "COMMIT");
		selection.started = false;
		fill_export(request, holdings, mapping_ids, stored, receipt.durable_revision,
			    output);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
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
#endif
