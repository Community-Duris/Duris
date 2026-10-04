#include "persistence/death_recovery_visibility.h"
#include "player/player_save_pipeline.h"
#include "net/network_wakeup.h"
#include "sql/sql_thread_init.h"
#include "persistence/persistence_observability.h"
#include <cstdlib>
#include <cstring>

#include "core/prototypes.h"
#include "core/files.h"
#include "classes/necromancy.h"
#include "flatfile/flatfile_player_repository.h"
#include "player/player_save_journal.h"
#include "player/player_save_worker.h"
#include "player/player_save_execution_guard.h"
#include "player/player_save_replay_ownership.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spell_item_lifecycle.h"
#include "item/item_movement_transaction.h"
#include "item/ordinary_drop_recovery.h"
#include "economy/coin_physical_recovery.h"
#include "economy/currency_transaction.h"
#include "persistence/critical_command_coordinator.h"
#include "economy/item_transfer_accounting.h"
#include "economy/collector_accounting.h"
#include "persistence/sql_room_item_payload.h"
#include "world/quest_reward_recovery.h"
#include "player/craft_progression_hooks.h"

#include <algorithm>
#include <cerrno>
#include <climits>
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
using player_save_execution_guard::resident_claim;
using player_save_execution_guard::execution_scope;
using player_save_execution_guard::ownership_status;

struct retained_snapshot
{
	resident_claim residence;
	player_snapshot body;
	retained_snapshot() noexcept = default;
	retained_snapshot(player_snapshot &&snapshot, resident_claim &&claim) noexcept
		: residence(std::move(claim))
		, body(std::move(snapshot))
	{
	}
	retained_snapshot(retained_snapshot &&) noexcept = default;
	retained_snapshot &operator=(retained_snapshot &&other) noexcept
	{
		if (this == &other)
			return *this;
		// Retire the old body before releasing its residence during deque moves.
		body = std::move(other.body);
		residence = std::move(other.residence);
		return *this;
	}
};
std::deque<retained_snapshot> pending_append;
std::deque<retained_snapshot> durable_ready;
// Allocation-free retention when a failed append cannot be requeued. Shutdown
// cannot destroy an unjournaled original merely because its thread has joined.
std::optional<retained_snapshot> append_retry;
std::thread dispatcher;
player_save_pipeline_health health = {};
player_save_pipeline_replay_gate replay_gate;
size_t retained_bytes = 0;
bool stop_requested = false;
bool accepting = false;
bool execution_started = false;
bool shutdown_incomplete = false;
// Enabled close must not turn late teardown saves into epoch-zero synchronous
// writes. Only an explicit cancellation before stopping may reopen admission.
bool lifecycle_admission_closed = false;
bool lifecycle_stop_attempted = false;
bool append_inflight = false;
std::atomic<bool> replay_revisit_requested{ false };
int append_inflight_pid = 0;
player_revision_t append_inflight_revision = 0;
/* One failed graph may be recaptured once.  Keep the PID armed until a later
 * database acknowledgement proves custody and payload agree; otherwise every
 * rejected recapture creates a new revision and an unbounded wizlog loop. */
std::set<int32_t> custody_recapture_armed;

struct literal_inventory_checkpoint
{
	player_literal_inventory_token token = {};
	std::vector<uint8_t> payload;
	player_revision_t captured_revision = 0;
	player_revision_t acknowledged_revision = 0;
	critical_operation_id operation_id = {};
	bool held = false;
	bool restored_sql_drop = false;
	uint64_t execution_hold_generation = 0;
};
std::array<literal_inventory_checkpoint, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>
	literal_inventory_checkpoints = {};
#ifndef __NO_MYSQL__
uint64_t literal_inventory_generation = 0;
#endif
constexpr player_component_mask_t LITERAL_INVENTORY_COMPONENTS = PLAYER_COMPONENT_EQUIPMENT |
								 PLAYER_COMPONENT_INVENTORY;

literal_inventory_checkpoint *find_literal_inventory_locked(int pid)
{
	for (auto &checkpoint : literal_inventory_checkpoints)
		if (checkpoint.token.pid == pid)
			return &checkpoint;
	return nullptr;
}

