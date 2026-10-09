#include "world/zone_reset_room_publication.h"
#include "world/handler.h"
#include "persistence/economic_sql_zone_reset_item_transaction.h"
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#endif
#include "world/db.h"
#include "world/events.h"
#include "world/object_template.h"
#include "world/vnum.obj.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_mode.h"
#include "persistence/zone_reset_item_terminal_sql.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <climits>
#include <type_traits>
#include <map>
#include <memory>
#include <utility>

extern P_obj object_list;
extern P_room world;
extern int top_of_world;

struct zone_reset_room_publication_stage::implementation
{
	// Provenance comparisons retained with the actual borrowed factory handles.
	// They never construct a flat factory scope or grant native permission.
	bool flat_backend = false;
	std::string selected_root;
	std::vector<economic_source_event> factory_sources;
	sql_room_item_graph graph;
	critical_command cold_command;
	zone_reset_item_image original;
	std::vector<std::unique_ptr<quest_mobile_native_item_stage>> stages;
	std::vector<P_obj> objects;
	std::vector<quest_mobile_native_item_stage *> stage_pointers;
	std::vector<item_ownership_runtime_entry> custody;
	std::vector<size_t> original_indices;
	std::unordered_set<uint64_t> next_published;
	// Recorded complete terminal BODY, with original-index correlations. These
	// observations are populated only by same-session authenticated cold reads.
	std::vector<uint8_t> cold_terminal;
	std::vector<quest_mobile_native_item_progress> cold_progress;
	std::vector<std::vector<quest_mobile_native_item_effect>> cold_effects;
	int room = NOWHERE;
	bool admitted = false, consumed = false, placed = false, bookkeeping_published = false;
	bool warm = false, placement_started = false, placement_returned = false;
	bool cold_adoption = false, cold_shape = false;
	bool cold_reconstruction = false, cold_forest_ready = false;
	bool cold_pending = false, cold_pending_ready = false;
	bool cold_present_ready = false;
	size_t cold_enrollment_next = 0;
	zone_reset_original_room_placement_stage *original_placement = nullptr;
};
namespace
{
#ifndef __NO_MYSQL__
bool cold_session(MYSQL *connection, unsigned long session) noexcept
{
	if (!connection || !session || mysql_thread_id(connection) != session ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return false;
	using mysql_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	mysql_flag reconnect = false;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
#endif
bool same_item(player_item_snapshot actual, player_item_snapshot expected)
{
	actual.parent_index = expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	actual.equipment_slot = expected.equipment_slot = 0;
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode({ actual }, &a) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode({ expected }, &b) ==
		       player_snapshot_codec_result::ok &&
	       a == b;
}
bool same_custody(const item_ownership_runtime_entry &a, const item_ownership_runtime_entry &b)
{
	return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.owner_revision <= b.owner_revision && a.vnum == b.vnum && a.state == b.state;
}
bool absent_native(const std::map<uint64_t, size_t> &selected)
{
	for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	P_obj previous = nullptr;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (object->prev != previous || selected.count(object->obj_uid))
			return false;
		previous = object;
	}
	return true;
}
}
bool zone_reset_room_publication_owner::discard_unpublished(
	zone_reset_room_publication_stage *stage) noexcept
{
	if (!stage || !nevent_is_game_thread())
		return false;
	if (!stage->state_)
		return true;
	auto &state = *stage->state_;
	if (state.warm || state.admitted || state.consumed)
		return false;
	// Every body still belongs to its detached factory stage. Break only our
	// callback-free local links before disposing each stage separately.
	for (P_obj object : state.objects)
		if (object)
		{
			object->contains = nullptr;
			object->next_content = nullptr;
			object->loc_p = LOC_NOWHERE;
			object->loc.room = NOWHERE;
		}
	for (size_t next = state.stages.size(); next; --next)
	{
		const auto at = next - 1;
		if (state.stages[at] && !state.stages[at]->discard_unadmitted())
			return false;
		state.objects[at] = nullptr;
		state.stage_pointers[at] = nullptr;
		state.stages[at].reset();
	}
	delete stage->state_;
	stage->state_ = nullptr;
	return true;
}
bool zone_reset_room_publication_owner::prepare(const sql_room_item_graph &graph,
						const std::unordered_set<uint64_t> &published,
						zone_reset_room_publication_stage *output) noexcept
{
	if (!output || output->state_ || !nevent_is_game_thread() ||
	    !persistence_mode_requires_mysql() || !recovery_object_templates_ready() ||
	    !graph.creation_origin || !graph.creation_origin->present ||
	    graph.owner.type != item_owner_type::room || !graph.owner.id ||
	    graph.owner.id > INT32_MAX || graph.owner.context_id || !graph.owner_revision ||
	    graph.items.empty() || graph.items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    graph.items.size() != graph.identities.size())
		return false;
	zone_reset_room_publication_stage candidate;
	try
	{
		auto state = std::make_unique<zone_reset_room_publication_stage::implementation>();
		state->graph = graph;
		state->room = real_room(static_cast<int>(graph.owner.id));
		if (!world || state->room < 0 || state->room > top_of_world ||
		    zone_reset_item_command_decode(graph.creation_origin->original,
						   &state->original) !=
			    economic_accounting_error::ok)
			return false;
		item_transfer_result receipt{};
		if (!item_transfer_command_decode_result(graph.creation_origin->result.data(),
							 graph.creation_origin->result.size(),
							 &receipt) ||
		    state->original.items.empty() ||
		    receipt.root_item_uid != state->original.items[0].object_uid ||
		    state->original.room_vnum != static_cast<int32_t>(graph.owner.id) ||
		    receipt.from_owner_revision ||
		    receipt.to_owner_revision != state->original.expected_room_revision + 1 ||
		    graph.owner_revision < receipt.to_owner_revision ||
		    receipt.item_count != state->original.items.size() ||
		    receipt.max_item_revision != 1 || receipt.corpse_revision ||
		    receipt.collector_catalog_changed ||
		    !native_mobile_birth_recipe_valid(state->original.items,
						      state->original.recipes))
			return false;
		std::map<uint64_t, size_t> original, selected;
		for (size_t i = 0; i < state->original.items.size(); ++i)
			if (!original.emplace(state->original.items[i].object_uid, i).second)
				return false;
		state->objects.reserve(graph.items.size());
		state->stages.reserve(graph.items.size());
		state->stage_pointers.reserve(graph.items.size());
		state->custody.reserve(graph.items.size());
		state->original_indices.reserve(graph.items.size());
		state->next_published = published;
		for (size_t i = 0; i < graph.items.size(); ++i)
		{
			const auto &item = graph.items[i];
			const auto &identity = graph.identities[i];
			const auto found = original.find(item.object_uid);
			if (found == original.end() ||
			    !selected.emplace(item.object_uid, i).second ||
			    published.count(item.object_uid) || item.type == ITEM_CORPSE ||
			    (item.extra_flags & ITEM_ARTIFACT) ||
			    (item.type == ITEM_MONEY) != (item.vnum == VOBJ_COINS) ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(i) ||
			    (i == 0 ? item.parent_index != PLAYER_SNAPSHOT_NO_PARENT :
				      item.parent_index < 0) ||
			    !same_item(item, state->original.items[found->second]))
				return false;
			const auto &birth = state->original.items[found->second];
			const auto birth_parent =
				birth.parent_index < 0 ?
					0 :
					state->original.items[birth.parent_index].object_uid;
			const auto parent = item.parent_index < 0 ?
						    0 :
						    graph.items[item.parent_index].object_uid;
			if (identity.item_uid != item.object_uid ||
			    identity.root_item_uid != receipt.root_item_uid ||
			    identity.parent_item_uid != parent || parent != birth_parent ||
			    !item_owner_identity_equal(identity.owner, graph.owner) ||
			    identity.item_revision != 1 ||
			    identity.owner_revision != graph.owner_revision ||
			    identity.state != item_custody_state::active ||
			    (i == 0 && item.object_uid != receipt.root_item_uid))
				return false;
			state->original_indices.push_back(found->second);
			state->custody.push_back({ identity.item_uid, identity.root_item_uid,
						   identity.parent_item_uid, identity.owner,
						   identity.item_revision, identity.owner_revision,
						   item.vnum, identity.state });
			state->next_published.insert(item.object_uid);
		}
		if (!absent_native(selected))
			return false;
		std::vector<item_ownership_runtime_entry> cached;
		if (!item_ownership_runtime_snapshot_active_root(receipt.root_item_uid,
								 ITEM_TRANSFER_MAX_ITEMS, &cached))
			return false;
		for (const auto &entry : cached)
		{
			const auto found = selected.find(entry.item_uid);
			if (found == selected.end() ||
			    !same_custody(entry, state->custody[found->second]))
				return false;
		}
		candidate.state_ = state.release();
		auto &prepared = *candidate.state_;
		for (size_t i = 0; i < graph.items.size(); ++i)
		{
			auto stage = std::make_unique<quest_mobile_native_item_stage>();
			auto literal = graph.items[i];
			literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			literal.equipment_slot = 0;
			if (!quest_mobile_native_item_stage::restore_bound(
				    literal,
				    prepared.original.recipes[prepared.original_indices[i]],
				    stage.get()))
			{
				discard_unpublished(&candidate);
				return false;
			}
			P_obj object = stage->object();
			if (!object || object->loc_p != LOC_NOWHERE ||
			    object->loc.room != NOWHERE || object->next || object->prev ||
			    object->contains || object->next_content ||
			    object->obj_uid != literal.object_uid)
			{
				stage->discard_unadmitted();
				discard_unpublished(&candidate);
				return false;
			}
			prepared.objects.push_back(object);
			prepared.stage_pointers.push_back(stage.get());
			prepared.stages.push_back(std::move(stage));
		}
		std::vector<P_obj> tails(graph.items.size(), nullptr);
		for (size_t i = 1; i < graph.items.size(); ++i)
		{
			const auto parent = static_cast<size_t>(graph.items[i].parent_index);
			P_obj object = prepared.objects[i], container = prepared.objects[parent];
			object->loc_p = LOC_INSIDE;
			object->loc.inside = container;
			if (tails[parent])
				tails[parent]->next_content = object;
			else
				container->contains = object;
			tails[parent] = object;
		}
		std::vector<player_item_snapshot> observed;
		if (player_item_snapshot_tree_capture_literal(prepared.objects[0], &observed,
							      nullptr) !=
			    player_snapshot_capture_result::ok ||
		    observed.size() != graph.items.size())
		{
			discard_unpublished(&candidate);
			return false;
		}
		std::unordered_set<uint64_t> seen;
		for (size_t i = 0; i < observed.size(); ++i)
		{
			const auto &item = observed[i];
			const auto found = selected.find(item.object_uid);
			if (found == selected.end() || !seen.insert(item.object_uid).second ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(i) ||
			    (item.parent_index < 0 ? 0 : observed[item.parent_index].object_uid) !=
				    graph.identities[found->second].parent_item_uid ||
			    !same_item(item, graph.items[found->second]))
			{
				discard_unpublished(&candidate);
				return false;
			}
		}
		if (!absent_native(selected))
		{
			discard_unpublished(&candidate);
			return false;
		}
		output->state_ = std::exchange(candidate.state_, nullptr);
		return true;
	}
	catch (...)
	{
		discard_unpublished(&candidate);
		return false;
	}
}
bool zone_reset_room_publication_owner::publish(const sql_room_item_graph &graph,
						std::unordered_set<uint64_t> *published) noexcept
{
	if (!published)
		return false;
	zone_reset_room_publication_stage prepared;
	if (!prepare(graph, *published, &prepared))
		return false;
	// A retained economic root is not a factory service-progress receipt. The
	// shared context owner must supply that authentic handoff before consuming
	// this complete detached forest. Never fabricate a completed stage prefix.
	discard_unpublished(&prepared);
	return false;
}
bool zone_reset_room_publication_owner::empty(
	const zone_reset_room_publication_stage &stage) noexcept
{
	return !stage.state_;
}

bool zone_reset_room_publication_owner::publish_locked(
	MYSQL *connection, uint64_t root_uid, std::unordered_set<uint64_t> *published,
	zone_reset_room_publication_stage *held) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)root_uid;
	(void)published;
	(void)held;
	return false;
