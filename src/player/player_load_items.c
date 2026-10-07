#include "player/player_load_items.h"

#include "net/comm.h"
#include "world/object_template.h"
#include "core/mm.h"
#include "item/objmisc.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "mob/studioproclib.h"
#include <utility>
#include "item/item_ownership_runtime.h"
#include "item/encumbrance_policy.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "magic/spells.h"
#include "core/structs.h"
#include "core/utils.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <limits>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern Skill skills[];

namespace
{
class staged_item_graph
{
    public:
	~staged_item_graph()
	{
		if (published)
			return;
		for (P_obj object : objects)
			if (object)
				object->obj_uid = 0;
		if (linked)
		{
			for (size_t index = 0; index < objects.size(); ++index)
				if (objects[index] && roots[index])
					extract_obj(objects[index], FALSE);
			return;
		}
		for (P_obj object : objects)
			if (object)
				extract_obj(object, FALSE);
	}

	std::vector<P_obj> objects;
	std::vector<bool> roots;
	bool linked = false;
	bool published = false;
};

bool fail(player_load_item_materialize_metrics *metrics,
	  player_load_item_materialize_outcome outcome)
{
	if (metrics)
		metrics->outcome = outcome;
	return false;
}

void clear_item_uids(P_obj object)
{
	if (!object)
		return;
	object->obj_uid = 0;
	for (P_obj child = object->contains; child; child = child->next_content)
		clear_item_uids(child);
}

bool count_operation(player_load_item_materialize_metrics *metrics, size_t item_count,
		     size_t amount = 1)
{
	if (!metrics ||
	    metrics->operation_count > PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM * item_count - amount)
		return false;
	metrics->operation_count += amount;
	return true;
}

bool parse_spellbook(const std::string &json, char *spell_bits = nullptr)
{
	std::array<bool, MAX_SKILLS> seen = {};
	size_t position = 0;
	auto skip_space = [&]()
	{
		while (position < json.size() && (json[position] == ' ' || json[position] == '\t' ||
						  json[position] == '\r' || json[position] == '\n'))
			++position;
	};
	skip_space();
	if (position >= json.size() || json[position++] != '[')
		return false;
	skip_space();
	if (position < json.size() && json[position] == ']')
	{
		++position;
		skip_space();
		return position == json.size();
	}
	for (;;)
	{
		skip_space();
		if (position >= json.size() || json[position] < '0' || json[position] > '9')
			return false;
		const size_t number_start = position;
		uint64_t value = 0;
		while (position < json.size() && json[position] >= '0' && json[position] <= '9')
		{
			const uint64_t digit = static_cast<unsigned int>(json[position++] - '0');
			if (value > (static_cast<uint64_t>(MAX_SKILLS) - 1 - digit) / 10)
				return false;
			value = value * 10 + digit;
		}
		if (json[number_start] == '0' && position != number_start + 1)
			return false;
		if (seen[value])
			return false;
		seen[value] = true;
		if (spell_bits)
			spell_bits[value / 8] = static_cast<char>(
				static_cast<unsigned char>(spell_bits[value / 8]) |
				static_cast<unsigned char>(1U << (value % 8)));
		skip_space();
		if (position >= json.size())
			return false;
		if (json[position] == ']')
		{
			++position;
			break;
		}
		if (json[position++] != ',')
			return false;
	}
	skip_space();
	return position == json.size();
}

// Captured/flatfile snapshots store typed spell IDs; SQL rows store JSON.
// Both representations feed the same validation and materialization path.
// When spell_bits is non-null, callers must provide a zero-initialized buffer
// of (MAX_SKILLS + 1) / 8 + 1 bytes because this helper ORs the selected bits
// into it rather than clearing it first.
bool decode_saved_spellbook(const player_item_extra_description_snapshot &description,
			    char *spell_bits = nullptr)
{
	if (!description.description.empty())
		return description.spell_ids.empty() &&
		       parse_spellbook(description.description, spell_bits);
	std::array<bool, MAX_SKILLS> seen = {};
	for (int32_t spell : description.spell_ids)
	{
		if (spell < 0 || spell >= MAX_SKILLS || seen[spell])
			return false;
		seen[spell] = true;
		if (spell_bits)
			spell_bits[spell / 8] = static_cast<char>(
				static_cast<unsigned char>(spell_bits[spell / 8]) |
				static_cast<unsigned char>(1U << (spell % 8)));
	}
	return true;
}

enum class metadata_validation_outcome
{
	valid,
	invalid,
	allocation_failure,
};

metadata_validation_outcome valid_item_metadata(const player_item_snapshot &item,
						const player_load_item_identity &identity,
						bool complete_snapshot_state)
{
	constexpr uint8_t allowed_string_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 |
						STRUNG_DESC3 | STRUNG_EDESC;
	if ((item.string_mask & ~allowed_string_mask) ||
	    identity.database_id > static_cast<uint64_t>(INT_MAX) ||
	    identity.item_uid > static_cast<uint64_t>(ULONG_MAX) ||
	    item.timers[0] < std::numeric_limits<time_t>::min() ||
	    item.timers[0] > std::numeric_limits<time_t>::max())
		return metadata_validation_outcome::invalid;
	if (complete_snapshot_state)
		for (int64_t timer : item.timers)
			if (timer < std::numeric_limits<time_t>::min() ||
			    timer > std::numeric_limits<time_t>::max())
				return metadata_validation_outcome::invalid;
	if ((identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_TYPE) &&
	    (item.type < ITEM_LOWEST || item.type > ITEM_LAST))
		return metadata_validation_outcome::invalid;
	if (complete_snapshot_state ||
	    (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS))
		for (const auto &affect : item.dynamic_affects)
			if (affect.extra2 > ULONG_MAX)
				return metadata_validation_outcome::invalid;
	if ((identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS) &&
	    !(identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_EXTRA2_FLAGS))
		return metadata_validation_outcome::invalid;
	for (const auto &affect : item.affects)
		if ((identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS) &&
		    (affect[0] < 0 || affect[0] > APPLY_LAST || affect[1] < INT8_MIN ||
		     affect[1] > INT8_MAX))
			return metadata_validation_outcome::invalid;
	std::unordered_set<std::string> descriptions;
	try
	{
		descriptions.reserve(item.extra_descriptions.size());
		for (const player_item_extra_description_snapshot &description :
		     item.extra_descriptions)
		{
			if (description.keyword.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
			    description.description.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
			    description.spellbook != (description.keyword == "SPELLBOOK"))
				return metadata_validation_outcome::invalid;
			std::string key = description.keyword;
			key.push_back('\0');
			key += description.description;
			if (description.spellbook)
			{
				std::array<char, (MAX_SKILLS + 1) / 8 + 1> spell_bits = {};
				if (!decode_saved_spellbook(description, spell_bits.data()))
					return metadata_validation_outcome::invalid;
				if (description.description.empty())
					key.append(spell_bits.data(), spell_bits.size());
			}
			else if (!description.spell_ids.empty())
				return metadata_validation_outcome::invalid;
			if (!descriptions.insert(std::move(key)).second)
				return metadata_validation_outcome::invalid;
		}
	}
	catch (const std::bad_alloc &)
	{
		return metadata_validation_outcome::allocation_failure;
	}
	return metadata_validation_outcome::valid;
}

void apply_saved_strings(P_obj object, const player_item_snapshot &item)
{
	if (item.string_mask & STRUNG_KEYS)
	{
		object->name = str_dup(item.name.c_str());
		object->str_mask |= STRUNG_KEYS;
	}
	if (item.string_mask & STRUNG_DESC2)
	{
		object->short_description = str_dup(item.short_description.c_str());
		object->str_mask |= STRUNG_DESC2;
	}
	if (item.string_mask & STRUNG_DESC1)
	{
		object->description = str_dup(item.description.c_str());
		object->str_mask |= STRUNG_DESC1;
	}
	if (item.string_mask & STRUNG_DESC3)
	{
		object->action_description = str_dup(item.action_description.c_str());
		object->str_mask |= STRUNG_DESC3;
	}
}

void apply_extra_descriptions(P_obj object, const player_item_snapshot &item)
{
	auto already_present = [object](const player_item_extra_description_snapshot &candidate)
	{
		std::array<char, (MAX_SKILLS + 1) / 8 + 1> spell_bits = {};
		if (candidate.spellbook && !decode_saved_spellbook(candidate, spell_bits.data()))
			return false;
		for (const extra_descr_data *existing = object->ex_description; existing;
		     existing = existing->next)
		{
			const bool existing_spellbook =
				existing->keyword && strlen(existing->keyword) == 3 &&
				existing->keyword[0] == 3 && existing->keyword[1] == 1 &&
				existing->keyword[2] == 3;
			if (candidate.spellbook != existing_spellbook)
				continue;
			if (candidate.spellbook)
			{
				if (existing->description &&
				    memcmp(existing->description, spell_bits.data(),
					   spell_bits.size()) == 0)
					return true;
				continue;
			}
			if (existing->keyword && candidate.keyword == existing->keyword &&
			    candidate.description ==
				    (existing->description ? existing->description : ""))
				return true;
		}
		return false;
	};
	for (auto description = item.extra_descriptions.rbegin();
	     description != item.extra_descriptions.rend(); ++description)
	{
		if (already_present(*description))
			continue;
		extra_descr_data *entry;
		CREATE(entry, extra_descr_data, 1, MEM_TAG_EXDESCD);
		memset(entry, 0, sizeof(*entry));
		if (description->spellbook)
		{
			const char marker[] = { 3, 1, 3, 0 };
			entry->keyword = str_dup(marker);
			const size_t byte_count = (MAX_SKILLS + 1) / 8 + 1;
			CREATE(entry->description, char, byte_count, MEM_TAG_STRING);
			memset(entry->description, 0, byte_count);
			decode_saved_spellbook(*description, entry->description);
		}
		else
		{
			entry->keyword = str_dup(description->keyword.c_str());
			entry->description = str_dup(description->description.c_str());
		}
		entry->next = object->ex_description;
		object->ex_description = entry;
		object->str_mask |= STRUNG_EDESC;
	}
}

void attach_loaded_inventory(P_char character, const std::vector<P_obj> &objects,
			     const std::vector<size_t> &roots,
			     const std::vector<player_item_snapshot> &items)
{
	P_obj carrying_tail = character->carrying;
	while (carrying_tail && carrying_tail->next_content)
		carrying_tail = carrying_tail->next_content;
	for (size_t index : roots)
	{
		P_obj object = objects[index];
		// Pet hydration precedes the owner link. Keep hidden helper roots in
		// this NPC's inventory instead of reactivating legacy worn snapshots.
		const int slot = IS_NPC(character) && (object->extra_flags & ITEM_NOSHOW) ?
					 0 :
					 items[index].equipment_slot;
		if (slot > 0)
		{
			character->equipment[slot - 1] = object;
			object->loc.wearing = character;
			object->loc_p = LOC_WORN;
			if (IS_PC(character) && GET_ITEM_TYPE(object) == ITEM_ARMOR)
				character->only.pc->prestige += object->value[2];
			GET_CARRYING_W(character) += encumbrance_weight(GET_OBJ_WEIGHT(object)) / 2;
		}
		else
		{
			object->next_content = nullptr;
			if (carrying_tail)
				carrying_tail->next_content = object;
			else
				character->carrying = object;
			carrying_tail = object;
			object->loc.carrying = character;
			object->loc_p = LOC_CARRIED;
			object->z_cord = 0;
			GET_CARRYING_W(character) += encumbrance_weight(GET_OBJ_WEIGHT(object));
			IS_CARRYING_N(character)++;
			if (IS_PC(character) && !object->g_key && GET_LEVEL(character) < 57 &&
			    GET_PID(character) < 10000000)
				object->g_key = 1;
		}
	}
	balance_affects(character);
}
}

