#include "player/player_death_recovery_query.h"
#include "persistence/death_recovery_visibility.h"
#include "player/player_load_repository.h"
#include "player/player_snapshot.h"
#include "persistence/persistence_observability.h"
#include "sql/sql_pool.h"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

bool valid_text_identity(const std::string &value, size_t maximum)
{
	return !value.empty() && value.size() <= maximum &&
	       std::all_of(value.begin(), value.end(),
			   [](unsigned char byte) { return byte >= 0x20 && byte <= 0x7e; });
}

std::string sql_escape(MYSQL *connection, const std::string &value)
{
	std::string output(value.size() * 2 + 1, '\0');
	const unsigned long length =
		mysql_real_escape_string(connection, output.data(), value.data(), value.size());
	output.resize(length);
	return output;
}

bool run_query(MYSQL *connection, const std::string &sql, MYSQL_RES **rows_out,
	       unsigned int *error_out)
{
	if (!connection || !rows_out || !error_out)
		return false;
	const uint64_t started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql.data(), sql.size());
	const unsigned int error = rc ? mysql_errno(connection) : 0;
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
				 persistence_statement_kind_from_sql(sql.c_str()),
				 persistence_observability_now_usec() - started, rc == 0, error,
				 rc ? mysql_sqlstate(connection) : "00000");
	if (rc)
	{
		*error_out = error ? error : EIO;
		return false;
	}
	*rows_out = mysql_store_result(connection);
	if (!*rows_out)
	{
		*error_out = mysql_errno(connection) ? mysql_errno(connection) : EIO;
		return false;
	}
	return true;
}

bool run_statement(MYSQL *connection, const char *sql, unsigned int *error_out)
{
	if (!connection || !sql || !error_out)
		return false;
	const uint64_t started = persistence_observability_now_usec();
	const int rc = mysql_real_query(connection, sql, std::strlen(sql));
	const unsigned int error = rc ? mysql_errno(connection) : 0;
	persistence_query_record(PERSISTENCE_QUERY_SITE,
				 PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER,
				 persistence_statement_kind_from_sql(sql),
				 persistence_observability_now_usec() - started, rc == 0, error,
				 rc ? mysql_sqlstate(connection) : "00000");
	if (rc)
	{
		*error_out = error ? error : EIO;
		return false;
	}
	MYSQL_RES *rows = mysql_store_result(connection);
	if (rows)
	{
		mysql_free_result(rows);
		return true;
	}
	if (mysql_field_count(connection) == 0)
		return true;
	*error_out = mysql_errno(connection) ? mysql_errno(connection) : EIO;
	return false;
}

class read_transaction
{
    public:
	explicit read_transaction(MYSQL *connection)
		: connection_(connection)
	{
		if (!connection_)
		{
			error_ = EBUSY;
			if (connection_)
				sql_pool_discard_connection(connection_);
			return;
		}
#ifndef __NO_MYSQL__
		if ((connection_->server_status & SERVER_STATUS_IN_TRANS) ||
		    !(connection_->server_status & SERVER_STATUS_AUTOCOMMIT))
		{
			error_ = EBUSY;
			sql_pool_discard_connection(connection_);
			return;
		}
#endif
		if (!run_statement(connection_, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ",
				   &error_))
		{
			discard_on_release();
			return;
		}
		if (!run_statement(connection_,
				   "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY",
				   &error_))
		{
			// A lost response cannot prove whether the snapshot started.
			discard_on_release();
			return;
		}
		active_ = true;
	}

	~read_transaction()
	{
		if (active_)
			rollback();
	}

	bool active() const { return active_; }
	unsigned int error() const { return error_; }

	bool commit()
	{
		if (!active_)
			return false;
		unsigned int commit_error = 0;
		if (!run_statement(connection_, "COMMIT", &commit_error))
		{
			error_ = commit_error ? commit_error : EIO;
			active_ = false;
			// The commit may have succeeded even if its response was lost.
			discard_on_release();
			return false;
		}
		active_ = false;
		return true;
	}

