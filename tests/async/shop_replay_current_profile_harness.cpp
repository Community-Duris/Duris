#include "economy/shop_trade_transaction.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef NDEBUG
#error SHOP metadata acceptance requires executable assertions
#endif

int main(int argc, char **argv)
{
	assert(argc == 2);
	const bool supported = std::strcmp(argv[1], "supported") == 0;
	assert(supported || std::strcmp(argv[1], "refusal") == 0);
	constexpr size_t sentinel = 0x51a7;
	errno = EDOM;
	size_t source = sentinel, initial = sentinel;
	assert(!shop_trade_transaction_replay_current_source_frame_bytes(nullptr));
	assert(!shop_trade_transaction_replay_current_initial_inline_bytes(nullptr));
	assert(shop_trade_transaction_replay_current_source_frame_bytes(&source) == supported);
	assert(shop_trade_transaction_replay_current_initial_inline_bytes(&initial) == supported);
	const size_t source_query =
		shop_trade_transaction_replay_current_source_query_frame_bytes();
	const size_t initial_query =
		shop_trade_transaction_replay_current_initial_query_frame_bytes();
	assert(errno == EDOM);
	if (supported)
	{
		// INITIAL is the genuine inline CURRENT accumulator, separate from all
		// source signatures/locals and the retained map/private-stage storage.
		assert(initial == sizeof(size_t));
		assert(source > initial && source != SIZE_MAX);
		assert(source_query > sizeof(size_t) && source_query != SIZE_MAX);
		assert(initial_query > sizeof(size_t) && initial_query != SIZE_MAX);
		for (int retry = 0; retry < 4; ++retry)
		{
			size_t repeated_source = sentinel, repeated_initial = sentinel;
			assert(shop_trade_transaction_replay_current_source_frame_bytes(
				&repeated_source));
			assert(shop_trade_transaction_replay_current_initial_inline_bytes(
				&repeated_initial));
			assert(repeated_source == source && repeated_initial == initial);
			assert(shop_trade_transaction_replay_current_source_query_frame_bytes() ==
			       source_query);
			assert(shop_trade_transaction_replay_current_initial_query_frame_bytes() ==
			       initial_query);
		}
	}
	else
	{
		assert(source == sentinel && initial == sentinel);
		assert(source_query == SIZE_MAX && initial_query == SIZE_MAX);
	}
	// No scheduler binding, pending trade, SQL session, root lock, constructor,
	// recovery command or publication is supplied: these are passive queries.
	std::printf(
		"SHOP CURRENT metadata: source=%zu initial=%zu source_query=%zu initial_query=%zu\n",
		source, initial, source_query, initial_query);
}
