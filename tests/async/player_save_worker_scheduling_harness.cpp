#include "net/network_wakeup.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
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

// Real worker/revision/codec/journal/observability; controlled repository apply.
// Only the caller of pulse is faulted. Background result push is not tested.
namespace
{
constexpr int target_pid = 67101, blocker_pid = 67102;
constexpr player_component_mask_t full_mask = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS |
					      PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
thread_local bool armed = false, injected = false, overflow = false;
thread_local int fault_ordinal = -1;
thread_local size_t fault_size = 0, allocation_count = 0;
thread_local std::array<size_t, 256> allocation_sizes{};

void require(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}
void arm(int ordinal = -1, size_t size = 0)
{
	armed = true;
	injected = overflow = false;
	fault_ordinal = ordinal;
	fault_size = size;
	allocation_count = 0;
}
void disarm()
{
	armed = false;
}
std::vector<uint8_t> encode(const player_snapshot &snapshot)
{
	std::vector<uint8_t> bytes;
	require(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok,
		"fixture uses valid real canonical codec");
	return bytes;
}
player_revision_snapshot revision(int pid)
{
	player_revision_snapshot value{};
	require(player_revision_snapshot_copy(pid, &value), "actual revision copy");
	return value;
}
bool equal_revision(const player_revision_snapshot &a, const player_revision_snapshot &b)
{
	return std::tie(a.pid, a.current_revision, a.acknowledged_revision, a.queued_revision,
			a.inflight_revision, a.dirty_components, a.unacknowledged_components,
			a.queued_components, a.inflight_components, a.overflowed) ==
	       std::tie(b.pid, b.current_revision, b.acknowledged_revision, b.queued_revision,
			b.inflight_revision, b.dirty_components, b.unacknowledged_components,
			b.queued_components, b.inflight_components, b.overflowed);
}
bool equal_health(const player_save_worker_health &a, const player_save_worker_health &b)
{
	// Age/latency fields naturally advance; ownership and effect counters must not.
	return std::tie(a.queued_pids, a.inflight_pids, a.deferred_pids, a.queued_bytes,
			a.submitted, a.coalesced, a.applied, a.stale, a.retryable_failures,
			a.journal_ack_failures, a.terminal_failures, a.custody_payload_mismatches,
			a.retries_exhausted, a.high_water_pids, a.high_water_bytes,
			a.max_capture_to_apply_usec, a.max_apply_usec, a.max_ack_latency_usec,
			a.max_revision_gap) ==
	       std::tie(b.queued_pids, b.inflight_pids, b.deferred_pids, b.queued_bytes,
			b.submitted, b.coalesced, b.applied, b.stale, b.retryable_failures,
			b.journal_ack_failures, b.terminal_failures, b.custody_payload_mismatches,
			b.retries_exhausted, b.high_water_pids, b.high_water_bytes,
			b.max_capture_to_apply_usec, b.max_apply_usec, b.max_ack_latency_usec,
			b.max_revision_gap);
}
using raw_frames = std::vector<std::vector<uint8_t>>;
uint64_t little(const uint8_t *p, unsigned n)
{
	uint64_t value = 0;
	for (unsigned i = 0; i < n; ++i)
		value |= static_cast<uint64_t>(p[i]) << (i * 8);
	return value;
}
raw_frames frames(const std::string &directory, int pid)
{
	std::ifstream stream(directory + "/player-save.journal", std::ios::binary);
	require(static_cast<bool>(stream), "real journal exists");
	std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(stream),
				    std::istreambuf_iterator<char>() };
	raw_frames result;
	for (size_t offset = 0; offset < bytes.size();)
	{
		require(bytes.size() - offset >= 72, "raw frame header complete");
		const auto length = little(bytes.data() + offset + 16, 8);
		require(little(bytes.data() + offset + 12, 4) == 72 && length >= 72 &&
				length <= bytes.size() - offset,
			"raw frame length complete");
		if (little(bytes.data() + offset + 40, 4) == static_cast<uint64_t>(pid))
			result.emplace_back(bytes.begin() + offset,
					    bytes.begin() + offset + length);
		offset += length;
	}
	return result;
}

