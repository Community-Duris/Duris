#include "classes/necromancy.h"
#include "core/files.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

// Real journal, codec and observability; repository application is controlled.
// No worker, database, Redis or game service is invoked by this component.
namespace
{
constexpr int held_pid = 61500;
constexpr int before_pid = held_pid - 1, after_pid = held_pid + 1;
// Appended public result contract. This numeric comparison compiles against the
// immutable BEFORE header as well as the AFTER header; the apply outcome itself
// is the actual existing production deferred enumerator in both builds.
constexpr auto deferred_result = static_cast<player_save_journal_result>(10);
thread_local int allocation_countdown = -1;
thread_local bool allocation_failed = false;
std::atomic<bool> fail_checkpoint_sync{ false };
std::atomic<bool> fail_directory_sync{ false };
std::atomic<unsigned> directory_failures{ 0 }, directory_successes{ 0 };

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

std::vector<uint8_t> encoded(const player_snapshot &snapshot)
{
	std::vector<uint8_t> result;
	require(player_snapshot_encode(snapshot, &result) == player_snapshot_codec_result::ok,
		"typed fixture must pass actual codec validation");
	return result;
}

player_snapshot snapshot(int pid, player_revision_t revision, const std::string &kind)
{
	player_snapshot result = {};
	result.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	result.pid = pid;
	result.revision = revision;
	result.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	result.save_intent = 4;
	result.room_vnum = 22800;
	result.encoded_size_bound = 16384;
	result.status_integers.push_back({ player_status_field::level, 42, 0, false });
	result.status_strings.push_back({ player_status_string_field::name, "synthetic-journal" });
	if (kind == "quest")
	{
		result.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
		player_quest_xp_receipt_snapshot receipt = {};
		receipt.offering_operation.bytes[0] = 0x41;
		receipt.offering_operation.bytes[1] = static_cast<uint8_t>(revision);
		receipt.reward_index = 4;
		receipt.amount = 19;
		result.quest_xp_receipts.push_back(receipt);
	}
	else if (kind == "spell")
	{
		result.schema_version = PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
		player_spell_effect_receipt_snapshot receipt = {};
		receipt.operation_id.bytes[0] = 0x42;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(revision);
		receipt.effect_id = 6;
		result.spell_effect_receipts.push_back(receipt);
	}
	else if (kind == "craft")
	{
		result.schema_version = PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
		player_craft_receipt_snapshot receipt = {};
		receipt.operation_id.bytes[0] = 0x43;
		receipt.operation_id.bytes[1] = static_cast<uint8_t>(revision);
		receipt.discipline = 2;
		receipt.experience = 71;
		result.craft_receipts.push_back(receipt);
	}
	else if (kind == "death")
	{
		result.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
		result.save_intent = RENT_DEATH;
		result.death.emplace();
		auto &death = *result.death;
		death.operation_id.bytes[0] = 0x44;
		death.operation_id.bytes[1] = static_cast<uint8_t>(revision);
		death.corpse_room_vnum = 22800;
		death.wallet_revision = 1;
		player_item_snapshot corpse = {};
		corpse.object_uid = 991000 + revision;
		corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		corpse.vnum = VOBJ_CORPSE;
		corpse.type = ITEM_CORPSE;
		corpse.values[CORPSE_PID] = pid;
		corpse.values[CORPSE_SAVEID] = 7;
		corpse.values[CORPSE_FLAGS] = PC_CORPSE;
		corpse.name = "corpse synthetic-journal";
		death.corpse.push_back(corpse);
	}
	else
		require(kind == "ordinary", "known synthetic schema kind");
	(void)encoded(result);
	return result;
}

uint64_t read_le(const uint8_t *bytes, unsigned count)
{
	uint64_t result = 0;
	for (unsigned index = 0; index < count; ++index)
		result |= static_cast<uint64_t>(bytes[index]) << (index * 8);
	return result;
}

using raw_frames = std::vector<std::vector<uint8_t>>;
raw_frames frames(const std::string &directory, int pid)
{
	std::ifstream input(directory + "/player-save.journal", std::ios::binary);
	require(static_cast<bool>(input), "actual journal file exists");
	const std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(input),
					  std::istreambuf_iterator<char>() };
	raw_frames result;
	for (size_t offset = 0; offset < bytes.size();)
	{
		require(bytes.size() - offset >= 72, "raw frame header bound");
		const uint64_t size = read_le(bytes.data() + offset + 16, 8);
		require(read_le(bytes.data() + offset + 12, 4) == 72 && size >= 72 &&
				size <= bytes.size() - offset,
			"raw frame size bound");
		if (read_le(bytes.data() + offset + 40, 4) == static_cast<uint64_t>(pid))
			result.emplace_back(bytes.begin() + offset, bytes.begin() + offset + size);
		offset += size;
	}
	return result;
}

