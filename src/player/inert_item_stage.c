#include "player/inert_item_stage.h"
#include "player/player_snapshot.h"
#include "world/object_template.h"
#include "core/prototypes.h"
#include "core/defines.h"
#include "core/mm.h"
#include <cstring>
#include <utility>
#include "player/player_load_items.h"
#include <limits>
#include <unordered_set>

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

static inert_item_stage_result literal_stage_eligibility(const object_template &prototype,
							 const player_item_snapshot &literal,
							 bool money) noexcept
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
	    literal.equipment_slot != (money ? -1 : 0) ||
	    literal.extra_descriptions.size() > max_descriptions)
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
	if (obj_index[number].func.obj ||
	    (money ? (prototype.type != ITEM_MONEY || literal.type != ITEM_MONEY) :
		     (unsupported_type(prototype.type) || unsupported_type(literal.type))) ||
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
	if (!money)
		for (auto timer : literal.timers)
			if (timer != 0)
				return inert_item_stage_result::unsupported;
	return inert_item_stage_result::ok;
}

inert_item_stage_result inert_item_stage_eligibility(const object_template &prototype,
						     const player_item_snapshot &literal) noexcept
{
	return literal_stage_eligibility(prototype, literal, false);
}

inert_item_stage_result prepare_inert_item_stage(const object_template &prototype,
						 const player_item_snapshot &literal,
						 inert_item_stage &output) noexcept
{
	const auto eligibility = inert_item_stage_eligibility(prototype, literal);
	if (eligibility != inert_item_stage_result::ok)
		return eligibility;
	return inert_item_stage::allocate_literal(prototype, literal, output);
}

inert_item_stage_result
prepare_inert_money_stage(const player_item_snapshot &literal, uint64_t original_uid,
			  const std::array<int32_t, 4> &verified_denominations,
			  inert_item_stage &output) noexcept
{
	if (!original_uid || original_uid != literal.object_uid || literal.type != ITEM_MONEY ||
	    literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT || literal.equipment_slot != -1 ||
	    literal.extra_descriptions.size() > 1)
		return inert_item_stage_result::invalid;
	bool nonempty = false;
	for (size_t index = 0; index < verified_denominations.size(); ++index)
	{
		if (verified_denominations[index] < 0 ||
		    literal.values[index] != verified_denominations[index])
			return inert_item_stage_result::invalid;
		nonempty = nonempty || verified_denominations[index] != 0;
	}
	if (!nonempty)
		return inert_item_stage_result::invalid;
	if (!recovery_object_templates_ready())
		return inert_item_stage_result::allocation_unavailable;
	const object_template *prototype = find_recovery_object_template(literal.vnum);
	if (!prototype)
		return inert_item_stage_result::unsupported;
	const auto eligibility = literal_stage_eligibility(*prototype, literal, true);
	if (eligibility != inert_item_stage_result::ok)
		return eligibility;
	// The persisted literal already contains the exact rendered descriptions and
	// native weight. Copy it unchanged; never rerender, reinterpret its unstrung
	// fields or replace the original denominations with prototype starting cash.
	return inert_item_stage::allocate_literal(*prototype, literal, output);
}

