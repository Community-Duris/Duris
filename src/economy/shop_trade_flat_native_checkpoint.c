#include "economy/shop_trade_transaction.h"
#include "economy/shop_trade_world_witness.h"
#include "economy/shop.h"
#include "account/account.h"
#include "core/prototypes.h"
#include "core/files.h"
#include "core/utils.h"
#include "world/world_singletons.h"
#include "persistence/persistence_mode.h"
#include "player/player_save_execution_guard.h"
#include "player/player_save_replay_ownership.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/pet_restore_runtime.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shopkeeper_ownership.h"
#include "persistence/shop_item_runtime_payload.h"
#include "flatfile/flatfile_accounting_shop_transaction.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <strings.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;
extern P_index mob_index;
extern int top_of_mobt;
extern struct shop_data *shop_index;
extern int number_of_shops;

namespace
{
#ifdef __NO_MYSQL__
// Reuse the original SHOP world observer's global observation bound.
constexpr size_t original_world_observation_limit = 1000000;

bool same_items(const std::vector<player_item_snapshot> &a,
		const std::vector<player_item_snapshot> &b)
{
	std::vector<uint8_t> x, y;
	return player_item_snapshot_list_encode(a, &x) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(b, &y) == player_snapshot_codec_result::ok &&
	       x == y;
}
bool same_snapshot(const player_snapshot &a, const player_snapshot &b)
{
	std::vector<uint8_t> x, y;
	return player_snapshot_encode(a, &x) == player_snapshot_codec_result::ok &&
	       player_snapshot_encode(b, &y) == player_snapshot_codec_result::ok && x == y;
}
bool same_mapping(const economic_shop_checkpoint_projection &a,
		  const economic_shop_checkpoint_projection &b)
{
	return a.lineage.bytes == b.lineage.bytes && a.epoch.bytes == b.epoch.bytes &&
	       economic_account_key_equal(a.wallet, b.wallet) &&
	       economic_account_key_equal(a.bank, b.bank);
}
bool same_authority(const flatfile_economic_authority_snapshot &a,
		    const flatfile_economic_authority_snapshot &b)
{
	if (a.lineage.bytes != b.lineage.bytes || a.epoch.bytes != b.epoch.bytes ||
	    a.lineage_revision != b.lineage_revision || a.mappings.size() != b.mappings.size())
		return false;
	for (size_t i = 0; i < a.mappings.size(); ++i)
	{
		const auto &x = a.mappings[i], &y = b.mappings[i];
		if (!economic_account_key_equal(x.account, y.account) ||
		    x.locator.kind != y.locator.kind ||
		    x.locator.native_id != y.locator.native_id ||
		    x.locator.name != y.locator.name ||
		    x.creating_operation.bytes != y.creating_operation.bytes ||
		    x.retiring_operation.bytes != y.retiring_operation.bytes ||
		    x.last_operation.bytes != y.last_operation.bytes || x.revision != y.revision)
			return false;
	}
	return true;
}
bool same_money(const flatfile_player_domain_record &a, const flatfile_player_domain_record &b)
{
	const auto &x = a.domains, &y = b.domains;
	return a.pid == b.pid && a.account_name == b.account_name && a.racewar == b.racewar &&
	       x.wallet_revision == y.wallet_revision && x.bank_revision == y.bank_revision &&
	       x.epic_revision == y.epic_revision && x.frag_revision == y.frag_revision &&
	       x.base_stat_revision == y.base_stat_revision && x.wallet == y.wallet &&
	       x.bank == y.bank && x.base_stats == y.base_stats && x.epics == y.epics &&
	       x.frags == y.frags && x.old_frags == y.old_frags &&
	       a.recent_pvp_deaths == b.recent_pvp_deaths &&
	       a.completed_epic_zones == b.completed_epic_zones;
}
bool runtime_money_supported(P_char actor, const flatfile_player_domain_record &source)
{
	if (!actor || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) != source.pid ||
	    GET_RACEWAR(actor) != source.racewar || !get_account_name_safe(actor) ||
	    strcasecmp(get_account_name_safe(actor), source.account_name.c_str()) ||
	    actor->only.pc->wallet_revision != source.domains.wallet_revision ||
	    actor->only.pc->bank_revision != source.domains.bank_revision)
		return false;
	const int wallet[] = { GET_COPPER(actor), GET_SILVER(actor), GET_GOLD(actor),
			       GET_PLATINUM(actor) };
	// This mirror is published by the original shared account/racewar bank
	// owner. The independently read initialized native bank remains authority.
	const int bank[] = { actor->only.pc->spare1, actor->only.pc->spare2, actor->only.pc->spare3,
			     actor->only.pc->spare4 };
	for (size_t i = 0; i < 4; ++i)
		if (wallet[i] < 0 || bank[i] < 0 ||
		    static_cast<uint64_t>(wallet[i]) != source.domains.wallet[i] ||
		    static_cast<uint64_t>(bank[i]) != source.domains.bank[i])
			return false;
	return true;
}

