#ifndef DURIS_NEWBIE_KIT_PLAN_H
#define DURIS_NEWBIE_KIT_PLAN_H
#include <vector>

struct newbie_kit_input
{
	int race = 0;
	int alignment = 0;
	int main_class = 0;
	bool all_classes = false;
	bool blighter = false;
	bool ailvio = false;
	bool bandages = false;
	bool shield = false;
};
struct newbie_kit_item
{
	int vnum;
	bool regular; // cost/transience, spellbook and weapon rules apply
};
struct newbie_item_facts
{
	bool available = false;
	bool weapon_admitted = true;
	bool spellbook = false;
};
struct prepared_newbie_item
{
	newbie_kit_item item;
	std::vector<int> spells;
};
std::vector<prepared_newbie_item>
prepare_newbie_kit_items(const std::vector<newbie_kit_item> &selection,
			 const std::vector<newbie_item_facts> &facts,
			 const std::vector<int> &first_circle);

// Owned, deterministic selection. No character/object pointers or runtime mutations.
std::vector<newbie_kit_item> make_newbie_kit_plan(const newbie_kit_input &input);
std::vector<int> newbie_kit_template_vnums();
#endif
