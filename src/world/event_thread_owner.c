#include "core/prototypes.h"

#include <thread>

static std::thread::id nevent_game_thread;
static bool nevent_game_thread_bound = false;

void nevent_bind_game_thread()
{
	const std::thread::id caller = std::this_thread::get_id();

	if (!nevent_game_thread_bound)
	{
		nevent_game_thread = caller;
		nevent_game_thread_bound = true;
		return;
	}
	if (nevent_game_thread != caller)
		panic_corruption("nevent",
				 "attempted to rebind scheduler ownership from another thread");
}

bool nevent_is_game_thread()
{
	return nevent_game_thread_bound && nevent_game_thread == std::this_thread::get_id();
}
