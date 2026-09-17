#ifndef DURIS_COLLECTOR_STORAGE_H
#define DURIS_COLLECTOR_STORAGE_H

#include "economy/collector_codec.h"
#include "item/item_ownership_runtime.h"

#include <cstdint>
#include <vector>

constexpr uint8_t COLLECTOR_HINT_NONE = 0;
constexpr uint8_t COLLECTOR_HINT_PENDING = 1;
constexpr uint8_t COLLECTOR_HINT_DELIVERED = 2;

// Immutable values transferred from persistence workers to the game thread.
// They deliberately contain no live game pointers.
struct collector_death_snapshot
{
	critical_operation_id operation_id = {};
	uint32_t beneficiary_pid = 0;
	uint64_t death_time = 0;
	collector::rules policy = {};
	uint8_t hint_state = 0;
	uint64_t hint_revision = 0;
};

struct collector_bootstrap_snapshot
{
	collector::catalog catalog;
	std::vector<item_ownership_runtime_entry> held_items;
	std::vector<collector_death_snapshot> deaths;
};

struct collector_listing_detail
{
	collector::record entry;
	std::vector<uint8_t> item_blob;
};

#endif
