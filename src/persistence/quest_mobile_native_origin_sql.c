#include "persistence/quest_mobile_native_origin_sql.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"

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
	const auto code = mysql_stmt_errno(statement);
	return code ? static_cast<int>(code) : EIO;
}
statement_ptr prepare(MYSQL *connection, const char *sql, int *error)
{
	statement_ptr result(mysql_stmt_init(connection), mysql_stmt_close);
	if (!result)
		*error = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) :
						   ENOMEM;
	else if (mysql_stmt_prepare(result.get(), sql, std::strlen(sql)))
	{
		*error = statement_error(result.get());
		result.reset();
	}
	return result;
}
MYSQL_BIND integer(void *value) noexcept
{
	MYSQL_BIND result{};
	result.buffer_type = MYSQL_TYPE_LONGLONG;
	result.buffer = value;
	result.is_unsigned = true;
	return result;
}
MYSQL_BIND binary(const void *value, unsigned long size) noexcept
{
	MYSQL_BIND result{};
	result.buffer_type = MYSQL_TYPE_BLOB;
	result.buffer = const_cast<void *>(value);
	result.buffer_length = size;
	return result;
}
int codec_error(economic_accounting_error error) noexcept
{
	if (error == economic_accounting_error::ok)
		return 0;
	if (error == economic_accounting_error::capacity)
		return ENOMEM;
	return EBADMSG;
}
bool lifetime_equal(quest_mobile_native_reference original,
		    const quest_mobile_native_reference &current) noexcept
{
	// Exclude only the two mutable revision fields. All original issuance,
	// birth/source/provenance/VNUM/zone facts remain byte-authenticated.
	original.mobile_revision = current.mobile_revision;
	original.stock_revision = current.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> left{}, right{};
	return quest_mobile_native_reference_encode(original, &left) ==
		       player_snapshot_codec_result::ok &&
	       quest_mobile_native_reference_encode(current, &right) ==
		       player_snapshot_codec_result::ok &&
	       left == right;
}
int read_locked(MYSQL *connection, unsigned long session,
		const quest_mobile_native_reference &reference,
		quest_mobile_native_published_origin *output, bool lock_origin = true)
{
	int error = session_error(connection, session);
	if (error)
		return error;
	static const char LOCKED_SQL[] =
		"SELECT birth_operation,publication_revision,OCTET_LENGTH(canonical_origin),"
		"SUBSTRING(canonical_origin,1,?) FROM quest_mobile_native_birth_origin "
		"WHERE mobile_instance_id=? LIMIT 2 FOR UPDATE";
	static const char PEEK_SQL[] =
		"SELECT birth_operation,publication_revision,OCTET_LENGTH(canonical_origin),"
		"SUBSTRING(canonical_origin,1,?) FROM quest_mobile_native_birth_origin "
		"WHERE mobile_instance_id=? LIMIT 2";
	auto statement = prepare(connection, lock_origin ? LOCKED_SQL : PEEK_SQL, &error);
	if (!statement)
		return error;
	uint64_t bound = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES + 1;
	uint64_t native_id = reference.mobile_instance_id;
	MYSQL_BIND parameters[] = { integer(&bound), integer(&native_id) };
	if (mysql_stmt_bind_param(statement.get(), parameters) ||
	    mysql_stmt_execute(statement.get()) || mysql_stmt_store_result(statement.get()))
		return statement_error(statement.get());
	quest_mobile_native_published_origin candidate;
	if (mysql_stmt_num_rows(statement.get()) > 1)
		return EILSEQ;
	if (mysql_stmt_num_rows(statement.get()))
	{
		critical_operation_id operation;
		uint64_t revision = 0, stored_size = 0;
		MYSQL_BIND values[] = { binary(operation.bytes.data(), operation.bytes.size()),
					integer(&revision), integer(&stored_size),
					binary(nullptr, 0) };
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
		    errors[2] || lengths[0] != operation.bytes.size() || !revision ||
		    !stored_size || stored_size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    lengths[3] != stored_size)
			return EBADMSG;
		candidate.original.attachment.resize(static_cast<size_t>(stored_size));
		auto body = binary(candidate.original.attachment.data(), lengths[3]);
		unsigned long body_length = lengths[3];
		mysql_flag body_null = false, body_truncated = false;
		body.length = &body_length;
		body.is_null = &body_null;
		body.error = &body_truncated;
		if (mysql_stmt_fetch_column(statement.get(), &body, 3, 0))
			return statement_error(statement.get());
		if (body_null || body_truncated ||
		    body_length != candidate.original.attachment.size())
			return EBADMSG;
		error = codec_error(native_mobile_birth_recovery_original_command_decode(
			candidate.original.attachment, &candidate.original.command));
		if (error)
			return error;
		candidate.original.phase = critical_native_recovery_phase::continuation_pending;
		candidate.original.revision = revision;
		if (candidate.original.command.operation_id.bytes != operation.bytes ||
		    operation.bytes != reference.birth_operation.bytes ||
		    !native_mobile_birth_recovery_terminal(candidate.original))
			return EILSEQ;
		quest_mobile_native_image original_image;
		error = codec_error(native_mobile_birth_command_decode(candidate.original.command,
								       &original_image));
		if (error)
			return error;
		if (!lifetime_equal(original_image.reference, reference) ||
		    reference.mobile_revision < original_image.reference.mobile_revision ||
		    reference.stock_revision < original_image.reference.stock_revision)
			return EILSEQ;
		// This private helper proves canonical values only. The public reader
		// separately authenticates the original root BEFORE taking native/origin
		// locks; retention already holds that complete original publication proof.
		candidate.present = true;
	}
	statement.reset();
	error = session_error(connection, session);
	if (error)
		return error;
	static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_published_origin>);
	*output = std::move(candidate);
	return 0;
}
}
#endif

