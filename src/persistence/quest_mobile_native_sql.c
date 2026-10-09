#include "persistence/quest_mobile_native_sql.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#ifndef __NO_MYSQL__
namespace
{
using statement_ptr = std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)>;
using mysql_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;

int session_error(MYSQL *connection, unsigned long original = 0) noexcept
{
	if (!connection)
		return EINVAL;
	const auto current = mysql_thread_id(connection);
	if (!current || (original && current != original))
		return ESTALE;
	if (!(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EBUSY;
	mysql_flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EINVAL;
	return 0;
}
int statement_error(MYSQL_STMT *statement) noexcept
{
	const unsigned int error = mysql_stmt_errno(statement);
	return error ? static_cast<int>(error) : EIO;
}
statement_ptr prepare(MYSQL *connection, const char *sql, int *error)
{
	statement_ptr statement(mysql_stmt_init(connection), mysql_stmt_close);
	if (!statement)
	{
		const unsigned int code = mysql_errno(connection);
		*error = code ? static_cast<int>(code) : ENOMEM;
	}
	else if (mysql_stmt_prepare(statement.get(), sql, std::strlen(sql)))
	{
		*error = statement_error(statement.get());
		statement.reset();
	}
	return statement;
}
MYSQL_BIND integer(void *value, enum enum_field_types type) noexcept
{
	MYSQL_BIND result = {};
	result.buffer_type = type;
	result.buffer = value;
	result.is_unsigned = true;
	return result;
}
MYSQL_BIND blob(std::vector<uint8_t> &bytes, unsigned long *length) noexcept
{
	MYSQL_BIND result = {};
	result.buffer_type = MYSQL_TYPE_BLOB;
	result.buffer = bytes.data();
	result.buffer_length = bytes.size();
	result.length = length;
	return result;
}
int codec_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return 0;
	if (result == player_snapshot_codec_result::allocation_failure)
		return ENOMEM;
	if (result == player_snapshot_codec_result::limit_exceeded)
		return E2BIG;
	return EBADMSG;
}
bool nonzero(const critical_operation_id &id) noexcept
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t byte) { return byte != 0; });
}
int read_locked(MYSQL *connection, uint64_t id, unsigned long original,
		quest_mobile_native_sql_row *output)
{
	static const char SQL[] =
		"SELECT mobile_instance_id,mobile_revision,stock_revision,lifetime_state,"
		"OCTET_LENGTH(canonical_image),SUBSTRING(canonical_image,1,?) "
		"FROM quest_mobile_native WHERE mobile_instance_id=? LIMIT 2 FOR UPDATE";
	int error = session_error(connection, original);
	if (error)
		return error;
	auto statement = prepare(connection, SQL, &error);
	if (!statement)
		return error;
	uint64_t bound = PLAYER_SNAPSHOT_MAX_BYTES + 1;
	MYSQL_BIND parameters[2] = { integer(&bound, MYSQL_TYPE_LONGLONG),
				     integer(&id, MYSQL_TYPE_LONGLONG) };
	if (mysql_stmt_bind_param(statement.get(), parameters) ||
	    mysql_stmt_execute(statement.get()) || mysql_stmt_store_result(statement.get()))
		return statement_error(statement.get());
	if (mysql_stmt_num_rows(statement.get()) > 1)
		return EBADMSG;
	quest_mobile_native_sql_row candidate;
	candidate.mobile_instance_id = id;
	candidate.original_session = original;
	if (!mysql_stmt_num_rows(statement.get()))
	{
		statement.reset();
		error = session_error(connection, original);
		if (error)
			return error;
		*output = std::move(candidate);
		return 0;
	}
	uint64_t stored_id = 0, mobile_revision = 0, stock_revision = 0, stored_length = 0;
	uint8_t state = 0;
	unsigned long lengths[6] = {};
	mysql_flag nulls[6] = {}, truncated[6] = {};
	MYSQL_BIND columns[6] = { integer(&stored_id, MYSQL_TYPE_LONGLONG),
				  integer(&mobile_revision, MYSQL_TYPE_LONGLONG),
				  integer(&stock_revision, MYSQL_TYPE_LONGLONG),
				  integer(&state, MYSQL_TYPE_TINY),
				  integer(&stored_length, MYSQL_TYPE_LONGLONG),
				  {} };
	columns[5].buffer_type =
		MYSQL_TYPE_BLOB; // Probe length, never allocate a MEDIUMBLOB-sized blind buffer.
	for (size_t index = 0; index < 6; ++index)
	{
		columns[index].length = &lengths[index];
		columns[index].is_null = &nulls[index];
		columns[index].error = &truncated[index];
	}
	if (mysql_stmt_bind_result(statement.get(), columns))
		return statement_error(statement.get());
	const int fetched = mysql_stmt_fetch(statement.get());
	if (fetched != 0 && fetched != MYSQL_DATA_TRUNCATED)
		return statement_error(statement.get());
	for (size_t index = 0; index < 6; ++index)
		if (nulls[index] || (index < 5 && truncated[index]))
			return EBADMSG;
	if (stored_id != id || !mobile_revision || !stock_revision || stored_length != lengths[5] ||
	    stored_length < QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD ||
	    stored_length > PLAYER_SNAPSHOT_MAX_BYTES)
		return EBADMSG;
	std::vector<uint8_t> bytes(lengths[5]);
	MYSQL_BIND payload = blob(bytes, &lengths[5]);
	mysql_flag payload_null = false, payload_truncated = false;
	payload.is_null = &payload_null;
	payload.error = &payload_truncated;
	if (mysql_stmt_fetch_column(statement.get(), &payload, 5, 0))
		return statement_error(statement.get());
	if (payload_null || payload_truncated || lengths[5] != bytes.size())
		return EBADMSG;
	error = codec_error(quest_mobile_native_image_decode(bytes, &candidate.image));
	if (error)
		return error;
	std::vector<uint8_t> canonical;
	error = codec_error(quest_mobile_native_image_encode(candidate.image, &canonical));
	if (error)
		return error;
	if (canonical != bytes || candidate.image.reference.mobile_instance_id != stored_id ||
	    candidate.image.reference.mobile_revision != mobile_revision ||
	    candidate.image.reference.stock_revision != stock_revision ||
	    static_cast<uint8_t>(candidate.image.state) != state)
		return EBADMSG;
	candidate.present = true;
	statement.reset();
	error = session_error(connection, original);
	if (error)
		return error;
	*output = std::move(candidate);
	return 0;
}
int stable_birth(const quest_mobile_native_reference &before,
		 const quest_mobile_native_reference &after)
{
	auto normalized = after;
	normalized.mobile_revision = before.mobile_revision;
	normalized.stock_revision = before.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> a = {}, b = {};
	int error = codec_error(quest_mobile_native_reference_encode(before, &a));
	if (!error)
		error = codec_error(quest_mobile_native_reference_encode(normalized, &b));
	return error ? error : a == b ? 0 : ESTALE;
}
}
#endif

