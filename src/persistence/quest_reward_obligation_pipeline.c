#include "net/network_wakeup.h"
#include "persistence/quest_reward_obligation_pipeline.h"

#include "flatfile/flatfile_item_repository.h"
#include "persistence/persistence_mode.h"
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"
#include "player/player_sql_transaction_cleanup.h"

#ifndef __NO_MYSQL__
#include <mysql/errmsg.h>
#endif

#include <condition_variable>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
struct ack_request
{
	uint32_t player_pid = 0;
	critical_operation_id offering_operation = {};
};

std::mutex pipeline_mutex;
std::condition_variable request_ready;
std::deque<ack_request> requests;
std::deque<quest_reward_ack_completion> completions;
std::vector<ack_request> active_requests;
std::thread worker;
bool initialized = false;
bool stopping = false;

bool same_request(const ack_request &left, const ack_request &right)
{
	return left.player_pid == right.player_pid &&
	       critical_operation_id_equal(left.offering_operation, right.offering_operation);
}

void remove_active_request(const ack_request &request)
{
	const auto found = std::find_if(active_requests.begin(), active_requests.end(),
					[&](const ack_request &active)
					{ return same_request(active, request); });
	if (found != active_requests.end())
		active_requests.erase(found);
}

#ifndef __NO_MYSQL__
// Owns only the explicit v6 ACK transaction; original autocommit v1-5 stays below.
quest_reward_ack_completion execute_native_fee_ack(MYSQL *connection, const ack_request &request,
						   const std::vector<uint8_t> &original_literal)
{
	quest_reward_ack_completion completion;
	completion.player_pid = request.player_pid;
	completion.offering_operation = request.offering_operation;
	completion.result = quest_reward_obligation_result::database_error;
	player_sql_pool_lease lease(connection);
	if (!connection || player_sql_idle_error(connection))
	{
		completion.error_code = EAGAIN;
		return completion;
	}
	player_sql_cleanup cleanup;
	player_sql_transaction_cleanup transaction(connection, cleanup);
	transaction.starting();
	bool committed = false;
	try
	{
		if (mysql_real_query(connection, "START TRANSACTION", 17))
			throw mysql_errno(connection);
		std::vector<uint8_t> literal;
		quest_reward_continuation terms;
		quest_reward_read_metrics preflight;
		unsigned int error = 0;
		const auto read = quest_reward_obligation_repository_read_ack_terms(
			connection, request.player_pid, request.offering_operation, &literal,
			&terms, &error, &preflight);
		if (read != quest_reward_obligation_result::ok || error ||
		    !quest_reward_is_fee_only(terms) || literal != original_literal ||
		    !transaction.same_session())
		{
			completion.result = read == quest_reward_obligation_result::ok ?
						    quest_reward_obligation_result::corrupt :
						    read;
			completion.error_code = error;
		}
		else if (!quest_reward_obligation_native_fee_owner::verify_in_transaction(
				 connection, request.offering_operation, literal) ||
			 !transaction.same_session())
		{
			// Missing retained carrier/proof is pending; raw current rows never substitute.
			completion.result = quest_reward_obligation_result::pending_effects;
			completion.error_code = EAGAIN;
		}
		else
		{
			// All original immutable child/parent/root proofs precede this first ACK DML.
			completion.result = quest_reward_obligation_repository_acknowledge(
				connection, request.player_pid, request.offering_operation,
				&completion.error_code);
			if (!completion.error_code && transaction.same_session() &&
			    (completion.result == quest_reward_obligation_result::ok ||
			     completion.result ==
				     quest_reward_obligation_result::already_acknowledged))
			{
				transaction.committing();
				if (mysql_real_query(connection, "COMMIT", 6))
					throw mysql_errno(connection);
				if (!transaction.committed())
					throw cleanup.cleanup_error;
				committed = true;
			}
		}
	}
	catch (unsigned int error)
	{
		completion.result = quest_reward_obligation_result::database_error;
		completion.error_code = error ? error : EIO;
	}
	catch (...)
	{
		completion.result = quest_reward_obligation_result::database_error;
		completion.error_code = ENOMEM;
	}
	transaction.finish();
	if (!transaction.commit_attempted() || committed)
		lease.reuse(cleanup);
	if (!transaction.same_session() || cleanup.cleanup_error ||
	    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
	    (!committed && !cleanup.rollback_confirmed) ||
	    (transaction.commit_attempted() && !committed))
	{
		completion.result = quest_reward_obligation_result::database_error;
		completion.error_code = cleanup.cleanup_error ? cleanup.cleanup_error : EIO;
	}
	return completion;
}
#endif

