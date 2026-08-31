#include "flatfile/flatfile_nexus_repository.h"
#include "nexus_stones.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

namespace fs = std::filesystem;

static std::string persistence_root;

const char *persistence_mode_flatfile_root()
{
	return persistence_root.c_str();
}

void logit(const char *, const char *, ...) {}

void debug(const char *, ...) {}

float get_property(const char *, double fallback)
{
	return fallback;
}

char *coin_stringv(int, int)
{
	static char value[] = "coins";
	return value;
}

void send_to_char(const char *, P_char) {}

namespace
{
void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "metadata", fs::perms::owner_all, fs::perm_options::replace);
}

std::vector<flatfile_nexus_record> records()
{
	return {
		{ 1, "Marduk", 1001, 0, -1, 0, 0, 1 },
		{ 3, "Enlil", 3003, -2, 4, 7, 123456, 3 },
	};
}
} // namespace

int main(int argc, char **argv)
{
	require(argc == 2, "expected temporary root");
	const fs::path base = argv[1];
	std::string error;

	const fs::path empty = base / "empty";
	prepare_root(empty);
	require(flatfile_nexus_establish(empty.string(), {}, &error) == flatfile_nexus_result::ok,
		"empty authority was not established");
	std::vector<flatfile_nexus_record> loaded;
	require(flatfile_nexus_list(empty.string(), &loaded, &error) == flatfile_nexus_result::ok &&
			loaded.empty(),
		"empty authority did not load");
	struct stat info = {};
	require(!stat((empty / "metadata/nexus_stones").c_str(), &info) &&
			(info.st_mode & 0777) == 0600,
		"authority is not owner-only");
	require(flatfile_nexus_establish(empty.string(), records(), &error) ==
			flatfile_nexus_result::unchanged,
		"establish overwrote existing authority");

	const fs::path state = base / "state";
	prepare_root(state);
	require(flatfile_nexus_establish(state.string(), records(), &error) ==
			flatfile_nexus_result::ok,
		"seed authority was not established");
	require(flatfile_nexus_list(state.string(), &loaded, &error) == flatfile_nexus_result::ok &&
			loaded.size() == 2 && loaded[0].id == 1 && loaded[0].name == "Marduk" &&
			loaded[1].id == 3 && loaded[1].room_vnum == 3003 && loaded[1].align == -2 &&
			loaded[1].stat_affect == 4 && loaded[1].affect_amount == 7 &&
			loaded[1].last_touched_at == 123456 && loaded[1].bonus == 3,
		"seed records did not round trip");
	flatfile_nexus_record found;
	require(flatfile_nexus_find(state.string(), 3, &found, &error) ==
				flatfile_nexus_result::ok &&
			found.name == "Enlil",
		"record lookup failed");
	require(flatfile_nexus_update_state(state.string(), 3, 3, 234567, &error) ==
			flatfile_nexus_result::ok,
		"state update failed");
	require(flatfile_nexus_find(state.string(), 3, &found, &error) ==
				flatfile_nexus_result::ok &&
			found.align == 3 && found.last_touched_at == 234567,
		"updated state did not persist");
	persistence_root = state.string();
	NexusStoneInfo live_info = {};
	require(nexus_stone_info(3, &live_info) && live_info.name == "Enlil" &&
			live_info.room_vnum == 3003 && live_info.align == 3 &&
			live_info.stat_affect == 4 && live_info.affect_amount == 7 &&
			live_info.last_touched_at == 234567,
		"live nexus info did not use flat authority");
	require(update_nexus_stone_align(3, -3), "live alignment update failed");
	require(flatfile_nexus_find(state.string(), 3, &found, &error) ==
				flatfile_nexus_result::ok &&
			found.align == -3 && found.last_touched_at > 234567,
		"live alignment update was not durable");
	require(flatfile_nexus_update_state(state.string(), 1, 3, 234568, &error) ==
			flatfile_nexus_result::ok,
		"bonus stone setup failed");
	char_data character = {};
	character.player.racewar = RACEWAR_GOOD;
	require(check_nexus_bonus(&character, 100, NEXUS_BONUS_EPICS) == 110,
		"aligned flat nexus bonus was not applied");
	character.player.racewar = RACEWAR_EVIL;
	require(check_nexus_bonus(&character, 100, NEXUS_BONUS_EPICS) == 100,
		"flat nexus bonus was applied to the opposing side");
	require(flatfile_nexus_update_state(state.string(), 2, 0, 0, &error) ==
			flatfile_nexus_result::not_found,
		"missing record update did not fail");
	require(flatfile_nexus_update_state(state.string(), 3, 4, 0, &error) ==
			flatfile_nexus_result::invalid,
		"invalid alignment was accepted");

	const fs::path authority = state / "metadata/nexus_stones";
	std::fstream file(authority, std::ios::binary | std::ios::in | std::ios::out);
	require(file.good(), "authority could not be opened for corruption test");
	file.seekg(-1, std::ios::end);
	char byte = 0;
	file.read(&byte, 1);
	file.seekp(-1, std::ios::end);
	byte ^= 0x5a;
	file.write(&byte, 1);
	file.close();
	const auto corrupt_bytes = fs::file_size(authority);
	require(flatfile_nexus_list(state.string(), &loaded, &error) ==
			flatfile_nexus_result::invalid,
		"checksum corruption was accepted");
	live_info.name = "preserved";
	require(!nexus_stone_info(3, &live_info) && live_info.name == "preserved",
		"failed live read changed caller state");
	require(flatfile_nexus_update_state(state.string(), 3, 0, 0, &error) ==
				flatfile_nexus_result::invalid &&
			fs::file_size(authority) == corrupt_bytes,
		"corrupt authority was overwritten");

	const fs::path symlink_root = base / "symlink";
	prepare_root(symlink_root);
	const fs::path external = base / "external";
	std::ofstream(external) << "not nexus state";
	fs::create_symlink(external, symlink_root / "metadata/nexus_stones");
	require(flatfile_nexus_list(symlink_root.string(), &loaded, &error) ==
			flatfile_nexus_result::invalid,
		"symlink authority was read");
	require(flatfile_nexus_establish(symlink_root.string(), records(), &error) ==
			flatfile_nexus_result::invalid,
		"symlink authority was overwritten");

	std::cout << "flat-file nexus repository passed\n";
	return 0;
}
