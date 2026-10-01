#include "player/player_save_pipeline.h"
#include "sql/sql_thread_init.h"
#include "persistence/persistence_observability.h"
#include <cstdlib>
#include <cstring>

#include "core/prototypes.h"
#include "core/files.h"
#include "flatfile/flatfile_player_repository.h"
#include "player/player_save_journal.h"
#include "player/player_save_worker.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spell_item_lifecycle.h"
#include "item/item_movement_transaction.h"
#include "world/quest_reward_recovery.h"
#include "player/craft_progression_hooks.h"

#include <algorithm>
#include <cerrno>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>

extern P_char character_list;

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#ifdef TEST_MUD
#include "player/player_death_conflict_repository.h"
#endif
#endif

namespace
{
/** Cache the explicit diagnostic switch for player-save capture and durability tracing. */
bool trace_player_saves()
{
	static const bool enabled = []
	{
		const char *value = std::getenv("DURIS_NEVENT_TRACE_PLAYER");
		return value && std::strcmp(value, "1") == 0;
	}();
	return enabled;
}

std::mutex pipeline_mutex;
std::condition_variable append_available;
std::deque<player_snapshot> pending_append;
std::deque<player_snapshot> durable_ready;
std::thread dispatcher;
player_save_pipeline_health health = {};
player_save_pipeline_replay_gate replay_gate;
size_t retained_bytes = 0;
bool stop_requested = false;
bool accepting = false;
bool append_inflight = false;
int append_inflight_pid = 0;
player_revision_t append_inflight_revision = 0;
/* One failed graph may be recaptured once.  Keep the PID armed until a later
 * database acknowledgement proves custody and payload agree; otherwise every
 * rejected recapture creates a new revision and an unbounded wizlog loop. */
std::set<int32_t> custody_recapture_armed;

player_save_apply_fn selected_snapshot_apply()
{
#ifdef __NO_MYSQL__
	return flatfile_player_snapshot_apply_selected;
#else
#ifdef TEST_MUD
	// Qualification only: no normal/production selection before the socket,
	// recovery-view and restart acceptance gates. Both worker and journal
	// replay use this selector; never substitute a different replay owner.
	const char *enabled = std::getenv("DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY");
	const char *disposable = std::getenv("TEST_DB_DISPOSABLE");
	const char *host = std::getenv("DB_HOST");
	const char *database = std::getenv("DB_NAME");
	constexpr char prefix[] = "corpse_journey_test_";
	constexpr size_t prefix_length = sizeof(prefix) - 1;
	if (enabled && std::strcmp(enabled, "1") == 0 && disposable &&
	    std::strcmp(disposable, "1") == 0 && host &&
	    (std::strcmp(host, "127.0.0.1") == 0 || std::strcmp(host, "localhost") == 0) &&
	    database && std::strlen(database) == prefix_length + 12 &&
	    std::strncmp(database, prefix, prefix_length) == 0 &&
	    std::strspn(database + prefix_length, "0123456789abcdef") == 12)
		return player_death_conflict_apply_from_pool;
#endif
	return player_snapshot_repository_apply_from_pool;
#endif
}

struct terminal_fence
{
	int pid = 0;
	player_revision_t revision = 0;
	bool journaled = false;
	bool acknowledged = false;
	bool death_pinned = false;
	uint64_t corpse_uid = 0;
	critical_operation_id operation_id = {};
	uint64_t wallet_pile_uid = 0;
	std::optional<player_snapshot> death_snapshot;
};

static_assert(std::is_nothrow_move_constructible_v<player_snapshot>);

std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS> terminal_fences = {};

struct target_save_login_fence
{
	int pid = 0;
	player_revision_t expected_revision = 0;
};

std::array<target_save_login_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>
	target_save_login_fences = {};

target_save_login_fence *find_target_save_login_fence_locked(int pid);

/** Find a player durability fence; the caller must hold pipeline_mutex. */
terminal_fence *find_terminal_fence_locked(int pid)
{
	for (terminal_fence &fence : terminal_fences)
		if (fence.pid == pid)
			return &fence;
	return nullptr;
}

terminal_fence *allocate_terminal_fence_locked(int pid)
{
	if (find_target_save_login_fence_locked(pid))
		return nullptr;
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

/** Find a recipient-only save/login fence; the caller must hold pipeline_mutex. */
target_save_login_fence *find_target_save_login_fence_locked(int pid)
{
	for (target_save_login_fence &fence : target_save_login_fences)
		if (fence.pid == pid)
			return &fence;
	return nullptr;
}

target_save_login_fence *
allocate_target_save_login_fence_locked(int pid, player_revision_t expected_revision)
{
	if (find_target_save_login_fence_locked(pid))
		return nullptr;
	for (target_save_login_fence &fence : target_save_login_fences)
		if (!fence.pid)
		{
			fence.pid = pid;
			fence.expected_revision = expected_revision;
			return &fence;
		}
	return nullptr;
}

size_t pinned_death_count_locked();

/** Refresh queue depth and high-water counters while pipeline_mutex is held. */
void update_depth_locked()
{
	health.pending_append = pending_append.size();
	health.durable_ready = durable_ready.size();
	health.retained_bytes = retained_bytes;
	health.accepting = accepting;
	health.append_inflight = append_inflight;
	health.high_water_snapshots =
		std::max(health.high_water_snapshots,
			 static_cast<uint64_t>(health.pending_append + health.durable_ready +
					       pinned_death_count_locked()));
	health.high_water_bytes =
		std::max(health.high_water_bytes, static_cast<uint64_t>(retained_bytes));
}

size_t pinned_death_count_locked()
{
	return static_cast<size_t>(std::count_if(terminal_fences.begin(), terminal_fences.end(),
						 [](const terminal_fence &fence)
						 { return fence.death_pinned; }));
}

void clear_terminal_fence_locked(terminal_fence &fence)
{
	if (fence.death_pinned)
	{
		(void)player_revision_unpin_terminal_death(fence.pid, fence.revision);
		if (fence.death_snapshot)
		{
			const size_t bytes = fence.death_snapshot->encoded_size_bound;
			retained_bytes = bytes <= retained_bytes ? retained_bytes - bytes : 0;
		}
	}
	fence = {};
	update_depth_locked();
}

bool death_snapshot_identity_matches(const terminal_fence &fence)
{
	if (!fence.death_snapshot)
		return false;
	const player_snapshot &snapshot = *fence.death_snapshot;
	return player_snapshot_is_death_request_schema(snapshot.schema_version) &&
	       snapshot.revision == fence.revision && snapshot.death &&
	       snapshot.death->operation_id.bytes == fence.operation_id.bytes &&
	       snapshot.death->wallet_pile_uid == fence.wallet_pile_uid &&
	       !snapshot.death->corpse.empty() &&
	       snapshot.death->corpse.front().object_uid == fence.corpse_uid;
}

/** Replay the journal, then append queued snapshots before making them eligible for persistence workers. */
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
		replay_gate.finish_replay(replay == player_save_journal_result::ok &&
					  health.initialized && !stop_requested);
	}

