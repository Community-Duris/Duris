#include "persistence/copyover_codec.h"
#include "item/item_ownership_runtime.h"
#include "world/world_recovery_pipeline.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static bool fail_allocation = false;
static bool fail_telemetry_allocation = false;
static bool fail_sync = false;
void *operator new(size_t size)
{
	if (fail_allocation ||
	    (fail_telemetry_allocation && size == sizeof(telemetry_copyover_entry)))
	{
		fail_allocation = false;
		fail_telemetry_allocation = false;
		throw std::bad_alloc();
	}
	if (void *p = malloc(size ? size : 1))
		return p;
	throw std::bad_alloc();
}
void operator delete(void *p) noexcept
{
	free(p);
}
void operator delete(void *p, size_t) noexcept
{
	free(p);
}

extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
	if (fail_sync)
	{
		errno = EIO;
		return -1;
	}
	return __real_fsync(fd);
}

static void write_fixture(FILE *file, bool door_only)
{
	copyover_header h = {};
	h.timestamp = -123456789;
	h.num_descriptors = h.num_mobs = h.num_objects = door_only ? 0 : 1;
	h.num_rooms = 1;
	const int listeners[3] = { -1, 7, 8 };
	assert(copyover_codec_begin(file, h, listeners));
	if (!door_only)
	{
		copyover_desc d = {};
		d.fd = 9;
		strcpy(d.player_name, "synthetic");
		strcpy(d.host, "127.0.0.1");
		strcpy(d.host2, "fixture.invalid");
		d.term_type = -128;
		d.gmcp_enabled = 1;
		d.out_compress = 2;
		d.room = -1;
		d.mtts_flags = -1;
		d.charset_detected = -1;
		strcpy(d.ttype_client, "fixture");
		strcpy(d.ttype_terminal, "terminal");
		d.fighting_type = 2;
		d.fighting_id = -1;
		strcpy(d.fighting_name, "target");
		d.num_pets = 1;
		for (int i = 0; i < 10; ++i)
		{
			d.pet_vnums[i] = 1000 + i;
			d.pet_hit[i] = -i;
			d.pet_max_hit[i] = 100 + i;
		}
		d.death_retry_pending = 1;
		d.death_retry_delay = 60;
		d.death_retry_corpse_uid = UINT64_MAX;
		assert(copyover_codec_write(file, d));
		telemetry_copyover_entry t = {};
		t.fd = 9;
		strcpy(t.player_name, "synthetic");
		t.handoff_valid = 1;
		t.handoff.session.id.producer = { 1, 2 };
		t.handoff.session.id.session_seq = 3;
		t.handoff.session.subject_id = UINT64_MAX;
		t.handoff.session.pid = -1;
		t.handoff.session.season_id = 4;
		t.handoff.session.environment_id = 5;
		t.handoff.previous_producer = { 6, 7 };
		t.handoff.last_checkpoint_revision = 8;
		t.handoff.cumulative = { 9, 10, 11, 12, 13, UINT64_MAX };
		t.handoff.quality_flags = UINT32_MAX;
		assert(copyover_codec_write(file, t));
		copyover_mob m = {};
		m.vnum = 1001;
		m.idnum = -1;
		m.room = 200;
		m.hit = -2;
		m.max_hit = INT32_MAX;
		m.mana = INT32_MIN;
		m.max_mana = 200;
		m.vitality = -3;
		m.max_vitality = 300;
		m.position = 10;
		m.fighting_type = 1;
		m.fighting_id = -1;
		strcpy(m.fighting_name, "synthetic");
		m.num_affects = m.num_carrying = 1;
		for (int &vnum : m.equipment_vnums)
			vnum = -1;
		m.equipment_vnums[42] = 1000;
		m.gold = -99;
		m.birthplace = -1;
		m.transport = { 200, 300, 2, -1, "synthetic" };
		m.shopkeeper_shop_id = -1;
		assert(copyover_codec_write(file, m));
		copyover_affect a = {};
		a.type = INT16_MIN;
		a.wear_off_message_index = -1;
		a.duration = -1;
		a.flags = UINT32_MAX;
		a.modifier = INT32_MIN;
		a.location = a.loc2 = UINT8_MAX;
		a.level = UINT16_MAX;
		a.bitvector = UINT64_MAX;
		a.bitvector2 = UINT64_C(0x8000000000000000);
		a.bitvector3 = 3;
		a.bitvector4 = 4;
		a.bitvector5 = 5;
		assert(copyover_codec_write(file, a));
		copyover_carried_item carried = {};
		carried.vnum = 1000;
		assert(copyover_codec_write(file, carried));
		assert(copyover_codec_write_generated(file, 1001, ""));
		world_recovery_object_record r = { 200, 1 };
		world_recovery_item_snapshot item = {};
		item.item_uid = item.root_item_uid = UINT64_MAX;
		item.vnum = 1000;
		item.flags = WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED;
		for (int i = 0; i < 8; ++i)
			item.values[i] = -i;
		item.timers[0] = -1;
		item.timers[1] = INT64_MIN;
		item.timers[2] = INT64_MAX;
		memset(item.name, 'x', 512); // Exact SQL maximum plus the terminator.
		strcpy(item.short_description, "a synthetic item");
		strcpy(item.description, "Synthetic fixture.");
		strcpy(item.action_description, "fixture action");
		item.wear_flags = UINT32_MAX;
		item.weight = -2;
		item.material = 3;
		item.trap_eff = INT16_MIN;
		item.condition = INT16_MAX;
		item.bitvector = UINT64_MAX;
		item.affect_locations[0] = 127;
		item.affect_modifiers[0] = -128;
		item_ownership_runtime_entry custody = {};
		custody.item_uid = custody.root_item_uid = UINT64_MAX;
		custody.owner = { item_owner_type::corpse, 77, 12 };
		custody.item_revision = UINT64_MAX;
		custody.owner_revision = 31;
		custody.vnum = 1000;
		custody.state = item_custody_state::active;
		std::vector<char> native(sizeof(r) + sizeof(item) + sizeof(custody));
		memcpy(native.data(), &r, sizeof(r));
		memcpy(native.data() + sizeof(r), &item, sizeof(item));
		memcpy(native.data() + sizeof(r) + sizeof(item), &custody, sizeof(custody));
		assert(copyover_codec_write_object(file, native.data(), native.size()));
	}
	copyover_room door = { 200, 9, -1 };
	assert(copyover_codec_write(file, door));
}

