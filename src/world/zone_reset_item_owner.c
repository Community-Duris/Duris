#include "world/zone_reset_item_owner.h"
#include "flatfile/flatfile_accounting_zone_reset_item_transaction.h"
#include "flatfile/flatfile_season_state.h"
#include "world/zone_reset_room_publication.h"
#include "world/handler.h"
#include <array>
#include "economy/economic_gameplay_authority.h"
#include "economy/zone_reset_item_recovery.h"
#include "player/player_snapshot_codec.h"
#include "player/inert_item_stage.h"
#include "persistence/sql_room_item_payload.h"
#include "item/item_transfer_repository.h"
#include "item/item_ownership_runtime.h"
#include "item/item_movement_transaction.h"
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#endif
#include "world/quest_mobile_native_birth.h"
#include "world/db.h"
#include "world/events.h"
#include "world/object_template.h"
#include "item/item_uid_allocator.h"
#include "player/player_snapshot_capture.h"
#include "player/player_save_pipeline.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/zone_reset_room_nesting.h"
#include "world/vnum.obj.h"
#include "persistence/critical_command_coordinator.h"
#include <algorithm>
#include <unordered_map>

#include <chrono>
#include <memory>
#include <utility>
#include <type_traits>

extern index_data *obj_index;
extern zone_data *zone_table;
extern int top_of_objt, top_of_zone_table;
extern P_room world;
extern P_obj object_list;
extern int top_of_world;

struct zone_reset_item_owner::flat_binding_scratch_guard
{
	bool active;
	explicit flat_binding_scratch_guard(bool selected_flat) noexcept
		: active(selected_flat)
	{
	}
	~flat_binding_scratch_guard()
	{
		// Declared before the original capture/binding temporaries, so their
		// storage dies before the genuine native owner's retained recensus.
		if (active)
			(void)quest_mobile_native_birth_owner::charge();
	}
};

struct zone_reset_item_root_stage::implementation
{
	quest_mobile_native_item_stage factory;
	std::unique_ptr<quest_mobile_native_flat_factory_scope> flat_scope;
	bool flat_backend = false;
	critical_operation_id flat_lineage{}, flat_epoch{};
	shop_trade_original_procedure_binding_stage bindings;
	zone_reset_item_root_facts facts;
	P_obj object = nullptr;
	uint64_t reserved_uid = 0; // Original allocator observation, including misses.
	uint32_t slot = 0;
	int room = NOWHERE;
	zone_reset_item_root_result result = zone_reset_item_root_result::held_refusal;
	int original_percent = 0, observed_itemvalue = 0;
	bool pure_pending = true, operation_started = false, operation_returned = false;
	bool factory_started = false, factory_returned = false, factory_succeeded = false;
	bool load_started = false, load_returned = false, load_passed = false;
	bool cleanup_started = false, cleanup_returned = false, cleanup_succeeded = false;
	bool literal_captured = false, bindings_prepared = false;
};

