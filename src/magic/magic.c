/*
 * ***************************************************************************
 * *  File: magic.c                                            Part of Duris *
 * *  Usage: procedures to create spell affects
 * * *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 * * *  Copyright 1994 - 2008 - Duris Systems Ltd.
 * *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <new>
#include <time.h>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <tuple>
#include <vector>
#include "world/achievements.h"
#include "guild/alliances.h"
#include "guild/assocs.h"
#include "combat/ctf.h"
#include "combat/damage.h"
#include "core/defines.h"
#include "classes/disguise.h"
#include "world/graph.h"
#include "combat/grapple.h"
#include "world/hardcore_config.h"
#include "guild/guildhall.h"
#include "combat/justice.h"
#include "combat/training_dummy.h"
#include "world/map.h"
#include "core/mm.h"
#include "classes/necromancy.h"
#include "persistence/persistence_checkpoint.h"
#include "persistence/persistence_observability.h"
#include "item/objmisc.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "magic/spell_item_lifecycle.h"
#include "player/player_load_repository.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot.h"
#include "economy/economic_gameplay_authority.h"
#include <array>
#include "kingdom/kingdom_store_piece.h"
#include "world/outposts.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "magic/spell_words_of_power.h"
#include "sql/sql.h"
#include "world/vnum.obj.h"
#include "world/weather.h"
#include "core/safe_format.h"

/*
 * external variables
 */

extern const char *command[];
extern Skill skills[];
extern char *spells[];
extern P_index obj_index;
extern P_char character_list;
extern P_obj object_list;
extern P_desc descriptor_list;
extern P_char combat_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef anti_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];
extern const struct stat_data stat_factor[];
// extern int rev_dir[];
extern int avail_hometowns[][LAST_RACE + 1];
extern int guild_locations[][CLASS_COUNT + 1];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern int hometown[];
extern const int top_of_world;
extern struct str_app_type str_app[];
extern struct con_app_type con_app[];
extern struct time_info_data time_info;
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern struct sector_data *sector_table;
extern const struct race_names race_names_table[];
extern const char *carve_part_name[];
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern const struct golem_description golem_data[];
extern float exp_mods[EXPMOD_MAX + 1];
extern bool is_dragoon_mounted(P_char ch);
extern bool is_dragoon_mount(P_char mount);
extern bool is_in_dragoon_group(P_char ch, P_char vict);
extern int get_next_dragoon_circle(P_char ch);
extern P_char get_dragoon_mount(P_char ch);
extern void do_point(P_char ch, P_char victim);
extern bool has_skin_spell(P_char ch);

// THE NEXT PERSON THAT OUTRIGHT COPIES A SPELL JUST TO CHANGE THE NAME/MESSAGES
// IT OUTPUTS IS GOING TO BE CASTRATED BY ME AND FORCED TO EAT THEIR OWN GENITALIA.
// There is no reason to do this other than to make a headache for another coder.
// If you feel the need to have a "different" spell than one already in the game
// for racewar purposes or whatever, MAKE CHANGES TO THE ORIGINAL SPELL and call
// THAT SPELL with a command in interp.c.  There is no reason to have 3253232 different
// functions for the exact same spell(transmute/ethereal grounds etc.) - Jexni 3/28/11

void affect_to_end(P_char ch, struct affected_type *af);

void do_nothing_spell(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	return;
}

// New function for spell components - Lucrot 31Aug2008
int get_spell_component(P_char ch, int vnum, int max_components)
{
	P_obj t_obj, next_obj;
	int found = 0;
	if (economic_gameplay_authority::active() && ch && IS_PC(ch))
		return 0;

	for (t_obj = ch->carrying; t_obj && found < max_components; t_obj = next_obj)
	{
		next_obj = t_obj->next_content;
		if (obj_index[t_obj->R_num].virtual_number == vnum)
		{
			extract_obj(t_obj, TRUE); // Spell components shouldn't be artis.
			found++;
		}
	}
	return found;
}

