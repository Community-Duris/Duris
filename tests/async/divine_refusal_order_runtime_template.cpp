#include "cmd/divine_refusal_policy.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>
#include <string>
#include <utility>
#include <vector>

#define FALSE false
#define TRUE true
#define MAX_INPUT_LENGTH 512
#define CH_INROOM_SIZE 256
#define WAIT_SEC 4
#define PULSE_VIOLENCE 16

#define CMD_NONE 0
#define CMD_ATTACK 1
#define CMD_HEAL 2
#define CMD_ABORT 3
#define CMD_FLEE 4
#define MAX_CMD 5

#define CLASS_CLERIC (1u << 0)
#define CLASS_SUMMONER (1u << 1)

#define AFF_CHARM (1u << 0)
#define AFF_KNOCKED_OUT (1u << 1)
#define AFF_WRAITHFORM (1u << 2)
#define AFF2_MAJOR_PARALYSIS (1u << 0)
#define AFF2_MINOR_PARALYSIS (1u << 1)
#define AFF2_CASTING (1u << 2)
#define AFF4_DEAF (1u << 0)
#define AFF5_ORDERING (1u << 0)
#define ROOM_SILENT (1u << 0)

#define TO_ROOM 0
#define TO_VICT 1
#define TO_NOTVICT 2
#define TO_CHAR 3
#define ACT_SILENCEABLE 16

struct char_data;
using P_char = char_data *;
using P_obj = void *;

struct char_data
{
	std::string name;
	bool npc = false;
	bool alive = true;
	bool can_act = true;
	bool immobile = false;
	bool morph = false;
	bool silent = false;
	bool can_speak = true;
	bool visible = true;
	bool live = true;
	bool item_action = false;
	bool extract_on_command = false;
	bool become_busy_on_command = false;
	unsigned int primary_classes = 0;
	unsigned int secondary_classes = 0;
	int level = 1;
	int in_room = 0;
	P_char master = nullptr;
	P_char next_in_room = nullptr;
	int handler_calls = 0;
	int resource_uses = 0;
	bool ordering_seen = false;
	struct
	{
		unsigned int affected_by = 0;
		unsigned int affected_by2 = 0;
		unsigned int affected_by4 = 0;
		unsigned int affected_by5 = 0;
		int z_cord = 0;
		divine_refusal_tick divine_refusal_until_pulse = 0;
	} specials;
};

struct room_data
{
	P_char people = nullptr;
	unsigned int flags = 0;
};

static room_data world[2];
static divine_refusal_tick ne_event_tick = 0;

#define IS_PC(ch) ((ch) && !(ch)->npc)
#define IS_NPC(ch) ((ch) && (ch)->npc)
#define IS_ALIVE(ch) ((ch) && (ch)->alive)
#define IS_MORPH(ch) ((ch)->morph)
#define CAN_ACT(ch) ((ch)->can_act)
#define IS_IMMOBILE(ch) ((ch)->immobile)
#define CAN_SPEAK(ch) ((ch)->can_speak)
#define IS_TRUSTED(ch) false
#define GET_MASTER(ch) ((ch)->master)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_CLASS(ch, bit) ((((ch)->primary_classes | (ch)->secondary_classes) & (bit)) != 0)
#define IS_AFFECTED(ch, bit) (((ch)->specials.affected_by & (bit)) != 0)
#define IS_AFFECTED2(ch, bit) (((ch)->specials.affected_by2 & (bit)) != 0)
#define IS_AFFECTED4(ch, bit) (((ch)->specials.affected_by4 & (bit)) != 0)
#define IS_SET(value, bit) (((value) & (bit)) != 0)
#define IS_ROOM(room, bit) ((world[(room)].flags & (bit)) != 0)
#define SET_BIT(value, bit) ((value) |= (bit))
#define REMOVE_BIT(value, bit) ((value) &= ~(bit))

struct output_event
{
	std::string channel;
	std::string text;
	P_char receiver = nullptr;
	P_char actor = nullptr;
	P_char victim = nullptr;
	int type = -1;
	bool hide_invisible = false;
};