bool same_custody(const std::vector<flatfile_item_ownership_record> &a,
		  const std::vector<flatfile_item_ownership_record> &b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); ++i)
	{
		const auto &x = a[i], &y = b[i];
		if (x.item_uid != y.item_uid || x.root_item_uid != y.root_item_uid ||
		    x.parent_item_uid != y.parent_item_uid ||
		    !item_owner_identity_equal(x.owner, y.owner) ||
		    x.item_revision != y.item_revision || x.vnum != y.vnum || x.state != y.state ||
		    x.coin_payload != y.coin_payload || x.equipment_slot != y.equipment_slot)
			return false;
	}
	return true;
}
bool same_player_cut(const flatfile_shop_native_player_cut &a,
		     const flatfile_shop_native_player_cut &b)
{
	if (!same_authority(a.authority, b.authority) ||
	    !same_money(a.native_money, b.native_money) || !same_snapshot(a.player, b.player) ||
	    a.player_owner_revision != b.player_owner_revision ||
	    !same_custody(a.player_custody, b.player_custody) ||
	    a.pet_custody.size() != b.pet_custody.size())
		return false;
	for (size_t i = 0; i < a.pet_custody.size(); ++i)
	{
		const auto &x = a.pet_custody[i], &y = b.pet_custody[i];
		if (x.native_pet_index != y.native_pet_index || x.pet_uid != y.pet_uid ||
		    x.owner_revision != y.owner_revision || !same_custody(x.custody, y.custody))
			return false;
	}
	return true;
}
bool current_cache_supported(const item_owner_identity &owner, uint64_t revision,
			     const std::vector<flatfile_item_ownership_record> &rows)
{
	// Cache observations never supply missing durable authority. Existing foreign
	// or progressed cache claims must not be overwritten by an old native cut.
	uint64_t cached = 0;
	if (item_ownership_runtime_peek_owner_revision(owner, &cached) && cached != revision)
		return false;
	std::unordered_set<uint64_t> current_uids;
	for (const auto &row : rows)
		if (!current_uids.insert(row.item_uid).second)
			return false;
	std::vector<item_ownership_runtime_entry> cached_rows;
	if (!item_ownership_runtime_snapshot_owner(owner, item_ownership_runtime_size(),
						   &cached_rows))
		return false;
	for (const auto &row : cached_rows)
		if (row.state == item_custody_state::active && !current_uids.count(row.item_uid))
			return false;
	for (const auto &row : rows)
	{
		item_ownership_runtime_entry observed{};
		if (item_ownership_runtime_lookup(row.item_uid, &observed) &&
		    (observed.item_uid != row.item_uid ||
		     observed.root_item_uid != row.root_item_uid ||
		     observed.parent_item_uid != row.parent_item_uid ||
		     !item_owner_identity_equal(observed.owner, owner) ||
		     observed.item_revision != row.item_revision ||
		     observed.owner_revision != revision || observed.vnum != row.vnum ||
		     observed.state != row.state))
			return false;
	}
	return true;
}

bool exact_held_cut(const player_flat_shop_checkpoint_cut &cut, int32_t pid,
		    const critical_operation_id &operation)
{
	// The installed original hold stays owned after this brief exact observation.
	// Holding its reserved ticket across holder methods would self-refuse.
	player_save_execution_guard::held_publication_reservation observed(
		cut.ownership_epoch, pid, operation, cut.execution_hold_generation);
	return observed.matches_pid(pid);
}