struct replay_state
{
	bool held = true;
	player_revision_t defer_at = 1;
	bool earlier_ordinary_newer = false;
	bool append_during_replay = false;
	bool appended = false;
	int second_held_pid = 0;
	std::map<int, player_save_apply_outcome> failures;
	bool throw_after_pid = false;
	player_save_apply_outcome control = player_save_apply_outcome::deferred;
	player_snapshot concurrent_frame = {};
	std::map<int, std::vector<player_revision_t>> calls;
	std::map<std::pair<int, player_revision_t>, std::vector<uint8_t>> expected;
};

player_save_apply_result apply(const player_snapshot &request, void *raw)
{
	// Allocation sweep stops at actual callback entry. Later repository/proof
	// map allocation behavior is outside the explicitly qualified scan boundary.
	allocation_countdown = -1;
	auto &state = *static_cast<replay_state *>(raw);
	state.calls[request.pid].push_back(request.revision);
	const auto expected = state.expected.find({ request.pid, request.revision });
	require(expected != state.expected.end() && encoded(request) == expected->second,
		"actual replay request preserves complete encoded receipt/corpse identity");
	if (request.pid == after_pid && state.throw_after_pid)
		throw std::runtime_error("controlled repository exception");
	if (const auto failure = state.failures.find(request.pid); failure != state.failures.end())
		return { failure->second, 0, 1205 };
	if (request.pid == state.second_held_pid && state.held && request.revision == 2)
		return { player_save_apply_outcome::deferred, 0, 0 };
	if (request.pid == after_pid && state.append_during_replay && !state.appended)
	{
		player_save_journal_result appended = player_save_journal_result::io_failure;
		std::thread producer(
			[&] { appended = player_save_journal_append(state.concurrent_frame); });
		producer.join();
		require(appended == player_save_journal_result::ok,
			"actual concurrent held-PID append outside replay lock");
		state.appended = true;
	}
	if (request.pid == held_pid && state.held && request.revision == state.defer_at)
		return { state.control, 0,
			 state.control == player_save_apply_outcome::deferred ? 0U : 1205U };
	const auto durable = request.pid == held_pid && state.earlier_ordinary_newer &&
					     request.revision < state.defer_at ?
				     99 :
				     request.revision;
	return { player_save_apply_outcome::applied, durable, 0 };
}

void append(replay_state &state, const player_snapshot &request)
{
	state.expected[{ request.pid, request.revision }] = encoded(request);
	require(player_save_journal_append(request) == player_save_journal_result::ok,
		"actual typed journal append");
}

void populate(replay_state &state, const std::string &first_kind, bool late)
{
	append(state, snapshot(before_pid, 1, "ordinary"));
	append(state, snapshot(held_pid, 1, first_kind));
	if (late)
	{
		state.defer_at = 2;
		append(state, snapshot(held_pid, 2, "ordinary"));
	}
	player_revision_t revision = late ? 3 : 2;
	for (const std::string kind : { "ordinary", "death", "quest", "spell", "craft" })
		append(state, snapshot(held_pid, revision++, kind));
	// Later duplicate identities are retained as independent raw records. A
	// parked PID must be skipped before duplicate detection, not counted twice.
	append(state, snapshot(held_pid, late ? 3 : 1, late ? "ordinary" : first_kind));
	append(state, snapshot(after_pid, 1, "ordinary"));
}

