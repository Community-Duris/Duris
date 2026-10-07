// Disposable native domain writer/oracle. Never use an existing authority root.
#include "flatfile/flatfile_player_domain_repository.h"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
static void output(const flatfile_player_domain_record &record)
{
	std::cout << "{\"wallet_revision\":" << record.domains.wallet_revision
		  << ",\"bank_revision\":" << record.domains.bank_revision << ",\"wallet\":[";
	for (size_t i = 0; i < 4; ++i)
		std::cout << (i ? "," : "") << record.domains.wallet[i];
	std::cout << "],\"bank\":[";
	for (size_t i = 0; i < 4; ++i)
		std::cout << (i ? "," : "") << record.domains.bank[i];
	std::cout << "]}\n";
}
int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string mode = argv[1], root = argv[2];
	assert(fs::path(root).is_absolute() && fs::is_directory(root));
	std::string error;
	if (mode == "seed")
	{
		assert(fs::is_empty(root));
		assert(fs::create_directory(fs::path(root) / "domains"));
		fs::permissions(fs::path(root) / "domains", fs::perms::owner_all);
		flatfile_player_domain_record record;
		record.pid = 11;
		record.account_name = "Native-Fixture";
		record.racewar = 1;
		record.domains.wallet = { 100, 2, 3, 4 };
		record.domains.bank = { 50, 6, 7, 8 };
		record.domains.epics = 15;
		record.domains.frags = -2;
		record.domains.old_frags = -3;
		record.recent_pvp_deaths = { 200, 100, 100 };
		record.completed_epic_zones = { 1, 3, 9 };
		const auto established = flatfile_player_domain_establish(root, record, &error);
		if (established != flatfile_player_domain_result::ok)
		{
			std::cerr << "native_domain_fixture_seed_failed " << error << '\n';
			return 1;
		}
		assert(!fs::exists(fs::path(root) / "economic-evidence"));
	}
	else
		assert(mode == "probe");
	flatfile_player_domain_record loaded;
	const auto result =
		flatfile_player_domain_load(root, 11, "native-fixture", 1, &loaded, &error);
	if (result != flatfile_player_domain_result::ok)
	{
		std::cerr << "native_domain_decode_refused\n";
		return 1;
	}
	output(loaded);
}
