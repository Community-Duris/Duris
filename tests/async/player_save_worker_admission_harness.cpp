#include "net/network_wakeup.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>

// Actual worker/revision/codec/journal/observability. Repository apply is a
// controlled canonical oracle; no SQL connection or gameplay is exercised.
namespace
{
constexpr int target_pid = 61700, blocker_pid = 61699;
constexpr player_component_mask_t full_mask = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS |
					      PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
struct fault
{
	int ordinal = -1;
	size_t size = 0;
};
thread_local fault allocation_fault;
thread_local bool recording = false, injected = false, overflow = false;
thread_local std::array<size_t, 256> allocation_sizes{};
thread_local size_t allocation_count = 0;

void require(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}

void arm(fault value)
{
	allocation_fault = value;
	recording = true;
	injected = overflow = false;
	allocation_count = 0;
}
void disarm()
{
	recording = false;
	allocation_fault = {};
}

std::vector<uint8_t> encoded(const player_snapshot &snapshot)
{
	std::vector<uint8_t> result;
	require(player_snapshot_encode(snapshot, &result) == player_snapshot_codec_result::ok,
		"typed admission fixture must pass actual codec");
	return result;
}

player_revision_snapshot revision(int pid)
{
	player_revision_snapshot result{};
	require(player_revision_snapshot_copy(pid, &result), "actual revision state exists");
	return result;
}
bool same_revision(const player_revision_snapshot &a, const player_revision_snapshot &b)
{
	return std::tie(a.pid, a.current_revision, a.acknowledged_revision, a.queued_revision,
			a.inflight_revision, a.dirty_components, a.unacknowledged_components,
			a.queued_components, a.inflight_components, a.overflowed) ==
	       std::tie(b.pid, b.current_revision, b.acknowledged_revision, b.queued_revision,
			b.inflight_revision, b.dirty_components, b.unacknowledged_components,
			b.queued_components, b.inflight_components, b.overflowed);
}

using raw_frames = std::vector<std::vector<uint8_t>>;
uint64_t little(const uint8_t *bytes, unsigned count)
{
	uint64_t result = 0;
	for (unsigned index = 0; index < count; ++index)
		result |= static_cast<uint64_t>(bytes[index]) << (index * 8);
	return result;
}
raw_frames frames(const std::string &directory, int pid)
{
	std::ifstream input(directory + "/player-save.journal", std::ios::binary);
	require(static_cast<bool>(input), "actual journal exists");
	const std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(input),
					  std::istreambuf_iterator<char>() };
	raw_frames result;
	for (size_t offset = 0; offset < bytes.size();)
	{
		require(bytes.size() - offset >= 72, "actual raw journal header bound");
		const auto size = little(bytes.data() + offset + 16, 8);
		require(little(bytes.data() + offset + 12, 4) == 72 && size >= 72 &&
				size <= bytes.size() - offset,
			"actual raw journal record bound");
		if (little(bytes.data() + offset + 40, 4) == static_cast<uint64_t>(pid))
			result.emplace_back(bytes.begin() + offset, bytes.begin() + offset + size);
		offset += size;
	}
	return result;
}

struct state
{
	std::mutex mutex;
	std::condition_variable changed;
	std::string directory;
	bool journal = false;
	int blocked_pid = blocker_pid;
	bool entered = false, release = false, deferred = false;
	bool append_pause = false, append_entered = false, append_release = false;
	bool inject_submit = false;
	fault requested;
	std::map<std::pair<int, player_revision_t>, std::vector<uint8_t>> expected;
	std::map<std::pair<int, player_revision_t>, unsigned> calls, completions;
	std::atomic<unsigned> ack_calls{ 0 }, ack_false{ 0 }, terminal_calls{ 0 };
};
state *current = nullptr;

