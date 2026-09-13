#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "net/output_style.h"
#include "player/output_preferences.h"
#include "player/output_message.h"
#include "player/player_save_pipeline.h"
#include "net/gmcp.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <vector>
#include <tuple>

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
static room_data rooms[3]{};
P_room world = rooms;
static index_data indexes[1]{};
P_index obj_index = indexes;
static bool visible = true;
static P_char hidden_recipient = nullptr;
static std::vector<std::pair<std::string, int>> logged;
static std::function<void(P_char)> command_action;
player_save_pipeline_result player_save_pipeline_request(P_char, player_component_mask_t, int, int)
{
	return player_save_pipeline_result::queued;
}
void command_interpreter(P_char ch, char *)
{
	command_action(ch);
}
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

// Keep the real tell/reply command and its recipient gates. Language and GMCP
// are controlled boundaries, so transformation is deterministic in this test.
P_desc descriptor_list = nullptr;
static unsigned language_calls = 0;
bool can_talk(P_char)
{
	return true;
}
bool is_silent(P_char, bool)
{
	return false;
}
bool racewar(P_char, P_char)
{
	return false;
}
bool isname(const char *name, const char *candidate)
{
	return !strcasecmp(name, candidate);
}
int is_linked_from(P_char, P_char, ush_int)
{
	return 0;
}
void half_chop(char *input, char *first, char *rest)
{
	const char *remaining = one_argument(input, first);
	while (*remaining == ' ')
		++remaining;
	strcpy(rest, remaining);
}
char *language_CRYPT(P_char, P_char, char *message)
{
	++language_calls;
	return message;
}
const char *language_known(P_char, P_char)
{
	return "";
}
float get_property(const char *, double value)
{
	return value;
}
void gmcp_comm_channel(P_char, const char *, const char *, const char *) {}

#include "world_output_stubs.inc"
#include "production_output.inc"

static std::string drain(P_desc d)
{
	std::string output;
	char buffer[MAX_STRING_LENGTH];
	while (get_from_q(&d->output, buffer))
		output += buffer;
	return output;
}

#include "world_output_checks.inc"

