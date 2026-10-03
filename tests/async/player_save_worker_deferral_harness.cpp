#include "net/network_wakeup.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

// The repository callback is controlled. Worker, revision state, codec and
// journal are production translation units. No database or game is contacted.
namespace
{
thread_local int allocation_countdown = -1;
thread_local bool allocation_failed = false;
std::atomic<bool> reject_checkpoint_sync{ false };
constexpr int held_pid = 61001;
constexpr player_component_mask_t components = PLAYER_COMPONENT_STATUS;

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

std::vector<uint8_t> encoded(const player_snapshot &snapshot)
{
	std::vector<uint8_t> bytes;
	require(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok,
		"fixture snapshot must encode through production codec");
	return bytes;
}

player_snapshot capture(int pid, bool receipt = true)
{
	player_snapshot snapshot = {};
	require(player_revision_mark(pid, components, &snapshot.revision), "mark revision");
	require(player_revision_queue(pid, &snapshot.revision, &snapshot.components),
		"queue revision");
	snapshot.pid = pid;
	snapshot.schema_version = receipt ? PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION :
					    PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 22800;
	snapshot.status_integers.push_back({ player_status_field::level, 42, 0, false });
	snapshot.status_strings.push_back(
		{ player_status_string_field::name, "synthetic-deferral" });
	if (receipt)
	{
		player_quest_xp_receipt_snapshot proof = {};
		proof.offering_operation.bytes[0] = 0x71;
		proof.offering_operation.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		proof.reward_index = 3;
		proof.amount = 17;
		snapshot.quest_xp_receipts.push_back(proof);
	}
	snapshot.encoded_size_bound = 8192;
	(void)encoded(snapshot);
	return snapshot;
}

struct state
{
	std::mutex mutex;
	std::condition_variable changed;
	bool held = true;
	bool pause_first = false;
	bool entered = false;
	bool leave_first = false;
	bool block_entered = false;
	bool unblock = false;
	std::map<int, unsigned> calls;
	std::map<int, std::vector<std::vector<uint8_t>>> payloads;
	std::atomic<unsigned> ack_calls{ 0 }, ack_false{ 0 }, terminal_calls{ 0 };
	unsigned held_completions = 0;
};
state *current_state = nullptr;

player_save_apply_result apply(const player_snapshot &snapshot, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	auto bytes = encoded(snapshot);
	std::unique_lock<std::mutex> lock(value.mutex);
	const unsigned count = ++value.calls[snapshot.pid];
	value.payloads[snapshot.pid].push_back(std::move(bytes));
	if (snapshot.pid == 66000)
	{
		value.block_entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.unblock; }),
			"fixture independent callback barrier deadline");
	}
	if (snapshot.pid == held_pid && value.pause_first && count == 1)
	{
		value.entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.leave_first; }),
			"fixture first callback barrier deadline");
	}
	value.changed.notify_all();
	if (snapshot.pid == held_pid && value.held)
	{
#if DURIS_SAVE_DEFERRED_API
		return { player_save_apply_outcome::deferred, 0, 0 };
#else
		// The old public API has no parking disposition. Its existing retry
		// path is the BEFORE control; it must not count as successful deferral.
		return { player_save_apply_outcome::retryable_failure, 0, EBUSY };
#endif
	}
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

bool ack(const player_snapshot &snapshot, player_revision_t revision, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	++value.ack_calls;
	const bool result = player_save_journal_worker_ack(snapshot, revision, nullptr);
	if (!result)
		++value.ack_false;
	return result;
}

void terminal(const player_snapshot &snapshot, void *raw) noexcept
{
	++static_cast<state *>(raw)->terminal_calls;
	player_save_journal_worker_terminal(snapshot, nullptr);
}

bool resume(int pid)
{
#if DURIS_SAVE_DEFERRED_API
	return player_save_worker_resume_deferred(pid);
#else
	(void)pid;
	return false;
#endif
}

