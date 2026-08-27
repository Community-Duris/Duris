#include "item_transfer_repository.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <new>
#include <vector>

namespace
{
struct current_item
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	uint8_t owner_type;
	uint64_t owner_id;
	uint64_t owner_context_id;
	uint64_t item_revision;
	int32_t vnum;
	uint8_t state;
};

bool prepare(MYSQL_STMT **statement, MYSQL *connection, const char *sql)
{
	*statement = mysql_stmt_init(connection);
	if (!*statement || mysql_stmt_prepare(*statement, sql, strlen(sql)) != 0)
	{
		errno = *statement ? mysql_stmt_errno(*statement) : ENOMEM;
		if (*statement)
			mysql_stmt_close(*statement);
		*statement = nullptr;
		return false;
	}
	return true;
}

bool statement_ok(MYSQL_STMT *statement, bool ok)
{
	if (!ok)
	{
		const unsigned int error = mysql_stmt_errno(statement);
		errno = error ? static_cast<int>(error) : EIO;
	}
	mysql_stmt_close(statement);
	return ok;
}

bool ensure_owner(MYSQL *connection, const item_owner_identity &owner)
{
	static const char SQL[] =
		"INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(?,?,?,0)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[3] = {};
	bindings[0].buffer_type = MYSQL_TYPE_TINY;
	bindings[0].buffer = &type;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[2].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool lock_owner(MYSQL *connection, const item_owner_identity &owner, uint64_t *revision)
{
	static const char SQL[] =
		"SELECT revision FROM item_owner_revision WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!revision || !prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND parameters[3] = {};
	parameters[0].buffer_type = MYSQL_TYPE_TINY;
	parameters[0].buffer = &type;
	parameters[0].is_unsigned = true;
	parameters[1].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[1].buffer = const_cast<uint64_t *>(&owner.id);
	parameters[1].is_unsigned = true;
	parameters[2].buffer_type = MYSQL_TYPE_LONGLONG;
	parameters[2].buffer = const_cast<uint64_t *>(&owner.context_id);
	parameters[2].is_unsigned = true;
	MYSQL_BIND output = {};
	output.buffer_type = MYSQL_TYPE_LONGLONG;
	output.buffer = revision;
	output.is_unsigned = true;
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_bind_result(statement, &output) == 0 &&
			mysql_stmt_fetch(statement) == 0;
	return statement_ok(statement, ok);
}

bool owner_less(const item_owner_identity &left, const item_owner_identity &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.id != right.id)
		return left.id < right.id;
	return left.context_id < right.context_id;
}

