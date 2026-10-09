#include "persistence/zone_reset_item_origin_sql.h"
#include "persistence/economic_sql_zone_reset_item_transaction.h"

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
struct observed_origin
{
	zone_reset_item_retained_origin value;
	std::vector<uint8_t> canonical;
};

int session_error(MYSQL *connection, unsigned long expected) noexcept
{
	if (!connection)
		return EINVAL;
	if (!expected || mysql_thread_id(connection) != expected ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return ENOTCONN;
	mysql_flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EINVAL;
	return 0;
}
int statement_error(MYSQL_STMT *statement) noexcept
{
	const auto error = mysql_stmt_errno(statement);
	return error ? static_cast<int>(error) : EIO;
}
statement_ptr prepare(MYSQL *connection, const char *sql, int *error)
{
	statement_ptr statement(mysql_stmt_init(connection), mysql_stmt_close);
	if (!statement)
		*error = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						   ENOMEM;
	else if (mysql_stmt_prepare(statement.get(), sql, std::strlen(sql)))
	{
		*error = statement_error(statement.get());
		statement.reset();
	}
	return statement;
}
MYSQL_BIND integer(void *value) noexcept
{
	MYSQL_BIND binding{};
	binding.buffer_type = MYSQL_TYPE_LONGLONG;
	binding.buffer = value;
	binding.is_unsigned = true;
	return binding;
}
MYSQL_BIND binary(const void *value, unsigned long length) noexcept
{
	MYSQL_BIND binding{};
	binding.buffer_type = MYSQL_TYPE_BLOB;
	binding.buffer = const_cast<void *>(value);
	binding.buffer_length = length;
	return binding;
}
int codec_error(economic_accounting_error error) noexcept
{
	return error == economic_accounting_error::ok	    ? 0 :
	       error == economic_accounting_error::capacity ? ENOMEM :
							      EBADMSG;
}
int read(MYSQL *connection, unsigned long session, const critical_operation_id &operation,
	 observed_origin *output, bool locked)
{
	int error = session_error(connection, session);
	if (error)
		return error;
	static const char LOCKED[] =
		"SELECT root_item_uid,room_revision,OCTET_LENGTH(canonical_origin),"
		"SUBSTRING(canonical_origin,1,?) FROM zone_reset_item_birth_origin "
		"WHERE birth_operation=? LIMIT 2 FOR UPDATE";
	static const char PEEK[] =
		"SELECT root_item_uid,room_revision,OCTET_LENGTH(canonical_origin),"
		"SUBSTRING(canonical_origin,1,?) FROM zone_reset_item_birth_origin "
		"WHERE birth_operation=? LIMIT 2";
	auto statement = prepare(connection, locked ? LOCKED : PEEK, &error);
	if (!statement)
		return error;
	uint64_t bound = ZONE_RESET_ITEM_ORIGIN_MAX_BYTES + 1;
	MYSQL_BIND parameters[] = { integer(&bound),
				    binary(operation.bytes.data(), operation.bytes.size()) };
	if (mysql_stmt_bind_param(statement.get(), parameters) ||
	    mysql_stmt_execute(statement.get()) || mysql_stmt_store_result(statement.get()))
		return statement_error(statement.get());
	if (mysql_stmt_num_rows(statement.get()) > 1)
		return EILSEQ;
	observed_origin candidate;
	if (mysql_stmt_num_rows(statement.get()))
	{
		uint64_t root_uid = 0, room_revision = 0, stored_size = 0;
		MYSQL_BIND values[] = { integer(&root_uid), integer(&room_revision),
					integer(&stored_size), binary(nullptr, 0) };
		unsigned long lengths[4]{};
		mysql_flag nulls[4]{}, errors[4]{};
		for (size_t index = 0; index < 4; ++index)
		{
			values[index].length = &lengths[index];
			values[index].is_null = &nulls[index];
			values[index].error = &errors[index];
		}
		if (mysql_stmt_bind_result(statement.get(), values))
			return statement_error(statement.get());
		const auto fetched = mysql_stmt_fetch(statement.get());
		if (fetched != 0 && fetched != MYSQL_DATA_TRUNCATED)
			return statement_error(statement.get());
		if (nulls[0] || nulls[1] || nulls[2] || nulls[3] || errors[0] || errors[1] ||
		    errors[2] || !root_uid || !room_revision || !stored_size ||
		    stored_size > ZONE_RESET_ITEM_ORIGIN_MAX_BYTES || lengths[3] != stored_size)
			return EBADMSG;
		candidate.canonical.resize(static_cast<size_t>(stored_size));
		auto body = binary(candidate.canonical.data(), lengths[3]);
		unsigned long length = lengths[3];
		mysql_flag is_null = false, truncated = false;
		body.length = &length;
		body.is_null = &is_null;
		body.error = &truncated;
		if (mysql_stmt_fetch_column(statement.get(), &body, 3, 0))
			return statement_error(statement.get());
		if (is_null || truncated || length != candidate.canonical.size())
			return EBADMSG;
		error = codec_error(
			zone_reset_item_origin_decode(candidate.canonical, &candidate.value));
		if (error)
			return error;
		item_transfer_result result{};
		if (!critical_operation_id_equal(candidate.value.original.operation_id,
						 operation) ||
		    !item_transfer_command_decode_result(candidate.value.result.data(),
							 candidate.value.result.size(), &result) ||
		    result.root_item_uid != root_uid || result.to_owner_revision != room_revision)
			return EILSEQ;
		candidate.value.present = true;
	}
	statement.reset();
	error = session_error(connection, session);
	if (error)
		return error;
	*output = std::move(candidate);
	return 0;
}
}
#endif