void pulse()
{
	player_save_completion completions[32] = {};
	const size_t count = player_save_worker_pulse(completions, 32);
	for (size_t index = 0; index < count; ++index)
		if (completions[index].pid == held_pid)
			++current_state->held_completions;
}

template <typename Predicate> void wait(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		require(std::chrono::steady_clock::now() < deadline,
			"native fixture progress deadline");
		pulse();
		std::this_thread::yield();
	}
}

unsigned calls(state &value, int pid)
{
	std::lock_guard<std::mutex> lock(value.mutex);
	return value.calls[pid];
}

void start(state &value, const std::string &directory)
{
	require(player_save_journal_init(directory.c_str()), "actual journal init");
	require(player_save_worker_set_journal_hooks(player_save_journal_worker_append, ack, &value,
						     terminal),
		"actual journal hooks");
	current_state = &value;
	require(player_save_worker_init(apply, &value, 1), "actual worker init");
	require(player_revision_hydrate(held_pid, 0), "held revision baseline");
}

std::vector<char> journal_bytes(const std::string &directory)
{
	std::ifstream input(directory + "/player-save.journal", std::ios::binary);
	require(static_cast<bool>(input), "actual retained journal exists");
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void submit(player_snapshot snapshot, player_save_submit_result expected)
{
	require(player_save_worker_submit(std::move(snapshot)) == expected, "submit disposition");
}

void healthy_progress(state &value, int first_pid, unsigned count)
{
	for (unsigned index = 0; index < count; ++index)
	{
		const int pid = first_pid + static_cast<int>(index);
		require(player_revision_hydrate(pid, 0), "healthy revision baseline");
		submit(capture(pid, false), player_save_submit_result::accepted);
		wait([&] { return !player_save_worker_pid_pending(pid); });
		require(calls(value, pid) == 1, "healthy actual worker applied once");
	}
}

void parked(state &value)
{
	wait(
		[&]
		{
#if DURIS_SAVE_DEFERRED_API
			return calls(value, held_pid) != 0 &&
			       player_save_worker_health_copy().deferred_pids == 1;
#else
			return calls(value, held_pid) != 0 &&
			       player_save_worker_health_copy().inflight_pids == 0;
#endif
		});
}

void invariant(state &value, const player_snapshot &original)
{
	const auto health = player_save_worker_health_copy();
	require(calls(value, held_pid) == 1, "held callback must remain parked without reapply");
	require(health.retryable_failures == 0 && health.retries_exhausted == 0 &&
			health.terminal_failures == 0 && value.terminal_calls == 0,
		"deferral must not consume retries or quarantine");
	require(player_save_worker_pid_pending(held_pid), "held original remains owned");
	require(!player_save_journal_pid_quarantined(held_pid), "held PID never quarantined");
	require(value.held_completions == 0, "deferred owner exposes no completion");
	player_revision_snapshot revision = {};
	require(player_revision_snapshot_copy(held_pid, &revision), "held actual revision exists");
	require(revision.inflight_revision == original.revision &&
			revision.acknowledged_revision == 0,
		"held exact revision neither replaced nor acknowledged");
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.payloads[held_pid].front() == encoded(original),
		"held original wire identity");
}

void release(state &value)
{
	std::lock_guard<std::mutex> lock(value.mutex);
	value.held = false;
}