inert_item_stage_result
inert_item_stage::prepare_money_for_flat_boot(const player_item_snapshot &literal, uint64_t original_uid,
			  const std::array<int32_t, 4> &verified_denominations,
			  inert_item_stage &output) noexcept
{
	if (!original_uid || original_uid != literal.object_uid || literal.type != ITEM_MONEY ||
	    literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT || literal.equipment_slot != -1 ||
	    literal.extra_descriptions.size() > 1)
		return inert_item_stage_result::invalid;
	bool nonempty = false;
	for (size_t index = 0; index < verified_denominations.size(); ++index)
	{
		if (verified_denominations[index] < 0 ||
		    literal.values[index] != verified_denominations[index])
			return inert_item_stage_result::invalid;
		nonempty = nonempty || verified_denominations[index] != 0;
	}
	if (!nonempty)
		return inert_item_stage_result::invalid;
	if (!flatfile_coin_boot_templates::ready())
		return inert_item_stage_result::allocation_unavailable;
	const object_template *prototype = flatfile_coin_boot_templates::find(literal.vnum);
	if (!prototype)
		return inert_item_stage_result::unsupported;
	const auto eligibility = literal_stage_eligibility(*prototype, literal, true);
	if (eligibility != inert_item_stage_result::ok)
		return eligibility;
	// The persisted literal already contains the exact rendered descriptions and
	// native weight. Copy it unchanged; never rerender, reinterpret its unstrung
	// fields or replace the original denominations with prototype starting cash.
	return inert_item_stage::allocate_literal(*prototype, literal, output);
}