namespace
{
bool materialize_item_graph(P_char character, std::vector<P_obj> *detached_roots,
			    const std::vector<player_item_snapshot> &items,
			    const std::vector<player_load_item_identity> &identities,
			    const item_owner_identity &expected_owner, uint64_t owner_revision,
			    bool hydrate_ownership, bool complete_snapshot_state,
			    player_load_item_materialize_metrics *metrics)
{
	player_load_item_materialize_metrics local_metrics = {};
	if (!metrics)
		metrics = &local_metrics;
	*metrics = {};
	const size_t item_count = items.size();
	metrics->item_count = item_count;
	const bool detached = detached_roots != nullptr;
	if ((!character && !detached) || (character && detached) ||
	    !item_owner_identity_valid(expected_owner) ||
	    expected_owner.type == item_owner_type::system ||
	    expected_owner.type == item_owner_type::destruction ||
	    identities.size() != item_count || item_count > PLAYER_LOAD_ITEM_MAX)
		return fail(metrics,
			    item_count > PLAYER_LOAD_ITEM_MAX ?
				    player_load_item_materialize_outcome::limit_exceeded :
				    player_load_item_materialize_outcome::invalid_snapshot);
	if (detached)
	{
		detached_roots->clear();
		try
		{
			detached_roots->reserve(item_count);
		}
		catch (const std::bad_alloc &)
		{
			return fail(metrics,
				    player_load_item_materialize_outcome::allocation_failure);
		}
	}
	if (!item_count)
	{
		if (hydrate_ownership &&
		    !item_ownership_runtime_hydrate_owner(expected_owner, owner_revision))
			return fail(metrics,
				    player_load_item_materialize_outcome::ownership_failure);
		metrics->outcome = player_load_item_materialize_outcome::applied;
		return true;
	}

	std::unordered_map<uint64_t, size_t> uid_indices;
	std::unordered_map<uint64_t, size_t> database_indices;
	std::vector<std::vector<size_t>> children;
	std::vector<size_t> roots;
	std::vector<size_t> depths;
	std::array<bool, MAX_WEAR> occupied_slots = {};
	try
	{
		uid_indices.reserve(item_count);
		database_indices.reserve(item_count);
		children.resize(item_count);
		roots.reserve(item_count);
		depths.assign(item_count, 0);
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_item_materialize_outcome::allocation_failure);
	}