void remember(state &value, const player_snapshot &snapshot)
{
	const auto bytes = encoded(snapshot);
	std::lock_guard<std::mutex> lock(value.mutex);
	value.expected[{ snapshot.pid, snapshot.revision }] = bytes;
}
player_snapshot capture(state &value, int pid, player_component_mask_t mask = full_mask,
			bool receipts = true, const player_snapshot *prior = nullptr,
			bool mark = true)
{
	player_snapshot snapshot{};
	require((!mark || player_revision_mark(pid, mask, &snapshot.revision)) &&
			player_revision_queue(pid, &snapshot.revision, &snapshot.components),
		"actual mark and queue");
	snapshot.pid = pid;
	snapshot.schema_version = receipts ? PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION :
					     PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 22800;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 42, 0, false });
	snapshot.status_strings.push_back(
		{ player_status_string_field::name, "synthetic-admission-original" });
	if (receipts)
	{
		if (prior)
		{
			snapshot.quest_xp_receipts = prior->quest_xp_receipts;
			snapshot.spell_effect_receipts = prior->spell_effect_receipts;
			snapshot.craft_receipts = prior->craft_receipts;
		}
		player_quest_xp_receipt_snapshot quest{};
		quest.offering_operation.bytes[0] = 0x31;
		quest.offering_operation.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		quest.reward_index = 2;
		quest.amount = 37;
		snapshot.quest_xp_receipts.push_back(quest);
		player_spell_effect_receipt_snapshot spell{};
		spell.operation_id.bytes[0] = 0x32;
		spell.operation_id.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		spell.effect_id = 6;
		snapshot.spell_effect_receipts.push_back(spell);
		player_craft_receipt_snapshot craft{};
		craft.operation_id.bytes[0] = 0x33;
		craft.operation_id.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		craft.discipline = 2;
		craft.experience = 79;
		snapshot.craft_receipts.push_back(craft);
	}
	remember(value, snapshot);
	return snapshot;
}

player_save_apply_result apply(const player_snapshot &snapshot, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	const auto bytes = encoded(snapshot);
	std::unique_lock<std::mutex> lock(value.mutex);
	require(value.expected.at({ snapshot.pid, snapshot.revision }) == bytes,
		"actual worker applies complete original canonical payload and receipts");
	++value.calls[{ snapshot.pid, snapshot.revision }];
	if (snapshot.pid == value.blocked_pid && !value.release)
	{
		value.entered = true;
		value.changed.notify_all();
		if (value.deferred)
			return { player_save_apply_outcome::deferred, 0, 0 };
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.release; }),
			"controlled actual apply barrier deadline");
	}
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}
bool append(const player_snapshot &snapshot, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	const bool result = player_save_journal_worker_append(snapshot, nullptr);
	if (!result)
		return false;
	if (snapshot.pid == target_pid && value.append_pause)
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		value.append_entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.append_release; }),
			"actual durable append boundary deadline");
	}
	// Fault starts only after production append and its durability work.
	if (snapshot.pid == target_pid && value.inject_submit)
		arm(value.requested);
	return true;
}
bool acknowledge(const player_snapshot &snapshot, player_revision_t rev, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	++value.ack_calls;
	const bool result = player_save_journal_worker_ack(snapshot, rev, nullptr);
	if (!result)
		++value.ack_false;
	return result;
}
void terminal(const player_snapshot &snapshot, void *raw) noexcept
{
	++static_cast<state *>(raw)->terminal_calls;
	player_save_journal_worker_terminal(snapshot, nullptr);
}
void pulse()
{
	player_save_completion completed[32]{};
	const auto count = player_save_worker_pulse(completed, 32);
	for (size_t index = 0; index < count; ++index)
	{
		const auto &item = completed[index];
		require(item.outcome == player_save_apply_outcome::applied &&
				item.durable_revision == item.revision,
			"admitted exact execution succeeds without retry/promotion fault injection");
		std::lock_guard<std::mutex> lock(current->mutex);
		++current->completions[{ item.pid, item.revision }];
	}
}
template <typename Predicate> void wait(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		require(std::chrono::steady_clock::now() < deadline,
			"native admission progress deadline");
		pulse();
		std::this_thread::yield();
	}
}
void release(state &value)
{
	std::lock_guard<std::mutex> lock(value.mutex);
	value.release = true;
	value.append_release = true;
	value.changed.notify_all();
}
void finish(state &value)
{
	release(value);
	wait(
		[&]
		{
			const auto h = player_save_worker_health_copy();
			return h.queued_pids == 0 && h.inflight_pids == 0;
		});
	require(value.terminal_calls == 0 &&
			player_save_worker_health_copy().retries_exhausted == 0,
		"admission checks invent no terminal or exhausted-retry disposition");
}
void start(state &value, const std::string &directory, bool journal, int blocked = blocker_pid,
	   bool deferred = false)
{
	value.directory = directory;
	value.journal = journal;
	value.blocked_pid = blocked;
	value.deferred = deferred;
	current = &value;
	require(player_save_journal_init(directory.c_str()), "fresh actual journal init");
	if (journal)
		require(player_save_worker_set_journal_hooks(append, acknowledge, &value, terminal),
			"actual journal hooks");
	require(player_save_worker_init(apply, &value, 1), "actual single worker init");
	require(player_revision_hydrate(target_pid, 0) && player_revision_hydrate(blocker_pid, 0),
		"actual native revision baselines");
}
void block(state &value, bool redundant = false)
{
	auto snapshot = capture(value, value.blocked_pid,
				value.blocked_pid == target_pid ?
					(redundant ? PLAYER_COMPONENT_LANGUAGES : full_mask) :
					PLAYER_COMPONENT_STATUS,
				value.blocked_pid == target_pid && !redundant);
	require(player_save_worker_submit_retained(&snapshot) ==
			player_save_submit_result::accepted,
		"admit actual barrier job");
	wait(
		[&]
		{
			std::lock_guard<std::mutex> lock(value.mutex);
			return value.entered;
		});
	if (value.deferred)
		wait([] { return player_save_worker_health_copy().deferred_pids == 1; });
}

