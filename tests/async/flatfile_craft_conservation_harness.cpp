#include "flatfile/flatfile_item_repository.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0x51;
	id.bytes.back() = discriminator;
	return id;
}

static player_item_snapshot snapshot(uint64_t uid, int32_t vnum, int32_t parent_index)
{
	player_item_snapshot item = {};
	item.parent_index = parent_index;
	item.equipment_slot = -1;
	item.object_uid = uid;
	item.vnum = vnum;
	item.string_mask = 1 | 2 | 4;
	item.name = "craft output name";
	item.short_description = "a crafted output";
	item.description = "A crafted output is here.";
	return item;
}

static critical_command craft_command(uint8_t discriminator, uint64_t pid, uint64_t revision,
				      const std::vector<item_transfer_entry> &inputs,
				      const std::vector<player_item_snapshot> &outputs)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::player, pid, 0 };
	payload.to_owner = payload.from_owner;
	payload.reason = item_transfer_reason::craft;
	payload.reason_id = 551;
	payload.expected_from_revision = revision;
	payload.expected_to_revision = revision;
	payload.multi_root = true;
	payload.item_count = static_cast<uint16_t>(inputs.size());
	for (size_t index = 0; index < inputs.size(); ++index)
		payload.items[index] = inputs[index];
	payload.selected_item_uid = outputs.empty() ? inputs.front().root_item_uid : outputs.front().object_uid;
	if (!outputs.empty())
	{
		std::vector<uint8_t> encoded;
		require(player_item_snapshot_list_encode(outputs, &encoded) ==
				player_snapshot_codec_result::ok &&
			encoded.size() <= payload.item_blob.size(),
			"craft output snapshot did not encode");
		payload.item_blob_size = static_cast<uint32_t>(encoded.size());
		std::copy(encoded.begin(), encoded.end(), payload.item_blob.begin());
	}
	critical_command command = {};
	const bool built = item_transfer_command_build(&command, operation(discriminator), payload,
					       critical_source_site::command,
					       critical_deadline_class::interactive);
	require(built, "craft command did not build");
	command.accepted_at_usec = discriminator;
	return command;
}

static void establish(const fs::path &root, uint64_t pid,
			      const std::vector<flatfile_item_ownership_record> &items)
{
	fs::create_directories(root / "domains");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	require(flatfile_item_repository_establish_owner(
			root.string(), { item_owner_type::player, pid, 0 }, items, &error) ==
			flatfile_item_baseline_result::applied,
			"craft owner baseline failed: " + error);
}

static std::vector<flatfile_item_ownership_record> load(const fs::path &root, uint64_t pid,
							 uint64_t *revision)
{
	std::vector<flatfile_item_ownership_record> items;
	std::string error;
	require(flatfile_item_repository_load_owner(
				root.string(), { item_owner_type::player, pid, 0 }, revision, &items, &error) ==
				flatfile_item_repository_result::ok,
				"craft owner load failed: " + error);
	return items;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const item_owner_identity owner = { item_owner_type::player, 77, 0 };
	establish(root, 77,
		  { { 1000, 1000, 0, owner, 1, 500, item_custody_state::active },
		    { 1001, 1001, 0, owner, 1, 501, item_custody_state::active } });
	const std::vector<item_transfer_entry> inputs = {
		{ 1000, 1000, 0, 1, 500, item_custody_state::active },
		{ 1001, 1001, 0, 1, 501, item_custody_state::active },
	};
	const auto success = craft_command(1, 77, 1, inputs,
					   { snapshot(2000, 800, PLAYER_SNAPSHOT_NO_PARENT),
					     snapshot(2001, 801, 0) });
	critical_apply_result applied = flatfile_item_repository_apply(root.string(), success);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
			"multi-output craft did not apply");
	uint64_t revision = 0;
	auto items = load(root, 77, &revision);
	require(revision == 2 && items.size() == 2 && items[0].item_uid == 2000 &&
			items[1].item_uid == 2001 && items[1].parent_item_uid == 2000,
			"multi-output craft did not publish exact output custody");
	applied = flatfile_item_repository_apply(root.string(), success);
	require(applied.outcome == critical_apply_outcome::already_applied,
			"craft replay was not idempotent");

	const std::vector<item_transfer_entry> stale_input = {
		{ 2000, 2000, 0, 1, 800, item_custody_state::active },
	};
	const auto stale = craft_command(2, 77, 1, stale_input, { snapshot(3000, 900, -1) });
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure,
			"stale craft was accepted");
	items = load(root, 77, &revision);
	require(revision == 2 && items.size() == 2 && items[0].item_uid == 2000 &&
			items[1].item_uid == 2001,
			"stale craft mutated authoritative custody");

	const fs::path failure_root = root.string() + "-failure";
	const item_owner_identity failure_owner = { item_owner_type::player, 88, 0 };
	establish(failure_root, 88,
		  { { 4000, 4000, 0, failure_owner, 1, 600, item_custody_state::active } });
	const auto failure = craft_command(
		3, 88, 1,
		{ { 4000, 4000, 0, 1, 600, item_custody_state::active } }, {});
	applied = flatfile_item_repository_apply(failure_root.string(), failure);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
			"zero-output craft failure did not apply");
	items = load(failure_root, 88, &revision);
	require(revision == 2 && items.empty(),
			"zero-output craft failure did not retire its consumed input atomically");

	std::cout << "Issue 551 flat-file craft conservation passed\n";
	return 0;
}
