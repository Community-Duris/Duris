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
// Callback failures are controlled; no SQL transaction or service is invoked.
namespace
{
constexpr int held_pid = 61500;
constexpr int before_pid = held_pid - 1, after_pid = held_pid + 1;
thread_local bool callback_allocation_fault = false;
std::atomic<bool> fail_checkpoint_sync{ false };
std::atomic<unsigned> checkpoint_sync_faults{ 0 };

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
	std::string failure;
	player_revision_t fail_at = 1;
	bool active = true;
	bool ordinary_newer = false;
	bool marked_effect = false;
	unsigned simulated_effects = 0;
	unsigned intended_branch_hits = 0;
	std::map<int, std::vector<player_revision_t>> calls;
	std::map<std::pair<int, player_revision_t>, std::vector<uint8_t>> expected;
};

player_save_apply_result apply(const player_snapshot &request, void *raw)
{
	auto &state = *static_cast<replay_state *>(raw);
	state.calls[request.pid].push_back(request.revision);
	const auto expected = state.expected.find({ request.pid, request.revision });
	require(expected != state.expected.end() && encoded(request) == expected->second,
		"native callback request preserves sealed full canonical payload");
	if (state.active && request.pid == held_pid && request.revision == state.fail_at)
	{
		if (state.failure == "badalloc")
		{
			require(callback_allocation_fault,
				"thread-local callback fault actually armed");
			++state.intended_branch_hits;
			if (state.marked_effect)
				++state.simulated_effects;
			// Commit-uncertainty simulation only. This controlled effect is
			// not a SQL COMMIT, repository rollback or connection-lease proof.
			throw std::bad_alloc();
		}
		if (state.failure == "runtime")
		{
			++state.intended_branch_hits;
			throw std::runtime_error("controlled repository terminal exception");
		}
		if (state.failure == "retry")
		{
			++state.intended_branch_hits;
			return { player_save_apply_outcome::retryable_failure, 0, 1205 };
		}
		if (state.failure == "ambiguous")
		{
			++state.intended_branch_hits;
			return { player_save_apply_outcome::ambiguous_commit, 0, 2013 };
		}
		player_save_apply_result result{
			player_save_apply_outcome::terminal_failure, 0,
			state.failure == "enomem" ? ENOMEM :
						    PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH
		};
		if (state.failure == "custody")
			result.custody_diagnosis =
				player_save_custody_diagnosis::active_custody_absent_from_snapshot;
		else if (state.failure == "death")
			result.custody_diagnosis =
				player_save_custody_diagnosis::invalid_death_payload;
		else
			require(state.failure == "enomem", "known terminal control");
		++state.intended_branch_hits;
		return result;
	}
	const auto durable = request.pid == held_pid && state.ordinary_newer &&
					     request.revision < state.fail_at ?
				     99 :
				     request.revision;
	return { player_save_apply_outcome::applied, durable, 0 };
}

uint64_t trace_cursor()
{
	const auto trace = persistence_trace_copy(
		{ critical_entity_type::player, static_cast<uint64_t>(held_pid) });
	require(trace.available, "actual native diagnostic recorder available");
	return trace.latest_sequence;
}

