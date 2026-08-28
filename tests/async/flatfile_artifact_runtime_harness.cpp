#include "flatfile_artifact_repository.h"
#include "prototypes.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void artifact_update_sql(int vnum, bool owned, int location_type, int location, time_t timer,
			 int type);

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

bool redis_report_cache_enabled()
{
	return false;
}

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

void debug(const char *, ...) {}
void logit(const char *, const char *, ...) {}
bool redis_invalidate_artifact_cache()
{
	return true;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	const fs::path root = state_root;
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);

	arti_data data = {};
	require(!get_artifact_data_sql(700, &data),
		"missing flat authority unexpectedly exposed artifact data");
	artifact_update_sql(700, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 5000, 1);
	std::vector<flatfile_artifact_record> records;
	std::string error;
	require(flatfile_artifact_list(state_root, &records, &error) ==
			flatfile_artifact_result::not_found,
		"runtime update synthesized a missing flat artifact authority");

	const flatfile_artifact_record unowned = {
		700, false, FLATFILE_ARTIFACT_ON_GROUND, 1201, 4000, 2, 1000, 42, 3000, 5
	};
	require(flatfile_artifact_establish(state_root, { unowned }, &error) ==
			flatfile_artifact_result::ok,
		"could not establish runtime artifact authority: " + error);
	require(!get_artifact_data_sql(700, &data) && data.vnum == 700 && !data.owned &&
			data.locType == FLATFILE_ARTIFACT_ON_GROUND && data.location == 1201 &&
			data.timer == 4000 && data.type == 2 && data.next == nullptr,
		"runtime artifact read did not preserve unowned database semantics");

	artifact_update_sql(700, true, FLATFILE_ARTIFACT_ON_PLAYER, 77, 6000, 3);
	require(get_artifact_data_sql(700, &data) && data.owned &&
			data.locType == FLATFILE_ARTIFACT_ON_PLAYER && data.location == 77 &&
			data.timer == 6000 && data.type == 3,
		"runtime artifact update did not round trip through the compatibility reader");
	require(flatfile_artifact_list(state_root, &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].bind_owner_pid == 42 &&
			records[0].bind_timer == 3000 && records[0].revision == 6,
		"runtime artifact update did not preserve binding or revision state");

	artifact_update_sql(701, true, FLATFILE_ARTIFACT_ON_GROUND, 1202, 7000, 1);
	require(get_artifact_data_sql(701, &data) && data.vnum == 701 && data.owned &&
			data.location == 1202 && data.timer == 7000 && data.type == 1,
		"runtime artifact update did not insert a newly tracked artifact");
	require(flatfile_artifact_list(state_root, &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 2 && records[1].bind_owner_pid == 0 &&
			records[1].bind_timer == 0 && records[1].revision == 1,
		"newly tracked runtime artifact did not receive binding defaults");

	const fs::path authority = root / "domains/artifact_catalog";
	std::fstream corrupt(authority, std::ios::in | std::ios::out | std::ios::binary);
	require(corrupt.good(), "could not open artifact authority for corruption test");
	corrupt.seekg(-1, std::ios::end);
	char byte = 0;
	corrupt.read(&byte, 1);
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	require(!get_artifact_data_sql(700, &data),
		"runtime artifact read exposed corrupt authority");
	artifact_update_sql(700, true, FLATFILE_ARTIFACT_ON_PLAYER, 88, 8000, 2);
	flatfile_artifact_record stored;
	require(flatfile_artifact_get(state_root, 700, &stored, &error) ==
			flatfile_artifact_result::invalid,
		"runtime artifact update overwrote corrupt authority");

	std::cout << "flat-file artifact gameplay runtime passed\n";
	return 0;
}
