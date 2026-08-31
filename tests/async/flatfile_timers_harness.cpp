#include "persistence/persistence_mode.h"
#include "timers.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace
{
std::string state_root;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}
} // namespace

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

void logit(const char *, const char *, ...) {}

void cargo_activity() {}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	fs::create_directories(fs::path(state_root) / "metadata");
	fs::permissions(state_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(fs::path(state_root) / "metadata", fs::perms::owner_all,
			fs::perm_options::replace);

	require(get_timer("update_cargo") == 0, "missing timer was not zero");
	set_timer("update_cargo", 123456);
	require(get_timer("update_cargo") == 123456, "timer did not round trip");
	set_timer("update_cargo", -17);
	require(get_timer("update_cargo") == -17, "replacement timer did not round trip");

	const fs::path record = fs::path(state_root) / "metadata/timer.update_cargo";
	require(fs::is_regular_file(record), "timer record was not created");
	require((fs::status(record).permissions() & fs::perms::group_all) == fs::perms::none &&
			(fs::status(record).permissions() & fs::perms::others_all) ==
				fs::perms::none,
		"timer record permissions were not private");

	{
		std::fstream file(record, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "timer record could not be opened for corruption test");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(get_timer("update_cargo") == 0, "corrupt timer record was accepted");

	fs::remove(record);
	const fs::path target = fs::path(state_root) / "target";
	std::ofstream(target) << "not a timer";
	fs::create_symlink(target, record);
	require(get_timer("update_cargo") == 0, "timer symlink was followed");
	fs::remove(record);

	const size_t before = static_cast<size_t>(
		std::distance(fs::directory_iterator(fs::path(state_root) / "metadata"),
			      fs::directory_iterator{}));
	set_timer("../unsafe", 42);
	const size_t after = static_cast<size_t>(
		std::distance(fs::directory_iterator(fs::path(state_root) / "metadata"),
			      fs::directory_iterator{}));
	require(before == after, "unsafe timer name created a record");

	std::cout << "flat-file timers passed\n";
	return 0;
}
