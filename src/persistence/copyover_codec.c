#include "persistence/copyover_codec.h"

#include "item/item_ownership_runtime.h"
#include "world/generated_npc_state.h"
#include "world/world_recovery_codec.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <new>
#include <set>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace
{
constexpr char MAGIC[4] = { 'D', 'C', 'O', 'F' };
constexpr uint32_t WIRE_BYTE_ORDER = 0x01020304U; // Stored as 04 03 02 01 (little endian).
constexpr size_t CHECKSUM_OFFSET = 32;
constexpr uint32_t MAX_WORLD_RECORDS = 1'000'000;
constexpr size_t RECORD_HEADER_BYTES = 8;
constexpr size_t CUSTODY_WIRE_BYTES = 62;
static_assert(sizeof(int) == 4 && sizeof(unsigned int) == 4 && sizeof(sh_int) == 2);
static_assert(sizeof(unsigned long) <= 8 && MAX_WEAR == 43 && MAX_OBJ_AFFECT == 4);

enum class record_type : uint16_t
{
	descriptor = 1,
	telemetry = 2,
	mob = 3,
	affect = 4,
	carried = 5,
	generated = 6,
	object = 7,
	door = 8,
};

uint64_t get_unsigned(const unsigned char *data, size_t width)
{
	uint64_t value = 0;
	for (size_t i = 0; i < width; ++i)
		value |= static_cast<uint64_t>(data[i]) << (8 * i);
	return value;
}

void put_unsigned(unsigned char *data, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		data[i] = static_cast<unsigned char>(value >> (8 * i));
}

// A field list serves both directions; widths are wire constants, never sizeof(T).
struct wire
{
	std::vector<unsigned char> bytes;
	const unsigned char *input = nullptr;
	size_t remaining = 0;
	bool ok = true;
	bool reading = false;

	wire() = default;
	wire(const unsigned char *data, size_t size)
		: input(data)
		, remaining(size)
		, reading(true)
	{
	}

	void raw(void *value, size_t size)
	{
		if (!ok)
			return;
		if (reading)
		{
			if (size > remaining)
			{
				ok = false;
				return;
			}
			memcpy(value, input, size);
			input += size;
			remaining -= size;
		}
		else
		{
			if (size > COPYOVER_MAX_RECORD_BYTES - bytes.size())
			{
				ok = false;
				return;
			}
			const auto *data = static_cast<const unsigned char *>(value);
			bytes.insert(bytes.end(), data, data + size);
		}
	}

	template <typename T> void number(T &value, size_t width, bool signed_value = false)
	{
		unsigned char data[8] = {};
		if (!reading)
			put_unsigned(data, static_cast<uint64_t>(value), width);
		raw(data, width);
		if (!ok || !reading)
			return;
		const uint64_t bits = get_unsigned(data, width);
		if (signed_value)
		{
			const uint64_t mask = width == 8 ? UINT64_MAX :
							   (UINT64_C(1) << (8 * width)) - 1;
			const int64_t decoded = bits & (UINT64_C(1) << (8 * width - 1)) ?
							-1 - static_cast<int64_t>((~bits) & mask) :
							static_cast<int64_t>(bits);
			if (!std::in_range<T>(decoded))
				ok = false;
			else
				value = static_cast<T>(decoded);
		}
		else if (bits > static_cast<uint64_t>(std::numeric_limits<T>::max()))
			ok = false;
		else
			value = static_cast<T>(bits);
	}

	template <size_t N> void text(char (&value)[N])
	{
		if (reading)
			raw(value, N);
		const char *end = static_cast<const char *>(memchr(value, 0, N));
		if (!end)
		{
			ok = false;
			return;
		}
		if (!reading)
		{
			char canonical[N] = {};
			memcpy(canonical, value, static_cast<size_t>(end - value));
			raw(canonical, N);
		}
	}

	bool done() const { return ok && (!reading || remaining == 0); }
};

void fields(wire &w, copyover_desc &e)
{
	w.number(e.fd, 4, true);
	w.text(e.player_name);
	w.text(e.host);
	w.text(e.host2);
	w.number(e.term_type, 1, true);
	for (int *value :
	     { &e.gmcp_enabled, &e.out_compress, &e.room, &e.mtts_flags, &e.charset_detected })
		w.number(*value, 4, true);
	w.text(e.ttype_client);
	w.text(e.ttype_terminal);
	w.number(e.fighting_type, 4, true);
	w.number(e.fighting_id, 4, true);
	w.text(e.fighting_name);
	w.number(e.num_pets, 4);
	for (int *values : { e.pet_vnums, e.pet_hit, e.pet_max_hit })
		for (size_t i = 0; i < 10; ++i)
			w.number(values[i], 4, true);
	w.number(e.death_retry_pending, 1);
	w.number(e.death_retry_delay, 4, true);
	w.number(e.death_retry_corpse_uid, 8);
}

void fields(wire &w, copyover_mob &e)
{
	for (int *value :
	     { &e.vnum, &e.idnum, &e.room, &e.hit, &e.max_hit, &e.mana, &e.max_mana, &e.vitality,
	       &e.max_vitality, &e.position, &e.fighting_type, &e.fighting_id })
		w.number(*value, 4, true);
	w.text(e.fighting_name);
	w.number(e.num_affects, 4);
	for (int &value : e.equipment_vnums)
		w.number(value, 4, true);
	w.number(e.num_carrying, 4);
	w.number(e.gold, 4, true);
	w.number(e.birthplace, 4, true);
	for (int *value : { &e.transport.origin, &e.transport.destination, &e.transport.state,
			    &e.transport.step })
		w.number(*value, 4, true);
	w.text(e.transport.rider);
	w.number(e.shopkeeper_shop_id, 4, true);
}

void fields(wire &w, copyover_affect &e)
{
	w.number(e.type, 2, true);
	w.number(e.wear_off_message_index, 1, true);
	w.number(e.duration, 4, true);
	w.number(e.flags, 4);
	w.number(e.modifier, 4, true);
	w.number(e.location, 1);
	w.number(e.loc2, 1);
	w.number(e.level, 2);
	for (unsigned long *value :
	     { &e.bitvector, &e.bitvector2, &e.bitvector3, &e.bitvector4, &e.bitvector5 })
		w.number(*value, 8);
}

void fields(wire &w, copyover_carried_item &e)
{
	// The old writer never initialized obj_uid, and recovery used only vnum.
	w.number(e.vnum, 4, true);
}

void fields(wire &w, copyover_room &e)
{
	w.number(e.vnum, 4, true);
	w.number(e.dir, 4, true);
	w.number(e.state, 4, true);
}

void fields(wire &w, telemetry_copyover_entry &e)
{
	w.number(e.fd, 4, true);
	w.text(e.player_name);
	w.number(e.handoff_valid, 1);
	auto &h = e.handoff;
	for (uint64_t *value : { &h.session.id.producer.boot_id, &h.session.id.producer.process_id,
				 &h.session.id.session_seq, &h.session.subject_id })
		w.number(*value, 8);
	w.number(h.session.pid, 4, true); // -1 means unknown in telemetry.
	for (uint64_t *value :
	     { &h.session.season_id, &h.session.environment_id, &h.previous_producer.boot_id,
	       &h.previous_producer.process_id, &h.last_checkpoint_revision,
	       &h.cumulative.connected_usec, &h.cumulative.active_usec, &h.cumulative.idle_usec,
	       &h.cumulative.unknown_usec, &h.cumulative.resident_usec,
	       &h.cumulative.linkdead_usec })
		w.number(*value, 8);
	w.number(h.quality_flags, 4);
}

void fields(wire &w, item_ownership_runtime_entry &e)
{
	for (uint64_t *value : { &e.item_uid, &e.root_item_uid, &e.parent_item_uid })
		w.number(*value, 8);
	uint8_t owner = static_cast<uint8_t>(e.owner.type);
	w.number(owner, 1);
	e.owner.type = static_cast<item_owner_type>(owner);
	for (uint64_t *value :
	     { &e.owner.id, &e.owner.context_id, &e.item_revision, &e.owner_revision })
		w.number(*value, 8);
	w.number(e.vnum, 4, true);
	uint8_t state = static_cast<uint8_t>(e.state);
	w.number(state, 1);
	e.state = static_cast<item_custody_state>(state);
}

bool fail(const char **error, const char *message)
{
	if (error)
		*error = message;
	return false;
}

uint32_t checksum(const unsigned char *data, size_t size)
{
	uint32_t crc = UINT32_MAX;
	for (size_t i = 0; i < size; ++i)
	{
		if (i >= CHECKSUM_OFFSET && i < CHECKSUM_OFFSET + 4)
			continue;
		crc ^= data[i];
		for (unsigned bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
	}
	return ~crc;
}

bool write_record(FILE *file, record_type type, const unsigned char *data, size_t size)
{
	if (!file || size > COPYOVER_MAX_RECORD_BYTES)
		return false;
	const long offset = ftell(file);
	if (offset < 0 ||
	    static_cast<size_t>(offset) > COPYOVER_MAX_FILE_BYTES - RECORD_HEADER_BYTES - size)
		return false;
	unsigned char header[RECORD_HEADER_BYTES] = {};
	put_unsigned(header, static_cast<uint16_t>(type), 2);
	put_unsigned(header + 2, 1, 2);
	put_unsigned(header + 4, size, 4);
	return fwrite(header, 1, RECORD_HEADER_BYTES, file) == RECORD_HEADER_BYTES &&
	       fwrite(data, 1, size, file) == size;
}

template <typename T> bool write_fields(FILE *file, record_type type, const T &entry)
{
	try
	{
		wire w;
		T value = entry;
		fields(w, value);
		return w.done() && write_record(file, type, w.bytes.data(), w.bytes.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool framed(wire &file, record_type expected, wire *payload)
{
	uint16_t type = 0, version = 0;
	uint32_t size = 0;
	file.number(type, 2);
	file.number(version, 2);
	file.number(size, 4);
	if (!file.ok || type != static_cast<uint16_t>(expected) || version != 1 ||
	    size > COPYOVER_MAX_RECORD_BYTES || size > file.remaining)
		return false;
	*payload = wire(file.input, size);
	file.input += size;
	file.remaining -= size;
	return true;
}

template <typename T> bool read_fields(wire &file, record_type type, T *entry)
{
	wire payload;
	if (!framed(file, type, &payload))
		return false;
	fields(payload, *entry);
	return payload.done();
}

bool valid_desc(const copyover_desc &e)
{
	return e.fd > 0 && e.player_name[0] && memchr(e.player_name, 0, 50) &&
	       memchr(e.host, 0, 50) && memchr(e.host2, 0, 254) && memchr(e.ttype_client, 0, 64) &&
	       memchr(e.ttype_terminal, 0, 32) && memchr(e.fighting_name, 0, 50) &&
	       e.fighting_type >= 0 && e.fighting_type <= 2 && e.num_pets >= 0 &&
	       e.num_pets <= 10 && e.death_retry_pending <= 1 && !e.death_retry_reserved[0] &&
	       !e.death_retry_reserved[1] && !e.death_retry_reserved[2] &&
	       (e.death_retry_pending ? e.death_retry_delay >= 4 && e.death_retry_delay <= 60 :
					!e.death_retry_delay && !e.death_retry_corpse_uid);
}

bool valid_mob(const copyover_mob &e)
{
	return e.vnum > 0 && e.room >= 0 && e.fighting_type >= 0 && e.fighting_type <= 2 &&
	       e.num_affects >= 0 && e.num_affects <= COPYOVER_MAX_CHILD_RECORDS &&
	       e.num_carrying >= 0 && e.num_carrying <= COPYOVER_MAX_CHILD_RECORDS &&
	       e.shopkeeper_shop_id >= -1 && memchr(e.fighting_name, 0, 50) &&
	       memchr(e.transport.rider, 0, 50);
}

bool valid_counts(const copyover_header &h, size_t bytes, bool portable)
{
	if (h.num_descriptors < 0 || h.num_descriptors > FD_SETSIZE || h.num_mobs < 0 ||
	    h.num_objects < 0 || h.num_rooms < 0 || h.num_combat || h.num_zones ||
	    static_cast<uint32_t>(h.num_mobs) > MAX_WORLD_RECORDS ||
	    static_cast<uint32_t>(h.num_objects) > MAX_WORLD_RECORDS ||
	    static_cast<uint32_t>(h.num_rooms) > MAX_WORLD_RECORDS)
		return false;
	// Necessary minimum sizes, checked before any count-based reserve or resize.
	const uint64_t minimum = static_cast<uint64_t>(h.num_descriptors) * (portable ? 678 + 191 :
									     h.version >= 15 ?
											660 + 200 :
											660) +
				 static_cast<uint64_t>(h.num_mobs) * (portable ? 380 : 288) +
				 static_cast<uint64_t>(h.num_objects) * (portable ? 3348 : 3340) +
				 static_cast<uint64_t>(h.num_rooms) * (portable ? 20 : 12);
	return minimum <= bytes;
}

// Validate portable object values before ANY gameplay object is materialized.
// Runtime prototype/room/custody-conflict checks still belong to the restorer.
bool valid_object(const std::vector<char> &native, std::set<uint64_t> *file_uids)
{
	world_recovery_object_record record = {};
	if (native.size() < sizeof(record))
		return false;
	memcpy(&record, native.data(), sizeof(record));
	if (!record.item_count || record.item_count > WORLD_RECOVERY_MAX_ITEM_TREE ||
	    record.room_vnum < 0)
		return false;
	const size_t tree_bytes = sizeof(record) + static_cast<size_t>(record.item_count) *
							   sizeof(world_recovery_item_snapshot);
	if (tree_bytes > native.size())
		return false;
	std::set<uint64_t> parents;
	uint64_t root = 0;
	size_t custody_offset = tree_bytes;
	for (uint32_t i = 0; i < record.item_count; ++i)
	{
		world_recovery_item_snapshot item = {};
		memcpy(&item,
		       native.data() + sizeof(record) + static_cast<size_t>(i) * sizeof(item),
		       sizeof(item));
		if (i == 0)
			root = item.item_uid;
		if (!item.item_uid || item.item_uid > ULONG_MAX || item.root_item_uid != root ||
		    item.vnum <= 0 || item.type < 0 || item.type > ITEM_LAST ||
		    (item.flags & ~WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED) ||
		    !memchr(item.name, 0, sizeof(item.name)) ||
		    !memchr(item.short_description, 0, sizeof(item.short_description)) ||
		    !memchr(item.description, 0, sizeof(item.description)) ||
		    !memchr(item.action_description, 0, sizeof(item.action_description)) ||
		    (i == 0 ? item.parent_item_uid != 0 : !parents.count(item.parent_item_uid)) ||
		    !parents.insert(item.item_uid).second ||
		    !file_uids->insert(item.item_uid).second)
			return false;
		for (int64_t timer : item.timers)
			if (!std::in_range<time_t>(timer))
				return false;
		for (size_t a = 0; a < MAX_OBJ_AFFECT; ++a)
			if (item.affect_locations[a] < 0 || item.affect_locations[a] > SCHAR_MAX ||
			    item.affect_modifiers[a] < SCHAR_MIN ||
			    item.affect_modifiers[a] > SCHAR_MAX)
				return false;
		if (item.material < 0 || item.material > SCHAR_MAX || item.bitvector > ULONG_MAX ||
		    item.bitvector2 > ULONG_MAX || item.bitvector3 > ULONG_MAX ||
		    item.bitvector4 > ULONG_MAX || item.bitvector5 > ULONG_MAX)
			return false;
		for (int value : { item.trap_eff, item.trap_dam, item.trap_charge, item.trap_level,
				   item.condition, item.craftsmanship, item.z_cord })
			if (value < SHRT_MIN || value > SHRT_MAX)
				return false;
		if (item.flags & WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED)
		{
			if (sizeof(item_ownership_runtime_entry) > native.size() - custody_offset)
				return false;
			item_ownership_runtime_entry entry = {};
			memcpy(&entry, native.data() + custody_offset, sizeof(entry));
			custody_offset += sizeof(entry);
			if (entry.item_uid != item.item_uid || entry.vnum != item.vnum ||
			    !entry.root_item_uid || !item_owner_identity_valid(entry.owner) ||
			    entry.state != item_custody_state::active)
				return false;
		}
	}
	return custody_offset == native.size();
}

bool decode_object(wire &file, std::vector<char> *native)
{
	wire payload;
	if (!framed(file, record_type::object, &payload))
		return false;
	uint32_t tree_size = 0;
	payload.number(tree_size, 4);
	if (!payload.ok || tree_size < 8 || tree_size > payload.remaining)
		return false;
	const unsigned char *tree = payload.input;
	payload.input += tree_size;
	payload.remaining -= tree_size;
	uint32_t count = 0;
	payload.number(count, 4);
	// Both tree count and custody count are validated before allocation.
	size_t native_tree_size = 0;
	if (!payload.ok || count > WORLD_RECOVERY_MAX_ITEM_TREE ||
	    count > get_unsigned(tree + 4, 4) ||
	    payload.remaining != static_cast<size_t>(count) * CUSTODY_WIRE_BYTES ||
	    !world_recovery_record_native_size(world_recovery_record_type::object, tree, tree_size,
					       &native_tree_size) ||
	    native_tree_size + static_cast<size_t>(count) * sizeof(item_ownership_runtime_entry) >
		    WORLD_RECOVERY_MAX_RECORD_BYTES)
		return false;
	std::vector<unsigned char> decoded;
	if (!world_recovery_decode_record(world_recovery_record_type::object, tree, tree_size,
					  &decoded))
		return false;
	native->assign(decoded.begin(), decoded.end());
	native->resize(native_tree_size +
		       static_cast<size_t>(count) * sizeof(item_ownership_runtime_entry));
	for (uint32_t i = 0; i < count; ++i)
	{
		item_ownership_runtime_entry entry = {};
		fields(payload, entry);
		memcpy(native->data() + native_tree_size + static_cast<size_t>(i) * sizeof(entry),
		       &entry, sizeof(entry));
	}
	return payload.done();
}

bool read_portable(wire &file, copyover_decoded_state *state, const char **error)
{
	for (int i = 0; i < state->header.num_descriptors; ++i)
	{
		copyover_desc e = {};
		if (!read_fields(file, record_type::descriptor, &e))
			return fail(error, "invalid descriptor frame");
		state->descriptors.push_back(e);
	}
	bool retain_telemetry = true;
	for (int i = 0; i < state->header.num_descriptors; ++i)
	{
		telemetry_copyover_entry e = {};
		if (!read_fields(file, record_type::telemetry, &e))
			return fail(error, "invalid telemetry frame");
		if (retain_telemetry)
			try
			{
				state->telemetry.push_back(e);
			}
			catch (const std::bad_alloc &)
			{
				state->telemetry.clear();
				retain_telemetry = false;
			}
	}
	for (int i = 0; i < state->header.num_mobs; ++i)
	{
		copyover_decoded_mob mob;
		if (!read_fields(file, record_type::mob, &mob.entry) || !valid_mob(mob.entry) ||
		    static_cast<uint64_t>(mob.entry.num_affects) * 67 +
				    static_cast<uint64_t>(mob.entry.num_carrying) * 12 + 16 >
			    file.remaining)
			return fail(error, "invalid mob or child counts");
		mob.affects.resize(static_cast<size_t>(mob.entry.num_affects));
		mob.inventory.resize(static_cast<size_t>(mob.entry.num_carrying));
		for (auto &e : mob.affects)
			if (!read_fields(file, record_type::affect, &e))
				return fail(error,
					    "invalid affect frame or unrepresentable bitvector");
		for (auto &e : mob.inventory)
			if (!read_fields(file, record_type::carried, &e))
				return fail(error, "invalid inventory frame");
		wire generated;
		if (!framed(file, record_type::generated, &generated) ||
		    !generated_npc_extension_decode(mob.entry.vnum,
						    reinterpret_cast<const char *>(generated.input),
						    generated.remaining, &mob.generated))
			return fail(error, "invalid generated NPC state");
		state->mobs.push_back(std::move(mob));
	}
	for (int i = 0; i < state->header.num_objects; ++i)
	{
		std::vector<char> object;
		if (!decode_object(file, &object))
			return fail(error, "invalid object lengths or counts");
		state->objects.push_back(std::move(object));
	}
	for (int i = 0; i < state->header.num_rooms; ++i)
	{
		copyover_room e = {};
		if (!read_fields(file, record_type::door, &e))
			return fail(error, "invalid door frame");
		state->doors.push_back(e);
	}
	return file.done() || fail(error, "trailing bytes or inconsistent record counts");
}

template <typename T> bool read_legacy_value(wire &file, T *value, size_t size = sizeof(T))
{
	if (size > sizeof(T))
		return false;
	file.raw(value, size);
	return file.ok;
}

bool read_legacy(wire &file, copyover_decoded_state *state, const char **error)
{
	const int version = state->header.version;
	for (int i = 0; i < state->header.num_descriptors; ++i)
	{
		copyover_desc e = {};
		if (!read_legacy_value(file, &e, version < 17 ? 660 : 680))
			return fail(error, "truncated legacy descriptor");
		state->descriptors.push_back(e);
	}
	if (version >= 15)
	{
		unsigned char header[12] = {};
		file.raw(header, 12);
		if (!file.ok || memcmp(header, "TLMY", 4) || get_unsigned(header + 4, 4) != 1 ||
		    get_unsigned(header + 8, 4) !=
			    static_cast<uint32_t>(state->header.num_descriptors) ||
		    static_cast<size_t>(state->header.num_descriptors) > file.remaining / 200)
			return fail(error, "invalid legacy telemetry framing");
		bool retain_telemetry = true;
		for (int i = 0; i < state->header.num_descriptors; ++i)
		{
			telemetry_copyover_entry e = {};
			if (!read_legacy_value(file, &e))
				return fail(error, "truncated legacy telemetry");
			if (retain_telemetry)
				try
				{
					state->telemetry.push_back(e);
				}
				catch (const std::bad_alloc &)
				{
					state->telemetry.clear();
					retain_telemetry = false;
				}
		}
	}
	for (int i = 0; i < state->header.num_mobs; ++i)
	{
		copyover_decoded_mob mob;
		if (!read_legacy_value(file, &mob.entry,
				       version == 12 ? 288 :
				       version < 16  ? 356 :
						       360))
			return fail(error, "truncated legacy mob");
		if (version < 16)
			mob.entry.shopkeeper_shop_id = -1;
		if (!valid_mob(mob.entry) ||
		    static_cast<uint64_t>(mob.entry.num_affects) * 64 +
				    static_cast<uint64_t>(mob.entry.num_carrying) * 16 >
			    file.remaining)
			return fail(error, "invalid legacy mob child counts");
		mob.affects.resize(static_cast<size_t>(mob.entry.num_affects));
		mob.inventory.resize(static_cast<size_t>(mob.entry.num_carrying));
		for (auto &e : mob.affects)
			if (!read_legacy_value(file, &e))
				return fail(error, "truncated legacy affect");
		for (auto &e : mob.inventory)
			if (!read_legacy_value(file, &e))
				return fail(error, "truncated legacy inventory");
		if (version >= 14)
		{
			if (file.remaining < 8 || memcmp(file.input, "GNP1", 4))
				return fail(error, "invalid legacy generated NPC framing");
			const uint64_t size = get_unsigned(file.input + 4, 4) + 8;
			if (size > file.remaining || size > GENERATED_NPC_EXTENSION_MAX_BYTES ||
			    !generated_npc_extension_decode(
				    mob.entry.vnum, reinterpret_cast<const char *>(file.input),
				    static_cast<size_t>(size), &mob.generated))
				return fail(error, "invalid legacy generated NPC state");
			file.input += size;
			file.remaining -= static_cast<size_t>(size);
		}
		state->mobs.push_back(std::move(mob));
	}
	for (int i = 0; i < state->header.num_objects; ++i)
	{
		uint32_t size = 0;
		if (!read_legacy_value(file, &size) || size > COPYOVER_MAX_RECORD_BYTES ||
		    size < 8 || size > file.remaining)
			return fail(error, "invalid legacy object length");
		const uint64_t count = get_unsigned(file.input + 4, 4);
		if (!count || count > WORLD_RECOVERY_MAX_ITEM_TREE || 8 + count * 3328 > size)
			return fail(error, "invalid legacy object count");
		state->objects.emplace_back(reinterpret_cast<const char *>(file.input),
					    reinterpret_cast<const char *>(file.input + size));
		file.input += size;
		file.remaining -= size;
	}
	for (int i = 0; i < state->header.num_rooms; ++i)
	{
		copyover_room e = {};
		if (!read_legacy_value(file, &e))
			return fail(error, "truncated legacy door");
		state->doors.push_back(e);
	}
	return file.done() || fail(error, "trailing legacy bytes or incompatible ABI");
}

bool validate_state(copyover_decoded_state *state, const char **error)
{
	std::set<int> fds;
	std::set<std::string> names;
	for (int fd : state->listeners)
		if (fd < -1 || (fd >= 0 && !fds.insert(fd).second))
			return fail(error, "invalid or duplicate listener descriptor");
	for (const auto &e : state->descriptors)
		if (!valid_desc(e) || !fds.insert(e.fd).second ||
		    !names.insert(e.player_name).second)
			return fail(error, "invalid descriptor identity or death retry state");
	// Metadata remains optional. Invalid legacy handoffs resume as absent, just as
	// before; portable framing/CRC errors reject the entire file.
	auto &telemetry = state->telemetry;
	telemetry.erase(std::remove_if(telemetry.begin(), telemetry.end(),
				       [](const auto &e)
				       {
					       if (e.fd <= 0 || !memchr(e.player_name, 0, 50) ||
						   e.handoff_valid > 1 || e.reserved[0] ||
						   e.reserved[1] || e.reserved[2])
						       return true;
					       if (!e.handoff_valid)
					       {
						       const auto &h = e.handoff;
						       return h.session.id.producer.boot_id ||
							      h.session.id.producer.process_id ||
							      h.session.id.session_seq ||
							      h.session.subject_id ||
							      h.session.pid ||
							      h.session.season_id ||
							      h.session.environment_id ||
							      h.previous_producer.boot_id ||
							      h.previous_producer.process_id ||
							      h.last_checkpoint_revision ||
							      h.cumulative.connected_usec ||
							      h.cumulative.active_usec ||
							      h.cumulative.idle_usec ||
							      h.cumulative.unknown_usec ||
							      h.cumulative.resident_usec ||
							      h.cumulative.linkdead_usec ||
							      h.quality_flags;
					       }
					       return false;
				       }),
			telemetry.end());
	std::set<uint64_t> item_uids;
	for (const auto &object : state->objects)
		if (!valid_object(object, &item_uids))
			return fail(error, "invalid object tree, values or custody handoff");
	for (const auto &door : state->doors)
		if (door.vnum < 0 || door.dir < 0 || door.dir >= NUM_EXITS)
			return fail(error, "invalid door direction or room");
	return true;
}

bool decode(const std::vector<unsigned char> &bytes, copyover_decoded_state *output,
	    const char **error)
{
	if (!output || bytes.size() < 8)
		return fail(error, "truncated header");
	copyover_decoded_state state;
	wire file(bytes.data(), bytes.size());
	const bool portable = memcmp(bytes.data(), MAGIC, 4) == 0;
	if (portable)
	{
		if (get_unsigned(bytes.data() + 4, 4) != COPYOVER_VERSION)
			return fail(error, "unsupported portable version");
		if (bytes.size() < COPYOVER_WIRE_HEADER_BYTES)
			return fail(error, "truncated portable header");
		if (get_unsigned(bytes.data() + 8, 4) != COPYOVER_WIRE_HEADER_BYTES ||
		    get_unsigned(bytes.data() + 12, 4) != WIRE_BYTE_ORDER ||
		    get_unsigned(bytes.data() + 24, 8) != bytes.size() - COPYOVER_WIRE_HEADER_BYTES)
			return fail(error, "invalid byte order, header or payload length");
		if (get_unsigned(bytes.data() + CHECKSUM_OFFSET, 4) !=
		    checksum(bytes.data(), bytes.size()))
			return fail(error, "checksum mismatch");
		memcpy(state.header.magic, COPYOVER_MAGIC, 4);
		state.header.version = COPYOVER_VERSION;
		wire header(bytes.data() + 16, 8);
		header.number(state.header.timestamp, 8, true);
		if (!header.done())
			return fail(error, "timestamp outside host time_t range");
		header = wire(bytes.data() + 36, 28);
		for (int *count : { &state.header.num_descriptors, &state.header.num_mobs,
				    &state.header.num_objects, &state.header.num_rooms })
			header.number(*count, 4);
		for (int &fd : state.listeners)
			header.number(fd, 4, true);
		if (!header.done())
			return fail(error, "counts outside supported range");
		file = wire(bytes.data() + COPYOVER_WIRE_HEADER_BYTES,
			    bytes.size() - COPYOVER_WIRE_HEADER_BYTES);
	}
	else
	{
		if (memcmp(bytes.data(), COPYOVER_MAGIC, 4))
			return fail(error, "unrecognized copyover magic");
		const uint64_t version = get_unsigned(bytes.data() + 4, 4);
		if (version < 12 || version > 17)
			return fail(error, "unsupported legacy version or byte order");
		if (!copyover_codec_legacy_abi_compatible())
			return fail(error,
				    "legacy ABI is incompatible; use the original binary/host");
		if (!read_legacy_value(file, &state.header) ||
		    !read_legacy_value(file, &state.listeners))
			return fail(error, "truncated legacy header/listeners");
	}
	if (!valid_counts(state.header, file.remaining, portable))
		return fail(error, "impossible counts or unsupported combat/zone sections");
	if (!(portable ? read_portable(file, &state, error) : read_legacy(file, &state, error)) ||
	    !validate_state(&state, error))
		return false;
	*output = std::move(state);
	return true;
}

bool read_bytes(FILE *file, std::vector<unsigned char> *bytes, const char **error)
{
	if (!file || !bytes || fseek(file, 0, SEEK_END) != 0)
		return fail(error, "cannot seek copyover file");
	const long size = ftell(file);
	if (size < 8 || static_cast<uint64_t>(size) > COPYOVER_MAX_FILE_BYTES)
		return fail(error, "truncated or oversized copyover file");
	if (fseek(file, 0, SEEK_SET) != 0)
		return fail(error, "cannot rewind copyover file");
	bytes->resize(static_cast<size_t>(size));
	if (fread(bytes->data(), 1, bytes->size(), file) != bytes->size() || fgetc(file) != EOF ||
	    ferror(file))
		return fail(error, "file read failed or size changed");
	return true;
}
} // namespace

bool copyover_codec_legacy_abi_compatible()
{
	// Versions 12-17 did not record an ABI tag. Only accept the established
	// little-endian LP64 layout, including every nested native object layout.
	return std::endian::native == std::endian::little && sizeof(int) == 4 &&
	       sizeof(time_t) == 8 && std::numeric_limits<time_t>::is_signed &&
	       sizeof(unsigned long) == 8 && sizeof(sh_int) == 2 && sizeof(copyover_header) == 40 &&
	       offsetof(copyover_header, timestamp) == 8 &&
	       offsetof(copyover_header, num_descriptors) == 16 && sizeof(copyover_desc) == 680 &&
	       offsetof(copyover_desc, gmcp_enabled) == 360 &&
	       offsetof(copyover_desc, num_pets) == 536 &&
	       offsetof(copyover_desc, death_retry_pending) == 660 &&
	       offsetof(copyover_desc, death_retry_corpse_uid) == 672 &&
	       sizeof(copyover_mob) == 360 && offsetof(copyover_mob, num_affects) == 100 &&
	       offsetof(copyover_mob, transport) == 288 &&
	       offsetof(copyover_mob, shopkeeper_shop_id) == 356 &&
	       sizeof(transport_snapshot) == 68 && sizeof(copyover_affect) == 64 &&
	       offsetof(copyover_affect, bitvector) == 24 && sizeof(copyover_carried_item) == 16 &&
	       offsetof(copyover_carried_item, vnum) == 8 && sizeof(copyover_room) == 12 &&
	       sizeof(world_recovery_object_record) == 8 &&
	       sizeof(world_recovery_item_snapshot) == 3328 &&
	       offsetof(world_recovery_item_snapshot, timers) == 72 &&
	       offsetof(world_recovery_item_snapshot, name) == 120 &&
	       offsetof(world_recovery_item_snapshot, bitvector) == 3256 &&
	       sizeof(item_ownership_runtime_entry) == 72 &&
	       offsetof(item_ownership_runtime_entry, owner) == 24 &&
	       offsetof(item_owner_identity, id) == 8 &&
	       offsetof(item_ownership_runtime_entry, item_revision) == 48 &&
	       sizeof(telemetry_copyover_entry) == 200 &&
	       offsetof(telemetry_copyover_entry, handoff) == 64 &&
	       sizeof(telemetry_session_handoff) == 136 &&
	       offsetof(telemetry_session_ref, season_id) == 40 &&
	       offsetof(telemetry_session_handoff, cumulative) == 80 && MAX_WEAR == 43 &&
	       MAX_OBJ_AFFECT == 4;
}

bool copyover_codec_begin(FILE *file, const copyover_header &header, const int listeners[3])
{
	if (!file || !listeners || !valid_counts(header, COPYOVER_MAX_FILE_BYTES, true) ||
	    !std::in_range<int64_t>(header.timestamp))
		return false;
	unsigned char bytes[COPYOVER_WIRE_HEADER_BYTES] = {};
	memcpy(bytes, MAGIC, 4);
	put_unsigned(bytes + 4, COPYOVER_VERSION, 4);
	put_unsigned(bytes + 8, COPYOVER_WIRE_HEADER_BYTES, 4);
	put_unsigned(bytes + 12, WIRE_BYTE_ORDER, 4);
	put_unsigned(bytes + 16, static_cast<uint64_t>(header.timestamp), 8);
	size_t offset = 36;
	for (int count :
	     { header.num_descriptors, header.num_mobs, header.num_objects, header.num_rooms })
	{
		put_unsigned(bytes + offset, static_cast<uint32_t>(count), 4);
		offset += 4;
	}
	for (size_t i = 0; i < 3; ++i)
		put_unsigned(bytes + 52 + i * 4, static_cast<uint32_t>(listeners[i]), 4);
	return fwrite(bytes, 1, COPYOVER_WIRE_HEADER_BYTES, file) == COPYOVER_WIRE_HEADER_BYTES;
}

bool copyover_codec_write(FILE *file, const copyover_desc &e)
{
	return valid_desc(e) && write_fields(file, record_type::descriptor, e);
}
bool copyover_codec_write(FILE *file, const telemetry_copyover_entry &e)
{
	return write_fields(file, record_type::telemetry, e);
}
bool copyover_codec_write(FILE *file, const copyover_mob &e)
{
	return valid_mob(e) && write_fields(file, record_type::mob, e);
}
bool copyover_codec_write(FILE *file, const copyover_affect &e)
{
	return write_fields(file, record_type::affect, e);
}
bool copyover_codec_write(FILE *file, const copyover_carried_item &e)
{
	return write_fields(file, record_type::carried, e);
}
bool copyover_codec_write(FILE *file, const copyover_room &e)
{
	return write_fields(file, record_type::door, e);
}

bool copyover_codec_write_generated(FILE *file, int vnum, const std::string &generated)
{
	std::string extension;
	return generated_npc_extension_encode(vnum, generated, &extension) &&
	       write_record(file, record_type::generated,
			    reinterpret_cast<const unsigned char *>(extension.data()),
			    extension.size());
}

bool copyover_codec_write_object(FILE *file, const char *native_data, size_t native_size)
{
	if (!native_data || native_size < sizeof(world_recovery_object_record) ||
	    native_size > WORLD_RECOVERY_MAX_RECORD_BYTES)
		return false;
	try
	{
		world_recovery_object_record record = {};
		memcpy(&record, native_data, sizeof(record));
		if (!record.item_count || record.item_count > WORLD_RECOVERY_MAX_ITEM_TREE)
			return false;
		const size_t tree_size =
			sizeof(record) + static_cast<size_t>(record.item_count) *
						 sizeof(world_recovery_item_snapshot);
		if (tree_size > native_size ||
		    (native_size - tree_size) % sizeof(item_ownership_runtime_entry))
			return false;
		const size_t count =
			(native_size - tree_size) / sizeof(item_ownership_runtime_entry);
		if (count > record.item_count)
			return false;
		const size_t wire_size =
			8 + static_cast<size_t>(record.item_count) * WORLD_RECOVERY_WIRE_ITEM_BYTES;
		wire w;
		uint32_t length = static_cast<uint32_t>(wire_size);
		w.number(length, 4);
		w.bytes.resize(4 + wire_size);
		size_t written = 0;
		if (!world_recovery_encode_record(
			    world_recovery_record_type::object,
			    reinterpret_cast<const unsigned char *>(native_data), tree_size,
			    w.bytes.data() + 4, wire_size, &written) ||
		    written != wire_size)
			return false;
		uint32_t custody_count = static_cast<uint32_t>(count);
		w.number(custody_count, 4);
		for (size_t i = 0; i < count; ++i)
		{
			item_ownership_runtime_entry entry = {};
			memcpy(&entry, native_data + tree_size + i * sizeof(entry), sizeof(entry));
			fields(w, entry);
		}
		return w.done() &&
		       write_record(file, record_type::object, w.bytes.data(), w.bytes.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool copyover_codec_read(FILE *file, copyover_decoded_state *output, const char **error)
{
	if (error)
		*error = nullptr;
	try
	{
		std::vector<unsigned char> bytes;
		return read_bytes(file, &bytes, error) && decode(bytes, output, error);
	}
	catch (const std::bad_alloc &)
	{
		return fail(error, "copyover validation allocation failed");
	}
}

bool copyover_codec_finish(FILE *file, const char **error)
{
	try
	{
		std::vector<unsigned char> bytes;
		if (!file || fflush(file) != 0 || !read_bytes(file, &bytes, error) ||
		    bytes.size() < COPYOVER_WIRE_HEADER_BYTES || memcmp(bytes.data(), MAGIC, 4))
			return fail(error, "cannot finish temporary copyover file");
		put_unsigned(bytes.data() + 24, bytes.size() - COPYOVER_WIRE_HEADER_BYTES, 8);
		put_unsigned(bytes.data() + CHECKSUM_OFFSET, checksum(bytes.data(), bytes.size()),
			     4);
		copyover_decoded_state checked;
		if (!decode(bytes, &checked, error))
			return false;
		if (fseek(file, 0, SEEK_SET) != 0 ||
		    fwrite(bytes.data(), 1, COPYOVER_WIRE_HEADER_BYTES, file) !=
			    COPYOVER_WIRE_HEADER_BYTES ||
		    fflush(file) != 0 || fsync(fileno(file)) != 0)
			return fail(error, "copyover file flush/fsync failed");
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return fail(error, "copyover sealing allocation failed");
	}
}

bool copyover_codec_sync_parent(const char *path)
try
{
	if (!path)
		return false;
	std::string directory(path);
	const size_t slash = directory.find_last_of('/');
	directory = slash == std::string::npos ? "." :
		    slash == 0		       ? "/" :
						 directory.substr(0, slash);
	const int fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd < 0)
		return false;
	const bool ok = fsync(fd) == 0;
	close(fd);
	return ok;
}
catch (const std::bad_alloc &)
{
	return false;
}
