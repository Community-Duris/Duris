#include "core/refresh_cache.h"
#include <atomic>
#include <cassert>
#include <thread>
#include <iostream>

std::atomic<bool> release_load{ false };
bool held_load(std::string &value, std::string &)
{
	while (!release_load.load())
		std::this_thread::yield();
	value = "first";
	return true;
}
bool fail_load(std::string &, std::string &error)
{
	error = "offline";
	return false;
}
bool next_load(std::string &value, std::string &)
{
	value = "second";
	return true;
}
bool throw_load(std::string &, std::string &)
{
	throw 1;
}
int main()
{
	refresh_cache<std::string> cache;
	assert(!cache.get());
	assert(cache.request(held_load));
	assert(!cache.request(next_load));
	// Simulate a second player's work while a read is held indefinitely.
	for (int commands = 0; commands < 10000; ++commands)
	{
		assert(!cache.poll());
		assert(!cache.get());
	}
	release_load = true;
	cache.shutdown();
	assert(*cache.get() == "first");
	assert(cache.request(fail_load));
	assert(*cache.get() == "first");
	cache.shutdown();
	assert(*cache.get() == "first");
	assert(cache.status().find("offline") != std::string::npos);
	assert(cache.request(throw_load));
	cache.shutdown();
	assert(*cache.get() == "first");
	assert(cache.request(next_load));
	cache.shutdown();
	assert(*cache.get() == "second");
	assert(cache.status() == "ready, generation 2");
	std::cout << "refresh publication, held worker, failure and shutdown passed\n";
}