enum class receipt_kind
{
	ordinary,
	quest,
	spell,
	craft,
	mixed
};
struct state
{
	std::mutex mutex;
	std::condition_variable changed;
	std::string directory;
	bool first_entered = false, release_first = false, blocker_entered = false,
	     release_blocker = false;
	bool forever = false;
	raw_frames original_frames;
	player_save_apply_outcome first_outcome = player_save_apply_outcome::applied;
	std::map<std::pair<int, player_revision_t>, std::vector<uint8_t>> expected;
	std::map<std::pair<int, player_revision_t>, player_snapshot> sealed;
	std::map<std::pair<int, player_revision_t>, unsigned> calls;
	std::vector<player_save_completion> delivered;
	std::atomic<unsigned> ack_calls{ 0 }, ack_false{ 0 }, terminal_calls{ 0 };
};
state *current = nullptr;

void remember(state &value, const player_snapshot &snapshot)
{
	const auto bytes = encode(snapshot);
	std::lock_guard<std::mutex> lock(value.mutex);
	value.expected[{ snapshot.pid, snapshot.revision }] = bytes;
	value.sealed[{ snapshot.pid, snapshot.revision }] = snapshot;
}
player_snapshot capture(state &value, int pid, receipt_kind kind = receipt_kind::mixed,
			player_component_mask_t marked = full_mask,
			const player_snapshot *prior = nullptr)
{
	player_snapshot snapshot{};
	require(player_revision_mark(pid, marked, &snapshot.revision) &&
			player_revision_queue(pid, &snapshot.revision, &snapshot.components),
		"real capture mark/queue");
	snapshot.pid = pid;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 22800;
	snapshot.encoded_size_bound = 8192;
	snapshot.schema_version =
		kind == receipt_kind::ordinary ? PLAYER_SNAPSHOT_SCHEMA_VERSION :
		kind == receipt_kind::quest    ? PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION :
		kind == receipt_kind::spell ? PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION :
					      PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
	snapshot.status_integers.push_back({ player_status_field::level, 42, 0, false });
	snapshot.status_strings.push_back(
		{ player_status_string_field::name, "synthetic-scheduling-original" });
	if (prior)
	{
		snapshot.quest_xp_receipts = prior->quest_xp_receipts;
		snapshot.spell_effect_receipts = prior->spell_effect_receipts;
		snapshot.craft_receipts = prior->craft_receipts;
	}
	if (kind == receipt_kind::quest || kind == receipt_kind::mixed)
	{
		player_quest_xp_receipt_snapshot receipt{};
		receipt.offering_operation.bytes[0] = 0x51;
		receipt.offering_operation.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		receipt.reward_index = 3;
		receipt.amount = 29;
		snapshot.quest_xp_receipts.push_back(receipt);
	}
	if (kind == receipt_kind::spell || kind == receipt_kind::mixed)
	{
		player_spell_effect_receipt_snapshot receipt{};
		receipt.operation_id.bytes[0] = 0x52;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		receipt.effect_id = 6;
		snapshot.spell_effect_receipts.push_back(receipt);
	}
	if (kind == receipt_kind::craft || kind == receipt_kind::mixed)
	{
		player_craft_receipt_snapshot receipt{};
		receipt.operation_id.bytes[0] = 0x53;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(snapshot.revision);
		receipt.discipline = 2;
		receipt.experience = 71;
		snapshot.craft_receipts.push_back(receipt);
	}
	remember(value, snapshot);
	return snapshot;
}
player_save_apply_result apply(const player_snapshot &snapshot, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	const auto bytes = encode(snapshot);
	std::unique_lock<std::mutex> lock(value.mutex);
	require(value.expected.at({ snapshot.pid, snapshot.revision }) == bytes,
		"actual execution retains complete canonical active/pending payload");
	const unsigned count = ++value.calls[{ snapshot.pid, snapshot.revision }];
	if (snapshot.pid == target_pid && snapshot.revision == 1 && count == 1)
	{
		value.first_entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.release_first; }),
			"first actual apply barrier deadline");
	}
	if (snapshot.pid == blocker_pid)
	{
		value.blocker_entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.release_blocker; }),
			"independent actual apply barrier deadline");
	}
	if (snapshot.pid == target_pid && snapshot.revision == 1 && (count == 1 || value.forever))
	{
		const auto outcome = value.first_outcome;
		return { outcome,
			 outcome == player_save_apply_outcome::applied ? snapshot.revision : 0,
			 outcome == player_save_apply_outcome::applied ?
				 0U :
				 static_cast<unsigned int>(EIO) };
	}
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}
bool ack(const player_snapshot &snapshot, player_revision_t durable, void *raw)
{
	auto &value = *static_cast<state *>(raw);
	++value.ack_calls;
	const bool result = player_save_journal_worker_ack(snapshot, durable, nullptr);
	if (!result)
		++value.ack_false;
	return result;
}
void terminal(const player_snapshot &snapshot, void *raw) noexcept
{
	++static_cast<state *>(raw)->terminal_calls;
	player_save_journal_worker_terminal(snapshot, nullptr);
}
void start(state &value, const std::string &directory)
{
	value.directory = directory;
	current = &value;
	require(player_save_journal_init(directory.c_str()), "fresh private real journal");
	require(player_save_worker_set_journal_hooks(player_save_journal_worker_append, ack, &value,
						     terminal),
		"actual durable append/ACK/terminal hooks");
	require(player_save_worker_init(apply, &value, 1), "actual single worker init");
	require(player_revision_hydrate(target_pid, 0) && player_revision_hydrate(blocker_pid, 0),
		"baseline revisions");
}
void release(state &value)
{
	std::lock_guard<std::mutex> lock(value.mutex);
	value.release_first = value.release_blocker = true;
	value.changed.notify_all();
}
void publish()
{
	player_save_completion completed[32]{};
	const auto count = player_save_worker_pulse(completed, 32);
	for (size_t index = 0; index < count; ++index)
		current->delivered.push_back(std::move(completed[index]));
}
template <typename Predicate> void wait(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		require(std::chrono::steady_clock::now() < deadline,
			"native scheduling progress deadline");
		publish();
		std::this_thread::yield();
	}
}
void finish(state &value)
{
	release(value);
	wait(
		[]
		{
			const auto h = player_save_worker_health_copy();
			return h.queued_pids == 0 && h.inflight_pids == 0;
		});
	require(value.ack_false == 0, "actual healthy repaired ACK succeeds");
}
unsigned count_calls(state &value, player_revision_t rev)
{
	std::lock_guard<std::mutex> lock(value.mutex);
	return value.calls[{ target_pid, rev }];
}
unsigned count_delivery(state &value, player_revision_t rev, player_save_apply_outcome outcome)
{
	return static_cast<unsigned>(std::count_if(
		value.delivered.begin(), value.delivered.end(), [&](const auto &c)
		{ return c.pid == target_pid && c.revision == rev && c.outcome == outcome; }));
}
bool successful_receipts(const player_save_completion &c, const player_snapshot &s)
{
	if (c.quest_xp_receipts.size() != s.quest_xp_receipts.size() ||
	    c.spell_effect_receipts.size() != s.spell_effect_receipts.size() ||
	    c.craft_receipts.size() != s.craft_receipts.size() ||
	    !c.failed_spell_effect_receipts.empty() || !c.failed_craft_receipts.empty())
		return false;
	for (size_t i = 0; i < s.quest_xp_receipts.size(); ++i)
		if (c.quest_xp_receipts[i].offering_operation.bytes !=
			    s.quest_xp_receipts[i].offering_operation.bytes ||
		    c.quest_xp_receipts[i].reward_index != s.quest_xp_receipts[i].reward_index ||
		    c.quest_xp_receipts[i].amount != s.quest_xp_receipts[i].amount)
			return false;
	for (size_t i = 0; i < s.spell_effect_receipts.size(); ++i)
		if (c.spell_effect_receipts[i].operation_id.bytes !=
			    s.spell_effect_receipts[i].operation_id.bytes ||
		    c.spell_effect_receipts[i].effect_id != s.spell_effect_receipts[i].effect_id)
			return false;
	for (size_t i = 0; i < s.craft_receipts.size(); ++i)
		if (c.craft_receipts[i].operation_id.bytes !=
			    s.craft_receipts[i].operation_id.bytes ||
		    c.craft_receipts[i].discipline != s.craft_receipts[i].discipline ||
		    c.craft_receipts[i].experience != s.craft_receipts[i].experience)
			return false;
	return true;
}
void require_success(state &value, const player_snapshot &snapshot)
{
	require(count_delivery(value, snapshot.revision, player_save_apply_outcome::applied) == 1,
		"one original applied completion delivered");
	for (const auto &c : value.delivered)
		if (c.pid == snapshot.pid && c.revision == snapshot.revision &&
		    c.outcome == player_save_apply_outcome::applied)
			require(c.components == snapshot.components &&
					c.durable_revision == snapshot.revision &&
					successful_receipts(c, snapshot),
				"actual delivered identity carries every original successful typed receipt exactly");
}

