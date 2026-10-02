#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/chaos_pouch_publication.h"
#include "item/craft_pouch_mutation.h"
#include "world/vnum.obj.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

static index_data prototype = {};
P_index obj_index = &prototype;
static int allocations = 0;
static int outstanding = 0;
static int fail_at = -1;

void *__malloc(size_t size, const char *, const char *, int)
{
	if (allocations++ == fail_at)
		return nullptr;
	auto result = std::malloc(size);
	if (result)
		++outstanding;
	return result;
}
void __free(void *value, const char *, int)
{
	if (value)
	{
		--outstanding;
		std::free(value);
	}
}
char *str_dup(const char *value)
{
	auto result = static_cast<char *>(__malloc(std::strlen(value) + 1, nullptr, nullptr, 0));
	if (result)
		std::strcpy(result, value);
	return result;
}
void str_free(const char *value)
{
	__free(const_cast<char *>(value), nullptr, 0);
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

static extra_descr_data *entry(const char *keyword, const char *description)
{
	auto result = static_cast<extra_descr_data *>(
		__malloc(sizeof(extra_descr_data), nullptr, nullptr, 0));
	assert(result);
	*result = {};
	result->keyword = str_dup(keyword);
	result->description = str_dup(description);
	assert(result->keyword && result->description);
	return result;
}

static void clear(obj_data *pouch)
{
	while (pouch->ex_description)
	{
		auto old = pouch->ex_description;
		pouch->ex_description = old->next;
		str_free(old->keyword);
		str_free(old->description);
		__free(old, nullptr, 0);
	}
}

int main()
{
	prototype.virtual_number = VOBJ_CHAOS_CRAFT_POUCH;
	craft_pouch_mutation mutation;
	mutation.before.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	mutation.before.object_uid = 72;
	mutation.before.vnum = VOBJ_CHAOS_CRAFT_POUCH;
	mutation.before.extra_descriptions.push_back(
		{ "CHAOS_POUCH_LEDGER_0", "0:7:0;", false, {} });
	mutation.mode = chaos_pouch_usage_mode::generated;
	mutation.usage = { { 400000, 12 } };
	assert(chaos_pouch_ledger_prepare(mutation.before, mutation.usage, mutation.mode,
					  &mutation.after) == chaos_pouch_ledger_result::ok);
	for (int fail = 0; fail < 4; ++fail)
	{
		obj_data pouch = {};
		pouch.R_num = 0;
		pouch.obj_uid = 72;
		pouch.cost = 333;
		pouch.ex_description = entry("inscription", "preserve pointer and text");
		pouch.ex_description->next = entry("CHAOS_POUCH_LEDGER_0", "0:7:0;");
		auto inscription = pouch.ex_description;
		auto ledger = inscription->next;
		const int initial_outstanding = outstanding;
		allocations = 0;
		fail_at = fail < 3 ? fail : -1;
		const bool published = chaos_pouch_publish_committed(&pouch, mutation);
		fail_at = -1;
		assert(published == (fail == 3));
		assert(pouch.cost == 333 && pouch.obj_uid == 72);
		assert(outstanding == initial_outstanding);
		if (!published)
		{
			assert(pouch.ex_description == inscription && inscription->next == ledger);
			assert(std::strcmp(ledger->description, "0:7:0;") == 0);
		}
		else
		{
			assert(pouch.ex_description->next == inscription);
			assert(std::strcmp(pouch.ex_description->description, "0:19:0;") == 0);
			allocations = 0;
			fail_at = 0;
			assert(chaos_pouch_publish_committed(&pouch, mutation));
			assert(allocations == 0);
			fail_at = -1;
			auto changed = mutation;
			++changed.after.cost;
			assert(!chaos_pouch_publish_committed(&pouch, changed));
		}
		clear(&pouch);
		assert(outstanding == 0);
	}
	obj_data drifted = {};
	drifted.R_num = 0;
	drifted.obj_uid = 72;
	drifted.ex_description = entry("CHAOS_POUCH_LEDGER_0", "0:20:0;");
	assert(!chaos_pouch_publish_committed(&drifted, mutation));
	assert(std::strcmp(drifted.ex_description->description, "0:20:0;") == 0);
	clear(&drifted);
	assert(outstanding == 0);
}
