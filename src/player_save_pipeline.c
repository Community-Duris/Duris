#include "player_save_pipeline.h"

#include "prototypes.h"
#include "files.h"
#include "player_save_journal.h"
#include "player_save_worker.h"
#include "player_snapshot_capture.h"
#include "player_snapshot_repository.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#endif

namespace
{
std::mutex pipeline_mutex;
std::condition_variable append_available;
std::deque<player_snapshot> pending_append;
std::deque<player_snapshot> durable_ready;
std::thread dispatcher;
player_save_pipeline_health health = {};
size_t retained_bytes = 0;
bool stop_requested = false;

void update_depth_locked()
{
	health.pending_append = pending_append.size();
	health.durable_ready = durable_ready.size();
	health.retained_bytes = retained_bytes;
	health.high_water_snapshots =
		std::max(health.high_water_snapshots, health.pending_append + health.durable_ready);
	health.high_water_bytes =
		std::max(health.high_water_bytes, static_cast<uint64_t>(retained_bytes));
}

void dispatcher_main()
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.dispatcher_running = true;
	}
	player_save_journal_result replay = player_save_journal_result::replay_blocked;
#ifndef __NO_MYSQL__
	const bool mysql_ready = mysql_thread_init() == 0;
#else
	const bool mysql_ready = true;
#endif
	if (mysql_ready)
		replay = player_save_journal_replay(player_snapshot_repository_apply_from_pool,
						    nullptr);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.replay_complete = replay == player_save_journal_result::ok;
		health.replay_blocked = replay != player_save_journal_result::ok;
	}

	for (;;)
	{
		player_snapshot snapshot;
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			append_available.wait(
				lock, [] { return stop_requested || !pending_append.empty(); });
			if (stop_requested && pending_append.empty())
				break;
			snapshot = std::move(pending_append.front());
			pending_append.pop_front();
			update_depth_locked();
		}

		const player_save_journal_result appended = player_save_journal_append(snapshot);
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (appended == player_save_journal_result::ok)
			{
				try
				{
					durable_ready.push_back(std::move(snapshot));
				}
				catch (const std::bad_alloc &)
				{
					retained_bytes -= snapshot.encoded_size_bound;
					++health.durable_spills;
					++health.overloads;
				}
			}
			else
			{
				++health.append_failures;
				try
				{
					pending_append.push_front(std::move(snapshot));
				}
				catch (const std::bad_alloc &)
				{
					retained_bytes -= snapshot.encoded_size_bound;
					++health.overloads;
				}
			}
			update_depth_locked();
		}
		if (appended != player_save_journal_result::ok)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health.dispatcher_running = false;
#ifndef __NO_MYSQL__
	if (mysql_ready)
		mysql_thread_end();
#endif
}

bool snapshot_is_retained_locked(int pid, player_revision_t revision)
{
	for (const player_snapshot &snapshot : pending_append)
		if (snapshot.pid == pid && snapshot.revision == revision)
			return true;
	for (const player_snapshot &snapshot : durable_ready)
		if (snapshot.pid == pid && snapshot.revision == revision)
			return true;
	return false;
}

player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || stop_requested)
		return player_save_pipeline_result::unavailable;
	for (player_snapshot &queued : pending_append)
	{
		if (queued.pid != snapshot.pid)
			continue;
		if (snapshot.revision <= queued.revision)
			return player_save_pipeline_result::coalesced;
		if ((snapshot.components & queued.components) != queued.components)
			return player_save_pipeline_result::capture_failed;
		if (snapshot.encoded_size_bound >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - (retained_bytes - queued.encoded_size_bound))
		{
			++health.overloads;
			return player_save_pipeline_result::overloaded;
		}
		retained_bytes =
			retained_bytes - queued.encoded_size_bound + snapshot.encoded_size_bound;
		queued = std::move(snapshot);
		++health.coalesced;
		update_depth_locked();
		return player_save_pipeline_result::coalesced;
	}
	if (pending_append.size() + durable_ready.size() >= PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    snapshot.encoded_size_bound > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	retained_bytes += snapshot.encoded_size_bound;
	try
	{
		pending_append.push_back(std::move(snapshot));
	}
	catch (const std::bad_alloc &)
	{
		retained_bytes -= snapshot.encoded_size_bound;
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	return player_save_pipeline_result::queued;
}
} // namespace