struct growth
{
	unsigned prefix = 0;
	size_t size = 0;
};
growth probe(bool deque)
{
	// Two real dispatches precede the injected pulse: target, then blocker.
	if (deque)
	{
		std::deque<int> keys;
		keys.push_back(target_pid);
		keys.pop_front();
		keys.push_back(blocker_pid);
		keys.pop_front();
		for (unsigned index = 1; index < PLAYER_SAVE_WORKER_MAX_PIDS - 2; ++index)
		{
			arm();
			keys.push_back(static_cast<int>(index));
			disarm();
			require(!overflow, "native deque probe observation bound");
			if (allocation_count)
				return { index - 1, allocation_sizes[0] };
		}
	}
	else
	{
		std::unordered_set<int> keys;
		keys.insert(target_pid);
		keys.erase(target_pid);
		keys.insert(blocker_pid);
		keys.erase(blocker_pid);
		for (unsigned index = 1; index < PLAYER_SAVE_WORKER_MAX_PIDS - 2; ++index)
		{
			const auto buckets = keys.bucket_count();
			arm();
			keys.insert(static_cast<int>(index));
			disarm();
			require(!overflow, "native set probe observation bound");
			if (keys.bucket_count() != buckets)
				return { index - 1, *std::max_element(allocation_sizes.begin(),
								      allocation_sizes.begin() +
									      allocation_count) };
		}
	}
	throw std::runtime_error(
		"native ready-container growth unavailable within production bounds");
}

