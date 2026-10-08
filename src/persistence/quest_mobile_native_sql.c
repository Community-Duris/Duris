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
