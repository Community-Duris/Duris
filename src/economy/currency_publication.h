#ifndef CURRENCY_PUBLICATION_H
#define CURRENCY_PUBLICATION_H

#include <cstdint>

// Currency durability and live publication are deliberately separate. The
// coordinator owns the durable completion; this state belongs to the game
// thread's publication adapter and remains keyed by the same operation ID.
enum class currency_publication_state : uint8_t
{
	awaiting_completion,
	ready,
	waiting_for_player,
	retrying_callback,
	blocked_receipt,
};

inline bool currency_publication_state_is_ready(currency_publication_state state)
{
	return state == currency_publication_state::ready ||
	       state == currency_publication_state::retrying_callback;
}

inline bool currency_publication_state_is_live_pending(currency_publication_state state)
{
	return currency_publication_state_is_ready(state) ||
	       state == currency_publication_state::waiting_for_player;
}

inline bool currency_publication_state_is_blocked(currency_publication_state state)
{
	return state == currency_publication_state::blocked_receipt;
}

#endif
