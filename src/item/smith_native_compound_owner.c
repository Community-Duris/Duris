#include "item/smith_native_compound_owner.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "economy/smith_native_accounting.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_codec.h"
#include "world/db.h"

#include <algorithm>
#include <array>
#include <utility>

struct smith_native_compound_owner::preparation
{
	smith_native_compound_terms terms;
	smith_native_compound_images images;
	smith_native_player_grant_projection original_grant;
	economic_native_money_checkpoint_projection admission_mapping{};
	player_smith_checkpoint_stage checkpoint;
	player_snapshot acknowledged_filtered_save;
	critical_command command;
	std::vector<uint8_t> recipe_bytes;
	native_mobile_birth_item_recipe factory_recipe;
};

namespace
{
bool smith_preparation_same_items(const std::vector<player_item_snapshot> &a,
				  const std::vector<player_item_snapshot> &b)
{
	std::vector<uint8_t> left, right;
	return player_item_snapshot_list_encode(a, &left) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(b, &right) == player_snapshot_codec_result::ok &&
	       left == right;
}

bool smith_preparation_same_native(const quest_mobile_native_image &a,
				   const quest_mobile_native_image &b)
{
	std::vector<uint8_t> left, right;
	return quest_mobile_native_image_encode(a, &left) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_image_encode(b, &right) == player_snapshot_codec_result::ok &&
	       left == right;
}

bool smith_preparation_same_catalog(const smith_native_catalog_selection &a,
				    const smith_native_catalog_selection &b) noexcept
{
	return a.smith_index == b.smith_index && a.menu_choice == b.menu_choice &&
	       a.forge_index == b.forge_index && a.ore_count == b.ore_count && a.fee == b.fee &&
	       a.ore_vnums == b.ore_vnums && a.keywords == b.keywords &&
	       a.long_description == b.long_description &&
	       a.short_description == b.short_description && a.affect_choices == b.affect_choices &&
	       a.allow_anti == b.allow_anti && a.classes == b.classes &&
	       a.wear_flags == b.wear_flags && a.bitvectors == b.bitvectors;
}

bool smith_preparation_same_grant(const smith_native_player_grant_projection &a,
				  const smith_native_player_grant_projection &b) noexcept
{
	if (a.original_player_level != b.original_player_level ||
	    a.original_output_rnum != b.original_output_rnum ||
	    a.original_carried_roots.size() != b.original_carried_roots.size())
		return false;
	for (size_t index = 0; index < a.original_carried_roots.size(); ++index)
		if (a.original_carried_roots[index].object_uid !=
			    b.original_carried_roots[index].object_uid ||
		    a.original_carried_roots[index].original_rnum !=
			    b.original_carried_roots[index].original_rnum)
			return false;
	return true;
}

bool smith_preparation_saved_wallet(const player_snapshot &saved,
				    const smith_native_wallet_cost &cost) noexcept
{
	constexpr std::array<player_status_field, 4> fields{ player_status_field::copper,
							     player_status_field::silver,
							     player_status_field::gold,
							     player_status_field::platinum };
	std::array<bool, 4> seen{};
	for (const auto &field : saved.status_integers)
		for (size_t index = 0; index < fields.size(); ++index)
			if (field.field == fields[index])
			{
				if (seen[index] || cost.before[index] < 0 ||
				    (field.is_unsigned ?
					     field.unsigned_value != uint64_t(cost.before[index]) :
					     field.signed_value != cost.before[index]))
					return false;
				seen[index] = true;
			}
	return std::all_of(seen.begin(), seen.end(), [](bool value) { return value; });
}
}

smith_native_compound_owner::smith_native_compound_owner(
	const smith_native_original_selection &original, quest_mobile_native_item_stage &factory,
	smith_native_output_preparation &output, smith_native_player_grant_preparation &grant,
	smith_native_persisted_before &persisted, const player_smith_checkpoint_token &checkpoint,
	const critical_operation_id &operation, const economic_source_event &source,
	uint64_t actual_accepted_at_usec) noexcept
	: original_(original)
	, factory_(factory)
	, output_(output)
	, grant_(grant)
	, persisted_(persisted)
	, checkpoint_(checkpoint)
	, operation_(operation)
	, source_(source)
	, accepted_at_usec_(actual_accepted_at_usec)
{
}

smith_native_compound_owner::~smith_native_compound_owner() = default;