bool healthy(state &value, int pid)
{
	require(player_revision_hydrate(pid, 0), "healthy PID hydrate");
	auto snapshot = capture(value, pid, PLAYER_COMPONENT_STATUS, false);
	const auto rev = snapshot.revision;
	require(player_save_worker_submit_retained(&snapshot) ==
			player_save_submit_result::accepted,
		"unrelated PID admission");
	wait([&] { return !player_save_worker_pid_pending(pid); });
	std::lock_guard<std::mutex> lock(value.mutex);
	return value.calls[{ pid, rev }] == 1 && value.completions[{ pid, rev }] == 1;
}

void check_refusal(state &value, player_snapshot &input, const std::vector<uint8_t> &canonical,
		   size_t bound, const player_revision_snapshot &before,
		   const player_save_worker_health &health, const raw_frames &disk,
		   bool prior_owner)
{
	std::vector<uint8_t> after;
	const bool input_exact = player_snapshot_encode(input, &after) ==
					 player_snapshot_codec_result::ok &&
				 after == canonical && input.encoded_size_bound == bound;
	const auto now = player_save_worker_health_copy();
	const bool revision_exact = same_revision(revision(target_pid), before);
	const bool health_exact =
		now.queued_bytes == health.queued_bytes && now.queued_pids == health.queued_pids &&
		now.inflight_pids == health.inflight_pids &&
		now.deferred_pids == health.deferred_pids && now.submitted == health.submitted &&
		now.coalesced == health.coalesced;
	const bool owner_exact = player_save_worker_pid_pending(target_pid) == prior_owner;
	std::cout << "injected=" << injected << " allocations=" << allocation_count
		  << " input_exact=" << input_exact << " revision_exact=" << revision_exact
		  << " health_exact=" << health_exact << " owner_exact=" << owner_exact
		  << " bytes=" << now.queued_bytes << '\n';
	for (size_t index = 0; index < allocation_count; ++index)
		std::cout << "allocation ordinal=" << index << " size=" << allocation_sizes[index]
			  << '\n';
	require(injected && !overflow, "requested actual allocator failure window was reached");
	require(input_exact && revision_exact && health_exact && owner_exact,
		"capacity refusal preserves exact retained input, revision and clean original slot accounting");
	if (value.journal)
	{
		const auto actual = frames(value.directory, target_pid);
		require(actual.size() == disk.size() + 1 &&
				std::equal(disk.begin(), disk.end(), actual.begin()),
			"real durable append retained every original raw frame before allocation refusal");
		const auto &raw = actual.back();
		require(std::vector<uint8_t>(raw.begin() + 72, raw.end()) == canonical &&
				!player_save_journal_pid_quarantined(target_pid),
			"refused original frame has exact canonical bytes, no fake ACK/quarantine");
	}
}

