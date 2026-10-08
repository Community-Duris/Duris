#include "core/structs.h"
#include "flatfile/flatfile_zone_story_quest_state.h"
#include "persistence/persistence_mode.h"
#include "sql/zone_story_quest_state_repository.h"
#include "world/zone_story_quest_production.h"
#include "world/zone_story_quest_runtime.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// Same minimal booted-index seam as zone_story_quest_production_harness.cpp.
P_index mob_index;
int number_of_quests = 0;
quest_data quest_index[1];
zone_data *zone_table = nullptr;
int top_of_zone_table = -1;

namespace
{
namespace fs = std::filesystem;
constexpr const char *story_name = "zone-story-quests.state";
constexpr const char *original_key = "runtime-reentry-original";
constexpr uint32_t season_one = 101;
constexpr uint32_t season_two = 102;
unsigned epoch_calls = 0;
unsigned target_renames = 0;
unsigned injections = 0;
bool armed = false;
bool awaiting_sync = false;
struct stat target_directory = {};
struct stat published_file = {};
fs::path evidence;
std::string definition_id;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

bool same_object(const struct stat &left, const struct stat &right)
{
	return left.st_dev == right.st_dev && left.st_ino == right.st_ino;
}

bool target_fd(int fd)
{
	struct stat info = {};
	return fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
	       same_object(info, target_directory);
}

void write_evidence(const std::string &name, const std::string &bytes)
{
	std::ofstream stream(evidence / name, std::ios::binary);
	stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	stream.close();
	require(!stream.fail(), "cannot write evidence " + name);
}

std::string read_file(const fs::path &path)
{
	std::ifstream stream(path, std::ios::binary);
	require(stream.good(), "cannot read " + path.string());
	std::string bytes((std::istreambuf_iterator<char>(stream)), {});
	require(!stream.bad(), "file read failed");
	return bytes;
}

std::string memory_state()
{
	require(zone_story_quest_runtime::ready(), "runtime not ready");
	std::string error;
	const auto state = zone_story_quest_runtime::service()->serialize_state(&error);
	require(!state.empty() && error.empty(), "memory serialization failed: " + error);
	return state;
}

std::string hex(const std::string &bytes)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string output;
	for (unsigned char byte : bytes)
	{
		output += digits[byte >> 4];
		output += digits[byte & 15];
	}
	return output;
}

std::string expected_fact(const std::string &key, uint32_t season)
{
	zone_story_quest_tracking::completion_transaction expected;
	expected.schema_version = 1;
	expected.transaction_id = key;
	expected.quest_definition_id = definition_id;
	expected.zone_number = 1;
	expected.direct_completer_pid = 701;
	expected.credited_pids = { 701, 702 };
	expected.room_vnum = 101;
	expected.completed_at = 1791400000;
	expected.season_id = season;
	expected.content_revision = 1;
	std::string error;
	const auto encoded = zone_story_quest_tracking::serialize_transaction(expected, &error);
	require(!encoded.empty() && error.empty(), "expected transaction invalid: " + error);
	// Only the existing T envelope is assembled here. The actual transaction
	// formatter, parser and service load remain production implementations.
	return "T|" + hex(key) + "|" + hex(encoded) + "\n";
}

void check_facts(const std::string &state, bool second = false)
{
	require(state.starts_with("ZSQF|1\n"), "wrong service header");
	std::istringstream lines(state);
	std::string line;
	unsigned count = 0;
	while (std::getline(lines, line))
		if (line.starts_with("T|"))
			++count;
	require(count == (second ? 2U : 1U), "wrong retained T fact count");
	require(state.find(expected_fact(original_key, season_one)) != std::string::npos,
		"original T does not contain every frozen S1 transaction field");
	if (second)
		require(state.find(expected_fact("runtime-reentry-distinct", season_two)) !=
				std::string::npos,
			"distinct S2 control fact absent");
}

struct cut
{
	std::string memory;
	std::string authority;
	std::string bytes;
	struct stat identity = {};
};

cut snapshot(const fs::path &root, const std::string &label)
{
	cut result;
	result.memory = memory_state();
	std::string error;
	require(flatfile_zone_story_quest_state_load(root.c_str(), 1, &result.authority, &error) ==
			flatfile_zone_story_quest_result::ok,
		"real provider readback failed: " + error);
	const auto path = root / "domains" / story_name;
	result.bytes = read_file(path);
	require(lstat(path.c_str(), &result.identity) == 0 && S_ISREG(result.identity.st_mode) &&
			result.identity.st_uid == geteuid() &&
			(result.identity.st_mode & 0077) == 0,
		"invalid visible story file metadata");
	write_evidence(label + ".memory.txt", result.memory);
	write_evidence(label + ".authority.txt", result.authority);
	write_evidence(label + ".file.bin", result.bytes);
	std::ostringstream info;
	info << "path=" << path << "\ndev=" << result.identity.st_dev
	     << "\nino=" << result.identity.st_ino << "\nmode=" << std::oct
	     << result.identity.st_mode << std::dec << "\nuid=" << result.identity.st_uid
	     << "\nsize=" << result.identity.st_size << "\nrenames=" << target_renames
	     << "\ninjections=" << injections << '\n';
	write_evidence(label + ".identity.txt", info.str());
	return result;
}

