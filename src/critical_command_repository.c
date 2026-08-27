#include "critical_command_repository.h"

#include "epic_command.h"
#include "sql_pool.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <new>
#include <openssl/sha.h>
#include <vector>

namespace
{
constexpr uint8_t INBOX_COMMITTED = 1;
constexpr uint16_t OUTBOX_DESTINATION_TEST = 1;
constexpr uint16_t OUTBOX_EVENT_TEST_MUTATED = 1;
constexpr uint16_t OUTBOX_DESTINATION_EPIC = 2;
constexpr uint16_t OUTBOX_EVENT_EPIC_BALANCE = 1;
thread_local unsigned int last_statement_error = 0;

struct stored_operation
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_hash;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> keys_hash;
	uint16_t command_type;
	uint32_t schema_version;
	uint16_t payload_version;
	uint8_t status;
	uint32_t result_code;
	uint64_t durable_revision;
	std::vector<uint8_t> result_payload;
};

bool connection_error(unsigned int error)
{
	return error == 2006 || error == 2013 || error == 2055;
}

bool retryable_error(unsigned int error)
{
	return connection_error(error) || error == 1205 || error == 1213;
}

critical_apply_result failure(unsigned int error)
{
	return { retryable_error(error) ? critical_apply_outcome::retryable_failure :
					  critical_apply_outcome::terminal_failure,
		 0, error };
}

critical_apply_result stored_result(critical_apply_outcome outcome, const stored_operation &stored)
{
	critical_apply_result result = { outcome, stored.durable_revision, stored.result_code };
	result.result_size = static_cast<uint16_t>(
		std::min(stored.result_payload.size(), result.result_payload.size()));
	std::copy_n(stored.result_payload.begin(), result.result_size,
		    result.result_payload.begin());
	return result;
}

bool execute(MYSQL *connection, const char *sql)
{
	return mysql_real_query(connection, sql, strlen(sql)) == 0;
}

void rollback(MYSQL *connection)
{
	if (connection && !connection_error(mysql_errno(connection)))
		execute(connection, "ROLLBACK");
}

bool prepare(MYSQL_STMT **statement, MYSQL *connection, const char *sql)
{
	last_statement_error = 0;
	*statement = mysql_stmt_init(connection);
	if (!*statement)
		return false;
	if (mysql_stmt_prepare(*statement, sql, strlen(sql)) != 0)
	{
		last_statement_error = mysql_stmt_errno(*statement);
		mysql_stmt_close(*statement);
		*statement = nullptr;
		return false;
	}
	return true;
}

bool statement_failure(MYSQL_STMT *statement)
{
	if (statement)
	{
		last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
	}
	return false;
}

unsigned int database_error(MYSQL *connection)
{
	return last_statement_error ? last_statement_error : mysql_errno(connection);
}

std::array<uint8_t, SHA256_DIGEST_LENGTH> hash_bytes(const uint8_t *data, size_t size)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(data, size, digest.data());
	return digest;
}