bool load_root(MYSQL *connection, uint64_t root_item_uid, std::vector<current_item> *items)
{
	static const char SQL[] =
		"SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
		"item_revision,vnum,state FROM item_current_owner WHERE root_item_uid=? "
		"ORDER BY item_uid FOR UPDATE";
	if (!items)
		return false;
	items->clear();
	try
	{
		items->reserve(ITEM_TRANSFER_MAX_ITEMS + 1);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONGLONG;
	parameter.buffer = &root_item_uid;
	parameter.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
		return statement_ok(statement, false);
	current_item item = {};
	my_bool parent_null = 0;
	MYSQL_BIND output[9] = {};
	output[0].buffer_type = MYSQL_TYPE_LONGLONG;
	output[0].buffer = &item.item_uid;
	output[0].is_unsigned = true;
	output[1].buffer_type = MYSQL_TYPE_LONGLONG;
	output[1].buffer = &item.root_item_uid;
	output[1].is_unsigned = true;
	output[2].buffer_type = MYSQL_TYPE_LONGLONG;
	output[2].buffer = &item.parent_item_uid;
	output[2].is_unsigned = true;
	output[2].is_null = &parent_null;
	output[3].buffer_type = MYSQL_TYPE_TINY;
	output[3].buffer = &item.owner_type;
	output[3].is_unsigned = true;
	output[4].buffer_type = MYSQL_TYPE_LONGLONG;
	output[4].buffer = &item.owner_id;
	output[4].is_unsigned = true;
	output[5].buffer_type = MYSQL_TYPE_LONGLONG;
	output[5].buffer = &item.owner_context_id;
	output[5].is_unsigned = true;
	output[6].buffer_type = MYSQL_TYPE_LONGLONG;
	output[6].buffer = &item.item_revision;
	output[6].is_unsigned = true;
	output[7].buffer_type = MYSQL_TYPE_LONG;
	output[7].buffer = &item.vnum;
	output[8].buffer_type = MYSQL_TYPE_TINY;
	output[8].buffer = &item.state;
	output[8].is_unsigned = true;
	if (mysql_stmt_bind_result(statement, output) != 0)
		return statement_ok(statement, false);
	int fetched = 0;
	while ((fetched = mysql_stmt_fetch(statement)) == 0)
	{
		item.parent_item_uid = parent_null ? 0 : item.parent_item_uid;
		items->push_back(item);
		if (items->size() > ITEM_TRANSFER_MAX_ITEMS)
			break;
		item = {};
		parent_null = 0;
	}
	return statement_ok(statement,
			    fetched == MYSQL_NO_DATA || items->size() > ITEM_TRANSFER_MAX_ITEMS);
}

bool item_exists(MYSQL *connection, uint64_t item_uid, bool *found)
{
	static const char SQL[] =
		"SELECT item_uid FROM item_current_owner WHERE item_uid=? FOR UPDATE";
	if (!found)
		return false;
	*found = false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_LONGLONG;
	parameter.buffer = &item_uid;
	parameter.is_unsigned = true;
	uint64_t stored_uid = 0;
	MYSQL_BIND output = {};
	output.buffer_type = MYSQL_TYPE_LONGLONG;
	output.buffer = &stored_uid;
	output.is_unsigned = true;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_bind_result(statement, &output) != 0)
		return statement_ok(statement, false);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched != 0 && fetched != MYSQL_NO_DATA)
		return statement_ok(statement, false);
	*found = fetched == 0 && stored_uid == item_uid;
	return statement_ok(statement, true);
}

bool insert_created_item(MYSQL *connection, const item_transfer_entry &entry,
			 const item_owner_identity &owner)
{
	static const char SQL[] =
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(?,?,NULL,?,?,?,0,?,1)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[6] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = const_cast<uint64_t *>(&entry.item_uid);
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = const_cast<uint64_t *>(&entry.root_item_uid);
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_TINY;
	bindings[2].buffer = &type;
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[3].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[4].is_unsigned = true;
	bindings[5].buffer_type = MYSQL_TYPE_LONG;
	bindings[5].buffer = const_cast<int32_t *>(&entry.vnum);
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}

bool update_item(MYSQL *connection, const item_transfer_entry &entry,
		 const item_owner_identity &owner, uint64_t prior_revision,
		 item_custody_state state)
{
	static const char SQL[] =
		"UPDATE item_current_owner SET parent_item_uid=IF(?=0,NULL,?),owner_type=?,owner_id=?,"
		"owner_context_id=?,item_revision=?,state=? WHERE item_uid=? AND item_revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t type = static_cast<uint8_t>(owner.type);
	uint8_t state_value = static_cast<uint8_t>(state);
	uint64_t revision = prior_revision + 1;
	MYSQL_BIND bindings[9] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = const_cast<uint64_t *>(&entry.parent_item_uid);
	bindings[0].is_unsigned = true;
	bindings[1] = bindings[0];
	bindings[2].buffer_type = MYSQL_TYPE_TINY;
	bindings[2].buffer = &type;
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[3].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[4].is_unsigned = true;
	bindings[5].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[5].buffer = &revision;
	bindings[5].is_unsigned = true;
	bindings[6].buffer_type = MYSQL_TYPE_TINY;
	bindings[6].buffer = &state_value;
	bindings[6].is_unsigned = true;
	bindings[7].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[7].buffer = const_cast<uint64_t *>(&entry.item_uid);
	bindings[7].is_unsigned = true;
	bindings[8].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[8].buffer = &prior_revision;
	bindings[8].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0 &&
					       mysql_stmt_affected_rows(statement) == 1);
}

