#include "ships/ships.h"

#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace fs = std::filesystem;

extern float ship_cargo_market_mod[NUM_PORTS][NUM_PORTS];
extern float ship_cargo_market_mod_delayed[NUM_PORTS][NUM_PORTS];
extern float ship_contra_market_mod[NUM_PORTS][NUM_PORTS];
void reset_cargo();

static std::string state_root;
static int cargo_timer;
static int delayed_timer;

const char *persistence_mode_flatfile_root(void)
{
	return state_root.c_str();
}

void logit(const char *, const char *, ...) {}

void set_timer(const char *name, int date);

void set_timer(const char *name)
{
	set_timer(name, 1);
}

void set_timer(const char *name, int date)
{
	if (std::string(name) == "update_cargo")
		cargo_timer = date;
	else if (std::string(name) == "update_delayed_cargo_prices")
		delayed_timer = date;
}

int get_timer(const char *)
{
	return 0;
}

bool has_elapsed(const char *, int)
{
	return false;
}

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "metadata", fs::perms::owner_all, fs::perm_options::replace);
}

int main(int argc, char **argv)
{
	require(argc == 2, "fixture root argument required");
	const fs::path fixtures = argv[1];
	state_root = (fixtures / "current").string();
	prepare_root(state_root);

	reset_cargo();
	require(read_cargo(), "missing cargo authority was not established");
	const fs::path authority = fs::path(state_root) / "metadata/cargo_market";
	require(fs::is_regular_file(authority), "cargo authority was not created");
	struct stat info = {};
	require(stat(authority.c_str(), &info) == 0 && (info.st_mode & 0777) == 0600,
		"cargo authority is not owner-only");

	ship_cargo_market_mod[2][3] = 1.2345f;
	ship_contra_market_mod[2][3] = 2.3456f;
	require(write_cargo(), "cargo market mutation did not persist");
	ship_cargo_market_mod[2][3] = 9.0f;
	ship_cargo_market_mod_delayed[2][3] = 8.0f;
	ship_contra_market_mod[2][3] = 7.0f;
	require(read_cargo() && std::fabs(ship_cargo_market_mod[2][3] - 1.2345f) < 0.00001f &&
			std::fabs(ship_cargo_market_mod_delayed[2][3] - 1.2345f) < 0.00001f &&
			std::fabs(ship_contra_market_mod[2][3] - 2.3456f) < 0.00001f,
		"cargo market did not round trip or initialize delayed prices");

	{
		std::fstream file(authority, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open cargo authority for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	ship_cargo_market_mod[2][3] = 6.0f;
	require(!read_cargo() && ship_cargo_market_mod[2][3] == 6.0f,
		"corrupt cargo authority changed live state");
	require(!write_cargo(), "corrupt cargo authority was overwritten");

	state_root = (fixtures / "maintenance").string();
	prepare_root(state_root);
	reset_cargo();
	require(read_cargo(), "maintenance cargo authority was not established");
	constexpr size_t count = 3 + NUM_PORTS * NUM_PORTS * 4;
	std::vector<int64_t> values(count);
	values[0] = 1700000000;
	values[1] = 1;
	values[2] = 1;
	for (int port = 0; port < NUM_PORTS; ++port)
		for (int type = 0; type < NUM_PORTS; ++type)
		{
			const size_t offset = 3 + (port * NUM_PORTS + type) * 4;
			values[offset] = 100;
			values[offset + 1] = 200;
			values[offset + 2] = 1100000 + port * 1000 + type;
			values[offset + 3] = 1200000 + port * 1000 + type;
		}
	require(flatfile_cargo_maintenance_apply(values.data(), values.size()),
		"scheduled cargo snapshot did not persist");
	require(cargo_timer == values[0] && delayed_timer == values[0],
		"scheduled cargo timers were not advanced");
	reset_cargo();
	require(read_cargo() && std::fabs(ship_cargo_market_mod[4][5] - 1.104005f) < 0.00001f &&
			std::fabs(ship_contra_market_mod[4][5] - 1.204005f) < 0.00001f,
		"scheduled cargo modifiers did not round trip");
	require(!flatfile_cargo_maintenance_apply(values.data(), values.size() - 1),
		"malformed scheduled cargo snapshot was accepted");

	state_root = (fixtures / "symlink").string();
	prepare_root(state_root);
	const fs::path target = fixtures / "outside";
	{
		std::ofstream file(target);
		file << "outside";
	}
	fs::create_symlink(target, fs::path(state_root) / "metadata/cargo_market");
	reset_cargo();
	require(!read_cargo() && !write_cargo(), "cargo authority symlink was followed");

	std::cout << "flat-file cargo market passed\n";
	return 0;
}
