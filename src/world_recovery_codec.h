#ifndef WORLD_RECOVERY_CODEC_H
#define WORLD_RECOVERY_CODEC_H

#include "world_recovery_pipeline.h"

#include <stddef.h>
#include <stdint.h>
#include <vector>

constexpr size_t WORLD_RECOVERY_WIRE_HEADER_BYTES = 64;
constexpr size_t WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES = 8;
constexpr uint8_t WORLD_RECOVERY_RECORD_VERSION = 1;
constexpr size_t WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES = 8;
constexpr size_t WORLD_RECOVERY_WIRE_ITEM_BYTES = 436;
constexpr size_t WORLD_RECOVERY_FLOOR_PREFIX_BYTES = 5;

enum class world_recovery_record_type : uint8_t
{
	mob = 1,
	object = 2,
	door = 3,
	zone = 4,
};

bool world_recovery_encode_header(const world_recovery_header *header, unsigned char *output,
				  size_t output_size);
bool world_recovery_decode_header(const unsigned char *data, size_t size,
				  world_recovery_header *header);
bool world_recovery_encode_record_header(world_recovery_record_type type, uint32_t payload_size,
					 unsigned char *output, size_t output_size);
bool world_recovery_decode_record_header(const unsigned char *data, size_t size,
					 world_recovery_record_type *type, uint32_t *payload_size);
bool world_recovery_encode_record(world_recovery_record_type type, const unsigned char *native_data,
				  size_t native_size, unsigned char *output, size_t output_capacity,
				  size_t *output_size);
bool world_recovery_record_native_size(world_recovery_record_type type,
				       const unsigned char *wire_data, size_t wire_size,
				       size_t *native_size);
bool world_recovery_decode_record(world_recovery_record_type type, const unsigned char *wire_data,
				  size_t wire_size, std::vector<unsigned char> *native_record);
bool world_recovery_encode_floor_object(const unsigned char *native_data, size_t native_size,
					std::vector<unsigned char> *encoded);
bool world_recovery_floor_object_root_uid(const unsigned char *encoded, size_t encoded_size,
					  uint64_t *root_uid);

#endif