bool update_owner_revision(MYSQL *connection, const item_owner_identity &owner,
			   uint64_t prior_revision)
{
	static const char SQL[] =
		"UPDATE item_owner_revision SET revision=? WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=? AND revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint64_t revision = prior_revision + 1;
	uint8_t type = static_cast<uint8_t>(owner.type);
	MYSQL_BIND bindings[5] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = &revision;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_TINY;
	bindings[1].buffer = &type;
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[2].buffer = const_cast<uint64_t *>(&owner.id);
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[3].buffer = const_cast<uint64_t *>(&owner.context_id);
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[4].buffer = &prior_revision;
	bindings[4].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0 &&
					       mysql_stmt_affected_rows(statement) == 1);
}

bool insert_ledger(MYSQL *connection, const critical_command &command,
		   const item_transfer_payload &payload, size_t index, uint64_t item_revision,
		   uint64_t from_revision, uint64_t to_revision)
{
	static const char SQL[] =
		"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
		"parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,"
		"to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,"
		"reason_type,reason_id,source_site) VALUES(?,?,?,?,IF(?=0,NULL,?),?,?,?,?,?,?,?,?,?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	const item_transfer_entry &entry = payload.items[index];
	uint16_t event_index = static_cast<uint16_t>(index);
	uint8_t from_type = static_cast<uint8_t>(payload.from_owner.type);
	uint8_t to_type = static_cast<uint8_t>(payload.to_owner.type);
	uint16_t reason = static_cast<uint16_t>(payload.reason);
	uint16_t source = static_cast<uint16_t>(command.source_site);
	unsigned long operation_length = command.operation_id.bytes.size();
	MYSQL_BIND bindings[18] = {};
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	bindings[0].buffer_length = operation_length;
	bindings[0].length = &operation_length;
	bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	bindings[1].buffer = &event_index;
	bindings[1].is_unsigned = true;
	uint64_t *unsigned_values[] = {
		const_cast<uint64_t *>(&entry.item_uid),
		const_cast<uint64_t *>(&entry.root_item_uid),
		const_cast<uint64_t *>(&entry.parent_item_uid),
		const_cast<uint64_t *>(&entry.parent_item_uid),
	};
	for (size_t value = 0; value < 4; ++value)
	{
		bindings[value + 2].buffer_type = MYSQL_TYPE_LONGLONG;
		bindings[value + 2].buffer = unsigned_values[value];
		bindings[value + 2].is_unsigned = true;
	}
	bindings[6].buffer_type = MYSQL_TYPE_TINY;
	bindings[6].buffer = &from_type;
	bindings[6].is_unsigned = true;
	bindings[7].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[7].buffer = const_cast<uint64_t *>(&payload.from_owner.id);
	bindings[7].is_unsigned = true;
	bindings[8].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[8].buffer = const_cast<uint64_t *>(&payload.from_owner.context_id);
	bindings[8].is_unsigned = true;
	bindings[9].buffer_type = MYSQL_TYPE_TINY;
	bindings[9].buffer = &to_type;
	bindings[9].is_unsigned = true;
	bindings[10].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[10].buffer = const_cast<uint64_t *>(&payload.to_owner.id);
	bindings[10].is_unsigned = true;
	bindings[11].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[11].buffer = const_cast<uint64_t *>(&payload.to_owner.context_id);
	bindings[11].is_unsigned = true;
	bindings[12].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[12].buffer = &item_revision;
	bindings[12].is_unsigned = true;
	bindings[13].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[13].buffer = &from_revision;
	bindings[13].is_unsigned = true;
	bindings[14].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[14].buffer = &to_revision;
	bindings[14].is_unsigned = true;
	bindings[15].buffer_type = MYSQL_TYPE_SHORT;
	bindings[15].buffer = &reason;
	bindings[15].is_unsigned = true;
	bindings[16].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[16].buffer = const_cast<int64_t *>(&payload.reason_id);
	bindings[17].buffer_type = MYSQL_TYPE_SHORT;
	bindings[17].buffer = &source;
	bindings[17].is_unsigned = true;
	return statement_ok(statement, mysql_stmt_bind_param(statement, bindings) == 0 &&
					       mysql_stmt_execute(statement) == 0);
}
} // namespace