static void tell_command_matrix()
{
	char_data alice{}, bob{};
	pc_only_data alice_pc{}, bob_pc{};
	descriptor_data alice_connection{}, bob_connection{};
	alice.only.pc = &alice_pc;
	bob.only.pc = &bob_pc;
	alice.desc = &alice_connection;
	bob.desc = &bob_connection;
	alice_connection.character = &alice;
	bob_connection.character = &bob;
	alice_connection.next = &bob_connection;
	descriptor_list = &alice_connection;
	char alice_name[] = "Alice", bob_name[] = "Bob";
	alice.player.name = alice_name;
	bob.player.name = bob_name;
	alice.player.level = bob.player.level = 20;
	alice.specials.position = bob.specials.position = POS_STANDING + STAT_NORMAL;
	alice_pc.pid = 100;
	bob_pc.pid = 101;
	SET_BIT(alice.specials.act, PLR_ECHO);
	SET_BIT(bob.specials.act, PLR_ECHO);
	OutputProfilePreferences a, b;
	a.set_color(OutputChannel::ChatTell, ATTR_FG(27));
	b.set_color(OutputChannel::ChatTell, ATTR_FG(20));
	assert(update_player_output_preferences(&alice, a) == OutputPreferenceUpdate::PendingSave);
	assert(update_player_output_preferences(&bob, b) == OutputPreferenceUpdate::PendingSave);
	char message[] = "Bob hello &+gforest&n";
	logged.clear();
	const auto calls = language_calls;
	do_tell(&alice, message, 0);
	std::string echo = drain(&alice_connection), received = drain(&bob_connection);
	AnsiString own(echo.c_str()), incoming(received.c_str());
	assert(own.attr(14) == ATTR_FG(27)); // hello in You tell Bob 'hello ...'
	assert(incoming.attr(16) == ATTR_FG(20)); // hello in Alice tells you 'hello ...'
	assert(language_calls == calls + 1 && bob_pc.last_tell == alice.player.name);
	assert(logged.size() == 2 && logged[0].second == LOG_PRIVATE &&
	       logged[1].second == LOG_PRIVATE);
	assert(logged[0].first == "&+WYou tell Bob 'hello &+gforest&n'\r\n");
	char reply[] = "reply text";
	do_reply(&bob, reply, 0);
	AnsiString reply_in(drain(&alice_connection).c_str()),
		reply_own(drain(&bob_connection).c_str());
	assert(reply_in.attr(14) == ATTR_FG(27) && reply_own.attr(16) == ATTR_FG(20));
	bob_pc.ignored = &alice;
	do_tell(&alice, message, 0);
	assert(!bob_connection.output.head &&
	       drain(&alice_connection).find("ignoring") != std::string::npos);
	bob_pc.ignored = nullptr;
	SET_BIT(bob.specials.act, PLR_NOTELL);
	do_tell(&alice, message, 0);
	assert(!bob_connection.output.head &&
	       drain(&alice_connection).find("No-one") != std::string::npos);
	REMOVE_BIT(bob.specials.act, PLR_NOTELL);
	REMOVE_BIT(alice.specials.act, PLR_ECHO);
	do_tell(&alice, message, 0);
	assert(drain(&alice_connection) == "Ok.\r\n");
	drain(&bob_connection);
	SET_BIT(alice.specials.act, PLR_ECHO);
	a.reset(OutputChannel::ChatTell);
	b.reset(OutputChannel::ChatTell);
	assert(update_player_output_preferences(&alice, a) == OutputPreferenceUpdate::PendingSave);
	assert(update_player_output_preferences(&bob, b) == OutputPreferenceUpdate::PendingSave);
	do_tell(&alice, message, 0);
	assert(drain(&alice_connection) == "&+WYou tell Bob 'hello &+gforest&n'\r\n");
	assert(drain(&bob_connection) == "&+WAlice&+W tells you 'hello &+gforest&n'&N\r\n");
	descriptor_list = nullptr;
}

static void begin_paging(P_char ch)
{
	executing_ch = ch;
	SET_BIT(ch->specials.act, PLR_PAGING_ON);
	command_output[0] = 0;
	output_length = 0;
}