	for (size_t index = 0; index < item_count; ++index)
	{
		const player_item_snapshot &item = items[index];
		const player_load_item_identity &identity = identities[index];
		const size_t metadata_operations =
			item.extra_descriptions.size() +
			((identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS) ?
				 item.affects.size() :
				 0) +
			((complete_snapshot_state ||
			  (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS)) ?
				 item.dynamic_affects.size() :
				 0);
		if (!count_operation(metrics, item_count, 2 + metadata_operations) ||
		    !identity.database_id || !identity.item_uid ||
		    identity.item_uid != item.object_uid || !identity.root_item_uid ||
		    identity.quantity != 1 || identity.state != item_custody_state::active ||
		    !item_owner_identity_equal(identity.owner, expected_owner) ||
		    identity.owner_revision != owner_revision ||
		    identity.override_mask &
			    ~(PLAYER_LOAD_ITEM_OVERRIDE_ALL | PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME))
			return fail(metrics,
				    player_load_item_materialize_outcome::invalid_snapshot);
		try
		{
			if (!uid_indices.emplace(identity.item_uid, index).second ||
			    !database_indices.emplace(identity.database_id, index).second)
				return fail(metrics,
					    player_load_item_materialize_outcome::invalid_snapshot);
		}
		catch (const std::bad_alloc &)
		{
			return fail(metrics,
				    player_load_item_materialize_outcome::allocation_failure);
		}
		const metadata_validation_outcome metadata = valid_item_metadata(
			item, identity,
			complete_snapshot_state ||
				(identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME));
		if (metadata != metadata_validation_outcome::valid)
			return fail(
				metrics,
				metadata == metadata_validation_outcome::allocation_failure ?
					player_load_item_materialize_outcome::allocation_failure :
					player_load_item_materialize_outcome::invalid_snapshot);
		if ((detached && item.equipment_slot != -1) ||
		    (!detached && (item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR ||
				   (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
				    item.equipment_slot != 0))))
			return fail(metrics,
				    player_load_item_materialize_outcome::invalid_snapshot);
		if (!detached && item.equipment_slot > 0 && occupied_slots[item.equipment_slot - 1])
			return fail(metrics,
				    player_load_item_materialize_outcome::invalid_snapshot);
		if (!detached && item.equipment_slot > 0)
			occupied_slots[item.equipment_slot - 1] = true;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (identity.serialized_parent_id || identity.parent_item_uid ||
			    identity.root_item_uid != identity.item_uid)
				return fail(metrics,
					    player_load_item_materialize_outcome::invalid_snapshot);
			roots.push_back(index);
		}
		else
		{
			if (item.parent_index < 0 ||
			    static_cast<size_t>(item.parent_index) >= item_count)
				return fail(metrics,
					    player_load_item_materialize_outcome::invalid_snapshot);
			const size_t parent = static_cast<size_t>(item.parent_index);
			const player_load_item_identity &parent_identity = identities[parent];
			if (identity.serialized_parent_id != parent_identity.database_id ||
			    identity.parent_item_uid != parent_identity.item_uid ||
			    identity.root_item_uid != parent_identity.root_item_uid)
				return fail(metrics,
					    player_load_item_materialize_outcome::invalid_snapshot);
			try
			{
				children[parent].push_back(index);
			}
			catch (const std::bad_alloc &)
			{
				return fail(
					metrics,
					player_load_item_materialize_outcome::allocation_failure);
			}
		}
	}
	if (roots.empty())
		return fail(metrics, player_load_item_materialize_outcome::invalid_snapshot);

	std::vector<size_t> traversal;
	try
	{
		traversal.reserve(item_count);
		for (size_t root : roots)
		{
			depths[root] = 1;
			traversal.push_back(root);
		}
		for (size_t cursor = 0; cursor < traversal.size(); ++cursor)
		{
			const size_t parent = traversal[cursor];
			if (!count_operation(metrics, item_count))
				return fail(metrics,
					    player_load_item_materialize_outcome::limit_exceeded);
			metrics->maximum_depth = std::max(metrics->maximum_depth, depths[parent]);
			if (depths[parent] > PLAYER_SNAPSHOT_MAX_DEPTH)
				return fail(metrics,
					    player_load_item_materialize_outcome::limit_exceeded);
			for (size_t child : children[parent])
			{
				if (depths[child])
					return fail(metrics, player_load_item_materialize_outcome::
								     invalid_snapshot);
				depths[child] = depths[parent] + 1;
				traversal.push_back(child);
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_item_materialize_outcome::allocation_failure);
	}
	if (traversal.size() != item_count)
		return fail(metrics, player_load_item_materialize_outcome::invalid_snapshot);

	staged_item_graph staged;
	std::vector<item_ownership_runtime_entry> ownership;
	try
	{
		staged.objects.assign(item_count, nullptr);
		staged.roots.assign(item_count, false);
		ownership.reserve(item_count);
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_item_materialize_outcome::allocation_failure);
	}
	for (size_t index = 0; index < item_count; ++index)
	{
		const player_item_snapshot &item = items[index];
		const player_load_item_identity &identity = identities[index];
		const int object_number = real_object(item.vnum);
		if (object_number < 0)
			return fail(metrics,
				    player_load_item_materialize_outcome::unknown_prototype);
		P_obj object = read_object(object_number, REAL);
		if (!object)
			return fail(metrics,
				    player_load_item_materialize_outcome::allocation_failure);
		staged.objects[index] = object;
		object->obj_uid = static_cast<unsigned long>(identity.item_uid);
		object->db_item_id = static_cast<int>(identity.database_id);
		REMOVE_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
		object->g_key = item.generated_key;
		object->weight = item.weight;
		object->cost = item.cost;
		if (complete_snapshot_state ||
		    (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME))
			for (size_t timer = 0; timer < item.timers.size(); ++timer)
				object->timer[timer] = static_cast<time_t>(item.timers[timer]);
		else
			object->timer[0] = static_cast<time_t>(item.timers[0]);
		object->extra_flags = item.extra_flags;
		if (complete_snapshot_state ||
		    (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME))
		{
			object->anti_flags = item.anti_flags;
			object->anti2_flags = item.anti2_flags;
			object->craftsmanship = item.craftsmanship;
		}
		if (complete_snapshot_state ||
		    (identity.override_mask &
		     (PLAYER_LOAD_ITEM_OVERRIDE_EXTRA2_FLAGS | PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME)))
			object->extra2_flags = item.extra2_flags;
		if (complete_snapshot_state ||
		    (identity.override_mask & (PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS |
					       PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME)))
		{
			const auto baseline = std::find_if(
				item.dynamic_affects.begin(), item.dynamic_affects.end(),
				[](const auto &affect)
				{ return affect.type == TAG_ALTERED_EXTRA2; });
			if (baseline != item.dynamic_affects.end())
				object->extra2_flags = static_cast<ulong>(baseline->extra2);
			for (auto affect = item.dynamic_affects.rbegin();
			     affect != item.dynamic_affects.rend(); ++affect)
			{
				if (affect->type == TAG_ALTERED_EXTRA2)
					continue;
				if (affect->extra2)
					set_obj_affected_extra(object, -1,
							       static_cast<sh_int>(affect->type),
							       static_cast<sh_int>(affect->data),
							       static_cast<ulong>(affect->extra2));
				else
					set_obj_affected(object, -1,
							 static_cast<sh_int>(affect->type),
							 static_cast<sh_int>(affect->data));
			}
		}
		object->condition = item.condition;
		for (size_t value_index = 0; value_index < item.values.size(); ++value_index)
			object->value[value_index] = item.values[value_index];
		if (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_WEAR_FLAGS)
			object->wear_flags = item.wear_flags;
		if (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_TYPE)
		{
			if (item.type == ITEM_CORPSE && object->type != ITEM_CORPSE)
				return fail(metrics,
					    player_load_item_materialize_outcome::invalid_snapshot);
			object->type = item.type;
		}
		if (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_MATERIAL)
			object->material = item.material;
		unsigned long *bitvectors[] = { &object->bitvector, &object->bitvector2,
						&object->bitvector3, &object->bitvector4,
						&object->bitvector5 };
		for (size_t bitvector = 0; bitvector < item.bitvectors.size(); ++bitvector)
			if (identity.override_mask &
			    (PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR1 << bitvector))
				*bitvectors[bitvector] =
					static_cast<unsigned long>(item.bitvectors[bitvector]);
		if (identity.override_mask & PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS)
			for (size_t affect = 0; affect < item.affects.size(); ++affect)
			{
				object->affected[affect].location = item.affects[affect][0];
				object->affected[affect].modifier = item.affects[affect][1];
			}
		apply_saved_strings(object, item);
		apply_extra_descriptions(object, item);
		try
		{
			ownership.push_back({ identity.item_uid, identity.root_item_uid,
					      identity.parent_item_uid, identity.owner,
					      identity.item_revision, identity.owner_revision,
					      item.vnum, identity.state });
		}
		catch (const std::bad_alloc &)
		{
			return fail(metrics,
				    player_load_item_materialize_outcome::allocation_failure);
		}
		if (!count_operation(metrics, item_count))
			return fail(metrics, player_load_item_materialize_outcome::limit_exceeded);
	}

	std::vector<P_obj> child_tails;
	try
	{
		child_tails.assign(item_count, nullptr);
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_item_materialize_outcome::allocation_failure);
	}
	for (size_t index = 0; index < item_count; ++index)
	{
		const int32_t parent_index = items[index].parent_index;
		if (parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			staged.roots[index] = true;
			continue;
		}
		if (!obj_can_nest(staged.objects[index],
				  staged.objects[static_cast<size_t>(parent_index)]))
			return fail(metrics,
				    player_load_item_materialize_outcome::invalid_snapshot);
	}
	for (size_t index = 0; index < item_count; ++index)
	{
		const int32_t parent_index = items[index].parent_index;
		if (parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			continue;
		P_obj child = staged.objects[index];
		P_obj parent = staged.objects[static_cast<size_t>(parent_index)];
		child->loc_p = LOC_INSIDE;
		child->loc.inside = parent;
		child->next_content = nullptr;
		if (child_tails[parent_index])
			child_tails[parent_index]->next_content = child;
		else
			parent->contains = child;
		child_tails[parent_index] = child;
		if (!count_operation(metrics, item_count))
			return fail(metrics, player_load_item_materialize_outcome::limit_exceeded);
	}
	staged.linked = true;
	for (auto index = traversal.rbegin(); index != traversal.rend(); ++index)
	{
		recalc_container_weight(staged.objects[*index]);
		if (!count_operation(metrics, item_count))
			return fail(metrics, player_load_item_materialize_outcome::limit_exceeded);
	}

	if (hydrate_ownership &&
	    !item_ownership_runtime_hydrate_batch(ownership.data(), ownership.size()))
		return fail(metrics, player_load_item_materialize_outcome::ownership_failure);
	if (detached)
		for (size_t root : roots)
			detached_roots->push_back(staged.objects[root]);
	else
		attach_loaded_inventory(character, staged.objects, roots, items);
	staged.published = true;
	metrics->outcome = player_load_item_materialize_outcome::applied;
	return true;
}
}

