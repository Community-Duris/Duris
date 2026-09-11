#include "cmd/information_cache.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	assert(argc == 2);
	std::filesystem::current_path(argv[1]);
	assert(!information_cache_get("credits"));
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(!information_cache_get("credits"));
	std::filesystem::create_directories("lib/information");
	for (const char *name : { "credits", "faq", "wizlist" })
		std::ofstream(std::string("lib/information/") + name) << name;
	assert(information_cache_refresh());
	assert(information_cache_refresh());
	information_cache_shutdown();
	for (const char *name : { "credits", "faq", "wizlist" })
		assert(*information_cache_get(name) == name);
	assert(!information_cache_get("lock"));
	// A pager's owned text outlives a replaced snapshot.
	std::string pager = *information_cache_get("credits");
	std::ofstream("lib/information/credits") << "new credits";
	std::ofstream("lib/information/faq");
	assert(information_cache_refresh());
	assert(*information_cache_get("credits") == "credits");
	information_cache_shutdown();
	assert(pager == "credits");
	assert(*information_cache_get("credits") == "new credits");
	assert(information_cache_get("faq")->empty());
	std::filesystem::remove("lib/information/wizlist");
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "wizlist");
	std::ofstream("lib/information/wizlist") << std::string(131073, 'x');
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "wizlist");
	std::filesystem::remove("lib/information/wizlist");
	assert(mkfifo("lib/information/wizlist", 0600) == 0);
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(*information_cache_get("wizlist") == "wizlist");
	std::filesystem::remove("lib/information/wizlist");
	std::ofstream("lib/information/wizlist") << std::string(128 * 1024, 'x');
	assert(information_cache_refresh());
	information_cache_shutdown();
	assert(information_cache_get("wizlist")->size() == 128 * 1024);
	for (int i = 0; i < 10000; ++i)
		assert(*information_cache_get("credits") == "new credits");
	std::cout
		<< "information cache initial failure, refresh, empty/missing/oversized content and owned text passed\n";
}