bool literal_inventory_capacity_locked(size_t incoming_bytes)
{
	if (incoming_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	for (const auto &checkpoint : literal_inventory_checkpoints)
	{
		if (checkpoint.payload.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.payload.size();
	}
	return true;
}

bool literal_inventory_blob(const player_snapshot &snapshot, uint64_t root_uid,
			    std::vector<uint8_t> *blob)
{
	if (!root_uid || !blob || snapshot.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    (snapshot.components & LITERAL_INVENTORY_COMPONENTS) != LITERAL_INVENTORY_COMPONENTS)
		return false;
	try
	{
		std::vector<int32_t> indices(snapshot.items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		std::vector<player_item_snapshot> selected;
		bool found = false;
		for (size_t index = 0; index < snapshot.items.size(); ++index)
		{
			const auto &item = snapshot.items[index];
			if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const bool root = item.object_uid == root_uid;
			if (root && (found || item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
				     item.equipment_slot != 0))
				return false;
			const bool child = item.parent_index >= 0 &&
					   indices[item.parent_index] >= 0;
			if (!root && !child)
				continue;
			if (!item.object_uid || item.equipment_slot ||
			    item.string_mask !=
				    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3))
				return false;
			found = found || root;
			indices[index] = static_cast<int32_t>(selected.size());
			selected.push_back(item);
			selected.back().parent_index = child ? indices[item.parent_index] :
							       PLAYER_SNAPSHOT_NO_PARENT;
		}
		return found &&
		       player_item_snapshot_list_encode(selected, blob) ==
			       player_snapshot_codec_result::ok &&
		       !blob->empty() && blob->size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

void note_literal_enqueue_locked(literal_inventory_checkpoint *checkpoint,
				 player_revision_t revision)
{
	if (checkpoint)
	{
		checkpoint->captured_revision = revision;
		checkpoint->acknowledged_revision = 0;
	}
}

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
	resident_claim death_residence;
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
	health.pending_append = pending_append.size() + (append_retry ? 1 : 0);
	health.durable_ready = durable_ready.size();
	health.retained_bytes = retained_bytes;
	health.accepting = accepting;
	health.append_inflight = append_inflight;
	health.high_water_snapshots = std::max(
		health.high_water_snapshots,
		static_cast<uint64_t>(health.pending_append + health.durable_ready +
				      pinned_death_count_locked() + (append_inflight ? 1 : 0)));
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
	std::vector<int> deferred_replay_pids;
	const auto remember_deferred = [&]()
	{
		deferred_replay_pids.clear();
		if (replay != player_save_journal_result::replay_deferred ||
		    !player_save_execution_guard::current_ownership_epoch())
			return;
		try
		{
			std::vector<player_save_journal_retained_frame> frames;
			const auto collected = player_save_journal_collect_retained(&frames);
			if (collected != player_save_journal_result::ok)
			{
				replay = collected;
				return;
			}
			deferred_replay_pids.reserve(frames.size());
			for (const auto &frame : frames)
				if (!frame.quarantined && !frame.policy_fenced)
					deferred_replay_pids.push_back(frame.snapshot.pid);
			std::sort(deferred_replay_pids.begin(), deferred_replay_pids.end());
			deferred_replay_pids.erase(std::unique(deferred_replay_pids.begin(),
							       deferred_replay_pids.end()),
						   deferred_replay_pids.end());
		}
		catch (...)
		{
			deferred_replay_pids.clear();
			replay = player_save_journal_result::replay_blocked;
		}
	};
	remember_deferred();
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.replay_complete = replay == player_save_journal_result::ok;
		health.replay_blocked = replay != player_save_journal_result::ok;
		replay_gate.finish_replay(replay == player_save_journal_result::ok &&
					  health.initialized && !stop_requested);
	}

	for (;;)
	{
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		// Observe before selecting any release/revisit work: a notification
		// arriving after this observation must also prevent the final wait.
		const auto observed = player_save_execution_guard::observe_ownership(epoch, 1);
		if (epoch)
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (stop_requested || !observed.available)
			{
				if (!observed.available)
				{
					accepting = false;
					health.replay_complete = false;
					health.replay_blocked = true;
					replay_gate.begin_replay();
					update_depth_locked();
				}
				break;
			}
		}
		if (epoch && replay == player_save_journal_result::replay_deferred)
			for (const int pid : deferred_replay_pids)
			{
				const auto state =
					player_save_execution_guard::observe_ownership(epoch, pid);
				if (state.available && !state.publication_held &&
				    !state.resident_count && !state.executing &&
				    !state.replay_pending && !state.replay_reserved)
				{
					replay_revisit_requested.store(true);
					break;
				}
			}
		if (epoch && mysql_ready && replay == player_save_journal_result::replay_deferred &&
		    replay_revisit_requested.exchange(false))
		{
			// Replay owns per-PID reservations and rereads after acquisition. This
			// is a retained release notice, never an unguarded second replay pass.
			replay = player_save_journal_replay(selected_snapshot_apply(), nullptr);
			remember_deferred();
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			health.replay_complete = replay == player_save_journal_result::ok;
			health.replay_blocked = replay != player_save_journal_result::ok;
			replay_gate.finish_replay(health.replay_complete && health.initialized &&
						  !stop_requested);
		}
		if (epoch && observed.available)
		{
			std::array<player_save_ownership_waiter, PLAYER_SAVE_WORKER_MAX_PIDS>
				waiters;
			const size_t count = player_save_worker_ownership_waiters(waiters.data(),
										  waiters.size());
			for (size_t i = 0; i < count; ++i)
			{
				const auto &waiter = waiters[i];
				const auto state = player_save_execution_guard::observe_ownership(
					epoch, waiter.identity.pid);
				if (waiter.epoch == epoch && state.available && !state.executing &&
				    !state.replay_reserved && !state.publication_held)
					(void)player_save_worker_resume_deferred_exact(
						waiter.identity);
			}
		}
		retained_snapshot retained;
		std::optional<execution_scope> append_scope;
		bool selected = false;
		bool requeue_failed = false;
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			if (epoch && (stop_requested || !observed.available))
			{
				if (!observed.available)
				{
					accepting = false;
					health.replay_blocked = true;
					update_depth_locked();
				}
				break;
			}
			if (append_retry)
			{
				bool retry_allowed = !epoch;
				if (epoch &&
				    append_retry->residence.matches_current(append_retry->body.pid))
				{
					append_scope.emplace(append_retry->residence);
					retry_allowed = append_scope->result() ==
							ownership_status::allowed;
					if (!retry_allowed)
						append_scope.reset();
				}
				if (retry_allowed)
				{
					// Retry native append directly even when deque storage is still
					// unavailable. This preserves the existing inactive OOM path.
					retained = std::move(*append_retry);
					append_retry.reset();
					selected = true;
				}
				else
					try
					{
						pending_append.push_front(std::move(*append_retry));
						append_retry.reset();
					}
					catch (const std::bad_alloc &)
					{
						++health.overloads;
						requeue_failed = true;
					}
			}
			if (!selected && !requeue_failed && !epoch)
			{
				append_available.wait(
					lock,
					[] { return stop_requested || !pending_append.empty(); });
				if (stop_requested && pending_append.empty())
					break;
			}
			if (!selected && !requeue_failed)
			{
				for (auto entry = pending_append.begin();
				     entry != pending_append.end(); ++entry)
				{
					if (epoch)
					{
						if (!entry->residence.matches_current(
							    entry->body.pid))
						{
							accepting = false;
							health.replay_blocked = true;
							break;
						}
						append_scope.emplace(entry->residence);
						const auto admitted = append_scope->result();
						if (admitted != ownership_status::allowed)
						{
							append_scope.reset();
							if (admitted == ownership_status::busy ||
							    admitted == ownership_status::held)
								continue;
							accepting = false;
							health.replay_blocked = true;
							break;
						}
					}
					retained = std::move(*entry);
					pending_append.erase(entry);
					selected = true;
					break;
				}
			}
			if (selected)
			{
				append_inflight = true;
				append_inflight_pid = retained.body.pid;
				append_inflight_revision = retained.body.revision;
			}
			update_depth_locked();
		}
		if (requeue_failed)
		{
			// Existing allocation/I/O backoff, never an ownership-contention retry.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		if (!selected)
		{
			if (epoch)
				player_save_execution_guard::wait_ownership_change(
					epoch, observed.change_sequence);
			continue;
		}
		auto &snapshot = retained.body;
		const player_save_journal_result appended = player_save_journal_append(snapshot);
		persistence_trace_event journal_trace;
		journal_trace.stage = persistence_trace_stage::save_journal;
		journal_trace.pid = snapshot.pid;
		journal_trace.revision = snapshot.revision;
		journal_trace.outcome = static_cast<uint32_t>(appended);
		persistence_trace_record(journal_trace);
		const bool quarantined = appended == player_save_journal_result::quarantined_pid;
		const bool quarantine_preserved =
			quarantined && player_save_journal_archive_quarantined(snapshot) ==
					       player_save_journal_result::ok;
		append_scope.reset();
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
					durable_ready.push_back(std::move(retained));
					network_wakeup_notify();
					if (terminal_fence *fence = find_terminal_fence_locked(
						    durable_ready.back().body.pid);
					    fence &&
					    fence->revision == durable_ready.back().body.revision)
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
						pending_append.push_back(std::move(retained));
					else
						pending_append.push_front(std::move(retained));
				}
				catch (const std::bad_alloc &)
				{
					// Retain complete bytes and claim in a stable owner across stop.
					append_retry.emplace(std::move(retained));
					++health.overloads;
				}
			}
			append_inflight = false;
			append_inflight_pid = 0;
			append_inflight_revision = 0;
			update_depth_locked();
		}
		if (appended != player_save_journal_result::ok && !quarantine_preserved)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
	if (append_retry && append_retry->body.pid == pid &&
	    append_retry->body.revision == revision)
		return true;
	for (const auto &retained : pending_append)
		if (retained.body.pid == pid && retained.body.revision == revision)
			return true;
	for (const auto &retained : durable_ready)
		if (retained.body.pid == pid && retained.body.revision == revision)
			return true;
	return false;
}

