#include "item/ordinary_drop_recovery.h"

#include "core/prototypes.h"
#include "economy/item_transfer_accounting.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#ifndef __NO_MYSQL__
#include "player/inert_item_stage.h"
#include "world/object_template.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "persistence/sql_room_item_payload.h"
#include "player/player_sql_transaction_cleanup.h"
#endif

#include <algorithm>
#include <cerrno>
#include <climits>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <vector>

extern P_obj object_list;
extern P_char character_list;
extern P_room world;
extern const int top_of_world;

#ifndef __NO_MYSQL__
extern P_index obj_index;
extern int top_of_objt;

// This owner is defined only in the integrated module. It has no public stage
// release or exported constructor; SQL authority and every private allocation
// remain synchronous and nonescaping until the last assignment-only step.
class ordinary_drop_enrollment_owner final
{
    public:
	static ordinary_drop_observation
	publish(MYSQL *, unsigned long, const critical_command &, const critical_completion &,
		const item_transfer_payload &, const item_transfer_result &,
		const sql_room_item_payload_batch &, const sql_room_item_graph &);

    private:
	ordinary_drop_enrollment_owner(const critical_command &command,
				       const critical_completion &receipt) noexcept
		: command_(command)
		, receipt_(receipt)
	{
	}
	ordinary_drop_enrollment_owner(const ordinary_drop_enrollment_owner &) = delete;
	ordinary_drop_enrollment_owner &operator=(const ordinary_drop_enrollment_owner &) = delete;
	void enroll() noexcept;
	const critical_command &command_;
	const critical_completion &receipt_;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	int room_ = -1, light_ = 0;
	P_index index_ = nullptr;
	std::vector<inert_item_stage> stages_;
	std::vector<const object_template *> prototypes_;
	std::vector<item_ownership_runtime_entry> runtime_;
	std::map<int, int> counts_;
};
#endif

namespace
{
ordinary_drop_observation observed(ordinary_drop_observation_status status, unsigned int error = 0,
				   uint64_t witness = 0, uint64_t room_revision = 0)
{
	return { status, error, witness, room_revision };
}

#ifndef __NO_MYSQL__
// Exhausting a complete-world traversal refuses observation. In particular,
// truncation cannot become an absent result. This bounds work, not world size.
constexpr size_t census_limit = 1000000;

bool session_current(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	return connection && session && mysql_thread_id(connection) == session &&
	       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
	       (connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
	       !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}

unsigned int current_error()
{
	return errno ? static_cast<unsigned int>(errno) : EIO;
}

bool canonical_item(player_item_snapshot item, std::vector<uint8_t> *bytes)
{
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	const auto result = player_item_snapshot_list_encode({ item }, bytes);
	if (result == player_snapshot_codec_result::allocation_failure)
		throw std::bad_alloc();
	return result == player_snapshot_codec_result::ok;
}

struct physical_item
{
	size_t input_index = 0;
	size_t original_index = 0;
	P_obj object = nullptr;
	size_t occurrences = 0;
	size_t room_links = 0;
	size_t child_links = 0;
};

ordinary_drop_observation compare_graph(const item_transfer_payload &payload,
					const item_transfer_result &result,
					const sql_room_item_payload_batch &original,
					const sql_room_item_graph &durable)
{
	if (!item_owner_identity_equal(durable.owner, payload.to_owner) ||
	    durable.owner_revision < result.to_owner_revision ||
	    durable.items.size() != payload.item_count ||
	    durable.identities.size() != payload.item_count)
		return observed(ordinary_drop_observation_status::conflict, ESTALE);
	std::map<uint64_t, physical_item> expected;
	for (size_t index = 0; index < payload.item_count; ++index)
		expected.emplace(payload.items[index].item_uid, physical_item{ index });
	for (size_t index = 0; index < original.items.size(); ++index)
		expected.at(original.items[index].object_uid).original_index = index;
	for (size_t index = 0; index < durable.items.size(); ++index)
	{
		const auto &item = durable.items[index];
		const auto &identity = durable.identities[index];
		const auto found = expected.find(item.object_uid);
		if (found == expected.end())
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					item.object_uid);
		const auto &entry = payload.items[found->second.input_index];
		std::vector<uint8_t> bytes;
		if (identity.item_uid != entry.item_uid ||
		    identity.root_item_uid != payload.selected_item_uid ||
		    identity.parent_item_uid != entry.parent_item_uid ||
		    !item_owner_identity_equal(identity.owner, payload.to_owner) ||
		    identity.state != item_custody_state::active ||
		    identity.item_revision != entry.expected_item_revision + 1 ||
		    identity.owner_revision != durable.owner_revision || item.vnum != entry.vnum ||
		    !canonical_item(item, &bytes) ||
		    bytes != original.payloads[found->second.original_index])
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					entry.item_uid);
	}