namespace
{
struct spell_component_retirement_context
{
	item_spell_component_effect effect = {};
	std::array<uint64_t, 8> item_uids = {};
	uint32_t continuation_context_size = 0;
	uint8_t item_count = 0;
	std::array<uint8_t, 48> continuation_context = {};
};

static_assert(sizeof(spell_component_retirement_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

enum class spell_component_retirement_stage : uint8_t
{
	items_pending,
	items_retired,
	effect_pending,
};

struct spell_component_retirement_state
{
	spell_component_retirement_stage stage = spell_component_retirement_stage::items_pending;
	uint32_t actor_pid = 0;
	uint32_t owner_pid = 0;
	item_spell_component_effect effect = item_spell_component_effect::faerie_sight;
	bool effect_applied = false;
	bool save_requested = false;
	bool save_acknowledged = false;
	uint64_t next_save_request_at_usec = 0;
};

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
// New data-free owner of the original unordered_map's actual table. No existing
// map is cast or layout-read. All selected ordinary algorithms remain inherited.
class spell_retirement_native_table final
	: public std::__umap_hashtable<std::string, spell_component_retirement_state>
{
	using table_type = std::__umap_hashtable<std::string, spell_component_retirement_state>;
	using original_type = std::unordered_map<std::string, spell_component_retirement_state>;
	using actual_node = std::__detail::_Hash_node<
		typename table_type::value_type,
		std::__cache_default<std::string, std::hash<std::string>>::value>;

    public:
	using table_type::table_type;
	using table_type::operator=;
	using table_type::insert;
	template <class... Args>
	std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args)
	{
		return table_type::try_emplace(this->cend(), key, std::forward<Args>(args)...);
	}
	template <class... Args>
	std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args)
	{
		return table_type::try_emplace(this->cend(), std::move(key),
					       std::forward<Args>(args)...);
	}

	// Exact original unordered_map node-handle forwarding used by selected
	// coin-publication refusal recovery; retain inherited value insert overloads.
	insert_return_type insert(node_type &&node)
	{
		return this->_M_reinsert_node(std::move(node));
	}
	bool current_table_heap_bytes(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t bytes = 0;
		const size_t buckets = this->bucket_count();
		if (!buckets ||
		    (buckets > 1 && buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *)))
			return false;
		if (buckets > 1)
			bytes = buckets * sizeof(std::__detail::_Hash_node_base *);
		if (this->size() > (SIZE_MAX - bytes) / sizeof(actual_node))
			return false;
		bytes += this->size() * sizeof(actual_node);
		for (const auto &entry : *this)
			if (entry.first.capacity() > 15)
			{
				if (entry.first.capacity() == SIZE_MAX ||
				    entry.first.capacity() + 1 > SIZE_MAX - bytes)
					return false;
				bytes += entry.first.capacity() + 1;
			}
		*output = bytes;
		return true;
	}
	bool next_insert_extra_peak(const std::string &key, size_t *output) const noexcept
	{
		if (!output || this->size() == this->max_size() || key.size() == SIZE_MAX)
			return false;
		const size_t text = key.size() > 15 ? key.size() + 1 : 0;
		if (text > SIZE_MAX - sizeof(actual_node))
			return false;
		size_t extra = sizeof(actual_node) + text;
		// These objects are admitted by the owning caller before this pure profile
		// call. Copy the actual CURRENT original policy, never guess a threshold.
		auto policy = this->__rehash_policy();
		const auto next = policy._M_need_rehash(this->bucket_count(), this->size(), 1);
		if (next.first && next.second > 1)
		{
			if (next.second >
			    (SIZE_MAX - extra) / sizeof(std::__detail::_Hash_node_base *))
				return false;
			extra += next.second * sizeof(std::__detail::_Hash_node_base *);
		}
		*output = extra;
		return true;
	}
};
static_assert(sizeof(spell_retirement_native_table) ==
	      sizeof(std::unordered_map<std::string, spell_component_retirement_state>));
static_assert(alignof(spell_retirement_native_table) ==
	      alignof(std::unordered_map<std::string, spell_component_retirement_state>));
static_assert(
	std::is_same_v<spell_retirement_native_table::iterator,
		       std::unordered_map<std::string, spell_component_retirement_state>::iterator>);
static_assert(std::is_same_v<
	      spell_retirement_native_table::allocator_type,
	      std::unordered_map<std::string, spell_component_retirement_state>::allocator_type>);
using spell_retirement_table = spell_retirement_native_table;
#else
using spell_retirement_table = std::unordered_map<std::string, spell_component_retirement_state>;
#endif
spell_retirement_table spell_component_retired_items;

std::string spell_component_operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

spell_component_effect_completion_fn
spell_component_effect_callback(item_spell_component_effect effect)
{
	switch (effect)
	{
	case item_spell_component_effect::faerie_sight:
		return spell_faerie_sight_component_completed;
	case item_spell_component_effect::spore_burst_initial:
		return spell_spore_burst_initial_components_completed;
	case item_spell_component_effect::spore_burst_repeat:
		return spell_spore_burst_repeat_components_completed;
	case item_spell_component_effect::summon_insects:
		return spell_summon_insects_component_completed;
	case item_spell_component_effect::wall_of_bones:
		return spell_wall_of_bones_scales_completed;
	case item_spell_component_effect::vines:
		return spell_vines_component_retirement_completed;
	}
	return nullptr;
}

P_obj spell_component_by_uid(uint64_t item_uid)
{
	P_obj found = NULL;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
		{
			if (found)
				return NULL;
			found = object;
		}
	return found;
}

