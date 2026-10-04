#include "net/network_wakeup.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <new>
#include <openssl/sha.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>

// Production worker/revision/codec/journal/observability, controlled apply.
// Only a worker's allocation window AFTER a successful real journal ACK is
// faulted. The real native wakeup-pipe write ends that window after enqueue.
namespace
{
constexpr int target_pid = 69901, healthy_pid = 69902, filler_pid = 70000;
constexpr auto full_mask = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS |
			   PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
thread_local bool profiling = false, armed = false, hit = false;
thread_local size_t allocation_count = 0, wanted_size = 0;
thread_local std::array<size_t, 16> sizes{};
std::atomic<bool> fail_ack_sync{ false };
std::atomic<unsigned> target_enqueues{ 0 }, target_faults{ 0 }, after_ack_allocations{ 0 },
	native_enqueues{ 0 };
int wakeup_descriptor = -1;

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}
std::vector<uint8_t> encode(const player_snapshot &snapshot)
{
	std::vector<uint8_t> result;
	require(player_snapshot_encode(snapshot, &result) == player_snapshot_codec_result::ok,
		"actual canonical codec accepts original");
	return result;
}
std::vector<uint8_t> raw(const std::string &directory)
{
	std::ifstream stream(directory + "/player-save.journal", std::ios::binary);
	require(bool(stream), "actual journal file");
	return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
}
uint64_t little(const uint8_t *bytes, unsigned count)
{
	uint64_t result = 0;
	for (unsigned i = 0; i < count; ++i)
		result |= static_cast<uint64_t>(bytes[i]) << (8 * i);
	return result;
}
bool contains(const std::vector<uint8_t> &bytes, int pid)
{
	for (size_t offset = 0; offset < bytes.size();)
	{
		require(bytes.size() - offset >= 72, "complete real frame header");
		const auto size = little(bytes.data() + offset + 16, 8);
		require(little(bytes.data() + offset + 12, 4) == 72 && size >= 72 &&
				size <= bytes.size() - offset,
			"complete real frame length");
		if (little(bytes.data() + offset + 40, 4) == static_cast<uint64_t>(pid))
			return true;
		offset += size;
	}
	return false;
}
void store(const std::string &path, const std::vector<uint8_t> &bytes)
{
	std::ofstream stream(path, std::ios::binary);
	stream.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	stream.close();
	require(bool(stream), "immutable fixture evidence written");
}
enum class kind
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
	std::map<int, std::vector<uint8_t>> expected;
	std::map<int, unsigned> calls;
	std::vector<player_save_completion> delivered;
	bool target_entered = false, release_target = false, retry = false, ack_failure = false,
	     fault = false;
	size_t growth_size = 0;
	char canonical_digest[65]{};
	std::atomic<unsigned> acknowledgements{ 0 }, successful_acknowledgements{ 0 },
		target_ack_true{ 0 }, target_ack_false{ 0 }, terminal{ 0 };
};
state *current = nullptr;

