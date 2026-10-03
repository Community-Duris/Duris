#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "economy/economic_baseline_command.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/economic_sql_source_normalize.h"
#include "persistence/economic_sql_baseline_transaction.h"
#include "persistence/economic_accounting_repository.h"
#include "world/vnum.obj.h"
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
unsigned int economic_sql_accounting_lifecycle_transaction::activate(
	MYSQL *, economic_sql_cutover_transaction_owner &, const critical_operation_id &,
	uint64_t *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::activate_verified(
	MYSQL *, economic_sql_cutover_transaction_owner &, const economic_sql_lifecycle_request &,
	const economic_sql_activation_evidence &, economic_sql_activation_verifier) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::pause(
	MYSQL *, economic_sql_cutover_transaction_owner &, const critical_operation_id &) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::recover_runtime(
	MYSQL *, const economic_sql_lifecycle_guard &, bool *) noexcept
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
struct holding_source
{
	economic_account_kind account_kind = economic_account_kind::wallet;
	uint64_t native_id = 0;
	uint8_t racewar = 0;
	uint32_t shop_id = 0;
	std::string name;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	economic_sql_source_digest digest = {};
};
uint16_t native_locator(economic_account_kind kind)
{
	switch (kind)
	{
	case economic_account_kind::wallet:
		return 1;
	case economic_account_kind::bank:
		return 2;
	case economic_account_kind::treasury:
		return 6;
	default:
		throw failure{ EINVAL };
	}
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
economic_sql_source_digest native_digest(const economic_sql_source_snapshot &snapshot,
					 const std::vector<holding_source> &holdings)
{
	const auto wallet = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					 [&](const auto &table)
					 { return table.name == "player_data"; });
	const auto bank = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
				       [&](const auto &table)
				       { return table.name == "account_banks"; });
	const auto shops = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[&](const auto &table)
					{ return table.name == "shopkeepers"; });
	require(wallet != snapshot.tables.end() && bank != snapshot.tables.end() &&
		shops != snapshot.tables.end());
	std::vector<uint8_t> data{ 'E', 'S', 'N', '1' };
	frame(data, wallet->content_digest);
	frame(data, bank->content_digest);
	std::vector<const holding_source *> piles;
	for (const auto &holding : holdings)
		if (holding.account_kind == economic_account_kind::pile)
			piles.push_back(&holding);
	if (!piles.empty())
	{
		auto pile_data = std::vector<uint8_t>{ 'E', 'S', 'P', '1' };
		number(pile_data, piles.size());
		for (const auto *pile : piles)
		{
			number(pile_data, pile->native_id);
			number(pile_data, pile->native_revision);
			for (const auto amount : pile->balance)
				number(pile_data, static_cast<uint64_t>(amount));
			frame(pile_data, pile->digest);
		}
		data[3] = '2';
		frame(data, sha(pile_data));
	}
	if (!shops->rows.empty())
	{
		data[3] = '3';
		frame(data, shops->content_digest);
	}
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
std::vector<holding_source> read_native_holdings(const economic_sql_source_snapshot &snapshot,
						 const economic_sql_normalized_sources &normalized)
{
	const auto wallets = table_index(snapshot, "player_data");
	const auto banks = table_index(snapshot, "account_banks");
	const auto shops = table_index(snapshot, "shopkeepers");
	const auto items = table_index(snapshot, "item_current_owner");
	const auto shop_items = std::find_if(snapshot.item_sources.begin(),
					     snapshot.item_sources.end(), [](const auto &source)
					     { return source.name == "shopkeeper_items"; });
	require(shop_items != snapshot.item_sources.end());
	for (const auto &source : shop_items->rows)
		require(source.cells.size() == 6 && source.cells[5], EBUSY);
	std::vector<holding_source> output;
	std::set<std::pair<std::string, uint8_t>> bank_names;
	uint64_t active_coin_rows = 0, unresolved_coin_rows = 0;
	for (const auto &source : snapshot.tables[items].rows)
	{
		require(source.cells.size() == 10);
		if (source.cells[7] && integer<int32_t>(source.cells[7]) == VOBJ_COINS)
		{
			const auto state = integer<uint8_t>(source.cells[8]);
			if (state == static_cast<uint8_t>(item_custody_state::active))
				++active_coin_rows;
			else if (state != static_cast<uint8_t>(item_custody_state::destroyed))
				++unresolved_coin_rows;
		}
	}
	require(unresolved_coin_rows == 0, EBUSY);
	uint64_t selected_coin_rows = 0;
	for (const auto &holding : normalized.holdings)
	{
		if (holding.kind != economic_sql_holding_kind::wallet &&
		    holding.kind != economic_sql_holding_kind::bank &&
		    holding.kind != economic_sql_holding_kind::treasury &&
		    holding.kind != economic_sql_holding_kind::pile)
			continue;
		require(holding.disposition == economic_sql_holding_disposition::current &&
			holding.balance && holding.native_revision.has_value() &&
			holding.native_id > 0);
		const auto expected_table =
			holding.kind == economic_sql_holding_kind::wallet   ? wallets :
			holding.kind == economic_sql_holding_kind::bank	    ? banks :
			holding.kind == economic_sql_holding_kind::treasury ? shops :
									      items;
		require(holding.source.row != SIZE_MAX && holding.source.table == expected_table);
		const auto &source = snapshot.tables[holding.source.table].rows[holding.source.row];
		if (holding.kind == economic_sql_holding_kind::treasury)
			require(source.cells.size() == 7 && source.cells[6] &&
					integer<uint8_t>(source.cells[6]) <= 1,
				EBUSY);
		for (auto amount : *holding.balance)
			require(amount >= 0, ERANGE);
		holding_source native;
		native.account_kind = holding.kind == economic_sql_holding_kind::wallet ?
					      economic_account_kind::wallet :
				      holding.kind == economic_sql_holding_kind::bank ?
					      economic_account_kind::bank :
				      holding.kind == economic_sql_holding_kind::treasury ?
					      economic_account_kind::treasury :
					      economic_account_kind::pile;
		native.native_id = holding.native_id;
		native.balance = *holding.balance;
		native.native_revision = *holding.native_revision;
		native.digest = source.digest;
		if (native.account_kind == economic_account_kind::bank)
		{
			require(holding.native_id <= UINT32_MAX && holding.native_context >= 0 &&
					holding.native_context <= 1 && source.cells.size() == 8,
				EINVAL);
			require(source.cells[1] && !source.cells[1]->empty(), EINVAL);
			native.name = *source.cells[1];
			native.racewar = static_cast<uint8_t>(holding.native_context);
			require(bank_names.emplace(native.name, native.racewar).second, EEXIST);
		}
		else if (native.account_kind == economic_account_kind::wallet)
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 9,
				ERANGE);
		}
		else if (native.account_kind == economic_account_kind::treasury)
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 7 &&
					integer<uint64_t>(source.cells[0]) == holding.native_id &&
					integer<uint64_t>(source.cells[1]) <= UINT32_MAX &&
					(*holding.balance)[1] == 0 && (*holding.balance)[2] == 0 &&
					(*holding.balance)[3] == 0,
				ERANGE);
			native.shop_id = integer<uint32_t>(source.cells[1]);
		}
		else
		{
			require(normalized.next_uid && source.cells.size() == 10 &&
					integer<uint64_t>(source.cells[0]) == holding.native_id &&
					holding.native_id < *normalized.next_uid &&
					integer<int32_t>(source.cells[7]) == VOBJ_COINS &&
					integer<uint8_t>(source.cells[8]) ==
						static_cast<uint8_t>(item_custody_state::active),
				EILSEQ);
			++selected_coin_rows;
		}
		output.push_back(std::move(native));
	}
	std::sort(output.begin(), output.end(),
		  [](const auto &left, const auto &right)
		  {
			  if (left.account_kind != right.account_kind)
				  return left.account_kind < right.account_kind;
			  return left.native_id < right.native_id;
		  });
	const auto wallet_rows = snapshot.tables[wallets].rows.size();
	const auto bank_rows = snapshot.tables[banks].rows.size();
	const auto shop_rows = snapshot.tables[shops].rows.size();
	const auto selected_wallets = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::wallet; }));
	const auto selected_banks = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::bank; }));
	const auto selected_shops = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::treasury; }));
	require(selected_wallets == static_cast<uint64_t>(wallet_rows) &&
			selected_banks == static_cast<uint64_t>(bank_rows) &&
			selected_shops == static_cast<uint64_t>(shop_rows) &&
			selected_coin_rows == active_coin_rows,
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
					   const std::vector<uint64_t> &account_ids)
{
	require(holdings.size() == account_ids.size());
	std::vector<uint8_t> data{ 'E', 'S', 'C', '1' };
	number(data, holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		data.push_back(static_cast<uint8_t>(holdings[index].account_kind));
		number(data, holdings[index].native_id);
		number(data, account_ids[index]);
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
economic_sql_source_digest parse_digest(const std::optional<std::string> &value)
{
	const auto text = lower_hex(value);
	require(text.size() == SHA256_DIGEST_LENGTH * 2);
	economic_sql_source_digest output = {};
	for (size_t index = 0; index < output.size(); ++index)
	{
		auto nibble = [](char ch) -> uint8_t
		{
			if (ch >= '0' && ch <= '9')
				return static_cast<uint8_t>(ch - '0');
			if (ch >= 'a' && ch <= 'f')
				return static_cast<uint8_t>(ch - 'a' + 10);
			throw failure{ EILSEQ };
		};
		output[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
						     nibble(text[index * 2 + 1]));
	}
	return output;
}
bool digest_nonzero(const economic_sql_source_digest &value)
{
	return std::any_of(value.begin(), value.end(), [](uint8_t byte) { return byte != 0; });
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
	result.request_hash = parse_digest(record[3]);
	result.capture_hash = parse_digest(record[4]);
	result.native_hash = parse_digest(record[5]);
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
	const auto mapped_count = static_cast<size_t>(
		std::count_if(holdings.begin(), holdings.end(), [](const auto &holding)
			      { return holding.account_kind != economic_account_kind::pile; }));
	if (create)
	{
		std::set<uint64_t> pile_ids;
		for (const auto &holding : holdings)
			if (holding.account_kind == economic_account_kind::pile)
				pile_ids.insert(holding.native_id);
		auto next_mapping_id =
			scalar(connection,
			       "SELECT COALESCE(MAX(mapping_id),0) FROM economic_account_mapping");
		require(next_mapping_id < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
		++next_mapping_id;
		for (const auto &holding : holdings)
		{
			if (holding.account_kind == economic_account_kind::pile)
				continue;
			while (pile_ids.contains(next_mapping_id))
			{
				require(next_mapping_id < std::numeric_limits<uint64_t>::max(),
					EOVERFLOW);
				++next_mapping_id;
			}
			const uint16_t account_kind = static_cast<uint16_t>(holding.account_kind);
			const bool bank = holding.account_kind == economic_account_kind::bank;
			const uint16_t locator = native_locator(holding.account_kind);
			execute(connection,
				"INSERT INTO economic_account_mapping(mapping_id,lineage,account_kind,context_id,backend_kind,"
				"locator_kind,native_id,active_native_id,creating_operation_id,retiring_operation_id,revision) VALUES(" +
					std::to_string(next_mapping_id) + "," +
					id(request.lineage) + "," + std::to_string(account_kind) +
					"," + std::to_string(bank ? holding.racewar : 0) + ",1," +
					std::to_string(locator) + "," +
					std::to_string(holding.native_id) + "," +
					std::to_string(holding.native_id) + "," +
					id(request.operation_id) + ",NULL,0)");
			require(next_mapping_id < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
			++next_mapping_id;
		}
	}
	const auto rows = query(
		connection,
		"SELECT mapping_id,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,HEX(creating_operation_id),HEX(retiring_operation_id),revision "
		"FROM economic_account_mapping WHERE lineage=" +
			id(request.lineage) + " ORDER BY locator_kind,native_id FOR UPDATE",
		10);
	require(rows.size() == mapped_count, EILSEQ);
	std::vector<uint64_t> account_ids(holdings.size());
	size_t mapping_index = 0;
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		if (source.account_kind == economic_account_kind::pile)
		{
			account_ids[index] = source.native_id;
			continue;
		}
		const auto &record = rows[mapping_index++];
		const auto account_kind = integer<uint16_t>(record[1]);
		const auto context = integer<uint64_t>(record[2]);
		const auto locator = integer<uint16_t>(record[4]);
		const bool bank = source.account_kind == economic_account_kind::bank;
		const auto expected_kind = static_cast<uint16_t>(source.account_kind);
		const uint16_t expected_locator = native_locator(source.account_kind);
		require(account_kind == expected_kind && context == (bank ? source.racewar : 0) &&
				integer<uint8_t>(record[3]) == ECONOMIC_MAPPING_BACKEND_SQL &&
				locator == expected_locator &&
				integer<uint64_t>(record[5]) == source.native_id &&
				integer<uint64_t>(record[6]) == source.native_id &&
				parse_id(record[7]).bytes == request.operation_id.bytes &&
				!record[8] && integer<uint64_t>(record[9]) == 0,
			EILSEQ);
		const auto mapping = integer<uint64_t>(record[0]);
		require(mapping > 0, EILSEQ);
		account_ids[index] = mapping;
	}
	return account_ids;
}
economic_baseline_batch make_batch(const economic_sql_lifecycle_request &request,
				   const std::vector<holding_source> &holdings,
				   const std::vector<uint64_t> &account_ids,
				   const economic_sql_source_digest &native_hash)
{
	require(holdings.size() == account_ids.size());
	economic_baseline_batch batch;
	batch.lineage = request.lineage;
	batch.epoch = request.epoch;
	batch.preparation_id = request.operation_id;
	batch.actor_id = request.actor_id;
	batch.batch_index = 0;
	batch.opening_account = { request.lineage, economic_account_kind::opening, 1, 0 };
	batch.boundary_digest = native_hash;
	batch.coverage_digest = coverage_digest(holdings, account_ids);
	batch.holdings.reserve(holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		economic_account_key account{
			request.lineage, source.account_kind, account_ids[index],
			static_cast<uint64_t>(source.account_kind == economic_account_kind::bank ?
						      source.racewar :
						      0)
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
		 const std::vector<uint64_t> &account_ids, const stored_installation &stored,
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
		if (source.account_kind == economic_account_kind::pile)
			continue;
		economic_account_key account{
			request.lineage, source.account_kind, account_ids[index],
			static_cast<uint64_t>(source.account_kind == economic_account_kind::bank ?
						      source.racewar :
						      0)
		};
		if (source.account_kind == economic_account_kind::bank)
			receipt.banks.push_back({ source.name, source.racewar, account });
		else if (source.account_kind == economic_account_kind::treasury)
			receipt.treasuries.push_back({ source.shop_id,
						       static_cast<uint32_t>(source.native_id),
						       account });
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
struct stored_activation
{
	bool exists = false;
	critical_operation_id epoch = {};
	critical_operation_id installation = {};
	critical_operation_id baseline = {};
	economic_sql_source_digest manifest_digest = {};
	economic_sql_source_digest audit_digest = {};
	uint64_t route_count = 0;
	uint64_t verified_route_count = 0;
	uint64_t unclassified_route_count = 0;
	uint64_t state = 0;
	uint64_t revision = 0;
};
stored_activation load_activation(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto rows =
		query(connection,
		      "SELECT HEX(epoch),HEX(installation_operation_id),HEX(baseline_operation_id),"
		      "HEX(manifest_digest),HEX(audit_digest),route_count,"
		      "verified_route_count,unclassified_route_count,state,revision FROM "
		      "economic_sql_global_activation WHERE lineage=" +
			      id(lineage) + " FOR UPDATE",
		      10);
	require(rows.size() <= 1);
	stored_activation output;
	if (rows.empty())
		return output;
	const auto &record = rows.front();
	output.exists = true;
	output.epoch = parse_id(record[0]);
	output.installation = parse_id(record[1]);
	output.baseline = parse_id(record[2]);
	output.manifest_digest = parse_digest(record[3]);
	output.audit_digest = parse_digest(record[4]);
	output.route_count = integer<uint64_t>(record[5]);
	output.verified_route_count = integer<uint64_t>(record[6]);
	output.unclassified_route_count = integer<uint64_t>(record[7]);
	output.state = integer<uint64_t>(record[8]);
	output.revision = integer<uint64_t>(record[9]);
	return output;
}
void verify_baseline_receipt(MYSQL *connection, const stored_installation &stored,
			     std::optional<uint64_t> expected_holdings = std::nullopt)
{
	require(stored.baseline_operation && stored.phase == 2 && stored.revision == 1 &&
			stored.selected_epoch && stored.selected_epoch->bytes == stored.epoch.bytes,
		EILSEQ);
	const auto receipt = one(
		connection,
		"SELECT HEX(c.last_operation_id),c.revision,w.holding_count,o.outcome,"
		"i.status FROM economic_baseline_control c JOIN economic_baseline_witness w "
		"ON w.lineage=c.lineage AND w.epoch=c.epoch AND w.operation_id=c.last_operation_id "
		"JOIN economic_accounting_operation o ON o.operation_id=w.operation_id "
		"JOIN critical_operation_inbox i ON i.operation_id=w.operation_id "
		"WHERE c.lineage=" +
			id(stored.lineage) + " AND c.epoch=" + id(stored.epoch) +
			" LOCK IN SHARE MODE",
		5);
	const auto holding_count = integer<uint64_t>(receipt[2]);
	require(parse_id(receipt[0]).bytes == stored.baseline_operation->bytes &&
			integer<uint64_t>(receipt[1]) == 1 &&
			holding_count >= stored.wallet_count + stored.bank_count &&
			(!expected_holdings || holding_count == *expected_holdings) &&
			integer<uint64_t>(receipt[3]) == 1 && integer<uint64_t>(receipt[4]) == 1,
		EILSEQ);
}
std::pair<economic_sql_source_snapshot, std::vector<holding_source>>
capture_current_holdings(MYSQL *connection, bool reject_defects)
{
	economic_sql_source_snapshot snapshot;
	const auto captured =
		economic_sql_capture_sources_in_transaction(connection, {}, &snapshot);
	require(captured == 0, captured);
	economic_sql_normalized_sources normalized;
	const auto status = economic_sql_normalize_sources(snapshot, 512, &normalized);
	require(status == economic_accounting_error::ok,
		status == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
	if (reject_defects)
		reject_cutover_defects(normalized);
	auto holdings = read_native_holdings(snapshot, normalized);
	return { std::move(snapshot), std::move(holdings) };
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
		// Raw death-conflict archives are never opening holdings. Until a
		// separately audited resolution path exists, every case remains open.
		// Maintenance owns the writer fence also used by the retention writer.
		require(scalar(connection, "SELECT COUNT(*) FROM player_death_conflict_evidence") ==
				0,
			EBUSY);
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
		const auto native_hash = native_digest(snapshot, holdings);
		const auto capture_hash = snapshot.digest;
		const auto wallets = static_cast<uint64_t>(std::count_if(
			holdings.begin(), holdings.end(), [](const auto &value)
			{ return value.account_kind == economic_account_kind::wallet; }));
		const auto banks = static_cast<uint64_t>(std::count_if(
			holdings.begin(), holdings.end(), [](const auto &value)
			{ return value.account_kind == economic_account_kind::bank; }));
		require(wallets + banks <= holdings.size() &&
				holdings.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS,
			E2BIG);
		const auto existing = load_installation(connection, request.lineage);
		std::vector<uint64_t> account_ids;
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
			account_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		else
		{
			verify_request(existing, request, expected_request_hash, native_hash,
				       wallets, banks);
			account_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		const auto stored = load_installation(connection, request.lineage);
		require(stored.exists);
		verify_request(stored, request, expected_request_hash, native_hash, wallets, banks);
		auto batch = make_batch(request, holdings, account_ids, native_hash);
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
		fill_export(request, holdings, account_ids, stored, receipt.durable_revision,
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

unsigned int economic_sql_accounting_lifecycle_transaction::activate_verified(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const economic_sql_lifecycle_request &request,
	const economic_sql_activation_evidence &evidence,
	economic_sql_activation_verifier verify) noexcept
{
	try
	{
		require(connection, EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		require(verify && evidence.route_count &&
				evidence.verified_route_count == evidence.route_count &&
				!evidence.unclassified_route_count &&
				digest_nonzero(evidence.manifest_digest) &&
				digest_nonzero(evidence.audit_digest),
			ENODATA);
		require(!critical_operation_id_is_zero(request.operation_id) &&
				!critical_operation_id_is_zero(request.lineage) &&
				!critical_operation_id_is_zero(request.epoch),
			EINVAL);
		// A committed active decision is an exact retry even after gameplay has
		// changed native holdings. Its durable evidence and pointer must still
		// match; a paused decision must pass fresh capture and verification below.
		const auto existing = load_activation(connection, request.lineage);
		if (existing.exists && existing.state == 1)
		{
			const auto staged = load_installation(connection, request.lineage);
			require(staged.exists && staged.phase == 2 && staged.baseline_operation &&
					staged.operation.bytes == request.operation_id.bytes &&
					staged.epoch.bytes == request.epoch.bytes &&
					staged.request_hash == request_digest(request) &&
					existing.epoch.bytes == request.epoch.bytes &&
					existing.installation.bytes == request.operation_id.bytes &&
					existing.baseline.bytes ==
						staged.baseline_operation->bytes &&
					existing.manifest_digest == evidence.manifest_digest &&
					existing.audit_digest == evidence.audit_digest &&
					existing.route_count == evidence.route_count &&
					existing.verified_route_count ==
						evidence.verified_route_count &&
					existing.unclassified_route_count == 0,
				EEXIST);
			const auto selected = one(
				connection,
				"SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
					id(request.lineage) + " FOR UPDATE",
				1);
			require(selected[0] && parse_id(selected[0]).bytes == request.epoch.bytes,
				EILSEQ);
			return activate(connection, owner, request.lineage);
		}
		auto [snapshot, holdings] = capture_current_holdings(connection, true);
		execute(connection, "SAVEPOINT economic_sql_activation_verifier");
		const auto verified = verify(connection, evidence, snapshot);
		require(!verified, verified);
		execute(connection, "ROLLBACK TO SAVEPOINT economic_sql_activation_verifier");
		execute(connection, "RELEASE SAVEPOINT economic_sql_activation_verifier");
		require(owner.is_valid(), EPERM);
		const auto stored = load_installation(connection, request.lineage);
		require(stored.exists && stored.operation.bytes == request.operation_id.bytes &&
				stored.epoch.bytes == request.epoch.bytes &&
				stored.request_hash == request_digest(request) &&
				stored.wallet_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::wallet;
						})) &&
				stored.bank_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::bank;
						})),
			EILSEQ);
		verify_baseline_receipt(connection, stored, holdings.size());
		(void)create_or_verify_mappings(connection, request, holdings, false);
		const auto activation = load_activation(connection, request.lineage);
		const auto lineage =
			one(connection,
			    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
				    id(request.lineage) + " FOR UPDATE",
			    1);
		if (!activation.exists)
		{
			require(!lineage[0] &&
					stored.native_hash == native_digest(snapshot, holdings),
				ESTALE);
			execute(connection,
				"INSERT INTO economic_sql_global_activation(lineage,epoch,"
				"installation_operation_id,baseline_operation_id,manifest_digest,"
				"audit_digest,route_count,verified_route_count,"
				"unclassified_route_count,state,revision) VALUES(" +
					id(request.lineage) + "," + id(request.epoch) + "," +
					id(request.operation_id) + "," +
					id(*stored.baseline_operation) + "," +
					digest_sql(evidence.manifest_digest) + "," +
					digest_sql(evidence.audit_digest) + "," +
					std::to_string(evidence.route_count) + "," +
					std::to_string(evidence.verified_route_count) + ",0,1,1)");
		}
		else
		{
			require(activation.epoch.bytes == request.epoch.bytes &&
					activation.installation.bytes ==
						request.operation_id.bytes &&
					activation.baseline.bytes ==
						stored.baseline_operation->bytes &&
					activation.manifest_digest == evidence.manifest_digest &&
					activation.audit_digest == evidence.audit_digest &&
					activation.route_count == evidence.route_count &&
					activation.verified_route_count ==
						evidence.verified_route_count &&
					activation.unclassified_route_count == 0,
				EEXIST);
			require((activation.state == 1 && lineage[0] &&
				 parse_id(lineage[0]).bytes == request.epoch.bytes) ||
					(activation.state == 2 && !lineage[0]),
				EILSEQ);
			if (activation.state == 2)
			{
				execute(connection,
					"UPDATE economic_sql_global_activation SET state=1,revision=revision+1 "
					"WHERE lineage=" +
						id(request.lineage) + " AND state=2 AND revision=" +
						std::to_string(activation.revision));
				require(mysql_affected_rows(connection) == 1, ESTALE);
			}
		}
		return activate(connection, owner, request.lineage);
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

unsigned int economic_sql_accounting_lifecycle_transaction::activate(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const critical_operation_id &lineage, uint64_t *new_lineage_revision) noexcept
{
	try
	{
		require(connection, EINVAL);
		require(!critical_operation_id_is_zero(lineage), EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		const auto stored = load_installation(connection, lineage);
		const auto decision = load_activation(connection, lineage);
		require(stored.exists && stored.phase == 2 && stored.revision == 1 &&
				stored.selected_epoch && stored.baseline_operation &&
				stored.selected_epoch->bytes == stored.epoch.bytes &&
				decision.exists && decision.state == 1 && decision.revision > 0 &&
				decision.epoch.bytes == stored.epoch.bytes &&
				decision.installation.bytes == stored.operation.bytes &&
				decision.baseline.bytes == stored.baseline_operation->bytes &&
				decision.route_count > 0 &&
				decision.verified_route_count == decision.route_count &&
				decision.unclassified_route_count == 0 &&
				digest_nonzero(decision.manifest_digest) &&
				digest_nonzero(decision.audit_digest),
			ENODATA);
		verify_baseline_receipt(connection, stored);
		const auto state = one(
			connection,
			"SELECT HEX(active_epoch),revision FROM economic_lineage_state WHERE lineage=" +
				id(lineage) + " FOR UPDATE",
			2);
		auto revision = integer<uint64_t>(state[1]);
		if (state[0])
			require(parse_id(state[0]).bytes == stored.epoch.bytes, EEXIST);
		else
		{
			require(revision < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
			execute(connection, "UPDATE economic_lineage_state SET active_epoch=" +
						    id(stored.epoch) +
						    ",revision=revision+1 WHERE lineage=" +
						    id(lineage) + " AND active_epoch IS NULL");
			require(mysql_affected_rows(connection) == 1, ESTALE);
			++revision;
		}
		if (new_lineage_revision)
			*new_lineage_revision = revision;
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

unsigned int economic_sql_accounting_lifecycle_transaction::pause(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const critical_operation_id &lineage_id) noexcept
{
	try
	{
		require(connection && !critical_operation_id_is_zero(lineage_id), EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		const auto stored = load_installation(connection, lineage_id);
		const auto activation = load_activation(connection, lineage_id);
		require(stored.exists && activation.exists && stored.phase == 2 &&
				activation.epoch.bytes == stored.epoch.bytes &&
				activation.installation.bytes == stored.operation.bytes &&
				stored.baseline_operation &&
				activation.baseline.bytes == stored.baseline_operation->bytes,
			EILSEQ);
		const auto state =
			one(connection,
			    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
				    id(lineage_id) + " FOR UPDATE",
			    1);
		if (activation.state == 1)
		{
			require(state[0] && parse_id(state[0]).bytes == stored.epoch.bytes, EILSEQ);
			execute(connection, "UPDATE economic_lineage_state SET active_epoch=NULL,"
					    "revision=revision+1 WHERE lineage=" +
						    id(lineage_id) +
						    " AND active_epoch=" + id(stored.epoch));
			require(mysql_affected_rows(connection) == 1, ESTALE);
			execute(connection,
				"UPDATE economic_sql_global_activation SET state=2,revision=revision+1 "
				"WHERE lineage=" +
					id(lineage_id) + " AND state=1 AND revision=" +
					std::to_string(activation.revision));
			require(mysql_affected_rows(connection) == 1, ESTALE);
		}
		else
			require(activation.state == 2 && !state[0], EILSEQ);
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
unsigned int economic_sql_accounting_lifecycle_transaction::recover_runtime(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority, bool *active) noexcept
{
	try
	{
		require(connection && active && authority.connection_ == connection &&
				!authority.maintenance_ && authority.is_valid_authority() &&
				!economic_gameplay_authority::active(),
			EPERM);
		*active = false;
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		transaction owner{ connection, mysql_thread_id(connection), true };
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT");
		const auto active_rows =
			query(connection,
			      "SELECT HEX(lineage),HEX(active_epoch) FROM economic_lineage_state "
			      "WHERE active_epoch IS NOT NULL FOR UPDATE",
			      2);
		require(active_rows.size() <= 1, EILSEQ);
		if (active_rows.empty())
		{
			execute(connection, "COMMIT");
			owner.started = false;
			return 0;
		}
		const auto lineage = parse_id(active_rows[0][0]);
		const auto epoch = parse_id(active_rows[0][1]);
		const auto stored = load_installation(connection, lineage);
		const auto activation = load_activation(connection, lineage);
		require(stored.exists && stored.phase == 2 && stored.epoch.bytes == epoch.bytes &&
				activation.exists && activation.state == 1 &&
				activation.epoch.bytes == epoch.bytes &&
				activation.installation.bytes == stored.operation.bytes &&
				stored.baseline_operation &&
				activation.baseline.bytes == stored.baseline_operation->bytes &&
				activation.route_count > 0 &&
				activation.verified_route_count == activation.route_count &&
				activation.unclassified_route_count == 0 &&
				digest_nonzero(activation.manifest_digest) &&
				digest_nonzero(activation.audit_digest),
			EILSEQ);
		verify_baseline_receipt(connection, stored);
		auto [snapshot, holdings] = capture_current_holdings(connection, false);
		(void)snapshot;
		require(stored.wallet_count == static_cast<uint64_t>(std::count_if(
						       holdings.begin(), holdings.end(),
						       [](const auto &value) {
							       return value.account_kind ==
								      economic_account_kind::wallet;
						       })) &&
				stored.bank_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::bank;
						})),
			EILSEQ);
		economic_sql_lifecycle_request request;
		request.operation_id = stored.operation;
		request.lineage = lineage;
		request.epoch = epoch;
		const auto account_ids =
			create_or_verify_mappings(connection, request, holdings, false);
		economic_sql_lifecycle_receipt receipt;
		fill_export(request, holdings, account_ids, stored, 1, &receipt);
		require(mysql_thread_id(connection) == owner.session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		execute(connection, "COMMIT");
		owner.started = false;
		std::vector<economic_gameplay_wallet_mapping> wallets;
		std::vector<economic_gameplay_bank_mapping> banks;
		wallets.reserve(receipt.wallets.size());
		banks.reserve(receipt.banks.size());
		for (const auto &wallet : receipt.wallets)
			wallets.push_back({ wallet.pid, wallet.account });
		for (const auto &bank : receipt.banks)
			banks.push_back({ bank.name, bank.racewar, bank.account });
		const auto installed = economic_gameplay_authority::install(
			lineage, epoch, *stored.baseline_operation, wallets, banks);
		require(installed == economic_accounting_error::ok,
			installed == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
		*active = true;
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
