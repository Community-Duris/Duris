#include "player/player_death_conflict_repository.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "sql/sql_pool.h"
#include "persistence/persistence_observability.h"
#include "persistence/economic_sql_lifecycle_guard.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <new>
#include <openssl/sha.h>
#include <set>
#include <string_view>
#include <tuple>
#include <type_traits>

#ifndef __NO_MYSQL__
namespace
{
using outcome = player_death_conflict_outcome;
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
unsigned int sql_error(MYSQL *connection)
{
	return mysql_errno(connection) ? mysql_errno(connection) : EIO;
}
void execute(MYSQL *connection, const std::string &sql)
{
	const auto started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql.data(), sql.size());
	const unsigned int code = rc ? sql_error(connection) : 0;
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_SAVE_WORKER,
				 persistence_statement_kind_from_sql(sql.c_str()),
				 persistence_observability_now_usec() - started, !rc, code,
				 rc ? mysql_sqlstate(connection) : "00000");
	if (rc)
		throw failure{ code };
}
void active(MYSQL *connection, unsigned long session)
{
	require(mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
}
void idle(MYSQL *connection)
{
	require(connection, EINVAL);
	require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
		EBUSY);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect,
		EINVAL);
}
struct transaction
{
	MYSQL *connection;
	unsigned long session;
	bool pending = false;
	explicit transaction(MYSQL *input)
		: connection(input)
		, session(mysql_thread_id(input))
	{
		execute(connection, "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE");
		try
		{
			execute(connection, "START TRANSACTION");
			pending = true;
			active(connection, session);
		}
		catch (...)
		{
			// A throwing constructor has no destructor; also close a START
			// whose reply was lost, without ever enabling automatic reconnect.
			if (mysql_thread_id(connection) == session)
				(void)mysql_query(connection, "ROLLBACK");
			throw;
		}
	}
	~transaction()
	{
		if (pending && mysql_thread_id(connection) == session)
			(void)mysql_query(connection, "ROLLBACK");
	}
};
std::string hex(std::string_view bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string output;
	output.reserve(bytes.size() * 2);
	for (unsigned char byte : bytes)
	{
		output.push_back(digits[byte >> 4]);
		output.push_back(digits[byte & 15]);
	}
	return output;
}
template <typename T> std::string bytes(const T &value)
{
	return { reinterpret_cast<const char *>(value.data()), value.size() };
}
std::string digest(std::string_view value)
{
	std::array<unsigned char, SHA256_DIGEST_LENGTH> output = {};
	require(SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(),
		       output.data()),
		EIO);
	return bytes(output);
}
uint64_t number(std::string_view value)
{
	uint64_t output = 0;
	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), output);
	require(!value.empty() && parsed.ec == std::errc{} &&
		parsed.ptr == value.data() + value.size());
	return output;
}
std::string wire(const player_snapshot &snapshot)
{
	std::vector<uint8_t> output;
	const auto result = player_snapshot_encode(snapshot, &output);
	require(result == player_snapshot_codec_result::ok,
		result == player_snapshot_codec_result::limit_exceeded	   ? E2BIG :
		result == player_snapshot_codec_result::allocation_failure ? ENOMEM :
									     EINVAL);
	return bytes(output);
}
void valid_request(const player_snapshot &snapshot)
{
	require(snapshot.pid > 0 && snapshot.revision && snapshot.death &&
			player_snapshot_is_death_request_schema(snapshot.schema_version) &&
			snapshot.components == PLAYER_CHECKPOINT_COMPONENT_ALL &&
			snapshot.items.empty() && !snapshot.death->conflict_evidence,
		EINVAL);
}
struct budget
{
	size_t rows = 0;
	size_t remaining = PLAYER_SNAPSHOT_MAX_BYTES;
	void consume(size_t count)
	{
		require(count <= remaining, E2BIG);
		remaining -= count;
	}
};
// Read SQL result bytes, not converted game objects. CAST AS BINARY avoids the
// connection character set rewriting text; lengths preserve embedded NULs.
// All metadata and source ranges are locked until the immutable insert commits.
player_death_evidence_table capture(MYSQL *connection, unsigned long session,
				    const std::string &table, const std::string &predicate,
				    const std::string &order, budget &bound)
{
	player_death_evidence_table output;
	execute(connection, "SELECT * FROM " + table + " LIMIT 0");
	result_ptr metadata(mysql_store_result(connection), mysql_free_result);
	require(bool(metadata), sql_error(connection));
	const size_t count = mysql_num_fields(metadata.get());
	require(count && count <= PLAYER_DEATH_EVIDENCE_MAX_COLUMNS, E2BIG);
	const auto fields = mysql_fetch_fields(metadata.get());
	require(fields);
	std::string selection, lengths;
	bound.consume(2 * sizeof(uint32_t));
	for (size_t index = 0; index < count; ++index)
	{
		std::string name(fields[index].name, fields[index].name_length);
		require(!name.empty() &&
			name.size() <= PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES &&
			std::all_of(name.begin(), name.end(),
				    [](unsigned char c)
				    {
					    return (c >= 'a' && c <= 'z') ||
						   (c >= 'A' && c <= 'Z') ||
						   (c >= '0' && c <= '9') || c == '_';
				    }));
		const auto expression = "CAST(`" + name + "` AS BINARY)";
		selection += (index ? "," : "") + expression;
		lengths += (index ? "+" : "") + std::string("COALESCE(OCTET_LENGTH(") + expression +
			   "),0)";
		bound.consume(sizeof(uint32_t) + name.size());
		output.columns.push_back(std::move(name));
	}
	metadata.reset();
	const auto from = " FROM " + table + " WHERE " + predicate;
	// Bound result allocation before transferring a MEDIUMBLOB or a large set.
	execute(connection,
		"SELECT COUNT(*),COALESCE(SUM(" + lengths + "),0)" + from + " FOR UPDATE");
	result_ptr size_result(mysql_store_result(connection), mysql_free_result);
	require(bool(size_result), sql_error(connection));
	require(mysql_num_fields(size_result.get()) == 2 && mysql_num_rows(size_result.get()) == 1);
	const auto sizes = mysql_fetch_row(size_result.get());
	const auto size_lengths = mysql_fetch_lengths(size_result.get());
	require(sizes && size_lengths && sizes[0] && sizes[1]);
	const auto row_count = number({ sizes[0], size_lengths[0] });
	const auto cell_bytes = number({ sizes[1], size_lengths[1] });
	require(row_count <= PLAYER_SNAPSHOT_MAX_ROWS - bound.rows && cell_bytes <= bound.remaining,
		E2BIG);
	size_result.reset();
	active(connection, session);
	execute(connection, "SELECT " + selection + from + " ORDER BY " + order + " LIMIT " +
				    std::to_string(PLAYER_SNAPSHOT_MAX_ROWS + 1) + " FOR UPDATE");
	result_ptr result(mysql_use_result(connection), mysql_free_result);
	require(bool(result), sql_error(connection));
	require(mysql_num_fields(result.get()) == count);
	while (const auto row = mysql_fetch_row(result.get()))
	{
		require(bound.rows < PLAYER_SNAPSHOT_MAX_ROWS && output.rows.size() < row_count,
			E2BIG);
		const auto field_lengths = mysql_fetch_lengths(result.get());
		require(field_lengths);
		player_death_evidence_row captured;
		captured.reserve(count);
		for (size_t column = 0; column < count; ++column)
		{
			bound.consume(sizeof(uint8_t));
			if (row[column])
			{
				bound.consume(sizeof(uint32_t) + field_lengths[column]);
				captured.emplace_back(
					std::string(row[column], field_lengths[column]));
			}
			else
				captured.emplace_back(std::nullopt);
		}
		output.rows.push_back(std::move(captured));
		++bound.rows;
	}
	require(!mysql_errno(connection), sql_error(connection));
	require(output.rows.size() == row_count);
	result.reset();
	active(connection, session);
	return output;
}
size_t column(const player_death_evidence_table &table, const char *name)
{
	const auto found = std::find(table.columns.begin(), table.columns.end(), name);
	require(found != table.columns.end());
	return static_cast<size_t>(found - table.columns.begin());
}
void add_ids(const player_death_evidence_table &table, const char *name, std::set<uint64_t> &ids)
{
	const auto index = column(table, name);
	for (const auto &row : table.rows)
		if (row[index])
		{
			const auto value = number(*row[index]);
			if (value)
				ids.insert(value);
		}
	require(ids.size() <= PLAYER_SNAPSHOT_MAX_ROWS, E2BIG);
}
std::string id_list(const std::set<uint64_t> &ids)
{
	std::string output = "0";
	for (auto value : ids)
		output += ',' + std::to_string(value);
	return output;
}
player_death_conflict_evidence observations(MYSQL *connection, unsigned long session,
					    const player_snapshot &request, size_t request_bytes)
{
	budget bound;
	bound.consume(request_bytes);
	player_death_conflict_evidence evidence;
	const auto pid = std::to_string(request.pid);
	evidence.player_items =
		capture(connection, session, "player_items", "pid=" + pid, "id", bound);
	std::set<uint64_t> ids;
	add_ids(evidence.player_items, "id", ids);
	const auto item_ids = id_list(ids);
	evidence.player_item_affects = capture(connection, session, "player_item_affects",
					       "item_id IN (" + item_ids + ")", "id", bound);
	evidence.player_item_extra_descr = capture(connection, session, "player_item_extra_descr",
						   "item_id IN (" + item_ids + ")", "id", bound);
	ids.clear();
	add_ids(evidence.player_items, "obj_uid", ids);
	for (const auto &item : request.death->corpse)
		if (item.object_uid)
			ids.insert(item.object_uid);
	for (const auto &row : request.death->custody)
		for (const auto uid :
		     { row.item.item_uid, row.item.root_item_uid, row.item.parent_item_uid })
			if (uid)
				ids.insert(uid);
	const auto player_owner =
		"owner_type=" + std::to_string(static_cast<unsigned>(item_owner_type::player)) +
		" AND owner_id=" + pid + " AND owner_context_id=0";
	// Keep a bounded closure of referenced roots/parents and their descendants,
	// including conflicting foreign custody. Never fetch foreign item payloads.
	const budget before_custody = bound;
	for (size_t depth = 0;; ++depth)
	{
		require(depth < PLAYER_SNAPSHOT_MAX_DEPTH && ids.size() <= PLAYER_SNAPSHOT_MAX_ROWS,
			E2BIG);
		const auto uids = id_list(ids);
		bound = before_custody;
		evidence.item_current_owner =
			capture(connection, session, "item_current_owner",
				"(" + player_owner + ") OR item_uid IN (" + uids +
					") OR root_item_uid IN (" + uids +
					") OR parent_item_uid IN (" + uids + ")",
				"item_uid", bound);
		const size_t previous = ids.size();
		for (const auto name : { "item_uid", "root_item_uid", "parent_item_uid" })
			add_ids(evidence.item_current_owner, name, ids);
		if (ids.size() == previous)
			break;
	}
	std::set<std::tuple<uint64_t, uint64_t, uint64_t>> owners;
	owners.emplace(static_cast<unsigned>(item_owner_type::player), request.pid, 0);
	for (const auto &row : request.death->custody)
		if (row.owner.type != item_owner_type::unknown)
			owners.emplace(static_cast<unsigned>(row.owner.type), row.owner.id,
				       row.owner.context_id);
	const auto type = column(evidence.item_current_owner, "owner_type");
	const auto owner_id = column(evidence.item_current_owner, "owner_id");
	const auto context = column(evidence.item_current_owner, "owner_context_id");
	for (const auto &row : evidence.item_current_owner.rows)
	{
		require(row[type] && row[owner_id] && row[context]);
		owners.emplace(number(*row[type]), number(*row[owner_id]), number(*row[context]));
	}
	std::string predicate = "0";
	for (const auto &[kind, id, ctx] : owners)
		predicate += " OR (owner_type=" + std::to_string(kind) +
			     " AND owner_id=" + std::to_string(id) +
			     " AND owner_context_id=" + std::to_string(ctx) + ')';
	evidence.item_owner_revision = capture(connection, session, "item_owner_revision",
					       predicate, "owner_type,owner_id,owner_context_id",
					       bound);
	return evidence;
}