void assert_failure_trace(const replay_state &state, uint64_t cursor, unsigned prior_hits)
{
	require(state.intended_branch_hits == prior_hits + 1,
		"exact intended callback failure branch ran, not an internal oracle exception");
	const auto trace = persistence_trace_copy(
		{ critical_entity_type::player, static_cast<uint64_t>(held_pid) });
	require(trace.available && trace.dropped == 0,
		"actual native replay diagnostic is available without dropped observations");
	player_save_apply_outcome outcome = player_save_apply_outcome::terminal_failure;
	unsigned error = EFAULT;
	player_save_custody_diagnosis diagnosis = player_save_custody_diagnosis::none;
	if (state.failure == "badalloc")
	{
		outcome = player_save_apply_outcome::retryable_failure;
		error = ENOMEM;
	}
	else if (state.failure == "retry")
	{
		outcome = player_save_apply_outcome::retryable_failure;
		error = 1205;
	}
	else if (state.failure == "ambiguous")
	{
		outcome = player_save_apply_outcome::ambiguous_commit;
		error = 2013;
	}
	else if (state.failure == "enomem")
		error = ENOMEM;
	else if (state.failure == "custody" || state.failure == "death")
	{
		error = PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH;
		diagnosis =
			state.failure == "custody" ?
				player_save_custody_diagnosis::active_custody_absent_from_snapshot :
				player_save_custody_diagnosis::invalid_death_payload;
	}
	else
		require(state.failure == "runtime", "known diagnostic failure oracle");
	unsigned matches = 0;
	for (size_t index = 0; index < trace.count; ++index)
	{
		const auto &event = trace.events[index];
		if (event.sequence <= cursor ||
		    event.stage != persistence_trace_stage::save_replay_result ||
		    event.pid != held_pid || event.revision != state.fail_at)
			continue;
		++matches;
		std::cout << "failure_trace outcome=" << event.outcome << " error=" << event.error
			  << " diagnosis=" << event.diagnosis << '\n';
		require(event.outcome == static_cast<unsigned>(outcome) && event.error == error &&
				event.diagnosis == static_cast<unsigned>(diagnosis) &&
				event.durable_revision == 0,
			"exact replay result outcome/error/diagnosis preserves failure classification");
	}
	require(matches == 1, "one actual replay result trace for exact failed PID/revision");
}

void append(replay_state &state, const player_snapshot &request)
{
	state.expected[{ request.pid, request.revision }] = encoded(request);
	require(player_save_journal_append(request) == player_save_journal_result::ok,
		"actual validated journal append");
}

using durable_files = std::vector<std::pair<bool, std::vector<uint8_t>>>;
durable_files fence_bytes(const std::string &directory)
{
	durable_files result;
	for (const char *name :
	     { "player-save.journal.quarantine", "player-save.journal.quarantine.archive",
	       "player-save.quarantine-pids" })
	{
		std::ifstream input(directory + "/" + name, std::ios::binary);
		const bool exists = static_cast<bool>(input);
		result.emplace_back(exists,
				    std::vector<uint8_t>{ std::istreambuf_iterator<char>(input),
							  std::istreambuf_iterator<char>() });
	}
	return result;
}

void assert_unresolved(const std::string &directory, replay_state &state,
		       const raw_frames &original, const raw_frames &later,
		       const durable_files &fences, unsigned expected_calls)
{
	const auto diagnostic = player_save_journal_diagnostic_copy(held_pid);
	require(diagnostic.available && !diagnostic.global_fence && !diagnostic.pid_fence &&
			!diagnostic.policy_fence && diagnostic.archived_frames == 0 &&
			diagnostic.archived_bytes == 0 &&
			diagnostic.health.quarantined_bytes == 0 &&
			diagnostic.health.corrupt_records == 0 &&
			diagnostic.health.unsupported_records == 0 &&
			fence_bytes(directory) == fences,
		"unresolved execution cannot invent quarantine, corruption or policy evidence");
	require(frames(directory, held_pid) == original,
		"all failed-PID raw frame bytes and identities remain, including earlier successful proofs");
	require(frames(directory, before_pid).empty() && frames(directory, after_pid) == later &&
			state.calls[before_pid].size() == 1 && state.calls[after_pid].empty(),
		"earlier unrelated PID checkpoints while later callbacks remain unattempted");
	require(state.calls[held_pid].size() == expected_calls &&
			state.calls[held_pid].back() == state.fail_at,
		"no same-PID callback executes after exact unresolved revision");
}