#else
	if (!connection || !root_uid || !published || !held || !nevent_is_game_thread() ||
	    !persistence_mode_requires_mysql())
		return false;
	zone_reset_room_publication_stage candidate;
	const auto refuse_candidate = [&]() noexcept
	{
		// An unexpected cleanup refusal retains every remaining actual private
		// stage too; never orphan raw ownership simply because admission has
		// not occurred. The original caller aborts room restoration on refusal.
		if (candidate.state_ && !discard_unpublished(&candidate) && !held->state_)
			held->state_ = std::exchange(candidate.state_, nullptr);
		return false;
	};
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		if (!cold_session(connection, session))
			return false;
		// Never consume the caller's DTO. This same transaction authenticates
		// original root, current season/room/custody/literals and actual money
		// head, including complete selected descendants and physical absence.
		sql_room_item_graph graph;
		if (!sql_room_item_payload_read(connection, root_uid, &graph) ||
		    !graph.creation_origin || !graph.creation_origin->present ||
		    !cold_session(connection, session))
			return false;
		zone_reset_item_retained_terminal terminal;
		if (zone_reset_item_terminal_sql_owner::load_locked(
			    connection, graph.creation_origin->original.operation_id, &terminal) ||
		    !terminal.present || !cold_session(connection, session) ||
		    !critical_command_equal(terminal.original, graph.creation_origin->original) ||
		    terminal.context.receipt.result_size != graph.creation_origin->result.size() ||
		    !std::equal(graph.creation_origin->result.begin(),
				graph.creation_origin->result.end(),
				terminal.context.receipt.result_payload.begin()))
			return false;
		if (held->state_)
		{
			const auto &state = *held->state_;
			// An existing hot or unrelated stage cannot be adopted as a cold
			// carrier. A changed current cut must not silently rebind its actual
			// objects/services to a new room clock or descendant selection.
			if (state.cold_terminal.empty() ||
			    state.cold_terminal != terminal.canonical ||
			    !state.graph.creation_origin ||
			    !critical_command_equal(state.graph.creation_origin->original,
						    terminal.original) ||
			    !item_owner_identity_equal(state.graph.owner, graph.owner) ||
			    state.graph.owner_revision != graph.owner_revision ||
			    state.graph.items.size() != graph.items.size() ||
			    state.graph.identities.size() != graph.identities.size())
				return false;
			for (size_t i = 0; i < graph.items.size(); ++i)
			{
				const auto &a = state.graph.identities[i];
				const auto &b = graph.identities[i];
				if (a.item_uid != b.item_uid ||
				    a.root_item_uid != b.root_item_uid ||
				    a.parent_item_uid != b.parent_item_uid ||
				    !item_owner_identity_equal(a.owner, b.owner) ||
				    a.item_revision != b.item_revision ||
				    a.owner_revision != b.owner_revision ||
				    a.quantity != b.quantity ||
				    a.override_mask != b.override_mask || a.state != b.state ||
				    !same_item(state.graph.items[i], graph.items[i]))
					return false;
			}
		}
		else
		{
			if (!prepare(graph, *published, &candidate))
				return false;
			auto &state = *candidate.state_;
			state.cold_progress.reserve(state.stages.size());
			state.cold_effects.reserve(state.stages.size());
			for (size_t i = 0; i < state.stages.size(); ++i)
			{
				const size_t original_at = state.original_indices[i];
				if (original_at >= terminal.context.items.size())
				{
					return refuse_candidate();
				}
				const auto &recorded = terminal.context.items[original_at];
				if (recorded.object_uid != state.objects[i]->obj_uid ||
				    !recorded.admitted || !recorded.published ||
				    recorded.current_step_started ||
				    recorded.next_step != step_count(candidate, i) ||
				    recorded.effects.size() != step_count(candidate, i))
				{
					return refuse_candidate();
				}
				// Exact recorded flags/prefix, never synthetic completed steps.
				state.cold_progress.push_back(
					{ recorded.next_step, recorded.current_step_started,
					  recorded.admitted, recorded.published });
				auto &effects = state.cold_effects.emplace_back();
				effects.reserve(recorded.effects.size());
				for (const auto &effect : recorded.effects)
					effects.push_back({ effect.started, effect.returned,
							    effect.succeeded, effect.periodic });
			}
			state.cold_terminal = std::move(terminal.canonical);
			if (!cold_session(connection, session) || !retain_admitted(candidate))
			{
				return refuse_candidate();
			}
			// Real retained admission prevents cleanup from this point. Transfer
			// ownership before the first global/cache/service mutation so every
			// uncertain return keeps actual objects and original metadata alive.
			held->state_ = std::exchange(candidate.state_, nullptr);
		}
		auto &state = *held->state_;
		if (state.cold_progress.size() != state.stages.size() ||
		    state.cold_effects.size() != state.stages.size() ||
		    !cold_session(connection, session))
			return false;
		if (!state.consumed && !consume(*held, published))
			return false;
		if (!verify_current(*held))
			return false;
		for (size_t i = 0; i < state.stages.size(); ++i)
		{
			if (!cold_session(connection, session) ||
			    !rebuild_enrollment(*held, i, state.cold_progress[i],
						state.cold_effects[i]))
				return false;
			// Original helper authenticates existing rebuilt enrollment on a
			// retry. Its vanished-event latch refuses instead of repeating a
			// service. Never invoke original publication_step callbacks here.
		}
		if (!cold_session(connection, session) || !place(*held) || !verify_current(*held) ||
		    !mark_published(*held, published) || !cold_session(connection, session))
			return false;
		// Caller still owns its original transaction and verifies termination;
		// this only releases genuinely completed factory metadata after exact
		// physical/cache/service proof. Success guarantees an empty handle.
		return release_completed(*held);
	}
	catch (...)
	{
		return refuse_candidate(); // held admitted/consumed stages are never discarded.
	}
#endif
}