zone_reset_item_root_result
zone_reset_item_owner::prepare_root(uint32_t slot, int room,
				    zone_reset_item_root_stage *output) noexcept
{
	using result = zone_reset_item_root_result;
	if (!output || !nevent_is_game_thread())
		return result::refused;
	economic_source_event source{};
	int32_t zone_vnum = -1;
	const bool observed =
		output->state_ ?
			quest_mobile_native_birth_owner::capture_retained_room_reset_source(
				slot, room, &source, &zone_vnum) :
			quest_mobile_native_birth_owner::capture_room_reset_source(
				slot, room, &source, &zone_vnum);
	if (!observed || !zone_table || !obj_index || !world || room < 0 || room > top_of_world)
		return output->state_ ? result::held_refusal : result::refused;
	const std::string *selected_flat_root = nullptr;
	if (!quest_mobile_native_birth_owner::capture_reset_backend(source, &selected_flat_root))
		return output->state_ ? result::held_refusal : result::refused;
	critical_operation_id flat_lineage{}, flat_epoch{};
	if (selected_flat_root && !quest_mobile_native_birth_owner::capture_reset_flat_projection(
					  source, &flat_lineage, &flat_epoch))
		return output->state_ ? result::held_refusal : result::refused;
	const zone_data *zone = nullptr;
	for (int index = 0; index <= top_of_zone_table; ++index)
		if (zone_table[index].number == zone_vnum)
		{
			if (zone)
				return result::refused;
			zone = &zone_table[index];
		}
	if (!zone || !zone->cmd)
		return result::refused;
	for (uint32_t index = 0; index < slot; ++index)
		if (zone->cmd[index].command == 'S')
			return result::refused;
	const auto command = zone->cmd[slot];
	if (command.command != 'O' || command.arg3 != room || command.arg1 < 0 ||
	    command.arg1 > top_of_objt || world[room].number <= 0)
		return result::refused;
	try
	{
		if (!output->state_)
		{
			auto state = std::make_unique<zone_reset_item_root_stage::implementation>();
			state->slot = slot;
			state->flat_backend = selected_flat_root != nullptr;
			state->flat_lineage = flat_lineage;
			state->flat_epoch = flat_epoch;
			state->room = room;
			state->original_percent = command.arg4;
			state->facts.reset_source = source;
			state->facts.zone_vnum = zone_vnum;
			state->facts.room_vnum = world[room].number;
			state->facts.object_rnum = command.arg1;
			state->facts.accepted_at_usec = static_cast<uint64_t>(
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count());
			output->state_ = state.release(); // Root before every fallible preparation.
		}
		auto &held = *output->state_;
		if (held.flat_backend != (selected_flat_root != nullptr) ||
		    held.flat_lineage.bytes != flat_lineage.bytes ||
		    held.flat_epoch.bytes != flat_epoch.bytes || held.slot != slot ||
		    held.room != room || held.facts.zone_vnum != zone_vnum ||
		    held.facts.room_vnum != world[room].number ||
		    held.facts.object_rnum != command.arg1 ||
		    held.original_percent != command.arg4 ||
		    held.facts.reset_source.kind != source.kind ||
		    held.facts.reset_source.source.bytes != source.source.bytes ||
		    held.facts.reset_source.generation.bytes != source.generation.bytes ||
		    held.facts.reset_source.sequence != source.sequence ||
		    held.facts.reset_source.slot != source.slot)
			return result::held_refusal;
		if (held.result != result::held_refusal || !held.pure_pending)
			return held.result;
		if (!quest_mobile_native_birth_owner::charge())
			return result::held_refusal;
		if (held.flat_backend)
		{
			if (!held.flat_scope)
			{
				const size_t chars = selected_flat_root->size();
				size_t peak = sizeof(quest_mobile_native_flat_factory_scope);
				if (chars > 15)
				{
					if (chars == SIZE_MAX || chars + 1 > SIZE_MAX - peak)
						return result::held_refusal;
					peak += chars + 1;
				}
				if (!quest_mobile_native_birth_owner::charge(peak))
					return result::held_refusal;
				// Pure provenance allocation precedes the actual factory attempt.
				// A refusal preserves the original source/UID and can retry safely.
				held.flat_scope.reset(new quest_mobile_native_flat_factory_scope(
					*selected_flat_root, source));
			}
			if (!held.flat_scope->current() ||
			    held.flat_scope->root_ != *selected_flat_root ||
			    !quest_mobile_native_birth_owner::charge())
				return result::held_refusal;
		}
		if (!held.operation_started)
		{
			held.operation_started = true;
			held.operation_returned =
				critical_operation_id_generate(&held.facts.operation_id) &&
				held.facts.operation_id.bytes != source.source.bytes;
		}
		if (!held.operation_returned)
		{
			held.pure_pending = false;
			return result::held_refusal;
		}
		if (!held.reserved_uid)
		{
			const auto uid = item_uid_allocator_next();
			if (!uid)
				return result::held_refusal; // No UID was issued; no factory attempted.
			held.reserved_uid = uid;
			if (uid == UINT64_MAX)
			{
				held.pure_pending = false;
				return result::held_refusal; // Never replace an actually returned UID.
			}
		}
		if (!held.factory_started)
		{
			if (held.flat_scope && !quest_mobile_native_birth_owner::charge(
						       held.flat_scope->retained_heap_bytes()))
				return result::held_refusal;
			held.factory_started = true;
			held.factory_succeeded =
				held.flat_scope ?
					quest_mobile_native_item_stage::prepare_retaining_flat(
						command.arg1, REAL, held.reserved_uid,
						*held.flat_scope, &held.factory) :
					quest_mobile_native_item_stage::prepare_retaining(
						command.arg1, REAL, held.reserved_uid,
						&held.factory);
			held.factory_returned = true;
			held.object = held.factory.object();
		}
		if (!held.factory_returned || !held.factory_succeeded)
		{
			held.pure_pending = false; // Never infer original read_object failure.
			(void)quest_mobile_native_birth_owner::charge();
			return result::held_refusal;
		}
		if (!quest_mobile_native_birth_owner::charge())
			return result::held_refusal; // Census the actual factory before the load roll.
		if (held.factory.is_flat_factory() != held.flat_backend ||
		    (held.flat_scope &&
		     !held.factory.flat_factory_matches(held.flat_scope->root_, source)))
			return result::held_refusal;
		P_obj object = held.object;
		if (!object || held.factory.object() != object ||
		    !held.factory.owns_pending_original_target(object) ||
		    object->obj_uid != held.reserved_uid || object->R_num != command.arg1 ||
		    object->loc_p != LOC_NOWHERE || object->loc.room != NOWHERE || object->next ||
		    object->prev || object->contains || object->next_content)
		{
			held.pure_pending = false;
			return result::held_refusal;
		}
		if (IS_ARTIFACT(object) || object->type == ITEM_CORPSE)
		{
			held.pure_pending = false;
			held.result = result::native_owner_required;
			return held.result;
		}
		if (!held.load_started)
		{
			held.load_started = true;
			held.observed_itemvalue = itemvalue(object);
			held.load_passed = ITEM_LOAD_CHECK(object, held.observed_itemvalue,
							   held.original_percent);
			held.load_returned = true;
		}
		if (!held.load_returned)
		{
			held.pure_pending = false;
			return result::held_refusal;
		}
		if (!held.load_passed)
		{
			if (!held.cleanup_started)
			{
				held.cleanup_started = true;
				held.cleanup_succeeded = held.factory.discard_unadmitted();
				held.cleanup_returned = true;
			}
			held.pure_pending = false;
			if (held.cleanup_returned && held.cleanup_succeeded)
			{
				held.object = nullptr;
				held.result = result::load_missed;
			}
			(void)quest_mobile_native_birth_owner::charge();
			return held.result;
		}
		if (held.flat_backend &&
		    !quest_mobile_native_birth_owner::charge(sizeof(flat_binding_scratch_guard)))
			return result::held_refusal;
		flat_binding_scratch_guard binding_scratch(held.flat_backend);
		std::vector<player_item_snapshot> literal;
		native_mobile_birth_item_recipe recipe;
		if (player_item_snapshot_tree_capture_literal(object, &literal, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    literal.size() != 1 || literal[0].object_uid != held.reserved_uid ||
		    literal[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    literal[0].equipment_slot != 0 ||
		    !held.factory.capture_recipe(literal[0], &recipe))
			return result::held_refusal;
		if (held.literal_captured)
		{
			std::vector<player_item_snapshot> first{ held.facts.literal };
			std::vector<native_mobile_birth_item_recipe> first_recipe{
				held.facts.recipe
			},
				current_recipe{ recipe };
			std::vector<uint8_t> before, current, before_recipe, now_recipe;
			if (player_item_snapshot_list_encode(first, &before) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(literal, &current) !=
				    player_snapshot_codec_result::ok ||
			    native_mobile_birth_recipe_encode(first, first_recipe,
							      &before_recipe) !=
				    economic_accounting_error::ok ||
			    native_mobile_birth_recipe_encode(literal, current_recipe,
							      &now_recipe) !=
				    economic_accounting_error::ok ||
			    before != current || before_recipe != now_recipe)
				return result::held_refusal;
		}
		else
		{
			held.facts.literal = std::move(literal[0]);
			held.facts.recipe = std::move(recipe);
			held.literal_captured = true;
		}
		if (!quest_mobile_native_birth_owner::charge())
			return result::held_refusal;
		if (!held.bindings_prepared)
		{
			if (held.flat_backend)
				held.bindings_prepared = prepare_single_flat_bindings(
					held.factory, literal, recipe, held.bindings,
					sizeof(binding_scratch) + sizeof(source) + sizeof(command) +
						sizeof(flat_lineage) + sizeof(flat_epoch));
			else
			{
				const auto binding = held.factory.binding_input();
				held.bindings_prepared =
					shop_trade_original_procedure_binding_stage::
						prepare_native_birth({ &binding, 1 },
								     held.bindings);
			}
		}
		if ((!held.flat_backend && !quest_mobile_native_birth_owner::charge()) ||
		    !held.bindings_prepared)
			return result::held_refusal;
		held.pure_pending = false;
		held.result = result::captured;
		return held.result;
	}
	catch (...)
	{
		if (output->state_)
		{
			auto &held = *output->state_;
			if ((held.operation_started && !held.operation_returned) ||
			    (held.factory_started && !held.factory_returned) ||
			    (held.load_started && !held.load_returned) ||
			    (held.cleanup_started && !held.cleanup_returned))
				held.pure_pending = false;
		}
		(void)quest_mobile_native_birth_owner::charge();
		return output->state_ ? result::held_refusal : result::refused;
	}
}

P_obj zone_reset_item_owner::object(const zone_reset_item_root_stage &stage) noexcept
{
	return nevent_is_game_thread() && stage.state_ ? stage.state_->object : nullptr;
}

bool zone_reset_item_owner::observe_root(const zone_reset_item_root_stage &stage,
					 zone_reset_item_root_facts *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !stage.state_ ||
	    stage.state_->result != zone_reset_item_root_result::captured)
		return false;
	try
	{
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_root_facts>);
		auto observed = stage.state_->facts;
		*output = std::move(observed);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_item_owner::discard_root(zone_reset_item_root_stage *stage) noexcept
{
	if (!stage || !nevent_is_game_thread())
		return false;
	if (!stage->state_)
		return true;
	if (!stage->state_->factory.discard_unadmitted())
		return false;
	stage->state_->object = nullptr;
	delete stage->state_;
	stage->state_ = nullptr;
	return true;
}

bool zone_reset_item_owner::empty(const zone_reset_item_root_stage &stage) noexcept
{
	return stage.state_ == nullptr;
}

struct zone_reset_item_child_stage::implementation
{
	quest_mobile_native_item_stage factory;
	std::unique_ptr<quest_mobile_native_flat_factory_scope> flat_scope;
	bool flat_backend = false;
	shop_trade_original_procedure_binding_stage bindings;
	zone_reset_item_child_facts facts;
	P_obj object = nullptr;
	uint64_t reserved_uid = 0; // Original allocator observation, including failures.
	zone_reset_item_child_result result = zone_reset_item_child_result::held_refusal;
	bool pure_pending = true, factory_started = false, factory_returned = false,
	     factory_succeeded = false, literal_captured = false, bindings_prepared = false;
};

zone_reset_item_child_result
zone_reset_item_owner::prepare_child(uint32_t slot, zone_reset_item_child_stage *output) noexcept
{
	using result = zone_reset_item_child_result;
	if (!output || !nevent_is_game_thread())
		return result::refused;
	economic_source_event actual{};
	int32_t zone_vnum = -1;
	const bool observed =
		output->state_ ?
			quest_mobile_native_birth_owner::capture_retained_reset_child_source(
				slot, &actual, &zone_vnum) :
			quest_mobile_native_birth_owner::capture_reset_child_source(slot, &actual,
										    &zone_vnum);
	if (!observed)
		return output->state_ ? result::held_refusal : result::refused;
	const std::string *selected_flat_root = nullptr;
	if (!quest_mobile_native_birth_owner::capture_reset_backend(actual, &selected_flat_root))
		return output->state_ ? result::held_refusal : result::refused;
	if (output->state_)
	{
		const auto &held = *output->state_;
		const bool same = held.facts.original_command_slot == slot &&
				  held.facts.zone_vnum == zone_vnum &&
				  critical_operation_id_is_zero(held.facts.scope_root_operation) &&
				  held.facts.reset_scope.source.bytes == actual.source.bytes &&
				  held.facts.reset_scope.generation.bytes ==
					  actual.generation.bytes &&
				  held.facts.reset_scope.kind == actual.kind &&
				  held.facts.reset_scope.sequence == actual.sequence &&
				  held.facts.reset_scope.slot == actual.slot;
		if (!same || held.flat_backend != (selected_flat_root != nullptr))
			return result::held_refusal;
		if (held.result != result::held_refusal || !held.pure_pending)
			return held.result;
	}
	if (!zone_table || !obj_index)
		return result::refused;
	const zone_data *zone = nullptr;
	for (int index = 0; index <= top_of_zone_table; ++index)
		if (zone_table[index].number == zone_vnum)
		{
			if (zone)
				return result::refused;
			zone = &zone_table[index];
		}
	if (!zone || !zone->cmd)
		return result::refused;
	for (uint32_t index = 0; index < slot; ++index)
		if (zone->cmd[index].command == 'S')
			return result::refused;
	const auto command = zone->cmd[slot];
	if (command.command != 'P' || command.arg1 < 0 || command.arg1 > top_of_objt ||
	    command.arg3 < 0)
		return result::refused;
	try
	{
		if (!output->state_)
		{
			auto state =
				std::make_unique<zone_reset_item_child_stage::implementation>();
			state->facts.reset_scope = actual;
			state->flat_backend = selected_flat_root != nullptr;
			state->facts.original_command_slot = slot;
			state->facts.zone_vnum = zone_vnum;
			state->facts.object_rnum = command.arg1;
			state->facts.target_rnum = command.arg3;
			state->facts.original_zone_percent = command.arg4;
			output->state_ = state.release(); // Own before UID/factory preparation.
		}
		auto &held = *output->state_;
		if (held.facts.object_rnum != command.arg1 ||
		    held.facts.target_rnum != command.arg3 ||
		    held.facts.original_zone_percent != command.arg4 ||
		    !quest_mobile_native_birth_owner::charge())
			return result::held_refusal;
		if (held.flat_backend)
		{
			if (!held.flat_scope)
			{
				const size_t chars = selected_flat_root->size();
				size_t peak = sizeof(quest_mobile_native_flat_factory_scope);
				if (chars > 15)
				{
					if (chars == SIZE_MAX || chars + 1 > SIZE_MAX - peak)
						return result::held_refusal;
					peak += chars + 1;
				}
				if (!quest_mobile_native_birth_owner::charge(peak))
					return result::held_refusal;
				held.flat_scope.reset(new quest_mobile_native_flat_factory_scope(
					*selected_flat_root, actual));
			}
			if (!held.flat_scope->current() ||
			    held.flat_scope->root_ != *selected_flat_root ||
			    !quest_mobile_native_birth_owner::charge())
				return result::held_refusal;
		}
		if (!held.reserved_uid)
		{
			const auto uid = item_uid_allocator_next();
			if (!uid)
				return result::held_refusal; // No actual UID/factory to replace.
			held.reserved_uid = uid;
			if (uid == UINT64_MAX)
			{
				held.pure_pending = false;
				return result::held_refusal;
			}
		}
		if (!held.factory_started)
		{
			if (held.flat_scope && !quest_mobile_native_birth_owner::charge(
						       held.flat_scope->retained_heap_bytes()))
				return result::held_refusal;
			held.factory_started = true;
			held.factory_succeeded =
				held.flat_scope ?
					quest_mobile_native_item_stage::prepare_retaining_flat(
						command.arg1, REAL, held.reserved_uid,
						*held.flat_scope, &held.factory) :
					quest_mobile_native_item_stage::prepare_retaining(
						command.arg1, REAL, held.reserved_uid,
						&held.factory);
			held.factory_returned = true;
			held.object = held.factory.object();
		}
		if (!held.factory_returned || !held.factory_succeeded)
		{
			held.pure_pending = false;
			(void)quest_mobile_native_birth_owner::charge();
			return result::held_refusal; // Nullable preparation is not read_object failure.
		}
		if (!quest_mobile_native_birth_owner::charge())
			return result::held_refusal;
		held.object = held.factory.object();
		if (held.factory.is_flat_factory() != held.flat_backend ||
		    (held.flat_scope &&
		     !held.factory.flat_factory_matches(held.flat_scope->root_, actual)))
			return result::held_refusal;
		P_obj object = held.object;
		if (!object || held.factory.object() != object ||
		    !held.factory.owns_pending_original_target(object) ||
		    object->obj_uid != held.reserved_uid || object->R_num != command.arg1 ||
		    object->loc_p != LOC_NOWHERE || object->loc.room != NOWHERE || object->next ||
		    object->prev || object->contains || object->next_content)
		{
			held.pure_pending = false;
			return held.result;
		}
		// Original artifact ownership is decided before target lookup. Keep its
		// actual constructor held; never invent the original owner/policy result.
		if (IS_ARTIFACT(object) || object->type == ITEM_CORPSE)
		{
			held.pure_pending = false;
			held.result = result::native_owner_required;
			return held.result;
		}
		if (held.flat_backend &&
		    !quest_mobile_native_birth_owner::charge(sizeof(flat_binding_scratch_guard)))
			return held.result;
		flat_binding_scratch_guard binding_scratch(held.flat_backend);
		std::vector<player_item_snapshot> literal;
		native_mobile_birth_item_recipe recipe;
		if (player_item_snapshot_tree_capture_literal(object, &literal, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    literal.size() != 1 || literal[0].object_uid != held.reserved_uid ||
		    literal[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    literal[0].equipment_slot != 0 ||
		    !held.factory.capture_recipe(literal[0], &recipe))
			return held.result;
		if (held.literal_captured)
		{
			std::vector<player_item_snapshot> first{ held.facts.literal };
			std::vector<native_mobile_birth_item_recipe> first_recipe{
				held.facts.recipe
			},
				current_recipe{ recipe };
			std::vector<uint8_t> before, current, before_recipe, now_recipe;
			if (player_item_snapshot_list_encode(first, &before) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(literal, &current) !=
				    player_snapshot_codec_result::ok ||
			    native_mobile_birth_recipe_encode(first, first_recipe,
							      &before_recipe) !=
				    economic_accounting_error::ok ||
			    native_mobile_birth_recipe_encode(literal, current_recipe,
							      &now_recipe) !=
				    economic_accounting_error::ok ||
			    before != current || before_recipe != now_recipe)
				return held.result;
		}
		else
		{
			held.facts.literal = std::move(literal[0]);
			held.facts.recipe = std::move(recipe);
			held.literal_captured = true;
		}
		if (!quest_mobile_native_birth_owner::charge())
			return held.result;
		if (!held.bindings_prepared)
		{
			if (held.flat_backend)
				held.bindings_prepared = prepare_single_flat_bindings(
					held.factory, literal, recipe, held.bindings,
					sizeof(binding_scratch) + sizeof(actual) + sizeof(command));
			else
			{
				const auto binding = held.factory.binding_input();
				held.bindings_prepared =
					shop_trade_original_procedure_binding_stage::
						prepare_native_birth({ &binding, 1 },
								     held.bindings);
			}
		}
		if ((!held.flat_backend && !quest_mobile_native_birth_owner::charge()) ||
		    !held.bindings_prepared)
			return held.result;
		held.pure_pending = false;
		// Do not call itemvalue or ITEM_LOAD_CHECK: original P target/respawn
		// decisions must happen first. No parent link, source claim or admission.
		held.result = result::constructed;
		return held.result;
	}
	catch (...)
	{
		if (output->state_ && output->state_->factory_started &&
		    !output->state_->factory_returned)
			output->state_->pure_pending = false;
		(void)quest_mobile_native_birth_owner::charge();
		// The handle already owns any genuine factory reached before failure.
		// Do not lose it or reconstruct this actual P slot on a retry.
		return output->state_ ? result::held_refusal : result::refused;
	}
}

P_obj zone_reset_item_owner::object(const zone_reset_item_child_stage &stage) noexcept
{
	return nevent_is_game_thread() && stage.state_ ? stage.state_->object : nullptr;
}

bool zone_reset_item_owner::observe_child(const zone_reset_item_child_stage &stage,
					  zone_reset_item_child_facts *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !stage.state_ ||
	    stage.state_->result != zone_reset_item_child_result::constructed)
		return false;
	try
	{
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_child_facts>);
		auto observed = stage.state_->facts;
		*output = std::move(observed);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_item_owner::discard_child(zone_reset_item_child_stage *stage) noexcept
{
	if (!stage || !nevent_is_game_thread())
		return false;
	if (!stage->state_)
		return true;
	if (!stage->state_->factory.discard_unadmitted())
		return false;
	stage->state_->object = nullptr;
	delete stage->state_;
	stage->state_ = nullptr;
	return true;
}

bool zone_reset_item_owner::empty(const zone_reset_item_child_stage &stage) noexcept
{
	return stage.state_ == nullptr;
}

bool zone_reset_item_owner::factory_backend_current(
	const quest_mobile_native_item_stage &factory,
	const quest_mobile_native_flat_factory_scope *scope,
	const economic_source_event &source) noexcept
{
	if (!scope)
		return !factory.is_flat_factory();
	return scope->current() && factory.flat_factory_matches(scope->root_, source);
}

struct zone_reset_item_owner::warm_child
{
	zone_reset_item_child_stage stage;
	uint32_t slot = 0;
	zone_reset_item_warm_result result = zone_reset_item_warm_result::held_refusal;
	critical_operation_id selected_root_operation{};
	P_obj target = nullptr;
	uint64_t target_uid = 0;
	bool target_returned = false, target_pending = false;
	int observed_itemvalue = 0;
	bool load_started = false, load_returned = false, load_passed = false;
	bool discard_started = false, discard_returned = false, discard_succeeded = false;
	bool nest_started = false, nest_returned = false, nest_succeeded = false;
	bool retired = false;
};
struct zone_reset_item_owner::warm_checkpoint
{
	critical_native_recovery_envelope expected, successor;
	zone_reset_item_recovery_context context;
};
struct zone_reset_item_owner::warm_root
{
	// Prospective allowance belongs to the original pulse's actual stack guard,
	// not to a copied envelope, caller DTO or detached reservation token.
	warm_command_scratch *preparation_owner = nullptr;
	size_t preparation_scratch = 0;
	zone_reset_item_root_stage stage;
	std::vector<std::unique_ptr<warm_child>> children;
	shop_trade_original_procedure_binding_stage whole_bindings;
	zone_reset_item_warm_forest_facts forest;
	// The command and ZRR1 initial body belong to these same real factories.
	// Empty means no original SQL preparation has returned; never rebind a
	// retained command to a later season, epoch or room revision on retry.
	critical_native_recovery_envelope original_envelope;
	std::vector<uint8_t> canonical_command;
	zone_reset_original_room_placement_stage placement;
	bool placement_captured = false;
	zone_reset_item_recovery_context context;
	std::unique_ptr<warm_checkpoint> checkpoint;
	std::array<std::unique_ptr<warm_checkpoint>, 4> returned_writers;
	std::unique_ptr<critical_native_recovery_envelope> ack_successor;
	zone_reset_room_publication_stage publication;
	std::unordered_set<uint64_t> published;
	std::vector<quest_mobile_native_item_stage *> refusal_factories;
	bool refusal_cleanup_started = false, refusal_cleanup_returned = false;
	critical_completion completion{};
	uint64_t coordinator_generation = 0;
	size_t pending_row = 0, pending_step = 0;
	uint8_t pending_action = 0;
	bool action_not_attempted = false, submitted = false, completed = false;
	bool cold = false, blocked = false, retired = false;
	bool cold_bindings_prepared = false, cold_binding_restored = false;
	uint32_t slot = 0;
	bool sealed = false, forest_captured = false;
};
struct zone_reset_item_owner::warm_registry
{
	warm_registry *next = nullptr;
	economic_source_event invocation{};
	int32_t zone_vnum = -1;
	std::vector<std::unique_ptr<warm_root>> roots;
	std::vector<std::unique_ptr<warm_child>> unselected;
	uint32_t last_capture_slot = 0, stop_slot = 0;
	int original_last_cmd = 0, original_force = 0;
	bool has_capture = false, closed = false, blocked = false, dispatcher_completed = false;
	bool sealing_pending = false;
};
zone_reset_item_owner::warm_registry *zone_reset_item_owner::warm_head_ = nullptr;
zone_reset_item_owner::warm_registry *zone_reset_item_owner::warm_current_ = nullptr;

bool zone_reset_item_owner::warm_bindings_current(const warm_root &root) noexcept
{
	if (!root.stage.state_)
		return false;
	const auto &body = *root.stage.state_;
	if (body.flat_backend != bool(body.flat_scope) ||
	    !factory_backend_current(body.factory, body.flat_scope.get(), body.facts.reset_source))
		return false;
	return body.flat_backend ? root.whole_bindings.valid_flat() : root.whole_bindings.valid();
}

struct zone_reset_item_owner::warm_command_scratch
{
	warm_root *root = nullptr;
	critical_native_recovery_envelope *output = nullptr;
	bool global_scope = false;
	bool literal_pool_scope = false;
	size_t current_bytes() const noexcept;
	static size_t inline_bytes() noexcept
	{
		return sizeof(warm_command_scratch) + sizeof(critical_native_recovery_envelope) +
		       economic_gameplay_authority::active_regular_flat_working_bytes();
	}
	explicit warm_command_scratch(warm_root *original,
				      bool includes_literal_pool = false) noexcept
		: root(original)
		, literal_pool_scope(includes_literal_pool)
	{
		if (root)
		{
			root->preparation_owner = this; // begin admitted this object before construction.
			if (!(literal_pool_scope ? begin_full_flat_command_scope(*this) :
						   begin_flat_command_scope(*this)))
				root = nullptr;
		}
	}
	~warm_command_scratch() { release_warm_command_scratch(*this); }
	warm_command_scratch(const warm_command_scratch &) = delete;
	warm_command_scratch &operator=(const warm_command_scratch &) = delete;
};

namespace
{
bool warm_scratch_add(size_t &bytes, size_t amount) noexcept
{
	if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
		return false;
	bytes += amount;
	return true;
}
bool warm_scratch_array(size_t &bytes, size_t count, size_t unit) noexcept
{
	return (!unit || count <= PLAYER_SAVE_PIPELINE_MAX_BYTES / unit) &&
	       warm_scratch_add(bytes, count * unit);
}
bool warm_scratch_envelope_heap(const critical_native_recovery_envelope &value,
				       bool fresh_copy, size_t *output) noexcept
{
	size_t bytes = 0;
	const auto count = [fresh_copy](const auto &v)
	{ return fresh_copy ? v.size() : v.capacity(); };
	if (!warm_scratch_array(bytes, count(value.command.keys), sizeof(critical_entity_key)) ||
	    !warm_scratch_array(bytes, count(value.command.expected_revisions),
				sizeof(critical_expected_revision)) ||
	    !warm_scratch_add(bytes, count(value.command.payload)) ||
	    !warm_scratch_add(bytes, count(value.command.accounting_intent)) ||
	    !warm_scratch_add(bytes, count(value.attachment)))
		return false;
	*output = bytes;
	return true;
}
bool warm_scratch_forest_heap(const std::vector<player_item_snapshot> &items,
	const std::vector<native_mobile_birth_item_recipe> &recipes,
	const std::vector<zone_reset_coin_output> &coins, bool fresh_copy, size_t *output) noexcept
{
	size_t bytes = 0;
	const auto count = [fresh_copy](const auto &v)
	{ return fresh_copy ? v.size() : v.capacity(); };
	const auto text = [&](const std::string &value)
	{
		const size_t chars = count(value);
		return chars <= 15 || (chars != SIZE_MAX && warm_scratch_add(bytes, chars + 1));
	};
	if (!warm_scratch_array(bytes, count(items), sizeof(player_item_snapshot)) ||
	    !warm_scratch_array(bytes, count(recipes), sizeof(native_mobile_birth_item_recipe)) ||
	    !warm_scratch_array(bytes, count(coins), sizeof(zone_reset_coin_output)))
		return false;
	for (const auto &item : items)
	{
		if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description) ||
		    !warm_scratch_array(bytes, count(item.dynamic_affects),
				    sizeof(player_item_dynamic_affect_snapshot)) ||
		    !warm_scratch_array(bytes, count(item.extra_descriptions),
				    sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : item.extra_descriptions)
			if (!text(description.keyword) || !text(description.description) ||
			    !warm_scratch_array(bytes, count(description.spell_ids), sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : recipes)
		if (!warm_scratch_array(bytes, count(recipe.libraries),
				    sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}
} // namespace

bool zone_reset_item_owner::reserve_flat_binding_scratch(size_t peak, void *) noexcept
{
	return quest_mobile_native_birth_owner::charge(peak);
}

bool zone_reset_item_owner::prepare_single_flat_bindings(
	const quest_mobile_native_item_stage &factory,
	const std::vector<player_item_snapshot> &literal,
	const native_mobile_birth_item_recipe &recipe,
	shop_trade_original_procedure_binding_stage &output, size_t additional_inline) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)factory;
	(void)literal;
	(void)recipe;
	(void)output;
	(void)additional_inline;
	return false;
#else
	if (!factory.is_flat_factory())
		return false;
	// These are the original producer's still-live capture temporaries, including
	// their actual capacities after the first literal/recipe move or a retry.
	size_t outer = additional_inline;
	if (!warm_scratch_add(outer, sizeof(literal)) || !warm_scratch_add(outer, sizeof(recipe)) ||
	    !warm_scratch_add(outer, sizeof(quest_mobile_native_item_binding)) ||
	    !warm_scratch_add(outer, sizeof(std::span<const quest_mobile_native_item_binding>)) ||
	    !warm_scratch_array(outer, literal.capacity(), sizeof(player_item_snapshot)) ||
	    !warm_scratch_array(outer, recipe.libraries.capacity(),
				sizeof(native_mobile_birth_library_recipe)))
		return false;
	{
		// This allocation-free census closure dies before the binding frame.
		const auto text = [&](const std::string &value) noexcept
		{
			return value.capacity() <= 15 ||
			       (value.capacity() != SIZE_MAX &&
				warm_scratch_add(outer, value.capacity() + 1));
		};
		for (const auto &item : literal)
		{
			if (!text(item.name) || !text(item.short_description) ||
			    !text(item.description) || !text(item.action_description) ||
			    !warm_scratch_array(outer, item.dynamic_affects.capacity(),
						sizeof(player_item_dynamic_affect_snapshot)) ||
			    !warm_scratch_array(outer, item.extra_descriptions.capacity(),
						sizeof(player_item_extra_description_snapshot)))
				return false;
			for (const auto &description : item.extra_descriptions)
				if (!text(description.keyword) || !text(description.description) ||
				    !warm_scratch_array(outer, description.spell_ids.capacity(),
							sizeof(int32_t)))
					return false;
		}
	}
	if (!reserve_flat_binding_scratch(outer, nullptr))
		return false;
	const auto binding = factory.binding_input();
	const std::span<const quest_mobile_native_item_binding> originals{ &binding, 1 };
	// The original caller's guard recensuses only after its real inputs die.
	return shop_trade_original_procedure_binding_stage::prepare_native_birth_flat_bounded(
		originals, output, reserve_flat_binding_scratch, nullptr, outer);
#endif
}

bool zone_reset_item_owner::begin_warm_command_scratch(warm_root &root) noexcept
{
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI
	(void)root;
	return false;
#else
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() || root.preparation_owner ||
	    root.preparation_scratch || root.cold || root.blocked || root.retired ||
	    !root.sealed || !root.stage.state_ ||
	    root.stage.state_->result != zone_reset_item_root_result::captured)
		return false;
	// Allocation-free membership/source check. Original current-world/factory
	// checks still run in preparation; this registration grants no execution.
	warm_registry *actual = nullptr;
	for (auto *registry = warm_head_; registry; registry = registry->next)
		for (const auto &owned : registry->roots)
			if (owned.get() == &root)
			{
				if (actual || !registry->closed || !registry->dispatcher_completed ||
				    registry->blocked || registry->sealing_pending)
					return false;
				actual = registry;
			}
	const auto &source = root.forest.reset_source;
	if (!actual || source.source.bytes != actual->invocation.source.bytes ||
	    source.generation.bytes != actual->invocation.generation.bytes ||
	    source.kind != actual->invocation.kind || source.sequence != actual->invocation.sequence ||
	    source.slot != root.slot || root.slot >= actual->stop_slot ||
	    root.forest.zone_vnum != actual->zone_vnum ||
	    root.forest.operation_id.bytes != root.stage.state_->facts.operation_id.bytes)
		return false;
	root.preparation_scratch = warm_command_scratch::inline_bytes();
	if (!quest_mobile_native_birth_owner::charge())
	{
		root.preparation_scratch = 0; // Failed admission never updates the budget cache.
		return false;
	}
	// The original selected-projection predicate has its own named shared_ptr.
	// The persistent base above admits it before even an inactive-state refusal.
	if (!economic_gameplay_authority::active_regular_flat())
	{
		root.preparation_scratch = 0;
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
	return true;
#endif
}

bool zone_reset_item_owner::rebase_warm_command_scratch(warm_command_scratch &scratch,
						       size_t bytes) noexcept
{
	if (!nevent_is_game_thread() || !scratch.root || !scratch.output ||
	    scratch.root->preparation_owner != &scratch ||
	    bytes < warm_command_scratch::inline_bytes() ||
	    bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	const size_t previous = scratch.root->preparation_scratch;
	scratch.root->preparation_scratch = bytes;
	if (!quest_mobile_native_birth_owner::charge())
	{
		scratch.root->preparation_scratch = previous;
		return false;
	}
	return true;
}

bool zone_reset_item_owner::reserve_warm_command_scratch(size_t bytes, void *context) noexcept
{
	auto *scratch = static_cast<warm_command_scratch *>(context);
	if (!scratch || !scratch->root || persistence_mode_requires_mysql() ||
	    !economic_gameplay_authority::active_regular_flat())
		return false;
	size_t exclusive = 0;
	// Only the genuine same-lock lender can authenticate borrowed coordinator
	// bytes. Keep those CURRENT bytes out of retained ROOT high-water scratch.
	// Without an actual borrow the complete original prefix is unchanged.
	if (!item_native_quest_coordinator_budget_scope_owner::exclusive_prefix(
		    scratch, reserve_warm_command_scratch, bytes, &exclusive))
		return false;
	return rebase_warm_command_scratch(*scratch,
					   std::max(exclusive, scratch->root->preparation_scratch));
}

bool zone_reset_item_owner::retain_warm_command_output(warm_command_scratch &scratch,
	const critical_native_recovery_envelope &output) noexcept
{
	if (scratch.output != &output)
		return false;
	size_t bytes = scratch.current_bytes(), heap = 0;
	return warm_scratch_envelope_heap(output, false, &heap) &&
	       warm_scratch_add(bytes, heap) && rebase_warm_command_scratch(scratch, bytes);
}

void zone_reset_item_owner::release_warm_command_scratch(warm_command_scratch &scratch) noexcept
{
	if (!nevent_is_game_thread() || !scratch.root ||
	    scratch.root->preparation_owner != &scratch)
		return;
	// Called after the pulse output and factory-vector temporaries die, before
	// the original registry-erasure loop. Failed recensus retains the older cache.
	scratch.root->preparation_owner = nullptr;
	scratch.root->preparation_scratch = 0;
	if (scratch.global_scope)
	{
		// No callback/allocation between scalar root removal and exact scope end.
		(void)item_native_quest_global_budget_scope_owner::end(&scratch);
		scratch.global_scope = false;
	}
	(void)quest_mobile_native_birth_owner::charge();
	scratch.root = nullptr;
	scratch.output = nullptr;
}

bool zone_reset_item_owner::begin_warm_capture() noexcept
{
	if (!nevent_is_game_thread())
		return false;
	economic_source_event source{};
	int32_t zone = -1;
	int force = 0;
	if (!quest_mobile_native_birth_owner::capture_reset_dispatch_scope(&source, &zone, &force))
		return false;
	size_t count = 0;
	for (auto *held = warm_head_; held; held = held->next)
	{
		++count;
		if (held->invocation.source.bytes == source.source.bytes &&
		    held->invocation.generation.bytes == source.generation.bytes &&
		    held->invocation.kind == source.kind && held->zone_vnum == zone)
		{
			if (held->closed || held->blocked)
				return false;
			warm_current_ = held;
			return quest_mobile_native_birth_owner::charge();
		}
	}
	size_t retained = 0;
	if (count >= CRITICAL_COORDINATOR_MAX_OPERATIONS || !warm_retained_size(&retained))
		return false;
	try
	{
		auto candidate = std::make_unique<warm_registry>();
		candidate->invocation = source;
		candidate->zone_vnum = zone;
		candidate->original_force = force;
		candidate->next = warm_head_;
		warm_current_ = candidate.release();
		warm_head_ = warm_current_; // Own the whole invocation before factories.
		if (!quest_mobile_native_birth_owner::charge())
			return false; // The genuine empty registry is rooted; no factory has run.
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_item_owner::warm_scope_current(const warm_registry &held) noexcept
{
	if (!nevent_is_game_thread() || held.closed)
		return false;
	auto *slow = warm_head_, *fast = warm_head_;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	bool registered = false;
	for (auto *entry = warm_head_; entry; entry = entry->next)
		if (entry == &held)
			registered = true;
	if (!registered)
		return false;
	// Prove an already existing nonce before the dispatch observer, whose
	// original constructor cut may otherwise lazily establish a new invocation.
	economic_source_event source{};
	int32_t zone = -1;
	uint32_t slot = 0;
	int last_cmd = 0, force = 0;
	if (!quest_mobile_native_birth_owner::capture_reset_abort_scope(&source, &zone, &slot,
									&last_cmd) ||
	    source.source.bytes != held.invocation.source.bytes ||
	    source.generation.bytes != held.invocation.generation.bytes ||
	    source.kind != held.invocation.kind || source.sequence != held.invocation.sequence ||
	    zone != held.zone_vnum)
		return false;
	return quest_mobile_native_birth_owner::capture_reset_dispatch_scope(&source, &zone,
									     &force) &&
	       force == held.original_force &&
	       source.source.bytes == held.invocation.source.bytes &&
	       source.generation.bytes == held.invocation.generation.bytes &&
	       source.kind == held.invocation.kind && source.sequence == held.invocation.sequence &&
	       zone == held.zone_vnum;
}
// These observations run only inside the genuine original open O/P cursor.
// They neither clear a held phase nor turn preparation refusal into read_object failure.
bool zone_reset_item_owner::warm_capture_retryable(uint32_t slot) noexcept
{
	if (!warm_current_ || warm_current_->blocked || !warm_scope_current(*warm_current_) ||
	    !zone_table)
		return false;
	const auto &registry = *warm_current_;
	economic_source_event current{};
	int32_t zone_vnum = -1;
	uint32_t actual_slot = 0;
	int last_cmd = 0;
	if (!quest_mobile_native_birth_owner::capture_reset_abort_scope(&current, &zone_vnum,
									&actual_slot, &last_cmd) ||
	    actual_slot != slot)
		return false;
	const zone_data *zone = nullptr;
	for (int i = 0; i <= top_of_zone_table; ++i)
		if (zone_table[i].number == zone_vnum)
		{
			if (zone)
				return false;
			zone = &zone_table[i];
		}
	if (!zone || !zone->cmd)
		return false;
	const auto &command = zone->cmd[slot];
	economic_source_event source{};
	int32_t observed_zone = -1;
	if (!(command.command == 'O' ?
		      quest_mobile_native_birth_owner::capture_room_reset_source(
			      slot, command.arg3, &source, &observed_zone) :
		      command.command == 'P' &&
			      quest_mobile_native_birth_owner::capture_reset_child_source(
				      slot, &source, &observed_zone)) ||
	    observed_zone != registry.zone_vnum ||
	    source.source.bytes != registry.invocation.source.bytes ||
	    source.generation.bytes != registry.invocation.generation.bytes ||
	    source.kind != registry.invocation.kind ||
	    source.sequence != registry.invocation.sequence)
		return false;
	const auto factory_ready = [](const quest_mobile_native_item_stage &factory)
	{
		quest_mobile_native_item_progress progress{};
		return factory.read_progress(&progress) && !progress.admitted &&
		       !progress.published && !progress.current_step_started &&
		       !progress.next_step && factory.object() &&
		       factory.owns_pending_original_target(factory.object());
	};
	size_t matches = 0;
	bool retryable = false;
	for (const auto &root : registry.roots)
	{
		if (!root || root->cold || root->blocked || root->retired || root->submitted ||
		    root->completed || root->checkpoint || root->action_not_attempted ||
		    root->refusal_cleanup_started)
			return false;
		if (root->slot != slot)
			continue;
		if (command.command != 'O' || ++matches != 1)
			return false;
		const auto *body = root->stage.state_;
		if (!body)
		{
			retryable = true;
			continue;
		}
		if (body->slot != slot || body->room != command.arg3 ||
		    body->facts.object_rnum != command.arg1 ||
		    body->original_percent != command.arg4 ||
		    body->facts.reset_source.source.bytes != source.source.bytes ||
		    body->facts.reset_source.generation.bytes != source.generation.bytes ||
		    body->facts.reset_source.kind != source.kind ||
		    body->facts.reset_source.sequence != source.sequence ||
		    body->facts.reset_source.slot != slot ||
		    body->facts.zone_vnum != observed_zone ||
		    (body->operation_started && !body->operation_returned) ||
		    (body->factory_started &&
		     (!body->factory_returned || !body->factory_succeeded)) ||
		    (body->load_started && !body->load_returned) ||
		    (body->cleanup_started &&
		     (!body->cleanup_returned || !body->cleanup_succeeded)))
			return false;
		if (body->result == zone_reset_item_root_result::load_missed)
			retryable = body->cleanup_succeeded && !body->object &&
				    !body->factory.object();
		else
			retryable = (body->pure_pending ||
				     body->result == zone_reset_item_root_result::captured) &&
				    (!body->factory_started || factory_ready(body->factory));
	}
	const auto child_ready = [&](const warm_child &child)
	{
		if (child.retired || (child.load_started && !child.load_returned) ||
		    (child.discard_started &&
		     (!child.discard_returned || !child.discard_succeeded)) ||
		    (child.nest_started && (!child.nest_returned || !child.nest_succeeded)))
			return false;
		const auto *body = child.stage.state_;
		if (!body)
			return child.result == zone_reset_item_warm_result::held_refusal;
		if (body->facts.original_command_slot != slot ||
		    body->facts.object_rnum != command.arg1 ||
		    body->facts.target_rnum != command.arg3 ||
		    body->facts.original_zone_percent != command.arg4 ||
		    body->facts.zone_vnum != observed_zone ||
		    body->facts.reset_scope.source.bytes != source.source.bytes ||
		    body->facts.reset_scope.generation.bytes != source.generation.bytes ||
		    body->facts.reset_scope.kind != source.kind ||
		    body->facts.reset_scope.sequence != source.sequence ||
		    body->facts.reset_scope.slot != slot ||
		    (body->factory_started &&
		     (!body->factory_returned || !body->factory_succeeded)))
			return false;
		if (child.result == zone_reset_item_warm_result::load_missed)
			return child.load_returned && !child.load_passed &&
			       child.discard_returned && child.discard_succeeded && !body->object &&
			       !body->factory.object();
		if (!child.target_returned &&
		    (child.target || child.target_uid || child.target_pending))
			return false;
		if (child.target_returned &&
		    ((!child.target && (child.target_uid || child.target_pending)) ||
		     (child.target &&
		      (!child.target_pending ||
		       !warm_object_current_impl(child.target, child.target_uid, false)))))
			return false;
		if (child.result != zone_reset_item_warm_result::captured &&
		    (child.load_started || child.discard_started || child.nest_started))
			return false; // A known-pure hold never retries a started native leg.
		return (body->pure_pending ||
			body->result == zone_reset_item_child_result::constructed) &&
		       (child.result == zone_reset_item_warm_result::held_refusal ||
			child.result == zone_reset_item_warm_result::constructed ||
			child.result == zone_reset_item_warm_result::captured) &&
		       (!body->factory_started || factory_ready(body->factory));
	};
	for (const auto &child : registry.unselected)
	{
		if (!child)
			return false;
		if (child->slot == slot)
		{
			if (command.command != 'P' || ++matches != 1)
				return false;
			retryable = child_ready(*child);
		}
	}
	for (const auto &root : registry.roots)
		for (const auto &child : root->children)
		{
			if (!child)
				return false;
			if (child->slot == slot)
			{
				if (command.command != 'P' || ++matches != 1)
					return false;
				retryable = child_ready(*child);
			}
		}
	// A genuine unstarted slot is retryable; an older missing registration is not.
	return matches ? retryable : (!registry.has_capture || slot > registry.last_capture_slot);
}

bool zone_reset_item_owner::warm_object_current(P_obj actual, uint64_t uid) noexcept
{
	return warm_object_current_impl(actual, uid, true);
}
bool zone_reset_item_owner::warm_object_current_impl(P_obj actual, uint64_t uid,
						     bool require_retryable) noexcept
{
	if (!actual || !uid || uid == UINT64_MAX || !warm_current_ ||
	    !warm_scope_current(*warm_current_))
		return false;
	economic_source_event source{};
	int32_t zone = -1;
	uint32_t slot = 0;
	int last_cmd = 0;
	if (!quest_mobile_native_birth_owner::capture_reset_abort_scope(&source, &zone, &slot,
									&last_cmd) ||
	    (require_retryable && !warm_capture_retryable(slot)))
		return false;
	if (!require_retryable)
	{
		// Only an authentic currently opened P cut can borrow this internal
		// frozen-target observation; it is not generic world-body authority.
		economic_source_event actual{};
		int32_t actual_zone = -1;
		if (!quest_mobile_native_birth_owner::capture_reset_child_source(slot, &actual,
										 &actual_zone) ||
		    actual_zone != zone || actual.source.bytes != source.source.bytes ||
		    actual.generation.bytes != source.generation.bytes ||
		    actual.kind != source.kind || actual.sequence != source.sequence ||
		    actual.slot != slot)
			return false;
	}
	const zone_data *actual_zone = nullptr;
	for (int i = 0; i <= top_of_zone_table; ++i)
		if (zone_table[i].number == zone)
		{
			if (actual_zone)
				return false;
			actual_zone = &zone_table[i];
		}
	if (!actual_zone || !actual_zone->cmd)
		return false;
	size_t matches = 0;
	const auto current = [&](const quest_mobile_native_item_stage &factory, P_obj body,
				 uint64_t reserved_uid)
	{
		if (body != actual)
			return true;
		if (++matches != 1 || reserved_uid != uid || factory.object() != actual ||
		    !factory.owns_pending_original_target(actual))
			return false;
		quest_mobile_native_item_progress progress{};
		return factory.read_progress(&progress) && !progress.admitted &&
		       !progress.published && !progress.current_step_started &&
		       !progress.next_step && actual->obj_uid == uid;
	};
	const auto child_current = [&](const warm_child &child)
	{
		const auto *body = child.stage.state_;
		if (!body)
			return true;
		if (body->reserved_uid == uid && body->object != actual)
			return false;
		if (body->object != actual)
			return true;
		economic_source_event original{};
		int32_t original_zone = -1;
		return !child.retired && !(child.load_started && !child.load_returned) &&
		       body->factory_returned && body->factory_succeeded &&
		       quest_mobile_native_birth_owner::capture_retained_reset_child_source(
			       child.slot, &original, &original_zone) &&
		       original_zone == zone && body->facts.zone_vnum == original_zone &&
		       body->facts.original_command_slot == child.slot &&
		       actual_zone->cmd[child.slot].arg1 == body->facts.object_rnum &&
		       actual_zone->cmd[child.slot].arg3 == body->facts.target_rnum &&
		       actual_zone->cmd[child.slot].arg4 == body->facts.original_zone_percent &&
		       original.source.bytes == body->facts.reset_scope.source.bytes &&
		       original.generation.bytes == body->facts.reset_scope.generation.bytes &&
		       original.kind == body->facts.reset_scope.kind &&
		       original.sequence == body->facts.reset_scope.sequence &&
		       original.slot == body->facts.reset_scope.slot &&
		       current(body->factory, body->object, body->reserved_uid);
	};
	for (const auto &root : warm_current_->roots)
	{
		const auto *body = root->stage.state_;
		if (body && body->reserved_uid == uid && body->object != actual)
			return false;
		if (body && body->object == actual)
		{
			economic_source_event original{};
			int32_t original_zone = -1;
			if (!body->factory_returned || !body->factory_succeeded ||
			    body->cleanup_started || (body->load_started && !body->load_returned) ||
			    !quest_mobile_native_birth_owner::capture_retained_room_reset_source(
				    root->slot, body->room, &original, &original_zone) ||
			    original_zone != zone || body->facts.zone_vnum != original_zone ||
			    body->slot != root->slot ||
			    actual_zone->cmd[root->slot].arg1 != body->facts.object_rnum ||
			    actual_zone->cmd[root->slot].arg3 != body->room ||
			    actual_zone->cmd[root->slot].arg4 != body->original_percent ||
			    body->facts.room_vnum != world[body->room].number ||
			    original.source.bytes != body->facts.reset_source.source.bytes ||
			    original.generation.bytes !=
				    body->facts.reset_source.generation.bytes ||
			    original.kind != body->facts.reset_source.kind ||
			    original.sequence != body->facts.reset_source.sequence ||
			    original.slot != body->facts.reset_source.slot ||
			    !current(body->factory, body->object, body->reserved_uid))
				return false;
		}
		for (const auto &child : root->children)
			if (!child_current(*child))
				return false;
	}
	for (const auto &child : warm_current_->unselected)
		if (!child_current(*child))
			return false;
	return matches == 1;
}

bool zone_reset_item_owner::original_room_incumbent(uint32_t slot, int room, P_obj *output,
						    uint64_t *uid) noexcept
{
	if (!output || !uid || !world || room < 0 || room > top_of_world ||
	    !warm_capture_retryable(slot))
		return false;
	economic_source_event source{};
	int32_t zone_vnum = -1;
	if (!quest_mobile_native_birth_owner::capture_room_reset_source(slot, room, &source,
									&zone_vnum))
		return false;
	const zone_data *zone = nullptr;
	for (int i = 0; i <= top_of_zone_table; ++i)
		if (zone_table[i].number == zone_vnum)
		{
			if (zone)
				return false;
			zone = &zone_table[i];
		}
	if (!zone || !zone->cmd)
		return false;
	const int rnum = zone->cmd[slot].arg1;
	if (rnum < 0 || rnum > top_of_objt)
		return false;
	P_obj candidate = nullptr;
	uint32_t candidate_slot = 0;
	for (const auto &root : warm_current_->roots)
	{
		if (root->slot >= slot)
			continue; // This slot's own body is never its incumbent.
		const auto *body = root->stage.state_;
		if (!body)
			return false; // A genuine earlier unfinished factory is not absence.
		if (body->room != room || body->facts.object_rnum != rnum)
			continue;
		if (body->result == zone_reset_item_root_result::load_missed)
		{
			if (!body->cleanup_succeeded || body->object || body->factory.object())
				return false;
			continue;
		}
		zone_reset_room_placement_recipe recipe{};
		if (body->result != zone_reset_item_root_result::captured ||
		    !root->placement_captured ||
		    !warm_object_current(body->object, body->reserved_uid) ||
		    !root->placement.matches_source(body->facts.reset_source) ||
		    !root->placement.recipe(&recipe) || recipe.fall_selected ||
		    recipe.root_uid != body->reserved_uid ||
		    recipe.room_vnum != world[room].number ||
		    recipe.original_sector_type != world[room].sector_type ||
		    recipe.original_chance_fall != world[room].chance_fall ||
		    recipe.original_z_cord != body->object->z_cord ||
		    recipe.original_levitates !=
			    bool(IS_SET(body->object->extra_flags, ITEM_LEVITATES)) ||
		    !OBJ_NOWHERE(body->object) || IS_WATER_ROOM(room) ||
		    IS_SET(body->object->extra_flags, ITEM_TRANSIENT) ||
		    IS_ARTIFACT(body->object) || body->object->type == ITEM_CORPSE)
			return false;
		// Genuine original obj_to_room inserts before the first same-Rnum floor item.
		// This observes that original decision without physically placing the body.
		if (!candidate || root->slot > candidate_slot)
		{
			candidate = body->object;
			candidate_slot = root->slot;
		}
	}
	if (candidate)
	{
		*output = candidate;
		*uid = candidate->obj_uid;
		return true;
	}
	// Observe the actual current indexed list, not a UID/VNUM replacement. The
	// serialized native list establishes live membership before reading room links.
	P_obj slow = object_list, fast = object_list;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	size_t live_count = 0;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (live_count == SIZE_MAX)
			return false;
		++live_count;
	}
	size_t floor_count = 0;
	for (P_obj object = world[room].contents; object; object = object->next_content)
	{
		if (floor_count == SIZE_MAX || ++floor_count > live_count)
			return false; // Includes any room-list cycle.
		bool indexed = false;
		for (P_obj live = object_list; live; live = live->next)
			if (live == object)
			{
				indexed = true;
				break;
			}
		if (!indexed || !OBJ_ROOM(object) || object->loc.room != room || !object->obj_uid ||
		    object->obj_uid == UINT64_MAX)
			return false;
		if (object->R_num == rnum && !candidate)
			candidate = object;
	}
	*output = candidate;
	*uid = candidate ? candidate->obj_uid : 0;
	return true;
}

zone_reset_item_root_result zone_reset_item_owner::capture_warm_root(uint32_t slot, int room,
								     P_obj *output) noexcept
{
	using result = zone_reset_item_root_result;
	if (!output || !begin_warm_capture())
		return result::refused;
	auto &registry = *warm_current_;
	for (const auto &root : registry.roots)
		if (root->slot == slot)
		{
			const auto observed = prepare_root(slot, room, &root->stage);
			if (!quest_mobile_native_birth_owner::charge())
				return result::
					held_refusal; // No placement decision enters after refusal.
			if (observed == result::captured && !root->placement_captured)
			{
				root->placement_captured = true;
				if (!zone_reset_original_room_placement_stage::capture(
					    object(root->stage), room,
					    root->stage.state_->facts.reset_source,
					    &root->placement))
				{
					registry.blocked = true;
					(void)quest_mobile_native_birth_owner::charge();
					return result::held_refusal;
				}
			}
			if (observed == result::captured)
				*output = object(root->stage);
			return observed;
		}
	if ((registry.has_capture && slot <= registry.last_capture_slot) ||
	    registry.roots.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
		return result::held_refusal;
	try
	{
		auto owned = std::make_unique<warm_root>();
		owned->slot = slot;
		registry.roots.reserve(registry.roots.size() + 1);
		auto *root = owned.get();
		registry.roots.push_back(std::move(owned)); // Before constructor/choice.
		registry.last_capture_slot = slot;
		registry.has_capture = true;
		const auto observed = prepare_root(slot, room, &root->stage);
		size_t retained = 0;
		if (!warm_retained_size(&retained) || !quest_mobile_native_birth_owner::charge())
		{
			if (root->stage.state_ &&
			    !(root->stage.state_->pure_pending || observed == result::captured ||
			      observed == result::load_missed))
				registry.blocked = true;
			return result::held_refusal;
		}
		if (observed == result::captured && !root->placement_captured)
		{
			root->placement_captured = true;
			if (!zone_reset_original_room_placement_stage::capture(
				    object(root->stage), room,
				    root->stage.state_->facts.reset_source, &root->placement))
			{
				registry.blocked = true;
				(void)quest_mobile_native_birth_owner::charge();
				return result::held_refusal;
			}
		}
		if (observed == result::captured)
			*output = object(root->stage);
		else if (observed != result::load_missed &&
			 !((observed == result::held_refusal && root->stage.state_ &&
			    root->stage.state_->pure_pending) ||
			   (observed == result::refused && !root->stage.state_)))
			registry.blocked = true;
		return observed;
	}
	catch (...)
	{
		for (const auto &root : registry.roots)
			if (root->slot == slot && root->stage.state_ &&
			    !root->stage.state_->pure_pending &&
			    root->stage.state_->result != result::captured &&
			    root->stage.state_->result != result::load_missed)
				registry.blocked = true;
		(void)quest_mobile_native_birth_owner::charge();
		return result::held_refusal;
	}
}
zone_reset_item_warm_result zone_reset_item_owner::capture_warm_child(uint32_t slot,
								      P_obj *output) noexcept
{
	using result = zone_reset_item_warm_result;
	if (!output || !begin_warm_capture() || !warm_current_ || warm_current_->blocked ||
	    !warm_scope_current(*warm_current_))
		return result::refused;
	auto &registry = *warm_current_;
	const auto observe_existing = [&](warm_child &child)
	{
		if (child.result != result::held_refusal ||
		    (child.stage.state_ && !child.stage.state_->pure_pending &&
		     child.stage.state_->result != zone_reset_item_child_result::constructed))
			return child.result;
		const auto observed = prepare_child(slot, &child.stage);
		if (!quest_mobile_native_birth_owner::charge())
			return result::held_refusal;
		if (observed == zone_reset_item_child_result::constructed)
			child.result = result::constructed;
		else if (!((observed == zone_reset_item_child_result::held_refusal &&
			    child.stage.state_ && child.stage.state_->pure_pending) ||
			   (observed == zone_reset_item_child_result::refused &&
			    !child.stage.state_)))
		{
			child.result =
				observed == zone_reset_item_child_result::native_owner_required ?
					result::native_owner_required :
					result::held_refusal;
			registry.blocked = true;
		}
		return child.result;
	};
	for (const auto &child : registry.unselected)
		if (child->slot == slot)
		{
			const auto observed = observe_existing(*child);
			if (observed == result::constructed || observed == result::captured)
				*output = object(child->stage);
			return observed;
		}
	for (const auto &root : registry.roots)
		for (const auto &child : root->children)
			if (child->slot == slot)
			{
				const auto observed = observe_existing(*child);
				if (observed == result::constructed || observed == result::captured)
					*output = object(child->stage);
				return observed;
			}
	if ((registry.has_capture && slot <= registry.last_capture_slot) ||
	    registry.unselected.size() >= PLAYER_SNAPSHOT_MAX_ROWS)
		return result::held_refusal;
	try
	{
		auto owned = std::make_unique<warm_child>();
		owned->slot = slot;
		registry.unselected.reserve(registry.unselected.size() + 1);
		auto *child = owned.get();
		registry.unselected.push_back(std::move(owned)); // Own before actual UID/factory.
		registry.last_capture_slot = slot;
		registry.has_capture = true;
		const auto observed = prepare_child(slot, &child->stage);
		size_t retained = 0;
		if (!warm_retained_size(&retained) || !quest_mobile_native_birth_owner::charge())
		{
			if (child->stage.state_ &&
			    !(child->stage.state_->pure_pending ||
			      observed == zone_reset_item_child_result::constructed))
				registry.blocked = true;
			return result::held_refusal;
		}
		if (observed == zone_reset_item_child_result::constructed)
		{
			child->result = result::constructed;
			*output = object(child->stage);
		}
		else
		{
			child->result =
				observed == zone_reset_item_child_result::native_owner_required ?
					result::native_owner_required :
					result::held_refusal;
			if (!((observed == zone_reset_item_child_result::held_refusal &&
			       child->stage.state_ && child->stage.state_->pure_pending) ||
			      (observed == zone_reset_item_child_result::refused &&
			       !child->stage.state_)))
				registry.blocked = true;
		}
		return child->result;
	}
	catch (...)
	{
		for (const auto &child : registry.unselected)
			if (child->slot == slot && child->stage.state_ &&
			    !child->stage.state_->pure_pending &&
			    child->stage.state_->result !=
				    zone_reset_item_child_result::constructed)
				registry.blocked = true;
		(void)quest_mobile_native_birth_owner::charge();
		return result::held_refusal;
	}
}
zone_reset_item_warm_result zone_reset_item_owner::place_warm_child(uint32_t slot) noexcept
{
	using result = zone_reset_item_warm_result;
	if (!warm_current_ || warm_current_->blocked || !warm_scope_current(*warm_current_))
		return result::refused;
	auto &registry = *warm_current_;
	for (const auto &root : registry.roots)
		for (const auto &child : root->children)
			if (child->slot == slot)
				return child->result; // Never repeat nesting or a chosen load roll.
	auto selected = std::find_if(registry.unselected.begin(), registry.unselected.end(),
				     [slot](const auto &child) { return child->slot == slot; });
	if (selected == registry.unselected.end())
		return result::refused;
	auto &child = **selected;
	if (child.result != result::constructed || !child.stage.state_)
		return child.result;
	auto &body = *child.stage.state_;
	const auto refuse = [&](result value)
	{
		child.result = value;
		registry.blocked = true;
		(void)quest_mobile_native_birth_owner::charge();
		return value;
	};
	const auto hold_pure = [&]()
	{
		if (!child.load_started && !child.discard_started && !child.nest_started &&
		    child.result == result::constructed && warm_capture_retryable(slot))
			return result::held_refusal; // Keep the genuine constructed stage intact.
		return refuse(result::held_refusal);
	};
	if (!warm_capture_retryable(slot) || !warm_object_current(body.object, body.reserved_uid))
		return refuse(result::held_refusal);
	if (!child.target_returned)
	{
		P_obj target = nullptr;
		bool pending = false;
		if (!quest_mobile_native_item_stage::original_reset_target(body.facts.target_rnum,
									   &target, &pending))
			return hold_pure();
		child.target = target;
		child.target_uid = target ? target->obj_uid : 0;
		child.target_pending = pending;
		child.target_returned = true; // Actual original selection returned once.
	}
	P_obj target = child.target;
	if (!target)
		return refuse(result::target_missing);
	if (!child.target_pending)
		return refuse(result::native_owner_required);
	// Reauthenticate the remembered original factory BEFORE dereferencing it.
	// Never observe the global target again after that selection returned.
	if (!warm_object_current_impl(target, child.target_uid, false))
		return refuse(result::native_owner_required);
	warm_root *owner = nullptr;
	quest_mobile_native_item_stage *target_factory = nullptr;
	for (const auto &root : registry.roots)
	{
		if (!root->stage.state_ ||
		    root->stage.state_->result != zone_reset_item_root_result::captured)
			continue;
		if (root->stage.state_->object == target &&
		    root->stage.state_->factory.owns_pending_original_target(target))
		{
			owner = root.get();
			target_factory = &root->stage.state_->factory;
		}
		for (const auto &existing : root->children)
			if (existing->result == result::captured && existing->stage.state_ &&
			    existing->stage.state_->object == target &&
			    existing->stage.state_->factory.owns_pending_original_target(target))
			{
				owner = root.get();
				target_factory = &existing->stage.state_->factory;
			}
	}
	if (!owner || !target_factory || target == body.object)
		return refuse(result::native_owner_required); // No foreign/self/older fallback.
	const auto &root_body = *owner->stage.state_;
	if (body.flat_backend != root_body.flat_backend ||
	    body.flat_backend != bool(body.flat_scope) ||
	    root_body.flat_backend != bool(root_body.flat_scope) ||
	    (body.flat_scope && body.flat_scope->root_ != root_body.flat_scope->root_) ||
	    !factory_backend_current(body.factory, body.flat_scope.get(), body.facts.reset_scope) ||
	    !factory_backend_current(root_body.factory, root_body.flat_scope.get(),
				     root_body.facts.reset_source))
		return refuse(result::held_refusal);
	const auto &operation = owner->stage.state_->facts.operation_id;
	if (!critical_operation_id_is_zero(child.selected_root_operation) &&
	    child.selected_root_operation.bytes != operation.bytes)
		return refuse(result::native_owner_required);
	child.selected_root_operation =
		operation; // Genuine original target owner, not a new clock.
	try
	{
		if (owner->children.size() + 2 > ITEM_TRANSFER_MAX_ITEMS ||
		    owner->children.size() + 2 > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    owner->children.size() + 2 > CRITICAL_COMMAND_MAX_KEYS - 2)
			return hold_pure();
		std::vector<quest_mobile_native_item_stage *> stages;
		stages.reserve(owner->children.size() + 2);
		stages.push_back(&owner->stage.state_->factory);
		for (const auto &existing : owner->children)
			stages.push_back(&existing->stage.state_->factory);
		stages.push_back(&body.factory);
		owner->children.reserve(owner->children.size() + 1);
		// Reserve real retained growth before the original load decision. A
		// refusing budget leaves the original unselected factory held, unrolled.
		if (!quest_mobile_native_birth_owner::charge())
			return hold_pure();
		child.result = result::held_refusal;
		child.load_started = true; // Includes unknown itemvalue/roll exceptions.
		child.observed_itemvalue = itemvalue(body.object);
		child.load_passed = ITEM_LOAD_CHECK(body.object, child.observed_itemvalue,
						    body.facts.original_zone_percent);
		child.load_returned = true;
		if (!child.load_passed)
		{
			child.discard_started = true;
			child.discard_succeeded = body.factory.discard_unadmitted();
			child.discard_returned = true;
			if (!child.discard_succeeded)
				return refuse(result::held_refusal);
			body.object = nullptr;
			child.result = result::load_missed;
			if (!quest_mobile_native_birth_owner::charge())
				return refuse(result::held_refusal);
			return child.result;
		}
		auto *owned_child = selected->get();
		owner->children.push_back(std::move(*selected));
		registry.unselected.erase(selected); // Complete child custody BEFORE mutation.
		owned_child->nest_started = true;
		const auto nested = quest_mobile_native_item_stage::nest_room(
			stages, owner->stage.state_->factory, body.factory, *target_factory);
		owned_child->nest_succeeded = nested == zone_reset_room_nest_result::nested;
		owned_child->nest_returned = true;
		if (!owned_child->nest_succeeded)
		{
			owned_child->result =
				nested == zone_reset_room_nest_result::original_shell_owner_required ?
					result::native_owner_required :
					result::held_refusal;
			registry.blocked = true;
			(void)quest_mobile_native_birth_owner::charge();
			return owned_child->result;
		}
		owned_child->result = result::captured;
		if (!quest_mobile_native_birth_owner::charge())
		{
			registry.blocked = true;
			return result::held_refusal;
		}
		return owned_child->result;
	}
	catch (...)
	{
		return hold_pure(); // Started/unknown native effects retain the closed original owner.
	}
}
bool zone_reset_item_owner::seal_warm_root(warm_root &root) noexcept
{
	if (root.sealed || !root.stage.state_ ||
	    root.stage.state_->result != zone_reset_item_root_result::captured)
		return root.sealed;
	try
	{
		auto &body = *root.stage.state_;
		if (!body.object || body.factory.object() != body.object ||
		    !body.factory.owns_pending_original_target(body.object) ||
		    body.flat_backend != bool(body.flat_scope) ||
		    !factory_backend_current(body.factory, body.flat_scope.get(),
					     body.facts.reset_source))
			return false;
		if (body.flat_backend &&
		    !quest_mobile_native_birth_owner::charge(sizeof(flat_binding_scratch_guard)))
			return false;
		flat_binding_scratch_guard binding_scratch(body.flat_backend);
		std::unordered_map<uint64_t, quest_mobile_native_item_stage *> factories;
		factories.reserve(root.children.size() + 1);
		factories.emplace(body.object->obj_uid, &body.factory);
		for (const auto &child : root.children)
		{
			if (child->result != zone_reset_item_warm_result::captured ||
			    !child->stage.state_ || !child->stage.state_->object ||
			    child->stage.state_->flat_backend != body.flat_backend ||
			    child->stage.state_->flat_backend !=
				    bool(child->stage.state_->flat_scope) ||
			    (body.flat_scope &&
			     child->stage.state_->flat_scope->root_ != body.flat_scope->root_) ||
			    !factory_backend_current(child->stage.state_->factory,
						     child->stage.state_->flat_scope.get(),
						     child->stage.state_->facts.reset_scope) ||
			    child->stage.state_->factory.object() != child->stage.state_->object ||
			    !factories
				     .emplace(child->stage.state_->object->obj_uid,
					      &child->stage.state_->factory)
				     .second)
				return false;
		}
		zone_reset_item_warm_forest_facts observed;
		observed.operation_id = body.facts.operation_id;
		observed.reset_source = body.facts.reset_source;
		observed.zone_vnum = body.facts.zone_vnum;
		observed.room_vnum = body.facts.room_vnum;
		if (player_item_snapshot_tree_capture_literal(body.object, &observed.items,
							      nullptr) !=
			    player_snapshot_capture_result::ok ||
		    observed.items.size() != factories.size() || observed.items.empty() ||
		    observed.items[0].object_uid != body.object->obj_uid)
			return false;
		observed.recipes.resize(observed.items.size());
		std::vector<quest_mobile_native_item_binding> bindings;
		bindings.reserve(observed.items.size());
		for (size_t row = 0; row < observed.items.size(); ++row)
		{
			const auto &literal = observed.items[row];
			auto found = factories.find(literal.object_uid);
			if (found == factories.end() || literal.equipment_slot ||
			    (row ? literal.parent_index < 0 ||
					     literal.parent_index >= static_cast<int32_t>(row) :
				   literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT) ||
			    !found->second->owns_pending_original_target(found->second->object()) ||
			    !found->second->capture_recipe(literal, &observed.recipes[row]))
				return false;
			bindings.push_back(found->second->binding_input());
			factories.erase(
				found); // Exactly one literal for every genuine owned factory.
			if (literal.type == ITEM_MONEY)
			{
				if (literal.vnum != VOBJ_COINS)
					return false;
				zone_reset_coin_output coin;
				coin.item_uid = literal.object_uid;
				for (size_t denomination = 0;
				     denomination < coin.denominations.size(); ++denomination)
				{
					if (literal.values[denomination] < 0)
						return false;
					coin.denominations[denomination] =
						literal.values[denomination];
				}
				observed.coins.push_back(
					coin); // Include exact zero and nested money literals.
			}
		}
		if (!factories.empty() ||
		    !native_mobile_birth_recipe_valid(observed.items, observed.recipes))
			return false;
		if (root.forest_captured)
		{
			// Retry pure original preparation against the exact first frozen
			// values. Never replace that forest with a later observation.
			std::vector<uint8_t> original_literal, current_literal, original_recipe,
				current_recipe;
			if (player_item_snapshot_list_encode(root.forest.items,
							     &original_literal) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(observed.items, &current_literal) !=
				    player_snapshot_codec_result::ok ||
			    native_mobile_birth_recipe_encode(
				    root.forest.items, root.forest.recipes, &original_recipe) !=
				    economic_accounting_error::ok ||
			    native_mobile_birth_recipe_encode(observed.items, observed.recipes,
							      &current_recipe) !=
				    economic_accounting_error::ok ||
			    original_literal != current_literal ||
			    original_recipe != current_recipe)
				return false;
		}
		else
		{
			root.forest = std::move(observed);
			root.forest_captured = true;
			if (!quest_mobile_native_birth_owner::charge())
				return false;
		}
		bool prepared = false;
		if (body.flat_backend)
		{
			size_t outer = sizeof(binding_scratch) + sizeof(observed) +
				       sizeof(bindings) + sizeof(factories) +
				       sizeof(std::span<const quest_mobile_native_item_binding>);
			size_t observed_heap = 0;
			if (!warm_scratch_forest_heap(observed.items, observed.recipes,
						      observed.coins, false, &observed_heap) ||
			    !warm_scratch_add(outer, observed_heap) ||
			    !warm_scratch_array(outer, bindings.capacity(),
						sizeof(quest_mobile_native_item_binding)) ||
			    !factories.empty())
				return false;
			// All original UID nodes were erased by the complete literal/factory
			// bijection. The genuine allocated bucket array remains live here.
			if (factories.bucket_count() > 1 &&
			    !warm_scratch_array(outer, factories.bucket_count(), sizeof(void *)))
				return false;
			if (!reserve_flat_binding_scratch(outer, nullptr))
				return false;
			const std::span<const quest_mobile_native_item_binding> originals{
				bindings
			};
			prepared = shop_trade_original_procedure_binding_stage::
				prepare_native_birth_flat_bounded(originals, root.whole_bindings,
								  reserve_flat_binding_scratch,
								  nullptr, outer);
		}
		else
			prepared =
				shop_trade_original_procedure_binding_stage::prepare_native_birth(
					bindings, root.whole_bindings);
		if (prepared)
			root.sealed = true; // Actual prepared binding ownership precedes recensus.
		return (body.flat_backend || quest_mobile_native_birth_owner::charge()) && prepared;
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
}
bool zone_reset_item_owner::warm_root_current(const warm_root &root) noexcept
{
	if (!nevent_is_game_thread() || !root.sealed || !root.stage.state_ ||
	    root.stage.state_->result != zone_reset_item_root_result::captured ||
	    !warm_bindings_current(root) || root.forest.items.empty())
		return false;
	try
	{
		const auto &held = *root.stage.state_;
		if (!root.placement_captured ||
		    !root.placement.matches_source(root.forest.reset_source))
			return false;
		if (!held.object || held.factory.object() != held.object ||
		    !held.factory.owns_pending_original_target(held.object) ||
		    root.forest.operation_id.bytes != held.facts.operation_id.bytes ||
		    root.forest.zone_vnum != held.facts.zone_vnum ||
		    root.forest.room_vnum != held.facts.room_vnum ||
		    root.forest.items.size() != root.children.size() + 1 ||
		    root.forest.recipes.size() != root.forest.items.size())
			return false;
		std::vector<player_item_snapshot> actual;
		std::vector<uint8_t> before, current;
		if (player_item_snapshot_tree_capture_literal(held.object, &actual, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    player_item_snapshot_list_encode(root.forest.items, &before) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(actual, &current) !=
			    player_snapshot_codec_result::ok ||
		    current != before)
			return false;
		std::vector<uint64_t> selected;
		selected.reserve(actual.size());
		for (const auto &item : actual)
		{
			quest_mobile_native_item_stage *factory = nullptr;
			if (item.object_uid == held.object->obj_uid)
				factory = &root.stage.state_->factory;
			for (const auto &child : root.children)
				if (child->stage.state_ && child->stage.state_->object &&
				    child->stage.state_->object->obj_uid == item.object_uid)
				{
					if (factory ||
					    child->result !=
						    zone_reset_item_warm_result::captured ||
					    child->selected_root_operation.bytes !=
						    held.facts.operation_id.bytes ||
					    child->stage.state_->flat_backend !=
						    held.flat_backend ||
					    child->stage.state_->flat_backend !=
						    bool(child->stage.state_->flat_scope) ||
					    (held.flat_scope &&
					     child->stage.state_->flat_scope->root_ !=
						     held.flat_scope->root_) ||
					    !factory_backend_current(
						    child->stage.state_->factory,
						    child->stage.state_->flat_scope.get(),
						    child->stage.state_->facts.reset_scope))
						return false;
					factory = &child->stage.state_->factory;
				}
			quest_mobile_native_item_progress progress{};
			if (!factory || !factory->owns_pending_original_target(factory->object()) ||
			    !factory->read_progress(&progress) || progress.admitted ||
			    progress.published || progress.next_step ||
			    progress.current_step_started)
				return false;
			selected.push_back(item.object_uid);
		}
		std::sort(selected.begin(), selected.end());
		std::vector<item_ownership_runtime_entry> cached;
		// This union includes any foreign root/parent link into a selected UID.
		// An unpublished forest has no current runtime custody anywhere.
		if (!item_ownership_runtime_published_native_observer::snapshot_links(
			    selected, ITEM_TRANSFER_MAX_ITEMS, &cached) ||
		    !cached.empty())
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
			if (object->prev != previous ||
			    std::find(selected.begin(), selected.end(), object->obj_uid) !=
				    selected.end())
				return false;
			previous = object;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_item_owner::warm_root_current_bounded(
	const warm_root &root, warm_command_scratch &scratch, size_t outer_live_scratch) noexcept
{
	if (!nevent_is_game_thread() || scratch.root != &root || !scratch.output ||
	    root.preparation_owner != &scratch || !root.sealed || !root.stage.state_ ||
	    root.stage.state_->result != zone_reset_item_root_result::captured ||
	    !warm_bindings_current(root) || root.forest.items.empty())
		return false;
	try
	{
		// matches_source owns two output arrays and its sequential encoder a
		// third canonical array. They die before the native capture vectors.
		size_t placement_live = outer_live_scratch;
		if (!warm_scratch_array(placement_live, 3,
			    sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)) ||
		    !reserve_warm_command_scratch(placement_live, &scratch))
			return false;
		const auto &held = *root.stage.state_;
		if (!root.placement_captured ||
		    !root.placement.matches_source(root.forest.reset_source))
			return false;
		if (!held.object || held.factory.object() != held.object ||
		    !held.factory.owns_pending_original_target(held.object) ||
		    root.forest.operation_id.bytes != held.facts.operation_id.bytes ||
		    root.forest.zone_vnum != held.facts.zone_vnum ||
		    root.forest.room_vnum != held.facts.room_vnum ||
		    root.forest.items.size() != root.children.size() + 1 ||
		    root.forest.recipes.size() != root.forest.items.size())
			return false;
		// Named vectors remain live through the final physical-list check. The
		// per-item progress local and read_progress's aggregate RHS can overlap.
		size_t live = outer_live_scratch;
		if (!warm_scratch_add(live, sizeof(std::vector<player_item_snapshot>)) ||
		    !warm_scratch_array(live, 2, sizeof(std::vector<uint8_t>)) ||
		    !warm_scratch_add(live, sizeof(std::vector<uint64_t>)) ||
		    !warm_scratch_add(live, sizeof(std::vector<item_ownership_runtime_entry>)) ||
		    !warm_scratch_array(live, 2, sizeof(quest_mobile_native_item_progress)) ||
		    !reserve_warm_command_scratch(live, &scratch))
			return false;
		std::vector<player_item_snapshot> actual;
		std::vector<uint8_t> before, current;
		size_t actual_heap = 0;
		if (player_item_snapshot_tree_capture_literal_bounded(held.object, &actual, nullptr,
			    reserve_warm_command_scratch, &scratch, live, &actual_heap) !=
			    player_snapshot_capture_result::ok ||
		    !warm_scratch_add(live, actual_heap) ||
		    player_item_snapshot_list_encode_bounded(root.forest.items, &before,
			    reserve_warm_command_scratch, &scratch, live) !=
			    player_snapshot_codec_result::ok ||
		    !warm_scratch_add(live, before.capacity()) ||
		    player_item_snapshot_list_encode_bounded(actual, &current,
			    reserve_warm_command_scratch, &scratch, live) !=
			    player_snapshot_codec_result::ok ||
		    !warm_scratch_add(live, current.capacity()) || current != before)
			return false;
		std::vector<uint64_t> selected;
		if (!warm_scratch_array(live, actual.size(), sizeof(uint64_t)) ||
		    !reserve_warm_command_scratch(live, &scratch))
			return false;
		selected.reserve(actual.size());
		for (const auto &item : actual)
		{
			quest_mobile_native_item_stage *factory = nullptr;
			if (item.object_uid == held.object->obj_uid)
				factory = &root.stage.state_->factory;
			for (const auto &child : root.children)
				if (child->stage.state_ && child->stage.state_->object &&
				    child->stage.state_->object->obj_uid == item.object_uid)
				{
					if (factory ||
					    child->result !=
						    zone_reset_item_warm_result::captured ||
					    child->selected_root_operation.bytes !=
						    held.facts.operation_id.bytes ||
					    child->stage.state_->flat_backend !=
						    held.flat_backend ||
					    child->stage.state_->flat_backend !=
						    bool(child->stage.state_->flat_scope) ||
					    (held.flat_scope &&
					     child->stage.state_->flat_scope->root_ !=
						     held.flat_scope->root_) ||
					    !factory_backend_current(
						    child->stage.state_->factory,
						    child->stage.state_->flat_scope.get(),
						    child->stage.state_->facts.reset_scope))
						return false;
					factory = &child->stage.state_->factory;
				}
			quest_mobile_native_item_progress progress{};
			if (!factory || !factory->owns_pending_original_target(factory->object()) ||
			    !factory->read_progress(&progress) || progress.admitted ||
			    progress.published || progress.next_step || progress.current_step_started)
				return false;
			selected.push_back(item.object_uid);
		}
		std::sort(selected.begin(), selected.end());
		std::vector<item_ownership_runtime_entry> cached;
		// Preserve the full foreign-root/parent union, not just selected owners.
		if (!item_ownership_runtime_published_native_observer::snapshot_links_bounded(
			    selected, ITEM_TRANSFER_MAX_ITEMS, &cached,
			    reserve_warm_command_scratch, &scratch, live) || !cached.empty())
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
			if (object->prev != previous ||
			    std::find(selected.begin(), selected.end(), object->obj_uid) != selected.end())
				return false;
			previous = object;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_item_owner::prepare_warm_command_flat(
	const critical_operation_id &operation, critical_native_recovery_envelope *output,
	warm_command_scratch &scratch) noexcept
{
	if (!output || !nevent_is_game_thread() || critical_operation_id_is_zero(operation) ||
	    scratch.output != output || !scratch.root ||
	    scratch.root->preparation_owner != &scratch ||
	    persistence_mode_requires_mysql() ||
	    !economic_gameplay_authority::active_regular_flat())
		return false;
	try
	{
		warm_root *root = nullptr;
		warm_registry *invocation = nullptr;
		for (auto *registry = warm_head_; registry; registry = registry->next)
			for (const auto &owned : registry->roots)
				if (owned->forest.operation_id.bytes == operation.bytes)
				{
					if (root || !registry->closed ||
					    !registry->dispatcher_completed || registry->blocked ||
					    registry->sealing_pending || !owned->stage.state_)
						return false;
					root = owned.get();
					invocation = registry;
				}
		if (!root || root != scratch.root || !invocation ||
		    !warm_root_current_bounded(*root, scratch, scratch.current_bytes()))
			return false;
		const auto &source = root->forest.reset_source;
		if (source.source.bytes != invocation->invocation.source.bytes ||
		    source.generation.bytes != invocation->invocation.generation.bytes ||
		    source.kind != invocation->invocation.kind ||
		    source.sequence != invocation->invocation.sequence ||
		    source.slot != root->slot || root->slot >= invocation->stop_slot ||
		    root->forest.zone_vnum != invocation->zone_vnum)
			return false;
		const auto &birth = *root->stage.state_;
		const char *configured = persistence_mode_flatfile_root();
		{
			// The two actual projection IDs die before the preparation frame.
			// Admit them alongside the observer already in inline_bytes().
			size_t projection_live = scratch.current_bytes();
			if (!warm_scratch_array(projection_live, 2,
						sizeof(critical_operation_id)) ||
			    !reserve_warm_command_scratch(projection_live, &scratch))
				return false;
			critical_operation_id current_lineage{}, current_epoch{};
			if (!birth.flat_backend || !birth.flat_scope || !configured ||
			    !*configured || birth.flat_scope->root_ != configured ||
			    !economic_gameplay_authority::capture_flat_reset_projection(
				    &current_lineage, &current_epoch) ||
			    current_lineage.bytes != birth.flat_lineage.bytes ||
			    current_epoch.bytes != birth.flat_epoch.bytes)
				return false;
		}
		if (!root->original_envelope.attachment.empty())
		{
			const size_t caller_live = scratch.current_bytes();
			size_t clone_live = caller_live, clone_heap = 0;
			if (!zone_reset_item_recovery_initial_bounded(root->original_envelope,
				    reserve_warm_command_scratch, &scratch, caller_live) ||
			    !warm_scratch_envelope_heap(root->original_envelope, true, &clone_heap) ||
			    !warm_scratch_add(clone_live, sizeof(critical_native_recovery_envelope)) ||
			    !warm_scratch_add(clone_live, clone_heap) ||
			    !rebase_warm_command_scratch(scratch, clone_live))
				return false;
			auto observed = root->original_envelope;
			*output = std::move(observed);
			return true; // Preserve its original season/room cut; no recapture/rebind.
		}
		// The original registry is newest-first and its roots are capture-order.
		// A prior retained root for this room must retire before this one takes
		// its ACTUAL current counter. Never predict future CAS clocks by adding
		// the number of earlier roots to a single observed revision.
		for (auto *older = invocation->next; older; older = older->next)
			for (const auto &owned : older->roots)
				if (owned->stage.state_ && owned->sealed &&
				    owned->forest.room_vnum == root->forest.room_vnum)
					return false;
		for (const auto &owned : invocation->roots)
		{
			if (owned.get() == root)
				break;
			if (owned->stage.state_ && owned->sealed &&
			    owned->forest.room_vnum == root->forest.room_vnum)
				return false;
		}
		// This is the authentic retained O factory's selected root. Current
		// configuration merely corroborates it; it cannot supply new provenance.
		configured = birth.flat_scope->root_.c_str();
		const size_t root_chars = std::char_traits<char>::length(configured);
		const size_t caller_live = scratch.current_bytes();
		size_t frame_live = caller_live, image_heap = 0;
		if (!warm_scratch_add(frame_live, sizeof(zone_reset_item_image)) ||
		    !warm_scratch_add(frame_live, sizeof(zone_reset_room_placement_recipe)) ||
		    !warm_scratch_add(frame_live, sizeof(critical_native_recovery_envelope)) ||
		    !warm_scratch_add(frame_live, sizeof(std::vector<uint8_t>)) ||
		    !warm_scratch_add(frame_live, sizeof(std::string)) ||
		    !warm_scratch_forest_heap(root->forest.items, root->forest.recipes,
			root->forest.coins, true, &image_heap) ||
		    !warm_scratch_add(frame_live, image_heap) ||
		    (root_chars > 15 && (root_chars == SIZE_MAX ||
			!warm_scratch_add(frame_live, root_chars + 1))) ||
		    !reserve_warm_command_scratch(frame_live, &scratch))
			return false;
		zone_reset_item_image image;
		image.operation_id = root->forest.operation_id;
		image.reset_source = root->forest.reset_source;
		image.zone_vnum = root->forest.zone_vnum;
		image.room_vnum = root->forest.room_vnum;
		image.items = root->forest.items;
		image.recipes = root->forest.recipes;
		image.coins = root->forest.coins;
		zone_reset_room_placement_recipe placement;
		// Comparison arrays die before recipe() constructs its candidate DTO;
		// both phases overlap the already-cloned image and caller-owned locals.
		size_t placement_live = frame_live;
		if (!warm_scratch_add(placement_live, std::max(
			    3 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
			    sizeof(zone_reset_room_placement_recipe))) ||
		    !reserve_warm_command_scratch(placement_live, &scratch) ||
		    !root->placement_captured ||
		    !root->placement.matches_source(image.reset_source) ||
		    !root->placement.recipe(&placement))
			return false;
		// The original selected-falling participant is still required. Keep
		// its real draw/full detached forest before flat admission or native
		// batch/services; do not discover this unsupported route after effects.
		if (placement.fall_selected)
			return false;
		image.placement = placement;

		critical_native_recovery_envelope original;
		std::vector<uint8_t> canonical;
		const std::string selected_root(configured);
		{
			// One actual recovered selected-root freeze. Values never grant
			// source, constructor, execution or publication permission.
			// Recovered providers and codecs below reserve their real scratch.
			// Native recapture and complete INITIAL observation are bounded as well.
			// Publication preparation still needs its own transitive bounds.
			size_t lock_live = frame_live;
			if (!warm_scratch_add(lock_live, sizeof(flatfile_authority_lock)) ||
			    !reserve_warm_command_scratch(lock_live, &scratch))
				return false;
			flatfile_authority_lock lock(reserve_warm_command_scratch, &scratch, frame_live);
			size_t lock_retained = 0;
			if (!lock.retained_bytes(&lock_retained))
				return false;
			lock_live = frame_live;
			if (!warm_scratch_add(lock_live, lock_retained) ||
			    !lock.acquire_bounded(selected_root, reserve_warm_command_scratch,
				&scratch, lock_live) || !lock.retained_bytes(&lock_retained))
				return false;
			size_t storage_live = frame_live;
			if (!warm_scratch_add(storage_live, lock_retained))
				return false;
			const auto recovered = flatfile_authority_transaction_recover_bounded(
				selected_root, lock, reserve_warm_command_scratch, &scratch, storage_live);
			if (recovered != flatfile_authority_transaction_result::ok &&
			    recovered != flatfile_authority_transaction_result::not_found)
				return false;
			if (!warm_scratch_add(storage_live, sizeof(flatfile_season_state)) ||
			    !warm_scratch_add(storage_live, sizeof(std::vector<flatfile_corpse_record>)) ||
			    !warm_scratch_add(storage_live, sizeof(std::vector<flatfile_room_item_record>)) ||
			    !warm_scratch_add(storage_live, sizeof(std::vector<flatfile_saved_world_item_record>)) ||
			    !reserve_warm_command_scratch(storage_live, &scratch))
				return false;
			flatfile_season_state season;
			if (flatfile_season_state_read_locked_bounded(selected_root, lock, &season,
				    reserve_warm_command_scratch, &scratch, storage_live) !=
				    flatfile_season_state_result::ok ||
			    season.status != flatfile_season_status::active || !season.epoch)
				return false;
			image.season_epoch = season.epoch;
			std::vector<flatfile_corpse_record> corpses;
			std::vector<flatfile_room_item_record> rooms;
			std::vector<flatfile_saved_world_item_record> saved;
			size_t world_heap = 0;
			const auto world_read = flatfile_world_item_recovery_list_all_locked_bounded(
				selected_root, lock, &corpses, &rooms, &saved,
				reserve_warm_command_scratch, &scratch, storage_live, &world_heap);
			if (world_read != flatfile_world_item_result::ok &&
			    world_read != flatfile_world_item_result::not_found)
				return false;
			if (!warm_scratch_add(storage_live, world_heap))
				return false;
			image.expected_room_revision = 0; // Genuine absent selected native ROOM.
			if (world_read == flatfile_world_item_result::ok)
			{
				const auto found = std::find_if(
					rooms.begin(), rooms.end(), [&](const auto &room)
					{ return room.room_vnum == image.room_vnum; });
				if (found != rooms.end())
					image.expected_room_revision = found->revision;
			}
			if (image.expected_room_revision == UINT64_MAX)
				return false;
			if (!warm_scratch_add(storage_live, sizeof(item_owner_identity)) ||
			    !warm_scratch_add(storage_live,
					 sizeof(std::vector<flatfile_item_ownership_record>)) ||
			    !reserve_warm_command_scratch(storage_live, &scratch))
				return false;
			const item_owner_identity selected_owner{
				item_owner_type::room, static_cast<uint64_t>(image.room_vnum), 0 };
			uint64_t custody_revision = 0;
			std::vector<flatfile_item_ownership_record> active;
			size_t custody_heap = 0;
			const auto custody_read = flatfile_item_repository_load_owner_locked_bounded(
				selected_root, lock, selected_owner, &custody_revision, &active,
				reserve_warm_command_scratch, &scratch, storage_live, &custody_heap);
			if (custody_read != flatfile_item_repository_result::ok &&
			    custody_read != flatfile_item_repository_result::not_found)
				return false;
			if (!warm_scratch_add(storage_live, custody_heap))
				return false;
			if (custody_revision != image.expected_room_revision)
				return false;
			// The filtered owner read supplies ONLY its genuine counter. Full
			// history/empty-context/born-UID/world topology proof follows below.
			if (!warm_root_current_bounded(*root, scratch, storage_live) ||
			    economic_gameplay_authority::prepare_zone_reset_item_flat_bounded(
				    image, root->stage.state_->facts.accepted_at_usec,
				    &original.command, reserve_warm_command_scratch,
				    &scratch, storage_live) != economic_accounting_error::ok)
				return false;
			size_t command_heap = 0;
			if (!warm_scratch_envelope_heap(original, false, &command_heap) ||
			    !warm_scratch_add(storage_live, command_heap) ||
			    critical_command_encode_bounded(original.command, &canonical,
				    reserve_warm_command_scratch, &scratch, storage_live) !=
				    critical_command_codec_result::ok)
				return false;
			if (!warm_scratch_add(storage_live, canonical.capacity()) ||
			    !warm_scratch_add(storage_live, sizeof(zone_reset_item_recovery_context)))
				return false;
			size_t initial_heap = 0;
			if (!warm_scratch_array(initial_heap, image.items.size(),
						sizeof(zone_reset_item_recovery_item)))
				return false;
			for (const auto &recipe : image.recipes)
			{
				if (recipe.libraries.size() > (SIZE_MAX - 3) / 2 ||
				    !warm_scratch_array(initial_heap, 2 * recipe.libraries.size() + 3,
						    sizeof(zone_reset_item_recovery_effect)))
					return false;
			}
			size_t initial_peak = storage_live;
			if (!warm_scratch_add(storage_live, initial_heap) ||
			    !warm_scratch_add(initial_peak, initial_heap) ||
			    !warm_scratch_add(initial_peak, sizeof(zone_reset_item_recovery_item)) ||
			    !reserve_warm_command_scratch(initial_peak, &scratch))
				return false;
			zone_reset_item_recovery_context initial;
			initial.items.reserve(image.items.size());
			for (size_t at = 0; at < image.items.size(); ++at)
			{
				zone_reset_item_recovery_item item;
				item.object_uid = image.items[at].object_uid;
				item.effects.resize(2 * image.recipes[at].libraries.size() + 3);
				initial.items.push_back(std::move(item));
			}
			original.revision = 1;
			original.phase = critical_native_recovery_phase::execution_pending;
			if (zone_reset_item_recovery_encode_bounded(
				    original.command, initial, &original.attachment,
				    reserve_warm_command_scratch, &scratch,
				    storage_live) != economic_accounting_error::ok ||
			    !warm_scratch_add(storage_live, original.attachment.capacity()) ||
			    !zone_reset_item_recovery_initial_bounded(original,
								      reserve_warm_command_scratch,
								      &scratch, storage_live) ||
			    flatfile_zone_reset_item_publication_storage::
					    observe_initial_locked_bounded(
						    selected_root, lock, original,
						    reserve_warm_command_scratch, &scratch,
						    storage_live) != 0 ||
			    !warm_root_current_bounded(*root, scratch, storage_live) ||
			    !lock.matches(selected_root) || persistence_mode_requires_mysql() ||
			    !economic_gameplay_authority::active_regular_flat())
				return false;
			const char *still_configured = persistence_mode_flatfile_root();
			if (!still_configured || selected_root != still_configured)
				return false;
		} // Release genuine root lock before retaining/delivering the capsule.
		size_t lockless_live = frame_live, original_heap = 0;
		if (!warm_scratch_envelope_heap(original, false, &original_heap) ||
		    !warm_scratch_add(lockless_live, original_heap) ||
		    !warm_scratch_add(lockless_live, canonical.capacity()) ||
		    !warm_root_current_bounded(*root, scratch, lockless_live))
			return false;
		root->original_envelope = std::move(original);
		root->canonical_command = std::move(canonical);
		// Locked provider/INITIAL temporaries have died and the two moved buffers
		// now belong to the retained root. Rebase before the final fresh clone;
		// count retained ownership once, plus still-live image/path/inline locals.
		size_t clone_live = frame_live, clone_heap = 0;
		if (!warm_scratch_envelope_heap(root->original_envelope, true, &clone_heap) ||
		    !warm_scratch_add(clone_live, sizeof(critical_native_recovery_envelope)) ||
		    !warm_scratch_add(clone_live, clone_heap) ||
		    !rebase_warm_command_scratch(scratch, clone_live))
			return false; // Retain frozen season/counter/time/forest; no reroll.
		auto result = root->original_envelope;
		*output = std::move(result);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool zone_reset_item_owner::prepare_warm_command(const warm_root &root,
						 critical_native_recovery_envelope *output,
						 warm_command_scratch &scratch) noexcept
{
	if (!root.stage.state_)
		return false;
	// Select the retained genuine factory backend, then let that backend's
	// original authority guards refuse any later configuration mismatch.
	if (!root.stage.state_->flat_backend)
		return prepare_warm_command_sql(root.forest.operation_id, output);
	if (scratch.root != &root)
		return false;
	return prepare_warm_command_flat(root.forest.operation_id, output, scratch);
}

bool zone_reset_item_owner::prepare_warm_command_sql(
	const critical_operation_id &operation, critical_native_recovery_envelope *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)operation;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() || critical_operation_id_is_zero(operation) ||
	    !economic_gameplay_authority::active_regular_sql())
		return false;
	try
	{
		warm_root *root = nullptr;
		warm_registry *invocation = nullptr;
		for (auto *registry = warm_head_; registry; registry = registry->next)
			for (const auto &owned : registry->roots)
				if (owned->forest.operation_id.bytes == operation.bytes)
				{
					if (root || !registry->closed ||
					    !registry->dispatcher_completed || registry->blocked ||
					    registry->sealing_pending || !owned->stage.state_)
						return false;
					root = owned.get();
					invocation = registry;
				}
		if (!root || !invocation || !warm_root_current(*root))
			return false;
		const auto &source = root->forest.reset_source;
		if (source.source.bytes != invocation->invocation.source.bytes ||
		    source.generation.bytes != invocation->invocation.generation.bytes ||
		    source.kind != invocation->invocation.kind ||
		    source.sequence != invocation->invocation.sequence ||
		    source.slot != root->slot || root->slot >= invocation->stop_slot ||
		    root->forest.zone_vnum != invocation->zone_vnum)
			return false;
		if (!root->original_envelope.attachment.empty())
		{
			size_t retained = 0;
			if (!zone_reset_item_recovery_initial(root->original_envelope) ||
			    !warm_retained_size(&retained) ||
			    !quest_mobile_native_birth_owner::charge())
				return false;
			auto observed = root->original_envelope;
			*output = std::move(observed);
			return true; // Preserve its original season/room cut; no recapture/rebind.
		}
		// The original registry is newest-first and its roots are capture-order.
		// A prior retained root for this room must retire before this one takes
		// its ACTUAL current counter. Never predict future CAS clocks by adding
		// the number of earlier roots to a single observed revision.
		for (auto *older = invocation->next; older; older = older->next)
			for (const auto &owned : older->roots)
				if (owned->stage.state_ && owned->sealed &&
				    owned->forest.room_vnum == root->forest.room_vnum)
					return false;
		for (const auto &owned : invocation->roots)
		{
			if (owned.get() == root)
				break;
			if (owned->stage.state_ && owned->sealed &&
			    owned->forest.room_vnum == root->forest.room_vnum)
				return false;
		}
		zone_reset_item_image image;
		image.operation_id = root->forest.operation_id;
		image.reset_source = root->forest.reset_source;
		image.zone_vnum = root->forest.zone_vnum;
		image.room_vnum = root->forest.room_vnum;
		image.items = root->forest.items;
		image.recipes = root->forest.recipes;
		image.coins = root->forest.coins;
		zone_reset_room_placement_recipe placement;
		if (!root->placement_captured ||
		    !root->placement.matches_source(image.reset_source) ||
		    !root->placement.recipe(&placement))
			return false;
		// The original selected-falling participant is still required. Keep
		// its real draw/full detached forest before SQL admission or native
		// batch/services; do not discover this unsupported route after effects.
		if (placement.fall_selected)
			return false;
		image.placement = placement;
		critical_native_recovery_envelope original;
		std::vector<uint8_t> canonical;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool observed = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17) ||
			    !transaction.same_session() ||
			    !sql_room_item_payload_lock_season(connection, &image.season_epoch) ||
			    !item_transfer_repository_lock_owner(
				    connection,
				    { item_owner_type::room, static_cast<uint64_t>(image.room_vnum),
				      0 },
				    &image.expected_room_revision))
				throw EAGAIN;
			sql_room_item_payload_batch absent;
			if (!sql_room_item_payload_prepare_creation(connection, image, &absent) ||
			    !warm_root_current(*root) ||
			    economic_gameplay_authority::prepare_zone_reset_item(
				    image, root->stage.state_->facts.accepted_at_usec,
				    &original.command) != economic_accounting_error::ok ||
			    critical_command_encode(original.command, &canonical) !=
				    critical_command_codec_result::ok)
				throw EAGAIN;
			zone_reset_item_recovery_context initial;
			initial.items.reserve(image.items.size());
			for (size_t at = 0; at < image.items.size(); ++at)
			{
				zone_reset_item_recovery_item item;
				item.object_uid = image.items[at].object_uid;
				item.effects.resize(2 * image.recipes[at].libraries.size() + 3);
				initial.items.push_back(std::move(item));
			}
			original.revision = 1;
			original.phase = critical_native_recovery_phase::execution_pending;
			if (zone_reset_item_recovery_encode(original.command, initial,
							    &original.attachment) !=
				    economic_accounting_error::ok ||
			    !zone_reset_item_recovery_initial(original) ||
			    !transaction.same_session() ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS))
				throw EAGAIN;
			observed = true;
		}
		catch (...)
		{
			observed = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!observed || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		root->original_envelope = std::move(original);
		root->canonical_command = std::move(canonical);
		size_t retained = 0;
		if (!warm_retained_size(&retained) || !quest_mobile_native_birth_owner::charge())
			return false; // Retain the original capsule; never restart its factory.
		auto result = root->original_envelope;
		*output = std::move(result);
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
enum : uint8_t
{
	WARM_BINDING = 1,
	WARM_BATCH,
	WARM_PLACE,
	WARM_ITEM_EFFECT
};
quest_mobile_native_item_effect warm_action_value(const zone_reset_item_recovery_context &context,
						  uint8_t kind, size_t row, size_t step)
{
	if (kind == WARM_ITEM_EFFECT)
	{
		const auto &effect = context.items.at(row).effects.at(step);
		return { effect.started, effect.returned, effect.succeeded, effect.periodic };
	}
	const auto &action = kind == WARM_BINDING ? context.whole_binding :
			     kind == WARM_BATCH	  ? context.batch_publication :
						    context.room_placement;
	return { action.started, action.returned, action.succeeded, false };
}
void set_warm_action(zone_reset_item_recovery_context &context, uint8_t kind, size_t row,
		     size_t step, const quest_mobile_native_item_effect &effect)
{
	if (kind == WARM_ITEM_EFFECT)
	{
		auto &item = context.items.at(row);
		item.effects.at(step) = { effect.started, effect.returned, effect.succeeded,
					  effect.periodic };
		item.current_step_started = effect.started &&
					    !(effect.returned && effect.succeeded);
		if (effect.returned && effect.succeeded)
			item.next_step = static_cast<uint32_t>(step + 1);
		return;
	}
	auto &action = kind == WARM_BINDING ? context.whole_binding :
		       kind == WARM_BATCH   ? context.batch_publication :
					      context.room_placement;
	action = { effect.started, effect.returned, effect.succeeded };
	if (kind == WARM_BATCH && effect.returned && effect.succeeded)
		for (auto &item : context.items)
			item.published = true;
}
}

bool zone_reset_item_owner::settle_warm_checkpoint(warm_root &root) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	if (!root.checkpoint)
		return true;
	if (!zone_reset_room_publication_owner::checkpoint_warm(root.checkpoint->expected,
								root.checkpoint->successor))
		return false;
	root.original_envelope = std::move(root.checkpoint->successor);
	root.context = std::move(root.checkpoint->context);
	root.checkpoint.reset();
	// Actual retained growth and discarded checkpoint storage share one census.
	return quest_mobile_native_birth_owner::charge();
}

bool zone_reset_item_owner::checkpoint_warm_context(
	warm_root &root, const zone_reset_item_recovery_context &next) noexcept
{
	if (!nevent_is_game_thread() || root.blocked ||
	    root.original_envelope.revision == UINT64_MAX)
		return false;
	if (root.checkpoint)
		return settle_warm_checkpoint(root);
	try
	{
		auto writer = std::make_unique<warm_checkpoint>();
		writer->expected = root.original_envelope;
		writer->successor = root.original_envelope;
		++writer->successor.revision;
		writer->context = next;
		if (zone_reset_item_recovery_encode(writer->successor.command, next,
						    &writer->successor.attachment) !=
			    economic_accounting_error::ok ||
		    !zone_reset_item_recovery_successor(writer->expected, writer->successor))
			return false;
		root.checkpoint = std::move(writer);
		if (!quest_mobile_native_birth_owner::charge())
		{
			root.checkpoint.reset();
			(void)quest_mobile_native_birth_owner::charge();
			return false;
		}
		return settle_warm_checkpoint(root);
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
}

int zone_reset_item_owner::prepare_warm_action(warm_root &root, uint8_t kind, size_t row,
					       size_t step) noexcept
{
	if (kind < WARM_BINDING || kind > WARM_ITEM_EFFECT || root.blocked ||
	    !settle_warm_checkpoint(root))
		return -1;
	try
	{
		const auto existing = warm_action_value(root.context, kind, row, step);
		if (existing.returned)
			return existing.succeeded ? 0 : -1;
		if (existing.started)
			return root.action_not_attempted && root.pending_action == kind &&
					       root.pending_row == row &&
					       root.pending_step == step ?
				       1 :
				       -1;
		if (root.action_not_attempted || root.original_envelope.revision > UINT64_MAX - 2)
			return -1;
		auto started = root.context;
		set_warm_action(started, kind, row, step, { true, false, false, false });
		auto intent = std::make_unique<warm_checkpoint>();
		intent->expected = root.original_envelope;
		intent->successor = root.original_envelope;
		++intent->successor.revision;
		intent->context = started;
		if (zone_reset_item_recovery_encode(intent->successor.command, started,
						    &intent->successor.attachment) !=
			    economic_accounting_error::ok ||
		    !zone_reset_item_recovery_successor(intent->expected, intent->successor))
			return -1;
		// Allocate and encode all actual supported returned variants BEFORE intent
		// I/O/effect. Their selection later records observation; it grants no effect.
		std::array<std::unique_ptr<warm_checkpoint>, 4> returned;
		for (size_t bits = 0; bits < returned.size(); ++bits)
		{
			if ((bits & 2) && kind != WARM_ITEM_EFFECT)
				continue;
			auto writer = std::make_unique<warm_checkpoint>();
			writer->expected = intent->successor;
			writer->successor = intent->successor;
			++writer->successor.revision;
			writer->context = started;
			set_warm_action(writer->context, kind, row, step,
					{ true, true, bool(bits & 1), bool(bits & 2) });
			if (zone_reset_item_recovery_encode(writer->successor.command,
							    writer->context,
							    &writer->successor.attachment) ==
				    economic_accounting_error::ok &&
			    zone_reset_item_recovery_successor(writer->expected, writer->successor))
				returned[bits] = std::move(writer);
		}
		if (!returned[1] && !returned[3])
			return -1;
		root.checkpoint = std::move(intent);
		root.returned_writers = std::move(returned);
		root.pending_action = kind;
		root.pending_row = row;
		root.pending_step = step;
		root.action_not_attempted = true;
		if (!quest_mobile_native_birth_owner::charge())
		{
			root.checkpoint.reset();
			for (auto &writer : root.returned_writers)
				writer.reset();
			root.action_not_attempted = false;
			(void)quest_mobile_native_birth_owner::charge();
			return -1;
		}
		return settle_warm_checkpoint(root) ? 1 : -1;
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return -1;
	}
}

bool zone_reset_item_owner::finish_warm_action(
	warm_root &root, const quest_mobile_native_item_effect &actual) noexcept
{
	if (!actual.started)
		return false; // Genuine owner still knows not-attempted.
	root.action_not_attempted = false;
	if (!actual.returned)
	{
		root.blocked = true;
		return false; // Unknown callback return cannot become absent/replay proof.
	}
	const size_t bits = size_t(actual.succeeded) | (size_t(actual.periodic) << 1);
	if (!root.returned_writers[bits])
	{
		root.blocked = true;
		return false;
	}
	root.checkpoint = std::move(root.returned_writers[bits]);
	for (auto &writer : root.returned_writers)
		writer.reset();
	// The actual returned state selects an already sealed writer before any
	// fallible recensus, next callback, encoding, allocation or SQL operation.
	return settle_warm_checkpoint(root) && actual.succeeded;
}

bool zone_reset_item_owner::replay_ready_ = false;
namespace
{
bool same_warm_completion(const critical_completion &a, const critical_completion &b) noexcept
{
	return a.operation_id.bytes == b.operation_id.bytes && a.outcome == b.outcome &&
	       a.disposition == b.disposition && a.durable_revision == b.durable_revision &&
	       a.error_code == b.error_code && a.failure_stage == b.failure_stage &&
	       a.attempt == b.attempt && a.queued_at_usec == b.queued_at_usec &&
	       a.started_at_usec == b.started_at_usec &&
	       a.completed_at_usec == b.completed_at_usec && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload &&
	       a.recovery_correlation == b.recovery_correlation;
}
bool successful_warm_receipt(const critical_completion &receipt) noexcept
{
	return receipt.disposition == critical_completion_disposition::execution &&
	       (receipt.outcome == critical_apply_outcome::applied ||
		receipt.outcome == critical_apply_outcome::already_applied) &&
	       !receipt.error_code && receipt.failure_stage == critical_failure_stage::none &&
	       receipt.result_size == ITEM_TRANSFER_RESULT_BYTES;
}

bool same_warm_economic_receipt(const critical_completion &a, const critical_completion &b) noexcept
{
	return successful_warm_receipt(a) && successful_warm_receipt(b) &&
	       a.operation_id.bytes == b.operation_id.bytes &&
	       a.durable_revision == b.durable_revision && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload;
}
}

bool zone_reset_item_owner::restore_cold_bindings(
	std::span<quest_mobile_native_item_stage *> factories, void *context) noexcept
{
	if (!context || !nevent_is_game_thread())
		return false;
	auto &root = *static_cast<warm_root *>(context);
	if (!root.cold || !root.submitted || !root.completed || root.blocked ||
	    factories.size() != root.forest.items.size())
		return false;
	if (root.cold_binding_restored)
		return true;
	try
	{
		if (!root.cold_bindings_prepared)
		{
			std::vector<quest_mobile_native_item_binding> inputs;
			inputs.reserve(factories.size());
			for (size_t at = 0; at < factories.size(); ++at)
			{
				const auto *factory = factories[at];
				if (!factory || !factory->object() ||
				    factory->object()->obj_uid != root.forest.items[at].object_uid)
					return false;
				inputs.push_back(factory->binding_input());
			}
			const bool prepared =
				shop_trade_original_procedure_binding_stage::prepare_native_birth(
					inputs, root.whole_bindings);
			if (prepared)
				root.cold_bindings_prepared = true;
			if (!quest_mobile_native_birth_owner::charge() || !prepared)
				return false;
		}
		if (!root.whole_bindings.valid())
			return false;
		// Rebuild actual process-local procedure bindings under the complete
		// original SQL/native absence cut. The recorded original returned action
		// stays unchanged; no original native callback or RNG is repeated.
		if (root.context.whole_binding.succeeded)
		{
			root.whole_bindings.commit_unchecked();
			root.cold_binding_restored = true;
		}
		return quest_mobile_native_birth_owner::charge();
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
}

bool zone_reset_item_owner::publish_warm(warm_root &root) noexcept
{
#ifdef __NO_MYSQL__
	(void)root;
	return false;
#else
	if (!nevent_is_game_thread() || !root.submitted || !root.completed || root.blocked ||
	    !root.coordinator_generation || !successful_warm_receipt(root.completion) ||
	    !settle_warm_checkpoint(root))
		return false;
	try
	{
		critical_native_recovery_envelope current;
		uint64_t generation = 0;
		if (!zone_reset_room_publication_owner::copy_warm(root.original_envelope.command,
								  &current) ||
		    !critical_command_equal(current.command, root.original_envelope.command) ||
		    current.revision != root.original_envelope.revision ||
		    current.phase != root.original_envelope.phase ||
		    current.attachment != root.original_envelope.attachment ||
		    !zone_reset_room_publication_owner::generation_warm(current, &generation) ||
		    generation != root.coordinator_generation)
			return false;
		if (root.context.items.empty())
		{
			if (zone_reset_item_recovery_decode(current.command, current.attachment,
							    &root.context) !=
				    economic_accounting_error::ok ||
			    !quest_mobile_native_birth_owner::charge())
				return false;
		}
		if (!root.context.receipt_present ||
		    !same_warm_completion(root.context.receipt, root.completion))
		{
			if (root.context.receipt_present &&
			    !same_warm_economic_receipt(root.context.receipt, root.completion))
				return false;
			if (current.phase == critical_native_recovery_phase::continuation_pending)
			{
				// The original ACK froze this complete terminal BODY. A replay
				// delivery may change attempts/times/outcome without changing
				// its economic receipt. The actual current coordinator delivery
				// remains required, but cannot rewrite that frozen attachment.
				if (!root.context.receipt_present ||
				    !zone_reset_item_recovery_publication(current, root.completion))
					return false;
			}
			else
			{
				auto next = root.context;
				next.receipt_present = true;
				next.receipt = root.completion;
				if (next.stage == zone_reset_item_recovery_stage::captured)
					next.stage = zone_reset_item_recovery_stage::publishing;
				if (!checkpoint_warm_context(root, next))
					return false;
			}
		}
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17) ||
			    !transaction.same_session() ||
			    (root.cold &&
			     !zone_reset_room_publication_owner::prepare_original_completed_locked(
				     connection, root.original_envelope, root.completion,
				     root.publication, &quest_mobile_native_birth_owner::charge) &&
			     !zone_reset_room_publication_owner::prepare_original_reconstructed_locked(
				     connection, root.original_envelope, root.completion,
				     root.publication, root.published, restore_cold_bindings, &root,
				     &quest_mobile_native_birth_owner::charge) &&
			     !zone_reset_room_publication_owner::
				     prepare_original_present_prefix_locked(
					     connection, root.original_envelope, root.completion,
					     root.publication, &root.placement,
					     &quest_mobile_native_birth_owner::charge) &&
			     !zone_reset_room_publication_owner::prepare_original_pending_locked(
				     connection, root.original_envelope, root.completion,
				     root.publication, &root.placement, root.published,
				     restore_cold_bindings, &root,
				     &quest_mobile_native_birth_owner::charge)) ||
			    !zone_reset_room_publication_owner::refresh_warm_locked(
				    connection, root.original_envelope.command, root.completion,
				    root.publication) ||
			    !quest_mobile_native_birth_owner::charge())
				throw EAGAIN;
			if (!root.context.batch_publication.succeeded)
			{
				if (!zone_reset_room_publication_owner::retain_admitted(
					    root.publication))
					throw EAGAIN;
				auto admitted = root.context;
				for (auto &item : admitted.items)
					item.admitted = true;
				if (!std::all_of(root.context.items.begin(),
						 root.context.items.end(),
						 [](const auto &item) { return item.admitted; }) &&
				    !checkpoint_warm_context(root, admitted))
					throw EAGAIN;
			}
			if (!root.context.whole_binding.succeeded)
			{
				if (!root.whole_bindings.valid())
					throw EAGAIN;
				const int run = prepare_warm_action(root, WARM_BINDING, 0, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					root.whole_bindings.commit_unchecked();
					if (root.cold)
						root.cold_binding_restored = true;
					if (!finish_warm_action(root, { true, true, true, false }))
						throw EAGAIN;
				}
			}
			if (!root.context.batch_publication.succeeded)
			{
				const bool reserved =
					zone_reset_room_publication_owner::reserve_warm_consume(
						root.publication, root.published);
				if (!quest_mobile_native_birth_owner::charge() || !reserved)
					throw EAGAIN;
				const int run = prepare_warm_action(root, WARM_BATCH, 0, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					// publish_many refuses atomically before native consumption.
					// Retain the genuine not-attempted latch; do not turn a
					// preflight refusal into an uncertain/returned native effect.
					if (!zone_reset_room_publication_owner::consume(
						    root.publication, &root.published))
						throw EAGAIN;
					if (!finish_warm_action(root, { true, true, true, false }))
						throw EAGAIN;
				}
			}
			if (!zone_reset_room_publication_owner::verify_current(root.publication))
				throw EAGAIN;
			for (size_t row = 0; row < root.context.items.size(); ++row)
				for (size_t step = root.context.items[row].next_step;
				     step < root.context.items[row].effects.size(); ++step)
				{
					if (!transaction.same_session() ||
					    !zone_reset_room_publication_owner::verify_current(
						    root.publication))
						throw EAGAIN;
					const int run = prepare_warm_action(root, WARM_ITEM_EFFECT,
									    row, step);
					if (run < 0)
						throw EAGAIN;
					if (!run)
						continue;
					quest_mobile_native_item_effect actual{};
					zone_reset_room_publication_owner::service_step(
						root.publication, row, step, actual);
					if (!finish_warm_action(root, actual))
						throw EAGAIN;
				}
			if (!root.context.room_placement.succeeded)
			{
				const int run = prepare_warm_action(root, WARM_PLACE, 0, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					quest_mobile_native_item_effect actual{};
					zone_reset_room_publication_owner::place_warm(
						root.publication, actual);
					if (!finish_warm_action(root, actual))
						throw EAGAIN;
				}
			}
			if (!zone_reset_room_publication_owner::verify_current(root.publication) ||
			    !zone_reset_room_publication_owner::mark_published(root.publication,
									       &root.published) ||
			    !zone_reset_room_publication_owner::refresh_warm_locked(
				    connection, root.original_envelope.command, root.completion,
				    root.publication) ||
			    !transaction.same_session())
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		if (root.context.stage != zone_reset_item_recovery_stage::physically_proven)
		{
			auto next = root.context;
			next.runtime_applied = true;
			next.stage = zone_reset_item_recovery_stage::physically_proven;
			if (!checkpoint_warm_context(root, next))
				return false;
		}
		if (root.original_envelope.phase ==
		    critical_native_recovery_phase::execution_pending)
		{
			if (!root.ack_successor)
			{
				if (root.original_envelope.revision == UINT64_MAX)
					return false;
				auto successor =
					std::make_unique<critical_native_recovery_envelope>(
						root.original_envelope);
				++successor->revision;
				successor->phase =
					critical_native_recovery_phase::continuation_pending;
				if (!zone_reset_item_recovery_successor(root.original_envelope,
									*successor))
					return false;
				root.ack_successor = std::move(successor);
				if (!quest_mobile_native_birth_owner::charge())
				{
					root.ack_successor.reset();
					(void)quest_mobile_native_birth_owner::charge();
					return false;
				}
			}
			if (!zone_reset_room_publication_owner::acknowledge_warm(
				    root.original_envelope, root.completion,
				    root.coordinator_generation))
				return false;
			root.original_envelope = std::move(*root.ack_successor);
			root.ack_successor.reset();
		}
		if (!zone_reset_room_publication_owner::retire_warm(root.original_envelope,
								    root.coordinator_generation))
			return false;
		root.retired = true;
		// The real game-thread world cannot interleave another native callback
		// between the confirmed terminal transfer/retirement and metadata release.
		return release_retired(root);
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
#endif
}

bool zone_reset_item_owner::restore_original(
	const critical_native_recovery_envelope &original) noexcept
{
	// Passive original registration under coordinator_mutex: no reentry, SQL,
	// constructors, RNG/UID issue, invented S cursor or physical publication.
	if (!zone_reset_item_recovery_valid(original))
		return false;
	try
	{
		std::vector<uint8_t> canonical;
		zone_reset_item_image image;
		zone_reset_item_recovery_context context;
		if (critical_command_encode(original.command, &canonical) !=
			    critical_command_codec_result::ok ||
		    zone_reset_item_command_decode(original.command, &image) !=
			    economic_accounting_error::ok ||
		    zone_reset_item_recovery_decode(original.command, original.attachment,
						    &context) != economic_accounting_error::ok)
			return false;
		for (const auto *registry = warm_head_; registry; registry = registry->next)
			for (const auto &root : registry->roots)
				if (root->forest.operation_id.bytes ==
				    original.command.operation_id.bytes)
					return root->canonical_command == canonical &&
					       root->original_envelope.revision ==
						       original.revision &&
					       root->original_envelope.phase == original.phase &&
					       root->original_envelope.attachment ==
						       original.attachment;
		auto registry = std::make_unique<warm_registry>();
		auto root = std::make_unique<warm_root>();
		root->original_envelope = original;
		root->canonical_command = std::move(canonical);
		root->context = std::move(context);
		root->forest.operation_id = image.operation_id;
		root->forest.reset_source = image.reset_source;
		root->forest.zone_vnum = image.zone_vnum;
		root->forest.room_vnum = image.room_vnum;
		root->forest.items = std::move(image.items);
		root->forest.recipes = std::move(image.recipes);
		root->forest.coins = std::move(image.coins);
		root->cold = true;
		root->submitted = true;
		root->sealed = true;
		root->slot = image.reset_source.slot;
		registry->zone_vnum = image.zone_vnum;
		registry->invocation = image.reset_source;
		registry->closed = true;
		// No observed dispatcher_completed/S boundary is fabricated for replay.
		registry->roots.push_back(std::move(root));
		registry->next = warm_head_;
		auto *held = registry.release();
		warm_head_ = held;
		if (!quest_mobile_native_birth_owner::charge())
		{
			warm_head_ = held->next;
			delete held;
			(void)quest_mobile_native_birth_owner::charge();
			return false;
		}
		return true;
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
}

bool zone_reset_room_item_restore(const critical_native_recovery_envelope &original) noexcept
{
	return zone_reset_item_owner::restore_original(original);
}
void zone_reset_room_item_replay_ready(bool ready) noexcept
{
	zone_reset_item_owner::replay_ready_ = ready;
}
void zone_reset_item_owner::completions_original(const critical_completion *incoming,
						 size_t count) noexcept
{
	if (!nevent_is_game_thread() || (!incoming && count))
		return;
	for (size_t at = 0; at < count; ++at)
		for (auto *registry = zone_reset_item_owner::warm_head_; registry;
		     registry = registry->next)
			for (auto &root : registry->roots)
				if (root->submitted && incoming[at].operation_id.bytes ==
							       root->forest.operation_id.bytes)
				{
					if (!critical_completion_disposition_valid(incoming[at]) ||
					    (root->completed &&
					     !same_warm_completion(root->completion,
								   incoming[at]) &&
					     !same_warm_economic_receipt(root->completion,
									 incoming[at])))
					{
						root->blocked = true;
						continue;
					}
					root->completion = incoming[at];
					root->completed = true;
				}
	zone_reset_room_item_pulse(false);
}
void zone_reset_item_owner::pulse_original(bool prepare_original_resets) noexcept
{
	if (!nevent_is_game_thread() || !zone_reset_item_owner::replay_ready_)
		return;
	try
	{
		for (auto *registry = zone_reset_item_owner::warm_head_; registry;
		     registry = registry->next)
		{
			if (registry->sealing_pending &&
			    (!prepare_original_resets ||
			     economic_gameplay_authority::active_sql_recovery() ||
			     !finish_closed_warm_capture(*registry)))
				continue;
			for (auto &owned : registry->roots)
			{
				auto &root = *owned;
				if (root.retired)
				{
					(void)release_retired(root);
					continue;
				}
				if (root.blocked)
					continue;
				if (!root.submitted)
				{
					if (!prepare_original_resets || root.cold ||
					    !registry->closed || !registry->dispatcher_completed ||
					    registry->blocked || registry->sealing_pending ||
					    !root.sealed ||
					    economic_gameplay_authority::active_sql_recovery())
						continue;
					const bool bounded_flat = !persistence_mode_requires_mysql();
					if (bounded_flat && !begin_warm_command_scratch(root))
						continue;
					// Guard is declared first: it outlives this returned envelope and
					// all publication/submission temporaries in the original pulse.
					warm_command_scratch scratch(bounded_flat ? &root : nullptr);
					critical_native_recovery_envelope original;
					scratch.output = &original;
					if (!zone_reset_item_owner::prepare_warm_command(
						    root, &original, scratch) ||
					    (bounded_flat &&
					     !retain_warm_command_output(scratch, original)))
						continue;
					if (zone_reset_room_publication_owner::empty(
						    root.publication))
					{
						if (bounded_flat)
						{
							if (!prepare_warm_publication_flat(
								    root, original, scratch))
								continue;
						}
						else
						{
							std::vector<quest_mobile_native_item_stage *>
								factories;
							factories.reserve(root.forest.items.size());
							for (const auto &item : root.forest.items)
							{
								quest_mobile_native_item_stage
									*factory = nullptr;
								if (root.stage.state_ &&
								    root.stage.state_->object &&
								    root.stage.state_->object
										    ->obj_uid ==
									    item.object_uid)
									factory =
										&root.stage.state_
											 ->factory;
								for (const auto &child :
								     root.children)
									if (child->stage.state_ &&
									    child->stage.state_
										    ->object &&
									    child->stage.state_
											    ->object
											    ->obj_uid ==
										    item.object_uid)
									{
										if (factory)
											throw EILSEQ;
										factory =
											&child->stage
												 .state_
												 ->factory;
									}
								if (!factory)
									throw EILSEQ;
								factories.push_back(factory);
							}
							if (!zone_reset_room_publication_owner::
								    prepare_warm(
									    original.command,
									    factories,
									    &root.placement,
									    &root.publication) ||
							    !quest_mobile_native_birth_owner::
								    charge())
								continue;
						}
					}
					const auto result =
						zone_reset_room_publication_owner::submit_warm(
							std::move(original));
					if (critical_submit_result_keeps_operation(result))
						root.submitted = true;
					else if (result == critical_submit_result::invalid ||
						 result ==
							 critical_submit_result::identity_conflict)
						root.blocked = true;
				}
				if (!root.submitted)
					continue;
				if (!root.coordinator_generation)
					zone_reset_room_publication_owner::generation_warm(
						root.original_envelope,
						&root.coordinator_generation);
				if (!root.completed)
				{
					critical_completion receipt{};
					if (critical_command_coordinator_get_completed(
						    root.original_envelope.command.operation_id,
						    &receipt))
					{
						if (!critical_completion_disposition_valid(receipt))
						{
							root.blocked = true;
							continue;
						}
						root.completion = receipt;
						root.completed = true;
					}
				}
				if (root.completed &&
				    root.completion.disposition ==
					    critical_completion_disposition::never_admitted)
				{
					if (zone_reset_room_publication_owner::cancel_warm(
						    root.original_envelope, root.completion,
						    root.coordinator_generation, cleanup_refusal,
						    &root))
					{
						root.retired = true;
						(void)quest_mobile_native_birth_owner::charge();
					}
				}
				else if (root.completed)
					(void)zone_reset_item_owner::publish_warm(root);
			}
		}
		auto **link = &warm_head_;
		while (*link)
		{
			auto *registry = *link;
			const bool settled =
				registry->closed &&
				std::all_of(
					registry->roots.begin(), registry->roots.end(),
					[](const auto &root)
					{
						return root->retired && !root->stage.state_ &&
						       zone_reset_room_publication_owner::empty(
							       root->publication) &&
						       std::all_of(
							       root->children.begin(),
							       root->children.end(),
							       [](const auto &child)
							       { return !child->stage.state_; });
					}) &&
				std::all_of(registry->unselected.begin(),
					    registry->unselected.end(),
					    [](const auto &child) { return !child->stage.state_; });
			if (!settled)
			{
				link = &registry->next;
				continue;
			}
			if (warm_current_ == registry)
				warm_current_ = nullptr;
			*link = registry->next;
			delete registry;
			(void)quest_mobile_native_birth_owner::charge();
		}
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
	}
}
bool zone_reset_item_owner::lifecycle_original() noexcept
{
	if (!nevent_is_game_thread())
		return false;
	if (!economic_gameplay_authority::active())
		return true;
	for (const auto *registry = warm_head_; registry; registry = registry->next)
	{
		if (registry->sealing_pending)
			return false;
		for (const auto &root : registry->roots)
			if (!root->retired && !root->submitted)
				return false;
		for (const auto &child : registry->unselected)
			if (child->stage.state_)
				return false;
	}
	return true;
}

void zone_reset_room_item_completions(const critical_completion *incoming, size_t count) noexcept
{
	zone_reset_item_owner::completions_original(incoming, count);
}
void zone_reset_room_item_pulse(bool prepare_original_resets) noexcept
{
	zone_reset_item_owner::pulse_original(prepare_original_resets);
}
bool zone_reset_room_item_lifecycle_ready() noexcept
{
	return zone_reset_item_owner::lifecycle_original();
}

bool zone_reset_item_owner::release_retired(warm_root &root) noexcept
{
	if (!root.retired || !nevent_is_game_thread())
		return false;
	if (!zone_reset_room_publication_owner::empty(root.publication) &&
	    !zone_reset_room_publication_owner::release_original_terminal_metadata(
		    root.publication))
		return false;
	// Only metadata from actual released published factories is deleted here.
	if (root.stage.state_)
	{
		if (!root.stage.state_->factory.empty())
			return false;
		delete root.stage.state_;
		root.stage.state_ = nullptr;
	}
	for (auto &child : root.children)
		if (child->stage.state_)
		{
			if (!child->stage.state_->factory.empty())
				return false;
			delete child->stage.state_;
			child->stage.state_ = nullptr;
		}
	return quest_mobile_native_birth_owner::charge();
}
bool zone_reset_item_owner::pending_original(int32_t zone, bool recovery) noexcept
{
	if (!nevent_is_game_thread())
		return true;
	if (!economic_gameplay_authority::active())
		return false;
	if (recovery && !replay_ready_)
		return true;
	for (const auto *registry = warm_head_; registry; registry = registry->next)
		if (zone < 0 || registry->zone_vnum == zone)
		{
			if (!recovery && registry->sealing_pending)
				return true;
			for (const auto &root : registry->roots)
				if (!root->retired && (recovery ? root->submitted : !root->cold))
					return true;
			if (!recovery)
				for (const auto &child : registry->unselected)
					if (child->stage.state_)
						return true;
		}
	return false;
}
bool zone_reset_room_item_warm_pending_vnum(int32_t zone) noexcept
{
	return zone < 0 || zone_reset_item_owner::pending_original(zone, false);
}
bool zone_reset_room_item_recovery_pending() noexcept
{
	return zone_reset_item_owner::pending_original(-1, true);
}

bool zone_reset_item_owner::cleanup_refusal(const critical_command &command,
					    const critical_completion &receipt,
					    void *context) noexcept
{
	if (!context || !nevent_is_game_thread())
		return false;
	auto &root = *static_cast<warm_root *>(context);
	try
	{
		if (!root.completed || !same_warm_completion(root.completion, receipt) ||
		    !critical_command_equal(command, root.original_envelope.command) || root.cold ||
		    root.context.whole_binding.started || root.context.batch_publication.started ||
		    root.context.room_placement.started)
			return false;
		if (root.refusal_cleanup_returned)
			return true;
		if (root.refusal_cleanup_started || !warm_root_current(root))
			return false;
		if (root.refusal_factories.empty())
		{
			root.refusal_factories.reserve(root.children.size() + 1);
			root.refusal_factories.push_back(&root.stage.state_->factory);
			for (const auto &child : root.children)
				if (child->stage.state_ &&
				    child->result == zone_reset_item_warm_result::captured)
					root.refusal_factories.push_back(
						&child->stage.state_->factory);
		}
		if (!quest_mobile_native_birth_owner::charge() ||
		    !zone_reset_room_publication_owner::discard_refused_warm(root.publication))
			return false;
		// The private coordinator pins and reproves the exact NEVER_ADMITTED
		// receipt around this original unadmitted disposal; there is no durable
		// economic frame. No callback, constructor, UID issue or RNG is retried.
		root.refusal_cleanup_started = true;
		for (auto child = root.children.rbegin(); child != root.children.rend(); ++child)
			if ((*child)->stage.state_)
			{
				auto &body = *(*child)->stage.state_;
				quest_mobile_native_item_stage *target = nullptr;
				for (auto *factory : root.refusal_factories)
					if (factory->object() &&
					    factory->object()->obj_uid == (*child)->target_uid)
					{
						if (target)
							return false;
						target = factory;
					}
				if (!target ||
				    !quest_mobile_native_item_stage::detach_room(
					    root.refusal_factories, root.stage.state_->factory,
					    body.factory, *target))
					return false;
				root.refusal_factories.erase(
					std::find(root.refusal_factories.begin(),
						  root.refusal_factories.end(), &body.factory));
				if (!discard_child(&(*child)->stage))
					return false;
				(void)quest_mobile_native_birth_owner::charge();
			}
		root.refusal_factories.clear();
		if (!discard_root(&root.stage))
			return false;
		root.refusal_cleanup_returned = true;
		(void)quest_mobile_native_birth_owner::charge();
		return true;
	}
	catch (...)
	{
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
}

void zone_reset_item_owner::abort_warm_capture() noexcept
{
	economic_source_event actual{};
	int32_t zone_vnum = -1;
	uint32_t stop_slot = 0;
	int last_cmd = 0;
	if (!warm_current_ || warm_current_->closed ||
	    !quest_mobile_native_birth_owner::capture_reset_abort_scope(&actual, &zone_vnum,
									&stop_slot, &last_cmd) ||
	    zone_vnum != warm_current_->zone_vnum ||
	    actual.source.bytes != warm_current_->invocation.source.bytes ||
	    actual.generation.bytes != warm_current_->invocation.generation.bytes ||
	    actual.kind != warm_current_->invocation.kind ||
	    actual.sequence != warm_current_->invocation.sequence)
		return;
	warm_current_->closed = true;
	warm_current_->stop_slot = stop_slot;
	warm_current_->original_last_cmd = last_cmd;
	warm_current_->blocked = true;
	warm_current_->dispatcher_completed = false;
	warm_current_ = nullptr; // Every retained factory stays in its registry.
}
bool zone_reset_item_owner::finish_warm_capture(uint32_t slot, int last_cmd) noexcept
{
	if (!warm_current_ || !warm_scope_current(*warm_current_) ||
	    (last_cmd != 0 && last_cmd != 1))
		return false;
	auto &registry = *warm_current_;
	economic_source_event boundary{};
	int32_t zone_vnum = -1;
	if (!quest_mobile_native_birth_owner::capture_reset_boundary(slot, last_cmd, &boundary,
								     &zone_vnum) ||
	    boundary.source.bytes != registry.invocation.source.bytes ||
	    boundary.generation.bytes != registry.invocation.generation.bytes ||
	    boundary.kind != registry.invocation.kind ||
	    boundary.sequence != registry.invocation.sequence || zone_vnum != registry.zone_vnum ||
	    (registry.has_capture && slot <= registry.last_capture_slot))
		return false;
	registry.dispatcher_completed = true;
	registry.stop_slot = slot;
	registry.original_last_cmd = last_cmd;
	registry.closed = true; // Preserve the boundary even if whole-forest sealing refuses.
	registry.sealing_pending = !registry.blocked;
	warm_current_ = nullptr; // The actual original dispatcher has reached S.
	return finish_closed_warm_capture(registry);
}

bool zone_reset_item_owner::finish_closed_warm_capture(warm_registry &registry) noexcept
{
	if (!nevent_is_game_thread() || !registry.closed || !registry.dispatcher_completed ||
	    registry.blocked || !registry.sealing_pending)
		return false;
	// Do not grow anything until the real current shared retained census permits it.
	if (!quest_mobile_native_birth_owner::charge())
		return false;
	for (const auto &child : registry.unselected)
		if (!child->retired &&
		    (child->result != zone_reset_item_warm_result::load_missed ||
		     !child->stage.state_ || !child->stage.state_->factory.empty()))
		{
			registry.blocked =
				true; // Actual unsupported prefix, never pure retry proof.
			return false;
		}
	for (auto &child : registry.unselected)
	{
		if (child->retired)
			continue;
		if (!discard_child(&child->stage))
		{
			(void)quest_mobile_native_birth_owner::charge();
			return false;
		}
		child->retired = true; // Genuine returned cleanup precedes fallible recensus.
		if (!quest_mobile_native_birth_owner::charge())
			return false;
	}
	for (const auto &root : registry.roots)
	{
		if (root->retired)
			continue;
		if (root->cold || root->submitted || root->completed || root->blocked ||
		    root->context.whole_binding.started ||
		    root->context.batch_publication.started ||
		    root->context.room_placement.started ||
		    !zone_reset_room_publication_owner::empty(root->publication))
		{
			registry.blocked =
				true; // The retry owner has never entered admission/effects.
			return false;
		}
		if (root->stage.state_ &&
		    root->stage.state_->result == zone_reset_item_root_result::load_missed)
		{
			if (!discard_root(&root->stage))
			{
				(void)quest_mobile_native_birth_owner::charge();
				return false;
			}
			root->retired = true;
			if (!quest_mobile_native_birth_owner::charge())
				return false;
			continue;
		}
		if (!seal_warm_root(*root))
		{
			(void)quest_mobile_native_birth_owner::charge();
			return false; // Preserve original forest and real prepared binding progress.
		}
		if (!quest_mobile_native_birth_owner::charge())
			return false; // Stop before another root's preparation/growth.
	}
	size_t retained = 0;
	if (!warm_retained_size(&retained) || !quest_mobile_native_birth_owner::charge())
		return false;
	registry.sealing_pending = false;
	return true; // Pure original preparation, never a new invocation or admission.
}

bool zone_reset_item_owner::observe_warm_forest(const critical_operation_id &operation,
						zone_reset_item_warm_forest_facts *output) noexcept
{
	if (!output || !nevent_is_game_thread() || critical_operation_id_is_zero(operation))
		return false;
	try
	{
		for (const auto *registry = warm_head_; registry; registry = registry->next)
			if (registry->closed && registry->dispatcher_completed &&
			    !registry->blocked && !registry->sealing_pending)
				for (const auto &root : registry->roots)
					if (root->sealed &&
					    root->forest.operation_id.bytes == operation.bytes)
					{
						static_assert(std::is_nothrow_move_assignable_v<
							      zone_reset_item_warm_forest_facts>);
						auto observed = root->forest;
						*output = std::move(observed);
						return true;
					}
	}
	catch (...)
	{
	}
	return false;
}
bool zone_reset_item_owner::warm_retained_size(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
	size_t bytes = 0;
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
	const auto literal = [&](const player_item_snapshot &value)
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
	const auto recipe = [&](const native_mobile_birth_item_recipe &value)
	{ return array(value.libraries.capacity(), sizeof(native_mobile_birth_library_recipe)); };
	const auto binding = [&](const shop_trade_original_procedure_binding_stage &value)
	{
		const size_t retained = value.retained_bytes();
		return retained >= sizeof(value) && add(retained - sizeof(value));
	};
	const auto factory = [&](const quest_mobile_native_item_stage &value)
	{
		if (value.empty())
			return true;
		size_t retained = 0;
		if (!persistence_mode_requires_mysql() && value.is_flat_factory() &&
		    item_native_quest_global_budget_scope_owner::literal_pool_owned())
		{
			if (!value.retained_bytes_excluding_literal_pools(&retained))
				return false;
		}
		else
			retained = value.retained_bytes();
		return retained >= sizeof(value) && add(retained - sizeof(value));
	};
	const auto source_scope = [&](const quest_mobile_native_flat_factory_scope *value)
	{
		// This original holder uses unique_ptr, separate from the independent
		// shared factory and binding copies counted by their own censuses.
		return !value || (add(sizeof(*value)) && (value->root_.capacity() <= 15 ||
							  (value->root_.capacity() != SIZE_MAX &&
							   add(value->root_.capacity() + 1))));
	};
	const auto command = [&](const critical_command &value)
	{
		return array(value.payload.capacity(), 1) &&
		       array(value.accounting_intent.capacity(), 1) &&
		       array(value.keys.capacity(), sizeof(critical_entity_key)) &&
		       array(value.expected_revisions.capacity(),
			     sizeof(critical_expected_revision));
	};
	const auto context = [&](const zone_reset_item_recovery_context &value)
	{
		if (!array(value.items.capacity(), sizeof(zone_reset_item_recovery_item)))
			return false;
		for (const auto &item : value.items)
			if (!array(item.effects.capacity(),
				   sizeof(zone_reset_item_recovery_effect)))
				return false;
		return true;
	};
	const auto envelope = [&](const critical_native_recovery_envelope &value)
	{ return command(value.command) && array(value.attachment.capacity(), 1); };
	const auto writer = [&](const warm_checkpoint *value)
	{
		return !value || (add(sizeof(*value)) && envelope(value->expected) &&
				  envelope(value->successor) && context(value->context));
	};
	const auto child = [&](const warm_child &value)
	{
		if (!add(sizeof(value)))
			return false;
		const auto *body = value.stage.state_;
		return !body || (add(sizeof(*body)) && source_scope(body->flat_scope.get()) &&
				 factory(body->factory) && binding(body->bindings) &&
				 literal(body->facts.literal) && recipe(body->facts.recipe));
	};
	for (const auto *registry = warm_head_; registry; registry = registry->next)
	{
		if (!add(sizeof(*registry)) ||
		    !array(registry->roots.capacity(), sizeof(std::unique_ptr<warm_root>)) ||
		    !array(registry->unselected.capacity(), sizeof(std::unique_ptr<warm_child>)))
			return false;
		for (const auto &owned : registry->unselected)
			if (!child(*owned))
				return false;
		for (const auto &owned : registry->roots)
		{
			const auto &root = *owned;
			if (!add(sizeof(root)) || !add(root.preparation_scratch) || !binding(root.whole_bindings) ||
			    !command(root.original_envelope.command) ||
			    !array(root.original_envelope.attachment.capacity(), 1) ||
			    !array(root.canonical_command.capacity(), 1) ||
			    !array(root.children.capacity(), sizeof(std::unique_ptr<warm_child>)) ||
			    !array(root.forest.items.capacity(), sizeof(player_item_snapshot)) ||
			    !array(root.forest.recipes.capacity(),
				   sizeof(native_mobile_birth_item_recipe)) ||
			    !array(root.forest.coins.capacity(), sizeof(zone_reset_coin_output)))
				return false;
			if (!array(root.refusal_factories.capacity(),
				   sizeof(quest_mobile_native_item_stage *)) ||
			    !context(root.context) || !writer(root.checkpoint.get()))
				return false;
			if (root.ack_successor &&
			    (!add(sizeof(*root.ack_successor)) || !envelope(*root.ack_successor)))
				return false;
			for (const auto &variant : root.returned_writers)
				if (!writer(variant.get()))
					return false;
			size_t publication_bytes = 0;
			if (!zone_reset_room_publication_owner::retained_size_registered_literal_pool(
				    root.publication, &publication_bytes) ||
			    !add(publication_bytes))
				return false;
#ifdef __GLIBCXX__
			if ((root.published.bucket_count() > 1 &&
			     !array(root.published.bucket_count(), sizeof(void *))) ||
			    !array(root.published.size(),
				   sizeof(std::__detail::_Hash_node<uint64_t, false>)))
				return false;
#else
			if (!root.published.empty() || root.published.bucket_count() > 1)
				return false;
#endif
			const auto *body = root.stage.state_;
			if (body && (!add(sizeof(*body)) || !source_scope(body->flat_scope.get()) ||
				     !factory(body->factory) || !binding(body->bindings) ||
				     !literal(body->facts.literal) || !recipe(body->facts.recipe)))
				return false;
			for (const auto &item : root.forest.items)
				if (!literal(item))
					return false;
			for (const auto &item : root.forest.recipes)
				if (!recipe(item))
					return false;
			for (const auto &item : root.children)
				if (!child(*item))
					return false;
		}
	}
	*output = bytes;
	return true; // Local measurement, never a shared coordinator budget reservation.
}

bool zone_reset_item_owner::pending_items(int rnum, size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !obj_index || rnum < 0 || rnum > top_of_objt)
		return false;
	size_t observed = 0;
	const auto count = [&](const quest_mobile_native_item_stage &factory, int original_rnum)
	{
		if (factory.empty())
			return true;
		quest_mobile_native_item_progress progress{};
		if (!factory.read_progress(&progress))
			return false;
		if (progress.published)
			return true; // Already counted by actual obj_index.number.
		const P_obj object = factory.object();
		if (!object || object->R_num != original_rnum ||
		    !factory.owns_pending_original_target(object))
			return false;
		if (original_rnum == rnum)
		{
			if (observed == SIZE_MAX)
				return false;
			++observed;
		}
		return true;
	};
	for (const auto *registry = warm_head_; registry; registry = registry->next)
	{
		for (const auto &root : registry->roots)
		{
			size_t restored = 0;
			if (!zone_reset_room_publication_owner::pending_items(root->publication,
									      rnum, &restored) ||
			    restored > SIZE_MAX - observed)
				return false;
			observed += restored;
			if (root->stage.state_ && !count(root->stage.state_->factory,
							 root->stage.state_->facts.object_rnum))
				return false;
			for (const auto &child : root->children)
				if (child->stage.state_ &&
				    !count(child->stage.state_->factory,
					   child->stage.state_->facts.object_rnum))
					return false;
		}
		for (const auto &child : registry->unselected)
			if (child->stage.state_ && !count(child->stage.state_->factory,
							  child->stage.state_->facts.object_rnum))
				return false;
	}
	*output = observed;
	return true;
}

bool zone_reset_item_owner::prepare_warm_publication_flat(
	warm_root &root, const critical_native_recovery_envelope &original,
	warm_command_scratch &scratch) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    scratch.root != &root || scratch.output != &original ||
	    root.preparation_owner != &scratch || root.cold || root.blocked || root.retired ||
	    !root.sealed || !root.stage.state_ ||
	    !zone_reset_room_publication_owner::empty(root.publication))
		return false;
	try
	{
		warm_registry *invocation = nullptr;
		for (auto *registry = warm_head_; registry; registry = registry->next)
			for (const auto &owned : registry->roots)
				if (owned.get() == &root)
				{
					if (invocation || !registry->closed ||
					    !registry->dispatcher_completed || registry->blocked ||
					    registry->sealing_pending)
						return false;
					invocation = registry;
				}
		const auto &held = *root.stage.state_;
		const auto &source = root.forest.reset_source;
		const char *configured = persistence_mode_flatfile_root();
		if (!invocation || !held.flat_backend || !held.flat_scope || !configured ||
		    !*configured || held.flat_scope->root_ != configured ||
		    !warm_bindings_current(root) || source.kind != invocation->invocation.kind ||
		    source.source.bytes != invocation->invocation.source.bytes ||
		    source.generation.bytes != invocation->invocation.generation.bytes ||
		    source.sequence != invocation->invocation.sequence ||
		    source.slot != root.slot || root.slot >= invocation->stop_slot ||
		    root.forest.zone_vnum != invocation->zone_vnum ||
		    original.command.operation_id.bytes !=
			    root.original_envelope.command.operation_id.bytes ||
		    original.revision != root.original_envelope.revision ||
		    original.phase != root.original_envelope.phase ||
		    original.attachment != root.original_envelope.attachment ||
		    root.canonical_command.empty())
			return false;
		size_t base = scratch.current_bytes(), envelope_heap = 0;
		if (!warm_scratch_envelope_heap(original, false, &envelope_heap) ||
		    !warm_scratch_add(base, envelope_heap))
			return false;
		{
			// Recheck the actual retained projection even on original-command retries.
			size_t projection_live = base;
			if (!warm_scratch_array(projection_live, 2,
						sizeof(critical_operation_id)) ||
			    !reserve_warm_command_scratch(projection_live, &scratch))
				return false;
			critical_operation_id lineage{}, epoch{};
			if (!economic_gameplay_authority::capture_flat_reset_projection(&lineage,
											&epoch) ||
			    lineage.bytes != held.flat_lineage.bytes ||
			    epoch.bytes != held.flat_epoch.bytes)
				return false;
		}
		size_t caller_live = base;
		if (!warm_scratch_add(caller_live,
				      sizeof(std::vector<quest_mobile_native_item_stage *>)) ||
		    !warm_scratch_add(caller_live, sizeof(std::vector<economic_source_event>)) ||
		    !warm_scratch_add(caller_live, sizeof(std::vector<uint8_t>)) ||
		    !warm_scratch_add(caller_live,
				      sizeof(std::span<quest_mobile_native_item_stage *>)) ||
		    !warm_scratch_add(caller_live,
				      sizeof(std::span<const economic_source_event>)) ||
		    !reserve_warm_command_scratch(caller_live, &scratch))
			return false;
		{
			std::vector<quest_mobile_native_item_stage *> factories;
			std::vector<economic_source_event> sources;
			std::vector<uint8_t> canonical;
			if (critical_command_encode_bounded(
				    original.command, &canonical, reserve_warm_command_scratch,
				    &scratch, caller_live) != critical_command_codec_result::ok)
				return false;
			if (canonical != root.canonical_command ||
			    !warm_scratch_add(caller_live, canonical.capacity()) ||
			    !warm_root_current_bounded(root, scratch, caller_live) ||
			    !warm_scratch_array(caller_live, root.forest.items.size(),
						sizeof(quest_mobile_native_item_stage *)) ||
			    !warm_scratch_array(caller_live, root.forest.items.size(),
						sizeof(economic_source_event)) ||
			    !reserve_warm_command_scratch(caller_live, &scratch))
				return false;
			factories.reserve(root.forest.items.size());
			sources.reserve(root.forest.items.size());
			for (const auto &item : root.forest.items)
			{
				quest_mobile_native_item_stage *factory = nullptr;
				const economic_source_event *actual_source = nullptr;
				if (held.object && held.object->obj_uid == item.object_uid)
				{
					factory = &root.stage.state_->factory;
					actual_source = &held.facts.reset_source;
				}
				for (const auto &child : root.children)
					if (child->stage.state_ && child->stage.state_->object &&
					    child->stage.state_->object->obj_uid == item.object_uid)
					{
						const auto &body = *child->stage.state_;
						if (factory ||
						    child->result !=
							    zone_reset_item_warm_result::captured ||
						    child->selected_root_operation.bytes !=
							    held.facts.operation_id.bytes ||
						    !body.flat_backend || !body.flat_scope ||
						    body.flat_scope->root_ !=
							    held.flat_scope->root_ ||
						    !factory_backend_current(
							    body.factory, body.flat_scope.get(),
							    body.facts.reset_scope))
							return false;
						factory = &child->stage.state_->factory;
						actual_source = &body.facts.reset_scope;
					}
				if (!factory || !actual_source)
					return false;
				factories.push_back(factory);
				sources.push_back(*actual_source);
			}
			const std::span<quest_mobile_native_item_stage *> factory_rows{ factories };
			const std::span<const economic_source_event> source_rows{ sources };
			if (!zone_reset_room_publication_owner::prepare_warm_flat_bounded(
				    original.command, held.flat_scope->root_, factory_rows,
				    source_rows, &root.placement, &root.publication,
				    reserve_warm_command_scratch, &scratch, caller_live))
				return false;
		}
		// The candidate and all local vector/span storage have died; publication
		// metadata now belongs to the actual root census, exactly once.
		return rebase_warm_command_scratch(scratch, base);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
bool warm_scratch_context_heap(const zone_reset_item_recovery_context &value, bool fresh,
			       size_t *output) noexcept
{
	size_t bytes = 0;
	if (!output ||
	    !warm_scratch_array(bytes, fresh ? value.items.size() : value.items.capacity(),
				sizeof(zone_reset_item_recovery_item)))
		return false;
	for (const auto &item : value.items)
		if (!warm_scratch_array(bytes,
					fresh ? item.effects.size() : item.effects.capacity(),
					sizeof(zone_reset_item_recovery_effect)))
			return false;
	*output = bytes;
	return true;
}
}

struct zone_reset_item_owner::warm_action_workspace
{
	zone_reset_item_recovery_context started;
	zone_reset_item_recovery_context variant;
	std::unique_ptr<warm_checkpoint> intent;
	std::array<std::unique_ptr<warm_checkpoint>, 4> returned;
};

bool zone_reset_item_owner::warm_checkpoint_storage(const warm_checkpoint *writer,
						    size_t *output) noexcept
{
	if (!output)
		return false;
	size_t bytes = 0, heap = 0;
	if (writer && (!warm_scratch_add(bytes, sizeof(*writer)) ||
		       !warm_scratch_envelope_heap(writer->expected, false, &heap) ||
		       !warm_scratch_add(bytes, heap) ||
		       !warm_scratch_envelope_heap(writer->successor, false, &heap) ||
		       !warm_scratch_add(bytes, heap) ||
		       !warm_scratch_context_heap(writer->context, false, &heap) ||
		       !warm_scratch_add(bytes, heap)))
		return false;
	*output = bytes;
	return true;
}

bool zone_reset_item_owner::make_warm_checkpoint_bounded(
	const critical_native_recovery_envelope &expected,
	const zone_reset_item_recovery_context &next, std::unique_ptr<warm_checkpoint> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || !output || *output || expected.revision == UINT64_MAX ||
	    !nevent_is_game_thread() || persistence_mode_requires_mysql())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)next;
	(void)context;
	(void)outer_live;
	return false;
#else
	try
	{
		size_t live = outer_live, envelope_heap = 0, context_heap = 0;
		if (!warm_scratch_add(live, sizeof(std::unique_ptr<warm_checkpoint>)) ||
		    !warm_scratch_add(live, sizeof(warm_checkpoint)) ||
		    !warm_scratch_envelope_heap(expected, true, &envelope_heap) ||
		    !warm_scratch_add(live, envelope_heap) ||
		    !warm_scratch_add(live, envelope_heap) ||
		    !warm_scratch_context_heap(next, true, &context_heap) ||
		    !warm_scratch_add(live, context_heap) || !reserve(live, context))
			return false;
		auto writer = std::make_unique<warm_checkpoint>();
		writer->expected = expected;
		writer->successor = expected;
		++writer->successor.revision;
		writer->context = next;
		// The copied old attachment coexists with the real fresh codec output.
		if (zone_reset_item_recovery_encode_bounded(
			    writer->successor.command, writer->context,
			    &writer->successor.attachment, reserve, context,
			    live) != economic_accounting_error::ok)
			return false;
		live = outer_live;
		size_t retained = 0;
		if (!warm_scratch_add(live, sizeof(writer)) ||
		    !warm_checkpoint_storage(writer.get(), &retained) ||
		    !warm_scratch_add(live, retained) ||
		    !zone_reset_item_recovery_successor_bounded(writer->expected, writer->successor,
								reserve, context, live))
			return false;
		*output = std::move(writer);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_item_owner::settle_warm_checkpoint_bounded(warm_root &root,
							   bool (*reserve)(size_t, void *) noexcept,
							   void *context,
							   size_t outer_live) noexcept
{
	if (!nevent_is_game_thread() || !reserve || persistence_mode_requires_mysql())
		return false;
	if (!root.checkpoint)
		return true;
	if (!zone_reset_room_publication_owner::checkpoint_warm_bounded(
		    root.checkpoint->expected, root.checkpoint->successor,
		    root.coordinator_generation, reserve, context, outer_live))
		return false;
	// Actual durable CAS has succeeded. Both nonthrowing ownership transfers
	// precede recensus, so a later refusal cannot lose the recorded action.
	static_assert(std::is_nothrow_move_assignable_v<critical_native_recovery_envelope>);
	static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_recovery_context>);
	root.original_envelope = std::move(root.checkpoint->successor);
	root.context = std::move(root.checkpoint->context);
	root.checkpoint.reset();
	return reserve(outer_live, context);
}

bool zone_reset_item_owner::checkpoint_warm_context_bounded(
	warm_root &root, const zone_reset_item_recovery_context &next,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!nevent_is_game_thread() || !reserve || persistence_mode_requires_mysql() ||
	    root.blocked || root.original_envelope.revision == UINT64_MAX)
		return false;
	if (root.checkpoint)
		return settle_warm_checkpoint_bounded(root, reserve, context, outer_live);
	size_t live = outer_live;
	if (!warm_scratch_add(live, sizeof(std::unique_ptr<warm_checkpoint>)) ||
	    !reserve(live, context))
		return false;
	std::unique_ptr<warm_checkpoint> writer;
	if (!make_warm_checkpoint_bounded(root.original_envelope, next, &writer, reserve, context,
					  live))
		return false;
	root.checkpoint = std::move(writer);
	// Its heap now belongs to the actual root census, not a second local charge.
	if (!reserve(live, context))
	{
		root.checkpoint.reset();
		(void)reserve(live, context);
		return false;
	}
	return settle_warm_checkpoint_bounded(root, reserve, context, live);
}

int zone_reset_item_owner::prepare_warm_action_bounded(warm_root &root, uint8_t kind, size_t row,
						       size_t step,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
	if (kind < WARM_BINDING || kind > WARM_ITEM_EFFECT || root.blocked || !reserve ||
	    !settle_warm_checkpoint_bounded(root, reserve, context, outer_live))
		return -1;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)row;
	(void)step;
	return -1;
#else
	try
	{
		// Same original once-only disposition and in-process not-attempted latch.
		size_t live = outer_live, started_heap = 0;
		if (!warm_scratch_add(live, sizeof(quest_mobile_native_item_effect)) ||
		    !reserve(live, context))
			return -1;
		const auto existing = warm_action_value(root.context, kind, row, step);
		if (existing.returned)
			return existing.succeeded ? 0 : -1;
		if (existing.started)
			return root.action_not_attempted && root.pending_action == kind &&
					       root.pending_row == row &&
					       root.pending_step == step ?
				       1 :
				       -1;
		if (root.action_not_attempted || root.original_envelope.revision > UINT64_MAX - 2)
			return -1;
		if (!warm_scratch_add(live, sizeof(warm_action_workspace)) ||
		    !warm_scratch_context_heap(root.context, true, &started_heap) ||
		    !warm_scratch_add(live, started_heap) ||
		    !warm_scratch_add(live, started_heap) ||
		    !warm_scratch_add(live, sizeof(quest_mobile_native_item_effect)) ||
		    !reserve(live, context))
			return -1;
		warm_action_workspace work;
		work.started = root.context;
		set_warm_action(work.started, kind, row, step, { true, false, false, false });
		work.variant = work.started;
		if (!make_warm_checkpoint_bounded(root.original_envelope, work.started,
						  &work.intent, reserve, context, live))
			return -1;
		size_t retained = 0;
		if (!warm_checkpoint_storage(work.intent.get(), &retained) ||
		    !warm_scratch_add(live, retained))
			return -1;
		for (size_t bits = 0; bits < work.returned.size(); ++bits)
		{
			if ((bits & 2) && kind != WARM_ITEM_EFFECT)
				continue;
			// All variants use the original started BODY and are constructed
			// before the intent CAS. Selection after native return never allocates.
			work.variant = work.started;
			set_warm_action(work.variant, kind, row, step,
					{ true, true, bool(bits & 1), bool(bits & 2) });
			if (make_warm_checkpoint_bounded(work.intent->successor, work.variant,
							 &work.returned[bits], reserve, context,
							 live))
			{
				if (!warm_checkpoint_storage(work.returned[bits].get(),
							     &retained) ||
				    !warm_scratch_add(live, retained))
					return -1;
			}
		}
		if (!work.returned[1] && !work.returned[3])
			return -1;
		root.checkpoint = std::move(work.intent);
		root.returned_writers = std::move(work.returned);
		root.pending_action = kind;
		root.pending_row = row;
		root.pending_step = step;
		root.action_not_attempted = true;
		// Actual transferred writers are now counted once by the root registry.
		live = outer_live;
		if (!warm_scratch_add(live, sizeof(existing)) ||
		    !warm_scratch_add(live, sizeof(work)) ||
		    !warm_scratch_add(live, started_heap) ||
		    !warm_scratch_add(live, started_heap) || !reserve(live, context))
		{
			root.checkpoint.reset();
			for (auto &writer : root.returned_writers)
				writer.reset();
			root.action_not_attempted = false;
			(void)reserve(live, context);
			return -1;
		}
		return settle_warm_checkpoint_bounded(root, reserve, context, live) ? 1 : -1;
	}
	catch (...)
	{
		return -1;
	}
#endif
}

bool zone_reset_item_owner::finish_warm_action_bounded(
	warm_root &root, const quest_mobile_native_item_effect &actual,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!actual.started)
		return false;
	root.action_not_attempted = false;
	if (!actual.returned)
	{
		root.blocked = true;
		return false;
	}
	const size_t bits = size_t(actual.succeeded) | (size_t(actual.periodic) << 1);
	if (!root.returned_writers[bits])
	{
		root.blocked = true;
		return false;
	}
	root.checkpoint = std::move(root.returned_writers[bits]);
	for (auto &writer : root.returned_writers)
		writer.reset();
	// Actual return chooses its sealed writer before the first fallible proof,
	// current recensus, codec, callback or I/O, including after service refusal.
	return settle_warm_checkpoint_bounded(root, reserve, context, outer_live) &&
	       actual.succeeded;
}

#include "net/comm.h"
#include "specs/specs.venthix.h"
#include "world/world_activity.h"

bool zone_reset_item_owner::flat_current_global_storage(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
	size_t bytes = 0, current = 0;
	if (!nevent_object_schedule_pool_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current) ||
	    !nevent_native_reschedule_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current) || !diagnostic_output_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current) ||
	    !quest_mobile_native_zombie_registry_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current) ||
	    !item_ownership_runtime_cache_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current) || !world_activity_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current))
		return false;
	*output = bytes;
	return true;
}

size_t zone_reset_item_owner::warm_command_scratch::current_bytes() const noexcept
{
	size_t current = 0, bytes = inline_bytes();
	return root && global_scope && root->preparation_owner == this &&
			       (literal_pool_scope ?
					(item_native_quest_global_budget_scope_owner::
						 literal_pool_owned() &&
					 flat_current_global_storage_with_literal_pools(&current)) :
					flat_current_global_storage(&current)) &&
			       warm_scratch_add(bytes, current) ?
		       bytes :
		       SIZE_MAX;
}

bool zone_reset_item_owner::begin_flat_command_scope(warm_command_scratch &scratch) noexcept
{
	if (!scratch.root || scratch.root->preparation_owner != &scratch)
		return false;
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() || scratch.global_scope ||
	    !item_native_quest_global_budget_scope_owner::begin(&scratch,
								flat_current_global_storage))
	{
		scratch.root->preparation_owner = nullptr;
		scratch.root->preparation_scratch = 0;
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
	scratch.global_scope = true;
	size_t current = 0, live = warm_command_scratch::inline_bytes();
	if (flat_current_global_storage(&current) && warm_scratch_add(live, current))
	{
		scratch.root->preparation_scratch = live;
		if (quest_mobile_native_birth_owner::charge())
			return true;
	}
	// Fixed guard was admitted before construction. Failed first current-global
	// admission cannot proceed to any provider allocation or native callback.
	// Remove root scalar scratch before ending exactly this scope, then recensus
	// CURRENT persistent globals outside it. No fallible callback between changes.
	scratch.root->preparation_owner = nullptr;
	scratch.root->preparation_scratch = 0;
	(void)item_native_quest_global_budget_scope_owner::end(&scratch);
	scratch.global_scope = false;
	(void)quest_mobile_native_birth_owner::charge();
	return false;
}

bool zone_reset_item_owner::flat_current_global_storage_with_literal_pools(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t bytes = 0, current = 0;
	if (!flat_current_global_storage(&bytes) ||
	    !native_mobile_birth_literal_pool_storage_bytes(&current) ||
	    !warm_scratch_add(bytes, current))
		return false;
	*output = bytes;
	return true;
}

bool zone_reset_item_owner::begin_full_flat_command_scope(warm_command_scratch &scratch) noexcept
{
	if (!scratch.root || scratch.root->preparation_owner != &scratch ||
	    !scratch.literal_pool_scope)
		return false;
	// The first genuine registration selects this complete immutable policy.
	// Native private stages use their paired census only after actual registration.
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() || scratch.global_scope ||
	    !item_native_quest_global_budget_scope_owner::begin(
		    &scratch, flat_current_global_storage_with_literal_pools, true))
	{
		scratch.root->preparation_owner = nullptr;
		scratch.root->preparation_scratch = 0;
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
	scratch.global_scope = true;
	size_t current = 0, live = warm_command_scratch::inline_bytes();
	// This private candidate runs only after startup. Genuine scope identity
	// selects its outside observer BEFORE the first shared charge; no selected
	// six-owner caller or replay callback registers the locking observer.
	if (item_native_quest_coordinator_budget_scope_owner::register_observer(
		    &scratch, zone_reset_room_publication_owner::current_coordinator_storage,
		    reserve_warm_command_scratch) &&
	    flat_current_global_storage_with_literal_pools(&current) &&
	    warm_scratch_add(live, current))
	{
		scratch.root->preparation_scratch = live;
		if (quest_mobile_native_birth_owner::charge())
			return true;
	}
	// No allocating or fallible callback between dropping the scalar and ending
	// this scope. Its registered observer remains CURRENT outside the guard.
	scratch.root->preparation_owner = nullptr;
	scratch.root->preparation_scratch = 0;
	(void)item_native_quest_global_budget_scope_owner::end(&scratch);
	scratch.global_scope = false;
	(void)quest_mobile_native_birth_owner::charge();
	return false;
}

bool zone_reset_item_owner::restore_cold_flat_bindings(
	const std::span<quest_mobile_native_item_stage *> &factories, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept
{
	if (!original_context || !reserve || !nevent_is_game_thread() ||
	    persistence_mode_requires_mysql())
		return false;
	auto &root = *static_cast<warm_root *>(original_context);
	if (!root.cold || !root.submitted || !root.completed || root.blocked ||
	    factories.size() != root.forest.items.size())
		return false;
	if (root.cold_binding_restored)
		return true;
	try
	{
		if (!root.cold_bindings_prepared)
		{
			struct input_workspace
			{
				std::vector<quest_mobile_native_item_binding> inputs;
				std::span<const quest_mobile_native_item_binding> view;
			};
			size_t live = outer_live;
			if (!warm_scratch_add(live, sizeof(input_workspace)) ||
			    !warm_scratch_add(live, sizeof(quest_mobile_native_item_binding)) ||
			    !warm_scratch_array(live, factories.size(),
						sizeof(quest_mobile_native_item_binding)) ||
			    !reserve(live, budget_context))
				return false;
			input_workspace work;
			work.inputs.reserve(factories.size());
			for (size_t at = 0; at < factories.size(); ++at)
			{
				const auto *factory = factories[at];
				if (!factory || !factory->object() ||
				    factory->object()->obj_uid != root.forest.items[at].object_uid)
					return false;
				work.inputs.push_back(factory->binding_input());
			}
			work.view = work.inputs;
			const bool prepared = shop_trade_original_procedure_binding_stage::
				prepare_native_birth_cold_flat_bounded(work.view,
								       root.whole_bindings, reserve,
								       budget_context, live);
			if (prepared)
				root.cold_bindings_prepared = true;
			if (!reserve(live, budget_context) || !prepared)
				return false;
		}
		if (!root.whole_bindings.valid_flat())
			return false;
		// Only authentic recorded success rebuilds allocation-free actual bindings.
		// Genuine unstarted action remains staged for the real intent/return driver.
		if (root.context.whole_binding.succeeded)
		{
			root.whole_bindings.commit_flat_unchecked();
			root.cold_binding_restored = true;
		}
		return reserve(outer_live, budget_context);
	}
	catch (...)
	{
		(void)reserve(outer_live, budget_context);
		return false;
	}
}

struct zone_reset_item_owner::cold_registration_workspace
{
	std::vector<uint8_t> canonical;
	zone_reset_item_image image;
	zone_reset_item_recovery_context recovery;
	std::span<const uint8_t> attachment;
	std::unique_ptr<warm_registry> registry;
	std::unique_ptr<warm_root> root;
};

bool zone_reset_item_owner::restore_original_bounded(
	const critical_native_recovery_envelope &original, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	// Genuine coordinator owns mutex/admission. NO coordinator reentry, pulse
	// charge, SQL, factory construction, RNG/UID issue, invented S witness or
	// physical publication occurs in this passive metadata registration.
	if (!reserve ||
	    !zone_reset_item_recovery_valid_bounded(original, reserve, context, outer_live))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	try
	{
		size_t live = outer_live;
		if (!warm_scratch_add(live, sizeof(cold_registration_workspace)) ||
		    !reserve(live, context))
			return false;
		cold_registration_workspace work;
		if (critical_command_encode_bounded(original.command, &work.canonical, reserve,
						    context,
						    live) != critical_command_codec_result::ok ||
		    !warm_scratch_add(live, work.canonical.capacity()))
			return false;
		size_t image_heap = 0, recovery_heap = 0;
		if (zone_reset_item_command_decode_bounded(original.command, &work.image, reserve,
							   context,
							   live) != economic_accounting_error::ok ||
		    !warm_scratch_forest_heap(work.image.items, work.image.recipes,
					      work.image.coins, false, &image_heap) ||
		    !warm_scratch_add(live, image_heap))
			return false;
		work.attachment = original.attachment;
		if (zone_reset_item_recovery_decode_bounded(
			    original.command, work.attachment, &work.recovery, reserve, context,
			    live, &recovery_heap) != economic_accounting_error::ok ||
		    !warm_scratch_add(live, recovery_heap))
			return false;
		for (const auto *registry = warm_head_; registry; registry = registry->next)
			for (const auto &root : registry->roots)
				if (root->forest.operation_id.bytes ==
				    original.command.operation_id.bytes)
					return root->canonical_command == work.canonical &&
					       root->original_envelope.revision ==
						       original.revision &&
					       root->original_envelope.phase == original.phase &&
					       root->original_envelope.attachment ==
						       original.attachment;
		if (!warm_scratch_add(live, sizeof(warm_registry)) || !reserve(live, context))
			return false;
		work.registry = std::make_unique<warm_registry>();
		if (!warm_scratch_add(live, sizeof(warm_root)) || !reserve(live, context))
			return false;
		work.root = std::make_unique<warm_root>();
		size_t envelope_heap = 0;
		if (!warm_scratch_add(live, sizeof(critical_native_recovery_envelope)) ||
		    !warm_scratch_envelope_heap(original, true, &envelope_heap) ||
		    !warm_scratch_add(live, envelope_heap) || !reserve(live, context))
			return false;
		critical_native_recovery_envelope retained(original);
		work.root->original_envelope = std::move(retained);
		work.root->canonical_command = std::move(work.canonical);
		work.root->context = std::move(work.recovery);
		work.root->forest.operation_id = work.image.operation_id;
		work.root->forest.reset_source = work.image.reset_source;
		work.root->forest.zone_vnum = work.image.zone_vnum;
		work.root->forest.room_vnum = work.image.room_vnum;
		work.root->forest.items = std::move(work.image.items);
		work.root->forest.recipes = std::move(work.image.recipes);
		work.root->forest.coins = std::move(work.image.coins);
		work.root->cold = true;
		work.root->submitted = true;
		work.root->sealed = true;
		work.root->slot = work.image.reset_source.slot;
		work.registry->zone_vnum = work.image.zone_vnum;
		work.registry->invocation = work.image.reset_source;
		work.registry->closed = true;
		// Preserve original rooted inline-string census allowance before attachment.
		// Actual large string heap is already counted; only embedded <=15-byte text
		// adds this original conservative field allowance at the ownership cut.
		for (const auto &item : work.root->forest.items)
		{
			const auto inline_text = [&](const std::string &text) noexcept {
				return text.capacity() > 15 ||
				       warm_scratch_add(live, text.capacity() + 1);
			};
			if (!inline_text(item.name) || !inline_text(item.short_description) ||
			    !inline_text(item.description) || !inline_text(item.action_description))
				return false;
			for (const auto &description : item.extra_descriptions)
				if (!inline_text(description.keyword) ||
				    !inline_text(description.description))
					return false;
		}
		// Exact genuine one-root vector growth; no dispatcher_completed/S fabricated.
		if (!warm_scratch_add(live, sizeof(std::unique_ptr<warm_root>)) ||
		    !reserve(live, context))
			return false;
		work.registry->roots.push_back(std::move(work.root));
		// All real storage is admitted before this allocation-free ownership transfer.
		// Caller refreshes actual native registry retention on every returned cut;
		// no outside pulse charge or allocating/fallible callback follows attachment.
		work.registry->next = warm_head_;
		warm_head_ = work.registry.release();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_room_item_restore_bounded(const critical_native_recovery_envelope &original,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer_live) noexcept
{
	return zone_reset_item_owner::restore_original_bounded(original, reserve, context,
							       outer_live);
}

bool zone_reset_item_owner::begin_submitted_flat_scratch(warm_root &root) noexcept
{
	if (!root.cold && !root.refusal_cleanup_returned)
		return begin_warm_command_scratch(root);
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    root.preparation_owner || root.preparation_scratch || !root.submitted || !root.sealed ||
	    root.blocked || root.retired || root.canonical_command.empty() ||
	    root.original_envelope.command.operation_id.bytes != root.forest.operation_id.bytes)
		return false;
	if (!root.cold &&
	    (!root.completed ||
	     root.completion.disposition != critical_completion_disposition::never_admitted ||
	     root.stage.state_ || !zone_reset_room_publication_owner::empty(root.publication)))
		return false;
	warm_registry *actual = nullptr;
	for (auto *registry = warm_head_; registry; registry = registry->next)
		for (const auto &owned : registry->roots)
			if (owned.get() == &root)
			{
				if (actual || !registry->closed || registry->blocked ||
				    registry->sealing_pending)
					return false;
				actual = registry;
			}
	// Passive cold replay carries NO captured live O/P or S/dispatcher witness.
	// A genuinely completed no-admission cleanup instead retains its authentic
	// warm source/closed S boundary, plus emitted destruction-return latch, for
	// coordinator removal only. Neither branch fabricates a factory or old body.
	// Full carrier/receipt/generation are checked by the owning actual driver.
	const auto &source = root.forest.reset_source;
	if (!actual || source.source.bytes != actual->invocation.source.bytes ||
	    source.generation.bytes != actual->invocation.generation.bytes ||
	    source.kind != actual->invocation.kind ||
	    source.sequence != actual->invocation.sequence || source.slot != root.slot ||
	    root.forest.zone_vnum != actual->zone_vnum ||
	    (root.cold ? source.slot != actual->invocation.slot :
			 (!actual->dispatcher_completed || root.slot >= actual->stop_slot)))
		return false;
	root.preparation_scratch = warm_command_scratch::inline_bytes();
	if (!quest_mobile_native_birth_owner::charge())
	{
		root.preparation_scratch = 0;
		return false;
	}
	if (!economic_gameplay_authority::active_regular_flat())
	{
		root.preparation_scratch = 0;
		(void)quest_mobile_native_birth_owner::charge();
		return false;
	}
	return true;
#endif
}

bool zone_reset_item_owner::prepare_flat_ack_successor_bounded(
	warm_root &root, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!reserve || !root.submitted || !root.completed || root.blocked ||
	    root.original_envelope.phase != critical_native_recovery_phase::execution_pending ||
	    root.original_envelope.revision == UINT64_MAX)
		return false;
	if (root.ack_successor)
		return zone_reset_item_recovery_successor_bounded(
			root.original_envelope, *root.ack_successor, reserve, context, outer_live);
	try
	{
		size_t live = outer_live, heap = 0;
		if (!warm_scratch_add(live,
				      sizeof(std::unique_ptr<critical_native_recovery_envelope>)) ||
		    !warm_scratch_add(live, sizeof(critical_native_recovery_envelope)) ||
		    !warm_scratch_envelope_heap(root.original_envelope, true, &heap) ||
		    !warm_scratch_add(live, heap) || !reserve(live, context))
			return false;
		auto successor =
			std::make_unique<critical_native_recovery_envelope>(root.original_envelope);
		++successor->revision;
		successor->phase = critical_native_recovery_phase::continuation_pending;
		// Original full monotonic BODY/receipt/state predicate; no synthetic phase ACK.
		if (!zone_reset_item_recovery_successor_bounded(root.original_envelope, *successor,
								reserve, context, live))
			return false;
		root.ack_successor = std::move(successor);
		// Actual root now owns clone once. Only live unique_ptr frame remains in outer;
		// a refusal retains original exact ACK successor for normal retry.
		live = outer_live;
		return warm_scratch_add(live, sizeof(successor)) && reserve(live, context);
	}
	catch (...)
	{
		return false;
	}
}

bool zone_reset_item_owner::observe_completed_flat_bounded(warm_root &root,
							   warm_command_scratch &scratch,
							   size_t outer_live) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    scratch.root != &root || !scratch.output || root.preparation_owner != &scratch ||
	    !scratch.global_scope || !root.submitted || root.blocked || root.retired)
		return false;
	size_t entry = scratch.current_bytes(), entry_heap = 0;
	if (!warm_scratch_envelope_heap(*scratch.output, false, &entry_heap) ||
	    !warm_scratch_add(entry, entry_heap) || outer_live < entry)
		return false;
	const size_t caller_extra = outer_live - entry;
	if (!root.coordinator_generation &&
	    !zone_reset_room_publication_owner::generation_warm_bounded(
		    root.original_envelope, &root.coordinator_generation,
		    reserve_warm_command_scratch, &scratch, outer_live))
		return false;
	if (root.completed)
		return true;
	size_t live = outer_live;
	if (!warm_scratch_add(live, sizeof(critical_completion)) ||
	    !reserve_warm_command_scratch(live, &scratch))
		return false;
	critical_completion receipt{};
	const bool available = zone_reset_room_publication_owner::completion_warm_bounded(
		root.original_envelope.command.operation_id, &receipt, reserve_warm_command_scratch,
		&scratch, live);
	if (available)
	{
		if (!critical_completion_disposition_valid(receipt))
			root.blocked = true;
		else
		{
			root.completion = receipt;
			root.completed =
				true; // Actual available receipt retained before later census.
		}
	}
	size_t current = scratch.current_bytes(), heap = 0;
	if (warm_scratch_envelope_heap(*scratch.output, false, &heap) &&
	    warm_scratch_add(current, heap) && warm_scratch_add(current, caller_extra) &&
	    warm_scratch_add(current, sizeof(receipt)))
		(void)rebase_warm_command_scratch(scratch, current);
	return root.completed && !root.blocked;
}