static std::vector<output_event> output_events;
static std::vector<std::pair<P_char, int>> waits;
static std::vector<int> rolls;
static size_t next_roll = 0;
static int roll_calls = 0;
static float setting_enabled = 0.0f;
static float setting_summoner_only = 1.0f;
static float setting_percent = 10.0f;
static float setting_lock_seconds = 4.0f;
static bool settings_present = true;

static bool starts_with_case_insensitive(const std::string &word, const char *prefix)
{
	const size_t length = std::strlen(prefix);
	if (length == 0 || length > word.size())
		return false;
	for (size_t index = 0; index < length; ++index)
	{
		if (std::tolower(static_cast<unsigned char>(word[index])) !=
		    std::tolower(static_cast<unsigned char>(prefix[index])))
			return false;
	}
	return true;
}

static bool equal_case_insensitive(const char *left, const char *right)
{
	if (!left || !right)
		return left == right;
	while (*left && *right)
	{
		if (std::tolower(static_cast<unsigned char>(*left)) !=
		    std::tolower(static_cast<unsigned char>(*right)))
			return false;
		++left;
		++right;
	}
	return *left == *right;
}

static float get_property(const char *name, float fallback, bool)
{
	if (!settings_present)
		return fallback;
	if (!std::strcmp(name, "pets.divine_refusal.enabled"))
		return setting_enabled;
	if (!std::strcmp(name, "pets.divine_refusal.summoner_only"))
		return setting_summoner_only;
	if (!std::strcmp(name, "pets.divine_refusal.percent"))
		return setting_percent;
	if (!std::strcmp(name, "pets.divine_refusal.retry_lock_seconds"))
		return setting_lock_seconds;
	return fallback;
}

static int number(int minimum, int maximum)
{
	assert(minimum == 1);
	assert(maximum == 100);
	assert(next_roll < rolls.size());
	++roll_calls;
	return rolls[next_roll++];
}

static bool is_silent(P_char ch, bool)
{
	return ch->silent || IS_ROOM(ch->in_room, ROOM_SILENT);
}

static bool item_action_active(P_char ch)
{
	return ch->item_action;
}

static bool cmd_allowed_while_casting(P_char, int cmd)
{
	return cmd == CMD_ABORT || cmd == CMD_FLEE;
}

static int ordered_command_number(const char *input)
{
	if (!input)
		return CMD_NONE;
	while (*input && std::isspace(static_cast<unsigned char>(*input)))
		++input;
	std::string word;
	while (*input && !std::isspace(static_cast<unsigned char>(*input)))
		word.push_back(*input++);
	if (starts_with_case_insensitive("attack", word.c_str()))
		return CMD_ATTACK;
	if (starts_with_case_insensitive("heal", word.c_str()))
		return CMD_HEAL;
	if (starts_with_case_insensitive("abort", word.c_str()))
		return CMD_ABORT;
	if (starts_with_case_insensitive("flee", word.c_str()))
		return CMD_FLEE;
	return CMD_NONE;
}

static void half_chop(const char *input, char *first, char *rest)
{
	while (*input && std::isspace(static_cast<unsigned char>(*input)))
		++input;
	while (*input && !std::isspace(static_cast<unsigned char>(*input)))
		*first++ = *input++;
	*first = '\0';
	while (*input && std::isspace(static_cast<unsigned char>(*input)))
		++input;
	std::strcpy(rest, input);
}

static int str_cmp(const char *left, const char *right)
{
	return equal_case_insensitive(left, right) ? 0 : 1;
}

static P_char get_char_room_vis(P_char ch, const char *name)
{
	for (P_char candidate = world[ch->in_room].people; candidate;
	     candidate = candidate->next_in_room)
	{
		if (candidate->live && candidate->visible &&
		    candidate->specials.z_cord == ch->specials.z_cord &&
		    starts_with_case_insensitive(candidate->name, name))
			return candidate;
	}
	return nullptr;
}

