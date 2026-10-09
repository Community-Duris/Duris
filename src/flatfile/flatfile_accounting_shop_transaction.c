#include "flatfile/flatfile_accounting_shop_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_shop_trade_repository.h"
#include "economy/shop_trade_accounting.h"
#include "economy/shop_trade_item_payload.h"
#include "core/defines.h"
#include <set>
#include <span>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <map>
#include <new>
#include <utility>

namespace
{
struct failure
{
	unsigned int code;
};
void need(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code };
}
void checked(unsigned int value)
{
	if (value)
		throw failure{ value };
}
void checked(economic_accounting_error value)
{
	need(value == economic_accounting_error::ok,
	     value == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
void checked(flatfile_accounting_status value)
{
	if (value == flatfile_accounting_status::ok)
		return;
	throw failure{ value == flatfile_accounting_status::conflict ||
				       value == flatfile_accounting_status::already_exists ?
			       unsigned(EEXIST) :
		       value == flatfile_accounting_status::not_found ? unsigned(EAGAIN) :
		       value == flatfile_accounting_status::io_error  ? unsigned(EIO) :
		       value == flatfile_accounting_status::capacity  ? unsigned(ENOSPC) :
									unsigned(EILSEQ) };
}
void checked(flatfile_player_domain_result value)
{
	need(value == flatfile_player_domain_result::ok,
	     value == flatfile_player_domain_result::io_error  ? EIO :
	     value == flatfile_player_domain_result::not_found ? ENOENT :
								 EILSEQ);
}
void checked(flatfile_item_repository_result value)
{
	need(value == flatfile_item_repository_result::ok,
	     value == flatfile_item_repository_result::io_error	 ? EIO :
	     value == flatfile_item_repository_result::not_found ? ENOENT :
								   EILSEQ);
}
void checked(flatfile_shopkeeper_result value)
{
	need(value == flatfile_shopkeeper_result::ok,
	     value == flatfile_shopkeeper_result::io_error  ? EIO :
	     value == flatfile_shopkeeper_result::not_found ? ENOENT :
							      EILSEQ);
}
void checked(flatfile_item_accounting_status value)
{
	need(value == flatfile_item_accounting_status::ok,
	     value == flatfile_item_accounting_status::io_error ? EIO :
	     value == flatfile_item_accounting_status::capacity ? ENOSPC :
								  EILSEQ);
}
void checked(flatfile_shop_trade_materialization_result value)
{
	need(value == flatfile_shop_trade_materialization_result::ok ||
		     value == flatfile_shop_trade_materialization_result::unchanged,
	     value == flatfile_shop_trade_materialization_result::io_error ? EIO : EILSEQ);
}
std::string canonical(std::string value)
{
	need(!value.empty() && value.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES, EINVAL);
	for (auto &c : value)
	{
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-',
		     EINVAL);
	}
	return value;
}
struct identity
{
	economic_frozen_intent intent;
	shop_trade_payload payload{};
	economic_account_key wallet{}, bank{}, counterparty{};
};
identity decode(const critical_command &command)
{
	need(command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		     command.type == critical_command_type::shop_trade &&
		     critical_command_envelope_valid(command),
	     EINVAL);
	identity value;
	checked(shop_trade_accounting_decode(command, &value.intent, &value.payload, &value.wallet,
					     &value.bank, &value.counterparty));
	need(value.intent.admission.facts.size() == 16 &&
		     shop_trade_payload_version_is_accounted(command.payload_version) &&
		     value.payload.player_pid <= INT32_MAX && value.payload.racewar <= INT8_MAX,
	     EINVAL);
	// The full original v6/v7/v8 codec/intent is validated before any projection.
	std::vector<uint8_t> encoded;
	const bool valid =
		command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ?
			shop_trade_command_encode_recovery_payload(value.payload, &encoded) :
		command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ?
			shop_trade_command_encode_native_payload(value.payload, &encoded) :
			shop_trade_command_encode_accounted_payload(value.payload, &encoded);
	need(valid && encoded == command.payload, EINVAL);
	return value;
}
shop_trade_payload legacy_body(const shop_trade_payload &full)
{
	auto body = full;
	body.expected_player_save_revision = 0;
	body.expected_player_level = 0;
	body.native_destination_weight_recorded = false;
	body.destination_weight = {};
	body.recovery_manifest_recorded = false;
	body.recovery_manifest = {};
	std::vector<uint8_t> encoded;
	need(shop_trade_command_encode_payload(body, &encoded));
	return body;
}
currency_command_result balances(const flatfile_player_domain_record &record)
{
	currency_command_result result{};
	for (size_t n = 0; n < 4; ++n)
	{
		need(record.domains.wallet[n] <= INT_MAX && record.domains.bank[n] <= INT_MAX);
		result.wallet.amount[n] = record.domains.wallet[n];
		result.bank.amount[n] = record.domains.bank[n];
	}
	result.wallet_revision = record.domains.wallet_revision;
	result.bank_revision = record.domains.bank_revision;
	return result;
}
uint64_t status(const player_snapshot &snapshot, player_status_field field)
{
	std::optional<uint64_t> result;
	for (const auto &value : snapshot.status_integers)
		if (value.field == field)
		{
			need(!result && (value.is_unsigned || value.signed_value >= 0));
			result = value.is_unsigned ? value.unsigned_value :
						     uint64_t(value.signed_value);
		}
	need(result.has_value());
	return *result;
}
item_owner_identity player_owner(const shop_trade_payload &p)
{
	return { item_owner_type::player, p.player_pid, 0 };
}
item_owner_identity keeper_owner(const shop_trade_payload &p)
{
	return { item_owner_type::shopkeeper, item_shopkeeper_owner_id(p.shop_id), 0 };
}
item_owner_identity primary(const shop_trade_payload &p)
{
	return p.action == shop_trade_action::discard_invalid ? keeper_owner(p) : player_owner(p);
}
item_owner_identity other(const shop_trade_payload &p)
{
	return p.action == shop_trade_action::buy_produced ?
		       item_owner_identity{ item_owner_type::system, 0, 0 } :
	       p.action == shop_trade_action::sell_destroy ||
			       p.action == shop_trade_action::discard_invalid ?
		       item_owner_identity{ item_owner_type::destruction, 0, 0 } :
		       keeper_owner(p);
}
economic_item_snapshot snapshot(const flatfile_item_ownership_record &record)
{
	return { record.item_uid,
		 { record.owner, record.root_item_uid, record.parent_item_uid, record.item_revision,
		   record.state, record.equipment_slot } };
}
void retained_identity(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_command &command, const identity &id)
{
	flatfile_economic_control control;
	checked(flatfile_economic_control_read(root, lock, &control, nullptr));
	const size_t bucket = command.operation_id.bytes[0];
	need(control.lineage.bytes == id.wallet.lineage.bytes &&
	     (control.evidence_initialized[bucket / 8] & (1U << (bucket % 8))));
	flatfile_economic_epoch epoch;
	checked(flatfile_economic_epoch_read(root, lock, id.wallet.lineage,
					     id.intent.admission.metadata.epoch, &epoch, nullptr));
	flatfile_economic_mapping wallet, bank;
	checked(flatfile_economic_mapping_read(root, lock, id.wallet, &wallet, nullptr));
	checked(flatfile_economic_mapping_read(root, lock, id.bank, &bank, nullptr));
	need(wallet.locator.kind == 1 && wallet.locator.native_id == id.payload.player_pid &&
	     bank.locator.kind == 2 && bank.locator.native_id == id.bank.authority_id);
}
void current_authority(const std::string &root, const flatfile_authority_lock &lock,
		       const identity &id)
{
	const flatfile_economic_mapping_request requests[] = {
		{ id.wallet, { 1, id.payload.player_pid, {} } },
		{ id.bank, { 2, id.bank.authority_id, canonical(id.payload.account_name.data()) } }
	};
	flatfile_economic_authority_snapshot snapshot;
	checked(economic_flatfile_lock_authority(root, lock, id.wallet.lineage,
						 id.intent.admission.metadata.epoch, requests,
						 &snapshot, nullptr));
}
uint64_t durable(const shop_trade_result &result)
{
	return std::max(
		{ result.wallet_revision, result.bank_revision, result.shop_revision,
		  result.player_owner_revision, result.counterparty_owner_revision,
		  *std::max_element(result.item_revisions.begin(), result.item_revisions.end()) });
}
critical_apply_result completion(const flatfile_accounting_record &record, bool replay)
{
	need(record.result.size() <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
	critical_apply_result result{ record.result_code ?
					      critical_apply_outcome::terminal_failure :
				      replay ? critical_apply_outcome::already_applied :
					       critical_apply_outcome::applied,
				      record.durable_revision, record.result_code };
	result.failure_stage = record.failure_stage;
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
std::vector<economic_accounting_item_reference> references(const critical_command &command,
							   const economic_accounting_plan &plan)
{
	std::vector<economic_accounting_item_reference> result;
	result.reserve(plan.item_events.size());
	for (const auto &event : plan.item_events)
	{
		economic_accounting_item_reference ref{};
		need(event.event_index < UINT16_MAX, E2BIG);
		ref.operation_id = command.operation_id;
		ref.line_index = event.event_index;
		ref.event_index = event.event_index;
		ref.child_index = event.child_index;
		ref.item_uid = event.uid;
		ref.before_revision = event.before.revision;
		ref.after_revision = event.after.revision;
		ref.legacy_operation_id = command.operation_id;
		ref.legacy_event_index = ref.line_index;
		need(economic_accounting_item_reference_validate(ref));
		result.push_back(ref);
	}
	return result;
}
shop_trade_accounting_authority historical_authority(const identity &id,
						     const shop_trade_result &result,
						     const economic_accounting_plan &plan)
{
	shop_trade_accounting_authority authority{};
	authority.epoch = id.intent.admission.metadata.epoch;
	authority.wallet_account = id.wallet;
	authority.bank_account = id.bank;
	authority.keeper_account = id.counterparty;
	authority.shop_id = id.payload.shop_id;
	authority.keeper_vnum = id.payload.keeper_vnum;
	authority.keeper_roaming = id.payload.keeper_roaming;
	authority.keeper_cash_before = id.payload.expected_keeper_cash;
	authority.shop_revision_before = id.payload.expected_shop_revision;
	need(result.player_owner_revision && result.counterparty_owner_revision);
	authority.player_owner_revision_before = result.player_owner_revision - 1;
	authority.counterparty_owner_revision_before = result.counterparty_owner_revision - 1;
	authority.balances_before = { result.wallet, result.bank, result.wallet_revision,
				      result.bank_revision };
	bool wallet = false, bank = false;
	for (const auto &effect : plan.accounts)
	{
		if (economic_account_key_equal(effect.key, id.wallet))
		{
			need(!wallet);
			wallet = true;
			authority.balances_before.wallet.amount = effect.before;
			authority.balances_before.wallet_revision = effect.before_revision;
		}
		if (economic_account_key_equal(effect.key, id.bank))
		{
			need(!bank);
			bank = true;
			authority.balances_before.bank.amount = effect.before;
			authority.balances_before.bank_revision = effect.before_revision;
		}
	}
	need(id.payload.action == shop_trade_action::discard_invalid ? plan.accounts.empty() :
								       wallet && bank);
	authority.items_before = plan.items_before;
	authority.item_vnums_before.resize(plan.items_before.size());
	for (size_t n = 0; n < plan.items_before.size(); ++n)
	{
		const auto uid = plan.items_before[n].uid;
		for (size_t p = 0; p < id.payload.item_count; ++p)
			if (id.payload.items[p].item_uid == uid &&
			    id.payload.action != shop_trade_action::buy_produced)
				authority.item_vnums_before[n] = id.payload.items[p].vnum;
		if (id.payload.action == shop_trade_action::buy_produced &&
		    uid == id.payload.stock_item_uid)
			authority.item_vnums_before[n] = id.payload.stock_vnum;
	}
	return authority;
}
// Native pet equipment and pet custody retain their original separate positions.
// The role comes from the actual native player file, never an item tag or UID guess.
enum class native_forest_role : uint8_t
{
	ordinary,
	pet
};
struct native_forest_view
{
	const std::vector<player_item_snapshot> *items;
	native_forest_role role;
};
void forests(std::span<const native_forest_view> native,
	     const std::vector<flatfile_item_ownership_record> &custody,
	     const item_owner_identity &owner)
{
	std::map<uint64_t, const flatfile_item_ownership_record *> indexed;
	for (const auto &row : custody)
		need(row.state == item_custody_state::active &&
		     item_owner_identity_equal(row.owner, owner) &&
		     indexed.emplace(row.item_uid, &row).second);
	size_t count = 0;
	for (const auto &view : native)
	{
		const auto &items = *view.items;
		need(items.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS - count, E2BIG);
		count += items.size();
		std::vector<uint8_t> encoded;
		checked(player_item_snapshot_list_encode(items, &encoded) ==
					player_snapshot_codec_result::ok ?
				0U :
				unsigned(EILSEQ));
		std::vector<uint64_t> roots(items.size());
		std::set<int16_t> native_pet_slots;
		for (size_t n = 0; n < items.size(); ++n)
		{
			const auto &item = items[n];
			const auto found = indexed.find(item.object_uid);
			need(found != indexed.end(), ESTALE);
			uint64_t parent = 0;
			if (item.parent_index >= 0)
			{
				need(size_t(item.parent_index) < n);
				parent = items[item.parent_index].object_uid;
				roots[n] = roots[item.parent_index];
			}
			else
			{
				need(item.parent_index == PLAYER_SNAPSHOT_NO_PARENT);
				roots[n] = item.object_uid;
			}
			const auto &row = *found->second;
			need(row.root_item_uid == roots[n] && row.parent_item_uid == parent &&
				     row.vnum == item.vnum && row.item_revision,
			     ESTALE);
			if (view.role == native_forest_role::ordinary)
				need(row.equipment_slot == item.equipment_slot, ESTALE);
			else
			{
				// Original flat pet baseline records custody slot zero. Native
				// loading separately accepts unique equipped roots at slot+1.
				need(row.equipment_slot == 0, ESTALE);
				need(item.equipment_slot >= 0 && item.equipment_slot <= MAX_WEAR &&
				     (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ||
				      item.equipment_slot == 0) &&
				     (!item.equipment_slot ||
				      native_pet_slots.insert(item.equipment_slot).second));
			}
			indexed.erase(found);
		}
	}
	need(indexed.empty() && count == custody.size(), ESTALE);
}
void forest(const std::vector<player_item_snapshot> &items,
	    const std::vector<flatfile_item_ownership_record> &custody,
	    const item_owner_identity &owner)
{
	const native_forest_view view{ &items, native_forest_role::ordinary };
	forests(std::span<const native_forest_view>(&view, 1), custody, owner);
}
std::vector<flatfile_item_ownership_record>
active_owner(const std::vector<flatfile_item_ownership_record> &, const item_owner_identity &);
void whole_player_forest(const player_snapshot &player,
			 const std::vector<flatfile_item_ownership_record> &player_rows,
			 const std::vector<flatfile_item_ownership_record> &pet_rows)
{
	std::vector<native_forest_view> legacy;
	legacy.push_back({ &player.items, native_forest_role::ordinary });
	for (const auto &pet : player.pets)
	{
		if (pet.pet_uid)
		{
			const item_owner_identity owner{ item_owner_type::pet, pet.pet_uid,
							 uint64_t(player.pid) };
			const native_forest_view view{ &pet.items, native_forest_role::pet };
			forests(std::span<const native_forest_view>(&view, 1),
				active_owner(pet_rows, owner), owner);
		}
		else
			legacy.push_back({ &pet.items, native_forest_role::pet });
	}
	// Preserve the original aggregate PC/UID-zero-pet codec byte bound.
	// This temporary framing is validation only; correspondence below uses
	// the authentic unmodified native forests and their separate slot rules.
	auto combined = player.items;
	for (const auto &pet : player.pets)
		if (!pet.pet_uid)
		{
			need(combined.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS &&
				     pet.items.size() <=
					     PLAYER_SNAPSHOT_MAX_OBJECTS - combined.size(),
			     E2BIG);
			const size_t offset = combined.size();
			for (auto item : pet.items)
			{
				if (item.parent_index >= 0)
					item.parent_index += offset;
				combined.push_back(std::move(item));
			}
		}
	std::vector<uint8_t> combined_bytes;
	checked(player_item_snapshot_list_encode(combined, &combined_bytes) ==
				player_snapshot_codec_result::ok ?
			0U :
			unsigned(EILSEQ));
	// Share the authentic player owner's row index across PC and UID-zero pets.
	// Each native forest retains its original body, positions and local parents.
	forests(legacy, player_rows, { item_owner_type::player, uint64_t(player.pid), 0 });
}
void manifest(const std::vector<player_item_snapshot> &items,
	      const shop_trade_recovery_forest_binding &binding,
	      shop_trade_recovery_forest_role role)
{
	std::vector<uint8_t> bytes;
	need(player_item_snapshot_list_encode(items, &bytes) == player_snapshot_codec_result::ok,
	     EILSEQ);
	need(shop_trade_recovery_forest_verify(bytes, role, binding), ESTALE);
}
std::vector<uint8_t> selected_blob(const std::vector<player_item_snapshot> &items, uint64_t uid)
{
	std::vector<player_item_snapshot> selected;
	std::vector<int32_t> positions(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
	for (size_t n = 0; n < items.size(); ++n)
	{
		const auto &item = items[n];
		const bool root = item.object_uid == uid;
		const bool child = item.parent_index >= 0 && size_t(item.parent_index) < n &&
				   positions[item.parent_index] >= 0;
		if (!root && !child)
			continue;
		need(!root || selected.empty(), EILSEQ);
		auto copy = item;
		copy.parent_index = root ? PLAYER_SNAPSHOT_NO_PARENT : positions[item.parent_index];
		positions[n] = selected.size();
		selected.push_back(std::move(copy));
	}
	std::vector<uint8_t> encoded;
	need(!selected.empty(), ESTALE);
	need(player_item_snapshot_list_encode(selected, &encoded) ==
	     player_snapshot_codec_result::ok);
	return encoded;
}
void original_literals(const identity &id, const flatfile_accounting_shop_projection &current)
{
	const auto &p = id.payload;
	if (p.action == shop_trade_action::buy_produced)
	{
		for (size_t n = 0; n < p.item_count; ++n)
		{
			const auto uid = p.items[n].item_uid;
			need(std::none_of(current.player.items.begin(), current.player.items.end(),
					  [uid](const auto &item)
					  { return item.object_uid == uid; }) &&
				     std::none_of(current.keeper.items.begin(),
						  current.keeper.items.end(),
						  [uid](const auto &item)
						  { return item.object_uid == uid; }),
			     ESTALE);
		}
		return;
	}
	const bool from_keeper = p.action == shop_trade_action::buy_existing ||
				 p.action == shop_trade_action::discard_invalid;
	const auto bytes = selected_blob(from_keeper ? current.keeper.items : current.player.items,
					 p.selected_item_uid);
	need(bytes.size() == p.item_blob_size &&
		     std::equal(bytes.begin(), bytes.end(), p.item_blob.begin()),
	     ESTALE);
}
const player_item_snapshot &item_body(const player_snapshot &player, uint64_t uid)
{
	const player_item_snapshot *found = nullptr;
	for (const auto &item : player.items)
		if (item.object_uid == uid)
		{
			need(!found);
			found = &item;
		}
	need(found, ESTALE);
	return *found;
}
const player_item_snapshot &target_body(const player_snapshot &player, uint64_t uid)
{
	const auto &item = item_body(player, uid);
	need(item.parent_index == PLAYER_SNAPSHOT_NO_PARENT && !item.equipment_slot &&
		     item.type == ITEM_CONTAINER,
	     ESTALE);
	return item;
}
void proposed_player(const identity &id, const player_snapshot &before,
		     const player_snapshot &after)
{
	// The native helper may change item placement/body only. Authenticate every
	// other original checkpoint field, including status, pets and components.
	auto restored = after;
	restored.items = before.items;
	std::vector<uint8_t> original, restored_bytes;
	need(flatfile_player_snapshot_encode_file(before, &original) &&
	     flatfile_player_snapshot_encode_file(restored, &restored_bytes) &&
	     original == restored_bytes);
	if (!id.payload.target_parent_item_uid)
		return;
	const auto &native_before = item_body(before, id.payload.target_parent_item_uid);
	const auto &native_after = item_body(after, id.payload.target_parent_item_uid);
	auto expected = native_before;
	if (id.payload.native_destination_weight_recorded)
	{
		std::vector<player_item_snapshot> selected;
		need(shop_trade_accounted_after_items(id.payload, &selected) && !selected.empty());
		need(shop_trade_destination_weight_verify(
			target_body(before, id.payload.target_parent_item_uid),
			selected.front().weight, id.payload.destination_weight));
		expected.weight = id.payload.destination_weight.after;
	}
	auto actual = native_after;
	expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	actual.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	std::vector<uint8_t> expected_bytes, actual_bytes;
	need(player_item_snapshot_list_encode({ expected }, &expected_bytes) ==
			     player_snapshot_codec_result::ok &&
		     player_item_snapshot_list_encode({ actual }, &actual_bytes) ==
			     player_snapshot_codec_result::ok &&
		     expected_bytes == actual_bytes,
	     ESTALE);
}
flatfile_accounting_shop_projection
current_values(const std::string &root, const flatfile_authority_lock &lock, const identity &id)
{
	flatfile_accounting_shop_projection result;
	checked(flatfile_player_domain_load_locked(
		root, lock, id.payload.player_pid, canonical(id.payload.account_name.data()),
		id.payload.racewar, &result.player_domains, nullptr));
	need(canonical(result.player_domains.account_name) ==
			     canonical(id.payload.account_name.data()) &&
		     result.player_domains.racewar == id.payload.racewar,
	     EACCES);
	const auto loaded =
		flatfile_player_snapshot_read(root, id.payload.player_pid, &result.player, nullptr);
	need(loaded == flatfile_player_load_result::ok,
	     loaded == flatfile_player_load_result::io_error  ? EIO :
	     loaded == flatfile_player_load_result::not_found ? ENOENT :
								EILSEQ);
	std::vector<flatfile_shopkeeper_record> keepers;
	checked(flatfile_shopkeeper_list_locked(root, lock, &keepers, nullptr));
	const auto keeper = std::find_if(keepers.begin(), keepers.end(), [&](const auto &value)
					 { return value.shop_id == id.payload.shop_id; });
	need(keeper != keepers.end(), ENOENT);
	result.keeper = *keeper;
	checked(flatfile_item_repository_load_owner_locked(root, lock, player_owner(id.payload),
							   &result.player_owner_revision,
							   &result.player_custody, nullptr));
	checked(flatfile_item_repository_load_owner_locked(root, lock, keeper_owner(id.payload),
							   &result.keeper_owner_revision,
							   &result.keeper_custody, nullptr));
	checked(flatfile_shop_trade_materialization_reconcile(
		root, lock, id.payload.player_pid, result.player_custody, &result.player, nullptr));
	for (const auto &pet : result.player.pets)
		if (pet.pet_uid)
		{
			const item_owner_identity owner{ item_owner_type::pet, pet.pet_uid,
							 id.payload.player_pid };
			need(std::none_of(
				result.pet_owner_revisions.begin(),
				result.pet_owner_revisions.end(), [&](const auto &entry)
				{ return item_owner_identity_equal(entry.owner, owner); }));
			uint64_t revision = 0;
			std::vector<flatfile_item_ownership_record> rows;
			checked(flatfile_item_repository_load_owner_locked(
				root, lock, owner, &revision, &rows, nullptr));
			need(std::all_of(rows.begin(), rows.end(), [&](const auto &row)
					 { return item_owner_identity_equal(row.owner, owner); }));
			result.pet_owner_revisions.push_back({ owner, revision });
			result.pet_custody.insert(result.pet_custody.end(), rows.begin(),
						  rows.end());
		}
	whole_player_forest(result.player, result.player_custody, result.pet_custody);
	forest(result.keeper.items, result.keeper_custody, keeper_owner(id.payload));
	std::vector<flatfile_item_ownership_record> ignored;
	const auto first = primary(id.payload), second = other(id.payload);
	checked(flatfile_item_repository_load_owner_locked(
		root, lock, first, &result.primary_owner_revision, &ignored, nullptr));
	const auto other_status = flatfile_item_repository_load_owner_locked(
		root, lock, second, &result.counterparty_owner_revision, &ignored, nullptr);
	// The original creation/destruction participant may establish this owner;
	// observed absence is revision zero only BEFORE that actual participant.
	if (other_status != flatfile_item_repository_result::not_found)
		checked(other_status);
	for (size_t n = 0; n < id.payload.item_count; ++n)
	{
		flatfile_item_ownership_record row;
		const auto found = flatfile_item_repository_lookup_uid_locked(
			root, lock, id.payload.items[n].item_uid, &row, nullptr);
		if (found == flatfile_item_repository_result::not_found &&
		    id.payload.action == shop_trade_action::buy_produced)
			continue;
		checked(found);
		result.selected_custody.push_back(std::move(row));
	}
	return result;
}
shop_trade_accounting_authority before_authority(const std::string &root,
						 const flatfile_authority_lock &lock,
						 const identity &id,
						 const flatfile_accounting_shop_projection &current)
{
	shop_trade_accounting_authority result{};
	result.epoch = id.intent.admission.metadata.epoch;
	result.wallet_account = id.wallet;
	result.bank_account = id.bank;
	result.keeper_account = id.counterparty;
	result.balances_before = balances(current.player_domains);
	result.shop_id = current.keeper.shop_id;
	result.keeper_vnum = current.keeper.mob_vnum;
	result.keeper_cash_before = current.keeper.cash;
	result.keeper_roaming = current.keeper.roaming;
	result.shop_revision_before = current.keeper.revision;
	result.player_owner_revision_before = current.primary_owner_revision;
	result.counterparty_owner_revision_before = current.counterparty_owner_revision;
	auto add = [&](uint64_t uid, bool optional)
	{
		if (std::any_of(result.items_before.begin(), result.items_before.end(),
				[&](const auto &value) { return value.uid == uid; }))
			return;
		flatfile_item_ownership_record row;
		const auto read =
			flatfile_item_repository_lookup_uid_locked(root, lock, uid, &row, nullptr);
		if (read == flatfile_item_repository_result::not_found && optional)
		{
			result.items_before.push_back({ uid, {} });
			result.item_vnums_before.push_back(0);
			return;
		}
		checked(read);
		result.items_before.push_back(snapshot(row));
		result.item_vnums_before.push_back(row.vnum);
	};
	for (size_t n = 0; n < id.payload.item_count; ++n)
		add(id.payload.items[n].item_uid,
		    id.payload.action == shop_trade_action::buy_produced);
	if (id.payload.action == shop_trade_action::buy_produced)
	{
		add(id.payload.stock_item_uid, false);
		uint64_t ancestor = id.payload.target_parent_item_uid;
		while (ancestor)
		{
			need(result.items_before.size() < ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES,
			     E2BIG);
			const size_t old_size = result.items_before.size();
			add(ancestor, false);
			need(result.items_before.size() != old_size);
			ancestor = result.items_before.back().position.parent_uid;
		}
	}
	return result;
}
bool business(unsigned int code)
{
	return code == ESTALE || code == ENOSPC || code == ERANGE || code == ENOENT ||
	       code == EOPNOTSUPP || code == EINVAL;
}
void frozen_before(const identity &id, const flatfile_accounting_shop_projection &current,
		   const shop_trade_accounting_authority &before)
{
	const auto &p = id.payload;
	need(before.balances_before.wallet_revision == p.expected_wallet_revision &&
		     before.balances_before.bank_revision == p.expected_bank_revision &&
		     current.keeper.revision == p.expected_shop_revision &&
		     current.keeper.cash == p.expected_keeper_cash &&
		     current.keeper.mob_vnum == p.keeper_vnum &&
		     current.keeper.roaming == bool(p.keeper_roaming),
	     ESTALE);
	const bool produced = p.action == shop_trade_action::buy_produced;
	const auto source = p.action == shop_trade_action::buy_existing ||
					    p.action == shop_trade_action::discard_invalid ?
				    keeper_owner(p) :
				    player_owner(p);
	for (size_t n = 0; n < p.item_count; ++n)
	{
		const auto &entry = p.items[n];
		const auto found = std::find_if(before.items_before.begin(),
						before.items_before.end(), [&](const auto &item)
						{ return item.uid == entry.item_uid; });
		const economic_item_position expected =
			produced ? economic_item_position{} :
				   economic_item_position{ source,
							   entry.root_item_uid,
							   entry.parent_item_uid,
							   entry.expected_item_revision,
							   entry.expected_state,
							   0 };
		need(found != before.items_before.end() &&
			     economic_item_position_equal(found->position, expected),
		     ESTALE);
	}
	if (produced)
	{
		const auto stock = std::find_if(before.items_before.begin(),
						before.items_before.end(), [&](const auto &item)
						{ return item.uid == p.stock_item_uid; });
		const economic_item_position expected{ keeper_owner(p),
						       p.stock_item_uid,
						       0,
						       p.expected_stock_item_revision,
						       item_custody_state::active,
						       0 };
		need(stock != before.items_before.end() &&
			     economic_item_position_equal(stock->position, expected),
		     ESTALE);
		if (p.target_parent_item_uid)
		{
			const auto target =
				std::find_if(before.items_before.begin(), before.items_before.end(),
					     [&](const auto &item)
					     { return item.uid == p.target_parent_item_uid; });
			need(target != before.items_before.end() &&
				     item_owner_identity_equal(target->position.owner,
							       player_owner(p)) &&
				     target->position.root_uid == p.target_root_item_uid &&
				     target->position.revision ==
					     p.expected_target_parent_revision &&
				     target->position.state == item_custody_state::active &&
				     !target->position.equipment_slot,
			     ESTALE);
		}
	}
	original_literals(id, current);
	if (p.native_destination_weight_recorded && p.target_parent_item_uid)
	{
		std::vector<player_item_snapshot> selected;
		need(shop_trade_accounted_after_items(p, &selected) && !selected.empty());
		need(shop_trade_destination_weight_verify(
			     target_body(current.player, p.target_parent_item_uid),
			     selected.front().weight, p.destination_weight),
		     ESTALE);
	}
}
std::vector<flatfile_item_ownership_record>
staged_custody(const identity &id, const flatfile_item_shop_trade_mutation &mutation,
	       const shop_trade_result &result, const economic_accounting_plan &plan)
{
	const std::array<item_owner_identity, 2> owners{ primary(id.payload), other(id.payload) };
	std::array<uint64_t, 2> revisions{};
	std::vector<flatfile_item_ownership_record> rows;
	checked(flatfile_item_repository_read_trade_after_image(mutation.after_image, owners,
								&revisions, &rows, nullptr));
	need(revisions[0] == result.player_owner_revision &&
	     revisions[1] == result.counterparty_owner_revision);
	for (const auto &expected : plan.items_after)
	{
		const auto found = std::find_if(rows.begin(), rows.end(), [&](const auto &row)
						{ return row.item_uid == expected.uid; });
		need(found != rows.end() &&
		     economic_item_position_equal(snapshot(*found).position, expected.position));
	}
	return rows;
}
std::vector<flatfile_item_ownership_record>
active_owner(const std::vector<flatfile_item_ownership_record> &rows,
	     const item_owner_identity &owner)
{
	std::vector<flatfile_item_ownership_record> result;
	for (const auto &row : rows)
		if (row.state == item_custody_state::active &&
		    item_owner_identity_equal(row.owner, owner))
			result.push_back(row);
	return result;
}
}

void flatfile_accounting_shop_transaction::verify_record_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record, std::string *error)
{
	const auto id = decode(record.command);
	retained_identity(root, lock, record.command, id);
	need(record.failure_stage == critical_failure_stage::none);
	shop_trade_result result{};
	need(shop_trade_command_decode_result(record.result.data(), record.result.size(), &result));
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded{};
	need(shop_trade_command_encode_result(result, &encoded) &&
	     record.result.size() == encoded.size() &&
	     std::equal(record.result.begin(), record.result.end(), encoded.begin()));
	need(result.action == id.payload.action && record.durable_revision == durable(result));
	critical_apply_result native;
	checked(flatfile_shop_trade_storage::lookup_locked(root, lock, record.command, &native,
							   error));
	need(native.error_code == record.result_code &&
	     native.result_size == record.result.size() &&
	     std::equal(record.result.begin(), record.result.end(), native.result_payload.begin()));
	if (record.result_code)
	{
		need(record.plan.empty() && business(record.result_code) &&
		     !result.keeper_cash_recorded && !result.shop_revision &&
		     !result.player_owner_revision && !result.counterparty_owner_revision &&
		     !result.item_count &&
		     result.wallet_revision == id.payload.expected_wallet_revision &&
		     result.bank_revision == id.payload.expected_bank_revision &&
		     std::all_of(result.item_uids.begin(), result.item_uids.end(),
				 [](uint64_t value) { return !value; }) &&
		     std::all_of(result.item_revisions.begin(), result.item_revisions.end(),
				 [](uint64_t value) { return !value; }));
		checked(flatfile_item_accounting_reference_verify_operation(
			root, record.command.operation_id, {}, error));
	}
	else
	{
		economic_accounting_plan retained, expected;
		checked(economic_plan_decode(record.plan, &retained));
		const auto original = historical_authority(id, result, retained);
		checked(shop_trade_accounting_plan(record.command, id.intent, original, result,
						   &expected));
		std::vector<uint8_t> bytes;
		checked(economic_plan_encode(expected, &bytes));
		need(bytes == record.plan);
		const auto refs = references(record.command, expected);
		checked(flatfile_item_accounting_reference_verify_operation(
			root, record.command.operation_id, refs, error));
	}
	checked(flatfile_accounting_storage::verify_source_claim(root, lock, record, error));
}
critical_apply_result flatfile_accounting_shop_transaction::verify_retained_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root), EINVAL);
		(void)decode(command);
		flatfile_accounting_record retained;
		checked(flatfile_accounting_lookup(root, lock, command, &retained, nullptr));
		verify_record_locked(root, lock, retained, nullptr);
		return completion(retained, true);
	}
	catch (const failure &error)
	{
		return { critical_apply_outcome::retryable_failure, 0, error.code };
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	catch (...)
	{
		return { critical_apply_outcome::retryable_failure, 0, EFAULT };
	}
}
unsigned int flatfile_accounting_shop_transaction::read_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, const critical_completion &receipt,
	flatfile_accounting_shop_projection *output, std::string *error) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root) && output, EINVAL);
		const auto id = decode(command);
		flatfile_accounting_record retained;
		checked(flatfile_accounting_lookup(root, lock, command, &retained, error));
		verify_record_locked(root, lock, retained, error);
		const auto terminal = completion(retained, true);
		need(receipt.disposition == critical_completion_disposition::execution &&
			     critical_operation_id_equal(receipt.operation_id,
							 command.operation_id) &&
			     (retained.result_code ?
				      receipt.outcome == critical_apply_outcome::terminal_failure :
				      receipt.outcome == critical_apply_outcome::applied ||
					      receipt.outcome ==
						      critical_apply_outcome::already_applied) &&
			     receipt.durable_revision == terminal.durable_revision &&
			     receipt.error_code == terminal.error_code &&
			     receipt.failure_stage == terminal.failure_stage &&
			     receipt.result_size == terminal.result_size &&
			     std::equal(retained.result.begin(), retained.result.end(),
					receipt.result_payload.begin()) &&
			     std::all_of(receipt.result_payload.begin() + receipt.result_size,
					 receipt.result_payload.end(),
					 [](uint8_t byte) { return !byte; }),
		     EEXIST);
		current_authority(root, lock, id);
		auto current = current_values(root, lock, id);
		const auto level = status(current.player, player_status_field::level);
		need(level && level <= UINT8_MAX &&
			     current.player.revision >= id.payload.expected_player_save_revision &&
			     status(current.player, player_status_field::racewar) ==
				     id.payload.racewar,
		     ESTALE);
		shop_trade_result result{};
		need(shop_trade_command_decode_result(retained.result.data(),
						      retained.result.size(), &result));
		const auto money = balances(current.player_domains);
		need(money.wallet.amount == result.wallet.amount &&
			     money.bank.amount == result.bank.amount &&
			     money.wallet_revision == result.wallet_revision &&
			     money.bank_revision == result.bank_revision,
		     ESTALE);
		if (retained.result_code)
			frozen_before(id, current, before_authority(root, lock, id, current));
		else
		{
			need(current.keeper.revision == result.shop_revision &&
				     current.keeper.cash == result.keeper_cash &&
				     current.keeper.mob_vnum == id.payload.keeper_vnum &&
				     current.keeper.roaming == bool(id.payload.keeper_roaming) &&
				     current.primary_owner_revision ==
					     result.player_owner_revision &&
				     current.counterparty_owner_revision ==
					     result.counterparty_owner_revision,
			     ESTALE);
			economic_accounting_plan plan;
			checked(economic_plan_decode(retained.plan, &plan));
			for (const auto &value : plan.items_after)
			{
				flatfile_item_ownership_record row;
				checked(flatfile_item_repository_lookup_uid_locked(
					root, lock, value.uid, &row, error));
				need(economic_item_position_equal(snapshot(row).position,
								  value.position),
				     ESTALE);
			}
			if (id.payload.action == shop_trade_action::buy_existing ||
			    id.payload.action == shop_trade_action::buy_produced ||
			    id.payload.action == shop_trade_action::sell_store)
			{
				std::vector<player_item_snapshot> after;
				need(shop_trade_accounted_after_items(id.payload, &after));
				std::vector<uint8_t> bytes;
				need(player_item_snapshot_list_encode(after, &bytes) ==
				     player_snapshot_codec_result::ok);
				need(selected_blob(id.payload.action ==
								   shop_trade_action::sell_store ?
							   current.keeper.items :
							   current.player.items,
						   id.payload.selected_item_uid) == bytes,
				     ESTALE);
			}
		}
		if (id.payload.recovery_manifest_recorded)
		{
			const auto &m = id.payload.recovery_manifest;
			manifest(current.player.items,
				 retained.result_code ? m.player_before : m.player_after,
				 retained.result_code ?
					 shop_trade_recovery_forest_role::player_before :
					 shop_trade_recovery_forest_role::player_after);
			manifest(current.keeper.items,
				 retained.result_code ? m.keeper_before : m.keeper_after,
				 retained.result_code ?
					 shop_trade_recovery_forest_role::keeper_before :
					 shop_trade_recovery_forest_role::keeper_after);
		}
		if (id.payload.native_destination_weight_recorded &&
		    id.payload.target_parent_item_uid)
			need(target_body(current.player, id.payload.target_parent_item_uid).weight ==
				     (retained.result_code ? id.payload.destination_weight.before :
							     id.payload.destination_weight.after),
			     ESTALE);
		*output = std::move(current);
		return 0;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}
