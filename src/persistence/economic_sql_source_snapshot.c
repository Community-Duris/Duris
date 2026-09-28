#include "persistence/economic_sql_source_snapshot.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <new>
#include <openssl/evp.h>
#include <span>
#include <string_view>
#include <type_traits>

namespace
{
struct source
{
	const char *table, *columns, *order;
};
// Selected field order is a versioned capture contract. A computed column is
// named by its expression, not disguised as its underlying untransformed field.
constexpr source sources[] = {
	{ "player_data",
	  "pid,account_name,racewar,copper,silver,gold,platinum,wallet_revision,save_revision",
	  "pid" },
	{ "account_banks",
	  "id,account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision",
	  "id" },
	{ "ships", "id,owner_name,money", "id" },
	{ "auctions",
	  "id,seller_pid,status,winning_bidder_pid,cur_price,buy_price,quantity,auction_revision,custody_state,listing_operation_id,obj_vnum,obj_blob_str",
	  "id" },
	{ "auction_money_pickups", "pid,money,claim_revision", "pid" },
	{ "auction_item_pickups", "id,pid,obj_blob_str,retrieved,quantity", "id" },
	{ "auction_item_custody",
	  "auction_id,slot,item_uid,item_revision,vnum,obj_blob,claim_pid,claim_operation_id,claimed_at IS NOT NULL",
	  "auction_id,slot" },
	{ "collector_catalog_state", "state_id,catalog_revision,next_listing", "state_id" },
	{ "collector_deaths",
	  "death_operation_id,beneficiary_pid,death_time,collection_delay,sale_delay,holding_duration,price_percent,minimum_value,hint_state,hint_revision",
	  "death_operation_id" },
	{ "collector_listings",
	  "listing_id,death_operation_id,beneficiary_pid,item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,record_blob,item_blob",
	  "listing_id" },
	{ "item_current_owner",
	  "item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload",
	  "item_uid" },
	{ "item_owner_revision", "owner_type,owner_id,owner_context_id,revision",
	  "owner_type,owner_id,owner_context_id" },
	{ "item_uid_allocator", "allocator_id,next_uid", "allocator_id" },
	{ "item_ownership_quarantine",
	  "quarantine_id,item_uid,source_table,source_row_id,conflict_code,evidence,repaired_at IS NOT NULL",
	  "quarantine_id" },
	{ "auction_reconciliation_quarantine",
	  "quarantine_id,auction_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL",
	  "quarantine_id" },
	{ "collector_reconciliation_quarantine",
	  "quarantine_id,listing_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL",
	  "quarantine_id" },
	{ "critical_operation_inbox",
	  "operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at IS NOT NULL",
	  "operation_id" },
	{ "critical_outbox",
	  "outbox_id,operation_id,event_index,destination,event_type,payload_version,payload,status,attempt_count,last_error_code,delivered_at IS NOT NULL,dead_lettered_at IS NOT NULL",
	  "outbox_id" },
};
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
class digest
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx{ EVP_MD_CTX_new(),
								     EVP_MD_CTX_free };