bool item_transfer_repository_execute(MYSQL *connection, const critical_command &command,
				      item_transfer_result *result, unsigned int *result_code,
				      bool *mutation_applied)
{
	item_transfer_payload payload = {};
	if (!connection || !result || !result_code || !mutation_applied ||
	    !item_transfer_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = { payload.items[0].root_item_uid, payload.item_count, 0, 0, 0 };
	*result_code = 0;
	*mutation_applied = false;
	uint64_t from_revision = 0, to_revision = 0;
	if (owner_less(payload.from_owner, payload.to_owner))
	{
		if (!ensure_owner(connection, payload.from_owner) ||
		    !ensure_owner(connection, payload.to_owner) ||
		    !lock_owner(connection, payload.from_owner, &from_revision) ||
		    !lock_owner(connection, payload.to_owner, &to_revision))
			return false;
	}
	else if (!ensure_owner(connection, payload.to_owner) ||
		 !ensure_owner(connection, payload.from_owner) ||
		 !lock_owner(connection, payload.to_owner, &to_revision) ||
		 !lock_owner(connection, payload.from_owner, &from_revision))
		return false;
	result->from_owner_revision = from_revision;
	result->to_owner_revision = to_revision;
	if (from_revision != payload.expected_from_revision ||
	    to_revision != payload.expected_to_revision)
	{
		*result_code = ESTALE;
		return true;
	}
	std::vector<current_item> current;
	if (!load_root(connection, payload.items[0].root_item_uid, &current))
		return false;
	const bool creation = payload.from_owner.type == item_owner_type::system;
	if ((creation && !current.empty()) || (!creation && current.empty()))
	{
		*result_code = creation ? EEXIST : ENOENT;
		return true;
	}
	if (creation)
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			bool found = false;
			if (!item_exists(connection, payload.items[index].item_uid, &found))
				return false;
			if (found)
			{
				*result_code = EEXIST;
				return true;
			}
		}
	if (!creation && current.size() != payload.item_count)
	{
		*result_code = EMSGSIZE;
		return true;
	}
	if (!creation)
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const current_item &stored = current[index];
			const item_transfer_entry &expected = payload.items[index];
			result->max_item_revision =
				std::max(result->max_item_revision, stored.item_revision);
			if (stored.item_uid != expected.item_uid ||
			    stored.root_item_uid != expected.root_item_uid ||
			    stored.parent_item_uid != expected.parent_item_uid ||
			    stored.owner_type != static_cast<uint8_t>(payload.from_owner.type) ||
			    stored.owner_id != payload.from_owner.id ||
			    stored.owner_context_id != payload.from_owner.context_id ||
			    stored.item_revision != expected.expected_item_revision ||
			    stored.vnum != expected.vnum ||
			    stored.state != static_cast<uint8_t>(expected.expected_state))
			{
				*result_code = ESTALE;
				return true;
			}
		}
	if (from_revision == std::numeric_limits<uint64_t>::max() ||
	    to_revision == std::numeric_limits<uint64_t>::max())
	{
		*result_code = ERANGE;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const uint64_t prior_revision = creation ? 0 : current[index].item_revision;
		if (prior_revision == std::numeric_limits<uint64_t>::max())
		{
			*result_code = ERANGE;
			return true;
		}
		if (creation &&
		    !insert_created_item(connection, payload.items[index], payload.to_owner))
			return false;
		if (!update_item(connection, payload.items[index], payload.to_owner, prior_revision,
				 payload.to_owner.type == item_owner_type::destruction ?
					 item_custody_state::destroyed :
					 item_custody_state::active))
			return false;
		const uint64_t item_revision = prior_revision + 1;
		result->max_item_revision = std::max(result->max_item_revision, item_revision);
		if (!insert_ledger(connection, command, payload, index, item_revision,
				   from_revision + 1, to_revision + 1))
			return false;
	}
	if (!update_owner_revision(connection, payload.from_owner, from_revision) ||
	    !update_owner_revision(connection, payload.to_owner, to_revision))
		return false;
	result->from_owner_revision = from_revision + 1;
	result->to_owner_revision = to_revision + 1;
	*mutation_applied = true;
	return true;
}