void assert_held(const replay_state &state, const std::string &directory,
		 const raw_frames &original, bool late)
{
	require(!player_save_journal_pid_quarantined(held_pid), "deferred PID never quarantined");
	const auto health = player_save_journal_health_copy();
	require(health.backpressure == 0 && health.quarantined_bytes == 0 && health.duplicates == 0,
		"deferred replay neither backpressure nor duplicate processing");
	require(frames(directory, held_pid) == original,
		"all raw held-PID frames remain exact, including earlier successful proof");
	require(frames(directory, before_pid).empty() && frames(directory, after_pid).empty(),
		"unaffected PIDs receive actual journal checkpoint");
	const auto &calls = state.calls.at(held_pid);
	require(calls.size() == (late ? 2U : 1U), "same-PID later schemas skip apply callback");
	require(calls.front() == 1 && calls.back() == state.defer_at,
		"defer exact original revision");
}

void deferred_pass(const std::string &directory, const std::string &kind, bool late, bool reopen,
		   bool concurrent, bool checkpoint_failure)
{
	replay_state state;
	populate(state, kind, late);
	state.earlier_ordinary_newer = late && kind == "ordinary";
	state.append_during_replay = concurrent;
	state.concurrent_frame = snapshot(held_pid, 101, "quest");
	state.expected[{ held_pid, 101 }] = encoded(state.concurrent_frame);
	auto original = frames(directory, held_pid);
	const auto before = frames(directory, before_pid), after = frames(directory, after_pid);
	if (checkpoint_failure)
		fail_checkpoint_sync = true;
	const auto result = player_save_journal_replay(apply, &state);
	fail_checkpoint_sync = false;
	std::cout << "pass_result=" << static_cast<unsigned>(result)
		  << " held_quarantined=" << player_save_journal_pid_quarantined(held_pid)
		  << " held_callbacks=" << state.calls[held_pid].size() << " healthy_callbacks="
		  << state.calls[before_pid].size() + state.calls[after_pid].size()
		  << " original_held_frames=" << original.size()
		  << " active_held_frames=" << frames(directory, held_pid).size() << '\n';
	if (checkpoint_failure)
	{
		require(result == player_save_journal_result::io_failure,
			"real checkpoint sync failure reports error before deferred readiness");
		require(frames(directory, held_pid) == original &&
				frames(directory, before_pid) == before &&
				frames(directory, after_pid) == after,
			"failed pre-rename checkpoint preserves exact frames for every PID");
		require(!player_save_journal_pid_quarantined(held_pid),
			"checkpoint failure does not quarantine deferred PID");
		state.calls.clear();
		require(player_save_journal_replay(apply, &state) == deferred_result,
			"checkpoint repair still reports unresolved PID deferral");
	}
	else
		require(result == deferred_result, "partial replay must report replay_deferred=10");
	if (concurrent)
	{
		require(state.appended, "concurrent append branch actually executed");
		const auto appended = frames(directory, held_pid);
		require(appended.size() == original.size() + 1 &&
				std::equal(original.begin(), original.end(), appended.begin()),
			"compaction preserves original frames and concurrent append");
		original = appended;
	}
	assert_held(state, directory, original, late);
	if (reopen)
	{
		player_save_journal_shutdown();
		require(player_save_journal_init(directory.c_str()), "cold actual journal reopen");
		require(frames(directory, held_pid) == original,
			"reopen retains exact deferred frames");
	}
	state.calls.clear();
	require(player_save_journal_replay(apply, &state) == deferred_result,
		"second held replay pass remains explicitly deferred");
	require(frames(directory, held_pid) == original &&
			!player_save_journal_pid_quarantined(held_pid),
		"repeated pass retains same exact unresolved frames");
	// This is an explicit controlled repository release, not a critical ACK,
	// revision-only proof, restored-PID admission, or safe stale-frame disposition.
	state.held = false;
	state.earlier_ordinary_newer = false;
	state.calls.clear();
	require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok,
		"controlled exact repository success drains held frames");
	require(player_save_journal_health_copy().records == 0,
		"only exact successful replay proofs checkpoint held ordinary and receipt frames");
}

