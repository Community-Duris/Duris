#include "player/player_load_items.h"

#include "net/comm.h"
#include "item_ownership_runtime.h"
#include "prototypes.h"
#include "magic/spells.h"
#include "structs.h"
#include "utils.h"

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
		uint64_t value = 0;
		while (position < json.size() && json[position] >= '0' && json[position] <= '9')
		{
			value = value * 10 + static_cast<unsigned int>(json[position++] - '0');
			if (value >= MAX_SKILLS)
				return false;
		}
		if (seen[value])
			return false;
		seen[value] = true;
		if (spell_bits)
			spell_bits[value / 8] |= static_cast<char>(1U << (value % 8));
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
						STRUNG_DESC3;
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
	if (complete_snapshot_state)
		for (const auto &affect : item.dynamic_affects)
			if (affect.extra2 > ULONG_MAX)
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
			if (!descriptions.insert(std::move(key)).second)
				return metadata_validation_outcome::invalid;
			if (description.spellbook)
				if (!parse_spellbook(description.description))
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
		if (candidate.spellbook &&
		    !parse_spellbook(candidate.description, spell_bits.data()))
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
			parse_spellbook(description->description, entry->description);
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
		const int slot = items[index].equipment_slot;
		if (slot > 0)
		{
			character->equipment[slot - 1] = object;
			object->loc.wearing = character;
			object->loc_p = LOC_WORN;
			if (IS_PC(character) && GET_ITEM_TYPE(object) == ITEM_ARMOR)
				character->only.pc->prestige += object->value[2];
			GET_CARRYING_W(character) += GET_OBJ_WEIGHT(object) / 2;
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
			GET_CARRYING_W(character) += GET_OBJ_WEIGHT(object);
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
				 0);
		if (!count_operation(metrics, item_count, 2 + metadata_operations) ||
		    !identity.database_id || !identity.item_uid ||
		    identity.item_uid != item.object_uid || !identity.root_item_uid ||
		    identity.quantity != 1 || identity.state != item_custody_state::active ||
		    !item_owner_identity_equal(identity.owner, expected_owner) ||
		    identity.owner_revision != owner_revision ||
		    identity.override_mask & ~PLAYER_LOAD_ITEM_OVERRIDE_ALL)
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
		const metadata_validation_outcome metadata =
			valid_item_metadata(item, identity, complete_snapshot_state);
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
		object->g_key = item.generated_key;
		object->weight = item.weight;
		object->cost = item.cost;
		if (complete_snapshot_state)
			for (size_t timer = 0; timer < item.timers.size(); ++timer)
				object->timer[timer] = static_cast<time_t>(item.timers[timer]);
		else
			object->timer[0] = static_cast<time_t>(item.timers[0]);
		object->extra_flags = item.extra_flags;
		if (complete_snapshot_state)
		{
			object->anti_flags = item.anti_flags;
			object->anti2_flags = item.anti2_flags;
			object->extra2_flags = item.extra2_flags;
			object->craftsmanship = item.craftsmanship;
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
