#include "world/quest_mobile_published_saved_forest.h"
#include "core/defines.h"
#include "core/prototypes.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_load_items.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "world/db.h"
#include <algorithm>
#include <cstdio>
#include <utility>
#include <type_traits>

namespace
{
void report_forest_refusal(unsigned int stage) noexcept
{
	static bool reported = false;
	if (!reported)
	{
		reported = true;
		std::fprintf(stderr, "forest-recovery-refusal stage=%u\n", stage);
	}
}
bool returned(const shop_trade_original_reload_effect &effect) noexcept
{
	return effect.started && effect.returned && effect.succeeded;
}
bool literal_matches(P_obj object, const player_item_snapshot &expected)
{
	std::vector<player_item_snapshot> actual;
	if (player_item_snapshot_tree_capture_literal(object, &actual, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    actual.empty())
		return false;
	actual.resize(1);
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode(actual, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode({ expected }, &b) ==
		       player_snapshot_codec_result::ok &&
	       a == b;
}
bool add(size_t &used, size_t count, size_t width = 1) noexcept
{
	if (width && count > (SIZE_MAX - used) / width)
		return false;
	used += count * width;
	return true;
}
bool literal_bytes(const player_item_snapshot &item, size_t &bytes) noexcept
{
	for (const auto *text :
	     { &item.name, &item.short_description, &item.description, &item.action_description })
		if (!add(bytes, text->capacity()) || !add(bytes, 1))
			return false;
	if (!add(bytes, item.dynamic_affects.capacity(), sizeof(item.dynamic_affects[0])) ||
	    !add(bytes, item.extra_descriptions.capacity(), sizeof(item.extra_descriptions[0])))
		return false;
	for (const auto &description : item.extra_descriptions)
		if (!add(bytes, description.keyword.capacity()) || !add(bytes, 1) ||
		    !add(bytes, description.description.capacity()) || !add(bytes, 1) ||
		    !add(bytes, description.spell_ids.capacity(), sizeof(description.spell_ids[0])))
			return false;
	return true;
}
}

bool quest_mobile_published_saved_forest::prepare(
	const quest_mobile_native_image &source,
	quest_mobile_published_saved_forest &output) noexcept
{
	if (!nevent_is_game_thread())
	{
		report_forest_refusal(9011);
		return false;
	}
	if (!economic_gameplay_authority::active())
	{
		report_forest_refusal(9012);
		return false;
	}
	if (output.prepared_ || output.consumed_ || !output.staged_.empty())
	{
		report_forest_refusal(9013);
		return false;
	}
	if (source.state != quest_mobile_lifetime_state::live)
	{
		report_forest_refusal(9014);
		return false;
	}
	if (!source.cash)
	{
		report_forest_refusal(9015);
		return false;
	}
	if (!recovery_object_templates_ready())
	{
		report_forest_refusal(9016);
		return false;
	}
	try
	{
		// Original image codec checks the complete native forest/reference/cash.
		// Its facts remain authenticated by the separate root SQL/world owner.
		std::vector<uint8_t> encoded;
		if (quest_mobile_native_image_encode(source, &encoded) !=
		    player_snapshot_codec_result::ok)
		{
			report_forest_refusal(9021);
			return false;
		}
		quest_mobile_published_saved_forest candidate;
		candidate.source_ = source;
		candidate.detached_ = source.items;
		candidate.staged_.resize(source.items.size());
		candidate.templates_.resize(source.items.size());
		candidate.objects_.resize(source.items.size());
		candidate.reload_.resize(source.items.size());
		for (size_t row = 0; row < source.items.size(); ++row)
		{
			auto &literal = candidate.detached_[row];
			literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			literal.equipment_slot = 0;
			const auto *prototype = find_recovery_object_template(literal.vnum);
			if (!prototype)
			{
				report_forest_refusal(9022);
				return false;
			}
			if (!shop_trade_original_item_stage::prepare(*prototype, literal,
								     candidate.staged_[row]))
			{
				report_forest_refusal(9023);
				return false;
			}
			candidate.templates_[row] = prototype;
			candidate.objects_[row] = candidate.staged_[row].object_;
			candidate.reload_[row].probes.resize(literal.extra_descriptions.size());
			if (!literal_matches(candidate.objects_[row], literal))
			{
				report_forest_refusal(9024);
				return false;
			}
		}
		if (!shop_trade_original_procedure_binding_stage::prepare(
			    candidate.objects_, candidate.detached_, candidate.bindings_))
		{
			report_forest_refusal(9025);
			return false;
		}
		candidate.prepared_ = true;
		if (!candidate.valid())
		{
			report_forest_refusal(9026);
			return false;
		}
		if (!candidate.retained_bytes())
		{
			report_forest_refusal(9027);
			return false;
		}
		output.transfer_prepared(std::move(candidate));
		return true;
	}
	catch (...)
	{
		report_forest_refusal(9028);
		return false;
	}
}

void quest_mobile_published_saved_forest::transfer_prepared(
	quest_mobile_published_saved_forest &&candidate) noexcept
{
	// Called only after prepare has refused nonempty/consumed output and
	// completed every allocating/refusing check. No consumed holder is moved.
	static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
	static_assert(std::is_nothrow_move_assignable_v<decltype(staged_)>);
	static_assert(std::is_nothrow_move_assignable_v<decltype(bindings_)>);
	source_ = std::move(candidate.source_);
	detached_ = std::move(candidate.detached_);
	staged_ = std::move(candidate.staged_);
	templates_ = std::move(candidate.templates_);
	objects_ = std::move(candidate.objects_);
	reload_ = std::move(candidate.reload_);
	bindings_ = std::move(candidate.bindings_);
	consumed_ = false;
	prepared_ = true;
	candidate.prepared_ = false;
}

bool quest_mobile_published_saved_forest::row_valid(size_t row, P_obj fresh) const noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active() ||
	    row >= objects_.size() || !fresh || fresh != objects_[row] || row >= detached_.size() ||
	    row >= templates_.size() || !templates_[row])
		return false;
	try
	{
		const auto &literal = detached_[row];
		return find_recovery_object_template(literal.vnum) == templates_[row] &&
		       fresh->obj_uid == literal.object_uid &&
		       fresh->R_num == templates_[row]->R_num && literal_matches(fresh, literal);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_published_saved_forest::valid(std::span<const P_obj> fresh) const noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active() ||
	    (!prepared_ && !consumed_) || source_.items.size() != detached_.size() ||
	    source_.items.size() != staged_.size() || source_.items.size() != templates_.size() ||
	    source_.items.size() != objects_.size() || source_.items.size() != reload_.size() ||
	    (consumed_ ? fresh.size() != objects_.size() : !bindings_.valid()))
		return false;
	for (size_t row = 0; row < objects_.size(); ++row)
		if ((!consumed_ && staged_[row].object_ != objects_[row]) ||
		    !row_valid(row, consumed_ ? fresh[row] : staged_[row].object_) ||
		    reload_[row].probes.size() != source_.items[row].extra_descriptions.size())
			return false;
	return true;
}

