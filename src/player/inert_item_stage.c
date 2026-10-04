#include "player/inert_item_stage.h"
#include "player/player_snapshot.h"
#include "world/object_template.h"
#include "core/prototypes.h"
#include "core/defines.h"
#include "core/mm.h"
#include <cstring>
#include <utility>

extern P_index obj_index;
extern int top_of_objt;
extern mm_ds *dead_obj_pool;

namespace
{
constexpr size_t max_literal_bytes = 128 * 1024;
constexpr size_t max_descriptions = 256;

bool text(const std::string &value, size_t *bytes) noexcept
{
	if (value.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
	    value.find('\0') != std::string::npos || value.size() + 1 > max_literal_bytes - *bytes)
		return false;
	*bytes += value.size() + 1;
	return true;
}

bool contains_ascii(const std::string &value, const char *needle) noexcept
{
	const size_t length = std::strlen(needle);
	for (size_t offset = 0; offset + length <= value.size(); ++offset)
	{
		size_t matched = 0;
		for (; matched < length; ++matched)
		{
			const char c = value[offset + matched];
			if ((c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c) != needle[matched])
				break;
		}
		if (matched == length)
			return true;
	}
	return false;
}

bool unsupported_type(int type) noexcept
{
	return type == ITEM_SWITCH || type == ITEM_MONEY || type == ITEM_CORPSE ||
	       type == ITEM_SPELLBOOK;
}

bool allocate_text(const std::string &value, char **output) noexcept
{
	auto *owned = static_cast<char *>(
		__try_malloc(value.size() + 1, MEM_TAG_STRING, __FILE__, __LINE__));
	if (!owned)
		return false;
	std::memcpy(owned, value.c_str(), value.size() + 1);
	*output = owned;
	return true;
}

void release(void *value) noexcept
{
	if (value)
		__free(value, __FILE__, __LINE__);
}
}

inert_item_stage::~inert_item_stage() noexcept
{
	reset();
}
inert_item_stage::inert_item_stage(inert_item_stage &&other) noexcept
	: object_(std::exchange(other.object_, nullptr))
	, pool_(std::exchange(other.pool_, nullptr))
{
}
inert_item_stage &inert_item_stage::operator=(inert_item_stage &&other) noexcept
{
	if (this != &other)
	{
		reset();
		object_ = std::exchange(other.object_, nullptr);
		pool_ = std::exchange(other.pool_, nullptr);
	}
	return *this;
}
void inert_item_stage::reset() noexcept
{
	if (!object_)
		return;
	release(object_->name);
	release(object_->short_description);
	release(object_->description);
	release(object_->action_description);
	for (auto *description = object_->ex_description; description;)
	{
		auto *next = description->next;
		release(description->keyword);
		release(description->description);
		release(description);
		description = next;
	}
	// Unsupported dynamic affects never acquire nodes. No recursive graph cleanup,
	// extraction, effect/procedure/event callback or custody mutation is permitted.
	mm_release(pool_, object_);
	object_ = nullptr;
	pool_ = nullptr;
}

