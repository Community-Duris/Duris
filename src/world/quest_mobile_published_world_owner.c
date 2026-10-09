#include "world/quest_mobile_published_world_owner.h"
#include "world/db.h"
#include "world/quest_mobile_native_birth.h"
#include "world/native_mobile_birth_artifact.h"
#include "world/native_mobile_birth_procedure.h"
#include "world/native_mobile_birth_reset_tail.h"
#include "world/events.h"
#include "world/handler.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/encumbrance_policy.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#endif
#include <algorithm>
#include <climits>
#include <cstdio>
#include <utility>
#include <cerrno>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include "persistence/economic_sql_lifecycle_guard.h"

extern P_char character_list;
extern P_obj object_list;
extern index_data *obj_index;
extern int top_of_objt;
extern P_room world;
extern int top_of_world;
extern P_index mob_index;
extern int top_of_mobt;

namespace
{
bool ordinary_wallet_origin(const critical_native_recovery_envelope &original) noexcept
{
	return original.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION;
}
bool published_origin_terminal(const critical_native_recovery_envelope &original) noexcept
{
	return ordinary_wallet_origin(original) ?
		       native_mobile_birth_cash_role_recovery_terminal(original) :
		       native_mobile_birth_recovery_terminal(original);
}
bool ordinary_role_selector_current(const quest_mobile_native_constructor_recipe &constructor,
				    const native_mobile_birth_cash_role_recipe &role) noexcept
{
	if (role.role != native_mobile_birth_cash_role::ordinary_wallet)
		return false;
	native_mobile_birth_cash_role_recipe observed;
	native_mobile_birth_cash_role_recipe_bytes expected{}, current{};
	return native_mobile_birth_cash_role_recipe_capture(constructor, &observed) &&
	       native_mobile_birth_cash_role_recipe_encode(role, &expected) &&
	       native_mobile_birth_cash_role_recipe_encode(observed, &current) &&
	       expected == current;
}
bool ordinary_origin_selector_current(const critical_native_recovery_envelope &original) noexcept
{
	if (!ordinary_wallet_origin(original))
		return true;
	try
	{
		quest_mobile_native_image born;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		return native_mobile_birth_cash_role_command_decode(original.command, &born,
								    &recipes, &role) ==
			       economic_accounting_error::ok &&
		       ordinary_role_selector_current(role.original, role);
	}
	catch (...)
	{
		return false;
	}
}

void report_world_refusal(unsigned int stage) noexcept
{
	static bool reported = false;
	if (!reported)
	{
		reported = true;
		std::fprintf(stderr, "world-recovery-refusal stage=%u\n", stage);
	}
}
bool same_custody(const item_ownership_runtime_entry &a, const item_ownership_runtime_entry &b)
{
	return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.owner_revision == b.owner_revision && a.vnum == b.vnum && a.state == b.state;
}
bool same_image(const quest_mobile_native_image &a, const quest_mobile_native_image &b)
{
	std::vector<uint8_t> x, y;
	return quest_mobile_native_image_encode(a, &x) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_image_encode(b, &y) == player_snapshot_codec_result::ok &&
	       x == y;
}
bool add(size_t &bytes, size_t count, size_t width = 1)
{
	if (width && count > (SIZE_MAX - bytes) / width)
		return false;
	bytes += count * width;
	return true;
}
bool lists_valid()
{
	for (P_char slow = character_list, fast = character_list; fast && fast->next;)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	P_obj previous = nullptr;
	for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	for (P_obj item = object_list; item; item = item->next)
	{
		if (item->prev != previous)
			return false;
		previous = item;
	}
	return true;
}
}

unsigned int quest_mobile_published_world_owner::observe_maintenance(
	const economic_sql_lifecycle_guard &authority, maintenance_world_report *output) noexcept
{
	return observe_maintenance_impl(&authority, nullptr, output);
}

unsigned int quest_mobile_published_world_owner::observe_maintenance(
	economic_sql_cutover_transaction_owner &owner, maintenance_world_report *output) noexcept
{
	return observe_maintenance_impl(nullptr, &owner, output);
}