unsigned int flatfile_accounting_shop_transaction::read_never_admitted_before_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, flatfile_accounting_shop_projection *output,
	std::string *error) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root) && output, EINVAL);
		const auto id = decode(command);
		need(command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION &&
			     id.payload.recovery_manifest_recorded,
		     EINVAL);
		// Drain the original authority and legacy/current wallet-bank journals
		// before any genuine CURRENT observation. No local authority is acquired.
		checked(flatfile_player_domain_recover_locked(root, lock, error));
		flatfile_accounting_record retained;
		const auto existing =
			flatfile_accounting_lookup(root, lock, command, &retained, error);
		if (existing != flatfile_accounting_status::not_found)
		{
			checked(existing);
			need(false, EEXIST);
		}
		critical_apply_result native;
		const auto recorded = flatfile_shop_trade_storage::lookup_locked(
			root, lock, command, &native, error);
		need(recorded == ENOENT, recorded ? recorded : unsigned(EEXIST));
		retained_identity(root, lock, command, id);
		current_authority(root, lock, id);
		auto current = current_values(root, lock, id);
		frozen_before(id, current, before_authority(root, lock, id, current));
		need(current.player.revision == id.payload.expected_player_save_revision &&
			     status(current.player, player_status_field::level) ==
				     id.payload.expected_player_level &&
			     status(current.player, player_status_field::racewar) ==
				     id.payload.racewar,
		     ESTALE);
		const auto &binding = id.payload.recovery_manifest;
		manifest(current.player.items, binding.player_before,
			 shop_trade_recovery_forest_role::player_before);
		manifest(current.keeper.items, binding.keeper_before,
			 shop_trade_recovery_forest_role::keeper_before);
		if (id.payload.target_parent_item_uid)
			need(shop_trade_recovery_forest_verify(
				     selected_blob(current.player.items,
						   id.payload.target_parent_item_uid),
				     shop_trade_recovery_forest_role::live_target_before,
				     binding.live_target_before),
			     ESTALE);
		// Only source values leave this owner. Original refusal, physical
		// cleanup and attempted-hold disposition belong to their genuine owners.
		*output = std::move(current);
		return 0;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}