void control(const std::string &directory, player_save_apply_outcome outcome)
{
	replay_state state;
	state.control = outcome;
	append(state, snapshot(held_pid, 1, "quest"));
	append(state, snapshot(after_pid, 1, "ordinary"));
	const auto original = frames(directory, held_pid);
	const auto result = player_save_journal_replay(apply, &state);
	if (outcome == player_save_apply_outcome::terminal_failure)
	{
		require(result == player_save_journal_result::ok &&
				player_save_journal_pid_quarantined(held_pid),
			"ordinary terminal failure keeps original quarantine semantics");
		require(frames(directory, after_pid).empty(),
			"terminal failure permits healthy replay");
	}
	else
	{
		require(result == player_save_journal_result::replay_blocked &&
				!player_save_journal_pid_quarantined(held_pid),
			"retryable or ambiguous failure keeps blocked semantics without quarantine");
		require(state.calls[after_pid].empty() && frames(directory, held_pid) == original,
			"retryable control does not fake later-PID progress or original ACK");
		state.held = false;
		require(player_save_journal_replay(apply, &state) ==
					player_save_journal_result::ok &&
				player_save_journal_health_copy().records == 0,
			"retryable control repairs through actual exact checkpoint");
	}
}

void multiple_deferred(const std::string &directory)
{
	replay_state state;
	populate(state, "ordinary", false);
	state.second_held_pid = held_pid + 2;
	append(state, snapshot(state.second_held_pid, 1, "craft"));
	append(state, snapshot(state.second_held_pid, 2, "ordinary"));
	append(state, snapshot(state.second_held_pid, 3, "spell"));
	append(state, snapshot(held_pid + 3, 1, "quest"));
	const auto first = frames(directory, held_pid),
		   second = frames(directory, state.second_held_pid);
	require(player_save_journal_replay(apply, &state) == deferred_result,
		"multiple held PIDs report one incomplete replay pass");
	require(frames(directory, held_pid) == first &&
			frames(directory, state.second_held_pid) == second,
		"scalar sorted skip preserves both original PID forests and earlier proof");
	require(!player_save_journal_pid_quarantined(held_pid) &&
			!player_save_journal_pid_quarantined(state.second_held_pid),
		"both held PIDs avoid quarantine");
	require(state.calls[held_pid] == std::vector<player_revision_t>{ 1 } &&
			state.calls[state.second_held_pid] ==
				std::vector<player_revision_t>{ 1, 2 },
		"each held PID skips only its own later callbacks");
	for (int pid : { before_pid, after_pid, held_pid + 3 })
		require(frames(directory, pid).empty() && state.calls[pid].size() == 1,
			"healthy PIDs before between and after held PIDs actually checkpoint");
	require(player_save_journal_health_copy().backpressure == 0,
		"multiple deferrals are not retry backpressure");
	player_save_journal_shutdown();
	require(player_save_journal_init(directory.c_str()), "multiple held PIDs cold reopen");
	state.calls.clear();
	require(player_save_journal_replay(apply, &state) == deferred_result &&
			frames(directory, held_pid) == first &&
			frames(directory, state.second_held_pid) == second,
		"both held PIDs survive another pass and reopen exactly");
	state.held = false;
	require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok &&
			player_save_journal_health_copy().records == 0,
		"controlled exact success releases both original PID frame sets");
}

