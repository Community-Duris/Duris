#include "economy/smith_native_producer.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/random.h"
#include "core/safe_format.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "persistence/shop_item_runtime_payload.h"
#include "world/db.h"
#include "world/quest_mobile_native.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <utility>

namespace
{
bool wallet_cost(P_char player, uint64_t mapping, uint32_t count, uint32_t fee,
		 smith_native_wallet_cost *output) noexcept
{
	if (!output || !mapping || player->only.pc->wallet_revision == UINT64_MAX)
		return false;
	smith_native_wallet_cost cost;
	cost.wallet_mapping_id = mapping;
	cost.before_revision = player->only.pc->wallet_revision;
	cost.after_revision = cost.before_revision + 1;
	cost.before = { GET_COPPER(player), GET_SILVER(player), GET_GOLD(player),
			GET_PLATINUM(player) };
	constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
	int64_t value = 0;
	for (size_t i = 0; i < units.size(); ++i)
	{
		if (cost.before[i] < 0)
			return false;
		value += int64_t(cost.before[i]) * units[i];
	}
	if (value < fee)
		return false;
	value -= fee;
	for (size_t i = units.size(); i-- > 0;)
	{
		if (value / units[i] > INT32_MAX)
			return false;
		cost.after[i] = static_cast<int32_t>(value / units[i]);
		value %= units[i];
	}
	if (!smith_native_wallet_cost_valid(cost, count, fee))
		return false;
	*output = cost;
	return true;
}
}

