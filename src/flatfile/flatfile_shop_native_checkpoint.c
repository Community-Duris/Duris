#include "flatfile/flatfile_shop_native_checkpoint.h"

#include "flatfile/flatfile_player_snapshot_file.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_codec.h"

#include <array>
#include <cerrno>
#include <new>
#include <set>
#include <unordered_set>
#include <utility>
#include "core/defines.h"
#include <algorithm>
#include <unordered_map>

namespace
{
constexpr player_component_mask_t checkpoint_components =
	PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;

bool canonical_account(const std::string &input, std::string *canonical)
{
	if (input.empty() || input.size() > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	*canonical = input;
	for (char &value : *canonical)
	{
		if (value >= 'A' && value <= 'Z')
			value += 'a' - 'A';
		if (!((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
		      value == '_' || value == '-'))
			return false;
	}
	return true;
}

unsigned int codec_result(player_snapshot_codec_result result)
{
	return result == player_snapshot_codec_result::ok		  ? 0U :
	       result == player_snapshot_codec_result::allocation_failure ? ENOMEM :
									    EILSEQ;
}

// Original SHOP checkpoint compares whole saved-policy PC items and level;
// other status may progress. Reject ambiguous duplicate status/UID/slot facts.
unsigned int checkpoint_body(const player_snapshot &snapshot, int32_t pid, int8_t racewar,
			     const player_shop_checkpoint_stage &stage, std::vector<uint8_t> *items)
{
	if (snapshot.pid != pid || snapshot.revision != stage.save_revision ||
	    (snapshot.components & checkpoint_components) != checkpoint_components)
		return ESTALE;
	std::set<player_status_field> fields;
	bool level_seen = false, racewar_seen = false;
	for (const auto &row : snapshot.status_integers)
	{
		if (!fields.insert(row.field).second)
			return EILSEQ;
		if (row.field == player_status_field::level)
		{
			if (row.is_unsigned ? !row.unsigned_value || row.unsigned_value > 255 :
					      row.signed_value <= 0 || row.signed_value > 255)
				return EILSEQ;
			const auto level = row.is_unsigned ?
						   row.unsigned_value :
						   static_cast<uint64_t>(row.signed_value);
			if (level != stage.level)
				return ESTALE;
			level_seen = true;
		}
		else if (row.field == player_status_field::racewar)
		{
			if (row.is_unsigned ? row.unsigned_value > INT8_MAX :
					      row.signed_value < 0 || row.signed_value > INT8_MAX)
				return EILSEQ;
			if (row.is_unsigned ? row.unsigned_value != static_cast<uint8_t>(racewar) :
					      row.signed_value != racewar)
				return ESTALE;
			racewar_seen = true;
		}
	}
	if (!level_seen || !racewar_seen)
		return EILSEQ;
	std::set<player_status_string_field> strings;
	for (const auto &row : snapshot.status_strings)
		if (!strings.insert(row.field).second)
			return EILSEQ;
	std::unordered_set<uint64_t> uids;
	std::set<int16_t> slots;
	for (const auto &item : snapshot.items)
		if (!item.object_uid || item.vnum <= 0 || !uids.insert(item.object_uid).second ||
		    item.equipment_slot < 0 ||
		    item.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT ||
		    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT && item.equipment_slot) ||
		    (item.equipment_slot && !slots.insert(item.equipment_slot).second))
			return EILSEQ;
	return codec_result(player_item_snapshot_list_encode(snapshot.items, items));
}
} // namespace

namespace
{
enum class current_native_forest_role : uint8_t
{
	player,
	pet
};
struct current_native_forest
{
	const std::vector<player_item_snapshot> *items;
	current_native_forest_role role;
};
// The actual flat player loader assigns UID-zero legacy pet forests to the
// player's owner/clock. Modern UID pets have their own namespace. Read-only
// comparison consumes every genuine active player-owner row without repairing,
// dropping or reconstructing any native item from custody.
unsigned int
current_owner_correspondence(const std::vector<flatfile_item_ownership_record> &custody,
			     const item_owner_identity &owner,
			     const std::vector<current_native_forest> &forests,
			     std::unordered_set<uint64_t> &native_uids)
{
	std::unordered_map<uint64_t, const flatfile_item_ownership_record *> by_uid;
	by_uid.reserve(custody.size());
	for (const auto &row : custody)
		if (!row.item_uid || !row.item_revision ||
		    row.state != item_custody_state::active ||
		    !item_owner_identity_equal(row.owner, owner) ||
		    !by_uid.emplace(row.item_uid, &row).second)
			return EILSEQ;
	auto forest = [&](const std::vector<player_item_snapshot> &items,
			  current_native_forest_role native_role) -> unsigned int
	{
		std::vector<uint8_t> encoded;
		auto code = codec_result(player_item_snapshot_list_encode(items, &encoded));
		if (code)
			return code;
		std::vector<uint64_t> roots(items.size());
		std::set<int16_t> native_pet_slots;
		for (size_t index = 0; index < items.size(); ++index)
		{
			const auto &item = items[index];
			const auto found = by_uid.find(item.object_uid);
			if (!item.object_uid || item.vnum <= 0 || found == by_uid.end() ||
			    !native_uids.insert(item.object_uid).second)
				return ESTALE;
			uint64_t parent_uid = 0;
			if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
				roots[index] = item.object_uid;
			else
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return EILSEQ;
				parent_uid = items[item.parent_index].object_uid;
				roots[index] = roots[item.parent_index];
			}
			const auto &row = *found->second;
			if (row.root_item_uid != roots[index] ||
			    row.parent_item_uid != parent_uid || row.vnum != item.vnum)
				return ESTALE;
			if (native_role == current_native_forest_role::player)
			{
				// Preserve exact original PC native/custody slot equality.
				if (item.equipment_slot < 0 ||
				    static_cast<uint16_t>(item.equipment_slot) !=
					    row.equipment_slot)
					return ESTALE;
			}
			else
			{
				// Native pet equipment and custody use separate original
				// representations. Baseline pet custody is slot zero, while
				// saved equipment is slot+1. Validate authentic positions per
				// pet forest without normalizing or widening either format.
				if (row.equipment_slot != 0)
					return ESTALE;
				if (item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR ||
				    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
				     item.equipment_slot != 0) ||
				    (item.equipment_slot > 0 &&
				     !native_pet_slots.insert(item.equipment_slot).second))
					return EILSEQ;
			}
			if (item.type == ITEM_MONEY &&
			    std::any_of(item.values.begin(), item.values.begin() + 4,
					[](int32_t value) { return value < 0; }))
				return EILSEQ;
			if (!row.coin_payload.empty())
			{
				std::vector<player_item_snapshot> coins;
				code = codec_result(player_item_snapshot_list_decode(
					row.coin_payload.data(), row.coin_payload.size(), &coins));
				if (code)
					return code;
				if (coins.size() != 1 || coins.front().type != ITEM_MONEY ||
				    item.type != ITEM_MONEY)
					return EILSEQ;
				// Topology and the authentic PC/pet native-position rules
				// are checked independently above. The original coin payload
				// stores one independent item; compare all remaining literal
				// properties through its original canonical codec.
				auto native_coin = item;
				auto retained_coin = coins.front();
				native_coin.parent_index = retained_coin.parent_index =
					PLAYER_SNAPSHOT_NO_PARENT;
				native_coin.equipment_slot = retained_coin.equipment_slot = 0;
				std::vector<uint8_t> native_bytes, retained_bytes;
				code = codec_result(player_item_snapshot_list_encode(
					{ native_coin }, &native_bytes));
				if (code)
					return code;
				code = codec_result(player_item_snapshot_list_encode(
					{ retained_coin }, &retained_bytes));
				if (code)
					return code;
				if (native_bytes != retained_bytes)
					return ESTALE;
			}
			// Erasure indexes consumption only; the actual rows/native file
			// remain intact. A repeated UID across PC/pets cannot match twice.
			by_uid.erase(found);
		}
		return 0;
	};
	for (const auto &native : forests)
	{
		const auto code = forest(*native.items, native.role);
		if (code)
			return code;
	}
	// No row filtering: missing payload, extra rows and incomplete current
	// forests refuse. The original actual owner clock (even zero) is retained.
	return by_uid.empty() ? 0U : ESTALE;
}