	P_obj slow = object_list, fast = object_list, previous = nullptr;
	std::vector<P_obj> global_objects;
	size_t visited = 0, present = 0;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (++visited > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		if (object->prev != previous)
			return observed(ordinary_drop_observation_status::conflict, ELOOP);
		previous = object;
		global_objects.push_back(object);
		if (fast && fast->next)
		{
			fast = fast->next->next;
			slow = slow->next;
			if (fast == slow)
				return observed(ordinary_drop_observation_status::conflict, ELOOP);
		}
		else
			fast = nullptr;
		const auto found = expected.find(object->obj_uid);
		if (found != expected.end())
		{
			if (++found->second.occurrences != 1)
				return observed(ordinary_drop_observation_status::conflict, EEXIST,
						object->obj_uid);
			found->second.object = object;
			++present;
		}
	}
	std::sort(global_objects.begin(), global_objects.end(), std::less<P_obj>{});
	std::vector<size_t> link_generation(global_objects.size(), 0);
	size_t generation = 0;
	const auto visit_link = [&](P_obj object, size_t list_generation) -> unsigned int
	{
		if (++visited > census_limit)
			return E2BIG;
		const auto found = std::lower_bound(global_objects.begin(), global_objects.end(),
						    object, std::less<P_obj>{});
		if (found == global_objects.end() || *found != object)
			return ENXIO;
		const size_t index = static_cast<size_t>(found - global_objects.begin());
		if (link_generation[index] == list_generation)
			return ELOOP;
		link_generation[index] = list_generation;
		return 0;
	};
	const auto link_failure = [](unsigned int error, uint64_t uid = 0)
	{
		return observed(error == E2BIG ? ordinary_drop_observation_status::unavailable :
						 ordinary_drop_observation_status::conflict,
				error, uid);
	};

