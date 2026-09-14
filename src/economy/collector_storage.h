#ifndef DURIS_COLLECTOR_STORAGE_H
#define DURIS_COLLECTOR_STORAGE_H

#include "economy/collector_codec.h"
#include "item/item_ownership_runtime.h"

#include <cstdint>
#include <vector>

// Immutable values transferred from persistence workers to the game thread.
// They deliberately contain no live game pointers.
struct collector_bootstrap_snapshot
{
	collector::catalog catalog;
	std::vector<item_ownership_runtime_entry> held_items;
};

struct collector_listing_detail
{
	collector::record entry;
	std::vector<uint8_t> item_blob;
};

#endif