	player_snapshot snapshot;
	bool retry_inflight = false;
	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			if (!retry_inflight)
			{
				append_available.wait(
					lock,
					[] { return stop_requested || !pending_append.empty(); });
				if (stop_requested && pending_append.empty())
					break;
				snapshot = std::move(pending_append.front());
				pending_append.pop_front();
				append_inflight = true;
				append_inflight_pid = snapshot.pid;
				append_inflight_revision = snapshot.revision;
			}
			retry_inflight = false;
			update_depth_locked();
		}

		const player_save_journal_result appended = player_save_journal_append(snapshot);
		const bool quarantined = appended == player_save_journal_result::quarantined_pid;
		// A capture queued before the fence still needs durable preservation.
		// Archive it without admitting it to SQL or claiming a revision ACK.
		const bool quarantine_preserved =
			quarantined && player_save_journal_archive_quarantined(snapshot) ==
					       player_save_journal_result::ok;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (quarantine_preserved)
			{
				retained_bytes -= snapshot.encoded_size_bound;
				++health.durable_spills;
			}
			else if (appended == player_save_journal_result::ok)
			{
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
					if (quarantined)
						pending_append.push_back(std::move(snapshot));
					else
						pending_append.push_front(std::move(snapshot));
				}
				catch (const std::bad_alloc &)
				{
					// No durable copy exists. Keep the already-owned capture and
					// its in-flight fence until it is archived, appended, or requeued.
					retry_inflight = true;
					++health.overloads;
				}
			}
			if (!retry_inflight)
			{
				append_inflight = false;
				append_inflight_pid = 0;
				append_inflight_revision = 0;
			}
			update_depth_locked();
		}
		if (appended != player_save_journal_result::ok && !quarantine_preserved)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (!retry_inflight)
			snapshot = {};
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health.dispatcher_running = false;
#ifndef __NO_MYSQL__
	if (mysql_ready)
		mysql_thread_end();
#endif
}

/** Check retained queues for an exact player revision while pipeline_mutex is held. */
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

/** Check retained queues for any snapshot belonging to a player. */
bool any_snapshot_is_retained_locked(int pid)
{
	for (const player_snapshot &snapshot : pending_append)
		if (snapshot.pid == pid)
			return true;
	for (const player_snapshot &snapshot : durable_ready)
		if (snapshot.pid == pid)
			return true;
	return false;
}

bool merge_quest_xp_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.quest_xp_receipts.empty())
		return true;
	if (source.death ||
	    (!player_snapshot_is_death_request_schema(target->schema_version) &&
	     target->schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION))
		return false;
	size_t added = 0;
	for (const auto &candidate : source.quest_xp_receipts)
	{
		auto found = std::find_if(
			target->quest_xp_receipts.begin(), target->quest_xp_receipts.end(),
			[&](const auto &existing)
			{
				return existing.offering_operation.bytes ==
					       candidate.offering_operation.bytes &&
				       existing.reward_index == candidate.reward_index;
			});
		if (found != target->quest_xp_receipts.end())
		{
			if (found->amount != candidate.amount)
				return false;
			continue;
		}
		++added;
	}
	if (!added)
		return true;
	const size_t schema_overhead =
		target->schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ? 8 :
		(target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
		 target->schema_version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION) ?
										 4 :
										 0;
	if (target->quest_xp_receipts.size() + added > 64 ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    schema_overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - schema_overhead) / 24)
		return false;
	try
	{
		target->quest_xp_receipts.reserve(target->quest_xp_receipts.size() + added);
		for (const auto &candidate : source.quest_xp_receipts)
		{
			const bool present = std::any_of(
				target->quest_xp_receipts.begin(), target->quest_xp_receipts.end(),
				[&](const auto &existing)
				{
					return existing.offering_operation.bytes ==
						       candidate.offering_operation.bytes &&
					       existing.reward_index == candidate.reward_index;
				});
			if (!present)
				target->quest_xp_receipts.push_back(candidate);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (target->death && !player_snapshot_has_craft_receipt_schema(target->schema_version))
		target->schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
	else if (target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION)
		target->schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
	target->encoded_size_bound += schema_overhead + 24 * added;
	return true;
}

bool merge_spell_effect_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.spell_effect_receipts.empty())
		return true;
	if (source.death ||
	    (!player_snapshot_is_death_request_schema(target->schema_version) &&
	     target->schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION))
		return false;
	size_t added = 0;
	for (const auto &candidate : source.spell_effect_receipts)
	{
		auto found = std::find_if(
			target->spell_effect_receipts.begin(), target->spell_effect_receipts.end(),
			[&](const auto &existing)
			{ return existing.operation_id.bytes == candidate.operation_id.bytes; });
		if (found != target->spell_effect_receipts.end())
		{
			if (found->effect_id != candidate.effect_id)
				return false;
			continue;
		}
		++added;
	}
	if (!added)
		return true;
	const size_t schema_overhead =
		target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ? 8 :
		target->schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
				target->schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ?
									   4 :
									   0;
	if (target->spell_effect_receipts.size() + added > PLAYER_SPELL_EFFECT_RECEIPT_MAX ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    schema_overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - schema_overhead) / 20)
		return false;
	try
	{
		target->spell_effect_receipts.reserve(target->spell_effect_receipts.size() + added);
		for (const auto &candidate : source.spell_effect_receipts)
		{
			const bool present =
				std::any_of(target->spell_effect_receipts.begin(),
					    target->spell_effect_receipts.end(),
					    [&](const auto &existing) {
						    return existing.operation_id.bytes ==
							   candidate.operation_id.bytes;
					    });
			if (!present)
				target->spell_effect_receipts.push_back(candidate);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!player_snapshot_has_craft_receipt_schema(target->schema_version))
		target->schema_version =
			target->death ?
				(target->quest_xp_receipts.empty() ?
					 PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION :
					 PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION) :
				PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
	target->encoded_size_bound += schema_overhead + 20 * added;
	return true;
}

