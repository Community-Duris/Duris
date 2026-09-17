#ifndef NET_SESSION_INPUT_H
#define NET_SESSION_INPUT_H

#include <cstdint>

// A session input decision identifies the existing owner that will consume one
// dequeued line. It does not own descriptor state, queues, prompts, or command
// effects; those remain with comm.c and the existing mode-specific handlers.
enum class session_input_route : uint8_t
{
	none,
	authentication_pending,
	pager,
	editor,
	playing,
	nanny,
};

inline bool session_input_route_dispatches(session_input_route route)
{
	return route == session_input_route::pager || route == session_input_route::editor ||
	       route == session_input_route::playing || route == session_input_route::nanny;
}

#endif