bool command_hashes(const critical_command &command,
		    std::array<uint8_t, SHA256_DIGEST_LENGTH> *command_hash,
		    std::array<uint8_t, SHA256_DIGEST_LENGTH> *keys_hash)
{
	std::vector<uint8_t> encoded;
	if (!command_hash || !keys_hash ||
	    critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return false;
	*command_hash = hash_bytes(encoded.data(), encoded.size());
	std::vector<uint8_t> keys;
	try
	{
		keys.reserve(command.keys.size() * 9);
		for (const critical_entity_key &key : command.keys)
		{
			keys.push_back(static_cast<uint8_t>(key.type));
			for (unsigned int byte = 0; byte < 8; ++byte)
				keys.push_back(static_cast<uint8_t>(key.id >> (byte * 8)));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*keys_hash = hash_bytes(keys.data(), keys.size());
	return true;
}

bool read_operation(MYSQL *connection, const critical_operation_id &operation_id, bool for_update,
		    stored_operation *stored, bool *found)
{
	static const char SELECT_SQL[] =
		"SELECT command_hash,keys_hash,command_type,schema_version,payload_version,status,"
		"result_code,durable_revision,result_payload FROM critical_operation_inbox "
		"WHERE operation_id=?";
	static const char SELECT_LOCK_SQL[] =
		"SELECT command_hash,keys_hash,command_type,schema_version,payload_version,status,"
		"result_code,durable_revision,result_payload FROM critical_operation_inbox "
		"WHERE operation_id=? FOR UPDATE";
	if (!stored || !found)
		return false;
	*found = false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, for_update ? SELECT_LOCK_SQL : SELECT_SQL))
		return false;
	unsigned long operation_length = operation_id.bytes.size();
	MYSQL_BIND parameter = {};
	parameter.buffer_type = MYSQL_TYPE_BLOB;
	parameter.buffer = const_cast<uint8_t *>(operation_id.bytes.data());
	parameter.buffer_length = operation_length;
	parameter.length = &operation_length;
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
		return statement_failure(statement);
	std::array<uint8_t, CRITICAL_COMMAND_RESULT_MAX_BYTES> result_payload = {};
	unsigned long command_hash_length = 0, keys_hash_length = 0, result_length = 0;
	MYSQL_BIND results[9] = {};
	results[0].buffer_type = MYSQL_TYPE_BLOB;
	results[0].buffer = stored->command_hash.data();
	results[0].buffer_length = stored->command_hash.size();
	results[0].length = &command_hash_length;
	results[1].buffer_type = MYSQL_TYPE_BLOB;
	results[1].buffer = stored->keys_hash.data();
	results[1].buffer_length = stored->keys_hash.size();
	results[1].length = &keys_hash_length;
	results[2].buffer_type = MYSQL_TYPE_SHORT;
	results[2].buffer = &stored->command_type;
	results[2].is_unsigned = true;
	results[3].buffer_type = MYSQL_TYPE_LONG;
	results[3].buffer = &stored->schema_version;
	results[3].is_unsigned = true;
	results[4].buffer_type = MYSQL_TYPE_SHORT;
	results[4].buffer = &stored->payload_version;
	results[4].is_unsigned = true;
	results[5].buffer_type = MYSQL_TYPE_TINY;
	results[5].buffer = &stored->status;
	results[5].is_unsigned = true;
	results[6].buffer_type = MYSQL_TYPE_LONG;
	results[6].buffer = &stored->result_code;
	results[6].is_unsigned = true;
	results[7].buffer_type = MYSQL_TYPE_LONGLONG;
	results[7].buffer = &stored->durable_revision;
	results[7].is_unsigned = true;
	results[8].buffer_type = MYSQL_TYPE_BLOB;
	results[8].buffer = result_payload.data();
	results[8].buffer_length = result_payload.size();
	results[8].length = &result_length;
	if (mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		return true;
	}
	if (fetched != 0 || command_hash_length != stored->command_hash.size() ||
	    keys_hash_length != stored->keys_hash.size() || result_length > result_payload.size())
		return statement_failure(statement);
	stored->result_payload.assign(result_payload.begin(),
				      result_payload.begin() + result_length);
	*found = true;
	mysql_stmt_close(statement);
	return true;
}

bool identity_matches(const stored_operation &stored, const critical_command &command,
		      const std::array<uint8_t, SHA256_DIGEST_LENGTH> &command_hash,
		      const std::array<uint8_t, SHA256_DIGEST_LENGTH> &keys_hash)
{
	return stored.command_hash == command_hash && stored.keys_hash == keys_hash &&
	       stored.command_type == static_cast<uint16_t>(command.type) &&
	       stored.schema_version == command.schema_version &&
	       stored.payload_version == command.payload_version;
}

bool insert_inbox(MYSQL *connection, const critical_command &command,
		  const std::array<uint8_t, SHA256_DIGEST_LENGTH> &command_hash,
		  const std::array<uint8_t, SHA256_DIGEST_LENGTH> &keys_hash)
{
	static const char SQL[] =
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload) "
		"VALUES(?,?,?,?,?,?,0,'')";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint16_t type = static_cast<uint16_t>(command.type);
	uint32_t schema = command.schema_version;
	uint16_t payload_version = command.payload_version;
	unsigned long lengths[3] = { command.operation_id.bytes.size(), command_hash.size(),
				     keys_hash.size() };
	MYSQL_BIND bindings[6] = {};
	for (unsigned int index = 0; index < 3; ++index)
	{
		bindings[index].buffer_type = MYSQL_TYPE_BLOB;
		bindings[index].length = &lengths[index];
	}
	bindings[0].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	bindings[0].buffer_length = lengths[0];
	bindings[1].buffer = const_cast<uint8_t *>(command_hash.data());
	bindings[1].buffer_length = lengths[1];
	bindings[2].buffer = const_cast<uint8_t *>(keys_hash.data());
	bindings[2].buffer_length = lengths[2];
	bindings[3].buffer_type = MYSQL_TYPE_SHORT;
	bindings[3].buffer = &type;
	bindings[3].is_unsigned = true;
	bindings[4].buffer_type = MYSQL_TYPE_LONG;
	bindings[4].buffer = &schema;
	bindings[4].is_unsigned = true;
	bindings[5].buffer_type = MYSQL_TYPE_SHORT;
	bindings[5].buffer = &payload_version;
	bindings[5].is_unsigned = true;
	const bool ok = mysql_stmt_bind_param(statement, bindings) == 0 &&
			mysql_stmt_execute(statement) == 0;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool execute_entity(MYSQL *connection, const critical_entity_key &key, int64_t delta,
		    int64_t *value, uint64_t *revision)
{
	static const char ENSURE_SQL[] =
		"INSERT IGNORE INTO critical_test_state(entity_type,entity_id) VALUES(?,?)";
	static const char LOCK_SQL[] =
		"SELECT value,revision FROM critical_test_state WHERE entity_type=? AND entity_id=? "
		"FOR UPDATE";
	static const char UPDATE_SQL[] =
		"UPDATE critical_test_state SET value=?,revision=? WHERE entity_type=? AND entity_id=?";
	uint8_t type = static_cast<uint8_t>(key.type);
	uint64_t id = key.id;
	MYSQL_BIND key_bindings[2] = {};
	key_bindings[0].buffer_type = MYSQL_TYPE_TINY;
	key_bindings[0].buffer = &type;
	key_bindings[0].is_unsigned = true;
	key_bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	key_bindings[1].buffer = &id;
	key_bindings[1].is_unsigned = true;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, ENSURE_SQL) ||
	    mysql_stmt_bind_param(statement, key_bindings) != 0 ||
	    mysql_stmt_execute(statement) != 0)
	{
		return statement_failure(statement);
	}
	mysql_stmt_close(statement);
	if (!prepare(&statement, connection, LOCK_SQL) ||
	    mysql_stmt_bind_param(statement, key_bindings) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
	{
		return statement_failure(statement);
	}
	MYSQL_BIND results[2] = {};
	results[0].buffer_type = MYSQL_TYPE_LONGLONG;
	results[0].buffer = value;
	results[1].buffer_type = MYSQL_TYPE_LONGLONG;
	results[1].buffer = revision;
	results[1].is_unsigned = true;
	const bool read = mysql_stmt_bind_result(statement, results) == 0 &&
			  mysql_stmt_fetch(statement) == 0;
	if (!read)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	if (!read)
		return false;
	if ((delta > 0 && *value > std::numeric_limits<int64_t>::max() - delta) ||
	    (delta < 0 && *value < std::numeric_limits<int64_t>::min() - delta) ||
	    *revision == std::numeric_limits<uint64_t>::max())
	{
		errno = ERANGE;
		return false;
	}
	*value += delta;
	++*revision;
	MYSQL_BIND update[4] = {};
	update[0].buffer_type = MYSQL_TYPE_LONGLONG;
	update[0].buffer = value;
	update[1].buffer_type = MYSQL_TYPE_LONGLONG;
	update[1].buffer = revision;
	update[1].is_unsigned = true;
	update[2] = key_bindings[0];
	update[3] = key_bindings[1];
	if (!prepare(&statement, connection, UPDATE_SQL) ||
	    mysql_stmt_bind_param(statement, update) != 0 || mysql_stmt_execute(statement) != 0 ||
	    mysql_stmt_affected_rows(statement) != 1)
	{
		return statement_failure(statement);
	}
	mysql_stmt_close(statement);
	return true;
}