bool merge_craft_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.craft_receipts.empty())
		return true;
	size_t added = 0;
	for (const auto &candidate : source.craft_receipts)
	{
		const auto found = std::find_if(
			target->craft_receipts.begin(), target->craft_receipts.end(),
			[&](const auto &existing)
			{ return existing.operation_id.bytes == candidate.operation_id.bytes; });
		if (found != target->craft_receipts.end())
		{
			if (found->discipline != candidate.discipline ||
			    found->experience != candidate.experience)
				return false;
		}
		else
			++added;
	}
	const size_t overhead =
		player_snapshot_has_craft_receipt_schema(target->schema_version) ?
			0 :
			4 +
				(player_snapshot_has_quest_receipt_schema(target->schema_version) ?
					 0 :
					 4) +
				(player_snapshot_has_spell_receipt_schema(target->schema_version) ?
					 0 :
					 4);
	if (target->craft_receipts.size() + added > PLAYER_CRAFT_RECEIPT_MAX ||
	    (target->components & CRAFT_PROGRESSION_COMPONENTS) != CRAFT_PROGRESSION_COMPONENTS ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - overhead) / 24)
		return false;
	try
	{
		target->craft_receipts.reserve(target->craft_receipts.size() + added);
		for (const auto &candidate : source.craft_receipts)
			if (std::none_of(target->craft_receipts.begin(),
					 target->craft_receipts.end(),
					 [&](const auto &existing) {
						 return existing.operation_id.bytes ==
							candidate.operation_id.bytes;
					 }))
				target->craft_receipts.push_back(candidate);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	target->schema_version =
		target->death ? (target->death->conflict_evidence ?
					 PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION :
					 PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION) :
				PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
	target->encoded_size_bound += overhead + 24 * added;
	return true;
}