/** Check retained queues for any snapshot belonging to a player. */
bool any_snapshot_is_retained_locked(int pid)
{
	if (append_retry && append_retry->body.pid == pid)
		return true;
	for (const auto &retained : pending_append)
		if (retained.body.pid == pid)
			return true;
	for (const auto &retained : durable_ready)
		if (retained.body.pid == pid)
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
player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot, resident_claim residence)
{
	const player_revision_t literal_revision = snapshot.revision;
	if (player_save_journal_pid_quarantined(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || stop_requested ||
	    find_target_save_login_fence_locked(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	literal_inventory_checkpoint *literal = find_literal_inventory_locked(snapshot.pid);
	if (literal)
	{
		std::vector<uint8_t> blob;
		if (literal->held ||
		    !literal_inventory_blob(snapshot, literal->token.root_uid, &blob) ||
		    blob != literal->payload)
			return player_save_pipeline_result::capture_failed;
	}
	if (terminal_fence *fence = find_terminal_fence_locked(snapshot.pid);
	    fence && fence->death_pinned)
		return player_save_pipeline_result::unavailable;
	for (auto &retained : pending_append)
	{
		auto &queued = retained.body;
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
		retained = retained_snapshot(std::move(snapshot), std::move(residence));
		note_literal_enqueue_locked(literal, literal_revision);
		++health.coalesced;
		update_depth_locked();
		player_save_execution_guard::signal_ownership_change(
			player_save_execution_guard::current_ownership_epoch());
		return player_save_pipeline_result::coalesced;
	}
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    snapshot.encoded_size_bound > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	retained_bytes += snapshot.encoded_size_bound;
	try
	{
		pending_append.emplace_back(std::move(snapshot), std::move(residence));
	}
	catch (const std::bad_alloc &)
	{
		retained_bytes -= snapshot.encoded_size_bound;
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	note_literal_enqueue_locked(literal, literal_revision);
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return player_save_pipeline_result::queued;
}
} // namespace

bool player_save_pipeline_prepare(const char *journal_directory, void (*verify_resolved_recovery)())
{
	if (!journal_directory || journal_directory[0] != '/')
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (health.initialized || shutdown_incomplete || lifecycle_admission_closed)
			return false;
	}
	if (!player_save_execution_guard::begin_registration())
		return false;
	replay_gate.begin_replay();
	if (!player_save_journal_init(journal_directory, PLAYER_SAVE_JOURNAL_MAX_BYTES))
	{
		player_save_execution_guard::end_registration();
		return false;
	}
	try
	{
		if (verify_resolved_recovery)
			verify_resolved_recovery();
	}
	catch (...)
	{
		player_save_journal_shutdown();
		player_save_execution_guard::end_registration();
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health = {};
		health.initialized = true;
		stop_requested = false;
		accepting = false;
		execution_started = false;
		append_inflight = false;
		append_inflight_pid = 0;
		append_inflight_revision = 0;
		replay_revisit_requested.store(false);
	}
	return true;
}

bool player_save_pipeline_start(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || execution_started ||
		    dispatcher.joinable())
			return false;
	}
	try
	{
		if (!player_save_worker_init(selected_snapshot_apply(), nullptr))
			return false;
		if (!player_save_worker_set_journal_hooks(nullptr, player_save_journal_worker_ack,
							  nullptr,
							  player_save_journal_worker_terminal))
		{
			player_save_worker_shutdown();
			return false;
		}
		dispatcher = std::thread(dispatcher_main);
	}
	catch (...)
	{
		player_save_worker_shutdown();
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		execution_started = true;
		player_save_execution_guard::end_registration();
		accepting = true;
		update_depth_locked();
	}
	return true;
}

bool player_save_pipeline_init(const char *journal_directory, void (*verify_resolved_recovery)())
{
	if (!player_save_pipeline_prepare(journal_directory, verify_resolved_recovery))
		return false;
	if (player_save_pipeline_start())
		return true;
	player_save_pipeline_shutdown();
	return false;
}

