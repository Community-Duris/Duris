#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "item/objmisc.h"
#include "world/events.h"
#include "magic/spells.h"
#include "persistence/persistence_checkpoint.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static bool bound_ok = true, usable = true, duplicate = false;
static extra_descr_data description{};
static int scheduled = 0, cancelled = 0, completed = 0;
static P_obj started_book = nullptr;
static scribing_data_type pending{};
static std::string message;
static obj_data source_book{};
static str_app_type strength[256]{};
const str_app_type *str_app = strength;
const char *apply_types[APPLY_LAST + 1]{};
Skill skills[MAX_SKILLS]{};
char Gbuf1[MAX_STRING_LENGTH];
void event_scribe(P_char, P_char, P_obj, void *);
void send_to_char(const char *s, P_char)
{
	message += s;
}
void act(const char *, int, P_char, P_obj, void *, int) {}
void wizlog(int, const char *, ...) {}
void MakeScrap(P_char, P_obj)
{
	assert(false);
}
void sprinttype(int, const char **, char *) {}
bool can_equip_soulbound_item(P_char, P_obj, bool)
{
	return bound_ok;
}
int can_char_use_item(P_char, P_obj)
{
	return usable;
}
int GET_CLASS(P_char ch, uint cls)
{
	return ch->player.m_class & cls;
}
int get_property(const char *, int value)
{
	return value;
}
void execute_wear(P_char ch, P_obj obj, int slot, int, bool)
{
	assert(!ch->equipment[slot]);
	assert(obj->loc_p == LOC_CARRIED);
	ch->equipment[slot] = obj;
	obj->loc_p = LOC_WORN;
	obj->loc.wearing = ch;
}
extra_descr_data *find_spell_description(P_obj)
{
	return &description;
}
int SpellInThisSpellBook(extra_descr_data *, int)
{
	return duplicate;
}
int GetSpellPages(P_char, int)
{
	return 2;
}
bool ac_can_see_obj(P_char, P_obj obj, int)
{
	return !IS_SET(obj->extra_flags, ITEM_INVISIBLE);
}
P_obj find_gh_library_book_obj(P_char)
{
	return &source_book;
}
P_obj Find_process_entry(P_char, P_obj obj, int)
{
	return obj;
}
int real_object(int number)
{
	return number;
}
bool isname(const char *, const char *)
{
	return false;
}
P_char FindTeacher(P_char)
{
	return nullptr;
}
int knows_spell(P_char, int)
{
	return true;
}
char *skip_spaces(char *s)
{
	return s;
}
int old_search_block(const char *, int, int, char **, int)
{
	return 1;
}
char *spells[2]{};
void add_scribing(P_char ch, int spell, P_obj book, int, P_obj, P_char)
{
	started_book = book;
	pending = {};
	pending.book = book;
	pending.spell = spell;
	SET_BIT(ch->specials.affected_by2, AFF2_SCRIBING);
}
void disarm_char_nevents(P_char, event_func)
{
	++cancelled;
}
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int, const void *, int)
{
	++scheduled;
	return {};
}
void extract_obj(P_obj, int)
{
	assert(false);
}
int AddSpellToSpellBook(P_char, P_obj, int)
{
	++completed;
	return true;
}
void mark_player_dirty_components(int, player_component_mask_t) {}
bool notch_skill(P_char, int, float)
{
	return false;
}