struct original_physical_source
{
	std::unordered_set<P_obj> objects;
	std::unordered_map<uint64_t, size_t> uids;
	std::unordered_set<P_char> bodies;
	std::unordered_map<P_obj, size_t> links;
	std::vector<flatfile_item_ownership_record> durable_catalog;
	std::unordered_map<uint64_t, const flatfile_item_ownership_record *> durable_by_uid;
	bool read_locked(const std::string &root, const flatfile_authority_lock &lock,
			 std::string *error)
	{
		if (flatfile_item_repository_recovery_catalog_locked(root, lock, &durable_catalog,
								     error) !=
		    flatfile_item_repository_result::ok)
			return false;
		durable_by_uid.reserve(durable_catalog.size());
		for (const auto &row : durable_catalog)
			if (!durable_by_uid.emplace(row.item_uid, &row).second)
				return false;
		return true;
	}
	size_t visits = 0;
	bool charge() { return ++visits <= original_world_observation_limit; }
	bool body(P_char value)
	{
		return !value || !bodies.insert(value).second ||
		       (charge() &&
			(IS_NPC(value) ? value->only.npc != nullptr : value->only.pc != nullptr));
	}
	bool link(P_obj value)
	{
		if (!objects.count(value) || !charge())
			return false;
		++links[value];
		return true;
	}
	bool chain_links(P_obj head)
	{
		std::unordered_set<P_obj> seen;
		for (P_obj obj = head; obj; obj = obj->next_content)
			if (!seen.insert(obj).second || !link(obj))
				return false;
		return true;
	}
	bool scan()
	{
		P_obj previous = nullptr;
		for (P_obj obj = object_list; obj; obj = obj->next)
		{
			if (!charge() || obj->prev != previous || !objects.insert(obj).second)
				return false;
			if (obj->obj_uid)
				++uids[obj->obj_uid];
			previous = obj;
		}
		std::unordered_set<P_char> chain;
		for (P_char ch = character_list; ch; ch = ch->next)
			if (!chain.insert(ch).second || !body(ch))
				return false;
		if (!world || top_of_world < 0)
			return false;
		for (int room = 0; room <= top_of_world; ++room)
		{
			if (!charge())
				return false;
			chain.clear();
			for (P_char ch = world[room].people; ch; ch = ch->next_in_room)
				if (!chain.insert(ch).second || !body(ch))
					return false;
		}
		std::unordered_set<P_desc> descriptors;
		for (P_desc d = descriptor_list; d; d = d->next)
			if (!charge() || !descriptors.insert(d).second || !body(d->character) ||
			    !body(d->original))
				return false;
		// Match the original world's switched/original body closure.
		std::vector<P_char> pending(bodies.begin(), bodies.end());
		for (size_t i = 0; i < pending.size(); ++i)
		{
			const P_char ch = pending[i];
			const P_char extra[] = { IS_NPC(ch) ? ch->only.npc->orig_char : nullptr,
						 GET_PLYR(ch) };
			for (P_char value : extra)
			{
				const bool known = !value || bodies.count(value);
				if (!body(value))
					return false;
				if (!known)
					pending.push_back(value);
			}
		}
		for (P_obj obj : objects)
			if (!chain_links(obj->contains))
				return false;
		for (int room = 0; room <= top_of_world; ++room)
			if (!chain_links(world[room].contents))
				return false;
		for (P_char ch : bodies)
		{
			if (!chain_links(ch->carrying))
				return false;
			for (int slot = 0; slot < MAX_WEAR; ++slot)
				if (ch->equipment[slot] && !link(ch->equipment[slot]))
					return false;
		}
		return true;
	}
	bool pet_identity(P_char actual, uint64_t uid) const
	{
		if (actual && (!bodies.count(actual) || !actual->runtime_id ||
			       find_character_by_runtime_id(actual->runtime_id) != actual))
			return false;
		if (!uid)
			return actual != nullptr;
		size_t count = 0;
		for (P_char ch : bodies)
			if (IS_NPC(ch) && ch->durable_pet_uid == uid)
			{
				if (ch != actual)
					return false;
				++count;
			}
		return actual ? count == 1 : count == 0;
	}
	bool tree(P_obj obj, P_char owner, P_obj parent, int slot, size_t depth,
		  bool omitted_parent, const std::unordered_set<uint64_t> &saved,
		  std::unordered_set<P_obj> &seen)
	{
		if (!obj || depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
		    seen.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS || !objects.count(obj) ||
		    links[obj] != 1 || !seen.insert(obj).second ||
		    (obj->obj_uid && uids.at(obj->obj_uid) != 1))
			return false;
		if (parent ? obj->loc_p != LOC_INSIDE || obj->loc.inside != parent :
			     (slot ? obj->loc_p != LOC_WORN || obj->loc.wearing != owner ||
					      owner->equipment[slot - 1] != obj ||
					      obj->next_content :
				     obj->loc_p != LOC_CARRIED || obj->loc.carrying != owner))
			return false;
		item_ownership_runtime_entry cache{};
		const bool cached = obj->obj_uid &&
				    item_ownership_runtime_lookup(obj->obj_uid, &cache);
		const bool omitted =
			omitted_parent ||
			(!saved.count(obj->obj_uid) && IS_SET(obj->extra_flags, ITEM_NORENT) &&
			 (!cached || cache.state != item_custody_state::active));
		if (saved.count(obj->obj_uid))
		{
			if (omitted || !obj->obj_uid || obj->obj_uid == UINT64_MAX)
				return false;
		}
		else
		{
			if (!omitted)
				return false;
			if (obj->obj_uid)
			{
				const auto found = durable_by_uid.find(obj->obj_uid);
				if (found != durable_by_uid.end() &&
				    found->second->state == item_custody_state::active)
					return false;
			}
		}
		for (P_obj child = obj->contains; child; child = child->next_content)
			if (!tree(child, owner, obj, 0, depth + 1, omitted, saved, seen))
				return false;
		return true;
	}
	bool forest(P_char owner, const std::vector<player_item_snapshot> &items)
	{
		std::unordered_set<uint64_t> saved;
		for (const auto &row : items)
			if (!saved.insert(row.object_uid).second)
				return false;
		std::unordered_set<P_obj> seen;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (owner->equipment[slot] && !tree(owner->equipment[slot], owner, nullptr,
							    slot + 1, 1, false, saved, seen))
				return false;
		for (P_obj obj = owner->carrying; obj; obj = obj->next_content)
			if (!tree(obj, owner, nullptr, 0, 1, false, saved, seen))
				return false;
		return true;
	}
};

bool configured_keeper_supported(P_char keeper, uint32_t shop, int64_t original_cash,
				 const original_physical_source &physical)
{
	if (!keeper || !IS_NPC(keeper) || !keeper->only.npc || GET_MASTER(keeper) || !shop_index ||
	    number_of_shops <= 0 || shop >= static_cast<uint32_t>(number_of_shops) ||
	    shop_index[shop].keeper < 0 || shop_index[shop].keeper > top_of_mobt ||
	    GET_RNUM(keeper) != shop_index[shop].keeper || keeper->in_room < 0 ||
	    keeper->in_room > top_of_world || singleton_shop_id(keeper) != static_cast<int>(shop))
		return false;
	const int home = real_room(shop_index[shop].in_room);
	if (!shop_index[shop].shop_is_roaming && (home < 0 || home > top_of_world))
		return false;
	size_t candidates = 0;
	for (P_char body : physical.bodies)
	{
		if (!IS_NPC(body) || !body->only.npc || GET_MASTER(body) ||
		    GET_RNUM(body) != shop_index[shop].keeper || body->in_room < 0 ||
		    body->in_room > top_of_world)
			continue;
		// Original dirty-save binding/roaming/home selector, observed only.
		const bool matches =
			body->only.npc->shopkeeper_shop_id >= 0 ?
				body->only.npc->shopkeeper_shop_id == static_cast<int>(shop) :
				(shop_index[shop].shop_is_roaming ?
					 singleton_shop_id(body) == static_cast<int>(shop) :
					 body->in_room == home);
		if (matches)
		{
			if (body != keeper)
				return false;
			++candidates;
		}
	}
	const int64_t cash = static_cast<int64_t>(GET_COPPER(keeper)) + 10LL * GET_SILVER(keeper) +
			     100LL * GET_GOLD(keeper) + 1000LL * GET_PLATINUM(keeper);
	return candidates == 1 && GET_COPPER(keeper) >= 0 && GET_SILVER(keeper) >= 0 &&
	       GET_GOLD(keeper) >= 0 && GET_PLATINUM(keeper) >= 0 && cash >= 0 && cash <= INT_MAX &&
	       cash == original_cash;
}