/** Join the dispatcher and workers, then release retained pipeline state. */
void player_save_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		replay_gate.begin_replay();
		stop_requested = true;
		accepting = false;
		append_available.notify_all();
		player_save_execution_guard::signal_ownership_change(
			player_save_execution_guard::current_ownership_epoch());
	}
	if (dispatcher.joinable())
		dispatcher.join();
	player_save_worker_shutdown();
	if (player_save_execution_guard::current_ownership_epoch())
	{
		// Joined workers can still own original slots/results, and the pipeline
		// can own unjournaled bytes and death pins. Until the lifecycle owner
		// proves their durable handoff, preserve journal namespace and all holds.
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		shutdown_incomplete = true;
		execution_started = false;
		update_depth_locked();
		return;
	}
	player_save_journal_shutdown();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	pending_append.clear();
	durable_ready.clear();
	append_retry.reset();
	for (terminal_fence &fence : terminal_fences)
		clear_terminal_fence_locked(fence);
	target_save_login_fences.fill({});
	shutdown_incomplete = !player_save_execution_guard::discard_quiesced_holds();
	if (!shutdown_incomplete)
		literal_inventory_checkpoints.fill({});
	// A later drain alone must not permit preparation against another journal.
	// Only a successful explicit shutdown retry clears this resident state.
	retained_bytes = 0;
	accepting = false;
	execution_started = false;
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
	const player_spell_effect_receipt_snapshot *spell_effect_receipt = nullptr,
	resident_claim *original_residence = nullptr)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return player_save_pipeline_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_pipeline_result::unavailable;
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::unavailable;
	// Pending grants must publish before capturing their recipient inventory.
	if (item_movement_transaction_player_creation_busy(ch) ||
	    item_creation_grant_player_publication_pending(ch))
		return player_save_pipeline_result::unavailable;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (find_target_save_login_fence_locked(GET_PID(ch)))
			return player_save_pipeline_result::unavailable;
		if (terminal_fence *fence = find_terminal_fence_locked(GET_PID(ch));
		    fence && fence->death_pinned)
			return player_save_pipeline_result::unavailable;
	}
	uint64_t literal_root_uid = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (auto *literal = find_literal_inventory_locked(GET_PID(ch)))
		{
			if (literal->held || literal->token.actor_runtime_id != ch->runtime_id)
				return player_save_pipeline_result::unavailable;
			literal_root_uid = literal->token.root_uid;
		}
	}
	const uint64_t ownership_epoch = player_save_execution_guard::current_ownership_epoch();
	resident_claim residence;
	if (ownership_epoch)
	{
		residence = original_residence ? std::move(*original_residence) :
						 resident_claim(ownership_epoch, GET_PID(ch));
		if (!residence.matches_current(GET_PID(ch)))
			return player_save_pipeline_result::unavailable;
	}
	player_snapshot pending_effects;
	player_component_mask_t required_components =
		literal_root_uid ? LITERAL_INVENTORY_COMPONENTS : 0;
	player_component_mask_t quest_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(
		    GET_PID(ch), &pending_effects.quest_xp_receipts, &quest_components))
		return player_save_pipeline_result::capture_failed;
	required_components |= quest_components;
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
	const auto captured = literal_root_uid ?
				      player_snapshot_capture_literal_inventory(
					      ch, queued_revision, components, save_intent,
					      room_vnum, literal_root_uid, &snapshot) :
				      player_snapshot_capture(ch, queued_revision, components,
							      save_intent, room_vnum, &snapshot);
	if (captured != player_snapshot_capture_result::ok)
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
	persistence_trace_event capture_trace;
	capture_trace.stage = persistence_trace_stage::save_capture;
	capture_trace.pid = snapshot.pid;
	capture_trace.revision = snapshot.revision;
	capture_trace.components = snapshot.components;
	persistence_trace_record(capture_trace);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=capture mono_us=%llu pid=%d revision=%llu components=%llu intent=%d room=%d",
		      (unsigned long long)persistence_observability_now_usec(), GET_PID(ch),
		      (unsigned long long)queued_revision, (unsigned long long)components,
		      save_intent, room_vnum);
	return enqueue_snapshot(std::move(snapshot), std::move(residence));
}

namespace
{
bool literal_actor_matches(const player_literal_inventory_token &token, P_char actor)
{
	return actor && IS_PC(actor) && actor->only.pc && GET_PID(actor) == token.pid &&
	       actor->runtime_id && actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor && token.root_uid &&
	       token.generation && !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
}
}

player_literal_inventory_state
player_save_pipeline_literal_inventory_begin(P_char actor, P_obj root, int room_vnum,
					     player_literal_inventory_token *token_out)
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)token_out;
	return player_literal_inventory_state::refused;
#else
	if (!actor || !root || !token_out || !IS_PC(actor) || !actor->only.pc ||
	    GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor || !root->obj_uid ||
	    !OBJ_CARRIED_BY(root, actor) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, LITERAL_INVENTORY_COMPONENTS, RENT_CRASH, room_vnum, root->obj_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !literal_inventory_blob(captured, root->obj_uid, &blob))
		return player_literal_inventory_state::refused;
	player_literal_inventory_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (!literal_actor_matches(existing->token, actor) ||
			    existing->token.root_uid != root->obj_uid ||
			    existing->payload != blob || existing->held)
				return player_literal_inventory_state::refused;
			*token_out = existing->token;
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root->obj_uid,
			  ++literal_inventory_generation };
		slot->token = token;
		slot->payload = std::move(blob);
	}
	const auto queued = player_save_pipeline_request(actor, LITERAL_INVENTORY_COMPONENTS,
							 RENT_CRASH, room_vnum);
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state
player_save_pipeline_literal_inventory_poll(const player_literal_inventory_token &token,
					    P_char actor)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	return player_literal_inventory_state::refused;