size_t zone_reset_room_publication_owner::item_count(
	const zone_reset_room_publication_stage &stage) noexcept
{
	return stage.state_ ? stage.state_->stage_pointers.size() : 0;
}
size_t
zone_reset_room_publication_owner::original_index(const zone_reset_room_publication_stage &stage,
						  size_t at) noexcept
{
	return stage.state_ && at < stage.state_->original_indices.size() ?
		       stage.state_->original_indices[at] :
		       SIZE_MAX;
}
const zone_reset_item_image *
zone_reset_room_publication_owner::original(const zone_reset_room_publication_stage &stage) noexcept
{
	return stage.state_ ? &stage.state_->original : nullptr;
}
const sql_room_item_graph *
zone_reset_room_publication_owner::current(const zone_reset_room_publication_stage &stage) noexcept
{
	return stage.state_ ? &stage.state_->graph : nullptr;
}
bool zone_reset_room_publication_owner::read_progress(
	const zone_reset_room_publication_stage &stage, size_t at,
	quest_mobile_native_item_progress *progress) noexcept
{
	return stage.state_ && at < stage.state_->stage_pointers.size() &&
	       stage.state_->stage_pointers[at]->read_progress(progress);
}
size_t zone_reset_room_publication_owner::step_count(const zone_reset_room_publication_stage &stage,
						     size_t at) noexcept
{
	return stage.state_ && at < stage.state_->stage_pointers.size() ?
		       stage.state_->stage_pointers[at]->publication_step_count() :
		       0;
}
bool zone_reset_room_publication_owner::retain_admitted(
	zone_reset_room_publication_stage &stage) noexcept
{
	if (!stage.state_ || !nevent_is_game_thread() || stage.state_->consumed)
		return false;
	auto &state = *stage.state_;
	if (state.stage_pointers.empty() || state.stage_pointers.size() != state.objects.size())
		return false;
	for (size_t i = 0; i < state.stage_pointers.size(); ++i)
	{
		quest_mobile_native_item_progress progress{};
		if (!state.stage_pointers[i] ||
		    state.stage_pointers[i]->object() != state.objects[i] || !state.objects[i] ||
		    !state.stage_pointers[i]->read_progress(&progress) || progress.published ||
		    progress.current_step_started || progress.next_step ||
		    progress.admitted != state.admitted)
			return false;
	}
	if (state.admitted)
		return true;
	// Only the genuine context owner calls this after its admitted carrier/SQL
	// proof. Retain the actual factory metadata, never fabricate service progress.
	for (auto &item : state.stage_pointers)
		item->retain_admitted();
	state.admitted = true;
	return true;
}
bool zone_reset_room_publication_owner::consume(zone_reset_room_publication_stage &stage,
						std::unordered_set<uint64_t> *published) noexcept
{
	if (!stage.state_ || !published || !nevent_is_game_thread() || !stage.state_->admitted ||
	    stage.state_->consumed)
		return false;
	try
	{
		auto &state = *stage.state_;
		// A held preparation may coexist with other completed roots. Refresh the
		// complete UID union before consuming any native stage, never overwrite a
		// later publisher's bookkeeping with the preparation-time snapshot.
		if (state.warm)
		{
			// The original warm caller allocated and charged this exact union
			// before its intent checkpoint. Do not grow holders after that cut.
			if (state.next_published.size() !=
			    published->size() + state.graph.items.size())
				return false;
			for (const auto uid : *published)
				if (!state.next_published.count(uid))
					return false;
			for (const auto &item : state.graph.items)
				if (published->count(item.object_uid) ||
				    !state.next_published.count(item.object_uid))
					return false;
		}
		else
		{
			state.next_published = *published;
			for (const auto &item : state.graph.items)
				if (!state.next_published.insert(item.object_uid).second)
					return false;
		}
		if (!world || state.room < 0 || state.room > top_of_world ||
		    state.objects[0]->loc_p != LOC_NOWHERE ||
		    state.objects[0]->loc.room != NOWHERE || state.objects[0]->next_content)
			return false;
		if (!quest_mobile_native_item_stage::publish_many(state.stage_pointers,
								  state.objects, state.custody))
			return false;
		state.consumed = true;
		// The complete batch is in the actual global list/cache. Root room splice
		// and factory effects remain separate real journaled context steps.
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_room_publication_owner::verify_current(
	const zone_reset_room_publication_stage &stage) noexcept
{
	if (!stage.state_ || !stage.state_->consumed || !nevent_is_game_thread())
		return false;
	try
	{
		const auto &state = *stage.state_;
		std::map<uint64_t, size_t> expected;
		for (size_t i = 0; i < state.graph.items.size(); ++i)
			if (!expected.emplace(state.graph.items[i].object_uid, i).second)
				return false;
		for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		std::unordered_set<uint64_t> seen;
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (object->prev != previous)
				return false;
			previous = object;
			const auto found = expected.find(object->obj_uid);
			if (found != expected.end() && (state.objects[found->second] != object ||
							!seen.insert(object->obj_uid).second))
				return false;
		}
		if (seen.size() != expected.size())
			return false;
		for (size_t i = 1; i < state.objects.size(); ++i)
		{
			const auto parent = expected.find(state.custody[i].parent_item_uid);
			if (parent == expected.end() || state.objects[i]->loc_p != LOC_INSIDE ||
			    state.objects[i]->loc.inside != state.objects[parent->second])
				return false;
		}
		P_obj root = state.objects[0];
		if (state.placed)
		{
			if (!world || state.room < 0 || state.room > top_of_world ||
			    root->loc_p != LOC_ROOM || root->loc.room != state.room)
				return false;
			std::unordered_set<P_obj> room_seen;
			size_t matches = 0;
			for (P_obj object = world[state.room].contents; object;
			     object = object->next_content)
			{
				if (!room_seen.insert(object).second || object->loc_p != LOC_ROOM ||
				    object->loc.room != state.room)
					return false;
				if (object == root)
					++matches;
			}
			if (matches != 1)
				return false;
		}
		else if (root->loc_p != LOC_NOWHERE || root->loc.room != NOWHERE ||
			 root->next_content)
			return false;
		std::vector<player_item_snapshot> observed;
		if (player_item_snapshot_tree_capture_literal(root, &observed, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    observed.size() != expected.size())
			return false;
		seen.clear();
		for (size_t i = 0; i < observed.size(); ++i)
		{
			const auto &item = observed[i];
			const auto found = expected.find(item.object_uid);
			if (found == expected.end() || !seen.insert(item.object_uid).second ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(i) ||
			    (item.parent_index < 0 ? 0 : observed[item.parent_index].object_uid) !=
				    state.custody[found->second].parent_item_uid ||
			    !same_item(item, state.graph.items[found->second]))
				return false;
		}
		std::vector<uint64_t> selected;
		selected.reserve(expected.size());
		for (const auto &[uid, at] : expected)
			selected.push_back(uid);
		std::vector<item_ownership_runtime_entry> actual;
		// Include every current selected UID/root/parent claim, even a foreign
		// root whose child points into this forest. Historical inactive custody
		// remains intact and cannot substitute for a current graph member.
		if (!item_ownership_runtime_published_native_observer::snapshot_links(
			    selected, ITEM_TRANSFER_MAX_ITEMS, &actual) ||
		    actual.size() != expected.size())
			return false;
		for (const auto &entry : actual)
		{
			const auto found = expected.find(entry.item_uid);
			if (found == expected.end() ||
			    !same_custody(entry, state.custody[found->second]) ||
			    entry.owner_revision != state.graph.owner_revision)
				return false;
		}
		uint64_t revision = 0;
		return item_ownership_runtime_peek_owner_revision(state.graph.owner, &revision) &&
		       revision == state.graph.owner_revision;
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_room_publication_owner::place(zone_reset_room_publication_stage &stage) noexcept
{
	if (!stage.state_ || !verify_current(stage))
		return false;
	auto &state = *stage.state_;
	if (state.placed)
		return true;
	for (const auto &item : state.stage_pointers)
	{
		quest_mobile_native_item_progress progress{};
		if (!item->read_progress(&progress) || !progress.admitted || !progress.published ||
		    progress.current_step_started ||
		    progress.next_step != item->publication_step_count())
			return false;
	}
	P_obj object = state.objects[0];
	// Restore current persisted topology without replaying reset/drop gameplay,
	// merging even zero/nested coins, changing decay or initiating falling.
	object->loc_p = LOC_ROOM;
	object->loc.room = state.room;
	object->next_content = world[state.room].contents;
	world[state.room].contents = object;
	state.placed = true;
	if (IS_SET(object->extra_flags, ITEM_LIT) ||
	    (object->type == ITEM_LIGHT && object->value[2] == -1))
		room_light(state.room, REAL);
	return true;
}
bool zone_reset_room_publication_owner::service_step(
	zone_reset_room_publication_stage &stage, size_t at, size_t step,
	quest_mobile_native_item_effect &effect) noexcept
{
	return stage.state_ && stage.state_->consumed && !stage.state_->placed &&
	       at < stage.state_->stage_pointers.size() &&
	       stage.state_->stage_pointers[at]->publication_step(step, stage.state_->objects[at],
								  effect);
}
bool zone_reset_room_publication_owner::rebuild_enrollment(
	zone_reset_room_publication_stage &stage, size_t at,
	const quest_mobile_native_item_progress &progress,
	std::span<const quest_mobile_native_item_effect> effects) noexcept
{
	if (!stage.state_ || !stage.state_->consumed || at >= stage.state_->stage_pointers.size())
		return false;
	auto &state = *stage.state_;
	// The genuine context owner authenticates these recorded facts first. The
	// original helper restores services and the recorded prefix only after actual
	// enrollment validation/rebuild; it never guesses completed factory steps.
	return state.stage_pointers[at]->rebuild_enrollment(
		state.objects[at], state.original.recipes[state.original_indices[at]], progress,
		effects);
}
bool zone_reset_room_publication_owner::mark_published(
	zone_reset_room_publication_stage &stage, std::unordered_set<uint64_t> *published) noexcept
{
	if (!stage.state_ || !published || !stage.state_->placed || !verify_current(stage))
		return false;
	auto &state = *stage.state_;
	if (state.bookkeeping_published)
	{
		for (const auto &item : state.graph.items)
			if (!published->count(item.object_uid))
				return false;
		return true;
	}
	// All nodes were allocated before batch publication. The real context owner
	// keeps this tracker stable across its publication steps; unrelated progress
	// must not be silently overwritten by an old preparation-time union.
	if (state.next_published.size() < state.graph.items.size() ||
	    published->size() != state.next_published.size() - state.graph.items.size())
		return false;
	for (const auto &item : state.graph.items)
		if (published->count(item.object_uid))
			return false;
	for (const auto uid : *published)
		if (!state.next_published.count(uid))
			return false;
	for (const auto &item : state.graph.items)
		if (!state.next_published.count(item.object_uid))
			return false;
	published->swap(state.next_published);
	state.bookkeeping_published = true;
	if (state.warm)
	{
		// Actual complete placement/cache proof above still protects these
		// live objects. End their creation-candidate marker before final SQL/
		// ACK/terminal proof; later metadata cleanup never touches old pointers.
		for (P_obj object : state.objects)
			REMOVE_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
	}
	return true;
}
bool zone_reset_room_publication_owner::release_completed(
	zone_reset_room_publication_stage &stage) noexcept
{
	if (!stage.state_ || !stage.state_->placed || !stage.state_->bookkeeping_published ||
	    !verify_current(stage))
		return false;
	auto &state = *stage.state_;
	for (size_t i = 0; i < state.stage_pointers.size(); ++i)
	{
		quest_mobile_native_item_progress progress{};
		if (!state.stage_pointers[i] || !state.stage_pointers[i]->can_release_published() ||
		    !state.stage_pointers[i]->read_progress(&progress) || !progress.admitted ||
		    !progress.published || progress.current_step_started ||
		    progress.next_step != state.stage_pointers[i]->publication_step_count())
			return false;
	}
	for (auto &item : state.stage_pointers)
		if (!item->release_published())
			return false;
	for (P_obj object : state.objects)
		REMOVE_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
	delete stage.state_;
	stage.state_ = nullptr;
	return true;
}

bool zone_reset_room_publication_owner::prepare_warm(
	const critical_command &command, std::span<quest_mobile_native_item_stage *> factories,
	zone_reset_original_room_placement_stage *placement,
	zone_reset_room_publication_stage *output) noexcept
{
	if (!output || output->state_ || !nevent_is_game_thread() ||
	    !persistence_mode_requires_mysql() || factories.empty())
		return false;
	try
	{
		auto state = std::make_unique<zone_reset_room_publication_stage::implementation>();
		if (zone_reset_item_command_decode(command, &state->original) !=
			    economic_accounting_error::ok ||
		    state->original.items.size() != factories.size())
			return false;
		state->room = real_room(state->original.room_vnum);
		if (!world || state->room < 0 || state->room > top_of_world)
			return false;
		state->stage_pointers.reserve(factories.size());
		state->objects.reserve(factories.size());
		state->original_indices.reserve(factories.size());
		state->custody.resize(factories.size());
		for (size_t at = 0; at < factories.size(); ++at)
		{
			auto *factory = factories[at];
			quest_mobile_native_item_progress progress{};
			if (!factory || !factory->object() ||
			    !factory->owns_pending_original_target(factory->object()) ||
			    factory->object()->obj_uid != state->original.items[at].object_uid ||
			    !factory->read_progress(&progress) || progress.admitted ||
			    progress.published || progress.next_step ||
			    progress.current_step_started)
				return false;
			state->stage_pointers.push_back(factory);
			state->objects.push_back(factory->object());
			state->original_indices.push_back(at);
		}
		std::vector<player_item_snapshot> observed;
		std::vector<uint8_t> a, b;
		if (player_item_snapshot_tree_capture_literal(state->objects[0], &observed,
							      nullptr) !=
			    player_snapshot_capture_result::ok ||
		    player_item_snapshot_list_encode(observed, &a) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(state->original.items, &b) !=
			    player_snapshot_codec_result::ok ||
		    a != b)
			return false;
		zone_reset_room_placement_recipe recipe;
		if (!placement || !placement->matches_source(state->original.reset_source) ||
		    !placement->recipe(&recipe) || !state->original.placement ||
		    recipe != *state->original.placement || recipe.fall_selected)
			return false;
		state->original_placement = placement;
		state->warm = true;
		output->state_ = state.release();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_room_publication_owner::prepare_original_completed_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	const critical_completion &receipt, zone_reset_room_publication_stage &held,
	bool (*charge)() noexcept) noexcept
{
	// Preserve the original completed-only terminal BODY contract exactly.
	return zone_reset_item_recovery_publication(envelope, receipt) &&
	       prepare_original_present_locked(connection, envelope, receipt, held, nullptr,
					       charge);
}

bool zone_reset_room_publication_owner::prepare_original_present_prefix_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	const critical_completion &receipt, zone_reset_room_publication_stage &held,
	zone_reset_original_room_placement_stage *placement, bool (*charge)() noexcept) noexcept
{
	return zone_reset_item_recovery_valid(envelope) &&
	       prepare_original_present_locked(connection, envelope, receipt, held, placement,
					       charge);
}

bool zone_reset_room_publication_owner::prepare_original_present_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	const critical_completion &receipt, zone_reset_room_publication_stage &held,
	zone_reset_original_room_placement_stage *placement, bool (*charge)() noexcept) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)envelope;
	(void)receipt;
	(void)held;
	(void)placement;
	(void)charge;
	return false;
#else
	if ((held.state_ && (held.state_->cold_reconstruction || held.state_->cold_pending)) ||
	    !charge || !nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    !zone_reset_item_recovery_valid(envelope))
		return false;
	try
	{
		const unsigned long session = connection ? mysql_thread_id(connection) : 0;
		if (held.state_ && held.state_->cold_present_ready)
		{
			// This same process already adopted every real original native
			// lifetime/enrollment under the complete cold proof. Only the
			// genuine journal driver owns its later intent/not-attempted and
			// returned dispositions. Fresh recovered unknown intent never earns
			// this private latch, and cannot repeat callbacks by this branch.
			return cold_session(connection, session) && held.state_->warm &&
			       held.state_->cold_adoption && !held.state_->cold_reconstruction &&
			       !held.state_->cold_pending &&
			       critical_command_equal(held.state_->cold_command,
						      envelope.command) &&
			       refresh_warm_locked(connection, envelope.command, receipt, held) &&
			       charge();
		}
		zone_reset_item_image original;
		zone_reset_item_recovery_context context;
		if (!cold_session(connection, session) ||
		    zone_reset_item_command_decode(envelope.command, &original) !=
			    economic_accounting_error::ok ||
		    zone_reset_item_recovery_decode(envelope.command, envelope.attachment,
						    &context) != economic_accounting_error::ok ||
		    original.items.empty() || !context.receipt_present ||
		    context.items.size() != original.items.size())
			return false;
		const auto completed = [](const zone_reset_item_recovery_action &action)
		{ return action.started && action.returned && action.succeeded; };
		if (!completed(context.whole_binding) || !completed(context.batch_publication) ||
		    (context.room_placement.started && !completed(context.room_placement)) ||
		    (!completed(context.room_placement) &&
		     (!placement || !original.placement || original.placement->fall_selected)))
			return false;
		for (size_t at = 0; at < context.items.size(); ++at)
		{
			const auto &item = context.items[at];
			if (item.object_uid != original.items[at].object_uid || !item.admitted ||
			    !item.published || item.current_step_started ||
			    item.effects.size() != original.recipes[at].libraries.size() * 2 + 3)
				return false;
			for (size_t step = 0; step < item.effects.size(); ++step)
			{
				const auto &effect = item.effects[step];
				if (step < item.next_step ? (!effect.started || !effect.returned ||
							     !effect.succeeded) :
							    (effect.started || effect.returned ||
							     effect.succeeded || effect.periodic))
					return false;
			}
		}
		if (held.state_ && (!held.state_->warm || !held.state_->cold_adoption))
			return false;
		if (!held.state_)
		{
			// Attach the actual metadata root before its first fallible growth.
			// The existing warm/root census owns every subsequent allocation.
			held.state_ = new zone_reset_room_publication_stage::implementation;
			held.state_->warm = true;
			held.state_->cold_adoption = true;
			if (!charge())
				return false;
		}
		auto &state = *held.state_;
		if (!state.cold_shape)
		{
			state.cold_command = envelope.command;
			if (!charge())
				return false;
			state.original = original;
			if (!charge())
				return false;
			state.room = real_room(original.room_vnum);
			if (!world || state.room < 0 || state.room > top_of_world)
				return false;
			state.objects.resize(original.items.size());
			if (!charge())
				return false;
			state.stage_pointers.resize(original.items.size());
			if (!charge())
				return false;
			state.stages.resize(original.items.size());
			if (!charge())
				return false;
			state.original_indices.resize(original.items.size());
			if (!charge())
				return false;
			for (size_t at = 0; at < original.items.size(); ++at)
				state.original_indices[at] = at;
			state.cold_shape = true;
		}
		if (!critical_command_equal(state.cold_command, envelope.command))
			return false;
		sql_room_item_graph current;
		if (!sql_room_item_payload_read(connection, original.items[0].object_uid,
						&current) ||
		    current.items.size() != original.items.size() ||
		    current.identities.size() != original.items.size() ||
		    !cold_session(connection, session))
			return false;
		sql_room_item_graph ordered;
		ordered.owner = current.owner;
		ordered.owner_revision = current.owner_revision;
		ordered.creation_origin = std::move(current.creation_origin);
		std::vector<item_ownership_runtime_entry> custody;
		for (size_t at = 0; at < original.items.size(); ++at)
		{
			size_t match = SIZE_MAX;
			for (size_t row = 0; row < current.items.size(); ++row)
				if (current.items[row].object_uid == original.items[at].object_uid)
				{
					if (match != SIZE_MAX)
						return false;
					match = row;
				}
			if (match == SIZE_MAX ||
			    !same_item(current.items[match], original.items[at]))
				return false;
			const auto &identity = current.identities[match];
			ordered.items.push_back(original.items[at]);
			ordered.identities.push_back(identity);
			custody.push_back({ identity.item_uid, identity.root_item_uid,
					    identity.parent_item_uid, identity.owner,
					    identity.item_revision, identity.owner_revision,
					    original.items[at].vnum, identity.state });
		}
		// Authenticate every selected native lifetime before dereferencing one.
		// A retained partial metadata adoption cannot silently change pointers.
		for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		std::vector<bool> seen(original.items.size(), false);
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (object->prev != previous)
				return false;
			previous = object;
			for (size_t at = 0; at < original.items.size(); ++at)
				if (object->obj_uid == original.items[at].object_uid)
				{
					if (seen[at] ||
					    (state.objects[at] && state.objects[at] != object))
						return false;
					seen[at] = true;
					state.objects[at] = object;
				}
		}
		if (!std::all_of(seen.begin(), seen.end(), [](bool found) { return found; }))
			return false; // No missing-body reconstruction or partial forest adoption.
		state.graph = std::move(ordered);
		state.custody = std::move(custody);
		state.consumed = true;
		state.placed = completed(context.room_placement);
		if (!charge() || !verify_current(held) ||
		    !refresh_warm_locked(connection, envelope.command, receipt, held) || !charge())
			return false;
		state.admitted = true;
		// These are authenticated original returned facts, not a replayed placement.
		state.placement_started = context.room_placement.started;
		state.placement_returned = context.room_placement.returned;
		for (size_t at = 0; at < original.items.size(); ++at)
		{
			if (!state.stages[at])
			{
				state.stages[at] =
					std::make_unique<quest_mobile_native_item_stage>();
				if (!charge())
					return false;
			}
			if (state.stages[at]->empty())
			{
				const auto &saved = context.items[at];
				quest_mobile_native_item_progress progress{ saved.next_step, false,
									    saved.admitted,
									    saved.published };
				std::vector<quest_mobile_native_item_effect> effects;
				for (const auto &effect : saved.effects)
					effects.push_back({ effect.started, effect.returned,
							    effect.succeeded, effect.periodic });
				const bool adopted =
					quest_mobile_native_item_stage::adopt_published(
						original.items[at], original.recipes[at],
						state.objects[at], progress, effects,
						state.stages[at].get());
				if (!charge() || !adopted)
					return false;
			}
			state.stage_pointers[at] = state.stages[at].get();
			quest_mobile_native_item_progress progress{};
			if (!state.stage_pointers[at]->read_progress(&progress) ||
			    !progress.admitted || !progress.published ||
			    progress.current_step_started ||
			    progress.next_step != context.items[at].next_step)
				return false;
		}
		if (!state.placed)
		{
			zone_reset_room_placement_recipe recorded;
			if (!placement->recipe(&recorded))
			{
				if (!zone_reset_original_room_placement_stage::restore(
					    *state.original.placement, state.objects[0],
					    state.original.reset_source, placement))
					return false;
			}
			else if (recorded != *state.original.placement)
				return false;
			if (!placement->matches_source(state.original.reset_source))
				return false;
			state.original_placement = placement;
		}
		if (!state.bookkeeping_published)
			for (const auto &item : state.graph.items)
			{
				state.next_published.insert(item.object_uid);
				if (!charge())
					return false;
			}
		if (!cold_session(connection, session) ||
		    !refresh_warm_locked(connection, envelope.command, receipt, held) ||
		    !verify_current(held))
			return false;
		state.cold_present_ready = true; // Earned only by the complete real adoption/proof.
		return charge() && cold_session(connection, session);
	}
	catch (...)
	{
		(void)charge();
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::restore_original_missing_forest(
	zone_reset_room_publication_stage &held, bool (*charge)() noexcept) noexcept
{
	if (!held.state_ || !charge || !nevent_is_game_thread() || !held.state_->warm ||
	    !held.state_->cold_reconstruction || !held.state_->cold_shape)
		return false;
	auto &state = *held.state_;
	if (state.cold_forest_ready)
		return true;
	try
	{
		for (size_t at = 0; at < state.original.items.size(); ++at)
		{
			if (!state.stages[at])
			{
				state.stages[at] =
					std::make_unique<quest_mobile_native_item_stage>();
				if (!charge())
					return false;
			}
			auto &factory = *state.stages[at];
			if (factory.empty())
			{
				auto literal = state.original.items[at];
				literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				literal.equipment_slot = 0;
				const auto &recipe = state.original.recipes[at];
				const bool restored =
					quest_mobile_native_item_stage::restore(literal, recipe,
										&factory) ||
					(factory.empty() &&
					 quest_mobile_native_item_stage::restore_bound(
						 literal, recipe, &factory)) ||
					(factory.empty() &&
					 quest_mobile_native_item_stage::restore_rebind(
						 literal, recipe, &factory));
				if (!charge() || !restored)
					return false;
			}
			P_obj object = factory.object();
			quest_mobile_native_item_progress progress{};
			if (!object || object->obj_uid != state.original.items[at].object_uid ||
			    object->next || object->prev || object->contains ||
			    object->next_content || object->loc_p != LOC_NOWHERE ||
			    object->loc.room != NOWHERE || !factory.read_progress(&progress) ||
			    progress.admitted || progress.published || progress.next_step ||
			    progress.current_step_started)
				return false;
			state.objects[at] = object;
			state.stage_pointers[at] = &factory;
		}
		// Preserve the frozen literal child order without replaying obj_to_obj
		// callbacks, weight mutation, original load decisions or constructors.
		std::vector<P_obj> tails(state.objects.size(), nullptr);
		for (size_t at = 1; at < state.objects.size(); ++at)
		{
			const auto parent =
				static_cast<size_t>(state.original.items[at].parent_index);
			P_obj object = state.objects[at], container = state.objects[parent];
			object->loc_p = LOC_INSIDE;
			object->loc.inside = container;
			if (tails[parent])
				tails[parent]->next_content = object;
			else
				container->contains = object;
			tails[parent] = object;
		}
		state.cold_forest_ready = true;
		return true;
	}
	catch (...)
	{
		(void)charge();
		return false;
	}
}

bool zone_reset_room_publication_owner::prepare_original_pending_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	const critical_completion &receipt, zone_reset_room_publication_stage &held,
	zone_reset_original_room_placement_stage *placement,
	std::unordered_set<uint64_t> &published,
	bool (*bindings)(std::span<quest_mobile_native_item_stage *>, void *) noexcept,
	void *binding_context, bool (*charge)() noexcept) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)envelope;
	(void)receipt;
	(void)held;
	(void)placement;
	(void)published;
	(void)bindings;
	(void)binding_context;
	(void)charge;
	return false;
