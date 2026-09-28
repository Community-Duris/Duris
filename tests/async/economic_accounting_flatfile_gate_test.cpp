#include "economy/economic_accounting_intent.h"
#include "economy/coin_transfer_command.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;
using inventory = std::map<std::string, std::vector<char>>;

static inventory contents(const fs::path &root)
{
	inventory result;
	for (const auto &entry : fs::recursive_directory_iterator(root))
	{
		const auto relative = fs::relative(entry.path(), root).generic_string();
		if (entry.is_directory())
			result[relative + "/"] = {};
		else
		{
			assert(entry.is_regular_file());
			std::ifstream input(entry.path(), std::ios::binary);
			assert(input);
			result[relative] = { std::istreambuf_iterator<char>(input), {} };
			assert(!input.bad());
		}
	}
	return result;
}

static critical_operation_id operation(uint8_t value)
{
	critical_operation_id id = {};
	id.bytes[0] = value;
	return id;
}

static critical_command wallet(uint32_t pid, int64_t delta)
{
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = currency_reason_type::coin_transfer;
	std::strcpy(payload.account_name.data(), "gate-fixture");
	payload.wallet_delta.amount[0] = delta;
	critical_command command;
	assert(currency_command_build(&command, operation(1), payload, 0, 0,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	currency_command_payload decoded;
	assert(currency_command_decode_payload(command, &decoded));
	return command;
}

static critical_command item()
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = 7;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 1;
	payload.items[0] = { 100, 100,
			     0,	  ITEM_TRANSFER_ABSENT_REVISION,
			     500, item_custody_state::absent };
	critical_command command;
	assert(item_transfer_command_build(&command, operation(2), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	assert(item_transfer_command_decode_payload(command, &payload));
	return command;
}

static critical_command coin()
{
	coin_transfer_payload payload;
	payload.source.before[0] = 10;
	payload.source.after[0] = 9;
	payload.source.change = wallet(42, -1);
	payload.destination.before[0] = 0;
	payload.destination.after[0] = 1;
	payload.destination.change = wallet(43, 1);
	critical_command command;
	assert(coin_transfer_command_build(&command, operation(3), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));
	assert(coin_transfer_command_decode_payload(command, &payload));
	return command;
}

static critical_command accounting(critical_command command)
{
	economic_admission_facts facts;
	facts.metadata.lineage.bytes[0] = 2;
	facts.metadata.epoch.bytes[0] = 3;
	facts.metadata.actor_kind = economic_actor_kind::domain;
	facts.metadata.actor_id = 42;
	facts.metadata.writer_id = 1;
	facts.metadata.reason = economic_reason::wallet_transfer;
	assert(economic_intent_freeze(command, facts, &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = 2;
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(command, &encoded) == critical_command_codec_result::ok);
	critical_command decoded;
	assert(critical_command_decode(encoded.data(), encoded.size(), &decoded) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(command, decoded));
	assert(!critical_command_valid(decoded));
	return decoded;
}

static void refused(const critical_apply_result &result)
{
	assert(result.outcome == critical_apply_outcome::terminal_failure);
	assert(result.error_code == EINVAL);
	assert(result.result_size == 0);
}

static void check(const fs::path &missing, const fs::path &seeded, const inventory &before,
		  const critical_command &command)
{
	for (const auto &root : { missing, seeded })
	{
		const std::string path = root.string();
		refused(flatfile_critical_command_repository_apply_selected(
			command, const_cast<char *>(path.c_str())));
		assert(!fs::exists(missing));
		assert(contents(seeded) == before);
		if (command.type == critical_command_type::account_bank)
			refused(flatfile_player_domain_apply(path, command));
		if (command.type == critical_command_type::item_transfer)
			refused(flatfile_item_repository_apply(path, command));
		assert(!fs::exists(missing));
		assert(contents(seeded) == before);
	}
}

// Replace one embedded wire record, retaining the other endpoint and outer header.
static critical_command nested(critical_command command, bool destination)
{
	auto read32 = [&](size_t at)
	{
		uint32_t value = 0;
		for (unsigned byte = 0; byte < 4; ++byte)
			value |= uint32_t(command.payload.at(at + byte)) << (byte * 8);
		return value;
	};
	const size_t header = destination ? 36 + read32(32) : 0;
	const size_t size = read32(header + 32);
	const size_t start = header + 36;
	critical_command leg;
	assert(critical_command_decode(command.payload.data() + start, size, &leg) ==
	       critical_command_codec_result::ok);
	leg = accounting(leg);
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(leg, &encoded) == critical_command_codec_result::ok);
	command.payload.erase(command.payload.begin() + start,
			      command.payload.begin() + start + size);
	command.payload.insert(command.payload.begin() + start, encoded.begin(), encoded.end());
	for (unsigned byte = 0; byte < 4; ++byte)
		command.payload[header + 32 + byte] = uint8_t(encoded.size() >> (byte * 8));
	assert(command.schema_version == 1 && command.accounting_intent.empty());
	assert(critical_command_valid(command));
	coin_transfer_payload rejected;
	assert(!coin_transfer_command_decode_payload(command, &rejected));
	return command;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path base(argv[1]), missing = base / "missing", seeded = base / "seeded";
	assert(!fs::exists(missing) && !fs::exists(seeded));
	fs::create_directories(seeded / "domains");
	fs::permissions(seeded, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(seeded / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const auto item_command = item();
	// Positive legacy control creates real authority state, not a synthetic sentinel.
	const auto applied = flatfile_item_repository_apply(seeded.string(), item_command);
	assert(applied.outcome == critical_apply_outcome::applied);
	const auto before = contents(seeded);
	assert(!before.empty());
	const auto coin_command = coin();
	for (const auto &legacy : { wallet(42, -1), item_command, coin_command })
	{
		auto changed = accounting(legacy);
		check(missing, seeded, before, changed);
		changed.schema_version = 1; // Legacy schema cannot hide frozen accounting bytes.
		check(missing, seeded, before, changed);
		changed = legacy;
		changed.schema_version = 99;
		check(missing, seeded, before, changed);
	}
	check(missing, seeded, before, nested(coin_command, false));
	check(missing, seeded, before, nested(coin_command, true));
	std::cout << "flatfile gates: valid account-bank/item/coin variants rejected; "
		     "both nested schema2 coin legs rejected; missing root absent and "
		     "seeded authority byte inventory unchanged\n";
}
