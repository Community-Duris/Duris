#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <string>
#include <vector>

constexpr size_t MAX_INPUT_LENGTH = 256;
constexpr size_t MAX_STRING_LENGTH = 4096;
constexpr int RENT_LINKDEAD = 5;

struct descriptor_data
{
	bool valid = true;
};

struct char_data
{
	const char *name = nullptr;
	bool npc = false;
	bool trusted = false;
	int level = 0;
	descriptor_data *desc = nullptr;
	char_data *next = nullptr;
	bool terminal_save_ok = true;
	bool extracted = false;
	int save_calls = 0;
	int extract_calls = 0;
	std::string output;
};

using P_char = char_data *;
using P_desc = descriptor_data *;

P_char character_list = nullptr;
const char *LOG_WIZ = "wiz";
std::vector<std::string> wiz_audit;
std::vector<std::string> file_audit;
int successful_extractions = 0;
int successful_wizlogs = 0;

#define IS_TRUSTED(ch) ((ch)->trusted)
#define IS_NPC(ch) ((ch)->npc)
#define GET_NAME(ch) ((ch)->name)
#define GET_LEVEL(ch) ((ch)->level)

static std::string format_message(const char *format, va_list arguments)
{
	char buffer[MAX_STRING_LENGTH];
	vsnprintf(buffer, sizeof(buffer), format, arguments);
	return buffer;
}

void reset_observations()
{
	character_list = nullptr;
	wiz_audit.clear();
	file_audit.clear();
	successful_extractions = 0;
	successful_wizlogs = 0;
}

char *one_argument(const char *argument, char *first)
{
	while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
		++argument;
	while (*argument && !std::isspace(static_cast<unsigned char>(*argument)))
		*first++ = *argument++;
	*first = '\0';
	while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
		++argument;
	return const_cast<char *>(argument);
}

void send_to_char(const char *message, P_char ch)
{
	ch->output += message;
}

int is_desc_valid(P_desc desc)
{
	return desc && desc->valid;
}

int isname(const char *wanted, const char *actual)
{
	return wanted && actual && !strcasecmp(wanted, actual);
}

bool persistence_save_character_terminal(P_char ch, int intent)
{
	assert(intent == RENT_LINKDEAD);
	++ch->save_calls;
	return ch->terminal_save_ok;
}

void extract_char_after_terminal_save(P_char ch)
{
	++ch->extract_calls;
	ch->extracted = true;
	ch->desc = nullptr;
	ch->name = nullptr;
	++successful_extractions;
}

void wizlog(int, const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	const std::string message = format_message(format, arguments);
	va_end(arguments);
	if (message.find(" extracted ghost character ") != std::string::npos)
	{
		assert(successful_extractions > successful_wizlogs);
		++successful_wizlogs;
	}
	wiz_audit.push_back(message);
}

void logit(const char *, const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	file_audit.push_back(format_message(format, arguments));
	va_end(arguments);
}

static bool contains(const std::string &text, const std::string &wanted)
{
	return text.find(wanted) != std::string::npos;
}

static size_t count_containing(const std::vector<std::string> &entries, const std::string &wanted)
{
	size_t count = 0;
	for (const std::string &entry : entries)
		if (contains(entry, wanted))
			++count;
	return count;
}

/*__EXTRACTLINK_PRODUCTION_FUNCTIONS__*/

static char_data make_admin()
{
	char_data admin;
	admin.name = "Liskins";
	admin.trusted = true;
	admin.level = 60;
	return admin;
}

static void named_success()
{
	reset_observations();
	char_data admin = make_admin();
	char_data ghost;
	ghost.name = "Spidgaul";
	admin.next = &ghost;
	character_list = &admin;
	char argument[] = "Spidgaul";

	do_extractlink(&admin, argument, 0);

	assert(ghost.extracted);
	assert(ghost.save_calls == 1);
	assert(ghost.extract_calls == 1);
	assert(contains(admin.output, "Attempting ghost extraction: Spidgaul (linkdead)."));
	assert(contains(admin.output, "Extracted ghost: Spidgaul."));
	assert(contains(admin.output,
			"extractlink result: 1 match; 1 ghost found; 1 extracted; 0 retained "
			"after save failure; 0 connected; 0 excluded."));
	assert(!contains(admin.output, "Retained ghost"));
	assert(!contains(admin.output, "No player character"));
	assert(wiz_audit.size() == 1);
	assert(contains(wiz_audit[0], "Liskins extracted ghost character Spidgaul"));
	assert(file_audit == wiz_audit);
}