int quest_mobile_native_sql_lock(MYSQL *connection, uint64_t id,
				 quest_mobile_native_sql_row *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)id;
	(void)output;
	return ENOTSUP;
#else
	if (!output || !id || id == UINT64_MAX)
		return EINVAL;
	try
	{
		int error = session_error(connection);
		if (error)
			return error;
		return read_locked(connection, id, mysql_thread_id(connection), output);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

int quest_mobile_native_sql_apply_locked(MYSQL *connection, const critical_operation_id &parent,
					 const quest_mobile_native_sql_row &before,
					 const quest_mobile_native_image &after,
					 quest_mobile_native_sql_row *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)parent;
	(void)before;
	(void)after;
	(void)output;
	return ENOTSUP;
#else
	if (!output || !before.mobile_instance_id || before.mobile_instance_id == UINT64_MAX ||
	    !before.original_session || !nonzero(parent) ||
	    after.reference.mobile_instance_id != before.mobile_instance_id ||
	    after.last_transition_operation.bytes != parent.bytes ||
	    !quest_mobile_native_cash_transition_valid(before.present ? &before.image : nullptr,
						       after))
		return EINVAL;
	try
	{
		int error = session_error(connection, before.original_session);
		if (error)
			return error;
		std::vector<uint8_t> after_bytes, before_bytes;
		error = codec_error(quest_mobile_native_image_encode(after, &after_bytes));
		if (error)
			return error;
		if (before.present)
		{
			if (before.image.reference.mobile_instance_id !=
				    before.mobile_instance_id ||
			    (before.image.state == quest_mobile_lifetime_state::retired &&
			     after.state != quest_mobile_lifetime_state::retired))
				return ESTALE;
			error = stable_birth(before.image.reference, after.reference);
			if (!error)
				error = codec_error(quest_mobile_native_image_encode(
					before.image, &before_bytes));
			if (error)
				return error;
		}
		else if (after.reference.birth_operation.bytes != parent.bytes)
			return EINVAL;
		quest_mobile_native_sql_row current;
		error = read_locked(connection, before.mobile_instance_id, before.original_session,
				    &current);
		if (error)
			return error;
		if (current.present != before.present)
			return ESTALE;
		if (current.present)
		{
			std::vector<uint8_t> actual;
			error = codec_error(
				quest_mobile_native_image_encode(current.image, &actual));
			if (error)
				return error;
			if (actual != before_bytes)
				return ESTALE;
		}
		static const char INSERT[] =
			"INSERT INTO quest_mobile_native(mobile_instance_id,mobile_revision,stock_revision,"
			"lifetime_state,canonical_image) VALUES(?,?,?,?,?)";
		static const char UPDATE[] =
			"UPDATE quest_mobile_native SET mobile_revision=?,stock_revision=?,lifetime_state=?,canonical_image=? "
			"WHERE mobile_instance_id=? AND mobile_revision=? AND stock_revision=? AND lifetime_state=? AND canonical_image=?";
		auto statement = prepare(connection, current.present ? UPDATE : INSERT, &error);
		if (!statement)
			return error;
		uint64_t id = before.mobile_instance_id,
			 mobile_revision = after.reference.mobile_revision,
			 stock_revision = after.reference.stock_revision;
		uint8_t state = static_cast<uint8_t>(after.state),
			old_state = static_cast<uint8_t>(before.image.state);
		uint64_t old_mobile = before.image.reference.mobile_revision,
			 old_stock = before.image.reference.stock_revision;
		unsigned long after_length = after_bytes.size(),
			      before_length = before_bytes.size();
		MYSQL_BIND parameters[9] = {};
		if (current.present)
		{
			parameters[0] = integer(&mobile_revision, MYSQL_TYPE_LONGLONG);
			parameters[1] = integer(&stock_revision, MYSQL_TYPE_LONGLONG);
			parameters[2] = integer(&state, MYSQL_TYPE_TINY);
			parameters[3] = blob(after_bytes, &after_length);
			parameters[4] = integer(&id, MYSQL_TYPE_LONGLONG);
			parameters[5] = integer(&old_mobile, MYSQL_TYPE_LONGLONG);
			parameters[6] = integer(&old_stock, MYSQL_TYPE_LONGLONG);
			parameters[7] = integer(&old_state, MYSQL_TYPE_TINY);
			parameters[8] = blob(before_bytes, &before_length);
		}
		else
		{
			parameters[0] = integer(&id, MYSQL_TYPE_LONGLONG);
			parameters[1] = integer(&mobile_revision, MYSQL_TYPE_LONGLONG);
			parameters[2] = integer(&stock_revision, MYSQL_TYPE_LONGLONG);
			parameters[3] = integer(&state, MYSQL_TYPE_TINY);
			parameters[4] = blob(after_bytes, &after_length);
		}
		if (mysql_stmt_bind_param(statement.get(), parameters) ||
		    mysql_stmt_execute(statement.get()))
			return statement_error(statement.get());
		const auto affected = mysql_stmt_affected_rows(statement.get());
		if (affected > 1 || (!current.present && affected != 1))
			return EIO;
		statement.reset();
		quest_mobile_native_sql_row verified;
		error = read_locked(connection, id, before.original_session, &verified);
		if (error)
			return error;
		if (!verified.present)
			return ESTALE;
		std::vector<uint8_t> actual;
		error = codec_error(quest_mobile_native_image_encode(verified.image, &actual));
		if (error)
			return error;
		if (actual != after_bytes)
			return ESTALE;
		error = session_error(connection, before.original_session);
		if (error)
			return error;
		*output = std::move(verified);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

#include "item/item_transfer_command.h"
#include <charconv>
#include <map>
#include <string_view>
#ifndef __NO_MYSQL__
namespace
{
struct mobile_catalog_failure
{
	int code;
};
void mobile_catalog_require(bool condition, int code = EBADMSG)
{
	if (!condition)
		throw mobile_catalog_failure{ code };
}
template <class T> std::optional<T> mobile_catalog_number(const std::optional<std::string> &cell)
{
	if (!cell)
		return {};
	T value{};
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	if (parsed.ec != std::errc{} || parsed.ptr != cell->data() + cell->size())
		return {};
	std::array<char, 32> canonical{};
	const auto encoded =
		std::to_chars(canonical.data(), canonical.data() + canonical.size(), value);
	if (encoded.ec != std::errc{} ||
	    std::string_view(canonical.data(), encoded.ptr - canonical.data()) != *cell)
		return {};
	return value;
}
template <class T> bool mobile_catalog_equal(const auto &cells, size_t field, T value)
{
	return mobile_catalog_number<T>(cells[field]) == std::optional<T>(value);
}
using mobile_catalog_result = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
mobile_catalog_result mobile_catalog_query(MYSQL *connection, unsigned long session,
					   const std::string &sql)
{
	const auto before = session_error(connection, session);
	mobile_catalog_require(!before, before);
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw mobile_catalog_failure{ static_cast<int>(
			mysql_errno(connection) ? mysql_errno(connection) : EIO) };
	mobile_catalog_result result(mysql_store_result(connection), mysql_free_result);
	mobile_catalog_require(bool(result),
			       static_cast<int>(mysql_errno(connection) ? mysql_errno(connection) :
									  EIO));
	const auto after = session_error(connection, session);
	mobile_catalog_require(!after, after);
	return result;
}
constexpr const char *mobile_catalog_projection =
	"mobile_instance_id,mobile_revision,stock_revision,lifetime_state,"
	"OCTET_LENGTH(canonical_image),SUBSTRING(canonical_image,1,4194305)";
void mobile_catalog_schema(MYSQL *connection, unsigned long session)
{
	{
		auto mdl = mobile_catalog_query(connection, session,
						std::string("SELECT ") + mobile_catalog_projection +
							" FROM quest_mobile_native LIMIT 0");
		mobile_catalog_require(
			mysql_num_fields(mdl.get()) == 6 && mysql_num_rows(mdl.get()) == 0, EPROTO);
	}
	// Exact immutable0059 verifier shape, read-only under the already-held MDL.
	const char *sql =
		"SELECT ((SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1 "
		"AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='quest_mobile_native')=5 "
		"AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND is_nullable='NO' AND column_default IS NULL AND extra='' AND column_type NOT LIKE '%zerofill%' AND ((ordinal_position=1 AND column_name='mobile_instance_id' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR (ordinal_position=2 AND column_name='mobile_revision' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR (ordinal_position=3 AND column_name='stock_revision' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR (ordinal_position=4 AND column_name='lifetime_state' AND data_type='tinyint' AND column_type LIKE '%unsigned%') OR (ordinal_position=5 AND column_name='canonical_image' AND data_type='mediumblob')))=5 "
		"AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='quest_mobile_native')=1 "
		"AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1 AND column_name='mobile_instance_id' AND sub_part IS NULL AND index_type='BTREE' AND collation='A')=1 "
		"AND (SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE() AND table_name='quest_mobile_native')=1 "
		"AND (SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')=1 "
		"AND (SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='quest_mobile_native')=1 "
		"AND (SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND constraint_name='PRIMARY' AND column_name='mobile_instance_id' AND ordinal_position=1 AND referenced_table_name IS NULL)=1 "
		"AND (SELECT COUNT(*) FROM information_schema.partitions WHERE table_schema=DATABASE() AND table_name='quest_mobile_native' AND partition_name IS NOT NULL)=0)";
	auto shape = mobile_catalog_query(connection, session, sql);
	mobile_catalog_require(
		mysql_num_fields(shape.get()) == 1 && mysql_num_rows(shape.get()) == 1, EPROTO);
	const auto row = mysql_fetch_row(shape.get());
	mobile_catalog_require(row && row[0] && std::string_view(row[0]) == "1", EPROTO);
}
void mobile_catalog_charge(uint64_t amount, uint64_t limit, uint64_t &total)
{
	mobile_catalog_require(total <= limit && amount <= limit - total, E2BIG);
	total += amount;
}
void mobile_catalog_raw(MYSQL *connection, unsigned long session,
			const economic_sql_source_limits &limits,
			quest_mobile_native_sql_catalog &report)
{
	uint64_t expected_rows = 0, expected_bytes = 0;
	{
		const char *sql =
			"SELECT COUNT(*),COALESCE(SUM(COALESCE(OCTET_LENGTH(CAST(mobile_instance_id AS BINARY)),0)+COALESCE(OCTET_LENGTH(CAST(mobile_revision AS BINARY)),0)+COALESCE(OCTET_LENGTH(CAST(stock_revision AS BINARY)),0)+COALESCE(OCTET_LENGTH(CAST(lifetime_state AS BINARY)),0)+COALESCE(OCTET_LENGTH(CAST(OCTET_LENGTH(canonical_image) AS BINARY)),0)+COALESCE(OCTET_LENGTH(canonical_image),0)),0),COALESCE(MAX(OCTET_LENGTH(canonical_image)),0) FROM quest_mobile_native";
		auto preflight = mobile_catalog_query(connection, session, sql);
		mobile_catalog_require(mysql_num_fields(preflight.get()) == 3 &&
				       mysql_num_rows(preflight.get()) == 1);
		const auto row = mysql_fetch_row(preflight.get());
		const auto lengths = mysql_fetch_lengths(preflight.get());
		mobile_catalog_require(row && lengths);
		std::array<uint64_t, 3> values{};
		for (size_t i = 0; i < 3; ++i)
		{
			mobile_catalog_require(row[i] && lengths[i] <= 20, E2BIG);
			auto number = mobile_catalog_number<uint64_t>(
				std::optional<std::string>{ std::string(row[i], lengths[i]) });
			mobile_catalog_require(number.has_value(), E2BIG);
			values[i] = *number;
		}
		expected_rows = values[0];
		expected_bytes = values[1];
		mobile_catalog_require(report.rows <= limits.maximum_rows &&
					       expected_rows <= limits.maximum_rows - report.rows,
				       E2BIG);
		mobile_catalog_require(report.cells <= limits.maximum_cells &&
					       expected_rows <=
						       (limits.maximum_cells - report.cells) / 6,
				       E2BIG);
		mobile_catalog_require(report.cell_bytes <= limits.maximum_cell_bytes &&
					       expected_bytes <= limits.maximum_cell_bytes -
									 report.cell_bytes,
				       E2BIG);
		mobile_catalog_require(values[2] <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
	}
	const auto before_rows = report.rows, before_bytes = report.cell_bytes;
	auto rows = mobile_catalog_query(
		connection, session,
		std::string("SELECT ") + mobile_catalog_projection +
			" FROM quest_mobile_native ORDER BY mobile_instance_id LIMIT " +
			std::to_string(limits.maximum_rows - report.rows + 1));
	mobile_catalog_require(mysql_num_fields(rows.get()) == 6 &&
			       mysql_num_rows(rows.get()) == expected_rows);
	constexpr std::array<unsigned long, 6> bounds = { 20, 20, 20,
							  3,  20, PLAYER_SNAPSHOT_MAX_BYTES + 1 };
	while (auto row = mysql_fetch_row(rows.get()))
	{
		const auto lengths = mysql_fetch_lengths(rows.get());
		mobile_catalog_require(lengths);
		mobile_catalog_charge(1, limits.maximum_rows, report.rows);
		mobile_catalog_charge(6, limits.maximum_cells, report.cells);
		quest_mobile_native_sql_catalog_row retained;
		for (size_t i = 0; i < 6; ++i)
		{
			if (!row[i])
				continue;
			mobile_catalog_require(
				lengths[i] <= bounds[i] &&
					(i == 5 || lengths[i] <= limits.maximum_single_cell_bytes),
				E2BIG);
			mobile_catalog_require(i != 5 || lengths[i] <= PLAYER_SNAPSHOT_MAX_BYTES,
					       E2BIG);
			mobile_catalog_charge(lengths[i], limits.maximum_cell_bytes,
					      report.cell_bytes);
			retained.cells[i] = std::string(row[i], lengths[i]);
		}
		report.catalog.push_back(std::move(retained));
	}
	mobile_catalog_require(!mysql_errno(connection), static_cast<int>(mysql_errno(connection)));
	mobile_catalog_require(report.rows - before_rows == expected_rows &&
			       report.cell_bytes - before_bytes == expected_bytes);
}
struct mobile_catalog_index
{
	size_t row = 0;
	bool ambiguous = false;
};
struct mobile_catalog_inspector
{
	mobile_catalog_inspector(quest_mobile_native_sql_catalog &r,
				 const economic_sql_physical_source_snapshot &b)
		: report(r)
		, base(b)
	{
	}

	quest_mobile_native_sql_catalog &report;
	const economic_sql_physical_source_snapshot &base;
	std::map<uint64_t, mobile_catalog_index> catalog, custody, equipment;
	std::map<std::pair<uint64_t, uint64_t>, mobile_catalog_index> clocks;
	std::map<uint64_t, std::vector<size_t>> live_uids;
	std::vector<uint8_t> matched;
	template <class K>
	void index(std::map<K, mobile_catalog_index> &map, const K &key, size_t row)
	{
		auto [at, added] = map.emplace(key, mobile_catalog_index{ row, false });
		if (!added)
			at->second.ambiguous = true;
	}
	std::optional<size_t> lookup(const auto &map, const auto &key) const
	{
		auto found = map.find(key);
		return found == map.end() || found->second.ambiguous ?
			       std::optional<size_t>{} :
			       std::optional<size_t>{ found->second.row };
	}
	quest_mobile_native_sql_catalog_reference native_ref(size_t row) const
	{
		return { row, base.source2.tables[11].rows[row].digest };
	}
	void finding(uint64_t flag, size_t row = SIZE_MAX, size_t item = SIZE_MAX,
		     std::optional<quest_mobile_native_sql_catalog_reference> source = {})
	{
		quest_mobile_native_sql_catalog_finding value{ flag, row, item, source };
		report.findings.push_back(value);
		if (report.diagnostics.size() < 512)
			report.diagnostics.push_back(value);
		for (size_t bit = 0; bit < report.issue_counts.size(); ++bit)
			if (flag & (uint64_t{ 1 } << bit))
				++report.issue_counts[bit];
	}
	void row_flag(size_t row, uint64_t flag)
	{
		if (!(report.catalog[row].flags & flag))
		{
			report.catalog[row].flags |= flag;
			finding(flag, row);
		}
	}
	void item_flag(size_t item, uint64_t flag)
	{
		auto &witness = report.items[item];
		if (!(witness.flags & flag))
		{
			witness.flags |= flag;
			finding(flag, witness.catalog_row, item, witness.custody);
		}
	}
	void parse_catalog()
	{
		using namespace quest_mobile_native_catalog_flags;
		for (size_t r = 0; r < report.catalog.size(); ++r)
		{
			auto &row = report.catalog[r];
			row.mobile_instance_id = mobile_catalog_number<uint64_t>(row.cells[0]);
			row.mobile_revision = mobile_catalog_number<uint64_t>(row.cells[1]);
			row.stock_revision = mobile_catalog_number<uint64_t>(row.cells[2]);
			row.lifetime_state = mobile_catalog_number<uint8_t>(row.cells[3]);
			const auto length = mobile_catalog_number<uint64_t>(row.cells[4]);
			auto valid_id = [](const auto &id)
			{ return id && *id && *id != UINT64_MAX; };
			if (!valid_id(row.mobile_instance_id) || !row.mobile_revision ||
			    !*row.mobile_revision || !row.stock_revision || !*row.stock_revision ||
			    !row.lifetime_state ||
			    (*row.lifetime_state != 1 && *row.lifetime_state != 2) || !length ||
			    !row.cells[5] || *length != row.cells[5]->size())
				row_flag(r, invalid_row);
			if (valid_id(row.mobile_instance_id))
				index(catalog, *row.mobile_instance_id, r);
			if (!row.cells[5])
			{
				row_flag(r, invalid_image);
				continue;
			}
			quest_mobile_native_image image;
			const auto code = quest_mobile_native_image_decode(
				{ reinterpret_cast<const uint8_t *>(row.cells[5]->data()),
				  row.cells[5]->size() },
				&image);
			if (code == player_snapshot_codec_result::allocation_failure)
				throw mobile_catalog_failure{ ENOMEM };
			if (code == player_snapshot_codec_result::limit_exceeded)
				throw mobile_catalog_failure{ E2BIG };
			if (code != player_snapshot_codec_result::ok)
			{
				row_flag(r, invalid_image);
				continue;
			}
			if (row.mobile_instance_id !=
				    std::optional<uint64_t>(image.reference.mobile_instance_id) ||
			    row.mobile_revision !=
				    std::optional<uint64_t>(image.reference.mobile_revision) ||
			    row.stock_revision !=
				    std::optional<uint64_t>(image.reference.stock_revision) ||
			    row.lifetime_state != std::optional<uint8_t>(uint8_t(image.state)))
				row_flag(r, invalid_row);
			if (!image.cash)
				row_flag(r, cash_unknown);
			row.image = std::move(image);
		}
		for (size_t r = 0; r < report.catalog.size(); ++r)
			if (report.catalog[r].mobile_instance_id &&
			    catalog.contains(*report.catalog[r].mobile_instance_id) &&
			    catalog.at(*report.catalog[r].mobile_instance_id).ambiguous)
				row_flag(r, duplicate_instance);
	}
	void borrowed_indexes()
	{
		for (size_t r = 0; r < base.source2.tables[11].rows.size(); ++r)
		{
			const auto &c = base.source2.tables[11].rows[r].cells;
			if (auto uid = mobile_catalog_number<uint64_t>(c[0]))
				index(custody, *uid, r);
			if (mobile_catalog_equal<uint8_t>(c, 3,
							  uint8_t(item_owner_type::native_mobile)))
				report.native_custody.push_back(native_ref(r));
		}
		matched.resize(base.source2.tables[11].rows.size());
		for (size_t r = 0; r < base.source2.tables[12].rows.size(); ++r)
		{
			const auto &c = base.source2.tables[12].rows[r].cells;
			if (!mobile_catalog_equal<uint8_t>(c, 0,
							   uint8_t(item_owner_type::native_mobile)))
				continue;
			report.native_owner_revisions.push_back(
				{ r, base.source2.tables[12].rows[r].digest });
			auto id = mobile_catalog_number<uint64_t>(c[1]),
			     context = mobile_catalog_number<uint64_t>(c[2]);
			if (id && context)
				index(clocks, std::pair{ *id, *context }, r);
		}
		for (size_t r = 0; r < base.source2.item_equipment_sources[0].rows.size(); ++r)
			if (auto uid = mobile_catalog_number<uint64_t>(
				    base.source2.item_equipment_sources[0].rows[r].cells[0]))
				index(equipment, *uid, r);
	}
	void live_items()
	{
		using namespace quest_mobile_native_catalog_flags;
		for (size_t r = 0; r < report.catalog.size(); ++r)
		{
			auto &row = report.catalog[r];
			if (!row.image || row.image->state != quest_mobile_lifetime_state::live)
				continue;
			const auto &image = *row.image;
			auto clock = lookup(clocks, std::pair{ image.reference.mobile_instance_id,
							       uint64_t{ 0 } });
			if (!clock)
				row_flag(r, owner_clock_missing);
			else
			{
				row.owner_revision = quest_mobile_native_sql_catalog_reference{
					*clock, base.source2.tables[12].rows[*clock].digest
				};
				const auto revision = mobile_catalog_number<uint64_t>(
					base.source2.tables[12].rows[*clock].cells[3]);
				if (!revision || !*revision ||
				    *revision != image.reference.stock_revision)
					row_flag(r, owner_clock_mismatch);
			}
			std::vector<uint64_t> roots;
			roots.reserve(image.items.size());
			for (size_t n = 0; n < image.items.size(); ++n)
			{
				const auto &literal = image.items[n];
				quest_mobile_native_sql_catalog_item witness;
				witness.catalog_row = r;
				witness.image_item = n;
				witness.owner_revision = row.owner_revision;
				if (literal.parent_index < 0)
					witness.root_item_uid = literal.object_uid;
				else
				{
					const auto parent =
						static_cast<size_t>(literal.parent_index);
					mobile_catalog_require(parent < n);
					witness.root_item_uid = roots[parent];
					witness.parent_item_uid = image.items[parent].object_uid;
				}
				roots.push_back(witness.root_item_uid);
				live_uids[literal.object_uid].push_back(report.items.size());
				report.items.push_back(std::move(witness));
			}
		}
		for (const auto &[uid, items] : live_uids)
		{
			(void)uid;
			if (items.size() > 1)
				for (size_t item : items)
					item_flag(item, duplicate_item_uid);
		}
	}
	void related_rows()
	{
		using namespace quest_mobile_native_catalog_flags;
		std::vector<uint64_t> affected_relations;
		for (size_t r = 0; r < base.source2.tables[11].rows.size(); ++r)
		{
			const auto &c = base.source2.tables[11].rows[r].cells;
			auto uid = mobile_catalog_number<uint64_t>(c[0]),
			     root = mobile_catalog_number<uint64_t>(c[1]),
			     parent = mobile_catalog_number<uint64_t>(c[2]);
			const bool related = (uid && live_uids.contains(*uid)) ||
					     (root && live_uids.contains(*root)) ||
					     (parent && live_uids.contains(*parent));
			const bool native = mobile_catalog_equal<uint8_t>(
				c, 3, uint8_t(item_owner_type::native_mobile));
			if (related)
				report.related_custody.push_back(native_ref(r));
			if (!related && !native)
				continue;
			bool valid = mobile_catalog_number<int32_t>(c[7]).has_value();
			for (size_t f = 0; f < 9; ++f)
				if (f != 7)
					valid = valid &&
						((f == 2 && !c[f]) ||
						 mobile_catalog_number<uint64_t>(c[f]).has_value());
			const auto state = mobile_catalog_number<uint8_t>(c[8]);
			valid = valid && state && *state >= uint8_t(item_custody_state::active) &&
				*state <= uint8_t(item_custody_state::quarantined);
			if (!valid)
			{
				report.malformed_custody.push_back(native_ref(r));
				finding(malformed_custody, SIZE_MAX, SIZE_MAX, native_ref(r));
			}
			const bool history = state &&
					     (*state == uint8_t(item_custody_state::destroyed) ||
					      *state == uint8_t(item_custody_state::quarantined));
			if (related && !history && (!uid || !live_uids.contains(*uid)))
			{
				report.extra_custody.push_back(native_ref(r));
				finding(extra_related_custody, SIZE_MAX, SIZE_MAX, native_ref(r));
				for (const auto &relation : { root, parent })
					if (relation && live_uids.contains(*relation))
						affected_relations.push_back(*relation);
			}
		}
		std::sort(affected_relations.begin(), affected_relations.end());
		affected_relations.erase(std::unique(affected_relations.begin(),
						     affected_relations.end()),
					 affected_relations.end());
		for (uint64_t uid : affected_relations)
			for (size_t item : live_uids.at(uid))
				row_flag(report.items[item].catalog_row, extra_related_custody);
		for (size_t r = 0; r < base.source2.item_equipment_sources[0].rows.size(); ++r)
			if (auto uid = mobile_catalog_number<uint64_t>(
				    base.source2.item_equipment_sources[0].rows[r].cells[0]);
			    uid && live_uids.contains(*uid))
				report.related_equipment.push_back(
					{ r,
					  base.source2.item_equipment_sources[0].rows[r].digest });
	}
	void correspondence()
	{
		using namespace quest_mobile_native_catalog_flags;
		for (size_t item = 0; item < report.items.size(); ++item)
		{
			auto &witness = report.items[item];
			const auto &row = report.catalog[witness.catalog_row];
			const auto &image = *row.image;
			const auto &literal = image.items[witness.image_item];
			auto native = lookup(custody, literal.object_uid),
			     gear = lookup(equipment, literal.object_uid);
			if (native)
				witness.custody = native_ref(*native);
			if (gear)
				witness.equipment = quest_mobile_native_sql_catalog_reference{
					*gear,
					base.source2.item_equipment_sources[0].rows[*gear].digest
				};
			bool match = !(row.flags & ~cash_unknown) && !witness.flags && native &&
				     gear;
			if (native)
			{
				const auto &c = base.source2.tables[11].rows[*native].cells;
				witness.observed_item_revision =
					mobile_catalog_number<uint64_t>(c[6]);
				match = match &&
					mobile_catalog_equal<uint64_t>(c, 1,
								       witness.root_item_uid) &&
					(witness.parent_item_uid ?
						 mobile_catalog_equal<uint64_t>(
							 c, 2, *witness.parent_item_uid) :
						 !c[2]) &&
					mobile_catalog_equal<uint8_t>(
						c, 3, uint8_t(item_owner_type::native_mobile)) &&
					mobile_catalog_equal<uint64_t>(
						c, 4, image.reference.mobile_instance_id) &&
					mobile_catalog_equal<uint64_t>(c, 5, 0) &&
					witness.observed_item_revision &&
					*witness.observed_item_revision &&
					*witness.observed_item_revision != UINT64_MAX &&
					mobile_catalog_equal<int32_t>(c, 7, literal.vnum) &&
					mobile_catalog_equal<uint8_t>(c, 8, 1) && !c[9];
			}
			if (gear)
				match = match && mobile_catalog_equal<int16_t>(
							 base.source2.item_equipment_sources[0]
								 .rows[*gear]
								 .cells,
							 1, literal.equipment_slot);
			witness.current_field_correspondence = match;
			if (match)
				matched[*native] = 1;
			else
				item_flag(item, custody_mismatch);
		}
		for (const auto &source : report.native_custody)
		{
			const auto &c = base.source2.tables[11].rows[source.row].cells;
			if (!mobile_catalog_equal<uint8_t>(c, 8, 1) || matched[source.row])
				continue;
			report.unmatched_active_custody.push_back(source);
			finding(unmatched_active_custody, SIZE_MAX, SIZE_MAX, source);
			auto id = mobile_catalog_number<uint64_t>(c[4]),
			     context = mobile_catalog_number<uint64_t>(c[5]);
			auto owner = id ? lookup(catalog, *id) : std::optional<size_t>{};
			if (!owner || !context || *context)
				finding(absent_catalog_owner, SIZE_MAX, SIZE_MAX, source);
			else if (report.catalog[*owner].image &&
				 report.catalog[*owner].image->state ==
					 quest_mobile_lifetime_state::retired)
				finding(retired_catalog_owner, *owner, SIZE_MAX, source);
		}
		report.diagnostics_truncated = report.findings.size() > report.diagnostics.size();
	}
	void inspect()
	{
		parse_catalog();
		borrowed_indexes();
		live_items();
		related_rows();
		correspondence();
	}
};
}
#endif
int quest_mobile_native_sql_capture_catalog_in_transaction(
	MYSQL *connection, const economic_sql_physical_source_snapshot &base,
	const sql_room_item_source_snapshot &supplement, const economic_sql_source_limits &limits,
	quest_mobile_native_sql_catalog *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)base;
	(void)supplement;
	(void)limits;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		mobile_catalog_require(output, EINVAL);
		const auto error = session_error(connection);
		mobile_catalog_require(!error, error);
		const auto session = mysql_thread_id(connection);
		mobile_catalog_require(supplement.physical_digest == base.digest);
		{
			sql_room_item_source_evidence validated;
			const auto code = sql_room_item_payload_inspect_sources(
				base, supplement.tables, limits, 512, &validated);
			mobile_catalog_require(!code, static_cast<int>(code));
		}
		quest_mobile_native_sql_catalog result;
		result.original_session = session;
		result.physical_digest = base.digest;
		result.rows = base.rows;
		result.cells = base.cells;
		result.cell_bytes = base.cell_bytes;
		for (size_t i = 0; i < supplement.tables.size(); ++i)
		{
			mobile_catalog_require(i < result.validated_room_table_digests.size());
			const auto &table = supplement.tables[i];
			result.validated_room_table_digests[i] = table.content_digest;
			for (const auto &row : table.rows)
			{
				mobile_catalog_charge(1, limits.maximum_rows, result.rows);
				mobile_catalog_charge(row.cells.size(), limits.maximum_cells,
						      result.cells);
				for (const auto &cell : row.cells)
					if (cell)
						mobile_catalog_charge(cell->size(),
								      limits.maximum_cell_bytes,
								      result.cell_bytes);
			}
		}
		mobile_catalog_require(result.rows == supplement.rows &&
				       result.cells == supplement.cells &&
				       result.cell_bytes == supplement.cell_bytes);
		mobile_catalog_schema(connection, session);
		mobile_catalog_raw(connection, session, limits, result);
		mobile_catalog_inspector{ result, base }.inspect();
		const auto final_error = session_error(connection, session);
		mobile_catalog_require(!final_error, final_error);
		*output = std::move(result);
		return 0;
	}
	catch (const mobile_catalog_failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}