/** Admit or coalesce a snapshot within queue and byte limits before notifying the dispatcher. */
player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot)
{
	if (player_save_journal_pid_quarantined(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || stop_requested ||
	    find_target_save_login_fence_locked(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	if (terminal_fence *fence = find_terminal_fence_locked(snapshot.pid);
	    fence && fence->death_pinned)
		return player_save_pipeline_result::unavailable;
	for (player_snapshot &queued : pending_append)
	{
		if (queued.pid != snapshot.pid)
			continue;
		if (snapshot.revision <= queued.revision)
		{
			for (const auto &receipt : snapshot.quest_xp_receipts)
			{
				const auto found = std::find_if(
					queued.quest_xp_receipts.begin(),
					queued.quest_xp_receipts.end(),
					[&](const auto &pending)
					{
						return pending.offering_operation.bytes ==
							       receipt.offering_operation.bytes &&
						       pending.reward_index ==
							       receipt.reward_index &&
						       pending.amount == receipt.amount;
					});
				if (found == queued.quest_xp_receipts.end())
					return player_save_pipeline_result::capture_failed;
			}
			for (const auto &receipt : snapshot.spell_effect_receipts)
			{
				const auto found = std::find_if(
					queued.spell_effect_receipts.begin(),
					queued.spell_effect_receipts.end(),
					[&](const auto &pending)
					{
						return pending.operation_id.bytes ==
							       receipt.operation_id.bytes &&
						       pending.effect_id == receipt.effect_id;
					});
				if (found == queued.spell_effect_receipts.end())
					return player_save_pipeline_result::capture_failed;
			}
			for (const auto &receipt : snapshot.craft_receipts)
				if (std::none_of(
					    queued.craft_receipts.begin(),
					    queued.craft_receipts.end(),
					    [&](const auto &pending)
					    {
						    return pending.operation_id.bytes ==
								   receipt.operation_id.bytes &&
							   pending.discipline ==
								   receipt.discipline &&
							   pending.experience == receipt.experience;
					    }))
					return player_save_pipeline_result::capture_failed;
			return player_save_pipeline_result::coalesced;
		}
		if (!merge_quest_xp_receipts(&snapshot, queued) ||
		    !merge_spell_effect_receipts(&snapshot, queued) ||
		    !merge_craft_receipts(&snapshot, queued))
			return player_save_pipeline_result::capture_failed;
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
	if (pending_append.size() + durable_ready.size() + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
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
	replay_gate.begin_replay();
	if (!player_save_journal_init(journal_directory, PLAYER_SAVE_JOURNAL_MAX_BYTES))
		return false;
	if (!player_save_worker_init(selected_snapshot_apply(), nullptr))
	{
		player_save_journal_shutdown();
		return false;
	}
	if (!player_save_worker_set_journal_hooks(nullptr, player_save_journal_worker_ack, nullptr,
						  player_save_journal_worker_terminal))
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
		append_inflight_pid = 0;
		append_inflight_revision = 0;
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

/** Join the dispatcher and workers, then release retained pipeline state. */
void player_save_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		replay_gate.begin_replay();
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
	for (terminal_fence &fence : terminal_fences)
		clear_terminal_fence_locked(fence);
	target_save_login_fences.fill({});
	retained_bytes = 0;
	accepting = false;
	append_inflight = false;
	append_inflight_pid = 0;
	append_inflight_revision = 0;
	health.initialized = false;
	update_depth_locked();
}

/** Mark player components dirty and advance any outstanding terminal fence to the new revision. */
bool player_save_pipeline_mark(int pid, player_component_mask_t components)
{
	/* Equipment and inventory are one custody graph.  Saving either half alone can
	 * delete container descendants or make an exact custody comparison impossible. */
	if (components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		components |= PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!accepting)
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	terminal_fence *existing_fence = find_terminal_fence_locked(pid);
	if (existing_fence && existing_fence->death_pinned)
		return false;
	const bool fenced = existing_fence != nullptr;
	player_revision_t revision = 0;
	// Keep admission and the revision transition under the same pipeline lock
	// as target-fence acquisition. Otherwise a save could mark dirty between
	// the offline preflight and the recipient-only fence.
	if (!player_revision_mark(pid, components, &revision))
		return false;
	if (fenced)
	{
		if (terminal_fence *fence = find_terminal_fence_locked(pid))
		{
			fence->revision = revision;
			fence->journaled = false;
			fence->acknowledged = false;
		}
	}
	++health.marked;
	return true;
}

/** Capture and enqueue pending player components with the supplied save intent and room. */
static player_save_pipeline_result checkpoint_dirty_with_quest_xp(
	P_char ch, int save_intent, int room_vnum, const player_quest_xp_receipt_snapshot *receipts,
	size_t receipt_count,
	const player_spell_effect_receipt_snapshot *spell_effect_receipt = nullptr)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return player_save_pipeline_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_pipeline_result::unavailable;
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::unavailable;
	if (item_movement_transaction_player_creation_busy(ch))
		return player_save_pipeline_result::unavailable;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (find_target_save_login_fence_locked(GET_PID(ch)))
			return player_save_pipeline_result::unavailable;
		if (terminal_fence *fence = find_terminal_fence_locked(GET_PID(ch));
		    fence && fence->death_pinned)
			return player_save_pipeline_result::unavailable;
	}
	player_snapshot pending_effects;
	player_component_mask_t required_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(
		    GET_PID(ch), &pending_effects.quest_xp_receipts, &required_components))
		return player_save_pipeline_result::capture_failed;
	if (!spell_component_retirement_pending_save_receipts(
		    GET_PID(ch), &pending_effects.spell_effect_receipts))
		return player_save_pipeline_result::capture_failed;
	if (!craft_progression_pending_save_receipts(GET_PID(ch), &pending_effects.craft_receipts))
		return player_save_pipeline_result::capture_failed;
	if (!pending_effects.craft_receipts.empty())
		required_components |= CRAFT_PROGRESSION_COMPONENTS;
	if (spell_effect_receipt)
	{
		try
		{
			const auto found = std::find_if(
				pending_effects.spell_effect_receipts.begin(),
				pending_effects.spell_effect_receipts.end(),
				[&](const auto &existing) {
					return existing.operation_id.bytes ==
					       spell_effect_receipt->operation_id.bytes;
				});
			if (found != pending_effects.spell_effect_receipts.end())
			{
				if (found->effect_id != spell_effect_receipt->effect_id)
					return player_save_pipeline_result::invalid;
			}
			else
				pending_effects.spell_effect_receipts.push_back(
					*spell_effect_receipt);
		}
		catch (const std::bad_alloc &)
		{
			return player_save_pipeline_result::capture_failed;
		}
	}
	// Applied rewards/effects need their component boundary even when the first
	// checkpoint never queued or this caller only dirtied another component.
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(GET_PID(ch), &revision))
		return player_save_pipeline_result::unavailable;
	if (!pending_effects.spell_effect_receipts.empty())
		required_components |= PLAYER_COMPONENT_AFFECTS;
	const auto missing_components = required_components & ~revision.dirty_components;
	if (missing_components)
	{
		if (!player_save_pipeline_mark(GET_PID(ch), missing_components) ||
		    !player_revision_snapshot_copy(GET_PID(ch), &revision))
			return player_save_pipeline_result::capture_failed;
	}
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
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (terminal_fence *fence = find_terminal_fence_locked(GET_PID(ch));
		    fence && fence->death_pinned)
			return player_save_pipeline_result::unavailable;
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
	if (receipt_count)
	{
		if (!receipts || receipt_count > 64 || snapshot.death ||
		    snapshot.encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
		    snapshot.encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES - 4 ||
		    receipt_count >
			    (PLAYER_SNAPSHOT_MAX_BYTES - snapshot.encoded_size_bound - 4) / 24)
			return player_save_pipeline_result::invalid;
		for (size_t index = 0; index < receipt_count; ++index)
		{
			const auto &receipt = receipts[index];
			const bool empty_operation =
				std::all_of(receipt.offering_operation.bytes.begin(),
					    receipt.offering_operation.bytes.end(),
					    [](uint8_t byte) { return !byte; });
			if (empty_operation || receipt.reward_index >= 64 || !receipt.amount)
				return player_save_pipeline_result::invalid;
			for (size_t prior = 0; prior < index; ++prior)
				if (receipts[prior].offering_operation.bytes ==
					    receipt.offering_operation.bytes &&
				    receipts[prior].reward_index == receipt.reward_index)
					return player_save_pipeline_result::invalid;
		}
		try
		{
			snapshot.quest_xp_receipts.assign(receipts, receipts + receipt_count);
		}
		catch (const std::bad_alloc &)
		{
			return player_save_pipeline_result::capture_failed;
		}
		snapshot.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
		snapshot.encoded_size_bound += 4 + 24 * receipt_count;
		std::vector<uint8_t> encoded;
		if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
			return player_save_pipeline_result::capture_failed;
	}
	if (!pending_effects.quest_xp_receipts.empty() ||
	    !pending_effects.spell_effect_receipts.empty() ||
	    !pending_effects.craft_receipts.empty())
	{
		if (!merge_quest_xp_receipts(&snapshot, pending_effects) ||
		    !merge_spell_effect_receipts(&snapshot, pending_effects) ||
		    !merge_craft_receipts(&snapshot, pending_effects))
			return player_save_pipeline_result::capture_failed;
		std::vector<uint8_t> encoded;
		if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
			return player_save_pipeline_result::capture_failed;
	}
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=capture mono_us=%llu pid=%d revision=%llu components=%llu intent=%d room=%d",
		      (unsigned long long)persistence_observability_now_usec(), GET_PID(ch),
		      (unsigned long long)queued_revision, (unsigned long long)components,
		      save_intent, room_vnum);
	return enqueue_snapshot(std::move(snapshot));
}