bool smith_native_producer::capture(P_char mobile, uint64_t mobile_runtime, P_char player,
				    uint64_t player_runtime, int original_menu_choice,
				    smith_native_original_selection *output) noexcept
{
	if (!output || output->captured || !nevent_is_game_thread() || !mobile || !player ||
	    !mobile_runtime || !player_runtime ||
	    find_character_by_runtime_id(mobile_runtime) != mobile ||
	    find_character_by_runtime_id(player_runtime) != player ||
	    mobile->runtime_id != mobile_runtime || player->runtime_id != player_runtime ||
	    !IS_NPC(mobile) || !mobile->only.npc || !IS_PC(player) || !player->only.pc ||
	    player->only.pc->load_degraded_components || GET_PID(player) <= 0 ||
	    !GET_NAME(player) || player->in_room == NOWHERE || player->in_room != mobile->in_room)
		return false;
	try
	{
		smith_native_original_selection candidate;
		candidate.mobile_runtime = mobile_runtime;
		candidate.player_runtime = player_runtime;
		candidate.player_name = GET_NAME(player);
		auto &terms = candidate.terms;
		terms.player_pid = static_cast<uint32_t>(GET_PID(player));
		if (!quest_mobile_native_cash_reference_copy(mobile, mobile_runtime,
							     &terms.native_before) ||
		    !catalog(terms.native_before.reference.mobile_vnum, original_menu_choice,
			     &candidate.catalog) ||
		    !economic_gameplay_authority::observe_craft_wallet_checkpoint(
			    terms.player_pid, &candidate.admission_mapping) ||
		    !critical_operation_id_equal(candidate.admission_mapping.lineage,
						 terms.native_before.lineage) ||
		    !wallet_cost(player, candidate.admission_mapping.player_wallet.authority_id,
				 candidate.catalog.ore_count, candidate.catalog.fee,
				 &terms.player_wallet))
			return false;
		const item_owner_identity native_owner{
			item_owner_type::native_mobile,
			terms.native_before.reference.mobile_instance_id, 0
		};
		const item_owner_identity player_owner{ item_owner_type::player, terms.player_pid,
							0 };
		uint64_t native_revision = 0;
		if (!item_ownership_runtime_peek_owner_revision(native_owner, &native_revision) ||
		    native_revision != terms.native_before.reference.stock_revision ||
		    native_revision == UINT64_MAX ||
		    !item_ownership_runtime_peek_owner_revision(
			    player_owner, &terms.expected_player_item_revision) ||
		    terms.expected_player_item_revision == UINT64_MAX ||
		    quest_mobile_native_items_observe(mobile, terms.native_before.reference,
						      &candidate.full_native_items) !=
			    player_snapshot_capture_result::ok)
			return false;
		terms.smith_catalog_index = candidate.catalog.smith_index;
		terms.original_menu_choice = candidate.catalog.menu_choice;
		terms.forge_catalog_index = candidate.catalog.forge_index;
		terms.ore_count = candidate.catalog.ore_count;
		terms.fee = candidate.catalog.fee;
		const auto &forest = candidate.full_native_items;
		// Original obj_from_char selection skips previously selected carried
		// roots. Read-only simulation preserves that recipe order without detach.
		for (size_t ore = 0; ore < candidate.catalog.ore_count; ++ore)
		{
			const auto selected = std::find_if(
				forest.begin(), forest.end(),
				[&](const auto &row)
				{
					return row.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
					       !row.equipment_slot &&
					       row.vnum == candidate.catalog.ore_vnums[ore] &&
					       std::find(terms.selected_root_order.begin(),
							 terms.selected_root_order.end(),
							 row.object_uid) ==
						       terms.selected_root_order.end();
				});
			if (selected == forest.end())
				return false;
			if (!ore)
				terms.first_ore_material = selected->material;
			terms.selected_root_order.push_back(selected->object_uid);
		}
		// Complete original native custody values, not publication authority.
		std::vector<int32_t> translated(forest.size(), PLAYER_SNAPSHOT_NO_PARENT);
		uint64_t root_uid = 0;
		bool selected_root = false;
		for (size_t i = 0; i < forest.size(); ++i)
		{
			const auto &row = forest[i];
			uint64_t parent_uid = 0;
			if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			{
				root_uid = row.object_uid;
				selected_root = std::find(terms.selected_root_order.begin(),
							  terms.selected_root_order.end(),
							  root_uid) !=
						terms.selected_root_order.end();
			}
			else
			{
				if (row.parent_index < 0 || size_t(row.parent_index) >= i)
					return false;
				parent_uid = forest[row.parent_index].object_uid;
			}
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(row.object_uid, &actual) ||
			    !item_owner_identity_equal(actual.owner, native_owner) ||
			    actual.owner_revision != native_revision ||
			    actual.state != item_custody_state::active || !actual.item_revision ||
			    actual.item_revision == UINT64_MAX ||
			    actual.root_item_uid != root_uid ||
			    actual.parent_item_uid != parent_uid || actual.vnum != row.vnum)
				return false;
			if (!selected_root)
				continue;
			if (terms.selected_native_items.size() >= ITEM_TRANSFER_MAX_ITEMS)
				return false;
			auto selected = row;
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				selected.parent_index = translated[row.parent_index];
				if (selected.parent_index < 0)
					return false;
			}
			translated[i] = static_cast<int32_t>(terms.selected_native_items.size());
			terms.selected_native_items.push_back(std::move(selected));
			terms.selected_custody.push_back({ row.object_uid, root_uid, parent_uid,
							   actual.item_revision, row.vnum,
							   item_custody_state::active });
		}
		std::sort(terms.selected_custody.begin(), terms.selected_custody.end(),
			  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
		// No historical transition, season, operation or output is inferred.
		candidate.captured = true;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool smith_native_producer::capture_output(const smith_native_original_selection &selection,
					   quest_mobile_native_item_stage &factory,
					   smith_native_output_preparation *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !selection.captured ||
	    (output->started && !output->returned))
		return false;
	try
	{
		// Resolve through the retained original factory before dereferencing any
		// remembered body pointer. This cannot authorize a fresh constructor.
		P_obj body = factory.object();
		quest_mobile_native_item_progress progress{};
		if (!body || !factory.owns_pending_original_target(body) ||
		    !factory.read_progress(&progress) || progress.admitted || progress.published ||
		    progress.next_step || progress.current_step_started || !obj_index ||
		    body->R_num < 0 || body->R_num > top_of_objt ||
		    obj_index[body->R_num].virtual_number != 1255 || !body->obj_uid ||
		    body->obj_uid == UINT64_MAX || body->loc_p != LOC_NOWHERE ||
		    body->loc.room != NOWHERE || body->next || body->prev || body->contains ||
		    body->next_content || body->nevents || body->nevents_tail ||
		    selection.terms.selected_root_order.empty())
			return false;
		for (const auto &input : selection.full_native_items)
			if (input.object_uid == body->obj_uid)
				return false;
		if (output->started &&
		    (output->original_ != &selection || output->factory_ != &factory ||
		     output->body_ != body || output->output_uid != body->obj_uid))
			return false;
		if (!output->started)
		{
			output->original_ = &selection;
			output->factory_ = &factory;
			output->body_ = body;
			output->output_uid = body->obj_uid;
			// The genuine constructor/parser/probes/conversion already returned.
			// Mark this original Smith cut before any string write or stat draw.
			output->started = true;
			const auto &catalog = selection.catalog;
			char dummy[MAX_INPUT_LENGTH];
			char short_description[MAX_STRING_LENGTH];
			char long_description[MAX_INPUT_LENGTH];
			char keywords[MAX_INPUT_LENGTH];
			snprintf(keywords, sizeof(keywords), "%s", catalog.keywords.c_str());
			snprintf(dummy, sizeof(dummy), "%s", catalog.short_description.c_str());
			checked_snprintf_runtime(short_description, sizeof(short_description),
						 dummy, selection.player_name.c_str());
			snprintf(dummy, sizeof(dummy), "%s", catalog.long_description.c_str());
			checked_snprintf_runtime(long_description, sizeof(long_description), dummy,
						 selection.player_name.c_str());
			set_short_description(body, short_description);
			set_long_description(body, long_description);
			set_keywords(body, keywords);
			body->material = selection.terms.first_ore_material;
			body->affected[0].location = catalog.affect_choices[0];
			output->rolls[0] =
				number(catalog.affect_choices[1], catalog.affect_choices[2]);
			output->rolls_returned = 1;
			body->affected[0].modifier = output->rolls[0];
			body->affected[1].location = catalog.affect_choices[3];
			output->rolls[1] =
				number(catalog.affect_choices[4], catalog.affect_choices[5]);
			output->rolls_returned = 2;
			body->affected[1].modifier = output->rolls[1];
			SET_BIT(body->wear_flags, ITEM_TAKE);
			SET_BIT(body->wear_flags, catalog.wear_flags);
			SET_BIT(body->bitvector, catalog.bitvectors[0]);
			SET_BIT(body->bitvector2, catalog.bitvectors[1]);
			SET_BIT(body->bitvector3, catalog.bitvectors[2]);
			SET_BIT(body->bitvector4, catalog.bitvectors[3]);
			body->anti_flags |= catalog.classes;
			if (catalog.allow_anti)
				body->extra_flags = ITEM_ALLOWED_CLASSES;
			output->quiver = isname("quiver", body->name);
			if (output->quiver)
			{
				output->rolls[2] = number(20, 80);
				output->rolls_returned = 3;
				body->value[0] = output->rolls[2];
				output->rolls[3] = number(0, 1);
				output->rolls_returned = 4;
				body->value[1] = output->rolls[3];
				body->value[2] = 1;
				body->value[3] = 0;
				body->type = ITEM_QUIVER;
			}
			else
				body->type = ITEM_ARMOR;
			// Normally returned native cuts are retained before fallible capture.
			output->returned = true;
		}
		std::vector<player_item_snapshot> literal;
		native_mobile_birth_item_recipe recipe;
		if (player_item_snapshot_tree_capture_literal(body, &literal, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    literal.size() != 1 || literal[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    literal[0].equipment_slot || literal[0].object_uid != output->output_uid ||
		    literal[0].vnum != 1255 || !factory.capture_recipe(literal[0], &recipe))
			return false;
		if (output->captured)
		{
			// A retained output is immutable. Re-observation cannot replace it
			// with altered literals or substitute another factory's decisions.
			std::vector<uint8_t> actual, retained, actual_recipe, retained_recipe;
			return player_item_snapshot_list_encode(literal, &actual) ==
				       player_snapshot_codec_result::ok &&
			       player_item_snapshot_list_encode(output->frozen_output, &retained) ==
				       player_snapshot_codec_result::ok &&
			       actual == retained &&
			       native_mobile_birth_recipe_encode(literal, { &recipe, 1 },
								 &actual_recipe) ==
				       economic_accounting_error::ok &&
			       native_mobile_birth_recipe_encode(output->frozen_output,
								 { &output->factory_recipe, 1 },
								 &retained_recipe) ==
				       economic_accounting_error::ok &&
			       actual_recipe == retained_recipe;
		}
		output->frozen_output = std::move(literal);
		output->factory_recipe = std::move(recipe);
		output->captured = true;
		return true;
	}
	catch (...)
	{
		// Neither a nonreturning original cut nor recapture refusal permits
		// another factory, another roll, native disposal or economic rollback.
		return false;
	}
}

bool smith_native_producer::capture_player_grant(
	const smith_native_original_selection &selection, quest_mobile_native_item_stage &factory,
	const smith_native_output_preparation &prepared,
	smith_native_player_grant_preparation *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !selection.captured || !prepared.started ||
	    !prepared.returned || !prepared.captured || prepared.original_ != &selection ||
	    prepared.factory_ != &factory)
		return false;
	try
	{
		// Resolve the actual generation before dereferencing remembered pointers.
		P_char player = find_character_by_runtime_id(selection.player_runtime);
		P_obj body = factory.object();
		quest_mobile_native_item_progress progress{};
		if (!player || player->runtime_id != selection.player_runtime || !IS_PC(player) ||
		    !player->only.pc || player->only.pc->load_degraded_components ||
		    GET_PID(player) != static_cast<int32_t>(selection.terms.player_pid) ||
		    GET_LEVEL(player) < 0 || !body || body != prepared.body_ ||
		    body->obj_uid != prepared.output_uid ||
		    !factory.owns_pending_original_target(body) ||
		    !factory.read_progress(&progress) || progress.admitted || progress.published ||
		    progress.next_step || progress.current_step_started || !obj_index ||
		    body->R_num < 0 || body->R_num > top_of_objt ||
		    obj_index[body->R_num].virtual_number != 1255 || body->loc_p != LOC_NOWHERE ||
		    body->loc.room != NOWHERE || body->next || body->prev || body->contains ||
		    body->next_content || body->nevents || body->nevents_tail)
			return false;
		if (output->captured &&
		    (output->original_ != &selection || output->factory_ != &factory ||
		     output->output_body_ != body))
			return false;
		// This existing observer accepts a character forest and includes NORENT;
		// it performs literal/location/UID/depth/cycle checks without SQL or writes.
		smith_native_player_grant_preparation candidate;
		if (!shop_item_runtime_capture_keeper_literal(player, &candidate.player_before))
			return false;
		candidate.projection.original_player_level =
			static_cast<uint32_t>(GET_LEVEL(player));
		candidate.projection.original_output_rnum = body->R_num;
		size_t carried_index = 0;
		for (const auto &row : candidate.player_before)
		{
			if (row.object_uid == prepared.output_uid)
				return false;
			if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT && !row.equipment_slot)
				++carried_index;
		}
		candidate.projection.original_carried_roots.reserve(carried_index);
		for (P_obj root = player->carrying; root; root = root->next_content)
		{
			if (candidate.projection.original_carried_roots.size() == carried_index ||
			    root->R_num < 0 || root->R_num > top_of_objt)
				return false;
			candidate.projection.original_carried_roots.push_back(
				{ root->obj_uid, root->R_num });
		}
		if (candidate.projection.original_carried_roots.size() != carried_index)
			return false;
		carried_index = 0;
		for (const auto &row : candidate.player_before)
			if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT && !row.equipment_slot)
			{
				const auto &actual =
					candidate.projection.original_carried_roots[carried_index++];
				if (row.object_uid != actual.object_uid ||
				    obj_index[actual.original_rnum].virtual_number != row.vnum)
					return false;
			}
		std::vector<player_item_snapshot> literal;
		std::vector<uint8_t> actual_output, frozen_output;
		if (player_item_snapshot_tree_capture_literal(body, &literal, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    player_item_snapshot_list_encode(literal, &actual_output) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(prepared.frozen_output, &frozen_output) !=
			    player_snapshot_codec_result::ok ||
		    actual_output != frozen_output)
			return false;
		if (output->captured)
		{
			if (output->projection.original_player_level !=
				    candidate.projection.original_player_level ||
			    output->projection.original_output_rnum !=
				    candidate.projection.original_output_rnum ||
			    output->projection.original_carried_roots.size() !=
				    candidate.projection.original_carried_roots.size())
				return false;
			for (size_t i = 0; i < candidate.projection.original_carried_roots.size();
			     ++i)
			{
				const auto &a = candidate.projection.original_carried_roots[i];
				const auto &b = output->projection.original_carried_roots[i];
				if (a.object_uid != b.object_uid ||
				    a.original_rnum != b.original_rnum)
					return false;
			}
			std::vector<uint8_t> actual, retained;
			return player_item_snapshot_list_encode(candidate.player_before, &actual) ==
				       player_snapshot_codec_result::ok &&
			       player_item_snapshot_list_encode(output->player_before, &retained) ==
				       player_snapshot_codec_result::ok &&
			       actual == retained;
		}
		candidate.original_ = &selection;
		candidate.factory_ = &factory;
		candidate.output_body_ = body;
		candidate.captured = true;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