static void check_fixture(const copyover_decoded_state &s, bool legacy)
{
	assert(s.header.timestamp == -123456789);
	assert(s.listeners[0] == -1 && s.listeners[1] == 7 && s.listeners[2] == 8);
	assert(s.descriptors.size() == 1 &&
	       s.telemetry.size() == (s.header.version >= 15 ? 1U : 0U));
	const auto &d = s.descriptors[0];
	assert(d.fd == 9 && !strcmp(d.player_name, "synthetic") && d.term_type == -128);
	assert(d.room == -1 && d.pet_vnums[9] == 1009 && d.pet_hit[9] == -9);
	assert(d.death_retry_pending == (s.header.version >= 17));
	if (d.death_retry_pending)
		assert(d.death_retry_delay == 60 && d.death_retry_corpse_uid == UINT64_MAX);
	assert(s.mobs.size() == 1 && s.objects.size() == 1 && s.doors.size() == 1);
	const auto &m = s.mobs[0];
	assert(m.entry.vnum == 1001 && m.entry.hit == -2 && m.entry.mana == INT32_MIN);
	assert(m.entry.max_hit == INT32_MAX && m.entry.equipment_vnums[42] == 1000);
	assert(m.entry.shopkeeper_shop_id == -1);
	assert(m.entry.transport.origin == (s.header.version >= 13 ? 200 : 0));
	assert(m.affects.size() == 1 && m.inventory.size() == 1 && m.generated.empty());
	assert(m.affects[0].duration == -1 && m.affects[0].modifier == INT32_MIN);
	assert(m.affects[0].bitvector == UINT64_MAX && m.affects[0].level == UINT16_MAX);
	assert(m.inventory[0].vnum == 1000);
	world_recovery_item_snapshot item = {};
	memcpy(&item, s.objects[0].data() + sizeof(world_recovery_object_record), sizeof(item));
	assert(item.item_uid == UINT64_MAX && strlen(item.name) == 512);
	assert(item.timers[1] == INT64_MIN && item.timers[2] == INT64_MAX);
	assert(item.affect_modifiers[0] == -128 && item.bitvector == UINT64_MAX);
	item_ownership_runtime_entry custody = {};
	memcpy(&custody, s.objects[0].data() + sizeof(world_recovery_object_record) + sizeof(item),
	       sizeof(custody));
	assert(custody.item_revision == UINT64_MAX && custody.owner.id == 77);
	assert(custody.root_item_uid == UINT64_MAX && custody.state == item_custody_state::active);
	assert(s.doors[0].dir == 9 && s.doors[0].state == -1);
	assert(legacy == (s.header.version < 18));
}

