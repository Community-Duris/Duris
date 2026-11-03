#include "world/zone_story_quest_feature.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
static size_t remaining;
static bool hit;
void *operator new(size_t size)
{
	if (remaining && --remaining == 0)
	{
		hit = true;
		throw std::bad_alloc();
	}
	if (void *p = std::malloc(size ? size : 1))
		return p;
	throw std::bad_alloc();
}
void *operator new[](size_t size)
{
	return ::operator new(size);
}
void operator delete(void *p) noexcept
{
	std::free(p);
}
void operator delete[](void *p) noexcept
{
	std::free(p);
}
void operator delete(void *p, size_t) noexcept
{
	std::free(p);
}
void operator delete[](void *p, size_t) noexcept
{
	std::free(p);
}
int main()
{
	zone_story_quest_feature::service state;
	for (unsigned pid = 1; pid < 30; ++pid)
		state.remember_character(1, pid, std::string(128, 'x'));
	const auto healthy = state.serialize_state();
	assert(healthy.starts_with("ZSQF|3\n") && healthy.size() > 29 * 128);
	size_t injected_failures = 0;
	bool completed = false;
	for (size_t index = 1; index < 500; ++index)
	{
		remaining = index;
		hit = false;
		std::string encoded;
		try
		{
			encoded = state.serialize_state();
		}
		catch (const std::bad_alloc &)
		{
			remaining = 0;
			++injected_failures;
			continue;
		}
		remaining = 0;
		if (hit)
			++injected_failures;
		if (hit && !encoded.empty() && encoded != healthy)
		{
			std::cerr
				<< "RED: production zone-story serializer returned partial personal state after allocation failure\n";
			return 1;
		}
		if (!hit)
		{
			assert(encoded == healthy);
			completed = true;
			break;
		}
	}
	assert(injected_failures > 0 && completed);
	std::cout << "PASS: serialization allocation failures cannot return partial state\n";
}
