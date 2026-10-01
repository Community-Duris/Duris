#ifndef CHAOS_POUCH_TYPES_H
#define CHAOS_POUCH_TYPES_H

#include <cstddef>
#include <cstdint>

constexpr size_t CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS = 8;
constexpr size_t CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES = 3500;

struct chaos_material_pouch_usage
{
	int vnum;
	uint64_t count;
};

enum class chaos_pouch_usage_mode : uint8_t
{
	generated = 1,
	collected = 2,
};

#endif
