#include "player/player_save_pipeline.h"
#include "sql_thread_init.h"

#include "prototypes.h"
#include "files.h"
#include "flatfile/flatfile_player_repository.h"
#include "player/player_save_journal.h"
#include "player/player_save_worker.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_repository.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <cerrno>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

extern P_char character_list;

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
bool accepting = false;
bool append_inflight = false;

player_save_apply_fn selected_snapshot_apply()
{
#ifdef __NO_MYSQL__
	return flatfile_player_snapshot_apply_selected;
#else
	return player_snapshot_repository_apply_from_pool;
#endif
}

struct terminal_fence
{
	int pid = 0;
	player_revision_t revision = 0;
	bool journaled = false;
	bool acknowledged = false;
};

std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS> terminal_fences = {};

struct journaled_revision
{
	int pid = 0;
	player_revision_t revision = 0;
};

std::array<journaled_revision, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS> journaled_revisions = {};

void remember_journaled_revision_locked(int pid, player_revision_t revision)
{
	for (journaled_revision &entry : journaled_revisions)
		if (entry.pid == pid && entry.revision == revision)
			return;
	for (journaled_revision &entry : journaled_revisions)
		if (!entry.pid)
		{
			entry = { pid, revision };
			return;
		}
}

void forget_journaled_revisions_locked(int pid, player_revision_t durable_revision)
{
	for (journaled_revision &entry : journaled_revisions)
		if (entry.pid == pid && entry.revision <= durable_revision)
			entry = {};
}

terminal_fence *find_terminal_fence_locked(int pid)
{
	for (terminal_fence &fence : terminal_fences)
		if (fence.pid == pid)
			return &fence;
	return nullptr;
}

terminal_fence *allocate_terminal_fence_locked(int pid)
{
	if (terminal_fence *existing = find_terminal_fence_locked(pid))
		return existing;
	for (terminal_fence &fence : terminal_fences)
		if (!fence.pid)
		{
			fence.pid = pid;
			++health.terminal_fences;
			return &fence;
		}
	return nullptr;
}

void update_depth_locked()
{
	health.pending_append = pending_append.size();
	health.durable_ready = durable_ready.size();
	health.retained_bytes = retained_bytes;
	health.accepting = accepting;
	health.append_inflight = append_inflight;
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
	const bool mysql_ready = sql_worker_thread_init() == 0;
#else
	const bool mysql_ready = true;
#endif
	if (mysql_ready)
		replay = player_save_journal_replay(selected_snapshot_apply(), nullptr);
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
			append_inflight = true;
			update_depth_locked();
		}

		const player_save_journal_result appended = player_save_journal_append(snapshot);
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (appended == player_save_journal_result::ok)
			{
				remember_journaled_revision_locked(snapshot.pid, snapshot.revision);
				try
				{
					durable_ready.push_back(std::move(snapshot));
					if (terminal_fence *fence = find_terminal_fence_locked(
						    durable_ready.back().pid);
					    fence &&
					    fence->revision == durable_ready.back().revision)
						fence->journaled = true;
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
			append_inflight = false;
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

bool snapshot_is_journaled_locked(const player_revision_snapshot &revision)
{
	for (const journaled_revision &entry : journaled_revisions)
		if (entry.pid == revision.pid && entry.revision == revision.current_revision)
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
	if (!player_save_worker_init(selected_snapshot_apply(), nullptr))
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
		accepting = true;
		append_inflight = false;
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
	terminal_fences.fill({});
	journaled_revisions.fill({});
	retained_bytes = 0;
	accepting = false;
	append_inflight = false;
	health.initialized = false;
	update_depth_locked();
}

bool player_save_pipeline_mark(int pid, player_component_mask_t components)
{
#ifdef __NO_MYSQL__
	if (components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		components |= PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
#endif
	bool fenced = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!accepting)
			return false;
		fenced = find_terminal_fence_locked(pid) != nullptr;
	}
	player_revision_t revision = 0;
	if (!player_revision_mark(pid, components, &revision))
		return false;
	if (fenced)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (terminal_fence *fence = find_terminal_fence_locked(pid))
		{
			fence->revision = revision;
			fence->journaled = false;
			fence->acknowledged = false;
		}
	}
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

player_save_terminal_result player_save_pipeline_terminal(P_char ch, int save_intent, int room_vnum,
							  uint64_t timeout_msec,
							  bool allow_journal_handoff)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	const int pid = GET_PID(ch);
	player_revision_t revision = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized)
			return player_save_terminal_result::unavailable;
		terminal_fence *fence = allocate_terminal_fence_locked(pid);
		if (!fence)
			return player_save_terminal_result::unavailable;
		revision = fence->revision;
	}
	if (!revision)
	{
		player_revision_snapshot current = {};
		if (!player_revision_snapshot_copy(pid, &current))
			return player_save_terminal_result::unavailable;
		const bool promote_existing =
			current.current_revision > current.acknowledged_revision &&
			current.unacknowledged_components == PLAYER_CHECKPOINT_COMPONENT_ALL;
		if (promote_existing)
			revision = current.current_revision;
		else if (!player_revision_mark(pid, PLAYER_CHECKPOINT_COMPONENT_ALL, &revision))
			return player_save_terminal_result::unavailable;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		terminal_fence *fence = find_terminal_fence_locked(pid);
		if (!fence)
			return player_save_terminal_result::unavailable;
		fence->revision = revision;
		if (promote_existing)
			fence->journaled = snapshot_is_journaled_locked(current);
	}
	player_save_pipeline_checkpoint_dirty(ch, save_intent, room_vnum);
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		player_save_pipeline_pulse();
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			terminal_fence *fence = find_terminal_fence_locked(pid);
			if (!fence || fence->revision != revision)
				return player_save_terminal_result::unavailable;
			if (fence->acknowledged)
			{
				++health.terminal_database_acks;
				*fence = {};
				return player_save_terminal_result::database_acknowledged;
			}
			if (allow_journal_handoff && fence->journaled)
			{
				++health.terminal_journal_handoffs;
				*fence = {};
				return player_save_terminal_result::journal_durable;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.terminal_timeouts;
	return player_save_terminal_result::timed_out;
}