	if (!world || top_of_world < 0 || static_cast<size_t>(top_of_world) >= census_limit)
		return observed(ordinary_drop_observation_status::unavailable, E2BIG);
	const int room = real_room(static_cast<int>(payload.to_owner.id));
	if (room < 0 || room > top_of_world || world[room].number <= 0 ||
	    static_cast<uint64_t>(world[room].number) != payload.to_owner.id)
		return observed(ordinary_drop_observation_status::unsupported, ENOENT);
	for (int location = 0; location <= top_of_world; ++location)
	{
		if (++visited > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		++generation;
		for (P_obj object = world[location].contents; object; object = object->next_content)
		{
			if (const auto error = visit_link(object, generation))
				return link_failure(error);
			const auto found = expected.find(object->obj_uid);
			if (found != expected.end() &&
			    (object != found->second.object ||
			     object->obj_uid != payload.selected_item_uid || location != room ||
			     ++found->second.room_links != 1))
				return observed(ordinary_drop_observation_status::conflict, ESTALE,
						object->obj_uid);
		}
	}
	// Census all global containers, including foreign parents, before absence.
	// Any unregistered intermediate refuses rather than hiding deeper children.
	for (P_obj parent : global_objects)
	{
		if (++visited > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		++generation;
		const auto selected_parent = expected.find(parent->obj_uid);
		for (P_obj child = parent->contains; child; child = child->next_content)
		{
			if (const auto error = visit_link(child, generation))
				return link_failure(error, parent->obj_uid);
			const auto found = expected.find(child->obj_uid);
			if (found == expected.end())
			{
				if (selected_parent != expected.end())
					return observed(ordinary_drop_observation_status::conflict,
							ESTALE, child->obj_uid);
				continue;
			}
			if (child->obj_uid == payload.selected_item_uid ||
			    selected_parent == expected.end() ||
			    selected_parent->second.object != parent ||
			    found->second.object != child ||
			    payload.items[found->second.input_index].parent_item_uid !=
				    parent->obj_uid ||
			    child->loc_p != LOC_INSIDE || child->loc.inside != parent ||
			    ++found->second.child_links != 1)
				return observed(ordinary_drop_observation_status::conflict, ESTALE,
						child->obj_uid);
		}
	}
	P_char slow_character = character_list, fast_character = character_list;
	for (P_char character = character_list; character; character = character->next)
	{
		if (++visited > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		if (fast_character && fast_character->next)
		{
			fast_character = fast_character->next->next;
			slow_character = slow_character->next;
			if (fast_character == slow_character)
				return observed(ordinary_drop_observation_status::conflict, ELOOP);
		}
		else
			fast_character = nullptr;
		++generation;
		for (P_obj object = character->carrying; object; object = object->next_content)
		{
			if (const auto error = visit_link(object, generation))
				return link_failure(error);
			if (expected.find(object->obj_uid) != expected.end())
				return observed(ordinary_drop_observation_status::conflict, ESTALE,
						object->obj_uid);
		}
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (++visited > census_limit)
				return observed(ordinary_drop_observation_status::unavailable,
						E2BIG);
			P_obj object = character->equipment[slot];
			if (!object)
				continue;
			if (const auto error = visit_link(object, ++generation))
				return link_failure(error);
			if (expected.find(object->obj_uid) != expected.end())
				return observed(ordinary_drop_observation_status::conflict, ESTALE,
						object->obj_uid);
		}
	}

	std::vector<item_ownership_runtime_entry> runtime;
	if (item_ownership_runtime_size() > (census_limit - visited) / 2)
		return observed(ordinary_drop_observation_status::unavailable, E2BIG);
	if (!item_ownership_runtime_snapshot_root(payload.selected_item_uid,
						  ITEM_TRANSFER_MAX_ITEMS + 1, &runtime))
		// The existing boolean census does not distinguish OOM from overflow.
		return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
	size_t runtime_present = 0;
	uint64_t max_owner_revision = result.to_owner_revision;
	for (const auto &[uid, item] : expected)
	{
		item_ownership_runtime_entry entry = {};
		if (!item_ownership_runtime_lookup(uid, &entry))
			continue;
		++runtime_present;
		const auto &input = payload.items[item.input_index];
		if (entry.root_item_uid != payload.selected_item_uid ||
		    entry.parent_item_uid != input.parent_item_uid || entry.vnum != input.vnum ||
		    !item_owner_identity_equal(entry.owner, payload.to_owner) ||
		    entry.state != item_custody_state::active ||
		    entry.item_revision != input.expected_item_revision + 1 ||
		    entry.owner_revision < result.to_owner_revision ||
		    entry.owner_revision > durable.owner_revision)
			return observed(ordinary_drop_observation_status::conflict, ESTALE, uid);
		max_owner_revision = std::max(max_owner_revision, entry.owner_revision);
	}
	for (const auto &entry : runtime)
		if (expected.find(entry.item_uid) == expected.end())
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					entry.item_uid);
	uint64_t cached_room_revision = 0;
	const bool has_cached_room =
		item_ownership_runtime_peek_owner_revision(payload.to_owner, &cached_room_revision);
	if (has_cached_room && cached_room_revision > durable.owner_revision)
		return observed(ordinary_drop_observation_status::conflict, ESTALE);
	if (!present && !runtime_present && runtime.empty())
		return observed(ordinary_drop_observation_status::absent, 0, 0,
				durable.owner_revision);
	if (present != payload.item_count || runtime_present != payload.item_count ||
	    runtime.size() != payload.item_count)
		return observed(ordinary_drop_observation_status::conflict, ESTALE);
	if (!has_cached_room || cached_room_revision < max_owner_revision)
		return observed(ordinary_drop_observation_status::conflict, ESTALE);
	P_obj root = expected.at(payload.selected_item_uid).object;
	if (root->loc_p != LOC_ROOM || root->loc.room != room ||
	    expected.at(payload.selected_item_uid).room_links != 1)
		return observed(ordinary_drop_observation_status::conflict, ESTALE,
				payload.selected_item_uid);
	std::vector<player_item_snapshot> actual;
	const auto capture = player_item_snapshot_tree_capture_literal(root, &actual, nullptr);
	if (capture == player_snapshot_capture_result::retryable_allocation_failure)
		return observed(ordinary_drop_observation_status::unavailable, ENOMEM);
	if (capture != player_snapshot_capture_result::ok || actual.size() != payload.item_count)
		return observed(ordinary_drop_observation_status::conflict, EBADMSG);
	for (const auto &[uid, item] : expected)
		if (item.child_links != (uid == payload.selected_item_uid ? 0u : 1u))
			return observed(ordinary_drop_observation_status::conflict, ESTALE, uid);
	for (size_t index = 0; index < actual.size(); ++index)
	{
		const auto &item = actual[index];
		const auto found = expected.find(item.object_uid);
		if (found == expected.end())
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					item.object_uid);
		const auto &input = payload.items[found->second.input_index];
		P_obj object = found->second.object;
		if (index == 0)
		{
			if (item.object_uid != payload.selected_item_uid ||
			    item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				return observed(ordinary_drop_observation_status::conflict, ESTALE);
		}
		else
		{
			if (item.parent_index < 0 ||
			    item.parent_index >= static_cast<int32_t>(index))
				return observed(ordinary_drop_observation_status::conflict,
						EBADMSG);
			const uint64_t parent = actual[item.parent_index].object_uid;
			if (parent != input.parent_item_uid || object->loc_p != LOC_INSIDE ||
			    object->loc.inside != expected.at(parent).object)
				return observed(ordinary_drop_observation_status::conflict, ESTALE,
						item.object_uid);
		}
		std::vector<uint8_t> bytes;
		if (!canonical_item(item, &bytes) ||
		    bytes != original.payloads[found->second.original_index])
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					item.object_uid);
	}
	return observed(ordinary_drop_observation_status::verified_existing, 0, 0,
			durable.owner_revision);
}

