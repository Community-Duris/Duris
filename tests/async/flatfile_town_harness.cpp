#include "persistence_mode.h"
#include "sql/sql_player.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/stat.h>

namespace fs = std::filesystem;

extern P_town towns;

namespace
{
std::string state_root;
char alpha_filename[] = "alpha";
char beta_filename[] = "beta";

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

void write_file(const fs::path &path, const std::string &content)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	require(output.good(), "could not create fixture " + path.string());
	output << content;
	require(output.good(), "could not write fixture " + path.string());
}

std::string read_file(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	require(input.good(), "could not read " + path.string());
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void clear_towns()
{
	while (towns)
	{
		P_town next = towns->next_town;
		delete towns;
		towns = next;
	}
}

fs::path make_state(const fs::path &base, const std::string &name)
{
	const fs::path root = base / name;
	const fs::path metadata = root / "metadata";
	fs::create_directories(metadata);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(metadata, fs::perms::owner_all, fs::perm_options::replace);
	return root;
}

const char defaults_fixture[] = "alpha\n"
				"1 2 3\n"
				"TRUE\n"
				"10 11 12\n"
				"FALSE\n"
				"20 21 22\n"
				"TRUE\n"
				"30 31\n"
				"beta\n"
				"4 5 6\n"
				"FALSE\n"
				"40 41 42\n"
				"TRUE\n"
				"50 51 52\n"
				"FALSE\n"
				"60 61\n";

const char legacy_fixture[] = "beta\n"
			      "70 71 72\n"
			      "TRUE\n"
			      "73 74 75\n"
			      "TRUE\n"
			      "76 77 78\n"
			      "TRUE\n"
			      "79 80\n";
} // namespace

int top_of_zone_table = 2;
struct zone_data zone_storage[3] = {};
struct zone_data *zone_table = zone_storage;
P_town towns = NULL;

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

void logit(const char *, const char *, ...) {}

int main(int argc, char **argv)
{
	/* Keep authority fixtures private even when the invoking account uses a
	 * collaborative umask such as 0002. */
	umask(0077);
	require(argc == 2, "temporary directory argument required");
	const fs::path base = fs::absolute(argv[1]);
	const fs::path working = base / "working";
	fs::create_directories(working / "defaults");
	fs::create_directories(working / "Players");
	write_file(working / "defaults/towns", defaults_fixture);
	zone_storage[1].filename = alpha_filename;
	zone_storage[2].filename = beta_filename;
	fs::current_path(working);

	const fs::path default_state = make_state(base, "default-state");
	state_root = default_state.string();
	require(sql_load_towns(), "tracked defaults did not establish fresh town state");
	require(towns && towns->next_town && !towns->next_town->next_town,
		"default town count was not restored");
	require(towns->zone == &zone_storage[1] && towns->offense == 1 && towns->defense == 2 &&
			towns->resources == 3 && towns->deploy_guard && !towns->deploy_cavalry &&
			towns->deploy_portals && towns->portal_load_room == 31,
		"default town fields were not restored in historical order");

	const fs::path authority = default_state / "metadata/towns";
	require(fs::is_regular_file(authority), "fresh town authority was not published");
	require(read_file(authority) == defaults_fixture,
		"fresh authority did not retain the historical town format");
	const fs::perms authority_permissions = fs::status(authority).permissions();
	require((authority_permissions & fs::perms::group_all) == fs::perms::none &&
			(authority_permissions & fs::perms::others_all) == fs::perms::none,
		"town authority permissions were not private");

	towns->resources = 900;
	towns->offense = -7;
	towns->deploy_cavalry = true;
	towns->cavalry_vnum = 12345;
	require(sql_save_towns(), "live town mutation did not save");
	clear_towns();
	require(sql_load_towns(), "saved towns did not reload");
	require(towns && towns->resources == 900 && towns->offense == -7 && towns->deploy_cavalry &&
			towns->cavalry_vnum == 12345,
		"saved town mutation did not survive reload");

	const std::string valid_authority = read_file(authority);
	write_file(authority, valid_authority + "BROKEN\n");
	const std::string corrupt_authority = read_file(authority);
	towns->resources = 901;
	require(!sql_save_towns(), "town save overwrote a corrupt authority");
	require(read_file(authority) == corrupt_authority, "corrupt town authority was modified");
	require(!sql_load_towns(), "partial town record was accepted");
	require(towns && towns->resources == 901, "failed town load replaced live state");
	write_file(authority, valid_authority);

	clear_towns();
	fs::remove(authority);
	write_file(working / "Players/towns", legacy_fixture);
	const fs::path legacy_state = make_state(base, "legacy-state");
	state_root = legacy_state.string();
	require(sql_load_towns(), "legacy Players/towns did not import");
	require(towns && !towns->next_town && towns->zone == &zone_storage[2] &&
			towns->offense == 70 && towns->resources == 72 && towns->portal_vnum == 79,
		"legacy town fields were not restored");
	require(read_file(legacy_state / "metadata/towns") == legacy_fixture,
		"legacy town state was not published canonically");

	clear_towns();
	fs::remove(working / "Players/towns");
	const fs::path unsafe_state = make_state(base, "unsafe-state");
	state_root = unsafe_state.string();
	write_file(base / "outside", defaults_fixture);
	fs::create_symlink(base / "outside", unsafe_state / "metadata/towns");
	require(!sql_load_towns(), "symlink town authority was accepted");

	std::cout << "flat-file towns passed\n";
	return 0;
}