void initial(state &value, int ordinal)
{
	block(value);
	// The fourth initial-admission allocation is an actual ready-set growth,
	// measured with this native standard library rather than a fixed bucket
	// count. Earlier cases cover job/node allocations without that growth.
	if (ordinal == 3)
	{
		std::unordered_set<int> keys;
		keys.insert(blocker_pid);
		keys.erase(blocker_pid);
		unsigned prefix = 0;
		while (prefix < PLAYER_SAVE_WORKER_MAX_PIDS - 1)
		{
			const auto old = keys.bucket_count();
			keys.insert(64000 + static_cast<int>(prefix));
			if (keys.bucket_count() != old)
				break;
			++prefix;
		}
		require(prefix < PLAYER_SAVE_WORKER_MAX_PIDS - 1,
			"fourth initial allocation native growth within bound");
		for (unsigned index = 0; index < prefix; ++index)
		{
			const int pid = 64000 + static_cast<int>(index);
			require(player_revision_hydrate(pid, 0), "initial growth filler hydrate");
			auto item = capture(value, pid, PLAYER_COMPONENT_STATUS, false);
			require(player_save_worker_submit_retained(&item) ==
					player_save_submit_result::accepted,
				"initial growth filler queued behind actual barrier");
		}
	}
	auto input = capture(value, target_pid);
	const auto canonical = encoded(input);
	const auto before = revision(target_pid);
	const auto health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	value.inject_submit = true;
	value.requested = { ordinal, 0 };
	if (!value.journal)
		arm(value.requested);
	const auto result = player_save_worker_submit_retained(&input);
	disarm();
	value.inject_submit = false;
	require(result == player_save_submit_result::capacity_exceeded,
		"injected initial admission refuses capacity");
	check_refusal(value, input, canonical, 8192, before, health, disk, false);
	require(player_save_worker_submit_retained(&input) == player_save_submit_result::accepted,
		"same retained input retry accepted");
	finish(value);
	require(healthy(value, 61801), "unrelated PID progresses after repaired admission");
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.calls[{ target_pid, before.current_revision }] == 1 &&
			value.completions[{ target_pid, before.current_revision }] == 1,
		"same retained complete payload receives one execution and exact completion");
}

void pending(state &value, const std::string &kind)
{
	block(value);
	player_snapshot prior;
	if (kind == "undispatched")
	{
		prior = capture(value, target_pid);
		require(player_save_worker_submit_retained(&prior) ==
				player_save_submit_result::accepted,
			"queued original accepted");
	}
	else
	{
		// Reconstruct only this test's sealed capture value from its explicit
		// deterministic revision, never production owner state.
		std::lock_guard<std::mutex> lock(value.mutex);
		require(player_snapshot_decode(value.expected.at({ target_pid, 1 }).data(),
					       value.expected.at({ target_pid, 1 }).size(),
					       &prior) == player_snapshot_codec_result::ok,
			"sealed active capture decode");
	}
	auto input = capture(value, target_pid, full_mask, true, &prior);
	if (kind == "replacement")
	{
		auto copy = input;
		require(player_save_worker_submit_retained(&copy) ==
				player_save_submit_result::coalesced,
			"first pending admitted");
		input = capture(value, target_pid, full_mask, true, &input);
	}
	const auto canonical = encoded(input);
	const auto before = revision(target_pid);
	const auto health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	value.inject_submit = true;
	value.requested = { 0, 0 };
	const auto result = player_save_worker_submit_retained(&input);
	disarm();
	value.inject_submit = false;
	require(result == player_save_submit_result::capacity_exceeded,
		"pending allocation refusal");
	check_refusal(value, input, canonical, 8192, before, health, disk, true);
	require(player_save_worker_submit_retained(&input) == player_save_submit_result::coalesced,
		"same pending retained input retry");
	if (value.deferred)
	{
		require(healthy(value, 61802),
			"unrelated PID progresses while original remains deferred");
		release(value);
		require(player_save_worker_resume_deferred(target_pid),
			"explicit controlled wake of protected original");
	}
	finish(value);
	require(healthy(value, 61803), "unrelated PID progresses after pending repair");
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.calls[{ target_pid, before.current_revision }] == 1 &&
			value.completions[{ target_pid, before.current_revision }] == 1,
		"pending exact original complete receipt snapshot applies once after retry");
}