    public:
	digest()
	{
		require(bool(ctx), ENOMEM);
		require(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) == 1, EIO);
	}
	void bytes(std::span<const uint8_t> data)
	{
		require(EVP_DigestUpdate(ctx.get(), data.data(), data.size()) == 1, EIO);
	}
	void number(uint64_t n)
	{
		std::array<uint8_t, 8> b = {};
		for (size_t i = 0; i < b.size(); ++i)
			b[i] = static_cast<uint8_t>(n >> (8 * i));
		bytes(b);
	}
	void text(std::string_view value)
	{
		number(value.size());
		bytes({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
	economic_sql_source_digest finish()
	{
		economic_sql_source_digest result = {};
		unsigned int length = 0;
		require(EVP_DigestFinal_ex(ctx.get(), result.data(), &length) == 1 &&
				length == result.size(),
			EIO);
		return result;
	}
};
void add(uint64_t &used, uint64_t extra, uint64_t maximum)
{
	require(used <= maximum && extra <= maximum - used, E2BIG);
	used += extra;
}
std::vector<std::string> columns(std::string_view names)
{
	std::vector<std::string> output;
	while (!names.empty())
	{
		const auto end = names.find(',');
		output.emplace_back(names.substr(0, end));
		if (end == std::string_view::npos)
			break;
		names.remove_prefix(end + 1);
	}
	return output;
}
#ifndef __NO_MYSQL__
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
void active(MYSQL *connection, unsigned long session)
{
	require(mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
}
uint64_t number(const char *data, unsigned long length)
{
	require(data && length);
	uint64_t value = 0;
	auto parsed = std::from_chars(data, data + length, value);
	require(parsed.ec == std::errc{} && parsed.ptr == data + length);
	return value;
}
std::vector<uint64_t> counts(MYSQL *connection, const std::string &sql, size_t fields)
{
	execute(connection, sql);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == fields && mysql_num_rows(result.get()) == 1);
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths);
	std::vector<uint64_t> values;
	for (size_t i = 0; i < fields; ++i)
		values.push_back(number(row[i], lengths[i]));
	return values;
}
void capture(MYSQL *connection, unsigned long session, const source &input,
	     const economic_sql_source_limits &limits, economic_sql_source_snapshot &output)
{
	economic_sql_source_table table;
	table.name = input.table;
	table.columns = columns(input.columns);
	std::string selection, total, largest;
	digest definition;
	definition.text("ESD1");
	definition.text(input.table);
	definition.text(input.order);
	definition.number(table.columns.size());
	for (const auto &column : table.columns)
	{
		definition.text(column);
		const auto expression = "CAST((" + column + ") AS BINARY)";
		const auto length = "COALESCE(OCTET_LENGTH(" + expression + "),0)";
		if (!selection.empty())
		{
			selection += ',';
			total += '+';
			largest += ',';
		}
		selection += expression;
		total += length;
		largest += length;
	}
	table.definition_digest = definition.finish();
	const auto from = " FROM `" + table.name + "`";
	auto size = counts(connection,
			   "SELECT COUNT(*),COALESCE(SUM(" + total + "),0),COALESCE(MAX(GREATEST(" +
				   largest + ")),0)" + from,
			   3);
	active(connection, session);
	add(output.rows, size[0], limits.maximum_rows);
	require(size[0] <= limits.maximum_cells / table.columns.size(), E2BIG);
	add(output.cells, size[0] * table.columns.size(), limits.maximum_cells);
	add(output.cell_bytes, size[1], limits.maximum_cell_bytes);
	require(size[2] <= limits.maximum_single_cell_bytes, E2BIG);
	table.rows.reserve(size[0]);
	digest content;
	content.text("EST1");
	content.bytes(table.definition_digest);
	content.number(size[0]);
	execute(connection, "SELECT " + selection + from + " ORDER BY " + input.order);
	result_ptr result(mysql_use_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == table.columns.size());
	uint64_t read_bytes = 0;
	while (auto row = mysql_fetch_row(result.get()))
	{
		require(table.rows.size() < size[0]);
		auto lengths = mysql_fetch_lengths(result.get());
		require(lengths);
		economic_sql_source_row captured;
		captured.cells.reserve(table.columns.size());
		digest row_hash;
		row_hash.text("ESR1");
		row_hash.bytes(table.definition_digest);
		for (size_t i = 0; i < table.columns.size(); ++i)
		{
			row_hash.number(row[i] ? 1 : 0);
			if (row[i])
			{
				require(lengths[i] <= limits.maximum_single_cell_bytes, E2BIG);
				add(read_bytes, lengths[i], size[1]);
				captured.cells.emplace_back(std::string(row[i], lengths[i]));
				row_hash.text(*captured.cells.back());
			}
			else
				captured.cells.emplace_back(std::nullopt);
		}
		captured.digest = row_hash.finish();
		content.bytes(captured.digest);
		table.rows.push_back(std::move(captured));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	require(table.rows.size() == size[0] && read_bytes == size[1]);
	result.reset();
	active(connection, session);
	table.content_digest = content.finish();
	output.tables.push_back(std::move(table));
}
#endif
}
#ifdef __NO_MYSQL__
unsigned int economic_sql_capture_sources(MYSQL *, const economic_sql_source_limits &,
					  economic_sql_source_snapshot *) noexcept
{
	return ENOTSUP;
}
#else
unsigned int economic_sql_capture_sources(MYSQL *connection,
					  const economic_sql_source_limits &limits,
					  economic_sql_source_snapshot *output) noexcept
{
	bool transaction = false;
	try
	{
		require(connection && output, EINVAL);
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
			EBUSY);
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect,
			EPERM);
		const economic_sql_source_limits hard;
		require(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
				limits.maximum_cells &&
				limits.maximum_cells <= hard.maximum_cells &&
				limits.maximum_cell_bytes &&
				limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
				limits.maximum_single_cell_bytes &&
				limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes,
			EINVAL);
		const auto *version = mysql_get_server_info(connection);
		require(version && ((!std::strncmp(version, "8.0.", 4) &&
				     !std::strstr(version, "MariaDB")) ||
				    (!std::strncmp(version, "10.11.", 6) &&
				     std::strstr(version, "MariaDB"))),
			ENOTSUP);
		const auto session = mysql_thread_id(connection);
		execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		// A lost START acknowledgement may still leave a transaction open.
		transaction = true;
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY");
		active(connection, session);
		std::string names;
		for (const auto &s : sources)
		{
			execute(connection, std::string("SELECT 1 FROM `") + s.table + "` LIMIT 0");
			result_ptr result(mysql_store_result(connection), mysql_free_result);
			require(bool(result),
				mysql_errno(connection) ? mysql_errno(connection) : EIO);
			active(connection, session);
			if (!names.empty())
				names += ',';
			names += "'" + std::string(s.table) + "'";
		}
		auto metadata = counts(
			connection,
			"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND engine='InnoDB' AND table_name IN (" +
				names + ")",
			1);
		require(metadata[0] == std::size(sources), ENOTSUP);
		economic_sql_source_snapshot captured;
		captured.tables.reserve(std::size(sources));
		for (const auto &s : sources)
			capture(connection, session, s, limits, captured);
		digest manifest;
		manifest.text("ESM1");
		manifest.number(captured.tables.size());
		for (const auto &table : captured.tables)
			manifest.bytes(table.content_digest);
		captured.digest = manifest.finish();
		active(connection, session);
		execute(connection, "ROLLBACK");
		require(mysql_thread_id(connection) == session &&
				!(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		transaction = false;
		*output = std::move(captured);
		return 0;
	}
	catch (const failure &e)
	{
		if (transaction)
			mysql_real_query(connection, "ROLLBACK", 8);
		return e.code ? e.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		if (transaction)
			mysql_real_query(connection, "ROLLBACK", 8);
		return ENOMEM;
	}
	catch (...)
	{
		if (transaction)
			mysql_real_query(connection, "ROLLBACK", 8);
		return EIO;
	}
}
#endif

unsigned int economic_sql_validate_sources(const economic_sql_source_snapshot &input,
					   const economic_sql_source_limits &limits) noexcept
{
	try
	{
		const economic_sql_source_limits hard;
		require(input.version == 1 && input.tables.size() == std::size(sources));
		require(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
				limits.maximum_cells &&
				limits.maximum_cells <= hard.maximum_cells &&
				limits.maximum_cell_bytes &&
				limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
				limits.maximum_single_cell_bytes &&
				limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes,
			EINVAL);
		uint64_t row_count = 0, cell_count = 0, byte_count = 0;
		digest manifest;
		manifest.text("ESM1");
		manifest.number(input.tables.size());
		for (size_t index = 0; index < input.tables.size(); ++index)
		{
			const auto &table = input.tables[index];
			const auto &spec = sources[index];
			require(table.name == spec.table && table.columns == columns(spec.columns));
			add(row_count, table.rows.size(), limits.maximum_rows);
			digest definition;
			definition.text("ESD1");
			definition.text(spec.table);
			definition.text(spec.order);
			definition.number(table.columns.size());
			for (const auto &column : table.columns)
				definition.text(column);
			require(definition.finish() == table.definition_digest);
			digest content;
			content.text("EST1");
			content.bytes(table.definition_digest);
			content.number(table.rows.size());
			for (const auto &row : table.rows)
			{
				require(row.cells.size() == table.columns.size());
				add(cell_count, row.cells.size(), limits.maximum_cells);
				digest row_hash;
				row_hash.text("ESR1");
				row_hash.bytes(table.definition_digest);
				for (const auto &cell : row.cells)
				{
					row_hash.number(cell ? 1 : 0);
					if (cell)
					{
						require(cell->size() <=
								limits.maximum_single_cell_bytes,
							E2BIG);
						add(byte_count, cell->size(),
						    limits.maximum_cell_bytes);
						row_hash.text(*cell);
					}
				}
				require(row_hash.finish() == row.digest);
				content.bytes(row.digest);
			}
			require(content.finish() == table.content_digest);
			manifest.bytes(table.content_digest);
		}
		require(manifest.finish() == input.digest && input.rows == row_count &&
			input.cells == cell_count && input.cell_bytes == byte_count);
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