struct stored_record
{
	player_death_conflict_case identity = {};
	int32_t pid = 0;
	std::string request_hash;
	player_snapshot snapshot = {};
};
std::optional<stored_record> read_record(MYSQL *connection, const std::string &predicate, bool lock)
{
	execute(connection,
		"SELECT operation_id,pid,save_revision,source_revision,corpse_item_uid,request_hash,payload_hash,"
		"CASE WHEN OCTET_LENGTH(payload)<=" +
			std::to_string(PLAYER_SNAPSHOT_MAX_BYTES) +
			" THEN payload ELSE NULL END FROM player_death_conflict_evidence WHERE " +
			predicate + " LIMIT 2" + (lock ? " FOR UPDATE" : ""));
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	require(bool(result), sql_error(connection));
	require(mysql_num_fields(result.get()) == 8);
	if (!mysql_num_rows(result.get()))
		return std::nullopt;
	require(mysql_num_rows(result.get()) == 1, EEXIST);
	const auto row = mysql_fetch_row(result.get());
	const auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths);
	for (size_t i = 0; i < 8; ++i)
		require(row[i]);
	require(lengths[0] == 16 && lengths[5] == SHA256_DIGEST_LENGTH &&
		lengths[6] == SHA256_DIGEST_LENGTH);
	stored_record output;
	std::copy_n(reinterpret_cast<const uint8_t *>(row[0]),
		    output.identity.operation_id.bytes.size(),
		    output.identity.operation_id.bytes.begin());
	const auto pid = number({ row[1], lengths[1] });
	require(pid > 0 && pid <= INT32_MAX);
	output.pid = static_cast<int32_t>(pid);
	output.identity.save_revision = number({ row[2], lengths[2] });
	output.identity.source_revision = number({ row[3], lengths[3] });
	output.identity.corpse_item_uid = number({ row[4], lengths[4] });
	output.request_hash.assign(row[5], lengths[5]);
	require(output.identity.source_revision < output.identity.save_revision &&
		digest({ row[7], lengths[7] }) == std::string(row[6], lengths[6]));
	require(player_snapshot_decode(reinterpret_cast<const uint8_t *>(row[7]), lengths[7],
				       &output.snapshot) == player_snapshot_codec_result::ok);
	const auto &snapshot = output.snapshot;
	require(player_snapshot_is_death_evidence_schema(snapshot.schema_version) &&
		snapshot.death && snapshot.death->conflict_evidence && snapshot.pid == output.pid &&
		snapshot.revision == output.identity.save_revision &&
		snapshot.death->operation_id.bytes == output.identity.operation_id.bytes &&
		snapshot.death->corpse.front().object_uid == output.identity.corpse_item_uid);
	auto request = snapshot;
	request.schema_version = player_snapshot_death_request_schema(snapshot.schema_version);
	request.death->conflict_evidence.reset();
	valid_request(request);
	require(digest(wire(request)) == output.request_hash);
	return output;
}

