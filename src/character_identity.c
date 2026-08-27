#include "prototypes.h"

extern P_char character_list;

static uint64_t next_runtime_id = 0;

uint64_t allocate_character_runtime_id()
{
	const uint64_t runtime_id = ++next_runtime_id;

	if (!runtime_id)
		panic_corruption("character", "process-local character identity exhausted");
	return runtime_id;
}

P_char find_character_by_runtime_id(uint64_t runtime_id)
{
	P_char character;

	if (!runtime_id)
		return NULL;
	for (character = character_list; character; character = character->next)
		if (character->runtime_id == runtime_id)
			return character;
	return NULL;
}