inert_item_stage_result inert_item_stage::allocate_literal(const object_template &prototype,
							   const player_item_snapshot &literal,
							   inert_item_stage &output) noexcept
{
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

// Original SHOP literal cleanup never extracts unpublished objects, removes
// affects through native handlers, cancels events or changes custody/global
// counts. Pool ownership is retained until the publication owner consumes all
// stages together. The legacy inert reset/eligibility functions above are intact.
shop_trade_original_item_stage::~shop_trade_original_item_stage() noexcept
{
	reset();
}
shop_trade_original_item_stage::shop_trade_original_item_stage(
	shop_trade_original_item_stage &&other) noexcept
	: object_(std::exchange(other.object_, nullptr))
	, pool_(std::exchange(other.pool_, nullptr))
	, affect_pool_(std::exchange(other.affect_pool_, nullptr))
{
}
shop_trade_original_item_stage &
shop_trade_original_item_stage::operator=(shop_trade_original_item_stage &&other) noexcept
{
	if (this != &other)
	{
		reset();
		object_ = std::exchange(other.object_, nullptr);
		pool_ = std::exchange(other.pool_, nullptr);
		affect_pool_ = std::exchange(other.affect_pool_, nullptr);
	}
	return *this;
}
void shop_trade_original_item_stage::reset() noexcept
{
	if (!object_)
		return;
	for (obj_affect *affect = object_->affects; affect;)
	{
		obj_affect *next = affect->next;
		mm_release(affect_pool_, affect);
		affect = next;
	}
	object_->affects = nullptr;
	// Reuse only the existing raw literal release, which owns scalar strings and
	// descriptions and has no graph/native effect. Dynamic nodes are gone first.
	inert_item_stage literal;
	literal.object_ = std::exchange(object_, nullptr);
	literal.pool_ = std::exchange(pool_, nullptr);
	affect_pool_ = nullptr;
}

namespace
{
// Exact original saved spellbook decoding; no strlen is used on binary bytes.
bool retained_parse_spellbook(const std::string &json, char *spell_bits = nullptr)
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
		const size_t number_start = position;
		uint64_t value = 0;
		while (position < json.size() && json[position] >= '0' && json[position] <= '9')
		{
			const uint64_t digit = static_cast<unsigned int>(json[position++] - '0');
			if (value > (static_cast<uint64_t>(MAX_SKILLS) - 1 - digit) / 10)
				return false;
			value = value * 10 + digit;
		}
		if (json[number_start] == '0' && position != number_start + 1)
			return false;
		if (seen[value])
			return false;
		seen[value] = true;
		if (spell_bits)
			spell_bits[value / 8] = static_cast<char>(
				static_cast<unsigned char>(spell_bits[value / 8]) |
				static_cast<unsigned char>(1U << (value % 8)));
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

// Captured/flatfile snapshots store typed spell IDs; SQL rows store JSON.
// Both representations feed the same validation and materialization path.
// When spell_bits is non-null, callers must provide a zero-initialized buffer
// of (MAX_SKILLS + 1) / 8 + 1 bytes because this helper ORs the selected bits
// into it rather than clearing it first.
bool retained_decode_saved_spellbook(const player_item_extra_description_snapshot &description,
				     char *spell_bits = nullptr)
{
	if (!description.description.empty())
		return description.spell_ids.empty() &&
		       retained_parse_spellbook(description.description, spell_bits);
	std::array<bool, MAX_SKILLS> seen = {};
	for (int32_t spell : description.spell_ids)
	{
		if (spell < 0 || spell >= MAX_SKILLS || seen[spell])
			return false;
		seen[spell] = true;
		if (spell_bits)
			spell_bits[spell / 8] = static_cast<char>(
				static_cast<unsigned char>(spell_bits[spell / 8]) |
				static_cast<unsigned char>(1U << (spell % 8)));
	}
	return true;
}

}

bool shop_trade_original_item_stage::retained_bytes(const player_item_snapshot &original_literal,
						    size_t *bytes_out) const noexcept
{
	if (!bytes_out)
		return false;
	if (!object_)
	{
		if (pool_ || affect_pool_)
			return false;
		*bytes_out = 0;
		return true;
	}
	try
	{
		if (!pool_ || pool_->size != sizeof(obj_data) ||
		    pool_->next_off != offsetof(obj_data, next) || !original_literal.object_uid ||
		    object_->obj_uid != original_literal.object_uid ||
		    object_->type != original_literal.type ||
		    object_->str_mask != original_literal.string_mask ||
		    original_literal.string_mask !=
			    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
		    original_literal.extra_descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
		    original_literal.dynamic_affects.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
		    !player_load_item_snapshot_metadata_valid(original_literal))
			return false;
		size_t bytes = pool_->size;
		std::unordered_set<const void *> owned;
		if (!owned.insert(object_).second)
			return false;
		const auto add = [&](size_t amount)
		{
			if (amount > std::numeric_limits<size_t>::max() - bytes)
				return false;
			bytes += amount;
			return true;
		};
		const auto allocation = [&](const void *pointer, size_t amount)
		{ return pointer && owned.insert(pointer).second && add(amount); };
		const auto literal_text = [&](const char *actual, const std::string &expected)
		{
			if (expected.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
			    expected.find('\0') != std::string::npos)
				return false;
			const size_t size = expected.size() + 1;
			return allocation(actual, size) &&
			       std::memcmp(actual, expected.c_str(), size) == 0;
		};
		if (!literal_text(object_->name, original_literal.name) ||
		    !literal_text(object_->short_description, original_literal.short_description) ||
		    !literal_text(object_->description, original_literal.description) ||
		    !literal_text(object_->action_description, original_literal.action_description))
			return false;
		const extra_descr_data *description = object_->ex_description;
		for (const auto &expected : original_literal.extra_descriptions)
		{
			if (!allocation(description, sizeof(extra_descr_data)))
				return false;
			if (expected.spellbook)
			{
				constexpr char marker[] = { 3, 1, 3, 0 };
				// prepare makes a binary std::string of the exact original bitset,
				// then allocate_text copies its extra trailing NUL too.
				std::array<char, (MAX_SKILLS + 1) / 8 + 2> bits{};
				if (!retained_decode_saved_spellbook(expected, bits.data()) ||
				    !allocation(description->keyword, sizeof(marker)) ||
				    std::memcmp(description->keyword, marker, sizeof(marker)) !=
					    0 ||
				    !allocation(description->description, bits.size()) ||
				    std::memcmp(description->description, bits.data(),
						bits.size()) != 0)
					return false;
			}
			else if (!literal_text(description->keyword, expected.keyword) ||
				 !literal_text(description->description, expected.description))
				return false;
			description = description->next;
		}
		// Bounded exact consumption also refuses cycles or extra linked nodes.
		if (description)
			return false;
		if (!original_literal.dynamic_affects.empty() &&
		    (!affect_pool_ || affect_pool_->size != sizeof(obj_affect) ||
		     affect_pool_->next_off != offsetof(obj_affect, next)))
			return false;
		if (original_literal.dynamic_affects.empty() && affect_pool_)
			return false;
		const obj_affect *affect = object_->affects;
		for (const auto &expected : original_literal.dynamic_affects)
		{
			if (!allocation(affect, affect_pool_->size) ||
			    affect->type != expected.type || affect->data != expected.data ||
			    affect->extra2 != expected.extra2)
				return false;
			affect = affect->next;
		}
		if (affect)
			return false;
		*bytes_out = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
bool literal_raw_add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}
constexpr size_t literal_raw_header_bytes() noexcept
{
#ifdef MEMCHK
	return sizeof(ALLOCATION_HEADER);
#else
	return 0;
#endif
}
}
inert_item_stage_result inert_item_stage::allocate_literal_bounded(
	const object_template &prototype, const player_item_snapshot &literal,
	inert_item_stage &output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_output_heap) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)prototype;
	(void)literal;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_output_heap;
	return inert_item_stage_result::unsupported;