ordinary_drop_observation observe_transaction(MYSQL *connection, const critical_command &command,
					      const critical_completion &completion,
					      const item_transfer_payload &payload,
					      const item_transfer_result &result,
					      const sql_room_item_payload_batch &original,
					      bool publish_absent)
{
	economic_sql_item_transfer_context context;
	const unsigned int authority_error =
		economic_sql_item_transfer_lock(connection, command, &context);
	if (authority_error)
		// Existing authority helpers can flatten nested allocation failures.
		return observed(ordinary_drop_observation_status::unavailable, authority_error);
	sql_room_item_graph durable;
	errno = 0;
	if (!sql_room_item_payload_read(connection, payload.selected_item_uid, &durable))
		return observed(ordinary_drop_observation_status::unavailable, current_error());
	// Current season -> room -> custody/payload locks precede historical retained
	// payload locks. The inbox seam uses a nonlocking read, never a late write lock.
	const auto retained = critical_command_repository_verify_ordinary_drop_in_transaction(
		connection, command);
	if (retained.outcome == critical_apply_outcome::retryable_failure ||
	    retained.outcome == critical_apply_outcome::ambiguous_commit)
		return observed(ordinary_drop_observation_status::unavailable,
				retained.error_code ? retained.error_code : EAGAIN);
	if (retained.outcome != critical_apply_outcome::already_applied || retained.error_code ||
	    retained.failure_stage != critical_failure_stage::none ||
	    retained.durable_revision != completion.durable_revision ||
	    retained.result_size != completion.result_size ||
	    retained.result_payload != completion.result_payload)
		return observed(ordinary_drop_observation_status::conflict,
				retained.error_code ? retained.error_code : EBADMSG);
	if (!session_current(connection, context.session_id))
		return observed(ordinary_drop_observation_status::unavailable, ENOTCONN);
	auto observation = compare_graph(payload, result, original, durable);
	if (!session_current(connection, context.session_id))
		return observed(ordinary_drop_observation_status::unavailable, ENOTCONN);
	if (publish_absent && observation.status == ordinary_drop_observation_status::absent)
		return ordinary_drop_enrollment_owner::publish(connection, context.session_id,
							       command, completion, payload, result,
							       original, durable);
	return observation;
}
#endif
} // namespace