player_snapshot capture(state &value, int pid, kind type)
{
	require(player_revision_hydrate(pid, 0), "independent baseline revision");
	player_snapshot snapshot{};
	require(player_revision_mark(pid,
				     type == kind::ordinary ? PLAYER_COMPONENT_STATUS : full_mask,
				     &snapshot.revision) &&
			player_revision_queue(pid, &snapshot.revision, &snapshot.components),
		"real revision capture/queue");
	snapshot.pid = pid;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 22800;
	snapshot.encoded_size_bound = 8192;
	snapshot.schema_version =
		type == kind::ordinary ? PLAYER_SNAPSHOT_SCHEMA_VERSION :
		type == kind::quest    ? PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION :
		type == kind::spell    ? PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION :
					 PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
	snapshot.status_integers.push_back({ player_status_field::level, 42, 0, false });
	snapshot.status_strings.push_back(
		{ player_status_string_field::name, "synthetic-result-delivery" });
	if (type == kind::quest || type == kind::mixed)
	{
		player_quest_xp_receipt_snapshot receipt{};
		receipt.offering_operation.bytes[0] = 0x61;
		receipt.offering_operation.bytes[1] = static_cast<uint8_t>(pid);
		receipt.offering_operation.bytes[2] = static_cast<uint8_t>(pid >> 8);
		receipt.reward_index = 3;
		receipt.amount = 29;
		snapshot.quest_xp_receipts.push_back(receipt);
	}
	if (type == kind::spell || type == kind::mixed)
	{
		player_spell_effect_receipt_snapshot receipt{};
		receipt.operation_id.bytes[0] = 0x62;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(pid);
		receipt.operation_id.bytes[2] = static_cast<uint8_t>(pid >> 8);
		receipt.effect_id = 6;
		snapshot.spell_effect_receipts.push_back(receipt);
	}
	if (type == kind::craft || type == kind::mixed)
	{
		player_craft_receipt_snapshot receipt{};
		receipt.operation_id.bytes[0] = 0x63;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(pid);
		receipt.operation_id.bytes[2] = static_cast<uint8_t>(pid >> 8);
		receipt.discipline = 2;
		receipt.experience = 71;
		snapshot.craft_receipts.push_back(receipt);
	}
	{
		std::lock_guard lock(value.mutex);
		value.expected.emplace(pid, encode(snapshot));
	}
	return snapshot;
}
player_save_apply_result apply(const player_snapshot &snapshot, void *context)
{
	auto &value = *static_cast<state *>(context);
	const auto bytes = encode(snapshot);
	std::unique_lock lock(value.mutex);
	require(value.expected.at(snapshot.pid) == bytes,
		"actual apply preserves full original canonical identity");
	const auto count = ++value.calls[snapshot.pid];
	if (snapshot.pid == target_pid && count == 1)
	{
		value.target_entered = true;
		value.changed.notify_all();
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.release_target; }),
			"target pre-ACK barrier");
		if (value.retry)
			return { player_save_apply_outcome::retryable_failure, 0,
				 static_cast<unsigned>(EIO) };
	}
	return { count == 1 ? player_save_apply_outcome::applied :
			      player_save_apply_outcome::already_applied,
		 snapshot.revision, 0 };
}
extern "C" ssize_t __real_write(int, const void *, size_t);
bool ack(const player_snapshot &snapshot, player_revision_t durable, void *context)
{
	auto &value = *static_cast<state *>(context);
	const auto bytes = encode(snapshot);
	{
		std::lock_guard lock(value.mutex);
		require(bytes == value.expected.at(snapshot.pid),
			"actual journal ACK sees full original payload");
	}
	++value.acknowledgements;
	const bool refuse = snapshot.pid == target_pid && value.ack_failure &&
			    value.target_ack_false == 0;
	fail_ack_sync = refuse;
	const bool result = player_save_journal_worker_ack(snapshot, durable, nullptr);
	fail_ack_sync = false;
	if (result)
		++value.successful_acknowledgements;
	if (snapshot.pid != target_pid)
		return result;
	if (!result)
	{
		++value.target_ack_false;
		require(contains(raw(value.directory), target_pid),
			"real failed ACK retains original raw frame");
		return false;
	}
	++value.target_ack_true;
	require(!contains(raw(value.directory), target_pid),
		"successful real ACK physically removes original frame");
	char marker[192];
	const int count = std::snprintf(marker, sizeof(marker),
					"REAL_ACK_EXACT pid=%d revision=%llu canonical_sha256=%s\n",
					snapshot.pid,
					static_cast<unsigned long long>(snapshot.revision),
					value.canonical_digest);
	require(count > 0 && static_cast<size_t>(count) < sizeof(marker),
		"bounded exact ACK marker");
	require(__real_write(STDOUT_FILENO, marker, count) == count,
		"actual ACK evidence recorded before fault");
	allocation_count = 0;
	hit = false;
	wanted_size = value.growth_size;
	armed = value.fault;
	return true;
}
void terminal(const player_snapshot &snapshot, void *context) noexcept
{
	++static_cast<state *>(context)->terminal;
	player_save_journal_worker_terminal(snapshot, nullptr);
}
struct growth
{
	unsigned prefix = 0;
	size_t size = 0;
};
growth probe(bool map)
{
	// Real STL container/type/library, not a copy of worker scheduling code.
	profiling = true;
	allocation_count = 0;
	std::deque<player_save_completion> queue;
	profiling = false;
	require(allocation_count == 2, "native deque constructor allocation calibration");
	const auto block = sizes[1];
	for (unsigned index = 1; index < PLAYER_SAVE_WORKER_MAX_RESULTS; ++index)
	{
		allocation_count = 0;
		profiling = true;
		const player_save_completion completion{};
		queue.push_back(completion);
		profiling = false;
		require(allocation_count <= sizes.size(), "bounded native allocation observations");
		for (size_t ordinal = 0; ordinal < allocation_count; ++ordinal)
			if (map ? sizes[ordinal] != block : sizes[ordinal] == block)
				return { index - 1, sizes[ordinal] };
	}
	throw std::runtime_error("native growth unavailable below MAX_RESULTS; unqualified setup");
}
void pulse(state &value, size_t maximum = 32)
{
	std::array<player_save_completion, PLAYER_SAVE_WORKER_MAX_RESULTS> completed{};
	const auto count = player_save_worker_pulse(completed.data(), maximum);
	for (size_t i = 0; i < count; ++i)
		value.delivered.push_back(std::move(completed[i]));
}
template <class Predicate> void wait(Predicate predicate, bool publish = false)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		require(std::chrono::steady_clock::now() < deadline,
			"bounded actual worker progress");
		if (publish)
			pulse(*current);
		std::this_thread::yield();
	}
}
void receipt_check(const player_save_completion &completion, const player_snapshot &snapshot)
{
	require(completion.quest_xp_receipts.size() == snapshot.quest_xp_receipts.size() &&
			completion.spell_effect_receipts.size() ==
				snapshot.spell_effect_receipts.size() &&
			completion.craft_receipts.size() == snapshot.craft_receipts.size() &&
			completion.failed_spell_effect_receipts.empty() &&
			completion.failed_craft_receipts.empty(),
		"only original successful typed receipts delivered");
	for (size_t i = 0; i < snapshot.quest_xp_receipts.size(); ++i)
	{
		const auto &a = completion.quest_xp_receipts[i], &b = snapshot.quest_xp_receipts[i];
		require(a.offering_operation.bytes == b.offering_operation.bytes &&
				a.reward_index == b.reward_index && a.amount == b.amount,
			"original quest identity and terms delivered once");
	}
	for (size_t i = 0; i < snapshot.spell_effect_receipts.size(); ++i)
	{
		const auto &a = completion.spell_effect_receipts[i],
			   &b = snapshot.spell_effect_receipts[i];
		require(a.operation_id.bytes == b.operation_id.bytes && a.effect_id == b.effect_id,
			"original spell identity/terms");
	}
	for (size_t i = 0; i < snapshot.craft_receipts.size(); ++i)
	{
		const auto &a = completion.craft_receipts[i], &b = snapshot.craft_receipts[i];
		require(a.operation_id.bytes == b.operation_id.bytes &&
				a.discipline == b.discipline && a.experience == b.experience,
			"original craft identity/terms");
	}
}
void run(state &value, const std::string &name)
{
	value.fault = name.ends_with("_block") || name == "mixed_map";
	value.retry = name == "retry_control";
	value.ack_failure = name == "ack_failure_control";
	const auto window = value.fault ? probe(name == "mixed_map") : growth{};
	value.growth_size = window.size;
	std::cout << "CALIBRATED prefix=" << window.prefix << " allocation_size=" << window.size
		  << '\n'
		  << std::flush;
	require(window.prefix + 1 < PLAYER_SAVE_WORKER_MAX_RESULTS,
		"non-full bounded result queue");
	require(player_save_journal_init(value.directory.c_str()) &&
			player_save_worker_set_journal_hooks(player_save_journal_worker_append, ack,
							     &value, terminal) &&
			player_save_worker_init(apply, &value, 1),
		"real journal/worker lifecycle");
	for (unsigned i = 0; i < window.prefix; ++i)
	{
		auto snapshot = capture(value, filler_pid + i, kind::ordinary);
		require(player_save_worker_submit_retained(&snapshot) ==
				player_save_submit_result::accepted,
			"native backlog admission");
		wait([&] { return native_enqueues == i + 1; });
		require(value.successful_acknowledgements == i + 1,
			"every calibrated prefix entry had a successful real ACK and actual native enqueue");
	}
	require(native_enqueues == window.prefix &&
			value.successful_acknowledgements == window.prefix &&
			value.delivered.empty(),
		"exact calibrated native prefix retained before target admission without pulse");
	std::cout << "PREFIX_RETAINED enqueues=" << native_enqueues
		  << " real_ack_successes=" << value.successful_acknowledgements
		  << " delivered=" << value.delivered.size() << std::endl;
	const auto type = name == "quest_block" ? kind::quest :
			  name == "spell_block" ? kind::spell :
			  name == "craft_block" ? kind::craft :
						  kind::mixed;
	auto snapshot = capture(value, target_pid, type);
	const auto original = snapshot;
	const auto payload = encode(original);
	unsigned char digest[SHA256_DIGEST_LENGTH];
	require(SHA256(payload.data(), payload.size(), digest) != nullptr,
		"real canonical SHA256 evidence");
	for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		std::snprintf(value.canonical_digest + i * 2, 3, "%02x", digest[i]);
	store(value.directory + "/original-payload.bin", payload);
	require(player_save_worker_submit_retained(&snapshot) ==
			player_save_submit_result::accepted,
		"original typed admission");
	{
		std::unique_lock lock(value.mutex);
		require(value.changed.wait_for(lock, std::chrono::seconds(10),
					       [&] { return value.target_entered; }),
			"actual original apply entered");
	}
	const auto journal = raw(value.directory);
	require(contains(journal, target_pid), "original raw durable frame exists before ACK");
	store(value.directory + "/original-journal.bin", journal);
	player_revision_snapshot prior{};
	require(player_revision_snapshot_copy(target_pid, &prior) &&
			prior.inflight_revision == original.revision &&
			prior.acknowledged_revision == 0,
		"real player revision remains unacknowledged before result delivery");
	{
		std::lock_guard lock(value.mutex);
		value.release_target = true;
		value.changed.notify_all();
	}
	if (!value.retry && !value.ack_failure)
	{
		wait([&] { return value.target_ack_true == 1; });
		player_revision_snapshot retained{};
		require(player_revision_snapshot_copy(target_pid, &retained) &&
				retained.inflight_revision == original.revision &&
				retained.acknowledged_revision == 0 &&
				player_save_worker_health_copy().queued_bytes >=
					original.encoded_size_bound,
			"real ACK retains original player owner until delivery");
	}
	if (value.fault)
	{
		// Preserve the probe's no-pop history until actual target insertion.
		// ACK happens before insertion; starting pulse at that earlier marker
		// can pop prefix nodes and eliminate the measured map allocation.
		wait([&] { return target_enqueues == 1; });
		require(native_enqueues == window.prefix + 1 && value.delivered.empty(),
			"complete calibrated backlog retained until actual target enqueue");
		std::cout << "RESULT_WINDOW retained_prefix=" << window.prefix
			  << " native_enqueues=" << native_enqueues
			  << " postack_allocations=" << after_ack_allocations
			  << " fault_hits=" << target_faults << std::endl;
	}
	if (name == "shutdown_reopen_control")
	{
		wait([&] { return target_enqueues == 1; });
		player_save_worker_shutdown();
		require(value.target_ack_true == 1 && !contains(raw(value.directory), target_pid),
			"non-full shutdown retains real ACK/result");
		player_save_journal_shutdown();
		require(player_save_journal_init(value.directory.c_str()), "actual journal reopen");
	}
	wait(
		[&]
		{
			const auto health = player_save_worker_health_copy();
			return health.queued_pids == 0 && health.inflight_pids == 0;
		},
		true);
	if (value.fault)
	{
		require(value.delivered.size() == window.prefix + 1,
			"calibrated prefix and target each delivered exactly once");
		for (unsigned i = 0; i < window.prefix; ++i)
		{
			const auto &completion = value.delivered[i];
			require(completion.pid == filler_pid + static_cast<int>(i) &&
					completion.revision == 1 &&
					completion.components == PLAYER_COMPONENT_STATUS &&
					completion.outcome == player_save_apply_outcome::applied &&
					completion.durable_revision == 1 &&
					completion.error_code == 0 &&
					completion.quest_xp_receipts.empty() &&
					completion.spell_effect_receipts.empty() &&
					completion.craft_receipts.empty(),
				"real calibrated native prefix retains FIFO PID/revision/receipt identity");
		}
		require(value.delivered[window.prefix].pid == target_pid,
			"original target follows retained native prefix in real FIFO order");
	}
	unsigned success = 0, retry = 0;
	for (const auto &completion : value.delivered)
		if (completion.pid == target_pid)
		{
			require(completion.revision == original.revision &&
					completion.components == original.components,
				"same original completion revision/mask");
			if (completion.outcome == player_save_apply_outcome::applied ||
			    completion.outcome == player_save_apply_outcome::already_applied)
			{
				++success;
				require(completion.durable_revision == original.revision &&
						completion.error_code == 0,
					"exact durable completion");
				receipt_check(completion, original);
			}
			else
			{
				++retry;
				require(completion.outcome ==
							player_save_apply_outcome::retryable_failure &&
						completion.error_code == EIO &&
						completion.quest_xp_receipts.empty() &&
						completion.spell_effect_receipts.empty() &&
						completion.craft_receipts.empty(),
					"retry has no invented successful receipt");
			}
		}
	require(success == 1 && retry == (value.retry || value.ack_failure ? 1U : 0U) &&
			value.target_ack_true == 1 &&
			value.target_ack_false == (value.ack_failure ? 1U : 0U) &&
			value.terminal == 0,
		"original success/real ACK/typed delivery exactly once");
	player_revision_snapshot after{};
	require(player_revision_snapshot_copy(target_pid, &after) &&
			after.acknowledged_revision == original.revision &&
			after.inflight_revision == 0,
		"actual revision ACK after original receipt delivered");
	{
		std::lock_guard lock(value.mutex);
		require(value.calls.at(target_pid) == (value.retry || value.ack_failure ? 2U : 1U),
			"no extra reapply from result allocation failure");
	}
	require(!value.fault || target_faults == 1 || after_ack_allocations == 0,
		"handled calibrated fault or proven allocation-free postACK enqueue");
	const auto previous = value.delivered.size();
	pulse(value);
	pulse(value);
	require(previous == value.delivered.size(),
		"null pulses cannot duplicate original delivery");
	if (name != "shutdown_reopen_control")
	{
		auto healthy = capture(value, healthy_pid, kind::ordinary);
		require(player_save_worker_submit_retained(&healthy) ==
				player_save_submit_result::accepted,
			"unrelated PID remains admitted");
		wait(
			[&]
			{
				const auto health = player_save_worker_health_copy();
				return health.inflight_pids == 0 && health.queued_pids == 0;
			},
			true);
		require(std::count_if(value.delivered.begin(), value.delivered.end(),
				      [](const auto &c) {
					      return c.pid == healthy_pid &&
						     c.outcome ==
							     player_save_apply_outcome::applied;
				      }) == 1,
			"unrelated PID actually progresses after original delivery");
	}
	require(raw(value.directory).empty(), "real original and unrelated journal frames ACKed");
	std::cout << "RESULT_DELIVERY success=" << success << " retry=" << retry
		  << " real_ack_true=" << value.target_ack_true
		  << " real_ack_false=" << value.target_ack_false
		  << " calibrated_faults=" << target_faults
		  << " postack_allocations=" << after_ack_allocations << '\n';
}
void indexing_control(state &value, const std::string &name)
{
	require(player_save_journal_init(value.directory.c_str()) &&
			player_save_worker_set_journal_hooks(player_save_journal_worker_append, ack,
							     &value, terminal) &&
			player_save_worker_init(apply, &value, 1),
		"native indexing lifecycle");
	std::vector<player_snapshot> originals;
	const auto submit = [&](unsigned count)
	{
		for (unsigned index = 0; index < count; ++index)
		{
			const int pid = filler_pid + 1000 + static_cast<int>(originals.size());
			auto snapshot = capture(value, pid, kind::mixed);
			originals.push_back(snapshot);
			require(player_save_worker_submit_retained(&snapshot) ==
					player_save_submit_result::accepted,
				"bounded distinct-PID typed admission");
		}
		wait([&] { return value.acknowledgements == originals.size(); });
	};
	const auto drain = [&](size_t count)
	{
		const size_t target = value.delivered.size() + count;
		wait(
			[&]
			{
				if (value.delivered.size() < target)
					pulse(value, std::min<size_t>(
							     32, target - value.delivered.size()));
				return value.delivered.size() == target;
			});
	};
	if (name == "full_shutdown_control")
	{
		submit(PLAYER_SAVE_WORKER_MAX_RESULTS);
		player_save_worker_shutdown();
		const auto retained = player_save_worker_health_copy();
		require(retained.inflight_pids == PLAYER_SAVE_WORKER_MAX_RESULTS &&
				retained.queued_bytes == PLAYER_SAVE_WORKER_MAX_RESULTS * 8192 &&
				raw(value.directory).empty() && value.delivered.empty(),
			"exactly capacity real ACKed results retain every owner through shutdown");
		drain(PLAYER_SAVE_WORKER_MAX_RESULTS);
	}
	else if (name == "partial_wrap_control")
	{
		submit(192);
		drain(64);
		submit(64);
		drain(96);
		submit(128);
		drain(128);
		submit(64);
		drain(160);
	}
	else
	{
		for (unsigned round = 0; round < 4; ++round)
		{
			submit(96);
			drain(96);
		}
	}
	require(value.delivered.size() == originals.size() &&
			(name == "full_shutdown_control" ||
			 originals.size() > PLAYER_SAVE_WORKER_MAX_RESULTS),
		"cumulative wrap or exact capacity retention actually exercised");
	for (size_t index = 0; index < originals.size(); ++index)
	{
		const auto &original = originals[index];
		const auto &completion = value.delivered[index];
		require(completion.pid == original.pid &&
				completion.revision == original.revision &&
				completion.components == original.components &&
				completion.outcome == player_save_apply_outcome::applied &&
				completion.durable_revision == original.revision &&
				completion.error_code == 0,
			"single-worker FIFO preserves every original PID/revision/result");
		receipt_check(completion, original);
		player_revision_snapshot revision{};
		require(player_revision_snapshot_copy(original.pid, &revision) &&
				revision.acknowledged_revision == original.revision &&
				revision.inflight_revision == 0,
			"every original revision acknowledged by actual pulse");
		std::lock_guard lock(value.mutex);
		require(value.calls.at(original.pid) == 1,
			"indexing never duplicates actual apply");
	}
	const auto settled = player_save_worker_health_copy();
	require(settled.inflight_pids == 0 && settled.queued_pids == 0 &&
			settled.queued_bytes == 0 && settled.applied == originals.size() &&
			value.terminal == 0 && raw(value.directory).empty(),
		"native completion accounting and actual journal fully settled");
	const auto delivered = value.delivered.size();
	pulse(value);
	pulse(value);
	require(value.delivered.size() == delivered,
		"null pulses cannot redeliver indexed receipts");
	player_save_worker_reset_for_tests();
	player_save_journal_shutdown();
	require(player_save_journal_init(value.directory.c_str()) &&
			player_save_journal_replay(apply, &value) ==
				player_save_journal_result::ok &&
			raw(value.directory).empty(),
		"actual empty journal reopen/replay after native ACKs");
	std::cout << "INDEXING deliveries=" << delivered << " real_acks=" << value.acknowledgements
		  << " capacity=" << PLAYER_SAVE_WORKER_MAX_RESULTS << '\n';
}
} // namespace

