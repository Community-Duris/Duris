#include "persistence/sql_room_creation_source.h"
#include "persistence/sql_room_item_payload.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <new>
#include <openssl/evp.h>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
namespace
{
struct failure
{
	unsigned int code;
};
void require(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code ? code : EIO };
}
struct source
{
	const char *name, *columns, *order;
};
constexpr source sources[] = {
	{ "zone_reset_item_birth_origin",
	  "root_item_uid,birth_operation,room_revision,canonical_origin,terminal_publication_context",
	  "root_item_uid" },
	{ "economic_accounting_operation",
	  "operation_id,lineage,epoch,original_operation_id,accounting_version,writer_id,policy_version,compiler_version,actor_kind,actor_id,reason,source_event,intent_digest,domain_digest,plan_digest,canonical_intent,canonical_plan,outcome,result_code,realized_price_copper,account_count,posting_count,child_count,item_event_count,before_witness_count,after_witness_count",
	  "operation_id" },
	{ "economic_accounting_account_effect",
	  "operation_id,account_index,account_key,before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,after_gold,after_platinum,before_revision,after_revision",
	  "operation_id,account_index" },
	{ "economic_accounting_coin_posting",
	  "operation_id,line_index,event_index,account_index,child_index,copper_value,delta_copper,delta_silver,delta_gold,delta_platinum",
	  "operation_id,line_index" },
	{ "economic_accounting_source_claim", "lineage,source_event,operation_id,outcome",
	  "lineage,source_event" },
	{ "economic_accounting_child",
	  "operation_id,child_index,child_operation_id,domain_id,discriminator,parent_index,relationship,receipt_operation_id",
	  "operation_id,child_index" },
	{ "economic_lineage_state", "lineage,active_epoch,revision", "lineage" },
	{ "economic_epoch", "lineage,epoch", "lineage,epoch" },
	{ "player_death_restitution_runtime", "item_uid", "item_uid" }
};
class hash
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{ EVP_MD_CTX_new(),
									 EVP_MD_CTX_free };

    public:
	hash()
	{
		require(bool(context), ENOMEM);
		require(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1, EIO);
	}
	void bytes(std::span<const uint8_t> value)
	{
		require(EVP_DigestUpdate(context.get(), value.data(), value.size()) == 1, EIO);
	}
	void number(uint64_t value)
	{
		std::array<uint8_t, 8> encoded{};
		for (size_t i = 0; i < encoded.size(); ++i)
			encoded[i] = static_cast<uint8_t>(value >> (8 * i));
		bytes(encoded);
	}
	void text(std::string_view value)
	{
		number(value.size());
		bytes({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
	economic_sql_source_digest finish()
	{
		economic_sql_source_digest result{};
		unsigned int length = 0;
		require(EVP_DigestFinal_ex(context.get(), result.data(), &length) == 1 &&
				length == result.size(),
			EIO);
		return result;
	}
};
struct budget
{
	uint64_t rows = 0, cells = 0, bytes = 0;
};
void add(uint64_t &used, uint64_t extra, uint64_t maximum)
{
	require(used <= maximum && extra <= maximum - used, E2BIG);
	used += extra;
}
std::vector<std::string> columns(std::string_view input)
{
	std::vector<std::string> result;
	while (!input.empty())
	{
		const size_t end = input.find(',');
		result.emplace_back(input.substr(0, end));
		if (end == input.npos)
			break;
		input.remove_prefix(end + 1);
	}
	return result;
}
economic_sql_source_digest definition(const source &spec)
{
	hash result;
	result.text("ESD1");
	result.text(spec.name);
	result.text(spec.order);
	const auto names = columns(spec.columns);
	result.number(names.size());
	for (const auto &name : names)
		result.text(name);
	return result.finish();
}
economic_sql_source_digest row_digest(const economic_sql_source_table &table,
				      const economic_sql_source_row &row)
{
	hash result;
	result.text("ESR1");
	result.bytes(table.definition_digest);
	for (const auto &cell : row.cells)
	{
		result.number(cell ? 1 : 0);
		if (cell)
			result.text(*cell);
	}
	return result.finish();
}
economic_sql_source_digest binding(const sql_room_creation_source_snapshot &packet)
{
	hash result;
	result.text("RSC2");
	result.number(packet.version);
	result.bytes(packet.physical_digest);
	result.bytes(packet.room_digest);
	result.number(packet.tables.size());
	for (const auto &table : packet.tables)
		result.bytes(table.content_digest);
	result.number(packet.additional_rows);
	result.number(packet.additional_cells);
	result.number(packet.additional_cell_bytes);
	return result.finish();
}
void charge(const std::vector<economic_sql_source_table> &tables,
	    const economic_sql_source_limits &limits, budget &used)
{
	for (const auto &table : tables)
	{
		add(used.rows, table.rows.size(), limits.maximum_rows);
		for (const auto &row : table.rows)
		{
			add(used.cells, row.cells.size(), limits.maximum_cells);
			for (const auto &cell : row.cells)
				if (cell)
				{
					require(cell->size() <= limits.maximum_single_cell_bytes,
						E2BIG);
					add(used.bytes, cell->size(), limits.maximum_cell_bytes);
				}
		}
	}
}
budget legacy(const economic_sql_physical_source_snapshot &base,
	      const sql_room_item_source_snapshot &room, const economic_sql_source_limits &limits)
{
	const economic_sql_source_limits hard;
	require(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
			limits.maximum_cells && limits.maximum_cells <= hard.maximum_cells &&
			limits.maximum_cell_bytes &&
			limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
			limits.maximum_single_cell_bytes &&
			limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes,
		EINVAL);
	require(room.physical_digest == base.digest);
	sql_room_item_source_evidence validated;
	const auto status =
		sql_room_item_payload_inspect_sources(base, room.tables, limits, 512, &validated);
	require(!status, status);
	require(room.tables.size() == 5);
	hash digest;
	digest.text("sql_room_item_payload persisted evidence");
	digest.bytes(base.digest);
	digest.number(room.tables.size());
	for (const auto &table : room.tables)
		digest.bytes(table.content_digest);
	require(room.digest == digest.finish());
	budget used{ base.rows, base.cells, base.cell_bytes };
	charge(room.tables, limits, used);
	require(used.rows == room.rows && used.cells == room.cells &&
		used.bytes == room.cell_bytes);
	return used;
}
void validate(const economic_sql_physical_source_snapshot &base,
	      const sql_room_item_source_snapshot &room,
	      const sql_room_creation_source_snapshot &packet,
	      const economic_sql_source_limits &limits)
{
	auto used = legacy(base, room, limits);
	const auto before = used;
	require(packet.version == 2 && packet.physical_digest == base.digest &&
		packet.room_digest == room.digest && packet.tables.size() == std::size(sources));
	// Check all sizes before hashing arbitrary supplied bodies.
	charge(packet.tables, limits, used);
	for (size_t i = 0; i < packet.tables.size(); ++i)
	{
		const auto &table = packet.tables[i];
		require(table.name == sources[i].name &&
			table.columns == columns(sources[i].columns) &&
			table.definition_digest == definition(sources[i]));
		hash content;
		content.text("EST1");
		content.bytes(table.definition_digest);
		content.number(table.rows.size());
		for (const auto &row : table.rows)
		{
			require(row.cells.size() == table.columns.size());
			require(row.digest == row_digest(table, row));
			content.bytes(row.digest);
		}
		require(table.content_digest == content.finish());
	}
	require(packet.additional_rows == used.rows - before.rows &&
		packet.additional_cells == used.cells - before.cells &&
		packet.additional_cell_bytes == used.bytes - before.bytes &&
		packet.digest == binding(packet));
}
#ifndef __NO_MYSQL__
using rows_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
void active(MYSQL *connection, unsigned long session)
{
	require(connection && session && mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
		ENOTCONN);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect,
		EPERM);
}
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
template <typename T> T integer(const char *text, size_t size)
{
	require(text && size && size <= 20);
	T value = 0;
	const auto parsed = std::from_chars(text, text + size, value);
	require(parsed.ec == std::errc{} && parsed.ptr == text + size);
	return value;
}
std::array<uint64_t, 3> counts(MYSQL *connection, unsigned long session, const std::string &sql)
{
	active(connection, session);
	execute(connection, sql);
	rows_ptr rows(mysql_store_result(connection), mysql_free_result);
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(rows.get()) == 1 && mysql_num_fields(rows.get()) == 3);
	const auto row = mysql_fetch_row(rows.get());
	const auto lengths = mysql_fetch_lengths(rows.get());
	require(row && lengths);
	std::array<uint64_t, 3> result{};
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = integer<uint64_t>(row[i], lengths[i]);
	rows.reset();
	active(connection, session);
	return result;
}
struct field_shape
{
	const char *type = "bigint";
	bool unsigned_ = true, nullable = false;
	uint64_t octets = 0;
};
field_shape shape(size_t table, size_t field)
{
	field_shape value;
	if (table == 0)
	{
		if (field == 1)
			value = { "binary", false, false, 16 };
		else if (field == 3)
			value = { "longblob", false, false, 0 };
		else if (field == 4)
			value = { "longblob", false, true, 0 };
	}
	else if (table == 1)
	{
		if (field <= 3)
			value = { "binary", false, field == 3, 16 };
		else if (field == 4 || field == 10 || field >= 20)
			value.type = "smallint";
		else if (field == 5 || field == 6 || field == 7 || field == 18)
			value.type = "int";
		else if (field == 8 || field == 17)
			value.type = "tinyint";
		else if (field == 11)
			value = { "binary", false, true, 48 };
		else if (field >= 12 && field <= 14)
			value = { "binary", false, field == 14, 32 };
		else if (field == 15)
			value = { "varbinary", false, false, 8192 };
		else if (field == 16)
			value = { "mediumblob", false, true, 0 };
		else if (field == 19)
			value = { "bigint", false, true, 0 };
	}
	else if (table == 2)
	{
		if (field == 0)
			value = { "binary", false, false, 16 };
		else if (field == 1)
			value.type = "smallint";
		else if (field == 2)
			value = { "binary", false, false, 40 };
		else if (field <= 10)
			value.unsigned_ = false;
	}
	else if (table == 3)
	{
		if (field == 0)
			value = { "binary", false, false, 16 };
		else if (field == 1 || field == 3 || field == 4)
			value.type = "smallint";
		else if (field == 2)
			value.type = "int";
		else
			value.unsigned_ = false;
	}
	else if (table == 4)
	{
		if (field == 0 || field == 2)
			value = { "binary", false, false, 16 };
		else if (field == 1)
			value = { "binary", false, false, 48 };
		else
			value.type = "tinyint";
	}
	else if (table == 5)
	{
		if (field == 0 || field == 2 || field == 7)
			value = { "binary", false, field == 7, 16 };
		else if (field == 1 || field == 5 || field == 6)
			value.type = "smallint";
		else if (field == 3)
			value.type = "int";
	}
	else if (table == 6)
	{
		if (field < 2)
			value = { "binary", false, field == 1, 16 };
	}
	else if (table == 7)
		value = { "binary", false, false, 16 };
	// Table8 selects only the existing unsigned BIGINT runtime UID.
	return value;
}
void metadata(MYSQL *connection, unsigned long session, size_t index)
{
	const auto &spec = sources[index];
	const auto names = columns(spec.columns);
	active(connection, session);
	// Acquire MDL before metadata and hold it until the caller ends this cut.
	execute(connection,
		"SELECT " + std::string(spec.columns) + " FROM `" + spec.name + "` LIMIT 0");
	rows_ptr empty(mysql_store_result(connection), mysql_free_result);
	require(bool(empty), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(empty.get()) == names.size() && mysql_num_rows(empty.get()) == 0,
		EPROTONOSUPPORT);
	empty.reset();
	execute(
		connection,
		"SELECT column_name,data_type,column_type,is_nullable,COALESCE(character_octet_length,0) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='" +
			std::string(spec.name) + "' AND column_name IN ('" +
			[&]()
			{
				std::string result;
				for (const auto &name : names)
				{
					if (!result.empty())
						result += "','";
					result += name;
				}
				return result;
			}() +
			"') ORDER BY ordinal_position");
	rows_ptr rows(mysql_store_result(connection), mysql_free_result);
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(rows.get()) == names.size() && mysql_num_fields(rows.get()) == 5,
		EPROTONOSUPPORT);
	std::vector<bool> seen(names.size(), false);
	while (const auto row = mysql_fetch_row(rows.get()))
	{
		const auto lengths = mysql_fetch_lengths(rows.get());
		require(lengths);
		for (size_t i = 0; i < 5; ++i)
			require(row[i] && lengths[i] <= 128, EPROTONOSUPPORT);
		const auto found =
			std::find(names.begin(), names.end(), std::string(row[0], lengths[0]));
		require(found != names.end(), EPROTONOSUPPORT);
		const auto field = static_cast<size_t>(found - names.begin());
		require(!seen[field], EPROTONOSUPPORT);
		seen[field] = true;
		const auto expected = shape(index, field);
		require(std::string_view(row[1], lengths[1]) == expected.type &&
				(std::strstr(row[2], "unsigned") != nullptr) ==
					expected.unsigned_ &&
				std::string_view(row[3], lengths[3]) ==
					(expected.nullable ? "YES" : "NO"),
			EPROTONOSUPPORT);
		if (expected.octets)
			require(integer<uint64_t>(row[4], lengths[4]) == expected.octets,
				EPROTONOSUPPORT);
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	rows.reset();
	execute(connection,
		"SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='" +
			std::string(spec.name) + "' AND index_name='PRIMARY' AND non_unique=0");
	rows.reset(mysql_store_result(connection));
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(rows.get()) == 1 && mysql_num_fields(rows.get()) == 1,
		EPROTONOSUPPORT);
	auto row = mysql_fetch_row(rows.get());
	auto lengths = mysql_fetch_lengths(rows.get());
	require(row && lengths && row[0] && lengths[0] <= 128 &&
			std::string_view(row[0], lengths[0]) == spec.order,
		EPROTONOSUPPORT);
	rows.reset();
	execute(connection,
		"SELECT engine,table_type FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='" +
			std::string(spec.name) + "'");
	rows.reset(mysql_store_result(connection));
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(rows.get()) == 1 && mysql_num_fields(rows.get()) == 2, ENOTSUP);
	row = mysql_fetch_row(rows.get());
	lengths = mysql_fetch_lengths(rows.get());
	require(row && lengths && row[0] && row[1] &&
			std::string_view(row[0], lengths[0]) == "InnoDB" &&
			std::string_view(row[1], lengths[1]) == "BASE TABLE",
		ENOTSUP);
	rows.reset();
	active(connection, session);
}
void capture_table(MYSQL *connection, unsigned long session, const source &spec,
		   const economic_sql_source_limits &limits, budget &used,
		   std::vector<economic_sql_source_table> &destination)
{
	economic_sql_source_table table;
	table.name = spec.name;
	table.columns = columns(spec.columns);
	table.definition_digest = definition(spec);
	std::string selected, total, largest = "0";
	for (const auto &column : table.columns)
	{
		const auto expression = "CAST((" + column + ") AS BINARY)";
		const auto length = "COALESCE(OCTET_LENGTH(" + expression + "),0)";
		if (!selected.empty())
		{
			selected += ',';
			total += '+';
		}
		selected += expression;
		total += length;
		largest += ',';
		largest += length;
	}
	const auto from = " FROM `" + table.name + "`";
	const auto expected =
		counts(connection, session,
		       "SELECT COUNT(*),COALESCE(SUM(" + total + "),0),COALESCE(MAX(GREATEST(" +
			       largest + ")),0)" + from);
	add(used.rows, expected[0], limits.maximum_rows);
	require(expected[0] <= limits.maximum_cells / table.columns.size(), E2BIG);
	add(used.cells, expected[0] * table.columns.size(), limits.maximum_cells);
	add(used.bytes, expected[1], limits.maximum_cell_bytes);
	require(expected[2] <= limits.maximum_single_cell_bytes, E2BIG);
	table.rows.reserve(expected[0]);
	execute(connection, "SELECT " + selected + from + " ORDER BY " + spec.order);
	rows_ptr rows(mysql_use_result(connection), mysql_free_result);
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(rows.get()) == table.columns.size());
	uint64_t bytes = 0;
	while (const auto row = mysql_fetch_row(rows.get()))
	{
		require(table.rows.size() < expected[0]);
		const auto lengths = mysql_fetch_lengths(rows.get());
		require(lengths);
		economic_sql_source_row retained;
		retained.cells.reserve(table.columns.size());
		for (size_t i = 0; i < table.columns.size(); ++i)
		{
			if (row[i])
			{
				require(lengths[i] <= limits.maximum_single_cell_bytes, E2BIG);
				add(bytes, lengths[i], expected[1]);
				retained.cells.emplace_back(std::string(row[i], lengths[i]));
			}
			else
				retained.cells.emplace_back(std::nullopt);
		}
		retained.digest = row_digest(table, retained);
		table.rows.push_back(std::move(retained));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	require(table.rows.size() == expected[0] && bytes == expected[1]);
	rows.reset();
	active(connection, session);
	hash content;
	content.text("EST1");
	content.bytes(table.definition_digest);
	content.number(table.rows.size());
	for (const auto &row : table.rows)
		content.bytes(row.digest);
	table.content_digest = content.finish();
	destination.push_back(std::move(table));
}
#endif
}
unsigned int
sql_room_creation_source_validate_sources(const economic_sql_physical_source_snapshot &base,
					  const sql_room_item_source_snapshot &room,
					  const sql_room_creation_source_snapshot &packet,
					  const economic_sql_source_limits &limits) noexcept
{
	try
	{
		validate(base, room, packet, limits);
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
sql_room_creation_source_capture_in_transaction(MYSQL *connection,
						const economic_sql_source_limits &limits,
						const economic_sql_physical_source_snapshot &base,
						const sql_room_item_source_snapshot &room,
						sql_room_creation_source_snapshot *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)limits;
	(void)base;
	(void)room;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		require(connection && output, EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		auto used = legacy(base, room, limits);
		const auto before = used;
		const char *server = mysql_get_server_info(connection);
		require(server, ENOTSUP);
		const bool maria = std::strstr(server, "MariaDB") != nullptr;
		if (maria && !std::strncmp(server, "5.5.5-", 6))
			server += 6;
		require((maria && !std::strncmp(server, "10.11.", 6)) ||
				(!maria && !std::strncmp(server, "8.0.", 4)),
			ENOTSUP);
		sql_room_creation_source_snapshot packet;
		packet.physical_digest = base.digest;
		packet.room_digest = room.digest;
		for (size_t index = 0; index < std::size(sources); ++index)
			metadata(connection, session, index);
		for (const auto &spec : sources)
			capture_table(connection, session, spec, limits, used, packet.tables);
		packet.additional_rows = used.rows - before.rows;
		packet.additional_cells = used.cells - before.cells;
		packet.additional_cell_bytes = used.bytes - before.bytes;
		packet.digest = binding(packet);
		validate(base, room, packet, limits);
		active(connection, session);
		static_assert(std::is_nothrow_move_assignable_v<sql_room_creation_source_snapshot>);
		*output = std::move(packet);
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
#endif
}