static void named_save_failure()
{
	reset_observations();
	char_data admin = make_admin();
	char_data ghost;
	ghost.name = "Spidgaul";
	ghost.terminal_save_ok = false;
	admin.next = &ghost;
	character_list = &admin;
	char argument[] = "Spidgaul";

	do_extractlink(&admin, argument, 0);

	assert(!ghost.extracted);
	assert(ghost.save_calls == 1);
	assert(ghost.extract_calls == 0);
	assert(contains(admin.output,
			"Retained ghost Spidgaul: terminal save was not durable, so no "
			"extraction was performed."));
	assert(contains(admin.output,
			"extractlink result: 1 match; 1 ghost found; 0 extracted; 1 retained "
			"after save failure; 0 connected; 0 excluded."));
	assert(!contains(admin.output, "No player character"));
	assert(!contains(admin.output, "Extracted ghost:"));
	assert(wiz_audit.size() == 1);
	assert(contains(wiz_audit[0],
			"could not extract ghost character Spidgaul: terminal save was not "
			"durable; character retained"));
	assert(file_audit == wiz_audit);
}

static void named_connected_match()
{
	reset_observations();
	descriptor_data connected_descriptor;
	char_data admin = make_admin();
	char_data player;
	player.name = "Spidgaul";
	player.desc = &connected_descriptor;
	admin.next = &player;
	character_list = &admin;
	char argument[] = "Spidgaul";

	do_extractlink(&admin, argument, 0);

	assert(player.save_calls == 0);
	assert(player.extract_calls == 0);
	assert(contains(admin.output,
			"Cannot extract Spidgaul: character has a valid connection."));
	assert(contains(admin.output,
			"extractlink result: 1 match; 0 ghosts found; 0 extracted; 0 retained "
			"after save failure; 1 connected; 0 excluded."));
	assert(!contains(admin.output, "No player character"));
	assert(wiz_audit.empty());
	assert(file_audit.empty());
}

static void named_no_match()
{
	reset_observations();
	char_data admin = make_admin();
	char_data other;
	other.name = "Other";
	admin.next = &other;
	character_list = &admin;
	char argument[] = "Spidgaul";

	do_extractlink(&admin, argument, 0);

	assert(admin.output == "No player character matching 'Spidgaul' was found.\r\n");
	assert(other.save_calls == 0);
	assert(other.extract_calls == 0);
	assert(wiz_audit.empty());
	assert(file_audit.empty());
}

static void mixed_all()
{
	reset_observations();
	descriptor_data valid_descriptor;
	descriptor_data invalid_descriptor{ false };
	char_data admin = make_admin();
	char_data connected;
	connected.name = "Connected";
	connected.desc = &valid_descriptor;
	char_data linkdead_success;
	linkdead_success.name = "Success";
	char_data linkdead_failure;
	linkdead_failure.name = "Failure";
	linkdead_failure.terminal_save_ok = false;
	char_data dangling_success;
	dangling_success.name = "Dangling";
	dangling_success.desc = &invalid_descriptor;
	admin.next = &connected;
	connected.next = &linkdead_success;
	linkdead_success.next = &linkdead_failure;
	linkdead_failure.next = &dangling_success;
	character_list = &admin;
	char argument[] = "all";

	do_extractlink(&admin, argument, 0);

	assert(connected.save_calls == 0);
	assert(connected.extract_calls == 0);
	assert(linkdead_success.extracted);
	assert(!linkdead_failure.extracted);
	assert(dangling_success.extracted);
	assert(dangling_success.desc == nullptr);
	assert(contains(admin.output, "Dangling (invalid descriptor)."));
	assert(!contains(admin.output, "Connected"));
	assert(contains(admin.output,
			"extractlink all complete: 3 ghosts found; 2 extracted; 1 retained "
			"after save failure."));
	assert(count_containing(wiz_audit, " extracted ghost character ") == 2);
	assert(count_containing(wiz_audit, " could not extract ghost character ") == 1);
	assert(file_audit == wiz_audit);
}

static void self_and_usage_feedback()
{
	reset_observations();
	char_data admin = make_admin();
	character_list = &admin;
	char self_argument[] = "Liskins";
	do_extractlink(&admin, self_argument, 0);
	assert(contains(admin.output, "Cannot extract yourself with extractlink."));
	assert(contains(admin.output, "1 excluded."));
	assert(!contains(admin.output, "No player character"));

	admin.output.clear();
	char blank_argument[] = "   ";
	do_extractlink(&admin, blank_argument, 0);
	assert(contains(admin.output, "extractlink <name>"));
	assert(contains(admin.output,
			"A character is retained when its terminal save is not durable."));
}

int main()
{
	named_success();
	named_save_failure();
	named_connected_match();
	named_no_match();
	mixed_all();
	self_and_usage_feedback();
	return 0;
}
