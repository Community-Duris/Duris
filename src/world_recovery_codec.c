#include "world_recovery_codec.h"

#include "copyover.h"

#include <climits>
#include <cstring>
#include <limits>
#include <new>

namespace
{
constexpr size_t MOB_EQUIPMENT_COUNT = 43;
constexpr size_t MOB_WIRE_FIXED_BYTES = 282;
constexpr size_t AFFECT_WIRE_BYTES = 60;
constexpr size_t DOOR_WIRE_BYTES = 12;
constexpr size_t ZONE_WIRE_BYTES = 20;
constexpr char FLOOR_MAGIC[] = "WRF3:";
static_assert(sizeof(int) == sizeof(int32_t));
static_assert(sizeof(unsigned int) == sizeof(uint32_t));
static_assert(sizeof(unsigned long) <= sizeof(uint64_t));
static_assert(MOB_WIRE_FIXED_BYTES <= sizeof(copyover_mob));
static_assert(AFFECT_WIRE_BYTES <= sizeof(copyover_affect));
static_assert(WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES <= sizeof(world_recovery_object_record));
static_assert(WORLD_RECOVERY_WIRE_ITEM_BYTES <= sizeof(world_recovery_item_snapshot));
static_assert(DOOR_WIRE_BYTES <= sizeof(copyover_room));
static_assert(ZONE_WIRE_BYTES <= sizeof(zone_age_entry));

void put_u16(unsigned char *output, uint16_t value)
{
	output[0] = static_cast<unsigned char>(value);
	output[1] = static_cast<unsigned char>(value >> 8);
}

void put_u32(unsigned char *output, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		output[index] = static_cast<unsigned char>(value >> (index * 8));
}

void put_u64(unsigned char *output, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		output[index] = static_cast<unsigned char>(value >> (index * 8));
}

uint16_t get_u16(const unsigned char *data)
{
	return static_cast<uint16_t>(data[0]) | static_cast<uint16_t>(data[1]) << 8;
}

uint32_t get_u32(const unsigned char *data)
{
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(data[index]) << (index * 8);
	return value;
}

uint64_t get_u64(const unsigned char *data)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= static_cast<uint64_t>(data[index]) << (index * 8);
	return value;
}

void put_i16(unsigned char *output, int16_t value)
{
	put_u16(output, static_cast<uint16_t>(value));
}

void put_i32(unsigned char *output, int32_t value)
{
	put_u32(output, static_cast<uint32_t>(value));
}

void put_i64(unsigned char *output, int64_t value)
{
	put_u64(output, static_cast<uint64_t>(value));
}

int16_t get_i16(const unsigned char *data)
{
	const uint16_t value = get_u16(data);
	int16_t result = 0;
	memcpy(&result, &value, sizeof(result));
	return result;
}

int32_t get_i32(const unsigned char *data)
{
	const uint32_t value = get_u32(data);
	int32_t result = 0;
	memcpy(&result, &value, sizeof(result));
	return result;
}

int64_t get_i64(const unsigned char *data)
{
	const uint64_t value = get_u64(data);
	int64_t result = 0;
	memcpy(&result, &value, sizeof(result));
	return result;
}