#else
	struct allocation_scan
	{
		size_t heap = 0;
	};
	if (!reserve ||
	    sizeof(allocation_scan) + sizeof(const std::string *[4]) > SIZE_MAX - outer_live ||
	    !reserve(outer_live + sizeof(allocation_scan) + sizeof(const std::string *[4]),
		     context))
		return inert_item_stage_result::allocation_unavailable;
	allocation_scan scan;
	const std::string *texts[] = { &literal.name, &literal.short_description,
				       &literal.description, &literal.action_description };
	// Each __try_malloc request owns its MEMCHK header; all earlier raw requests
	// coexist until candidate destruction/transfer. No pool pages are counted twice.
	for (const auto *value : texts)
		if (value->size() == SIZE_MAX || !literal_raw_add(scan.heap, value->size() + 1) ||
		    !literal_raw_add(scan.heap, literal_raw_header_bytes()))
			return inert_item_stage_result::allocation_unavailable;
	for (const auto &description : literal.extra_descriptions)
		if (description.keyword.size() == SIZE_MAX ||
		    description.description.size() == SIZE_MAX ||
		    !literal_raw_add(scan.heap, sizeof(extra_descr_data)) ||
		    !literal_raw_add(scan.heap, description.keyword.size() + 1) ||
		    !literal_raw_add(scan.heap, description.description.size() + 1) ||
		    !literal_raw_add(scan.heap, 3 * literal_raw_header_bytes()))
			return inert_item_stage_result::allocation_unavailable;
	size_t peak = outer_live;
	if (!literal_raw_add(peak, sizeof(allocation_scan)) ||
	    !literal_raw_add(peak, sizeof(texts)) ||
	    !literal_raw_add(peak, sizeof(inert_item_stage)) ||
	    !literal_raw_add(peak, sizeof(unsigned long *[5])) ||
	    !literal_raw_add(peak, scan.heap) || !reserve(peak, context))
		return inert_item_stage_result::allocation_unavailable;
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
	if (retained_output_heap)
		*retained_output_heap = scan.heap;
	return inert_item_stage_result::ok;
#endif
}

#include "world/events.h"
extern mm_ds *dead_obj_affect_pool;
extern mm_ds_list *mmds_list;
// Serialized current retained pools, including unused pages after cleanup/refusal.
bool native_mobile_birth_literal_pool_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	const mm_ds_list *slow = mmds_list, *fast = mmds_list;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	size_t bytes = 0;
	if (dead_obj_pool && dead_obj_pool == dead_obj_affect_pool)
		return false;
	for (const mm_ds *pool : { dead_obj_pool, dead_obj_affect_pool })
	{
		if (!pool)
			continue;
		bool found = false;
		for (const auto *entry = mmds_list; entry; entry = entry->next)
			if (entry->mmds == pool)
			{
				if (found)
					return false;
				found = true;
			}
		if (!found || !literal_raw_add(bytes, sizeof(mm_ds)) ||
		    !literal_raw_add(bytes, sizeof(mm_ds_list)) ||
		    !literal_raw_add(bytes, 2 * literal_raw_header_bytes()) ||
		    pool->pages_owned > SIZE_MAX / 4096 ||
		    !literal_raw_add(bytes, pool->pages_owned * 4096))
			return false;
	}
	*output = bytes;
	return true;