bool player_load_item_graph_materialize_for_owner(
	P_char character, const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities,
	const item_owner_identity &expected_owner, uint64_t owner_revision, bool hydrate_ownership,
	bool complete_snapshot_state, player_load_item_materialize_metrics *metrics)
{
	return materialize_item_graph(character, nullptr, items, identities, expected_owner,
				      owner_revision, hydrate_ownership, complete_snapshot_state,
				      metrics);
}

bool player_load_item_graph_materialize_detached(
	const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities,
	const item_owner_identity &expected_owner, uint64_t owner_revision, bool hydrate_ownership,
	bool complete_snapshot_state, std::vector<P_obj> *roots,
	player_load_item_materialize_metrics *metrics)
{
	return materialize_item_graph(nullptr, roots, items, identities, expected_owner,
				      owner_revision, hydrate_ownership, complete_snapshot_state,
				      metrics);
}

bool player_load_item_graph_materialize_creation(const item_transfer_payload &payload,
						 const item_transfer_result &result,
						 std::vector<P_obj> *roots)
{
	if (!roots || !item_owner_identity_valid(payload.from_owner) ||
	    payload.from_owner.type != item_owner_type::system ||
	    payload.to_owner.type != item_owner_type::player ||
	    !item_owner_identity_valid(payload.to_owner) ||
	    payload.reason != item_transfer_reason::creation || !payload.multi_root ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS ||
	    result.item_count != payload.item_count ||
	    result.root_item_uid != item_transfer_result_root(payload) ||
	    !result.to_owner_revision || payload.item_blob_size > payload.item_blob.size() ||
	    !payload.item_blob_size || payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid)
		return false;

	std::vector<player_item_snapshot> items;
	std::vector<player_load_item_identity> identities;
	std::unordered_map<uint64_t, size_t> snapshot_indices;
	try
	{
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &items) != player_snapshot_codec_result::ok ||
		    items.size() != payload.item_count)
			return false;
		for (player_item_snapshot &item : items)
			item.equipment_slot = -1;
		snapshot_indices.reserve(items.size());
		identities.reserve(items.size());
		for (size_t index = 0; index < items.size(); ++index)
		{
			const player_item_snapshot &item = items[index];
			if (!item.object_uid || item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(items.size()) ||
			    !snapshot_indices.emplace(item.object_uid, index).second)
				return false;
		}
		for (size_t index = 0; index < items.size(); ++index)
		{
			const player_item_snapshot &item = items[index];
			const auto entry = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const item_transfer_entry &candidate)
				{ return candidate.item_uid == item.object_uid; });
			if (entry == payload.items.begin() + payload.item_count ||
			    entry->vnum <= 0 || entry->vnum != item.vnum ||
			    entry->expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
			    entry->expected_state != item_custody_state::absent)
				return false;
			const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
			if ((root &&
			     (entry->parent_item_uid || entry->root_item_uid != entry->item_uid)) ||
			    (!root &&
			     entry->parent_item_uid !=
				     items[static_cast<size_t>(item.parent_index)].object_uid))
				return false;
			player_load_item_identity identity = {};
			identity.database_id = index + 1;
			identity.serialized_parent_id =
				root ? 0 : static_cast<uint64_t>(item.parent_index) + 1;
			identity.quantity = 1;
			identity.override_mask = PLAYER_LOAD_ITEM_OVERRIDE_ALL;
			identity.item_uid = entry->item_uid;
			identity.root_item_uid = entry->root_item_uid;
			identity.parent_item_uid = entry->parent_item_uid;
			identity.owner = payload.to_owner;
			identity.item_revision = 1;
			identity.owner_revision = result.to_owner_revision;
			identity.state = item_custody_state::active;
			identities.push_back(identity);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	player_load_item_materialize_metrics metrics = {};
	return player_load_item_graph_materialize_detached(items, identities, payload.to_owner,
							   result.to_owner_revision, false, true,
							   roots, &metrics);
}