int zone_reset_item_origin_sql_lock(MYSQL *connection, const critical_operation_id &operation,
				    zone_reset_item_retained_origin *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)operation;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output || critical_operation_id_is_zero(operation))
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		observed_origin observed;
		int error = read(connection, session, operation, &observed, false);
		if (error)
			return error;
		if (!observed.value.present)
		{
			*output = std::move(observed.value);
			return 0;
		}
		error = static_cast<int>(economic_sql_zone_reset_item_verify_retained(
			connection, observed.value.original, 0, observed.value.result));
		if (error)
			return error;
		observed_origin locked;
		error = read(connection, session, operation, &locked, true);
		if (error)
			return error;
		if (!locked.value.present || observed.canonical != locked.canonical)
			return EILSEQ;
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_retained_origin>);
		*output = std::move(locked.value);
		return 0;
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

int zone_reset_item_origin_sql_retain_locked(MYSQL *connection, const critical_command &command,
					     std::span<const uint8_t> result) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result;
	return ENOTSUP;
#else
	if (!connection)
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		int error = session_error(connection, session);
		if (error)
			return error;
		std::vector<uint8_t> canonical;
		error = codec_error(zone_reset_item_origin_encode(command, result, &canonical));
		if (error)
			return error;
		error = static_cast<int>(economic_sql_zone_reset_item_verify_retained(
			connection, command, 0, result));
		if (error)
			return error;
		observed_origin retained;
		error = read(connection, session, command.operation_id, &retained, true);
		if (error)
			return error;
		if (retained.value.present)
			return retained.canonical == canonical ? 0 : EILSEQ;
		item_transfer_result receipt{};
		if (!item_transfer_command_decode_result(result.data(), result.size(), &receipt))
			return EBADMSG;
		static const char SQL[] =
			"INSERT INTO zone_reset_item_birth_origin "
			"(root_item_uid,birth_operation,room_revision,canonical_origin) VALUES (?,?,?,?)";
		auto statement = prepare(connection, SQL, &error);
		if (!statement)
			return error;
		MYSQL_BIND parameters[] = { integer(&receipt.root_item_uid),
					    binary(command.operation_id.bytes.data(),
						   command.operation_id.bytes.size()),
					    integer(&receipt.to_owner_revision),
					    binary(canonical.data(), canonical.size()) };
		if (mysql_stmt_bind_param(statement.get(), parameters) ||
		    mysql_stmt_execute(statement.get()))
			return statement_error(statement.get());
		if (mysql_stmt_affected_rows(statement.get()) != 1)
			return EIO;
		statement.reset();
		error = read(connection, session, command.operation_id, &retained, true);
		if (error)
			return error;
		return retained.value.present && retained.canonical == canonical ? 0 : EILSEQ;
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