player_snapshot prepare(state &value, receipt_kind kind, bool pending, unsigned fillers,
			bool ordinary = false)
{
	auto first = capture(value, target_pid, ordinary ? receipt_kind::ordinary : kind,
			     ordinary ? PLAYER_COMPONENT_LANGUAGES : full_mask);
	auto original = first;
	require(player_save_worker_submit_retained(&first) == player_save_submit_result::accepted,
		"actual first admission");
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.first_entered; }),
			"actual target entered");
	}
	value.original_frames = frames(value.directory, target_pid);
	require(value.original_frames.size() == 1 &&
			std::vector<uint8_t>(value.original_frames.front().begin() + 72,
					     value.original_frames.front().end()) ==
				encode(original),
		"independent original full raw journal identity sealed before any apply disposition");
	if (pending)
	{
		auto second = capture(value, target_pid, ordinary ? receipt_kind::ordinary : kind,
				      ordinary ? PLAYER_COMPONENT_STATUS : full_mask,
				      ordinary ? nullptr : &original);
		require(player_save_worker_submit_retained(&second) ==
				player_save_submit_result::coalesced,
			"actual cumulative pending admission");
	}
	auto blocker = capture(value, blocker_pid, receipt_kind::ordinary, PLAYER_COMPONENT_STATUS);
	require(player_save_worker_submit_retained(&blocker) == player_save_submit_result::accepted,
		"unrelated blocker admitted");
	{
		std::unique_lock<std::mutex> lock(value.mutex);
		value.release_first = true;
		value.changed.notify_all();
		// The same worker only reaches this callback after results.push_back for
		// target. This is a completion-queue witness, not a sleep or a guessed delay.
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.blocker_entered; }),
			"target completion queued before blocker");
	}
	// Fill only after blocker has been removed from ready_set/ready_pids, so
	// the actual container history matches the two-dispatch native probe.
	for (unsigned index = 0; index < fillers; ++index)
	{
		const int pid = 68000 + static_cast<int>(index);
		require(player_revision_hydrate(pid, 0), "growth filler hydrate");
		auto filler = capture(value, pid, receipt_kind::ordinary, PLAYER_COMPONENT_STATUS);
		require(player_save_worker_submit_retained(&filler) ==
				player_save_submit_result::accepted,
			"growth filler queued");
	}
	return original;
}