	bool rollback()
	{
		if (!active_)
			return true;
		unsigned int rollback_error = 0;
		if (!run_statement(connection_, "ROLLBACK", &rollback_error))
		{
			error_ = rollback_error ? rollback_error : EIO;
			active_ = false;
			discard_on_release();
			return false;
		}
		active_ = false;
		return true;
	}

    private:
	void discard_on_release()
	{
		if (connection_)
			sql_pool_discard_connection(connection_);
	}

	MYSQL *connection_ = nullptr;
	unsigned int error_ = 0;
	bool active_ = false;
};

bool account_owns_character(MYSQL *connection, int32_t pid, const std::string &account_name,
			    const std::string &character_name, unsigned int *error_out)
{
	// Match load_status(): a non-NULL character account is authoritative;
	// only a legacy NULL account may fall back to the live association.
	const std::string sql =
		"SELECT ac.pid FROM account_characters ac JOIN player_data pd ON pd.pid=ac.pid "
		"WHERE ac.pid=" +
		std::to_string(pid) + " AND LOWER(ac.account_name)=LOWER('" +
		sql_escape(connection, account_name) + "') AND LOWER(ac.char_name)=LOWER('" +
		sql_escape(connection, character_name) + "') AND LOWER(pd.name)=LOWER('" +
		sql_escape(connection, character_name) +
		"') AND (pd.account_name IS NULL OR LOWER(pd.account_name)=LOWER(ac.account_name)) "
		"AND ac.deleted_at IS NULL LIMIT 2";
	MYSQL_RES *raw_rows = nullptr;
	if (!run_query(connection, sql, &raw_rows, error_out))
		return false;
	result_ptr rows(raw_rows, mysql_free_result);
	if (mysql_num_fields(rows.get()) != 1 || mysql_num_rows(rows.get()) != 1)
		return false;
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	if (!row || !row[0])
		return false;
	int32_t owner_pid = 0;
	const char *end = row[0] + std::char_traits<char>::length(row[0]);
	const auto parsed = std::from_chars(row[0], end, owner_pid);
	return parsed.ec == std::errc{} && parsed.ptr == end && owner_pid == pid;
}

std::string sanitize_item_label(const std::string &source)
{
	std::string output;
	output.reserve(std::min(source.size(), PLAYER_DEATH_RECOVERY_ITEM_LABEL_MAX));
	for (unsigned char byte : source)
	{
		if (output.size() >= PLAYER_DEATH_RECOVERY_ITEM_LABEL_MAX)
			break;
		// Account-menu text accepts printable ASCII only. Drop MUD colour markers,
		// shell-like format delimiters, controls, and non-ASCII format sequences.
		if (byte >= 0x20 && byte <= 0x7e && byte != '&' && byte != '$' && byte != '%')
			output.push_back(static_cast<char>(byte));
		else if (!output.empty() && output.back() != ' ')
			output.push_back(' ');
	}
	while (!output.empty() && output.back() == ' ')
		output.pop_back();
	while (!output.empty() && output.front() == ' ')
		output.erase(output.begin());
	return output.empty() ? std::string("unnamed item") : output;
}

void append_bounded(std::string *output, std::string_view text)
{
	if (!output || output->size() >= PLAYER_DEATH_RECOVERY_SUMMARY_MAX)
		return;
	const size_t remaining = PLAYER_DEATH_RECOVERY_SUMMARY_MAX - output->size();
	output->append(text.data(), std::min(remaining, text.size()));
}

size_t evidence_rows(const std::optional<player_death_conflict_evidence> &evidence,
		     const player_death_evidence_table player_death_conflict_evidence::*table)
{
	return evidence ? (evidence.value().*table).rows.size() : 0;
}
} // namespace

