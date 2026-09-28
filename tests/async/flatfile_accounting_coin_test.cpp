#include "phase8_bank_fixture.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_accounting_pile_baseline.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_item_repository.h"
#include "economy/coin_transfer_accounting.h"
#include "core/defines.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>

flatfile_player_domain_record peer_state(const std::string &root)
{
	flatfile_player_domain_record value;
	assert(flatfile_player_domain_load(root, 2, "account-one", 1, &value, nullptr) ==
	       flatfile_player_domain_result::ok);
	return value;
}

void establish_peer(const std::string &root)
{
	int32_t pid = 0;
	assert(flatfile_identity_allocate_pid(root, &pid, nullptr) ==
		       flatfile_identity_result::ok &&
	       pid == 2);
	assert(flatfile_identity_claim(root, pid, "Peer", "ACCOUNT-ONE", nullptr) ==
	       flatfile_identity_result::ok);
	flatfile_identity_record first, second;
	assert(flatfile_identity_lookup_pid(root, 1, &first, nullptr) ==
	       flatfile_identity_result::ok);
	assert(flatfile_identity_lookup_pid(root, 2, &second, nullptr) ==
	       flatfile_identity_result::ok);
	second.racewar = 1;
	assert(flatfile_identity_sync_account(root, "account-one", { first, second }, nullptr) ==
	       flatfile_identity_result::ok);
	flatfile_player_domain_record peer;
	peer.pid = 2;
	peer.account_name = "account-one";
	peer.racewar = 1;
	peer.domains.bank = { 100, 20, 3, 1 };
	assert(flatfile_player_domain_establish(root, peer, nullptr) ==
	       flatfile_player_domain_result::ok);
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	initialize_bucket(root, lock, bucket(1, 0, 2, {}));
	flatfile_economic_mapping mapping;
	ops changes;
	assert(access_type::create(root, lock, control(root, lock).revision,
				   economic_account_kind::wallet, 0, { 1, 2, {} }, id(90009),
				   &mapping, &changes, nullptr) == 0);
	assert(mapping.account.authority_id == 3);
	commit(root, lock, changes);
}