bool zone_reset_item_owner::cleanup_refusal_bounded(
	const critical_command &command, const critical_completion &receipt, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept
{
	if (!original_context || !reserve || !nevent_is_game_thread() ||
	    persistence_mode_requires_mysql())
		return false;
	auto &root = *static_cast<warm_root *>(original_context);
	auto *scratch = static_cast<warm_command_scratch *>(budget_context);
	if (!scratch || scratch->root != &root || root.preparation_owner != scratch ||
	    !scratch->global_scope)
		return false;
	try
	{
		if (!root.completed || !same_warm_completion(root.completion, receipt) ||
		    root.cold || root.context.whole_binding.started ||
		    root.context.batch_publication.started || root.context.room_placement.started)
			return false;
		size_t live = outer_live;
		if (!warm_scratch_add(live, sizeof(std::vector<uint8_t>)) ||
		    !warm_scratch_add(live, sizeof(std::span<quest_mobile_native_item_stage *>)) ||
		    !reserve(live, budget_context))
			return false;
		std::vector<uint8_t> canonical;
		if (critical_command_encode_bounded(command, &canonical, reserve, budget_context,
						    live) != critical_command_codec_result::ok ||
		    canonical != root.canonical_command ||
		    !warm_scratch_add(live, canonical.capacity()))
			return false;
		if (root.refusal_cleanup_returned)
			return true;
		if (root.refusal_cleanup_started ||
		    !warm_root_current_bounded(root, *scratch, live))
			return false;
		if (root.refusal_factories.empty())
		{
			size_t request = live;
			if (root.children.size() == SIZE_MAX ||
			    (root.children.size() + 1 > root.refusal_factories.capacity() &&
			     !warm_scratch_array(request, root.children.size() + 1,
						 sizeof(quest_mobile_native_item_stage *))) ||
			    !reserve(request, budget_context))
				return false;
			root.refusal_factories.reserve(root.children.size() + 1);
			root.refusal_factories.push_back(&root.stage.state_->factory);
			for (const auto &child : root.children)
				if (child->stage.state_ &&
				    child->result == zone_reset_item_warm_result::captured)
					root.refusal_factories.push_back(
						&child->stage.state_->factory);
		}
		if (!reserve(live, budget_context) ||
		    !zone_reset_room_publication_owner::discard_refused_warm(root.publication))
			return false;
		// Original coordinator pins/reproves exact never-admitted delivery. This is
		// the original unadmitted disposal; no durable economic action is replayed.
		root.refusal_cleanup_started = true;
		for (auto child = root.children.rbegin(); child != root.children.rend(); ++child)
			if ((*child)->stage.state_)
			{
				auto &body = *(*child)->stage.state_;
				quest_mobile_native_item_stage *target = nullptr;
				for (auto *factory : root.refusal_factories)
					if (factory->object() &&
					    factory->object()->obj_uid == (*child)->target_uid)
					{
						if (target)
							return false;
						target = factory;
					}
				std::span<quest_mobile_native_item_stage *> factories(
					root.refusal_factories);
				if (!target ||
				    !quest_mobile_native_item_stage::detach_room_bounded(
					    factories, root.stage.state_->factory, body.factory,
					    *target, reserve, budget_context, live))
					return false;
				root.refusal_factories.erase(
					std::find(root.refusal_factories.begin(),
						  root.refusal_factories.end(), &body.factory));
				if (!discard_child(&(*child)->stage))
					return false;
				(void)reserve(live, budget_context);
			}
		root.refusal_factories.clear();
		if (!discard_root(&root.stage))
			return false;
		root.refusal_cleanup_returned =
			true; // Actual destruction success BEFORE any further refusal.
		(void)reserve(live, budget_context);
		return true; // A census refusal cannot relabel completed destruction as false.
	}
	catch (...)
	{
		(void)reserve(outer_live, budget_context);
		return false;
	}
}

struct zone_reset_item_owner::flat_refusal_workspace
{
	warm_root &root;
	warm_command_scratch &scratch;
	bool cleanup_called = false, cleanup_succeeded = false;
};

bool zone_reset_item_owner::cleanup_refusal_flat_relay(const critical_command &command,
						       const critical_completion &receipt,
						       void *opaque,
						       size_t coordinator_live) noexcept
{
	auto *work = static_cast<flat_refusal_workspace *>(opaque);
	return work &&
	       cleanup_refusal_bounded(command, receipt, &work->root, reserve_warm_command_scratch,
				       &work->scratch, coordinator_live);
}

bool zone_reset_item_owner::cancel_refused_flat_bounded(warm_root &root,
							warm_command_scratch &scratch,
							size_t outer_live) noexcept
{
	if (scratch.root != &root || !scratch.output || root.preparation_owner != &scratch ||
	    !scratch.global_scope || !root.submitted || !root.completed || root.blocked ||
	    root.retired ||
	    root.completion.disposition != critical_completion_disposition::never_admitted)
		return false;
	size_t entry = scratch.current_bytes(), entry_heap = 0;
	if (!warm_scratch_envelope_heap(*scratch.output, false, &entry_heap) ||
	    !warm_scratch_add(entry, entry_heap) || outer_live < entry)
		return false;
	const size_t caller_extra = outer_live - entry;
	size_t live = outer_live;
	if (!warm_scratch_add(live, sizeof(flat_refusal_workspace)) ||
	    !reserve_warm_command_scratch(live, &scratch))
		return false;
	flat_refusal_workspace work{ root, scratch };
	const bool removed = zone_reset_room_publication_owner::cancel_warm_bounded(
		root.original_envelope, root.completion, root.coordinator_generation,
		cleanup_refusal_flat_relay, &work, reserve_warm_command_scratch, &scratch, live,
		&work.cleanup_called, &work.cleanup_succeeded);
	// Actual disposal and actual coordinator removal are distinct returned facts.
	// Removal refusal cannot repeat a cleanup whose native effect already returned.
	if (work.cleanup_called && work.cleanup_succeeded)
		root.refusal_cleanup_returned = true;
	if (removed)
		root.retired = true;
	// The retained root/guard outlive cleanup; native stage pointers may be gone.
	// Refresh all actual globals before any fallible proof or next pulse handoff.
	size_t current = scratch.current_bytes(), heap = 0;
	if (!warm_scratch_envelope_heap(*scratch.output, false, &heap) ||
	    !warm_scratch_add(current, heap) || !warm_scratch_add(current, caller_extra) ||
	    !warm_scratch_add(current, sizeof(work)))
		return removed;
	(void)rebase_warm_command_scratch(scratch, current);
	return removed;
}

struct zone_reset_item_owner::flat_publication_workspace
{
	critical_native_recovery_envelope &current;
	size_t caller_extra = 0;
	explicit flat_publication_workspace(critical_native_recovery_envelope &output,
					    size_t extra) noexcept
		: current(output)
		, caller_extra(extra)
	{
	}
	std::vector<uint8_t> canonical;
	zone_reset_item_recovery_context next;
	std::span<const uint8_t> attachment;
	quest_mobile_native_item_effect actual{};
	std::string selected_root;
	uint64_t generation = 0;
	bool current_live(const warm_command_scratch &scratch, const flatfile_authority_lock *lock,
			  size_t *output) const noexcept
	{
		if (!output)
			return false;
		// Guard fixed bytes already contain the actual single output carrier inline.
		size_t live = scratch.current_bytes(), heap = 0, lock_heap = 0;
		if (!warm_scratch_add(live, caller_extra) ||
		    !warm_scratch_add(live, sizeof(*this)) ||
		    !warm_scratch_envelope_heap(current, false, &heap) ||
		    !warm_scratch_add(live, heap) ||
		    !warm_scratch_add(live, canonical.capacity()) ||
		    !warm_scratch_context_heap(next, false, &heap) ||
		    !warm_scratch_add(live, heap) ||
		    (selected_root.capacity() > 15 &&
		     (selected_root.capacity() == SIZE_MAX ||
		      !warm_scratch_add(live, selected_root.capacity() + 1))) ||
		    (lock &&
		     (!lock->retained_bytes(&lock_heap) || !warm_scratch_add(live, lock_heap))))
			return false;
		*output = live;
		return true;
	}
};

bool zone_reset_item_owner::refresh_flat_publication_scratch(warm_command_scratch &scratch,
							     const flat_publication_workspace &work,
							     const flatfile_authority_lock *lock,
							     size_t *output) noexcept
{
	size_t live = 0;
	if (!work.current_live(scratch, lock, &live) || !rebase_warm_command_scratch(scratch, live))
		return false;
	if (output)
		*output = live;
	return true;
}

bool zone_reset_item_owner::copy_flat_next_context(warm_root &root,
						   flat_publication_workspace &work,
						   warm_command_scratch &scratch,
						   const flatfile_authority_lock *lock) noexcept
{
	size_t live = 0, heap = 0;
	if (!work.current_live(scratch, lock, &live) ||
	    !warm_scratch_add(live, sizeof(zone_reset_item_recovery_context)) ||
	    !warm_scratch_context_heap(root.context, true, &heap) ||
	    !warm_scratch_add(live, heap) || !reserve_warm_command_scratch(live, &scratch))
		return false;
	try
	{
		{
			zone_reset_item_recovery_context next(root.context);
			work.next = std::move(next);
		}
		// The admitted moved-from temporary has died before dropping its inline bytes.
		return refresh_flat_publication_scratch(scratch, work, lock, nullptr);
	}
	catch (...)
	{
		(void)refresh_flat_publication_scratch(scratch, work, lock, nullptr);
		return false;
	}
}

bool zone_reset_item_owner::finish_flat_publication_action(
	warm_root &root, flat_publication_workspace &work, warm_command_scratch &scratch,
	const flatfile_authority_lock &lock) noexcept
{
	const auto &actual = work.actual;
	if (!actual.started)
		return false;
	root.action_not_attempted = false;
	if (!actual.returned)
	{
		root.blocked = true;
		return false;
	}
	const size_t bits = size_t(actual.succeeded) | (size_t(actual.periodic) << 1);
	if (!root.returned_writers[bits])
	{
		root.blocked = true;
		return false;
	}
	root.checkpoint = std::move(root.returned_writers[bits]);
	for (auto &writer : root.returned_writers)
		writer.reset();
	// The real returned variant is rooted before any fallible current observation,
	// budget/proof, codec or I/O. A later refusal cannot repeat this native effect.
	size_t live = 0;
	return refresh_flat_publication_scratch(scratch, work, &lock, &live) &&
	       settle_warm_checkpoint_bounded(root, reserve_warm_command_scratch, &scratch, live) &&
	       actual.succeeded;
}

bool zone_reset_item_owner::publish_flat_bounded(warm_root &root, warm_command_scratch &scratch,
						 size_t outer_live) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() || !root.submitted ||
	    !root.completed || root.blocked || !root.coordinator_generation ||
	    !successful_warm_receipt(root.completion) || !scratch.root || scratch.root != &root ||
	    !scratch.global_scope || !scratch.literal_pool_scope || !scratch.output ||
	    root.preparation_owner != &scratch ||
	    !item_native_quest_global_budget_scope_owner::literal_pool_owned())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = scratch.current_bytes(), output_heap = 0;
	if (!warm_scratch_envelope_heap(*scratch.output, false, &output_heap) ||
	    !warm_scratch_add(live, output_heap) || outer_live < live)
		return false;
	const size_t caller_extra = outer_live - live;
	live = outer_live;
	if (!warm_scratch_add(live, sizeof(flat_publication_workspace)) ||
	    !reserve_warm_command_scratch(live, &scratch))
		return false;
	// Preserve ALL other caller frames/capacities on every CURRENT rebase. This
	// does not claim ownership of the root-owned coordinator/journal handoff.
	flat_publication_workspace work(*scratch.output, caller_extra);
	try
	{
		if (!settle_warm_checkpoint_bounded(root, reserve_warm_command_scratch, &scratch,
						    live) ||
		    !refresh_flat_publication_scratch(scratch, work, nullptr, &live) ||
		    !zone_reset_room_publication_owner::copy_warm_bounded(
			    root.original_envelope.command, &work.current,
			    reserve_warm_command_scratch, &scratch, live) ||
		    !refresh_flat_publication_scratch(scratch, work, nullptr, &live) ||
		    critical_command_encode_bounded(work.current.command, &work.canonical,
						    reserve_warm_command_scratch, &scratch,
						    live) != critical_command_codec_result::ok ||
		    work.canonical != root.canonical_command ||
		    work.current.revision != root.original_envelope.revision ||
		    work.current.phase != root.original_envelope.phase ||
		    work.current.attachment != root.original_envelope.attachment ||
		    !refresh_flat_publication_scratch(scratch, work, nullptr, &live) ||
		    !zone_reset_room_publication_owner::generation_warm_bounded(
			    work.current, &work.generation, reserve_warm_command_scratch, &scratch,
			    live) ||
		    work.generation != root.coordinator_generation)
			return false;
		if (root.context.items.empty())
		{
			work.attachment = work.current.attachment;
			if (!refresh_flat_publication_scratch(scratch, work, nullptr, &live) ||
			    zone_reset_item_recovery_decode_bounded(
				    work.current.command, work.attachment, &root.context,
				    reserve_warm_command_scratch, &scratch,
				    live) != economic_accounting_error::ok ||
			    !refresh_flat_publication_scratch(scratch, work, nullptr, &live))
				return false;
		}
		if (!root.context.receipt_present ||
		    !same_warm_completion(root.context.receipt, root.completion))
		{
			if (root.context.receipt_present &&
			    !same_warm_economic_receipt(root.context.receipt, root.completion))
				return false;
			if (work.current.phase ==
			    critical_native_recovery_phase::continuation_pending)
			{
				if (!root.context.receipt_present ||
				    !refresh_flat_publication_scratch(scratch, work, nullptr,
								      &live) ||
				    !zone_reset_item_recovery_publication_bounded(
					    work.current, root.completion,
					    reserve_warm_command_scratch, &scratch, live))
					return false;
			}
			else
			{
				if (!copy_flat_next_context(root, work, scratch, nullptr))
					return false;
				work.next.receipt_present = true;
				work.next.receipt = root.completion;
				if (work.next.stage == zone_reset_item_recovery_stage::captured)
					work.next.stage =
						zone_reset_item_recovery_stage::publishing;
				if (!refresh_flat_publication_scratch(scratch, work, nullptr,
								      &live) ||
				    !checkpoint_warm_context_bounded(root, work.next,
								     reserve_warm_command_scratch,
								     &scratch, live))
					return false;
			}
		}
		const char *configured = persistence_mode_flatfile_root();
		if (!configured || !*configured)
			return false;
		if (!root.cold)
		{
			if (!root.stage.state_ || !root.stage.state_->flat_backend ||
			    !root.stage.state_->flat_scope ||
			    root.stage.state_->flat_scope->root_ != configured)
				return false;
			configured = root.stage.state_->flat_scope->root_.c_str();
		}
		if (!refresh_flat_publication_scratch(scratch, work, nullptr, &live))
			return false;
		const size_t chars = std::char_traits<char>::length(configured);
		if (!warm_scratch_add(live, sizeof(std::string)) ||
		    (chars > 15 && (chars == SIZE_MAX || !warm_scratch_add(live, chars + 1))) ||
		    !reserve_warm_command_scratch(live, &scratch))
			return false;
		{
			std::string selected(configured);
			work.selected_root = std::move(selected);
		}
		if (!refresh_flat_publication_scratch(scratch, work, nullptr, &live) ||
		    !warm_scratch_add(live, sizeof(flatfile_authority_lock)) ||
		    !reserve_warm_command_scratch(live, &scratch))
			return false;
		flatfile_authority_lock lock(reserve_warm_command_scratch, &scratch,
					     live - sizeof(flatfile_authority_lock));
		if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
		    !lock.acquire_bounded(work.selected_root, reserve_warm_command_scratch,
					  &scratch, live) ||
		    !refresh_flat_publication_scratch(scratch, work, &lock, &live))
			return false;
		const auto recovered = flatfile_authority_transaction_recover_bounded(
			work.selected_root, lock, reserve_warm_command_scratch, &scratch, live);
		if ((recovered != flatfile_authority_transaction_result::ok &&
		     recovered != flatfile_authority_transaction_result::not_found) ||
		    !refresh_flat_publication_scratch(scratch, work, &lock, &live))
			return false;
		if (root.cold)
		{
			// Exact original four-way dispatch. A failed partial actual adoption never
			// becomes permission to replace a live body in the later missing routes.
			bool ready = zone_reset_room_publication_owner::
				prepare_original_completed_flat_locked_bounded(
					work.selected_root, lock, root.original_envelope,
					root.completion, root.publication,
					reserve_warm_command_scratch, &scratch, live);
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
				return false;
			if (!ready)
			{
				ready = zone_reset_room_publication_owner::
					prepare_original_reconstructed_flat_locked_bounded(
						work.selected_root, lock, root.original_envelope,
						root.completion, root.publication, root.published,
						restore_cold_flat_bindings, &root,
						reserve_warm_command_scratch, &scratch, live);
				if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
					return false;
			}
			if (!ready)
			{
				ready = zone_reset_room_publication_owner::
					prepare_original_present_prefix_flat_locked_bounded(
						work.selected_root, lock, root.original_envelope,
						root.completion, root.publication, &root.placement,
						reserve_warm_command_scratch, &scratch, live);
				if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
					return false;
			}
			if (!ready)
			{
				ready = zone_reset_room_publication_owner::
					prepare_original_pending_flat_locked_bounded(
						work.selected_root, lock, root.original_envelope,
						root.completion, root.publication, &root.placement,
						root.published, restore_cold_flat_bindings, &root,
						reserve_warm_command_scratch, &scratch, live);
				if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
					return false;
			}
			if (!ready)
				return false;
		}
		if (!(root.cold ?
			      zone_reset_room_publication_owner::refresh_cold_flat_locked_bounded(
				      work.selected_root, lock, root.original_envelope,
				      root.completion, root.publication,
				      reserve_warm_command_scratch, &scratch, live) :
			      zone_reset_room_publication_owner::refresh_warm_flat_locked_bounded(
				      work.selected_root, lock, root.original_envelope,
				      root.completion, root.publication,
				      reserve_warm_command_scratch, &scratch, live)) ||
		    !refresh_flat_publication_scratch(scratch, work, &lock, &live))
			return false;
		if (!root.context.batch_publication.succeeded)
		{
			if (!zone_reset_room_publication_owner::retain_admitted(root.publication) ||
			    !copy_flat_next_context(root, work, scratch, &lock))
				return false;
			for (auto &item : work.next.items)
				item.admitted = true;
			if (!std::all_of(root.context.items.begin(), root.context.items.end(),
					 [](const auto &item) { return item.admitted; }) &&
			    (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
			     !checkpoint_warm_context_bounded(root, work.next,
							      reserve_warm_command_scratch,
							      &scratch, live)))
				return false;
		}
		if (!root.context.whole_binding.succeeded)
		{
			if (!root.whole_bindings.valid_flat() ||
			    !refresh_flat_publication_scratch(scratch, work, &lock, &live))
				return false;
			const int run = prepare_warm_action_bounded(root, WARM_BINDING, 0, 0,
								    reserve_warm_command_scratch,
								    &scratch, live);
			if (run < 0)
				return false;
			if (run)
			{
				root.whole_bindings.commit_flat_unchecked();
				if (root.cold)
					root.cold_binding_restored = true;
				work.actual = { true, true, true, false };
				const bool finished =
					finish_flat_publication_action(root, work, scratch, lock);
				if (!refresh_flat_publication_scratch(scratch, work, &lock,
								      &live) ||
				    !finished)
					return false;
			}
		}
		if (!root.context.batch_publication.succeeded)
		{
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
				return false;
			const bool reserved = zone_reset_room_publication_owner::
				reserve_rooted_flat_consume_bounded(root.publication,
								    root.published,
								    reserve_warm_command_scratch,
								    &scratch, live);
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
			    !reserved)
				return false;
			const int run = prepare_warm_action_bounded(root, WARM_BATCH, 0, 0,
								    reserve_warm_command_scratch,
								    &scratch, live);
			if (run < 0)
				return false;
			if (run)
			{
				// Genuine provider is atomic before consumption. False keeps the real
				// not-attempted latch; unknown native effects cannot earn this branch.
				if (!zone_reset_room_publication_owner::consume_bounded(
					    root.publication, &root.published,
					    reserve_warm_command_scratch, &scratch, live))
					return false;
				work.actual = { true, true, true, false };
				const bool finished =
					finish_flat_publication_action(root, work, scratch, lock);
				if (!refresh_flat_publication_scratch(scratch, work, &lock,
								      &live) ||
				    !finished)
					return false;
			}
		}
		if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
		    !zone_reset_room_publication_owner::verify_warm_flat_current_bounded(
			    root.publication, reserve_warm_command_scratch, &scratch, live))
			return false;
		for (size_t row = 0; row < root.context.items.size(); ++row)
			for (size_t step = root.context.items[row].next_step;
			     step < root.context.items[row].effects.size(); ++step)
			{
				if (!lock.matches(work.selected_root) ||
				    !refresh_flat_publication_scratch(scratch, work, &lock,
								      &live) ||
				    !zone_reset_room_publication_owner::
					    verify_warm_flat_current_bounded(
						    root.publication, reserve_warm_command_scratch,
						    &scratch, live))
					return false;
				const int run = prepare_warm_action_bounded(
					root, WARM_ITEM_EFFECT, row, step,
					reserve_warm_command_scratch, &scratch, live);
				if (run < 0)
					return false;
				if (!run)
					continue;
				work.actual = {};
				(void)zone_reset_room_publication_owner::service_step_bounded(
					root.publication, row, step, work.actual,
					reserve_warm_command_scratch, &scratch, live);
				const bool finished =
					finish_flat_publication_action(root, work, scratch, lock);
				if (!refresh_flat_publication_scratch(scratch, work, &lock,
								      &live) ||
				    !finished)
					return false;
			}
		if (!root.context.room_placement.succeeded)
		{
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live))
				return false;
			const int run = prepare_warm_action_bounded(root, WARM_PLACE, 0, 0,
								    reserve_warm_command_scratch,
								    &scratch, live);
			if (run < 0)
				return false;
			if (run)
			{
				work.actual = {};
				(void)zone_reset_room_publication_owner::place_warm_bounded(
					root.publication, work.actual, reserve_warm_command_scratch,
					&scratch, live);
				const bool finished =
					finish_flat_publication_action(root, work, scratch, lock);
				if (!refresh_flat_publication_scratch(scratch, work, &lock,
								      &live) ||
				    !finished)
					return false;
			}
		}
		if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
		    !zone_reset_room_publication_owner::verify_warm_flat_current_bounded(
			    root.publication, reserve_warm_command_scratch, &scratch, live) ||
		    !zone_reset_room_publication_owner::mark_published_bounded(
			    root.publication, &root.published, reserve_warm_command_scratch,
			    &scratch, live) ||
		    !refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
		    !(root.cold ?
			      zone_reset_room_publication_owner::refresh_cold_flat_locked_bounded(
				      work.selected_root, lock, root.original_envelope,
				      root.completion, root.publication,
				      reserve_warm_command_scratch, &scratch, live) :
			      zone_reset_room_publication_owner::refresh_warm_flat_locked_bounded(
				      work.selected_root, lock, root.original_envelope,
				      root.completion, root.publication,
				      reserve_warm_command_scratch, &scratch, live)) ||
		    !lock.matches(work.selected_root))
			return false;
		if (root.context.stage != zone_reset_item_recovery_stage::physically_proven)
		{
			if (!copy_flat_next_context(root, work, scratch, &lock))
				return false;
			work.next.runtime_applied = true;
			work.next.stage = zone_reset_item_recovery_stage::physically_proven;
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
			    !checkpoint_warm_context_bounded(
				    root, work.next, reserve_warm_command_scratch, &scratch, live))
				return false;
		}
		if (root.original_envelope.phase ==
		    critical_native_recovery_phase::execution_pending)
		{
			if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
			    !prepare_flat_ack_successor_bounded(root, reserve_warm_command_scratch,
								&scratch, live) ||
			    !refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
			    !zone_reset_room_publication_owner::acknowledge_warm_bounded(
				    root.original_envelope, root.completion,
				    root.coordinator_generation, reserve_warm_command_scratch,
				    &scratch, live))
				return false;
			root.original_envelope = std::move(*root.ack_successor);
			root.ack_successor.reset();
		}
		if (!refresh_flat_publication_scratch(scratch, work, &lock, &live) ||
		    !zone_reset_room_publication_owner::retire_warm_flat_locked_bounded(
			    work.selected_root, lock, root.forest.items[0].object_uid,
			    root.original_envelope, root.coordinator_generation,
			    reserve_warm_command_scratch, &scratch, live))
			return false;
		root.retired = true;
		return release_retired(
			root); // Original metadata-only terminal release, no old-body dereference.
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool zone_reset_item_owner::restore_startup_original_bounded(
	const critical_native_recovery_envelope &original, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	// The real startup caller owns the coordinator lock and supplies its pure
	// nonreentrant lender. This wrapper grants no factory or publication authority.
	if (!reserve)
		return false;
	struct startup_restore_frame
	{
		warm_registry *before;
		warm_registry *attached;
		warm_registry **link;
		size_t live;
	} work{ nullptr, nullptr, nullptr, outer_live };
	if (!warm_scratch_add(work.live, sizeof(work)) || !reserve(work.live, context))
		return false;
	work.before = warm_head_;
	if (!restore_original_bounded(original, reserve, context, work.live))
		return false;
	// Complete original duplicate equality returned without attaching a node.
	// Preserve that exact branch, including its lack of post-attachment charge.
	if (warm_head_ == work.before)
		return true;
	work.attached = warm_head_;
	// The provider's actual decode/clone/candidate temporaries are now dead.
	// The same genuine lender observes current coordinator/native ownership;
	// transferred ROOT metadata is no longer part of private caller scratch.
	if (reserve(work.live, context))
		return true;
	// Restore the original new-registry rollback without removing a preexisting
	// duplicate. Cleanup is nonallocating and never requests budget after denial.
	work.link = &warm_head_;
	while (*work.link && *work.link != work.attached)
		work.link = &(*work.link)->next;
	if (*work.link == work.attached)
	{
		*work.link = work.attached->next;
		delete work.attached;
	}
	return false;
}

bool zone_reset_room_item_restore_startup_bounded(const critical_native_recovery_envelope &original,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	return zone_reset_item_owner::restore_startup_original_bounded(original, reserve, context,
								       outer_live);
}