void injected_scheduling(state &value, const std::string &name)
{
	const bool retry = name.starts_with("retry") || name.starts_with("ambiguous");
	value.first_outcome = name.starts_with("ambiguous") ?
				      player_save_apply_outcome::ambiguous_commit :
			      retry ? player_save_apply_outcome::retryable_failure :
				      player_save_apply_outcome::applied;
	receipt_kind kind = receipt_kind::mixed;
	if (name.find("quest") != std::string::npos)
		kind = receipt_kind::quest;
	if (name.find("spell") != std::string::npos)
		kind = receipt_kind::spell;
	if (name.find("craft") != std::string::npos)
		kind = receipt_kind::craft;
	const bool grow = name.ends_with("_set_growth") || name.ends_with("_deque_growth");
	const auto window = grow ? probe(name.ends_with("_deque_growth")) : growth{};
	const auto first = prepare(value, kind, !retry, window.prefix);
	const auto before_revision = revision(target_pid);
	const auto before_health = player_save_worker_health_copy();
	const auto disk = frames(value.directory, target_pid);
	const auto ack_before = value.ack_calls.load();
	require(disk.size() == 1 && (!retry ? ack_before == 1 : ack_before == 0),
		"promotion starts after real original ACK; retry retains its unacknowledged original frame");
	player_save_completion output{};
	output.pid = -777;
	output.revision = 999;
	size_t count = 0;
	bool escaped = false;
	arm(grow ? -1 : 0, window.size);
	try
	{
		count = player_save_worker_pulse(&output, 1);
	}
	catch (const std::bad_alloc &)
	{
		escaped = true;
	}
	disarm();
	const auto after_health = player_save_worker_health_copy();
	const bool revision_exact = equal_revision(revision(target_pid), before_revision);
	const bool health_exact = equal_health(before_health, after_health);
	const bool journal_exact = frames(value.directory, target_pid) == disk;
	std::cout << "pulse_fault hit=" << injected << " escaped=" << escaped
		  << " published=" << count << " revision_exact=" << revision_exact
		  << " health_exact=" << health_exact << " journal_exact=" << journal_exact
		  << " real_ack_before=" << ack_before << " allocation_count=" << allocation_count
		  << '\n';
	for (size_t i = 0; i < allocation_count; ++i)
		std::cout << "allocation ordinal=" << i << " size=" << allocation_sizes[i] << '\n';
	require(injected && !overflow, "requested actual scheduling allocation reached");
	require(!escaped && count == 0 && output.pid == -777 && output.revision == 999 &&
			output.quest_xp_receipts.empty() && output.spell_effect_receipts.empty() &&
			output.craft_receipts.empty() &&
			output.failed_spell_effect_receipts.empty() &&
			output.failed_craft_receipts.empty() && revision_exact && health_exact &&
			journal_exact && value.ack_calls == ack_before &&
			player_save_worker_pid_pending(target_pid) && count_calls(value, 1) == 1,
		"pulse OOM preserves original deliverable result, active/pending/revision/receipt ownership and real journal");
	// No injection on recovery: the retained real result must be published,
	// not reconstructed from revision state or fabricated receipt vectors.
	publish();
	if (retry)
		require(count_delivery(value, 1, value.first_outcome) == 1,
			"original retry/ambiguity result remains deliverable once");
	else
		require_success(value, first);
	finish(value);
	require(value.terminal_calls == 0 &&
			player_save_worker_health_copy().retries_exhausted == 0 &&
			frames(value.directory, target_pid).empty(),
		"recovered native path has exact real ACK, no exhausted/terminal frame");
	if (retry)
	{
		require(count_calls(value, 1) == 2,
			"one controlled failed apply then one same-ID exact recovery");
		require_success(value, first);
	}
	else
	{
		require(count_calls(value, 1) == 1 && count_calls(value, 2) == 1,
			"active and pending apply once each");
		player_snapshot second;
		{
			std::lock_guard<std::mutex> lock(value.mutex);
			second = value.sealed.at({ target_pid, 2 });
		}
		require_success(value, second);
	}
	require(std::count_if(value.delivered.begin(), value.delivered.end(),
			      [](const auto &c) {
				      return c.pid == blocker_pid &&
					     c.outcome == player_save_apply_outcome::applied;
			      }) == 1,
		"unrelated PID's real completion progresses after allocation repair");
	const auto delivered = value.delivered.size();
	publish();
	publish();
	require(value.delivered.size() == delivered,
		"null pulse repeats no completion or receipt notification");
}