coin_transfer_endpoint endpoint(uint32_t pid, const flatfile_player_domain_record &before,
				std::array<int32_t, 4> after, uint64_t operation)
{
	coin_transfer_endpoint value;
	for (size_t index = 0; index < 4; ++index)
		value.before[index] = before.domains.wallet[index];
	value.after = after;
	currency_command_payload change = {};
	change.pid = pid;
	change.racewar = 1;
	change.reason = currency_reason_type::coin_transfer;
	strcpy(change.account_name.data(), "ACCOUNT-ONE");
	for (size_t index = 0; index < 4; ++index)
		change.wallet_delta.amount[index] =
			static_cast<int64_t>(after[index]) - value.before[index];
	assert(currency_command_build(&value.change, id(operation), change,
				      before.domains.wallet_revision, before.domains.bank_revision,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	return value;
}

coin_transfer_endpoint room_pile(const std::string &root, uint64_t uid,
				 std::array<int32_t, 4> before, std::array<int32_t, 4> after,
				 uint64_t operation)
{
	const item_owner_identity room = { item_owner_type::room, 987654321, 0 };
	const bool created = before == std::array<int32_t, 4>{};
	const bool consumed = after == std::array<int32_t, 4>{};
	coin_transfer_endpoint value;
	value.before = before;
	value.after = after;
	item_transfer_payload transfer = {};
	transfer.from_owner = created ? item_owner_identity{ item_owner_type::system, 0, 0 } : room;
	transfer.to_owner = consumed ? item_owner_identity{ item_owner_type::destruction, 0, 0 } :
				       room;
	transfer.reason = created  ? item_transfer_reason::creation :
			  consumed ? item_transfer_reason::destruction :
				     item_transfer_reason::player_put;
	uint64_t room_revision = 0;
	std::vector<flatfile_item_ownership_record> owned;
	assert(flatfile_item_repository_load_owner(root, room, &room_revision, &owned, nullptr) ==
	       flatfile_item_repository_result::ok);
	if (created)
	{
		std::vector<flatfile_item_ownership_record> system_items;
		uint64_t system_revision = 0;
		const auto system = item_owner_identity{ item_owner_type::system, 0, 0 };
		const auto status = flatfile_item_repository_load_owner(
			root, system, &system_revision, &system_items, nullptr);
		assert(status == flatfile_item_repository_result::ok ||
		       status == flatfile_item_repository_result::not_found);
		transfer.expected_from_revision = system_revision;
	}
	else
		transfer.expected_from_revision = room_revision;
	if (consumed)
	{
		std::vector<flatfile_item_ownership_record> destroyed_items;
		uint64_t destruction_revision = 0;
		const auto destruction = item_owner_identity{ item_owner_type::destruction, 0, 0 };
		const auto status = flatfile_item_repository_load_owner(
			root, destruction, &destruction_revision, &destroyed_items, nullptr);
		assert(status == flatfile_item_repository_result::ok ||
		       status == flatfile_item_repository_result::not_found);
		transfer.expected_to_revision = destruction_revision;
	}
	else
		transfer.expected_to_revision = room_revision;
	transfer.selected_item_uid = uid;
	transfer.target_root_item_uid = uid;
	transfer.item_count = 1;
	uint64_t item_revision = ITEM_TRANSFER_ABSENT_REVISION;
	if (!created)
	{
		const auto found = std::find_if(owned.begin(), owned.end(), [uid](const auto &item)
						{ return item.item_uid == uid; });
		assert(found != owned.end());
		item_revision = found->item_revision;
	}
	transfer.items[0] = {
		uid,	uid,
		0,	item_revision,
		402013, created ? item_custody_state::absent : item_custody_state::active
	};
	player_item_snapshot snapshot = {};
	snapshot.object_uid = uid;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.equipment_slot = -1;
	snapshot.vnum = 402013;
	snapshot.type = ITEM_MONEY;
	for (size_t index = 0; index < 4; ++index)
		snapshot.values[index] = consumed ? before[index] : after[index];
	snapshot.name = "coins";
	snapshot.string_mask = 1;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ snapshot }, &blob) ==
	       player_snapshot_codec_result::ok);
	transfer.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), transfer.item_blob.begin());
	assert(item_transfer_command_build(&value.change, id(operation), transfer,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	return value;
}

void preexisting_pile_journey(const fs::path &path)
{
	setup(path, false);
	const auto root = path.string();
	const item_owner_identity owner{ item_owner_type::player, 1, 0 };
	player_item_snapshot coin = {};
	coin.object_uid = 700;
	coin.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	coin.equipment_slot = -1;
	coin.vnum = 402013;
	coin.type = ITEM_MONEY;
	coin.values[0] = 1;
	coin.name = "coins";
	coin.string_mask = 1;
	std::vector<uint8_t> payload;
	assert(player_item_snapshot_list_encode({ coin }, &payload) ==
	       player_snapshot_codec_result::ok);
	flatfile_item_ownership_record native;
	native.item_uid = native.root_item_uid = 700;
	native.owner = owner;
	native.item_revision = 1;
	native.vnum = coin.vnum;
	native.state = item_custody_state::active;
	native.coin_payload = payload;
	assert(flatfile_item_repository_establish_owner(root, owner, { native }, nullptr) ==
	       flatfile_item_baseline_result::applied);
	flatfile_accounting_pile_baseline_source source;
	flatfile_accounting_pile_state head;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		std::vector<flatfile_coin_pile_source> catalog_piles;
		assert(flatfile_item_repository_list_coin_piles_locked(root, lock, &catalog_piles,
								       nullptr) ==
			       flatfile_item_repository_result::ok &&
		       catalog_piles.size() == 1 && catalog_piles[0].ownership.item_uid == 700 &&
		       catalog_piles[0].item.values[0] == 1);
		assert(flatfile_accounting_pile_baseline_capture(root, lock, id(90001), id(90005),
								 id(91001), 700, &source,
								 nullptr) == 0);
		assert(source.holding.balance == (economic_coin_vector{ 1, 0, 0, 0 }) &&
		       source.holding.native_revision == 1 &&
		       source.item.snapshot.position.owner.type == item_owner_type::player &&
		       source.item.snapshot.position.revision == 1 &&
		       source.holding.source_digest == source.item.source_digest);
		const auto unchanged = source;
		assert(flatfile_accounting_pile_baseline_capture(root, lock, id(90001), id(90005),
								 id(91001), 701, &source,
								 nullptr) == ENOENT &&
		       source.holding.account.authority_id ==
			       unchanged.holding.account.authority_id);
		economic_baseline_batch batch;
		batch.lineage = id(90001);
		batch.epoch = id(90005);
		batch.preparation_id = id(91001);
		batch.actor_id = 1;
		batch.opening_account = { id(90001), economic_account_kind::opening, 900, 0 };
		batch.boundary_digest[0] = 1;
		batch.coverage_digest[0] = 1;
		batch.holdings.push_back(source.holding);
		batch.items.push_back(source.item);
		std::optional<economic_prepared_baseline> prepared;
		assert(economic_baseline_prepare(batch, &prepared) ==
		       economic_accounting_error::ok);
		critical_command baseline;
		assert(economic_baseline_command_build(*prepared, 12, &baseline) ==
		       economic_accounting_error::ok);
		const auto slot = baseline.operation_id.bytes[0];
		auto state = control(root, lock);
		if (!(state.evidence_initialized[slot / 8] & (1U << (slot % 8))))
		{
			ops changes;
			assert(access_type::initialize_evidence(root, lock, state.revision, slot,
								id(91002), &changes, nullptr) == 0);
			commit(root, lock, changes);
		}
		ops changes;
		assert(access_type::baseline_initialize(root, lock, batch.lineage, batch.epoch,
							batch.opening_account, id(91003), &changes,
							nullptr) == flatfile_accounting_status::ok);
		commit(root, lock, changes);
		assert(access_type::baseline_stage(root, lock, baseline, *prepared, &changes,
						   nullptr) == flatfile_accounting_status::ok);
		assert(flatfile_accounting_pile_state_stage_baseline(root, lock, source.head,
								     &changes, nullptr) ==
		       flatfile_accounting_status::ok);
		commit(root, lock, changes);
		flatfile_accounting_record receipt;
		std::vector<uint8_t> witness;
		assert(flatfile_accounting_baseline_lookup(root, lock, baseline, &receipt, &witness,
							   nullptr) ==
		       flatfile_accounting_status::ok);
		assert(receipt.result_code == 0 && receipt.durable_revision == 1);
		economic_accounting_plan opening_plan;
		assert(economic_plan_decode(receipt.plan, &opening_plan) ==
		       economic_accounting_error::ok);
		assert(opening_plan.accounts.size() == 2 && opening_plan.postings.size() == 2 &&
		       opening_plan.postings[0].copper + opening_plan.postings[1].copper == 0 &&
		       opening_plan.items_before.size() == 1 &&
		       opening_plan.items_before[0].uid == 700);
		assert(flatfile_accounting_pile_state_read(root, lock, 700, &head, nullptr) ==
			       flatfile_accounting_status::ok &&
		       head.balance == source.holding.balance);
		assert(flatfile_accounting_pile_baseline_capture(root, lock, id(90001), id(90005),
								 id(91001), 700, &source,
								 nullptr) == EALREADY);
		assert(access_type::select_epoch(root, lock, control(root, lock).revision, true,
						 id(91004), &changes, nullptr) == 0);
		commit(root, lock, changes);
	}
	// The isolated fixture can now spend the opening pile. Its owner catalog
	// retains the exact payload, so no legacy player snapshot is inferred.
	coin_transfer_endpoint pile;
	pile.before[0] = 1;
	item_transfer_payload release = {};
	release.from_owner = owner;
	release.to_owner = { item_owner_type::destruction, 0, 0 };
	release.reason = item_transfer_reason::destruction;
	release.selected_item_uid = release.target_root_item_uid = 700;
	release.expected_from_revision = 1;
	release.item_count = 1;
	release.items[0] = { 700, 700, 0, 1, 402013, item_custody_state::active };
	release.item_blob_size = payload.size();
	std::copy(payload.begin(), payload.end(), release.item_blob.begin());
	assert(item_transfer_command_build(&pile.change, id(91005), release,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	const auto wallet_before = ::state(root);
	coin_transfer_payload transfer;
	transfer.source = pile;
	transfer.destination = endpoint(1, wallet_before, { 101, 20, 3, 1 }, 91006);
	critical_command pickup;
	assert(coin_transfer_command_build(&pickup, id(91007), transfer,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(pickup, id(90005), source.holding.account, wallet(),
					       &pickup.accounting_intent) ==
	       economic_accounting_error::ok);
	pickup.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	pickup.accepted_at_usec = 12;
	pickup.publication_required = true;
	const auto applied = flatfile_accounting_coin_transaction::apply(root, pickup);
	assert(applied.outcome == outcome::applied && !applied.error_code);
	assert(::state(root).domains.wallet[0] == 101);
	assert(flatfile_accounting_coin_transaction::apply(root, pickup).outcome ==
	       outcome::already_applied);
	{
		flatfile_authority_lock verify_lock;
		assert(verify_lock.acquire(root, nullptr));
		assert(flatfile_accounting_pile_state_read(root, verify_lock, 700, &head,
							   nullptr) ==
			       flatfile_accounting_status::ok &&
		       head.retired && head.item_revision == 2);
		std::vector<flatfile_coin_pile_source> remaining;
		assert(flatfile_item_repository_list_coin_piles_locked(root, verify_lock,
								       &remaining, nullptr) ==
			       flatfile_item_repository_result::ok &&
		       remaining.empty());
	}
	const auto negative_path = path.parent_path() / "negative-pile";
	setup(negative_path, false);
	coin.values[0] = -1;
	assert(player_item_snapshot_list_encode({ coin }, &payload) ==
	       player_snapshot_codec_result::ok);
	native.coin_payload = payload;
	assert(flatfile_item_repository_establish_owner(negative_path.string(), owner, { native },
							nullptr) ==
	       flatfile_item_baseline_result::applied);
	flatfile_authority_lock negative_lock;
	assert(negative_lock.acquire(negative_path.string(), nullptr));
	std::vector<flatfile_coin_pile_source> invalid_piles;
	assert(flatfile_item_repository_list_coin_piles_locked(
		       negative_path.string(), negative_lock, &invalid_piles, nullptr) ==
		       flatfile_item_repository_result::invalid &&
	       invalid_piles.empty());
	assert(flatfile_accounting_pile_baseline_capture(negative_path.string(), negative_lock,
							 id(90001), id(90005), id(91001), 700,
							 &source, nullptr) == EILSEQ);
	assert(flatfile_accounting_pile_state_read(negative_path.string(), negative_lock, 700,
						   &head, nullptr) ==
	       flatfile_accounting_status::not_found);
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path path = argv[1];
	setup(path);
	const auto root = path.string();
	establish_peer(root);
	const auto first = state(root), second = peer_state(root);
	assert(first.domains.bank_revision == second.domains.bank_revision);
	coin_transfer_payload transfer;
	transfer.source = endpoint(1, first, { 99, 20, 3, 1 }, 81001);
	transfer.destination = endpoint(2, second, { 1, 0, 0, 0 }, 81002);
	critical_command command;
	const char *reason = nullptr;
	if (!coin_transfer_command_build(&command, id(201), transfer, critical_source_site::command,
					 critical_deadline_class::interactive, &reason))
	{
		std::cerr << "flatfile coin build: " << (reason ? reason : "unknown") << '\n';
		assert(false);
	}
	assert(coin_transfer_accounting_intent(command, id(90005), wallet(),
					       { id(90001), economic_account_kind::wallet, 3, 0 },
					       &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accepted_at_usec = 12;
	command.publication_required = true;
	assert(critical_command_envelope_valid(command));
	const auto applied = flatfile_accounting_coin_transaction::apply(root, command);
	if (applied.outcome != outcome::applied || applied.error_code)
		std::cerr << "flatfile peer apply outcome="
			  << static_cast<unsigned int>(applied.outcome)
			  << " error=" << applied.error_code << '\n';
	assert(applied.outcome == outcome::applied && !applied.error_code);
	const auto after_first = state(root), after_second = peer_state(root);
	assert(after_first.domains.wallet[0] == 99 && after_second.domains.wallet[0] == 1);
	assert(after_first.domains.wallet_revision == first.domains.wallet_revision + 1 &&
	       after_second.domains.wallet_revision == second.domains.wallet_revision + 1);
	assert(after_first.domains.bank_revision == first.domains.bank_revision + 2 &&
	       after_second.domains.bank_revision == after_first.domains.bank_revision &&
	       after_first.domains.bank == first.domains.bank &&
	       after_second.domains.bank == second.domains.bank);
	const auto stored = retained(root, command);
	economic_accounting_plan plan;
	assert(economic_plan_decode(stored.plan, &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 2 && plan.postings.size() == 2 && plan.children.empty() &&
	       plan.item_events.empty());
	assert(plan.accounts[0].before[0] == 100 && plan.accounts[0].after[0] == 99 &&
	       plan.accounts[1].before[0] == 0 && plan.accounts[1].after[0] == 1);
	assert(plan.postings[0].copper + plan.postings[1].copper == 0);
	const auto replay = flatfile_accounting_coin_transaction::apply(root, command);
	assert(replay.outcome == outcome::already_applied);
	same(applied, replay);
	auto changed = command;
	++changed.accepted_at_usec;
	assert(flatfile_accounting_coin_transaction::apply(root, changed).error_code == EEXIST);
	assert(flatfile_player_domain_restore_recover(root, nullptr) ==
	       flatfile_player_domain_result::ok);
	const auto restarted = flatfile_accounting_coin_transaction::apply(root, command);
	assert(restarted.outcome == outcome::already_applied);
	same(applied, restarted);
	assert(state(root).domains.wallet == after_first.domains.wallet &&
	       peer_state(root).domains.wallet == after_second.domains.wallet);
	critical_command stale;
	assert(coin_transfer_command_build(&stale, id(202), transfer, critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(stale, id(90005), wallet(),
					       { id(90001), economic_account_kind::wallet, 3, 0 },
					       &stale.accounting_intent) ==
	       economic_accounting_error::ok);
	stale.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	stale.accepted_at_usec = 12;
	stale.publication_required = true;
	const auto rejected = flatfile_accounting_coin_transaction::apply(root, stale);
	assert(rejected.outcome == outcome::terminal_failure && rejected.error_code == ESTALE);
	const auto rejected_record = retained(root, stale);
	assert(rejected_record.result_code == ESTALE && rejected_record.plan.empty());
	const auto rejected_replay = flatfile_accounting_coin_transaction::apply(root, stale);
	assert(rejected_replay.outcome == outcome::terminal_failure &&
	       rejected_replay.error_code == ESTALE);
	same(rejected, rejected_replay);
	assert(state(root).domains.wallet == after_first.domains.wallet &&
	       peer_state(root).domains.wallet == after_second.domains.wallet);
	coin_transfer_endpoint created_pile;
	created_pile.after[0] = 1;
	item_transfer_payload pile = {};
	pile.from_owner = { item_owner_type::system, 0, 0 };
	pile.to_owner = { item_owner_type::room, 987654321, 0 };
	pile.reason = item_transfer_reason::creation;
	pile.selected_item_uid = 500;
	pile.target_root_item_uid = 500;
	pile.item_count = 1;
	pile.items[0] = { 500,	  500,
			  0,	  ITEM_TRANSFER_ABSENT_REVISION,
			  402013, item_custody_state::absent };
	player_item_snapshot snapshot = {};
	snapshot.object_uid = 500;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.equipment_slot = -1;
	snapshot.vnum = 402013;
	snapshot.type = ITEM_MONEY;
	snapshot.values[0] = 1;
	snapshot.name = "coins";
	snapshot.string_mask = 1;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ snapshot }, &blob) ==
	       player_snapshot_codec_result::ok);
	pile.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), pile.item_blob.begin());
	assert(item_transfer_command_build(&created_pile.change, id(81006), pile,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	coin_transfer_payload drop_payload;
	drop_payload.source = endpoint(1, after_first, { 98, 20, 3, 1 }, 81005);
	drop_payload.destination = created_pile;
	critical_command drop_command;
	assert(coin_transfer_command_build(&drop_command, id(204), drop_payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(drop_command, id(90005), wallet(),
					       { id(90001), economic_account_kind::pile, 500, 0 },
					       &drop_command.accounting_intent) ==
	       economic_accounting_error::ok);
	drop_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	drop_command.accepted_at_usec = 12;
	drop_command.publication_required = true;
	const auto deposited = flatfile_accounting_coin_transaction::apply(root, drop_command);
	if (deposited.outcome != outcome::applied || deposited.error_code)
		std::cerr << "flatfile pile create outcome="
			  << static_cast<unsigned int>(deposited.outcome)
			  << " error=" << deposited.error_code << '\n';
	assert(deposited.outcome == outcome::applied && !deposited.error_code);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		flatfile_accounting_pile_state head;
		const economic_coin_vector pile_balance = { 1, 0, 0, 0 };
		assert(flatfile_accounting_pile_state_read(root, lock, 500, &head, nullptr) ==
			       flatfile_accounting_status::ok &&
		       head.balance == pile_balance && head.item_revision == 1 && !head.retired);
		std::vector<flatfile_item_ownership_record> owned;
		assert(flatfile_item_repository_load_coins_locked(root, lock, { 500 }, &owned,
								  nullptr) ==
			       flatfile_item_repository_result::ok &&
		       owned.size() == 1 && owned[0].item_revision == 1);
	}
	const auto pile_record = retained(root, drop_command);
	economic_accounting_plan pile_plan;
	assert(economic_plan_decode(pile_record.plan, &pile_plan) == economic_accounting_error::ok);
	assert(pile_plan.accounts.size() == 2 && pile_plan.postings.size() == 2 &&
	       pile_plan.item_events.size() == 1 &&
	       pile_plan.postings[0].copper + pile_plan.postings[1].copper == 0);
	economic_accounting_item_reference pile_reference = {};
	coin_transfer_payload decoded_pile_transfer;
	assert(coin_transfer_command_decode_payload(drop_command, &decoded_pile_transfer));
	const auto reference_status = flatfile_item_accounting_reference_find_by_legacy(
		root, decoded_pile_transfer.destination.change.operation_id, 0, &pile_reference);
	if (reference_status != flatfile_item_accounting_status::ok ||
	    pile_reference.operation_id.bytes != drop_command.operation_id.bytes ||
	    pile_reference.item_uid != 500 || pile_reference.before_revision != 0 ||
	    pile_reference.after_revision != 1)
		std::cerr << "flatfile pile reference status="
			  << static_cast<unsigned int>(reference_status)
			  << " uid=" << pile_reference.item_uid
			  << " before=" << pile_reference.before_revision
			  << " after=" << pile_reference.after_revision << '\n';
	assert(reference_status == flatfile_item_accounting_status::ok &&
	       pile_reference.operation_id.bytes == drop_command.operation_id.bytes &&
	       pile_reference.item_uid == 500 && pile_reference.before_revision == 0 &&
	       pile_reference.after_revision == 1);
	const auto pile_replay = flatfile_accounting_coin_transaction::apply(root, drop_command);
	assert(pile_replay.outcome == outcome::already_applied);
	same(deposited, pile_replay);
	assert(state(root).domains.wallet[0] == 98 &&
	       peer_state(root).domains.wallet == after_second.domains.wallet);
	coin_transfer_payload interrupted_transfer;
	interrupted_transfer.source = endpoint(1, state(root), { 97, 20, 3, 1 }, 81003);
	interrupted_transfer.destination = endpoint(2, peer_state(root), { 2, 0, 0, 0 }, 81004);
	critical_command interrupted;
	assert(coin_transfer_command_build(&interrupted, id(203), interrupted_transfer,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(interrupted, id(90005), wallet(),
					       { id(90001), economic_account_kind::wallet, 3, 0 },
					       &interrupted.accounting_intent) ==
	       economic_accounting_error::ok);
	interrupted.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	interrupted.accepted_at_usec = 12;
	interrupted.publication_required = true;
	assert(setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1) == 0);
	const auto unknown = flatfile_accounting_coin_transaction::apply(root, interrupted);
	assert(unknown.outcome == outcome::ambiguous_commit && unknown.error_code == EIO);
	assert(unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE") == 0);
	const auto recovered = flatfile_accounting_coin_transaction::apply(root, interrupted);
	assert(recovered.outcome == outcome::already_applied && !recovered.error_code);
	assert(state(root).domains.wallet[0] == 97 && peer_state(root).domains.wallet[0] == 2);
	assert(state(root).domains.bank_revision == after_first.domains.bank_revision + 3 &&
	       peer_state(root).domains.bank_revision == after_second.domains.bank_revision + 3);
	const auto recovered_record = retained(root, interrupted);
	economic_accounting_plan recovered_plan;
	assert(economic_plan_decode(recovered_record.plan, &recovered_plan) ==
	       economic_accounting_error::ok);
	assert(recovered_plan.postings.size() == 2 &&
	       recovered_plan.postings[0].copper + recovered_plan.postings[1].copper == 0);
	const auto recovered_replay =
		flatfile_accounting_coin_transaction::apply(root, interrupted);
	same(recovered, recovered_replay);
	coin_transfer_endpoint consumed_pile;
	consumed_pile.before[0] = 1;
	item_transfer_payload consume = {};
	consume.from_owner = pile.to_owner;
	consume.to_owner = { item_owner_type::destruction, 0, 0 };
	consume.reason = item_transfer_reason::destruction;
	consume.selected_item_uid = 500;
	consume.target_root_item_uid = 500;
	consume.item_count = 1;
	consume.items[0] = { 500, 500, 0, 1, 402013, item_custody_state::active };
	std::vector<flatfile_item_ownership_record> room_coins;
	assert(flatfile_item_repository_load_owner(root, consume.from_owner,
						   &consume.expected_from_revision, &room_coins,
						   nullptr) == flatfile_item_repository_result::ok);
	consume.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), consume.item_blob.begin());
	assert(item_transfer_command_build(&consumed_pile.change, id(81007), consume,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	coin_transfer_payload pickup_payload;
	pickup_payload.source = consumed_pile;
	pickup_payload.destination = endpoint(1, state(root), { 98, 20, 3, 1 }, 81008);
	critical_command pickup;
	assert(coin_transfer_command_build(&pickup, id(205), pickup_payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(
		       pickup, id(90005), { id(90001), economic_account_kind::pile, 500, 0 },
		       wallet(), &pickup.accounting_intent) == economic_accounting_error::ok);
	pickup.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	pickup.accepted_at_usec = 12;
	pickup.publication_required = true;
	const auto picked_up = flatfile_accounting_coin_transaction::apply(root, pickup);
	if (picked_up.outcome != outcome::applied || picked_up.error_code)
		std::cerr << "flatfile pile pickup outcome="
			  << static_cast<unsigned int>(picked_up.outcome)
			  << " error=" << picked_up.error_code << '\n';
	assert(picked_up.outcome == outcome::applied && !picked_up.error_code);
	assert(state(root).domains.wallet[0] == 98);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		flatfile_accounting_pile_state head;
		assert(flatfile_accounting_pile_state_read(root, lock, 500, &head, nullptr) ==
			       flatfile_accounting_status::ok &&
		       head.retired && head.item_revision == 2 &&
		       head.balance == economic_coin_vector{});
	}
	assert(flatfile_accounting_coin_transaction::apply(root, pickup).outcome ==
	       outcome::already_applied);
	const auto pile_600 =
		economic_account_key{ id(90001), economic_account_kind::pile, 600, 0 };
	const auto pile_601 =
		economic_account_key{ id(90001), economic_account_kind::pile, 601, 0 };
	auto accounted = [&](coin_transfer_endpoint source, coin_transfer_endpoint destination,
			     uint64_t operation, economic_account_key from, economic_account_key to)
	{
		critical_command root_command;
		assert(coin_transfer_command_build(
			&root_command, id(operation), { source, destination },
			critical_source_site::command, critical_deadline_class::interactive));
		assert(coin_transfer_accounting_intent(root_command, id(90005), from, to,
						       &root_command.accounting_intent) ==
		       economic_accounting_error::ok);
		root_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		root_command.accepted_at_usec = 12;
		root_command.publication_required = true;
		assert(critical_command_envelope_valid(root_command));
		return root_command;
	};
	const auto ten = std::array<int32_t, 4>{ 10, 0, 0, 0 };
	const auto six = std::array<int32_t, 4>{ 6, 0, 0, 0 };
	const auto four = std::array<int32_t, 4>{ 4, 0, 0, 0 };
	const auto empty = std::array<int32_t, 4>{};
	const economic_coin_vector ten_balance = { 10, 0, 0, 0 };
	const economic_coin_vector six_balance = { 6, 0, 0, 0 };
	const economic_coin_vector four_balance = { 4, 0, 0, 0 };
	const auto second_drop = accounted(endpoint(1, state(root), { 88, 20, 3, 1 }, 81009),
					   room_pile(root, 600, empty, ten, 81010), 206, wallet(),
					   pile_600);
	const auto dropped = flatfile_accounting_coin_transaction::apply(root, second_drop);
	if (dropped.outcome != outcome::applied || dropped.error_code)
		std::cerr << "flatfile second pile create outcome="
			  << static_cast<unsigned int>(dropped.outcome)
			  << " error=" << dropped.error_code << '\n';
	assert(dropped.outcome == outcome::applied && !dropped.error_code);
	const auto split = accounted(room_pile(root, 600, ten, six, 81011),
				     room_pile(root, 601, empty, four, 81012), 207, pile_600,
				     pile_601);
	const auto divided = flatfile_accounting_coin_transaction::apply(root, split);
	if (divided.outcome != outcome::applied || divided.error_code)
		std::cerr << "flatfile pile split outcome="
			  << static_cast<unsigned int>(divided.outcome)
			  << " error=" << divided.error_code << '\n';
	assert(divided.outcome == outcome::applied && !divided.error_code);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		flatfile_accounting_pile_state left, right;
		assert(flatfile_accounting_pile_state_read(root, lock, 600, &left, nullptr) ==
			       flatfile_accounting_status::ok &&
		       left.balance == six_balance && left.item_revision == 2);
		assert(flatfile_accounting_pile_state_read(root, lock, 601, &right, nullptr) ==
			       flatfile_accounting_status::ok &&
		       right.balance == four_balance && right.item_revision == 1);
	}
	const auto split_record = retained(root, split);
	economic_accounting_plan split_plan;
	assert(economic_plan_decode(split_record.plan, &split_plan) ==
	       economic_accounting_error::ok);
	assert(split_plan.accounts.size() == 2 && split_plan.item_events.size() == 2 &&
	       split_plan.postings[0].copper + split_plan.postings[1].copper == 0);
	std::vector<economic_accounting_item_reference> split_references;
	assert(flatfile_item_accounting_reference_find_by_operation(root, split.operation_id,
								    &split_references) ==
		       flatfile_item_accounting_status::ok &&
	       split_references.size() == 2);
	assert(flatfile_accounting_coin_transaction::apply(root, split).outcome ==
	       outcome::already_applied);
	const auto merge = accounted(room_pile(root, 601, four, empty, 81013),
				     room_pile(root, 600, six, ten, 81014), 208, pile_601,
				     pile_600);
	const auto joined = flatfile_accounting_coin_transaction::apply(root, merge);
	if (joined.outcome != outcome::applied || joined.error_code)
		std::cerr << "flatfile pile merge outcome="
			  << static_cast<unsigned int>(joined.outcome)
			  << " error=" << joined.error_code << '\n';
	assert(joined.outcome == outcome::applied && !joined.error_code);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		flatfile_accounting_pile_state left, right;
		assert(flatfile_accounting_pile_state_read(root, lock, 600, &left, nullptr) ==
			       flatfile_accounting_status::ok &&
		       left.balance == ten_balance && left.item_revision == 3);
		assert(flatfile_accounting_pile_state_read(root, lock, 601, &right, nullptr) ==
			       flatfile_accounting_status::ok &&
		       right.retired && right.balance == economic_coin_vector{} &&
		       right.item_revision == 2);
	}
	assert(flatfile_accounting_coin_transaction::apply(root, merge).outcome ==
	       outcome::already_applied);
	const auto final_pickup = accounted(room_pile(root, 600, ten, empty, 81015),
					    endpoint(1, state(root), { 98, 20, 3, 1 }, 81016), 209,
					    pile_600, wallet());
	const auto collected = flatfile_accounting_coin_transaction::apply(root, final_pickup);
	assert(collected.outcome == outcome::applied && !collected.error_code);
	assert(state(root).domains.wallet[0] == 98);
	assert(flatfile_accounting_coin_transaction::apply(root, final_pickup).outcome ==
	       outcome::already_applied);
	assert(flatfile_accounting_coin_transaction::apply(root, split).outcome ==
	       outcome::already_applied);
	assert(flatfile_accounting_coin_transaction::apply(root, merge).outcome ==
	       outcome::already_applied);
	coin_transfer_payload stale_piles;
	assert(coin_transfer_command_decode_payload(split, &stale_piles));
	const auto stale_split =
		accounted(stale_piles.source, stale_piles.destination, 210, pile_600, pile_601);
	const auto stale_pile_result =
		flatfile_accounting_coin_transaction::apply(root, stale_split);
	assert(stale_pile_result.outcome == outcome::terminal_failure &&
	       stale_pile_result.error_code == ESTALE);
	const auto stale_pile_record = retained(root, stale_split);
	assert(stale_pile_record.result_code == ESTALE && stale_pile_record.plan.empty());
	const auto stale_pile_replay =
		flatfile_accounting_coin_transaction::apply(root, stale_split);
	assert(stale_pile_replay.outcome == outcome::terminal_failure &&
	       stale_pile_replay.error_code == ESTALE);
	same(stale_pile_result, stale_pile_replay);
	const auto change_pile =
		economic_account_key{ id(90001), economic_account_kind::pile, 602, 0 };
	const auto before_change = state(root);
	const auto change_drop = accounted(endpoint(1, before_change, { 98, 29, 2, 1 }, 81017),
					   room_pile(root, 602, empty, ten, 81018), 211, wallet(),
					   change_pile);
	const auto changed_wallet = flatfile_accounting_coin_transaction::apply(root, change_drop);
	assert(changed_wallet.outcome == outcome::applied && !changed_wallet.error_code);
	const auto after_change = state(root);
	assert(after_change.domains.wallet == (std::array<uint64_t, 4>{ 98, 29, 2, 1 }) &&
	       after_change.domains.wallet_revision == before_change.domains.wallet_revision + 1);
	const auto change_record = retained(root, change_drop);
	economic_accounting_plan change_plan;
	assert(economic_plan_decode(change_record.plan, &change_plan) ==
	       economic_accounting_error::ok);
	assert(change_plan.accounts.size() == 2 && change_plan.postings.size() == 2 &&
	       change_plan.accounts[0].before == (economic_coin_vector{ 98, 20, 3, 1 }) &&
	       change_plan.accounts[0].after == (economic_coin_vector{ 98, 29, 2, 1 }) &&
	       change_plan.postings[0].delta == (economic_coin_vector{ 0, 9, -1, 0 }) &&
	       change_plan.postings[0].copper == -10 && change_plan.postings[1].copper == 10);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		flatfile_accounting_pile_state head;
		assert(flatfile_accounting_pile_state_read(root, lock, 602, &head, nullptr) ==
			       flatfile_accounting_status::ok &&
		       head.balance == ten_balance && head.item_revision == 1);
	}
	assert(flatfile_accounting_coin_transaction::apply(root, change_drop).outcome ==
	       outcome::already_applied);
	assert(state(root).domains.wallet == after_change.domains.wallet);
	const auto head_path = path / "economic-evidence" / "pile-head-0000000000000259.eph";
	{
		std::fstream head(head_path, std::ios::in | std::ios::out | std::ios::binary);
		assert(head);
		head.seekp(52);
		head.put('\x7f');
		assert(head);
	}
	const auto corrupt_replay = flatfile_accounting_coin_transaction::apply(root, merge);
	assert(corrupt_replay.outcome == outcome::retryable_failure &&
	       corrupt_replay.error_code == EILSEQ);
	preexisting_pile_journey(path / "preexisting");
	std::cout
		<< "flatfile peer coin root: native wallets, pile creation, split, merge, pickup and denomination change, shared bank revisions, balanced evidence, retained replay, stale rejection, and interrupted commit recovery passed\n";
}