void deferred_then_failure(const std::string &directory, player_save_apply_outcome outcome,
			   bool throws)
{
	replay_state state;
	populate(state, "ordinary", false);
	append(state, snapshot(held_pid + 2, 1, "quest"));
	state.throw_after_pid = throws;
	if (!throws)
		state.failures[after_pid] = outcome;
	const auto held = frames(directory, held_pid), failed = frames(directory, after_pid);
	const auto result = player_save_journal_replay(apply, &state);
	const bool terminal = throws || outcome == player_save_apply_outcome::terminal_failure;
	require(result == (terminal ? deferred_result : player_save_journal_result::replay_blocked),
		"real later error retains precedence over incomplete replay report");
	require(frames(directory, held_pid) == held &&
			!player_save_journal_pid_quarantined(held_pid),
		"later unrelated failure neither acknowledges nor quarantines held PID");
	require(frames(directory, before_pid).empty() && state.calls[after_pid].size() == 1,
		"earlier healthy proof checkpoints and actual later failure executes once");
	if (terminal)
	{
		require(player_save_journal_pid_quarantined(after_pid) &&
				frames(directory, after_pid).empty() &&
				frames(directory, held_pid + 2).empty(),
			"real terminal or throw quarantines only failed PID and continues later healthy PID");
		std::vector<player_snapshot> archived;
		std::array<uint8_t, 32> digest = {};
		require(player_save_journal_recovery_inspect(after_pid, &archived, &digest) ==
					player_save_journal_result::ok &&
				archived.size() == 1 &&
				encoded(archived.front()) == state.expected.at({ after_pid, 1 }),
			"legitimate terminal archive retains original complete failed request");
		std::ifstream input(directory + "/player-save.journal.quarantine.archive",
				    std::ios::binary);
		const std::vector<uint8_t> archive{ std::istreambuf_iterator<char>(input),
						    std::istreambuf_iterator<char>() };
		require(failed.size() == 1 &&
				std::search(archive.begin(), archive.end(), failed.front().begin(),
					    failed.front().end()) != archive.end(),
			"terminal archive also preserves exact original raw frame header and checksum");
	}
	else
	{
		require(!player_save_journal_pid_quarantined(after_pid) &&
				frames(directory, after_pid) == failed &&
				state.calls[held_pid + 2].empty(),
			"retry or ambiguity retains exact failed frame and blocks its later work");
	}
	state.failures.clear();
	state.throw_after_pid = false;
	require(player_save_journal_replay(apply, &state) == deferred_result &&
			frames(directory, held_pid) == held,
		"unrelated error repair does not release original held PID");
	state.held = false;
	require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok &&
			player_save_journal_health_copy().records == 0,
		"controlled held-PID exact success drains active frames after unrelated repair");
}

void directory_sync_failure(const std::string &directory)
{
	replay_state state;
	const auto healthy = snapshot(before_pid, 1, "quest");
	append(state, healthy);
	append(state, snapshot(held_pid, 1, "ordinary"));
	append(state, snapshot(held_pid, 2, "craft"));
	append(state, snapshot(after_pid, 1, "spell"));
	const auto held = frames(directory, held_pid);
	const unsigned failures = directory_failures;
	fail_directory_sync = true;
	const auto result = player_save_journal_replay(apply, &state);
	fail_directory_sync = false;
	require(result == player_save_journal_result::io_failure &&
			directory_failures == failures + 1,
		"actual one-shot directory fsync failure overrides deferred readiness");
	require(frames(directory, held_pid) == held &&
			!player_save_journal_pid_quarantined(held_pid),
		"post-rename sync failure retains original held frame bytes without quarantine");
	require(frames(directory, before_pid).empty() && frames(directory, after_pid).empty(),
		"actual compaction rename happened before directory durability failed");
	require(state.calls[before_pid].size() == 1 && state.calls[after_pid].size() == 1,
		"healthy receipt-bearing callbacks actually supplied exact controlled results");
	// An absent ordinary frame can take a no-op checkpoint path. Instead retry
	// the exact healthy receipt-bearing ACK, whose missing original proof path
	// must sync the observed directory state before reporting success.
	const unsigned repaired_before = directory_successes;
	require(player_save_journal_worker_ack(healthy, healthy.revision, nullptr) &&
			directory_successes > repaired_before,
		"exact healthy receipt ACK repair performs successful actual directory fsync");
	require(frames(directory, held_pid) == held,
		"unrelated directory-sync repair never disposes held PID frames");
	state.calls.clear();
	require(player_save_journal_replay(apply, &state) == deferred_result &&
			frames(directory, held_pid) == held,
		"successful unrelated durability repair still reports global replay incomplete");
	state.held = false;
	require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok &&
			player_save_journal_health_copy().records == 0,
		"only controlled exact held-PID success retires original held frames");
}