bool player_death_recovery_query_request_valid(const player_death_recovery_query_request &request,
					       int32_t pid, const std::string &account_name,
					       const std::string &character_name)
{
	if (pid <= 0 || !valid_text_identity(account_name, PLAYER_LOAD_ACCOUNT_MAX) ||
	    !valid_text_identity(character_name, PLAYER_LOAD_NAME_MAX))
		return false;
	switch (request.kind)
	{
	case player_death_recovery_query_kind::list:
		return critical_operation_id_is_zero(request.operation_id);
	case player_death_recovery_query_kind::detail:
		return critical_operation_id_is_zero(request.operation_id) == false &&
		       request.after_revision == 0;
	case player_death_recovery_query_kind::none:
		return false;
	}
	return false;
}

bool player_death_recovery_detail_identity_valid(int32_t pid,
						 const critical_operation_id &requested,
						 const player_death_conflict_case &identity,
						 const player_snapshot &snapshot)
{
	return pid > 0 && !critical_operation_id_is_zero(requested) && snapshot.pid == pid &&
	       player_snapshot_is_death_evidence_schema(snapshot.schema_version) &&
	       snapshot.death && snapshot.death->conflict_evidence &&
	       !snapshot.death->corpse.empty() && snapshot.revision == identity.save_revision &&
	       snapshot.death->operation_id.bytes == requested.bytes &&
	       identity.operation_id.bytes == requested.bytes &&
	       identity.source_revision < identity.save_revision &&
	       identity.corpse_item_uid == snapshot.death->corpse.front().object_uid;
}

bool player_death_recovery_summary_build(const player_death_conflict_case &identity,
					 const player_snapshot &snapshot, std::string *summary)
{
	if (!summary || !player_death_recovery_detail_identity_valid(
				snapshot.pid, identity.operation_id, identity, snapshot))
		return false;
	std::string output;
	output.reserve(PLAYER_DEATH_RECOVERY_SUMMARY_MAX);
	char correlation[33] = {};
	death_recovery_correlation(
		(static_cast<uint64_t>(snapshot.pid) << 32) |
			static_cast<uint32_t>(snapshot.death->corpse.front().values[6]),
		correlation);
	output += "correlation=";
	output += correlation;
	output += " recovery_owner=retained_death_conflict. ";
	output += "Unresolved archive evidence only; this is not a completed terminal recovery. ";
	output += "Saved revision ";
	output += std::to_string(identity.save_revision);
	output += "; source revision ";
	output += std::to_string(identity.source_revision);
	output += ". Captured corpse item count: ";
	output += std::to_string(snapshot.death->corpse.size());
	output += ".";
	const size_t shown =
		std::min(snapshot.death->corpse.size(), PLAYER_DEATH_RECOVERY_ITEM_LABEL_COUNT);
	for (size_t index = 0; index < shown; ++index)
	{
		const player_item_snapshot &item = snapshot.death->corpse[index];
		char label[40];
		const int written =
			std::snprintf(label, sizeof label, "\nItem template %d: ", item.vnum);
		if (written > 0)
			append_bounded(&output,
				       std::string_view(label, static_cast<size_t>(written)));
		append_bounded(&output, sanitize_item_label(item.name));
		append_bounded(&output, ".");
	}
	if (snapshot.death->corpse.size() > shown)
		append_bounded(&output, "\nAdditional captured item labels withheld.");
	const auto &evidence = snapshot.death->conflict_evidence;
	output += "\nEvidence rows (historical, not inventory): items=";
	output += std::to_string(
		evidence_rows(evidence, &player_death_conflict_evidence::player_items));
	output += ", affects=";
	output += std::to_string(
		evidence_rows(evidence, &player_death_conflict_evidence::player_item_affects));
	output += ", descriptions=";
	output += std::to_string(
		evidence_rows(evidence, &player_death_conflict_evidence::player_item_extra_descr));
	output += ", current owners=";
	output += std::to_string(
		evidence_rows(evidence, &player_death_conflict_evidence::item_current_owner));
	output += ", owner revisions=";
	output += std::to_string(
		evidence_rows(evidence, &player_death_conflict_evidence::item_owner_revision));
	output += ". Raw payload withheld.";
	if (output.size() > PLAYER_DEATH_RECOVERY_SUMMARY_MAX)
		output.resize(PLAYER_DEATH_RECOVERY_SUMMARY_MAX);
	*summary = std::move(output);
	return true;
}

