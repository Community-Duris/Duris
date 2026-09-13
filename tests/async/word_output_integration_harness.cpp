#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "net/output_style.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <vector>

// Allocation and outside-world visibility/logging boundaries are test doubles;
// the output/pager logic below is extracted verbatim from production sources.
#undef CREATE
#undef RECREATE
#undef FREE
#define CREATE(result, type, number, tag) result = (type *)calloc(number, sizeof(type))
#define RECREATE(result, type, number) result = (type *)realloc(result, sizeof(type) * (number))
#define FREE(pointer)              \
	do                         \
	{                          \
		free(pointer);     \
		pointer = nullptr; \
	} while (false)
#define PAGE_WIDTH 80
static char send_to_char_f_buf[MAX_STRING_LENGTH];
P_char executing_ch = nullptr;
static room_data rooms[1]{};
P_room world = rooms;
static index_data indexes[1]{};
P_index obj_index = indexes;
static bool visible = true;
static P_char hidden_recipient = nullptr;
static std::vector<std::pair<std::string, int>> logged;
void logit(const char *, const char *, ...) {}
void panic_corruption(const char *, const char *, ...)
{
	abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
int IS_MORPH(P_char)
{
	return 0;
}
void write_to_pc_log(P_char, const char *message, int policy)
{
	logged.emplace_back(message, policy);
}
bool ac_can_see(P_char, P_char, bool)
{
	return visible;
}
bool ac_can_see_obj(P_char, P_obj, int)
{
	return visible;
}
char *PERS(P_char ch, P_char to, int)
{
	static char unknown[] = "someone";
	return to == hidden_recipient ? unknown : ch->player.name;
}
char *FirstWord(char *input)
{
	return input;
}
char *str_dup(const char *input)
{
	return strdup(input);
}
char *one_argument(const char *input, char *out)
{
	while (*input == ' ')
		++input;
	while (*input && *input != ' ')
		*out++ = *input++;
	*out = 0;
	return const_cast<char *>(input);
}

#include "production_output.inc"

static std::string drain(P_desc d)
{
	std::string output;
	char buffer[MAX_STRING_LENGTH];
	while (get_from_q(&d->output, buffer))
		output += buffer;
	return output;
}

static void begin_paging(P_char ch)
{
	executing_ch = ch;
	SET_BIT(ch->specials.act, PLR_PAGING_ON);
	command_output[0] = 0;
	output_length = 0;
}

int main()
{
	char_data actor{}, viewer{}, observer{};
	pc_only_data pc{}, pc2{}, pc3{};
	descriptor_data desc{}, desc2{}, desc3{};
	actor.only.pc = &pc;
	viewer.only.pc = &pc2;
	observer.only.pc = &pc3;
	actor.desc = &desc;
	viewer.desc = &desc2;
	observer.desc = &desc3;
	desc.character = &actor;
	desc2.character = &viewer;
	desc3.character = &observer;
	desc.connected = desc2.connected = desc3.connected = CON_PLAYING;
	actor.specials.position = viewer.specials.position = observer.specials.position =
		STAT_NORMAL;
	char actor_name[] = "water";
	actor.player.name = actor_name;
	WordColorDictionary words = { { "water", ATTR_FG(25) }, { "forest", ATTR_FG(18) } };
	OutputContext style{ OutputChannel::Chat, OutputPolicy::Static, &words };
	OutputContext preserve = style;
	preserve.policy = OutputPolicy::Preserve;

	// Legacy/default and explicit Preserve retain byte-for-byte messages and logs.
	const char *raw = "&Nwater &+rforest&n\n\r&&+?";
	for (int policy : { LOG_PUBLIC, LOG_PRIVATE, LOG_NONE })
	{
		logged.clear();
		send_to_char(raw, &actor, policy);
		assert(drain(&desc) == raw);
		auto baseline_log = logged;
		logged.clear();
		send_to_char(raw, &actor, policy, preserve);
		assert(drain(&desc) == raw && logged == baseline_log);
	}
	logged.clear();
	send_to_char("water", &actor, LOG_PRIVATE, style);
	assert(drain(&desc) == "&+Bwater&n");
	assert(logged.size() == 1 &&
	       logged[0] == std::make_pair(std::string("water"), LOG_PRIVATE));
	send_to_char("water", &actor, LOG_NONE, style);
	assert(drain(&desc) == "&+Bwater&n" && logged.size() == 1);
	actor.player.level = MAXLVLMORTAL + 1;
	send_to_char("water", &actor, style);
	assert(drain(&desc) == "&+Bwater&n" && logged.size() == 1);
	actor.player.level = 1;

	// Match after printf formatting. Queuing never matches across send boundaries.
	send_to_char_f(&actor, style, "%s%s %d", "wa", "ter", 7);
	assert(drain(&desc) == "&+Bwater&n 7");
	send_to_char("wa", &actor, style);
	send_to_char("ter", &actor, preserve);
	send_to_char(" forest", &actor, style);
	assert(desc.output.head == desc.output.tail); // real queue merged these sends
	assert(drain(&desc) == "water &+gforest&n");

	// Switched prefix remains authored and is outside the word renderer/log content.
	desc.original = &actor;
	send_to_char("water", &actor, style);
	assert(drain(&desc) == "&+M@&+Wwater&n: &+Bwater&n");
	desc.original = nullptr;

	// Real act() does recipient substitutions, entity protection and capitalization.
	rooms[0].people = &actor;
	actor.next_in_room = &viewer;
	viewer.next_in_room = &observer;
	hidden_recipient = &observer;
	char body[] = "wa&nter";
	act("$n sees $T in a forest.", false, &actor, nullptr, body, TO_ROOM | ACT_PRIVATE, style);
	assert(AnsiString(drain(&desc2).c_str()) ==
	       AnsiString("Water sees &+Bwater&n in a &+gforest&n.\n"));
	assert(AnsiString(drain(&desc3).c_str()) ==
	       AnsiString("Someone sees &+Bwater&n in a &+gforest&n.\n"));
	assert(!desc.output.head);
	act("water", false, &actor, nullptr, nullptr, TO_CHAR, style);
	assert(AnsiString(drain(&desc).c_str()) == AnsiString("&+BWater&n\n"));
	act("water", false, &actor, nullptr, nullptr, TO_CHAR);
	assert(drain(&desc) == "Water\n\r");
	visible = false;
	act("water", true, &actor, nullptr, nullptr, TO_ROOM, style);
	assert(!desc2.output.head && !desc3.output.head);
	visible = true;
	viewer.specials.z_cord = 1;
	observer.specials.affected_by4 = AFF4_DEAF;
	act("water", false, &actor, nullptr, nullptr, TO_ROOM | ACT_SILENCEABLE, style);
	assert(!desc2.output.head && !desc3.output.head);
	viewer.specials.z_cord = 0;
	observer.specials.affected_by4 = 0;

	// Snoop gets the already selected colors, without a second dictionary pass.
	std::string frozen;
	assert(render_output_message("water\nforest", style, frozen));
	char snoop[MAX_STRING_LENGTH];
	format_to_snoopers(frozen.data(), snoop);
	assert(AnsiString(snoop) == AnsiString("&+C%&n &+Bwater&n\n&+C%&n &+gforest&n"));
	std::string near_limit_snoop, expected_snoop;
	for (int i = 0; i < 1675; ++i)
	{
		near_limit_snoop += "water 雪\n";
		expected_snoop += "% water 雪\n";
	}
	assert(render_output_message(near_limit_snoop.c_str(), style, frozen));
	format_to_snoopers(frozen.data(), snoop);
	char terminal[MAX_STRING_LENGTH];
	AnsiString(snoop).term(terminal, TL_UNDERLINE);
	std::string snooped_plain;
	for (const char *p = terminal; *p; ++p)
	{
		if (*p == '\x1b')
		{
			while (*p && *p != 'm')
				++p;
			assert(*p);
		}
		else if (*p != '\r')
			snooped_plain += *p;
	}
	assert(snooped_plain == expected_snoop);

	// Pager replay/refresh is frozen even after the dictionary changes, and does
	// not log decorated text or change the original LOG_NONE/LOG_PRIVATE choice.
	pc.screen_length = 12;
	begin_paging(&actor);
	logged.clear();
	std::string pages;
	for (int i = 0; i < 20; ++i)
		pages += "water\n";
	send_to_char(pages.c_str(), &actor, LOG_PRIVATE, style);
	send_to_char("forest\n", &actor, LOG_NONE, style);
	assert(logged.size() == 1 && logged[0].first == pages && logged[0].second == LOG_PRIVATE);
	std::string accumulated = command_output;
	assert(accumulated.find("&+Bwater") != std::string::npos);
	executing_ch = nullptr;
	words["water"] = ATTR_FG(20);
	page_string_real(&desc, command_output);
	std::string first_page = drain(&desc);
	assert(first_page.find("&+Bwater") != std::string::npos);
	show_string(&desc, "r");
	assert(drain(&desc) == first_page);
	assert(logged.size() == 1);
	while (desc.showstr_count)
	{
		show_string(&desc, "");
		drain(&desc);
	}
	assert(logged.size() == 1);

	// Styling may never displace later visible output at the accumulation limit.
	words["water"] = ATTR_FG(25);
	begin_paging(&actor);
	std::string original;
	std::string chunk;
	for (int i = 0; i < 4000; ++i)
		chunk += "water ";
	for (int i = 0; i < 40; ++i)
	{
		send_to_char(chunk.c_str(), &actor, LOG_NONE, style);
		original += chunk;
	}
	assert(original.size() < MAX_COMMAND_OUTPUT);
	assert(std::string(command_output) == original && output_length == original.size());
	// Once original content reaches the legacy warning, later sends must leave it.
	std::string excess(MAX_COMMAND_OUTPUT, 'x');
	send_to_char(excess.c_str(), &actor, LOG_NONE, style);
	std::string warned = command_output;
	assert(warned.find("the list goes on") != std::string::npos);
	send_to_char("water", &actor, LOG_NONE, style);
	assert(std::string(command_output) == warned);
	executing_ch = nullptr;
	REMOVE_BIT(actor.specials.act, PLR_PAGING_ON);

	// Oversized expansion returns original bytes, including its original UTF-8.
	std::string large;
	for (int i = 0; i < 8000; ++i)
		large += "water ";
	send_to_char(large.c_str(), &actor, LOG_NONE, style);
	assert(drain(&desc) == large);
	puts("Output integration: sends, act recipients, queue merging, privacy, paging and snoop passed");
}