bool player_load_item_graph_materialize(P_char character,
					const std::vector<player_item_snapshot> &items,
					const std::vector<player_load_item_identity> &identities,
					int32_t pid, uint64_t owner_revision,
					bool hydrate_ownership,
					player_load_item_materialize_metrics *metrics)
{
	if (pid <= 0)
	{
		if (metrics)
		{
			*metrics = {};
			metrics->item_count = items.size();
			metrics->outcome = player_load_item_materialize_outcome::invalid_snapshot;
		}
		return false;
	}
	return player_load_item_graph_materialize_for_owner(
		character, items, identities,
		{ item_owner_type::player, static_cast<uint64_t>(pid), 0 }, owner_revision,
		hydrate_ownership, false, metrics);
}

void player_load_item_runtime_state_apply(P_obj object, const player_item_snapshot &item)
{
	object->g_key = item.generated_key;
	for (size_t timer = 1; timer < item.timers.size(); ++timer)
		object->timer[timer] = static_cast<time_t>(item.timers[timer]);
	object->anti_flags = item.anti_flags;
	object->anti2_flags = item.anti2_flags;
	object->extra2_flags = item.extra2_flags;
	object->craftsmanship = item.craftsmanship;
	for (auto affect = item.dynamic_affects.rbegin(); affect != item.dynamic_affects.rend();
	     ++affect)
	{
		if (affect->type == TAG_ALTERED_EXTRA2)
			continue;
		if (affect->extra2)
			set_obj_affected_extra(object, -1, static_cast<sh_int>(affect->type),
					       static_cast<sh_int>(affect->data),
					       static_cast<ulong>(affect->extra2));
		else
			set_obj_affected(object, -1, static_cast<sh_int>(affect->type),
					 static_cast<sh_int>(affect->data));
	}
}