void unchanged(const cut &before, const cut &after)
{
	require(before.memory == after.memory && before.authority == after.authority &&
			before.bytes == after.bytes && same_object(before.identity, after.identity),
		"conflict/reload changed exact memory, authority bytes or file identity");
}

void configure(const fs::path &root)
{
	require(setenv("PERSISTENCE_MODE", "flatfile-primary", 1) == 0 &&
			setenv("FLATFILE_STATE_DIR", root.c_str(), 1) == 0 &&
			setenv("ZONE_STORY_SEASON_ID", "101", 1) == 0 &&
			unsetenv("ZONE_STORY_DAILY_ENABLED") == 0,
		"environment setup failed");
	char error[512] = {};
	require(persistence_mode_configure(error, sizeof(error)),
		"real mode/root provisioning failed: " + std::string(error));
	require(persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
			std::string(persistence_mode_flatfile_root()) == root.string(),
		"real persistence mode/root mismatch");
	require(stat((root / "domains").c_str(), &target_directory) == 0,
		"cannot bind target directory identity");
	std::string story_error;
	require(zone_story_quest_runtime::bootstrap(&story_error),
		"actual bootstrap failed: " + story_error);
	require(memory_state() == "ZSQF|1\n" && !fs::exists(root / "domains" / story_name),
		"fresh fixture unexpectedly contains story authority");
}

bool record(const std::string &key, std::string *error)
{
	return zone_story_quest_runtime::record_authoritative_completion(
		definition_id, 1, 701, { 701, 702 }, 101, 1791400000, "ComponentActor", 20, 1, true,
		2, 25, error, key);
}
} // namespace

// SQL-only seams: SQL state is never selected, and an epoch of zero exercises
// the real runtime's environment-season fallback. No SQL success is fabricated.
uint64_t sql_season_epoch()
{
	++epoch_calls;
	return 0;
}
sql_zone_story_quest_state_result sql_zone_story_quest_state_load(uint32_t, std::string *,
								  std::string *)
{
	std::abort();
}
sql_zone_story_quest_state_result sql_zone_story_quest_state_save(uint32_t, const std::string &,
								  std::string *)
{
	std::abort();
}

extern "C" int __real_renameat(int, const char *, int, const char *);
extern "C" int __real_fsync(int);

extern "C" int __wrap_renameat(int old_fd, const char *old_name, int new_fd, const char *new_name)
{
	const int result = __real_renameat(old_fd, old_name, new_fd, new_name);
	if (result == 0 && target_fd(old_fd) && target_fd(new_fd) &&
	    std::strcmp(new_name, story_name) == 0)
	{
		++target_renames;
		if (armed)
		{
			require(!awaiting_sync && injections == 0,
				"fault was armed more than once");
			require(fstatat(new_fd, story_name, &published_file, AT_SYMLINK_NOFOLLOW) ==
						0 &&
					S_ISREG(published_file.st_mode),
				"target rename did not publish a regular file");
			awaiting_sync = true;
		}
	}
	return result;
}

extern "C" int __wrap_fsync(int fd)
{
	if (armed && awaiting_sync && target_fd(fd))
	{
		struct stat current = {};
		require(fstatat(fd, story_name, &current, AT_SYMLINK_NOFOLLOW) == 0 &&
				same_object(current, published_file),
			"directory sync no longer belongs to the exact renamed story file");
		armed = false;
		awaiting_sync = false;
		++injections;
		errno = EIO;
		return -1;
	}
	return __real_fsync(fd);
}