struct growth
{
	unsigned prefix;
	size_t size;
};
growth probe(bool deque)
{
	if (deque)
	{
		std::deque<int> values;
		values.push_back(blocker_pid);
		values.pop_front();
		for (unsigned index = 1; index < PLAYER_SAVE_WORKER_MAX_PIDS; ++index)
		{
			arm({});
			values.push_back(static_cast<int>(index));
			disarm();
			require(!overflow, "native deque probe fits fixed allocation observations");
			if (allocation_count)
				return { index - 1, allocation_sizes[0] };
		}
	}
	else
	{
		std::unordered_set<int> values;
		values.insert(blocker_pid);
		values.erase(blocker_pid);
		for (unsigned index = 1; index < PLAYER_SAVE_WORKER_MAX_PIDS; ++index)
		{
			const auto buckets = values.bucket_count();
			arm({});
			values.insert(static_cast<int>(index));
			disarm();
			require(!overflow, "native set probe fits fixed allocation observations");
			if (values.bucket_count() != buckets)
			{
				size_t largest = 0;
				for (size_t ordinal = 0; ordinal < allocation_count; ++ordinal)
					largest = std::max(largest, allocation_sizes[ordinal]);
				return { index - 1, largest };
			}
		}
	}
	throw std::runtime_error(
		"native standard-container growth window unavailable within production PID bound");
}
void queue_growth(state &value, bool deque)
{
	const auto window = probe(deque);
	block(value);
	for (unsigned index = 0; index < window.prefix; ++index)
	{
		const int pid = 62000 + static_cast<int>(index);
		require(player_revision_hydrate(pid, 0), "growth filler hydrate");
		auto item = capture(value, pid, PLAYER_COMPONENT_STATUS, false);
		require(player_save_worker_submit_retained(&item) ==
				player_save_submit_result::accepted,
			"growth filler queued");
	}
	std::cout << "native_growth prefix=" << window.prefix << " allocation_size=" << window.size
		  << '\n';
	auto input = capture(value, target_pid);
	const auto canonical = encoded(input);
	const auto before = revision(target_pid);
	const auto health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	value.inject_submit = true;
	value.requested = { -1, window.size };
	const auto result = player_save_worker_submit_retained(&input);
	disarm();
	value.inject_submit = false;
	require(injected && allocation_count > 1 && allocation_sizes[0] != window.size,
		"native queue growth fault is distinct from initial job allocation");
	require(result == player_save_submit_result::capacity_exceeded,
		"actual ready queue growth refusal");
	check_refusal(value, input, canonical, 8192, before, health, disk, false);
	require(player_save_worker_submit_retained(&input) == player_save_submit_result::accepted,
		"same ready queue original retries");
	finish(value);
	require(healthy(value, 61900), "unrelated PID progresses after queue growth repair");
}