static void send_to_char(const char *message, P_char receiver)
{
	output_events.push_back({ "send", message, receiver, nullptr, nullptr, -1, false });
}

static void act(const char *message, bool hide_invisible, P_char actor, P_obj, const void *victim,
		int type)
{
	P_char receiver = (type & 15) == TO_CHAR ? actor : nullptr;
	output_events.push_back({ "act", message, receiver, actor,
				  static_cast<P_char>(const_cast<void *>(victim)), type,
				  hide_invisible });
}

static void CharWait(P_char ch, int delay)
{
	waits.emplace_back(ch, delay);
}

static int char_in_list(const P_char ch)
{
	return ch && ch->live;
}

static void command_interpreter(P_char ch, char *input)
{
	const int cmd = ordered_command_number(input);
	if (cmd <= CMD_NONE || cmd >= MAX_CMD)
		return;
	if ((item_action_active(ch) || IS_AFFECTED2(ch, AFF2_CASTING)) &&
	    !cmd_allowed_while_casting(ch, cmd))
		return;
	++ch->handler_calls;
	++ch->resource_uses;
	ch->ordering_seen = ch->master && IS_SET(ch->master->specials.affected_by5, AFF5_ORDERING);
	if (ch->become_busy_on_command)
		ch->can_act = false;
	if (ch->extract_on_command)
		ch->live = false;
}

/*__DIVINE_REFUSAL_PRODUCTION_FUNCTIONS__*/

static char_data make_master(const char *name = "master")
{
	char_data ch;
	ch.name = name;
	ch.level = 50;
	ch.primary_classes = CLASS_SUMMONER;
	return ch;
}

static char_data make_pet(const char *name, bool cleric = true)
{
	char_data pet;
	pet.name = name;
	pet.npc = true;
	pet.primary_classes = cleric ? CLASS_CLERIC : 0;
	pet.specials.affected_by = AFF_CHARM;
	return pet;
}

static void place(std::initializer_list<P_char> characters)
{
	P_char previous = nullptr;
	world[0].people = nullptr;
	for (P_char ch : characters)
	{
		ch->in_room = 0;
		ch->live = true;
		ch->next_in_room = nullptr;
		if (previous)
			previous->next_in_room = ch;
		else
			world[0].people = ch;
		previous = ch;
	}
}

static void reset_runtime()
{
	world[0] = {};
	world[1] = {};
	ne_event_tick = 0;
	output_events.clear();
	waits.clear();
	rolls.clear();
	next_roll = 0;
	roll_calls = 0;
	setting_enabled = 0.0f;
	setting_summoner_only = 1.0f;
	setting_percent = 10.0f;
	setting_lock_seconds = 4.0f;
	settings_present = true;
}

static void prepare_rolls(std::initializer_list<int> values)
{
	rolls.assign(values);
	next_roll = 0;
	roll_calls = 0;
}

static void issue_order(P_char master, const char *text)
{
	char argument[MAX_INPUT_LENGTH];
	std::snprintf(argument, sizeof(argument), "%s", text);
	do_order(master, argument, 0);
}

static int count_output(const char *needle)
{
	return static_cast<int>(std::count_if(
		output_events.begin(), output_events.end(), [needle](const output_event &event)
		{ return event.text.find(needle) != std::string::npos; }));
}

static int count_public_output(const char *needle)
{
	return static_cast<int>(std::count_if(output_events.begin(), output_events.end(),
					      [needle](const output_event &event) {
						      return (event.type & 15) == TO_NOTVICT &&
							     event.text.find(needle) !=
								     std::string::npos;
					      }));
}

static int first_output(const char *needle)
{
	for (size_t index = 0; index < output_events.size(); ++index)
	{
		if (output_events[index].text.find(needle) != std::string::npos)
			return static_cast<int>(index);
	}
	return -1;
}

static void expect_wait(P_char actor, int delay)
{
	assert(waits.size() == 1);
	assert(waits[0].first == actor);
	assert(waits[0].second == delay);
}