int main(int argc, char **argv)
{
	require(argc == 3, "usage: component PRIVATE_ROOT_PARENT EVIDENCE_DIRECTORY");
	evidence = argv[2];
	const fs::path parent = argv[1];
	index_data mobs[1] = {};
	mob_index = mobs;
	mobs[0].virtual_number = 17;
	char mob_name[] = "the archivist";
	mobs[0].desc2 = mob_name;
	zone_data zones[1] = {};
	char zone_name[] = "The First Heavens";
	zones[0].number = 1;
	zones[0].name = zone_name;
	zone_table = zones;
	top_of_zone_table = 0;
	goal_data give{ .goal_type = QUEST_GOAL_ITEM, .number = 24402, .next = nullptr };
	goal_data receive{ .goal_type = QUEST_GOAL_ITEM, .number = 24403, .next = nullptr };
	quest_complete_data completion{ .message = nullptr,
					.receive = &receive,
					.give = &give,
					.disappear = false,
					.disappear_message = nullptr,
					.echoAll = false,
					.next = nullptr };
	quest_index[0] = { .quester = 0, .quest_message = nullptr, .quest_complete = &completion };
	number_of_quests = 1;

	const auto healthy_root = parent / "healthy";
	configure(healthy_root);
	const auto *bound = zone_story_quest_production::definition_id_for(&completion);
	require(bound && zone_story_quest_runtime::service()->catalog().definitions.size() == 1 &&
			zone_story_quest_runtime::content_revision() == 1,
		"real production catalog did not bind the one fixture Q");
	definition_id = *bound;
	write_evidence("healthy-before.memory.txt", memory_state());
	std::string error;
	require(record(original_key, &error), "healthy S1 record failed: " + error);
	const auto healthy = snapshot(healthy_root, "healthy-S1");
	check_facts(healthy.authority);
	require(healthy.memory == healthy.authority && injections == 0,
		"healthy runtime/provider disagreement");
	require(record(original_key, &error), "identical S1 replay failed: " + error);
	const auto replay = snapshot(healthy_root, "healthy-replay");
	require(replay.memory == healthy.memory && replay.authority == healthy.authority &&
			replay.bytes == healthy.bytes,
		"identical replay changed retained document/file bytes");
	check_facts(replay.authority);

	const auto failure_root = parent / "post-rename-failure";
	configure(failure_root);
	const auto before = memory_state();
	write_evidence("failure-before.memory.txt", before);
	const unsigned rename_before = target_renames;
	armed = true;
	error.clear();
	require(!record(original_key, &error), "post-rename runtime save unexpectedly succeeded");
	write_evidence("failure.error.txt", error);
	require(!armed && !awaiting_sync && injections == 1 &&
			target_renames == rename_before + 1 &&
			error.find("sync authority directory") != std::string::npos,
		"failure did not occur exactly once after the target rename");
	const auto visible = snapshot(failure_root, "failure-visible-S1");
	require(visible.memory == before && visible.authority == healthy.authority &&
			visible.bytes == healthy.bytes &&
			same_object(visible.identity, published_file),
		"false return did not restore memory while retaining exact visible S1 file");
	check_facts(visible.authority);

	require(setenv("ZONE_STORY_SEASON_ID", "102", 1) == 0, "S2 configuration failed");
	require(zone_story_quest_runtime::bootstrap(&error), "actual reload failed: " + error);
	const auto loaded = snapshot(failure_root, "reloaded-S2");
	require(loaded.memory == visible.authority && loaded.authority == visible.authority &&
			loaded.bytes == visible.bytes &&
			same_object(loaded.identity, visible.identity),
		"bootstrap did not load exact retained S1 authority under S2");
	require(zone_story_quest_runtime::current_season_id() == season_two,
		"runtime did not use actual S2 fallback");
	const unsigned before_conflict = target_renames;
	error.clear();
	require(!record(original_key, &error), "same original key under S2 did not conflict");
	require(error == "completion transaction ID was reused with different data",
		"S2 retry failed at a boundary other than actual transaction conflict: " + error);
	write_evidence("retry-S2.error.txt", error);
	const auto conflict = snapshot(failure_root, "retry-S2-conflict");
	unchanged(loaded, conflict);
	check_facts(conflict.authority);
	require(target_renames == before_conflict && injections == 1,
		"conflicting retry reached publication or reinjected fault");

	require(setenv("ZONE_STORY_SEASON_ID", "101", 1) == 0, "S1 control setup failed");
	require(record(original_key, &error), "restored S1 same-key control failed: " + error);
	const auto restored = snapshot(failure_root, "retry-S1-control");
	require(restored.memory == loaded.memory && restored.authority == loaded.authority &&
			restored.bytes == loaded.bytes,
		"same-key S1 control changed retained bytes");
	require(setenv("ZONE_STORY_SEASON_ID", "102", 1) == 0, "distinct control setup failed");
	require(record("runtime-reentry-distinct", &error), "distinct S2 key failed: " + error);
	const auto distinct = snapshot(failure_root, "distinct-S2-control");
	check_facts(distinct.authority, true);
	require(distinct.memory == distinct.authority && distinct.bytes != loaded.bytes &&
			epoch_calls > 0 && injections == 1,
		"distinct-key control did not publish actual changed authority");
	for (const auto &root : { healthy_root, failure_root })
		require(!fs::exists(root / "domains" / ".critical-authority-transaction"),
			"unexpected pending authority journal");
	std::cout << "PASS healthy S1 record/replay; exact post-rename EIO; memory restore; "
		     "real reload; S2 same-key conflict; S1 replay; distinct S2 key\n"
		  << "epoch_calls=" << epoch_calls << " target_renames=" << target_renames
		  << " injections=" << injections << '\n';
}