inert_item_stage_result inert_item_stage_eligibility(const object_template &prototype,
						     const player_item_snapshot &literal) noexcept
{
	const int number = prototype.R_num;
	if (!obj_index || number < 0 || number > top_of_objt || literal.vnum <= 0 ||
	    obj_index[number].virtual_number != literal.vnum || !literal.object_uid ||
	    prototype.type < ITEM_LOWEST || prototype.type > ITEM_LAST ||
	    prototype.name.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
	    prototype.descriptions.size() > max_descriptions ||
	    !std::in_range<unsigned long>(literal.object_uid) ||
	    !std::in_range<long>(literal.generated_key) || literal.type < ITEM_LOWEST ||
	    literal.type > ITEM_LAST ||
	    literal.string_mask != (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
	    literal.equipment_slot != 0 || literal.extra_descriptions.size() > max_descriptions)
		return inert_item_stage_result::invalid;
	size_t bytes = 0;
	if (!text(literal.name, &bytes) || !text(literal.short_description, &bytes) ||
	    !text(literal.description, &bytes) || !text(literal.action_description, &bytes))
		return inert_item_stage_result::invalid;
	for (const auto &description : literal.extra_descriptions)
	{
		if (sizeof(extra_descr_data) > max_literal_bytes - bytes)
			return inert_item_stage_result::invalid;
		bytes += sizeof(extra_descr_data);
		if (!text(description.keyword, &bytes) || !text(description.description, &bytes))
			return inert_item_stage_result::invalid;
	}
	for (const auto &affect : literal.affects)
		if (!std::in_range<decltype(std::declval<obj_data &>().affected[0].location)>(
			    affect[0]) ||
		    !std::in_range<decltype(std::declval<obj_data &>().affected[0].modifier)>(
			    affect[1]))
			return inert_item_stage_result::invalid;
	for (auto value : literal.bitvectors)
		if (!std::in_range<unsigned long>(value))
			return inert_item_stage_result::invalid;
	for (auto timer : literal.timers)
		if (!std::in_range<time_t>(timer))
			return inert_item_stage_result::invalid;

	constexpr uint32_t dynamic_flags = ITEM_PROCLIB | ITEM_ARTIFACT | ITEM_TRANSIENT;
	if (obj_index[number].func.obj || unsupported_type(prototype.type) ||
	    unsupported_type(literal.type) ||
	    ((prototype.extra_flags | literal.extra_flags) & dynamic_flags) ||
	    contains_ascii(prototype.name, "random_exit") ||
	    contains_ascii(literal.name, "random_exit") || prototype.trap_eff ||
	    prototype.trap_dam || prototype.trap_charge || prototype.trap_level ||
	    !literal.dynamic_affects.empty())
		return inert_item_stage_result::unsupported;
	for (const auto &description : prototype.descriptions)
		if (description.keyword.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
		    contains_ascii(description.keyword, "_proclib_"))
			return inert_item_stage_result::unsupported;
	for (const auto &description : literal.extra_descriptions)
		if (description.spellbook || !description.spell_ids.empty() ||
		    contains_ascii(description.keyword, "_proclib_"))
			return inert_item_stage_result::unsupported;
	for (auto timer : literal.timers)
		if (timer != 0)
			return inert_item_stage_result::unsupported;
	return inert_item_stage_result::ok;
}

inert_item_stage_result prepare_inert_item_stage(const object_template &prototype,
						 const player_item_snapshot &literal,
						 inert_item_stage &output) noexcept
{
	const auto eligibility = inert_item_stage_eligibility(prototype, literal);
	if (eligibility != inert_item_stage_result::ok)
		return eligibility;
	const int number = prototype.R_num;
	if (!dead_obj_pool || dead_obj_pool->size != sizeof(obj_data) ||
	    dead_obj_pool->next_off != offsetof(obj_data, next))
		return inert_item_stage_result::allocation_unavailable;
	inert_item_stage candidate;
	candidate.pool_ = dead_obj_pool;
	candidate.object_ = static_cast<P_obj>(mm_try_get(candidate.pool_));
	if (!candidate.object_)
		return inert_item_stage_result::allocation_unavailable;
	P_obj object = candidate.object_;
	object->R_num = number;
	object->obj_uid = static_cast<unsigned long>(literal.object_uid);
	object->g_key = static_cast<long>(literal.generated_key);
	object->type = literal.type;
	object->str_mask = literal.string_mask;
	object->loc_p = LOC_NOWHERE;
	object->loc.room = NOWHERE;
	object->material = literal.material;
	object->craftsmanship = literal.craftsmanship;
	object->wear_flags = literal.wear_flags;
	object->extra_flags = literal.extra_flags;
	object->extra2_flags = literal.extra2_flags;
	object->anti_flags = literal.anti_flags;
	object->anti2_flags = literal.anti2_flags;
	object->weight = literal.weight;
	object->cost = literal.cost;
	object->condition = literal.condition;
	static_assert(sizeof(object->value) / sizeof(object->value[0]) == 8);
	for (size_t index = 0; index < literal.values.size(); ++index)
		object->value[index] = literal.values[index];
	for (size_t index = 0; index < literal.timers.size(); ++index)
		object->timer[index] = static_cast<time_t>(literal.timers[index]);
	unsigned long *bitvectors[] = { &object->bitvector, &object->bitvector2,
					&object->bitvector3, &object->bitvector4,
					&object->bitvector5 };
	for (size_t index = 0; index < literal.bitvectors.size(); ++index)
		*bitvectors[index] = static_cast<unsigned long>(literal.bitvectors[index]);
	static_assert(sizeof(object->affected) / sizeof(object->affected[0]) == 4);
	for (size_t index = 0; index < literal.affects.size(); ++index)
	{
		object->affected[index].location =
			static_cast<decltype(object->affected[index].location)>(
				literal.affects[index][0]);
		object->affected[index].modifier =
			static_cast<decltype(object->affected[index].modifier)>(
				literal.affects[index][1]);
	}
	if (!allocate_text(literal.name, &object->name) ||
	    !allocate_text(literal.short_description, &object->short_description) ||
	    !allocate_text(literal.description, &object->description) ||
	    !allocate_text(literal.action_description, &object->action_description))
		return inert_item_stage_result::allocation_unavailable;
	extra_descr_data **tail = &object->ex_description;
	for (const auto &description : literal.extra_descriptions)
	{
		auto *node = static_cast<extra_descr_data *>(__try_malloc(
			sizeof(extra_descr_data), MEM_TAG_EXDESCD, __FILE__, __LINE__));
		if (!node)
			return inert_item_stage_result::allocation_unavailable;
		std::memset(node, 0, sizeof(*node));
		*tail = node;
		tail = &node->next;
		if (!allocate_text(description.keyword, &node->keyword) ||
		    !allocate_text(description.description, &node->description))
			return inert_item_stage_result::allocation_unavailable;
	}
	output = std::move(candidate);
	return inert_item_stage_result::ok;
}