#endif
}

bool shop_trade_original_item_stage::current_private_heap_bytes(const player_item_snapshot &literal,
								size_t *output) const noexcept
{
	if (!output)
		return false;
	if (!object_)
	{
		if (pool_ || affect_pool_)
			return false;
		*output = 0;
		return true;
	}
	// The original constructor allocates all private texts to exact literal
	// byte counts. The actual stage retains that original object and UID.
	if (!pool_ || pool_ != dead_obj_pool || pool_->size != sizeof(obj_data) ||
	    pool_->next_off != offsetof(obj_data, next) || !literal.object_uid ||
	    (affect_pool_ && affect_pool_ != dead_obj_affect_pool) ||
	    object_->obj_uid != literal.object_uid ||
	    literal.string_mask != (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
	    object_->str_mask != literal.string_mask)
		return false;
	size_t bytes = 0;
	const auto text = [&](const char *actual, const std::string &expected) noexcept
	{
		return actual && expected.size() != SIZE_MAX &&
		       expected.find('\0') == std::string::npos &&
		       std::memcmp(actual, expected.c_str(), expected.size() + 1) == 0 &&
		       literal_raw_add(bytes, expected.size() + 1) &&
		       literal_raw_add(bytes, literal_raw_header_bytes());
	};
	if (!text(object_->name, literal.name) ||
	    !text(object_->short_description, literal.short_description) ||
	    !text(object_->description, literal.description) ||
	    !text(object_->action_description, literal.action_description))
		return false;
	const extra_descr_data *actual = object_->ex_description;
	for (const auto &expected : literal.extra_descriptions)
	{
		if (!actual || !literal_raw_add(bytes, sizeof(extra_descr_data)) ||
		    !literal_raw_add(bytes, literal_raw_header_bytes()))
			return false;
		// Literal-guided traversal has a fixed finite end. It never builds an
		// allocating address set, and genuine constructor ownership supplies
		// distinct descriptor/text requests. A changed/cyclic tail refuses.
		if (expected.spellbook)
		{
			constexpr char marker[] = { 3, 1, 3, 0 };
			constexpr size_t bits_bytes = (MAX_SKILLS + 1) / 8 + 2;
			if (!actual->keyword || !actual->description ||
			    std::memcmp(actual->keyword, marker, sizeof(marker)) != 0 ||
			    !literal_raw_add(bytes, sizeof(marker)) ||
			    !literal_raw_add(bytes, bits_bytes) ||
			    !literal_raw_add(bytes, 2 * literal_raw_header_bytes()))
				return false;
		}
		else if (!text(actual->keyword, expected.keyword) ||
			 !text(actual->description, expected.description))
			return false;
		actual = actual->next;
	}
	if (actual)
		return false;
	*output = bytes;
	return true;
}
size_t shop_trade_original_item_stage::current_private_heap_observer_frame_bytes() noexcept
{
	// This method's arguments/result/bytes/text closure/descriptor/binding
	// reference/vector range iterators; text lambda arguments/result and actual
	// string find, char_traits::find/memchr and memcmp scalar carriers. Binary
	// spellbook byte allocation length is fixed by the original constructor;
	// this ownership observer never invokes the allocating/decoding validator.
	return 6 * sizeof(void *) + 2 * sizeof(size_t) + 3 * sizeof(bool) +
	       sizeof(const extra_descr_data *) +
	       2 * sizeof(std::vector<player_item_extra_description_snapshot>::const_iterator) +
	       10 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(char) + sizeof(int) +
	       3 * sizeof(bool) + 4 * sizeof(char);
}