#else
	if (!placement || !bindings || !charge || !nevent_is_game_thread() ||
	    !persistence_mode_requires_mysql() || !zone_reset_item_recovery_valid(envelope))
		return false;
	try
	{
		const unsigned long session = connection ? mysql_thread_id(connection) : 0;
		if (!cold_session(connection, session))
			return false;
		if (held.state_ && held.state_->cold_pending_ready)
		{
			// This original process already prepared the actual forest/witness.
			// The real journal driver owns subsequent intent/not-attempted and
			// returned latches; never reinterpret its in-process intent as a new
			// cold entitlement or restart factories/bindings/placement.
			return held.state_->cold_pending &&
			       critical_command_equal(held.state_->cold_command,
						      envelope.command) &&
			       refresh_warm_locked(connection, envelope.command, receipt, held) &&
			       charge();
		}
		zone_reset_item_image original;
		zone_reset_item_recovery_context context;
		if (zone_reset_item_command_decode(envelope.command, &original) !=
			    economic_accounting_error::ok ||
		    zone_reset_item_recovery_decode(envelope.command, envelope.attachment,
						    &context) != economic_accounting_error::ok ||
		    !context.receipt_present ||
		    (!context.room_placement.succeeded &&
		     (!original.placement || original.placement->fall_selected)) ||
		    (context.whole_binding.started &&
		     (!context.whole_binding.returned || !context.whole_binding.succeeded)) ||
		    (context.batch_publication.started && (!context.batch_publication.returned ||
							   !context.batch_publication.succeeded)) ||
		    (context.room_placement.started &&
		     (!context.room_placement.returned || !context.room_placement.succeeded)))
			return false;
		for (const auto &item : context.items)
		{
			if (item.current_step_started)
				return false;
			for (size_t step = 0; step < item.effects.size(); ++step)
			{
				const auto &effect = item.effects[step];
				if (step < item.next_step ? (!effect.started || !effect.returned ||
							     !effect.succeeded) :
							    (effect.started || effect.returned ||
							     effect.succeeded || effect.periodic))
					return false;
			}
		}
		if (!held.state_)
		{
			held.state_ = new zone_reset_room_publication_stage::implementation;
			held.state_->warm = true;
			held.state_->cold_adoption = true;
			held.state_->cold_pending = true;
			held.state_->cold_reconstruction = true;
			if (!charge())
				return false;
		}
		auto &state = *held.state_;
		if (!state.cold_pending)
		{
			if (!state.warm || !state.cold_adoption || !state.cold_shape ||
			    state.admitted || state.consumed || state.placed ||
			    state.cold_reconstruction ||
			    !critical_command_equal(state.cold_command, envelope.command))
				return false;
			for (const auto &factory : state.stages)
				if (factory && !factory->empty())
					return false;
			for (const P_obj object : state.objects)
				if (object)
					return false; // An actually observed lifetime cannot be replaced.
			std::map<uint64_t, size_t> selected;
			for (size_t at = 0; at < state.original.items.size(); ++at)
				if (!selected.emplace(state.original.items[at].object_uid, at)
					     .second)
					return false;
			if (!absent_native(selected))
				return false;
			state.cold_pending = true;
			state.cold_reconstruction = true;
		}
		if (!state.cold_shape)
		{
			state.cold_command = envelope.command;
			if (!charge())
				return false;
			state.original = original;
			if (!charge())
				return false;
			state.room = real_room(original.room_vnum);
			if (!world || state.room < 0 || state.room > top_of_world)
				return false;
			state.objects.resize(original.items.size());
			if (!charge())
				return false;
			state.stage_pointers.resize(original.items.size());
			if (!charge())
				return false;
			state.stages.resize(original.items.size());
			if (!charge())
				return false;
			state.original_indices.resize(original.items.size());
			if (!charge())
				return false;
			for (size_t at = 0; at < original.items.size(); ++at)
				state.original_indices[at] = at;
			state.cold_shape = true;
		}
		if (!critical_command_equal(state.cold_command, envelope.command) ||
		    !refresh_warm_locked(connection, envelope.command, receipt, held) ||
		    !charge() || !restore_original_missing_forest(held, charge) ||
		    !refresh_warm_locked(connection, envelope.command, receipt, held) ||
		    !charge() || !bindings(state.stage_pointers, binding_context) ||
		    !cold_session(connection, session))
			return false;
		if (context.batch_publication.succeeded)
		{
			// Rebuild actual current publication from the authenticated recorded
			// successful batch; do not rerun its original journal action.
			if (!state.consumed)
			{
				if (!retain_admitted(held))
					return false;
				const bool reserved = reserve_warm_consume(held, published);
				if (!charge() || !reserved || !cold_session(connection, session))
					return false;
				const bool consumed = consume(held, &published);
				if (!charge() || !consumed)
					return false;
			}
			if (!cold_session(connection, session) || !verify_current(held))
				return false;
			while (state.cold_enrollment_next < context.items.size())
			{
				const size_t at = state.cold_enrollment_next;
				const auto &saved = context.items[at];
				quest_mobile_native_item_progress progress{
					saved.next_step, saved.current_step_started, saved.admitted,
					saved.published
				};
				std::vector<quest_mobile_native_item_effect> effects;
				for (const auto &effect : saved.effects)
					effects.push_back({ effect.started, effect.returned,
							    effect.succeeded, effect.periodic });
				if (!cold_session(connection, session))
					return false;
				const bool rebuilt =
					rebuild_enrollment(held, at, progress, effects);
				if (rebuilt)
					++state.cold_enrollment_next;
				if (!charge() || !rebuilt)
					return false;
			}
		}
		if (context.room_placement.succeeded)
		{
			// Original placement already returned. Restore only its current
			// persisted room topology/light through the existing cold owner.
			if (!cold_session(connection, session) || !place(held) ||
			    !verify_current(held))
				return false;
			state.placement_started = context.room_placement.started;
			state.placement_returned = context.room_placement.returned;
		}
		else
		{
			zone_reset_room_placement_recipe recorded;
			if (!placement->recipe(&recorded))
			{
				if (!zone_reset_original_room_placement_stage::restore(
					    *state.original.placement, state.objects[0],
					    state.original.reset_source, placement))
					return false;
			}
			else if (recorded != *state.original.placement)
				return false;
			if (!placement->matches_source(state.original.reset_source))
				return false;
			state.original_placement = placement;
		}
		state.cold_pending_ready = true;
		return refresh_warm_locked(connection, envelope.command, receipt, held) &&
		       charge() && cold_session(connection, session);
	}
	catch (...)
	{
		(void)charge();
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::prepare_original_reconstructed_locked(
	MYSQL *connection, const critical_native_recovery_envelope &envelope,
	const critical_completion &receipt, zone_reset_room_publication_stage &held,
	std::unordered_set<uint64_t> &published,
	bool (*bindings)(std::span<quest_mobile_native_item_stage *>, void *) noexcept,
	void *binding_context, bool (*charge)() noexcept) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)envelope;
	(void)receipt;
	(void)held;
	(void)published;
	(void)bindings;
	(void)binding_context;
	(void)charge;
	return false;
#else
	if (!held.state_ || held.state_->cold_pending || !held.state_->warm ||
	    !held.state_->cold_adoption || !held.state_->cold_shape || !bindings || !charge ||
	    !nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    !zone_reset_item_recovery_publication(envelope, receipt))
		return false;
	try
	{
		auto &state = *held.state_;
		const unsigned long session = connection ? mysql_thread_id(connection) : 0;
		zone_reset_item_recovery_context context;
		if (!cold_session(connection, session) ||
		    !critical_command_equal(state.cold_command, envelope.command) ||
		    zone_reset_item_recovery_decode(envelope.command, envelope.attachment,
						    &context) != economic_accounting_error::ok ||
		    context.items.size() != state.original.items.size())
			return false;
		if (!state.cold_reconstruction)
		{
			// Adoption refusal never licenses replacing an existing native
			// lifetime or partially adopted metadata with a new object body.
			for (const auto &factory : state.stages)
				if (factory && !factory->empty())
					return false;
			for (const P_obj object : state.objects)
				if (object)
					return false; // An actually observed prior lifetime stays retained.
			std::map<uint64_t, size_t> selected;
			for (size_t at = 0; at < state.original.items.size(); ++at)
				if (!selected.emplace(state.original.items[at].object_uid, at)
					     .second)
					return false;
			if (!absent_native(selected))
				return false;
			state.cold_reconstruction = true;
		}
		// Original source/receipt, current full SQL graph, and complete selected
		// native/cache absence are authenticated before any frozen-body restore.
		if (!refresh_warm_locked(connection, envelope.command, receipt, held) || !charge())
			return false;
		if (!restore_original_missing_forest(held, charge))
			return false;
		if (!state.consumed)
		{
			if (!refresh_warm_locked(connection, envelope.command, receipt, held) ||
			    !charge() || !bindings(state.stage_pointers, binding_context) ||
			    !cold_session(connection, session) || !retain_admitted(held))
				return false;
			const bool reserved = reserve_warm_consume(held, published);
			if (!charge() || !reserved)
				return false;
			const bool consumed = consume(held, &published);
			if (!charge() || !consumed)
				return false;
		}
		if (!cold_session(connection, session) || !verify_current(held))
			return false;
		for (size_t at = 0; at < context.items.size(); ++at)
		{
			const auto &saved = context.items[at];
			quest_mobile_native_item_progress progress{ saved.next_step,
								    saved.current_step_started,
								    saved.admitted,
								    saved.published };
			std::vector<quest_mobile_native_item_effect> effects;
			for (const auto &effect : saved.effects)
				effects.push_back({ effect.started, effect.returned,
						    effect.succeeded, effect.periodic });
			if (!cold_session(connection, session))
				return false;
			const bool rebuilt = rebuild_enrollment(held, at, progress, effects);
			if (!charge() || !rebuilt)
				return false;
		}
		if (!cold_session(connection, session) || !place(held) || !verify_current(held))
			return false;
		// The original recorded native placement already returned. This actual
		// cold topology restore is separate and does not repeat original gameplay.
		state.placement_started = context.room_placement.started;
		state.placement_returned = context.room_placement.returned;
		return cold_session(connection, session) &&
		       refresh_warm_locked(connection, envelope.command, receipt, held) && charge();
	}
	catch (...)
	{
		(void)charge();
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::pending_items(
	const zone_reset_room_publication_stage &stage, int rnum, size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread() || rnum < 0)
		return false;
	size_t count = 0;
	if (stage.state_ && stage.state_->cold_reconstruction)
		for (const auto &factory : stage.state_->stages)
			if (factory && !factory->empty())
			{
				quest_mobile_native_item_progress progress{};
				if (!factory->read_progress(&progress))
					return false;
				if (progress.published)
					continue; // Actual native index already counts this published body.
				const P_obj object = factory->object();
				if (!object || object->R_num < 0)
					return false;
				if (object->R_num == rnum)
				{
					if (count == SIZE_MAX)
						return false;
					++count;
				}
			}
	*output = count;
	return true;
}