void parking(state &value, const std::string &directory, bool pending)
{
	auto first = capture(held_pid);
	const auto first_wire = encoded(first);
	submit(first, player_save_submit_result::accepted);
	parked(value);
	const auto disk = journal_bytes(directory);
	std::vector<uint8_t> final_wire;
	if (pending)
	{
		submit(capture(held_pid), player_save_submit_result::coalesced);
		auto latest = capture(held_pid);
		final_wire = encoded(latest);
		submit(latest, player_save_submit_result::coalesced);
	}
	healthy_progress(value, 62000, PLAYER_SAVE_WORKER_MAX_RETRIES + 3);
	invariant(value, first);
	require(value.ack_calls == PLAYER_SAVE_WORKER_MAX_RETRIES + 3,
		"only healthy PIDs reached real journal ACK");
	if (!pending)
		require(journal_bytes(directory) == disk,
			"healthy ACK preserves exact held disk frame");
	require(!resume(0) && !resume(-1) && !resume(64000), "invalid resume refuses");
	release(value);
	require(resume(held_pid), "parked original resumes explicitly");
	wait([&] { return !player_save_worker_pid_pending(held_pid); });
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		const auto &payloads = value.payloads[held_pid];
		require(payloads[1] == first_wire, "resume applies exact original before pending");
		if (pending)
			require(payloads.size() == 3 && payloads[2] == final_wire,
				"only latest pending identity is promoted after original");
		else
			require(payloads.size() == 2, "original resumes only once");
	}
	require(player_save_journal_health_copy().records == (pending ? 1U : 0U),
		"exact receipt ACK retires only proven frames; coalesced middle stays durable");
	require(!resume(held_pid), "settled owner cannot be resumed");
}

void wake_race(state &value, bool newer)
{
	value.pause_first = true;
	auto original = capture(held_pid);
	submit(original, player_save_submit_result::accepted);
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.entered; }),
			"actual callback entered before wake race");
	}
	require(!resume(held_pid), "resume while callback executing refuses");
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		value.leave_first = true;
		value.changed.notify_all();
	}
	parked(value);
	invariant(value, original);
	// Occupy the sole actual worker, giving a deterministic wake-before-dispatch
	// window. The parked original must remain immutable in that window.
	require(player_revision_hydrate(66000, 0), "blocker revision baseline");
	submit(capture(66000, false), player_save_submit_result::accepted);
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.block_entered; }),
			"independent worker is executing before concurrent wakes");
	}
	std::atomic<unsigned> winners{ 0 };
	std::vector<std::thread> wake_threads;
	for (unsigned index = 0; index < 8; ++index)
		wake_threads.emplace_back(
			[&]
			{
				if (resume(held_pid))
					++winners;
			});
	for (auto &thread : wake_threads)
		thread.join();
	require(winners == 1, "concurrent wakes enqueue one original while worker occupied");
	std::vector<uint8_t> newest_wire;
	if (newer)
	{
		submit(capture(held_pid), player_save_submit_result::coalesced);
		auto latest = capture(held_pid);
		newest_wire = encoded(latest);
		submit(latest, player_save_submit_result::coalesced);
		player_revision_snapshot revision = {};
		require(player_revision_snapshot_copy(held_pid, &revision) &&
				revision.inflight_revision == original.revision &&
				revision.queued_revision == latest.revision,
			"wake undispatched window preserves original and coalesces only pending");
	}
	require(value.ack_calls == 0 && value.terminal_calls == 0,
		"wake races never manufacture completion or acknowledgement");
	release(value);
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		value.unblock = true;
		value.changed.notify_all();
	}
	wait([&] { return !player_save_worker_pid_pending(held_pid); });
	std::lock_guard<std::mutex> lock(value.mutex);
	const auto &payloads = value.payloads[held_pid];
	require(payloads.size() == (newer ? 3U : 2U) && payloads[1] == encoded(original),
		"wake preserves original until exact completion");
	if (newer)
		require(payloads[2] == newest_wire, "pending after wake uses only latest identity");
	require(value.ack_calls == (newer ? 3U : 2U), "one ACK each for actual applied identity");
}

