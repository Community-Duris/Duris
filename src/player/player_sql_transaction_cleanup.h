#ifndef PLAYER_SQL_TRANSACTION_CLEANUP_H
#define PLAYER_SQL_TRANSACTION_CLEANUP_H

#include "sql/sql_pool.h"
#include <mysql/mysql.h>
#include <cerrno>
#include <type_traits>

// SQL ownership evidence only. This is neither a save ACK nor a worker result.
enum class player_sql_cleanup_disposition : unsigned char
{
	untouched,
	idle_verified,
	retire_required,
};

struct player_sql_cleanup
{
	unsigned long original_session = 0;
	player_sql_cleanup_disposition disposition = player_sql_cleanup_disposition::untouched;
	bool rollback_confirmed = false;
	unsigned int cleanup_error = 0;
};

inline unsigned int player_sql_idle_error(MYSQL *connection) noexcept
{
	if (!connection)
		return EINVAL;
#ifdef __NO_MYSQL__
	return ENOTSUP;
#else
	if ((connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    !(connection->server_status & SERVER_STATUS_AUTOCOMMIT))
		return EBUSY;
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	if (mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) || reconnect)
		return EINVAL;
	return 0;
#endif
}

class player_sql_transaction_cleanup
{
	MYSQL *connection_;
	player_sql_cleanup &proof_;
	bool pending_ = false;
	bool commit_attempted_ = false;

    public:
	player_sql_transaction_cleanup(MYSQL *connection, player_sql_cleanup &proof) noexcept
		: connection_(connection)
		, proof_(proof)
	{
		proof_ = {};
#ifndef __NO_MYSQL__
		if (connection)
			proof_.original_session = mysql_thread_id(connection);
#endif
	}
	player_sql_transaction_cleanup(const player_sql_transaction_cleanup &) = delete;
	player_sql_transaction_cleanup &operator=(const player_sql_transaction_cleanup &) = delete;
	~player_sql_transaction_cleanup() noexcept
	{
		// Fallback cleanup is not explicit reuse proof. Call finish() first.
		if (pending_)
		{
			proof_.disposition = player_sql_cleanup_disposition::retire_required;
			try
			{
				if (same_session())
					(void)mysql_real_query(connection_, "ROLLBACK", 8);
			}
			catch (...)
			{
			}
		}
	}
	bool same_session() const noexcept
	{
#ifdef __NO_MYSQL__
		return false;
#else
		return connection_ && mysql_thread_id(connection_) == proof_.original_session;
#endif
	}
	bool commit_attempted() const noexcept { return commit_attempted_; }
	void starting() noexcept
	{
		pending_ = true;
		proof_.disposition = player_sql_cleanup_disposition::retire_required;
	}
	void committing() noexcept { commit_attempted_ = true; }
	bool committed() noexcept
	{
		const unsigned int code = same_session() ? player_sql_idle_error(connection_) :
							   ENOTCONN;
		if (code)
		{
			proof_.cleanup_error = code;
			return false;
		}
		pending_ = false;
		proof_.disposition = player_sql_cleanup_disposition::idle_verified;
		return true;
	}
	void finish() noexcept
	{
		if (!pending_)
			return;
		pending_ = false;
		proof_.disposition = player_sql_cleanup_disposition::retire_required;
		proof_.rollback_confirmed = false;
		if (!same_session())
		{
			proof_.cleanup_error = ENOTCONN;
			return;
		}
		try
		{
			const int rc = mysql_real_query(connection_, "ROLLBACK", 8);
			const unsigned int rollback_error = rc ? mysql_errno(connection_) : 0;
			const unsigned int code = rc ? (rollback_error ? rollback_error : EIO) :
						       (same_session() ?
								player_sql_idle_error(connection_) :
								ENOTCONN);
			if (code)
			{
				proof_.cleanup_error = code;
				return;
			}
			proof_.cleanup_error = 0;
			proof_.rollback_confirmed = true;
			proof_.disposition = player_sql_cleanup_disposition::idle_verified;
		}
		catch (...)
		{
			proof_.cleanup_error = EIO;
		}
	}
};

// Replacement consumes the old lease even when it returns NULL. The owner
// holds only the returned handle; uncertain cleanup always retires on release.
class player_sql_pool_lease
{
	MYSQL *connection_;
	bool reusable_ = false;

    public:
	explicit player_sql_pool_lease(MYSQL *connection) noexcept
		: connection_(connection)
	{
	}
	player_sql_pool_lease(const player_sql_pool_lease &) = delete;
	player_sql_pool_lease &operator=(const player_sql_pool_lease &) = delete;
	~player_sql_pool_lease() noexcept
	{
		if (connection_)
		{
			if (!reusable_)
				sql_pool_discard_connection(connection_);
			sql_pool_release(connection_);
		}
	}
	MYSQL *get() const noexcept { return connection_; }
	void reuse(const player_sql_cleanup &proof) noexcept
	{
		reusable_ = connection_ &&
			    proof.disposition == player_sql_cleanup_disposition::idle_verified &&
			    !proof.cleanup_error && !player_sql_idle_error(connection_);
#ifndef __NO_MYSQL__
		reusable_ = reusable_ && mysql_thread_id(connection_) == proof.original_session;
#endif
	}
	bool replace()
	{
		MYSQL *old = connection_;
		connection_ = nullptr;
		reusable_ = false;
		connection_ = sql_pool_replace_connection(old);
		return connection_ != nullptr;
	}
};

#endif