static void test_disabled_named_is_legacy_exact()
{
	reset_runtime();
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");

	assert(roll_calls == 0);
	assert(pet.handler_calls == 1 && pet.resource_uses == 1);
	assert(pet.ordering_seen);
	assert(!IS_SET(master.specials.affected_by5, AFF5_ORDERING));
	assert(count_output("Ok.") == 1);
	expect_wait(&master, 2);
}

static void test_disabled_busy_group_acknowledges_before_busy()
{
	reset_runtime();
	char_data master = make_master();
	char_data first = make_pet("first");
	char_data second = make_pet("second");
	first.master = second.master = &master;
	first.can_act = second.can_act = false;
	place({ &master, &first, &second });
	issue_order(&master, "followers attack goblin");

	assert(roll_calls == 0);
	assert(first.handler_calls == 0 && second.handler_calls == 0);
	assert(count_output("Ok.") == 1);
	assert(count_output("busy at the moment") == 2);
	assert(first_output("Ok.") < first_output("busy at the moment"));
	expect_wait(&master, 2);
}

static void test_missing_config_and_zero_percent_are_legacy()
{
	reset_runtime();
	settings_present = false;
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);
	assert(count_output("Ok.") == 1);
	expect_wait(&master, 2);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 0.0f;
	master = make_master();
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);
	assert(count_output("Ok.") == 1);
	expect_wait(&master, 2);
}

static void test_named_refusal_and_active_retry()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	ne_event_tick = 100;
	prepare_rolls({ 1 });
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");

	assert(roll_calls == 1);
	assert(pet.handler_calls == 0 && pet.resource_uses == 0);
	assert(pet.specials.divine_refusal_until_pulse == 116);
	assert(count_public_output("My deity has warned me") == 1);
	assert(count_output("Ok.") == 0);
	assert(!IS_SET(master.specials.affected_by5, AFF5_ORDERING));
	expect_wait(&master, PULSE_VIOLENCE);

	output_events.clear();
	waits.clear();
	issue_order(&master, "followers heal master");
	assert(roll_calls == 1);
	assert(pet.specials.divine_refusal_until_pulse == 116);
	assert(count_public_output("My deity has warned me") == 0);
	assert(count_output("still refusing") == 1);
	assert(count_output("Ok.") == 0);
	expect_wait(&master, 2);
}

static void test_expiry_rerolls_and_success_is_not_banked()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 50.0f;
	ne_event_tick = 116;
	prepare_rolls({ 99, 99 });
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	pet.specials.divine_refusal_until_pulse = 116;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 1);
	assert(pet.handler_calls == 1);
	assert(pet.specials.divine_refusal_until_pulse == 0);

	output_events.clear();
	waits.clear();
	issue_order(&master, "priest heal master");
	assert(roll_calls == 2);
	assert(pet.handler_calls == 2);
}

static void test_exempt_unknown_and_busy_commands_do_not_roll()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });

	issue_order(&master, "priest abort");
	assert(roll_calls == 0 && pet.handler_calls == 1);
	output_events.clear();
	waits.clear();
	issue_order(&master, "priest flee");
	assert(roll_calls == 0 && pet.handler_calls == 2);
	output_events.clear();
	waits.clear();
	issue_order(&master, "priest frobnicate");
	assert(roll_calls == 0 && pet.handler_calls == 2);

	pet.specials.affected_by2 |= AFF2_CASTING;
	output_events.clear();
	waits.clear();
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 2);
	pet.specials.affected_by2 &= ~AFF2_CASTING;
	pet.item_action = true;
	output_events.clear();
	waits.clear();
	issue_order(&master, "priest he master");
	assert(roll_calls == 0 && pet.handler_calls == 2);

	pet.item_action = false;
	output_events.clear();
	waits.clear();
	prepare_rolls({ 1 });
	issue_order(&master, "priest att goblin");
	assert(roll_calls == 1 && pet.handler_calls == 2);
}

