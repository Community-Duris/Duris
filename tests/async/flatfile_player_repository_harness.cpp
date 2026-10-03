#include "flatfile/flatfile_player_repository.h"
#include "flatfile/flatfile_boon_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_craft_progression.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "persistence/persistence_observability.h"
#include "economy/coin_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "player/player_quarantine_recovery.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "economy/item_transfer_accounting.h"
#include "classes/necromancy.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <csignal>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static const char *read_failure_filename = nullptr;
#ifdef DURIS_FLATFILE_PLAYER_READ_FAULT_TEST
extern "C" int __real_openat(int, const char *, int, ...);
extern "C" int __wrap_openat(int fd, const char *name, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT)
	{
		va_list arguments;
		va_start(arguments, flags);
		mode = va_arg(arguments, int);
		va_end(arguments);
	}
	if (!(flags & O_CREAT) && read_failure_filename && !strcmp(name, read_failure_filename))
	{
		errno = EIO;
		return -1;
	}
	return __real_openat(fd, name, flags, mode);
}
#endif

bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
	const bool pid_identity = request.pid > 0 && !request.account_name.empty() &&
				  request.account_name.size() <= PLAYER_LOAD_ACCOUNT_MAX;
	const bool name_identity = request.pid == 0 && !request.player_name.empty() &&
				   request.player_name.size() <= PLAYER_LOAD_NAME_MAX;
	return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id > 0 &&
	       (pid_identity || name_identity) && request.deadline_usec > now &&
	       request.deadline_usec - now <= PLAYER_LOAD_TIMEOUT_USEC &&
	       (!request.include_pets || request.include_items);
}

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static player_snapshot make_full(player_revision_t revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 42;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 50, 0, false });
	snapshot.status_integers.push_back({ player_status_field::racewar, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 11, 0, false });
	snapshot.status_integers.push_back({ player_status_field::silver, 12, 0, false });
	snapshot.status_integers.push_back({ player_status_field::gold, 13, 0, false });
	snapshot.status_integers.push_back({ player_status_field::platinum, 14, 0, false });
	snapshot.status_integers.push_back({ player_status_field::epics, 15, 0, false });
	snapshot.status_integers.push_back({ player_status_field::frags, 16, 0, false });
	snapshot.status_integers.push_back({ player_status_field::old_frags, 17, 0, false });
	for (int index = 0; index < 10; ++index)
		snapshot.status_integers.push_back(
			{ static_cast<player_status_field>(
				  static_cast<unsigned int>(player_status_field::base_strength) +
				  index),
			  50 + index, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Player" });
	snapshot.conditions = { 1, 2, 3, 4, 5 };
	snapshot.quest_values[3] = 77;
	snapshot.languages.push_back({ 1, 90, 0 });
	snapshot.introductions.push_back({ 2, 44, 12345 });
	snapshot.timers.push_back({ 3, 67890, 0 });
	snapshot.undead_slots.push_back({ 4, 2, 0 });
	snapshot.forged_items.push_back({ 5, 6001, 0 });
	snapshot.granted_commands.push_back(42);
	snapshot.skills.push_back({ 9, 80, 1 });
	player_affect_snapshot affect = {};
	affect.type = 11;
	affect.duration = 12;
	affect.bitvectors[2] = 99;
	affect.wear_off_character = "gone";
	snapshot.affects.push_back(affect);
	player_item_snapshot parent = {};
	parent.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	parent.object_uid = 100;
	parent.vnum = 500;
	parent.string_mask = 1;
	parent.name = "container";
	parent.values[0] = 8;
	parent.dynamic_affects.push_back({ 1, 2, 3 });
	player_item_extra_description_snapshot description = {};
	description.keyword = "SPELLBOOK";
	description.spellbook = true;
	description.spell_ids = { 7, 12 };
	parent.extra_descriptions.push_back(description);
	snapshot.items.push_back(parent);
	player_item_snapshot child = {};
	child.parent_index = 0;
	child.object_uid = 101;
	child.vnum = 501;
	snapshot.items.push_back(child);
	player_pet_snapshot pet = {};
	pet.mob_vnum = 700;
	pet.room_vnum = 1201;
	pet.items.push_back(child);
	pet.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	pet.items[0].object_uid = 102;
	snapshot.pets.push_back(pet);
	snapshot.shapes.push_back({ 800, 2, 100, 200 });
	snapshot.trophies.push_back({ 12, 300 });
	snapshot.recipes_are_external = true;
	snapshot.output_preferences = "v1;m=1;12=27";
	return snapshot;
}

// The immutable record of a death whose corpse handoff the ledger refused: the
// corpse identity and room, the wallet a rejected conversion never took, the
// captured player items and the disputed custody rows, none of them in inventory.
static player_snapshot make_death(player_revision_t revision)
{
	player_snapshot snapshot = make_full(revision);
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.save_intent = 4; // RENT_DEATH
	snapshot.items.clear();
	snapshot.pets.clear();
	for (player_snapshot_integer &row : snapshot.status_integers)
		if (row.field == player_status_field::copper ||
		    row.field == player_status_field::silver ||
		    row.field == player_status_field::gold ||
		    row.field == player_status_field::platinum)
			row.signed_value = 0;
	snapshot.death.emplace();
	player_death_snapshot &death = *snapshot.death;
	death.operation_id.bytes.fill(0);
	death.operation_id.bytes[0] = 0x11;
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	death.wallet_before = { 11, 12, 13, 14 };
	death.wallet_pile_uid = 202;

	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 200;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = snapshot.pid;
	corpse.values[CORPSE_SAVEID] = 9001;
	death.corpse.push_back(corpse);

	player_item_snapshot refused = {};
	refused.parent_index = 0;
	refused.object_uid = 100;
	refused.vnum = 500;
	death.corpse.push_back(refused);

	player_item_snapshot refused_child = {};
	refused_child.parent_index = 1;
	refused_child.object_uid = 101;
	refused_child.vnum = 501;
	death.corpse.push_back(refused_child);

	player_item_snapshot wallet = {};
	wallet.parent_index = 0;
	wallet.object_uid = death.wallet_pile_uid;
	wallet.vnum = VOBJ_COINS;
	wallet.type = ITEM_MONEY;
	for (size_t denomination = 0; denomination < death.wallet_before.size(); ++denomination)
		wallet.values[denomination] = death.wallet_before[denomination];
	death.corpse.push_back(wallet);

	// The refused row is still attributed to the player; the wallet pile the
	// conversion never committed has no ledger row at all.
	death.custody.push_back({ { 100, 100, 0, 1, 500, item_custody_state::active },
				  { item_owner_type::player, 42, 0 },
				  5 });
	death.custody.push_back({ { 101, 100, 100, 1, 501, item_custody_state::active },
				  { item_owner_type::player, 42, 0 },
				  5 });
	death.custody.push_back(
		{ { death.wallet_pile_uid, death.wallet_pile_uid, 0, ITEM_TRANSFER_ABSENT_REVISION,
		    VOBJ_COINS, item_custody_state::absent },
		  {},
		  0 });
	snapshot.encoded_size_bound = 8192;
	return snapshot;
}

/** Create a minimal status-only snapshot with the revision, level, and room under test. */
static player_snapshot make_status(player_revision_t revision, int level, int room)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 42;
	snapshot.revision = revision;
	snapshot.components = PLAYER_COMPONENT_STATUS;
	snapshot.save_intent = 1;
	snapshot.room_vnum = room;
	snapshot.encoded_size_bound = 1024;
	snapshot.status_integers.push_back({ player_status_field::level, level, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Player" });
	snapshot.recipes_are_external = true;
	return snapshot;
}

// Read existing synthetic authority without recovery or mutation. Used by the
// full-world journey to compare item identities across real server restarts.
static void inspect_authority(const std::string &root, int32_t pid, bool item_only = false)
{
	std::string error;
	flatfile_authority_lock lock;
	require(lock.acquire(root, &error), "inspect authority lock: " + error);
	for (const char *journal : { ".critical-authority-transaction",
				     ".player-domain-transaction", ".currency-transaction" })
		require(!fs::exists(fs::path(root) / "domains" / journal),
			std::string("inspect refuses pending recovery: ") + journal);
	player_snapshot snapshot;
	require(flatfile_player_snapshot_read(root, pid, &snapshot, &error) ==
			flatfile_player_load_result::ok,
		"inspect player snapshot: " + error);
	flatfile_identity_record identity;
	require(flatfile_identity_lookup_pid(root, pid, &identity, &error) ==
			flatfile_identity_result::ok,
		"inspect identity: " + error);
	flatfile_player_domain_record domains = {};
	if (!item_only)
		require(flatfile_player_domain_load_locked(root, lock, pid, identity.account,
							   identity.racewar, &domains, &error) ==
				flatfile_player_domain_result::ok,
			"inspect wallet: " + error);
	std::cout << "{\"revision\":" << snapshot.revision << ",\"intent\":" << snapshot.save_intent
		  << ",\"room\":" << snapshot.room_vnum;
	std::cout << ",\"snapshot_uids\":[";
	for (size_t i = 0; i < snapshot.items.size(); ++i)
	{
		if (i)
			std::cout << ',';
		std::cout << snapshot.items[i].object_uid;
	}
	std::cout << "],\"wallet\":[";
	for (size_t i = 0; i < 4; ++i)
	{
		if (i)
			std::cout << ',';
		std::cout << domains.domains.wallet[i];
	}
	std::cout << "],\"wallet_revision\":" << domains.domains.wallet_revision;
	std::cout << ",\"bank\":[";
	for (size_t i = 0; i < 4; ++i)
	{
		if (i)
			std::cout << ',';
		std::cout << domains.domains.bank[i];
	}
	std::cout << "],\"bank_revision\":" << domains.domains.bank_revision;
	for (const auto &field : snapshot.status_integers)
		if (field.field == player_status_field::wimpy)
			std::cout
				<< ",\"wimpy\":"
				<< (field.is_unsigned ? field.unsigned_value : field.signed_value);
	for (const auto &field : snapshot.status_integers)
	{
		const char *name = field.field == player_status_field::deaths	  ? "death_count" :
				   field.field == player_status_field::experience ? "experience" :
				   field.field == player_status_field::level	  ? "level" :
										    nullptr;
		if (name)
			std::cout
				<< ",\"" << name << "\":"
				<< (field.is_unsigned ? field.unsigned_value : field.signed_value);
	}
	for (const auto &owner :
	     { item_owner_identity{ item_owner_type::player, static_cast<uint64_t>(pid), 0 },
	       item_owner_identity{ item_owner_type::room,
				    static_cast<uint64_t>(snapshot.room_vnum), 0 } })
	{
		uint64_t revision = 0;
		std::vector<flatfile_item_ownership_record> items;
		const auto result = flatfile_item_repository_load_owner_locked(
			root, lock, owner, &revision, &items, &error);
		require(result == flatfile_item_repository_result::ok ||
				result == flatfile_item_repository_result::not_found,
			"inspect item owner: " + error);
		std::cout << (owner.type == item_owner_type::player ? ",\"player_items\":[" :
								      ",\"room_items\":[");
		bool first = true;
		for (const auto &item : items)
		{
			if (!first)
				std::cout << ',';
			first = false;
			std::cout << "{\"uid\":" << item.item_uid << ",\"vnum\":" << item.vnum
				  << ",\"root\":" << item.root_item_uid
				  << ",\"parent\":" << item.parent_item_uid << '}';
		}
		std::cout << ']';
		std::cout
			<< (owner.type == item_owner_type::player ? ",\"player_owner_revision\":" :
								    ",\"room_owner_revision\":")
			<< revision;
	}
	std::cout << ",\"deaths\":[";
	bool first_death = true;
	const fs::path deaths = flatfile_player_snapshot_file::death_directory(root);
	if (fs::exists(deaths))
		for (const auto &file : fs::directory_iterator(deaths))
		{
			if (!file.path().filename().string().starts_with(std::to_string(pid) +
									 "-") ||
			    file.path().extension() != ".death")
				continue;
			player_snapshot disposition;
			require(flatfile_player_snapshot_read_file(
					deaths.string(), file.path().filename().string(), pid,
					&disposition, &error) == flatfile_player_load_result::ok &&
					disposition.death,
				"inspect death: " + error);
			if (!first_death)
				std::cout << ',';
			first_death = false;
			std::cout << "{\"revision\":" << disposition.revision << ",\"items\":[";
			bool first_item = true;
			for (const auto &item : disposition.death->corpse)
			{
				if (!first_item)
					std::cout << ',';
				first_item = false;
				std::cout << "{\"uid\":" << item.object_uid
					  << ",\"vnum\":" << item.vnum
					  << ",\"parent\":" << item.parent_index << ",\"coins\":[";
				for (size_t i = 0; i < 4; ++i)
				{
					if (i)
						std::cout << ',';
					std::cout << (item.type == ITEM_MONEY ? item.values[i] : 0);
				}
				std::cout << "]}";
			}
			std::cout << "],\"custody\":[";
			first_item = true;
			for (const auto &item : disposition.death->custody)
			{
				if (!first_item)
					std::cout << ',';
				first_item = false;
				std::cout << "{\"uid\":" << item.item.item_uid
					  << ",\"root\":" << item.item.root_item_uid
					  << ",\"parent\":" << item.item.parent_item_uid
					  << ",\"owner_type\":"
					  << static_cast<unsigned>(item.owner.type)
					  << ",\"owner_id\":" << item.owner.id << '}';
			}
			std::cout << "]}";
		}
	std::cout << "]}\n";
}

static void coin_player_matrix(const fs::path &path)
{
	const std::string root = path.string();
	for (const auto &directory : { path, path / "players", path / "domains",
				       path / "identities", path / "identities/names" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	std::string error;
	int32_t pid = 0;
	for (int32_t i = 1; i <= 42; ++i)
		require(flatfile_identity_allocate_pid(root, &pid, &error) ==
				flatfile_identity_result::ok,
			"coin player identity allocation");
	require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"coin player identity claim");
	auto snapshot = make_full(1);
	snapshot.items[0].equipment_slot = 5;
	snapshot.items[1].vnum = VOBJ_COINS;
	snapshot.items[1].type = ITEM_MONEY;
	snapshot.items[1].values[0] = 50;
	snapshot.items[1].name = "legacy coins";
	snapshot.items[1].string_mask = 1;
	require(flatfile_player_snapshot_apply(root, snapshot, &error).outcome ==
			player_save_apply_outcome::applied,
		"coin player snapshot baseline: " + error);
	const item_owner_identity owner = { item_owner_type::player, 42, 0 };
	for (int32_t before : { 50, 20 })
	{
		const int32_t after = before == 50 ? 20 : 0;
		flatfile_player_domain_record domain;
		require(flatfile_player_domain_load(root, 42, "Account-One", 0, &domain, &error) ==
				flatfile_player_domain_result::ok,
			"coin player domain load");
		uint64_t owner_revision = 0, destroyed_revision = 0;
		std::vector<flatfile_item_ownership_record> owned, destroyed;
		require(flatfile_item_repository_load_owner(root, owner, &owner_revision, &owned,
							    &error) ==
				flatfile_item_repository_result::ok,
			"coin player custody load");
		require(std::any_of(owned.begin(), owned.end(), [](const auto &item)
				    { return item.item_uid == 100 && item.equipment_slot == 5; }),
			"equipped player baseline slot missing");
		flatfile_item_repository_load_owner(root, { item_owner_type::destruction, 0, 0 },
						    &destroyed_revision, &destroyed, &error);
		const auto found = std::find_if(owned.begin(), owned.end(), [](const auto &item)
						{ return item.item_uid == 101; });
		require(found != owned.end(), "coin player pile custody missing");
		coin_transfer_payload payload;
		payload.source.before[0] = before;
		payload.source.after[0] = after;
		item_transfer_payload pile = {};
		pile.from_owner = owner;
		pile.to_owner = after ? owner :
					item_owner_identity{ item_owner_type::destruction, 0, 0 };
		pile.expected_from_revision = owner_revision;
		pile.expected_to_revision = after ? owner_revision : destroyed_revision;
		pile.reason = after ? item_transfer_reason::player_put :
				      item_transfer_reason::destruction;
		pile.selected_item_uid = 101;
		pile.target_root_item_uid = after ? 100 : 101;
		pile.target_parent_item_uid = after ? 100 : 0;
		pile.expected_target_parent_revision = after ? 1 : 0;
		pile.item_count = 1;
		pile.items[0] = { 101,	      100,
				  100,	      found->item_revision,
				  VOBJ_COINS, item_custody_state::active };
		auto item = snapshot.items[1];
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		item.equipment_slot = -1;
		item.values[0] = after ? after : before;
		std::vector<uint8_t> blob;
		require(player_item_snapshot_list_encode({ item }, &blob) ==
				player_snapshot_codec_result::ok,
			"coin player encode");
		pile.item_blob_size = blob.size();
		std::copy(blob.begin(), blob.end(), pile.item_blob.begin());
		critical_operation_id id;
		require(critical_operation_id_generate(&id), "coin player operation id");
		require(item_transfer_command_build(&payload.source.change, id, pile,
						    critical_source_site::command,
						    critical_deadline_class::interactive),
			"coin player pile command");
		currency_command_payload currency = {};
		currency.pid = 42;
		currency.reason = currency_reason_type::coin_transfer;
		strcpy(currency.account_name.data(), "Account-One");
		currency.wallet_delta.amount[0] = before - after;
		for (size_t denomination = 0; denomination < 4; ++denomination)
			payload.destination.before[denomination] =
				payload.destination.after[denomination] =
					domain.domains.wallet[denomination];
		payload.destination.after[0] += before - after;
		require(currency_command_build(&payload.destination.change, id, currency,
					       domain.domains.wallet_revision,
					       domain.domains.bank_revision,
					       critical_source_site::command,
					       critical_deadline_class::interactive),
			"coin player wallet command");
		critical_command command;
		require(coin_transfer_command_build(&command, id, payload,
						    critical_source_site::command,
						    critical_deadline_class::interactive),
			"coin player command");
		command.accepted_at_usec = 1;
		auto apply = [&]()
		{
			return flatfile_critical_command_repository_apply_selected(
				command, const_cast<char *>(root.c_str()));
		};
		const auto applied = apply();
		require(applied.outcome == critical_apply_outcome::applied &&
				apply().outcome == critical_apply_outcome::already_applied,
			"coin player pickup failed: " + std::to_string(applied.error_code));
		// Saving an old projection cannot undo either the durable remainder or retirement.
		++snapshot.revision;
		require(flatfile_player_snapshot_apply(root, snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"coin stale snapshot write");
		player_load_request request = {};
		request.request_id = 1;
		request.pid = 42;
		request.account_name = "Account-One";
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		const auto loaded = flatfile_player_load_repository_execute(root, request);
		require(loaded.outcome == player_load_outcome::applied && !loaded.stale_item_rows &&
				loaded.domains.wallet[0] == static_cast<uint64_t>(61 - after) &&
				loaded.snapshot.items.size() == (after ? 2u : 1u) &&
				loaded.snapshot.pets[0].items.size() == 1,
			"coin player re-entry failed or restored old money: " +
				std::to_string(loaded.error_code));
		if (after)
			require(loaded.snapshot.items[1].object_uid == 101 &&
					loaded.snapshot.items[1].values[0] == after &&
					loaded.snapshot.items[1].parent_index == 0,
				"coin player re-entry lost authoritative remainder/topology");
	}
	std::cout << "flatfile legacy player coin pickup, replay and re-entry passed\n";
}

/** Inspect synthetic authority on request, otherwise exercise player repository durability and recovery. */
static void spell_receipt_matrix(const fs::path &path)
{
	const std::string root = path.string();
	for (const auto &directory :
	     { path, path / "players", path / "domains", path / "identities",
	       path / "identities/names", path / "player-deaths" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	std::string error;
	int32_t pid = 0;
	for (int32_t i = 1; i <= 42; ++i)
		require(flatfile_identity_allocate_pid(root, &pid, &error) ==
				flatfile_identity_result::ok,
			"spell identity allocation");
	require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"spell identity claim");
	require(flatfile_player_snapshot_apply(root, make_full(1), &error).outcome ==
			player_save_apply_outcome::applied,
		"spell player baseline: " + error);
	auto effect = make_full(2);
	effect.schema_version = PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
	effect.components = PLAYER_COMPONENT_AFFECTS;
	effect.affects[0].type = 333;
	effect.affects[0].duration = 20;
	player_spell_effect_receipt_snapshot receipt = {};
	receipt.operation_id.bytes[0] = 0xa5;
	receipt.effect_id = 6;
	effect.spell_effect_receipts.push_back(receipt);
	const fs::path receipt_path = path / "players/42-a5000000000000000000000000000000.spell";
	player_load_request request;
	request.pid = 42;
	request.account_name = "Account-One";
	request.request_id = 1;
	request.pending_spell_effect_operations.push_back(receipt.operation_id);
	const auto load = [&]
	{
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		return flatfile_player_load_repository_execute(root, request);
	};
	setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
	require(flatfile_player_snapshot_apply(root, effect, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"effect acknowledged a refused authority commit");
	unsetenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT");
	auto loaded = load();
	require(loaded.outcome == player_load_outcome::applied && loaded.snapshot.revision == 1 &&
			loaded.spell_effect_receipts.empty() && !fs::exists(receipt_path),
		"refused effect changed player or left a phantom receipt");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_player_snapshot_apply(root, effect, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"effect acknowledged an interrupted authority commit");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	// A fresh process must recover player and receipt together before returning either.
	const pid_t child = fork();
	require(child >= 0, "spell restart fork");
	if (!child)
	{
		const auto restarted = load();
		_exit(restarted.outcome == player_load_outcome::applied &&
				      restarted.snapshot.revision == 2 &&
				      restarted.snapshot.affects[0].type == 333 &&
				      restarted.snapshot.affects[0].duration == 20 &&
				      restarted.spell_effect_receipts.size() == 1 &&
				      restarted.spell_effect_receipts[0].effect_id == 6 ?
			      0 :
			      1);
	}
	int status = 0;
	require(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status),
		"separate-process effect recovery lost its affect or receipt");
	require(flatfile_player_snapshot_apply(root, effect, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"effect exact replay failed");
	read_failure_filename = "42-a5000000000000000000000000000000.spell";
	const auto failed_replay = flatfile_player_snapshot_apply(root, effect, &error);
	require(failed_replay.outcome == player_save_apply_outcome::retryable_failure &&
			failed_replay.error_code == EIO &&
			load().outcome == player_load_outcome::retryable_failure,
		"receipt read I/O error became a durable acknowledgment or terminal corruption");
	read_failure_filename = nullptr;
	std::ifstream receipt_file(receipt_path, std::ios::binary);
	const std::string original((std::istreambuf_iterator<char>(receipt_file)),
				   std::istreambuf_iterator<char>());
	receipt_file.close();
	fs::remove(receipt_path);
	require(flatfile_player_snapshot_apply(root, effect, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"bare revision acknowledged a missing spell receipt");
	{
		std::ofstream restore(receipt_path, std::ios::binary);
		restore.write(original.data(), original.size());
	}
	fs::permissions(receipt_path, fs::perms::owner_read | fs::perms::owner_write,
			fs::perm_options::replace);
	auto conflicting = effect;
	conflicting.spell_effect_receipts[0].effect_id = 1;
	require(flatfile_player_snapshot_apply(root, conflicting, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"exact replay accepted a conflicting spell effect");
	auto ordinary = make_status(3, 51, 1202);
	ordinary.status_integers.push_back({ player_status_field::racewar, 0, 0, false });
	require(flatfile_player_snapshot_apply(root, ordinary, &error).outcome ==
			player_save_apply_outcome::applied,
		"ordinary checkpoint after spell receipt failed");
	const auto obsolete = flatfile_player_snapshot_apply(root, effect, &error);
	require(obsolete.outcome == player_save_apply_outcome::stale_revision &&
			obsolete.operation_receipts_verified &&
			!player_save_result_matches_exact_request(effect, obsolete),
		"obsolete spell receipt was not verified separately from live ACK");
	require(flatfile_player_snapshot_apply(root, conflicting, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"newer counter hid a conflicting historical spell receipt");
	// An unrelated corrupt historical file cannot consume the pending-operation budget.
	{
		std::ofstream unrelated(path / "players/42-ff000000000000000000000000000000.spell");
		unrelated << "unrelated protected history";
	}
	loaded = load();
	require(loaded.outcome == player_load_outcome::applied && loaded.snapshot.revision == 3 &&
			loaded.spell_effect_receipts.size() == 1 &&
			loaded.snapshot.spell_effect_receipts.empty(),
		"ordinary save discarded receipt history or load scanned unrelated history");
	conflicting.revision = 4;
	require(flatfile_player_snapshot_apply(root, conflicting, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"a later effect save overwrote a conflicting operation receipt");
	{
		std::fstream corrupt(receipt_path, std::ios::in | std::ios::out | std::ios::binary);
		corrupt.seekp(-1, std::ios::end);
		corrupt.put(static_cast<char>(original.back() ^ 0x5a));
	}
	require(load().outcome == player_load_outcome::component_failure,
		"corrupt requested receipt allowed effect recovery");
	{
		std::ofstream restore(receipt_path, std::ios::binary);
		restore.write(original.data(), original.size());
	}
	// A valid receipt newer than the restored player file is inconsistent evidence.
	// It cannot authorize a later save to acknowledge the effect without recovery.
	const fs::path player_path = path / "players/42.snapshot";
	std::ifstream player_file(player_path, std::ios::binary);
	const std::string prior_player((std::istreambuf_iterator<char>(player_file)),
				       std::istreambuf_iterator<char>());
	player_file.close();
	auto future = effect;
	future.revision = 4;
	future.spell_effect_receipts[0].operation_id.bytes[0] = 0xc7;
	require(flatfile_player_snapshot_apply(root, future, &error).outcome ==
			player_save_apply_outcome::applied,
		"future receipt fixture commit failed");
	{
		std::ofstream restore(player_path, std::ios::binary);
		restore.write(prior_player.data(), prior_player.size());
	}
	request.pending_spell_effect_operations.push_back(
		future.spell_effect_receipts[0].operation_id);
	require(load().outcome == player_load_outcome::component_failure,
		"receipt ahead of restored player state allowed effect recovery");
	future.revision = 5;
	require(flatfile_player_snapshot_apply(root, future, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"a later save acknowledged a receipt ahead of prior durable player state");
	request.pending_spell_effect_operations.pop_back();
	fs::remove(path / "players/42-c7000000000000000000000000000000.spell");
	auto death = make_death(4);
	death.schema_version = PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
	receipt.operation_id.bytes[0] = 0xb6;
	death.spell_effect_receipts.push_back(receipt);
	death.affects[0].type = 333;
	death.affects[0].duration = 30;
	request.pending_spell_effect_operations.push_back(receipt.operation_id);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_player_snapshot_apply(root, death, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"death effect acknowledged an interrupted authority commit");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	loaded = load();
	require(loaded.outcome == player_load_outcome::applied && loaded.snapshot.revision == 4 &&
			loaded.snapshot.items.empty() &&
			loaded.snapshot.affects[0].duration == 30 &&
			loaded.spell_effect_receipts.size() == 2,
		"death receipt, player state or custody did not recover together: outcome=" +
			std::to_string(static_cast<unsigned>(loaded.outcome)) + " component=" +
			(loaded.failed_component ? loaded.failed_component : "none") +
			" revision=" + std::to_string(loaded.snapshot.revision) +
			" receipts=" + std::to_string(loaded.spell_effect_receipts.size()));
	require(flatfile_player_snapshot_apply(root, death, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"death spell receipt exact replay failed");
	read_failure_filename = "42-4.death";
	const auto failed_death_replay = flatfile_player_snapshot_apply(root, death, &error);
	require(failed_death_replay.outcome == player_save_apply_outcome::retryable_failure &&
			failed_death_replay.error_code == EIO,
		"death evidence read I/O error became terminal corruption");
	read_failure_filename = nullptr;
	auto wrong_death = death;
	wrong_death.affects[0].duration = 31;
	require(flatfile_player_snapshot_apply(root, wrong_death, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"changed death bytes were acknowledged by revision alone");
	std::cout
		<< "flat spell receipts: affect/save atomicity, separate-process recovery, scoped history, corruption/conflict refusal, death retry\n";
}

static void craft_progression_matrix(const fs::path &path, bool retain_receipt = false)
{
	const auto root = path.string();
	for (const auto &directory :
	     { path, path / "players", path / "domains", path / "identities",
	       path / "identities/names", path / "player-deaths" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	std::string error;
	int32_t pid = 0;
	for (int index = 0; index < 42; ++index)
		require(flatfile_identity_allocate_pid(root, &pid, &error) ==
				flatfile_identity_result::ok,
			"craft fixture identity");
	require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"craft fixture name");
	auto baseline = make_full(1);
	baseline.pets.clear();
	baseline.status_integers.push_back({ player_status_field::experience, 100, 0, false });
	require(flatfile_player_snapshot_apply(root, baseline, &error).outcome ==
			player_save_apply_outcome::applied,
		"craft baseline: " + error);
	auto output = baseline.items.back();
	output.object_uid = 200;
	output.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	output.equipment_slot = 0;
	std::vector<uint8_t> encoded;
	require(player_item_snapshot_list_encode({ output }, &encoded) ==
			player_snapshot_codec_result::ok,
		"craft output encoding");
	item_transfer_payload payload = {};
	payload.from_owner = payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.expected_from_revision = payload.expected_to_revision = 1;
	payload.reason = item_transfer_reason::craft;
	payload.reason_id = output.vnum;
	payload.multi_root = true;
	payload.selected_item_uid = output.object_uid;
	payload.item_count = 2;
	payload.items[0] = { 100, 100, 0, 1, 500, item_custody_state::active };
	payload.items[1] = { 101, 100, 100, 1, 501, item_custody_state::active };
	payload.item_blob_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), payload.item_blob.begin());
	craft_recipe_continuation terms;
	terms.player_pid = 42;
	terms.recipe_vnum = output.vnum;
	terms.output_uid = 200;
	terms.experience = 7000;
	payload.continuation.kind = item_transfer_continuation_kind::craft_recipe;
	require(craft_recipe_continuation_encode(terms, &payload.continuation.data), "craft terms");
	critical_operation_id operation = {};
	operation.bytes[0] = 0xf1;
	critical_command command = {};
	require(item_transfer_command_build(&command, operation, payload,
					    critical_source_site::operator_repair,
					    critical_deadline_class::interactive),
		"craft command");
	command.accepted_at_usec = 1;
	const auto obligation =
		path / "players" / flatfile_craft_receipt_filename(42, operation, true);
	const auto applied_receipt =
		path / "players" / flatfile_craft_receipt_filename(42, operation);
	setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
	require(flatfile_item_repository_apply(root, command).outcome ==
			critical_apply_outcome::retryable_failure,
		"craft root refusal");
	unsetenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT");
	require(!fs::exists(obligation), "refused craft left a progression obligation");
	require(flatfile_item_repository_apply(root, command).outcome ==
			critical_apply_outcome::applied,
		"craft root commit: " + error);
	require(fs::exists(obligation) && !fs::exists(applied_receipt),
		"craft root and award were conflated");
	auto checkpoint = baseline;
	checkpoint.schema_version = PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
	checkpoint.revision = 2;
	checkpoint.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS |
				PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
	checkpoint.status_integers.back().signed_value = 7100;
	checkpoint.craft_receipts.push_back({ operation, 1, 7000 });
	player_load_request request;
	request.pid = 42;
	request.account_name = "Account-One";
	request.request_id = 1;
	request.pending_craft_operations = { operation };
	const auto load = [&]
	{
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		return flatfile_player_load_repository_execute(root, request);
	};
	setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
	require(flatfile_player_snapshot_apply(root, checkpoint, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"craft save refusal: " + error);
	unsetenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT");
	auto loaded = load();
	require(loaded.outcome == player_load_outcome::applied && loaded.snapshot.revision == 1 &&
			loaded.craft_receipts.empty() && !fs::exists(applied_receipt),
		"failed save acknowledged progression");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_player_snapshot_apply(root, checkpoint, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"interrupted craft save: " + error);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	loaded = load();
	require(loaded.outcome == player_load_outcome::applied && loaded.snapshot.revision == 2 &&
			loaded.craft_receipts.size() == 1 &&
			loaded.craft_receipts[0].experience == 7000,
		"craft player save and receipt did not recover together");
	require(flatfile_player_snapshot_apply(root, checkpoint, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"craft save exact replay");
	auto wrong = checkpoint;
	++wrong.craft_receipts[0].experience;
	require(flatfile_player_snapshot_apply(root, wrong, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"changed craft terms acknowledged");
	std::ifstream receipt_input(applied_receipt, std::ios::binary);
	const std::vector<char> receipt_bytes{ std::istreambuf_iterator<char>(receipt_input),
					       std::istreambuf_iterator<char>() };
	receipt_input.close();
	fs::remove(applied_receipt);
	require(flatfile_player_snapshot_apply(root, checkpoint, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"stale craft save manufactured a missing receipt");
	if (retain_receipt)
	{
		std::ofstream receipt_output(applied_receipt, std::ios::binary);
		receipt_output.write(receipt_bytes.data(), receipt_bytes.size());
		require(receipt_output.good(), "craft fixture receipt restore");
	}
	std::cout
		<< "flat craft progression: root obligation, failed save, interrupted recovery, scoped receipts and replay passed\n";
}

static void quest_xp_matrix(const fs::path &path, uint32_t version, bool group = false,
			    bool terminal = false)
{
	const std::string root = path.string();
	const auto read_bytes = [](const fs::path &file)
	{
		std::ifstream input(file, std::ios::binary);
		require(input.good(), "quest snapshot read");
		return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
					    std::istreambuf_iterator<char>());
	};
	const auto write_bytes = [](const fs::path &file, const std::vector<uint8_t> &bytes)
	{
		std::ofstream output(file, std::ios::binary | std::ios::trunc);
		output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
		require(output.good(), "quest snapshot restore");
	};
	for (const auto &directory :
	     { path, path / "players", path / "domains", path / "identities",
	       path / "identities/names", path / "player-deaths" })
	{
		fs::create_directories(directory);
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	}
	std::string error;
	int32_t pid = 0;
	for (int32_t i = 1; i <= 43; ++i)
		require(flatfile_identity_allocate_pid(root, &pid, &error) ==
				flatfile_identity_result::ok,
			"quest identity allocation");
	require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
				flatfile_identity_result::ok &&
			flatfile_identity_claim(root, 43, "Peer", "Account-One", &error) ==
				flatfile_identity_result::ok,
		"quest identity claims");
	auto baseline = make_full(1);
	if (terminal)
	{
		baseline.pets.clear();
		auto held = baseline.items[1];
		held.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		held.object_uid = 103;
		baseline.items.push_back(held);
	}
	baseline.status_integers.push_back({ player_status_field::experience, 100, 0, false });
	require(flatfile_player_snapshot_apply(root, baseline, &error).outcome ==
			player_save_apply_outcome::applied,
		"quest owner baseline");
	auto peer = baseline;
	peer.pid = 43;
	peer.items.clear();
	peer.pets.clear();
	peer.status_strings[0].value = "Peer";
	require(flatfile_player_snapshot_apply(root, peer, &error).outcome ==
			player_save_apply_outcome::applied,
		"quest peer baseline");
	const auto player_bytes_before = read_bytes(path / "players/42.snapshot");
	item_transfer_payload offering = {};
	offering.from_owner = { item_owner_type::player, 42, 0 };
	offering.to_owner = { item_owner_type::destruction, 0, 0 };
	offering.reason = item_transfer_reason::quest_turnin;
	offering.reason_id = 711;
	offering.expected_from_revision = 1;
	offering.multi_root = true;
	offering.item_count = 2;
	offering.items[0] = { 100, 100, 0, 1, 500, item_custody_state::active };
	offering.items[1] = { 101, 100, 100, 1, 501, item_custody_state::active };
	offering.continuation.kind = item_transfer_continuation_kind::quest_offering;
	auto &data = offering.continuation.data;
	const auto append32 = [&](uint32_t value)
	{
		for (size_t byte = 0; byte < 4; ++byte)
			data.push_back(value >> (byte * 8));
	};
	const auto append64 = [&](uint64_t value)
	{
		for (size_t byte = 0; byte < 8; ++byte)
			data.push_back(value >> (byte * 8));
	};
	for (auto value : { version, 42U, 0U, 0U, 711U, 500U })
		append32(value);
	append64(1700000000);
	append32(1);
	append64(100);
	append32(1);
	for (auto value : { 5U, 100U, 0U, 75U })
		append32(value);
	const uint32_t count = group ? 2 : 1;
	for (auto value : { 3U, 50U, 0U, count, 50U, count })
		append32(value);
	append32(42);
	if (count == 2)
		append32(43);
	append32(6);
	data.insert(data.end(), { 'P', 'l', 'a', 'y', 'e', 'r' });
	append32(8);
	data.insert(data.end(), { 'q', 'u', 'e', 's', 't', '-', 'i', 'd' });
	if (version == 5)
	{
		append32(count);
		for (auto value : { 42U, 0U, 75U })
			append32(value);
		if (group)
			for (auto value : { 43U, 0U, 50U })
				append32(value);
	}
	quest_reward_continuation terms;
	require(quest_reward_continuation_decode(data.data(), data.size(), &terms),
		"quest continuation fixture");
	critical_operation_id operation = {};
	operation.bytes[0] = 0xd2;
	critical_command command = {};
	require(item_transfer_command_build(&command, operation, offering,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"quest offering build");
	command.accepted_at_usec = 100;
	require(flatfile_item_repository_apply(root, command).outcome ==
				critical_apply_outcome::applied &&
			flatfile_item_repository_apply(root, command).outcome ==
				critical_apply_outcome::already_applied,
		"current quest offering did not commit or replay");
	const auto catalog_before_xp = read_bytes(path / "domains/item_ownership");
	player_load_request request;
	request.account_name = "Account-One";
	request.request_id = 1;
	const auto load = [&](int32_t target)
	{
		request.pid = target;
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		return flatfile_player_load_repository_execute(root, request);
	};
	auto loaded = load(42);
	require(loaded.outcome == player_load_outcome::applied && !loaded.degraded_components &&
			loaded.pending_quest_rewards.size() == 1 &&
			loaded.pending_quest_rewards[0].xp_applied_mask == 0,
		"current continuation was not recoverable");
	require(flatfile_item_repository_ack_quest_reward(root, 42, operation, &error) ==
			flatfile_item_repository_result::invalid,
		"unpaid owner XP acknowledged");
	auto reward = baseline;
	reward.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
	reward.revision = 2;
	reward.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
	reward.status_integers.back().signed_value = 175;
	if (terminal)
	{
		reward = make_death(2);
		reward.schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
		reward.status_integers.push_back(
			{ player_status_field::experience, 175, 0, false });
		reward.death->wallet_pile_uid = 0;
		reward.death->wallet_before = {};
		reward.death->corpse.resize(1);
		auto held = baseline.items.back();
		held.parent_index = 0;
		reward.death->corpse.push_back(held);
		reward.death->custody.clear();
		reward.death->custody.push_back(
			{ { 103, 103, 0, 1, 501, item_custody_state::active },
			  { item_owner_type::player, 42, 0 },
			  2 });
	}
	reward.quest_xp_receipts.push_back({ operation, 0, 75 });
	auto bad = reward;
	bad.pid = 44;
	bad.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	bad.items.clear();
	bad.pets.clear();
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
				player_save_apply_outcome::terminal_failure &&
			read_bytes(path / "domains/item_ownership") == catalog_before_xp &&
			!fs::exists(path / "players/44.snapshot"),
		"XP save recreated missing player authority");
	bad = reward;
	bad.quest_xp_receipts[0].amount = 76;
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"changed XP amount accepted");
	bad = reward;
	bad.components = PLAYER_COMPONENT_TROPHIES;
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"XP receipt without status accepted");
	bad = reward;
	bad.quest_xp_receipts[0].offering_operation.bytes[0] = 0xe2;
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"unknown XP offering accepted");
	if (version == 5)
	{
		bad = reward;
		bad.pid = 43;
		require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
				player_save_apply_outcome::terminal_failure,
			"foreign XP amount accepted");
		// Existing schema 12 can commit both receipt families with one player save.
		reward.schema_version = terminal ?
						PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION :
						PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
		reward.components |= PLAYER_COMPONENT_AFFECTS;
		player_spell_effect_receipt_snapshot spell = {};
		spell.operation_id.bytes[0] = 0xd3;
		spell.effect_id = 6;
		reward.spell_effect_receipts.push_back(spell);
		request.pending_spell_effect_operations.push_back(spell.operation_id);
	}
	setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"refused XP save acknowledged");
	unsetenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT");
	loaded = load(42);
	require(loaded.snapshot.revision == 1 && loaded.pending_quest_rewards.size() == 1 &&
			loaded.pending_quest_rewards[0].xp_applied_mask == 0 &&
			loaded.spell_effect_receipts.empty(),
		"refused XP save changed receipt or XP");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"interrupted XP save acknowledged");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	const pid_t child = fork();
	require(child >= 0, "quest restart fork");
	if (!child)
	{
		const auto restarted = load(42);
		_exit(restarted.outcome == player_load_outcome::applied &&
				      !restarted.degraded_components &&
				      restarted.snapshot.revision == 2 &&
				      restarted.snapshot.status_integers.back().signed_value ==
					      175 &&
				      restarted.pending_quest_rewards.size() == 1 &&
				      restarted.pending_quest_rewards[0].xp_applied_mask == 1 &&
				      restarted.spell_effect_receipts.size() ==
					      (version == 5 ? 1U : 0U) ?
			      0 :
			      1);
	}
	int status = 0;
	require(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status),
		"separate-process XP recovery lost XP or application marker");
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"exact XP replay failed");
	read_failure_filename = "item_ownership";
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"XP read error was acknowledged");
	read_failure_filename = nullptr;
	bad = reward;
	bad.quest_xp_receipts[0].amount = 76;
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"changed XP replay acknowledged");
	const auto player_bytes_after = read_bytes(path / "players/42.snapshot");
	const auto catalog_after_xp = read_bytes(path / "domains/item_ownership");
	if (terminal)
	{
		flatfile_item_ownership_record held;
		require(flatfile_item_repository_lookup_uid(root, 103, &held, &error) ==
					flatfile_item_repository_result::ok &&
				held.state == item_custody_state::quarantined &&
				held.item_revision == 2,
			"combined XP marker and custody image lost quarantine");
		player_snapshot disposition;
		require(flatfile_player_snapshot_read_file(
				flatfile_player_snapshot_file::death_directory(root),
				flatfile_player_snapshot_file::death_filename(42, 2), 42,
				&disposition, &error) == flatfile_player_load_result::ok &&
				disposition.schema_version ==
					PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION &&
				disposition.quest_xp_receipts.size() == 1 &&
				disposition.spell_effect_receipts.size() == 1,
			"immutable death disposition lost XP or spell receipt");
	}
	write_bytes(path / "domains/item_ownership", catalog_before_xp);
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"missing exact XP application marker acknowledged");
	write_bytes(path / "domains/item_ownership", catalog_after_xp);
	write_bytes(path / "players/42.snapshot", player_bytes_before);
	loaded = load(42);
	require(loaded.outcome != player_load_outcome::applied ||
			(loaded.degraded_components & PLAYER_LOAD_DEGRADED_RECOVERY),
		"player restored behind XP receipt did not hold recovery");
	bad = reward;
	bad.revision = 3;
	require(flatfile_player_snapshot_apply(root, bad, &error).outcome ==
			player_save_apply_outcome::terminal_failure,
		"future XP marker accepted");
	write_bytes(path / "players/42.snapshot", player_bytes_after);
	require(flatfile_item_repository_ack_quest_reward(root, 42, operation, &error) ==
			flatfile_item_repository_result::ok,
		"paid owner XP acknowledgement failed");
	require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"acknowledged XP replay failed");
	if (group)
	{
		loaded = load(43);
		require(!loaded.degraded_components && loaded.pending_quest_rewards.empty() &&
				loaded.pending_quest_xp_entitlements.size() == 1 &&
				loaded.pending_quest_xp_entitlements[0].amount == 50,
			"owner acknowledgement erased peer entitlement");
		peer.revision = 2;
		peer.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
		peer.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
		peer.status_integers.back().signed_value = 150;
		peer.quest_xp_receipts.push_back({ operation, 0, 50 });
		require(flatfile_player_snapshot_apply(root, peer, &error).outcome ==
					player_save_apply_outcome::applied &&
				flatfile_player_snapshot_apply(root, peer, &error).outcome ==
					player_save_apply_outcome::already_applied,
			"peer XP save or exact replay failed after owner acknowledgement");
		loaded = load(43);
		require(!loaded.degraded_components && loaded.pending_quest_xp_entitlements.empty(),
			"paid peer entitlement recovered twice");
	}
	auto ordinary = baseline;
	ordinary.revision = 3;
	ordinary.components = PLAYER_COMPONENT_STATUS;
	ordinary.status_integers.back().signed_value = 175;
	require(flatfile_player_snapshot_apply(root, ordinary, &error).outcome ==
				player_save_apply_outcome::applied &&
			flatfile_player_snapshot_apply(root, reward, &error).outcome ==
				player_save_apply_outcome::stale_revision,
		"stale XP replay advanced receipt");
	if (!terminal)
	{
		const auto obsolete = flatfile_player_snapshot_apply(root, reward, &error);
		require(obsolete.operation_receipts_verified &&
				!player_save_result_matches_exact_request(reward, obsolete),
			"obsolete XP receipt was not verified separately from live ACK");
		write_bytes(path / "domains/item_ownership", catalog_before_xp);
		require(flatfile_player_snapshot_apply(root, reward, &error).outcome ==
				player_save_apply_outcome::terminal_failure,
			"newer counter hid missing historical XP marker");
		write_bytes(path / "domains/item_ownership", catalog_after_xp);
	}
	std::cout
		<< "flat quest XP v" << version << (group ? " group" : " solo")
		<< (terminal ? " death" : " ordinary")
		<< ": atomic save, recipient validation, process recovery, exact replay, restore refusal\n";
}

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static constexpr auto initialize_evidence =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
};

static void recovery_accounting(const std::string &root, critical_command *command,
				std::string *error)
{
	fs::create_directories(root + "/economic-evidence");
	fs::permissions(root + "/economic-evidence", fs::perms::owner_all);
	critical_operation_id lineage{}, epoch_id{}, control_id{};
	lineage.bytes[0] = 70;
	epoch_id.bytes[0] = 71;
	control_id.bytes[0] = 72;
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "recovery accounting lock");
	std::vector<flatfile_authority_operation> operations;
	const auto commit = [&]
	{
		require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
				flatfile_authority_transaction_result::ok,
			"accounting commit: " + *error);
		operations.clear();
	};
	const auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root, lock, &control, error) == 0,
			"accounting read");
		return control.revision;
	};
	require(flatfile_accounting_test_access::bootstrap(root, lock, lineage, control_id,
							   &operations, error) == 0,
		"accounting bootstrap");
	commit();
	flatfile_economic_epoch epoch;
	epoch.epoch = epoch_id;
	epoch.creating_operation = control_id;
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	require(flatfile_accounting_test_access::append_epoch(root, lock, revision(), epoch,
							      &operations, error) == 0,
		"accounting epoch");
	commit();
	require(flatfile_accounting_test_access::select_epoch(root, lock, revision(), true,
							      control_id, &operations, error) == 0,
		"accounting activation");
	commit();
	require(flatfile_accounting_test_access::initialize_evidence(
			root, lock, revision(), command->operation_id.bytes[0], control_id,
			&operations, error) == 0,
		"accounting bucket");
	commit();
	std::vector<uint8_t> intent;
	require(item_transfer_accounting_intent(*command, lineage, epoch_id, 42, &intent,
						economic_source_kind::starter_grant) ==
			economic_accounting_error::ok,
		"accounting intent");
	command->schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command->accounting_intent = std::move(intent);
}