void prac_all_spells(P_char)
{
	assert(false);
}
bool is_stat_max(signed char)
{
	return false;
}
int STAT_INDEX(int)
{
	return 100;
}
int GET_CHAR_SKILL_P(P_char ch, int skill)
{
	return ch->only.pc->skills[skill].learned;
}
int IS_WARRIOR(P_char ch)
{
	return GET_CLASS(ch, CLASS_WARRIOR);
}
int IS_THIEF(P_char ch)
{
	return GET_CLASS(ch, CLASS_THIEF);
}
int lookup_spell(const char *, int)
{
	return 1;
}
void mobsay(P_char, const char *)
{
	assert(false);
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
// PRODUCTION_FUNCTIONS

static pc_only_data pc{};
static char_data actor{};
static int permutations = 0;
static void reset(int race = RACE_HUMAN, bool four_arms = false)
{
	pc = {};
	actor = {};
	actor.only.pc = &pc;
	actor.player.race = race;
	actor.curr_stats.Str = 100;
	actor.specials.position = STAT_RESTING | POS_SITTING;
	if (four_arms)
		SET_BIT(actor.specials.affected_by3, AFF3_FOUR_ARMS);
	pc.skills[SKILL_DUAL_WIELD].learned = 100;
	pc.skills[SKILL_SCRIBE].learned = 100;
	bound_ok = usable = true;
	duplicate = false;
	scheduled = cancelled = completed = 0;
	started_book = nullptr;
	message.clear();
}
static obj_data item(int type, int flags, bool two = false, int weight = 1)
{
	obj_data obj{};
	obj.type = type;
	obj.wear_flags = flags;
	obj.extra_flags = two ? ITEM_TWOHANDS : 0;
	obj.condition = 100;
	obj.weight = weight;
	obj.loc_p = LOC_CARRIED;
	obj.value[2] = 10;
	return obj;
}
static void remove(P_obj obj)
{
	bool found = false;
	for (auto &equipped : actor.equipment)
	{
		if (equipped == obj)
		{
			equipped = nullptr;
			found = true;
		}
	}
	assert(found);
	obj->loc_p = LOC_CARRIED;
}
static void scribe_cycle(P_obj book, P_obj pen)
{
	assert(ScriberSillyChecks(&actor, 1));
	duplicate = true;
	assert(!ScriberSillyChecks(&actor, 1));
	duplicate = false;
	book->value[2] = 1;
	assert(!ScriberSillyChecks(&actor, 1));
	book->value[2] = 10;
	char spell[] = "fixture";
	do_scribe(&actor, spell, 0);
	assert(started_book == book && IS_AFFECTED2(&actor, AFF2_SCRIBING));
	event_scribe(&actor, nullptr, nullptr, &pending);
	assert(pending.page == 1 && book->value[3] == 1 && scheduled == 1);
	event_scribe(&actor, nullptr, nullptr, &pending);
	assert(completed == 1 && book->value[3] == 2);
	assert(!IS_AFFECTED2(&actor, AFF2_SCRIBING));
	// A missing pen cancels before another page is consumed.
	do_scribe(&actor, spell, 0);
	remove(pen);
	event_scribe(&actor, nullptr, nullptr, &pending);
	assert(cancelled == 1 && book->value[3] == 2);
	assert(!IS_AFFECTED2(&actor, AFF2_SCRIBING));
	assert(!ScriberSillyChecks(&actor, 1));
	assert(wear(&actor, pen, 13, false));
	// An alternate book cannot replace the original destination mid-event.
	do_scribe(&actor, spell, 0);
	remove(book);
	auto replacement = item(ITEM_SPELLBOOK, ITEM_HOLD);
	assert(wear(&actor, &replacement, 13, false));
	event_scribe(&actor, nullptr, nullptr, &pending);
	assert(cancelled == 2 && replacement.value[3] == 0 && book->value[3] == 2);
	assert(!IS_AFFECTED2(&actor, AFF2_SCRIBING));
	remove(&replacement);
	assert(wear(&actor, book, 13, false));
}
static void combinations(std::vector<obj_data> items, std::vector<int> roles, int race = RACE_HUMAN,
			 bool four_arms = false)
{
	std::vector<int> order;
	for (int i = 0; i < (int)items.size(); ++i)
		order.push_back(i);
	do
	{
		reset(race, four_arms);
		const int capacity = HAS_FOUR_HANDS(&actor) ? 4 : 2;
		auto objects = items;
		int used = 0;
		P_obj book = nullptr, pen = nullptr;
		for (int i : order)
		{
			auto &obj = objects[i];
			const int cost = wield_item_size(&actor, &obj);
			assert(wear(&actor, &obj, roles[i], false));
			used += cost;
			assert(get_numb_free_hands(&actor) == capacity - used);
			if (obj.type == ITEM_SPELLBOOK)
				book = &obj;
			if (obj.type == ITEM_PEN)
				pen = &obj;
		}
		if (used == capacity)
		{
			for (int role : { 12, 13, 14 })
			{
				auto extra = item(ITEM_WEAPON,
						  ITEM_WIELD | ITEM_HOLD | ITEM_WEAR_SHIELD);
				assert(!wear(&actor, &extra, role, false));
				assert(extra.loc_p == LOC_CARRIED);
			}
		}
		if (book && pen)
			scribe_cycle(book, pen);
		for (int i : order)
		{
			used -= wield_item_size(&actor, &objects[i]);
			remove(&objects[i]);
			assert(get_numb_free_hands(&actor) == capacity - used);
		}
		++permutations;
	} while (std::next_permutation(order.begin(), order.end()));
}
int main()
{
	for (auto &row : strength)
		row.wield_w = 100;
	skills[1].name = "fixture";
	auto weapon = item(ITEM_WEAPON, ITEM_WIELD);
	auto great = item(ITEM_WEAPON, ITEM_WIELD, true);
	auto book = item(ITEM_SPELLBOOK, ITEM_HOLD);
	auto pen = item(ITEM_PEN, ITEM_HOLD);
	auto shield = item(ITEM_ARMOR, ITEM_WEAR_SHIELD);
	auto twoheld = item(ITEM_ARMOR, ITEM_HOLD, true);
	auto twoshield = item(ITEM_ARMOR, ITEM_WEAR_SHIELD, true);
	combinations({ book, pen }, { 13, 13 });
	combinations({ weapon, shield }, { 12, 14 });
	combinations({ weapon, book }, { 12, 13 });
	combinations({ shield, book }, { 14, 13 });
	combinations({ weapon, weapon }, { 12, 12 });
	combinations({ great }, { 12 });
	combinations({ twoheld }, { 13 });
	combinations({ twoshield }, { 14 });
	for (bool effect : { false, true })
	{
		int race = effect ? RACE_HUMAN : RACE_THRIKREEN;
		combinations({ weapon, weapon, weapon, weapon }, { 12, 12, 12, 12 }, race, effect);
		combinations({ book, pen, item(ITEM_ARMOR, ITEM_HOLD),
			       item(ITEM_ARMOR, ITEM_HOLD) },
			     { 13, 13, 13, 13 }, race, effect);
		combinations({ weapon, shield, book, pen }, { 12, 14, 13, 13 }, race, effect);
		combinations({ great, great }, { 12, 12 }, race, effect);
		combinations({ great, weapon, weapon }, { 12, 12, 12 }, race, effect);
		combinations({ great, book, pen }, { 12, 13, 13 }, race, effect);
		combinations({ twoheld, shield, weapon }, { 13, 14, 12 }, race, effect);
	}
	// Racial size reduces weapon cost, not total capacity. Minotaurs retain
	// two-handed weapons in the primary slot, even with only one hand free.
	for (int race : { RACE_GIANT, RACE_OGRE, RACE_SGIANT, RACE_MINOTAUR, RACE_SNOW_OGRE,
			  RACE_FIRBOLG, RACE_FIREGIANT, RACE_FROSTGIANT })
	{
		reset(race);
		assert(wield_item_size(&actor, &great) == 1);
		assert(wield_item_size(&actor, &twoheld) == 2);
		assert(wield_item_size(&actor, &twoshield) == 2);
		combinations({ great, shield }, { 12, 14 }, race);
		combinations({ great, book }, { 12, 13 }, race);
		combinations({ great, great }, { 12, 12 }, race);
		combinations({ book, pen }, { 13, 13 }, race);
		combinations({ twoheld }, { 13 }, race);
		combinations({ twoshield }, { 14 }, race);
		combinations({ great, great, great, great }, { 12, 12, 12, 12 }, race, true);
		combinations({ great, shield, book, pen }, { 12, 14, 13, 13 }, race, true);
		reset(race);
		auto primary = great;
		auto offhand = shield;
		assert(get_numb_free_hands(&actor) == 2);
		assert(wear(&actor, &offhand, 14, false));
		assert(get_numb_free_hands(&actor) == 1);
		assert(wear(&actor, &primary, 12, false));
		assert(actor.equipment[PRIMARY_WEAPON] == &primary);
		assert(get_numb_free_hands(&actor) == 0);
		remove(&primary);
		assert(get_numb_free_hands(&actor) == 1);
		remove(&offhand);
		assert(get_numb_free_hands(&actor) == 2);
		auto onehand = item(ITEM_ARMOR, ITEM_HOLD);
		auto costly = twoheld;
		assert(wear(&actor, &onehand, 13, false));
		assert(!wear(&actor, &costly, 13, false));
		auto costly_shield = twoshield;
		assert(!wear(&actor, &costly_shield, 14, false));
		assert(get_numb_free_hands(&actor) == 1);
		remove(&onehand);
		// Holding a discounted weapon also admits it with just one hand left.
		auto held_weapon = item(ITEM_WEAPON, ITEM_HOLD, true);
		assert(wear(&actor, &primary, 12, false));
		assert(wear(&actor, &held_weapon, 13, false));
		assert(get_numb_free_hands(&actor) == 0);
	}
	reset();
	auto old_sword = item(ITEM_WEAPON, ITEM_WIELD);
	old_sword.value[0] = WEAPON_2HANDSWORD;
	assert(wield_item_size(&actor, &old_sword) == 2);
	auto occupied = item(ITEM_ARMOR, ITEM_HOLD);
	assert(wear(&actor, &occupied, 13, false));
	assert(!wear(&actor, &old_sword, 12, false));
	assert(get_numb_free_hands(&actor) == 1);
	auto nonweapon = item(ITEM_ARMOR, ITEM_HOLD);
	nonweapon.value[0] = WEAPON_2HANDSWORD;
	assert(wield_item_size(&actor, &nonweapon) == 1);
	// A non-weapon occupying WIELD must not require dual-weapon training.
	reset();
	pc.skills[SKILL_DUAL_WIELD].learned = 0;
	assert(wear(&actor, &book, 13, true));
	assert(wear(&actor, &pen, 13, true));
	remove(&book);
	assert(wear(&actor, &weapon, 12, true));
	reset(RACE_THRIKREEN);
	book.loc_p = LOC_CARRIED;
	actor.equipment[FOURTH_WEAPON] = &book;
	assert(wear(&actor, &great, 12, false));
	assert(actor.equipment[FOURTH_WEAPON] == &book);
	assert(get_numb_free_hands(&actor) == 1);
	auto small = item(ITEM_WEAPON, ITEM_WIELD);
	assert(wear(&actor, &small, 12, false));
	assert(actor.equipment[FOURTH_WEAPON] == &book);
	reset(RACE_THRIKREEN);
	assert(wear(&actor, &shield, 14, false));
	auto another_shield = item(ITEM_ARMOR, ITEM_WEAR_SHIELD);
	assert(!wear(&actor, &another_shield, 14, false));
	weapon.loc_p = book.loc_p = LOC_CARRIED;
	// Preserve combat-sensitive placement for traditional four-hand weapons.
	reset(RACE_THRIKREEN);
	auto paired_great = great;
	paired_great.loc_p = LOC_CARRIED;
	auto paired_first = item(ITEM_WEAPON, ITEM_WIELD);
	auto paired_second = item(ITEM_WEAPON, ITEM_WIELD);
	assert(wear(&actor, &paired_great, 12, false));
	assert(wear(&actor, &paired_first, 12, false));
	assert(wear(&actor, &paired_second, 12, false));
	assert(actor.equipment[PRIMARY_WEAPON] == &paired_great);
	assert(actor.equipment[THIRD_WEAPON] == &paired_first);
	assert(actor.equipment[FOURTH_WEAPON] == &paired_second);
	assert(!actor.equipment[SECONDARY_WEAPON]);
	// Training and weight remain enforced, including occupied secondary slots.
	reset();
	pc.skills[SKILL_DUAL_WIELD].learned = 0;
	assert(wear(&actor, &weapon, 12, false));
	auto other = item(ITEM_WEAPON, ITEM_WIELD | ITEM_HOLD);
	assert(!wear(&actor, &other, 12, false));
	actor.equipment[SECONDARY_WEAPON] = actor.equipment[PRIMARY_WEAPON];
	actor.equipment[PRIMARY_WEAPON] = nullptr;
	assert(!wear(&actor, &other, 12, false));
	reset();
	weapon.loc_p = LOC_CARRIED;
	auto heavy = item(ITEM_WEAPON, ITEM_WIELD | ITEM_HOLD, false, 101);
	assert(!wear(&actor, &heavy, 12, false));
	assert(wear(&actor, &book, 13, false));
	assert(!wear(&actor, &heavy, 13, false)); // hold fallback cannot bypass wield weight
	auto holdonly = item(ITEM_WEAPON, ITEM_HOLD);
	assert(!wear(&actor, &holdonly, 13, false)); // nor wield flags
	reset();
	assert(wear(&actor, &weapon, 12, false));
	auto offhand_heavy = item(ITEM_WEAPON, ITEM_WIELD, false, 34);
	assert(!wear(&actor, &offhand_heavy, 12, false));
	reset();
	bound_ok = false;
	for (int role : { 12, 13, 14 })
	{
		auto obj = item(ITEM_WEAPON, ITEM_WIELD | ITEM_HOLD | ITEM_WEAR_SHIELD);
		assert(!wear(&actor, &obj, role, false));
		assert(obj.loc_p == LOC_CARRIED);
	}
	bound_ok = true;
	usable = false;
	assert(!wear(&actor, &other, 12, false));
	reset();
	book.loc_p = LOC_CARRIED;
	assert(!wear(&actor, &book, 12, false));
	assert(!wear(&actor, &book, 14, false));
	// At-hand lookup used by guild scribing and memorization accepts every hand
	// slot, while retaining visibility and the legacy WIELD-before-HOLD order.
	for (int slot : { WIELD, HOLD, WIELD2, WIELD3, WIELD4 })
	{
		reset(RACE_THRIKREEN);
		auto destination = item(ITEM_SPELLBOOK, ITEM_HOLD);
		auto quill = item(ITEM_PEN, ITEM_HOLD);
		actor.equipment[slot] = &destination;
		actor.equipment[slot == HOLD ? WIELD : HOLD] = &quill;
		assert(ScriberSillyChecks(&actor, 1));
		assert(SpellBookAtHand(&actor) == &destination);
		assert(FindSpellBookWithSpell(&actor, 1, SBOOK_MODE_AT_HAND) == &destination);
		assert(!FindSpellBookWithSpell(&actor, 1, SBOOK_MODE_AT_HAND | SBOOK_MODE_NO_BOOK));
		add_scribing(&actor, 1, SpellBookAtHand(&actor), 0, nullptr, nullptr);
		event_scribe(&actor, nullptr, nullptr, &pending);
		assert(pending.book == &destination && pending.page == 1 && !cancelled);
		SET_BIT(destination.extra_flags, ITEM_INVISIBLE);
		assert(!SpellBookAtHand(&actor));
		assert(!FindSpellBookWithSpell(&actor, 1, SBOOK_MODE_AT_HAND));
	}
	reset(RACE_THRIKREEN);
	auto unseen = item(ITEM_SPELLBOOK, ITEM_HOLD);
	auto seen = item(ITEM_SPELLBOOK, ITEM_HOLD);
	SET_BIT(unseen.extra_flags, ITEM_INVISIBLE);
	actor.equipment[WIELD] = &unseen;
	actor.equipment[WIELD4] = &seen;
	assert(SpellBookAtHand(&actor) == &seen);
	assert(FindSpellBookWithSpell(&actor, 1, SBOOK_MODE_AT_HAND) == &seen);
	REMOVE_BIT(unseen.extra_flags, ITEM_INVISIBLE);
	assert(SpellBookAtHand(&actor) == &unseen);
	actor.equipment[WIELD] = nullptr;
	actor.equipment[HOLD] = &unseen;
	assert(SpellBookAtHand(&actor) == &unseen);
	// Reproduce two holds followed by removal of HOLD: the sole weapon lands
	// in SECONDARY but must get normal weight/reach eligibility without training.
	for (bool reach : { false, true })
	{
		reset(reach ? RACE_MINOTAUR : RACE_HUMAN);
		pc.skills[SKILL_DUAL_WIELD].learned = 0;
		auto held = item(ITEM_ARMOR, ITEM_HOLD);
		auto implement = item(ITEM_PEN, ITEM_HOLD);
		auto sole = item(ITEM_WEAPON, ITEM_WIELD, reach, reach ? 1 : 34);
		if (reach)
			sole.value[0] = WEAPON_SPEAR;
		assert(wear(&actor, &held, 13, false));
		assert(wear(&actor, &implement, 13, false));
		remove(&held);
		assert(wear(&actor, &sole, 12, false));
		assert(actor.equipment[SECONDARY_WEAPON] == &sole);
		remove(&implement);
		auto another = item(ITEM_WEAPON, ITEM_WIELD);
		assert(!wear(&actor, &another, 12, false)); // still needs dual training
		pc.skills[SKILL_DUAL_WIELD].learned = 100;
		assert(!wear(&actor, &another, 12, false)); // existing offhand is still restricted
		remove(&sole);
		assert(wear(&actor, &another, 12, false));
		assert(!wear(&actor, &sole, 12, false)); // same restrictions in the normal order
	}
	// The existing zero-hands guard already distinguishes shield messages.
	reset();
	auto full_hands = item(ITEM_ARMOR, ITEM_HOLD, true);
	assert(wear(&actor, &full_hands, 13, false));
	auto standard_shield = item(ITEM_ARMOR, ITEM_WEAR_SHIELD);
	message.clear();
	assert(!wear(&actor, &standard_shield, 14, true));
	assert(message == "Your hands are full.\r\n");
	remove(&full_hands);
	auto one_hand = item(ITEM_ARMOR, ITEM_HOLD);
	assert(wear(&actor, &one_hand, 13, false));
	auto large_shield = item(ITEM_ARMOR, ITEM_WEAR_SHIELD, true);
	message.clear();
	assert(!wear(&actor, &large_shield, 14, true));
	assert(message == "You need two free hands to use that shield.\r\n");
	// Slot exhaustion is independent of the clamped hand count; never overwrite.
	reset(RACE_THRIKREEN);
	for (int slot : { HOLD, WIELD, WIELD2, WIELD3, WIELD4 })
		actor.equipment[slot] = &book;
	assert(free_hand_slot(&actor, true, 1) == -1);
	assert(free_hand_slot(&actor, false, 2) == -1);
	std::printf("%d equip-order permutations and scribing/eligibility checks passed\n",
		    permutations);
}
