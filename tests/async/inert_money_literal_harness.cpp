#include "player/inert_item_stage.h"
#include "player/player_snapshot.h"
#include "world/object_template.h"
#include "core/prototypes.h"
#include "core/mm.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
P_index obj_index = nullptr;
int top_of_objt = 0;
mm_ds *dead_obj_pool = nullptr;
P_obj object_list = nullptr;
unsigned long next_obj_uid = 77001;
extern void init_mem_used();
static object_template prototype;
// Read-only registry seam: no production enrollment, SQL receipt or game success.
bool recovery_object_templates_ready() noexcept
{
	return true;
}
const object_template *find_recovery_object_template(int vnum) noexcept
{
	return vnum == 3 ? &prototype : nullptr;
}
void logit(const char *, const char *, ...)
{
	std::abort();
}
void fatal_boot_error(const char *, const char *, ...)
{
	std::abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
static void check(bool value, const char *message)
{
	if (!value)
	{
		std::fprintf(stderr, "FAIL %s\n", message);
		std::exit(1);
	}
}
int main(int argc, char **argv)
{
	check(argc == 2, "case argument");
	init_mem_used();
	obj_data slot = {}, sentinel = {};
	index_data index = {};
	mm_ds pool = {};
	pool.size = sizeof(obj_data);
	pool.next_off = offsetof(obj_data, next);
	pool.head = pool.tail = reinterpret_cast<char *>(&slot);
	pool.chunk_size = 1;
	index.virtual_number = 3;
	index.number = 17;
	obj_index = &index;
	dead_obj_pool = &pool;
	object_list = &sentinel;
	prototype.R_num = 0;
	prototype.type = ITEM_MONEY;
	prototype.name = "coins";
	player_item_snapshot literal = {};
	literal.vnum = 3;
	literal.object_uid = 99001;
	literal.type = ITEM_MONEY;
	literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	literal.equipment_slot = -1;
	literal.string_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
	literal.name = "coins";
	literal.short_description = "a silver coin";
	literal.description = "A silver coin is here.";
	literal.values[1] = 1;
	const std::array<int32_t, 4> amounts = { 0, 1, 0, 0 };
	auto expected = inert_item_stage_result::ok;
	const bool zero = std::strcmp(argv[1], "zero") == 0;
	if (!zero)
	{
		player_item_extra_description_snapshot detail = {};
		detail.keyword = "coins";
		detail.description = "One silver coin.";
		literal.extra_descriptions.push_back(detail);
	}
	if (std::strcmp(argv[1], "two") == 0)
	{
		literal.extra_descriptions.push_back(literal.extra_descriptions[0]);
		expected = inert_item_stage_result::invalid;
	}
	if (std::strcmp(argv[1], "malformed") == 0)
	{
		literal.extra_descriptions[0].description = std::string("bad\0tail", 8);
		expected = inert_item_stage_result::invalid;
	}
	if (std::strcmp(argv[1], "spellbook") == 0)
	{
		literal.extra_descriptions[0].spellbook = true;
		expected = inert_item_stage_result::unsupported;
	}
	if (std::strcmp(argv[1], "identity") == 0)
	{
		literal.object_uid = 99002;
		expected = inert_item_stage_result::invalid;
	}
	if (std::strcmp(argv[1], "currency") == 0)
	{
		literal.values[1] = 2;
		expected = inert_item_stage_result::invalid;
	}
	{
		inert_item_stage stage;
		const auto result = prepare_inert_money_stage(literal, 99001, amounts, stage);
		check(result == expected, "actual full TU result");
		if (expected == inert_item_stage_result::ok)
		{
			const auto *object = stage.get();
			check(object && object->obj_uid == 99001 && object->value[1] == 1,
			      "unchanged identity and currency");
			check(object->loc_p == LOC_NOWHERE && !object->next && !object->prev &&
				      !object->next_content,
			      "unpublished inert object");
			check(zero ? object->ex_description == nullptr :
				     (object->ex_description && !object->ex_description->next &&
				      std::strcmp(
					      object->ex_description->description,
					      literal.extra_descriptions[0].description.c_str()) ==
					      0),
			      "literal absence or exact one preserved");
		}
		else
			check(!stage.get() && pool.objs_used == 0, "refusal before owned output");
	}
	check(pool.objs_used == 0 && index.number == 17 && object_list == &sentinel &&
		      next_obj_uid == 77001,
	      "cleanup and no publication");
	std::printf("PASS %s\n", argv[1]);
	return 0;
}