bool quest_mobile_published_saved_forest::consume(std::span<P_obj> output) noexcept
{
	if (consumed_ || !prepared_ || output.size() != objects_.size() ||
	    std::any_of(output.begin(), output.end(),
			[](P_obj object) { return object != nullptr; }) ||
	    !valid())
		return false;
	// All potentially refusing/allocating work preceded this original no-fail
	// binding commit. Root's full cut must still hold; this is not a proof token.
	bindings_.commit_unchecked();
	for (size_t row = 0; row < staged_.size(); ++row)
	{
		output[row] = std::exchange(staged_[row].object_, nullptr);
		staged_[row].pool_ = nullptr;
		staged_[row].affect_pool_ = nullptr;
	}
	prepared_ = false;
	consumed_ = true;
	return true;
}

bool quest_mobile_published_saved_forest::proclib_probe(size_t row, size_t description,
							P_obj fresh) noexcept
{
	if (!consumed_ || row >= reload_.size() || description >= reload_[row].probes.size() ||
	    !row_valid(row, fresh))
		return false;
	auto &state = reload_[row];
	if (!returned(state.steps[0]) || !returned(state.steps[1]))
		return false;
	for (size_t previous = 0; previous < description; ++previous)
		if (!returned(state.probes[previous]))
			return false;
	auto &effect = state.probes[description];
	if (effect.started)
		return returned(effect);
	return shop_trade_original_item_stage::proclib_probe(fresh, *templates_[row], description,
							     effect);
}