int main(int argc, char **argv)
{
	assert(argc >= 2);
	const std::string action(argv[1]);
	const char *error = nullptr;
	if (action == "write" || action == "door")
	{
		assert(argc == 3);
		FILE *file = fopen(argv[2], "w+b");
		assert(file);
		write_fixture(file, action == "door");
		assert(copyover_codec_finish(file, &error));
		assert(fclose(file) == 0);
	}
	else if (action == "read" || action == "legacy" || action == "reject")
	{
		assert(argc >= 3);
		for (int i = 2; i < argc; ++i)
		{
			FILE *file = fopen(argv[i], "rb");
			assert(file);
			copyover_decoded_state state;
			state.header.version = 999; // Failed decodes must leave output unchanged.
			if (action == "reject")
			{
				assert(!copyover_codec_read(file, &state, &error));
				assert(error && *error && state.header.version == 999 &&
				       state.objects.empty());
			}
			else
			{
				if (!copyover_codec_read(file, &state, &error))
				{
					fprintf(stderr, "%s: %s\n", argv[i], error);
					abort();
				}
				check_fixture(state, action == "legacy");
			}
			fclose(file);
		}
	}
	else if (action == "high-fds")
	{
		// poll() admits sockets above the old select() bitmap limit.
		FILE *file = tmpfile();
		assert(file);
		copyover_header header = {};
		header.num_descriptors = 1;
		const int listeners[3] = { 4096, 4097, 4098 };
		assert(copyover_codec_begin(file, header, listeners));
		copyover_desc descriptor = {};
		descriptor.fd = 4099;
		strcpy(descriptor.player_name, "synthetic");
		assert(copyover_codec_write(file, descriptor));
		telemetry_copyover_entry telemetry = {};
		telemetry.fd = descriptor.fd;
		strcpy(telemetry.player_name, descriptor.player_name);
		assert(copyover_codec_write(file, telemetry));
		assert(copyover_codec_finish(file, &error));
		copyover_decoded_state state;
		assert(copyover_codec_read(file, &state, &error));
		for (size_t i = 0; i < 3; ++i)
			assert(state.listeners[i] == listeners[i]);
		assert(state.descriptors.size() == 1 && state.descriptors[0].fd == descriptor.fd);
		assert(state.telemetry.size() == 1 && state.telemetry[0].fd == descriptor.fd);
		fclose(file);
	}
	else if (action == "children")
	{
		FILE *file = tmpfile();
		assert(file);
		copyover_header header = {};
		header.num_mobs = 1;
		const int listeners[3] = { -1, -1, -1 };
		assert(copyover_codec_begin(file, header, listeners));
		copyover_mob mob = {};
		mob.vnum = 1001;
		mob.room = 200;
		mob.shopkeeper_shop_id = -1;
		mob.num_affects = 65;
		mob.num_carrying = 257;
		assert(copyover_codec_write(file, mob));
		for (int i = 0; i < mob.num_affects; ++i)
		{
			copyover_affect affect = {};
			affect.duration = -1;
			affect.modifier = i;
			assert(copyover_codec_write(file, affect));
		}
		for (int i = 0; i < mob.num_carrying; ++i)
		{
			copyover_carried_item carried = {};
			carried.vnum = 1000 + i;
			assert(copyover_codec_write(file, carried));
		}
		assert(copyover_codec_write_generated(file, mob.vnum, ""));
		assert(copyover_codec_finish(file, &error));
		copyover_decoded_state state;
		assert(copyover_codec_read(file, &state, &error));
		assert(state.mobs.size() == 1 && state.mobs[0].affects.size() == 65 &&
		       state.mobs[0].inventory.size() == 257);
		assert(state.mobs[0].affects.back().modifier == 64 &&
		       state.mobs[0].affects.back().duration == -1 &&
		       state.mobs[0].inventory.back().vnum == 1256);
		fclose(file);
	}
	else if (action == "failures")
	{
		FILE *file = tmpfile();
		assert(file);
		write_fixture(file, false);
		fail_sync = true;
		assert(!copyover_codec_finish(file, &error));
		fail_sync = false;
		assert(copyover_codec_finish(file, &error));
		copyover_decoded_state state;
		state.header.version = 999;
		fail_allocation = true;
		assert(!copyover_codec_read(file, &state, &error) && !fail_allocation);
		assert(state.header.version == 999);
		assert(copyover_codec_read(file, &state, &error));
		check_fixture(state, false);
		fail_telemetry_allocation = true;
		assert(copyover_codec_read(file, &state, &error) && !fail_telemetry_allocation);
		assert(state.telemetry.empty() && state.objects.size() == 1 &&
		       state.mobs.size() == 1);
		fclose(file);
		fail_sync = true;
		assert(!copyover_codec_sync_parent("synthetic.dat"));
		fail_sync = false;
		assert(copyover_codec_sync_parent("synthetic.dat"));
		assert(!copyover_codec_sync_parent("missing-directory/synthetic.dat"));
	}
	else if (action == "sweep")
	{
		FILE *source = fopen(argv[2], "rb");
		assert(source);
		assert(fseek(source, 0, SEEK_END) == 0);
		const size_t size = static_cast<size_t>(ftell(source));
		rewind(source);
		std::vector<unsigned char> bytes(size);
		assert(fread(bytes.data(), 1, size, source) == size);
		fclose(source);
		for (size_t i = 0; i < size; ++i)
		{
			for (bool flip : { false, true })
			{
				FILE *file = tmpfile();
				assert(file);
				if (flip)
					bytes[i] ^= 0x80;
				const size_t length = flip ? size : i;
				assert(fwrite(bytes.data(), 1, length, file) == length);
				copyover_decoded_state state;
				assert(!copyover_codec_read(file, &state, &error));
				if (flip)
					bytes[i] ^= 0x80;
				fclose(file);
			}
		}
	}
	else
		assert(false);
	puts("portable copyover codec fixture passed");
}