void coalesce(state &value)
{
	block(value);
	player_snapshot first;
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		const auto &raw = value.expected.at({ target_pid, 1 });
		require(player_snapshot_decode(raw.data(), raw.size(), &first) ==
				player_snapshot_codec_result::ok,
			"original active decode");
	}
	auto second = capture(value, target_pid, full_mask, true, &first);
	auto middle = second;
	require(player_save_worker_submit_retained(&second) == player_save_submit_result::coalesced,
		"pending receipt capture admitted");
	auto third = capture(value, target_pid, full_mask, true, &middle);
	require(player_save_worker_submit_retained(&third) == player_save_submit_result::coalesced,
		"newer complete cumulative receipt capture admitted");
	finish(value);
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.calls[{ target_pid, 1 }] == 1 && value.calls[{ target_pid, 2 }] == 0 &&
			value.calls[{ target_pid, 3 }] == 1 &&
			value.completions[{ target_pid, 1 }] == 1 &&
			value.completions[{ target_pid, 3 }] == 1,
		"actual active and coalesced pending complete receipt captures each apply once");
	const auto retained = frames(value.directory, target_pid);
	require(retained.size() == 1 &&
			std::vector<uint8_t>(retained.front().begin() + 72,
					     retained.front().end()) == encoded(middle),
		"superseded receipt frame remains durable; later revision cannot fabricate exact receipt ACK");
}

void capacity(state &value, bool bytes)
{
	block(value);
	unsigned count = bytes ? 8 : PLAYER_SAVE_WORKER_MAX_PIDS - 1;
	for (unsigned index = 0; index < count; ++index)
	{
		const int pid = 63000 + static_cast<int>(index);
		require(player_revision_hydrate(pid, 0), "capacity filler hydrate");
		auto item = capture(value, pid, PLAYER_COMPONENT_STATUS, false);
		if (bytes)
		{
			item.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
			// The bound is part of the production codec's canonical bytes.
			// Seal the final admitted payload, not the earlier 8192-byte draft.
			remember(value, item);
		}
		const auto result = player_save_worker_submit_retained(&item);
		if (bytes && index == 7)
			require(result == player_save_submit_result::durably_spilled,
				"final byte filler exceeds bound after durable append");
		else
			require(result == player_save_submit_result::accepted,
				"capacity filler admitted");
	}
	auto input = capture(value, target_pid);
	if (bytes)
	{
		input.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		remember(value, input);
	}
	const auto canonical = encoded(input);
	const auto before = revision(target_pid);
	const auto health = player_save_worker_health_copy();
	const auto result = player_save_worker_submit_retained(&input);
	require(result == player_save_submit_result::durably_spilled &&
			encoded(input) == canonical &&
			same_revision(revision(target_pid), before) &&
			!player_save_worker_pid_pending(target_pid) &&
			player_save_worker_health_copy().queued_bytes == health.queued_bytes,
		"real capacity guard spills durable complete input without consuming revision or adding slot");
	const auto retained = frames(value.directory, target_pid);
	require(retained.size() == 1 && std::vector<uint8_t>(retained.front().begin() + 72,
							     retained.front().end()) == canonical,
		"capacity spill keeps actual exact durable frame");
	finish(value);
	require(healthy(value, 61901), "unrelated PID progresses after capacity owners finish");
}