bool quest_mobile_published_saved_forest::reload_step(size_t row, unsigned int step,
						      P_obj fresh) noexcept
{
	if (!consumed_ || row >= reload_.size() || step >= reload_[row].steps.size() ||
	    !row_valid(row, fresh))
		return false;
	auto &state = reload_[row];
	for (size_t previous = 0; previous < step; ++previous)
		if (!returned(state.steps[previous]))
			return false;
	auto &effect = state.steps[step];
	if (effect.started)
		return returned(effect);
	if (step == 1)
		effect.periodic = state.steps[0].periodic;
	if (step == 2)
	{
		bool periodic = false;
		for (const auto &probe : state.probes)
		{
			if (!returned(probe))
				return false;
			periodic = periodic || probe.periodic;
		}
		effect.periodic = periodic;
	}
	return shop_trade_original_item_stage::reload_step(fresh, *templates_[row], step, effect);
}

const quest_mobile_published_saved_forest::reload_state *
quest_mobile_published_saved_forest::effects(size_t row) const noexcept
{
	return row < reload_.size() ? &reload_[row] : nullptr;
}
bool quest_mobile_published_saved_forest::reload_complete() const noexcept
{
	if (!consumed_)
		return false;
	for (const auto &state : reload_)
	{
		for (const auto &effect : state.steps)
			if (!returned(effect))
				return false;
		for (const auto &probe : state.probes)
			if (!returned(probe))
				return false;
	}
	return true;
}

size_t quest_mobile_published_saved_forest::retained_bytes() const noexcept
{
	size_t bytes = sizeof(*this);
	const size_t binding = bindings_.retained_bytes();
	if (!binding || !add(bytes, binding) ||
	    !add(bytes, source_.items.capacity(), sizeof(player_item_snapshot)) ||
	    !add(bytes, detached_.capacity(), sizeof(player_item_snapshot)) ||
	    !add(bytes, staged_.capacity(), sizeof(staged_[0])) ||
	    !add(bytes, templates_.capacity(), sizeof(templates_[0])) ||
	    !add(bytes, objects_.capacity(), sizeof(objects_[0])) ||
	    !add(bytes, reload_.capacity(), sizeof(reload_[0])))
		return 0;
	for (const auto &item : source_.items)
		if (!literal_bytes(item, bytes))
			return 0;
	for (size_t row = 0; row < detached_.size(); ++row)
	{
		const auto &literal = detached_[row];
		if (!literal_bytes(literal, bytes) ||
		    !add(bytes, reload_[row].probes.capacity(),
			 sizeof(shop_trade_original_reload_effect)))
			return 0;
		// Detached allocations only. Consumed native bodies belong to root's
		// separately charged world lifetime, never this holder's destructor.
		if (consumed_)
			continue;
		if (!add(bytes, sizeof(obj_data)) ||
		    !add(bytes, literal.dynamic_affects.size(), sizeof(obj_affect)) ||
		    !add(bytes, literal.extra_descriptions.size(), sizeof(extra_descr_data)))
			return 0;
		for (const auto *text : { &literal.name, &literal.short_description,
					  &literal.description, &literal.action_description })
			if (!add(bytes, text->size()) || !add(bytes, 1))
				return 0;
		for (const auto &description : literal.extra_descriptions)
			if (!add(bytes, description.spellbook ? 3 : description.keyword.size()) ||
			    !add(bytes, 1) ||
			    !add(bytes, description.spellbook ? (MAX_SKILLS + 1) / 8 + 1 :
								description.description.size()) ||
			    !add(bytes, 1))
				return 0;
	}
	return bytes;
}
