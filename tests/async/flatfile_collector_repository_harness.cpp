#include "core/defines.h"
#include "economy/collector_command.h"
#include "economy/collector_accounting.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_collector_repository.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto initialize_evidence =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto create_mapping =
		&flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
};

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xd3;
	id.bytes.back() = discriminator;
	return id;
}

static size_t native_bucket(uint16_t kind, uint64_t context, uint64_t pid, const std::string &name)
{
	std::vector<uint8_t> bytes;
	const auto add = [&](uint64_t value, size_t width)
	{
		for (size_t index = 0; index < width; ++index)
			bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
	};
	add(kind, 2);
	add(context, 8);
	add(kind, 2);
	if (kind == 1)
		add(pid, 8);
	else
		bytes.insert(bytes.end(), name.begin(), name.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest[0];
}

static critical_command item_command(const item_transfer_payload &payload, uint8_t discriminator)
{
	critical_command command = {};
	require(item_transfer_command_build(&command, operation(discriminator), payload,
					    critical_source_site::combat,
					    critical_deadline_class::interactive),
		"could not build collector death transfer");
	command.accepted_at_usec = discriminator;
	return command;
}

static critical_command collector_command(const collector_command_payload &payload,
					  uint8_t discriminator)
{
	critical_command command = {};
	require(collector_command_build(&command, operation(discriminator), payload,
					critical_source_site::zone_event,
					critical_deadline_class::background),
		"could not build collector authority command");
	command.accepted_at_usec = discriminator;
	return command;
}

static collector_command_result collector_result(const critical_apply_result &applied)
{
	collector_command_result result = {};
	require(collector_command_decode_result(applied.result_payload.data(), applied.result_size,
						&result),
		"could not decode collector result");
	return result;
}

static player_item_snapshot snapshot(uint64_t uid, int32_t vnum, int32_t parent_index,
				     int32_t weight, int32_t cost, bool eligible)
{
	player_item_snapshot item = {};
	item.parent_index = parent_index;
	item.equipment_slot = -1;
	item.object_uid = uid;
	item.generated_key = static_cast<int64_t>(uid + 1000);
	item.vnum = vnum;
	item.type = uid == 100 ? ITEM_CONTAINER : ITEM_WEAPON;
	item.name = uid == 100 ? "ineligible death container" : "eligible antique";
	item.short_description = item.name;
	item.description = "A collector authority test object is here.";
	item.wear_flags = ITEM_TAKE;
	item.extra_flags = eligible ? 0 : ITEM_NORENT;
	item.weight = weight;
	item.cost = cost;
	item.condition = 93;
	return item;
}

static std::vector<uint8_t> encode(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> blob;
	require(player_item_snapshot_list_encode(items, &blob) ==
				player_snapshot_codec_result::ok &&
			!blob.empty() && blob.size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES,
		"could not encode collector item snapshots");
	return blob;
}

static std::vector<uint8_t> singleton(player_item_snapshot item)
{
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	return encode({ item });
}

static flatfile_player_domain_record player(uint32_t pid, const char *account)
{
	flatfile_player_domain_record record;
	record.pid = static_cast<int32_t>(pid);
	record.account_name = account;
	record.racewar = 1;
	record.domains.wallet = { 0, 0, 0, 10 };
	record.domains.bank = { 0, 0, 0, 0 };
	return record;
}

static collector_listing_detail listing(const std::string &root, uint64_t id, std::string *error)
{
	collector_listing_detail detail;
	bool found = false;
	require(flatfile_collector_repository_read_listing(root, id, &detail, &found, error) ==
				flatfile_collector_repository_result::ok &&
			found,
		"collector listing was not readable: " + *error);
	return detail;
}

static uint64_t owner_revision(const std::string &root, const item_owner_identity &owner,
			       std::vector<flatfile_item_ownership_record> *items,
			       std::string *error)
{
	uint64_t revision = 0;
	items->clear();
	const auto loaded =
		flatfile_item_repository_load_owner(root, owner, &revision, items, error);
	if (loaded == flatfile_item_repository_result::not_found)
		return 0;
	require(loaded == flatfile_item_repository_result::ok,
		"collector owner was not readable: " + *error);
	return revision;
}

static collector_command_payload collect_payload(const std::string &root,
						 const collector::record &record,
						 const item_owner_identity &corpse,
						 const player_item_snapshot &selected, uint64_t now,
						 std::string *error)
{
	std::vector<flatfile_item_ownership_record> items;
	const uint64_t corpse_revision = owner_revision(root, corpse, &items, error);
	collector_command_payload payload = {};
	payload.action = collector_action::collect;
	payload.target_state = item_custody_state::active;
	payload.listing = record.listing;
	payload.expected_listing_revision = record.revision;
	payload.observed_at = now;
	payload.from_owner = corpse;
	payload.to_owner = { item_owner_type::collector, item_collector_owner_id(record.listing),
			     0 };
	payload.expected_from_owner_revision = corpse_revision;
	std::vector<flatfile_item_ownership_record> held;
	payload.expected_to_owner_revision = owner_revision(root, payload.to_owner, &held, error);
	payload.selected_item_uid = record.uid;
	for (const auto &item : items)
		if (item.root_item_uid == 100)
			payload.items[payload.item_count++] = {
				item.item_uid,	    item.root_item_uid, item.parent_item_uid,
				item.item_revision, item.vnum,		item.state
			};
	const std::vector<uint8_t> blob = singleton(selected);
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	return payload;
}

static collector_command_payload metadata(collector_action action, const collector::record &record,
					  uint64_t now)
{
	collector_command_payload payload = {};
	payload.action = action;
	payload.listing = record.listing;
	payload.expected_listing_revision = record.revision;
	payload.observed_at = now;
	return payload;
}

static collector_command_payload held_payload(const std::string &root, collector_action action,
					      const collector_listing_detail &detail,
					      const item_owner_identity &destination,
					      item_custody_state target, uint64_t now,
					      std::string *error)
{
	collector_command_payload payload = {};
	payload.action = action;
	payload.target_state = target;
	payload.listing = detail.entry.listing;
	payload.expected_listing_revision = detail.entry.revision;
	payload.observed_at = now;
	payload.from_owner = { item_owner_type::collector,
			       item_collector_owner_id(detail.entry.listing), 0 };
	payload.to_owner = destination;
	std::vector<flatfile_item_ownership_record> held;
	payload.expected_from_owner_revision =
		owner_revision(root, payload.from_owner, &held, error);
	std::vector<flatfile_item_ownership_record> destination_items;
	payload.expected_to_owner_revision =
		owner_revision(root, destination, &destination_items, error);
	require(held.size() == 1 && held[0].item_uid == detail.entry.uid,
		"collector held custody did not match listing");
	payload.selected_item_uid = detail.entry.uid;
	payload.item_count = 1;
	payload.items[0] = { held[0].item_uid,	    held[0].root_item_uid, held[0].parent_item_uid,
			     held[0].item_revision, held[0].vnum,	   held[0].state };
	payload.item_blob_size = static_cast<uint32_t>(detail.item_blob.size());
	std::copy(detail.item_blob.begin(), detail.item_blob.end(), payload.item_blob.begin());
	return payload;
}

static void enable_accounting(const std::string &root, economic_account_key *wallet,
			      economic_account_key *bank, std::string *error)
{
	fs::create_directories(fs::path(root) / "economic-evidence");
	fs::permissions(fs::path(root) / "economic-evidence", fs::perms::owner_all,
			fs::perm_options::replace);
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock collector accounting setup");
	std::vector<flatfile_authority_operation> operations;
	const auto commit = [&]
	{
		require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
				flatfile_authority_transaction_result::ok,
			"could not commit collector accounting setup: " + *error);
		operations.clear();
	};
	const auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root, lock, &control, error) == 0,
			"could not read collector accounting control: " + *error);
		return control.revision;
	};
	require(flatfile_accounting_test_access::bootstrap(root, lock, operation(70), operation(71),
							   &operations, error) == 0,
		"could not bootstrap collector accounting: " + *error);
	commit();
	const size_t wallet_bucket = native_bucket(1, 0, 42, {});
	const size_t bank_bucket = native_bucket(2, 1, 0, "beneficiary");
	for (const size_t bucket : { wallet_bucket, bank_bucket })
	{
		const auto initialized = flatfile_accounting_test_access::initialize_native(
			root, lock, revision(), bucket, operation(72), &operations, error);
		require(initialized == 0 || initialized == EALREADY,
			"could not initialize collector native bucket: " + *error);
		if (!initialized)
			commit();
	}
	flatfile_economic_mapping mapping;
	require(flatfile_accounting_test_access::create_mapping(
			root, lock, revision(), economic_account_kind::wallet, 0, { 1, 42, {} },
			operation(73), &mapping, &operations, error) == 0,
		"could not map collector wallet: " + *error);
	*wallet = mapping.account;
	commit();
	require(flatfile_accounting_test_access::create_mapping(
			root, lock, revision(), economic_account_kind::bank, 1,
			{ 2, 0, "beneficiary" }, operation(74), &mapping, &operations, error) == 0,
		"could not map collector bank: " + *error);
	*bank = mapping.account;
	commit();
	flatfile_economic_epoch epoch;
	epoch.epoch = operation(75);
	epoch.creating_operation = operation(76);
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	require(flatfile_accounting_test_access::append_epoch(root, lock, revision(), epoch,
							      &operations, error) == 0,
		"could not append collector accounting epoch: " + *error);
	commit();
	require(flatfile_accounting_test_access::select_epoch(
			root, lock, revision(), true, operation(77), &operations, error) == 0,
		"could not activate collector accounting epoch: " + *error);
	commit();
	require(flatfile_accounting_test_access::initialize_evidence(
			root, lock, revision(), operation(9).bytes[0], operation(78), &operations,
			error) == 0,
		"could not initialize collector evidence bucket: " + *error);
	commit();
}