void ordinary_control(state &value)
{
	const auto first = prepare(value, receipt_kind::ordinary, true, 0, true);
	player_snapshot second;
	{
		std::lock_guard<std::mutex> lock(value.mutex);
		second = value.sealed.at({ target_pid, 2 });
	}
	require(second.components == (PLAYER_COMPONENT_LANGUAGES | PLAYER_COMPONENT_STATUS),
		"ordinary original captured both components");
	second.components = PLAYER_COMPONENT_STATUS;
	remember(value, second);
	publish();
	require_success(value, first);
	finish(value);
	require_success(value, second);
	require(frames(value.directory, target_pid).empty() && value.terminal_calls == 0,
		"ordinary pending mask narrowing preserves real journal ACK and healthy completion");
}
void failure_control(state &value, const std::string &name)
{
	value.first_outcome =
		name == "terminal_control" ? player_save_apply_outcome::terminal_failure :
		name == "stale_control"	   ? player_save_apply_outcome::stale_revision :
					     player_save_apply_outcome::retryable_failure;
	value.forever = name == "exhaustion_control";
	const auto first = prepare(value, receipt_kind::mixed, false, 0);
	finish(value);
	const auto wanted_calls = value.forever ? PLAYER_SAVE_WORKER_MAX_RETRIES + 1 : 1;
	require(count_calls(value, 1) == wanted_calls && value.ack_calls == 1 &&
			value.ack_false == 0,
		"terminal/stale/exhaustion use original identity with no invented target ACK");
	const bool quarantined = name != "stale_control";
	require(value.terminal_calls == (quarantined ? 1U : 0U) &&
			player_save_journal_pid_quarantined(target_pid) == quarantined,
		"real failure fencing unchanged");
	if (quarantined)
	{
		const auto diagnostic = player_save_journal_diagnostic_copy(target_pid);
		require(diagnostic.available && diagnostic.pid_fence && !diagnostic.global_fence &&
				diagnostic.archived_frames == 1 &&
				frames(value.directory, target_pid).empty(),
			"terminal/exhausted request actually archived, not silently ACKed or dropped");
		std::ifstream stream(value.directory + "/player-save.journal.quarantine.archive",
				     std::ios::binary);
		const std::vector<uint8_t> archive{ std::istreambuf_iterator<char>(stream),
						    std::istreambuf_iterator<char>() };
		const auto &raw = value.original_frames.front();
		require(std::search(archive.begin(), archive.end(), raw.begin(), raw.end()) !=
					archive.end() &&
				diagnostic.archived_bytes == raw.size(),
			"genuine failure archive retains exact original raw header and payload");
	}
	else
		require(frames(value.directory, target_pid) == value.original_frames,
			"stale control preserves exact unacknowledged original raw identity");
	const auto health = player_save_worker_health_copy();
	require((name != "terminal_control" || health.terminal_failures == 1) &&
			(name != "stale_control" || health.stale == 1) &&
			(name != "exhaustion_control" || health.retries_exhausted == 1),
		"native failure accounting unchanged");
	unsigned final_receipts = 0;
	for (const auto &c : value.delivered)
		if (c.pid == target_pid && !c.failed_craft_receipts.empty())
		{
			require(c.revision == first.revision && c.components == first.components &&
					c.failed_craft_receipts.size() == 1 &&
					c.failed_spell_effect_receipts.size() == 1 &&
					c.failed_craft_receipts[0].operation_id.bytes ==
						first.craft_receipts[0].operation_id.bytes &&
					c.failed_craft_receipts[0].discipline ==
						first.craft_receipts[0].discipline &&
					c.failed_craft_receipts[0].experience ==
						first.craft_receipts[0].experience &&
					c.failed_spell_effect_receipts[0].operation_id.bytes ==
						first.spell_effect_receipts[0].operation_id.bytes &&
					c.failed_spell_effect_receipts[0].effect_id ==
						first.spell_effect_receipts[0].effect_id &&
					c.error_code == EIO &&
					(name != "exhaustion_control" ||
					 c.retry_count == PLAYER_SAVE_WORKER_MAX_RETRIES),
				"real original failed receipt identities delivered unchanged");
			++final_receipts;
		}
	require(final_receipts == 1, "final failed receipt delivery occurs once");
}
} // namespace

extern "C" void *__real__Znwm(size_t size);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (armed)
	{
		if (allocation_count < allocation_sizes.size())
			allocation_sizes[allocation_count++] = size;
		else
			overflow = true;
		if ((fault_ordinal >= 0 && fault_ordinal-- == 0) ||
		    (fault_size && fault_size == size))
		{
			injected = true;
			armed = false;
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
		start(value, argv[2]);
		if (name == "ordinary_newer_mask")
			ordinary_control(value);
		else if (name.ends_with("_control"))
			failure_control(value, name);
		else
			injected_scheduling(value, name);
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