bool player_load_items_materialize(P_char character, const player_load_result &result,
				   player_load_item_materialize_metrics *metrics)
{
	return player_load_item_graph_materialize(character, result.snapshot.items,
						  result.item_identities, result.pid,
						  result.item_owner_revision, true, metrics);
}

void player_load_items_activate_equipment(P_char character)
{
	if (!character)
		return;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		P_obj object = character->equipment[slot];
		obj_affect *affect = object ? get_obj_affect(object, SKILL_ENCHANT) : nullptr;
		if (!affect)
			continue;
		act("&+YA magical aura forms around your body.&n", FALSE, character, object, 0,
		    TO_CHAR);
		((*skills[affect->data].spell_pointer)(static_cast<int>(GET_LEVEL(character)),
						       character, 0, SPELL_TYPE_SPELL, character,
						       0));
	}
}

void player_load_items_discard(P_char character)
{
	if (!character)
		return;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (character->equipment[slot])
		{
			P_obj object = character->equipment[slot];
			character->equipment[slot] = nullptr;
			object->loc_p = LOC_NOWHERE;
			object->loc.wearing = nullptr;
			clear_item_uids(object);
			extract_obj(object, FALSE);
		}
	while (character->carrying)
	{
		P_obj object = character->carrying;
		character->carrying = object->next_content;
		object->next_content = nullptr;
		object->loc_p = LOC_NOWHERE;
		object->loc.carrying = nullptr;
		clear_item_uids(object);
		extract_obj(object, FALSE);
	}
	GET_CARRYING_W(character) = 0;
	IS_CARRYING_N(character) = 0;
}

bool player_load_item_snapshot_metadata_valid(const player_item_snapshot &item)
{
	player_load_item_identity identity = {};
	identity.database_id = 1;
	identity.item_uid = item.object_uid;
	identity.override_mask = PLAYER_LOAD_ITEM_OVERRIDE_ALL;
	return valid_item_metadata(item, identity, true) == metadata_validation_outcome::valid;
}