void allocation_sweep(const std::string &directory)
{
	auto fence_bytes = [](const std::string &path)
	{
		std::vector<std::pair<bool, std::vector<uint8_t>>> result;
		for (const char *name :
		     { "player-save.journal.quarantine", "player-save.journal.quarantine.archive",
		       "player-save.quarantine-pids" })
		{
			std::ifstream input(path + "/" + name, std::ios::binary);
			const bool exists = static_cast<bool>(input);
			result.emplace_back(
				exists, std::vector<uint8_t>{ std::istreambuf_iterator<char>(input),
							      std::istreambuf_iterator<char>() });
		}
		return result;
	};
	unsigned failures = 0;
	bool complete = false;
	for (int position = 0; position < 512 && !complete; ++position)
	{
		player_save_journal_shutdown();
		const auto isolated = directory + "-allocation-" + std::to_string(position);
		require(player_save_journal_init(isolated.c_str()),
			"isolated allocation journal init");
		replay_state state;
		// Isolate scan/collection allocation containment from the separately
		// missing deferral contract, including on immutable BEFORE production.
		state.held = false;
		populate(state, "ordinary", false);
		const auto held = frames(isolated, held_pid), first = frames(isolated, before_pid),
			   last = frames(isolated, after_pid);
		const auto fences = fence_bytes(isolated);
		const auto initial_health = player_save_journal_health_copy();
		allocation_countdown = position;
		allocation_failed = false;
		player_save_journal_result result = player_save_journal_result::replay_blocked;
		try
		{
			result = player_save_journal_replay(apply, &state);
		}
		catch (...)
		{
			allocation_countdown = -1;
			std::cerr << "scan_allocation_position=" << position << '\n';
			throw std::runtime_error(
				"pre-apply allocation must not escape production replay");
		}
		allocation_countdown = -1;
		if (allocation_failed)
		{
			++failures;
			require(result == player_save_journal_result::io_failure ||
					result == player_save_journal_result::replay_blocked,
				"pre-apply resource failure cannot claim successful or deferred pass");
			require(state.calls.empty(),
				"pre-apply allocation failure invokes no repository");
			require(frames(isolated, held_pid) == held &&
					frames(isolated, before_pid) == first &&
					frames(isolated, after_pid) == last,
				"pre-apply resource failure keeps every exact raw frame");
			const auto diagnostic = player_save_journal_diagnostic_copy(held_pid);
			require(diagnostic.available && !diagnostic.pid_fence &&
					!diagnostic.policy_fence &&
					diagnostic.archived_frames == 0 &&
					diagnostic.archived_bytes == 0 &&
					diagnostic.health.corrupt_records ==
						initial_health.corrupt_records &&
					diagnostic.health.unsupported_records ==
						initial_health.unsupported_records &&
					fence_bytes(isolated) == fences,
				"resource exhaustion cannot create durable corruption or policy quarantine");
			std::cout << "safe_scan_allocation_position=" << position
				  << " global_fence=" << diagnostic.global_fence << '\n';
			if (diagnostic.global_fence)
			{
				require(!diagnostic.health.initialized &&
						player_save_journal_pid_quarantined(held_pid) &&
						player_save_journal_replay(apply, &state) ==
							player_save_journal_result::not_initialized &&
						state.calls.empty(),
					"global resource fence refuses replay without repository effects");
				player_save_journal_shutdown();
				require(player_save_journal_init(isolated.c_str()),
					"allocation-disabled reopen recovers safe global resource fence");
				require(frames(isolated, held_pid) == held &&
						frames(isolated, before_pid) == first &&
						frames(isolated, after_pid) == last &&
						fence_bytes(isolated) == fences &&
						!player_save_journal_pid_quarantined(held_pid),
					"reopen preserves all raw frames and original durable fence files");
			}
			require(player_save_journal_replay(apply, &state) ==
					player_save_journal_result::ok,
				"safe retry completes controlled successful repository replay");
		}
		else
		{
			require(result == player_save_journal_result::ok,
				"uninjected scan completes controlled successful repository replay");
			complete = true;
		}
		require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok,
			"post-fault exact replay remains usable");
	}
	require(complete && failures != 0,
		"pre-apply allocation sweep reaches uninjected boundary");
	std::cout << "preapply_allocation_failures=" << failures << '\n';
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
	if (fail_checkpoint_sync && fstat(descriptor, &status) == 0 && S_ISREG(status.st_mode))
	{
		errno = EIO;
		return -1;
	}
	return __real_fdatasync(descriptor);
}

