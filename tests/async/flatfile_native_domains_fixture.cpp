// Disposable native domain writer/oracle. Never use an existing authority root.
#include "flatfile/flatfile_player_domain_repository.h"

#include <algorithm>
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
	const bool economic = mode.find("economic") != std::string::npos;
	if (mode.starts_with("seed"))
	{
		assert(mode == "seed" || mode == "seed-economic" ||
		       mode == "seed-economic-transfer" || mode == "seed-economic-latest" ||
		       mode == "seed-economic-extra");
		assert(fs::is_empty(root));
		assert(fs::create_directory(fs::path(root) / "domains"));
		fs::permissions(fs::path(root) / "domains", fs::perms::owner_all);
		flatfile_player_domain_record record;
		record.pid = mode == "seed-economic-extra" ? 12 : 11;
		record.account_name = economic ? "renamed" : "Native-Fixture";
		record.racewar = 1;
		record.domains.wallet = { 100, 2, 3, 4 };
		record.domains.bank = { 50, 6, 7, 8 };
		if (economic)
		{
			record.domains.wallet = { mode == "seed-economic-latest" ? 200u : 100u, 0,
						  0, 0 };
			record.domains.bank = { mode == "seed-economic-latest" ? 70u : 50u, 0, 0,
						0 };
		}
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
		if (mode == "seed-economic-transfer")
		{
			currency_command_payload payload = {};
			payload.pid = 11;
			payload.racewar = 1;
			payload.reason = currency_reason_type::atm_deposit;
			std::copy_n("renamed", 8, payload.account_name.begin());
			payload.wallet_delta.amount[0] = -10;
			payload.bank_delta.amount[0] = 10;
			critical_command command;
			critical_operation_id operation = {};
			operation.bytes[0] = operation.bytes[15] = 1;
			assert(currency_command_build(&command, operation, payload, UINT64_MAX,
						      UINT64_MAX, critical_source_site::command,
						      critical_deadline_class::interactive));
			command.accepted_at_usec = 1;
			const auto applied = flatfile_player_domain_apply(root, command);
			assert(applied.outcome == critical_apply_outcome::applied &&
			       !applied.error_code);
			assert(!fs::exists(fs::path(root) / "economic-evidence"));
		}
	}
	else
		assert(mode == "probe" || mode == "probe-economic" ||
		       mode == "probe-economic-extra");
	flatfile_player_domain_record loaded;
	const auto result = flatfile_player_domain_load(root,
							mode.ends_with("economic-extra") ? 12 : 11,
							economic ? "renamed" : "native-fixture", 1,
							&loaded, &error);
	if (result != flatfile_player_domain_result::ok)
	{
		std::cerr << "native_domain_decode_refused\n";
		return 1;
	}
	output(loaded);
}