#else
	if (!literal_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, LITERAL_INVENTORY_COMPONENTS, RENT_CRASH, NOWHERE, token.root_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !literal_inventory_blob(captured, token.root_uid, &blob))
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->token != token || literal->held)
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_literal_inventory_hold(const player_literal_inventory_token &token,
						 const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_literal_inventory_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!health.initialized || stop_requested || !accepting || !health.replay_complete ||
	    health.replay_blocked || find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal || literal->token != token ||
	    literal->held || !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_literal_inventory_release(const player_literal_inventory_token &token,
						    const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->token != token || !literal->held || literal->restored_sql_drop ||
	    literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_literal_inventory_cancel(const player_literal_inventory_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.root_uid || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->token != token || literal->held)
		return false;
	*literal = {};
	return true;
}

static bool restore_sql_publication_obligation(const critical_command &command, int pid,
					       uint64_t root_uid)
{
	std::vector<uint8_t> frozen;
	if (pid <= 0 || !root_uid ||
	    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	// The typed caller classified the original immutable room operation.
	// Registration remains closed before any execution owner starts.
	if (!health.initialized || stop_requested || execution_started)
		return false;
	literal_inventory_checkpoint *slot = nullptr;
	for (auto &candidate : literal_inventory_checkpoints)
	{
		if (candidate.token.pid == pid ||
		    (candidate.held && candidate.operation_id.bytes == command.operation_id.bytes))
		{
			uint64_t generation = 0;
			if (!candidate.restored_sql_drop || !candidate.held ||
			    candidate.token.pid != pid || candidate.token.root_uid != root_uid ||
			    candidate.operation_id.bytes != command.operation_id.bytes ||
			    candidate.payload != frozen ||
			    !player_save_execution_guard::install_hold(pid, command.operation_id,
								       &generation))
				return false;
			if (generation != candidate.execution_hold_generation)
			{
				player_save_execution_guard::poison_integrity();
				return false;
			}
			return true;
		}
		if (!candidate.token.pid && !slot)
			slot = &candidate;
	}
	if (!slot || !literal_inventory_capacity_locked(frozen.size()))
		return false;
	uint64_t generation = 0;
	if (!player_save_execution_guard::install_hold(pid, command.operation_id, &generation))
		return false;
	// All fallible command allocation/validation precedes guard installation.
	slot->execution_hold_generation = generation;
	slot->token.pid = pid;
	slot->token.root_uid = root_uid;
	slot->payload = std::move(frozen);
	slot->operation_id = command.operation_id;
	slot->held = true;
	slot->restored_sql_drop = true;
	return true;
}

bool player_save_pipeline_restore_sql_drop_obligation(const critical_command &command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	try
	{
		if (!command.publication_required || !critical_command_envelope_valid(command) ||
		    !item_transfer_accounting_command_supported(command))
			return false;
		item_transfer_payload payload = {};
		sql_room_item_payload_batch captured;
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    !sql_room_item_payload_capture(payload, &captured))
			return false;
		return restore_sql_publication_obligation(command,
							  static_cast<int>(payload.from_owner.id),
							  payload.selected_item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#endif
}

bool player_save_pipeline_restore_sql_coin_obligation(const critical_command &command)
{
	try
	{
		int pid = 0;
		uint64_t uid = 0;
		if (!coin_physical_recovery_identity(command, &pid, &uid))
			return false;
		return restore_sql_publication_obligation(command, pid, uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
namespace
{
bool collector_purchase_identity(const critical_command &command, int *pid, uint64_t *uid)
{
	economic_frozen_intent intent;
	collector_command_payload payload;
	collector::record original;
	economic_account_key wallet, bank;
	if (!command.publication_required || !critical_command_envelope_valid(command) ||
	    collector_purchase_accounting_decode(command, &intent, &payload, &original, &wallet,
						 &bank) != economic_accounting_error::ok ||
	    !payload.actor_pid || payload.actor_pid > INT_MAX ||
	    payload.action != collector_action::purchase || payload.item_count != 1 ||
	    !payload.selected_item_uid)
		return false;
	*pid = static_cast<int>(payload.actor_pid);
	*uid = payload.selected_item_uid;
	return true;
}
}
#endif

bool player_save_pipeline_restore_sql_collector_purchase_obligation(const critical_command &command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	try
	{
		int pid = 0;
		uint64_t uid = 0;
		return collector_purchase_identity(command, &pid, &uid) &&
		       restore_sql_publication_obligation(command, pid, uid);
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result collector_purchase_submit_owned(critical_command command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	int pid = 0;
	uint64_t uid = 0, generation = 0;
	std::vector<uint8_t> frozen;
	bool new_hold = false;
	try
	{
		if (!collector_purchase_identity(command, &pid, &uid) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(pid))
			return critical_submit_result::invalid;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid))
			return critical_submit_result::unavailable;
		auto *slot = find_literal_inventory_locked(pid);
		if (slot)
		{
			if (!slot->held || !slot->restored_sql_drop || slot->payload != frozen ||
			    slot->token.root_uid != uid ||
			    slot->operation_id.bytes != command.operation_id.bytes)
				return critical_submit_result::identity_conflict;
			generation = slot->execution_hold_generation;
		}
		else
		{
			player_revision_snapshot revision = {};
			if (player_revision_snapshot_copy(pid, &revision) &&
			    (revision.overflowed || revision.dirty_components ||
			     revision.unacknowledged_components || revision.queued_components ||
			     revision.inflight_components ||
			     revision.current_revision != revision.acknowledged_revision))
				return critical_submit_result::unavailable;
			for (auto &candidate : literal_inventory_checkpoints)
				if (!candidate.token.pid)
				{
					slot = &candidate;
					break;
				}
			if (!slot || !literal_inventory_capacity_locked(frozen.size()) ||
			    !player_save_execution_guard::install_live_publication_hold(
				    pid, command.operation_id, &generation))
				return critical_submit_result::unavailable;
			slot->token.pid = pid;
			slot->token.root_uid = uid;
			slot->execution_hold_generation = generation;
			slot->operation_id = command.operation_id;
			slot->payload = std::move(frozen);
			slot->held = slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	// No fallible command allocation follows hold installation: ownership passes
	// by move. A thrown coordinator path may have admitted it, so remains held.
	critical_submit_result submitted;
	const critical_operation_id operation = command.operation_id;
	try
	{
		submitted = critical_command_coordinator_submit_for_publication(std::move(command));
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
	if (!critical_submit_result_keeps_operation(submitted) && new_hold)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!slot || !slot->held || !slot->restored_sql_drop ||
		    slot->operation_id.bytes != operation.bytes ||
		    slot->execution_hold_generation != generation)
		{
			player_save_execution_guard::poison_integrity();
			return critical_submit_result::journal_uncertain;
		}
		// Exact synchronous refusal occurred before coordinator journal admission.
		if (!player_save_execution_guard::release_hold(pid, operation, generation))
			return critical_submit_result::journal_uncertain;
		*slot = {};
	}
	return critical_submit_result_keeps_operation(submitted) || new_hold ?
		       submitted :
		       critical_submit_result::journal_uncertain;
#endif
}

bool player_save_restored_publication_owner::publish_collector(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		int pid = 0;
		uint64_t uid = 0, generation = 0;
		critical_command command;
		std::vector<uint8_t> frozen;
		if (!collector_purchase_identity(original, &pid, &uid) ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			if (!health.initialized || stop_requested || !slot || !slot->held ||
			    !slot->restored_sql_drop || slot->token.root_uid != uid ||
			    slot->payload != frozen ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(frozen.data(), frozen.size(), &command) !=
				    critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		if (completion.disposition == critical_completion_disposition::never_admitted)
			return critical_command_coordinator_cancel_collector_publication(owner);
		player_revision_snapshot revision = {};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.dirty_components ||
		     revision.unacknowledged_components || revision.queued_components ||
		     revision.inflight_components ||
		     revision.current_revision != revision.acknowledged_revision))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

void player_save_pipeline_sql_drop_publication_acknowledged(
	const critical_operation_id &operation_id) noexcept
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	for (auto &checkpoint : literal_inventory_checkpoints)
		if (checkpoint.token.pid && checkpoint.held && !checkpoint.restored_sql_drop &&
		    checkpoint.operation_id.bytes == operation_id.bytes)
			checkpoint = {};
	// Restored holds are consumed only by the private guarded-ACK owner. An
	// ID-only assertion cannot release them, including before epoch enable.
}

bool player_save_restored_publication_owner::publish(const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		critical_command command = {};
		std::vector<uint8_t> frozen;
		int pid = 0;
		uint64_t generation = 0;
		std::unique_lock<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested)
			return false;
		for (const auto &checkpoint : literal_inventory_checkpoints)
			if (checkpoint.restored_sql_drop && checkpoint.held &&
			    checkpoint.operation_id.bytes == completion.operation_id.bytes)
			{
				pid = checkpoint.token.pid;
				generation = checkpoint.execution_hold_generation;
				frozen = checkpoint.payload;
				break;
			}
		if (pid <= 0 || find_terminal_fence_locked(pid) ||
		    find_target_save_login_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    critical_command_decode(frozen.data(), frozen.size(), &command) !=
			    critical_command_codec_result::ok)
			return false;
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		lock.unlock();
		const bool coin = owner.command_.type == critical_command_type::coin_transfer;
#ifdef __NO_MYSQL__
		// Only the typed ordinary-room coin scope has a flat native publisher.
		if (!coin)
			return false;
#endif
		if (coin &&
		    !currency_transaction_restored_coin_receipt_current(owner.command_, completion))
			return false;
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		player_revision_snapshot revision = {};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.dirty_components ||
		     revision.unacknowledged_components || revision.queued_components ||
		     revision.inflight_components ||
		     revision.current_revision != revision.acknowledged_revision))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
		    player_save_journal_result::ok)
			return false;
		bool publication_complete = false;
		if (coin)
			publication_complete =
				coin_physical_recovery_publish(owner.command_, completion);
		else
		{
			const auto published =
				ordinary_drop_recovery_publish(owner.command_, completion);
			publication_complete =
				published.status ==
					ordinary_drop_observation_status::verified_existing ||
				published.status == ordinary_drop_observation_status::published ||
				(published.status ==
					 ordinary_drop_observation_status::verified_rejected &&
				 completion.disposition ==
					 critical_completion_disposition::execution &&
				 completion.outcome == critical_apply_outcome::terminal_failure &&
				 completion.error_code &&
				 completion.failure_stage == critical_failure_stage::none);
		}
		if (!publication_complete ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		if (coin &&
		    !currency_transaction_restored_coin_receipt_current(owner.command_, completion))
			return false;
		owner.publication_proven_ = true;
		// The original hold/reservation survives fresh proof, confirmed native
		// cleanup and critical checkpoint. Failure retries the whole observation.
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
}

bool coin_physical_publication_restore_and_acknowledge(const critical_command &command,
						       const critical_completion &completion)
{
	if (!currency_transaction_restored_coin_receipt_current(command, completion))
		return false;
	// The slot's original immutable encoding must match the domain handoff;
	// an ID-only call cannot select or consume another publication obligation.
	std::vector<uint8_t> frozen;
	if (critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		bool found = false;
		for (const auto &slot : literal_inventory_checkpoints)
			if (slot.restored_sql_drop && slot.held &&
			    slot.operation_id.bytes == command.operation_id.bytes)
			{
				if (slot.payload != frozen)
					return false;
				found = true;
				break;
			}
		if (!found)
			return false;
	}
	return player_save_restored_publication_owner::publish(completion);
}

bool player_save_restored_publication_owner::consume_acknowledged_hold() noexcept
{
	if (!acknowledged_)
		return false;
	player_save_deferred_identity wake = {};
	bool active = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *checkpoint = find_literal_inventory_locked(pid_);
		if (!checkpoint || !checkpoint->restored_sql_drop || !checkpoint->held ||
		    checkpoint->execution_hold_generation != generation_ ||
		    checkpoint->operation_id.bytes != completion_.operation_id.bytes ||
		    checkpoint->payload != frozen_)
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		active = player_save_worker_deferred_identity(pid_, &wake);
		if (!reservation_.consume_acknowledged_hold())
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		*checkpoint = {};
	}
	if (active)
		(void)player_save_worker_resume_deferred_exact(wake);
	replay_revisit_requested.store(true);
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
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
	if (auto *literal = find_literal_inventory_locked(pid))
	{
		if (literal->held)
			return false;
		// No operation was admitted. A terminal intent supersedes this capture
		// request, while its already queued immutable saves retain normal order.
		*literal = {};
	}
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
				       const critical_operation_id &operation_id,
				       resident_claim residence)
{
	const size_t snapshot_bytes = snapshot.encoded_size_bound;
	resident_claim pinned_residence;
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (epoch)
	{
		if (!residence.matches_current(snapshot.pid))
			return false;
		pinned_residence = resident_claim::derive_existing(residence);
		if (!pinned_residence.matches_current(snapshot.pid))
			return false;
	}
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
	if (auto *literal = find_literal_inventory_locked(snapshot.pid); literal && literal->held)
		return false;
	if (!health.initialized || stop_requested || !fence || !fence->death_pinned ||
	    fence->revision != snapshot.revision || fence->death_snapshot ||
	    !player_snapshot_is_death_request_schema(snapshot.schema_version) || !snapshot.death ||
	    snapshot.death->operation_id.bytes != operation_id.bytes ||
	    snapshot.death->wallet_pile_uid != wallet_pile_uid || snapshot.death->corpse.empty() ||
	    snapshot.death->corpse.front().object_uid != corpse_uid)
		return false;
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > (PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes) / 2)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.emplace_back(std::move(snapshot), std::move(residence));
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
	fence->death_residence = std::move(pinned_residence);
	retained_bytes += snapshot_bytes * 2;
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
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
	resident_claim residence;
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (epoch)
	{
		residence = resident_claim(epoch, pid);
		if (!residence.matches_current(pid))
			return player_save_terminal_result::unavailable;
	}
	player_revision_t revision = 0;
	if (!begin_terminal_fence(pid, &revision, false))
		return player_save_terminal_result::unavailable;
	const auto checkpoint = checkpoint_dirty_with_quest_xp(ch, save_intent, room_vnum, nullptr,
							       0, nullptr, &residence);
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
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	resident_claim residence;
	if (epoch)
	{
		residence = resident_claim(epoch, pid);
		if (!residence.matches_current(pid))
			return player_save_terminal_result::unavailable;
	}
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
					       wallet_pile_uid, operation_id, std::move(residence)))
		return player_save_terminal_result::unavailable;
	guard.queued = true;
	char correlation[33] = {};
	death_recovery_correlation((static_cast<uint64_t>(pid) << 32) |
					   static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]),
				   correlation);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: correlation=%s stage=terminal_death_begin mono_us=%llu pid=%d revision=%llu room=%d timeout_ms=%llu journal_allowed=0",
		      correlation, (unsigned long long)persistence_observability_now_usec(), pid,
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
	resident_claim residence;
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
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		if (epoch)
		{
			residence = resident_claim::derive_existing(fence->death_residence);
			if (!residence.matches_current(pid))
				return false;
		}
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
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.emplace_back(std::move(retry_snapshot), std::move(residence));
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
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
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
	persistence_trace_event timeout_trace;
	timeout_trace.stage = persistence_trace_stage::save_timeout;
	timeout_trace.pid = pid;
	timeout_trace.revision = revision;
	timeout_trace.incident = true;
	persistence_trace_record(timeout_trace);
	++health.terminal_timeouts;
	return player_save_terminal_result::timed_out;
}
} // namespace