player_death_conflict_result commit(transaction &tx, outcome success, player_revision_t source)
{
	active(tx.connection, tx.session);
	try
	{
		execute(tx.connection, "COMMIT");
	}
	catch (const failure &error)
	{
		return { outcome::commit_unknown, error.code, source };
	}
	tx.pending = false;
	require(mysql_thread_id(tx.connection) == tx.session &&
			!(tx.connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
	return { success, 0, source };
}

player_death_conflict_result complete_terminal(transaction &tx, const player_snapshot &request,
					       const stored_record &record)
{
	active(tx.connection, tx.session);
	const auto written = player_snapshot_repository_write_retained_death(
		tx.connection, request, record.snapshot, record.identity.source_revision);
	require(written.outcome != player_death_terminal_write_outcome::failed, written.error_code);
	active(tx.connection, tx.session);
	if (written.outcome == player_death_terminal_write_outcome::already_written)
		return { outcome::already_terminal, 0, record.identity.source_revision };
	return commit(tx, outcome::terminal_committed, record.identity.source_revision);
}

bool connection_error(unsigned int code)
{
	return code == 2002 || code == 2003 || code == 2006 || code == 2013;
}

player_save_apply_result save_failure(unsigned int code)
{
	const bool retryable = code == EAGAIN || code == ETIMEDOUT || code == 1040 ||
			       code == 1205 || code == 1213 || connection_error(code);
	return { retryable ? player_save_apply_outcome::retryable_failure :
			     player_save_apply_outcome::terminal_failure,
		 0, code ? code : EIO };
}
} // namespace
#endif

static player_death_conflict_result
retain_death_conflict(MYSQL *connection, const player_snapshot &request, bool terminal) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)request;
	(void)terminal;
	return { player_death_conflict_outcome::failed, ENOTSUP, 0 };
