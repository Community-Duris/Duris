#ifndef DURIS_ECONOMIC_SQL_LIFECYCLE_GUARD_H
#define DURIS_ECONOMIC_SQL_LIFECYCLE_GUARD_H

#include <mysql/mysql.h>
#include <mutex>
#include <shared_mutex>

// Shared runtime admission + currency-writer fence. The runtime guard is held
// on a dedicated reconnect-disabled SQL control connection for the whole game
// process lifetime. Maintenance can acquire its exclusive named lock only while
// every runtime has released that lock. Do not replace either lock with a bool,
// digest, or caller assertion.
class economic_sql_lifecycle_guard
{
    public:
	economic_sql_lifecycle_guard() = default;
	economic_sql_lifecycle_guard(const economic_sql_lifecycle_guard &) = delete;
	economic_sql_lifecycle_guard &operator=(const economic_sql_lifecycle_guard &) = delete;
	~economic_sql_lifecycle_guard();

	// Acquire at process boot before admitting gameplay; retain until shutdown.
	static unsigned int acquire_runtime(MYSQL *control_connection,
					    economic_sql_lifecycle_guard *) noexcept;
	// Acquire only from the quiesced maintenance process, before source capture.
	// It owns both the boot/runtime gate and the legacy currency writer fence.
	static unsigned int acquire_maintenance(MYSQL *control_connection,
						economic_sql_lifecycle_guard *) noexcept;
	bool is_maintenance_authority() const noexcept;

    private:
	friend class economic_sql_accounting_lifecycle_transaction;
	friend class economic_sql_currency_writer_guard;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	bool runtime_lock_ = false;
	bool writer_lock_ = false;
	bool maintenance_ = false;
	bool local_runtime_ = false;
	bool local_maintenance_ = false;
	std::unique_lock<std::shared_mutex> local_exclusive_;
};

// Every legacy SQL wallet/shared-bank mutation must hold this guard from before
// its transaction starts through COMMIT/ROLLBACK. After a staged installation
// exists it refuses the old writer path. Missing lifecycle schema/errors fail
// closed. The caller owns the MYSQL connection and must not reconnect it.
class economic_sql_currency_writer_guard
{
    public:
	economic_sql_currency_writer_guard() = default;
	economic_sql_currency_writer_guard(const economic_sql_currency_writer_guard &) = delete;
	economic_sql_currency_writer_guard &
	operator=(const economic_sql_currency_writer_guard &) = delete;
	~economic_sql_currency_writer_guard();

	static unsigned int acquire(MYSQL *connection,
				    economic_sql_currency_writer_guard *) noexcept;

    private:
	std::shared_lock<std::shared_mutex> local_shared_;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	bool writer_lock_ = false;
};

#endif
