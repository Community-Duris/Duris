#include "phase8_bank_fixture.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_lifecycle_transaction.h"
#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_accounting_pile_baseline.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_item_repository.h"
#include "economy/coin_transfer_accounting.h"
#include "core/defines.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>

flatfile_player_domain_record peer_state(const std::string &root)
{
	flatfile_player_domain_record value;
	assert(flatfile_player_domain_load(root, 2, "account-one", 1, &value, nullptr) ==
	       flatfile_player_domain_result::ok);
	return value;
}

flatfile_player_domain_record third_state(const std::string &root)
{
	flatfile_player_domain_record value;
	assert(flatfile_player_domain_load(root, 3, "account-one", 1, &value, nullptr) ==
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

void establish_third_player(const std::string &root)
{
	int32_t pid = 0;
	assert(flatfile_identity_allocate_pid(root, &pid, nullptr) ==
		       flatfile_identity_result::ok &&
	       pid == 3);
	assert(flatfile_identity_claim(root, pid, "Third", "ACCOUNT-ONE", nullptr) ==
	       flatfile_identity_result::ok);
	flatfile_identity_record first, second, third;
	assert(flatfile_identity_lookup_pid(root, 1, &first, nullptr) ==
	       flatfile_identity_result::ok);
	assert(flatfile_identity_lookup_pid(root, 2, &second, nullptr) ==
	       flatfile_identity_result::ok);
	assert(flatfile_identity_lookup_pid(root, 3, &third, nullptr) ==
	       flatfile_identity_result::ok);
	third.racewar = 1;
	assert(flatfile_identity_sync_account(root, "account-one", { first, second, third },
					      nullptr) == flatfile_identity_result::ok);
	flatfile_player_domain_record player;
	player.pid = 3;
	player.account_name = "account-one";
	player.racewar = 1;
	player.domains.bank = { 100, 20, 3, 1 };
	assert(flatfile_player_domain_establish(root, player, nullptr) ==
	       flatfile_player_domain_result::ok);
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	initialize_bucket(root, lock, bucket(1, 0, 3, {}));
	flatfile_economic_mapping mapping;
	ops changes;
	assert(access_type::create(root, lock, control(root, lock).revision,
				   economic_account_kind::wallet, 0, { 1, 3, {} }, id(90010),
				   &mapping, &changes, nullptr) == 0);
	assert(mapping.account.authority_id == 4);
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

critical_command split_child_command(const flatfile_player_domain_record &source,
				     const flatfile_player_domain_record &recipient,
				     uint32_t recipient_pid, uint64_t recipient_account_id,
				     uint64_t operation)
{
	std::array<int32_t, 4> source_after = {}, recipient_after = {};
	for (size_t index = 0; index < source_after.size(); ++index)
	{
		source_after[index] = static_cast<int32_t>(source.domains.wallet[index]);
		recipient_after[index] = static_cast<int32_t>(recipient.domains.wallet[index]);
	}
	source_after[0] -= 3;
	recipient_after[0] += 3;
	coin_transfer_payload transfer;
	transfer.source = endpoint(1, source, source_after, 82000 + operation * 2);
	transfer.destination =
		endpoint(recipient_pid, recipient, recipient_after, 82001 + operation * 2);
	critical_command command;
	assert(coin_transfer_command_build(&command, id(operation), transfer,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(coin_transfer_accounting_intent(
		       command, id(90005), wallet(),
		       { id(90001), economic_account_kind::wallet, recipient_account_id, 0 },
		       &command.accounting_intent) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accepted_at_usec = 12;
	command.publication_required = true;
	assert(critical_command_envelope_valid(command));
	return command;
}

void split_children_journey(const fs::path &path)
{
	setup(path);
	const auto root = path.string();
	establish_peer(root);
	establish_third_player(root);
	const auto sender_before = state(root);
	const auto first_before = peer_state(root);
	const auto second_before = third_state(root);
	assert(sender_before.domains.wallet[0] == 100 && first_before.domains.wallet[0] == 0 &&
	       second_before.domains.wallet[0] == 0);
	assert(sender_before.domains.bank_revision == first_before.domains.bank_revision &&
	       first_before.domains.bank_revision == second_before.domains.bank_revision);
	const auto first = split_child_command(sender_before, first_before, 2, 3, 220);
	const auto first_applied = flatfile_accounting_coin_transaction::apply(root, first);
	assert(first_applied.outcome == outcome::applied && !first_applied.error_code);
	const auto after_first = state(root);
	const auto first_recipient_after = peer_state(root);
	assert(after_first.domains.wallet[0] == 97 && peer_state(root).domains.wallet[0] == 3 &&
	       third_state(root).domains.wallet[0] == 0);
	assert(after_first.domains.wallet_revision == sender_before.domains.wallet_revision + 1 &&
	       peer_state(root).domains.wallet_revision ==
		       first_before.domains.wallet_revision + 1 &&
	       after_first.domains.bank_revision == sender_before.domains.bank_revision + 2 &&
	       peer_state(root).domains.bank_revision == after_first.domains.bank_revision &&
	       third_state(root).domains.bank_revision == after_first.domains.bank_revision);
	const auto first_record = retained(root, first);
	economic_accounting_plan first_plan;
	assert(first_record.result_code == 0 &&
	       economic_plan_decode(first_record.plan, &first_plan) ==
		       economic_accounting_error::ok);
	assert(first_plan.accounts.size() == 2 && first_plan.postings.size() == 2 &&
	       !first_plan.metadata.source_event && first_plan.children.empty());
	assert(first_plan.accounts[0].before[0] == 100 && first_plan.accounts[0].after[0] == 97 &&
	       first_plan.accounts[0].key.authority_id == 1 &&
	       first_plan.accounts[0].before_revision == sender_before.domains.wallet_revision &&
	       first_plan.accounts[0].after_revision == after_first.domains.wallet_revision &&
	       first_plan.accounts[1].before[0] == 0 && first_plan.accounts[1].after[0] == 3 &&
	       first_plan.accounts[1].key.authority_id == 3 &&
	       first_plan.accounts[1].before_revision == first_before.domains.wallet_revision &&
	       first_plan.accounts[1].after_revision ==
		       first_recipient_after.domains.wallet_revision);
	assert(first_plan.postings[0].delta[0] == -3 && first_plan.postings[1].delta[0] == 3 &&
	       first_plan.postings[0].copper == -3 && first_plan.postings[1].copper == 3);
	// A later child with the old source revision is retained as a refusal.
	const auto stale = split_child_command(sender_before, second_before, 3, 4, 221);
	const auto rejected = flatfile_accounting_coin_transaction::apply(root, stale);
	assert(rejected.outcome == outcome::terminal_failure && rejected.error_code == ESTALE);
	coin_transfer_payload stale_payload;
	assert(coin_transfer_command_decode_payload(stale, &stale_payload));
	coin_transfer_stale_result repair;
	assert(rejected.result_size == COIN_TRANSFER_STALE_RESULT_BYTES &&
	       coin_transfer_command_decode_stale_result(stale_payload, rejected.failure_stage,
							 rejected.result_payload.data(),
							 rejected.result_size, &repair));
	assert(repair.endpoint_index == 0 && repair.wallet_stale && repair.bank_stale &&
	       repair.current.wallet.amount[0] == 97 &&
	       repair.current.wallet_revision == after_first.domains.wallet_revision &&
	       repair.current.bank_revision == after_first.domains.bank_revision);
	assert(retained(root, stale).plan.empty());
	same(rejected, flatfile_accounting_coin_transaction::apply(root, stale));
	assert(state(root).domains.wallet[0] == 97 && peer_state(root).domains.wallet[0] == 3 &&
	       third_state(root).domains.wallet[0] == 0);
	const auto second = split_child_command(state(root), third_state(root), 3, 4, 222);
	const auto second_applied = flatfile_accounting_coin_transaction::apply(root, second);
	assert(second_applied.outcome == outcome::applied && !second_applied.error_code);
	const auto sender_after = state(root);
	const auto first_recipient_final = peer_state(root);
	const auto second_after = third_state(root);
	assert(sender_after.domains.wallet[0] == 94 &&
	       first_recipient_final.domains.wallet[0] == 3 && second_after.domains.wallet[0] == 3);
	assert(sender_after.domains.wallet_revision == sender_before.domains.wallet_revision + 2 &&
	       first_recipient_final.domains.wallet_revision ==
		       first_recipient_after.domains.wallet_revision &&
	       second_after.domains.wallet_revision == second_before.domains.wallet_revision + 1 &&
	       sender_after.domains.bank_revision == sender_before.domains.bank_revision + 4 &&
	       first_recipient_final.domains.bank_revision == sender_after.domains.bank_revision &&
	       second_after.domains.bank_revision == sender_after.domains.bank_revision &&
	       sender_after.domains.bank == sender_before.domains.bank &&
	       first_recipient_final.domains.bank == first_before.domains.bank &&
	       second_after.domains.bank == second_before.domains.bank);
	assert(sender_after.domains.wallet[0] == sender_before.domains.wallet[0] - 11 + 5 &&
	       sender_after.domains.wallet[0] + peer_state(root).domains.wallet[0] +
			       second_after.domains.wallet[0] ==
		       sender_before.domains.wallet[0]);
	const auto second_record = retained(root, second);
	economic_accounting_plan second_plan;
	assert(second_record.result_code == 0 &&
	       economic_plan_decode(second_record.plan, &second_plan) ==
		       economic_accounting_error::ok);
	assert(second_plan.accounts.size() == 2 && second_plan.postings.size() == 2 &&
	       !second_plan.metadata.source_event && second_plan.children.empty());
	assert(second_plan.accounts[0].before[0] == 97 && second_plan.accounts[0].after[0] == 94 &&
	       second_plan.accounts[0].key.authority_id == 1 &&
	       second_plan.accounts[0].before_revision == after_first.domains.wallet_revision &&
	       second_plan.accounts[0].after_revision == sender_after.domains.wallet_revision &&
	       second_plan.accounts[1].before[0] == 0 && second_plan.accounts[1].after[0] == 3 &&
	       second_plan.accounts[1].key.authority_id == 4 &&
	       second_plan.accounts[1].before_revision == second_before.domains.wallet_revision &&
	       second_plan.accounts[1].after_revision == second_after.domains.wallet_revision);
	assert(second_plan.postings[0].delta[0] == -3 && second_plan.postings[1].delta[0] == 3 &&
	       second_plan.postings[0].copper == -3 && second_plan.postings[1].copper == 3);
	same(first_applied, flatfile_accounting_coin_transaction::apply(root, first));
	same(second_applied, flatfile_accounting_coin_transaction::apply(root, second));
	assert(flatfile_player_domain_restore_recover(root, nullptr) ==
	       flatfile_player_domain_result::ok);
	same(first_applied, flatfile_accounting_coin_transaction::apply(root, first));
	same(second_applied, flatfile_accounting_coin_transaction::apply(root, second));
	assert(state(root).domains.wallet[0] == 94 && peer_state(root).domains.wallet[0] == 3 &&
	       third_state(root).domains.wallet[0] == 3);
	std::cout
		<< "flatfile split children: sequential roots, remainder, stale later child, replay and restart passed\n";
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
		const auto found = std::find_if(owned.begin(), owned.end(),
						[uid](const auto &item)
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
	write_pending_snapshot(path, 0, 1, { coin });
	flatfile_item_ownership_record native;
	native.item_uid = native.root_item_uid = 700;
	native.owner = owner;
	native.item_revision = 1;
	native.vnum = coin.vnum;
	native.state = item_custody_state::active;
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
	// The isolated fixture can now spend the opening pile, resolving its exact
	// money values from the native player snapshot under the authority lock.
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

void legacy_pile_inventory(const fs::path &path)
{
	setup(path, false);
	const item_owner_identity owner{ item_owner_type::player, 1, 0 };
	player_item_snapshot coin = {};
	coin.object_uid = 702;
	coin.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	coin.equipment_slot = -1;
	coin.vnum = 402014;
	coin.type = ITEM_MONEY;
	coin.values[0] = 2;
	coin.name = "legacy coins";
	coin.string_mask = 1;
	auto ordinary = coin;
	ordinary.object_uid = 703;
	ordinary.vnum = 402015;
	ordinary.type = ITEM_OTHER;
	ordinary.values[0] = 0;
	ordinary.name = "ordinary item";
	write_pending_snapshot(path, 0, 1, { coin, ordinary });
	flatfile_item_ownership_record native;
	native.item_uid = native.root_item_uid = coin.object_uid;
	native.owner = owner;
	native.item_revision = 1;
	native.vnum = coin.vnum;
	native.state = item_custody_state::active;
	auto ordinary_native = native;
	ordinary_native.item_uid = ordinary_native.root_item_uid = ordinary.object_uid;
	ordinary_native.vnum = ordinary.vnum;
	assert(flatfile_item_repository_establish_owner(path.string(), owner,
							{ native, ordinary_native }, nullptr) ==
	       flatfile_item_baseline_result::applied);
	std::vector<flatfile_coin_pile_source> piles;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(path.string(), nullptr));
		assert(flatfile_item_repository_list_coin_piles_locked(path.string(), lock, &piles,
								       nullptr) ==
		       flatfile_item_repository_result::ok);
		assert(piles.size() == 1 && piles[0].ownership.item_uid == coin.object_uid &&
		       piles[0].item.values[0] == 2);
		flatfile_coin_pile_source ignored;
		assert(flatfile_item_repository_read_coin_pile_locked(
			       path.string(), lock, ordinary.object_uid, &ignored, nullptr) ==
		       flatfile_item_repository_result::not_found);
		flatfile_accounting_pile_baseline_source source;
		assert(flatfile_accounting_pile_baseline_capture(
			       path.string(), lock, id(90001), id(90005), id(91001),
			       coin.object_uid, &source, nullptr) == 0);
		assert(source.holding.balance == (economic_coin_vector{ 2, 0, 0, 0 }));
	}
	const auto missing_path = path.parent_path() / "missing-legacy-native";
	setup(missing_path, false);
	native.item_uid = native.root_item_uid = 704;
	assert(flatfile_item_repository_establish_owner(missing_path.string(), owner, { native },
							nullptr) ==
	       flatfile_item_baseline_result::applied);
	{
		flatfile_authority_lock missing_lock;
		assert(missing_lock.acquire(missing_path.string(), nullptr));
		piles.clear();
		assert(flatfile_item_repository_list_coin_piles_locked(
			       missing_path.string(), missing_lock, &piles, nullptr) ==
			       flatfile_item_repository_result::invalid &&
		       piles.empty());
	}
	const auto pet_path = path.parent_path() / "legacy-pet-native";
	setup(pet_path, false);
	player_item_snapshot pet_coin = coin;
	pet_coin.object_uid = 705;
	pet_coin.values[0] = 3;
	player_pet_snapshot pet = {};
	pet.pet_uid = 1705;
	pet.items = { pet_coin };
	write_pending_snapshot(pet_path, 0, 1, {}, { pet });
	const item_owner_identity pet_owner{ item_owner_type::pet, pet.pet_uid, 1 };
	flatfile_item_ownership_record pet_native;
	pet_native.item_uid = pet_native.root_item_uid = pet_coin.object_uid;
	pet_native.owner = pet_owner;
	pet_native.item_revision = 1;
	pet_native.vnum = pet_coin.vnum;
	pet_native.state = item_custody_state::active;
	assert(flatfile_item_repository_establish_owner(pet_path.string(), pet_owner,
							{ pet_native }, nullptr) ==
	       flatfile_item_baseline_result::applied);
	{
		flatfile_authority_lock pet_lock;
		assert(pet_lock.acquire(pet_path.string(), nullptr));
		piles.clear();
		assert(flatfile_item_repository_list_coin_piles_locked(pet_path.string(), pet_lock,
								       &piles, nullptr) ==
		       flatfile_item_repository_result::ok);
		assert(piles.size() == 1 &&
		       item_owner_identity_equal(piles[0].ownership.owner, pet_owner) &&
		       piles[0].ownership.item_uid == pet_coin.object_uid &&
		       piles[0].item.values[0] == 3);
	}
	const auto wrong_owner_path = path.parent_path() / "legacy-pet-wrong-owner";
	setup(wrong_owner_path, false);
	const item_owner_identity player_owner{ item_owner_type::player, 1, 0 };
	pet_native.owner = player_owner;
	assert(flatfile_item_repository_establish_owner(wrong_owner_path.string(), player_owner,
							{ pet_native }, nullptr) ==
	       flatfile_item_baseline_result::applied);
	write_pending_snapshot(wrong_owner_path, 0, 1, {}, { pet });
	flatfile_authority_lock wrong_owner_lock;
	assert(wrong_owner_lock.acquire(wrong_owner_path.string(), nullptr));
	flatfile_coin_pile_source wrong_owner_source;
	assert(flatfile_item_repository_read_coin_pile_locked(
		       wrong_owner_path.string(), wrong_owner_lock, pet_coin.object_uid,
		       &wrong_owner_source, nullptr) == flatfile_item_repository_result::not_found);
}

void lifecycle_native_capture(const fs::path &path)
{
	const auto root = path.string();
	setup(path, false);
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
	peer.domains.wallet = { 11, 12, 13, 14 };
	peer.domains.bank = { 100, 20, 3, 1 };
	assert(flatfile_player_domain_establish(root, peer, nullptr) ==
	       flatfile_player_domain_result::ok);

	flatfile_accounting_lifecycle_native_sources captured;
	{
		flatfile_identity_lock identity_lock;
		assert(identity_lock.acquire(root, nullptr));
		flatfile_authority_lock authority_lock;
		assert(authority_lock.acquire(root, nullptr));
		assert(flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
			       root, identity_lock, authority_lock, nullptr, nullptr) == EINVAL);
		std::string error;
		const auto result =
			flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
				root, identity_lock, authority_lock, &captured, &error);
		if (result)
			std::cerr << "native lifecycle capture error " << result << ": " << error
				  << '\n';
		assert(result == 0);
	}
	const auto native_first = state(root), native_second = peer_state(root);
	assert((captured.wallets.size() == 2 && captured.wallets[0].pid == 1 &&
		captured.wallets[0].account_name == "account-one" &&
		captured.wallets[0].racewar == 1 &&
		captured.wallets[0].balance == economic_coin_vector{ 100, 20, 3, 1 } &&
		captured.wallets[0].native_revision == native_first.domains.wallet_revision &&
		captured.wallets[0].source_digest != economic_digest{}));
	assert((captured.wallets[1].pid == 2 && captured.wallets[1].account_name == "account-one" &&
		captured.wallets[1].racewar == 1 &&
		captured.wallets[1].balance == economic_coin_vector{ 11, 12, 13, 14 } &&
		captured.wallets[1].native_revision == native_second.domains.wallet_revision &&
		captured.wallets[1].source_digest != economic_digest{}));
	assert((captured.banks.size() == 1 && captured.banks[0].name == "account-one" &&
		captured.banks[0].racewar == 1 &&
		captured.banks[0].balance == economic_coin_vector{ 100, 20, 3, 1 } &&
		captured.banks[0].native_revision == native_first.domains.bank_revision &&
		captured.banks[0].source_digest != economic_digest{}));

	economic_baseline_batch baseline;
	baseline.lineage = id(90001);
	baseline.epoch = id(90005);
	baseline.preparation_id = id(90999);
	baseline.actor_id = 1;
	baseline.opening_account = { id(90001), economic_account_kind::opening, 90002, 0 };
	baseline.boundary_digest[0] = 1;
	baseline.coverage_digest[0] = 2;
	baseline.holdings.push_back({ wallet(), captured.wallets[0].balance,
				      captured.wallets[0].native_revision,
				      captured.wallets[0].source_digest });
	baseline.holdings.push_back({ bank(), captured.banks[0].balance,
				      captured.banks[0].native_revision,
				      captured.banks[0].source_digest });
	std::optional<economic_prepared_baseline> prepared;
	assert(economic_baseline_prepare(baseline, &prepared) == economic_accounting_error::ok);
	assert(prepared && prepared->witness().holdings.size() == 2);
	const auto wallet_witness = std::find_if(
		prepared->witness().holdings.begin(), prepared->witness().holdings.end(),
		[](const auto &holding)
		{ return holding.account.kind == economic_account_kind::wallet; });
	const auto bank_witness = std::find_if(
		prepared->witness().holdings.begin(), prepared->witness().holdings.end(),
		[](const auto &holding)
		{ return holding.account.kind == economic_account_kind::bank; });
	assert(wallet_witness != prepared->witness().holdings.end() &&
	       wallet_witness->native_revision == captured.wallets[0].native_revision &&
	       wallet_witness->source_digest == captured.wallets[0].source_digest);
	assert(bank_witness != prepared->witness().holdings.end() &&
	       bank_witness->native_revision == captured.banks[0].native_revision &&
	       bank_witness->source_digest == captured.banks[0].source_digest);
	critical_command baseline_command;
	assert(economic_baseline_command_build(*prepared, 99, &baseline_command) ==
	       economic_accounting_error::ok);

	const auto missing_wallet = path / "missing-wallet";
	setup(missing_wallet, false, false);
	assert(fs::remove(missing_wallet / "domains" / "player-1.domain"));
	flatfile_accounting_lifecycle_native_sources unchanged;
	flatfile_accounting_lifecycle_wallet_source sentinel;
	sentinel.pid = 99;
	unchanged.wallets.push_back(sentinel);
	{
		const auto missing_root = missing_wallet.string();
		flatfile_identity_lock identity_lock;
		assert(identity_lock.acquire(missing_root, nullptr));
		flatfile_authority_lock authority_lock;
		assert(authority_lock.acquire(missing_root, nullptr));
		assert(flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
			       missing_root, identity_lock, authority_lock, &unchanged, nullptr) ==
		       EILSEQ);
	}
	assert(unchanged.wallets.size() == 1 && unchanged.wallets[0].pid == 99 &&
	       unchanged.banks.empty());

	const auto missing_bank = path / "missing-bank";
	setup(missing_bank, false, false);
	assert(fs::remove(missing_bank / "domains" / "bank-account-one-1.domain"));
	{
		const auto missing_root = missing_bank.string();
		flatfile_identity_lock identity_lock;
		assert(identity_lock.acquire(missing_root, nullptr));
		flatfile_authority_lock authority_lock;
		assert(authority_lock.acquire(missing_root, nullptr));
		assert(flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
			       missing_root, identity_lock, authority_lock, &unchanged, nullptr) ==
		       EILSEQ);
	}
	assert(unchanged.wallets.size() == 1 && unchanged.wallets[0].pid == 99 &&
	       unchanged.banks.empty());
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
	assert(stale_pile_result.result_size == 0 &&
	       stale_pile_result.failure_stage == critical_failure_stage::coin_revision_unknown);
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
	legacy_pile_inventory(path / "legacy-inventory");
	lifecycle_native_capture(path / "lifecycle-native-capture");
	split_children_journey(path / "split-children");
	std::cout
		<< "flatfile peer coin root: native wallets, pile creation, split, merge, pickup and denomination change, shared bank revisions, balanced evidence, retained replay, stale rejection, and interrupted commit recovery passed\n";
}