bool live_pet_items_supported(P_char actor, const player_snapshot &native,
			      original_physical_source &physical)
{
	player_snapshot live{};
	if (player_snapshot_capture(actor, native.revision, PLAYER_COMPONENT_PETS, RENT_CRASH,
				    NOWHERE, &live) != player_snapshot_capture_result::ok ||
	    live.pets.size() != native.pets.size())
		return false;
	std::vector<P_char> active;
	std::unordered_set<const follow_type *> followers;
	std::unordered_set<P_char> seen_pets;
	for (const follow_type *follow = actor->followers; follow; follow = follow->next)
	{
		if (!followers.insert(follow).second)
			return false;
		P_char pet = follow->follower;
		if (!pet || !IS_NPC(pet) || pet->in_room != actor->in_room ||
		    GET_MASTER(pet) != actor)
			continue;
		if (!seen_pets.insert(pet).second)
			return false;
		active.push_back(pet);
	}
	const size_t held = actor->only.pc->held_pets ? actor->only.pc->held_pets->pets.size() : 0;
	if (held > live.pets.size() || active.size() != live.pets.size() - held)
		return false;
	std::vector<bool> consumed(native.pets.size(), false);
	for (size_t i = 0; i < live.pets.size(); ++i)
	{
		const auto &current = live.pets[i];
		size_t selected = native.pets.size(), matches = 0;
		for (size_t j = 0; j < native.pets.size(); ++j)
		{
			const auto &stored = native.pets[j];
			if (stored.pet_uid == current.pet_uid &&
			    stored.mob_vnum == current.mob_vnum &&
			    stored.hold_reason == current.hold_reason &&
			    same_items(stored.items, current.items))
			{
				selected = j;
				++matches;
			}
		}
		// UID-zero legacy association is allowed only when the genuine body/
		// held source has one unambiguous persisted item forest and native vnum.
		if (matches != 1 || consumed[selected])
			return false;
		consumed[selected] = true;
		if (i < held)
		{
			if (current.pet_uid && !physical.pet_identity(nullptr, current.pet_uid))
				return false;
			for (const auto &item : current.items)
				if (physical.uids.count(item.object_uid))
					return false;
		}
		else
		{
			P_char pet = active[i - held];
			if (!pet->only.npc || GET_RNUM(pet) < 0 || GET_RNUM(pet) > top_of_mobt ||
			    !mob_index ||
			    mob_index[GET_RNUM(pet)].virtual_number != current.mob_vnum ||
			    pet->durable_pet_uid != current.pet_uid ||
			    !physical.pet_identity(pet, current.pet_uid) ||
			    !physical.forest(pet, current.items))
				return false;
		}
	}
	return true;
}

// The original source-checkpoint caller retains its exact full native cut.
// Publication supplies the independently authenticated CURRENT full player file.
bool live_pet_items_supported(P_char actor, const flatfile_shop_native_player_cut &cut,
			      original_physical_source &physical)
{
	return live_pet_items_supported(actor, cut.player, physical);
}

bool current_cut_cache_supported(const shop_trade_flat_native_checkpoint_stage &stage)
{
	if (!current_cache_supported(
		    { item_owner_type::player, static_cast<uint64_t>(stage.player.player.pid), 0 },
		    stage.player.player_owner_revision, stage.player.player_custody) ||
	    !current_cache_supported(flatfile_shopkeeper_item_owner(stage.keeper_after.shop_id),
				     stage.keeper_owner_revision, stage.keeper_custody))
		return false;
	for (const auto &pet : stage.player.pet_custody)
		if (!current_cache_supported({ item_owner_type::pet, pet.pet_uid,
					       static_cast<uint64_t>(stage.player.player.pid) },
					     pet.owner_revision, pet.custody))
			return false;
	return true;
}

bool hydrate_current_cut(const shop_trade_flat_native_checkpoint_stage &stage)
{
	const item_owner_identity player = { item_owner_type::player,
					     static_cast<uint64_t>(stage.player.player.pid), 0 };
	const item_owner_identity keeper =
		flatfile_shopkeeper_item_owner(stage.keeper_after.shop_id);
	std::vector<item_ownership_runtime_entry> rows;
	auto append =
		[&](const std::vector<flatfile_item_ownership_record> &source, uint64_t revision)
	{
		for (const auto &row : source)
			rows.push_back({ row.item_uid, row.root_item_uid, row.parent_item_uid,
					 row.owner, row.item_revision, revision, row.vnum,
					 row.state });
	};
	if (!current_cache_supported(player, stage.player.player_owner_revision,
				     stage.player.player_custody) ||
	    !current_cache_supported(keeper, stage.keeper_owner_revision, stage.keeper_custody))
		return false;
	append(stage.player.player_custody, stage.player.player_owner_revision);
	append(stage.keeper_custody, stage.keeper_owner_revision);
	for (const auto &pet : stage.player.pet_custody)
	{
		const item_owner_identity owner = { item_owner_type::pet, pet.pet_uid,
						    static_cast<uint64_t>(
							    stage.player.player.pid) };
		if (!current_cache_supported(owner, pet.owner_revision, pet.custody))
			return false;
		append(pet.custody, pet.owner_revision);
	}
	if (!item_ownership_runtime_hydrate_many_atomic(rows.data(), rows.size()) ||
	    !item_ownership_runtime_hydrate_owner(player, stage.player.player_owner_revision) ||
	    !item_ownership_runtime_hydrate_owner(keeper, stage.keeper_owner_revision))
		return false;
	for (const auto &pet : stage.player.pet_custody)
		if (!item_ownership_runtime_hydrate_owner(
			    { item_owner_type::pet, pet.pet_uid,
			      static_cast<uint64_t>(stage.player.player.pid) },
			    pet.owner_revision))
			return false;
	return true;
}
#endif
} // namespace