static size_t visible_terminal_bytes(const std::string &markup)
{
	char buffer[MAX_STRING_LENGTH];
	AnsiString(markup.c_str()).term(buffer, TL_UNDERLINE);
	size_t bytes = 0;
	for (const char *p = buffer; *p; ++p)
		if (*p == '\x1b')
		{
			while (*p && *p != 'm')
				++p;
			assert(*p);
		}
		else if (*p != '\r')
			++bytes;
	return bytes;
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

	// The real send boundary owns a separate sequence per receiving connection and
	// channel. Interleaved sends, protected content, replay, and another recipient
	// cannot disturb a room's next frame. No room identity participates.
	OutputStyleRecipe flow;
	flow.kind = OutputRecipeKind::Flow;
	flow.palette = { ATTR_FG(25), ATTR_FG(19), ATTR_FG(27) };
	flow.palette_size = 3;
	flow.width = 2;
	WordRecipeDictionary recipes{ { "water", &flow } };
	OutputContext moving{ OutputChannel::RoomDescription, OutputPolicy::Animated, &words };
	moving.recipes = &recipes;
	const size_t room_channel = (size_t)OutputChannel::RoomDescription;
	send_to_char("water", &actor, LOG_PRIVATE, moving);
	std::string frame_zero = drain(&desc);
	assert(desc.output_sequences[room_channel] == 1);
	for (const char *unchanged : { "stone", "&+rwater&n", "wa&+rte&nr" })
	{
		send_to_char(unchanged, &actor, LOG_NONE, moving);
		assert(drain(&desc) == unchanged);
		assert(desc.output_sequences[room_channel] == 1);
	}
	OutputContext chat_motion = moving;
	chat_motion.channel = OutputChannel::ChatSay;
	send_to_char("water", &actor, LOG_NONE, chat_motion);
	assert(drain(&desc) == frame_zero);
	assert(desc.output_sequences[(size_t)OutputChannel::ChatSay] == 1);
	send_to_char("water", &viewer, LOG_NONE, moving);
	assert(drain(&desc2) == frame_zero && desc2.output_sequences[room_channel] == 1);
	send_to_char("water", &actor, LOG_NONE, moving);
	assert(drain(&desc) != frame_zero && desc.output_sequences[room_channel] == 2);
	OutputContext stationary = moving;
	stationary.policy = OutputPolicy::Static;
	send_to_char("water", &actor, LOG_NONE, stationary);
	assert(drain(&desc) == "&+Bwater&n" && desc.output_sequences[room_channel] == 2);
	stationary.policy = OutputPolicy::Preserve;
	send_to_char("water", &actor, LOG_NONE, stationary);
	assert(drain(&desc) == "water" && desc.output_sequences[room_channel] == 2);
	// Unsigned wrap is defined; a new connection starts again at frame zero.
	desc.output_sequences[room_channel] = UINT64_MAX;
	send_to_char("water", &actor, LOG_NONE, moving);
	drain(&desc);
	assert(desc.output_sequences[room_channel] == 0);
	send_to_char("water", &actor, LOG_NONE, moving);
	assert(drain(&desc) == frame_zero);
	descriptor_data reconnected{};
	reconnected.character = &actor;
	actor.desc = &reconnected;
	send_to_char("water", &actor, LOG_NONE, moving);
	assert(drain(&reconnected) == frame_zero &&
	       reconnected.output_sequences[room_channel] == 1);
	actor.desc = &desc;

	pc.screen_length = 12;
	begin_paging(&actor);
	std::string animated_pages;
	for (int i = 0; i < 20; ++i)
		animated_pages += "water\n";
	send_to_char(animated_pages.c_str(), &actor, LOG_NONE, moving);
	auto frozen_sequence = desc.output_sequences[room_channel];
	assert(frozen_sequence == 2); // one eligible message, not twenty words
	executing_ch = nullptr;
	page_string_real(&desc, command_output);
	std::string animated_page = drain(&desc);
	show_string(&desc, "r");
	assert(drain(&desc) == animated_page &&
	       desc.output_sequences[room_channel] == frozen_sequence);
	while (desc.showstr_count)
	{
		show_string(&desc, "");
		drain(&desc);
	}
	assert(desc.output_sequences[room_channel] == frozen_sequence);
	REMOVE_BIT(actor.specials.act, PLR_PAGING_ON);
	std::string oversized_animation;
	for (int i = 0; i < 8000; ++i)
		oversized_animation += "water ";
	send_to_char(oversized_animation.c_str(), &actor, LOG_NONE, moving);
	assert(drain(&desc) == oversized_animation &&
	       desc.output_sequences[room_channel] == frozen_sequence);

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

	// Literal ampersands do not advance the legacy pager's column counter. Added
	// markup across several sends must not turn a deliverable page into a rejected one.
	char command[] = "output-test";
	for (int count : { 1, 3 })
		for (OutputPolicy policy : { OutputPolicy::Preserve, OutputPolicy::Static })
		{
			OutputContext amp_style{ OutputChannel::Chat, policy, nullptr,
						 ATTR_FG(25) };
			std::string amp_chunk(8000, '&');
			command_action = [&](P_char ch)
			{
				for (int i = 0; i < count; ++i)
					send_to_char(amp_chunk.c_str(), ch, LOG_NONE, amp_style);
			};
			process_with_paging(&actor, command);
			std::string output = drain(&desc);
			assert(AnsiString(output.c_str()).size() == amp_chunk.size() * count);
			assert(visible_terminal_bytes(output) == amp_chunk.size() * count);
			assert(GET_ATTR(AnsiString(output.c_str())[0]) ==
			       (count == 1 && policy == OutputPolicy::Static ? ATTR_FG(25) : 0));
		}
	// The combined markup can fit while its terminal expansion does not. Authored
	// red ampersands alternate with default ampersands that acquire the base color.
	std::string terminal_chunk;
	for (int i = 0; i < 2200; ++i)
		terminal_chunk += "&+r&&n&";
	for (OutputPolicy policy : { OutputPolicy::Preserve, OutputPolicy::Static })
	{
		OutputContext amp_style{ OutputChannel::Chat, policy, nullptr, ATTR_FG(25) };
		command_action = [&](P_char ch)
		{
			for (int i = 0; i < 2; ++i)
				send_to_char(terminal_chunk.c_str(), ch, LOG_NONE, amp_style);
		};
		process_with_paging(&actor, command);
		assert(visible_terminal_bytes(drain(&desc)) == 8800);
	}
	// Falling back at finalization must retain the legacy accumulation warning.
	std::string warning_baseline;
	for (OutputPolicy policy : { OutputPolicy::Preserve, OutputPolicy::Static })
	{
		OutputContext amp_style{ OutputChannel::Chat, policy, nullptr, ATTR_FG(25) };
		command_action = [&](P_char ch)
		{
			std::string amp_chunk(8000, '&');
			for (int i = 0; i < 3; ++i)
				send_to_char(amp_chunk.c_str(), ch, LOG_NONE, amp_style);
			std::string excess(MAX_COMMAND_OUTPUT, 'x');
			send_to_char(excess.c_str(), ch, LOG_NONE, amp_style);
		};
		process_with_paging(&actor, command);
		std::string output = drain(&desc);
		assert(output.find("the list goes on") != std::string::npos);
		if (policy == OutputPolicy::Preserve)
			warning_baseline = output;
		else
			assert(output == warning_baseline);
	}
	// Main-menu output bypasses paging even when the paging preference is set.
	// Turning paging off during a command also makes replay send the whole command.
	for (bool menu : { false, true })
		for (OutputPolicy policy : { OutputPolicy::Preserve, OutputPolicy::Static })
		{
			OutputContext menu_style{ OutputChannel::Chat, policy, &words };
			std::string menu_chunk;
			for (int i = 0; i < 1600; ++i)
				menu_chunk += "water ";
			SET_BIT(actor.specials.act, PLR_PAGING_ON);
			desc.connected = menu ? CON_MAIN_MENU : CON_PLAYING;
			command_action = [&](P_char ch)
			{
				for (int i = 0; i < 4; ++i)
					send_to_char(menu_chunk.c_str(), ch, LOG_NONE, menu_style);
				if (!menu)
					REMOVE_BIT(ch->specials.act, PLR_PAGING_ON);
			};
			process_with_paging(&actor, command);
			assert(visible_terminal_bytes(drain(&desc)) == menu_chunk.size() * 4);
		}
	desc.connected = CON_PLAYING;
	SET_BIT(actor.specials.act, PLR_PAGING_ON);
	command_action = {};

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

	// Real recipient fanout resolves each character's settings after visibility.
	// A known template's color may change; authored names/body and logs retain theirs.
	OutputProfilePreferences personal, other;
	personal.set_color(OutputChannel::ChatTell, ATTR_FG(27));
	other.set_color(OutputChannel::ChatTell, ATTR_FG(20));
	pc.output_preferences = personal.state();
	pc2.output_preferences = other.state();
	const auto actor_phase = desc.output_sequences[room_channel];
	const auto viewer_phase = desc2.output_sequences[room_channel];
	for (auto [recipient, connection, color] : std::vector<std::tuple<P_char, P_desc, int>>{
		     { &actor, &desc, 27 }, { &viewer, &desc2, 20 }, { &observer, &desc3, 0 } })
	{
		logged.clear();
		PlayerOutputMessage(recipient, OutputChannel::ChatTell)
			.literal("&+W")
			.entity("&+mName&n", StyleOrigin::Sender)
			.literal("&+W tells you '")
			.body("hello &+gforest&n wa&+rter&n")
			.literal("'&N\r\n")
			.send(LOG_PRIVATE);
		std::string text = drain(connection);
		const std::string original =
			"&+W&+mName&n&+W tells you 'hello &+gforest&n wa&+rter&n'&N\r\n";
		assert(logged.size() == 1 && logged[0] == std::make_pair(original, LOG_PRIVATE));
		if (!color)
			assert(text == original);
		else
		{
			AnsiString ansi(text.c_str());
			assert(ansi.attr(0) == ATTR_FG(21));
			assert(ansi.attr(16) == ATTR_FG(color)); // hello
			assert(ansi.attr(22) == ATTR_FG(18)); // authored forest
			assert(ansi.attr(31) == ATTR_FG(20)); // partially styled water
		}
	}
	assert(desc.output_sequences[room_channel] == actor_phase &&
	       desc2.output_sequences[room_channel] == viewer_phase);
	for (OutputChannel channel :
	     { OutputChannel::ChatSay, OutputChannel::ChatGuild, OutputChannel::ChatShout,
	       OutputChannel::ChatYell, OutputChannel::ChatWhisper, OutputChannel::ChatAsk,
	       OutputChannel::ChatPetition })
	{
		personal.set_color(channel, ATTR_FG(27));
		other.set_color(channel, ATTR_FG(20));
		pc.output_preferences = personal.state();
		pc2.output_preferences = other.state();
		auto adopted = recipient_output_context(channel);
		act("$n says '$T'", false, &actor, nullptr, (void *)"hello", TO_CHAR | ACT_PRIVATE,
		    adopted);
		AnsiString own(drain(&desc).c_str());
		assert(own.attr(own.size() - 3) == ATTR_FG(27));
		act("$n says '$T'", false, &actor, nullptr, (void *)"hello", TO_ROOM | ACT_PRIVATE,
		    adopted);
		AnsiString incoming(drain(&desc2).c_str()), inherited(drain(&desc3).c_str());
		assert(incoming.attr(incoming.size() - 3) == ATTR_FG(20));
		assert(inherited.attr(inherited.size() - 3) == 0);
		assert(incoming.attr(0) == 0); // entity role never mistaken for body
		pc2.ignored = &actor;
		act("hello", false, &actor, nullptr, nullptr, TO_ROOM, adopted);
		assert(!desc2.output.head);
		drain(&desc3);
		pc2.ignored = nullptr;
	}
	personal.reset_all();
	other.reset_all();
	pc.output_preferences = personal.state();
	pc2.output_preferences = other.state();
	PlayerOutputMessage(&actor, OutputChannel::ChatTell)
		.literal("&+Whello&N\r\n")
		.send(LOG_NONE);
	assert(drain(&desc) == "&+Whello&N\r\n");
	personal.set_color(OutputChannel::ChatTell, ATTR_FG(27));
	pc.output_preferences = personal.state();
	std::string near_limit(MAX_STRING_LENGTH - 7, 'x');
	PlayerOutputMessage(&actor, OutputChannel::ChatTell)
		.literal("&+W")
		.body(near_limit)
		.literal("&N\r\n")
		.send(LOG_NONE);
	std::string fallback = "&+W" + near_limit + "&N\r\n";
	fallback.resize(MAX_STRING_LENGTH - 1);
	assert(drain(&desc) == fallback);
	personal.reset_all();
	pc.output_preferences = personal.state();

	// Oversized expansion returns original bytes, including its original UTF-8.
	std::string large;
	for (int i = 0; i < 8000; ++i)
		large += "water ";
	send_to_char(large.c_str(), &actor, LOG_NONE, style);
	assert(drain(&desc) == large);
	tell_command_matrix();
	world_output_matrix();
	puts("Output integration: tell/reply, recipient profiles, sends, act, queue merging, privacy, paging and snoop passed");
}