player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum)
{
	return checkpoint_dirty_with_quest_xp(ch, save_intent, room_vnum, nullptr, 0);
}

/** Mark requested components and capture a checkpoint with the supplied intent and room. */
player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum)
{
	if (ch && IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::unavailable;
	if (ch && player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_pipeline_result::unavailable;
	if (!ch || IS_NPC(ch) || !player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::invalid;
	return player_save_pipeline_checkpoint_dirty(ch, save_intent, room_vnum);
}

player_save_pipeline_result
player_save_pipeline_request_quest_xp(P_char ch, player_component_mask_t components,
				      const player_quest_xp_receipt_snapshot *receipts,
				      size_t receipt_count, int room_vnum)
{
	if (!ch || IS_NPC(ch) || !receipts || !receipt_count || receipt_count > 64 ||
	    !(components & PLAYER_COMPONENT_STATUS) ||
	    (components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) ||
	    IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::invalid;
	if (!player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::unavailable;
	return checkpoint_dirty_with_quest_xp(ch, RENT_CRASH, room_vnum, receipts, receipt_count);
}

player_save_pipeline_result
player_save_pipeline_request_spell_effect(P_char ch, player_component_mask_t components,
					  const player_spell_effect_receipt_snapshot *receipt,
					  int room_vnum)
{
	if (!ch || IS_NPC(ch) || !receipt || !(components & PLAYER_COMPONENT_AFFECTS) ||
	    (components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) ||
	    IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::invalid;
	if (!player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::unavailable;
	return checkpoint_dirty_with_quest_xp(ch, RENT_CRASH, room_vnum, nullptr, 0, receipt);
}

namespace
{
/** Reserve this player's durability fence and mark the revision the caller will wait on. */
bool begin_terminal_fence(int pid, player_revision_t *revision, bool pin_death = false)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || find_target_save_login_fence_locked(pid))
		return false;
	if (terminal_fence *existing = find_terminal_fence_locked(pid);
	    existing && existing->death_pinned)
		return false;
	terminal_fence *fence = allocate_terminal_fence_locked(pid);
	if (!fence)
		return false;
	// Ordinary terminal intents deliberately take a fresh revision on every
	// call. Death retries use the separate pinned request path below.
	if (!player_revision_mark(pid, PLAYER_CHECKPOINT_COMPONENT_ALL, revision))
	{
		clear_terminal_fence_locked(*fence);
		return false;
	}
	if (pin_death && !player_revision_pin_terminal_death(pid, *revision))
	{
		clear_terminal_fence_locked(*fence);
		return false;
	}
	fence->pid = pid;
	fence->revision = *revision;
	fence->journaled = false;
	fence->acknowledged = false;
	fence->death_pinned = pin_death;
	return true;
}

bool retain_and_enqueue_death_snapshot(player_snapshot snapshot, uint64_t corpse_uid,
				       uint64_t wallet_pile_uid,
				       const critical_operation_id &operation_id)
{
	const size_t snapshot_bytes = snapshot.encoded_size_bound;
	player_snapshot pinned_copy;
	try
	{
		pinned_copy = snapshot;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	terminal_fence *fence = find_terminal_fence_locked(snapshot.pid);
	if (!health.initialized || stop_requested || !fence || !fence->death_pinned ||
	    fence->revision != snapshot.revision || fence->death_snapshot ||
	    !player_snapshot_is_death_request_schema(snapshot.schema_version) || !snapshot.death ||
	    snapshot.death->operation_id.bytes != operation_id.bytes ||
	    snapshot.death->wallet_pile_uid != wallet_pile_uid || snapshot.death->corpse.empty() ||
	    snapshot.death->corpse.front().object_uid != corpse_uid)
		return false;
	if (pending_append.size() + durable_ready.size() + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > (PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes) / 2)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.push_back(std::move(snapshot));
	}
	catch (const std::bad_alloc &)
	{
		++health.overloads;
		return false;
	}
	fence->corpse_uid = corpse_uid;
	fence->operation_id = operation_id;
	fence->wallet_pile_uid = pinned_copy.death->wallet_pile_uid;
	fence->death_snapshot.emplace(std::move(pinned_copy));
	retained_bytes += snapshot_bytes * 2;
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	return true;
}

bool requeue_pinned_death(int pid, uint64_t corpse_uid);
player_save_terminal_result await_terminal_fence(int pid, player_revision_t revision,
						 uint64_t timeout_msec, bool allow_journal_handoff);
} // namespace

/** Capture fresh terminal intent and wait for its durability fence within the caller timeout. */
player_save_terminal_result player_save_pipeline_terminal(P_char ch, int save_intent, int room_vnum,
							  uint64_t timeout_msec,
							  bool allow_journal_handoff)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_terminal_result::unavailable;
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_terminal_result::unavailable;
	const int pid = GET_PID(ch);
	player_revision_t revision = 0;
	if (!begin_terminal_fence(pid, &revision, false))
		return player_save_terminal_result::unavailable;
	const auto checkpoint = player_save_pipeline_checkpoint_dirty(ch, save_intent, room_vnum);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=terminal_begin mono_us=%llu pid=%d revision=%llu checkpoint=%u intent=%d timeout_ms=%llu journal_allowed=%d",
		      (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision, (unsigned)checkpoint, save_intent,
		      (unsigned long long)timeout_msec, allow_journal_handoff);
	return await_terminal_fence(pid, revision, timeout_msec, allow_journal_handoff);
}

/** Record one immutable death disposition; retries resume its exact pinned bytes. */
player_save_terminal_result
player_save_pipeline_terminal_death(P_char ch, P_obj corpse, P_obj wallet_pile,
				    const critical_operation_id &operation_id, int room_vnum,
				    uint64_t timeout_msec, bool allow_journal_handoff)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	const player_save_terminal_result resumed =
		player_save_pipeline_terminal_death_resume(ch, 0, timeout_msec);
	if (resumed != player_save_terminal_result::not_pending)
		return resumed;
	if (!corpse)
		return player_save_terminal_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_terminal_result::unavailable;
	// A payload-gap load keeps its valid item graph read-only. Its only safe
	// terminal write is the immutable death disposition, which records that
	// graph and quarantines matching durable custody. Every other degraded load
	// may be missing state the disposition cannot reconstruct.
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) &&
	    !IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP))
		return player_save_terminal_result::unavailable;
	const int pid = GET_PID(ch);
	player_revision_t revision = 0;
	if (!begin_terminal_fence(pid, &revision, true))
		return player_save_terminal_result::unavailable;
	struct unqueued_fence_guard
	{
		int pid;
		bool queued = false;
		~unqueued_fence_guard()
		{
			if (!queued)
			{
				std::lock_guard<std::mutex> lock(pipeline_mutex);
				if (terminal_fence *fence = find_terminal_fence_locked(pid))
					clear_terminal_fence_locked(*fence);
			}
		}
	} guard{ pid };
	player_snapshot snapshot;
	if (player_death_snapshot_capture(ch, corpse, wallet_pile, operation_id, revision,
					  room_vnum, {},
					  &snapshot) != player_snapshot_capture_result::ok ||
	    !player_snapshot_is_death_request_schema(snapshot.schema_version) || !snapshot.death)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		++health.capture_failures;
		return player_save_terminal_result::invalid;
	}
	player_snapshot pending_effects;
	player_component_mask_t required_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(pid, &pending_effects.quest_xp_receipts,
							 &required_components) ||
	    !merge_quest_xp_receipts(&snapshot, pending_effects) ||
	    !spell_component_retirement_pending_save_receipts(
		    pid, &pending_effects.spell_effect_receipts) ||
	    !merge_spell_effect_receipts(&snapshot, pending_effects) ||
	    !craft_progression_pending_save_receipts(pid, &pending_effects.craft_receipts) ||
	    !merge_craft_receipts(&snapshot, pending_effects))
		return player_save_terminal_result::unavailable;
	std::vector<uint8_t> encoded;
	if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
		return player_save_terminal_result::invalid;
	player_revision_t queued_revision = 0;
	player_component_mask_t components = 0;
	if (!player_revision_queue(pid, &queued_revision, &components) ||
	    queued_revision != revision || components != snapshot.components)
		return player_save_terminal_result::unavailable;
	const uint64_t wallet_pile_uid = wallet_pile ? wallet_pile->obj_uid : 0;
	if (!retain_and_enqueue_death_snapshot(std::move(snapshot), corpse->obj_uid,
					       wallet_pile_uid, operation_id))
		return player_save_terminal_result::unavailable;
	guard.queued = true;
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=terminal_death_begin mono_us=%llu pid=%d revision=%llu room=%d timeout_ms=%llu journal_allowed=0",
		      (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision, room_vnum, (unsigned long long)timeout_msec);
	(void)allow_journal_handoff;
	return await_terminal_fence(pid, revision, timeout_msec, false);
}