void uncaptured_mark(state &value)
{
	block(value);
	auto original = capture(value, target_pid);
	const auto original_revision = original.revision;
	require(player_save_worker_submit_retained(&original) ==
			player_save_submit_result::accepted,
		"original exact capture queued behind unrelated actual apply barrier");
	player_snapshot sealed_original;
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		const auto &raw = value.expected.at({ target_pid, original_revision });
		require(player_snapshot_decode(raw.data(), raw.size(), &sealed_original) ==
				player_snapshot_codec_result::ok,
			"decode sealed original only from independent canonical oracle");
	}
	auto second = capture(value, target_pid, full_mask, true, &sealed_original);
	const auto canonical = encoded(second);
	player_revision_t marked = 0;
	require(player_revision_mark(target_pid, PLAYER_COMPONENT_LANGUAGES, &marked) &&
			marked == 3,
		"newer unrecaptured mark advances actual current revision independently");
	const auto before = revision(target_pid);
	const auto health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	const auto result = player_save_worker_submit_retained(&second);
	std::vector<uint8_t> observed;
	const bool input_exact = player_snapshot_encode(second, &observed) ==
					 player_snapshot_codec_result::ok &&
				 observed == canonical && second.encoded_size_bound == 8192;
	const auto now = player_save_worker_health_copy();
	const bool revision_exact = same_revision(revision(target_pid), before);
	const bool owner_exact = player_save_worker_pid_pending(target_pid) &&
				 now.queued_pids == health.queued_pids &&
				 now.inflight_pids == health.inflight_pids &&
				 now.queued_bytes == health.queued_bytes &&
				 now.submitted == health.submitted &&
				 now.coalesced == health.coalesced;
	std::cout << "uncaptured result=" << static_cast<unsigned>(result)
		  << " input_exact=" << input_exact << " revision_exact=" << revision_exact
		  << " original_owner_exact=" << owner_exact << '\n';
	require(result == player_save_submit_result::revision_state_mismatch && input_exact &&
			revision_exact && owner_exact,
		"newer uncaptured mark refuses before moving input or changing original active/revision/bytes");
	const auto retained = frames(value.directory, target_pid);
	require(retained.size() == disk.size() + 1 &&
			std::equal(disk.begin(), disk.end(), retained.begin()) &&
			std::vector<uint8_t>(retained.back().begin() + 72, retained.back().end()) ==
				canonical,
		"refused older capture retains real original journal and exact appended frame");
	// Capture the already-marked r3 without inventing another mark, preserving
	// all cumulative receipt payloads and the new component's real queue mask.
	auto third = capture(value, target_pid, full_mask | PLAYER_COMPONENT_LANGUAGES, true,
			     &second, false);
	require(third.revision == marked && player_save_worker_submit_retained(&third) ==
						    player_save_submit_result::coalesced,
		"fresh cumulative current capture repairs undispatched replacement");
	finish(value);
	require(value.ack_false == 0 && frames(value.directory, target_pid) == retained,
		"real repaired ACK retires only exact r3 frame and preserves unexecuted original frames");
	require(healthy(value, 61904), "unrelated PID progresses after uncaptured mark repair");
	std::lock_guard<std::mutex> lock(value.mutex);
	require(value.calls[{ target_pid, original_revision }] == 0 &&
			value.calls[{ target_pid, 2 }] == 0 &&
			value.calls[{ target_pid, marked }] == 1 &&
			value.completions[{ target_pid, marked }] == 1,
		"only repaired cumulative current capture executes once with exact completion");
}

void narrowed(state &value)
{
	// Real earlier worker ACK narrows queued components while the later
	// original journal append is durable but its submit callback is paused.
	// Old ordinary LANGUAGES is retired by real r1 ACK while the newer ordinary
	// STATUS capture waits at the real durable append boundary.
	block(value, true);
	auto input = capture(value, target_pid, PLAYER_COMPONENT_STATUS, false);
	const auto canonical = encoded(input);
	value.append_pause = true;
	value.inject_submit = true;
	value.requested = { 0, 0 };
	player_save_submit_result result = player_save_submit_result::invalid;
	std::exception_ptr error;
	bool submit_injected = false, submit_overflow = false;
	size_t submit_allocations = 0;
	std::jthread submitter(
		[&]
		{
			try
			{
				result = player_save_worker_submit_retained(&input);
				disarm();
			}
			catch (...)
			{
				disarm();
				error = std::current_exception();
			}
			submit_injected = injected;
			submit_overflow = overflow;
			submit_allocations = allocation_count;
		});
	// Ensure an oracle failure releases the real append barrier before joining
	// its caller; no fixture error can strand the native submitter.
	struct append_join_guard
	{
		state &value;
		std::jthread &thread;
		~append_join_guard()
		{
			release(value);
			if (thread.joinable())
				thread.join();
		}
	} cleanup{ value, submitter };
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.append_entered; }),
			"append boundary entered");
		value.release = true;
		value.changed.notify_all();
	}
	wait([] { return !player_save_worker_pid_pending(target_pid); });
	const auto before = revision(target_pid);
	require(before.queued_components == PLAYER_COMPONENT_STATUS && before.queued_revision == 2,
		"actual prior worker ACK narrowed the later queued component mask");
	const auto health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		value.append_release = true;
		value.changed.notify_all();
	}
	submitter.join();
	if (error)
		std::rethrow_exception(error);
	std::cout << "narrowing_allocator_hit=" << submit_injected
		  << " allocations=" << submit_allocations << '\n';
	require(submit_injected && !submit_overflow, "actual submitter allocation boundary hit");
	value.inject_submit = false;
	value.append_pause = false;
	require(result == player_save_submit_result::capacity_exceeded &&
			encoded(input) == canonical &&
			same_revision(revision(target_pid), before) &&
			!player_save_worker_pid_pending(target_pid) &&
			frames(value.directory, target_pid) == disk &&
			player_save_worker_health_copy().queued_bytes == health.queued_bytes,
		"allocation refusal after authentic ACK narrowing preserves original full capture and queued state");
	auto expected = input;
	expected.components = before.queued_components;
	remember(value, expected);
	require(player_save_worker_submit_retained(&input) == player_save_submit_result::accepted,
		"same unconsumed narrowed-mask original retries");
	// The separate receipt-bearing exact-identity contract is deliberately not
	// substituted for this ordinary admission rollback/repair check.
	finish(value);
	require(value.ack_false == 0 && frames(value.directory, target_pid).empty(),
		"ordinary repaired admission receives real successful journal ACK");
	require(healthy(value, 61902), "unrelated PID progresses after narrowed admission repair");
}
} // namespace