unsigned int current_player_owner_correspondence(const flatfile_shop_native_player_cut &cut,
						 std::unordered_set<uint64_t> &native_uids)
{
	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(cut.player.pid), 0 };
	std::vector<current_native_forest> forests;
	forests.push_back({ &cut.player.items, current_native_forest_role::player });
	for (const auto &pet : cut.player.pets)
		if (!pet.pet_uid)
			forests.push_back({ &pet.items, current_native_forest_role::pet });
	return current_owner_correspondence(cut.player_custody, owner, forests, native_uids);
}
} // namespace

unsigned int flatfile_shop_native_checkpoint_storage::read_current_player_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const economic_shop_checkpoint_projection &projection, int32_t pid,
	const std::string &account_name, int8_t racewar, const player_snapshot &original_queued_ack,
	const player_shop_checkpoint_stage &original_status,
	flatfile_shop_native_player_cut *output, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !output || pid <= 0 || racewar < 0 ||
	    !original_status.save_revision || !original_status.level ||
	    original_status.level > 255 || critical_operation_id_is_zero(projection.lineage) ||
	    critical_operation_id_is_zero(projection.epoch) ||
	    !economic_account_key_valid(projection.wallet) ||
	    !economic_account_key_valid(projection.bank) ||
	    projection.wallet.lineage.bytes != projection.lineage.bytes ||
	    projection.bank.lineage.bytes != projection.lineage.bytes ||
	    projection.wallet.kind != economic_account_kind::wallet ||
	    projection.wallet.context_id || projection.bank.kind != economic_account_kind::bank ||
	    projection.bank.context_id != static_cast<uint8_t>(racewar) ||
	    projection.wallet.authority_id == projection.bank.authority_id)
		return EINVAL;
	try
	{
		std::string account;
		if (!canonical_account(account_name, &account))
			return EINVAL;
		std::vector<uint8_t> validated_ack;
		auto code =
			codec_result(player_snapshot_encode(original_queued_ack, &validated_ack));
		if (code)
			return code;
		std::vector<uint8_t> original_items;
		code = checkpoint_body(original_queued_ack, pid, racewar, original_status,
				       &original_items);
		if (code)
			return code;

		flatfile_shop_native_player_cut cut;
		const flatfile_economic_mapping_request mappings[] = {
			{ projection.wallet, { 1, static_cast<uint64_t>(pid), {} } },
			{ projection.bank, { 2, projection.bank.authority_id, account } }
		};
		code = economic_flatfile_read_current_authority_locked(root, lock,
								       projection.lineage,
								       projection.epoch, mappings,
								       &cut.authority, error);
		if (code)
			return code;
		const auto money = flatfile_player_domain_read_current_locked(
			root, lock, pid, account, racewar, &cut.native_money, error);
		if (money != flatfile_player_domain_result::ok)
			return money == flatfile_player_domain_result::io_error	 ? EIO :
			       money == flatfile_player_domain_result::not_found ? ENOENT :
			       money == flatfile_player_domain_result::conflict	 ? ESTALE :
										   EILSEQ;
		// The original immutable file reader takes no player lock; acquiring one
		// here would reverse the actual snapshot writer's lock order.
		const auto player = flatfile_player_snapshot_read(root, pid, &cut.player, error);
		if (player != flatfile_player_load_result::ok)
			return player == flatfile_player_load_result::io_error	? EIO :
			       player == flatfile_player_load_result::not_found ? ENOENT :
										  EILSEQ;
		std::vector<uint8_t> current_items;
		code = checkpoint_body(cut.player, pid, racewar, original_status, &current_items);
		if (code)
			return code;
		if (current_items != original_items)
			return ESTALE;
		const item_owner_identity owner = { item_owner_type::player,
						    static_cast<uint64_t>(pid), 0 };
		const auto custody = flatfile_item_repository_load_owner_locked(
			root, lock, owner, &cut.player_owner_revision, &cut.player_custody, error);
		if (custody != flatfile_item_repository_result::ok)
			return custody == flatfile_item_repository_result::io_error  ? EIO :
			       custody == flatfile_item_repository_result::not_found ? ENOENT :
										       EILSEQ;
		std::unordered_set<uint64_t> native_uids;
		code = current_player_owner_correspondence(cut, native_uids);
		if (code)
			return code;
		std::unordered_set<uint64_t> pet_uids;
		cut.pet_custody.reserve(std::count_if(cut.player.pets.begin(),
						      cut.player.pets.end(), [](const auto &pet)
						      { return pet.pet_uid != 0; }));
		for (size_t index = 0; index < cut.player.pets.size(); ++index)
		{
			const auto &pet = cut.player.pets[index];
			if (!pet.pet_uid)
				continue;
			if (!pet_uids.insert(pet.pet_uid).second)
				return EILSEQ;
			flatfile_shop_native_pet_cut pet_cut;
			pet_cut.native_pet_index = index;
			pet_cut.pet_uid = pet.pet_uid;
			const item_owner_identity pet_owner = { item_owner_type::pet, pet.pet_uid,
								static_cast<uint64_t>(pid) };
			const auto pet_read = flatfile_item_repository_load_owner_locked(
				root, lock, pet_owner, &pet_cut.owner_revision, &pet_cut.custody,
				error);
			if (pet_read != flatfile_item_repository_result::ok)
				return pet_read == flatfile_item_repository_result::io_error ?
					       EIO :
				       pet_read == flatfile_item_repository_result::not_found ?
					       ENOENT :
					       EILSEQ;
			code = current_owner_correspondence(
				pet_cut.custody, pet_owner,
				{ { &pet.items, current_native_forest_role::pet } }, native_uids);
			if (code)
				return code;
			cut.pet_custody.push_back(std::move(pet_cut));
		}
		// Preserve the full file and all actual player/pet custody/clocks.
		// Stored-source correspondence never proves fresh runtime pet/NORENT
		// equality; that remains the original held native source owner's work.
		*output = std::move(cut);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
