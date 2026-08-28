#include "flatfile_artifact_repository.h"
#include "player_snapshot_codec.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
}

static void set_transfer_item(item_transfer_payload *payload, int32_t vnum, bool artifact)
{
	require(payload != nullptr, "transfer payload is required");
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = -1;
	item.object_uid = 123;
	item.vnum = vnum;
	item.extra_flags = artifact ? 1U << 28 : 0;
	std::vector<uint8_t> encoded;
	require(player_item_snapshot_list_encode({ item }, &encoded) ==
				player_snapshot_codec_result::ok &&
			encoded.size() <= payload->item_blob.size(),
		"could not encode transfer item");
	payload->selected_item_uid = item.object_uid;
	payload->item_count = 1;
	payload->items[0].item_uid = item.object_uid;
	payload->items[0].vnum = item.vnum;
	payload->item_blob_size = static_cast<uint32_t>(encoded.size());
	std::copy(encoded.begin(), encoded.end(), payload->item_blob.begin());
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "release";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_artifact_record> records;
	require(flatfile_artifact_list(root.string(), &records, &error) ==
			flatfile_artifact_result::not_found,
		"missing artifact authority did not fail closed");
	const fs::path ensure_root = fs::path(argv[1]) / "ensure";
	prepare_root(ensure_root);
	require(flatfile_artifact_ensure(ensure_root.string(), &error) ==
			flatfile_artifact_result::ok,
		"fresh artifact catalog ensure failed: " + error);
	require(flatfile_artifact_list(ensure_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.empty(),
		"fresh artifact catalog was not established empty");
	require(flatfile_artifact_gameplay_update(ensure_root.string(), 99, true,
						  FLATFILE_ARTIFACT_ON_GROUND, 1200, 5000, 1, 1000,
						  &error) == flatfile_artifact_result::ok,
		"ensured catalog did not accept its first gameplay artifact");
	require(flatfile_artifact_ensure(ensure_root.string(), &error) ==
			flatfile_artifact_result::already_exists,
		"existing artifact catalog was not recognized by ensure");
	require(flatfile_artifact_list(ensure_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].vnum == 99,
		"artifact ensure replaced an existing nonempty catalog");
	const flatfile_artifact_record held = {
		100, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 9000, 1, 1000, 42, 8000, 5
	};
	const flatfile_artifact_record bound = {
		200, false, FLATFILE_ARTIFACT_ON_GROUND, 1201, 0, 2, 1001, 42, 7000, 7
	};
	const flatfile_artifact_record untouched = {
		300, true, FLATFILE_ARTIFACT_ON_NPC, 81, 6000, 3, 1002, -1, 0, 2
	};
	require(flatfile_artifact_establish(root.string(), { untouched, held, bound }, &error) ==
			flatfile_artifact_result::ok,
		"artifact establishment failed: " + error);
	require(flatfile_artifact_establish(root.string(), { bound, untouched, held }, &error) ==
			flatfile_artifact_result::already_exists,
		"exact artifact establishment retry was not idempotent");
	auto conflicting = held;
	conflicting.timer++;
	require(flatfile_artifact_establish(root.string(), { conflicting }, &error) ==
			flatfile_artifact_result::invalid,
		"conflicting artifact establishment was accepted");
	require(flatfile_artifact_list(root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records == std::vector<flatfile_artifact_record>{ held, bound, untouched },
		"artifact catalog was not canonical or did not round trip");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 99,
								 &operation, &error) ==
				flatfile_artifact_result::unchanged,
			"unreferenced player did not produce an unchanged artifact result");
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
					flatfile_artifact_result::ok &&
				operation.store == flatfile_authority_store::domains &&
				operation.kind == flatfile_authority_operation_kind::write &&
				operation.filename == "artifact_catalog",
			"player artifact release did not prepare a catalog image");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"artifact release transaction failed: " + error);
	}
	require(flatfile_artifact_list(root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 3 && !records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			records[0].location == 0 && records[0].timer == 0 &&
			records[0].bind_owner_pid == -1 && records[0].bind_timer == 0 &&
			records[0].revision == 6 &&
			records[1].location_type == FLATFILE_ARTIFACT_ON_GROUND &&
			records[1].location == 1201 && records[1].bind_owner_pid == -1 &&
			records[1].bind_timer == 0 && records[1].revision == 8 &&
			records[2] == untouched,
		"artifact release did not preserve and rewrite the expected fields");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
				flatfile_artifact_result::unchanged,
			"artifact release retry was not idempotent");
	}

	const flatfile_artifact_record corpse = {
		400, true, FLATFILE_ARTIFACT_ON_CORPSE, 42, 5000, 1, 1003, 42, 4000, 1
	};
	const fs::path transfer_root = fs::path(argv[1]) / "corpse-transfer";
	prepare_root(transfer_root);
	require(flatfile_artifact_establish(transfer_root.string(), { corpse }, &error) ==
			flatfile_artifact_result::ok,
		"corpse transfer artifact establishment failed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(transfer_root.string(), &error),
			"could not acquire corpse artifact transfer authority");
		item_transfer_payload loot = {};
		loot.from_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 20), 0 };
		loot.to_owner = { item_owner_type::player, 77, 0 };
		loot.reason = item_transfer_reason::corpse_loot;
		set_transfer_item(&loot, 401, false);
		flatfile_artifact_transfer_mutation mutation;
		require(flatfile_artifact_prepare_corpse_transfer(
				transfer_root.string(), lock, loot, 10000000000ULL, &mutation,
				&error) == flatfile_artifact_result::unchanged,
			"ordinary corpse loot did not leave artifact authority unchanged");
		set_transfer_item(&loot, 401, true);
		require(flatfile_artifact_prepare_corpse_transfer(
				transfer_root.string(), lock, loot, 10000000000ULL, &mutation,
				&error) == flatfile_artifact_result::conflict,
			"artifact snapshot missing from the catalog did not fail closed");
		set_transfer_item(&loot, 400, true);
		require(flatfile_artifact_prepare_corpse_transfer(
				transfer_root.string(), lock, loot, 10000000000ULL, &mutation,
				&error) == flatfile_artifact_result::conflict,
			"historical artifact-bearing corpse loot did not fail closed");
		loot.corpse.present = true;
		loot.corpse.actor_racewar = 2;
		loot.corpse.values[5] = 1;
		require(flatfile_artifact_prepare_corpse_transfer(
				transfer_root.string(), lock, loot, 10000000000ULL, &mutation,
				&error) == flatfile_artifact_result::ok &&
				mutation.after_image.filename == "artifact_catalog",
			"cross-race artifact loot did not prepare its authority image");
		require(flatfile_authority_transaction_commit(transfer_root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"cross-race artifact loot transaction failed: " + error);
	}
	require(flatfile_artifact_list(transfer_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			records[0].location == 77 && records[0].timer == 442000 &&
			records[0].last_update == 10000 && records[0].bind_owner_pid == -1 &&
			records[0].bind_timer == 10000 && records[0].revision == 2,
		"cross-race artifact loot did not preserve feed and binding semantics");

	const fs::path corpse_root = fs::path(argv[1]) / "corpse";
	prepare_root(corpse_root);
	require(flatfile_artifact_establish(corpse_root.string(), { corpse }, &error) ==
			flatfile_artifact_result::ok,
		"corpse artifact establishment failed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(corpse_root.string(), &error),
			"could not acquire corpse artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(corpse_root.string(), lock, 42,
								 &operation, &error) ==
					flatfile_artifact_result::ok &&
				operation.filename == "artifact_catalog",
			"corpse-held artifact release was not prepared");
		require(flatfile_authority_transaction_commit_operations(corpse_root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"corpse-held artifact release failed: " + error);
	}
	require(flatfile_artifact_list(corpse_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && !records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			records[0].location == 0 && records[0].timer == 0 &&
			records[0].bind_owner_pid == -1 && records[0].bind_timer == 0 &&
			records[0].revision == 2,
		"corpse-held artifact was not released with player deletion semantics");

	const fs::path bind_root = fs::path(argv[1]) / "bind";
	prepare_root(bind_root);
	int32_t bind_owner = 99;
	int64_t bind_timer = 99;
	require(flatfile_artifact_bind_get(bind_root.string(), 500, &bind_owner, &bind_timer,
					   &error) == flatfile_artifact_result::not_found &&
			bind_owner == 0 && bind_timer == 0,
		"missing bind authority did not fail closed with cleared outputs");
	const flatfile_artifact_record bind_record = {
		500, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 5000, 1, 1000, 42, 4000, 3
	};
	require(flatfile_artifact_establish(bind_root.string(), { bind_record }, &error) ==
			flatfile_artifact_result::ok,
		"bind artifact establishment failed");
	require(flatfile_artifact_bind_get(bind_root.string(), 500, &bind_owner, &bind_timer,
					   &error) == flatfile_artifact_result::ok &&
			bind_owner == 42 && bind_timer == 4000,
		"artifact binding did not round trip");
	require(flatfile_artifact_bind_update(bind_root.string(), 500, 77, 4100, &error) ==
			flatfile_artifact_result::ok,
		"artifact binding update failed: " + error);
	require(flatfile_artifact_bind_update(bind_root.string(), 500, 77, 4100, &error) ==
			flatfile_artifact_result::unchanged,
		"identical artifact binding update was not idempotent");
	require(flatfile_artifact_bind_get(bind_root.string(), 500, &bind_owner, &bind_timer,
					   &error) == flatfile_artifact_result::ok &&
			bind_owner == 77 && bind_timer == 4100,
		"updated artifact binding did not round trip");
	require(flatfile_artifact_bind_update(bind_root.string(), 501, 77, 4100, &error) ==
			flatfile_artifact_result::not_found,
		"binding update synthesized a missing artifact");
	require(flatfile_artifact_bind_update(bind_root.string(), 500, -2, 4100, &error) ==
			flatfile_artifact_result::invalid,
		"invalid binding owner was accepted");
	require(flatfile_artifact_list(bind_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].bind_owner_pid == 77 &&
			records[0].bind_timer == 4100 && records[0].revision == 4,
		"binding update did not preserve the canonical artifact record");
	require(flatfile_artifact_bind_reset_all(bind_root.string(), &error) ==
			flatfile_artifact_result::ok,
		"binding reset-all failed: " + error);
	require(flatfile_artifact_list(bind_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].bind_owner_pid == -1 &&
			records[0].bind_timer == 0 && records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			records[0].location == 42 && records[0].timer == 5000 &&
			records[0].revision == 5,
		"binding reset-all did not preserve gameplay fields or advance revision");
	require(flatfile_artifact_bind_reset_all(bind_root.string(), &error) ==
			flatfile_artifact_result::unchanged,
		"identical binding reset-all was not idempotent");
	require(flatfile_artifact_repair_player_binding(bind_root.string(), 500, 9000, 2000, 2000,
							&error) == flatfile_artifact_result::ok,
		"player binding repair failed: " + error);
	require(flatfile_artifact_list(bind_root.string(), &records, &error) ==
				flatfile_artifact_result::ok &&
			records.size() == 1 && records[0].owned &&
			records[0].location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			records[0].location == 42 && records[0].timer == 9000 &&
			records[0].last_update == 2000 && records[0].bind_owner_pid == 42 &&
			records[0].bind_timer == 2000 && records[0].revision == 6,
		"player binding repair did not persist its complete atomic outcome");
	require(flatfile_artifact_repair_player_binding(bind_root.string(), 500, 10000, 3000, 3000,
							&error) ==
			flatfile_artifact_result::unchanged,
		"already-correct player binding was unnecessarily rewritten");
	require(flatfile_artifact_repair_player_binding(bind_root.string(), 501, 9000, 2000, 2000,
							&error) ==
			flatfile_artifact_result::not_found,
		"player binding repair synthesized a missing artifact");

	const fs::path gameplay_root = fs::path(argv[1]) / "gameplay";
	prepare_root(gameplay_root);
	flatfile_artifact_record gameplay_record;
	require(flatfile_artifact_get(gameplay_root.string(), 700, &gameplay_record, &error) ==
				flatfile_artifact_result::not_found &&
			gameplay_record == flatfile_artifact_record{},
		"missing gameplay authority did not fail closed with a cleared record");
	const flatfile_artifact_record gameplay_original = {
		700, false, FLATFILE_ARTIFACT_ON_GROUND, 1201, 5000, 2, 1000, 42, 4000, 5
	};
	require(flatfile_artifact_establish(gameplay_root.string(), { gameplay_original },
					    &error) == flatfile_artifact_result::ok,
		"gameplay artifact establishment failed");
	require(flatfile_artifact_get(gameplay_root.string(), 700, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record == gameplay_original,
		"gameplay artifact did not round trip");
	require(flatfile_artifact_gameplay_update(gameplay_root.string(), 700, true,
						  FLATFILE_ARTIFACT_ON_PLAYER, 77, 6000, 3, 1100,
						  &error) == flatfile_artifact_result::ok,
		"existing gameplay artifact update failed: " + error);
	require(flatfile_artifact_gameplay_update(gameplay_root.string(), 700, true,
						  FLATFILE_ARTIFACT_ON_PLAYER, 77, 6000, 3, 1100,
						  &error) == flatfile_artifact_result::unchanged,
		"identical gameplay artifact update was not idempotent");
	require(flatfile_artifact_get(gameplay_root.string(), 700, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
			gameplay_record.location == 77 && gameplay_record.timer == 6000 &&
			gameplay_record.type == 3 && gameplay_record.last_update == 1100 &&
			gameplay_record.bind_owner_pid == 42 &&
			gameplay_record.bind_timer == 4000 && gameplay_record.revision == 6,
		"gameplay update did not preserve binding or advance its revision");
	require(flatfile_artifact_gameplay_update(gameplay_root.string(), 701, true,
						  FLATFILE_ARTIFACT_ON_GROUND, 1202, 7000, 1, 1200,
						  &error) == flatfile_artifact_result::ok,
		"new gameplay artifact upsert failed: " + error);
	require(flatfile_artifact_get(gameplay_root.string(), 701, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.vnum == 701 && gameplay_record.bind_owner_pid == 0 &&
			gameplay_record.bind_timer == 0 && gameplay_record.revision == 1,
		"new gameplay artifact did not receive safe binding and revision defaults");
	require(flatfile_artifact_gameplay_update(gameplay_root.string(), 701, true, 99, 1202, 7000,
						  1, 1200,
						  &error) == flatfile_artifact_result::invalid,
		"invalid gameplay artifact location type was accepted");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 701, -1, 1, 1300, &error) ==
			flatfile_artifact_result::ok,
		"owned artifact removal failed: " + error);
	require(flatfile_artifact_get(gameplay_root.string(), 701, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			!gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			gameplay_record.location == -1 && gameplay_record.timer == 7000 &&
			gameplay_record.type == 1 && gameplay_record.last_update == 1300 &&
			gameplay_record.bind_owner_pid == -1 && gameplay_record.bind_timer == 0 &&
			gameplay_record.revision == 2,
		"owned artifact removal did not preserve gameplay data or clear binding");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 701, -1, 1, 1300, &error) ==
			flatfile_artifact_result::unchanged,
		"identical owned artifact removal was not idempotent");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 701, 77, 1, 1400, &error) ==
			flatfile_artifact_result::ok,
		"artifact corpse ownership update failed: " + error);
	require(flatfile_artifact_get(gameplay_root.string(), 701, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_ON_CORPSE &&
			gameplay_record.location == 77 && gameplay_record.timer == 7000 &&
			gameplay_record.bind_owner_pid == -1 && gameplay_record.bind_timer == 0 &&
			gameplay_record.revision == 3,
		"artifact corpse ownership did not preserve timer/type or clear binding");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 799, -1, 2, 1500, &error) ==
			flatfile_artifact_result::unchanged,
		"removing an untracked artifact synthesized a record");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 799, 88, 2, 1500, &error) ==
			flatfile_artifact_result::ok,
		"missing corpse-held artifact was not recovered");
	require(flatfile_artifact_get(gameplay_root.string(), 799, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_ON_CORPSE &&
			gameplay_record.location == 88 && gameplay_record.timer == 0 &&
			gameplay_record.type == 2 && gameplay_record.bind_owner_pid == -1 &&
			gameplay_record.revision == 1,
		"missing corpse-held artifact recovery did not use safe defaults");
	require(flatfile_artifact_remove_owned(gameplay_root.string(), 799, 88, 9, 1500, &error) ==
			flatfile_artifact_result::invalid,
		"owned artifact removal accepted an invalid type");
	require(flatfile_artifact_extend_timer(gameplay_root.string(), 701, 8000, 1600, &error) ==
			flatfile_artifact_result::ok,
		"artifact timer extension failed: " + error);
	require(flatfile_artifact_extend_timer(gameplay_root.string(), 701, 7500, 1700, &error) ==
			flatfile_artifact_result::ok,
		"artifact timer extension could not refresh last-update time");
	require(flatfile_artifact_get(gameplay_root.string(), 701, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.timer == 8000 && gameplay_record.last_update == 1700 &&
			gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_ON_CORPSE &&
			gameplay_record.location == 77 && gameplay_record.type == 1 &&
			gameplay_record.bind_owner_pid == -1 && gameplay_record.bind_timer == 0 &&
			gameplay_record.revision == 5,
		"artifact timer extension lowered its timer or changed non-timer fields");
	require(flatfile_artifact_extend_timer(gameplay_root.string(), 701, 7500, 1700, &error) ==
			flatfile_artifact_result::unchanged,
		"identical artifact timer extension was not idempotent");
	require(flatfile_artifact_extend_timer(gameplay_root.string(), 798, 8000, 1700, &error) ==
			flatfile_artifact_result::not_found,
		"artifact timer extension synthesized a missing artifact");
	require(flatfile_artifact_extend_timer(gameplay_root.string(), 701, 0, 1700, &error) ==
			flatfile_artifact_result::invalid,
		"artifact timer extension accepted a zero minimum");
	require(flatfile_artifact_erase(gameplay_root.string(), 799, &error) ==
			flatfile_artifact_result::ok,
		"artifact erase failed: " + error);
	require(flatfile_artifact_get(gameplay_root.string(), 799, &gameplay_record, &error) ==
			flatfile_artifact_result::not_found,
		"erased artifact remained in canonical authority");
	require(flatfile_artifact_erase(gameplay_root.string(), 799, &error) ==
			flatfile_artifact_result::not_found,
		"identical artifact erase was not safely reported as missing");
	flatfile_artifact_record expired_record;
	require(flatfile_artifact_find_next_expired(gameplay_root.string(), 0, 6500,
						    &expired_record,
						    &error) == flatfile_artifact_result::ok &&
			expired_record.vnum == 700,
		"first expired artifact was not selected in vnum order");
	require(flatfile_artifact_find_next_expired(gameplay_root.string(), 700, 8001,
						    &expired_record,
						    &error) == flatfile_artifact_result::ok &&
			expired_record.vnum == 701,
		"expired artifact cursor did not advance by vnum");
	require(flatfile_artifact_expire(gameplay_root.string(), 701, 6500, &error) ==
			flatfile_artifact_result::unchanged,
		"non-expired artifact was cleared");
	require(flatfile_artifact_expire(gameplay_root.string(), 700, 6500, &error) ==
			flatfile_artifact_result::ok,
		"expired artifact was not cleared: " + error);
	require(flatfile_artifact_expire(gameplay_root.string(), 700, 6500, &error) ==
			flatfile_artifact_result::unchanged,
		"expired artifact clear was not idempotent");
	require(flatfile_artifact_get(gameplay_root.string(), 700, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			!gameplay_record.owned &&
			gameplay_record.location_type == FLATFILE_ARTIFACT_NOT_IN_GAME &&
			gameplay_record.location == -1 && gameplay_record.timer == 0 &&
			gameplay_record.last_update == 6500 && gameplay_record.type == 3 &&
			gameplay_record.bind_owner_pid == 42 &&
			gameplay_record.bind_timer == 4000 && gameplay_record.revision == 7,
		"expiry did not clear gameplay fields while preserving type and binding");
	require(flatfile_artifact_find_next_expired(gameplay_root.string(), 0, 6500,
						    &expired_record,
						    &error) == flatfile_artifact_result::not_found,
		"cleared or future artifact remained in the expired selection");

	const fs::path wars_root = fs::path(argv[1]) / "wars";
	prepare_root(wars_root);
	const std::vector<flatfile_artifact_record> war_records = {
		{ 800, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 2000, 1, 1000, 42, 500, 1 },
		{ 801, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 3001, 1, 1000, 42, 500, 1 },
		{ 802, true, FLATFILE_ARTIFACT_ON_PLAYER, 42, 0, 2, 1000, 42, 500, 1 },
		{ 803, true, FLATFILE_ARTIFACT_ON_PLAYER, 43, 5000, 1, 1000, 43, 500, 1 },
		{ 804, true, FLATFILE_ARTIFACT_ON_PLAYER, 44, 4000, 3, 1000, 44, 500, 1 },
		{ 805, true, FLATFILE_ARTIFACT_ON_PLAYER, 44, 5000, 3, 1000, 44, 500, 1 },
		{ 806, false, FLATFILE_ARTIFACT_ON_PLAYER, 45, 4000, 2, 1000, 45, 500, 1 },
		{ 807, true, FLATFILE_ARTIFACT_ON_PLAYER, 45, 5000, 2, 1000, 45, 500, 1 },
	};
	require(flatfile_artifact_establish(wars_root.string(), war_records, &error) ==
			flatfile_artifact_result::ok,
		"artifact-war establishment failed: " + error);
	std::vector<flatfile_artifact_war_owner> war_owners;
	require(flatfile_artifact_war_owners(wars_root.string(), 0, 2, &war_owners, &error) ==
				flatfile_artifact_result::ok &&
			war_owners.size() == 2 && war_owners[0].pid == 42 &&
			war_owners[0].total == 3 && war_owners[0].major == 2 &&
			war_owners[0].unique == 1 && war_owners[0].ioun == 0 &&
			war_owners[1].pid == 44,
		"artifact-war owners were not grouped, filtered, or bounded by pid");
	require(flatfile_artifact_war_owners(wars_root.string(), 42, 2, &war_owners, &error) ==
				flatfile_artifact_result::ok &&
			war_owners.size() == 2 && war_owners[0].pid == 44 &&
			war_owners[0].ioun == 2 && war_owners[1].pid == 45 &&
			war_owners[1].unique == 2,
		"artifact-war owner cursor did not preserve database grouping semantics");
	require(flatfile_artifact_apply_war_burn(wars_root.string(), 42, 1000, 0.5, 1100, &error) ==
			flatfile_artifact_result::ok,
		"artifact-war timer burn failed: " + error);
	require(flatfile_artifact_get(wars_root.string(), 800, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.timer == 1500 && gameplay_record.last_update == 1100 &&
			gameplay_record.bind_owner_pid == 42 && gameplay_record.bind_timer == 500 &&
			gameplay_record.revision == 2,
		"artifact-war burn did not reduce the first future timer safely");
	require(flatfile_artifact_get(wars_root.string(), 801, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.timer == 2000 && gameplay_record.revision == 2,
		"artifact-war burn did not apply database floor semantics");
	require(flatfile_artifact_get(wars_root.string(), 802, &gameplay_record, &error) ==
				flatfile_artifact_result::ok &&
			gameplay_record.timer == 0 && gameplay_record.last_update == 1000 &&
			gameplay_record.revision == 1,
		"artifact-war burn changed a null-equivalent timer");
	require(flatfile_artifact_apply_war_burn(wars_root.string(), 99, 1000, 0.5, 1100, &error) ==
			flatfile_artifact_result::unchanged,
		"artifact-war burn changed an unreferenced owner");
	require(flatfile_artifact_apply_war_burn(wars_root.string(), 42, 1000, -0.1, 1100,
						 &error) == flatfile_artifact_result::invalid,
		"artifact-war burn accepted an invalid retained fraction");

	const fs::path catalog = root / "domains/artifact_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open artifact catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_artifact_list(root.string(), &records, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed");
	require(flatfile_artifact_bind_get(root.string(), 100, &bind_owner, &bind_timer, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed through binding lookup");
	require(flatfile_artifact_bind_update(root.string(), 100, 42, 5000, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through binding update");
	require(flatfile_artifact_bind_reset_all(root.string(), &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through binding reset-all");
	require(flatfile_artifact_repair_player_binding(root.string(), 100, 9000, 2000, 2000,
							&error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through binding repair");
	require(flatfile_artifact_get(root.string(), 100, &gameplay_record, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed through gameplay lookup");
	require(flatfile_artifact_erase(root.string(), 100, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through erase");
	require(flatfile_artifact_gameplay_update(root.string(), 100, true,
						  FLATFILE_ARTIFACT_ON_PLAYER, 42, 5000, 1, 1200,
						  &error) == flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through gameplay update");
	require(flatfile_artifact_remove_owned(root.string(), 100, -1, 1, 1200, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through owned removal");
	require(flatfile_artifact_extend_timer(root.string(), 100, 6000, 1200, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through timer extension");
	require(flatfile_artifact_find_next_expired(root.string(), 0, 6000, &expired_record,
						    &error) == flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed through expiry selection");
	require(flatfile_artifact_expire(root.string(), 100, 6000, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through expiry");
	require(flatfile_artifact_war_owners(root.string(), 0, 4, &war_owners, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was exposed through war grouping");
	require(flatfile_artifact_apply_war_burn(root.string(), 42, 1000, 0.5, 1100, &error) ==
			flatfile_artifact_result::invalid,
		"corrupt artifact authority was overwritten through war timer burn");
	require(flatfile_artifact_ensure(root.string(), &error) ==
			flatfile_artifact_result::invalid,
		"artifact ensure accepted or overwrote corrupt authority");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire corrupt artifact authority");
		flatfile_authority_operation operation;
		require(flatfile_artifact_prepare_player_release(root.string(), lock, 42,
								 &operation, &error) ==
				flatfile_artifact_result::invalid,
			"corrupt artifact authority was accepted for deletion");
	}
	std::cout << "flat-file artifact repository passed\n";
	return 0;
}