critical_apply_result flatfile_accounting_shop_transaction::apply(const std::string &root,
								  const critical_command &command)
{
	flatfile_authority_commit_outcome commit_state =
		flatfile_authority_commit_outcome::not_published;
	critical_apply_result provisional{ critical_apply_outcome::retryable_failure, 0, EIO };
	try
	{
		need(!root.empty(), EINVAL);
		const auto id = decode(command);
		flatfile_identity_lock identity_lock;
		flatfile_authority_lock lock;
		need(identity_lock.acquire(root, nullptr) && lock.acquire(root, nullptr), EIO);
		checked(flatfile_player_domain_recover_locked(root, lock, nullptr));
		flatfile_accounting_record retained;
		const auto existing =
			flatfile_accounting_lookup(root, lock, command, &retained, nullptr);
		if (existing == flatfile_accounting_status::ok)
		{
			verify_record_locked(root, lock, retained, nullptr);
			return completion(retained, true);
		}
		if (existing != flatfile_accounting_status::not_found)
			checked(existing);
		critical_apply_result legacy;
		const auto native = flatfile_shop_trade_storage::lookup_locked(root, lock, command,
									       &legacy, nullptr);
		need(native == ENOENT, native ? native : unsigned(EEXIST));
		retained_identity(root, lock, command, id);
		current_authority(root, lock, id);
		flatfile_identity_record actor;
		need(flatfile_identity_lookup_pid_locked(root, identity_lock, lock,
							 id.payload.player_pid, &actor,
							 nullptr) == flatfile_identity_result::ok,
		     EIO);
		need(actor.active &&
			     canonical(actor.account) ==
				     canonical(id.payload.account_name.data()) &&
			     actor.racewar == id.payload.racewar,
		     EACCES);
		auto current = current_values(root, lock, id);
		auto authority = before_authority(root, lock, id, current);
		frozen_before(id, current, authority);
		unsigned int result_code = 0;
		need(current.player.revision == id.payload.expected_player_save_revision &&
			     status(current.player, player_status_field::level) ==
				     id.payload.expected_player_level &&
			     status(current.player, player_status_field::racewar) ==
				     id.payload.racewar,
		     ESTALE);
		if (id.payload.recovery_manifest_recorded)
		{
			manifest(current.player.items, id.payload.recovery_manifest.player_before,
				 shop_trade_recovery_forest_role::player_before);
			manifest(current.keeper.items, id.payload.recovery_manifest.keeper_before,
				 shop_trade_recovery_forest_role::keeper_before);
		}
		// v6 keeps its actual original persisted target weight. Only v7/v8
		// recorded native weight may change the full player-body participant.
		const auto body = legacy_body(id.payload);
		flatfile_wallet_mutation wallet;
		flatfile_item_shop_trade_mutation items;
		flatfile_shopkeeper_trade_mutation shop;
		flatfile_shop_trade_materialization_mutation materialization;
		if (!result_code)
			checked(flatfile_player_domain_prepare_wallet(
				root, lock, body.player_pid, body.account_name.data(), body.racewar,
				body.expected_wallet_revision, body.expected_bank_revision, 0,
				false, &wallet, &result_code, nullptr));
		if (!result_code)
			checked(flatfile_item_repository_prepare_shop_trade(
				root, lock, body, &items, &result_code, nullptr));
		if (!result_code)
			checked(flatfile_shopkeeper_prepare_trade(root, lock, body, &shop,
								  &result_code, nullptr));
		if (!result_code && body.action != shop_trade_action::discard_invalid)
		{
			const bool buy = body.action == shop_trade_action::buy_existing ||
					 body.action == shop_trade_action::buy_produced;
			checked(flatfile_player_domain_prepare_wallet(
				root, lock, body.player_pid, body.account_name.data(), body.racewar,
				body.expected_wallet_revision, body.expected_bank_revision,
				buy ? -body.price : body.price, true, &wallet, &result_code,
				nullptr));
		}
		shop_trade_result result{};
		result.action = body.action;
		result.wallet = authority.balances_before.wallet;
		result.bank = authority.balances_before.bank;
		result.wallet_revision = authority.balances_before.wallet_revision;
		result.bank_revision = authority.balances_before.bank_revision;
		if (!result_code)
		{
			if (body.action != shop_trade_action::discard_invalid)
			{
				result.wallet = wallet.wallet;
				result.bank = wallet.bank;
				result.wallet_revision = wallet.wallet_revision;
				result.bank_revision = wallet.bank_revision;
			}
			result.shop_revision = shop.shop_revision;
			result.keeper_cash = shop.keeper_cash;
			result.keeper_cash_recorded = true;
			result.player_owner_revision = items.player_owner_revision;
			result.counterparty_owner_revision = items.counterparty_owner_revision;
			result.item_count = items.item_count;
			result.item_uids = items.item_uids;
			result.item_revisions = items.item_revisions;
		}
		flatfile_accounting_record record;
		record.command = command;
		record.result_code = result_code;
		record.durable_revision = durable(result);
		std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> bytes{};
		need(shop_trade_command_encode_result(result, &bytes));
		record.result.assign(bytes.begin(), bytes.end());
		std::vector<flatfile_authority_operation> operations;
		if (!result_code)
		{
			economic_accounting_plan plan;
			checked(shop_trade_accounting_plan(command, id.intent, authority, result,
							   &plan));
			checked(economic_plan_encode(plan, &record.plan));
			const auto rows = staged_custody(id, items, result, plan);
			const auto player_rows = active_owner(rows, player_owner(id.payload));
			const auto keeper_rows = active_owner(rows, keeper_owner(id.payload));
			flatfile_shopkeeper_record keeper_after;
			checked(flatfile_shopkeeper_read_trade_after_image(
				shop.after_image, id.payload.shop_id, &keeper_after, nullptr));
			need(keeper_after.revision == result.shop_revision &&
			     keeper_after.cash == result.keeper_cash &&
			     keeper_after.mob_vnum == id.payload.keeper_vnum &&
			     keeper_after.roaming == bool(id.payload.keeper_roaming));
			forest(keeper_after.items, keeper_rows, keeper_owner(id.payload));
			player_snapshot player_after = current.player;
			if (body.action != shop_trade_action::discard_invalid)
				checked(flatfile_shop_trade_materialization_prepare_accounted(
					root, lock, command.operation_id, id.payload,
					current.player_custody, player_rows, current.player,
					&materialization, &player_after, nullptr));
			proposed_player(id, current.player, player_after);
			whole_player_forest(player_after, player_rows, rows);
			if (id.payload.recovery_manifest_recorded)
			{
				manifest(player_after.items,
					 id.payload.recovery_manifest.player_after,
					 shop_trade_recovery_forest_role::player_after);
				manifest(keeper_after.items,
					 id.payload.recovery_manifest.keeper_after,
					 shop_trade_recovery_forest_role::keeper_after);
			}
			auto image = [&](flatfile_authority_after_image &value)
			{
				operations.push_back({ flatfile_authority_store::domains,
						       flatfile_authority_operation_kind::write,
						       std::move(value.filename),
						       std::move(value.bytes) });
			};
			image(shop.after_image);
			image(items.after_image);
			if (body.action != shop_trade_action::discard_invalid)
			{
				image(materialization.after_image);
				for (auto &value : wallet.after_images)
					image(value);
			}
			if (id.payload.target_parent_item_uid &&
			    id.payload.native_destination_weight_recorded)
			{
				need(player_after.pid == current.player.pid &&
				     player_after.revision == current.player.revision &&
				     player_after.components == current.player.components &&
				     status(player_after, player_status_field::level) ==
					     id.payload.expected_player_level &&
				     status(player_after, player_status_field::racewar) ==
					     id.payload.racewar);
				std::vector<uint8_t> native_body;
				need(flatfile_player_snapshot_encode_file(player_after,
									  &native_body),
				     EILSEQ);
				operations.push_back(
					{ flatfile_authority_store::players,
					  flatfile_authority_operation_kind::write,
					  flatfile_player_snapshot_file::player_filename(
						  id.payload.player_pid),
					  std::move(native_body) });
			}
			const auto refs = references(command, plan);
			checked(flatfile_item_accounting_reference_stage(
				root, lock, command.operation_id, refs, &operations, nullptr));
		}
		else
			need(business(result_code));
		checked(flatfile_shop_trade_storage::stage_locked(
			root, lock, command, result_code, record.result, &operations, nullptr));
		checked(flatfile_accounting_storage::stage(root, lock, record, &operations,
							   nullptr));
		checked(flatfile_accounting_storage::stage_source_claim(root, lock, record,
									&operations, nullptr));
		provisional = completion(record, false);
		const auto committed = flatfile_accounting_storage::commit_with_outcome(
			root, lock, operations, nullptr, &commit_state);
		need(committed == flatfile_authority_transaction_result::ok, EIO);
		checked(flatfile_accounting_lookup(root, lock, command, &retained, nullptr));
		verify_record_locked(root, lock, retained, nullptr);
		need(retained.plan == record.plan && retained.result == record.result &&
		     retained.result_code == record.result_code &&
		     retained.durable_revision == record.durable_revision);
		critical_completion receipt{};
		receipt.operation_id = command.operation_id;
		receipt.outcome = provisional.outcome;
		receipt.durable_revision = provisional.durable_revision;
		receipt.error_code = provisional.error_code;
		receipt.failure_stage = provisional.failure_stage;
		receipt.result_size = provisional.result_size;
		receipt.result_payload = provisional.result_payload;
		flatfile_accounting_shop_projection verified;
		checked(read_current_locked(root, lock, command, receipt, &verified, nullptr));
		return completion(retained, false);
	}
	catch (const failure &error)
	{
		if (commit_state != flatfile_authority_commit_outcome::not_published)
		{
			provisional.outcome = critical_apply_outcome::ambiguous_commit;
			return provisional;
		}
		return { error.code == EEXIST ? critical_apply_outcome::terminal_failure :
						critical_apply_outcome::retryable_failure,
			 0, error.code };
	}
	catch (const std::bad_alloc &)
	{
		if (commit_state != flatfile_authority_commit_outcome::not_published)
		{
			provisional.outcome = critical_apply_outcome::ambiguous_commit;
			return provisional;
		}
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	catch (...)
	{
		if (commit_state != flatfile_authority_commit_outcome::not_published)
		{
			provisional.outcome = critical_apply_outcome::ambiguous_commit;
			return provisional;
		}
		return { critical_apply_outcome::retryable_failure, 0, EFAULT };
	}
}
