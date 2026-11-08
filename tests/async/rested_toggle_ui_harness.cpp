#include "core/prototypes.h"
#include "core/json_utils.h"
#include "account/account.h"
#include "net/ws_handlers.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int feature_enabled = -1;
static int property_reads = 0;
static int restore_calls = 0;
static std::string sent_json;

int get_property(const char *key, int fallback)
{
	if (key && !strcmp(key, "exp.rested.enabled"))
	{
		++property_reads;
		return feature_enabled < 0 ? fallback : feature_enabled;
	}
	return fallback;
}

float get_property(const char *, double fallback)
{
	return static_cast<float>(fallback);
}

float get_property(const char *, double fallback, bool)
{
	return static_cast<float>(fallback);
}

int restoreCharOnly(P_char ch, char *name)
{
	++restore_calls;
	ch->player.name = strdup(name);
	ch->player.time.saved = 0;
	return 0;
}

void cleanup_temp_char(P_char) {}

int websocket_send_text(P_desc, const char *text)
{
	sent_json = text ? text : "";
	return 0;
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "default";
	const bool authenticated = strcmp(mode, "unauth") != 0;
	if (!strcmp(mode, "0"))
		feature_enabled = 0;
	else if (!strcmp(mode, "1"))
		feature_enabled = 1;
	else if (!strcmp(mode, "unauth"))
		feature_enabled = 0;
	else
		feature_enabled = -1;

	acct_entry account = {};
	acct_chars character = {};
	character.charname = const_cast<char *>("fixture");
	account.acct_character_list = &character;

	descriptor_data descriptor = {};
	descriptor.account = authenticated ? &account : nullptr;
	descriptor.websocket = 1;

	ws_cmd_rested_bonus(&descriptor, nullptr);
	std::puts(sent_json.c_str());
	std::printf("restore_calls=%d\n", restore_calls);
	std::printf("property_reads=%d\n", property_reads);
	return sent_json.empty() ? 1 : 0;
}