/** Apply bounded worker completions and submit journaled snapshots from the game thread. */
void player_save_pipeline_pulse(void)
{
	player_save_owned_completion owned_completions[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	int32_t missing_baseline[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t missing_baseline_count = 0;
	struct custody_diagnostic
	{
		int32_t pid = 0;
		player_revision_t revision = 0;
		player_component_mask_t components = 0;
		player_save_custody_diagnosis custody_diagnosis =
			player_save_custody_diagnosis::none;
	};
	custody_diagnostic custody_mismatches[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	bool custody_recapture_allowed[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t custody_mismatch_count = 0;
	const size_t completed = player_save_worker_pulse_owned(owned_completions,
								PLAYER_SAVE_PIPELINE_PULSE_BUDGET);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		for (auto &literal : literal_inventory_checkpoints)
			if (literal.token.pid && !literal.held)
			{
				P_char actor = find_character_by_runtime_id(
					literal.token.actor_runtime_id);
				if (!literal_actor_matches(literal.token, actor))
					literal = {};
			}
		health.completions += completed;
		for (size_t index = 0; index < completed; ++index)
		{
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::applied ||
			    owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::already_applied)
				custody_recapture_armed.erase(
					owned_completions[index].completion.pid);
			if (trace_player_saves())
			{
				const auto &completion = owned_completions[index].completion;
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
			(void)acknowledge_terminal_fence_completion_locked(
				owned_completions[index].completion);
			const auto &completion = owned_completions[index].completion;
			if (auto *literal = find_literal_inventory_locked(completion.pid);
			    literal && !literal->held &&
			    literal->captured_revision == completion.revision &&
			    completion.durable_revision == completion.revision &&
			    !completion.error_code &&
			    (completion.components & LITERAL_INVENTORY_COMPONENTS) ==
				    LITERAL_INVENTORY_COMPONENTS &&
			    (completion.outcome == player_save_apply_outcome::applied ||
			     completion.outcome == player_save_apply_outcome::already_applied))
				literal->acknowledged_revision = completion.revision;
			// The worker only ever UPDATEs player_data. A missing row means the
			// character never got its baseline INSERT, and every further async
			// save would fail the same way; record it for the sync fallback.
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    owned_completions[index].completion.error_code == ENOENT &&
			    owned_completions[index].completion.pid > 0)
				missing_baseline[missing_baseline_count++] =
					owned_completions[index].completion.pid;
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    owned_completions[index].completion.error_code ==
				    PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
			    owned_completions[index].completion.pid > 0)
			{
				custody_mismatches[custody_mismatch_count] = {
					completion.pid, completion.revision, completion.components,
					completion.custody_diagnosis
				};
				try
				{
					custody_recapture_allowed[custody_mismatch_count] =
						custody_recapture_armed.insert(completion.pid)
							.second;
				}
				catch (const std::bad_alloc &)
				{
					++health.overloads;
				}
				++custody_mismatch_count;
			}
		}
	}
	for (size_t index = 0; index < completed; ++index)
	{
		if (craft_progression_hooks.saved)
		{
			if (!owned_completions[index].completion.craft_receipts.empty())
				craft_progression_hooks.saved(
					owned_completions[index].completion.pid, true,
					owned_completions[index].completion.craft_receipts.data(),
					owned_completions[index].completion.craft_receipts.size());
			if (!owned_completions[index].completion.failed_craft_receipts.empty())
				craft_progression_hooks.saved(
					owned_completions[index].completion.pid, false,
					owned_completions[index]
						.completion.failed_craft_receipts.data(),
					owned_completions[index]
						.completion.failed_craft_receipts.size());
		}
		if (!owned_completions[index].completion.quest_xp_receipts.empty())
			quest_reward_recovery_save_acknowledged(
				owned_completions[index].completion.pid,
				owned_completions[index].completion.revision,
				owned_completions[index].completion.quest_xp_receipts.data(),
				owned_completions[index].completion.quest_xp_receipts.size());
		if (!owned_completions[index].completion.spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				owned_completions[index].completion.pid, true,
				owned_completions[index].completion.spell_effect_receipts.data(),
				owned_completions[index].completion.spell_effect_receipts.size());
		if (!owned_completions[index].completion.failed_spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				owned_completions[index].completion.pid, false,
				owned_completions[index]
					.completion.failed_spell_effect_receipts.data(),
				owned_completions[index]
					.completion.failed_spell_effect_receipts.size());
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
		persistence_alert(
			AVATAR, "player_save", "redacted", "none", "none",
			"custody_payload_mismatch_rejected",
			"pid=%d revision=%llu components=%llu destructive_write=0 "
			"custody_diagnosis_code=%u recapture_scheduled=%d",
			custody_mismatches[index].pid,
			(unsigned long long)custody_mismatches[index].revision,
			(unsigned long long)custody_mismatches[index].components,
			static_cast<unsigned>(custody_mismatches[index].custody_diagnosis),
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
		auto selected = durable_ready.begin();
		size_t snapshot_bytes = 0;
		int trace_pid = 0;
		player_revision_t trace_revision = 0;
		player_save_submit_result submitted = player_save_submit_result::replay_busy;
		for (size_t visited = 0; selected != durable_ready.end() &&
					 visited < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS;
		     ++visited, ++selected)
		{
			snapshot_bytes = selected->body.encoded_size_bound;
			trace_pid = selected->body.pid;
			trace_revision = selected->body.revision;
			submitted = player_save_worker_submit_owned_retained(&selected->body,
									     selected->residence);
			if (submitted != player_save_submit_result::replay_busy)
				break;
		}
		if (selected == durable_ready.end() ||
		    submitted == player_save_submit_result::replay_busy)
			break;

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
		durable_ready.erase(selected);
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
	if (health.initialized && execution_started && !stop_requested && !lifecycle_stop_attempted)
	{
		lifecycle_admission_closed = false;
		accepting = true;
	}
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
			if (pending_append.empty() && !append_retry && !append_inflight)
				return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.drain_failures;
	return false;
}

namespace
{
bool owned_pipeline_idle_locked()
{
	if (!pending_append.empty() || append_retry || append_inflight || !durable_ready.empty() ||
	    retained_bytes || append_inflight_pid || append_inflight_revision)
		return false;
	for (const auto &fence : terminal_fences)
		if (fence.death_pinned || fence.death_snapshot || fence.death_residence)
			return false;
	for (const auto &literal : literal_inventory_checkpoints)
		if (literal.token.pid || literal.held || literal.execution_hold_generation)
			return false;
	return true;
}

bool owned_coordinator_idle()
{
	const auto state = critical_command_coordinator_health_copy();
	return !state.queued && !state.inflight && !state.blocked && !state.publication_pending &&
	       !state.awaiting_durability && !state.admission_queue_bytes && !state.append_inflight;
}

bool owned_lifecycle_idle(uint64_t epoch)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || !owned_pipeline_idle_locked())
			return false;
	}
	if (!player_save_worker_idle() || !owned_coordinator_idle() ||
	    !player_save_execution_guard::ownership_epoch_quiescent(epoch))
		return false;
	// A cached frame list or scalar revision is not a namespace proof. Retained
	// quarantine/policy originals stay byte-for-byte on disk across clean close.
	std::vector<player_save_journal_retained_frame> frames;
	if (player_save_journal_collect_lifecycle_frames(&frames) != player_save_journal_result::ok)
		return false;
	for (const auto &frame : frames)
		if (!frame.quarantined && !frame.policy_fenced)
			return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return owned_pipeline_idle_locked() && player_save_worker_idle() &&
	       player_save_execution_guard::ownership_epoch_quiescent(epoch);
}
} // namespace