static void check_quarantine_recovery(const std::string &base)
{
	for (int boundary = 0; boundary <= 4; ++boundary)
	{
		const std::string root = base + "/case-" + std::to_string(boundary);
		const std::string journal = root + "/journal";
		for (const auto &directory :
		     { root, journal, root + "/players", root + "/domains", root + "/metadata",
		       root + "/identities", root + "/identities/names" })
		{
			fs::create_directories(directory);
			fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
		}
		std::string error;
		int32_t pid;
		require(player_save_journal_init(journal.c_str()), "recovery journal");
		for (int index = 1; index <= 42; ++index)
			require(flatfile_identity_allocate_pid(root, &pid, &error) ==
					flatfile_identity_result::ok,
				"recovery allocate");
		require(flatfile_identity_claim(root, 42, "Player", "Account-One", &error) ==
				flatfile_identity_result::ok,
			"recovery claim");
		auto baseline = make_full(1);
		baseline.pets.clear();
		require(flatfile_player_snapshot_apply(root, baseline, &error).outcome ==
				player_save_apply_outcome::applied,
			"recovery baseline: " + error);
		item_transfer_payload grant{};
		grant.from_owner = { item_owner_type::system, 0, 0 };
		grant.to_owner = { item_owner_type::player, 42, 0 };
		std::vector<flatfile_item_ownership_record> owned;
		const auto system_loaded = flatfile_item_repository_load_owner(
			root, grant.from_owner, &grant.expected_from_revision, &owned, &error);
		require(system_loaded == flatfile_item_repository_result::ok ||
				system_loaded == flatfile_item_repository_result::not_found,
			"system revision");
		require(flatfile_item_repository_load_owner(
				root, grant.to_owner, &grant.expected_to_revision, &owned,
				&error) == flatfile_item_repository_result::ok,
			"player revision");
		player_item_snapshot item{};
		item.object_uid = 200;
		item.vnum = 806;
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		item.string_mask = 1;
		item.name = "retained grant";
		grant.reason = item_transfer_reason::creation;
		grant.reason_id = 664;
		grant.selected_item_uid = grant.target_root_item_uid = 200;
		grant.item_count = 1;
		grant.items[0] = { 200, 200,
				   0,	ITEM_TRANSFER_ABSENT_REVISION,
				   806, item_custody_state::absent };
		std::vector<uint8_t> blob;
		require(player_item_snapshot_list_encode({ item }, &blob) ==
				player_snapshot_codec_result::ok,
			"grant blob");
		grant.item_blob_size = blob.size();
		std::copy(blob.begin(), blob.end(), grant.item_blob.begin());
		critical_operation_id operation;
		critical_command command;
		require(critical_operation_id_generate(&operation) &&
				item_transfer_command_build(&command, operation, grant,
							    critical_source_site::operator_repair,
							    critical_deadline_class::interactive),
			"grant command");
		command.accepted_at_usec = 1;
		if (boundary == 4)
			recovery_accounting(root, &command, &error);
		const auto granted = flatfile_item_repository_apply(root, command);
		require(granted.outcome == critical_apply_outcome::applied, "native grant commit");
		auto stale = baseline;
		stale.revision = 2;
		for (auto &field : stale.status_integers)
			if (field.field == player_status_field::copper)
				field.signed_value = 9999;
		player_snapshot skills{};
		skills.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		skills.pid = 42;
		skills.revision = 3;
		skills.components = PLAYER_COMPONENT_SKILLS;
		skills.encoded_size_bound = 8192;
		skills.skills.push_back({ 9, 88, 1 });
		require(player_save_journal_append(stale) == player_save_journal_result::ok &&
				player_save_journal_append(skills) ==
					player_save_journal_result::ok,
			"retained component frames");
		player_save_journal_worker_terminal(stale, nullptr);
		require(player_save_journal_pid_quarantined(42), "initial fence");
		player_save_recovery_record record;
		auto wrong = command;
		++wrong.accepted_at_usec;
		require(!player_quarantine_recovery_prepare_flatfile(root, 42, { wrong }, "",
								     &record, &error),
			"changed command accepted");
		require(!player_quarantine_recovery_prepare_flatfile(root, 42, {}, "", &record,
								     &error),
			"missing command accepted");
		require(player_quarantine_recovery_prepare_flatfile(root, 42, { command }, "",
								    &record, &error),
			"native prepare: " + error);
		item_transfer_result receipt{};
		require(item_transfer_command_decode_result(granted.result_payload.data(),
							    granted.result_size, &receipt),
			"grant receipt");
		const auto native = flatfile_player_quarantine_inspect(
			root, player_quarantine_recovery_request(record));
		for (int refusal = 0; refusal < 7; ++refusal)
		{
			auto current = native;
			auto frames = std::vector<player_snapshot>{ stale, skills };
			if (refusal == 0)
				current.item_identities[0].item_revision = 2;
			if (refusal == 1)
				current.missing_payload_rows = 1;
			if (refusal == 2)
				frames[1].revision = frames[0].revision;
			if (refusal == 3)
				frames[0].schema_version =
					PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
			if (refusal == 4)
				frames[0].items.back().parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			if (refusal == 5)
				current.snapshot.items.back().vnum += 1;
			if (refusal == 6)
				current.item_identities.back().root_item_uid += 1;
			player_save_recovery_record rejected;
			require(!player_quarantine_recovery_build(current, frames,
								  { { command, receipt } },
								  &rejected, &error),
				"conflicting native/frame evidence accepted");
		}
		auto changed = record;
		++changed.replacement.skills[0].learned;
		require(flatfile_player_quarantine_apply(root, changed, &error).error_code == EPERM,
			"nonprepared replacement accepted");
		require(!player_quarantine_recovery_resume_flatfile(
				root, 42, "flatfile:wrong-generation", &error),
			"wrong backend accepted");
		require(flatfile_player_snapshot_apply(root, record.replacement, &error)
					.error_code == EPERM,
			"ordinary save bypassed fence");
		if (boundary)
		{
			player_save_journal_shutdown();
			const pid_t child = fork();
			require(child >= 0, "recovery fork");
			if (!child)
			{
				require(player_save_journal_init(journal.c_str()), "child journal");
				if (boundary == 4)
					require(flatfile_player_quarantine_apply(root, record,
										 &error)
								.outcome ==
							player_save_apply_outcome::applied,
						"child native commit");
				else
				{
					const char *fault =
						boundary == 1 ?
							"DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT" :
						boundary == 2 ?
							"DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL" :
							"DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION";
					setenv(fault, "1", 1);
					require(!player_quarantine_recovery_resume_flatfile(
							root, 42, "", &error),
						"fault did not interrupt recovery");
				}
				kill(getpid(), SIGKILL);
				_exit(2);
			}
			int status;
			require(waitpid(child, &status, 0) == child && WIFSIGNALED(status) &&
					WTERMSIG(status) == SIGKILL,
				"native process boundary");
			require(player_save_journal_init(journal.c_str()) &&
					player_save_journal_pid_quarantined(42),
				"interruption lost fence");
		}
		require(player_quarantine_recovery_resume_flatfile(root, 42, "", &error),
			"native resume: " + error);
		require(!player_save_journal_pid_quarantined(42), "resolution fence");
		player_load_request request = player_quarantine_recovery_request(record);
		auto recovered = flatfile_player_load_repository_execute(root, request);
		require(recovered.outcome == player_load_outcome::applied &&
				recovered.snapshot.items.size() == 3 &&
				recovered.snapshot.skills[0].learned == 88 &&
				recovered.domains.wallet[0] == 11,
			"recovery lost items/components/native wallet");
		require(player_quarantine_recovery_resume_flatfile(root, 42, "", &error),
			"duplicate resume");
		if (boundary == 0)
		{
			currency_command_payload reward{};
			reward.pid = 42;
			reward.racewar = 0;
			reward.reason = currency_reason_type::wallet_reward;
			std::strcpy(reward.account_name.data(), "Account-One");
			reward.wallet_delta.amount[0] = 1;
			critical_operation_id reward_id{};
			critical_command reward_command;
			require(critical_operation_id_generate(&reward_id) &&
					currency_command_build(
						&reward_command, reward_id, reward,
						recovered.domains.wallet_revision,
						recovered.domains.bank_revision,
						critical_source_site::operator_repair,
						critical_deadline_class::interactive),
				"native reward command");
			reward_command.accepted_at_usec = 1;
			require(flatfile_player_domain_apply(root, reward_command).outcome ==
					critical_apply_outcome::applied,
				"native post-resolution reward");
			player_save_journal_shutdown();
			require(player_save_journal_init(journal.c_str()), "native reward restart");
			require(player_quarantine_recovery_resume_flatfile(root, 42, "", &error),
				"committed native domain evolution invalidated recovery");
			request.deadline_usec =
				persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
			recovered = flatfile_player_load_repository_execute(root, request);
			require(recovered.domains.wallet[0] == 12 &&
					recovered.snapshot.revision == record.replacement.revision,
				"native reward proof lost authority");
		}
		auto later = recovered.snapshot;
		++later.revision;
		later.skills[0].learned = 89;
		require(flatfile_player_snapshot_apply(root, later, &error).outcome ==
				player_save_apply_outcome::applied,
			"later normal save");
		player_save_journal_shutdown();
		require(player_save_journal_init(journal.c_str()) &&
				player_save_journal_pid_quarantined(42),
			"restart trusted archive without backend");
		require(player_quarantine_recovery_resume_flatfile(root, 42, "", &error) &&
				!player_save_journal_pid_quarantined(42),
			"later save restart proof");
		if (boundary == 4)
		{
			player_save_journal_shutdown();
			fs::remove(fs::path(root) / "metadata" /
				   player_quarantine_recovery_receipt_filename(record));
			require(player_save_journal_init(journal.c_str()), "missing proof journal");
			require(!player_quarantine_recovery_resume_flatfile(root, 42, "", &error) &&
					player_save_journal_pid_quarantined(42),
				"higher revision without proof released PID");
		}
		player_save_journal_shutdown();
	}
	std::cout
		<< "[PASS] native flatfile quarantine recovery: original legacy/accounting commands, component preservation, wallet authority, conflict refusals, commit interruptions, repeat and later-save restart, missing proof refusal\n";
}

