#include "sql/sql.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
room_data rooms[1] = {};
zone_data zones[1] = {};
std::string logged_file;
std::string logged_line;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}
} // namespace

P_room world = rooms;
zone_data *zone_table = zones;
P_index mob_index = nullptr;
int top_of_zone_table = 0;
extern const int top_of_world = 0;

void debug(const char *, ...) {}

void logit(const char *file, const char *format, ...)
{
	char line[MAX_STRING_LENGTH];
	va_list arguments;
	va_start(arguments, format);
	const int length = vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	require(length >= 0 && length < static_cast<int>(sizeof(line)),
		"flat SQL log output overflowed its bound");
	logged_file = file ? file : "";
	logged_line.assign(line, static_cast<size_t>(length));
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main()
{
	rooms[0].number = 1201;
	rooms[0].zone = 0;
	zones[0].number = 12;

	pc_only_data player_data = {};
	player_data.pid = 42;
	descriptor_data descriptor = {};
	snprintf(descriptor.host, sizeof(descriptor.host), "127.0.0.1\nforged");
	char_data player = {};
	player.in_room = 0;
	player.only.pc = &player_data;
	player.desc = &descriptor;
	player.player.name = const_cast<char *>("Tester\rName");

	sql_log(&player, CONNECTLOG, "Connected\r\nforged %d", 7);
	require(logged_file == LOG_PLAYER, "connection event did not use the player log");
	require(logged_line.find("kind=connect") != std::string::npos &&
			logged_line.find("ip=127.0.0.1 forged") != std::string::npos &&
			logged_line.find("pid=42 player=Tester Name") != std::string::npos &&
			logged_line.find("zone=12 room=1201") != std::string::npos &&
			logged_line.find("message=Connected  forged 7") != std::string::npos,
		"connection event lost database-visible log fields");
	require(logged_line.find('\n') == std::string::npos &&
			logged_line.find('\r') == std::string::npos,
		"control characters escaped the flat log record");

	sql_log(&player, WIZLOG, "Loaded object");
	require(logged_file == LOG_WIZ, "staff event did not use the durable staff log");
	sql_log(&player, EXPLOG, "Experience gained");
	require(logged_file == LOG_EXP, "experience event did not use the experience log");

	sql_log_player_login(&player, "login");
	require(logged_file == LOG_PLAYER &&
			logged_line.find("message=Session audit: login") != std::string::npos,
		"login audit event disappeared in flat-file mode");
	logged_line.clear();
	sql_log_player_login(&player, "invalid");
	require(logged_line.empty(), "invalid session audit status was accepted");

	const std::string oversized(MAX_STRING_LENGTH, 'x');
	sql_log(&player, PLAYERLOG, "%s", oversized.c_str());
	require(logged_line.empty(), "oversized flat log event was truncated and accepted");

	std::cout << "flat-file SQL log runtime passed\n";
	return 0;
}