player_save_terminal_result
player_save_pipeline_terminal_death_resume(P_char ch, uint64_t corpse_uid, uint64_t timeout_msec)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	const int pid = GET_PID(ch);
	player_revision_t revision = 0;
	bool acknowledged = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		terminal_fence *fence = find_terminal_fence_locked(pid);
		if (!fence || !fence->death_pinned)
			return player_save_terminal_result::not_pending;
		if ((corpse_uid && corpse_uid != fence->corpse_uid) ||
		    !death_snapshot_identity_matches(*fence))
			return player_save_terminal_result::unavailable;
		revision = fence->revision;
		acknowledged = fence->acknowledged;
	}
	if (!acknowledged && !requeue_pinned_death(pid, corpse_uid))
		return player_save_terminal_result::unavailable;
	return await_terminal_fence(pid, revision, timeout_msec, false);
}

namespace
{
/** Requeue only the pinned immutable bytes after the worker has released a failed slot. */
bool requeue_pinned_death(int pid, uint64_t corpse_uid)
{
	player_snapshot retry_snapshot;
	player_revision_t revision = 0;
	size_t snapshot_bytes = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		terminal_fence *fence = find_terminal_fence_locked(pid);
		if (!fence || !fence->death_pinned ||
		    (corpse_uid && corpse_uid != fence->corpse_uid) ||
		    !death_snapshot_identity_matches(*fence))
			return false;
		if (fence->acknowledged || snapshot_is_retained_locked(pid, fence->revision) ||
		    (append_inflight_pid == pid && append_inflight_revision == fence->revision))
			return true;
		revision = fence->revision;
		snapshot_bytes = fence->death_snapshot->encoded_size_bound;
		try
		{
			retry_snapshot = *fence->death_snapshot;
		}
		catch (const std::bad_alloc &)
		{
			++health.overloads;
			return false;
		}
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot current = {};
	if (!player_revision_snapshot_copy(pid, &current) || current.current_revision != revision ||
	    current.queued_revision != revision ||
	    current.queued_components != retry_snapshot.components || current.inflight_components)
		return false;

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	terminal_fence *fence = find_terminal_fence_locked(pid);
	if (!fence || !fence->death_pinned || fence->revision != revision ||
	    !death_snapshot_identity_matches(*fence))
		return false;
	if (fence->acknowledged || snapshot_is_retained_locked(pid, revision) ||
	    (append_inflight_pid == pid && append_inflight_revision == revision))
		return true;
	if (pending_append.size() + durable_ready.size() + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.push_back(std::move(retry_snapshot));
	}
	catch (const std::bad_alloc &)
	{
		++health.overloads;
		return false;
	}
	retained_bytes += snapshot_bytes;
	++health.terminal_death_requeues;
	update_depth_locked();
	append_available.notify_one();
	return true;
}

bool acknowledge_terminal_fence_completion_locked(const player_save_completion &completion)
{
	terminal_fence *fence = find_terminal_fence_locked(completion.pid);
	if (!fence || fence->revision != completion.revision)
		return false;
	const bool applied = completion.outcome == player_save_apply_outcome::applied ||
			     completion.outcome == player_save_apply_outcome::already_applied;
	if (fence->death_pinned)
	{
		if (!applied || completion.durable_revision != fence->revision ||
		    !death_snapshot_identity_matches(*fence) ||
		    completion.components != fence->death_snapshot->components)
			return false;
		fence->acknowledged = true;
		return true;
	}
	if ((applied || completion.outcome == player_save_apply_outcome::stale_revision) &&
	    completion.durable_revision >= fence->revision)
		fence->acknowledged = true;
	return fence->acknowledged;
}

/** Pump the pipeline until this player revision is durable or the deadline passes. */
player_save_terminal_result await_terminal_fence(int pid, player_revision_t revision,
						 uint64_t timeout_msec, bool allow_journal_handoff)
{
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
				clear_terminal_fence_locked(*fence);
				return player_save_terminal_result::database_acknowledged;
			}
			if (!fence->death_pinned && allow_journal_handoff && fence->journaled)
			{
				++health.terminal_journal_handoffs;
				clear_terminal_fence_locked(*fence);
				return player_save_terminal_result::journal_durable;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (trace_player_saves())
	{
		const terminal_fence *fence = find_terminal_fence_locked(pid);
		player_revision_snapshot current = {};
		player_revision_snapshot_copy(pid, &current);
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=terminal_timeout mono_us=%llu pid=%d target=%llu fence=%llu journaled=%d acknowledged=%d current=%llu ack=%llu dirty=%llu queued=%llu inflight=%llu append=%llu ready=%llu append_failures=%llu",
		      (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision,
		      (unsigned long long)(fence ? fence->revision : 0), fence && fence->journaled,
		      fence && fence->acknowledged, (unsigned long long)current.current_revision,
		      (unsigned long long)current.acknowledged_revision,
		      (unsigned long long)current.dirty_components,
		      (unsigned long long)current.queued_revision,
		      (unsigned long long)current.inflight_revision,
		      (unsigned long long)health.pending_append,
		      (unsigned long long)health.durable_ready,
		      (unsigned long long)health.append_failures);
	}
	++health.terminal_timeouts;
	return player_save_terminal_result::timed_out;
}
} // namespace