extern "C" int __real_fsync(int descriptor);
extern "C" int __wrap_fsync(int descriptor)
{
	struct stat status = {};
	const bool directory = fstat(descriptor, &status) == 0 && S_ISDIR(status.st_mode);
	if (directory && fail_directory_sync.exchange(false))
	{
		++directory_failures;
		errno = EIO;
		return -1;
	}
	const int result = __real_fsync(descriptor);
	if (directory && result == 0)
		++directory_successes;
	return result;
}

int main(int argc, char **argv)
{
	try
	{
		require(argc == 3, "case and fresh private journal directory required");
		const std::string scenario = argv[1], directory = argv[2];
		require(player_save_journal_init(directory.c_str()), "actual journal init");
		if (scenario == "first_deferred_all_schemas")
			deferred_pass(directory, "ordinary", false, false, false, false);
		else if (scenario == "late_ordinary_newer_revision")
			deferred_pass(directory, "ordinary", true, false, false, false);
		else if (scenario.starts_with("late_") && scenario.ends_with("_proof"))
			deferred_pass(directory, scenario.substr(5, scenario.size() - 11), true,
				      false, false, false);
		else if (scenario == "repeat_reopen_release")
			deferred_pass(directory, "quest", true, true, false, false);
		else if (scenario == "concurrent_append")
			deferred_pass(directory, "ordinary", false, false, true, false);
		else if (scenario == "checkpoint_failure_repair")
			deferred_pass(directory, "craft", true, false, false, true);
		else if (scenario == "retryable_control")
			control(directory, player_save_apply_outcome::retryable_failure);
		else if (scenario == "ambiguous_control")
			control(directory, player_save_apply_outcome::ambiguous_commit);
		else if (scenario == "terminal_control")
			control(directory, player_save_apply_outcome::terminal_failure);
		else if (scenario == "allocation_scan_sweep")
			allocation_sweep(directory);
		else if (scenario == "multiple_deferred_pids")
			multiple_deferred(directory);
		else if (scenario == "deferred_then_retryable")
			deferred_then_failure(directory,
					      player_save_apply_outcome::retryable_failure, false);
		else if (scenario == "deferred_then_ambiguous")
			deferred_then_failure(directory,
					      player_save_apply_outcome::ambiguous_commit, false);
		else if (scenario == "deferred_then_terminal")
			deferred_then_failure(directory,
					      player_save_apply_outcome::terminal_failure, false);
		else if (scenario == "deferred_then_throw")
			deferred_then_failure(directory,
					      player_save_apply_outcome::terminal_failure, true);
		else if (scenario == "directory_sync_failure_repair")
			directory_sync_failure(directory);
		else
			throw std::runtime_error("unknown native scenario");
		player_save_journal_shutdown();
		std::cout << "PASS " << scenario << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		allocation_countdown = -1;
		fail_checkpoint_sync = false;
		fail_directory_sync = false;
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << " backpressure=" << player_save_journal_health_copy().backpressure
			  << " quarantine_bytes="
			  << player_save_journal_health_copy().quarantined_bytes
			  << " held_quarantined=" << player_save_journal_pid_quarantined(held_pid)
			  << " active_records=" << player_save_journal_health_copy().records
			  << '\n';
		player_save_journal_shutdown();
		return 1;
	}
}