bool encode_mob(const unsigned char *native_data, size_t native_size, unsigned char *output,
		size_t output_capacity, size_t *output_size)
{
	if (!native_data || native_size < sizeof(copyover_mob) || !output || !output_size)
		return false;
	copyover_mob mob = {};
	memcpy(&mob, native_data, sizeof(mob));
	if (mob.num_affects < 0 || mob.num_carrying != 0 ||
	    static_cast<size_t>(mob.num_affects) >
		    (WORLD_RECOVERY_MAX_RECORD_BYTES - sizeof(mob)) / sizeof(copyover_affect) ||
	    native_size !=
		    sizeof(mob) + static_cast<size_t>(mob.num_affects) * sizeof(copyover_affect))
		return false;
	const size_t encoded_size =
		MOB_WIRE_FIXED_BYTES + static_cast<size_t>(mob.num_affects) * AFFECT_WIRE_BYTES;
	if (encoded_size > output_capacity)
		return false;
	size_t offset = 0;
#define PUT_MOB_I32(field)                                         \
	put_i32(output + offset, static_cast<int32_t>(mob.field)); \
	offset += 4
	PUT_MOB_I32(vnum);
	PUT_MOB_I32(idnum);
	PUT_MOB_I32(room);
	PUT_MOB_I32(hit);
	PUT_MOB_I32(max_hit);
	PUT_MOB_I32(mana);
	PUT_MOB_I32(max_mana);
	PUT_MOB_I32(vitality);
	PUT_MOB_I32(max_vitality);
	PUT_MOB_I32(position);
	PUT_MOB_I32(fighting_type);
	PUT_MOB_I32(fighting_id);
#undef PUT_MOB_I32
	memcpy(output + offset, mob.fighting_name, sizeof(mob.fighting_name));
	offset += sizeof(mob.fighting_name);
	put_u32(output + offset, static_cast<uint32_t>(mob.num_affects));
	offset += 4;
	for (int equipment : mob.equipment_vnums)
	{
		put_i32(output + offset, static_cast<int32_t>(equipment));
		offset += 4;
	}
	put_u32(output + offset, 0);
	offset += 4;
	put_i32(output + offset, static_cast<int32_t>(mob.gold));
	offset += 4;
	for (int index = 0; index < mob.num_affects; ++index)
	{
		copyover_affect affect = {};
		memcpy(&affect,
		       native_data + sizeof(mob) + static_cast<size_t>(index) * sizeof(affect),
		       sizeof(affect));
		put_i16(output + offset, static_cast<int16_t>(affect.type));
		offset += 2;
		output[offset++] = affect.wear_off_message_index;
		output[offset++] = 0;
		put_i32(output + offset, static_cast<int32_t>(affect.duration));
		offset += 4;
		put_u32(output + offset, static_cast<uint32_t>(affect.flags));
		offset += 4;
		put_i32(output + offset, static_cast<int32_t>(affect.modifier));
		offset += 4;
		output[offset++] = affect.location;
		output[offset++] = affect.loc2;
		put_u16(output + offset, affect.level);
		offset += 2;
		const unsigned long vectors[] = { affect.bitvector, affect.bitvector2,
						  affect.bitvector3, affect.bitvector4,
						  affect.bitvector5 };
		for (unsigned long vector : vectors)
		{
			put_u64(output + offset, static_cast<uint64_t>(vector));
			offset += 8;
		}
	}
	*output_size = offset;
	return offset == encoded_size;
}

bool mob_native_size(const unsigned char *wire_data, size_t wire_size, size_t *native_size)
{
	if (!wire_data || !native_size || wire_size < MOB_WIRE_FIXED_BYTES)
		return false;
	const uint32_t affect_count = get_u32(wire_data + 98);
	const uint32_t carrying_count = get_u32(wire_data + 274);
	if (!memchr(wire_data + 48, '\0', 50) || carrying_count ||
	    affect_count > (WORLD_RECOVERY_MAX_RECORD_BYTES - sizeof(copyover_mob)) /
				   sizeof(copyover_affect) ||
	    wire_size !=
		    MOB_WIRE_FIXED_BYTES + static_cast<size_t>(affect_count) * AFFECT_WIRE_BYTES)
		return false;
	for (size_t index = 0; index < MOB_EQUIPMENT_COUNT; ++index)
		if (get_i32(wire_data + 102 + index * 4) > 0)
			return false;
	*native_size =
		sizeof(copyover_mob) + static_cast<size_t>(affect_count) * sizeof(copyover_affect);
	return *native_size <= WORLD_RECOVERY_MAX_RECORD_BYTES;
}