void player_save_pipeline_pulse(void)
{
	player_save_completion completions[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	int32_t missing_baseline[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t missing_baseline_count = 0;
	const size_t completed =
		player_save_worker_pulse(completions, PLAYER_SAVE_PIPELINE_PULSE_BUDGET);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.completions += completed;
		for (size_t index = 0; index < completed; ++index)
		{
			if ((completions[index].outcome == player_save_apply_outcome::applied ||
			     completions[index].outcome ==
				     player_save_apply_outcome::already_applied ||
			     completions[index].outcome ==
				     player_save_apply_outcome::stale_revision) &&
			    completions[index].durable_revision >= completions[index].revision)
				forget_journaled_revisions_locked(
					completions[index].pid,
					completions[index].durable_revision);
			if (terminal_fence *fence =
				    find_terminal_fence_locked(completions[index].pid);
			    fence && fence->revision == completions[index].revision &&
			    (completions[index].outcome == player_save_apply_outcome::applied ||
			     completions[index].outcome ==
				     player_save_apply_outcome::already_applied ||
			     completions[index].outcome ==
				     player_save_apply_outcome::stale_revision) &&
			    completions[index].durable_revision >= fence->revision)
				fence->acknowledged = true;
			// The worker only ever UPDATEs player_data. A missing row means the
			// character never got its baseline INSERT, and every further async
			// save would fail the same way; record it for the sync fallback.
			if (completions[index].outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    completions[index].error_code == ENOENT && completions[index].pid > 0)
				missing_baseline[missing_baseline_count++] = completions[index].pid;
		}
	}
	for (size_t index = 0; index < missing_baseline_count; ++index)
	{
		const int32_t pid = missing_baseline[index];
		bool rearmed = false;
		for (P_char ch = character_list; ch; ch = ch->next)
			if (IS_PC(ch) && GET_PID(ch) == pid)
			{
				SET_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE);
				rearmed = true;
				break;
			}
		logit(LOG_PLAYER,
		      "player_save_pipeline_pulse: component=apply outcome=missing_baseline "
		      "pid=%d sync_fallback=%d",
		      pid, rearmed ? 1 : 0);
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

void player_save_pipeline_quiesce(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	accepting = false;
	update_depth_locked();
}

void player_save_pipeline_resume(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (health.initialized && !stop_requested)
		accepting = true;
	update_depth_locked();
}

bool player_save_pipeline_drain(uint64_t timeout_msec)
{
	if (!timeout_msec)
		return false;
	player_save_pipeline_quiesce();
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		player_save_pipeline_pulse();
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (pending_append.empty() && !append_inflight)
				return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.drain_failures;
	return false;
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
	journaled_revisions.fill({});
	stop_requested = false;
	accepting = false;
	append_inflight = false;
}