void encode_u64(std::array<uint8_t, 16> *output, size_t offset, uint64_t value)
{
	for (unsigned int byte = 0; byte < 8; ++byte)
		(*output)[offset + byte] = static_cast<uint8_t>(value >> (byte * 8));
}

bool insert_outbox(MYSQL *connection, const critical_command &command, const uint8_t *payload,
		   size_t payload_size)
{
	static const char SQL[] =
		"INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,"
		"payload_version,payload) VALUES(?,0,?,?,1,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint16_t destination = command.type == critical_command_type::epic ?
				       OUTBOX_DESTINATION_EPIC :
				       OUTBOX_DESTINATION_TEST;
	uint16_t event_type = command.type == critical_command_type::epic ?
				      OUTBOX_EVENT_EPIC_BALANCE :
				      OUTBOX_EVENT_TEST_MUTATED;
	unsigned long operation_length = command.operation_id.bytes.size(),
		      payload_length = payload_size;
	MYSQL_BIND bindings[4] = {};
	bindings[0].buffer_type = MYSQL_TYPE_BLOB;
	bindings[0].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	bindings[0].buffer_length = operation_length;
	bindings[0].length = &operation_length;
	bindings[1].buffer_type = MYSQL_TYPE_SHORT;
	bindings[1].buffer = &destination;
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_SHORT;
	bindings[2].buffer = &event_type;
	bindings[2].is_unsigned = true;
	bindings[3].buffer_type = MYSQL_TYPE_BLOB;
	bindings[3].buffer = const_cast<uint8_t *>(payload);
	bindings[3].buffer_length = payload_length;
	bindings[3].length = &payload_length;
	const bool ok = mysql_stmt_bind_param(statement, bindings) == 0 &&
			mysql_stmt_execute(statement) == 0;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool finish_inbox(MYSQL *connection, const critical_command &command, uint64_t revision,
		  unsigned int result_code, const uint8_t *payload, size_t payload_size)
{
	static const char SQL[] =
		"UPDATE critical_operation_inbox SET status=1,result_code=?,durable_revision=?,"
		"result_payload=?,committed_at=CURRENT_TIMESTAMP(6) WHERE operation_id=? AND status=0";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	unsigned long payload_length = payload_size,
		      operation_length = command.operation_id.bytes.size();
	MYSQL_BIND bindings[4] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONG;
	bindings[0].buffer = &result_code;
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = &revision;
	bindings[1].is_unsigned = true;
	bindings[2].buffer_type = MYSQL_TYPE_BLOB;
	bindings[2].buffer = const_cast<uint8_t *>(payload);
	bindings[2].buffer_length = payload_length;
	bindings[2].length = &payload_length;
	bindings[3].buffer_type = MYSQL_TYPE_BLOB;
	bindings[3].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	bindings[3].buffer_length = operation_length;
	bindings[3].length = &operation_length;
	const bool ok = mysql_stmt_bind_param(statement, bindings) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_affected_rows(statement) == 1;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool execute_epic_state(MYSQL *connection, const critical_command &command,
			epic_command_result *result, unsigned int *result_code,
			bool *mutation_applied)
{
	static const char BASELINE_SQL[] =
		"INSERT IGNORE INTO epic_balance_baseline(pid,opening_balance,opening_revision) "
		"SELECT pid,epics,epic_revision FROM player_data WHERE pid=?";
	static const char LOCK_SQL[] =
		"SELECT epics,epic_revision FROM player_data WHERE pid=? FOR UPDATE";
	static const char UPDATE_SQL[] =
		"UPDATE player_data SET epics=?,epic_revision=? WHERE pid=? AND epic_revision=?";
	static const char LEDGER_SQL[] =
		"INSERT INTO epic_ledger(operation_id,pid,delta,balance_after,epic_revision,"
		"reason_type,reason_id,source_site) VALUES(?,?,?,?,?,?,?,?)";
	if (!result || !result_code || !mutation_applied)
		return false;
	epic_command_payload payload = {};
	if (!epic_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	uint32_t pid = payload.pid;
	MYSQL_BIND pid_binding = {};
	pid_binding.buffer_type = MYSQL_TYPE_LONG;
	pid_binding.buffer = &pid;
	pid_binding.is_unsigned = true;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, BASELINE_SQL) ||
	    mysql_stmt_bind_param(statement, &pid_binding) != 0 ||
	    mysql_stmt_execute(statement) != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	if (!prepare(&statement, connection, LOCK_SQL) ||
	    mysql_stmt_bind_param(statement, &pid_binding) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0)
		return statement_failure(statement);
	int64_t balance = 0;
	uint64_t revision = 0;
	MYSQL_BIND state[2] = {};
	state[0].buffer_type = MYSQL_TYPE_LONGLONG;
	state[0].buffer = &balance;
	state[1].buffer_type = MYSQL_TYPE_LONGLONG;
	state[1].buffer = &revision;
	state[1].is_unsigned = true;
	const bool found = mysql_stmt_bind_result(statement, state) == 0 &&
			   mysql_stmt_fetch(statement) == 0;
	if (!found)
		last_statement_error = mysql_stmt_errno(statement) ? mysql_stmt_errno(statement) :
								     ENOENT;
	mysql_stmt_close(statement);
	if (!found)
		return false;
	*result = { .balance = balance, .revision = revision, .delta = payload.delta };
	const uint64_t expected = command.expected_revisions[0].revision;
	if (expected != std::numeric_limits<uint64_t>::max() && expected != revision)
	{
		*result_code = ESTALE;
		*mutation_applied = false;
		return true;
	}
	if (payload.delta < 0 && (payload.flags & EPIC_COMMAND_REQUIRE_FUNDS) &&
	    balance < -payload.delta)
	{
		*result_code = ENOSPC;
		*mutation_applied = false;
		return true;
	}
	if ((payload.delta > 0 && balance > std::numeric_limits<int64_t>::max() - payload.delta) ||
	    (payload.delta < 0 && balance < std::numeric_limits<int64_t>::min() - payload.delta) ||
	    revision == std::numeric_limits<uint64_t>::max())
	{
		*result_code = ERANGE;
		*mutation_applied = false;
		return true;
	}
	uint64_t prior_revision = revision;
	balance += payload.delta;
	++revision;
	MYSQL_BIND update[4] = {};
	update[0].buffer_type = MYSQL_TYPE_LONGLONG;
	update[0].buffer = &balance;
	update[1].buffer_type = MYSQL_TYPE_LONGLONG;
	update[1].buffer = &revision;
	update[1].is_unsigned = true;
	update[2] = pid_binding;
	update[3].buffer_type = MYSQL_TYPE_LONGLONG;
	update[3].buffer = &prior_revision;
	update[3].is_unsigned = true;
	if (!prepare(&statement, connection, UPDATE_SQL) ||
	    mysql_stmt_bind_param(statement, update) != 0 || mysql_stmt_execute(statement) != 0 ||
	    mysql_stmt_affected_rows(statement) != 1)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	uint16_t reason = static_cast<uint16_t>(payload.reason);
	uint16_t source = static_cast<uint16_t>(command.source_site);
	unsigned long operation_length = command.operation_id.bytes.size();
	MYSQL_BIND ledger[8] = {};
	ledger[0].buffer_type = MYSQL_TYPE_BLOB;
	ledger[0].buffer = const_cast<uint8_t *>(command.operation_id.bytes.data());
	ledger[0].buffer_length = operation_length;
	ledger[0].length = &operation_length;
	ledger[1] = pid_binding;
	ledger[2].buffer_type = MYSQL_TYPE_LONGLONG;
	ledger[2].buffer = &payload.delta;
	ledger[3].buffer_type = MYSQL_TYPE_LONGLONG;
	ledger[3].buffer = &balance;
	ledger[4].buffer_type = MYSQL_TYPE_LONGLONG;
	ledger[4].buffer = &revision;
	ledger[4].is_unsigned = true;
	ledger[5].buffer_type = MYSQL_TYPE_SHORT;
	ledger[5].buffer = &reason;
	ledger[5].is_unsigned = true;
	ledger[6].buffer_type = MYSQL_TYPE_LONGLONG;
	ledger[6].buffer = &payload.reason_id;
	ledger[7].buffer_type = MYSQL_TYPE_SHORT;
	ledger[7].buffer = &source;
	ledger[7].is_unsigned = true;
	if (!prepare(&statement, connection, LEDGER_SQL) ||
	    mysql_stmt_bind_param(statement, ledger) != 0 || mysql_stmt_execute(statement) != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	*result = { .balance = balance, .revision = revision, .delta = payload.delta };
	*result_code = 0;
	*mutation_applied = true;
	return true;
}
} // namespace

critical_apply_result critical_command_repository_apply(MYSQL *connection,
							const critical_command &command)
{
	last_statement_error = 0;
	epic_command_payload epic_payload = {};
	const bool test_command = command.type == critical_command_type::test &&
				  command.payload.size() == 8;
	const bool epic_command = epic_command_decode_payload(command, &epic_payload);
	if (!connection || (!test_command && !epic_command) || !critical_command_valid(command))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_hash = {}, keys_hash = {};
	if (!command_hashes(command, &command_hash, &keys_hash))
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	if (!execute(connection, "START TRANSACTION"))
		return failure(mysql_errno(connection));
	if (!insert_inbox(connection, command, command_hash, keys_hash))
	{
		const unsigned int error = database_error(connection);
		if (error == 1062)
		{
			stored_operation stored = {};
			bool found = false;
			if (!read_operation(connection, command.operation_id, true, &stored,
					    &found))
			{
				const unsigned int read_error = database_error(connection);
				rollback(connection);
				return failure(read_error);
			}
			rollback(connection);
			if (!found || stored.status != INBOX_COMMITTED)
				return { critical_apply_outcome::retryable_failure, 0, EAGAIN };
			if (!identity_matches(stored, command, command_hash, keys_hash))
				return { critical_apply_outcome::terminal_failure, 0, EEXIST };
			return stored_result(stored.result_code ?
						     critical_apply_outcome::terminal_failure :
						     critical_apply_outcome::already_applied,
					     stored);
		}
		rollback(connection);
		return failure(error);
	}
	if (epic_command)
	{
		epic_command_result epic_result = {};
		unsigned int result_code = 0;
		bool mutation_applied = false;
		if (!execute_epic_state(connection, command, &epic_result, &result_code,
					&mutation_applied))
		{
			const unsigned int database_failure = database_error(connection);
			const unsigned int error = database_failure ? database_failure : errno;
			rollback(connection);
			return failure(error);
		}
		std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> result_payload = {};
		if (!epic_command_encode_result(epic_result, &result_payload) ||
		    (mutation_applied && !insert_outbox(connection, command, result_payload.data(),
							result_payload.size())) ||
		    !finish_inbox(connection, command, epic_result.revision, result_code,
				  result_payload.data(), result_payload.size()))
		{
			const unsigned int database_failure = database_error(connection);
			const unsigned int error = database_failure ? database_failure : errno;
			rollback(connection);
			return failure(error);
		}
		if (!execute(connection, "COMMIT"))
		{
			const unsigned int error = mysql_errno(connection);
			if (!connection_error(error))
				rollback(connection);
			return { connection_error(error) ?
					 critical_apply_outcome::ambiguous_commit :
					 failure(error).outcome,
				 epic_result.revision, error };
		}
		critical_apply_result applied = { result_code ?
							  critical_apply_outcome::terminal_failure :
							  critical_apply_outcome::applied,
						  epic_result.revision, result_code };
		applied.result_size = result_payload.size();
		std::copy(result_payload.begin(), result_payload.end(),
			  applied.result_payload.begin());
		return applied;
	}
	uint64_t delta_bits = 0;
	for (unsigned int byte = 0; byte < 8; ++byte)
		delta_bits |= static_cast<uint64_t>(command.payload[byte]) << (byte * 8);
	const int64_t delta = static_cast<int64_t>(delta_bits);
	if (!delta)
	{
		rollback(connection);
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	}
	int64_t aggregate = 0;
	uint64_t durable_revision = 0;
	for (const critical_entity_key &key : command.keys)
	{
		int64_t value = 0;
		uint64_t revision = 0;
		if (!execute_entity(connection, key, delta, &value, &revision))
		{
			const unsigned int database_failure = database_error(connection);
			const unsigned int error = database_failure ? database_failure : errno;
			rollback(connection);
			return failure(error);
		}
		aggregate ^= value;
		durable_revision = std::max(durable_revision, revision);
	}
	std::array<uint8_t, 16> result_payload = {};
	encode_u64(&result_payload, 0, static_cast<uint64_t>(aggregate));
	encode_u64(&result_payload, 8, durable_revision);
	if (!insert_outbox(connection, command, result_payload.data(), result_payload.size()) ||
	    !finish_inbox(connection, command, durable_revision, 0, result_payload.data(),
			  result_payload.size()))
	{
		const unsigned int error = database_error(connection);
		rollback(connection);
		return failure(error);
	}
	if (!execute(connection, "COMMIT"))
	{
		const unsigned int error = mysql_errno(connection);
		if (!connection_error(error))
			rollback(connection);
		return { connection_error(error) ? critical_apply_outcome::ambiguous_commit :
						   failure(error).outcome,
			 durable_revision, error };
	}
	critical_apply_result applied = { critical_apply_outcome::applied, durable_revision, 0 };
	applied.result_size = result_payload.size();
	std::copy(result_payload.begin(), result_payload.end(), applied.result_payload.begin());
	return applied;
}

critical_apply_result critical_command_repository_apply_from_pool(const critical_command &command,
								  void *context)
{
	(void)context;
	if (mysql_thread_init() != 0)
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		mysql_thread_end();
		return { critical_apply_outcome::retryable_failure, 0, ETIMEDOUT };
	}
	critical_apply_result applied = critical_command_repository_apply(connection, command);
	if (applied.outcome == critical_apply_outcome::ambiguous_commit ||
	    connection_error(applied.error_code))
	{
		MYSQL *replacement = sql_pool_replace_connection(connection);
		connection = replacement;
	}
	if (applied.outcome == critical_apply_outcome::ambiguous_commit && connection)
		applied = critical_command_repository_reconcile(connection, command);
	sql_pool_release(connection);
	mysql_thread_end();
	return applied;
}

critical_apply_result critical_command_repository_reconcile(MYSQL *connection,
							    const critical_command &command)
{
	last_statement_error = 0;
	if (!connection || !critical_command_valid(command))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_hash = {}, keys_hash = {};
	stored_operation stored = {};
	bool found = false;
	if (!command_hashes(command, &command_hash, &keys_hash))
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	if (!read_operation(connection, command.operation_id, false, &stored, &found))
		return failure(database_error(connection));
	if (!found || stored.status != INBOX_COMMITTED)
		return { critical_apply_outcome::retryable_failure, 0, EAGAIN };
	if (!identity_matches(stored, command, command_hash, keys_hash))
		return { critical_apply_outcome::terminal_failure, 0, EEXIST };
	return stored_result(stored.result_code ? critical_apply_outcome::terminal_failure :
						  critical_apply_outcome::already_applied,
			     stored);
}
