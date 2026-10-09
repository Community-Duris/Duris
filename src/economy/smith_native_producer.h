#ifndef SMITH_NATIVE_PRODUCER_H
#define SMITH_NATIVE_PRODUCER_H

#include "item/smith_native_compound.h"
#include "item/smith_native_compound_images.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/native_mobile_birth_recipe.h"

#include <array>
#include <string>

struct char_data;
struct obj_data;
class quest_mobile_native_item_stage;
class smith_native_compound_owner;

// Literal catalog decisions, before the genuine factory performs its original
// allocation/callbacks and random draws. These values authorize no output.
struct smith_native_catalog_selection
{
	uint32_t smith_index = 0, menu_choice = 0, forge_index = 0, ore_count = 0, fee = 0;
	std::array<int32_t, 5> ore_vnums{};
	std::string keywords, long_description, short_description;
	std::array<int32_t, 6> affect_choices{};
	int32_t allow_anti = 0;
	uint32_t classes = 0, wear_flags = 0;
	std::array<uint32_t, 4> bitvectors{};
};

// Private owner's retained original observation, not a save/SQL/source/ACK
// capability. Full native forest, selected native forest, canonical custody,
// and original ore-selection root order deliberately remain separate.
struct smith_native_original_selection
{
	bool captured = false;
	uint64_t mobile_runtime = 0, player_runtime = 0;
	std::string player_name;
	economic_native_money_checkpoint_projection admission_mapping{};
	smith_native_catalog_selection catalog;
	smith_native_compound_terms terms;
	std::vector<player_item_snapshot> full_native_items;
};

// Original post-constructor Smith preparation. The root owns the actual factory
// before calling; no literal or returned flag grants admission/publication.
struct smith_native_output_preparation
{
	bool started = false, returned = false, captured = false, quiver = false;
	uint64_t output_uid = 0;
	std::array<int32_t, 4> rolls{};
	uint8_t rolls_returned = 0;
	std::vector<player_item_snapshot> frozen_output;
	native_mobile_birth_item_recipe factory_recipe;

    private:
	friend class smith_native_producer;
	friend class smith_native_compound_owner;
	const smith_native_original_selection *original_ = nullptr;
	const quest_mobile_native_item_stage *factory_ = nullptr;
	obj_data *body_ = nullptr;
};

// Complete physical PC and original runtime grouping observation, retained by
// the original owner independently of the filtered acknowledged save body.
// This is not save, economic-source, admission, publication or ACK authority.
struct smith_native_player_grant_preparation
{
	bool captured = false;
	std::vector<player_item_snapshot> player_before;
	smith_native_player_grant_projection projection;

    private:
	friend class smith_native_producer;
	friend class smith_native_compound_owner;
	const smith_native_original_selection *original_ = nullptr;
	const quest_mobile_native_item_stage *factory_ = nullptr;
	const obj_data *output_body_ = nullptr;
};

class smith_native_producer final
{
    private:
	friend class smith_native_compound_owner;
	// Caller retains the original indexed generations; no replacement pointer,
	// fresh operation, fee, detach, factory, RNG, save or admission is issued.
	// Failure preserves output; a captured observation cannot be overwritten.
	static bool capture(char_data *mobile, uint64_t mobile_runtime, char_data *player,
			    uint64_t player_runtime, int original_menu_choice,
			    smith_native_original_selection *output) noexcept;
	// Defined beside the actual tables, without a duplicate catalog registry.
	static bool catalog(int32_t mobile_vnum, int original_menu_choice,
			    smith_native_catalog_selection *output) noexcept;
	// Existing genuine completed/unadmitted original factory only. Original
	// started-unreturned cuts remain held; returned cuts only retry capture.
	// Never prepare, restore, publish, discard, issue identity or charge a fee.
	static bool capture_output(const smith_native_original_selection &,
				   quest_mobile_native_item_stage &,
				   smith_native_output_preparation *) noexcept;
	// Reads the original indexed PC and genuinely prepared unadmitted factory;
	// freezes actual carried-root R_nums and PC level before any native grant.
	// Captured values are only rechecked on retry, never rebound to a new body.
	static bool capture_player_grant(const smith_native_original_selection &,
					 quest_mobile_native_item_stage &,
					 const smith_native_output_preparation &,
					 smith_native_player_grant_preparation *) noexcept;
};

#endif