bool player_save_pipeline_drain_owned(uint64_t timeout_msec)
{
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (!epoch || !timeout_msec || !nevent_is_game_thread() ||
	    !critical_command_coordinator_lifecycle_guard_held_by_current_thread())
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		lifecycle_admission_closed = true;
		accepting = false;
		if (!health.initialized || stop_requested || !execution_started)
			return false;
		update_depth_locked();
	}
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	try
	{
		while (std::chrono::steady_clock::now() < deadline)
		{
			player_save_pipeline_pulse();
			const auto now = std::chrono::steady_clock::now();
			if (now >= deadline)
				break;
			const auto remaining =
				std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
										      now)
					.count();
			// Its game-thread observer pumps save receipts before publication,
			// even when the critical completion batch is empty.
			if (!critical_command_coordinator_drain(static_cast<uint64_t>(remaining)))
				break;
			if (owned_lifecycle_idle(epoch) &&
			    std::chrono::steady_clock::now() < deadline)
				return true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	catch (...)
	{
		// An unavailable proof or callback leaves the original owners running;
		// it never authorizes stop, epoch end or journal namespace teardown.
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.drain_failures;
	return false;
}

bool player_save_pipeline_shutdown_owned(void)
{
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (!epoch || !nevent_is_game_thread() ||
	    !critical_command_coordinator_lifecycle_guard_held_by_current_thread())
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		lifecycle_admission_closed = true;
		accepting = false;
		update_depth_locked();
	}
	try
	{
		// No callbacks or native work are pumped after destructive world teardown.
		if (!owned_lifecycle_idle(epoch))
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!owned_pipeline_idle_locked())
				return false;
			lifecycle_stop_attempted = true;
			stop_requested = true;
			replay_gate.begin_replay();
			append_available.notify_all();
		}
		player_save_execution_guard::signal_ownership_change(epoch);
		if (dispatcher.joinable())
			dispatcher.join();
		if (!player_save_worker_shutdown_if_idle() || !owned_lifecycle_idle(epoch) ||
		    !player_save_execution_guard::end_ownership_epoch(epoch))
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			shutdown_incomplete = true;
			return false;
		}
		// Namespace cleanup cannot clear owners while this epoch still exists.
		player_save_journal_shutdown();
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.initialized = false;
		execution_started = false;
		shutdown_incomplete = false;
		update_depth_locked();
		return true;
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		shutdown_incomplete = true;
		return false;
	}
}