#else
	try
	{
		valid_request(request);
		const auto request_bytes = wire(request);
		const auto request_hash = digest(request_bytes);
		idle(connection);
		// Serialize new unresolved evidence with the maintenance source gate.
		// This is not a currency mutation; it must nevertheless prevent a
		// cutover from racing a newly retained unresolved case.
		economic_sql_currency_writer_guard writer;
		const auto admission =
			economic_sql_currency_writer_guard::acquire(connection, &writer);
		require(admission == 0, admission);
		transaction tx(connection);
		const auto pid = std::to_string(request.pid);
		const auto revision = std::to_string(request.revision);
		const auto corpse = std::to_string(request.death->corpse.front().object_uid);
		const auto operation =
			"UNHEX('" + hex(bytes(request.death->operation_id.bytes)) + "')";
		auto existing = read_record(connection,
					    "operation_id=" + operation + " OR (pid=" + pid +
						    " AND (save_revision=" + revision +
						    " OR corpse_item_uid=" + corpse + "))",
					    true);
		active(connection, tx.session);
		if (existing)
		{
			if (existing->pid != request.pid ||
			    existing->identity.save_revision != request.revision ||
			    existing->identity.operation_id.bytes !=
				    request.death->operation_id.bytes ||
			    existing->request_hash != request_hash)
				return { outcome::identity_conflict, EEXIST, 0 };
			if (terminal)
				return complete_terminal(tx, request, *existing);
			return { outcome::already_retained, 0, existing->identity.source_revision };
		}
		execute(connection,
			"SELECT save_revision FROM player_data WHERE pid=" + pid + " FOR UPDATE");
		result_ptr result(mysql_store_result(connection), mysql_free_result);
		require(bool(result), sql_error(connection));
		require(mysql_num_rows(result.get()) == 1 && mysql_num_fields(result.get()) == 1,
			ENOENT);
		const auto row = mysql_fetch_row(result.get());
		const auto lengths = mysql_fetch_lengths(result.get());
		require(row && lengths && row[0]);
		const auto source_revision = number({ row[0], lengths[0] });
		result.reset();
		if (source_revision >= request.revision)
			return { outcome::stale_revision, ESTALE, source_revision };
		if (terminal)
		{
			// A new ID must not overwrite an unresolved case's terminal state.
			execute(connection,
				"SELECT operation_id FROM player_death_conflict_evidence WHERE pid=" +
					pid + " LIMIT 1 FOR UPDATE");
			result_ptr cases(mysql_store_result(connection), mysql_free_result);
			require(bool(cases), sql_error(connection));
			require(mysql_num_rows(cases.get()) == 0, EEXIST);
		}
		auto retained = request;
		retained.schema_version =
			player_snapshot_death_evidence_schema(request.schema_version);
		retained.death->conflict_evidence =
			observations(connection, tx.session, request, request_bytes.size());
		const auto payload = wire(retained);
		active(connection, tx.session);
		execute(connection,
			"INSERT INTO player_death_conflict_evidence (operation_id,pid,save_revision,source_revision,"
			"corpse_item_uid,request_hash,payload_hash,payload) VALUES (" +
				operation + ',' + pid + ',' + revision + ',' +
				std::to_string(source_revision) + ',' + corpse + ",UNHEX('" +
				hex(request_hash) + "'),UNHEX('" + hex(digest(payload)) +
				"'),UNHEX('" + hex(payload) + "'))");
		active(connection, tx.session);
		require(mysql_affected_rows(connection) == 1);
		if (terminal)
		{
			// Hash-checked exact read-back and terminal writes share this one
			// transaction: no archive-only completion can authorize extraction.
			auto verified = read_record(
				connection, "pid=" + pid + " AND operation_id=" + operation, true);
			require(bool(verified) && verified->request_hash == request_hash &&
				verified->identity.source_revision == source_revision);
			return complete_terminal(tx, request, *verified);
		}
		return commit(tx, outcome::retained, source_revision);
	}
	catch (const failure &error)
	{
		return { error.code == EEXIST || error.code == 1062 ? outcome::identity_conflict :
								      outcome::failed,
			 error.code, 0 };
	}
	catch (const std::bad_alloc &)
	{
		return { outcome::failed, ENOMEM, 0 };
	}
	catch (...)
	{
		return { outcome::failed, EIO, 0 };
	}