bool player_save_pipeline_init(const char *journal_directory)
{
	if (!journal_directory || journal_directory[0] != '/')
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (health.initialized)
			return false;
	}
	if (!player_save_journal_init(journal_directory, PLAYER_SAVE_JOURNAL_MAX_BYTES))
		return false;
	if (!player_save_worker_init(player_snapshot_repository_apply_from_pool, nullptr))
	{
		player_save_journal_shutdown();
		return false;
	}
	if (!player_save_worker_set_journal_hooks(nullptr, player_save_journal_worker_ack, nullptr))
	{
		player_save_worker_shutdown();
		player_save_journal_shutdown();
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health = {};
		health.initialized = true;
		stop_requested = false;
	}
	try
	{
		dispatcher = std::thread(dispatcher_main);
	}
	catch (const std::system_error &)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.initialized = false;
		player_save_worker_shutdown();
		player_save_journal_shutdown();
		return false;
	}
	return true;
}

void player_save_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		stop_requested = true;
		append_available.notify_all();
	}
	if (dispatcher.joinable())
		dispatcher.join();
	player_save_worker_shutdown();
	player_save_journal_shutdown();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	pending_append.clear();
	durable_ready.clear();
	retained_bytes = 0;
	health.initialized = false;
	update_depth_locked();
}

bool player_save_pipeline_mark(int pid, player_component_mask_t components)
{
	player_revision_t revision = 0;
	if (!player_revision_mark(pid, components, &revision))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.marked;
	return true;
}

player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return player_save_pipeline_result::invalid;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(GET_PID(ch), &revision))
		return player_save_pipeline_result::unavailable;
	if (!revision.dirty_components)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (revision.inflight_components || !revision.queued_components ||
		    snapshot_is_retained_locked(GET_PID(ch), revision.queued_revision))
		{
			++health.unchanged;
			return player_save_pipeline_result::unchanged;
		}
	}
	player_revision_t queued_revision = 0;
	player_component_mask_t components = 0;
	if (!player_revision_queue(GET_PID(ch), &queued_revision, &components))
		return player_save_pipeline_result::capture_failed;
	player_snapshot snapshot;
	if (player_snapshot_capture(ch, queued_revision, components, save_intent, room_vnum,
				    &snapshot) != player_snapshot_capture_result::ok)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		++health.capture_failures;
		return player_save_pipeline_result::capture_failed;
	}
	return enqueue_snapshot(std::move(snapshot));
}

player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum)
{
	if (!ch || IS_NPC(ch) || !player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::invalid;
	return player_save_pipeline_checkpoint_dirty(ch, save_intent, room_vnum);
}

void player_save_pipeline_pulse(void)
{
	player_save_completion completions[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	const size_t completed =
		player_save_worker_pulse(completions, PLAYER_SAVE_PIPELINE_PULSE_BUDGET);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.completions += completed;
	}
	for (size_t count = 0; count < PLAYER_SAVE_PIPELINE_PULSE_BUDGET; ++count)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (durable_ready.empty())
			break;
		const size_t snapshot_bytes = durable_ready.front().encoded_size_bound;
		const player_save_submit_result submitted =
			player_save_worker_submit_retained(&durable_ready.front());
		if (submitted == player_save_submit_result::worker_unavailable ||
		    submitted == player_save_submit_result::capacity_exceeded)
			break;
		retained_bytes -= snapshot_bytes;
		durable_ready.pop_front();
		if (submitted == player_save_submit_result::durably_spilled)
			++health.durable_spills;
		else
			++health.dispatched;
		update_depth_locked();
	}
}

player_save_pipeline_health player_save_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return health;
}

size_t player_save_pipeline_dirty_count(void)
{
	return player_revision_dirty_count();
}

bool player_save_pipeline_is_nonterminal_type(int save_intent)
{
	return save_intent != RENT_INN && save_intent != RENT_LINKDEAD &&
	       save_intent != RENT_CAMPED && save_intent != RENT_DEATH &&
	       save_intent != RENT_POOFARTI && save_intent != RENT_SWAPARTI &&
	       save_intent != RENT_FIGHTARTI;
}

void player_save_pipeline_reset_for_tests(void)
{
	player_save_pipeline_shutdown();
	player_save_worker_reset_for_tests();
	player_revision_reset_for_tests();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
	stop_requested = false;
}