int main(int argc, char **argv)
{
	if (argc == 3 && std::string(argv[2]) == "quarantine-recovery")
	{
		check_quarantine_recovery(argv[1]);
		return 0;
	}
	if (argc == 3 && std::string(argv[2]) == "seed-creation-bank")
	{
		std::string error;
		flatfile_player_domain_record seed;
		seed.pid = 999;
		seed.account_name = "Journeyacct";
		seed.racewar = 1;
		seed.domains.bank = { 17, 23, 31, 47 };
		require(flatfile_player_domain_establish(argv[1], seed, &error) ==
				flatfile_player_domain_result::ok,
			"seed creation bank: " + error);
		currency_command_payload payload = {};
		payload.pid = seed.pid;
		payload.racewar = seed.racewar;
		payload.reason = currency_reason_type::bank_reward;
		strcpy(payload.account_name.data(), seed.account_name.c_str());
		payload.bank_delta.amount[0] = 2;
		critical_operation_id id = {};
		id.bytes[0] = 201;
		critical_command command;
		require(currency_command_build(&command, id, payload, 0, 1,
					       critical_source_site::command,
					       critical_deadline_class::interactive),
			"seed bank command");
		command.accepted_at_usec = 1;
		require(flatfile_player_domain_apply(argv[1], command).outcome ==
				critical_apply_outcome::applied,
			"advance creation bank revision");
		return 0;
	}
	if (argc == 3 && std::string(argv[2]) == "seed-combat")
	{
		std::string error;
		require(flatfile_boon_establish(argv[1], {}, &error) == flatfile_boon_result::ok,
			"seed empty combat boon catalog: " + error);
		return 0;
	}
	// Offline fixture setup only, against the temporary state owned by the journey.
	// Use the real creation transaction and checkpoint writer so item UID/custody
	// agree when the unmodified server restores the item.
	if (argc == 3 && std::string(argv[2]) == "seed-item-operator")
	{
		std::string error;
		player_snapshot snapshot;
		require(flatfile_player_snapshot_load(argv[1], 1, &snapshot, &error) ==
				flatfile_player_load_result::ok,
			"seed operator: " + error);
		for (auto &field : snapshot.status_integers)
			if (field.field == player_status_field::level)
				field = { player_status_field::level, 61, 0, false };
		++snapshot.revision;
		snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
		snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		require(flatfile_player_snapshot_apply(argv[1], snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"save operator: " + error);
		return 0;
	}
	if (argc == 4 && std::string(argv[2]) == "seed-item")
	{
		std::string error;
		player_snapshot snapshot;
		require(flatfile_player_snapshot_load(argv[1], 1, &snapshot, &error) ==
				flatfile_player_load_result::ok,
			"seed player: " + error);
		player_item_snapshot item = {};
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::ifstream input(argv[3]);
		int type, material;
		input >> item.vnum >> type >> material >> item.craftsmanship >> item.extra_flags >>
			item.wear_flags >> item.extra2_flags >> item.anti_flags >>
			item.anti2_flags >> item.weight >> item.cost >> item.condition;
		item.type = type;
		item.material = material;
		for (auto &value : item.values)
			input >> value;
		for (auto &value : item.bitvectors)
			input >> value;
		for (auto &affect : item.affects)
			input >> affect[0] >> affect[1];
		require(input.good(), "read fixture prototype");
		item.object_uid = 1000000 + item.vnum;
		item_transfer_payload payload = {};
		payload.from_owner = { item_owner_type::system, 0, 0 };
		payload.to_owner = { item_owner_type::player, 1, 0 };
		std::vector<flatfile_item_ownership_record> owned;
		require(flatfile_item_repository_load_owner(
				argv[1], payload.from_owner, &payload.expected_from_revision,
				&owned, &error) == flatfile_item_repository_result::ok,
			"seed system owner: " + error);
		require(flatfile_item_repository_load_owner(
				argv[1], payload.to_owner, &payload.expected_to_revision, &owned,
				&error) == flatfile_item_repository_result::ok,
			"seed player owner: " + error);
		payload.reason = item_transfer_reason::creation;
		payload.reason_id = 297;
		payload.selected_item_uid = payload.target_root_item_uid = item.object_uid;
		payload.item_count = 1;
		payload.items[0] = { item.object_uid,
				     item.object_uid,
				     0,
				     ITEM_TRANSFER_ABSENT_REVISION,
				     item.vnum,
				     item_custody_state::absent };
		std::vector<uint8_t> blob;
		require(player_item_snapshot_list_encode({ item }, &blob) ==
				player_snapshot_codec_result::ok,
			"encode fixture item");
		payload.item_blob_size = blob.size();
		std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
		critical_operation_id operation;
		critical_command command;
		require(critical_operation_id_generate(&operation) &&
				item_transfer_command_build(&command, operation, payload,
							    critical_source_site::command,
							    critical_deadline_class::interactive),
			"build fixture creation");
		command.accepted_at_usec = static_cast<uint64_t>(time(nullptr)) * 1000000;
		const auto created = flatfile_item_repository_apply(argv[1], command);
		require(created.outcome == critical_apply_outcome::applied,
			"create fixture item: " + std::to_string(created.error_code));
		snapshot.items.push_back(item);
		++snapshot.revision;
		snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
		snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		require(flatfile_player_snapshot_apply(argv[1], snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"save seeded item: " + error);
		if (item.extra_flags & (1U << 28))
		{
			const auto updated = flatfile_artifact_gameplay_update(
				argv[1], item.vnum, true, FLATFILE_ARTIFACT_ON_PLAYER, 1,
				time(nullptr) + 86400, 2, time(nullptr), &error);
			require(updated == flatfile_artifact_result::ok,
				"seed artifact catalog: " + error);
		}
		std::cout << item.object_uid << '\n';
		return 0;
	}
	if (argc == 4 && std::string(argv[2]) == "pending-quest-rewards")
	{
		std::vector<flatfile_quest_reward_obligation> obligations;
		std::string error;
		const auto result = flatfile_item_repository_pending_quest_rewards(
			argv[1], static_cast<uint32_t>(std::stoul(argv[3])), &obligations, &error);
		require(result == flatfile_item_repository_result::ok,
			"inspect pending quest rewards: " + error);
		std::cout << obligations.size() << '\n';
		return 0;
	}
	if (argc == 4 &&
	    (std::string(argv[2]) == "inspect" || std::string(argv[2]) == "inspect-items"))
	{
		inspect_authority(argv[1], std::stoi(argv[3]),
				  std::string(argv[2]) == "inspect-items");
		return 0;
	}
	if (argc == 4 && std::string(argv[2]) == "inspect-item")
	{
		flatfile_item_ownership_record item = {};
		std::string error;
		require(flatfile_item_repository_lookup_uid(argv[1], std::stoull(argv[3]), &item,
							    &error) ==
				flatfile_item_repository_result::ok,
			"inspect item UID: " + error);
		std::cout << "{\"uid\":" << item.item_uid << ",\"root\":" << item.root_item_uid
			  << ",\"parent\":" << item.parent_item_uid << ",\"vnum\":" << item.vnum
			  << ",\"revision\":" << item.item_revision
			  << ",\"owner_type\":" << static_cast<unsigned>(item.owner.type)
			  << ",\"state\":" << static_cast<unsigned>(item.state) << "}\n";
		return 0;
	}
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	coin_player_matrix(root / "coin-player");
	spell_receipt_matrix(root / "spell-player");
	craft_progression_matrix(root / "craft-player");
	quest_xp_matrix(root / "quest-xp-solo", 4);
	quest_xp_matrix(root / "quest-xp-current-solo", 5);
	quest_xp_matrix(root / "quest-xp-group", 5, true);
	quest_xp_matrix(root / "quest-xp-death", 5, true, true);
	const fs::path players = root / "players";
	const fs::path identities = root / "identities/names";
	const fs::path domains = root / "domains";
	fs::create_directories(players);
	fs::create_directories(identities);
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(players, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(identities, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	int32_t allocated_pid = 0;
	for (int32_t expected_pid = 1; expected_pid <= 42; ++expected_pid)
		require(flatfile_identity_allocate_pid(root.string(), &allocated_pid, &error) ==
					flatfile_identity_result::ok &&
				allocated_pid == expected_pid,
			"could not allocate player PID: " + error);
	require(flatfile_identity_claim(root.string(), 42, "Player", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"could not claim player identity: " + error);
	player_save_apply_result applied =
		flatfile_player_snapshot_apply(root.string(), make_status(1, 10, 100), &error);
	require(applied.outcome == player_save_apply_outcome::terminal_failure &&
			applied.error_code == ENOENT,
		"partial snapshot created a missing player baseline");
	player_snapshot full = make_full(1);
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::applied &&
			applied.durable_revision == 1,
		"full baseline apply failed: " + error);
	player_snapshot loaded;
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.components == PLAYER_CHECKPOINT_COMPONENT_ALL &&
			loaded.revision == 1 && loaded.status_integers[0].signed_value == 50 &&
			loaded.languages[0].value == 90 && loaded.items.size() == 2 &&
			loaded.items[1].parent_index == 0 &&
			loaded.items[0].extra_descriptions[0].spell_ids[1] == 12 &&
			loaded.pets[0].items[0].vnum == 501 && loaded.shapes[0].mob_vnum == 800 &&
			loaded.trophies[0].experience == 300 &&
			loaded.output_preferences == full.output_preferences,
		"full player snapshot did not round trip: " + error);
	player_load_request load_request = {};
	load_request.request_id = 1;
	load_request.pid = 42;
	load_request.account_name = "account-one";
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result load_result =
		flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.pid == 42 && load_result.snapshot.revision == 1 &&
			load_result.item_owner_revision == 1 &&
			load_result.item_identities.size() == 2 &&
			load_result.item_identities[1].parent_item_uid == 100 &&
			load_result.pet_identities.size() == 1 &&
			load_result.pet_identities[0].item_identities.size() == 1 &&
			load_result.domains.wallet == std::array<uint64_t, 4>{ 11, 12, 13, 14 } &&
			load_result.domains.bank_revision == 1 && load_result.domains.epics == 15 &&
			load_result.domains.frags == 16 && load_result.domains.old_frags == 17 &&
			load_result.domains.base_stat_revision == 1 &&
			load_result.domains.base_stats[0] == 50 &&
			load_result.domains.base_stats[9] == 59 &&
			load_result.read_components == PLAYER_LOAD_SESSION04_READS &&
			load_result.outcome == player_load_outcome::applied &&
			load_result.error_code == 0 && !load_result.failed_component,
		"verified snapshot/domain load was not reported as materializable");
	load_request.request_id = 2;
	load_request.account_name = "wrong-account";
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.outcome == player_load_outcome::component_failure &&
			load_result.error_code == EACCES &&
			std::string(load_result.failed_component) == "identity",
		"account/PID mismatch was accepted");
	load_request = {};
	load_request.request_id = 3;
	load_request.player_name = "pLaYeR";
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.pid == 42 && load_result.account_name == "Account-One" &&
			load_result.outcome == player_load_outcome::applied,
		"canonical name lookup did not resolve the snapshot identity");
	load_request.deadline_usec = persistence_observability_now_usec();
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.outcome == player_load_outcome::timed_out &&
			load_result.error_code == ETIMEDOUT,
		"expired flat-file load request was accepted");
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::already_applied &&
			applied.durable_revision == 1,
		"duplicate revision was not idempotent");

	player_snapshot trophy_checkpoint = make_status(2, 51, 1202);
	trophy_checkpoint.components |= PLAYER_COMPONENT_TROPHIES;
	trophy_checkpoint.trophies = { { 12, 645 }, { 34, 678 } };
	applied = flatfile_player_snapshot_apply(root.string(), trophy_checkpoint, &error);
	require(applied.outcome == player_save_apply_outcome::applied &&
			applied.durable_revision == 2,
		"partial status merge failed: " + error);
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.revision == 2 && loaded.room_vnum == 1202 &&
			loaded.status_integers[0].signed_value == 51 && loaded.items.size() == 2 &&
			loaded.languages[0].value == 90 && loaded.trophies.size() == 2 &&
			loaded.trophies[0].experience == 645 &&
			loaded.trophies[1].experience == 678 && loaded.output_preferences.empty(),
		"partial status merge discarded an untouched component");
	applied = flatfile_player_snapshot_apply(root.string(), full, &error);
	require(applied.outcome == player_save_apply_outcome::stale_revision &&
			applied.durable_revision == 2,
		"stale player revision was accepted");

	player_snapshot torn_items = {};
	torn_items.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	torn_items.pid = 42;
	torn_items.revision = 3;
	torn_items.components = PLAYER_COMPONENT_INVENTORY;
	torn_items.encoded_size_bound = 256;
	applied = flatfile_player_snapshot_apply(root.string(), torn_items, &error);
	require(applied.outcome == player_save_apply_outcome::terminal_failure &&
			applied.error_code == EINVAL,
		"one-sided item component replacement was accepted");

	trophy_checkpoint.revision = 3;
	trophy_checkpoint.trophies.clear();
	applied = flatfile_player_snapshot_apply(root.string(), trophy_checkpoint, &error);
	require(applied.outcome == player_save_apply_outcome::applied,
		"empty trophy checkpoint failed");
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.trophies.empty(),
		"empty trophy checkpoint did not remove previous totals");

	for (player_revision_t revision : { 4U, 5U })
	{
		const pid_t child_process = fork();
		require(child_process >= 0, "player writer fork failed");
		if (!child_process)
		{
			std::string child_error;
			const player_save_apply_result child_result =
				flatfile_player_snapshot_apply(root.string(),
							       make_status(revision, 50 + revision,
									   1200 + revision),
							       &child_error);
			_exit(child_result.outcome == player_save_apply_outcome::applied ||
					      child_result.outcome ==
						      player_save_apply_outcome::stale_revision ?
				      0 :
				      2);
		}
	}
	for (int child = 0; child < 2; ++child)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent player writer failed");
	}
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.revision == 5 && loaded.status_integers[0].signed_value == 55,
		"concurrent player writers lost the highest revision");

	// The ownership catalog is written once, at baseline, so a later save can leave the
	// two files disagreeing. None of these disagreements may lock the character out.
	auto reload = [&](const player_snapshot &snapshot, uint64_t request_id)
	{
		require(flatfile_player_snapshot_apply(root.string(), snapshot, &error).outcome ==
				player_save_apply_outcome::applied,
			"item-consistency fixture save failed: " + error);
		player_load_request items_request = {};
		items_request.request_id = request_id;
		items_request.pid = 42;
		items_request.account_name = "account-one";
		items_request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		return flatfile_player_load_repository_execute(root.string(), items_request);
	};
	player_item_snapshot orphan = {};
	orphan.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	orphan.object_uid = 103;
	orphan.vnum = 503;

	// A stale projection that still shows an authoritative child at top level heals
	// back into its container instead of refusing the character.
	player_snapshot stale_parent = make_full(6);
	stale_parent.items[1].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	player_load_result recovered = reload(stale_parent, 9);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.repaired_item_rows == 1 &&
			recovered.snapshot.items[1].parent_index == 0 &&
			recovered.item_identities[1].serialized_parent_id ==
				recovered.item_identities[0].database_id,
		"stale flat-file item placement refused the load");

	player_snapshot extra_payload = make_full(7);
	extra_payload.items.push_back(orphan);
	recovered = reload(extra_payload, 10);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 2 && recovered.stale_item_rows == 1 &&
			recovered.missing_payload_rows == 0 &&
			recovered.authoritative_item_count == 3,
		"a payload item missing from the ownership catalog refused the load");

	player_snapshot dropped_payload = make_full(8);
	dropped_payload.items.pop_back();
	recovered = reload(dropped_payload, 11);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 1 &&
			recovered.missing_payload_rows == 1 && recovered.stale_item_rows == 0 &&
			recovered.authoritative_item_count == 2,
		"an ownership record without its payload item refused the load");

	// The orphan is the container this time: its contents load at the top level rather
	// than disappearing with it.
	player_snapshot orphan_container = make_full(9);
	orphan_container.items.insert(orphan_container.items.begin(), orphan);
	orphan_container.items[1].parent_index = 0;
	orphan_container.items[2].parent_index = 1;
	recovered = reload(orphan_container, 12);
	require(recovered.outcome == player_load_outcome::applied &&
			recovered.snapshot.items.size() == 2 && recovered.stale_item_rows == 1 &&
			recovered.promoted_item_rows == 1 && recovered.missing_payload_rows == 0 &&
			recovered.snapshot.items[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			recovered.snapshot.items[1].parent_index == 0 &&
			recovered.item_identities[0].root_item_uid == 100 &&
			!recovered.item_identities[0].parent_item_uid &&
			recovered.item_identities[1].root_item_uid == 100,
		"contents of an orphaned container did not survive the load");
	require(flatfile_player_snapshot_apply(root.string(), make_full(10), &error).outcome ==
			player_save_apply_outcome::applied,
		"could not restore the consistent item fixture: " + error);

	// A refused corpse handoff is finalized through the durable disposition. It has
	// to leave the player empty-handed for normal re-entry while every refused
	// payload, UID and custody observation survives outside that player file.
	const player_snapshot death_record = make_death(13);
	const fs::path deaths = root / "player-deaths";
	fs::create_directories(deaths);
	fs::permissions(deaths, fs::perms::owner_all, fs::perm_options::replace);
	setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
	require(flatfile_player_snapshot_apply(root.string(), death_record, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"death acknowledged a failed authority commit");
	unsetenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT");
	require(fs::is_empty(deaths), "failed death commit left phantom disposition evidence");
	uint64_t retained_revision = 0;
	std::vector<flatfile_item_ownership_record> retained_items;
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			loaded.revision == 10 && !loaded.items.empty() &&
			flatfile_item_repository_load_owner(
				root.string(), { item_owner_type::player, 42, 0 },
				&retained_revision, &retained_items,
				&error) == flatfile_item_repository_result::ok &&
			!retained_items.empty(),
		"failed death commit changed active inventory or custody");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	require(flatfile_player_snapshot_apply(root.string(), death_record, &error).outcome ==
			player_save_apply_outcome::retryable_failure,
		"death acknowledged an interrupted authority commit");
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(flatfile_player_snapshot_apply(root.string(), death_record, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"death retry did not recover its custody and player after-images: " + error);
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
				flatfile_player_load_result::ok &&
			!loaded.death && loaded.items.empty() && loaded.pets.empty() &&
			loaded.revision == 13,
		"the death left assets in the player file: " + error);
	uint64_t quarantine_revision = 0;
	std::vector<flatfile_item_ownership_record> active_after_death;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 42, 0 }, &quarantine_revision,
			&active_after_death, &error) == flatfile_item_repository_result::ok &&
			active_after_death.size() == 1 && active_after_death[0].item_uid == 102 &&
			active_after_death[0].state == item_custody_state::active,
		"death quarantine changed custody outside the captured graph");
	load_request.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	load_result = flatfile_player_load_repository_execute(root.string(), load_request);
	require(load_result.outcome == player_load_outcome::applied &&
			load_result.snapshot.items.empty() && load_result.snapshot.pets.empty() &&
			load_result.missing_payload_rows == 1,
		"normal player loading restored disputed assets or lost the live pet item");
	player_snapshot disposition = {};
	require(flatfile_player_snapshot_read_file(
			flatfile_player_snapshot_file::death_directory(root.string()),
			flatfile_player_snapshot_file::death_filename(42, 13), 42, &disposition,
			&error) == flatfile_player_load_result::ok &&
			disposition.death.has_value(),
		"the death disposition was not published durably: " + error);
	require(disposition.death->corpse_room_vnum == 1201 &&
			disposition.death->corpse.size() == 4 &&
			disposition.death->corpse[0].object_uid == 200 &&
			disposition.death->corpse[0].values[CORPSE_SAVEID] == 9001 &&
			disposition.death->corpse[1].object_uid == 100 &&
			disposition.death->corpse[2].object_uid == 101 &&
			disposition.death->corpse[3].object_uid == 202 &&
			disposition.death->wallet_before ==
				std::array<int32_t, 4>{ 11, 12, 13, 14 } &&
			disposition.death->wallet_pile_uid == 202 &&
			disposition.death->custody.size() == 3 &&
			disposition.death->custody[0].item.item_uid == 100 &&
			disposition.death->custody[0].owner.type == item_owner_type::player &&
			disposition.death->custody[1].item.item_uid == 101 &&
			disposition.death->custody[2].item.expected_state ==
				item_custody_state::absent,
		"the death disposition lost corpse identity, wallet or custody evidence");
	require(flatfile_player_snapshot_apply(root.string(), death_record, &error).outcome ==
			player_save_apply_outcome::already_applied,
		"replaying the death repeated its consequences: " + error);
	uint64_t replay_owner_revision = 0;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 42, 0 }, &replay_owner_revision,
			&active_after_death, &error) == flatfile_item_repository_result::ok &&
			replay_owner_revision == quarantine_revision &&
			active_after_death.size() == 1 && active_after_death[0].item_uid == 102 &&
			active_after_death[0].state == item_custody_state::active,
		"death replay repeated quarantine or changed live active custody");
	require(flatfile_player_snapshot_apply(root.string(), make_full(14), &error).outcome ==
				player_save_apply_outcome::applied &&
			flatfile_player_snapshot_read_file(
				flatfile_player_snapshot_file::death_directory(root.string()),
				flatfile_player_snapshot_file::death_filename(42, 13), 42,
				&disposition, &error) == flatfile_player_load_result::ok,
		"a later ordinary save discarded the death disposition: " + error);
	{
		flatfile_player_snapshot_lock snapshot_lock;
		flatfile_authority_lock authority_lock;
		flatfile_authority_operation snapshot_remove, domain_remove;
		require(snapshot_lock.acquire(root.string(), 42, &error) &&
				authority_lock.acquire(root.string(), &error),
			"could not acquire player deletion preparation locks: " + error);
		require(flatfile_player_snapshot_prepare_remove(
				root.string(), snapshot_lock, authority_lock, 42, &snapshot_remove,
				&error) == flatfile_player_load_result::ok &&
				snapshot_remove.store == flatfile_authority_store::players &&
				snapshot_remove.kind == flatfile_authority_operation_kind::remove &&
				snapshot_remove.filename == "42.snapshot",
			"player snapshot removal was not prepared: " + error);
		require(flatfile_player_domain_prepare_remove(root.string(), authority_lock, 42,
							      &domain_remove, &error) ==
					flatfile_player_domain_result::ok &&
				domain_remove.store == flatfile_authority_store::domains &&
				domain_remove.kind == flatfile_authority_operation_kind::remove &&
				domain_remove.filename == "player-42.domain",
			"player domain removal was not prepared: " + error);
	}

	const fs::path snapshot_path = players / "42.snapshot";
	{
		std::fstream file(snapshot_path, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open player snapshot for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_player_snapshot_load(root.string(), 42, &loaded, &error) ==
			flatfile_player_load_result::invalid,
		"corrupt player checksum was accepted");
	for (const fs::directory_entry &entry : fs::directory_iterator(players))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary player file was left behind");

	std::cout << "flat-file player repository passed\n";
	return 0;
}