quest_reward_ack_completion execute_ack(const ack_request &request)
{
	quest_reward_ack_completion completion;
	completion.player_pid = request.player_pid;
	completion.offering_operation = request.offering_operation;
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
	{
		const char *root = persistence_mode_flatfile_root();
		std::string error;
		if (!root)
		{
			completion.result = quest_reward_obligation_result::database_error;
			completion.error_code = EINVAL;
			return completion;
		}
		const auto result = flatfile_item_repository_ack_quest_reward(
			root, request.player_pid, request.offering_operation, &error);
		if (result == flatfile_item_repository_result::ok)
			completion.result = quest_reward_obligation_result::ok;
		else if (result == flatfile_item_repository_result::unchanged)
			completion.result = quest_reward_obligation_result::already_acknowledged;
		else if (result == flatfile_item_repository_result::not_found)
			completion.result = quest_reward_obligation_result::not_found;
		else
		{
			completion.result = quest_reward_obligation_result::database_error;
			completion.error_code =
				result == flatfile_item_repository_result::io_error ? EIO : EILSEQ;
		}
		return completion;
	}

#ifdef __NO_MYSQL__
	completion.result = quest_reward_obligation_result::database_error;
	completion.error_code = ENOTSUP;
#else
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
	{
		completion.result = quest_reward_obligation_result::database_error;
		completion.error_code = EAGAIN;
		return completion;
	}
	// Bounded literal discriminator only, before selecting the original owner.
	// V6 repeats this exact literal read on its trusted same-session transaction.
	std::vector<uint8_t> literal;
	quest_reward_continuation terms;
	quest_reward_read_metrics preflight;
	const auto read = quest_reward_obligation_repository_read_ack_terms(
		connection, request.player_pid, request.offering_operation, &literal, &terms,
		&completion.error_code, &preflight);
	if (read == quest_reward_obligation_result::ok && quest_reward_is_fee_only(terms))
		return execute_native_fee_ack(connection, request,
					      literal); // consumes this pool lease
	if (read != quest_reward_obligation_result::ok &&
	    read != quest_reward_obligation_result::not_found)
	{
		completion.result = read;
		if (completion.error_code && mysql_errno(connection) >= CR_MIN_ERROR)
			sql_pool_discard_connection(connection);
		sql_pool_release(connection);
		return completion;
	}
	completion.result = quest_reward_obligation_repository_acknowledge(
		connection, request.player_pid, request.offering_operation, &completion.error_code);
	if (completion.result == quest_reward_obligation_result::database_error &&
	    mysql_errno(connection) >= CR_MIN_ERROR)
		sql_pool_discard_connection(connection);
	sql_pool_release(connection);
#endif
	return completion;
}

void worker_main()
{
	const bool thread_ready = sql_worker_thread_init() == 0;
	for (;;)
	{
		ack_request request;
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			request_ready.wait(lock, [] { return stopping || !requests.empty(); });
			if (stopping)
				break;
			request = requests.front();
			requests.pop_front();
		}
		quest_reward_ack_completion completion;
		if (thread_ready)
			completion = execute_ack(request);
		else
		{
			completion.player_pid = request.player_pid;
			completion.offering_operation = request.offering_operation;
			completion.result = quest_reward_obligation_result::database_error;
			completion.error_code = EIO;
		}
		if (completion.result == quest_reward_obligation_result::pending_effects)
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			const bool stopped = request_ready.wait_for(lock, std::chrono::seconds(5),
								    [] { return stopping; });
			if (stopped)
				break;
			try
			{
				requests.push_back(request);
			}
			catch (...)
			{
				remove_active_request(request);
			}
			lock.unlock();
			request_ready.notify_one();
			continue;
		}
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		try
		{
			completions.push_back(completion);
			network_wakeup_notify();
		}
		catch (...)
		{
			remove_active_request(request);
			// The retained obligation remains pending unless the ack already committed;
			// a later load can safely resubmit either outcome.
		}
	}
#ifndef __NO_MYSQL__
	mysql_thread_end();
#endif
}
} // namespace

bool quest_reward_obligation_pipeline_init()
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (initialized || worker.joinable())
		return false;
	stopping = false;
	try
	{
		worker = std::thread(worker_main);
	}
	catch (...)
	{
		return false;
	}
	initialized = true;
	return true;
}

void quest_reward_obligation_pipeline_shutdown()
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		stopping = true;
		requests.clear();
		active_requests.clear();
		request_ready.notify_all();
	}
	if (worker.joinable())
		worker.join();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	completions.clear();
	initialized = false;
	stopping = false;
}

quest_reward_ack_submit_result
quest_reward_obligation_pipeline_submit(uint32_t player_pid,
					const critical_operation_id &offering_operation)
{
	if (!player_pid || critical_operation_id_is_zero(offering_operation))
		return quest_reward_ack_submit_result::invalid;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!initialized || stopping)
		return quest_reward_ack_submit_result::unavailable;
	const ack_request request = { player_pid, offering_operation };
	if (std::find_if(active_requests.begin(), active_requests.end(),
			 [&](const ack_request &active)
			 { return same_request(active, request); }) != active_requests.end())
		return quest_reward_ack_submit_result::already_queued;
	if (active_requests.size() >= QUEST_REWARD_ACK_PIPELINE_MAX)
		return quest_reward_ack_submit_result::overloaded;
	try
	{
		active_requests.push_back(request);
		requests.push_back(request);
	}
	catch (...)
	{
		if (!active_requests.empty() && same_request(active_requests.back(), request))
			active_requests.pop_back();
		return quest_reward_ack_submit_result::overloaded;
	}
	request_ready.notify_one();
	return quest_reward_ack_submit_result::queued;
}

size_t quest_reward_obligation_pipeline_pulse(quest_reward_ack_completion *output, size_t capacity)
{
	if (!output || !capacity)
		return 0;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	const size_t count =
		std::min({ capacity, QUEST_REWARD_ACK_PIPELINE_PULSE_MAX, completions.size() });
	for (size_t index = 0; index < count; ++index)
	{
		output[index] = completions.front();
		remove_active_request(
			{ output[index].player_pid, output[index].offering_operation });
		completions.pop_front();
	}
	return count;
}