#endif
}

player_death_conflict_result player_death_conflict_retain(MYSQL *connection,
							  const player_snapshot &request) noexcept
{
	return retain_death_conflict(connection, request, false);
}

player_save_apply_result player_death_conflict_apply(MYSQL *connection,
						     const player_snapshot &request) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)request;
	return { player_save_apply_outcome::terminal_failure, 0, ENOTSUP };
#else
	try
	{
		idle(connection);
		if (!request.death)
			return player_snapshot_repository_apply(connection, request);
		valid_request(request);
		{
			economic_sql_currency_writer_guard writer;
			const auto admission =
				economic_sql_currency_writer_guard::acquire(connection, &writer);
			require(admission == 0, admission);
			const auto existing = read_record(
				connection,
				"pid=" + std::to_string(request.pid) + " AND operation_id=UNHEX('" +
					hex(bytes(request.death->operation_id.bytes)) + "')",
				false);
			if (!existing)
			{
				const auto ordinary =
					player_snapshot_repository_apply(connection, request);
				if (ordinary.outcome !=
					    player_save_apply_outcome::terminal_failure ||
				    ordinary.error_code !=
					    PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH)
					return ordinary;
			}
		}
		// Reacquire admission and locked evidence after normal rollback. The
		// participant verifies the exact mismatch again, never an arbitrary error.
		const auto completed = retain_death_conflict(connection, request, true);
		if (completed.outcome == outcome::terminal_committed ||
		    completed.outcome == outcome::already_terminal)
			return { completed.outcome == outcome::terminal_committed ?
					 player_save_apply_outcome::applied :
					 player_save_apply_outcome::already_applied,
				 request.revision, 0 };
		if (completed.outcome == outcome::commit_unknown)
			return { player_save_apply_outcome::ambiguous_commit,
				 completed.source_revision, completed.error_code };
		return save_failure(completed.error_code);
	}
	catch (const failure &error)
	{
		return save_failure(error.code);
	}
	catch (const std::bad_alloc &)
	{
		return save_failure(ENOMEM);
	}
	catch (...)
	{
		return save_failure(EIO);
	}
