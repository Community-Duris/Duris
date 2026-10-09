#include "persistence/zone_reset_item_terminal_sql.h"

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
int read_terminal(MYSQL *connection, unsigned long session, const critical_operation_id &operation,
		  std::vector<uint8_t> *bytes, bool *present)
{
	int error = session_error(connection, session);
	if (error)
		return error;
	static const char SQL[] =
		"SELECT OCTET_LENGTH(terminal_publication_context),"
		"SUBSTRING(terminal_publication_context,1,?) "
		"FROM zone_reset_item_birth_origin WHERE birth_operation=? LIMIT 2 FOR UPDATE";
	auto statement = prepare(connection, SQL, &error);
	if (!statement)
		return error;
	uint64_t bound = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES + 1;
	MYSQL_BIND parameters[] = { integer(&bound),
				    binary(operation.bytes.data(), operation.bytes.size()) };
	if (mysql_stmt_bind_param(statement.get(), parameters) ||
	    mysql_stmt_execute(statement.get()) || mysql_stmt_store_result(statement.get()))
		return statement_error(statement.get());
	if (mysql_stmt_num_rows(statement.get()) != 1)
		return EILSEQ;
	uint64_t stored_size = 0;
	MYSQL_BIND values[] = { integer(&stored_size), binary(nullptr, 0) };
	unsigned long lengths[2]{};
	mysql_flag nulls[2]{}, errors[2]{};
	for (size_t i = 0; i < 2; ++i)
	{
		values[i].length = &lengths[i];
		values[i].is_null = &nulls[i];
		values[i].error = &errors[i];
	}
	if (mysql_stmt_bind_result(statement.get(), values))
		return statement_error(statement.get());
	const int fetched = mysql_stmt_fetch(statement.get());
	if (fetched != 0 && fetched != MYSQL_DATA_TRUNCATED)
		return statement_error(statement.get());
	if (nulls[0] != nulls[1] || errors[0])
		return EBADMSG;
	std::vector<uint8_t> candidate;
	const bool found = !nulls[0];
	if (found)
	{
		if (!stored_size || stored_size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    lengths[1] != stored_size)
			return EBADMSG;
		candidate.resize(static_cast<size_t>(stored_size));
		auto body = binary(candidate.data(), lengths[1]);
		unsigned long length = lengths[1];
		mysql_flag is_null = false, truncated = false;
		body.length = &length;
		body.is_null = &is_null;
		body.error = &truncated;
		if (mysql_stmt_fetch_column(statement.get(), &body, 1, 0))
			return statement_error(statement.get());
		if (is_null || truncated || length != candidate.size())
			return EBADMSG;
	}
	statement.reset();
	error = session_error(connection, session);
	if (error)
		return error;
	*bytes = std::move(candidate);
	*present = found;
	return 0;
}
int correlate(const zone_reset_item_retained_origin &origin,
	      zone_reset_item_retained_terminal *terminal)
{
	if (!origin.present)
		return ENODATA;
	int error = codec_error(zone_reset_item_recovery_original_command_decode(
		terminal->canonical, &terminal->original));
	if (error)
		return error;
	if (!critical_command_equal(origin.original, terminal->original))
		return EILSEQ;
	error = codec_error(zone_reset_item_recovery_decode(terminal->original, terminal->canonical,
							    &terminal->context));
	if (error)
		return error;
	const auto &receipt = terminal->context.receipt;
	if (!terminal->context.receipt_present || receipt.result_size != origin.result.size() ||
	    !std::equal(origin.result.begin(), origin.result.end(), receipt.result_payload.begin()))
		return EILSEQ;
	std::vector<uint8_t> canonical;
	error = codec_error(
		zone_reset_item_recovery_encode(terminal->original, terminal->context, &canonical));
	if (error)
		return error;
	if (canonical != terminal->canonical)
		return EILSEQ;
	terminal->present = true;
	return 0;
}
}
#endif

int zone_reset_item_terminal_sql_owner::load_locked(
	MYSQL *connection, const critical_operation_id &operation,
	zone_reset_item_retained_terminal *output) noexcept
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
		int error = session_error(connection, session);
		if (error)
			return error;
		zone_reset_item_retained_origin origin;
		// Original helper verifies full historical successful root before lock;
		// terminal BODY alone cannot confer source/economic admission authority.
		error = zone_reset_item_origin_sql_lock(connection, operation, &origin);
		if (error)
			return error;
		if (!origin.present)
			return ENODATA;
		zone_reset_item_retained_terminal candidate;
		bool present = false;
		error = read_terminal(connection, session, operation, &candidate.canonical,
				      &present);
		if (error)
			return error;
		if (present)
		{
			error = correlate(origin, &candidate);
			if (error)
				return error;
		}
		error = session_error(connection, session);
		if (error)
			return error;
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_retained_terminal>);
		*output = std::move(candidate);
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

int zone_reset_item_terminal_sql_owner::retain_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)envelope;
	return ENOTSUP;
#else
	if (!connection || !zone_reset_item_recovery_terminal(envelope))
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		int error = session_error(connection, session);
		if (error)
			return error;
		zone_reset_item_retained_origin origin;
		error = zone_reset_item_origin_sql_lock(connection, envelope.command.operation_id,
							&origin);
		if (error)
			return error;
		if (!origin.present)
			return ENODATA;
		zone_reset_item_retained_terminal expected;
		expected.canonical = envelope.attachment;
		error = correlate(origin, &expected);
		if (error)
			return error;
		if (!critical_command_equal(envelope.command, expected.original))
			return EILSEQ;
		std::vector<uint8_t> retained;
		bool present = false;
		error = read_terminal(connection, session, envelope.command.operation_id, &retained,
				      &present);
		if (error)
			return error;
		if (present)
			return retained == expected.canonical ? 0 : EILSEQ;
		static const char SQL[] =
			"UPDATE zone_reset_item_birth_origin SET terminal_publication_context=? "
			"WHERE birth_operation=? AND terminal_publication_context IS NULL";
		auto statement = prepare(connection, SQL, &error);
		if (!statement)
			return error;
		MYSQL_BIND parameters[] = { binary(expected.canonical.data(),
						   expected.canonical.size()),
					    binary(envelope.command.operation_id.bytes.data(),
						   envelope.command.operation_id.bytes.size()) };
		if (mysql_stmt_bind_param(statement.get(), parameters) ||
		    mysql_stmt_execute(statement.get()))
			return statement_error(statement.get());
		if (mysql_stmt_affected_rows(statement.get()) != 1)
			return EILSEQ;
		statement.reset();
		error = read_terminal(connection, session, envelope.command.operation_id, &retained,
				      &present);
		if (error)
			return error;
		return present && retained == expected.canonical ? 0 : EILSEQ;
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