// Private original SHOP counterpart of the existing persisted-load metadata
// and spellbook policy. It never calls read_object, normal instantiation,
// set_obj_affected, UID issuance, property conversion or a native callback.
// All persisted dynamic-node ordering and timer bytes are literal, not rerolled.
bool shop_trade_original_item_stage::prepare(const object_template &prototype,
					     const player_item_snapshot &literal,
					     shop_trade_original_item_stage &output) noexcept
{
	try
	{
		if (!literal.object_uid || !std::in_range<unsigned long>(literal.object_uid) ||
		    !std::in_range<long>(literal.generated_key) || literal.vnum <= 0 ||
		    literal.type < ITEM_LOWEST || literal.type > ITEM_LAST ||
		    literal.equipment_slot ||
		    literal.string_mask !=
			    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
		    !player_load_item_snapshot_metadata_valid(literal))
			return false;
		// Complete full-literal SHOP capture uses these exact four strings. No
		// prototype default is substituted for a missing original string.
		for (const auto *text : { &literal.name, &literal.short_description,
					  &literal.description, &literal.action_description })
			if (text->size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
			    text->find('\0') != std::string::npos)
				return false;
		for (const auto &affect : literal.affects)
			if (!std::in_range<decltype(std::declval<obj_data &>().affected[0].location)>(
				    affect[0]) ||
			    !std::in_range<decltype(std::declval<obj_data &>().affected[0].modifier)>(
				    affect[1]))
				return false;
		for (auto value : literal.bitvectors)
			if (!std::in_range<unsigned long>(value))
				return false;
		for (auto timer : literal.timers)
			if (!std::in_range<time_t>(timer))
				return false;
		player_item_snapshot decoded = literal;
		for (auto &description : decoded.extra_descriptions)
		{
			if (description.spellbook)
			{
				std::array<char, (MAX_SKILLS + 1) / 8 + 1> bits{};
				if (!decode_saved_spellbook(description, bits.data()))
					return false;
				description.keyword.assign("\3\1\3", 3);
				description.description.assign(bits.data(), bits.size());
			}
			else if (description.keyword.find('\0') != std::string::npos ||
				 description.description.find('\0') != std::string::npos)
				return false;
		}
		// Cold SHOP restoration may be the first object allocation after mm_create.
		// Reserve real native capacity before unpublished literal staging, without
		// changing the generic inert/money allocator's free-slot-only contract.
		extern mm_ds *dead_obj_pool;
		if (!dead_obj_pool || dead_obj_pool->size != sizeof(obj_data) ||
		    dead_obj_pool->next_off != offsetof(obj_data, next) ||
		    !mm_try_reserve_free_slot(dead_obj_pool))
			return false;
		inert_item_stage raw;
		if (inert_item_stage::allocate_literal(prototype, decoded, raw) !=
		    inert_item_stage_result::ok)
			return false;
		shop_trade_original_item_stage candidate;
		candidate.object_ = std::exchange(raw.object_, nullptr);
		candidate.pool_ = std::exchange(raw.pool_, nullptr);
		// Trap machinery is omitted by the existing persisted snapshot policy.
		// Keep its established sealed-template reload policy, never asserting
		// these defaults are original economic/source/runtime-state evidence.
		candidate.object_->trap_eff = prototype.trap_eff;
		candidate.object_->trap_dam = prototype.trap_dam;
		candidate.object_->trap_charge = prototype.trap_charge;
		candidate.object_->trap_level = prototype.trap_level;
		if (!literal.dynamic_affects.empty())
		{
			// Keep native pool ownership: obj_affect_remove returns consumed nodes
			// to this existing pool. All creation/growth happens before enrollment.
			extern mm_ds *dead_obj_affect_pool;
			if (!dead_obj_affect_pool)
				dead_obj_affect_pool = mm_create("OBJ_AFFECTS", sizeof(obj_affect),
								 offsetof(obj_affect, next), 100);
			candidate.affect_pool_ = dead_obj_affect_pool;
			obj_affect **tail = &candidate.object_->affects;
			for (const auto &saved : literal.dynamic_affects)
			{
				auto *node =
					static_cast<obj_affect *>(mm_get(candidate.affect_pool_));
				if (!node)
					return false;
				node->type = saved.type;
				node->data = saved.data;
				node->extra2 = static_cast<ulong>(saved.extra2);
				node->next = nullptr;
				*tail = node;
				tail = &node->next;
			}
		}
		output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

// Four individually retained post-consumption native steps. Full allocating
// literal forest consumption precedes every call, and the caller re-censuses
// original identities/SQL/world after each one. Existing saved-policy ephemeral
// scheduling does not provide UID/economic/source authority or change literals.
bool shop_trade_original_item_stage::reload_step(P_obj object, const object_template &prototype,
						 unsigned int step,
						 shop_trade_original_reload_effect &effect) noexcept
{
	if (!nevent_is_game_thread() || !object || object->R_num != prototype.R_num ||
	    effect.started || step > 3)
		return false;
	try
	{
		extern P_index obj_index;
		extern int top_of_objt;
		extern void event_object_proc(P_char, P_char, P_obj, void *);
		extern void proclib_obj_event(P_char, P_char, P_obj, void *);
		extern void event_random_exit(P_char, P_char, P_obj, void *);
		if (!obj_index || object->R_num < 0 || object->R_num > top_of_objt)
			return false;
		if (find_recovery_object_template(obj_index[object->R_num].virtual_number) !=
		    &prototype)
			return false;
		const auto proc = obj_index[object->R_num].func.obj;
		// Original binding was separately committed under the complete owner cut.
		// Callback phases never bind procedures or duplicate saved descriptions.
		if ((object->type == ITEM_SWITCH && !proc) ||
		    (IS_SET(object->extra_flags, ITEM_PROCLIB) && proc != proclib_obj_cmd_bridge))
			return false;
		effect.started = true;
		switch (step)
		{
		case 0:
			effect.periodic = proc && invoke_object_special(object, nullptr,
									CMD_SET_PERIODIC, nullptr);
			effect.succeeded = true;
			break;
		case 1:
			effect.succeeded =
				!effect.periodic || get_scheduled(object, event_object_proc) ||
				add_event(event_object_proc, PULSE_MOBILE + number(-4, 4), nullptr,
					  nullptr, object, 0, nullptr, 0)
					.was_scheduled();
			break;
		case 2:
			// Restored saved proclib parameter descriptions are already literal.
			// Each saved library's normal eligibility probe was retained before
			// this separate schedule step. No parser/new description is invoked.
			// Command-only proclibs do not gain periodic events.
			effect.succeeded =
				!effect.periodic || get_scheduled(object, proclib_obj_event) ||
				add_event(proclib_obj_event, PULSE_MOBILE + number(-4, 4), nullptr,
					  nullptr, object, 0, nullptr, 0)
					.was_scheduled();
			break;
		case 3:
			effect.succeeded = !isname("random_exit", object->name) ||
					   get_scheduled(object, event_random_exit) ||
					   add_event(event_random_exit, 3, nullptr, nullptr, object,
						     0, nullptr, 0)
						   .was_scheduled();
			break;
		}
		effect.returned = true;
		return effect.succeeded;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_original_item_stage::proclib_probe(
	P_obj object, const object_template &prototype, size_t description_index,
	shop_trade_original_reload_effect &effect) noexcept
{
	if (!nevent_is_game_thread() || !object || object->R_num != prototype.R_num ||
	    effect.started)
		return false;
	try
	{
		extern P_index obj_index;
		extern int top_of_objt;
		if (!obj_index || object->R_num < 0 || object->R_num > top_of_objt ||
		    find_recovery_object_template(obj_index[object->R_num].virtual_number) !=
			    &prototype)
			return false;
		effect.started = true;
		bool requested = false;
		if (IS_SET(object->extra_flags, ITEM_PROCLIB) &&
		    !proclib_saved_periodic_probe(object, description_index, &requested))
			return false;
		effect.periodic = requested;
		effect.returned = effect.succeeded = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

// Birth-only complete persisted literal hydration. Existing SHOP/legacy bodies
// are unchanged; reuse their actual raw allocator and metadata/spellbook rules.
bool native_mobile_birth_literal_stage::prepare(const object_template &prototype,
						const player_item_snapshot &literal,
						native_mobile_birth_literal_stage &output) noexcept
{
	try
	{
		if (!literal.object_uid || !std::in_range<unsigned long>(literal.object_uid) ||
		    !std::in_range<long>(literal.generated_key) || literal.vnum <= 0 ||
		    literal.type < ITEM_LOWEST || literal.type > ITEM_LAST ||
		    literal.string_mask !=
			    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
		    !player_load_item_snapshot_metadata_valid(literal))
			return false;
		// Complete full-literal SHOP capture uses these exact four strings. No
		// prototype default is substituted for a missing original string.
		for (const auto *text : { &literal.name, &literal.short_description,
					  &literal.description, &literal.action_description })
			if (text->size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
			    text->find('\0') != std::string::npos)
				return false;
		for (const auto &affect : literal.affects)
			if (!std::in_range<decltype(std::declval<obj_data &>().affected[0].location)>(
				    affect[0]) ||
			    !std::in_range<decltype(std::declval<obj_data &>().affected[0].modifier)>(
				    affect[1]))
				return false;
		for (auto value : literal.bitvectors)
			if (!std::in_range<unsigned long>(value))
				return false;
		for (auto timer : literal.timers)
			if (!std::in_range<time_t>(timer))
				return false;
		player_item_snapshot decoded = literal;
		for (auto &description : decoded.extra_descriptions)
		{
			if (description.spellbook)
			{
				std::array<char, (MAX_SKILLS + 1) / 8 + 1> bits{};
				if (!decode_saved_spellbook(description, bits.data()))
					return false;
				description.keyword.assign("\3\1\3", 3);
				description.description.assign(bits.data(), bits.size());
			}
			else if (description.keyword.find('\0') != std::string::npos ||
				 description.description.find('\0') != std::string::npos)
				return false;
		}
		// Cold birth replay may be the first object allocation after mm_create.
		// Reserve native capacity only after complete literal validation; keep
		// the generic inert/money allocator's free-slot-only contract unchanged.
		extern mm_ds *dead_obj_pool;
		if (!dead_obj_pool || dead_obj_pool->size != sizeof(obj_data) ||
		    dead_obj_pool->next_off != offsetof(obj_data, next) ||
		    !mm_try_reserve_free_slot(dead_obj_pool))
			return false;
		inert_item_stage raw;
		if (inert_item_stage::allocate_literal(prototype, decoded, raw) !=
		    inert_item_stage_result::ok)
			return false;
		native_mobile_birth_literal_stage candidate;
		candidate.object_ = std::exchange(raw.object_, nullptr);
		candidate.pool_ = std::exchange(raw.pool_, nullptr);
		// Exact trap values come from the original birth recipe, never this template.
		if (!literal.dynamic_affects.empty())
		{
			// Keep native pool ownership: obj_affect_remove returns consumed nodes
			// to this existing pool. All creation/growth happens before enrollment.
			extern mm_ds *dead_obj_affect_pool;
			if (!dead_obj_affect_pool)
				dead_obj_affect_pool = mm_create("OBJ_AFFECTS", sizeof(obj_affect),
								 offsetof(obj_affect, next), 100);
			candidate.affect_pool_ = dead_obj_affect_pool;
			obj_affect **tail = &candidate.object_->affects;
			for (const auto &saved : literal.dynamic_affects)
			{
				auto *node =
					static_cast<obj_affect *>(mm_get(candidate.affect_pool_));
				if (!node)
					return false;
				node->type = saved.type;
				node->data = saved.data;
				node->extra2 = static_cast<ulong>(saved.extra2);
				node->next = nullptr;
				*tail = node;
				tail = &node->next;
			}
		}
		if (output.object_)
			return false;
		output.object_ = std::exchange(candidate.object_, nullptr);
		output.pool_ = std::exchange(candidate.pool_, nullptr);
		output.affect_pool_ = std::exchange(candidate.affect_pool_, nullptr);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

native_mobile_birth_literal_stage::~native_mobile_birth_literal_stage() noexcept
{
	reset();
}
void native_mobile_birth_literal_stage::reset() noexcept
{
	if (!object_)
		return;
	for (obj_affect *affect = object_->affects; affect;)
	{
		obj_affect *next = affect->next;
		mm_release(affect_pool_, affect);
		affect = next;
	}
	object_->affects = nullptr;
	inert_item_stage literal;
	literal.object_ = std::exchange(object_, nullptr);
	literal.pool_ = std::exchange(pool_, nullptr);
	affect_pool_ = nullptr;
}