shop_trade_preparation_state
shop_trade_native_checkpoint_owner::attempt_flat(const shop_trade_flat_preparation_token &token,
						 P_char actor, P_char keeper, P_obj selected,
						 P_obj stock, P_obj destination) noexcept
{
#ifndef __NO_MYSQL__
	// persistence_mode_configure refuses flatfile-primary with a SQL client.
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	return shop_trade_preparation_state::pending;
#else
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_flat() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    !persistence_mode_flatfile_root() || !*persistence_mode_flatfile_root())
		return shop_trade_preparation_state::pending;
	try
	{
		// Observe every active UID/root/parent claim, including foreign owners.
		// This cache observation never supplies missing durable source authority.
		const auto cache_links_supported =
			[](const shop_trade_flat_native_checkpoint_stage &stage)
		{
			if (!current_cut_cache_supported(stage))
				return false;
			std::unordered_map<uint64_t, item_ownership_runtime_entry> expected;
			const auto append =
				[&](const std::vector<flatfile_item_ownership_record> &rows,
				    uint64_t revision)
			{
				for (const auto &row : rows)
					if (!expected.emplace(row.item_uid,
							      item_ownership_runtime_entry{
								      row.item_uid,
								      row.root_item_uid,
								      row.parent_item_uid,
								      row.owner, row.item_revision,
								      revision, row.vnum,
								      row.state })
						     .second)
						return false;
				return true;
			};
			if (!append(stage.player.player_custody,
				    stage.player.player_owner_revision) ||
			    !append(stage.keeper_custody, stage.keeper_owner_revision))
				return false;
			for (const auto &pet : stage.player.pet_custody)
				if (!append(pet.custody, pet.owner_revision))
					return false;
			std::vector<uint64_t> uids;
			uids.reserve(expected.size());
			for (const auto &[uid, row] : expected)
			{
				(void)row;
				uids.push_back(uid);
			}
			std::sort(uids.begin(), uids.end());
			std::vector<item_ownership_runtime_entry> observed;
			if (!item_ownership_runtime_published_native_observer::snapshot_links(
				    uids, std::max<size_t>(1, item_ownership_runtime_size()),
				    &observed))
				return false;
			for (const auto &row : observed)
			{
				const auto found = expected.find(row.item_uid);
				if (found == expected.end())
					return false;
				const auto &source = found->second;
				if (row.root_item_uid != source.root_item_uid ||
				    row.parent_item_uid != source.parent_item_uid ||
				    !item_owner_identity_equal(row.owner, source.owner) ||
				    row.item_revision != source.item_revision ||
				    row.owner_revision != source.owner_revision ||
				    row.vnum != source.vnum || row.state != source.state)
					return false;
			}
			return true;
		};
		const shop_trade_flat_native_checkpoint_stage *retained = nullptr;
		if (shop_trade_preparation_owner::completed_native_checkpoint_flat(token,
										   &retained))
			return shop_trade_preparation_state::ready;
		shop_trade_flat_checkpoint_context context;
		auto phase = shop_trade_flat_native_checkpoint_phase::preparing;
		if (!shop_trade_preparation_owner::begin_native_checkpoint_flat(
			    token, actor, keeper, selected, stock, destination, &context, &retained,
			    &phase))
			return shop_trade_preparation_state::pending;
		const std::string root = retained ? retained->player_hold_cut.selected_root :
						    context.player_hold_cut.selected_root;
		if (root != persistence_mode_flatfile_root())
			return shop_trade_preparation_state::pending;
		flatfile_identity_lock identity_lock;
		flatfile_authority_lock authority;
		std::string error;
		if (!identity_lock.acquire(root, &error) || !authority.acquire(root, &error) ||
		    flatfile_player_domain_recover_locked(root, authority, &error) !=
			    flatfile_player_domain_result::ok)
			return shop_trade_preparation_state::pending;
		const int32_t pid = retained ? retained->player.player.pid :
					       static_cast<int32_t>(context.original.player_pid);
		const std::string account = retained ? retained->player.native_money.account_name :
						       context.original.account_name.data();
		const int8_t racewar = retained ? retained->player.native_money.racewar :
						  static_cast<int8_t>(context.original.racewar);
		flatfile_identity_record identity;
		if (flatfile_identity_lookup_pid_locked(root, identity_lock, authority, pid,
							&identity,
							&error) != flatfile_identity_result::ok ||
		    identity.pid != pid || !identity.active || identity.blocked ||
		    identity.racewar != racewar ||
		    strcasecmp(identity.account.c_str(), account.c_str()))
			return shop_trade_preparation_state::pending;
		// All journal recovery precedes the pure CURRENT cut below.
		const auto &mapping = retained ? retained->mapping : context.original.mapping;
		const auto &original_ack = retained ? retained->original_queued_ack :
						      context.original_queued_ack;
		const auto &status = retained ? retained->acknowledged_status :
						context.original.player;
		const auto &hold = retained ? retained->player_hold_cut : context.player_hold_cut;
		const critical_operation_id operation = retained ? retained->operation_id :
								   context.original.operation_id;
		if (!exact_held_cut(hold, pid, operation))
			return shop_trade_preparation_state::pending;
		flatfile_shop_native_player_cut current_player;
		if (flatfile_shop_native_checkpoint_storage::read_current_player_locked(
			    root, authority, mapping, pid, account, racewar, original_ack, status,
			    &current_player, &error))
			return shop_trade_preparation_state::pending;
		flatfile_authority_after_image current_catalog;
		if (flatfile_shopkeeper_source_checkpoint_storage::read_current_catalog_locked(
			    root, authority, &current_catalog, &error) !=
		    flatfile_shopkeeper_result::ok)
			return shop_trade_preparation_state::pending;
		const uint32_t shop = retained ? retained->keeper_before.shop_id :
						 context.original.shop_id;
		uint64_t keeper_revision = 0;
		std::vector<flatfile_item_ownership_record> keeper_custody;
		if (flatfile_item_repository_load_owner_locked(
			    root, authority, flatfile_shopkeeper_item_owner(shop), &keeper_revision,
			    &keeper_custody, &error) != flatfile_item_repository_result::ok)
			return shop_trade_preparation_state::pending;
		std::vector<flatfile_authority_operation> operations;
		if (!retained)
		{
			player_snapshot ack;
			player_shop_checkpoint_stage held_status{};
			economic_shop_checkpoint_projection held_mapping{};
			player_flat_shop_checkpoint_cut held_cut;
			if (!player_save_shop_checkpoint_owner::observe_held_flat(
				    context.player_token, actor, context.original.operation_id,
				    &ack, &held_status, &held_mapping, &held_cut) ||
			    !same_snapshot(ack, original_ack) ||
			    held_status.save_revision != status.save_revision ||
			    held_status.level != status.level ||
			    !same_mapping(mapping, held_mapping) ||
			    held_cut.selected_root != hold.selected_root ||
			    held_cut.ownership_epoch != hold.ownership_epoch ||
			    held_cut.execution_hold_generation != hold.execution_hold_generation ||
			    !actor || !keeper ||
			    actor->runtime_id != context.original.actor_runtime_id ||
			    keeper->runtime_id != context.original.keeper_runtime_id ||
			    GET_PID(actor) != pid || GET_RACEWAR(actor) != racewar ||
			    !get_account_name_safe(actor) ||
			    strcasecmp(get_account_name_safe(actor), account.c_str()) ||
			    !GET_NAME(actor) ||
			    strcasecmp(GET_NAME(actor), identity.name.c_str()) || !shop_index ||
			    number_of_shops <= 0 ||
			    shop >= static_cast<uint32_t>(number_of_shops) ||
			    GET_RNUM(keeper) != shop_index[shop].keeper ||
			    GET_VNUM(keeper) != context.original.keeper_vnum ||
			    (shop_index[shop].shop_is_roaming != 0) !=
				    (context.original.keeper_roaming != 0))
				return shop_trade_preparation_state::pending;
			std::array<uint64_t, 1> literal{ context.player_token.root_uid };
			std::vector<player_item_snapshot> detached_selection;
			// The private original preparation just proved this exact selected
			// pointer/UID/literal. A produced detached tree still needs the real
			// global census; a pointer or UID alone is not factory authority.
			if (selected && selected->loc_p == LOC_NOWHERE &&
			    player_item_snapshot_tree_capture_literal(selected, &detached_selection,
								      nullptr) !=
				    player_snapshot_capture_result::ok)
				return shop_trade_preparation_state::pending;
			shop_trade_world_expectation expected;
			expected.detached_items = detached_selection;
			expected.actor_pid = pid;
			expected.actor_runtime_id = context.original.actor_runtime_id;
			expected.keeper_runtime_id = context.original.keeper_runtime_id;
			expected.shop_id = shop;
			expected.keeper_vnum = context.original.keeper_vnum;
			expected.player_items = original_ack.items;
			expected.keeper_items = context.original.keeper_items;
			expected.literal_player_root_uids =
				literal[0] ? std::span<const uint64_t>(literal) :
					     std::span<const uint64_t>{};
			shop_trade_world_witness witness;
			original_physical_source physical;
			if (!shop_trade_world_witness_observe(expected, &witness) ||
			    witness.actor != actor || witness.keeper != keeper ||
			    !physical.read_locked(root, authority, &error) || !physical.scan() ||
			    !configured_keeper_supported(keeper, shop, context.original.keeper_cash,
							 physical) ||
			    !physical.forest(actor, original_ack.items) ||
			    !live_pet_items_supported(actor, current_player, physical) ||
			    !runtime_money_supported(actor, current_player.native_money))
				return shop_trade_preparation_state::pending;
			auto stage = std::make_unique<shop_trade_flat_native_checkpoint_stage>();
			stage->operation_id = context.original.operation_id;
			stage->player_token = context.player_token;
			stage->player_hold_cut = std::move(context.player_hold_cut);
			stage->mapping = mapping;
			stage->acknowledged_status = status;
			stage->original_queued_ack = std::move(context.original_queued_ack);
			stage->player = std::move(current_player);
			stage->keeper_owner_revision = keeper_revision;
			stage->keeper_custody = std::move(keeper_custody);
			stage->catalog_before = std::move(current_catalog);
			flatfile_shopkeeper_record selected_catalog;
			if (flatfile_shopkeeper_read_trade_after_image(stage->catalog_before, shop,
								       &selected_catalog, &error) !=
				    flatfile_shopkeeper_result::ok ||
			    flatfile_shopkeeper_source_checkpoint_storage::prepare_locked(
				    root, authority, shop, selected_catalog.revision,
				    context.original.keeper_vnum, context.original.keeper_cash,
				    context.original.keeper_roaming != 0,
				    context.original.keeper_items, stage->keeper_custody,
				    stage->keeper_owner_revision, &stage->keeper_before,
				    &stage->keeper_after, &stage->catalog_after,
				    &error) != flatfile_shopkeeper_result::ok)
				return shop_trade_preparation_state::pending;
			// Complete operation copies before consuming the irreversible marker.
			operations.push_back({ flatfile_authority_store::domains,
					       flatfile_authority_operation_kind::write,
					       stage->catalog_after.filename,
					       stage->catalog_after.bytes });
			retained = stage.get();
			if (!cache_links_supported(*stage) ||
			    !runtime_money_supported(actor, stage->player.native_money) ||
			    !shop_trade_preparation_owner::seal_native_checkpoint_flat(
				    token, std::move(stage)))
				return shop_trade_preparation_state::pending;
			phase = shop_trade_flat_native_checkpoint_phase::sealed;
		}
		else if (!same_player_cut(current_player, retained->player) ||
			 keeper_revision != retained->keeper_owner_revision ||
			 !same_custody(keeper_custody, retained->keeper_custody))
			return shop_trade_preparation_state::pending;
		if (phase == shop_trade_flat_native_checkpoint_phase::sealed)
		{
			if (operations.empty() &&
			    (current_catalog.filename != retained->catalog_before.filename ||
			     current_catalog.bytes != retained->catalog_before.bytes))
				return shop_trade_preparation_state::pending;
			if (operations.empty())
				operations.push_back({ flatfile_authority_store::domains,
						       flatfile_authority_operation_kind::write,
						       retained->catalog_after.filename,
						       retained->catalog_after.bytes });
			if (!cache_links_supported(*retained) ||
			    !runtime_money_supported(
				    find_character_by_runtime_id(
					    retained->player_token.actor_runtime_id),
				    retained->player.native_money) ||
			    !shop_trade_preparation_owner::start_native_checkpoint_flat(token))
				return shop_trade_preparation_state::pending;
			flatfile_authority_commit_outcome outcome =
				flatfile_authority_commit_outcome::publication_uncertain;
			const auto result =
				flatfile_authority_transaction_commit_operations_with_outcome(
					root, authority, operations, &error, &outcome);
			if (!shop_trade_preparation_owner::retain_native_outcome_flat(token, result,
										      outcome))
				return shop_trade_preparation_state::pending;
			phase = shop_trade_flat_native_checkpoint_phase::uncertain;
		}
		if (phase == shop_trade_flat_native_checkpoint_phase::unpublished_before)
			return shop_trade_preparation_state::pending;
		if (phase != shop_trade_flat_native_checkpoint_phase::attempt_started &&
		    phase != shop_trade_flat_native_checkpoint_phase::uncertain)
			return shop_trade_preparation_state::pending;
		// Read-only resolution of the original attempt. Never restage, increment
		// a new clock, reissue a journal write or release the original held leaf.
		if (flatfile_player_domain_recover_locked(root, authority, &error) !=
			    flatfile_player_domain_result::ok ||
		    flatfile_shop_native_checkpoint_storage::read_current_player_locked(
			    root, authority, retained->mapping, pid, account, racewar,
			    retained->original_queued_ack, retained->acknowledged_status,
			    &current_player, &error) ||
		    !same_player_cut(current_player, retained->player) ||
		    flatfile_item_repository_load_owner_locked(
			    root, authority, flatfile_shopkeeper_item_owner(shop), &keeper_revision,
			    &keeper_custody, &error) != flatfile_item_repository_result::ok ||
		    keeper_revision != retained->keeper_owner_revision ||
		    !same_custody(keeper_custody, retained->keeper_custody) ||
		    flatfile_shopkeeper_source_checkpoint_storage::read_current_catalog_locked(
			    root, authority, &current_catalog, &error) !=
			    flatfile_shopkeeper_result::ok ||
		    current_catalog.filename != retained->catalog_before.filename ||
		    !exact_held_cut(retained->player_hold_cut, pid, retained->operation_id))
			return shop_trade_preparation_state::pending;
		if (current_catalog.bytes == retained->catalog_after.bytes)
		{
			if (!cache_links_supported(*retained) || !hydrate_current_cut(*retained) ||
			    !exact_held_cut(retained->player_hold_cut, pid, retained->operation_id))
				return shop_trade_preparation_state::pending;
			const shop_trade_flat_native_checkpoint_resolution proof(
				retained, shop_trade_flat_native_checkpoint_resolution::cut::after);
			return shop_trade_preparation_owner::resolve_native_checkpoint_flat(token,
											    proof) ?
				       shop_trade_preparation_state::ready :
				       shop_trade_preparation_state::pending;
		}
		if (current_catalog.bytes == retained->catalog_before.bytes)
		{
			const shop_trade_flat_native_checkpoint_resolution proof(
				retained,
				shop_trade_flat_native_checkpoint_resolution::cut::before);
			(void)shop_trade_preparation_owner::resolve_native_checkpoint_flat(token,
											   proof);
		}
		return shop_trade_preparation_state::pending;
	}
	catch (...)
	{
		return shop_trade_preparation_state::pending;
	}