bool decode_mob(const unsigned char *wire_data, size_t wire_size,
		std::vector<unsigned char> *native_record)
{
	size_t native_size = 0;
	if (!native_record || !mob_native_size(wire_data, wire_size, &native_size))
		return false;
	try
	{
		native_record->assign(native_size, 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	copyover_mob mob = {};
	size_t offset = 0;
#define GET_MOB_I32(field)                       \
	mob.field = get_i32(wire_data + offset); \
	offset += 4
	GET_MOB_I32(vnum);
	GET_MOB_I32(idnum);
	GET_MOB_I32(room);
	GET_MOB_I32(hit);
	GET_MOB_I32(max_hit);
	GET_MOB_I32(mana);
	GET_MOB_I32(max_mana);
	GET_MOB_I32(vitality);
	GET_MOB_I32(max_vitality);
	GET_MOB_I32(position);
	GET_MOB_I32(fighting_type);
	GET_MOB_I32(fighting_id);
#undef GET_MOB_I32
	memcpy(mob.fighting_name, wire_data + offset, sizeof(mob.fighting_name));
	offset += sizeof(mob.fighting_name);
	mob.num_affects = static_cast<int>(get_u32(wire_data + offset));
	offset += 4;
	for (int &equipment : mob.equipment_vnums)
	{
		equipment = get_i32(wire_data + offset);
		offset += 4;
	}
	mob.num_carrying = static_cast<int>(get_u32(wire_data + offset));
	offset += 4;
	mob.gold = get_i32(wire_data + offset);
	offset += 4;
	memcpy(native_record->data(), &mob, sizeof(mob));
	for (int index = 0; index < mob.num_affects; ++index)
	{
		copyover_affect affect = {};
		affect.type = get_i16(wire_data + offset);
		offset += 2;
		affect.wear_off_message_index = wire_data[offset++];
		++offset;
		affect.duration = get_i32(wire_data + offset);
		offset += 4;
		affect.flags = get_u32(wire_data + offset);
		offset += 4;
		affect.modifier = get_i32(wire_data + offset);
		offset += 4;
		affect.location = wire_data[offset++];
		affect.loc2 = wire_data[offset++];
		affect.level = get_u16(wire_data + offset);
		offset += 2;
		unsigned long *vectors[] = { &affect.bitvector, &affect.bitvector2,
					     &affect.bitvector3, &affect.bitvector4,
					     &affect.bitvector5 };
		for (unsigned long *vector : vectors)
		{
			const uint64_t value = get_u64(wire_data + offset);
			if (value > ULONG_MAX)
				return false;
			*vector = static_cast<unsigned long>(value);
			offset += 8;
		}
		memcpy(native_record->data() + sizeof(mob) +
			       static_cast<size_t>(index) * sizeof(affect),
		       &affect, sizeof(affect));
	}
	return offset == wire_size;
}

bool encode_object(const unsigned char *native_data, size_t native_size, unsigned char *output,
		   size_t output_capacity, size_t *output_size)
{
	if (!native_data || native_size < sizeof(world_recovery_object_record) || !output ||
	    !output_size)
		return false;
	world_recovery_object_record record = {};
	memcpy(&record, native_data, sizeof(record));
	if (!record.item_count || record.item_count > WORLD_RECOVERY_MAX_ITEM_TREE ||
	    native_size != sizeof(record) + static_cast<size_t>(record.item_count) *
						    sizeof(world_recovery_item_snapshot))
		return false;
	const size_t encoded_size =
		WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES +
		static_cast<size_t>(record.item_count) * WORLD_RECOVERY_WIRE_ITEM_BYTES;
	if (encoded_size > output_capacity)
		return false;
	put_i32(output, record.room_vnum);
	put_u32(output + 4, record.item_count);
	size_t offset = WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES;
	for (uint32_t index = 0; index < record.item_count; ++index)
	{
		world_recovery_item_snapshot item = {};
		memcpy(&item,
		       native_data + sizeof(record) + static_cast<size_t>(index) * sizeof(item),
		       sizeof(item));
		put_u64(output + offset, item.item_uid);
		offset += 8;
		put_u64(output + offset, item.root_item_uid);
		offset += 8;
		put_u64(output + offset, item.parent_item_uid);
		offset += 8;
		put_i32(output + offset, item.vnum);
		offset += 4;
		put_i32(output + offset, item.type);
		offset += 4;
		for (int32_t value : item.values)
		{
			put_i32(output + offset, value);
			offset += 4;
		}
		for (int64_t timer : item.timers)
		{
			put_i64(output + offset, timer);
			offset += 8;
		}
		memcpy(output + offset, item.name, sizeof(item.name));
		offset += sizeof(item.name);
		memcpy(output + offset, item.short_description, sizeof(item.short_description));
		offset += sizeof(item.short_description);
		memcpy(output + offset, item.description, sizeof(item.description));
		offset += sizeof(item.description);
	}
	*output_size = offset;
	return offset == encoded_size;
}

bool object_native_size(const unsigned char *wire_data, size_t wire_size, size_t *native_size)
{
	if (!wire_data || !native_size || wire_size < WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES)
		return false;
	const uint32_t item_count = get_u32(wire_data + 4);
	if (!item_count || item_count > WORLD_RECOVERY_MAX_ITEM_TREE ||
	    wire_size != WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES +
				 static_cast<size_t>(item_count) * WORLD_RECOVERY_WIRE_ITEM_BYTES)
		return false;
	*native_size = sizeof(world_recovery_object_record) +
		       static_cast<size_t>(item_count) * sizeof(world_recovery_item_snapshot);
	return *native_size <= WORLD_RECOVERY_MAX_RECORD_BYTES;
}

bool decode_object(const unsigned char *wire_data, size_t wire_size,
		   std::vector<unsigned char> *native_record)
{
	size_t native_size = 0;
	if (!native_record || !object_native_size(wire_data, wire_size, &native_size))
		return false;
	try
	{
		native_record->assign(native_size, 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	world_recovery_object_record record = { get_i32(wire_data), get_u32(wire_data + 4) };
	memcpy(native_record->data(), &record, sizeof(record));
	size_t offset = WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES;
	for (uint32_t index = 0; index < record.item_count; ++index)
	{
		world_recovery_item_snapshot item = {};
		item.item_uid = get_u64(wire_data + offset);
		offset += 8;
		item.root_item_uid = get_u64(wire_data + offset);
		offset += 8;
		item.parent_item_uid = get_u64(wire_data + offset);
		offset += 8;
		item.vnum = get_i32(wire_data + offset);
		offset += 4;
		item.type = get_i32(wire_data + offset);
		offset += 4;
		for (int32_t &value : item.values)
		{
			value = get_i32(wire_data + offset);
			offset += 4;
		}
		for (int64_t &timer : item.timers)
		{
			timer = get_i64(wire_data + offset);
			offset += 8;
		}
		memcpy(item.name, wire_data + offset, sizeof(item.name));
		offset += sizeof(item.name);
		memcpy(item.short_description, wire_data + offset, sizeof(item.short_description));
		offset += sizeof(item.short_description);
		memcpy(item.description, wire_data + offset, sizeof(item.description));
		offset += sizeof(item.description);
		memcpy(native_record->data() + sizeof(record) +
			       static_cast<size_t>(index) * sizeof(item),
		       &item, sizeof(item));
	}
	return offset == wire_size;
}

bool encode_door(const unsigned char *native_data, size_t native_size, unsigned char *output,
		 size_t output_capacity, size_t *output_size)
{
	if (!native_data || native_size != sizeof(copyover_room) || !output ||
	    output_capacity < DOOR_WIRE_BYTES || !output_size)
		return false;
	copyover_room door = {};
	memcpy(&door, native_data, sizeof(door));
	put_i32(output, door.vnum);
	put_i32(output + 4, door.dir);
	put_i32(output + 8, door.state);
	*output_size = DOOR_WIRE_BYTES;
	return true;
}

bool encode_zone(const unsigned char *native_data, size_t native_size, unsigned char *output,
		 size_t output_capacity, size_t *output_size)
{
	if (!native_data || native_size != sizeof(zone_age_entry) || !output ||
	    output_capacity < ZONE_WIRE_BYTES || !output_size)
		return false;
	zone_age_entry zone = {};
	memcpy(&zone, native_data, sizeof(zone));
	put_i32(output, zone.zone_rnum);
	put_i32(output + 4, zone.age);
	put_i32(output + 8, zone.lifespan);
	put_i32(output + 12, zone.fullreset_age);
	put_i32(output + 16, zone.fullreset_lifespan);
	*output_size = ZONE_WIRE_BYTES;
	return true;
}

template <typename T> bool decode_fixed(const unsigned char *wire_data, size_t wire_size,
					size_t expected_wire_size,
					std::vector<unsigned char> *native_record)
{
	if (!wire_data || !native_record || wire_size != expected_wire_size)
		return false;
	try
	{
		native_record->assign(sizeof(T), 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
} // namespace

bool world_recovery_encode_header(const world_recovery_header *header, unsigned char *output,
				  size_t output_size)
{
	if (!header || !output || output_size < WORLD_RECOVERY_WIRE_HEADER_BYTES)
		return false;
	memcpy(output, "WRS9", 4);
	put_u32(output + 4, WORLD_RECOVERY_SCHEMA_VERSION);
	put_u32(output + 8, WORLD_RECOVERY_WIRE_HEADER_BYTES);
	put_u64(output + 12, header->sequence);
	put_i64(output + 20, header->timestamp);
	put_u64(output + 28, header->payload_size);
	put_u32(output + 36, header->checksum);
	put_u32(output + 40, header->mob_count);
	put_u32(output + 44, header->object_count);
	put_u32(output + 48, header->door_count);
	put_u32(output + 52, header->zone_count);
	output[56] = header->complete;
	memset(output + 57, 0, 7);
	return true;
}

bool world_recovery_decode_header(const unsigned char *data, size_t size,
				  world_recovery_header *header)
{
	if (!data || size < WORLD_RECOVERY_WIRE_HEADER_BYTES || !header || memcmp(data, "WRS9", 4))
		return false;
	*header = {};
	memcpy(header->magic, data, 4);
	header->schema_version = get_u32(data + 4);
	header->header_size = get_u32(data + 8);
	header->sequence = get_u64(data + 12);
	header->timestamp = get_i64(data + 20);
	header->payload_size = get_u64(data + 28);
	header->checksum = get_u32(data + 36);
	header->mob_count = get_u32(data + 40);
	header->object_count = get_u32(data + 44);
	header->door_count = get_u32(data + 48);
	header->zone_count = get_u32(data + 52);
	header->complete = data[56];
	for (size_t index = 57; index < WORLD_RECOVERY_WIRE_HEADER_BYTES; ++index)
		if (data[index])
			return false;
	return true;
}

bool world_recovery_encode_record_header(world_recovery_record_type type, uint32_t payload_size,
					 unsigned char *output, size_t output_size)
{
	if (!output || output_size < WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES || !payload_size)
		return false;
	const uint8_t raw_type = static_cast<uint8_t>(type);
	if (raw_type < static_cast<uint8_t>(world_recovery_record_type::mob) ||
	    raw_type > static_cast<uint8_t>(world_recovery_record_type::zone))
		return false;
	put_u32(output, payload_size);
	output[4] = raw_type;
	output[5] = WORLD_RECOVERY_RECORD_VERSION;
	output[6] = 0;
	output[7] = 0;
	return true;
}

bool world_recovery_decode_record_header(const unsigned char *data, size_t size,
					 world_recovery_record_type *type, uint32_t *payload_size)
{
	if (!data || size < WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES || !type || !payload_size ||
	    data[5] != WORLD_RECOVERY_RECORD_VERSION || data[6] || data[7])
		return false;
	const uint8_t raw_type = data[4];
	if (raw_type < static_cast<uint8_t>(world_recovery_record_type::mob) ||
	    raw_type > static_cast<uint8_t>(world_recovery_record_type::zone))
		return false;
	*type = static_cast<world_recovery_record_type>(raw_type);
	*payload_size = get_u32(data);
	return *payload_size > 0 && *payload_size <= WORLD_RECOVERY_MAX_RECORD_BYTES;
}

bool world_recovery_encode_record(world_recovery_record_type type, const unsigned char *native_data,
				  size_t native_size, unsigned char *output, size_t output_capacity,
				  size_t *output_size)
{
	switch (type)
	{
	case world_recovery_record_type::mob:
		return encode_mob(native_data, native_size, output, output_capacity, output_size);
	case world_recovery_record_type::object:
		return encode_object(native_data, native_size, output, output_capacity,
				     output_size);
	case world_recovery_record_type::door:
		return encode_door(native_data, native_size, output, output_capacity, output_size);
	case world_recovery_record_type::zone:
		return encode_zone(native_data, native_size, output, output_capacity, output_size);
	}
	return false;
}

bool world_recovery_record_native_size(world_recovery_record_type type,
				       const unsigned char *wire_data, size_t wire_size,
				       size_t *native_size)
{
	if (!native_size)
		return false;
	switch (type)
	{
	case world_recovery_record_type::mob:
		return mob_native_size(wire_data, wire_size, native_size);
	case world_recovery_record_type::object:
		return object_native_size(wire_data, wire_size, native_size);
	case world_recovery_record_type::door:
		*native_size = sizeof(copyover_room);
		return wire_data && wire_size == DOOR_WIRE_BYTES;
	case world_recovery_record_type::zone:
		*native_size = sizeof(zone_age_entry);
		return wire_data && wire_size == ZONE_WIRE_BYTES;
	}
	return false;
}

bool world_recovery_decode_record(world_recovery_record_type type, const unsigned char *wire_data,
				  size_t wire_size, std::vector<unsigned char> *native_record)
{
	switch (type)
	{
	case world_recovery_record_type::mob:
		return decode_mob(wire_data, wire_size, native_record);
	case world_recovery_record_type::object:
		return decode_object(wire_data, wire_size, native_record);
	case world_recovery_record_type::door:
		if (!decode_fixed<copyover_room>(wire_data, wire_size, DOOR_WIRE_BYTES,
						 native_record))
			return false;
		{
			copyover_room door = { get_i32(wire_data), get_i32(wire_data + 4),
					       get_i32(wire_data + 8) };
			memcpy(native_record->data(), &door, sizeof(door));
		}
		return true;
	case world_recovery_record_type::zone:
		if (!decode_fixed<zone_age_entry>(wire_data, wire_size, ZONE_WIRE_BYTES,
						  native_record))
			return false;
		{
			zone_age_entry zone = { get_i32(wire_data), get_i32(wire_data + 4),
						get_i32(wire_data + 8), get_i32(wire_data + 12),
						get_i32(wire_data + 16) };
			memcpy(native_record->data(), &zone, sizeof(zone));
		}
		return true;
	}
	return false;
}

bool world_recovery_encode_floor_object(const unsigned char *native_data, size_t native_size,
					std::vector<unsigned char> *encoded)
{
	if (!native_data || native_size < sizeof(world_recovery_object_record) || !encoded)
		return false;
	world_recovery_object_record record = {};
	memcpy(&record, native_data, sizeof(record));
	if (!record.item_count || record.item_count > WORLD_RECOVERY_MAX_ITEM_TREE)
		return false;
	const size_t encoded_size =
		WORLD_RECOVERY_FLOOR_PREFIX_BYTES + WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES +
		static_cast<size_t>(record.item_count) * WORLD_RECOVERY_WIRE_ITEM_BYTES;
	try
	{
		encoded->assign(encoded_size, 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	memcpy(encoded->data(), FLOOR_MAGIC, WORLD_RECOVERY_FLOOR_PREFIX_BYTES);
	size_t payload_size = 0;
	if (!encode_object(native_data, native_size,
			   encoded->data() + WORLD_RECOVERY_FLOOR_PREFIX_BYTES,
			   encoded->size() - WORLD_RECOVERY_FLOOR_PREFIX_BYTES, &payload_size))
	{
		encoded->clear();
		return false;
	}
	return WORLD_RECOVERY_FLOOR_PREFIX_BYTES + payload_size == encoded->size();
}

bool world_recovery_floor_object_root_uid(const unsigned char *encoded, size_t encoded_size,
					  uint64_t *root_uid)
{
	if (!encoded || !root_uid || encoded_size <= WORLD_RECOVERY_FLOOR_PREFIX_BYTES ||
	    memcmp(encoded, FLOOR_MAGIC, WORLD_RECOVERY_FLOOR_PREFIX_BYTES))
		return false;
	const unsigned char *wire = encoded + WORLD_RECOVERY_FLOOR_PREFIX_BYTES;
	const size_t wire_size = encoded_size - WORLD_RECOVERY_FLOOR_PREFIX_BYTES;
	size_t native_size = 0;
	if (!object_native_size(wire, wire_size, &native_size))
		return false;
	*root_uid = get_u64(wire + WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES);
	return *root_uid != 0;
}