player_save_pipeline_health player_save_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return health;
}

player_save_pipeline_diagnostic player_save_pipeline_diagnostic_copy(int pid)
{
	player_save_pipeline_diagnostic result;
	std::unique_lock<std::mutex> lock(pipeline_mutex, std::try_to_lock);
	if (!lock.owns_lock())
		return result;
	result.available = true;
	result.health = health;
	const terminal_fence *fence = find_terminal_fence_locked(pid);
	const auto *literal = find_literal_inventory_locked(pid);
	result.pid_admission_open = !find_target_save_login_fence_locked(pid) &&
				    !(fence && fence->death_pinned) && !(literal && literal->held);
	result.retained_save = !health.initialized || stop_requested || !accepting || fence ||
			       literal || append_inflight_pid == pid ||
			       any_snapshot_is_retained_locked(pid);
	return result;
}

bool player_save_pipeline_loads_allowed(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (lifecycle_admission_closed)
		return false;
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

bool player_save_pipeline_sealed_save_pending(int pid)
{
	if (pid <= 0 || player_save_journal_pid_quarantined(pid))
		return true;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_target_save_login_fence_locked(pid) || find_terminal_fence_locked(pid) ||
		    find_literal_inventory_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid))
			return true;
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(pid, &revision))
		return false;
	return revision.overflowed || revision.queued_components || revision.inflight_components;
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
		    find_terminal_fence_locked(pid) || find_literal_inventory_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid))
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
		    find_literal_inventory_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
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
	if (lifecycle_admission_closed || (health.initialized && !execution_started))
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	if (auto *literal = find_literal_inventory_locked(pid); literal && literal->held)
		return false;
	if (terminal_fence *fence = find_terminal_fence_locked(pid); fence && fence->death_pinned)
		return false;
	return true;
}

bool player_save_pipeline_authoritative_hydration_admitted(int pid)
{
	if (pid <= 0 || player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (lifecycle_admission_closed || (health.initialized && !execution_started))
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	if (auto *literal = find_literal_inventory_locked(pid);
	    literal && literal->held && !literal->restored_sql_drop)
		return false;
#ifdef __NO_MYSQL__
	if (auto *literal = find_literal_inventory_locked(pid); literal && literal->held)
	{
		try
		{
			critical_command original;
			int original_pid = 0;
			uint64_t root_uid = 0;
			if (critical_command_decode(literal->payload.data(),
						    literal->payload.size(), &original) !=
				    critical_command_codec_result::ok ||
			    !coin_physical_recovery_identity(original, &original_pid, &root_uid) ||
			    original_pid != pid || root_uid != literal->token.root_uid)
				return false;
		}
		catch (...)
		{
			return false;
		}
	}
#endif
	if (terminal_fence *fence = find_terminal_fence_locked(pid); fence && fence->death_pinned)
		return false;
	return true;
}

/** Stop the pipeline and clear worker, revision, and health state for an isolated test. */
void player_save_pipeline_reset_for_tests(void)
{
	player_save_pipeline_shutdown();
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (shutdown_incomplete)
			return;
	}
	player_save_worker_reset_for_tests();
	player_revision_reset_for_tests();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
	lifecycle_admission_closed = false;
	lifecycle_stop_attempted = false;
	replay_gate.begin_replay();
	stop_requested = false;
	accepting = false;
	append_inflight = false;
	append_inflight_pid = 0;
	append_inflight_revision = 0;
	target_save_login_fences.fill({});
}
