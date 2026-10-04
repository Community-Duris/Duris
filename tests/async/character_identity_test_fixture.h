#ifndef DURIS_CHARACTER_IDENTITY_TEST_FIXTURE_H
#define DURIS_CHARACTER_IDENTITY_TEST_FIXTURE_H

#include "core/prototypes.h"
#include <cassert>
#include <cstdlib>
#include <thread>

extern P_char character_list;

// The native fixture owns one game thread; workers cannot touch its real index.
static const auto fixture_game_thread = std::this_thread::get_id();
bool nevent_require_game_thread(const char *)
{
	return std::this_thread::get_id() == fixture_game_thread;
}
#ifdef DURIS_CHARACTER_IDENTITY_TEST_PANIC_STUB
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
#endif

static void fixture_register_character(P_char character)
{
	if (!character->runtime_id)
		character->runtime_id = allocate_character_runtime_id();
	register_character_runtime_id(character);
	assert(find_character_by_runtime_id(character->runtime_id) == character);
}

static void fixture_retire_character(P_char character)
{
	const auto identity = character->runtime_id;
	unregister_character_runtime_id(character);
	assert(find_character_by_runtime_id(identity) == nullptr);
}

struct fixture_character_registration
{
	P_char character;
	explicit fixture_character_registration(P_char value)
		: character(value)
	{
		fixture_register_character(character);
	}
	~fixture_character_registration() { fixture_retire_character(character); }
};

static void fixture_check_runtime_identity_retirement()
{
	char_data character{};
	const auto saved_list = character_list;
	character_list = &character;
	fixture_register_character(&character);
	const auto retired_identity = character.runtime_id;
	fixture_retire_character(&character);
	// A retained list pointer is not authority after retirement.
	assert(character_list == &character);
	assert(find_character_by_runtime_id(retired_identity) == nullptr);
	character.runtime_id = allocate_character_runtime_id();
	fixture_register_character(&character);
	assert(find_character_by_runtime_id(retired_identity) == nullptr);
	bool worker_lookup_refused = false;
	std::thread worker(
		[&] {
			worker_lookup_refused =
				find_character_by_runtime_id(character.runtime_id) == nullptr;
		});
	worker.join();
	assert(worker_lookup_refused);
	fixture_retire_character(&character);
	character_list = saved_list;
}

#endif
