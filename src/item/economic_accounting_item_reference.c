#include "item/economic_accounting_item_reference.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>

namespace
{
bool id_is_nonzero(const critical_operation_id &id)
{
	for (auto byte : id.bytes)
	{
		if (byte != 0)
			return true;
	}
	return false;
}
} // namespace

bool economic_accounting_item_reference_validate(const economic_accounting_item_reference &ref)
{
	if (ref.line_index >= 3000)
		return false;
	if (ref.event_index != ref.line_index)
		return false;
	if (ref.child_index > 64)
		return false;
	if (ref.item_uid == 0)
		return false;
	if (ref.after_revision <= ref.before_revision)
		return false;
	if (!id_is_nonzero(ref.operation_id))
		return false;
	if (!id_is_nonzero(ref.legacy_operation_id))
		return false;
	return true;
}

#ifdef __NO_MYSQL__

bool economic_accounting_item_reference_insert(MYSQL *, const economic_accounting_item_reference &)
{
	errno = ENOTSUP;
	return false;
}

bool economic_accounting_item_reference_insert_batch(
	MYSQL *, std::span<const economic_accounting_item_reference>)
{
	errno = ENOTSUP;
	return false;
}

bool economic_accounting_item_reference_find_by_legacy(MYSQL *, const critical_operation_id &,
						       uint16_t,
						       economic_accounting_item_reference *)
{
	errno = ENOTSUP;
	return false;
}

bool economic_accounting_item_reference_find_by_operation(
	MYSQL *, const critical_operation_id &, std::vector<economic_accounting_item_reference> *)
{
	errno = ENOTSUP;
	return false;
}

bool economic_accounting_item_reference_find_history(
	MYSQL *, uint64_t, std::vector<economic_accounting_item_reference> *)
{
	errno = ENOTSUP;
	return false;
}

#else

namespace
{
bool statement_ok(MYSQL_STMT *statement, bool condition)
{
	// Capture the statement diagnostic before close and never report ambient errno.
	const unsigned int error = !condition && statement ? mysql_stmt_errno(statement) : 0;
	if (statement)
		mysql_stmt_close(statement);
	if (!condition)
		errno = error ? error : EIO;
	return condition;
}

bool prepare(MYSQL_STMT **statement, MYSQL *connection, const char *sql)
{
	*statement = mysql_stmt_init(connection);
	if (!*statement)
	{
		errno = mysql_errno(connection);
		if (!errno)
			errno = ENOMEM;
		return false;
	}
	if (mysql_stmt_prepare(*statement, sql, std::strlen(sql)) != 0)
	{
		statement_ok(*statement, false);
		*statement = nullptr;
		return false;
	}
	return true;
}
} // namespace

bool economic_accounting_item_reference_insert(MYSQL *connection,
					       const economic_accounting_item_reference &ref)
{
	if (!connection)
	{
		errno = EINVAL;
		return false;
	}
	if (!economic_accounting_item_reference_validate(ref))
	{
		errno = EINVAL;
		return false;
	}

	static const char SQL[] = "INSERT INTO economic_accounting_item_reference("
				  "operation_id, line_index, event_index, child_index, "
				  "item_uid, before_revision, after_revision, "
				  "legacy_operation_id, legacy_event_index"
				  ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;

	MYSQL_BIND bindings[9] = {};
	unsigned long op_length = ref.operation_id.bytes.size();
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(ref.operation_id.bytes.data());
	bindings[0].buffer_length = op_length;
	bindings[0].length = &op_length;

	uint16_t line_index = ref.line_index;
	bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	bindings[1].buffer = &line_index;
	bindings[1].is_unsigned = true;

	uint32_t event_index = ref.event_index;
	bindings[2].buffer_type = MYSQL_TYPE_LONG;
	bindings[2].buffer = &event_index;
	bindings[2].is_unsigned = true;

	uint16_t child_index = ref.child_index;
	bindings[3].buffer_type = MYSQL_TYPE_SHORT;
	bindings[3].buffer = &child_index;
	bindings[3].is_unsigned = true;

	uint64_t item_uid = ref.item_uid;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = &item_uid;
	bindings[4].is_unsigned = true;

	uint64_t before_rev = ref.before_revision;
	bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[5].buffer = &before_rev;
	bindings[5].is_unsigned = true;

	uint64_t after_rev = ref.after_revision;
	bindings[6].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[6].buffer = &after_rev;
	bindings[6].is_unsigned = true;

	unsigned long legacy_op_length = ref.legacy_operation_id.bytes.size();
	bindings[7].buffer_type = MYSQL_TYPE_BLOB;
	bindings[7].buffer = const_cast<uint8_t *>(ref.legacy_operation_id.bytes.data());
	bindings[7].buffer_length = legacy_op_length;
	bindings[7].length = &legacy_op_length;

	uint16_t legacy_event = ref.legacy_event_index;
	bindings[8].buffer_type = MYSQL_TYPE_SHORT;
	bindings[8].buffer = &legacy_event;
	bindings[8].is_unsigned = true;

	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0 &&
					       mysql_stmt_affected_rows(statement) == 1);
}