#endif
}

bool shop_trade_native_checkpoint_owner::observe_flat_publication_sources_locked(
	const std::string &root, const flatfile_authority_lock &lock, P_char actor, P_char keeper,
	uint32_t shop, const flatfile_accounting_shop_projection &current,
	const std::vector<player_item_snapshot> &physical_pc,
	const std::vector<player_item_snapshot> &physical_keeper,
	int64_t runtime_keeper_cash) noexcept
{
#ifndef __NO_MYSQL__
	(void)root;
	(void)lock;
	(void)actor;
	(void)keeper;
	(void)shop;
	(void)current;
	(void)physical_pc;
	(void)physical_keeper;
	(void)runtime_keeper_cash;
	return false;
#else
	if (!nevent_is_game_thread() || root.empty() || !lock.matches(root) || !actor ||
	    !IS_PC(actor) || !actor->only.pc || !keeper || !IS_NPC(keeper) || !keeper->only.npc ||
	    actor == keeper || GET_PID(actor) <= 0 || GET_PID(actor) != current.player.pid ||
	    current.player_domains.pid != current.player.pid || current.keeper.shop_id != shop)
		return false;
	try
	{
		// The sole caller holds the authentic command/receipt CURRENT cut and
		// the same root lock. These observations grant no mutation/ACK authority.
		original_physical_source physical;
		std::string error;
		return physical.read_locked(root, lock, &error) && physical.scan() &&
		       physical.bodies.count(actor) && physical.bodies.count(keeper) &&
		       configured_keeper_supported(keeper, shop, runtime_keeper_cash, physical) &&
		       physical.forest(actor, physical_pc) &&
		       physical.forest(keeper, physical_keeper) &&
		       live_pet_items_supported(actor, current.player, physical);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_native_checkpoint_owner::observe_flat_cold_publication_sources_locked(
	const std::string &root, const flatfile_authority_lock &lock, P_char actor, P_char keeper,
	uint32_t shop, const flatfile_accounting_shop_projection &current,
	const std::vector<player_item_snapshot> &physical_pc,
	const std::vector<player_item_snapshot> &physical_keeper,
	int64_t authenticated_stage_keeper_cash) noexcept
{
#ifndef __NO_MYSQL__
	(void)root;
	(void)lock;
	(void)actor;
	(void)keeper;
	(void)shop;
	(void)current;
	(void)physical_pc;
	(void)physical_keeper;
	(void)authenticated_stage_keeper_cash;
	return false;
#else
	if (!nevent_is_game_thread() || root.empty() || !lock.matches(root) ||
	    current.player.pid <= 0 || current.player_domains.pid != current.player.pid ||
	    current.keeper.shop_id != shop || !shop_index || !mob_index || number_of_shops <= 0 ||
	    shop >= static_cast<uint32_t>(number_of_shops) || shop_index[shop].keeper < 0 ||
	    shop_index[shop].keeper > top_of_mobt ||
	    mob_index[shop_index[shop].keeper].virtual_number != current.keeper.mob_vnum ||
	    static_cast<bool>(shop_index[shop].shop_is_roaming) != current.keeper.roaming ||
	    current.keeper.cash < 0 || current.keeper.cash > INT_MAX ||
	    authenticated_stage_keeper_cash < 0 || authenticated_stage_keeper_cash > INT_MAX)
		return false;
	try
	{
		// The caller authenticates the full original command/receipt CURRENT cut
		// and the original shop_world_cold_observe literal/selected/target witness.
		// The caller also authenticates original BEFORE or CURRENT keeper cash
		// from the actual retained cash-publication stage, as in the live helper.
		// This separate borrowed-lock source census cannot grant publication/ACK.
		original_physical_source physical;
		std::string error;
		if (!physical.read_locked(root, lock, &error) || !physical.scan())
			return false;
		const int home = real_room(shop_index[shop].in_room);
		if (!current.keeper.roaming && (home < 0 || home > top_of_world))
			return false;
		P_char actual_actor = nullptr, actual_keeper = nullptr;
		std::unordered_map<uint64_t, size_t> runtime_ids, pet_ids;
		for (P_char body : physical.bodies)
		{
			if (body->runtime_id)
				++runtime_ids[body->runtime_id];
			if (IS_PC(body) && GET_PID(body) == current.player.pid)
			{
				if (actual_actor)
					return false;
				actual_actor = body;
			}
			if (IS_NPC(body))
			{
				if (body->durable_pet_uid)
					++pet_ids[body->durable_pet_uid];
				if (body->only.npc->shopkeeper_shop_id >= 0 &&
				    static_cast<uint32_t>(body->only.npc->shopkeeper_shop_id) ==
					    shop)
				{
					if (actual_keeper)
						return false;
					actual_keeper = body;
				}
			}
		}
		if (actual_actor != actor || actual_keeper != keeper ||
		    (actor && (!actor->runtime_id || runtime_ids[actor->runtime_id] != 1)) ||
		    (keeper && (!keeper->runtime_id || runtime_ids[keeper->runtime_id] != 1)))
			return false;
		// Preserve the original cold binding rule. A same-template fallback with
		// no unambiguous other-shop binding is unresolved, never certified absent.
		for (P_char body : physical.bodies)
		{
			if (!IS_NPC(body) || GET_MASTER(body) ||
			    GET_RNUM(body) != shop_index[shop].keeper)
				continue;
			const int bound = body->only.npc->shopkeeper_shop_id;
			if (bound >= 0)
			{
				if (bound >= number_of_shops ||
				    shop_index[bound].keeper != GET_RNUM(body))
					return false;
				continue;
			}
			const int fallback = singleton_shop_id(body);
			if (fallback < 0 || fallback == static_cast<int>(shop))
				return false;
		}
		if (keeper && (!configured_keeper_supported(
				       keeper, shop, authenticated_stage_keeper_cash, physical) ||
			       !physical.forest(keeper, physical_keeper)))
			return false;
		if (actor)
		{
			// Descriptor/switch ownership and full literal PC properties are also
			// authenticated by the mandatory original cold world witness.
			return physical.forest(actor, physical_pc) &&
			       live_pet_items_supported(actor, current.player, physical);
		}
		// No PID body exists anywhere in the original body/descriptor closure.
		// Modern pet identity and every pet item must likewise be globally absent.
		// UID-zero legacy metadata has no durable body identity: item absence or
		// a matching VNUM/order cannot certify that its body is absent.
		std::unordered_set<uint64_t> expected_pet_ids;
		for (const auto &pet : current.player.pets)
		{
			if (!pet.pet_uid || !expected_pet_ids.insert(pet.pet_uid).second ||
			    pet_ids.count(pet.pet_uid))
				return false;
			for (const auto &item : pet.items)
				if (physical.uids.count(item.object_uid))
					return false;
		}
		// CURRENT PC items may include the authenticated selected transitional
		// tree; original cold world UID/location proof handles that exception.
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