unsigned int quest_mobile_published_world_owner::observe_maintenance_impl(
	const economic_sql_lifecycle_guard *authority,
	economic_sql_cutover_transaction_owner *owner, maintenance_world_report *output) noexcept
{
	if (!output || (authority == nullptr) == (owner == nullptr))
		return EINVAL;
	const auto held = [&]() noexcept
	{
		return authority ? authority->is_maintenance_authority() :
				   owner->maintenance_ && owner->writer_lock_ && owner->is_valid();
	};
	if (!nevent_is_game_thread() || !held())
		return EINVAL;
	try
	{
		constexpr size_t maximum_rows = 262144;
		maintenance_world_report candidate;
		std::vector<P_char> actors;
		std::vector<P_obj> objects;
		std::unordered_map<P_char, size_t> actor_index;
		std::unordered_map<P_obj, size_t> object_index;
		const auto charge = [&](size_t count, size_t width = 1)
		{
			if (!add(candidate.retained_bytes, count, width) ||
			    candidate.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES)
				throw E2BIG;
		};
		const auto charge_items = [&](const std::vector<player_item_snapshot> &items)
		{
			charge(items.capacity(), sizeof(player_item_snapshot));
			for (const auto &item : items)
			{
				for (const auto *text :
				     { &item.name, &item.short_description, &item.description,
				       &item.action_description })
				{
					charge(text->capacity());
					charge(1);
				}
				charge(item.dynamic_affects.capacity(),
				       sizeof(item.dynamic_affects[0]));
				charge(item.extra_descriptions.capacity(),
				       sizeof(item.extra_descriptions[0]));
				for (const auto &description : item.extra_descriptions)
				{
					charge(description.keyword.capacity());
					charge(description.description.capacity());
					charge(2);
					charge(description.spell_ids.capacity(),
					       sizeof(description.spell_ids[0]));
				}
			}
		};
		charge(1, sizeof(candidate));
		for (P_char actor = character_list; actor; actor = actor->next)
		{
			if (actor_index.count(actor))
			{
				candidate.character_list_complete = false;
				candidate.issues |= list_cycle;
				break;
			}
			if (actors.size() == maximum_rows)
				return E2BIG;
			actor_index.emplace(actor, actors.size());
			actors.push_back(actor);
		}
		charge(actors.size(), sizeof(maintenance_character));
		candidate.characters.reserve(actors.size());
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (object_index.count(object))
			{
				candidate.object_list_complete = false;
				candidate.issues |= list_cycle;
				break;
			}
			if (objects.size() == maximum_rows)
				return E2BIG;
			object_index.emplace(object, objects.size());
			objects.push_back(object);
			maintenance_object value;
			value.uid = object->obj_uid;
			value.location = object->loc_p;
			if (object->prev != previous)
				value.issues |= invalid_placement;
			charge(1, sizeof(maintenance_object));
			candidate.objects.push_back(value);
			previous = object;
		}
		charge(candidate.objects.capacity() - candidate.objects.size(),
		       sizeof(maintenance_object));
		candidate.world_tables_present = world && top_of_world >= 0 && obj_index &&
						 top_of_objt >= 0;
		if (world && top_of_world >= 0 && static_cast<size_t>(top_of_world) >= maximum_rows)
			return E2BIG;
		std::unordered_map<uint64_t, size_t> runtimes, natives, uids;
		std::vector<std::pair<critical_operation_id, size_t>> births;
		for (P_char actor : actors)
		{
			maintenance_character value;
			value.runtime = actor->runtime_id;
			value.npc = IS_NPC(actor);
			value.alive = IS_ALIVE(actor);
			value.room = actor->in_room;
			value.denominations = { GET_COPPER(actor), GET_SILVER(actor),
						GET_GOLD(actor), GET_PLATINUM(actor) };
			if (!value.npc && IS_PC(actor) && actor->only.pc && GET_PID(actor) > 0)
				value.observed_owner =
					item_owner_identity{ item_owner_type::player,
							     static_cast<uint64_t>(GET_PID(actor)),
							     0 };
			if (world && actor->in_room >= 0 && actor->in_room <= top_of_world)
				value.room_vnum = world[actor->in_room].number;
			else
				value.issues |= invalid_placement;
			if (value.npc && actor->only.npc && mob_index && GET_RNUM(actor) >= 0 &&
			    GET_RNUM(actor) <= top_of_mobt)
				value.mobile_vnum = mob_index[GET_RNUM(actor)].virtual_number;
			else if (value.npc)
				value.issues |= invalid_identity;
			bool present = false;
			quest_mobile_native_reference reference;
			if (!quest_mobile_native_birth_owner::observe_current_published_identity(
				    actor, value.runtime, &present, &reference))
			{
				value.identity = maintenance_identity::malformed;
				value.issues |= malformed_binding;
			}
			else if (present)
			{
				value.identity = maintenance_identity::published;
				value.reference = reference;
				quest_mobile_native_cash_reference cash;
				if (quest_mobile_native_cash_reference_copy(actor, value.runtime,
									    &cash))
					value.cash_binding = cash;
				else
					value.issues |= unknown_cash_binding;
				uint64_t revision = 0;
				const item_owner_identity owner{ item_owner_type::native_mobile,
								 reference.mobile_instance_id, 0 };
				value.observed_owner = owner;
				if (item_ownership_runtime_peek_owner_revision(owner, &revision))
					value.owner_revision = revision;
				else
					value.issues |= missing_cache;
				if (!value.owner_revision || !*value.owner_revision ||
				    *value.owner_revision != reference.stock_revision)
					value.issues |= conflicting_cache;
				const auto [found, unique] = natives.emplace(
					reference.mobile_instance_id, candidate.characters.size());
				if (!unique)
				{
					value.issues |= duplicate_identity;
					candidate.characters[found->second].issues |=
						duplicate_identity;
				}
				births.emplace_back(reference.birth_operation,
						    candidate.characters.size());
			}
			if (!value.runtime || find_character_by_runtime_id(value.runtime) != actor)
				value.issues |= invalid_identity;
			const auto [found, unique] =
				runtimes.emplace(value.runtime, candidate.characters.size());
			if (!unique)
			{
				value.issues |= duplicate_identity;
				candidate.characters[found->second].issues |= duplicate_identity;
			}
			candidate.characters.push_back(std::move(value));
		}
		charge(candidate.characters.capacity() - candidate.characters.size(),
		       sizeof(maintenance_character));
		std::sort(births.begin(), births.end(), [](const auto &a, const auto &b)
			  { return a.first.bytes < b.first.bytes; });
		for (size_t i = 1; i < births.size(); ++i)
			if (critical_operation_id_equal(births[i - 1].first, births[i].first))
			{
				candidate.characters[births[i - 1].second].issues |=
					duplicate_identity;
				candidate.characters[births[i].second].issues |= duplicate_identity;
			}
		// Enumerate every actual physical list, including links to objects absent
		// from object_list. Never dereference a referenced pointer before membership.
		const auto retain_link = [&](maintenance_link link)
		{
			if (candidate.links.size() == maximum_rows)
				throw E2BIG;
			charge(1, sizeof(maintenance_link));
			candidate.links.push_back(std::move(link));
		};
		const auto list = [&](P_obj head, uint8_t location, size_t owner, int room)
		{
			std::unordered_set<P_obj> seen;
			for (P_obj member = head; member; member = member->next_content)
			{
				maintenance_link link;
				link.location = location;
				if (location == LOC_CARRIED)
					link.character = owner;
				else if (location == LOC_INSIDE)
					link.parent = owner;
				else
					link.room = room;
				const auto found = object_index.find(member);
				if (found == object_index.end())
				{
					candidate.issues |= foreign_link;
					candidate.physical_lists_complete = false;
					link.issues = foreign_link;
					retain_link(std::move(link));
					break;
				}
				auto &value = candidate.objects[found->second];
				link.object = found->second;
				if (!seen.insert(member).second)
				{
					value.issues |= list_cycle;
					candidate.physical_lists_complete = false;
					link.issues = list_cycle;
					retain_link(std::move(link));
					if (location == LOC_INSIDE)
						candidate.objects[owner].issues |= list_cycle;
					else if (location == LOC_CARRIED)
						candidate.characters[owner].issues |= list_cycle;
					break;
				}
				++value.physical_links;
				if (member->loc_p != location ||
				    (location == LOC_CARRIED &&
				     member->loc.carrying != actors[owner]) ||
				    (location == LOC_INSIDE &&
				     member->loc.inside != objects[owner]) ||
				    (location == LOC_ROOM && member->loc.room != room))
				{
					value.issues |= foreign_link;
					link.issues = foreign_link;
					if (location == LOC_INSIDE)
						candidate.objects[owner].issues |= foreign_link;
					else if (location == LOC_CARRIED)
						candidate.characters[owner].issues |= foreign_link;
				}
				retain_link(std::move(link));
			}
		};
		for (size_t i = 0; i < actors.size(); ++i)
		{
			list(actors[i]->carrying, LOC_CARRIED, i, -1);
			for (int slot = 0; slot < MAX_WEAR; ++slot)
				if (P_obj member = actors[i]->equipment[slot])
				{
					maintenance_link link;
					link.location = LOC_WORN;
					link.character = i;
					link.equipment_slot = static_cast<uint16_t>(slot + 1);
					const auto found = object_index.find(member);
					if (found == object_index.end())
					{
						candidate.characters[i].issues |= foreign_link;
						candidate.physical_lists_complete = false;
						link.issues = foreign_link;
						retain_link(std::move(link));
						continue;
					}
					auto &value = candidate.objects[found->second];
					link.object = found->second;
					++value.physical_links;
					value.equipment_slot = static_cast<uint16_t>(slot + 1);
					if (member->loc_p != LOC_WORN ||
					    member->loc.wearing != actors[i] ||
					    member->next_content)
					{
						value.issues |= foreign_link;
						candidate.characters[i].issues |= foreign_link;
						link.issues = foreign_link;
					}
					retain_link(std::move(link));
				}
		}
		size_t character_links = 0;
		if (world && top_of_world >= 0)
			for (int room = 0; room <= top_of_world; ++room)
			{
				list(world[room].contents, LOC_ROOM, 0, room);
				std::unordered_set<P_char> seen;
				for (P_char actor = world[room].people; actor;
				     actor = actor->next_in_room)
				{
					if (++character_links > maximum_rows)
						return E2BIG;
					const auto found = actor_index.find(actor);
					if (found == actor_index.end())
					{
						candidate.issues |= foreign_link;
						candidate.physical_lists_complete = false;
						break;
					}
					if (!seen.insert(actor).second)
					{
						candidate.characters[found->second].issues |=
							list_cycle;
						candidate.physical_lists_complete = false;
						break;
					}
					if (actor->in_room != room)
						candidate.characters[found->second].issues |=
							foreign_link;
					++candidate.characters[found->second].room_links;
				}
			}
		for (size_t i = 0; i < objects.size(); ++i)
			list(objects[i]->contains, LOC_INSIDE, i, -1);
		charge(candidate.links.capacity() - candidate.links.size(),
		       sizeof(maintenance_link));
		for (auto &actor : candidate.characters)
		{
			if (!actor.room_links)
				actor.issues |= missing_link;
			else if (actor.room_links != 1)
				actor.issues |= multiple_links;
		}
		for (size_t i = 0; i < objects.size(); ++i)
		{
			P_obj object = objects[i];
			auto &value = candidate.objects[i];
			if (!value.uid || value.uid == UINT64_MAX)
				value.issues |= invalid_identity;
			const auto [duplicate, unique] = uids.emplace(value.uid, i);
			if (!unique)
			{
				value.issues |= duplicate_identity;
				candidate.objects[duplicate->second].issues |= duplicate_identity;
			}
			if (obj_index && object->R_num >= 0 && object->R_num <= top_of_objt)
				value.vnum = obj_index[object->R_num].virtual_number;
			else
				value.issues |= invalid_identity;
			if (!value.physical_links)
				value.issues |= missing_link;
			else if (value.physical_links != 1)
				value.issues |= multiple_links;
			if (object->loc_p == LOC_INSIDE)
			{
				const auto parent = object_index.find(object->loc.inside);
				if (parent != object_index.end())
					value.parent = parent->second;
				else
					value.issues |= invalid_topology;
			}
			else if (object->loc_p == LOC_CARRIED || object->loc_p == LOC_WORN)
			{
				const auto owner = actor_index.find(object->loc_p == LOC_WORN ?
									    object->loc.wearing :
									    object->loc.carrying);
				if (owner != actor_index.end())
					value.character = owner->second;
				else
					value.issues |= invalid_placement;
			}
			else if (object->loc_p == LOC_ROOM && world && object->loc.room >= 0 &&
				 object->loc.room <= top_of_world)
				value.room_vnum = world[object->loc.room].number;
			else
				value.issues |= invalid_placement;
		}
		for (size_t i = 0; i < objects.size(); ++i)
		{
			auto &value = candidate.objects[i];
			size_t root = i, depth = 1;
			while (candidate.objects[root].parent)
			{
				root = *candidate.objects[root].parent;
				if (++depth > PLAYER_SNAPSHOT_MAX_DEPTH)
				{
					value.issues |= invalid_topology;
					break;
				}
			}
			if (candidate.objects[root].issues & invalid_topology)
				value.issues |= invalid_topology;
			if (!(value.issues & invalid_topology))
				value.root = root;
		}
		const auto cache_error = item_ownership_runtime_snapshot_all_active(
			maximum_rows, &candidate.active_cache);
		if (cache_error)
			return cache_error;
		charge(candidate.active_cache.capacity(), sizeof(item_ownership_runtime_entry));
		candidate.cache_objects.resize(candidate.active_cache.size());
		candidate.cache_issues.resize(candidate.active_cache.size());
		candidate.cache_owner_revisions.resize(candidate.active_cache.size());
		charge(candidate.cache_objects.capacity(), sizeof(candidate.cache_objects[0]));
		charge(candidate.cache_issues.capacity(), sizeof(uint32_t));
		charge(candidate.cache_owner_revisions.capacity(),
		       sizeof(candidate.cache_owner_revisions[0]));
		for (size_t i = 0; i < candidate.objects.size(); ++i)
		{
			auto &value = candidate.objects[i];
			const auto found = std::lower_bound(candidate.active_cache.begin(),
							    candidate.active_cache.end(), value.uid,
							    [](const auto &row, uint64_t uid)
							    { return row.item_uid < uid; });
			if (found == candidate.active_cache.end() || found->item_uid != value.uid)
			{
				value.issues |= missing_cache;
				continue;
			}
			const size_t row =
				static_cast<size_t>(found - candidate.active_cache.begin());
			value.cache_row = row;
			candidate.cache_objects[row].push_back(i);
			const uint64_t parent_uid =
				value.parent ? candidate.objects[*value.parent].uid : 0;
			if (!value.root ||
			    found->root_item_uid != candidate.objects[*value.root].uid ||
			    found->parent_item_uid != parent_uid || !value.vnum ||
			    found->vnum != *value.vnum || !item_owner_identity_valid(found->owner))
				value.issues |= conflicting_cache;
			if (value.root)
			{
				const auto &root = candidate.objects[*value.root];
				if (root.character)
				{
					const auto &actor = candidate.characters[*root.character];
					if (actor.observed_owner &&
					    !item_owner_identity_equal(found->owner,
								       *actor.observed_owner))
						value.issues |= conflicting_cache;
					if (actor.reference &&
					    (!actor.owner_revision ||
					     found->owner_revision > *actor.owner_revision))
						value.issues |= conflicting_cache;
				}
				else if (root.room_vnum &&
					 !item_owner_identity_equal(
						 found->owner,
						 { item_owner_type::room,
						   static_cast<uint64_t>(*root.room_vnum), 0 }))
					value.issues |= conflicting_cache;
			}
		}
		for (size_t row = 0; row < candidate.active_cache.size(); ++row)
		{
			charge(candidate.cache_objects[row].capacity(), sizeof(size_t));
			const auto &entry = candidate.active_cache[row];
			if (candidate.cache_objects[row].empty())
				candidate.cache_issues[row] |= missing_link;
			else if (candidate.cache_objects[row].size() != 1)
				candidate.cache_issues[row] |= multiple_links;
			if (!entry.item_uid || entry.item_uid == UINT64_MAX ||
			    !entry.root_item_uid || entry.root_item_uid == UINT64_MAX ||
			    !item_owner_identity_valid(entry.owner))
				candidate.cache_issues[row] |= conflicting_cache;
			uint64_t revision = 0;
			if (item_ownership_runtime_peek_owner_revision(entry.owner, &revision))
			{
				candidate.cache_owner_revisions[row] = revision;
				// Entry clocks record the last touched item; unrelated items may
				// legitimately precede the current owner clock, including zero.
				if (entry.owner_revision > revision)
					candidate.cache_issues[row] |= conflicting_cache;
			}
			else
				candidate.cache_issues[row] |= missing_cache;
			for (const size_t object : candidate.cache_objects[row])
				candidate.cache_issues[row] |= candidate.objects[object].issues;
		}
		std::vector<std::vector<size_t>> members(objects.size());
		for (size_t i = 0; i < candidate.objects.size(); ++i)
			if (candidate.objects[i].root)
				members[*candidate.objects[i].root].push_back(i);
		// Unsafe reciprocal lists are retained above and never passed to a codec
		// which assumes pointers refer to the current allocated native world.
		for (size_t root = 0; root < objects.size(); ++root)
		{
			if (!candidate.objects[root].root || *candidate.objects[root].root != root)
				continue;
			maintenance_forest forest;
			forest.root_object = root;
			bool safe = candidate.object_list_complete &&
				    !(candidate.issues & foreign_link);
			for (const size_t member : members[root])
				if (candidate.objects[member].issues &
				    (invalid_identity | duplicate_identity | invalid_placement |
				     missing_link | multiple_links | foreign_link |
				     invalid_topology | list_cycle))
					safe = false;
			size_t estimated = 0;
			if (safe)
				forest.result = player_item_snapshot_tree_capture_literal(
					objects[root], &forest.items, &estimated);
			else
				forest.result = player_snapshot_capture_result::malformed_source;
			if (forest.result ==
			    player_snapshot_capture_result::retryable_allocation_failure)
				return ENOMEM;
			if (forest.result == player_snapshot_capture_result::ok)
			{
				charge_items(forest.items);
			}
			const size_t index = candidate.forests.size();
			for (const size_t member : members[root])
			{
				auto &value = candidate.objects[member];
				value.forest = index;
				if (forest.result != player_snapshot_capture_result::ok)
					value.issues |= literal_refused;
			}
			candidate.forests.push_back(std::move(forest));
		}
		charge(candidate.forests.capacity(), sizeof(maintenance_forest));
		std::vector<bool> actor_safe(actors.size(),
					     candidate.object_list_complete &&
						     candidate.physical_lists_complete &&
						     !(candidate.issues & foreign_link));
		for (const auto &value : candidate.objects)
			if (value.root && candidate.objects[*value.root].character &&
			    (!value.forest || candidate.forests[*value.forest].result !=
						      player_snapshot_capture_result::ok))
				actor_safe[*candidate.objects[*value.root].character] = false;
		for (size_t character = 0; character < actors.size(); ++character)
		{
			auto &actor = candidate.characters[character];
			if (!actor.reference)
				continue; // Legacy literal trees above need no invented reference.
			const bool safe = actor_safe[character] &&
					  !(actor.issues & (foreign_link | list_cycle));
			if (safe)
				actor.native_items_result = quest_mobile_native_items_observe(
					actors[character], *actor.reference, &actor.native_items);
			else
				actor.native_items_result =
					player_snapshot_capture_result::malformed_source;
			if (*actor.native_items_result ==
			    player_snapshot_capture_result::retryable_allocation_failure)
				return ENOMEM;
			if (*actor.native_items_result != player_snapshot_capture_result::ok)
				actor.issues |= literal_refused;
			else
			{
				std::vector<uint8_t> canonical;
				const auto encoded = player_item_snapshot_list_encode(
					actor.native_items, &canonical);
				if (encoded == player_snapshot_codec_result::allocation_failure)
					return ENOMEM;
				if (encoded != player_snapshot_codec_result::ok)
					return EILSEQ;
				charge_items(actor.native_items);
			}
		}
		for (const auto &value : candidate.characters)
			candidate.issues |= value.issues;
		for (const auto &value : candidate.objects)
			candidate.issues |= value.issues;
		for (size_t row = 0; row < candidate.cache_issues.size(); ++row)
		{
			for (const size_t object : candidate.cache_objects[row])
				candidate.cache_issues[row] |= candidate.objects[object].issues;
			candidate.issues |= candidate.cache_issues[row];
		}
		if (!held() || !nevent_is_game_thread())
			return EINVAL;
		static_assert(std::is_nothrow_move_assignable_v<maintenance_world_report>);
		*output = std::move(candidate);
		return 0;
	}
	catch (int error)
	{
		return static_cast<unsigned int>(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
}

bool quest_mobile_published_world_owner::observe_union(std::vector<held_native> *output) noexcept
{
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_sql_recovery() || !lists_valid())
		return false;
	try
	{
		std::vector<held_native> candidate;
		std::vector<uint64_t> runtimes;
		for (P_char actor = character_list; actor; actor = actor->next)
		{
			bool present = false;
			quest_mobile_native_reference reference;
			if (!quest_mobile_native_birth_owner::observe_current_published_identity(
				    actor, actor->runtime_id, &present, &reference))
				return false;
			runtimes.push_back(actor->runtime_id);
			if (present)
				candidate.push_back({ reference, actor->runtime_id });
		}
		std::sort(runtimes.begin(), runtimes.end());
		if (std::adjacent_find(runtimes.begin(), runtimes.end()) != runtimes.end())
			return false;
		std::sort(candidate.begin(), candidate.end(),
			  [](const auto &a, const auto &b) {
				  return a.reference.mobile_instance_id <
					 b.reference.mobile_instance_id;
			  });
		for (size_t i = 1; i < candidate.size(); ++i)
			if (candidate[i - 1].reference.mobile_instance_id ==
			    candidate[i].reference.mobile_instance_id)
				return false;
		std::vector<critical_operation_id> operations;
		operations.reserve(candidate.size());
		for (const auto &item : candidate)
			operations.push_back(item.reference.birth_operation);
		std::sort(operations.begin(), operations.end(),
			  [](const auto &a, const auto &b) { return a.bytes < b.bytes; });
		for (size_t i = 1; i < operations.size(); ++i)
			if (critical_operation_id_equal(operations[i - 1], operations[i]))
				return false;
		output->swap(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::verify_held(const locked_native &locked,
						     const held_native &held) noexcept
{
	try
	{
		const auto &source = locked.current;
		P_char body = find_character_by_runtime_id(held.runtime);
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> a{}, b{};
		if (!body || !held.runtime || body->runtime_id != held.runtime || !IS_NPC(body) ||
		    !body->only.npc || !IS_ALIVE(body) || body->in_room < 0 ||
		    body->in_room > top_of_world || !source.cash ||
		    source.state != quest_mobile_lifetime_state::live ||
		    source.items.size() != locked.custody.size() ||
		    locked.owner.type != item_owner_type::native_mobile ||
		    locked.owner.context_id ||
		    locked.owner.id != source.reference.mobile_instance_id ||
		    !locked.owner_revision ||
		    locked.owner_revision != source.reference.stock_revision ||
		    !locked.origin.present ||
		    !ordinary_origin_selector_current(locked.origin.original) ||
		    !quest_mobile_native_birth_owner::validate_progressed_origin(
			    locked.origin.original, source, locked.wallet) ||
		    quest_mobile_native_reference_encode(held.reference, &a) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(source.reference, &b) !=
			    player_snapshot_codec_result::ok ||
		    a != b)
			return false;
		quest_mobile_native_cash_reference metadata;
		quest_mobile_native_image observed;
		if (!quest_mobile_native_cash_reference_copy(body, held.runtime, &metadata) ||
		    metadata.wallet_mapping_id != locked.wallet.wallet_mapping_id ||
		    metadata.cash_revision != source.cash->revision ||
		    !critical_operation_id_equal(metadata.lineage, locked.wallet.lineage) ||
		    !critical_operation_id_equal(metadata.birth_epoch, locked.wallet.birth_epoch) ||
		    metadata.denominations != source.cash->denominations.amount ||
		    quest_mobile_native_reference_encode(metadata.reference, &a) !=
			    player_snapshot_codec_result::ok ||
		    a != b ||
		    quest_mobile_native_capture(body, source.reference, source.state,
						source.last_transition_operation,
						source.cash->revision,
						&observed) != player_snapshot_capture_result::ok ||
		    !same_image(source, observed))
			return false;

		uint64_t owner_revision = 0;
		if (!item_ownership_runtime_peek_owner_revision(locked.owner, &owner_revision) ||
		    owner_revision != locked.owner_revision)
			return false;
		// Resolve every canonical row through the complete global list. Each
		// pointer must also be that exact body's physical parent/root member.
		std::vector<P_obj> objects(source.items.size(), nullptr);
		for (size_t row = 0; row < source.items.size(); ++row)
		{
			for (P_obj object = object_list; object; object = object->next)
				if (object->obj_uid == source.items[row].object_uid)
				{
					if (objects[row])
						return false;
					objects[row] = object;
				}
			if (!objects[row])
				return false;
		}
		uint64_t previous_uid = 0;
		for (const auto &custody : locked.custody)
		{
			if (custody.item_uid <= previous_uid || !custody.item_revision ||
			    custody.owner_revision != locked.owner_revision ||
			    !item_owner_identity_equal(custody.owner, locked.owner) ||
			    custody.state != item_custody_state::active)
				return false;
			previous_uid = custody.item_uid;
			const auto literal = std::find_if(
				source.items.begin(), source.items.end(), [&](const auto &row)
				{ return row.object_uid == custody.item_uid; });
			if (literal == source.items.end() || literal->vnum != custody.vnum)
				return false;
			size_t row = static_cast<size_t>(literal - source.items.begin()),
			       root = row;
			const auto parent = literal->parent_index;
			while (source.items[root].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				root = source.items[root].parent_index;
			if (custody.root_item_uid != source.items[root].object_uid ||
			    custody.parent_item_uid != (parent == PLAYER_SNAPSHOT_NO_PARENT ?
								0 :
								source.items[parent].object_uid))
				return false;
			P_obj member = objects[row], head = nullptr;
			if (parent != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (member->loc_p != LOC_INSIDE ||
				    member->loc.inside != objects[parent])
					return false;
				head = objects[parent]->contains;
			}
			else if (literal->equipment_slot)
			{
				if (member->loc_p != LOC_WORN || member->loc.wearing != body ||
				    body->equipment[literal->equipment_slot - 1] != member)
					return false;
			}
			else
			{
				if (member->loc_p != LOC_CARRIED || member->loc.carrying != body)
					return false;
				head = body->carrying;
			}
			if (head)
			{
				// Complete canonical capture already refused list cycles.
				size_t matches = 0;
				for (P_obj item = head; item; item = item->next_content)
					if (item == member)
						++matches;
				if (matches != 1)
					return false;
			}
			else if (!literal->equipment_slot || parent != PLAYER_SNAPSHOT_NO_PARENT)
				return false;
			item_ownership_runtime_entry cached;
			if (!item_ownership_runtime_lookup(custody.item_uid, &cached) ||
			    !same_custody(cached, custody))
				return false;
		}
		const auto matches_current = [&](const item_ownership_runtime_entry &cached)
		{
			const auto expected = std::find_if(
				locked.custody.begin(), locked.custody.end(),
				[&](const auto &row) { return row.item_uid == cached.item_uid; });
			return expected != locked.custody.end() && same_custody(cached, *expected);
		};
		std::vector<item_ownership_runtime_entry> cached;
		if (!item_ownership_runtime_snapshot_owner(locked.owner, 262144, &cached))
			return false;
		for (const auto &row : cached)
			if (row.state == item_custody_state::active && !matches_current(row))
				return false;
		std::vector<uint64_t> selected_uids;
		selected_uids.reserve(locked.custody.size());
		for (const auto &row : locked.custody)
			selected_uids.push_back(row.item_uid);
		if (!item_ownership_runtime_published_native_observer::snapshot_links(
			    selected_uids, 262144, &cached))
			return false;
		for (const auto &claim : cached)
			if (!matches_current(claim))
				return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::select_held(const std::vector<locked_native> &full,
						     std::vector<held_native> *output) noexcept
{
	if (!output || full.size() > 262144)
		return false;
	try
	{
		std::vector<held_native> observed, candidate;
		if (!observe_union(&observed))
			return false;
		uint64_t previous = 0;
		for (const auto &item : full)
		{
			const auto &reference = item.current.reference;
			if (reference.mobile_instance_id <= previous)
				return false;
			previous = reference.mobile_instance_id;
			for (const auto &body : observed)
				if (body.reference.mobile_instance_id ==
					    reference.mobile_instance_id ||
				    critical_operation_id_equal(body.reference.birth_operation,
								reference.birth_operation))
				{
					if (!verify_held(item, body))
						return false;
					candidate.push_back(body);
				}
		}
		output->swap(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::split_held(const std::vector<locked_native> &full,
						    const std::vector<held_native> &held,
						    std::vector<locked_native> *unheld) noexcept
{
	if (!unheld || unheld == &full || full.size() > 262144 || held.size() > full.size())
		return false;
	try
	{
		std::vector<held_native> observed;
		if (!observe_union(&observed))
			return false;
		uint64_t previous = 0;
		for (const auto &value : held)
		{
			if (!value.runtime || value.reference.mobile_instance_id <= previous)
				return false;
			previous = value.reference.mobile_instance_id;
			const auto actual = std::find_if(observed.begin(), observed.end(),
							 [&](const auto &row)
							 { return row.runtime == value.runtime; });
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> a{}, b{};
			if (actual == observed.end() ||
			    quest_mobile_native_reference_encode(actual->reference, &a) !=
				    player_snapshot_codec_result::ok ||
			    quest_mobile_native_reference_encode(value.reference, &b) !=
				    player_snapshot_codec_result::ok ||
			    a != b)
				return false;
		}
		std::vector<locked_native> candidate;
		candidate.reserve(full.size() - held.size());
		size_t position = 0;
		previous = 0;
		for (const auto &item : full)
		{
			const uint64_t id = item.current.reference.mobile_instance_id;
			if (id <= previous)
				return false;
			previous = id;
			if (position < held.size() &&
			    held[position].reference.mobile_instance_id < id)
				return false;
			if (position < held.size() &&
			    held[position].reference.mobile_instance_id == id)
			{
				if (!verify_held(item, held[position]))
					return false;
				++position;
			}
			else
				candidate.push_back(item);
		}
		if (position != held.size())
			return false;
		unheld->swap(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

struct quest_mobile_published_world_owner::entry
{
	locked_native locked;
	native_mobile_birth_recovery_context recovery;
	quest_mobile_native_constructor_recipe constructor;
	native_mobile_birth_cash_role_recipe cash_role;
	bool ordinary_wallet = false;
	quest_mobile_native_stage mobile;
	quest_mobile_published_saved_forest forest;
	std::vector<P_obj> objects, fresh;
	std::vector<native_mobile_birth_recovery_action> enrollment;
	std::vector<int> prototype_counts;
	P_char character = nullptr;
	uint64_t runtime = 0;
	int room = -1;
	bool constructed = false, materialized = false, mobile_restored = false;
	bool metadata_started = false, metadata_returned = false, metadata_succeeded = false;
	bool cache_applied = false, forest_prepared = false;
	bool policy_started = false, policy_returned = false, policy_succeeded = false;
	~entry() noexcept
	{
		// No callback disposal and no fictional rollback of consumed ownership.
		// Boot retains partial materialization until actual world teardown.
		if (!materialized && mobile.character())
			(void)mobile.discard_empty();
	}
};
quest_mobile_published_world_owner::quest_mobile_published_world_owner() noexcept = default;
quest_mobile_published_world_owner::~quest_mobile_published_world_owner() noexcept = default;

bool quest_mobile_published_world_owner::token_valid(const idle_token &token) const noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	return false;
#else
	return nevent_is_game_thread() && economic_gameplay_authority::active() &&
	       token.connection_ && token.session_ &&
	       !critical_operation_id_is_zero(token.lineage_) &&
	       mysql_thread_id(token.connection_) == token.session_ &&
	       !player_sql_idle_error(token.connection_) &&
	       (!prepared_ || (connection_ == token.connection_ && session_ == token.session_ &&
			       critical_operation_id_equal(lineage_, token.lineage_)));
#endif
}

bool quest_mobile_published_world_owner::prepare(std::vector<locked_native> &&input,
						 const idle_token &token) noexcept
{
	if (prepared_ || !entries_.empty() || !token_valid(token) ||
	    !recovery_object_templates_ready() || input.size() > 262144 || !obj_index ||
	    top_of_objt < 0 || !world)
		return false;
	try
	{
		std::vector<std::unique_ptr<entry>> candidate;
		candidate.reserve(input.size());
		uint64_t previous = 0;
		for (auto &value : input)
		{
			if (!value.origin.present ||
			    !published_origin_terminal(value.origin.original) ||
			    value.current.state != quest_mobile_lifetime_state::live ||
			    !value.current.cash ||
			    value.current.reference.mobile_instance_id <= previous ||
			    value.current.items.size() != value.custody.size() ||
			    value.owner.type != item_owner_type::native_mobile ||
			    value.owner.context_id ||
			    value.owner.id != value.current.reference.mobile_instance_id ||
			    !value.owner_revision ||
			    value.owner_revision != value.current.reference.stock_revision ||
			    !critical_operation_id_equal(value.wallet.lineage, token.lineage_) ||
			    !quest_mobile_native_birth_owner::validate_progressed_origin(
				    value.origin.original, value.current, value.wallet))
				return false;
			previous = value.current.reference.mobile_instance_id;
			auto item = std::make_unique<entry>();
			quest_mobile_native_image born;
			std::vector<native_mobile_birth_item_recipe> original_recipes;
			item->ordinary_wallet = ordinary_wallet_origin(value.origin.original);
			if (item->ordinary_wallet)
			{
				// The authentic original attachment supplies the complete NMB4
				// command/constructor; CURRENT never supplies creating metadata.
				if (native_mobile_birth_cash_role_command_decode(
					    value.origin.original.command, &born, &original_recipes,
					    &item->cash_role) != economic_accounting_error::ok ||
				    item->cash_role.role !=
					    native_mobile_birth_cash_role::ordinary_wallet ||
				    native_mobile_birth_cash_role_recovery_decode(
					    value.origin.original.command,
					    value.origin.original.attachment,
					    &item->recovery) != economic_accounting_error::ok ||
				    !ordinary_role_selector_current(item->cash_role.original,
								    item->cash_role))
					return false;
				item->constructor = item->cash_role.original;
			}
			else
			{
				if (native_mobile_birth_command_decode(
					    value.origin.original.command, &born, &original_recipes,
					    &item->constructor) != economic_accounting_error::ok ||
				    native_mobile_birth_recovery_decode(
					    value.origin.original.command,
					    value.origin.original.attachment,
					    &item->recovery) != economic_accounting_error::ok ||
				    (item->constructor.wire_version !=
					     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
				     item->constructor.wire_version !=
					     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION))
					return false;
			}
			for (const auto &effect : item->recovery.mobile_effects)
				if (!effect.started || !effect.returned || !effect.succeeded)
					return false;
			uint64_t previous_uid = 0;
			for (const auto &custody : value.custody)
			{
				if (custody.item_uid <= previous_uid || !custody.item_revision ||
				    custody.owner_revision != value.owner_revision ||
				    !item_owner_identity_equal(custody.owner, value.owner) ||
				    custody.state != item_custody_state::active)
					return false;
				previous_uid = custody.item_uid;
				const auto literal = std::find_if(
					value.current.items.begin(), value.current.items.end(),
					[&](const auto &row)
					{ return row.object_uid == custody.item_uid; });
				if (literal == value.current.items.end() ||
				    literal->vnum != custody.vnum)
					return false;
				size_t position =
					static_cast<size_t>(literal - value.current.items.begin());
				const uint64_t parent =
					literal->parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
						0 :
						value.current.items[literal->parent_index]
							.object_uid;
				while (value.current.items[position].parent_index !=
				       PLAYER_SNAPSHOT_NO_PARENT)
					position = value.current.items[position].parent_index;
				if (custody.parent_item_uid != parent ||
				    custody.root_item_uid !=
					    value.current.items[position].object_uid)
					return false;
			}
			for (int64_t amount : value.current.cash->denominations.amount)
				if (amount < 0 || amount > INT_MAX)
					return false;
			item->room = real_room(item->constructor.reset_room_vnum);
			if (item->room < 0 || item->room > top_of_world ||
			    world[item->room].number != value.current.reference.birthplace_vnum)
				return false;
			item->objects.resize(value.current.items.size());
			item->fresh.resize(value.current.items.size());
			item->enrollment.resize(value.current.items.size());
			item->prototype_counts.resize(value.current.items.size());
			item->locked = std::move(value);
			candidate.push_back(std::move(item));
		}
		entries_ = std::move(candidate);
		lineage_ = token.lineage_;
		connection_ = token.connection_;
		session_ = token.session_;
		prepared_ = true;
		if (!charge())
			return false;
		for (auto &item : entries_)
			if (!census(*item, false))
				return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::same_cut(
	const std::vector<locked_native> &fresh) const noexcept
{
	if (!prepared_ || fresh.size() != entries_.size())
		return false;
	try
	{
		for (size_t i = 0; i < fresh.size(); ++i)
		{
			const auto &a = entries_[i]->locked;
			const auto &b = fresh[i];
			if (!b.origin.present ||
			    a.origin.original.phase != b.origin.original.phase ||
			    a.origin.original.revision != b.origin.original.revision ||
			    a.origin.original.attachment != b.origin.original.attachment ||
			    !critical_command_equal(a.origin.original.command,
						    b.origin.original.command) ||
			    !same_image(a.current, b.current) ||
			    !critical_operation_id_equal(a.wallet.birth_operation,
							 b.wallet.birth_operation) ||
			    !critical_operation_id_equal(a.wallet.lineage, b.wallet.lineage) ||
			    !critical_operation_id_equal(a.wallet.birth_epoch,
							 b.wallet.birth_epoch) ||
			    a.wallet.mobile_instance_id != b.wallet.mobile_instance_id ||
			    a.wallet.wallet_mapping_id != b.wallet.wallet_mapping_id ||
			    !item_owner_identity_equal(a.owner, b.owner) ||
			    a.owner_revision != b.owner_revision ||
			    a.custody.size() != b.custody.size())
				return false;
			for (size_t j = 0; j < a.custody.size(); ++j)
				if (!same_custody(a.custody[j], b.custody[j]))
					return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::census(entry &item, bool require_registered) noexcept
{
	try
	{
		if (!lists_valid())
			return false;
		// Re-census the actual configured selector before every original cold
		// constructor/policy, materialization, enrollment, reload and cache cut.
		if (item.ordinary_wallet &&
		    !ordinary_role_selector_current(item.constructor, item.cash_role))
			return false;
		if (item.constructed)
		{
			quest_mobile_native_constructor_digest build{}, procedure{}, tail{};
			if (!native_mobile_birth_running_artifact_digest(&build) ||
			    build != item.constructor.build_digest ||
			    !native_mobile_birth_procedure_capture(item.constructor.mobile_vnum,
								   build, &procedure) ||
			    procedure != item.constructor.procedure_after ||
			    !native_mobile_birth_reset_tail_capture(
				    item.constructor.mobile_vnum, item.constructor.reset_room_vnum,
				    item.constructor.reset_shop_index, &tail) ||
			    tail != item.constructor.reset_tail)
				return false;
		}
		const auto &source = item.locked.current;
		const bool registered = item.mobile.publication_consumed_;
		if (require_registered && !registered)
			return false;
		P_char body = registered ? find_character_by_runtime_id(item.runtime) :
					   item.mobile.character();
		if (item.constructed &&
		    (!body || body != item.character || body->runtime_id != item.runtime))
			return false;
		size_t matches = 0;
		for (P_char actual = character_list; actual; actual = actual->next)
		{
			bool present = false;
			quest_mobile_native_reference reference;
			if (!quest_mobile_native_birth_owner::observe_current_published_identity(
				    actual, actual->runtime_id, &present, &reference))
				return false;
			if (actual == body || (item.runtime && actual->runtime_id == item.runtime))
			{
				if (!registered || actual != body)
					return false;
				++matches;
			}
			if (present &&
			    (reference.mobile_instance_id == source.reference.mobile_instance_id ||
			     critical_operation_id_equal(reference.birth_operation,
							 source.reference.birth_operation)))
				if (!registered || actual != body)
					return false;
		}
		if (matches != (registered ? 1u : 0u))
			return false;
		for (size_t row = 0; row < source.items.size(); ++row)
		{
			P_obj found = nullptr;
			for (P_obj object = object_list; object; object = object->next)
				if (object->obj_uid == source.items[row].object_uid)
				{
					if (found || !item.materialized ||
					    object != item.objects[row])
						return false;
					found = object;
				}
			if (item.materialized && !found)
				return false;
			item.fresh[row] = found;
			item_ownership_runtime_entry cached;
			if (item_ownership_runtime_lookup(source.items[row].object_uid, &cached))
			{
				const auto expected = std::find_if(
					item.locked.custody.begin(), item.locked.custody.end(),
					[&](const auto &value)
					{ return value.item_uid == source.items[row].object_uid; });
				if (expected == item.locked.custody.end() ||
				    !same_custody(cached, *expected))
					return false;
			}
			else if (item.cache_applied)
				return false;
		}
		std::vector<item_ownership_runtime_entry> owner_rows;
		if (!item_ownership_runtime_snapshot_owner(item.locked.owner, 262144, &owner_rows))
			return false;
		for (const auto &cached : owner_rows)
		{
			if (cached.state != item_custody_state::active)
				continue;
			const auto expected =
				std::find_if(item.locked.custody.begin(), item.locked.custody.end(),
					     [&](const auto &value)
					     { return value.item_uid == cached.item_uid; });
			if (expected == item.locked.custody.end() ||
			    !same_custody(cached, *expected))
				return false;
		}
		// Observe all active UID/root/parent links, including foreign claims.
		std::vector<uint64_t> selected_uids;
		selected_uids.reserve(source.items.size());
		for (const auto &literal : source.items)
			selected_uids.push_back(literal.object_uid);
		std::sort(selected_uids.begin(), selected_uids.end());
		if (std::adjacent_find(selected_uids.begin(), selected_uids.end()) !=
		    selected_uids.end())
			return false;
		std::vector<item_ownership_runtime_entry> claim;
		if (!item_ownership_runtime_published_native_observer::snapshot_links(
			    selected_uids, 262144, &claim))
			return false;
		for (const auto &cached : claim)
		{
			const auto expected =
				std::find_if(item.locked.custody.begin(), item.locked.custody.end(),
					     [&](const auto &value)
					     { return value.item_uid == cached.item_uid; });
			if (expected == item.locked.custody.end() ||
			    !same_custody(cached, *expected))
				return false;
		}
		uint64_t cached_revision = 0;
		if (item_ownership_runtime_peek_owner_revision(item.locked.owner, &cached_revision))
		{
			if (cached_revision != item.locked.owner_revision)
				return false;
		}
		else if (item.cache_applied)
			return false;
		if (item.materialized)
		{
			quest_mobile_native_image observed;
			if (quest_mobile_native_capture(body, source.reference, source.state,
							source.last_transition_operation,
							source.cash->revision, &observed) !=
				    player_snapshot_capture_result::ok ||
			    !same_image(source, observed) || !item.forest.valid(item.fresh))
				return false;
			if (registered)
			{
				quest_mobile_native_cash_reference metadata;
				if (!quest_mobile_native_cash_reference_copy(body, item.runtime,
									     &metadata) ||
				    metadata.wallet_mapping_id !=
					    item.locked.wallet.wallet_mapping_id ||
				    metadata.cash_revision != source.cash->revision ||
				    !critical_operation_id_equal(metadata.lineage,
								 item.locked.wallet.lineage) ||
				    !critical_operation_id_equal(metadata.birth_epoch,
								 item.locked.wallet.birth_epoch) ||
				    metadata.denominations != source.cash->denominations.amount)
					return false;
				std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> a{}, b{};
				if (quest_mobile_native_reference_encode(metadata.reference, &a) !=
					    player_snapshot_codec_result::ok ||
				    quest_mobile_native_reference_encode(source.reference, &b) !=
					    player_snapshot_codec_result::ok ||
				    a != b)
					return false;
			}
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_world_owner::construct(entry &item) noexcept
{
	if (!census(item, false))
	{
		report_world_refusal(8001);
		return false;
	}
	if (!item.constructed)
	{
		quest_mobile_native_constructor_digest build{}, procedure{}, tail{};
		if (!native_mobile_birth_running_artifact_digest(&build))
		{
			report_world_refusal(8002);
			return false;
		}
		if (build != item.constructor.build_digest)
		{
			report_world_refusal(8003);
			return false;
		}
		if (!native_mobile_birth_procedure_capture(item.constructor.mobile_vnum, build,
							   &procedure))
		{
			report_world_refusal(8004);
			return false;
		}
		if (procedure != item.constructor.procedure_after)
		{
			report_world_refusal(8005);
			return false;
		}
		if (!native_mobile_birth_reset_tail_capture(
			    item.constructor.mobile_vnum, item.constructor.reset_room_vnum,
			    item.constructor.reset_shop_index, &tail))
		{
			report_world_refusal(8006);
			return false;
		}
		if (tail != item.constructor.reset_tail)
		{
			report_world_refusal(8007);
			return false;
		}
		if (!item.mobile.restore_constructor(item.constructor, build))
		{
			report_world_refusal(8008);
			return false;
		}
		item.character = item.mobile.character();
		item.runtime = item.character->runtime_id;
		item.constructed = true;
	}
	if (item.policy_started && (!item.policy_returned || !item.policy_succeeded))
	{
		report_world_refusal(8009);
		return false;
	}
	if (!item.policy_started)
	{
		// Refuse before recording the original effect intent, rather than
		// turning a changed keeper selector into a failed attempted policy.
		if (item.ordinary_wallet &&
		    !ordinary_role_selector_current(item.constructor, item.cash_role))
			return false;
		item.policy_started = true;
		item.policy_succeeded =
			quest_mobile_native_birth_owner::restore_current_published_constructor_policy(
				item.character, item.constructor);
		item.policy_returned = true;
		if (!item.policy_succeeded)
		{
			report_world_refusal(8010);
			return false;
		}
	}
	if (!item.forest_prepared)
	{
		if (!quest_mobile_published_saved_forest::prepare(item.locked.current, item.forest))
		{
			report_world_refusal(8011);
			return false;
		}
		item.forest_prepared = true;
	}
	if (!charge())
	{
		report_world_refusal(8012);
		return false;
	}
	return true;
}

bool quest_mobile_published_world_owner::materialize(entry &item) noexcept
{
	if (item.materialized)
	{
		const bool valid = census(item, false);
		if (!valid)
			report_world_refusal(8198);
		return valid;
	}
	if (!item.constructed || !census(item, false) || !item.forest.valid())
	{
		report_world_refusal(8101);
		return false;
	}
	const auto &source = item.locked.current;
	if (!item.character || item.character != item.mobile.character() ||
	    item.character->carrying || item.character->in_room != NOWHERE ||
	    item.character->next || item.character->next_in_room || item.character->desc ||
	    item.character->nevents || item.character->nevents_tail ||
	    item.character->character_maintenance_in_world ||
	    find_character_by_runtime_id(item.runtime))
	{
		report_world_refusal(8102);
		return false;
	}
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (item.character->equipment[slot])
		{
			report_world_refusal(8103);
			return false;
		}
	int64_t weight = 0, count = 0;
	for (size_t row = 0; row < item.forest.objects().size(); ++row)
	{
		P_obj object = item.forest.objects()[row];
		if (!object || object->R_num < 0 || object->R_num > top_of_objt ||
		    obj_index[object->R_num].number < 0)
		{
			report_world_refusal(8104);
			return false;
		}
		item.prototype_counts[row] = object->R_num;
	}
	std::sort(item.prototype_counts.begin(), item.prototype_counts.end());
	for (size_t first = 0; first < item.prototype_counts.size();)
	{
		size_t last = first + 1;
		while (last < item.prototype_counts.size() &&
		       item.prototype_counts[last] == item.prototype_counts[first])
			++last;
		if (last - first >
		    static_cast<size_t>(INT_MAX - obj_index[item.prototype_counts[first]].number))
		{
			report_world_refusal(8105);
			return false;
		}
		first = last;
	}
	for (const auto &literal : source.items)
		if (literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			weight += encumbrance_weight(literal.weight) /
				  (literal.equipment_slot ? 2 : 1);
			if (!literal.equipment_slot)
				++count;
		}
	if (weight < 0 || weight > INT_MAX || count > INT_MAX)
	{
		report_world_refusal(8106);
		return false;
	}
	if (!quest_mobile_native_item_cold_prepend_cut_ready(item.forest.objects().size()))
		return false;
	for (P_obj object : item.forest.objects())
		if (!quest_mobile_native_item_cold_prepend_body_ready(object))
			return false;
	// All allocations/refusers precede the no-fail binding/ownership consumption.
	if (!item.forest.consume(item.objects))
	{
		report_world_refusal(8107);
		return false;
	}
	item.materialized = true;
	for (size_t offset = item.objects.size(); offset; --offset)
	{
		const size_t row = offset - 1;
		P_obj object = item.objects[row];
		const auto &literal = source.items[row];
		if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		{
			P_obj parent = item.objects[literal.parent_index];
			object->loc_p = LOC_INSIDE;
			object->loc.inside = parent;
			object->next_content = parent->contains;
			parent->contains = object;
		}
		else if (literal.equipment_slot)
		{
			item.character->equipment[literal.equipment_slot - 1] = object;
			object->loc_p = LOC_WORN;
			object->loc.wearing = item.character;
		}
		else
		{
			object->loc_p = LOC_CARRIED;
			object->loc.carrying = item.character;
			object->next_content = item.character->carrying;
			item.character->carrying = object;
		}
		++obj_index[object->R_num].number;
		quest_mobile_native_item_observe_native_prepend(object);
		if (object_list)
			object_list->prev = object;
		object->next = object_list;
		object_list = object;
	}
	GET_CARRYING_W(item.character) = static_cast<int>(weight);
	IS_CARRYING_N(item.character) = static_cast<int>(count);
	GET_COPPER(item.character) = static_cast<int>(source.cash->denominations.amount[0]);
	GET_SILVER(item.character) = static_cast<int>(source.cash->denominations.amount[1]);
	GET_GOLD(item.character) = static_cast<int>(source.cash->denominations.amount[2]);
	GET_PLATINUM(item.character) = static_cast<int>(source.cash->denominations.amount[3]);
	item.metadata_started = true;
	item.metadata_succeeded =
		quest_mobile_native_birth_owner::install_current_published_metadata(
			item.character, item.runtime, source, item.locked.wallet);
	item.metadata_returned = true;
	const bool valid = item.metadata_succeeded && census(item, false);
	if (!valid)
		report_world_refusal(8199);
	return valid;
}

bool quest_mobile_published_world_owner::restore_mobile(entry &item) noexcept
{
	if (!item.materialized || !item.metadata_started || !item.metadata_returned ||
	    !item.metadata_succeeded || !census(item, false))
	{
		report_world_refusal(8201);
		return false;
	}
	if (!item.mobile_restored)
	{
		P_char current = nullptr;
		const bool restored =
			item.mobile.restore_published(item.room, item.recovery.mobile_effects,
						      item.recovery.mobile_choices, &current);
		// Actual stage retains any consumed runtime ownership even on failure.
		if (!restored || current != item.character ||
		    find_character_by_runtime_id(item.runtime) != item.character)
		{
			report_world_refusal(8202);
			return false;
		}
		item.mobile_restored = true;
	}
	const bool valid = census(item, true);
	if (!valid)
		report_world_refusal(8299);
	return valid;
}

quest_mobile_published_world_owner::progress
quest_mobile_published_world_owner::advance(const std::vector<locked_native> &fresh,
					    const idle_token &token) noexcept
{
	if (!token_valid(token) || !same_cut(fresh) || !charge())
	{
		report_world_refusal(8301);
		return progress::refused;
	}
	try
	{
		// All native stages already have their preallocated effect/output storage.
		for (auto &owned : entries_)
		{
			auto &item = *owned;
			if (!construct(item) || !materialize(item) || !restore_mobile(item))
			{
				report_world_refusal(8302);
				return progress::refused;
			}
			for (size_t row = 0; row < item.objects.size(); ++row)
			{
				auto &enrollment = item.enrollment[row];
				if (enrollment.started &&
				    (!enrollment.returned || !enrollment.succeeded))
				{
					report_world_refusal(8303);
					return progress::refused;
				}
				if (!enrollment.started)
				{
					if (!census(item, true))
					{
						report_world_refusal(8304);
						return progress::refused;
					}
					enrollment.started = true;
					enrollment.succeeded = quest_mobile_native_birth_owner::
						restore_current_published_enrollment(
							item.fresh[row],
							find_character_by_runtime_id(item.runtime));
					enrollment.returned = true;
					if (!enrollment.succeeded || !census(item, true))
					{
						report_world_refusal(8305);
						return progress::refused;
					}
					return progress::pending;
				}
				const auto *effects = item.forest.effects(row);
				if (!effects)
				{
					report_world_refusal(8306);
					return progress::refused;
				}
				for (unsigned step = 0; step < 4; ++step)
				{
					if (step == 2)
						for (size_t description = 0;
						     description < effects->probes.size();
						     ++description)
						{
							const auto &effect =
								effects->probes[description];
							if (effect.started &&
							    (!effect.returned || !effect.succeeded))
							{
								report_world_refusal(8307);
								return progress::refused;
							}
							if (!effect.started)
							{
								if (!census(item, true) ||
								    !item.forest.proclib_probe(
									    row, description,
									    item.fresh[row]) ||
								    !census(item, true))
								{
									report_world_refusal(8308);
									return progress::refused;
								}
								return progress::pending;
							}
						}
					const auto &effect = effects->steps[step];
					if (effect.started &&
					    (!effect.returned || !effect.succeeded))
					{
						report_world_refusal(8309);
						return progress::refused;
					}
					if (!effect.started)
					{
						if (!census(item, true) ||
						    !item.forest.reload_step(row, step,
									     item.fresh[row]) ||
						    !census(item, true))
						{
							report_world_refusal(8310);
							return progress::refused;
						}
						return progress::pending;
					}
				}
			}
			if (!item.cache_applied)
			{
				if (!census(item, true) ||
				    !item_ownership_runtime_hydrate_many_atomic(
					    item.locked.custody.data(),
					    item.locked.custody.size()) ||
				    !item_ownership_runtime_hydrate_owner(
					    item.locked.owner, item.locked.owner_revision))
				{
					report_world_refusal(8311);
					return progress::refused;
				}
				item.cache_applied = true;
				if (!census(item, true))
				{
					report_world_refusal(8312);
					return progress::refused;
				}
			}
		}
		const bool valid = verify(fresh, token);
		if (!valid)
			report_world_refusal(8399);
		return valid ? progress::complete : progress::refused;
	}
	catch (...)
	{
		{
			report_world_refusal(8313);
			return progress::refused;
		}
	}
}

bool quest_mobile_published_world_owner::verify(const std::vector<locked_native> &fresh,
						const idle_token &token) noexcept
{
	if (!token_valid(token) || !same_cut(fresh) || !charge())
	{
		report_world_refusal(8401);
		return false;
	}
	for (auto &owned : entries_)
	{
		auto &item = *owned;
		if (!item.mobile_restored || !item.cache_applied ||
		    !item.forest.reload_complete() || !census(item, true))
		{
			report_world_refusal(8402);
			return false;
		}
		for (const auto &effect : item.enrollment)
			if (!effect.started || !effect.returned || !effect.succeeded)
			{
				report_world_refusal(8403);
				return false;
			}
	}
	return true;
}

bool quest_mobile_published_world_owner::charge() const noexcept
{
	// Root also charges the live consumed world. This finite retained-owner cap
	// uses the existing coordinator byte ceiling, never substitutes for release
	// performance/memory qualification or complete SQL-source row/cell bounds.
	try
	{
		size_t bytes = sizeof(*this);
		if (!add(bytes, entries_.capacity(), sizeof(entries_[0])))
			return false;
		for (const auto &owned : entries_)
		{
			const auto &item = *owned;
			std::vector<uint8_t> current, command;
			if (quest_mobile_native_image_encode(item.locked.current, &current) !=
				    player_snapshot_codec_result::ok ||
			    critical_command_encode(item.locked.origin.original.command,
						    &command) !=
				    critical_command_codec_result::ok ||
			    !add(bytes, sizeof(item)) || !add(bytes, current.capacity()) ||
			    !add(bytes, command.capacity()) ||
			    !add(bytes, item.locked.origin.original.attachment.capacity()) ||
			    !add(bytes, item.locked.custody.capacity(),
				 sizeof(item.locked.custody[0])) ||
			    !add(bytes, item.objects.capacity() + item.fresh.capacity(),
				 sizeof(P_obj)) ||
			    !add(bytes, item.enrollment.capacity(), sizeof(item.enrollment[0])) ||
			    !add(bytes, item.prototype_counts.capacity(), sizeof(int)))
				return false;
			const auto &stored_command = item.locked.origin.original.command;
			if (!add(bytes, stored_command.payload.capacity()) ||
			    !add(bytes, stored_command.accounting_intent.capacity()) ||
			    !add(bytes, stored_command.keys.capacity(),
				 sizeof(stored_command.keys[0])) ||
			    !add(bytes, stored_command.expected_revisions.capacity(),
				 sizeof(stored_command.expected_revisions[0])) ||
			    !add(bytes, item.recovery.items.capacity(),
				 sizeof(item.recovery.items[0])) ||
			    !add(bytes, item.locked.current.items.capacity(),
				 sizeof(player_item_snapshot)))
				return false;
			for (const auto &row : item.recovery.items)
				if (!add(bytes, row.effects.capacity(), sizeof(row.effects[0])))
					return false;
			for (const auto &row : item.locked.current.items)
			{
				for (const auto *text :
				     { &row.name, &row.short_description, &row.description,
				       &row.action_description })
					if (!add(bytes, text->capacity()) || !add(bytes, 1))
						return false;
				if (!add(bytes, row.dynamic_affects.capacity(),
					 sizeof(row.dynamic_affects[0])) ||
				    !add(bytes, row.extra_descriptions.capacity(),
					 sizeof(row.extra_descriptions[0])))
					return false;
				for (const auto &description : row.extra_descriptions)
					if (!add(bytes, description.keyword.capacity()) ||
					    !add(bytes, 1) ||
					    !add(bytes, description.description.capacity()) ||
					    !add(bytes, 1) ||
					    !add(bytes, description.spell_ids.capacity(),
						 sizeof(description.spell_ids[0])))
						return false;
			}
			if (item.forest_prepared)
			{
				const size_t forest = item.forest.retained_bytes();
				if (!forest || !add(bytes, forest))
					return false;
			}
			if (bytes > CRITICAL_COORDINATOR_MAX_BYTES)
				return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}