bool economic_accounting_item_reference_insert_batch(
	MYSQL *connection, std::span<const economic_accounting_item_reference> refs)
{
	for (const auto &ref : refs)
	{
		if (!economic_accounting_item_reference_insert(connection, ref))
			return false;
	}
	return true;
}

bool economic_accounting_item_reference_find_by_legacy(
	MYSQL *connection, const critical_operation_id &legacy_operation_id,
	uint16_t legacy_event_index, economic_accounting_item_reference *ref)
{
	if (!connection)
	{
		errno = EINVAL;
		return false;
	}

	static const char SQL[] = "SELECT operation_id, line_index, event_index, child_index, "
				  "item_uid, before_revision, after_revision "
				  "FROM economic_accounting_item_reference "
				  "WHERE legacy_operation_id=? AND legacy_event_index=?";

	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;

	MYSQL_BIND param_bindings[2] = {};
	unsigned long legacy_op_length = legacy_operation_id.bytes.size();
	param_bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	param_bindings[0].buffer = const_cast<uint8_t *>(legacy_operation_id.bytes.data());
	param_bindings[0].buffer_length = legacy_op_length;
	param_bindings[0].length = &legacy_op_length;

	param_bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	param_bindings[1].buffer = &legacy_event_index;
	param_bindings[1].is_unsigned = true;

	if (mysql_stmt_bind_param(statement, param_bindings) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
	{
		return statement_ok(statement, false);
	}

	if (mysql_stmt_num_rows(statement) != 1)
	{
		mysql_stmt_close(statement);
		errno = ENOENT;
		return false;
	}

	uint8_t op_bytes[16] = {};
	unsigned long fetched_op_length = 0;
	uint16_t line_index = 0;
	uint32_t event_index = 0;
	uint16_t child_index = 0;
	uint64_t item_uid = 0;
	uint64_t before_rev = 0;
	uint64_t after_rev = 0;

	MYSQL_BIND result_bindings[7] = {};
	result_bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	result_bindings[0].buffer = op_bytes;
	result_bindings[0].buffer_length = sizeof(op_bytes);
	result_bindings[0].length = &fetched_op_length;

	result_bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[1].buffer = &line_index;
	result_bindings[1].is_unsigned = true;

	result_bindings[2].buffer_type = MYSQL_TYPE_LONG;
	result_bindings[2].buffer = &event_index;
	result_bindings[2].is_unsigned = true;

	result_bindings[3].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[3].buffer = &child_index;
	result_bindings[3].is_unsigned = true;

	result_bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[4].buffer = &item_uid;
	result_bindings[4].is_unsigned = true;

	result_bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[5].buffer = &before_rev;
	result_bindings[5].is_unsigned = true;

	result_bindings[6].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[6].buffer = &after_rev;
	result_bindings[6].is_unsigned = true;

	if (mysql_stmt_bind_result(statement, result_bindings) != 0 ||
	    mysql_stmt_fetch(statement) != 0)
	{
		return statement_ok(statement, false);
	}

	mysql_stmt_close(statement);

	if (ref)
	{
		std::copy(std::begin(op_bytes), std::end(op_bytes),
			  ref->operation_id.bytes.begin());
		ref->line_index = line_index;
		ref->event_index = event_index;
		ref->child_index = child_index;
		ref->item_uid = item_uid;
		ref->before_revision = before_rev;
		ref->after_revision = after_rev;
		ref->legacy_operation_id = legacy_operation_id;
		ref->legacy_event_index = legacy_event_index;
	}

	return true;
}

bool economic_accounting_item_reference_find_by_operation(
	MYSQL *connection, const critical_operation_id &operation_id,
	std::vector<economic_accounting_item_reference> *refs)
{
	if (!connection || !id_is_nonzero(operation_id) || !refs)
	{
		errno = EINVAL;
		return false;
	}
	refs->clear();

	static const char SQL[] =
		"SELECT operation_id, line_index, event_index, child_index, "
		"item_uid, before_revision, after_revision, legacy_operation_id, legacy_event_index "
		"FROM economic_accounting_item_reference "
		"WHERE operation_id=? "
		"ORDER BY line_index ASC";

	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;

	MYSQL_BIND param_binding = {};
	unsigned long op_length = operation_id.bytes.size();
	param_binding.buffer_type = MYSQL_TYPE_BLOB;
	param_binding.buffer = const_cast<uint8_t *>(operation_id.bytes.data());
	param_binding.buffer_length = op_length;
	param_binding.length = &op_length;

	if (mysql_stmt_bind_param(statement, &param_binding) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
	{
		mysql_stmt_close(statement);
		return false;
	}

	uint8_t op_bytes[16] = {};
	unsigned long fetched_op_length = 0;
	uint16_t line_index = 0;
	uint32_t event_index = 0;
	uint16_t child_index = 0;
	uint64_t item_uid = 0;
	uint64_t before_rev = 0;
	uint64_t after_rev = 0;
	uint8_t legacy_op_bytes[16] = {};
	unsigned long fetched_legacy_op_length = 0;
	uint16_t legacy_event_index = 0;

	MYSQL_BIND result_bindings[9] = {};
	result_bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	result_bindings[0].buffer = op_bytes;
	result_bindings[0].buffer_length = sizeof(op_bytes);
	result_bindings[0].length = &fetched_op_length;

	result_bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[1].buffer = &line_index;
	result_bindings[1].is_unsigned = true;

	result_bindings[2].buffer_type = MYSQL_TYPE_LONG;
	result_bindings[2].buffer = &event_index;
	result_bindings[2].is_unsigned = true;

	result_bindings[3].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[3].buffer = &child_index;
	result_bindings[3].is_unsigned = true;

	result_bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[4].buffer = &item_uid;
	result_bindings[4].is_unsigned = true;

	result_bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[5].buffer = &before_rev;
	result_bindings[5].is_unsigned = true;

	result_bindings[6].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[6].buffer = &after_rev;
	result_bindings[6].is_unsigned = true;

	result_bindings[7].buffer_type = MYSQL_TYPE_BLOB;
	result_bindings[7].buffer = legacy_op_bytes;
	result_bindings[7].buffer_length = sizeof(legacy_op_bytes);
	result_bindings[7].length = &fetched_legacy_op_length;

	result_bindings[8].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[8].buffer = &legacy_event_index;
	result_bindings[8].is_unsigned = true;

	if (mysql_stmt_bind_result(statement, result_bindings) != 0)
	{
		mysql_stmt_close(statement);
		return false;
	}

	while (mysql_stmt_fetch(statement) == 0)
	{
		economic_accounting_item_reference r = {};
		std::copy(std::begin(op_bytes), std::end(op_bytes), r.operation_id.bytes.begin());
		r.line_index = line_index;
		r.event_index = event_index;
		r.child_index = child_index;
		r.item_uid = item_uid;
		r.before_revision = before_rev;
		r.after_revision = after_rev;
		std::copy(std::begin(legacy_op_bytes), std::end(legacy_op_bytes),
			  r.legacy_operation_id.bytes.begin());
		r.legacy_event_index = legacy_event_index;
		refs->push_back(r);
	}

	mysql_stmt_close(statement);
	return true;
}

bool economic_accounting_item_reference_find_history(
	MYSQL *connection, uint64_t item_uid,
	std::vector<economic_accounting_item_reference> *history)
{
	if (!connection || item_uid == 0 || !history)
	{
		errno = EINVAL;
		return false;
	}
	history->clear();

	static const char SQL[] =
		"SELECT operation_id, line_index, event_index, child_index, "
		"item_uid, before_revision, after_revision, legacy_operation_id, legacy_event_index "
		"FROM economic_accounting_item_reference "
		"WHERE item_uid=? "
		"ORDER BY after_revision ASC";

	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;

	MYSQL_BIND param_binding = {};
	param_binding.buffer_type = MYSQL_TYPE_LONGLONG;
	param_binding.buffer = &item_uid;
	param_binding.is_unsigned = true;

	if (mysql_stmt_bind_param(statement, &param_binding) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
	{
		mysql_stmt_close(statement);
		return false;
	}

	uint8_t op_bytes[16] = {};
	unsigned long fetched_op_length = 0;
	uint16_t line_index = 0;
	uint32_t event_index = 0;
	uint16_t child_index = 0;
	uint64_t fetched_item_uid = 0;
	uint64_t before_rev = 0;
	uint64_t after_rev = 0;
	uint8_t legacy_op_bytes[16] = {};
	unsigned long fetched_legacy_op_length = 0;
	uint16_t legacy_event_index = 0;

	MYSQL_BIND result_bindings[9] = {};
	result_bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	result_bindings[0].buffer = op_bytes;
	result_bindings[0].buffer_length = sizeof(op_bytes);
	result_bindings[0].length = &fetched_op_length;

	result_bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[1].buffer = &line_index;
	result_bindings[1].is_unsigned = true;

	result_bindings[2].buffer_type = MYSQL_TYPE_LONG;
	result_bindings[2].buffer = &event_index;
	result_bindings[2].is_unsigned = true;

	result_bindings[3].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[3].buffer = &child_index;
	result_bindings[3].is_unsigned = true;

	result_bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[4].buffer = &fetched_item_uid;
	result_bindings[4].is_unsigned = true;

	result_bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[5].buffer = &before_rev;
	result_bindings[5].is_unsigned = true;

	result_bindings[6].buffer_type = MYSQL_TYPE_LONGLONG;
	result_bindings[6].buffer = &after_rev;
	result_bindings[6].is_unsigned = true;

	result_bindings[7].buffer_type = MYSQL_TYPE_BLOB;
	result_bindings[7].buffer = legacy_op_bytes;
	result_bindings[7].buffer_length = sizeof(legacy_op_bytes);
	result_bindings[7].length = &fetched_legacy_op_length;

	result_bindings[8].buffer_type = MYSQL_TYPE_SHORT;
	result_bindings[8].buffer = &legacy_event_index;
	result_bindings[8].is_unsigned = true;

	if (mysql_stmt_bind_result(statement, result_bindings) != 0)
	{
		mysql_stmt_close(statement);
		return false;
	}

	while (mysql_stmt_fetch(statement) == 0)
	{
		economic_accounting_item_reference r = {};
		std::copy(std::begin(op_bytes), std::end(op_bytes), r.operation_id.bytes.begin());
		r.line_index = line_index;
		r.event_index = event_index;
		r.child_index = child_index;
		r.item_uid = fetched_item_uid;
		r.before_revision = before_rev;
		r.after_revision = after_rev;
		std::copy(std::begin(legacy_op_bytes), std::end(legacy_op_bytes),
			  r.legacy_operation_id.bytes.begin());
		r.legacy_event_index = legacy_event_index;
		history->push_back(r);
	}

	mysql_stmt_close(statement);
	return true;
}

#endif