bool zone_reset_room_publication_owner::refresh_warm_locked(
	MYSQL *connection, const critical_command &command, const critical_completion &receipt,
	zone_reset_room_publication_stage &stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)receipt;
	(void)stage;
	return false;
#else
	if (!stage.state_ || !stage.state_->warm || !nevent_is_game_thread() ||
	    receipt.operation_id.bytes != command.operation_id.bytes ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.error_code || receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
		return false;
	try
	{
		auto &state = *stage.state_;
		const unsigned long session = connection ? mysql_thread_id(connection) : 0;
		if (!cold_session(connection, session) || state.original.items.empty() ||
		    economic_sql_zone_reset_item_verify_retained(connection, command, 0,
								 { receipt.result_payload.data(),
								   receipt.result_size }))
			return false;
		sql_room_item_graph current;
		if (!sql_room_item_payload_read(connection, state.original.items[0].object_uid,
						&current) ||
		    !current.creation_origin || !current.creation_origin->present ||
		    !critical_command_equal(current.creation_origin->original, command) ||
		    current.creation_origin->result.size() != receipt.result_size ||
		    !std::equal(current.creation_origin->result.begin(),
				current.creation_origin->result.end(),
				receipt.result_payload.begin()) ||
		    current.owner.type != item_owner_type::room || current.owner.context_id ||
		    current.owner.id != static_cast<uint64_t>(state.original.room_vnum) ||
		    current.owner_revision != receipt.durable_revision ||
		    receipt.durable_revision != state.original.expected_room_revision + 1 ||
		    current.items.size() != state.original.items.size() ||
		    current.identities.size() != current.items.size() ||
		    !cold_session(connection, session))
			return false;
		// Normalize current native evidence into the full original factory order.
		// UID/parent correlation, never an inferred prototype/order identity.
		sql_room_item_graph ordered;
		ordered.owner = current.owner;
		ordered.owner_revision = current.owner_revision;
		ordered.creation_origin = std::move(current.creation_origin);
		ordered.items.reserve(state.original.items.size());
		ordered.identities.reserve(state.original.items.size());
		std::vector<item_ownership_runtime_entry> custody;
		custody.reserve(state.original.items.size());
		std::map<uint64_t, size_t> selected;
		for (size_t at = 0; at < state.original.items.size(); ++at)
		{
			const auto &original = state.original.items[at];
			if (!selected.emplace(original.object_uid, at).second)
				return false;
			size_t match = SIZE_MAX;
			for (size_t row = 0; row < current.items.size(); ++row)
				if (current.items[row].object_uid == original.object_uid)
				{
					if (match != SIZE_MAX)
						return false;
					match = row;
				}
			if (match == SIZE_MAX || !same_item(current.items[match], original))
				return false;
			const auto &identity = current.identities[match];
			const uint64_t parent =
				original.parent_index < 0 ?
					0 :
					state.original.items[original.parent_index].object_uid;
			if (identity.item_uid != original.object_uid ||
			    identity.root_item_uid != state.original.items[0].object_uid ||
			    identity.parent_item_uid != parent || identity.item_revision != 1 ||
			    identity.owner_revision != ordered.owner_revision ||
			    !item_owner_identity_equal(identity.owner, ordered.owner) ||
			    identity.state != item_custody_state::active)
				return false;
			ordered.items.push_back(original);
			ordered.identities.push_back(identity);
			custody.push_back({ identity.item_uid, identity.root_item_uid,
					    identity.parent_item_uid, identity.owner,
					    identity.item_revision, identity.owner_revision,
					    original.vnum, identity.state });
		}
		if (!state.consumed)
		{
			// Before cold cloning, authenticate the same complete original SQL
			// cut and full selected native/cache absence. Full detached-factory
			// proof becomes mandatory once the genuine restored forest exists.
			if (!state.cold_reconstruction || state.cold_forest_ready)
			{
				std::vector<player_item_snapshot> native;
				std::vector<uint8_t> a, b;
				if (state.objects.size() != state.original.items.size() ||
				    state.stage_pointers.size() != state.objects.size() ||
				    player_item_snapshot_tree_capture_literal(state.objects[0],
									      &native, nullptr) !=
					    player_snapshot_capture_result::ok ||
				    player_item_snapshot_list_encode(native, &a) !=
					    player_snapshot_codec_result::ok ||
				    player_item_snapshot_list_encode(state.original.items, &b) !=
					    player_snapshot_codec_result::ok ||
				    a != b)
					return false;
				for (size_t at = 0; at < state.stage_pointers.size(); ++at)
				{
					quest_mobile_native_item_progress progress{};
					if (!state.stage_pointers[at] ||
					    state.stage_pointers[at]->object() !=
						    state.objects[at] ||
					    (!state.cold_reconstruction &&
					     !state.stage_pointers[at]->owns_pending_original_target(
						     state.objects[at])) ||
					    !state.stage_pointers[at]->read_progress(&progress) ||
					    progress.admitted != state.admitted ||
					    progress.published || progress.next_step ||
					    progress.current_step_started)
						return false;
				}
			}
			if (!absent_native(selected))
				return false;
			std::vector<uint64_t> uids;
			uids.reserve(selected.size());
			for (const auto &[uid, at] : selected)
				uids.push_back(uid);
			std::vector<item_ownership_runtime_entry> runtime;
			if (!item_ownership_runtime_published_native_observer::snapshot_links(
				    uids, ITEM_TRANSFER_MAX_ITEMS, &runtime) ||
			    !runtime.empty())
				return false;
		}
		else
		{
			if (state.graph.owner_revision != ordered.owner_revision ||
			    state.custody.size() != custody.size())
				return false;
			for (size_t at = 0; at < custody.size(); ++at)
				if (!same_custody(state.custody[at], custody[at]))
					return false;
			if (!verify_current(stage))
				return false;
		}
		if (!cold_session(connection, session))
			return false;
		state.graph = std::move(ordered);
		state.custody = std::move(custody);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::place_warm(zone_reset_room_publication_stage &stage,
						   quest_mobile_native_item_effect &effect) noexcept
{
	if (!stage.state_ || !stage.state_->warm || !stage.state_->original_placement ||
	    !verify_current(stage))
		return false;
	auto &state = *stage.state_;
	if (state.placed || state.placement_started || state.placement_returned)
		return false;
	for (auto *factory : state.stage_pointers)
	{
		quest_mobile_native_item_progress progress{};
		if (!factory || !factory->read_progress(&progress) || !progress.admitted ||
		    !progress.published || progress.current_step_started ||
		    progress.next_step != factory->publication_step_count())
			return false;
	}
	const bool placed = state.original_placement->place(effect);
	// Real returned markers are retained before any fallible recensus. The
	// handler consumes its original witness before starting the native tail.
	state.placement_started = effect.started;
	state.placement_returned = effect.returned;
	if (placed && effect.started && effect.returned && effect.succeeded)
		state.placed = true;
	return placed;
}

bool zone_reset_room_publication_owner::reserve_warm_consume(
	zone_reset_room_publication_stage &stage,
	const std::unordered_set<uint64_t> &published) noexcept
{
	if (!stage.state_ || !stage.state_->warm || stage.state_->consumed ||
	    !nevent_is_game_thread() || stage.state_->graph.items.empty())
		return false;
	try
	{
		auto &state = *stage.state_;
		// An earlier allocation refusal can retain a partial original union.
		// Extend only that proven subset before intent; never overwrite it or
		// interpret its incompleteness as a successful native publication.
		for (const auto uid : state.next_published)
			if (!published.count(uid) &&
			    std::none_of(state.graph.items.begin(), state.graph.items.end(),
					 [uid](const auto &item)
					 { return item.object_uid == uid; }))
				return false;
		for (const auto uid : published)
			state.next_published.insert(uid);
		for (const auto &item : state.graph.items)
		{
			if (published.count(item.object_uid))
				return false;
			state.next_published.insert(item.object_uid);
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_room_publication_owner::retained_size(
	const zone_reset_room_publication_stage &stage, size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
	if (!stage.state_)
	{
		*output = 0;
		return true;
	}
	const auto &state = *stage.state_;
	// Warm factories remain counted exactly once by their original warm root.
	// This measures the real publication metadata, not a second factory owner.
	if (!state.warm || (!state.stages.empty() && !state.cold_adoption))
		return false;
	size_t bytes = sizeof(state);
	const auto add = [&](size_t amount)
	{
		if (amount > CRITICAL_COORDINATOR_MAX_BYTES - bytes)
			return false;
		bytes += amount;
		return true;
	};
	const auto array = [&](size_t count, size_t unit)
	{ return (!unit || count <= CRITICAL_COORDINATOR_MAX_BYTES / unit) && add(count * unit); };
	const auto text = [&](const std::string &value)
	{ return value.capacity() != SIZE_MAX && add(value.capacity() + 1); };
	const auto item = [&](const player_item_snapshot &value)
	{
		if (!text(value.name) || !text(value.short_description) ||
		    !text(value.description) || !text(value.action_description) ||
		    !array(value.dynamic_affects.capacity(),
			   sizeof(player_item_dynamic_affect_snapshot)) ||
		    !array(value.extra_descriptions.capacity(),
			   sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : value.extra_descriptions)
			if (!text(description.keyword) || !text(description.description) ||
			    !array(description.spell_ids.capacity(), sizeof(int32_t)))
				return false;
		return true;
	};
	const auto command = [&](const critical_command &value)
	{
		return array(value.payload.capacity(), 1) &&
		       array(value.accounting_intent.capacity(), 1) &&
		       array(value.keys.capacity(), sizeof(critical_entity_key)) &&
		       array(value.expected_revisions.capacity(),
			     sizeof(critical_expected_revision));
	};
	if ((state.selected_root.capacity() > 15 && !text(state.selected_root)) ||
	    !array(state.factory_sources.capacity(), sizeof(economic_source_event)) ||
	    !array(state.objects.capacity(), sizeof(P_obj)) ||
	    !array(state.stage_pointers.capacity(), sizeof(quest_mobile_native_item_stage *)) ||
	    !array(state.original_indices.capacity(), sizeof(size_t)) ||
	    !array(state.custody.capacity(), sizeof(item_ownership_runtime_entry)) ||
	    !array(state.stages.capacity(),
		   sizeof(std::unique_ptr<quest_mobile_native_item_stage>)) ||
	    !array(state.graph.items.capacity(), sizeof(player_item_snapshot)) ||
	    !array(state.graph.identities.capacity(), sizeof(player_load_item_identity)) ||
	    !array(state.original.items.capacity(), sizeof(player_item_snapshot)) ||
	    !array(state.original.recipes.capacity(), sizeof(native_mobile_birth_item_recipe)) ||
	    !array(state.original.coins.capacity(), sizeof(zone_reset_coin_output)) ||
	    !array(state.cold_terminal.capacity(), 1) ||
	    !array(state.cold_progress.capacity(), sizeof(quest_mobile_native_item_progress)) ||
	    !array(state.cold_effects.capacity(),
		   sizeof(std::vector<quest_mobile_native_item_effect>)))
		return false;
	// Warm borrowed stages remain in their actual root census. Cold adoption
	// owns only these actual native metadata wrappers and their measured heaps.
	for (const auto &factory : state.stages)
		if (factory)
		{
			const size_t retained = factory->empty() ? sizeof(*factory) :
								   factory->retained_bytes();
			if (retained < sizeof(*factory) || !add(retained))
				return false;
		}
	for (const auto &effects : state.cold_effects)
		if (!array(effects.capacity(), sizeof(quest_mobile_native_item_effect)))
			return false;
	for (const auto &value : state.graph.items)
		if (!item(value))
			return false;
	for (const auto &value : state.original.items)
		if (!item(value))
			return false;
	for (const auto &value : state.original.recipes)
		if (!array(value.libraries.capacity(), sizeof(native_mobile_birth_library_recipe)))
			return false;
	if (state.cold_adoption && !command(state.cold_command))
		return false;
	if (state.graph.creation_origin && !command(state.graph.creation_origin->original))
		return false;
#ifdef __GLIBCXX__
	if ((state.next_published.bucket_count() > 1 &&
	     !array(state.next_published.bucket_count(), sizeof(void *))) ||
	    !array(state.next_published.size(), sizeof(std::__detail::_Hash_node<uint64_t, false>)))
		return false;
#else
	// The supported actual server allocator ABI is measured, never guessed.
	if (!state.next_published.empty() || state.next_published.bucket_count() > 1)
		return false;
#endif
	*output = bytes;
	return true;
}

critical_submit_result
zone_reset_room_publication_owner::submit_warm(critical_native_recovery_envelope envelope)
{
	return critical_zone_reset_item_publication_owner::submit(std::move(envelope));
}
bool zone_reset_room_publication_owner::copy_warm(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	return critical_zone_reset_item_publication_owner::copy_context(command, output);
}
bool zone_reset_room_publication_owner::checkpoint_warm(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	return critical_zone_reset_item_publication_owner::checkpoint_context(expected, successor);
}
bool zone_reset_room_publication_owner::generation_warm(
	const critical_native_recovery_envelope &expected, uint64_t *generation) noexcept
{
	return critical_zone_reset_item_publication_owner::observe_generation(expected, generation);
}
bool zone_reset_room_publication_owner::acknowledge_warm(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation) noexcept
{
	return critical_zone_reset_item_publication_owner::acknowledge(expected, receipt,
								       generation);
}
bool zone_reset_room_publication_owner::retire_warm(
	const critical_native_recovery_envelope &expected, uint64_t generation) noexcept
{
	return critical_zone_reset_item_publication_owner::retire(expected, generation,
								  retain_terminal, nullptr);
}
bool zone_reset_room_publication_owner::retain_terminal(
	const critical_native_recovery_envelope &expected, void *) noexcept
{
#ifdef __NO_MYSQL__
	(void)expected;
	return false;
#else
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool retained = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17) ||
			    !transaction.same_session() ||
			    zone_reset_item_terminal_sql_owner::retain_locked(connection,
									      expected) ||
			    !transaction.same_session())
				throw EAGAIN;
			transaction.committing();
			if (mysql_real_query(connection, "COMMIT", 6) || !transaction.committed())
				throw EIO;
			retained = true;
		}
		catch (...)
		{
			retained = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		return retained && transaction.same_session() &&
		       cleanup.disposition == player_sql_cleanup_disposition::idle_verified;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::cancel_warm(
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	uint64_t generation,
	bool (*cleanup)(const critical_command &, const critical_completion &, void *) noexcept,
	void *context) noexcept
{
	return critical_zone_reset_item_publication_owner::cancel_refusal(
		original, receipt, generation, cleanup, context);
}
bool zone_reset_room_publication_owner::discard_refused_warm(
	zone_reset_room_publication_stage &stage) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	if (!stage.state_)
		return true;
	const auto &state = *stage.state_;
	if (!state.warm || state.admitted || state.consumed || state.placement_started ||
	    !state.stages.empty())
		return false;
	for (auto *factory : state.stage_pointers)
	{
		quest_mobile_native_item_progress progress{};
		if (!factory || !factory->read_progress(&progress) || progress.admitted ||
		    progress.published || progress.current_step_started || progress.next_step)
			return false;
	}
	delete stage.state_;
	stage.state_ = nullptr;
	return true;
}

// This private path follows the original owner's successful guarded terminal
// retirement. The completed world may legitimately move or disappear later;
// never re-read an old room/custody cut or dereference a former live object.
// All native effects, current physical proof, ACK and durable terminal transfer
// precede that owner call. This releases only the authentic factory metadata.
bool zone_reset_room_publication_owner::release_original_terminal_metadata(
	zone_reset_room_publication_stage &stage) noexcept
{
	if (!stage.state_ || !nevent_is_game_thread())
		return false;
	auto &state = *stage.state_;
	if (!state.warm || !state.admitted || !state.consumed || !state.placed ||
	    !state.bookkeeping_published || !state.placement_started || !state.placement_returned)
		return false;
	for (auto *factory : state.stage_pointers)
	{
		quest_mobile_native_item_progress progress{};
		if (!factory || !factory->can_release_published() ||
		    !factory->read_progress(&progress) || !progress.admitted ||
		    !progress.published || progress.current_step_started ||
		    progress.next_step != factory->publication_step_count())
			return false;
	}
	// Original helpers delete their retained metadata without allocation,
	// callback, extraction, native effect or world-object access. This game
	// thread cannot change the proven factory states between these two loops.
	for (auto *factory : state.stage_pointers)
		if (!factory->release_published())
			return false;
	delete stage.state_;
	stage.state_ = nullptr;
	return true;
}

namespace
{
bool room_prepare_add(size_t &bytes, size_t amount) noexcept
{
	if (bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
	    amount > CRITICAL_COORDINATOR_MAX_BYTES - bytes)
		return false;
	bytes += amount;
	return true;
}
bool room_prepare_array(size_t &bytes, size_t count, size_t unit) noexcept
{
	return (!unit || count <= CRITICAL_COORDINATOR_MAX_BYTES / unit) &&
	       room_prepare_add(bytes, count * unit);
}
bool room_prepare_image_heap(const zone_reset_item_image &image, size_t *output) noexcept
{
	size_t bytes = 0;
	const auto text = [&](const std::string &value) noexcept
	{
		return value.capacity() <= 15 || (value.capacity() != SIZE_MAX &&
						  room_prepare_add(bytes, value.capacity() + 1));
	};
	if (!room_prepare_array(bytes, image.items.capacity(), sizeof(player_item_snapshot)) ||
	    !room_prepare_array(bytes, image.recipes.capacity(),
				sizeof(native_mobile_birth_item_recipe)) ||
	    !room_prepare_array(bytes, image.coins.capacity(), sizeof(zone_reset_coin_output)))
		return false;
	for (const auto &item : image.items)
	{
		if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description) ||
		    !room_prepare_array(bytes, item.dynamic_affects.capacity(),
					sizeof(player_item_dynamic_affect_snapshot)) ||
		    !room_prepare_array(bytes, item.extra_descriptions.capacity(),
					sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : item.extra_descriptions)
			if (!text(description.keyword) || !text(description.description) ||
			    !room_prepare_array(bytes, description.spell_ids.capacity(),
						sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : image.recipes)
		if (!room_prepare_array(bytes, recipe.libraries.capacity(),
					sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}
bool room_prepare_same_invocation(const economic_source_event &a,
				  const economic_source_event &b) noexcept
{
	return a.kind == b.kind && a.source.bytes == b.source.bytes &&
	       a.generation.bytes == b.generation.bytes && a.sequence == b.sequence;
}
}

bool zone_reset_room_publication_owner::prepare_warm_flat_bounded(
	const critical_command &command, const std::string &selected_root,
	const std::span<quest_mobile_native_item_stage *> &factories,
	const std::span<const economic_source_event> &sources,
	zone_reset_original_room_placement_stage *placement,
	zone_reset_room_publication_stage *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!output || output->state_ || !nevent_is_game_thread() ||
	    persistence_mode_requires_mysql() || selected_root.empty() || !reserve ||
	    factories.empty() || factories.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    sources.size() != factories.size() || !placement)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)context;
	(void)outer_live;
	return false;
#else
	try
	{
		// Admit the actual owning unique_ptr and heap implementation before creation.
		size_t live = outer_live;
		if (!room_prepare_add(
			    live, sizeof(std::unique_ptr<
					  zone_reset_room_publication_stage::implementation>)) ||
		    !room_prepare_add(live,
				      sizeof(zone_reset_room_publication_stage::implementation)) ||
		    !reserve(live, context))
			return false;
		auto state = std::make_unique<zone_reset_room_publication_stage::implementation>();
		if (zone_reset_item_command_decode_bounded(command, &state->original, reserve,
							   context,
							   live) != economic_accounting_error::ok ||
		    state->original.items.size() != factories.size())
			return false;
		size_t image_heap = 0;
		if (!room_prepare_image_heap(state->original, &image_heap) ||
		    !room_prepare_add(live, image_heap))
			return false;
		state->room = real_room(state->original.room_vnum);
		if (!world || state->room < 0 || state->room > top_of_world ||
		    !room_prepare_same_invocation(sources[0], state->original.reset_source) ||
		    sources[0].slot != state->original.reset_source.slot)
			return false;
		// Fresh reserve/resize and string assignment requests on the pinned policy.
		if (!room_prepare_array(live, factories.size(),
					sizeof(quest_mobile_native_item_stage *)) ||
		    !room_prepare_array(live, factories.size(), sizeof(P_obj)) ||
		    !room_prepare_array(live, factories.size(), sizeof(size_t)) ||
		    !room_prepare_array(live, factories.size(),
					sizeof(item_ownership_runtime_entry)) ||
		    !room_prepare_array(live, factories.size(), sizeof(economic_source_event)) ||
		    (selected_root.size() > 15 &&
		     (selected_root.size() == SIZE_MAX ||
		      !room_prepare_add(live, selected_root.size() + 1))) ||
		    !reserve(live, context))
			return false;
		state->stage_pointers.reserve(factories.size());
		state->objects.reserve(factories.size());
		state->original_indices.reserve(factories.size());
		state->custody.resize(factories.size());
		state->factory_sources.reserve(factories.size());
		{
			// Copy construction requests exact source length; fresh SSO assignment
			// would instead grow to twice its prior inline capacity for short roots.
			size_t text_live = live;
			if (!room_prepare_add(text_live, sizeof(std::string)) ||
			    !reserve(text_live, context))
				return false;
			std::string retained_root(selected_root);
			state->selected_root = std::move(retained_root);
		}
		{
			size_t progress_live = live;
			if (!room_prepare_array(progress_live, 2,
						sizeof(quest_mobile_native_item_progress)) ||
			    !reserve(progress_live, context))
				return false;
			for (size_t at = 0; at < factories.size(); ++at)
			{
				auto *factory = factories[at];
				quest_mobile_native_item_progress progress{};
				if (!factory || !factory->is_flat_factory() ||
				    !factory->flat_factory_matches(selected_root, sources[at]) ||
				    !room_prepare_same_invocation(sources[at],
								  state->original.reset_source) ||
				    !factory->object() ||
				    !factory->owns_pending_original_target(factory->object()) ||
				    factory->object()->obj_uid !=
					    state->original.items[at].object_uid ||
				    !factory->read_progress(&progress) || progress.admitted ||
				    progress.published || progress.next_step ||
				    progress.current_step_started)
					return false;
				for (size_t prior = 0; prior < at; ++prior)
					if (factories[prior] == factory ||
					    sources[prior].slot == sources[at].slot)
						return false;
				state->stage_pointers.push_back(factory);
				state->objects.push_back(factory->object());
				state->original_indices.push_back(at);
				state->factory_sources.push_back(sources[at]);
			}
		}
		// The full original literal capture and both canonical comparisons survive
		// through placement validation; each first output stays in the next prefix.
		if (!room_prepare_add(live, sizeof(std::vector<player_item_snapshot>)) ||
		    !room_prepare_array(live, 2, sizeof(std::vector<uint8_t>)) ||
		    !reserve(live, context))
			return false;
		std::vector<player_item_snapshot> observed;
		std::vector<uint8_t> a, b;
		size_t observed_heap = 0;
		if (player_item_snapshot_tree_capture_literal_bounded(
			    state->objects[0], &observed, nullptr, reserve, context, live,
			    &observed_heap) != player_snapshot_capture_result::ok ||
		    !room_prepare_add(live, observed_heap) ||
		    player_item_snapshot_list_encode_bounded(observed, &a, reserve, context,
							     live) !=
			    player_snapshot_codec_result::ok ||
		    !room_prepare_add(live, a.capacity()) ||
		    player_item_snapshot_list_encode_bounded(state->original.items, &b, reserve,
							     context, live) !=
			    player_snapshot_codec_result::ok ||
		    !room_prepare_add(live, b.capacity()) || a != b)
			return false;
		// Source comparison arrays die before recipe()'s actual candidate DTO.
		size_t placement_live = live;
		if (!room_prepare_add(placement_live, sizeof(zone_reset_room_placement_recipe)) ||
		    !room_prepare_add(
			    placement_live,
			    std::max(3 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
				     sizeof(zone_reset_room_placement_recipe))) ||
		    !reserve(placement_live, context))
			return false;
		zone_reset_room_placement_recipe recipe;
		if (!placement->matches_source(state->original.reset_source) ||
		    !placement->recipe(&recipe) || !state->original.placement ||
		    recipe != *state->original.placement || recipe.fall_selected)
			return false;
		// Existing publication retained_size conservatively counts inline string
		// capacity as well. Carry that established census allowance prospectively
		// before transfer; it is not a new heap-allocation claim or ABI certificate.
		for (const auto &item : state->original.items)
		{
			if ((item.name.capacity() <= 15 &&
			     !room_prepare_add(placement_live, item.name.capacity() + 1)) ||
			    (item.short_description.capacity() <= 15 &&
			     !room_prepare_add(placement_live,
					       item.short_description.capacity() + 1)) ||
			    (item.description.capacity() <= 15 &&
			     !room_prepare_add(placement_live, item.description.capacity() + 1)) ||
			    (item.action_description.capacity() <= 15 &&
			     !room_prepare_add(placement_live,
					       item.action_description.capacity() + 1)))
				return false;
			for (const auto &description : item.extra_descriptions)
				if ((description.keyword.capacity() <= 15 &&
				     !room_prepare_add(placement_live,
						       description.keyword.capacity() + 1)) ||
				    (description.description.capacity() <= 15 &&
				     !room_prepare_add(placement_live,
						       description.description.capacity() + 1)))
					return false;
		}
		if (!reserve(placement_live, context))
			return false;
		state->original_placement = placement;
		state->flat_backend = true;
		state->warm = true;
		output->state_ = state.release();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

#include "flatfile/flatfile_accounting_zone_reset_item_transaction.h"

namespace
{
// Exact fresh copy-construction requests, including nested row-owned payload.
// No allocator metadata/stack-padding claim; request ABI is checked by caller.
bool room_snapshot_fresh_heap(const player_item_snapshot &item, size_t *output) noexcept
{
	size_t bytes = 0;
	const auto text = [&](const std::string &value) noexcept
	{
		return value.size() <= 15 ||
		       (value.size() != SIZE_MAX && room_prepare_add(bytes, value.size() + 1));
	};
	if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
	    !text(item.action_description) ||
	    !room_prepare_array(bytes, item.dynamic_affects.size(),
				sizeof(player_item_dynamic_affect_snapshot)) ||
	    !room_prepare_array(bytes, item.extra_descriptions.size(),
				sizeof(player_item_extra_description_snapshot)))
		return false;
	for (const auto &description : item.extra_descriptions)
		if (!text(description.keyword) || !text(description.description) ||
		    !room_prepare_array(bytes, description.spell_ids.size(), sizeof(int32_t)))
			return false;
	*output = bytes;
	return true;
}
bool room_snapshot_inline_census(const player_item_snapshot &item, size_t &bytes) noexcept
{
	// Preserve the old retained_size policy's conservative inline-text term.
	const auto text = [&](const std::string &value) noexcept
	{ return value.capacity() > 15 || room_prepare_add(bytes, value.capacity() + 1); };
	if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
	    !text(item.action_description))
		return false;
	for (const auto &description : item.extra_descriptions)
		if (!text(description.keyword) || !text(description.description))
			return false;
	return true;
}
bool room_same_item_bounded(const player_item_snapshot &actual,
			    const player_item_snapshot &expected,
			    bool (*reserve)(size_t, void *) noexcept, void *context,
			    size_t outer_live) noexcept
{
	size_t actual_heap = 0, expected_heap = 0, live = outer_live;
	if (!reserve || !room_snapshot_fresh_heap(actual, &actual_heap) ||
	    !room_snapshot_fresh_heap(expected, &expected_heap) ||
	    !room_prepare_array(live, 2, sizeof(player_item_snapshot)) ||
	    !room_prepare_array(live, 2, sizeof(std::vector<uint8_t>)) ||
	    !room_prepare_add(live, actual_heap) || !room_prepare_add(live, expected_heap) ||
	    !reserve(live, context))
		return false;
	try
	{
		// Same original normalization and both full canonical encodes. Singleton
		// reserve/push avoids initializer-list copies without reducing validation.
		player_item_snapshot a(actual), b(expected);
		a.parent_index = b.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		a.equipment_slot = b.equipment_slot = 0;
		std::vector<uint8_t> encoded_a, encoded_b;
		{
			size_t phase = live;
			if (!room_prepare_add(phase, sizeof(std::vector<player_item_snapshot>)) ||
			    !room_prepare_add(phase, sizeof(player_item_snapshot)) ||
			    !room_prepare_add(phase, actual_heap) || !reserve(phase, context))
				return false;
			std::vector<player_item_snapshot> singleton;
			singleton.reserve(1);
			singleton.push_back(a);
			if (player_item_snapshot_list_encode_bounded(singleton, &encoded_a, reserve,
								     context, phase) !=
			    player_snapshot_codec_result::ok)
				return false;
		}
		if (!room_prepare_add(live, encoded_a.capacity()))
			return false;
		{
			size_t phase = live;
			if (!room_prepare_add(phase, sizeof(std::vector<player_item_snapshot>)) ||
			    !room_prepare_add(phase, sizeof(player_item_snapshot)) ||
			    !room_prepare_add(phase, expected_heap) || !reserve(phase, context))
				return false;
			std::vector<player_item_snapshot> singleton;
			singleton.reserve(1);
			singleton.push_back(b);
			if (player_item_snapshot_list_encode_bounded(singleton, &encoded_b, reserve,
								     context, phase) !=
			    player_snapshot_codec_result::ok)
				return false;
		}
		return encoded_a == encoded_b;
	}
	catch (...)
	{
		return false;
	}
}
size_t room_uid_index(const std::vector<player_item_snapshot> &items, uint64_t uid) noexcept
{
	size_t found = SIZE_MAX;
	for (size_t at = 0; at < items.size(); ++at)
		if (items[at].object_uid == uid)
		{
			if (found != SIZE_MAX)
				return SIZE_MAX;
			found = at;
		}
	return found;
}
bool room_absent_native(const std::vector<player_item_snapshot> &items) noexcept
{
	for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	P_obj previous = nullptr;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (object->prev != previous ||
		    std::any_of(items.begin(), items.end(), [object](const auto &item)
				{ return item.object_uid == object->obj_uid; }))
			return false;
		previous = object;
	}
	return true;
}
struct room_physical_workspace
{
	std::vector<uint8_t> seen;
	std::vector<player_item_snapshot> observed;
	std::vector<uint64_t> selected;
	std::vector<item_ownership_runtime_entry> actual;
};
struct room_refresh_workspace
{
	flatfile_zone_reset_item_projection projection;
	sql_room_item_graph ordered;
	std::vector<item_ownership_runtime_entry> custody;
	std::vector<player_item_snapshot> native;
	std::vector<uint8_t> a, b;
	std::vector<uint64_t> uids;
	std::vector<item_ownership_runtime_entry> runtime;
};
}

bool zone_reset_room_publication_owner::verify_warm_flat_current_bounded(
	const zone_reset_room_publication_stage &stage, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!stage.state_ || !stage.state_->warm || !stage.state_->flat_backend ||
	    !stage.state_->consumed || !nevent_is_game_thread() || !reserve)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	return false;
#else
	try
	{
		const auto &state = *stage.state_;
		const size_t count = state.graph.items.size();
		if (!count || count > ITEM_TRANSFER_MAX_ITEMS || state.objects.size() != count ||
		    state.custody.size() != count)
			return false;
		size_t live = outer_live;
		if (!room_prepare_add(live, sizeof(room_physical_workspace)) ||
		    !room_prepare_add(live, count) || !reserve(live, context))
			return false;
		room_physical_workspace work;
		work.seen.resize(count, 0);
		for (size_t at = 0; at < count; ++at)
			if (!state.objects[at] ||
			    room_uid_index(state.graph.items, state.graph.items[at].object_uid) !=
				    at)
				return false;
		for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (object->prev != previous)
				return false;
			previous = object;
			const size_t at = room_uid_index(state.graph.items, object->obj_uid);
			if (at != SIZE_MAX)
			{
				if (state.objects[at] != object || work.seen[at])
					return false;
				work.seen[at] = 1;
			}
		}
		if (std::find(work.seen.begin(), work.seen.end(), 0) != work.seen.end())
			return false;
		for (size_t at = 1; at < count; ++at)
		{
			const size_t parent = room_uid_index(state.graph.items,
							     state.custody[at].parent_item_uid);
			if (parent == SIZE_MAX || state.objects[at]->loc_p != LOC_INSIDE ||
			    state.objects[at]->loc.inside != state.objects[parent])
				return false;
		}
		P_obj root = state.objects[0];
		if (state.placed)
		{
			if (!world || state.room < 0 || state.room > top_of_world ||
			    root->loc_p != LOC_ROOM || root->loc.room != state.room)
				return false;
			for (P_obj slow = world[state.room].contents, fast = slow;
			     fast && fast->next_content;)
			{
				slow = slow->next_content;
				fast = fast->next_content->next_content;
				if (slow == fast)
					return false;
			}
			size_t matches = 0;
			for (P_obj object = world[state.room].contents; object;
			     object = object->next_content)
			{
				if (object->loc_p != LOC_ROOM || object->loc.room != state.room)
					return false;
				if (object == root)
					++matches;
			}
			if (matches != 1)
				return false;
		}
		else if (root->loc_p != LOC_NOWHERE || root->loc.room != NOWHERE ||
			 root->next_content)
			return false;
		size_t observed_heap = 0;
		if (player_item_snapshot_tree_capture_literal_bounded(
			    root, &work.observed, nullptr, reserve, context, live,
			    &observed_heap) != player_snapshot_capture_result::ok ||
		    work.observed.size() != count || !room_prepare_add(live, observed_heap))
			return false;
		std::fill(work.seen.begin(), work.seen.end(), 0);
		for (size_t row = 0; row < work.observed.size(); ++row)
		{
			const auto &item = work.observed[row];
			const size_t at = room_uid_index(state.graph.items, item.object_uid);
			if (at == SIZE_MAX || work.seen[at] ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(row) ||
			    (item.parent_index < 0 ? 0 :
						     work.observed[item.parent_index].object_uid) !=
				    state.custody[at].parent_item_uid ||
			    !room_same_item_bounded(item, state.graph.items[at], reserve, context,
						    live))
				return false;
			work.seen[at] = 1;
		}
		if (!room_prepare_array(live, count, sizeof(uint64_t)) || !reserve(live, context))
			return false;
		work.selected.reserve(count);
		for (const auto &item : state.graph.items)
			work.selected.push_back(item.object_uid);
		std::sort(work.selected.begin(), work.selected.end());
		size_t observer_live = live;
		if (!room_prepare_array(observer_live, 2, sizeof(std::span<const uint64_t>)) ||
		    !reserve(observer_live, context))
			return false;
		if (!item_ownership_runtime_published_native_observer::snapshot_links_bounded(
			    work.selected, ITEM_TRANSFER_MAX_ITEMS, &work.actual, reserve, context,
			    observer_live) ||
		    work.actual.size() != count ||
		    !room_prepare_array(live, work.actual.capacity(),
					sizeof(item_ownership_runtime_entry)) ||
		    !reserve(live, context))
			return false;
		std::fill(work.seen.begin(), work.seen.end(), 0);
		for (const auto &entry : work.actual)
		{
			const size_t at = room_uid_index(state.graph.items, entry.item_uid);
			if (at == SIZE_MAX || work.seen[at] ||
			    !same_custody(entry, state.custody[at]) ||
			    entry.owner_revision != state.graph.owner_revision)
				return false;
			work.seen[at] = 1;
		}
		uint64_t revision = 0;
		return item_ownership_runtime_peek_owner_revision(state.graph.owner, &revision) &&
		       revision == state.graph.owner_revision;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::refresh_warm_flat_locked_bounded(
	const std::string &selected_root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	zone_reset_room_publication_stage &stage, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!stage.state_ || !stage.state_->warm || !stage.state_->flat_backend ||
	    !nevent_is_game_thread() || !reserve || !lock.matches(selected_root) ||
	    stage.state_->selected_root != selected_root || stage.state_->cold_adoption ||
	    stage.state_->cold_reconstruction || stage.state_->cold_pending ||
	    receipt.operation_id.bytes != original.command.operation_id.bytes ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.error_code || receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	return false;
#else
	try
	{
		auto &state = *stage.state_;
		const size_t count = state.original.items.size();
		if (!count || count > ITEM_TRANSFER_MAX_ITEMS || state.objects.size() != count ||
		    state.stage_pointers.size() != count || state.factory_sources.size() != count ||
		    state.original.operation_id.bytes != original.command.operation_id.bytes ||
		    state.original.expected_room_revision == UINT64_MAX ||
		    receipt.durable_revision != state.original.expected_room_revision + 1)
			return false;
		size_t live = outer_live;
		if (!room_prepare_add(live, sizeof(room_refresh_workspace)) ||
		    !reserve(live, context))
			return false;
		room_refresh_workspace work;
		size_t projection_heap = 0;
		if (flatfile_zone_reset_item_publication_storage::read_locked_bounded(
			    selected_root, lock, original, receipt, &work.projection, reserve,
			    context, live, &projection_heap) != 0 ||
		    !room_prepare_add(live, projection_heap) ||
		    work.projection.room.room_vnum != state.original.room_vnum ||
		    work.projection.room.revision != receipt.durable_revision ||
		    work.projection.custody.size() != count)
			return false;
		// The provider authenticates the WHOLE room and custody history. Select each
		// original UID exactly once; prior unrelated room forests remain untouched.
		if (!room_prepare_array(live, count, sizeof(player_item_snapshot)) ||
		    !room_prepare_array(live, count, sizeof(player_load_item_identity)) ||
		    !room_prepare_array(live, count, sizeof(item_ownership_runtime_entry)))
			return false;
		for (const auto &item : state.original.items)
		{
			size_t heap = 0;
			if (!room_snapshot_fresh_heap(item, &heap) || !room_prepare_add(live, heap))
				return false;
		}
		if (!reserve(live, context))
			return false;
		work.ordered.owner = { item_owner_type::room,
				       static_cast<uint64_t>(state.original.room_vnum), 0 };
		work.ordered.owner_revision = work.projection.room.revision;
		work.ordered.items.reserve(count);
		work.ordered.identities.reserve(count);
		work.custody.reserve(count);
		for (size_t at = 0; at < count; ++at)
		{
			const auto &item = state.original.items[at];
			const size_t row =
				room_uid_index(work.projection.room.items, item.object_uid);
			if (room_uid_index(state.original.items, item.object_uid) != at ||
			    row == SIZE_MAX ||
			    !room_same_item_bounded(work.projection.room.items[row], item, reserve,
						    context, live))
				return false;
			size_t match = SIZE_MAX;
			for (size_t i = 0; i < work.projection.custody.size(); ++i)
				if (work.projection.custody[i].item_uid == item.object_uid)
				{
					if (match != SIZE_MAX)
						return false;
					match = i;
				}
			if (match == SIZE_MAX || item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(at))
				return false;
			const auto &identity = work.projection.custody[match];
			const uint64_t parent =
				item.parent_index < 0 ?
					0 :
					state.original.items[item.parent_index].object_uid;
			if (identity.root_item_uid != state.original.items[0].object_uid ||
			    identity.parent_item_uid != parent || identity.item_revision != 1 ||
			    identity.vnum != item.vnum ||
			    identity.state != item_custody_state::active ||
			    !item_owner_identity_equal(identity.owner, work.ordered.owner))
				return false;
			// Only actual DTO temporaries overlap these three non-growing pushes.
			size_t row_live = live;
			if (!room_prepare_add(row_live, sizeof(player_load_item_identity)) ||
			    !room_prepare_add(row_live, sizeof(item_ownership_runtime_entry)) ||
			    !reserve(row_live, context))
				return false;
			player_load_item_identity projected{};
			projected.item_uid = identity.item_uid;
			projected.root_item_uid = identity.root_item_uid;
			projected.parent_item_uid = identity.parent_item_uid;
			projected.owner = identity.owner;
			projected.item_revision = identity.item_revision;
			projected.owner_revision = work.ordered.owner_revision;
			projected.state = identity.state;
			work.ordered.items.push_back(item);
			work.ordered.identities.push_back(projected);
			work.custody.push_back(
				{ identity.item_uid, identity.root_item_uid,
				  identity.parent_item_uid, identity.owner, identity.item_revision,
				  work.ordered.owner_revision, item.vnum, identity.state });
		}
		if (!state.consumed)
		{
			size_t native_heap = 0;
			if (player_item_snapshot_tree_capture_literal_bounded(
				    state.objects[0], &work.native, nullptr, reserve, context, live,
				    &native_heap) != player_snapshot_capture_result::ok ||
			    !room_prepare_add(live, native_heap) ||
			    player_item_snapshot_list_encode_bounded(work.native, &work.a, reserve,
								     context, live) !=
				    player_snapshot_codec_result::ok ||
			    !room_prepare_add(live, work.a.capacity()) ||
			    player_item_snapshot_list_encode_bounded(state.original.items, &work.b,
								     reserve, context, live) !=
				    player_snapshot_codec_result::ok ||
			    !room_prepare_add(live, work.b.capacity()) || work.a != work.b)
				return false;
			size_t progress_live = live;
			if (!room_prepare_array(progress_live, 2,
						sizeof(quest_mobile_native_item_progress)) ||
			    !reserve(progress_live, context))
				return false;
			for (size_t at = 0; at < count; ++at)
			{
				auto *factory = state.stage_pointers[at];
				quest_mobile_native_item_progress progress{};
				if (!factory ||
				    !factory->flat_factory_matches(selected_root,
								   state.factory_sources[at]) ||
				    factory->object() != state.objects[at] ||
				    !factory->owns_pending_original_target(state.objects[at]) ||
				    !factory->read_progress(&progress) ||
				    progress.admitted != state.admitted || progress.published ||
				    progress.next_step || progress.current_step_started)
					return false;
			}
			if (!room_absent_native(state.original.items) ||
			    !room_prepare_array(live, count, sizeof(uint64_t)) ||
			    !reserve(live, context))
				return false;
			work.uids.reserve(count);
			for (const auto &item : state.original.items)
				work.uids.push_back(item.object_uid);
			std::sort(work.uids.begin(), work.uids.end());
			size_t observer_live = live;
			if (!room_prepare_array(observer_live, 2,
						sizeof(std::span<const uint64_t>)) ||
			    !reserve(observer_live, context))
				return false;
			if (!item_ownership_runtime_published_native_observer::snapshot_links_bounded(
				    work.uids, ITEM_TRANSFER_MAX_ITEMS, &work.runtime, reserve,
				    context, observer_live) ||
			    !work.runtime.empty())
				return false;
		}
		else
		{
			if (state.graph.owner_revision != work.ordered.owner_revision ||
			    state.custody.size() != work.custody.size())
				return false;
			for (size_t at = 0; at < count; ++at)
				if (!same_custody(state.custody[at], work.custody[at]))
					return false;
			if (!verify_warm_flat_current_bounded(stage, reserve, context, live))
				return false;
		}
		// Preserved inline-text census allowance precedes nonthrowing transfer.
		for (const auto &item : work.ordered.items)
			if (!room_snapshot_inline_census(item, live))
				return false;
		if (!lock.matches(selected_root) || !reserve(live, context))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<sql_room_item_graph>);
		state.graph = std::move(work.ordered);
		state.custody = std::move(work.custody);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
using room_union_set = std::unordered_set<uint64_t>;
using room_union_node =
	std::__detail::_Hash_node<uint64_t,
				  std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;
bool room_union_heap(const room_union_set &values, size_t &heap) noexcept
{
	heap = 0;
	return room_prepare_array(heap, values.size(), sizeof(room_union_node)) &&
	       (values.bucket_count() == 1 ||
		room_prepare_array(heap, values.bucket_count(),
				   sizeof(std::__detail::_Hash_node_base *)));
}
struct room_union_workspace
{
	std::__detail::_Prime_rehash_policy policy;
};
struct room_union_live
{
	const room_union_set &values;
	const size_t &fixed;
	bool admit(size_t request, bool (*reserve)(size_t, void *) noexcept,
		   void *context) const noexcept
	{
		size_t current = fixed, heap = 0;
		return room_union_heap(values, heap) && room_prepare_add(current, heap) &&
		       room_prepare_add(current, request) && reserve && reserve(current, context);
	}
};
bool room_union_insert_bounded(room_union_set &values, uint64_t uid, room_union_workspace &work,
			       const room_union_live &live,
			       bool (*reserve)(size_t, void *) noexcept, void *context)
{
	if (values.find(uid) != values.end())
		return true;
	// This private warm union has only original default-growth insertions before
	// consumption (including retained partial attempts), never reserve/rehash or
	// load-factor changes. Reconstruct that actual policy's next-resize threshold;
	// bucket1 is the original embedded empty state with next_resize0.
	if (values.max_load_factor() != work.policy.max_load_factor())
		return false;
	work.policy._M_reset(values.bucket_count() == 1 ? 0 : values.bucket_count());
	const auto growth = work.policy._M_need_rehash(values.bucket_count(), values.size(), 1);
	size_t request = sizeof(room_union_node);
	if ((growth.first && !room_prepare_array(request, growth.second,
						 sizeof(std::__detail::_Hash_node_base *))) ||
	    !live.admit(request, reserve, context))
		return false;
	// Original unreserved insertion pattern is retained: new node + old buckets +
	// replacement buckets coexist, admitted before the node's first allocation.
	values.insert(uid);
	return true;
}
#endif
} // namespace

bool zone_reset_room_publication_owner::reserve_warm_consume_bounded(
	zone_reset_room_publication_stage &stage, const std::unordered_set<uint64_t> &published,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!stage.state_ || !stage.state_->warm || !stage.state_->flat_backend ||
	    stage.state_->consumed || !nevent_is_game_thread() || stage.state_->graph.items.empty())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)published;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	try
	{
		auto &state = *stage.state_;
		size_t initial_heap = 0;
		if (!room_union_heap(state.next_published, initial_heap) ||
		    outer_live < initial_heap)
			return false;
		size_t fixed = outer_live - initial_heap;
		if (!room_prepare_add(fixed, sizeof(room_union_workspace)) ||
		    !room_prepare_add(fixed, sizeof(room_union_live)) ||
		    !room_prepare_add(fixed, sizeof(std::pair<bool, size_t>)))
			return false;
		size_t initial = fixed;
		if (!room_prepare_add(initial, initial_heap) || !reserve ||
		    !reserve(initial, context))
			return false;
		room_union_workspace work;
		room_union_live live{ state.next_published, fixed };
		// An earlier allocation refusal can retain a partial original union.
		// Extend only that proven subset before intent; never overwrite it or
		// interpret its incompleteness as a successful native publication.
		for (const auto uid : state.next_published)
			if (!published.count(uid) &&
			    std::none_of(state.graph.items.begin(), state.graph.items.end(),
					 [uid](const auto &item)
					 { return item.object_uid == uid; }))
				return false;
		for (const auto uid : published)
			if (!room_union_insert_bounded(state.next_published, uid, work, live,
						       reserve, context))
				return false;
		for (const auto &item : state.graph.items)
		{
			if (published.count(item.object_uid))
				return false;
			if (!room_union_insert_bounded(state.next_published, item.object_uid, work,
						       live, reserve, context))
				return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_room_publication_owner::consume_bounded(zone_reset_room_publication_stage &stage,
							std::unordered_set<uint64_t> *published,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	if (!stage.state_ || !published || !nevent_is_game_thread() || !stage.state_->admitted ||
	    stage.state_->consumed || !stage.state_->warm || !stage.state_->flat_backend)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	auto &state = *stage.state_;
	// The original warm caller allocated and charged this exact union
	// before its intent checkpoint. Do not grow holders after that cut.
	if (state.next_published.size() != published->size() + state.graph.items.size())
		return false;
	for (const auto uid : *published)
		if (!state.next_published.count(uid))
			return false;
	for (const auto &item : state.graph.items)
		if (published->count(item.object_uid) ||
		    !state.next_published.count(item.object_uid))
			return false;
	if (!world || state.room < 0 || state.room > top_of_world ||
	    state.objects[0]->loc_p != LOC_NOWHERE || state.objects[0]->loc.room != NOWHERE ||
	    state.objects[0]->next_content)
		return false;
	struct input_spans
	{
		std::span<quest_mobile_native_item_stage *> factories;
		std::span<P_obj> objects;
		std::span<const item_ownership_runtime_entry> custody;
		input_spans(zone_reset_room_publication_stage::implementation &source) noexcept
			: factories(source.stage_pointers)
			, objects(source.objects)
			, custody(source.custody)
		{
		}
	};
	size_t live = outer_live;
	if (!room_prepare_add(live, sizeof(input_spans)) || !reserve || !reserve(live, context))
		return false;
	input_spans inputs{ state };
	if (!quest_mobile_native_item_stage::publish_many_bounded(
		    inputs.factories, inputs.objects, inputs.custody, reserve, context, live))
		return false;
	// No allocating work or budget callback may intervene after atomic cache/
	// native consumption. Caller retains CURRENT cache on every return.
	state.consumed = true;
	return true;
#endif
}

bool zone_reset_room_publication_owner::mark_published_bounded(
	zone_reset_room_publication_stage &stage, std::unordered_set<uint64_t> *published,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!stage.state_ || !published || !stage.state_->warm || !stage.state_->flat_backend ||
	    !stage.state_->placed ||
	    !verify_warm_flat_current_bounded(stage, reserve, context, outer_live))
		return false;
	auto &state = *stage.state_;
	if (state.bookkeeping_published)
	{
		for (const auto &item : state.graph.items)
			if (!published->count(item.object_uid))
				return false;
		return true;
	}
	// All nodes were allocated before batch publication. The real context owner
	// keeps this tracker stable across its publication steps; unrelated progress
	// must not be silently overwritten by an old preparation-time union.
	if (state.next_published.size() < state.graph.items.size() ||
	    published->size() != state.next_published.size() - state.graph.items.size())
		return false;
	for (const auto &item : state.graph.items)
		if (published->count(item.object_uid))
			return false;
	for (const auto uid : *published)
		if (!state.next_published.count(uid))
			return false;
	for (const auto &item : state.graph.items)
		if (!state.next_published.count(item.object_uid))
			return false;
	published->swap(state.next_published);
	state.bookkeeping_published = true;
	if (state.warm)
	{
		// Actual complete placement/cache proof above still protects these
		// live objects. End their creation-candidate marker before final SQL/
		// ACK/terminal proof; later metadata cleanup never touches old pointers.
		for (P_obj object : state.objects)
			REMOVE_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
	}
	return true;
}

// The callback borrows the caller's genuine selected-root lock. It never
// acquires a second lock, invents an envelope, or drops the coordinator prefix.
struct zone_reset_room_publication_owner::flat_terminal_retirement
{
	const std::string &root;
	const flatfile_authority_lock &lock;
	uint64_t origin_uid;
	bool (*reserve)(size_t, void *) noexcept;
	void *budget_context;
};

bool zone_reset_room_publication_owner::retain_terminal_flat_bounded(
	const critical_native_recovery_envelope &expected, void *context,
	size_t coordinator_live) noexcept
{
	const auto *transfer = static_cast<const flat_terminal_retirement *>(context);
	if (!transfer || !nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    !transfer->reserve || !transfer->lock.matches(transfer->root))
		return false;
	// The real storage owner recovers on EVERY attempt under this same lock,
	// authenticates immutable origin/full terminal BODY and receipt, and requires
	// complete apply/unlink plus exact readback before reporting durable transfer.
	// Refusal leaves the guarded coordinator's real carrier/fences intact.
	return flatfile_zone_reset_item_publication_storage::retain_terminal_locked_bounded(
		       transfer->root, transfer->lock, transfer->origin_uid, expected,
		       transfer->reserve, transfer->budget_context, coordinator_live) == 0;
}

bool zone_reset_room_publication_owner::retire_warm_flat_locked_bounded(
	const std::string &selected_root, const flatfile_authority_lock &lock, uint64_t origin_uid,
	const critical_native_recovery_envelope &expected, uint64_t original_generation,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() || !reserve ||
	    selected_root.empty() || !origin_uid || origin_uid == UINT64_MAX ||
	    !original_generation || !lock.matches(selected_root))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)expected;
	(void)budget_context;
	(void)outer_live;
	return false;
#else
	// References borrow already-counted root/lock and actual expected carrier.
	// Admit this complete named callback frame BEFORE construction and keep it
	// in the prefix through coordinator validation, terminal transfer and journal.
	size_t live = outer_live;
	if (!room_prepare_add(live, sizeof(flat_terminal_retirement)) ||
	    !reserve(live, budget_context))
		return false;
	flat_terminal_retirement transfer{ selected_root, lock, origin_uid, reserve,
					   budget_context };
	// The owning coordinator pins original operation/generation/physical release,
	// proves full phase/revision/BODY, relays its identity/scratch to our callback,
	// retires the complete mixed journal, then releases only confirmed fences.
	// No budget callback or allocating revalidation follows confirmed retirement.
	return critical_zone_reset_item_publication_owner::retire_bounded(
		expected, original_generation, retain_terminal_flat_bounded, &transfer, reserve,
		budget_context, live);
#endif
}

bool zone_reset_room_publication_owner::service_step_bounded(
	zone_reset_room_publication_stage &stage, size_t at, size_t step,
	quest_mobile_native_item_effect &effect, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	// Genuine retained warm-FLAT factories only; other original paths stay original.
	// All stage/private retention and CURRENT pool/output/Zombie allowances belong
	// to caller outer exactly once and must be refreshed on EVERY return.
	if (!reserve || !stage.state_ || !stage.state_->warm || !stage.state_->flat_backend ||
	    !stage.state_->consumed || stage.state_->placed ||
	    at >= stage.state_->stage_pointers.size())
		return false;
	return stage.state_->stage_pointers[at]->publication_step_bounded(
		step, stage.state_->objects[at], effect, reserve, context, outer_live);
}

bool zone_reset_room_publication_owner::copy_warm_bounded(const critical_command &command,
							  critical_native_recovery_envelope *output,
							  bool (*reserve)(size_t, void *) noexcept,
							  void *context, size_t outer_live) noexcept
{
	return critical_zone_reset_item_publication_owner::copy_context_bounded(
		command, output, reserve, context, outer_live);
}
bool zone_reset_room_publication_owner::checkpoint_warm_bounded(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor, uint64_t original_generation,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return critical_zone_reset_item_publication_owner::checkpoint_context_bounded(
		expected, successor, reserve, context, outer_live, original_generation);
}
bool zone_reset_room_publication_owner::generation_warm_bounded(
	const critical_native_recovery_envelope &expected, uint64_t *generation,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return critical_zone_reset_item_publication_owner::observe_generation_bounded(
		expected, generation, reserve, context, outer_live);
}
bool zone_reset_room_publication_owner::acknowledge_warm_bounded(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t original_generation, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return critical_zone_reset_item_publication_owner::acknowledge_bounded(
		expected, receipt, original_generation, reserve, context, outer_live);
}
