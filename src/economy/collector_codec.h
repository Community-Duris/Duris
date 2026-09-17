#ifndef DURIS_COLLECTOR_CODEC_H
#define DURIS_COLLECTOR_CODEC_H

#include "economy/collector_policy.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace collector
{
// V1 is a fixed-width little-endian metadata format. Catalogs begin with
// "DCAT", the codec version, two reserved zero bytes, catalog revision,
// next-listing cursor, and record count. Object payloads remain the transaction
// layer's responsibility and are deliberately not embedded here.
constexpr uint16_t catalog_codec_version = 1;
constexpr size_t catalog_header_bytes = 28;
constexpr size_t encoded_record_bytes = 154;
constexpr uint32_t catalog_max_records = 262144;

struct catalog
{
	uint64_t revision = 0;
	uint64_t next_listing = 1;
	std::vector<record> records;
};

enum class codec_result : uint8_t
{
	ok = 0,
	invalid,
	malformed,
	unsupported_version,
	too_many_records,
	allocation_failure,
};

bool valid_catalog(const catalog &value);
codec_result record_encode(const record &entry, std::array<uint8_t, encoded_record_bytes> *encoded);
codec_result record_decode(const uint8_t *encoded, size_t size, record *entry);
codec_result catalog_encode(const catalog &value, std::vector<uint8_t> *encoded);
codec_result catalog_decode(const uint8_t *encoded, size_t size, catalog *value);
}

#endif