#endif
}

player_save_apply_result player_death_conflict_apply_from_pool(const player_snapshot &request,
							       void *context)
{
#ifdef __NO_MYSQL__
	(void)request;
	(void)context;
	return { player_save_apply_outcome::terminal_failure, 0, ENOTSUP };
#else
	if (!request.death)
		return player_snapshot_repository_apply_from_pool(request, context);
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return save_failure(ETIMEDOUT);
	auto applied = player_death_conflict_apply(connection, request);
	if (applied.outcome == player_save_apply_outcome::ambiguous_commit ||
	    connection_error(applied.error_code))
	{
		connection = sql_pool_replace_connection(connection);
		if (connection)
		{
			// Same-request replay proves a terminal receipt, not merely a counter
			// that an unrelated checkpoint could have advanced.
			applied = player_death_conflict_apply(connection, request);
		}
	}
	sql_pool_release(connection);
	return applied;
#endif
}

player_death_conflict_result player_death_conflict_read(MYSQL *connection, int32_t pid,
							const critical_operation_id &operation_id,
							player_snapshot *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)pid;
	(void)operation_id;
	(void)output;
	return { player_death_conflict_outcome::failed, ENOTSUP, 0 };
#else
	try
	{
		require(connection && pid > 0 && output, EINVAL);
		auto stored =
			read_record(connection,
				    "pid=" + std::to_string(pid) + " AND operation_id=UNHEX('" +
					    hex(bytes(operation_id.bytes)) + "')",
				    false);
		if (!stored)
			return { outcome::not_found, ENOENT, 0 };
		const auto revision = stored->identity.source_revision;
		*output = std::move(stored->snapshot);
		return { outcome::read, 0, revision };
	}
	catch (const failure &error)
	{
		return { outcome::failed, error.code, 0 };
	}
	catch (const std::bad_alloc &)
	{
		return { outcome::failed, ENOMEM, 0 };
	}
	catch (...)
	{
		return { outcome::failed, EIO, 0 };
	}
