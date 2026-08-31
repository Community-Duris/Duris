#include "sql/sql.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
std::string sent_content;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}
} // namespace

void logit(const char *, const char *, ...) {}

void send_to_char(const char *message, P_char, int)
{
	sent_content = message ? message : "";
}

int main()
{
	const std::string news = get_mud_info("NeWs");
	require(news.find("Added 'taunt'") != std::string::npos,
		"get_mud_info did not return tracked news");
	require(!get_mud_info("motd").empty(), "get_mud_info did not return tracked motd");
	require(get_mud_info("../unsafe").empty(), "get_mud_info accepted an unsafe name");
	send_mud_info("credits", nullptr);
	require(!sent_content.empty(), "send_mud_info did not send tracked credits");
	std::cout << "flat-file mud_info runtime passed\n";
	return 0;
}