int quest_mobile_native_origin_sql_lock(MYSQL *connection,
					const quest_mobile_native_reference &reference,
					quest_mobile_native_published_origin *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)reference;
	(void)output;
	return ENOTSUP;
#else
	if (!connection || !output || !quest_mobile_native_reference_valid(reference))
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		quest_mobile_native_published_origin observed;
		int error = read_locked(connection, session, reference, &observed, false);
		if (error)
			return error;
		if (!observed.present)
		{
			// Unknown historical coverage is not restoration authority.
			*output = std::move(observed);
			return 0;
		}
		native_mobile_birth_recovery_context recovery;
		error = codec_error(native_mobile_birth_recovery_decode(
			observed.original.command, observed.original.attachment, &recovery));
		if (error)
			return error;
		error = static_cast<int>(economic_sql_native_mobile_birth_verify_retained(
			connection, observed.original.command, recovery.receipt.error_code,
			std::span<const uint8_t>(recovery.receipt.result_payload.data(),
						 recovery.receipt.result_size)));
		if (error)
			return error;
		quest_mobile_native_sql_row current;
		error = quest_mobile_native_sql_lock(connection, reference.mobile_instance_id,
						     &current);
		if (error)
			return error;
		if (!current.present || !lifetime_equal(current.image.reference, reference) ||
		    current.image.reference.mobile_revision != reference.mobile_revision ||
		    current.image.reference.stock_revision != reference.stock_revision)
			return ESTALE;
		quest_mobile_native_published_origin locked;
		error = read_locked(connection, session, reference, &locked);
		if (error)
			return error;
		if (!locked.present || locked.original.revision != observed.original.revision ||
		    locked.original.phase != observed.original.phase ||
		    locked.original.attachment != observed.original.attachment ||
		    !critical_command_equal(locked.original.command, observed.original.command))
			return EILSEQ;
		error = session_error(connection, session);
		if (error)
			return error;
		*output = std::move(locked);
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

int quest_mobile_native_origin_sql_retain_locked(
	MYSQL *connection, const critical_native_recovery_envelope &original) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)original;
	return ENOTSUP;
#else
	if (!connection ||
	    original.command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
	    !native_mobile_birth_recovery_terminal(original))
		return EINVAL;
	try
	{
		const auto session = mysql_thread_id(connection);
		int error = session_error(connection, session);
		if (error)
			return error;
		native_mobile_birth_recovery_context recovery;
		error = codec_error(native_mobile_birth_recovery_decode(
			original.command, original.attachment, &recovery));
		if (error)
			return error;
		quest_mobile_native_image image;
		std::vector<item_ownership_runtime_entry> custody;
		error = static_cast<int>(economic_sql_native_mobile_birth_lock_publication(
			connection, original.command, recovery.receipt, &image, &custody));
		if (error)
			return error;
		quest_mobile_native_published_origin retained;
		error = read_locked(connection, session, image.reference, &retained);
		if (error)
			return error;
		const auto exact = [&](const quest_mobile_native_published_origin &value)
		{
			return value.present && value.original.revision == original.revision &&
			       value.original.phase == original.phase &&
			       value.original.attachment == original.attachment &&
			       critical_command_equal(value.original.command, original.command);
		};
		if (retained.present)
			return exact(retained) ? session_error(connection, session) : EILSEQ;
		static const char SQL[] =
			"INSERT INTO quest_mobile_native_birth_origin "
			"(mobile_instance_id,birth_operation,publication_revision,canonical_origin) "
			"VALUES (?,?,?,?)";
		auto statement = prepare(connection, SQL, &error);
		if (!statement)
			return error;
		uint64_t native_id = image.reference.mobile_instance_id,
			 revision = original.revision;
		MYSQL_BIND parameters[] = {
			integer(&native_id), binary(original.command.operation_id.bytes.data(), 16),
			integer(&revision),
			binary(original.attachment.data(), original.attachment.size())
		};
		if (mysql_stmt_bind_param(statement.get(), parameters) ||
		    mysql_stmt_execute(statement.get()))
			return statement_error(statement.get());
		if (mysql_stmt_affected_rows(statement.get()) != 1)
			return EIO;
		statement.reset();
		error = read_locked(connection, session, image.reference, &retained);
		if (error)
			return error;
		return exact(retained) ? session_error(connection, session) : EILSEQ;
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