bool smith_native_compound_owner::prepare_sql(MYSQL *actual_original_session) noexcept
{
	if (!actual_original_session || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() || !original_.captured ||
	    !persisted_.captured || persisted_.original_ != &original_ || !output_.started ||
	    !output_.returned || !output_.captured || output_.original_ != &original_ ||
	    output_.factory_ != &factory_ || !grant_.captured || grant_.original_ != &original_ ||
	    grant_.factory_ != &factory_ || grant_.output_body_ != output_.body_ ||
	    !checkpoint_.generation || checkpoint_.root_uid ||
	    checkpoint_.pid != int32_t(original_.terms.player_pid) ||
	    checkpoint_.actor_runtime_id != original_.player_runtime ||
	    critical_operation_id_is_zero(operation_) || !accepted_at_usec_ ||
	    source_.kind != economic_source_kind::crafting || !economic_source_event_valid(source_))
		return false;
	try
	{
		// Current indexed generations are resolved before remembered body pointers.
		P_char mobile = find_character_by_runtime_id(original_.mobile_runtime);
		P_char player = find_character_by_runtime_id(original_.player_runtime);
		if (!mobile || !player || !IS_NPC(mobile) || !IS_PC(player) || !player->only.pc ||
		    mobile->runtime_id != original_.mobile_runtime ||
		    player->runtime_id != original_.player_runtime ||
		    GET_PID(player) != int32_t(original_.terms.player_pid))
			return false;
		// Genuine SQL observer reauthenticates this already captured capsule,
		// original immutable birth history, current native image and physical
		// forest/season under the supplied actual session. It cannot overwrite a
		// captured BEFORE and does not invent history from current runtime clocks.
		if (!smith_native_before_sql::observe(actual_original_session, original_,
						      &persisted_))
			return false;
		// Both already captured providers take recheck-only branches. There are no
		// new constructor/stat draws, native string writes or grant/ore effects.
		if (!smith_native_producer::capture_output(original_, factory_, &output_) ||
		    !smith_native_producer::capture_player_grant(original_, factory_, output_,
								 &grant_))
			return false;
		smith_native_original_selection current;
		if (!smith_native_producer::capture(mobile, original_.mobile_runtime, player,
						    original_.player_runtime,
						    int(original_.catalog.menu_choice), &current) ||
		    current.admission_mapping.lineage.bytes !=
			    original_.admission_mapping.lineage.bytes ||
		    current.admission_mapping.epoch.bytes !=
			    original_.admission_mapping.epoch.bytes ||
		    !economic_account_key_equal(current.admission_mapping.player_wallet,
						original_.admission_mapping.player_wallet) ||
		    current.player_name != original_.player_name ||
		    !smith_preparation_same_catalog(current.catalog, original_.catalog) ||
		    !smith_preparation_same_items(current.full_native_items,
						  original_.full_native_items))
			return false;
		// Preserve actual SQL-born lineage/epoch/wallet lifetime independently of
		// season and today's PC save/item/wallet clocks. Intent freezes original
		// source VALUES; this preparation does not authenticate/claim that source.
		if (!persisted_.season_epoch ||
		    original_.admission_mapping.lineage.bytes !=
			    original_.terms.native_before.lineage.bytes ||
		    persisted_.native_origin.birth_operation.bytes !=
			    original_.terms.native_before.reference.birth_operation.bytes ||
		    persisted_.native_origin.mobile_instance_id !=
			    original_.terms.native_before.reference.mobile_instance_id ||
		    persisted_.native_origin.lineage.bytes !=
			    original_.terms.native_before.lineage.bytes ||
		    persisted_.native_origin.birth_epoch.bytes !=
			    original_.terms.native_before.birth_epoch.bytes ||
		    persisted_.native_origin.wallet_mapping_id !=
			    original_.terms.native_before.wallet_mapping_id)
			return false;
		// A populated original identity/output is checked, never silently rebound.
		// The genuine capture normally leaves these four preparation fields empty.
		const economic_source_event empty_source{};
		if ((!critical_operation_id_is_zero(original_.terms.operation_id) &&
		     original_.terms.operation_id.bytes != operation_.bytes) ||
		    (original_.terms.season_epoch &&
		     original_.terms.season_epoch != persisted_.season_epoch) ||
		    (!smith_native_accounting_detail::source_equal(original_.terms.source,
								   empty_source) &&
		     !smith_native_accounting_detail::source_equal(original_.terms.source,
								   source_)) ||
		    (!original_.terms.frozen_outputs.empty() &&
		     !smith_preparation_same_items(original_.terms.frozen_outputs,
						   output_.frozen_output)))
			return false;
		auto candidate = std::make_unique<preparation>();
		candidate->terms = original_.terms;
		candidate->terms.operation_id = operation_;
		candidate->terms.source = source_;
		candidate->terms.season_epoch = persisted_.season_epoch;
		candidate->terms.frozen_outputs = output_.frozen_output;
		candidate->original_grant = grant_.projection;
		candidate->admission_mapping = original_.admission_mapping;
		candidate->factory_recipe = output_.factory_recipe;
		current.terms.operation_id = operation_;
		current.terms.source = source_;
		current.terms.season_epoch = persisted_.season_epoch;
		current.terms.frozen_outputs = output_.frozen_output;
		std::vector<uint8_t> original_terms, current_terms;
		if (smith_native_compound_encode(candidate->terms, &original_terms) !=
			    player_snapshot_codec_result::ok ||
		    smith_native_compound_encode(current.terms, &current_terms) !=
			    player_snapshot_codec_result::ok ||
		    original_terms != current_terms)
			return false;
		// This real observer validates the held operation, actual worker ACK and
		// exact queued/coalesced filtered body against the CURRENT same actor.
		// Full physical grant images remain independently captured, including
		// NORENT; filtered ACK items are not substituted for that complete forest.
		std::vector<player_item_snapshot> acknowledged_current_items;
		if (!player_save_smith_checkpoint_owner::observe_held(
			    checkpoint_, player, operation_, &candidate->checkpoint,
			    &acknowledged_current_items, &candidate->acknowledged_filtered_save) ||
		    !candidate->checkpoint.save_revision ||
		    candidate->checkpoint.level != grant_.projection.original_player_level ||
		    candidate->acknowledged_filtered_save.revision !=
			    candidate->checkpoint.save_revision ||
		    candidate->acknowledged_filtered_save.pid != checkpoint_.pid ||
		    !smith_preparation_same_items(acknowledged_current_items,
						  candidate->acknowledged_filtered_save.items) ||
		    !smith_preparation_saved_wallet(candidate->acknowledged_filtered_save,
						    candidate->terms.player_wallet))
			return false;
		if (smith_native_compound_prepare_images(
			    candidate->terms, persisted_.native_before, grant_.player_before,
			    candidate->original_grant,
			    &candidate->images) != player_snapshot_codec_result::ok ||
		    smith_native_command_build(candidate->terms, operation_, accepted_at_usec_,
					       &candidate->command) !=
			    economic_accounting_error::ok)
			return false;
		std::vector<uint8_t> intent;
		if (smith_native_accounting_intent(
			    candidate->command, original_.admission_mapping.lineage,
			    original_.admission_mapping.epoch, original_.terms.player_pid,
			    original_.admission_mapping.player_wallet,
			    &intent) != economic_accounting_error::ok)
			return false;
		candidate->command.accounting_intent = std::move(intent);
		candidate->command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		candidate->command.publication_required = true;
		smith_native_compound_terms checked;
		if (smith_native_command_validate(candidate->command, &checked) !=
			    economic_accounting_error::ok ||
		    native_mobile_birth_recipe_encode(
			    output_.frozen_output, { &candidate->factory_recipe, 1 },
			    &candidate->recipe_bytes) != economic_accounting_error::ok)
			return false;
		if (prepared_)
		{
			std::vector<uint8_t> held_save, current_save;
			return prepared_->checkpoint.save_revision ==
				       candidate->checkpoint.save_revision &&
			       prepared_->checkpoint.level == candidate->checkpoint.level &&
			       smith_preparation_same_grant(prepared_->original_grant,
							    candidate->original_grant) &&
			       critical_command_equal(prepared_->command, candidate->command) &&
			       prepared_->recipe_bytes == candidate->recipe_bytes &&
			       smith_preparation_same_native(prepared_->images.native_before,
							     candidate->images.native_before) &&
			       smith_preparation_same_native(prepared_->images.native_after,
							     candidate->images.native_after) &&
			       smith_preparation_same_items(prepared_->images.player_before,
							    candidate->images.player_before) &&
			       smith_preparation_same_items(prepared_->images.player_after,
							    candidate->images.player_after) &&
			       player_snapshot_encode(prepared_->acknowledged_filtered_save,
						      &held_save) ==
				       player_snapshot_codec_result::ok &&
			       player_snapshot_encode(candidate->acknowledged_filtered_save,
						      &current_save) ==
				       player_snapshot_codec_result::ok &&
			       held_save == current_save;
		}
		// Complete genuine value freeze before this nonallocating ownership cut.
		// Retrying may only authenticate this same preparation; never replace it.
		prepared_ = std::move(candidate);
		return true;
	}
	catch (...)
	{
		// Borrowed original/returned markers, held save and factory stay intact.
		// Destroy only this attempt's private values; never perform native rollback.
		return false;
	}
}

bool smith_native_compound_owner::observe_prepared(
	smith_native_compound_prepared_view *output) const noexcept
{
	if (!output || !prepared_)
		return false;
	*output = { &prepared_->terms,
		    &prepared_->images,
		    &prepared_->original_grant,
		    &prepared_->admission_mapping,
		    &prepared_->command,
		    &prepared_->checkpoint,
		    &prepared_->acknowledged_filtered_save,
		    &prepared_->recipe_bytes,
		    &prepared_->factory_recipe };
	return true;
}