void allocation_walk(state &value)
{
	auto original = capture(held_pid);
	submit(original, player_save_submit_result::accepted);
	parked(value);
	unsigned injected = 0;
	for (int position = 0; position < 2; ++position)
	{
		bool observed = false;
		// std::deque allocates a new block periodically. Continue until both
		// the set-node and the subsequent deque-block allocation are exercised.
		for (unsigned attempt = 0; attempt < 260 && !observed; ++attempt)
		{
			const auto bytes = player_save_worker_health_copy().queued_bytes;
			const auto previous_calls = calls(value, held_pid);
			allocation_countdown = position;
			allocation_failed = false;
			const bool resumed = resume(held_pid);
			allocation_countdown = -1;
			observed = allocation_failed;
			if (observed)
			{
				++injected;
				require(!resumed && calls(value, held_pid) == previous_calls,
					"allocation failure returns false without invoking original");
				require(player_save_worker_health_copy().queued_bytes == bytes,
					"failed queue allocation retains original accounted bytes");
			}
			else
				require(resumed, "uninjected resume accepted");
			parked(value);
		}
		require(observed, "actual set and deque queue allocation faults both exercised");
		healthy_progress(value, 63000 + position, 1);
		require(player_save_worker_pid_pending(held_pid) && value.terminal_calls == 0,
			"allocation failure preserves parked identity and healthy progress");
	}
	require(injected == 2, "actual resume queue allocation fault walk completed");
	release(value);
	require(resume(held_pid), "repair resumes original after allocation walk");
	wait([&] { return !player_save_worker_pid_pending(held_pid); });
	std::lock_guard<std::mutex> lock(value.mutex);
	for (const auto &wire : value.payloads[held_pid])
		require(wire == encoded(original), "allocation walk never changes retained bytes");
}

void ack_failure(state &value)
{
	value.held = false;
	auto original = capture(held_pid);
	// A real temp-file sync failure occurs inside journal checkpoint. Retrying
	// the same original must acknowledge it without changing the receipt.
	// Append needs directory sync too: append first, then submit with an append
	// observer disabled through the actual hooks API while the worker is empty.
	require(player_save_journal_append(original) == player_save_journal_result::ok,
		"original durable append before checkpoint fault");
	require(player_save_worker_set_journal_hooks(nullptr, ack, &value, terminal),
		"worker borrows already durable frame");
	reject_checkpoint_sync = true;
	submit(original, player_save_submit_result::accepted);
	wait([&] { return value.ack_false.load() != 0; });
	require(player_save_journal_health_copy().records == 1 &&
			!player_save_journal_pid_quarantined(held_pid),
		"failed real checkpoint retains original active frame without quarantine");
	// Stop retry dispatch until the same-ID checkpoint fault has been observed.
	reject_checkpoint_sync = false;
	wait([&] { return !player_save_worker_pid_pending(held_pid); });
	require(value.ack_calls >= 2 && value.ack_false >= 1 && value.terminal_calls == 0,
		"real checkpoint false followed by same-original successful ACK");
	require(player_save_journal_health_copy().records == 0,
		"repaired checkpoint actually retires frame");
	std::lock_guard<std::mutex> lock(value.mutex);
	for (const auto &wire : value.payloads[held_pid])
		require(wire == encoded(original), "checkpoint retry preserves full receipt bytes");
}

struct replay_state
{
	std::vector<std::vector<uint8_t>> expected;
	unsigned applied = 0;
};