#ifndef __NO_MYSQL__
ordinary_drop_observation ordinary_drop_enrollment_owner::publish(
	MYSQL *connection, unsigned long session, const critical_command &command,
	const critical_completion &receipt, const item_transfer_payload &payload,
	const item_transfer_result &result, const sql_room_item_payload_batch &original,
	const sql_room_item_graph &durable)
{
	ordinary_drop_enrollment_owner owner(command, receipt);
	owner.connection_ = connection;
	owner.session_ = session;
	owner.index_ = obj_index;
	owner.room_ = real_room(static_cast<int>(payload.to_owner.id));
	if (!owner.index_ || owner.room_ < 0 || owner.room_ > top_of_world ||
	    original.items.empty() || original.items.size() != payload.item_count ||
	    original.items[0].object_uid != payload.selected_item_uid)
		return observed(ordinary_drop_observation_status::refused, EINVAL);
	if (top_of_objt < 0 || top_of_objt == INT_MAX)
		return observed(ordinary_drop_observation_status::refused, EINVAL);
	if (!recovery_object_templates_ready())
		return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
	owner.runtime_.reserve(durable.identities.size());
	for (size_t index = 0; index < durable.identities.size(); ++index)
	{
		const auto &identity = durable.identities[index];
		owner.runtime_.push_back({ identity.item_uid, identity.root_item_uid,
					   identity.parent_item_uid, identity.owner,
					   identity.item_revision, identity.owner_revision,
					   durable.items[index].vnum, identity.state });
	}
	std::vector<size_t> depths(original.items.size(), 0);
	for (size_t index = 0; index < original.items.size(); ++index)
	{
		const auto &literal = original.items[index];
		if (!index)
		{
			if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				return observed(ordinary_drop_observation_status::refused, EBADMSG);
			continue;
		}
		if (literal.parent_index < 0 || literal.parent_index >= static_cast<int32_t>(index))
			return observed(ordinary_drop_observation_status::refused, EBADMSG);
		const int parent_type = original.items[literal.parent_index].type;
		if (parent_type != ITEM_CONTAINER && parent_type != ITEM_QUIVER &&
		    parent_type != ITEM_STORAGE)
			return observed(ordinary_drop_observation_status::refused, EBADMSG,
					literal.object_uid);
		depths[index] = depths[literal.parent_index] + 1;
		if (depths[index] > static_cast<size_t>(top_of_objt) + 1)
			return observed(ordinary_drop_observation_status::refused, E2BIG,
					literal.object_uid);
	}
	owner.stages_.resize(original.items.size());
	owner.prototypes_.reserve(original.items.size());
	std::vector<P_obj> child_tails(original.items.size(), nullptr);
	// Resolve every trusted cache entry and aggregate signed counters before
	// acquiring any pooled object. Runtime cache misses never invoke a parser.
	for (const auto &literal : original.items)
	{
		const auto *prototype = find_recovery_object_template(literal.vnum);
		if (!prototype)
			return observed(ordinary_drop_observation_status::unsupported, ENOENT,
					literal.object_uid);
		const int number = prototype->R_num;
		if (number < 0 || number > top_of_objt ||
		    owner.index_[number].virtual_number != literal.vnum)
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					literal.object_uid);
		// These types can enroll allocating activity indexes or external ship
		// bindings. A future complete activity owner is required before admission.
		const auto activity_type = [](int type)
		{ return type == ITEM_TELEPORT || type == ITEM_SHIP || type == ITEM_BOAT; };
		if (activity_type(prototype->type) || activity_type(literal.type))
			return observed(ordinary_drop_observation_status::unsupported, ENOTSUP,
					literal.object_uid);
		const auto eligibility = inert_item_stage_eligibility(*prototype, literal);
		if (eligibility != inert_item_stage_result::ok)
			return observed(eligibility == inert_item_stage_result::unsupported ?
						ordinary_drop_observation_status::unsupported :
						ordinary_drop_observation_status::refused,
					eligibility == inert_item_stage_result::unsupported ?
						ENOTSUP :
						EBADMSG,
					literal.object_uid);
		owner.prototypes_.push_back(prototype);
		++owner.counts_[number];
	}
	for (const auto &[number, count] : owner.counts_)
		if (owner.index_[number].number < 0 ||
		    owner.index_[number].number > INT_MAX - count)
			return observed(ordinary_drop_observation_status::refused, EOVERFLOW);
	for (size_t index = 0; index < original.items.size(); ++index)
	{
		// Original retained canonical literals use slot0. The differently ordered
		// current SQL loader may use -1; compare_graph already normalizes that.
		const auto &literal = original.items[index];
		const auto prepared = prepare_inert_item_stage(*owner.prototypes_[index], literal,
							       owner.stages_[index]);
		if (prepared != inert_item_stage_result::ok)
			return observed(
				prepared == inert_item_stage_result::allocation_unavailable ?
					ordinary_drop_observation_status::unavailable :
				prepared == inert_item_stage_result::unsupported ?
					ordinary_drop_observation_status::unsupported :
					ordinary_drop_observation_status::refused,
				prepared == inert_item_stage_result::allocation_unavailable ?
					ENOMEM :
				prepared == inert_item_stage_result::unsupported ? ENOTSUP :
										   EBADMSG,
				literal.object_uid);
		if (!index)
		{
			if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				return observed(ordinary_drop_observation_status::refused, EBADMSG);
			continue;
		}
		if (literal.parent_index < 0 || literal.parent_index >= static_cast<int32_t>(index))
			return observed(ordinary_drop_observation_status::refused, EBADMSG);
		P_obj object = owner.stages_[index].object_;
		P_obj parent = owner.stages_[literal.parent_index].object_;
		object->loc_p = LOC_INSIDE;
		object->loc.inside = parent;
		P_obj &tail = child_tails[literal.parent_index];
		if (tail)
			tail->next_content = object;
		else
			parent->contains = object;
		tail = object;
	}

	// Prepare the exact room-light projection without logs, callbacks, signed
	// overflow or a walk that could accept a cyclic/foreign person list.
	std::vector<P_char> characters;
	size_t visits = 0;
	for (P_char character = character_list; character; character = character->next)
	{
		if (++visits > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		characters.push_back(character);
	}
	std::sort(characters.begin(), characters.end(), std::less<P_char>{});
	std::vector<bool> linked_characters(characters.size(), false);
	int light = 0;
	for (P_char character = world[owner.room_].people; character;
	     character = character->next_in_room)
	{
		if (++visits > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		const auto found = std::lower_bound(characters.begin(), characters.end(), character,
						    std::less<P_char>{});
		if (found == characters.end() || *found != character)
			return observed(ordinary_drop_observation_status::conflict, ENXIO);
		const size_t index = static_cast<size_t>(found - characters.begin());
		if (linked_characters[index] || character->in_room != owner.room_)
			return observed(ordinary_drop_observation_status::conflict, ELOOP);
		linked_characters[index] = true;
		if (character->light > 0)
			light = std::min(127, light + std::min(127, int(character->light)));
	}
	for (P_obj object = world[owner.room_].contents; object; object = object->next_content)
	{
		if (++visits > census_limit)
			return observed(ordinary_drop_observation_status::unavailable, E2BIG);
		if (object->loc_p != LOC_ROOM || object->loc.room != owner.room_)
			return observed(ordinary_drop_observation_status::conflict, ESTALE);
		if ((object->extra_flags & ITEM_LIT) ||
		    (object->type == ITEM_LIGHT && object->value[2] == -1))
			light = std::min(127, light + 1);
	}
	P_obj root = owner.stages_[0].object_;
	if ((root->extra_flags & ITEM_LIT) || (root->type == ITEM_LIGHT && root->value[2] == -1))
		light = std::min(127, light + 1);
	owner.light_ = light;

	// Nothing above publishes an object. Hold the original locked authority,
	// then take a fresh complete absence census immediately before mutation.
	if (!session_current(owner.connection_, owner.session_))
		return observed(ordinary_drop_observation_status::unavailable, ENOTCONN);
	const auto fresh = compare_graph(payload, result, original, durable);
	if (fresh.status != ordinary_drop_observation_status::absent)
		return fresh;
	if (!session_current(owner.connection_, owner.session_) || obj_index != owner.index_ ||
	    !recovery_object_templates_ready() || world[owner.room_].number <= 0 ||
	    static_cast<uint64_t>(world[owner.room_].number) != payload.to_owner.id ||
	    !critical_operation_id_equal(owner.command_.operation_id, owner.receipt_.operation_id))
		return observed(ordinary_drop_observation_status::unavailable, ESTALE);
	for (size_t index = 0; index < original.items.size(); ++index)
	{
		const auto &literal = original.items[index];
		const auto *prototype = owner.prototypes_[index];
		if (find_recovery_object_template(literal.vnum) != prototype ||
		    prototype->R_num != owner.stages_[index].object_->R_num ||
		    prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    owner.index_[prototype->R_num].virtual_number != literal.vnum ||
		    owner.index_[prototype->R_num].func.obj ||
		    inert_item_stage_eligibility(*prototype, literal) !=
			    inert_item_stage_result::ok)
			return observed(ordinary_drop_observation_status::conflict, ESTALE,
					literal.object_uid);
	}
	for (const auto &[number, count] : owner.counts_)
		if (owner.index_[number].number < 0 ||
		    owner.index_[number].number > INT_MAX - count)
			return observed(ordinary_drop_observation_status::refused, EOVERFLOW);
	// Final fallible step rolls back entry/owner values on allocation refusal.
	// No callback, SQL, allocation or other fallible operation follows success.
	if (!item_ownership_runtime_hydrate_many_atomic(owner.runtime_.data(),
							owner.runtime_.size()))
		return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
	owner.enroll();
	return observed(ordinary_drop_observation_status::published, 0, 0, durable.owner_revision);
}

void ordinary_drop_enrollment_owner::enroll() noexcept
{
	for (auto &stage : stages_)
	{
		P_obj object = stage.object_;
		object->next = object_list;
		if (object_list)
			object_list->prev = object;
		object_list = object;
	}
	for (const auto &[number, count] : counts_)
		index_[number].number += count;
	P_obj root = stages_[0].object_;
	root->loc_p = LOC_ROOM;
	root->loc.room = room_;
	root->next_content = world[room_].contents;
	world[room_].contents = root;
	world[room_].light = light_;
	for (auto &stage : stages_)
	{
		stage.object_ = nullptr;
		stage.pool_ = nullptr;
	}
}
#endif

static ordinary_drop_observation recover_drop(const critical_command &command,
					      const critical_completion &completion,
					      bool publish_absent) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)publish_absent;
	return observed(ordinary_drop_observation_status::unsupported, ENOTSUP);