extern "C" void *__real__Znwm(size_t size);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (recording)
	{
		if (allocation_count < allocation_sizes.size())
			allocation_sizes[allocation_count++] = size;
		else
			overflow = true;
		if ((allocation_fault.ordinal >= 0 && allocation_fault.ordinal-- == 0) ||
		    (allocation_fault.size && allocation_fault.size == size))
		{
			injected = true;
			recording = false;
			throw std::bad_alloc();
		}
	}
	return __real__Znwm(size);
}

int main(int argc, char **argv)
{
	state value;
	try
	{
		require(argc == 3, "scenario and fresh private journal required");
		const std::string name = argv[1];
		const bool no_journal = name.starts_with("initial_none");
		const bool actor_block = name.starts_with("pending_") ||
					 name == "protected_deferred" ||
					 name == "receipt_coalesce" || name == "ack_mask_narrow";
		start(value, argv[2], !no_journal, actor_block ? target_pid : blocker_pid,
		      name == "protected_deferred");
		if (name.starts_with("initial_"))
			initial(value, std::stoi(name.substr(name.size() - 1)));
		else if (name == "pending_new")
			pending(value, "new");
		else if (name == "pending_replacement")
			pending(value, "replacement");
		else if (name == "undispatched_replacement")
			pending(value, "undispatched");
		else if (name == "undispatched_uncaptured_mark")
			uncaptured_mark(value);
		else if (name == "protected_deferred")
			pending(value, "new");
		else if (name == "ready_set_growth")
			queue_growth(value, false);
		else if (name == "ready_deque_growth")
			queue_growth(value, true);
		else if (name == "receipt_coalesce")
			coalesce(value);
		else if (name == "capacity_pids")
			capacity(value, false);
		else if (name == "capacity_bytes")
			capacity(value, true);
		else if (name == "ack_mask_narrow")
			narrowed(value);
		else if (name == "invalid_pointer")
		{
			require(player_save_worker_submit_retained(nullptr) ==
						player_save_submit_result::invalid &&
					player_save_worker_health_copy().queued_bytes == 0 &&
					player_save_journal_health_copy().records == 0,
				"invalid pointer has no revision, journal or worker ownership effects");
			require(healthy(value, 61903),
				"unrelated valid PID still progresses after invalid admission");
		}
		else
			throw std::runtime_error("unknown admission scenario");
		player_save_worker_reset_for_tests();
		player_revision_reset_for_tests();
		player_save_journal_shutdown();
		std::cout << "PASS " << name << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		disarm();
		release(value);
		player_save_worker_shutdown();
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << " ack_calls=" << value.ack_calls << " ack_false=" << value.ack_false
			  << " terminal=" << value.terminal_calls << '\n';
		return 1;
	}
}