player_save_apply_result replay(const player_snapshot &snapshot, void *raw)
{
	auto &value = *static_cast<replay_state *>(raw);
	require(value.applied < value.expected.size() &&
			encoded(snapshot) == value.expected[value.applied],
		"reopen replays original encoded frames in revision order");
	++value.applied;
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

void shutdown_reopen(state &value, const std::string &directory)
{
	auto original = capture(held_pid);
	submit(original, player_save_submit_result::accepted);
	parked(value);
	auto pending = capture(held_pid);
	submit(pending, player_save_submit_result::coalesced);
	healthy_progress(value, 65000, 2);
	invariant(value, original);
	const auto disk = journal_bytes(directory);
	player_save_worker_shutdown();
	require(player_save_worker_pid_pending(held_pid) && !resume(held_pid),
		"shutdown preserves ownership and refuses wake");
	require(journal_bytes(directory) == disk && value.ack_calls == 2,
		"shutdown does not ACK or alter either held frame");
	player_save_journal_shutdown();
	require(player_save_journal_init(directory.c_str()), "cold journal reopen");
	require(journal_bytes(directory) == disk, "reopen preserves exact disk bytes");
	replay_state cold{ { encoded(original), encoded(pending) }, 0 };
	require(player_save_journal_replay(replay, &cold) == player_save_journal_result::ok &&
			cold.applied == 2,
		"explicit fixture release permits original native journal replay");
	require(player_save_journal_health_copy().records == 0, "cold exact proofs retire frames");
}

void warm_reinit(state &value, const std::string &directory)
{
	auto original = capture(held_pid);
	submit(original, player_save_submit_result::accepted);
	parked(value);
	invariant(value, original);
	const auto disk = journal_bytes(directory);
	player_save_worker_shutdown();
	require(player_save_worker_pid_pending(held_pid) && !resume(held_pid),
		"warm shutdown retains parked owner and refuses unavailable wake");
	// Hooks and their live context remain retained. Reinitialization requires a
	// valid apply context; it must not silently retry an unresolved parked job.
	require(player_save_worker_init(apply, &value, 1), "warm worker reinit with live context");
	healthy_progress(value, 67000, 2);
	invariant(value, original);
	require(journal_bytes(directory) == disk && value.ack_calls == 2,
		"warm restart healthy ACKs preserve exact held frame");
	release(value);
	require(resume(held_pid), "explicit owner release wakes retained warm original");
	wait([&] { return !player_save_worker_pid_pending(held_pid); });
	require(value.ack_calls == 3 && player_save_journal_health_copy().records == 0,
		"warm original receives one real exact journal ACK after explicit wake");
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.payloads[held_pid].size() == 2 &&
			value.payloads[held_pid][1] == encoded(original),
		"warm resume preserves original encoded payload and receipt");
}
} // namespace

extern "C" void *__real__Znwm(size_t size);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (allocation_countdown >= 0 && allocation_countdown-- == 0)
	{
		allocation_failed = true;
		throw std::bad_alloc();
	}
	return __real__Znwm(size);
}

extern "C" int __real_fdatasync(int descriptor);
extern "C" int __wrap_fdatasync(int descriptor)
{
	struct stat status = {};
	if (reject_checkpoint_sync && fstat(descriptor, &status) == 0 && S_ISREG(status.st_mode))
	{
		errno = EIO;
		return -1;
	}
	return __real_fdatasync(descriptor);
}

int main(int argc, char **argv)
{
	state value;
	try
	{
		require(argc == 3, "case and fresh private journal directory required");
		start(value, argv[2]);
		const std::string scenario = argv[1];
		if (scenario == "parking")
			parking(value, argv[2], false);
		else if (scenario == "pending_coalesce")
			parking(value, argv[2], true);
		else if (scenario == "wake_race")
			wake_race(value, false);
		else if (scenario == "wake_before_newer_submit")
			wake_race(value, true);
		else if (scenario == "allocation_walk")
			allocation_walk(value);
		else if (scenario == "ack_failure")
			ack_failure(value);
		else if (scenario == "shutdown_reopen")
			shutdown_reopen(value, argv[2]);
		else if (scenario == "warm_reinit")
			warm_reinit(value, argv[2]);
		else
			throw std::runtime_error("unknown native case");
		player_save_worker_shutdown();
		player_save_worker_reset_for_tests();
		player_revision_reset_for_tests();
		player_save_journal_shutdown();
		std::cout << "PASS " << scenario << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		{
			std::lock_guard<std::mutex> lock(value.mutex);
			value.leave_first = true;
			value.unblock = true;
			value.changed.notify_all();
		}
		reject_checkpoint_sync = false;
		allocation_countdown = -1;
		player_save_worker_shutdown();
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << " calls=" << calls(value, held_pid) << " ack_calls=" << value.ack_calls
			  << " ack_false=" << value.ack_false
			  << " terminal=" << value.terminal_calls << " retries_exhausted="
			  << player_save_worker_health_copy().retries_exhausted << '\n';
		return 1;
	}
}