#endif
}

player_death_conflict_result
player_death_conflict_list(MYSQL *connection, int32_t pid, player_revision_t after_revision,
			   std::vector<player_death_conflict_case> *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)pid;
	(void)after_revision;
	(void)output;
	return { player_death_conflict_outcome::failed, ENOTSUP, 0 };
#else
	try
	{
		require(connection && pid > 0 && output, EINVAL);
		execute(connection,
			"SELECT operation_id,save_revision,source_revision,corpse_item_uid FROM player_death_conflict_evidence WHERE pid=" +
				std::to_string(pid) + " AND save_revision>" +
				std::to_string(after_revision) + " ORDER BY save_revision LIMIT " +
				std::to_string(PLAYER_DEATH_CONFLICT_LIST_LIMIT));
		result_ptr result(mysql_store_result(connection), mysql_free_result);
		require(bool(result), sql_error(connection));
		require(mysql_num_fields(result.get()) == 4 &&
			mysql_num_rows(result.get()) <= PLAYER_DEATH_CONFLICT_LIST_LIMIT);
		std::vector<player_death_conflict_case> cases;
		while (const auto row = mysql_fetch_row(result.get()))
		{
			const auto lengths = mysql_fetch_lengths(result.get());
			require(lengths && row[0] && row[1] && row[2] && row[3] &&
				lengths[0] == 16);
			player_death_conflict_case value = {};
			std::copy_n(reinterpret_cast<const uint8_t *>(row[0]),
				    value.operation_id.bytes.size(),
				    value.operation_id.bytes.begin());
			value.save_revision = number({ row[1], lengths[1] });
			value.source_revision = number({ row[2], lengths[2] });
			value.corpse_item_uid = number({ row[3], lengths[3] });
			require(value.source_revision < value.save_revision &&
				value.save_revision > after_revision && value.corpse_item_uid);
			cases.push_back(value);
		}
		require(!mysql_errno(connection), sql_error(connection));
		*output = std::move(cases);
		return { outcome::read, 0, 0 };
	}
	catch (const failure &error)
	{
		return { outcome::failed, error.code, 0 };
	}
	catch (const std::bad_alloc &)
	{
		return { outcome::failed, ENOMEM, 0 };
	}
	catch (...)
	{
		return { outcome::failed, EIO, 0 };
	}
#endif
}