#else
	if (!nevent_is_game_thread())
		return observed(ordinary_drop_observation_status::refused, EPERM);
	try
	{
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.type != critical_command_type::item_transfer ||
		    !command.publication_required)
			return observed(ordinary_drop_observation_status::unsupported, ENOTSUP);
		if (!critical_command_envelope_valid(command) ||
		    !critical_operation_id_equal(command.operation_id, completion.operation_id) ||
		    completion.disposition != critical_completion_disposition::execution ||
		    (completion.outcome != critical_apply_outcome::applied &&
		     completion.outcome != critical_apply_outcome::already_applied) ||
		    completion.error_code ||
		    completion.failure_stage != critical_failure_stage::none ||
		    completion.result_size != ITEM_TRANSFER_RESULT_BYTES)
			return observed(ordinary_drop_observation_status::refused, EBADMSG);
		// The existing support predicate hides allocation failure behind false.
		// Do not classify an unproven supported command as permanently rejected.
		if (!item_transfer_accounting_command_supported(command))
			return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
		auto payload = std::make_unique<item_transfer_payload>();
		if (!item_transfer_command_decode_payload(command, payload.get()))
			// Keep an unproven second decode unavailable; no authority exists.
			return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
		sql_room_item_payload_batch original;
		errno = 0;
		if (!sql_room_item_payload_capture(*payload, &original))
		{
			const auto error = current_error();
			return observed(error == ENOTSUP ?
						ordinary_drop_observation_status::unsupported :
						ordinary_drop_observation_status::unavailable,
					error);
		}
		item_transfer_result result = {};
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical = {};
		uint64_t maximum = 0;
		for (size_t index = 0; index < payload->item_count; ++index)
		{
			if (payload->items[index].item_uid == UINT64_MAX)
				return observed(ordinary_drop_observation_status::refused, EBADMSG);
			maximum =
				std::max(maximum, payload->items[index].expected_item_revision + 1);
		}
		if (payload->expected_from_revision == UINT64_MAX ||
		    payload->expected_to_revision == UINT64_MAX ||
		    !item_transfer_command_decode_result(completion.result_payload.data(),
							 completion.result_size, &result) ||
		    !item_transfer_command_encode_result(result, &canonical) ||
		    !std::equal(canonical.begin(), canonical.end(),
				completion.result_payload.begin()) ||
		    result.root_item_uid != payload->selected_item_uid ||
		    result.item_count != payload->item_count ||
		    result.from_owner_revision != payload->expected_from_revision + 1 ||
		    result.to_owner_revision != payload->expected_to_revision + 1 ||
		    result.max_item_revision != maximum || result.corpse_revision ||
		    result.collector_catalog_changed ||
		    completion.durable_revision != std::max({ result.from_owner_revision,
							      result.to_owner_revision, maximum }))
			return observed(ordinary_drop_observation_status::refused, EBADMSG);

		player_sql_pool_lease lease(sql_pool_acquire());
		MYSQL *connection = lease.get();
		if (!connection)
			return observed(ordinary_drop_observation_status::unavailable, EAGAIN);
		const unsigned int idle_error = player_sql_idle_error(connection);
		if (idle_error)
			return observed(ordinary_drop_observation_status::unavailable, idle_error);
		player_sql_cleanup proof;
		player_sql_transaction_cleanup transaction(connection, proof);
		transaction.starting();
		ordinary_drop_observation observation;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
			{
				const unsigned int error = mysql_errno(connection);
				observation =
					observed(ordinary_drop_observation_status::unavailable,
						 error ? error : EIO);
			}
			else if (!session_current(connection, proof.original_session))
				observation = observed(
					ordinary_drop_observation_status::unavailable, ENOTCONN);
			else
				observation = observe_transaction(connection, command, completion,
								  *payload, result, original,
								  publish_absent);
		}
		catch (const std::bad_alloc &)
		{
			observation =
				observed(ordinary_drop_observation_status::unavailable, ENOMEM);
		}
		catch (...)
		{
			observation = observed(ordinary_drop_observation_status::unavailable, EIO);
		}
		transaction.finish();
		if (!proof.rollback_confirmed || proof.cleanup_error ||
		    proof.disposition != player_sql_cleanup_disposition::idle_verified)
			return observed(ordinary_drop_observation_status::unavailable,
					proof.cleanup_error ? proof.cleanup_error : EIO);
		lease.reuse(proof);
		return observation;
	}
	catch (const std::bad_alloc &)
	{
		return observed(ordinary_drop_observation_status::unavailable, ENOMEM);
	}
	catch (...)
	{
		return observed(ordinary_drop_observation_status::unavailable, EIO);
	}
#endif
}

ordinary_drop_observation
ordinary_drop_recovery_observe_existing(const critical_command &command,
					const critical_completion &completion) noexcept
{
	return recover_drop(command, completion, false);
}

ordinary_drop_observation
ordinary_drop_recovery_publish(const critical_command &command,
			       const critical_completion &completion) noexcept
{
	return recover_drop(command, completion, true);
}