void unresolved(const std::string &directory, const std::string &kind, const std::string &failure,
		bool late, bool marked, bool checkpoint_failure)
{
	replay_state state;
	state.failure = failure;
	state.marked_effect = marked;
	state.fail_at = late ? 2 : 1;
	state.ordinary_newer = late && kind == "ordinary";
	append(state, snapshot(before_pid, 1, "quest"));
	append(state, snapshot(held_pid, 1, kind));
	if (late)
		append(state, snapshot(held_pid, 2, "ordinary"));
	player_revision_t revision = late ? 3 : 2;
	for (const std::string later_kind : { "ordinary", "death", "quest", "spell", "craft" })
		append(state, snapshot(held_pid, revision++, later_kind));
	append(state, snapshot(held_pid, late ? 3 : 1, late ? "ordinary" : kind));
	append(state, snapshot(after_pid, 1, "craft"));
	const auto original = frames(directory, held_pid), earlier = frames(directory, before_pid),
		   later = frames(directory, after_pid);
	const auto fences = fence_bytes(directory);
	callback_allocation_fault = true;
	fail_checkpoint_sync = checkpoint_failure;
	const auto first_cursor = trace_cursor();
	const auto result = player_save_journal_replay(apply, &state);
	fail_checkpoint_sync = false;
	std::cout << "result=" << static_cast<unsigned>(result)
		  << " original_failed_frames=" << original.size()
		  << " active_failed_frames=" << frames(directory, held_pid).size()
		  << " callbacks=" << state.calls[held_pid].size()
		  << " simulated_effects=" << state.simulated_effects << '\n';
	assert_failure_trace(state, first_cursor, 0);
	if (checkpoint_failure)
	{
		require(checkpoint_sync_faults != 0 &&
				result == player_save_journal_result::io_failure,
			"real checkpoint failure retains error priority over unresolved replay");
		require(frames(directory, held_pid) == original &&
				frames(directory, before_pid) == earlier &&
				frames(directory, after_pid) == later &&
				fence_bytes(directory) == fences,
			"pre-rename checkpoint failure retains every original durable frame and fence file");
		state.calls.clear();
		const auto repair_cursor = trace_cursor();
		const auto repair_prior_hits = state.intended_branch_hits;
		require(player_save_journal_replay(apply, &state) ==
				player_save_journal_result::replay_blocked,
			"checkpoint repair remains globally unresolved without granting readiness");
		assert_failure_trace(state, repair_cursor, repair_prior_hits);
	}
	else
		require(result == player_save_journal_result::replay_blocked,
			"callback allocation/retry/ambiguity reports unresolved, not ok or deferred");
	assert_unresolved(directory, state, original, later, fences, late ? 2 : 1);
	if (marked)
		require(state.simulated_effects == 1,
			"controlled uncertainty effect actually occurred once");
	player_save_journal_shutdown();
	require(player_save_journal_init(directory.c_str()), "actual journal cold reopen");
	require(frames(directory, held_pid) == original && frames(directory, after_pid) == later &&
			fence_bytes(directory) == fences,
		"cold reopen preserves exact unresolved frames and fence content");
	state.calls.clear();
	const auto reopen_cursor = trace_cursor();
	const auto reopen_prior_hits = state.intended_branch_hits;
	require(player_save_journal_replay(apply, &state) ==
			player_save_journal_result::replay_blocked,
		"reopened unresolved execution still blocks global completion");
	assert_failure_trace(state, reopen_cursor, reopen_prior_hits);
	require(frames(directory, held_pid) == original && frames(directory, after_pid) == later &&
			state.calls[after_pid].empty() &&
			state.calls[held_pid].size() == (late ? 2U : 1U) &&
			fence_bytes(directory) == fences &&
			!player_save_journal_pid_quarantined(held_pid),
		"repeated unresolved pass neither ACKs nor quarantines original failed PID");
	// Controlled exact-success repair only. No actual SQL cleanup or safe
	// stale-frame release is inferred from disabling this test callback fault.
	state.active = false;
	state.ordinary_newer = false;
	callback_allocation_fault = false;
	state.calls.clear();
	require(player_save_journal_replay(apply, &state) == player_save_journal_result::ok &&
			player_save_journal_health_copy().records == 0,
		"only exact controlled repository success retires all original requests");
	require(state.calls[after_pid].size() == 1 &&
			state.calls[held_pid].size() == original.size() - 1,
		"successful repair handles every distinct original request and later PID");
}

