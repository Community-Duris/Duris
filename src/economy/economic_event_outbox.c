#include "economy/economic_event_outbox.h"
#include "player/player_save_worker.h"
#ifndef __NO_MYSQL__
#include "sql/sql_pool.h"
#endif
#include "core/prototypes.h"
#include "core/utils.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

namespace
{
constexpr size_t OUTBOX_CAPACITY = 16384;
std::mutex outbox_mutex;
std::deque<EconomicEvent> outbox_queue;
uint64_t next_event_id = 1;
EconomicOutboxHealth outbox_health = {};
bool table_initialized = false;

uint64_t current_time_usec(void)
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		       std::chrono::system_clock::now().time_since_epoch())
		.count();
}

player_save_apply_result write_economy_events_to_db(const std::vector<EconomicEvent> &events)
{
#ifdef __NO_MYSQL__
	(void)events;
	return { player_save_apply_outcome::applied, 0, 0 };
#else
	if (events.empty())
		return { player_save_apply_outcome::applied, 0, 0 };

	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return { player_save_apply_outcome::retryable_failure, 0, ETIMEDOUT };

	if (!table_initialized)
	{
		const char *create_sql =
			"CREATE TABLE IF NOT EXISTS economy_events ("
			"event_id BIGINT UNSIGNED NOT NULL PRIMARY KEY, "
			"timestamp_usec BIGINT UNSIGNED NOT NULL, "
			"actor_pid INT UNSIGNED NOT NULL, "
			"action_type SMALLINT UNSIGNED NOT NULL, "
			"copper_delta BIGINT NOT NULL, "
			"target_id INT UNSIGNED NOT NULL DEFAULT 0, "
			"context_vnum INT UNSIGNED NOT NULL DEFAULT 0, "
			"reason VARCHAR(32) NOT NULL DEFAULT '', "
			"INDEX idx_actor_time (actor_pid, timestamp_usec), "
			"INDEX idx_action (action_type)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
		if (mysql_query(connection, create_sql) != 0)
		{
			unsigned int err = mysql_errno(connection);
			sql_pool_release(connection);
			if (err == 2006 || err == 2013)
				return { player_save_apply_outcome::retryable_failure, 0, err };
			return { player_save_apply_outcome::terminal_failure, 0, err };
		}
		table_initialized = true;
	}

	for (const auto &event : events)
	{
		char safe_reason[65] = {};
		mysql_real_escape_string(connection, safe_reason, event.reason, strlen(event.reason));
		char sql[512];
		snprintf(sql, sizeof(sql),
			 "INSERT INTO economy_events (event_id, timestamp_usec, actor_pid, action_type, copper_delta, target_id, context_vnum, reason) "
			 "VALUES (%llu, %llu, %u, %u, %lld, %u, %u, '%s') "
			 "ON DUPLICATE KEY UPDATE timestamp_usec = VALUES(timestamp_usec)",
			 static_cast<unsigned long long>(event.event_id),
			 static_cast<unsigned long long>(event.timestamp_usec),
			 event.actor_pid,
			 static_cast<unsigned int>(event.action),
			 static_cast<long long>(event.copper_delta),
			 event.target_id,
			 event.context_vnum,
			 safe_reason);

		if (mysql_query(connection, sql) != 0)
		{
			unsigned int err = mysql_errno(connection);
			sql_pool_release(connection);
			if (err == 2006 || err == 2013)
				return { player_save_apply_outcome::retryable_failure, 0, err };
			return { player_save_apply_outcome::terminal_failure, 0, err };
		}
	}

	sql_pool_release(connection);
	return { player_save_apply_outcome::applied, 0, 0 };
#endif
}
} // namespace

void economic_event_record(uint32_t actor_pid, EconomicAction action, int64_t copper_delta,
			   uint32_t target_id, uint32_t context_vnum, const char *reason)
{
	std::lock_guard<std::mutex> lock(outbox_mutex);

	// Resilient open-world classification:
	// If the action is unknown or novel, map to UNCLASSIFIED fallback without panicking
	if (action == EconomicAction::UNKNOWN)
	{
		if (copper_delta >= 0)
		{
			action = EconomicAction::UNCLASSIFIED_INFLOW;
			++outbox_health.unclassified_inflows;
		}
		else
		{
			action = EconomicAction::UNCLASSIFIED_OUTFLOW;
			++outbox_health.unclassified_outflows;
		}
		logit(LOG_DEBUG, "economy: unclassified event for pid %u, delta %lld (reason: %s)",
		      actor_pid, static_cast<long long>(copper_delta), reason ? reason : "none");
	}
	else if (action == EconomicAction::UNCLASSIFIED_INFLOW)
	{
		++outbox_health.unclassified_inflows;
	}
	else if (action == EconomicAction::UNCLASSIFIED_OUTFLOW)
	{
		++outbox_health.unclassified_outflows;
	}

	EconomicEvent event = {};
	event.event_id = next_event_id++;
	event.timestamp_usec = current_time_usec();
	event.actor_pid = actor_pid;
	event.action = action;
	event.copper_delta = copper_delta;
	event.target_id = target_id;
	event.context_vnum = context_vnum;
	if (reason)
		snprintf(event.reason, sizeof(event.reason), "%s", reason);

	if (outbox_queue.size() >= OUTBOX_CAPACITY)
	{
		outbox_queue.pop_front();
		++outbox_health.total_dropped;
	}

	outbox_queue.push_back(event);
	++outbox_health.total_recorded;
	outbox_health.queue_depth = outbox_queue.size();
}

size_t economic_event_drain(std::vector<EconomicEvent> *batch_out, size_t max_events)
{
	if (!batch_out || !max_events)
		return 0;

	std::lock_guard<std::mutex> lock(outbox_mutex);
	size_t drained = 0;
	while (drained < max_events && !outbox_queue.empty())
	{
		batch_out->push_back(outbox_queue.front());
		outbox_queue.pop_front();
		++drained;
	}

	outbox_health.total_drained += drained;
	outbox_health.queue_depth = outbox_queue.size();
	return drained;
}

EconomicOutboxHealth economic_event_outbox_health(void)
{
	std::lock_guard<std::mutex> lock(outbox_mutex);
	EconomicOutboxHealth copy = outbox_health;
	copy.queue_depth = outbox_queue.size();
	return copy;
}

void economic_event_flush_outbox(void)
{
	std::vector<EconomicEvent> batch;
	batch.reserve(256);
	const size_t drained = economic_event_drain(&batch, 256);
	if (!drained)
		return;

	static uint64_t next_owner_id = 1;
	const uint64_t owner = next_owner_id++;
	const size_t bytes = batch.size() * sizeof(EconomicEvent);

	persistence_writer_submit(
		persistence_job_kind::economy, owner, bytes,
		[events = std::move(batch)]() -> player_save_apply_result {
			return write_economy_events_to_db(events);
		});
}

void economic_event_outbox_reset_for_tests(void)
{
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_queue.clear();
	next_event_id = 1;
	outbox_health = {};
	table_initialized = false;
}