static void test_secondary_classes_and_scope_filters()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1 });
	char_data master = make_master();
	master.primary_classes = 0;
	master.secondary_classes = CLASS_SUMMONER;
	char_data pet = make_pet("priest");
	pet.primary_classes = 0;
	pet.secondary_classes = CLASS_CLERIC;
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 1 && pet.handler_calls == 0);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	pet = make_pet("warrior", false);
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "warrior attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	master.primary_classes = 0;
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_summoner_only = 0.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1 });
	master = make_master();
	master.primary_classes = 0;
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 1 && pet.handler_calls == 0);
}

static void test_actor_pet_and_locality_filters()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.npc = false;
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	master.npc = true;
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	pet = make_pet("priest");
	pet.master = &master;
	pet.specials.affected_by &= ~AFF_CHARM;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 0);
	assert(count_output("indifferent look") == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	char_data other = make_master("other");
	pet = make_pet("priest");
	pet.master = &other;
	place({ &master, &other, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 0);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	master.alive = false;
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 0);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	pet = make_pet("priest");
	pet.master = &master;
	pet.visible = false;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 0);
	assert(count_output("isn't here") == 1);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	master = make_master();
	pet = make_pet("priest");
	pet.master = &master;
	pet.specials.z_cord = 1;
	place({ &master, &pet });
	issue_order(&master, "followers attack goblin");
	assert(roll_calls == 0 && pet.handler_calls == 0);
	assert(count_output("None here are loyal") == 1);
}

static void test_mixed_and_all_refused_groups()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 50.0f;
	prepare_rolls({ 1, 99 });
	char_data master = make_master();
	char_data refusing = make_pet("refusing");
	char_data warrior = make_pet("warrior", false);
	char_data accepting = make_pet("accepting");
	refusing.master = warrior.master = accepting.master = &master;
	place({ &master, &refusing, &warrior, &accepting });
	issue_order(&master, "followers attack goblin");

	assert(roll_calls == 2);
	assert(refusing.handler_calls == 0);
	assert(warrior.handler_calls == 1 && accepting.handler_calls == 1);
	assert(count_output("Ok.") == 1);
	assert(count_public_output("My deity has warned me") == 1);
	expect_wait(&master, PULSE_VIOLENCE);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1, 1 });
	master = make_master();
	char_data first = make_pet("first");
	char_data second = make_pet("second");
	first.master = second.master = &master;
	place({ &master, &first, &second });
	issue_order(&master, "followers attack goblin");
	assert(roll_calls == 2);
	assert(first.handler_calls == 0 && second.handler_calls == 0);
	assert(count_output("Ok.") == 0);
	assert(count_output("None here are loyal") == 0);
	assert(count_public_output("My deity has warned me") == 2);
	expect_wait(&master, PULSE_VIOLENCE);
}

static void test_group_snapshot_survives_pet_extraction()
{
	reset_runtime();
	char_data master = make_master();
	char_data first = make_pet("first");
	char_data second = make_pet("second");
	first.master = second.master = &master;
	first.extract_on_command = true;
	place({ &master, &first, &second });
	issue_order(&master, "followers attack goblin");
	assert(first.handler_calls == 1 && second.handler_calls == 1);
	assert(!first.live && second.live);
	expect_wait(&master, PULSE_VIOLENCE);
}

static void test_deadline_follows_instance_and_reset_clears_it()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	ne_event_tick = 40;
	prepare_rolls({ 1 });
	char_data first_master = make_master("first-master");
	char_data second_master = make_master("second-master");
	char_data pet = make_pet("priest");
	pet.master = &first_master;
	place({ &first_master, &second_master, &pet });
	issue_order(&first_master, "priest attack goblin");
	assert(roll_calls == 1 && pet.specials.divine_refusal_until_pulse == 56);

	pet.specials.affected_by &= ~AFF_CHARM;
	pet.in_room = 1;
	pet.in_room = 0;
	pet.specials.affected_by |= AFF_CHARM;
	pet.master = &second_master;
	output_events.clear();
	waits.clear();
	issue_order(&second_master, "priest heal second-master");
	assert(roll_calls == 1);
	assert(pet.specials.divine_refusal_until_pulse == 56);
	assert(count_output("still refusing") == 1);

	char_data replacement = make_pet("replacement");
	replacement.master = &second_master;
	prepare_rolls({ 1 });
	place({ &second_master, &replacement });
	output_events.clear();
	waits.clear();
	issue_order(&second_master, "replacement attack goblin");
	assert(roll_calls == 1);
	assert(replacement.specials.divine_refusal_until_pulse == 56);
}