void terminal_control(const std::string &directory, const std::string &failure)
{
	replay_state state;
	state.failure = failure;
	append(state, snapshot(before_pid, 1, "ordinary"));
	append(state, snapshot(held_pid, 1, failure == "death" ? "death" : "quest"));
	append(state, snapshot(held_pid, 2, "craft"));
	append(state, snapshot(after_pid, 1, "ordinary"));
	const auto original = frames(directory, held_pid);
	const auto cursor = trace_cursor();
	const auto result = player_save_journal_replay(apply, &state);
	assert_failure_trace(state, cursor, 0);
	require(result == player_save_journal_result::ok &&
			player_save_journal_pid_quarantined(held_pid) &&
			frames(directory, held_pid).empty() &&
			frames(directory, before_pid).empty() &&
			frames(directory, after_pid).empty() && state.calls[held_pid].size() == 1 &&
			state.calls[after_pid].size() == 1,
		"explicit terminal/runtime/custody/death controls preserve actual quarantine and healthy progress");
	const auto diagnostic = player_save_journal_diagnostic_copy(held_pid);
	require(diagnostic.available && diagnostic.pid_fence && !diagnostic.global_fence &&
			diagnostic.archived_frames == original.size(),
		"genuine terminal control retains every typed frame in durable quarantine");
	std::ifstream input(directory + "/player-save.journal.quarantine.archive",
			    std::ios::binary);
	const std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(input),
					  std::istreambuf_iterator<char>() };
	uint64_t archived_bytes = 0;
	for (const auto &raw : original)
	{
		const auto found = std::search(bytes.begin(), bytes.end(), raw.begin(), raw.end());
		require(found != bytes.end(),
			"terminal archive contains exact original raw header, identity and full payload");
		player_snapshot archived;
		require(player_snapshot_decode(&*found + 72, raw.size() - 72, &archived) ==
					player_snapshot_codec_result::ok &&
				encoded(archived) ==
					state.expected.at({ archived.pid, archived.revision }),
			"actual archived payload decodes to the sealed original typed request");
		archived_bytes += raw.size();
	}
	require(diagnostic.archived_bytes == archived_bytes,
		"archive diagnostic byte total matches exact original raw records");
	player_save_journal_shutdown();
	require(player_save_journal_init(directory.c_str()) &&
			player_save_journal_pid_quarantined(held_pid),
		"terminal quarantine persists after actual reopen");
}
} // namespace

extern "C" int __real_fdatasync(int descriptor);
extern "C" int __wrap_fdatasync(int descriptor)
{
	struct stat status = {};
	if (fail_checkpoint_sync && fstat(descriptor, &status) == 0 && S_ISREG(status.st_mode))
	{
		++checkpoint_sync_faults;
		errno = EIO;
		return -1;
	}
	return __real_fdatasync(descriptor);
}

int main(int argc, char **argv)
{
	try
	{
		require(argc == 3, "case and fresh private journal directory required");
		const std::string scenario = argv[1], directory = argv[2];
		require(player_save_journal_init(directory.c_str()), "actual journal init");
		if (scenario == "first_badalloc")
			unresolved(directory, "ordinary", "badalloc", false, false, false);
		else if (scenario == "first_badalloc_after_effect")
			unresolved(directory, "ordinary", "badalloc", false, true, false);
		else if (scenario == "checkpoint_badalloc_repair")
			unresolved(directory, "quest", "badalloc", true, false, true);
		else if (scenario == "checkpoint_retry_repair")
			unresolved(directory, "ordinary", "retry", true, false, true);
		else if (scenario.starts_with("late_"))
		{
			const auto separator = scenario.find('_', 5);
			require(separator != std::string::npos, "late scenario shape");
			unresolved(directory, scenario.substr(5, separator - 5),
				   scenario.substr(separator + 1), true, false, false);
		}
		else if (scenario.ends_with("_control"))
			terminal_control(directory, scenario.substr(0, scenario.size() - 8));
		else
			throw std::runtime_error("unknown unresolved native case");
		player_save_journal_shutdown();
		std::cout << "PASS " << scenario << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		callback_allocation_fault = false;
		fail_checkpoint_sync = false;
		const auto health = player_save_journal_health_copy();
		std::cerr << "FAIL " << (argc > 1 ? argv[1] : "setup") << ": " << error.what()
			  << " backpressure=" << health.backpressure
			  << " quarantine_bytes=" << health.quarantined_bytes
			  << " failed_pid_quarantined="
			  << player_save_journal_pid_quarantined(held_pid)
			  << " active_records=" << health.records << '\n';
		player_save_journal_shutdown();
		return 1;
	}
}