/** Apply bounded worker completions and submit journaled snapshots from the game thread. */
void player_save_pipeline_pulse(void)
{
	player_save_completion completions[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	int32_t missing_baseline[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t missing_baseline_count = 0;
	player_save_completion custody_mismatches[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	bool custody_recapture_allowed[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t custody_mismatch_count = 0;
	const size_t completed =
		player_save_worker_pulse(completions, PLAYER_SAVE_PIPELINE_PULSE_BUDGET);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.completions += completed;
		for (size_t index = 0; index < completed; ++index)
		{
			if (completions[index].outcome == player_save_apply_outcome::applied ||
			    completions[index].outcome ==
				    player_save_apply_outcome::already_applied)
				custody_recapture_armed.erase(completions[index].pid);
			if (trace_player_saves())
			{
				const auto &completion = completions[index];
				logit(LOG_STATUS,
				      "PLAYER SAVE TRACE: stage=completion mono_us=%llu pid=%d revision=%llu components=%llu outcome=%u durable=%llu error=%u custody_diagnosis=%s retries=%u queued_us=%llu started_us=%llu completed_us=%llu",
				      (unsigned long long)persistence_observability_now_usec(),
				      completion.pid, (unsigned long long)completion.revision,
				      (unsigned long long)completion.components,
				      (unsigned)completion.outcome,
				      (unsigned long long)completion.durable_revision,
				      completion.error_code,
				      player_save_custody_diagnosis_name(
					      completion.custody_diagnosis),
				      completion.retry_count,
				      (unsigned long long)completion.queued_at_usec,
				      (unsigned long long)completion.started_at_usec,
				      (unsigned long long)completion.completed_at_usec);
			}
			(void)acknowledge_terminal_fence_completion_locked(completions[index]);
			// The worker only ever UPDATEs player_data. A missing row means the
			// character never got its baseline INSERT, and every further async
			// save would fail the same way; record it for the sync fallback.
			if (completions[index].outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    completions[index].error_code == ENOENT && completions[index].pid > 0)
				missing_baseline[missing_baseline_count++] = completions[index].pid;
			if (completions[index].outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    completions[index].error_code ==
				    PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
			    completions[index].pid > 0)
			{
				custody_mismatches[custody_mismatch_count] = completions[index];
				custody_recapture_allowed[custody_mismatch_count] =
					custody_recapture_armed.insert(completions[index].pid)
						.second;
				++custody_mismatch_count;
			}
		}
	}
	for (size_t index = 0; index < completed; ++index)
	{
		if (craft_progression_hooks.saved)
		{
			if (!completions[index].craft_receipts.empty())
				craft_progression_hooks.saved(
					completions[index].pid, true,
					completions[index].craft_receipts.data(),
					completions[index].craft_receipts.size());
			if (!completions[index].failed_craft_receipts.empty())
				craft_progression_hooks.saved(
					completions[index].pid, false,
					completions[index].failed_craft_receipts.data(),
					completions[index].failed_craft_receipts.size());
		}
		if (!completions[index].quest_xp_receipts.empty())
			quest_reward_recovery_save_acknowledged(
				completions[index].pid, completions[index].revision,
				completions[index].quest_xp_receipts.data(),
				completions[index].quest_xp_receipts.size());
		if (!completions[index].spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				completions[index].pid, true,
				completions[index].spell_effect_receipts.data(),
				completions[index].spell_effect_receipts.size());
		if (!completions[index].failed_spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				completions[index].pid, false,
				completions[index].failed_spell_effect_receipts.data(),
				completions[index].failed_spell_effect_receipts.size());
	}
	for (size_t index = 0; index < custody_mismatch_count; ++index)
	{
		bool recapture_scheduled = false;
		for (P_char ch = custody_recapture_allowed[index] ? character_list : NULL; ch;
		     ch = ch->next)
			if (IS_PC(ch) && GET_PID(ch) == custody_mismatches[index].pid &&
			    GET_STAT(ch) != STAT_DEAD &&
			    !IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
			{
				/* Custody may advance after a snapshot is sealed (for example,
				 * an item granted during login).  Preserve the rejection, then
				 * capture the current graph instead of retrying stale bytes or
				 * requiring the player to issue a manual save. */
				persistence_schedule_character_save(ch, RENT_CRASH, 2,
								    "custody-mismatch-recapture");
				recapture_scheduled = true;
				break;
			}
		persistence_alert(AVATAR, "player_save", "redacted", "none", "none",
				  "custody_payload_mismatch_rejected",
				  "pid=%d revision=%llu components=%llu destructive_write=0 "
				  "custody_diagnosis=%s recapture_scheduled=%d",
				  custody_mismatches[index].pid,
				  (unsigned long long)custody_mismatches[index].revision,
				  (unsigned long long)custody_mismatches[index].components,
				  player_save_custody_diagnosis_name(
					  custody_mismatches[index].custody_diagnosis),
				  recapture_scheduled ? 1 : 0);
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
		const int trace_pid = durable_ready.front().pid;
		const player_revision_t trace_revision = durable_ready.front().revision;
		const player_save_submit_result submitted =
			player_save_worker_submit_retained(&durable_ready.front());
		if (trace_player_saves())
			logit(LOG_STATUS,
			      "PLAYER SAVE TRACE: stage=submit mono_us=%llu pid=%d revision=%llu outcome=%u append=%llu ready=%llu",
			      (unsigned long long)persistence_observability_now_usec(), trace_pid,
			      (unsigned long long)trace_revision, (unsigned)submitted,
			      (unsigned long long)health.pending_append,
			      (unsigned long long)health.durable_ready);
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

bool player_save_pipeline_loads_allowed(void)
{
	return replay_gate.loads_allowed();
}

size_t player_save_pipeline_dirty_count(void)
{
	return player_revision_dirty_count();
}

/** Classify save intents that do not require a terminal durability fence. */
bool player_save_pipeline_is_nonterminal_type(int save_intent)
{
	return save_intent != RENT_INN && save_intent != RENT_LINKDEAD &&
	       save_intent != RENT_CAMPED && save_intent != RENT_DEATH &&
	       save_intent != RENT_POOFARTI && save_intent != RENT_SWAPARTI &&
	       save_intent != RENT_FIGHTARTI;
}

bool player_save_pipeline_target_save_pending(int pid)
{
	if (pid <= 0)
		return true;
	if (player_save_journal_pid_quarantined(pid))
		return true;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_terminal_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid))
			return true;
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(pid, &revision))
		return false;
	return revision.overflowed || revision.dirty_components ||
	       revision.unacknowledged_components || revision.queued_components ||
	       revision.inflight_components ||
	       revision.current_revision != revision.acknowledged_revision;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid,
							  player_revision_t expected_revision)
{
	if (pid <= 0 || expected_revision == std::numeric_limits<player_revision_t>::max())
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
		    !allocate_target_save_login_fence_locked(pid, expected_revision))
			return false;
	}

	// Reserve before checking the worker/revision state. New marks are rejected
	// while reserved, so the final check cannot race a newly admitted save.
	if (player_save_worker_pid_pending(pid))
	{
		player_save_pipeline_release_target_save_login_fence(pid, expected_revision);
		return false;
	}
	player_revision_snapshot revision = {};
	if (player_revision_snapshot_copy(pid, &revision) &&
	    (revision.overflowed || revision.dirty_components ||
	     revision.unacknowledged_components || revision.queued_components ||
	     revision.inflight_components || revision.current_revision != expected_revision ||
	     revision.acknowledged_revision != expected_revision))
	{
		player_save_pipeline_release_target_save_login_fence(pid, expected_revision);
		return false;
	}

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	target_save_login_fence *fence = find_target_save_login_fence_locked(pid);
	if (!fence || fence->expected_revision != expected_revision || append_inflight_pid == pid ||
	    any_snapshot_is_retained_locked(pid))
	{
		if (fence && fence->expected_revision == expected_revision)
			*fence = {};
		return false;
	}
	return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid,
							  player_revision_t expected_revision)
{
	if (pid <= 0)
		return;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (target_save_login_fence *fence = find_target_save_login_fence_locked(pid))
		if (fence->expected_revision == expected_revision)
			*fence = {};
}

bool player_save_pipeline_target_save_login_fenced(int pid)
{
	if (pid <= 0)
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return true;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return find_target_save_login_fence_locked(pid) != nullptr;
}

bool player_save_pipeline_save_admitted(int pid)
{
	if (pid <= 0)
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (find_target_save_login_fence_locked(pid))
		return false;
	if (terminal_fence *fence = find_terminal_fence_locked(pid); fence && fence->death_pinned)
		return false;
	return true;
}

/** Stop the pipeline and clear worker, revision, and health state for an isolated test. */
void player_save_pipeline_reset_for_tests(void)
{
	player_save_pipeline_shutdown();
	player_save_worker_reset_for_tests();
	player_revision_reset_for_tests();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
	replay_gate.begin_replay();
	stop_requested = false;
	accepting = false;
	append_inflight = false;
	append_inflight_pid = 0;
	append_inflight_revision = 0;
	target_save_login_fences.fill({});
}
