#include "persistence/economic_outcome_mode.h"
#include "persistence/economic_outcome.h"
#include "persistence/critical_command_journal.h"
#include "economy/economic_gameplay_authority.h"
#include "sql/sql_pool.h"
#include <mysql/mysqld_error.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

namespace
{
enum class boot_choice
{
	unconfigured,
	off,
	ram,
	refused
};
std::atomic<boot_choice> selected{ boot_choice::unconfigured };

bool no_commissioned_stream() noexcept
{
#ifdef __NO_MYSQL__
	return true;
#else
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return false;
	enum class presence
	{
		absent,
		empty,
		commissioned,
		failed
	};
	const auto inspect = [connection](const char *query)
	{
		if (mysql_query(connection, query))
			return mysql_errno(connection) == ER_NO_SUCH_TABLE ? presence::absent :
									     presence::failed;
		MYSQL_RES *result = mysql_store_result(connection);
		if (!result)
			return presence::failed;
		MYSQL_ROW row = mysql_fetch_row(result);
		presence found = presence::failed;
		if (mysql_num_fields(result) == 1 && mysql_num_rows(result) == 1 && row && row[0])
		{
			if (!std::strcmp(row[0], "0"))
				found = presence::empty;
			else if (!std::strcmp(row[0], "1"))
				found = presence::commissioned;
		}
		mysql_free_result(result);
		return found;
	};
	// Exact named reads distinguish genuine absent-table errors from access
	// denial. Privilege-filtered information_schema emptiness proves nothing.
	const auto cursor = inspect(
		"SELECT IF(EXISTS(SELECT 1 FROM economic_ram_outcome_projection_cursor),1,0)");
	const auto receipts = inspect(
		"SELECT IF(EXISTS(SELECT 1 FROM economic_ram_outcome_projection_receipt),1,0)");
	bool clean = cursor == presence::absent && receipts == presence::absent;
	if (cursor == presence::empty && receipts == presence::empty &&
	    !mysql_query(
		    connection,
		    "SELECT table_name,table_type,engine FROM information_schema.tables "
		    "WHERE table_schema=DATABASE() AND table_name IN "
		    "('economic_ram_outcome_projection_cursor','economic_ram_outcome_projection_receipt') "
		    "ORDER BY table_name"))
	{
		MYSQL_RES *tables = mysql_store_result(connection);
		if (tables)
		{
			const auto count = mysql_num_rows(tables);
			bool shape = mysql_num_fields(tables) == 3 && count == 2;
			const char *names[] = { "economic_ram_outcome_projection_cursor",
						"economic_ram_outcome_projection_receipt" };
			for (size_t index = 0; shape && index < count; ++index)
			{
				MYSQL_ROW row = mysql_fetch_row(tables);
				shape = row && row[0] && row[1] && row[2] &&
					!std::strcmp(row[0], names[index]) &&
					!std::strcmp(row[1], "BASE TABLE") &&
					!std::strcmp(row[2], "InnoDB");
			}
			mysql_free_result(tables);
			clean = shape;
		}
	}
	clean = clean && (connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
		!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
		!mysql_more_results(connection);
	if (!clean)
		sql_pool_discard_connection(connection);
	sql_pool_release(connection);
	return clean;
#endif
}

bool clean_off_namespace(const char *directory) noexcept
try
{
	if (!directory || !*directory)
		return true;
	const std::string path = std::string(directory) + "/economic-outcomes.journal";
	struct stat file
	{
	};
	if (lstat(path.c_str(), &file))
		return errno == ENOENT;
	// Reuse the real namespace parser/CRC/permission/locking checks. No new
	// file is created for ordinary clean OFF; a symlink or malformed file fails.
	if (!S_ISREG(file.st_mode) ||
	    !economic_outcome_journal_init(directory, CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA,
					   CRITICAL_COMMAND_JOURNAL_MAX_RECORDS))
		return false;
	const auto health = economic_outcome_journal_health_copy();
	const bool clean = health.initialized && !health.uncertain && !health.durable_sequence &&
			   !health.compacted_through && !health.retained_records;
	economic_outcome_journal_shutdown();
	// Never erase/repair a namespace or interpret a disabled request as proof
	// that prior acknowledged state was projected. Nonzero cuts need recovery.
	return clean;
}
catch (...)
{
	return false;
}
}

bool economic_outcome_mode_configure(const char *journal_directory) noexcept
{
	if (selected.load(std::memory_order_acquire) != boot_choice::unconfigured)
		return false;
	const char *configured = std::getenv("DURIS_ACCOUNTING_MODE");
	boot_choice choice = boot_choice::refused;
	if (!configured || !std::strcmp(configured, "off"))
		choice = clean_off_namespace(journal_directory) ? boot_choice::off :
								  boot_choice::refused;
	else if (!std::strcmp(configured, "ram"))
		choice = boot_choice::ram;
	boot_choice expected = boot_choice::unconfigured;
	if (!selected.compare_exchange_strong(expected, choice, std::memory_order_acq_rel))
		return false;
	return choice != boot_choice::refused;
}

bool economic_outcome_accounting_requested() noexcept
{
	// Unconfigured/invalid startup is closed too. A failed requested RAM boot
	// cannot become OFF because a ready owner or SQL connection is unavailable.
	return selected.load(std::memory_order_acquire) != boot_choice::off;
}

bool economic_outcome_mode_validate_selected_policy() noexcept
{
	const auto choice = selected.load(std::memory_order_acquire);
	if (choice == boot_choice::unconfigured || choice == boot_choice::refused)
		return false;
	if (choice == boot_choice::off &&
	    (economic_gameplay_authority::active() || !no_commissioned_stream()))
	{
		selected.store(boot_choice::refused, std::memory_order_release);
		return false;
	}
	return true;
}