player_death_recovery_query_result
player_death_recovery_query_execute(MYSQL *connection, int32_t pid, const std::string &account_name,
				    const std::string &character_name,
				    const player_death_recovery_query_request &request) noexcept
{
	player_death_recovery_query_result output = {};
	output.kind = request.kind;
	if (!connection ||
	    !player_death_recovery_query_request_valid(request, pid, account_name, character_name))
	{
		output.error_code = EINVAL;
		return output;
	}
	try
	{
		read_transaction transaction(connection);
		if (!transaction.active())
		{
			output.error_code = transaction.error() ? transaction.error() : EIO;
			return output;
		}
		const auto rollback_error = [&transaction](unsigned int original_error)
		{
			if (transaction.rollback())
				return original_error;
			return transaction.error() ? transaction.error() : EIO;
		};
		unsigned int error = 0;
		if (!account_owns_character(connection, pid, account_name, character_name, &error))
		{
			output.error_code = rollback_error(error);
			output.outcome = output.error_code ?
						 player_death_recovery_query_outcome::failed :
						 player_death_recovery_query_outcome::unauthorized;
			return output;
		}
		if (request.kind == player_death_recovery_query_kind::list)
		{
			std::vector<player_death_conflict_case> cases;
			const player_death_conflict_result listed = player_death_conflict_list(
				connection, pid, request.after_revision, &cases);
			if (listed.outcome != player_death_conflict_outcome::read)
			{
				output.error_code =
					rollback_error(listed.error_code ? listed.error_code : EIO);
				return output;
			}
			if (!transaction.commit())
			{
				output.error_code = transaction.error() ? transaction.error() : EIO;
				return output;
			}
			output.cases = std::move(cases);
			output.outcome = player_death_recovery_query_outcome::read;
			return output;
		}

		player_snapshot snapshot = {};
		const player_death_conflict_result read = player_death_conflict_read(
			connection, pid, request.operation_id, &snapshot);
		if (read.outcome == player_death_conflict_outcome::not_found)
		{
			const unsigned int cleanup_error = rollback_error(0);
			if (cleanup_error)
			{
				output.error_code = cleanup_error;
				return output;
			}
			output.outcome = player_death_recovery_query_outcome::not_found;
			output.error_code = ENOENT;
			return output;
		}
		if (read.outcome != player_death_conflict_outcome::read)
		{
			output.error_code = rollback_error(read.error_code ? read.error_code : EIO);
			return output;
		}
		player_death_conflict_case identity = {};
		identity.operation_id = request.operation_id;
		identity.save_revision = snapshot.revision;
		identity.source_revision = read.source_revision;
		identity.corpse_item_uid = snapshot.death && !snapshot.death->corpse.empty() ?
						   snapshot.death->corpse.front().object_uid :
						   0;
		std::string summary;
		if (!player_death_recovery_detail_identity_valid(pid, request.operation_id,
								 identity, snapshot) ||
		    !player_death_recovery_summary_build(identity, snapshot, &summary))
		{
			output.error_code = rollback_error(EILSEQ);
			return output;
		}
		if (!transaction.commit())
		{
			output.error_code = transaction.error() ? transaction.error() : EIO;
			return output;
		}
		output.detail_identity = identity;
		output.detail_summary = std::move(summary);
		output.outcome = player_death_recovery_query_outcome::read;
		return output;
	}
	catch (const std::bad_alloc &)
	{
		output.error_code = ENOMEM;
	}
	catch (...)
	{
		output.error_code = EIO;
	}
	return output;
}