static void interrupted_saved_room_collection(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const std::string path = root.string();
	std::string error;
	const item_owner_identity room = { item_owner_type::room, 700, 0 };
	const auto container = snapshot(100, 2100, PLAYER_SNAPSHOT_NO_PARENT, 30, 10, false);
	const auto shell = snapshot(101, 2101, 0, 5, 500, true);
	flatfile_saved_world_item_record saved = {};
	saved.item_key = "item.collector.saved";
	saved.room_vnum = 700;
	saved.revision = 1;
	saved.items = { container, shell };
	require(flatfile_world_item_establish(path, {}, { saved }, &error) ==
			flatfile_world_item_result::ok,
		"could not establish saved-room collector fixture: " + error);
	require(flatfile_item_repository_establish_owner(
			path, room,
			{ { 100, 100, 0, room, 1, 2100, item_custody_state::active },
			  { 101, 100, 100, room, 1, 2101, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish saved-room item custody: " + error);
	item_transfer_payload enrollment = {};
	enrollment.from_owner = { item_owner_type::player, 42, 0 };
	enrollment.to_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 2000), 0 };
	enrollment.reason = item_transfer_reason::corpse_create;
	enrollment.selected_item_uid = 100;
	enrollment.target_root_item_uid = 100;
	enrollment.item_count = 2;
	enrollment.items[0] = { 100, 100, 0, 0, 2100, item_custody_state::active };
	enrollment.items[1] = { 101, 100, 100, 0, 2101, item_custody_state::active };
	const auto blob = encode(saved.items);
	enrollment.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), enrollment.item_blob.begin());
	enrollment.collector.present = true;
	enrollment.collector.death_operation = operation(90);
	enrollment.collector.beneficiary_pid = 42;
	enrollment.collector.death_time = 1000;
	enrollment.collector.policy = { 10, 20, 100, 200, 100 };
	enrollment.collector.eligible_item_uids = { 101 };
	{
		flatfile_authority_lock lock;
		require(lock.acquire(path, &error), "could not lock saved-room enrollment fixture");
		flatfile_collector_enrollment_mutation mutation;
		unsigned int result_code = 0;
		const item_transfer_result transfer = { 100, 2, 0, 1, 1, 0 };
		require(flatfile_collector_prepare_death_enrollment(
				path, lock, enrollment, transfer, &mutation, &result_code,
				&error) == flatfile_collector_repository_result::ok &&
				!result_code && mutation.enrolled == 1,
			"could not prepare saved-room collector entitlement: " + error);
		require(flatfile_authority_transaction_commit(path, lock, { mutation.after_image },
							      &error) ==
				flatfile_authority_transaction_result::ok,
			"could not commit saved-room collector entitlement: " + error);
	}
	const auto detail = listing(path, 1, &error);
	const auto payload = collect_payload(path, detail.entry, room, shell, 1010, &error);
	const auto command = collector_command(payload, 91);
	require(setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION", "2", 1) == 0,
		"could not arm saved-room collection fault");
	const auto interrupted = flatfile_collector_repository_apply(path, command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION");
	require(interrupted.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains/.critical-authority-transaction"),
		"saved-room collector command did not interrupt after partial publication");
	const auto recovered = listing(path, 1, &error);
	require(recovered.entry.status == collector::state::collected &&
			!fs::exists(root / "domains/.critical-authority-transaction"),
		"saved-room collection did not recover its complete journal");
	const auto replay = flatfile_collector_repository_apply(path, command);
	require(replay.outcome == critical_apply_outcome::already_applied &&
			collector_result(replay).entry.item_revision == 2,
		"saved-room collection did not replay exactly");
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(path, &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty() && saved_items.size() == 1 &&
			saved_items[0].revision == 2 && saved_items[0].items.size() == 1 &&
			saved_items[0].items[0].object_uid == 100 &&
			saved_items[0].items[0].weight == 25,
		"saved-room collection left a duplicate or damaged the retained container");
	std::vector<flatfile_item_ownership_record> room_items, held_items;
	require(owner_revision(path, room, &room_items, &error) == 2 && room_items.size() == 1 &&
			room_items[0].item_uid == 100 && room_items[0].item_revision == 2,
		"saved-room collection did not retain exact room custody");
	require(owner_revision(path, payload.to_owner, &held_items, &error) == 1 &&
			held_items.size() == 1 && held_items[0].item_uid == 101 &&
			held_items[0].item_revision == 2,
		"saved-room collection did not publish exact collector custody");
	std::vector<economic_accounting_item_reference> references;
	for (uint16_t index = 0; index < 2; ++index)
	{
		economic_accounting_item_reference reference = {};
		require(flatfile_item_accounting_reference_find_by_legacy(
				path, command.operation_id, index, &reference, &error) ==
					flatfile_item_accounting_status::ok &&
				reference.item_uid == UINT64_C(100) + index &&
				reference.before_revision == 1 && reference.after_revision == 2 &&
				reference.operation_id.bytes == command.operation_id.bytes &&
				reference.line_index == index && reference.child_index == 1,
			"saved-room collection lost an exact item reference: " + error);
		references.push_back(reference);
	}
	require(flatfile_item_accounting_reference_verify_operation(path, command.operation_id,
								    references, &error) ==
			flatfile_item_accounting_status::ok,
		"saved-room collection retained an extra item reference: " + error);
	size_t removed_buckets = 0;
	for (const auto &entry : fs::directory_iterator(root / "accounting/item_references"))
	{
		require(entry.is_regular_file() && fs::remove(entry.path()),
			"could not fault the saved-room reference bucket");
		++removed_buckets;
	}
	require(removed_buckets == 1,
		"saved-room collection had an unexpected reference bucket count");
	const auto missing_reference = flatfile_collector_repository_apply(path, command);
	require(missing_reference.outcome == critical_apply_outcome::terminal_failure &&
			missing_reference.error_code == EILSEQ,
		"collector replay accepted a missing committed item reference");
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	interrupted_saved_room_collection(root / "saved-room-fault");
	const std::string root_path = root.string();
	fs::create_directories(root / "domains");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	const auto established =
		flatfile_player_domain_establish(root_path, player(42, "beneficiary"), &error);
	require(established == flatfile_player_domain_result::ok,
		"could not establish collector beneficiary (result " +
			std::to_string(static_cast<unsigned int>(established)) + "): " + error);
	require(flatfile_artifact_ensure(root_path, &error) == flatfile_artifact_result::ok,
		"could not establish empty artifact authority: " + error);

	const item_owner_identity player_owner = { item_owner_type::player, 42, 0 };
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(42, 1000), 0 };
	const std::vector<player_item_snapshot> snapshots = {
		snapshot(100, 2100, PLAYER_SNAPSHOT_NO_PARENT, 30, 10, false),
		snapshot(101, 2101, 0, 5, 500, true),
		snapshot(102, 2102, 0, 6, 600, true),
		snapshot(103, 2103, 0, 7, 700, true),
	};
	std::vector<flatfile_item_ownership_record> baseline;
	for (size_t index = 0; index < snapshots.size(); ++index)
		baseline.push_back({ snapshots[index].object_uid, 100,
				     index ? UINT64_C(100) : UINT64_C(0), player_owner, 1,
				     snapshots[index].vnum, item_custody_state::active });
	require(flatfile_item_repository_establish_owner(root_path, player_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish death custody baseline: " + error);

	item_transfer_payload death = {};
	death.from_owner = player_owner;
	death.to_owner = corpse_owner;
	death.reason = item_transfer_reason::corpse_create;
	death.reason_id = 1000;
	death.expected_from_revision = 1;
	death.expected_to_revision = 0;
	death.selected_item_uid = 100;
	death.target_root_item_uid = 100;
	death.item_count = static_cast<uint16_t>(snapshots.size());
	for (size_t index = 0; index < snapshots.size(); ++index)
		death.items[index] = { snapshots[index].object_uid,
				       100,
				       index ? UINT64_C(100) : UINT64_C(0),
				       1,
				       snapshots[index].vnum,
				       item_custody_state::active };
	const std::vector<uint8_t> death_blob = encode(snapshots);
	death.item_blob_size = static_cast<uint32_t>(death_blob.size());
	std::copy(death_blob.begin(), death_blob.end(), death.item_blob.begin());
	death.corpse.present = true;
	death.corpse.room_vnum = 500;
	death.corpse.weight = 30;
	death.corpse.actor_racewar = 1;
	death.corpse.values[3] = 42;
	death.corpse.values[5] = 1;
	death.corpse.values[6] = 1000;
	death.corpse.owner_name = "beneficiary";
	death.corpse.short_description = "the corpse of a collector beneficiary";
	death.corpse.description = "A collector authority test corpse lies here.";
	death.corpse.keywords = "corpse beneficiary _pcorpse_";
	death.collector.present = true;
	death.collector.death_operation = operation(1);
	death.collector.beneficiary_pid = 42;
	death.collector.death_time = 1000;
	death.collector.policy = { 10, 20, 100, 200, 100 };
	death.collector.eligible_item_uids = { 101, 102, 103 };
	const critical_command death_handoff = item_command(death, 1);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error),
			"could not acquire enrollment preflight lock");
		flatfile_collector_enrollment_mutation mutation;
		unsigned int result_code = 0;
		const item_transfer_result transfer = { 100, 4, 2, 1, 2, 1 };
		const auto prepared = flatfile_collector_prepare_death_enrollment(
			root_path, lock, death, transfer, &mutation, &result_code, &error);
		require(prepared == flatfile_collector_repository_result::ok && !result_code &&
				!mutation.after_image.bytes.empty(),
			"death enrollment preflight failed (result " +
				std::to_string(static_cast<unsigned int>(prepared)) + ", code " +
				std::to_string(result_code) + "): " + error);
		flatfile_corpse_transfer_mutation corpse;
		const auto world = flatfile_world_item_prepare_corpse_transfer(
			root_path, lock, death, &corpse, &error);
		require(world == flatfile_world_item_result::ok,
			"death world preflight failed (result " +
				std::to_string(static_cast<unsigned int>(world)) + "): " + error);
		flatfile_artifact_transfer_mutation artifacts;
		const auto artifact = flatfile_artifact_prepare_corpse_transfer(
			root_path, lock, death, 1, &artifacts, &error);
		require(artifact == flatfile_artifact_result::ok ||
				artifact == flatfile_artifact_result::unchanged,
			"death artifact preflight failed (result " +
				std::to_string(static_cast<unsigned int>(artifact)) +
				"): " + error);
		flatfile_shop_trade_materialization_mutation materialization;
		const auto materialized = flatfile_item_transfer_materialization_prepare(
			root_path, lock, operation(1), death, &materialization, &error);
		require(materialized == flatfile_shop_trade_materialization_result::ok ||
				materialized ==
					flatfile_shop_trade_materialization_result::unchanged,
			"death materialization preflight failed (result " +
				std::to_string(static_cast<unsigned int>(materialized)) +
				"): " + error);
	}
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	critical_apply_result applied = flatfile_item_repository_apply(root_path, death_handoff);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains" / ".critical-authority-transaction"),
		"interrupted death enrollment did not preserve authority intent (outcome " +
			std::to_string(static_cast<unsigned int>(applied.outcome)) + ", error " +
			std::to_string(applied.error_code) + ", journal " +
			std::to_string(
				fs::exists(root / "domains" / ".critical-authority-transaction")) +
			")");
	collector_bootstrap_snapshot bootstrap;
	require(flatfile_collector_repository_read_bootstrap(root_path, &bootstrap, &error) ==
				flatfile_collector_repository_result::ok &&
			bootstrap.catalog.revision == 1 && bootstrap.catalog.next_listing == 4 &&
			bootstrap.catalog.records.size() == 3 && bootstrap.held_items.empty() &&
			!fs::exists(root / "domains" / ".critical-authority-transaction"),
		"collector bootstrap did not recover death enrollment atomically: " + error);
	applied = flatfile_item_repository_apply(root_path, death_handoff);
	require(applied.outcome == critical_apply_outcome::already_applied,
		"death enrollment did not replay exactly");
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved;
	require(flatfile_world_item_list(root_path, &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].items.size() == 4,
		"death enrollment did not recover the corpse image");

	collector_listing_detail first = listing(root_path, 1, &error);
	collector_command_payload collect_first =
		collect_payload(root_path, first.entry, corpse_owner, snapshots[1], 1010, &error);
	const critical_command collect_first_command = collector_command(collect_first, 2);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_collector_repository_apply(root_path, collect_first_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains" / ".critical-authority-transaction"),
		"interrupted collector purchase did not retain its authority journal");
	economic_accounting_item_reference interrupted_reference = {};
	require(flatfile_item_accounting_reference_find_by_legacy(
			root_path, collect_first_command.operation_id, 0, &interrupted_reference,
			&error) == flatfile_item_accounting_status::not_found,
		"interrupted collector purchase published an item reference before recovery");
	collector_bootstrap_snapshot recovered_purchase;
	require(flatfile_collector_repository_read_bootstrap(root_path, &recovered_purchase,
							     &error) ==
				flatfile_collector_repository_result::ok &&
			!fs::exists(root / "domains" / ".critical-authority-transaction"),
		"collector bootstrap did not recover interrupted purchase");
	applied = flatfile_collector_repository_apply(root_path, collect_first_command);
	collector_command_result collected_first = collector_result(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			collected_first.entry.status == collector::state::collected &&
			collected_first.entry.item_revision == 3 &&
			collected_first.entry.price_value == 1000,
		"recovered collector purchase did not replay its result");
	std::vector<economic_accounting_item_reference> purchase_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			root_path, collect_first_command.operation_id, &purchase_references,
			&error) == flatfile_item_accounting_status::ok &&
			purchase_references.size() == collect_first.item_count &&
			purchase_references[0].item_uid == collect_first.items[0].item_uid &&
			purchase_references[0].before_revision ==
				collect_first.items[0].expected_item_revision &&
			purchase_references[0].after_revision ==
				collect_first.items[0].expected_item_revision + 1,
		"recovered collector purchase did not atomically retain its item reference");
	economic_accounting_item_reference reference = {};
	require(flatfile_item_accounting_reference_find_by_legacy(
			root_path, collect_first_command.operation_id, 1, &reference, &error) ==
				flatfile_item_accounting_status::ok &&
			reference.item_uid == 101 && reference.before_revision == 2 &&
			reference.after_revision == 3,
		"recovered collector command lost its exact item reference: " + error);
	applied = flatfile_collector_repository_apply(root_path, collect_first_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			collector_result(applied).entry.revision == collected_first.entry.revision,
		"collector collection replay changed its result");
	require(flatfile_world_item_list(root_path, &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses[0].items.size() == 3 && corpses[0].items[0].weight == 25 &&
			corpses[0].items[1].object_uid == 102 &&
			corpses[0].items[1].parent_index == 0,
		"first collection did not detach only the selected shell");

	collector_listing_detail second = listing(root_path, 2, &error);
	collector_command_payload collect_second =
		collect_payload(root_path, second.entry, corpse_owner, snapshots[2], 1010, &error);
	applied = flatfile_collector_repository_apply(root_path,
						      collector_command(collect_second, 3));
	require(applied.outcome == critical_apply_outcome::applied &&
			collector_result(applied).entry.item_revision == 4,
		"second candidate did not tolerate unrelated root revision advancement");
	require(flatfile_world_item_list(root_path, &corpses, &saved, &error) ==
				flatfile_world_item_result::ok &&
			corpses[0].items.size() == 2 && corpses[0].items[0].weight == 19 &&
			corpses[0].items[1].object_uid == 103,
		"second shell collection damaged retained corpse contents");

	collector_listing_detail third = listing(root_path, 3, &error);
	// Taking the remaining container must cancel every candidate in its captured
	// subtree in the same authority commit. Here only uid 103 remains eligible;
	// the ineligible container shell proves subtree traversal is not a UID-only
	// root shortcut.
	std::vector<flatfile_item_ownership_record> claim_items;
	item_transfer_payload claim = {};
	claim.from_owner = corpse_owner;
	claim.to_owner = player_owner;
	claim.reason = item_transfer_reason::corpse_loot;
	claim.reason_id = 1000;
	claim.expected_from_revision =
		owner_revision(root_path, corpse_owner, &claim_items, &error);
	std::vector<flatfile_item_ownership_record> current_player_items;
	claim.expected_to_revision =
		owner_revision(root_path, player_owner, &current_player_items, &error);
	claim.selected_item_uid = 100;
	claim.item_count = static_cast<uint16_t>(claim_items.size());
	for (size_t index = 0; index < claim_items.size(); ++index)
		claim.items[index] = { claim_items[index].item_uid,
				       claim_items[index].root_item_uid,
				       claim_items[index].parent_item_uid,
				       claim_items[index].item_revision,
				       claim_items[index].vnum,
				       claim_items[index].state };
	player_item_snapshot claimed_root = snapshots[0];
	claimed_root.weight = 19;
	const std::vector<player_item_snapshot> claimed_snapshots = { claimed_root, snapshots[3] };
	const std::vector<uint8_t> claim_blob = encode(claimed_snapshots);
	claim.item_blob_size = static_cast<uint32_t>(claim_blob.size());
	std::copy(claim_blob.begin(), claim_blob.end(), claim.item_blob.begin());
	claim.corpse = death.corpse;
	const critical_command claim_command = item_command(claim, 40);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error), "could not acquire claim preflight lock");
		flatfile_collector_enrollment_mutation mutation;
		unsigned int result_code = 0;
		const item_transfer_result transfer = { 100,
							claim.item_count,
							claim.expected_from_revision + 1,
							claim.expected_to_revision + 1,
							5,
							0 };
		const auto prepared = flatfile_collector_prepare_item_boundary(
			root_path, lock, claim, transfer, &mutation, &result_code, &error);
		require(prepared == flatfile_collector_repository_result::ok && !result_code &&
				mutation.cancelled == 1 && !mutation.after_image.bytes.empty(),
			"container cancellation preflight failed (result " +
				std::to_string(static_cast<unsigned int>(prepared)) + ", code " +
				std::to_string(result_code) + "): " + error);
	}
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_item_repository_apply(root_path, claim_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains" / ".critical-authority-transaction"),
		"interrupted container cancellation did not retain its authority journal");
	// This read first recovers the complete ownership+collector transaction.
	third = listing(root_path, 3, &error);
	require(!fs::exists(root / "domains" / ".critical-authority-transaction"),
		"container cancellation read did not finish journal recovery");
	applied = flatfile_item_repository_apply(root_path, claim_command);
	item_transfer_result claim_result = {};
	const bool claim_decoded = item_transfer_command_decode_result(
		applied.result_payload.data(), applied.result_size, &claim_result);
	require(applied.outcome == critical_apply_outcome::already_applied && claim_decoded &&
			claim_result.collector_catalog_changed,
		"container acquisition did not atomically invalidate the collector catalog "
		"(outcome " +
			std::to_string(static_cast<unsigned int>(applied.outcome)) + ", error " +
			std::to_string(applied.error_code) + ", decoded " +
			std::to_string(claim_decoded) + ", changed " +
			std::to_string(claim_result.collector_catalog_changed) + ")");
	require(third.entry.status == collector::state::cancelled &&
			third.entry.closed_reason == collector::reason::claimed &&
			third.entry.item_revision == claim_result.max_item_revision,
		"subtree candidate cancellation did not record the acquisition revision");
	applied = flatfile_item_repository_apply(root_path, claim_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			item_transfer_command_decode_result(applied.result_payload.data(),
							    applied.result_size, &claim_result) &&
			claim_result.collector_catalog_changed,
		"container acquisition replay lost its collector invalidation receipt");

	// The same durable UID may become eligible on a later death. The old record
	// stays terminal while a distinct death/listing is enrolled.
	const item_owner_identity second_corpse_owner = { item_owner_type::corpse,
							  item_corpse_owner_id(42, 1001), 0 };
	std::vector<flatfile_item_ownership_record> redeath_items;
	item_transfer_payload redeath = {};
	redeath.from_owner = player_owner;
	redeath.to_owner = second_corpse_owner;
	redeath.reason = item_transfer_reason::corpse_create;
	redeath.reason_id = 1001;
	redeath.expected_from_revision =
		owner_revision(root_path, player_owner, &redeath_items, &error);
	std::vector<flatfile_item_ownership_record> second_corpse_items;
	redeath.expected_to_revision =
		owner_revision(root_path, second_corpse_owner, &second_corpse_items, &error);
	redeath.selected_item_uid = 100;
	redeath.target_root_item_uid = 100;
	redeath.item_count = static_cast<uint16_t>(redeath_items.size());
	for (size_t index = 0; index < redeath_items.size(); ++index)
		redeath.items[index] = { redeath_items[index].item_uid,
					 redeath_items[index].root_item_uid,
					 redeath_items[index].parent_item_uid,
					 redeath_items[index].item_revision,
					 redeath_items[index].vnum,
					 redeath_items[index].state };
	redeath.item_blob_size = static_cast<uint32_t>(claim_blob.size());
	std::copy(claim_blob.begin(), claim_blob.end(), redeath.item_blob.begin());
	redeath.corpse = death.corpse;
	redeath.corpse.values[6] = 1001;
	redeath.collector.present = true;
	redeath.collector.death_operation = operation(41);
	redeath.collector.beneficiary_pid = 42;
	redeath.collector.death_time = 1001;
	redeath.collector.policy = death.collector.policy;
	redeath.collector.eligible_item_uids = { 103 };
	const critical_command redeath_command = item_command(redeath, 41);
	applied = flatfile_item_repository_apply(root_path, redeath_command);
	item_transfer_result redeath_result = {};
	require(applied.outcome == critical_apply_outcome::applied &&
			item_transfer_command_decode_result(applied.result_payload.data(),
							    applied.result_size, &redeath_result) &&
			redeath_result.collector_catalog_changed,
		"later death did not enroll the reused durable UID");
	const collector_listing_detail repeated = listing(root_path, 4, &error);
	require(repeated.entry.uid == third.entry.uid &&
			repeated.entry.status == collector::state::candidate &&
			!std::equal(repeated.entry.death_operation.begin(),
				    repeated.entry.death_operation.end(),
				    third.entry.death_operation.begin()),
		"later death did not preserve distinct listing history for the same UID");

	// A pet/NPC acquisition has no persistent mobile owner. Its same-owner
	// mobile_claim still advances the exact subtree and atomically closes the new
	// death record; reason_id records the controlling player when one exists.
	std::vector<flatfile_item_ownership_record> mobile_items;
	item_transfer_payload mobile_claim = {};
	mobile_claim.from_owner = second_corpse_owner;
	mobile_claim.to_owner = second_corpse_owner;
	mobile_claim.reason = item_transfer_reason::mobile_claim;
	mobile_claim.reason_id = 42;
	mobile_claim.expected_from_revision =
		owner_revision(root_path, second_corpse_owner, &mobile_items, &error);
	mobile_claim.expected_to_revision = mobile_claim.expected_from_revision;
	mobile_claim.selected_item_uid = 100;
	mobile_claim.item_count = static_cast<uint16_t>(mobile_items.size());
	for (size_t index = 0; index < mobile_items.size(); ++index)
		mobile_claim.items[index] = { mobile_items[index].item_uid,
					      mobile_items[index].root_item_uid,
					      mobile_items[index].parent_item_uid,
					      mobile_items[index].item_revision,
					      mobile_items[index].vnum,
					      mobile_items[index].state };
	mobile_claim.item_blob_size = static_cast<uint32_t>(claim_blob.size());
	std::copy(claim_blob.begin(), claim_blob.end(), mobile_claim.item_blob.begin());
	const critical_command mobile_claim_command = item_command(mobile_claim, 42);
	applied = flatfile_item_repository_apply(root_path, mobile_claim_command);
	item_transfer_result mobile_claim_result = {};
	require(applied.outcome == critical_apply_outcome::applied &&
			item_transfer_command_decode_result(applied.result_payload.data(),
							    applied.result_size,
							    &mobile_claim_result) &&
			mobile_claim_result.collector_catalog_changed &&
			mobile_claim_result.from_owner_revision ==
				mobile_claim.expected_from_revision + 1 &&
			mobile_claim_result.to_owner_revision ==
				mobile_claim_result.from_owner_revision,
		"same-owner mobile acquisition did not commit atomically");
	const collector_listing_detail mobile_cancelled = listing(root_path, 4, &error);
	require(mobile_cancelled.entry.status == collector::state::cancelled &&
			mobile_cancelled.entry.closed_reason == collector::reason::claimed &&
			mobile_cancelled.entry.item_revision ==
				mobile_claim_result.max_item_revision,
		"mobile acquisition did not close the captured candidate subtree");
	applied = flatfile_item_repository_apply(root_path, mobile_claim_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			item_transfer_command_decode_result(applied.result_payload.data(),
							    applied.result_size,
							    &mobile_claim_result) &&
			mobile_claim_result.collector_catalog_changed,
		"mobile acquisition replay lost its collector invalidation receipt");

	first = listing(root_path, 1, &error);
	applied = flatfile_collector_repository_apply(
		root_path,
		collector_command(metadata(collector_action::activate, first.entry, 1020), 5));
	require(applied.outcome == critical_apply_outcome::applied &&
			collector_result(applied).entry.status == collector::state::available,
		"first listing did not activate");
	const collector::record first_available = collector_result(applied).entry;
	const collector_command_payload hint =
		metadata(collector_action::hint, first_available, 1020);
	applied = flatfile_collector_repository_apply(root_path, collector_command(hint, 11));
	require(applied.outcome == critical_apply_outcome::applied &&
			collector_result(applied).entry.status == collector::state::available,
		"first available listing did not claim its recovery hint");
	require(flatfile_collector_repository_read_bootstrap(root_path, &bootstrap, &error) ==
				flatfile_collector_repository_result::ok &&
			bootstrap.deaths.size() == 2 &&
			bootstrap.deaths[0].hint_state == COLLECTOR_HINT_PENDING &&
			bootstrap.deaths[0].hint_revision == first_available.revision,
		"collector hint claim was not durable per death");
	second = listing(root_path, 2, &error);
	applied = flatfile_collector_repository_apply(
		root_path,
		collector_command(metadata(collector_action::activate, second.entry, 1020), 6));
	require(applied.outcome == critical_apply_outcome::applied,
		"second listing did not activate");
	collector_command_payload competing_hint =
		metadata(collector_action::hint, collector_result(applied).entry, 1020);
	applied = flatfile_collector_repository_apply(root_path,
						      collector_command(competing_hint, 12));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EALREADY,
		"second listing bypassed the per-death first-hint gate");
	collector_command_payload acknowledgement =
		metadata(collector_action::hint_ack, first_available, 1020);
	acknowledgement.expected_listing_revision = first_available.revision;
	applied = flatfile_collector_repository_apply(root_path,
						      collector_command(acknowledgement, 13));
	require(applied.outcome == critical_apply_outcome::applied,
		"collector hint acknowledgement was not durable");
	require(flatfile_collector_repository_read_bootstrap(root_path, &bootstrap, &error) ==
				flatfile_collector_repository_result::ok &&
			bootstrap.deaths[0].hint_state == COLLECTOR_HINT_DELIVERED,
		"collector hint acknowledgement did not survive a bootstrap read");
	second = listing(root_path, 2, &error);
	applied = flatfile_collector_repository_apply(
		root_path,
		collector_command(metadata(collector_action::pause, second.entry, 1030), 7));
	require(applied.outcome == critical_apply_outcome::applied &&
			collector_result(applied).entry.holding_paused,
		"available listing did not pause");
	second = listing(root_path, 2, &error);
	applied = flatfile_collector_repository_apply(
		root_path,
		collector_command(metadata(collector_action::resume, second.entry, 1040), 8));
	require(applied.outcome == critical_apply_outcome::applied &&
			collector_result(applied).entry.expires_at == 1130,
		"paused listing did not resume with shifted expiry");

	flatfile_player_domain_record beneficiary;
	require(flatfile_player_domain_load(root_path, 42, "beneficiary", 1, &beneficiary,
					    &error) == flatfile_player_domain_result::ok,
		"could not read beneficiary wallet");
	economic_account_key wallet_account, bank_account;
	enable_accounting(root_path, &wallet_account, &bank_account, &error);
	const fs::path cancel_root = root.parent_path() / "collector-cancel";
	fs::copy(root, cancel_root, fs::copy_options::recursive);
	const std::string cancel_path = cancel_root.string();
	const collector_listing_detail cancel_listing = listing(cancel_path, 2, &error);
	collector_command_payload cancel = held_payload(
		cancel_path, collector_action::cancel, cancel_listing,
		{ item_owner_type::system, 0, 0 }, item_custody_state::quarantined, 1130, &error);
	cancel.cancel_reason = collector::reason::quarantined;
	critical_command accounted_cancel = collector_command(cancel, 15);
	require(collector_held_accounting_intent(
			accounted_cancel, operation(70), operation(75), cancel_listing.entry,
			&accounted_cancel.accounting_intent) == economic_accounting_error::ok,
		"could not freeze collector cancellation intent");
	accounted_cancel.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_collector_repository_apply_accounted(cancel_path, accounted_cancel);
	const collector_command_result cancelled = collector_result(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			cancelled.entry.status == collector::state::cancelled &&
			cancelled.entry.closed_reason == collector::reason::quarantined &&
			cancelled.entry.item_revision == 5 && cancelled.to_owner_revision == 1,
		"accounted held cancellation did not quarantine the original item");
	flatfile_accounting_record cancel_record;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(cancel_path, &error), "could not lock cancellation lookup");
		require(flatfile_accounting_lookup(cancel_path, lock, accounted_cancel,
						   &cancel_record,
						   &error) == flatfile_accounting_status::ok &&
				!cancel_record.plan.empty(),
			"accounted held cancellation did not retain its plan: " + error);
	}
	economic_accounting_plan cancel_plan;
	require(economic_plan_decode(cancel_record.plan, &cancel_plan) ==
				economic_accounting_error::ok &&
			cancel_plan.accounts.empty() && cancel_plan.postings.empty() &&
			cancel_plan.item_events.size() == 1 &&
			cancel_plan.item_events[0].after.state == item_custody_state::quarantined,
		"accounted held cancellation lost its item-only plan");
	std::vector<economic_accounting_item_reference> cancel_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			cancel_path, accounted_cancel.operation_id, &cancel_references, &error) ==
				flatfile_item_accounting_status::ok &&
			cancel_references.size() == 1 && cancel_references[0].child_index == 0 &&
			cancel_references[0].item_uid == cancel.selected_item_uid &&
			cancel_references[0].after_revision == cancelled.entry.item_revision,
		"accounted held cancellation lost its exact item reference");
	applied = flatfile_collector_repository_apply_accounted(cancel_path, accounted_cancel);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			collector_result(applied).entry.item_revision ==
				cancelled.entry.item_revision,
		"accounted held cancellation did not replay exactly");
	second = listing(root_path, 2, &error);
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	collector_command_payload expire = held_payload(root_path, collector_action::expire, second,
							destruction, item_custody_state::destroyed,
							1130, &error);
	critical_command accounted_expire = collector_command(expire, 10);
	require(collector_held_accounting_intent(
			accounted_expire, operation(70), operation(75), second.entry,
			&accounted_expire.accounting_intent) == economic_accounting_error::ok,
		"could not freeze collector expiry intent");
	accounted_expire.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_collector_repository_apply_accounted(root_path, accounted_expire);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains" / ".critical-authority-transaction"),
		"interrupted accounted collector expiry did not retain its journal");
	flatfile_accounting_record held_retained;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error), "could not lock collector expiry lookup");
		require(flatfile_accounting_lookup(root_path, lock, accounted_expire,
						   &held_retained,
						   &error) == flatfile_accounting_status::ok &&
				!held_retained.plan.empty() && held_retained.result_code == 0,
			"accounted collector expiry did not recover its plan: " + error);
	}
	applied = flatfile_collector_repository_apply_accounted(root_path, accounted_expire);
	const collector_command_result expired = collector_result(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			expired.entry.status == collector::state::expired &&
			expired.entry.item_revision == 5 && expired.to_owner_revision == 1,
		"elapsed held listing did not expire");
	economic_accounting_plan held_plan;
	require(economic_plan_decode(held_retained.plan, &held_plan) ==
				economic_accounting_error::ok &&
			held_plan.accounts.empty() && held_plan.postings.empty() &&
			held_plan.item_events.size() == 1 &&
			held_plan.item_events[0].after.state == item_custody_state::destroyed,
		"accounted collector expiry lost its item-only plan");
	std::vector<economic_accounting_item_reference> held_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			root_path, accounted_expire.operation_id, &held_references, &error) ==
				flatfile_item_accounting_status::ok &&
			held_references.size() == 1 && held_references[0].child_index == 0 &&
			held_references[0].item_uid == expire.selected_item_uid &&
			held_references[0].after_revision == expired.entry.item_revision,
		"accounted collector expiry lost its exact item reference");
	first = listing(root_path, 1, &error);
	collector_command_payload purchase = held_payload(root_path, collector_action::purchase,
							  first, player_owner,
							  item_custody_state::active, 1021, &error);
	purchase.actor_pid = 42;
	purchase.racewar = 1;
	purchase.capacity_admitted = true;
	strcpy(purchase.account_name.data(), "beneficiary");
	purchase.expected_wallet_revision = beneficiary.domains.wallet_revision;
	purchase.expected_bank_revision = beneficiary.domains.bank_revision;
	critical_command accounted_purchase = collector_command(purchase, 9);
	require(collector_purchase_accounting_intent(accounted_purchase, operation(75),
						     wallet_account, bank_account, first.entry,
						     &accounted_purchase.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze collector purchase intent");
	accounted_purchase.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_collector_repository_apply_accounted(root_path, accounted_purchase);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(root / "domains" / ".critical-authority-transaction"),
		"interrupted accounted collector purchase did not retain authority intent");
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error), "could not lock collector replay lookup");
		require(flatfile_accounting_lookup(root_path, lock, accounted_purchase, &retained,
						   &error) == flatfile_accounting_status::ok &&
				!retained.plan.empty() && retained.result_code == 0,
			"accounted collector purchase did not recover its plan: " + error);
	}
	applied = flatfile_collector_repository_apply_accounted(root_path, accounted_purchase);
	collector_command_result purchased = collector_result(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			purchased.entry.status == collector::state::purchased &&
			purchased.entry.item_revision == 4 && purchased.wallet_revision == 1 &&
			purchased.bank_revision == 2,
		"beneficiary purchase did not atomically move item and payment");
	std::vector<economic_accounting_item_reference> accounted_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			root_path, accounted_purchase.operation_id, &accounted_references,
			&error) == flatfile_item_accounting_status::ok &&
			accounted_references.size() == 1 &&
			accounted_references[0].child_index == 0 &&
			accounted_references[0].item_uid == purchase.selected_item_uid &&
			accounted_references[0].after_revision == purchased.entry.item_revision,
		"accounted purchase did not retain its canonical item reference");
	critical_command stale_purchase = collector_command(purchase, 14);
	require(collector_purchase_accounting_intent(
			stale_purchase, operation(75), wallet_account, bank_account, first.entry,
			&stale_purchase.accounting_intent) == economic_accounting_error::ok,
		"could not freeze stale collector purchase intent");
	stale_purchase.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_collector_repository_apply_accounted(root_path, stale_purchase);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale accounted purchase was not durably rejected");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error), "could not lock stale purchase lookup");
		require(flatfile_accounting_lookup(root_path, lock, stale_purchase, &retained,
						   &error) == flatfile_accounting_status::ok &&
				retained.plan.empty() && retained.result_code == ESTALE,
			"stale accounted purchase did not retain its rejection");
	}
	applied = flatfile_collector_repository_apply_accounted(root_path, stale_purchase);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale accounted purchase replay changed its rejection");

	require(flatfile_collector_repository_read_bootstrap(root_path, &bootstrap, &error) ==
				flatfile_collector_repository_result::ok &&
			bootstrap.catalog.revision == 14 && bootstrap.catalog.records.size() == 4 &&
			bootstrap.held_items.empty(),
		"terminal collector catalog did not reconcile exact held custody: " + error);
	std::vector<flatfile_item_ownership_record> player_items;
	require(owner_revision(root_path, player_owner, &player_items, &error) == 5 &&
			player_items.size() == 1 && player_items[0].item_uid == 101 &&
			player_items[0].item_revision == 4,
		"purchased collector item was not in player custody");
	std::vector<flatfile_item_ownership_record> destroyed;
	require(owner_revision(root_path, destruction, &destroyed, &error) == 1 &&
			destroyed.empty(),
		"expired collector item was not durably destroyed");
	std::cout << "flatfile collector authority tests passed\n";
	return 0;
}