extern "C" void *__real__Znwm(size_t);
extern "C" int __real_fdatasync(int);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (profiling)
	{
		if (allocation_count < sizes.size())
			sizes[allocation_count] = size;
		++allocation_count;
	}
	if (armed)
	{
		++allocation_count;
		++after_ack_allocations;
		if (size == wanted_size)
		{
			armed = false;
			hit = true;
			++target_faults;
			char marker[160];
			const int length = std::snprintf(
				marker, sizeof(marker),
				"RESULT_ALLOCATION_FAULT after_real_ack=1 worker_thread=1 allocation_size=%zu ordinal=%zu\n",
				size, allocation_count);
			if (length > 0 && static_cast<size_t>(length) < sizeof(marker))
				(void)__real_write(STDOUT_FILENO, marker, length);
			throw std::bad_alloc();
		}
	}
	return __real__Znwm(size);
}
extern "C" ssize_t __wrap_write(int descriptor, const void *bytes, size_t size)
{
	if (descriptor == wakeup_descriptor && size == 1 && current)
	{
		++native_enqueues;
		// Only target ACK enables this TLS window; fillers/controls remain off.
		if (armed || hit)
		{
			armed = false;
			hit = false;
			++target_enqueues;
		}
		else if (current->target_ack_true > 0 || current->target_ack_false > 0 ||
			 current->retry)
			++target_enqueues;
	}
	return __real_write(descriptor, bytes, size);
}
extern "C" int __wrap_fdatasync(int descriptor)
{
	if (fail_ack_sync)
	{
		errno = ENOSPC;
		return -1;
	}
	return __real_fdatasync(descriptor);
}
int main(int argc, char **argv)
{
	state value;
	current = &value;
	try
	{
		require(argc == 3, "known case and fresh private journal path required");
		const std::string name = argv[1];
		value.directory = argv[2];
		const std::array<std::string, 12> cases = {
			"quest_block",	       "spell_block",	       "craft_block",
			"mixed_map",	       "unrelated_block",      "retry_control",
			"ack_failure_control", "healthy_control",      "shutdown_reopen_control",
			"wraparound_control",  "partial_wrap_control", "full_shutdown_control"
		};
		require(std::find(cases.begin(), cases.end(), name) != cases.end(),
			"known scenario only");
		(void)network_wakeup_fd();
		wakeup_descriptor = network_wakeup_channel().descriptors[1];
		if (name == "wraparound_control" || name == "partial_wrap_control" ||
		    name == "full_shutdown_control")
			indexing_control(value, name);
		else
			run(value, name);
		player_save_worker_reset_for_tests();
		player_revision_reset_for_tests();
		player_save_journal_shutdown();
		std::cout << "PASS " << name << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		armed = profiling = false;
		fail_ack_sync = false;
		{
			std::lock_guard lock(value.mutex);
			value.release_target = true;
			value.changed.notify_all();
		}
		player_save_worker_shutdown();
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << '\n';
		return 1;
	}
}