static void test_extraction_cannot_leak_to_reused_address()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	ne_event_tick = 70;
	char_data master = make_master();
	alignas(char_data) std::byte storage[sizeof(char_data)];
	P_char first = new (storage) char_data(make_pet("first"));
	first->master = &master;
	prepare_rolls({ 1 });
	place({ &master, first });
	issue_order(&master, "first attack goblin");
	assert(first->specials.divine_refusal_until_pulse == 86);
	first->~char_data();

	P_char replacement = new (storage) char_data(make_pet("replacement"));
	assert(replacement == first);
	assert(replacement->specials.divine_refusal_until_pulse == 0);
	replacement->master = &master;
	prepare_rolls({ 1 });
	output_events.clear();
	waits.clear();
	place({ &master, replacement });
	issue_order(&master, "replacement attack goblin");
	assert(roll_calls == 1);
	assert(replacement->specials.divine_refusal_until_pulse == 86);
	replacement->~char_data();
}

static void test_output_falls_back_to_nonverbal_safely()
{
	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1 });
	char_data master = make_master();
	char_data pet = make_pet("priest");
	pet.master = &master;
	pet.silent = true;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(count_output("My deity has warned me") == 0);
	assert(count_public_output("refuses the order with a solemn shake") == 1);
	assert(count_output("refuses your order with a solemn shake") == 1);
	for (const output_event &event : output_events)
		assert(event.text.find("says '") == std::string::npos);

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1 });
	master = make_master();
	pet = make_pet("hidden");
	pet.master = &master;
	pet.visible = false;
	place({ &master, &pet });
	issue_order(&master, "followers attack goblin");
	assert(count_public_output("My deity has warned me") == 1);
	const auto public_message = std::find_if(
		output_events.begin(), output_events.end(),
		[](const output_event &event)
		{
			return (event.type & 15) == TO_NOTVICT &&
			       event.text.find("My deity has warned me") != std::string::npos;
		});
	assert(public_message != output_events.end());
	assert(public_message->hide_invisible);
	assert(IS_SET(public_message->type, ACT_SILENCEABLE));

	reset_runtime();
	setting_enabled = 1.0f;
	setting_percent = 100.0f;
	prepare_rolls({ 1 });
	master = make_master();
	master.specials.affected_by4 |= AFF4_DEAF;
	pet = make_pet("priest");
	pet.master = &master;
	place({ &master, &pet });
	issue_order(&master, "priest attack goblin");
	assert(count_public_output("My deity has warned me") == 1);
	assert(count_output("refuses your order with a solemn shake") == 1);
}

int main()
{
	test_disabled_named_is_legacy_exact();
	test_disabled_busy_group_acknowledges_before_busy();
	test_missing_config_and_zero_percent_are_legacy();
	test_named_refusal_and_active_retry();
	test_expiry_rerolls_and_success_is_not_banked();
	test_exempt_unknown_and_busy_commands_do_not_roll();
	test_secondary_classes_and_scope_filters();
	test_actor_pet_and_locality_filters();
	test_mixed_and_all_refused_groups();
	test_group_snapshot_survives_pet_extraction();
	test_deadline_follows_instance_and_reset_clears_it();
	test_extraction_cannot_leak_to_reused_address();
	test_output_falls_back_to_nonverbal_safely();
	std::puts("divine refusal production order runtime: ok");
	return 0;
}
