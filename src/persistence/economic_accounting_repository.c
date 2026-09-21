#include "persistence/economic_accounting_repository.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

#ifdef __NO_MYSQL__
unsigned int economic_sql_lock_authority(MYSQL *, const critical_operation_id &,
					 const critical_operation_id &,
					 std::span<const economic_sql_mapping_request>,
					 economic_sql_authority_snapshot *)
{
	return ENOTSUP;
}
#else
namespace
{
using statement_ptr = std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)>;

unsigned int statement_error(MYSQL_STMT *statement)
{
	const auto error = mysql_stmt_errno(statement);
	return error ? error : EIO;
}

// One unique indexed row, including explicit NULL/truncation detection. Do not
// silently interpret a corrupt nullable identity as an all-zero/default value.
template <size_t Count> unsigned int read_row(MYSQL *connection, const char *sql,
					      MYSQL_BIND *parameters,
					      std::array<MYSQL_BIND, Count> &columns)
{
	statement_ptr statement(mysql_stmt_init(connection), mysql_stmt_close);
	if (!statement)
		return ENOMEM;
	if (mysql_stmt_prepare(statement.get(), sql, strlen(sql)) ||
	    mysql_stmt_bind_param(statement.get(), parameters) ||
	    mysql_stmt_execute(statement.get()) || mysql_stmt_store_result(statement.get()))
		return statement_error(statement.get());
	const auto rows = mysql_stmt_num_rows(statement.get());
	if (rows != 1)
		return rows == 0 ? ENOENT : EILSEQ;
	using null_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	std::array<null_flag, Count> nulls = {};
	std::array<null_flag, Count> errors = {};
	for (size_t index = 0; index < Count; ++index)
	{
		columns[index].is_null = &nulls[index];
		columns[index].error = &errors[index];
	}
	if (mysql_stmt_bind_result(statement.get(), columns.data()))
		return statement_error(statement.get());
	const auto fetched = mysql_stmt_fetch(statement.get());
	if (fetched == MYSQL_DATA_TRUNCATED)
		return EILSEQ;
	if (fetched != 0)
		return statement_error(statement.get());
	for (size_t index = 0; index < Count; ++index)
		if (nulls[index] || errors[index])
			return EILSEQ;
	return 0;
}

MYSQL_BIND bytes(void *data, unsigned long size, unsigned long *length = nullptr)
{
	MYSQL_BIND binding = {};
	binding.buffer_type = MYSQL_TYPE_BLOB;
	binding.buffer = data;
	binding.buffer_length = size;
	binding.length = length;
	return binding;
}

MYSQL_BIND number(uint64_t *value)
{
	MYSQL_BIND binding = {};
	binding.buffer_type = MYSQL_TYPE_LONGLONG;
	binding.buffer = value;
	binding.is_unsigned = true;
	return binding;
}
}

unsigned int economic_sql_lock_authority(MYSQL *connection, const critical_operation_id &lineage,
					 const critical_operation_id &expected_epoch,
					 std::span<const economic_sql_mapping_request> requests,
					 economic_sql_authority_snapshot *snapshot)
{
	if (!connection || !snapshot || critical_operation_id_is_zero(lineage) ||
	    critical_operation_id_is_zero(expected_epoch))
		return EINVAL;
	if (requests.size() > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS)
		return E2BIG;
	// A shared lock in autocommit would expire immediately after its statement.
	// mysql's status is updated by START TRANSACTION/COMMIT/ROLLBACK responses.
	if (!(connection->server_status & SERVER_STATUS_IN_TRANS))
		return EPERM;
	using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	client_flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EPERM;
	const auto session_id = mysql_thread_id(connection);
	try
	{
		economic_sql_authority_snapshot candidate;
		candidate.lineage = lineage;
		candidate.epoch = expected_epoch;
		candidate.mappings.reserve(requests.size());
		for (const auto &request : requests)
		{
			if (!economic_account_key_valid(request.account) ||
			    !economic_account_is_ordinary(request.account.kind) ||
			    request.account.lineage.bytes != lineage.bytes ||
			    !request.locator_kind || !request.native_id)
				return EINVAL;
			candidate.mappings.push_back({ request, 0 });
		}
		std::sort(candidate.mappings.begin(), candidate.mappings.end(),
			  [](const auto &left, const auto &right) {
				  return left.request.account.authority_id <
					 right.request.account.authority_id;
			  });
		for (size_t index = 1; index < candidate.mappings.size(); ++index)
			if (candidate.mappings[index - 1].request.account.authority_id ==
			    candidate.mappings[index].request.account.authority_id)
				return EINVAL;

		critical_operation_id active_epoch = {};
		unsigned long epoch_length = 0;
		auto lineage_binding =
			bytes(candidate.lineage.bytes.data(), candidate.lineage.bytes.size());
		std::array<MYSQL_BIND, 2> lineage_columns = {
			bytes(active_epoch.bytes.data(), active_epoch.bytes.size(), &epoch_length),
			number(&candidate.lineage_revision)
		};
		auto error = read_row(
			connection,
			"SELECT COALESCE(active_epoch,REPEAT(CHAR(0),16)),revision FROM economic_lineage_state WHERE lineage=? LOCK IN SHARE MODE",
			&lineage_binding, lineage_columns);
		if (error)
			return error;
		if (epoch_length != active_epoch.bytes.size())
			return EILSEQ;
		if (critical_operation_id_is_zero(active_epoch))
			return ENODATA;
		if (active_epoch.bytes != expected_epoch.bytes)
			return ESTALE;

		for (auto &mapping : candidate.mappings)
		{
			critical_operation_id mapped_lineage = {};
			unsigned long lineage_length = 0;
			uint64_t kind = 0, context = 0, backend = 0, locator = 0, native = 0,
				 active = 0;
			auto id = mapping.request.account.authority_id;
			auto id_binding = number(&id);
			std::array<MYSQL_BIND, 8> columns = { bytes(mapped_lineage.bytes.data(),
								    mapped_lineage.bytes.size(),
								    &lineage_length),
							      number(&kind),
							      number(&context),
							      number(&backend),
							      number(&locator),
							      number(&native),
							      number(&active),
							      number(&mapping.revision) };
			error = read_row(
				connection,
				"SELECT lineage,account_kind,context_id,backend_kind,locator_kind,native_id,"
				"COALESCE(active_native_id,0),revision FROM economic_account_mapping WHERE mapping_id=? LOCK IN SHARE MODE",
				&id_binding, columns);
			if (error)
				return error;
			if (lineage_length != mapped_lineage.bytes.size())
				return EILSEQ;
			if (mapped_lineage.bytes != lineage.bytes ||
			    kind != static_cast<uint16_t>(mapping.request.account.kind) ||
			    context != mapping.request.account.context_id ||
			    backend != ECONOMIC_MAPPING_BACKEND_SQL ||
			    locator != mapping.request.locator_kind ||
			    native != mapping.request.native_id || active != native)
				return ESTALE;
		}
		// Defensive against client reconnect settings: identities read after a
		// lost transaction must never be returned as still protected by locks.
		if (!(connection->server_status & SERVER_STATUS_IN_TRANS) ||
		    mysql_thread_id(connection) != session_id)
			return ENOTCONN;
		*snapshot = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

#endif