bool spell_component_retirement_published(const critical_operation_id &operation_id, P_char actor,
					  bool committed, const item_transfer_result &result,
					  unsigned int error_code, const uint8_t *encoded,
					  size_t encoded_size)
{
	if (!actor || !encoded || encoded_size != sizeof(spell_component_retirement_context))
		return false;
	spell_component_retirement_context context = {};
	memcpy(&context, encoded, sizeof(context));
	if (context.effect < item_spell_component_effect::faerie_sight ||
	    context.effect > item_spell_component_effect::vines || !context.item_count ||
	    context.item_count > context.item_uids.size() ||
	    context.continuation_context_size > context.continuation_context.size())
		return false;
	std::string operation_key;
	if (committed)
	{
		try
		{
			operation_key = spell_component_operation_key(operation_id);
			auto [state, inserted] =
				spell_component_retired_items.try_emplace(operation_key);
			if (inserted)
			{
				state->second.actor_pid = static_cast<uint32_t>(GET_PID(actor));
				state->second.owner_pid = state->second.actor_pid;
				state->second.effect = context.effect;
			}
			if (state->second.actor_pid != static_cast<uint32_t>(GET_PID(actor)) ||
			    state->second.effect != context.effect)
				return false;
			if (state->second.stage !=
				    spell_component_retirement_stage::items_retired &&
			    state->second.stage != spell_component_retirement_stage::effect_pending)
			{
				if (result.item_count < context.item_count)
					return false;
				for (size_t index = 0; index < context.item_count; ++index)
				{
					P_obj item =
						spell_component_by_uid(context.item_uids[index]);
					item_ownership_runtime_entry runtime = {};
					if (!item || !OBJ_CARRIED_BY(item, actor) ||
					    !item_ownership_runtime_lookup(item->obj_uid,
									   &runtime) ||
					    runtime.state != item_custody_state::destroyed ||
					    runtime.owner.type != item_owner_type::destruction ||
					    runtime.vnum != OBJ_VNUM(item) ||
					    runtime.root_item_uid != item->obj_uid ||
					    runtime.parent_item_uid)
					{
						logit(LOG_FILE,
						      "spell component publication retained (pid=%d uid=%llu)",
						      GET_PID(actor),
						      (unsigned long long)context.item_uids[index]);
						return false;
					}
				}
				for (size_t index = 0; index < context.item_count; ++index)
					extract_obj(
						spell_component_by_uid(context.item_uids[index]));
				state->second.stage =
					spell_component_retirement_stage::items_retired;
			}
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (committed)
	{
		const auto state = spell_component_retired_items.find(operation_key);
		if (state != spell_component_retired_items.end() && state->second.save_acknowledged)
		{
			spell_component_retired_items.erase(state);
			return true;
		}
	}
	spell_component_effect_completion_fn continuation =
		spell_component_effect_callback(context.effect);
	if (!continuation)
		return false;
	const spell_component_effect_status status = continuation(
		operation_id, actor, committed, result, error_code,
		context.continuation_context.data(), context.continuation_context_size);
	if (committed && status == spell_component_effect_status::complete)
		spell_component_retired_items.erase(operation_key);
	else if (committed)
	{
		auto state = spell_component_retired_items.find(operation_key);
		if (state == spell_component_retired_items.end())
			return false;
		state->second.stage = status == spell_component_effect_status::waiting_for_owner ?
					      spell_component_retirement_stage::effect_pending :
					      spell_component_retirement_stage::items_retired;
	}
	return status == spell_component_effect_status::complete;
}

} // namespace

bool spell_component_retirement_restore_context(
	const item_transfer_payload &payload,
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> *encoded_context,
	size_t *encoded_context_size, uint32_t *restored_effect_id, uint32_t *restored_owner_pid)
{
	if (!encoded_context || !encoded_context_size ||
	    payload.continuation.kind !=
		    item_transfer_continuation_kind::spell_component_retirement ||
	    payload.reason != item_transfer_reason::destruction || !payload.multi_root ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	const std::vector<uint8_t> &data = payload.continuation.data;
	const bool legacy = data.size() >= 4 && data.size() <= 4 + 48 && data[1] == 0 &&
			    data[2] == 0 && data[3] == 0;
	if (!legacy &&
	    (data.size() < 6 || data.size() > 6 + 48 || data[0] != 1 || data[5] != data.size() - 6))
		return false;
	const size_t effect_offset = legacy ? 0 : 1;
	const size_t context_offset = legacy ? 4 : 6;
	const uint32_t effect_id = static_cast<uint32_t>(data[effect_offset]) |
				   (static_cast<uint32_t>(data[effect_offset + 1]) << 8) |
				   (static_cast<uint32_t>(data[effect_offset + 2]) << 16) |
				   (static_cast<uint32_t>(data[effect_offset + 3]) << 24);
	if (effect_id < static_cast<uint32_t>(item_spell_component_effect::faerie_sight) ||
	    effect_id > static_cast<uint32_t>(item_spell_component_effect::vines))
		return false;
	spell_component_retirement_context context = {};
	context.effect = static_cast<item_spell_component_effect>(effect_id);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &item = payload.items[index];
		if (!item.item_uid)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (payload.items[prior].item_uid == item.item_uid)
				return false;
		if (!item.parent_item_uid)
		{
			if (item.root_item_uid != item.item_uid ||
			    context.item_count >= context.item_uids.size())
				return false;
			context.item_uids[context.item_count++] = item.item_uid;
		}
	}
	if (!context.item_count)
		return false;
	context.continuation_context_size = static_cast<uint32_t>(data.size() - context_offset);
	if (context.continuation_context_size)
		memcpy(context.continuation_context.data(), data.data() + context_offset,
		       context.continuation_context_size);
	uint32_t owner_pid = context.effect == item_spell_component_effect::vines ?
				     static_cast<uint32_t>(payload.from_owner.id) :
				     0;
	if (context.effect == item_spell_component_effect::vines)
	{
		spell_component_context_reader reader(context.continuation_context.data(),
						      context.continuation_context_size);
		int32_t level = 0, count = 0;
		if (!reader.get_i32(&level) || !reader.get_i32(&count) || !reader.finished() ||
		    count < 1 || count > 4 || count != context.item_count)
			return false;
	}
	if (context.effect == item_spell_component_effect::faerie_sight)
	{
		spell_component_context_reader reader(context.continuation_context.data(),
						      context.continuation_context_size);
		uint64_t runtime_id = 0;
		int32_t actor_pid = 0, level = 0, target_pid = 0;
		uint8_t dust_count = 0, self_target = 0;
		if (!reader.get_u64(&runtime_id) || !reader.get_i32(&actor_pid) ||
		    !reader.get_i32(&level) || !reader.get_u8(&dust_count) ||
		    !reader.get_u8(&self_target) || self_target > 1 || actor_pid <= 0 ||
		    static_cast<uint64_t>(actor_pid) != payload.from_owner.id ||
		    dust_count != context.item_count ||
		    (context.continuation_context_size == 22 && !reader.get_i32(&target_pid)) ||
		    !reader.finished() || target_pid < 0 ||
		    (self_target && target_pid && target_pid != actor_pid))
			return false;
		owner_pid = self_target ? static_cast<uint32_t>(actor_pid) :
					  static_cast<uint32_t>(target_pid);
	}
	if (sizeof(context) > encoded_context->size())
		return false;
	memcpy(encoded_context->data(), &context, sizeof(context));
	*encoded_context_size = sizeof(context);
	if (restored_effect_id)
		*restored_effect_id = effect_id;
	if (restored_owner_pid)
		*restored_owner_pid = owner_pid;
	return true;
}

bool spell_component_retirement_replayed_publication(const critical_operation_id &operation_id,
						     P_char actor, bool committed,
						     const item_transfer_result &result,
						     unsigned int error_code,
						     const uint8_t *context, size_t context_size)
{
	if (!actor || !context || context_size != sizeof(spell_component_retirement_context))
		return false;
	if (committed)
	{
		spell_component_retirement_context restored = {};
		memcpy(&restored, context, sizeof(restored));
		const auto state = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		if (state != spell_component_retired_items.end() && state->second.owner_pid &&
		    (restored.effect == item_spell_component_effect::vines ||
		     restored.effect == item_spell_component_effect::faerie_sight))
			return spell_component_retirement_published(operation_id, actor, true,
								    result, error_code, context,
								    context_size);
		send_to_char(
			"Your spell components were consumed, but the spell effect is waiting for safe recovery. Please wait or contact staff.\r\n",
			actor);
		return false;
	}
	return spell_component_retirement_published(operation_id, actor, false, result, error_code,
						    context, context_size);
}

bool spell_component_retirement_waiting_for_effect(const critical_operation_id &operation_id)
{
	try
	{
		const auto found = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		return found != spell_component_retired_items.end() &&
		       found->second.stage == spell_component_retirement_stage::effect_pending;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool spell_component_retirement_restore_replayed_effect(const critical_operation_id &operation_id,
							uint32_t actor_pid, uint32_t effect_id,
							uint32_t receipt_owner_pid)
{
	const auto effect = static_cast<item_spell_component_effect>(effect_id);
	if (effect != item_spell_component_effect::vines &&
	    effect != item_spell_component_effect::faerie_sight)
		return true;
	if (effect == item_spell_component_effect::vines && !receipt_owner_pid)
		receipt_owner_pid = actor_pid;
	if (!receipt_owner_pid)
		return true;
	if (!actor_pid)
		return false;
	try
	{
		auto [found, inserted] = spell_component_retired_items.try_emplace(
			spell_component_operation_key(operation_id));
		if (inserted)
		{
			found->second.stage = spell_component_retirement_stage::items_retired;
			found->second.actor_pid = actor_pid;
			found->second.owner_pid = receipt_owner_pid;
			found->second.effect = effect;
		}
		return found->second.actor_pid == actor_pid &&
		       found->second.owner_pid == receipt_owner_pid &&
		       found->second.effect == effect;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool spell_component_retirement_bind_effect_owner(const critical_operation_id &operation_id,
						  uint32_t actor_pid, uint32_t owner_pid,
						  item_spell_component_effect effect)
{
	if (!actor_pid || !owner_pid)
		return false;
	try
	{
		auto found = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		if (found == spell_component_retired_items.end() ||
		    found->second.actor_pid != actor_pid || found->second.effect != effect ||
		    (found->second.effect_applied && found->second.owner_pid != owner_pid))
			return false;
		found->second.owner_pid = owner_pid;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool spell_component_retirement_append_owner_operations(
	uint32_t owner_pid, std::vector<critical_operation_id> *operations)
{
	if (!owner_pid || !operations)
		return false;
	try
	{
		for (const auto &[key, state] : spell_component_retired_items)
		{
			if (state.owner_pid != owner_pid ||
			    (state.effect != item_spell_component_effect::vines &&
			     state.effect != item_spell_component_effect::faerie_sight))
				continue;
			critical_operation_id operation_id = {};
			if (key.size() != operation_id.bytes.size())
				return false;
			memcpy(operation_id.bytes.data(), key.data(), key.size());
			bool duplicate = false;
			for (const auto &existing : *operations)
				if (existing.bytes == operation_id.bytes)
					duplicate = true;
			if (duplicate)
				continue;
			if (operations->size() >= ITEM_MOVEMENT_PENDING_MAX)
				return false;
			operations->push_back(operation_id);
		}
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool spell_component_retirement_pending_save_receipts(
	uint32_t owner_pid, std::vector<player_spell_effect_receipt_snapshot> *receipts)
{
	if (!owner_pid || !receipts)
		return false;
	try
	{
		for (const auto &[key, state] : spell_component_retired_items)
		{
			if (state.owner_pid != owner_pid || !state.effect_applied ||
			    state.save_acknowledged)
				continue;
			player_spell_effect_receipt_snapshot receipt = {};
			if (key.size() != receipt.operation_id.bytes.size() ||
			    (state.effect != item_spell_component_effect::vines &&
			     state.effect != item_spell_component_effect::faerie_sight) ||
			    receipts->size() >= PLAYER_SPELL_EFFECT_RECEIPT_MAX)
				return false;
			memcpy(receipt.operation_id.bytes.data(), key.data(), key.size());
			receipt.effect_id = static_cast<uint32_t>(state.effect);
			receipts->push_back(receipt);
		}
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

void spell_component_retirement_recover_receipts(uint32_t actor_pid,
						 const player_load_spell_effect_receipt *receipts,
						 size_t count)
{
	if (!actor_pid || (count && !receipts))
		return;
	for (auto &[key, state] : spell_component_retired_items)
	{
		if (state.owner_pid != actor_pid)
			continue;
		bool found = false;
		for (size_t index = 0; index < count; ++index)
			if (receipts[index].effect_id == static_cast<uint32_t>(state.effect) &&
			    key.size() == receipts[index].operation_id.bytes.size() &&
			    memcmp(key.data(), receipts[index].operation_id.bytes.data(),
				   key.size()) == 0)
			{
				found = true;
				break;
			}
		state.effect_applied = found;
		state.save_acknowledged = found;
		state.save_requested = false;
	}
}

void spell_component_retirement_save_completed(int32_t actor_pid, bool acknowledged,
					       const player_spell_effect_receipt_snapshot *receipts,
					       size_t count)
{
	if (actor_pid <= 0 || (count && !receipts))
		return;
	for (size_t index = 0; index < count; ++index)
	{
		try
		{
			auto found = spell_component_retired_items.find(
				spell_component_operation_key(receipts[index].operation_id));
			if (found == spell_component_retired_items.end() ||
			    found->second.owner_pid != static_cast<uint32_t>(actor_pid) ||
			    static_cast<uint32_t>(found->second.effect) !=
				    receipts[index].effect_id)
				continue;
			found->second.save_requested = false;
			if (acknowledged)
				found->second.save_acknowledged = true;
		}
		catch (const std::bad_alloc &)
		{
			return;
		}
	}
}

bool spell_component_retirement_effect_applied(const critical_operation_id &operation_id)
{
	try
	{
		const auto found = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		return found != spell_component_retired_items.end() && found->second.effect_applied;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool spell_component_retirement_effect_applied_once(const critical_operation_id &operation_id)
{
	try
	{
		auto found = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		if (found == spell_component_retired_items.end() ||
		    (found->second.effect != item_spell_component_effect::vines &&
		     found->second.effect != item_spell_component_effect::faerie_sight))
			return false;
		found->second.effect_applied = true;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

spell_component_effect_status
spell_component_retirement_save_effect(const critical_operation_id &operation_id, P_char actor,
				       item_spell_component_effect effect)
{
	if (!actor || GET_PID(actor) <= 0 ||
	    (effect != item_spell_component_effect::vines &&
	     effect != item_spell_component_effect::faerie_sight))
		return spell_component_effect_status::retry;
	try
	{
		auto found = spell_component_retired_items.find(
			spell_component_operation_key(operation_id));
		if (found == spell_component_retired_items.end() ||
		    found->second.owner_pid != static_cast<uint32_t>(GET_PID(actor)) ||
		    found->second.effect != effect || !found->second.effect_applied)
			return spell_component_effect_status::retry;
		if (found->second.save_acknowledged)
			return spell_component_effect_status::complete;
		const uint64_t now = persistence_observability_now_usec();
		if (!found->second.save_requested && now >= found->second.next_save_request_at_usec)
		{
			constexpr uint64_t retry_delay_usec = UINT64_C(5000000);
			found->second.next_save_request_at_usec =
				now > UINT64_MAX - retry_delay_usec ? UINT64_MAX :
								      now + retry_delay_usec;
			const player_spell_effect_receipt_snapshot receipt = {
				operation_id, static_cast<uint32_t>(effect)
			};
			const int room_vnum = actor->in_room >= 0 && world ?
						      world[actor->in_room].number :
						      NOWHERE;
			const auto saved = player_save_pipeline_request_spell_effect(
				actor, PLAYER_COMPONENT_AFFECTS, &receipt, room_vnum);
			found->second.save_requested =
				saved == player_save_pipeline_result::queued ||
				saved == player_save_pipeline_result::coalesced;
		}
		return spell_component_effect_status::waiting_for_owner;
	}
	catch (const std::bad_alloc &)
	{
		return spell_component_effect_status::waiting_for_owner;
	}
}

bool spell_consume_components(P_char actor, int vnum, size_t max_components, uint32_t reason_id,
			      item_spell_component_effect effect,
			      spell_component_effect_completion_fn continuation,
			      const void *continuation_context, size_t continuation_context_size,
			      bool require_exact_count)
{
	if (!actor || !IS_PC(actor) || GET_PID(actor) <= 0 || vnum < 0 || !max_components ||
	    max_components > 8 || !reason_id || !continuation ||
	    effect < item_spell_component_effect::faerie_sight ||
	    effect > item_spell_component_effect::vines || continuation_context_size > 48 ||
	    (continuation_context_size && !continuation_context) ||
	    !economic_gameplay_authority::active())
		return false;

	P_obj selected[8] = {};
	size_t selected_count = 0;
	for (P_obj object = actor->carrying; object && selected_count < max_components;
	     object = object->next_content)
		if (obj_index[object->R_num].virtual_number == vnum)
			selected[selected_count++] = object;
	if (!selected_count || (require_exact_count && selected_count != max_components))
		return false;

	spell_component_retirement_context context = {};
	context.effect = effect;
	spell_component_effect_completion_fn stable_continuation =
		spell_component_effect_callback(effect);
	if (!stable_continuation || continuation != stable_continuation)
		return false;
	context.continuation_context_size = static_cast<uint32_t>(continuation_context_size);
	context.item_count = static_cast<uint8_t>(selected_count);
	if (continuation_context_size)
		memcpy(context.continuation_context.data(), continuation_context,
		       continuation_context_size);
	for (size_t index = 0; index < selected_count; ++index)
		context.item_uids[index] = selected[index]->obj_uid;

	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(GET_PID(actor)), 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	item_movement_reject reject = item_movement_reject::none;
	item_transfer_continuation durable_continuation = {};
	durable_continuation.kind = item_transfer_continuation_kind::spell_component_retirement;
	try
	{
		durable_continuation.data.resize(6 + continuation_context_size);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	durable_continuation.data[0] = 1;
	const uint32_t effect_id = static_cast<uint32_t>(effect);
	for (size_t byte = 0; byte < sizeof(effect_id); ++byte)
		durable_continuation.data[byte + 1] = static_cast<uint8_t>(effect_id >> (byte * 8));
	durable_continuation.data[5] = static_cast<uint8_t>(continuation_context_size);
	if (continuation_context_size)
		memcpy(durable_continuation.data.data() + 6, continuation_context,
		       continuation_context_size);
	return item_movement_transaction_submit_batch(
		actor, selected, selected_count, NULL, owner, destruction,
		item_transfer_reason::destruction, static_cast<int64_t>(reason_id), nullptr,
		&context, sizeof(context), NULL, &reject, spell_component_retirement_published,
		economic_source_kind::spell_consumption, 0, durable_continuation);
}

/*
 * Offensive Spells
 */

// Old edrain below. -Lucrot Jul09
// Shows exactly why Lucrot shouldn't have been touching code... seriously, nice commenting.
/*
 * Drain XP, MANA, HP - caster gains HP and MANA
 */
// void spell_energy_drain(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj)
// {
// int xp, mana, dam;
// struct damage_messages messages = {
// "You drain $N of some of $S energy.",
// "You feel less energetic as $n drains you.",
// "$n drains $N - what a waste of energy!",
// "$N crumples as you kill $M by draining $S energy.",
// "As $n drains your last bit of energy, you look forward to the peace of the graveyard.",
// "$n drains the energy of $N who crumbles into a lifeless husk."
// };

// if( !IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch )
// {
// return;
// }

// if(resists_spell(ch, victim))
// {
// return;
// }

//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 2);

// dam = (int) ((level * 2.5) + number(-10, 10));

// if(IS_PC(ch) &&
// !(GET_CLASS(ch, CLASS_NECROMANCER | CLASS_ANTIPALADIN)))
// {
// dam /= 4;
// }

// if(IS_AFFECTED4(victim, AFF4_DEFLECT))
// {
// if(GET_LEVEL(ch) >= 50)
// {
// dam <<= 1;
// }

// spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
// return;
// }

// if(saves_spell(victim, SAVING_SPELL))
// {
// dam /= 2;
// }

//  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 4);
// if(GET_LEVEL(victim) <= 2)
// {
// /*
// * Kill the sucker
// */
// act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
// act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
// act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
// die(victim, ch);
// victim = NULL;
// }
// else
// {
// xp = GET_LEVEL(ch) * 1000;
// xp = MIN(xp, GET_EXP(victim));

// if(GET_LEVEL(ch) >= 50)
// {
// dam <<= 1;
// }

// if(!IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// mana = MIN(GET_MANA(victim), 100);
// GET_MANA(victim) -= mana;
// StartRegen(victim, regen_resource::mana);
// GET_MANA(ch) += mana >> 1;
// }

// if(IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// vamp(ch, (int)(dam / 6), (int) (GET_MAX_HIT(ch) * 1.25));
// }
// else
// {
// vamp(ch, (int)(dam / 2), (int) (GET_MAX_HIT(ch) * 1.25));
// }

// if(GET_LEVEL(ch) <= 50)
// {
// send_to_char("&+LYour life energy is drained!\n",
// victim);
// }
// else
// {
// send_to_char("&+LYour life energy is &+rtapped&+L.\n",
// victim);
// }

// if(GET_VITALITY(victim) >= 10 &&
// !IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
// {
// GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 15);
// GET_VITALITY(ch) += 10;
// }

// StartRegen(ch, regen_resource::vitality);
// StartRegen(victim, regen_resource::vitality);

// spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG,
// &messages);
// }
// }

/* The conjure_terrain_check() function checks if it is a specialized conjurer  */
/* trying to conjure on terrain appropriate for the elemental type, and handles */
/* appropriate messages. Bonuses/maluses themselves are attributed in the       */
/* conjour_elemental() and conjure_specialized() functions.                     */
/* Return values from this function are:                                        */
/* -1 - bad terrain for the elemental type                                      */
/* 0 - neutral terrain                                                          */
/* 1 - good terrain for the conjurer elemental type                             */

/*
 * cast_as_damage_area passes pointer to the index of hit victim as arg
 */

// ch and victim is backwards so disarm will work right.
/*
 * spells2.c - Not directly offensive spells
 */

/* Seeya!
void spell_healing_blade(int level, P_char ch, char *arg, int type,
                         P_char victim, P_obj obj)
{
  struct affected_type af;
  P_obj    wpn;

  if(affected_by_spell(ch, SPELL_HEALING_BLADE))
  {
    send_to_char("You are already on &+Rfire!!\n", ch);
    return;
  }

  if(!(wpn = ch->equipment[WIELD]) || !IS_SWORD(wpn))
  {
    send_to_char("You need to be wielding a slashing weapon!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));

  af.type = SPELL_HEALING_BLADE;
  af.duration = level;
  af.location = APPLY_HITROLL;
  af.modifier = number(1, 4);

  affect_to_char(victim, &af);

  send_to_char("&+BA blue aura covers your blade with a healing power!&n\n",
               ch);

}
*/

/* OLD HEAL - 21 Sep 08 -Lucrot
{
   int      healpoints = 100;


  spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
  if(GET_HIT(victim) > GET_MAX_HIT(victim))
    return;

  heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
  update_pos(victim);
  if(IS_RACEWAR_UNDEAD(victim))
    send_to_char("&+WYou feel the powers of darkness strengthen you!\n",
                 victim);
  else
    send_to_char("&+WA warm feeling fills your body.\n", victim);
  grapple_heal(victim);

}
*/

void spell_ventriloquate(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/,
			 P_char /*victim*/, P_obj /*obj*/) {
	/*
	 * Not possible!! No argument!
	 */
}

/* void spell_vigorize_light(int level, P_char ch, char *arg, int type,
                          P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = number(4, 15);

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  update_pos(victim);

  send_to_char("You feel a bit more invigorated!\n", victim);
} */

/* void spell_vigorize_serious(int level, P_char ch, char *arg, int type,
                            P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = dice(3, (level / 3));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

  send_to_char("You feel much more invigorated!\n", victim);

  update_pos(victim);
} */

/* void spell_vigorize_critic(int level, P_char ch, char *arg, int type,
                           P_char victim, P_obj obj)
{
  int      movepoints;

  movepoints = (level / 2) + dice(4, (level / 4));

  if((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
    GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
  else
    GET_VITALITY(victim) += movepoints;

    act("",
       FALSE, ch, 0, 0, TO_ROOM);
    act
      ("&+WThe orb of light begins to take shape... a mounting sense of awe grips the area.",
       FALSE, ch, 0, 0, TO_CHAR);

  send_to_char("You feel invigorated!\n", victim);
} */

/*
void event_plague(P_char ch, P_char vict, P_obj obj, void *data)
{
  P_char target;
  int timer, both = FALSE;
  struct affected_type af;

  if(!affected_by_spell(ch, SPELL_PLAGUE) && !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
    return;

  if(affected_by_spell(ch, SPELL_PLAGUE) && affected_by_spell(ch, SPELL_DISEASE))
  {
    act("$n's &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_ROOM);
    act("Your &+rin&+Rfec&+rtio&+Rus w&+roun&+Rds&n blister and pop spreading the &+Gplague&n everywhere!", TRUE, ch, 0, 0, TO_CHAR);
    both = TRUE;
  }

  if(affected_by_spell(ch, SPELL_PLAGUE) && !affected_by_spell(ch, SPELL_DISEASE) &&
      !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
  {
    spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, ch, 0);
  }

  for (target = world[ch->in_room].people; target; target = target->next_in_room)
  {
    if(IS_TRUSTED(target))
      continue;
    if(IS_UNDEADRACE(target))
      continue;

    if(!affected_by_spell(target, SPELL_PLAGUE) &&
        number(0, 100) < ((int)get_property("spell.plague.spread.perc", 30)+(both ? 30 : 0)))
    {
      bzero(&af, sizeof(af));
      af.type = SPELL_PLAGUE;
      af.duration = (int)get_property("spell.plague.duration", 10);
      af.modifier = 500;
      affect_to_char(target, &af);

      if(!get_scheduled(target, event_plague))
        add_event(event_plague, WAIT_SEC * 1, target, 0, 0, 0, 0, 0);
    }
  }

  timer = (int)get_property("spell.plague.eventTime", 60);
  if(affected_by_spell(ch, SPELL_DISEASE))
    timer -= 20;
  timer += number(-10, 10);
  if(IS_FIGHTING(ch))
    timer -= 30;
  if(timer < 5)
    timer = 5;

  add_event(event_plague, WAIT_SEC * timer, ch, 0, 0, 0, 0, 0);
}

*/

/*
void spell_windstrom_blessing(int level, P_char ch, char *arg, int type,
                              P_char victim, P_obj obj)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_WINDSTROM_BLESSING))
  {
    send_to_char("&+CWindstrom&n has blessed your blade already!\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = SPELL_WINDSTROM_BLESSING;
  af.duration = 5;
  affect_to_char(ch, &af);
}
*/

/*
 * ***************************************************************************
 * *                     NPC spells..
 * * *
 * *************************************************************************
 */

;

// end spell_feeblemind

#include <cerrno>
#include <type_traits>

namespace
{
bool spell_replay_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t spell_replay_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t spell_replay_key_frames =
	// Original operation_key reference and returned string carrier, actual
	// string(char*,n,allocator) this/source/n/allocator, _Alloc_hider,
	// _M_construct forward begin/end/dnew/_Guard/_M_create parameters.
	sizeof(void *) + sizeof(std::string) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(std::allocator<char>) + 3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(std::string *) + 3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(void *) +
	// Actual traits copy(dest,source,n,result), _M_data/_M_capacity/setlength,
	// original string destruction and equal-allocator dispose/deallocate.
	3 * sizeof(void *) + sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) +
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + spell_replay_allocator_frames;

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t spell_replay_table_frames =
	// current_table_heap_bytes: this/output/bool, bytes/buckets, range begin/end,
	// entry reference and real iterator query/advance/key capacity/add carriers.
	4 * sizeof(void *) + sizeof(bool) + 2 * sizeof(size_t) +
	2 * sizeof(spell_retirement_table::const_iterator) +
	4 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(bool);
constexpr size_t spell_replay_policy_frames =
	// next_insert_extra_peak: this/key/output, bool, text/extra, CURRENT policy
	// copy, _M_need_rehash input this/buckets/elements/insert, result/local pair.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(size_t) +
	sizeof(std::__detail::_Prime_rehash_policy) + 2 * sizeof(std::pair<bool, size_t>) +
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t spell_replay_insert_frames =
	// Original try_emplace -> table try_emplace -> lookup key/hash/bucket/node.
	4 * (3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	// _Scoped_node original two-pointer owner, saved policy state/new pair,
	// node/bucket allocator, piecewise pair/forward tuples and returned pair.
	3 * sizeof(void *) + sizeof(std::__detail::_Prime_rehash_policy::_State) +
	2 * sizeof(std::pair<bool, size_t>) +
	sizeof(std::allocator<std::__detail::_Hash_node_base *>) +
	sizeof(std::pair<spell_retirement_table::iterator, bool>) +
	sizeof(std::piecewise_construct_t) + sizeof(std::tuple<std::string &&>) +
	sizeof(std::tuple<>) + 6 * sizeof(void *) + spell_replay_allocator_frames +
	// Moving the actual key into the node is allocation-free but owns real
	// string move/hider/traits copied inline and destructor carriers.
	spell_replay_key_frames;
#endif
} // namespace

bool spell_component_retirement_current_storage_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8)
		return false;
	size_t bytes = 0;
	if (!spell_component_retired_items.current_table_heap_bytes(&bytes) ||
	    !spell_replay_add(bytes, sizeof(spell_component_retired_items)))
		return false;
	*output = bytes;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool spell_component_retirement_restore_replayed_effect_bounded(
	const critical_operation_id &operation_id, uint32_t actor_pid, uint32_t effect_id,
	uint32_t receipt_owner_pid, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t exclusive_outer) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!reserve)
		return false;
	// Genuine source parameter/local/reference/result carriers. This is source
	// lifetime accounting; emitted/native stack qualification remains separate.
	constexpr size_t frames =
		3 * sizeof(void *) + 3 * sizeof(uint32_t) + sizeof(size_t) +
		sizeof(item_spell_component_effect) + sizeof(bool) + sizeof(std::string) +
		sizeof(std::pair<spell_retirement_table::iterator, bool>) + 3 * sizeof(size_t) +
		2 * sizeof(void *) + spell_replay_table_frames + spell_replay_key_frames;
	const auto peak = [&](size_t extra) noexcept
	{
		size_t bytes = exclusive_outer, current = 0;
		return spell_component_retirement_current_storage_bytes(&current) &&
		       spell_replay_add(bytes, current) && spell_replay_add(bytes, frames) &&
		       spell_replay_add(bytes, sizeof(extra) + 2 * sizeof(size_t) +
						       3 * sizeof(void *) + 2 * sizeof(bool)) &&
		       spell_replay_add(bytes, extra) && reserve(bytes, context);
	};
	if (!peak(0))
		return false;
	const auto effect = static_cast<item_spell_component_effect>(effect_id);
	if (effect != item_spell_component_effect::vines &&
	    effect != item_spell_component_effect::faerie_sight)
		return true;
	if (effect == item_spell_component_effect::vines && !receipt_owner_pid)
		receipt_owner_pid = actor_pid;
	if (!receipt_owner_pid)
		return true;
	if (!actor_pid)
		return false;
	try
	{
		// The original 16-byte operation-key constructor owns one fresh 17-byte
		// string allocation. Admit before construction, not after it is retained.
		if (!peak(operation_id.bytes.size() > 15 ? operation_id.bytes.size() + 1 : 0))
			return false;
		std::string key = spell_component_operation_key(operation_id);
		const size_t key_heap = key.capacity() > 15 ? key.capacity() + 1 : 0;
		if (!peak(key_heap + spell_replay_policy_frames))
			return false;
		size_t request = 0;
		// try_emplace's duplicate branch allocates neither node nor buckets.
		if (spell_component_retired_items.find(key) ==
			    spell_component_retired_items.end() &&
		    !spell_component_retired_items.next_insert_extra_peak(key, &request))
			return false;
		// Move-key try_emplace transfers the original string heap into the node.
		// The shared prospective provider also includes a fresh key
		// copy request; drop it here because this original path is an rvalue move.
		if (request && key_heap)
			request -= key.size() + 1;
		if (!spell_replay_add(request, key_heap) ||
		    !spell_replay_add(request, spell_replay_insert_frames) || !peak(request))
			return false;
		auto [found, inserted] = spell_component_retired_items.try_emplace(std::move(key));
		if (inserted)
		{
			found->second.stage = spell_component_retirement_stage::items_retired;
			found->second.actor_pid = actor_pid;
			found->second.owner_pid = receipt_owner_pid;
			found->second.effect = effect;
		}
		// No fallible reservation follows insertion/attachment. The caller obtains
		// fresh CURRENT on every outcome, including retained bucket growth on throw.
		return found->second.actor_pid == actor_pid &&
		       found->second.owner_pid == receipt_owner_pid &&
		       found->second.effect == effect;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#else
	(void)operation_id;
	(void)actor_pid;
	(void)effect_id;
	(void)receipt_owner_pid;
	(void)reserve;
	(void)context;
	(void)exclusive_outer;
	return false;
#endif
}
