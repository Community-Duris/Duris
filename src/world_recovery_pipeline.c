#include "world_recovery_pipeline.h"

#include "copyover.h"
#include "db.h"
#include "item_ownership_runtime.h"
#include "prototypes.h"
#include "redis.h"
#include "ships/ships.h"
#include "utils.h"
#include "world_recovery_codec.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <zlib.h>

extern struct zone_data *zone_table;
extern struct room_data *world;
extern P_char character_list;
extern P_obj object_list;
extern int top_of_world;
extern int top_of_zone_table;
extern bool sql_persistence_reconcile_world_recovery_items(
	const world_recovery_authority_item *items, size_t count,
	item_ownership_runtime_entry *authoritative, size_t authoritative_capacity);
namespace
{
enum class capture_stage : uint8_t
{
	idle,
	mobs,
	objects,
	doors,
	zones,
};

struct record_header
{
	uint32_t size;
	uint8_t type;
	uint8_t reserved[3];
};

struct recovery_generation
{
	uint64_t sequence = 0;
	int64_t timestamp = 0;
	uint32_t mob_count = 0;
	uint32_t object_count = 0;
	uint32_t door_count = 0;
	uint32_t zone_count = 0;
	std::vector<unsigned char> blob;
};

struct capture_state
{
	capture_stage stage = capture_stage::idle;
	recovery_generation generation;
	P_char next_character = nullptr;
	P_obj next_object = nullptr;
	int room = 0;
	int direction = 0;
	int zone = 0;
	std::chrono::steady_clock::time_point started = {};
};

std::mutex recovery_mutex;
std::condition_variable generation_available;
std::deque<recovery_generation> queued;
std::deque<world_recovery_completion> completions;
std::thread publisher_worker;
world_recovery_publish_fn publish_callback = nullptr;
void *publish_context = nullptr;
world_recovery_health health = {};
capture_state active_capture;
uint64_t next_sequence = 1;
bool stop_requested = false;
bool worker_busy = false;
bool capture_failure_pending = false;
world_recovery_completion capture_failure_completion = {};
std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> capture_buffer = {};

struct planned_mob
{
	std::vector<unsigned char> record;
	int room_rnum = NOWHERE;
};

struct planned_object
{
	int room_rnum = NOWHERE;
	int room_vnum = 0;
	std::vector<world_recovery_item_snapshot> items;
	P_obj existing_root = nullptr;
};

struct recovery_plan
{
	world_recovery_header header = {};
	std::vector<planned_mob> mobs;
	std::vector<planned_object> objects;
	std::vector<copyover_room> doors;
	std::vector<zone_age_entry> zones;
	std::vector<world_recovery_authority_item> authority_items;
	std::unordered_set<uint64_t> item_uids;
};

uint64_t elapsed_msec(std::chrono::steady_clock::time_point started)
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
					     std::chrono::steady_clock::now() - started)
					     .count());
}

bool append_record(recovery_generation &generation, world_recovery_record_type type,
		   const void *data, size_t size)
{
	if (!data || !size || size > WORLD_RECOVERY_MAX_RECORD_BYTES)
		return false;
	const size_t added = sizeof(record_header) + size;
	if (generation.blob.size() > WORLD_RECOVERY_MAX_BYTES - added)
		return false;
	try
	{
		const size_t offset = generation.blob.size();
		generation.blob.resize(offset + added);
		record_header header = {};
		header.size = static_cast<uint32_t>(size);
		header.type = static_cast<uint8_t>(type);
		memcpy(generation.blob.data() + offset, &header, sizeof(header));
		memcpy(generation.blob.data() + offset + sizeof(header), data, size);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_generation(recovery_generation *generation, world_recovery_header *header)
{
	if (!generation || !header || generation->blob.size() < sizeof(world_recovery_header))
		return false;
	size_t source_offset = sizeof(world_recovery_header);
	size_t destination_offset = WORLD_RECOVERY_WIRE_HEADER_BYTES;
	std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> encoded = {};
	while (source_offset < generation->blob.size())
	{
		if (generation->blob.size() - source_offset < sizeof(record_header))
			return false;
		record_header native_header = {};
		memcpy(&native_header, generation->blob.data() + source_offset,
		       sizeof(native_header));
		source_offset += sizeof(native_header);
		if (!native_header.size || native_header.size > WORLD_RECOVERY_MAX_RECORD_BYTES ||
		    native_header.size > generation->blob.size() - source_offset)
			return false;
		size_t encoded_size = 0;
		const auto type = static_cast<world_recovery_record_type>(native_header.type);
		if (!world_recovery_encode_record(type, generation->blob.data() + source_offset,
						  native_header.size, encoded.data(),
						  encoded.size(), &encoded_size) ||
		    encoded_size > UINT32_MAX ||
		    destination_offset + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES + encoded_size >
			    source_offset + native_header.size ||
		    destination_offset >
			    generation->blob.size() - WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES ||
		    encoded_size > generation->blob.size() - destination_offset -
					   WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES)
			return false;
		if (!world_recovery_encode_record_header(
			    type, static_cast<uint32_t>(encoded_size),
			    generation->blob.data() + destination_offset,
			    generation->blob.size() - destination_offset))
			return false;
		destination_offset += WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES;
		memcpy(generation->blob.data() + destination_offset, encoded.data(), encoded_size);
		destination_offset += encoded_size;
		source_offset += native_header.size;
	}
	generation->blob.resize(destination_offset);
	memcpy(header->magic, "WRS9", 4);
	header->schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
	header->header_size = WORLD_RECOVERY_WIRE_HEADER_BYTES;
	header->sequence = generation->sequence;
	header->timestamp = generation->timestamp;
	header->payload_size = generation->blob.size() - WORLD_RECOVERY_WIRE_HEADER_BYTES;
	header->checksum = crc32(0, generation->blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES,
				 generation->blob.size() - WORLD_RECOVERY_WIRE_HEADER_BYTES);
	header->mob_count = generation->mob_count;
	header->object_count = generation->object_count;
	header->door_count = generation->door_count;
	header->zone_count = generation->zone_count;
	header->complete = 1;
	return world_recovery_encode_header(header, generation->blob.data(),
					    generation->blob.size());
}

void fail_capture(bool expired)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	capture_failure_completion = { active_capture.generation.sequence, false, 0 };
	capture_failure_pending = true;
	++health.capture_failures;
	if (expired)
		++health.capture_expirations;
	health.last_capture_duration_msec = elapsed_msec(active_capture.started);
	health.capture_active = false;
	active_capture = {};
}

bool submit_capture()
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	if (queued.size() >= WORLD_RECOVERY_QUEUE_CAPACITY)
		return false;
	const size_t bytes = active_capture.generation.blob.size();
	const uint64_t sequence = active_capture.generation.sequence;
	try
	{
		queued.push_back(std::move(active_capture.generation));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	++health.submitted;
	health.last_submitted_sequence = sequence;
	health.queued_generations = queued.size();
	health.captured_bytes += bytes;
	health.high_water_bytes = std::max(health.high_water_bytes, static_cast<uint64_t>(bytes));
	health.capture_age_msec = elapsed_msec(active_capture.started);
	health.last_capture_duration_msec = health.capture_age_msec;
	health.capture_active = false;
	active_capture = {};
	generation_available.notify_one();
	return true;
}

bool capture_item_tree(P_obj object, uint64_t root_uid, uint64_t parent_uid,
		       world_recovery_item_snapshot *items, uint32_t *count)
{
	if (!object || !items || !count || *count >= WORLD_RECOVERY_MAX_ITEM_TREE ||
	    !object->obj_uid || OBJ_VNUM(object) <= 0)
		return false;
	const uint64_t item_uid = object->obj_uid;
	if (!root_uid)
		root_uid = item_uid;
	world_recovery_item_snapshot &entry = items[(*count)++];
	entry = {};
	entry.item_uid = item_uid;
	entry.root_item_uid = root_uid;
	entry.parent_item_uid = parent_uid;
	entry.vnum = OBJ_VNUM(object);
	entry.type = object->type;
	for (int index = 0; index < NUMB_OBJ_VALS; ++index)
		entry.values[index] = object->value[index];
	for (int index = 0; index < 6; ++index)
		entry.timers[index] = static_cast<int64_t>(object->timer[index]);
	if (object->name)
		strlcpy(entry.name, object->name, sizeof(entry.name));
	if (object->short_description)
		strlcpy(entry.short_description, object->short_description,
			sizeof(entry.short_description));
	if (object->description)
		strlcpy(entry.description, object->description, sizeof(entry.description));
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!capture_item_tree(child, root_uid, item_uid, items, count))
			return false;
	return true;
}

int write_object_record(P_obj object, int room_vnum, char *buffer, size_t maximum)
{
	if (!object || !buffer || room_vnum <= 0 || maximum < sizeof(world_recovery_object_record))
		return -1;
	std::array<world_recovery_item_snapshot, WORLD_RECOVERY_MAX_ITEM_TREE> items = {};
	uint32_t count = 0;
	if (!capture_item_tree(object, 0, 0, items.data(), &count) || !count)
		return -1;
	const size_t size = sizeof(world_recovery_object_record) +
			    static_cast<size_t>(count) * sizeof(world_recovery_item_snapshot);
	if (size > maximum)
		return -1;
	const world_recovery_object_record record = { room_vnum, count };
	memcpy(buffer, &record, sizeof(record));
	memcpy(buffer + sizeof(record), items.data(),
	       static_cast<size_t>(count) * sizeof(world_recovery_item_snapshot));
	return static_cast<int>(size);
}

int write_mob_record(P_char mob, char *buffer, size_t maximum)
{
	if (!mob || !buffer || maximum < sizeof(copyover_mob))
		return -1;
	copyover_mob entry = {};
	const int mob_rnum = GET_RNUM(mob);
	if (mob_rnum < 0 || mob->in_room < 0 || mob->in_room > top_of_world)
		return -1;
	entry.vnum = mob_index[mob_rnum].virtual_number;
	entry.idnum = GET_IDNUM(mob);
	entry.room = world[mob->in_room].number;
	entry.hit = GET_HIT(mob);
	entry.max_hit = GET_MAX_HIT(mob);
	entry.mana = GET_MANA(mob);
	entry.max_mana = GET_MAX_MANA(mob);
	entry.vitality = GET_VITALITY(mob);
	entry.max_vitality = GET_MAX_VITALITY(mob);
	entry.position = GET_POS(mob);
	// Currency is authoritative player state and must not be replayed from a fuzzy world view.
	entry.gold = 0;
	if (mob->specials.fighting)
	{
		P_char target = mob->specials.fighting;
		if (IS_NPC(target))
		{
			entry.fighting_type = 2;
			entry.fighting_id = GET_IDNUM(target);
		}
		else
		{
			entry.fighting_type = 1;
			if (GET_NAME(target))
				strlcpy(entry.fighting_name, GET_NAME(target),
					sizeof(entry.fighting_name));
		}
	}
	std::fill(std::begin(entry.equipment_vnums), std::end(entry.equipment_vnums), -1);
	entry.num_carrying = 0;
	for (affected_type *affect = mob->affected; affect; affect = affect->next)
	{
		if (sizeof(entry) +
			    (static_cast<size_t>(entry.num_affects) + 1) * sizeof(copyover_affect) >
		    maximum)
			return -1;
		++entry.num_affects;
	}
	memcpy(buffer, &entry, sizeof(entry));
	size_t offset = sizeof(entry);
	for (affected_type *affect = mob->affected; affect; affect = affect->next)
	{
		copyover_affect saved = {};
		saved.type = affect->type;
		saved.wear_off_message_index = affect->wear_off_message_index;
		saved.duration = affect->duration;
		saved.flags = affect->flags;
		saved.modifier = affect->modifier;
		saved.location = affect->location;
		saved.loc2 = affect->loc2;
		saved.level = affect->level;
		saved.bitvector = affect->bitvector;
		saved.bitvector2 = affect->bitvector2;
		saved.bitvector3 = affect->bitvector3;
		saved.bitvector4 = affect->bitvector4;
		saved.bitvector5 = affect->bitvector5;
		memcpy(buffer + offset, &saved, sizeof(saved));
		offset += sizeof(saved);
	}
	return static_cast<int>(offset);
}

void publisher_main()
{
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		health.worker_running = true;
	}
	for (;;)
	{
		recovery_generation generation;
		{
			std::unique_lock<std::mutex> lock(recovery_mutex);
			generation_available.wait(lock,
						  [] { return stop_requested || !queued.empty(); });
			if (stop_requested && queued.empty())
				break;
			generation = std::move(queued.front());
			queued.pop_front();
			worker_busy = true;
			health.worker_busy = true;
			health.queued_generations = queued.size();
		}

		world_recovery_header header = {};
		bool published = false;
		unsigned int attempts = 0;
		const auto started = std::chrono::steady_clock::now();
		if (encode_generation(&generation, &header))
		{
			for (; attempts < WORLD_RECOVERY_MAX_RETRIES && !published; ++attempts)
			{
				published = publish_callback &&
					    publish_callback(generation.blob.data(),
							     generation.blob.size(), &header,
							     publish_context);
				if (!published)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		std::lock_guard<std::mutex> lock(recovery_mutex);
		health.worker_runtime_msec = elapsed_msec(started);
		health.last_published_bytes = generation.blob.size();
		if (published)
			++health.published;
		else
			++health.publish_failures;
		if (completions.size() < WORLD_RECOVERY_QUEUE_CAPACITY * 2)
			completions.push_back({ generation.sequence, published, attempts });
		worker_busy = false;
		health.worker_busy = false;
	}
	std::lock_guard<std::mutex> lock(recovery_mutex);
	health.worker_running = false;
}

bool capture_one_record()
{
	switch (active_capture.stage)
	{
	case capture_stage::mobs:
		while (active_capture.next_character)
		{
			P_char ch = active_capture.next_character;
			active_capture.next_character = ch->next;
			if (!IS_NPC(ch) || ch->in_room < 0 || IS_PC_PET(ch))
				return true;
			const int size = write_mob_record(
				ch, reinterpret_cast<char *>(capture_buffer.data()),
				capture_buffer.size());
			if (size <= 0 || !append_record(active_capture.generation,
							world_recovery_record_type::mob,
							capture_buffer.data(), size))
				return false;
			++active_capture.generation.mob_count;
			return true;
		}
		active_capture.stage = capture_stage::objects;
		active_capture.next_object = object_list;
		return true;
	case capture_stage::objects:
		while (active_capture.next_object)
		{
			P_obj obj = active_capture.next_object;
			active_capture.next_object = obj->next;
			if (!OBJ_ROOM(obj))
				return true;
			const int vnum = OBJ_VNUM(obj);
			if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS ||
			    vnum == VOBJ_CARGO_CRATE)
				return true;
			if (vnum == 2 && obj->value[6] > 0 &&
			    time(NULL) - static_cast<time_t>(obj->value[6]) > 86400)
				return true;
			const int size =
				write_object_record(obj, world[obj->loc.room].number,
						    reinterpret_cast<char *>(capture_buffer.data()),
						    capture_buffer.size());
			if (size <= 0 || !append_record(active_capture.generation,
							world_recovery_record_type::object,
							capture_buffer.data(), size))
				return false;
			++active_capture.generation.object_count;
			return true;
		}
		active_capture.stage = capture_stage::doors;
		return true;
	case capture_stage::doors:
		while (active_capture.room <= top_of_world)
		{
			while (active_capture.direction < NUM_EXITS)
			{
				const int direction = active_capture.direction++;
				if (!world[active_capture.room].dir_option[direction] ||
				    !IS_SET(world[active_capture.room]
						    .dir_option[direction]
						    ->exit_info,
					    EX_ISDOOR))
					continue;
				const int size = copyover_write_door_to_buffer(
					active_capture.room, direction,
					reinterpret_cast<char *>(capture_buffer.data()),
					capture_buffer.size());
				if (size <= 0 || !append_record(active_capture.generation,
								world_recovery_record_type::door,
								capture_buffer.data(), size))
					return false;
				++active_capture.generation.door_count;
				return true;
			}
			++active_capture.room;
			active_capture.direction = 0;
			return true;
		}
		active_capture.stage = capture_stage::zones;
		return true;
	case capture_stage::zones:
		if (active_capture.zone <= top_of_zone_table)
		{
			const int size = copyover_write_zone_age_to_buffer(
				active_capture.zone++,
				reinterpret_cast<char *>(capture_buffer.data()),
				capture_buffer.size());
			if (size <= 0 || !append_record(active_capture.generation,
							world_recovery_record_type::zone,
							capture_buffer.data(), size))
				return false;
			++active_capture.generation.zone_count;
			return true;
		}
		return submit_capture();
	case capture_stage::idle:
		return true;
	}
	return false;
}
} // namespace

bool world_recovery_pipeline_init(world_recovery_publish_fn publish, void *context)
{
	if (!publish)
		return false;
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		if (health.initialized)
			return false;
		health = {};
		health.initialized = true;
		publish_callback = publish;
		publish_context = context;
		stop_requested = false;
		worker_busy = false;
		capture_failure_pending = false;
		capture_failure_completion = {};
	}
	try
	{
		publisher_worker = std::thread(publisher_main);
	}
	catch (const std::system_error &)
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		health.initialized = false;
		return false;
	}
	return true;
}

bool world_recovery_pipeline_set_sequence_floor(uint64_t durable_sequence)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	if (!health.initialized || health.capture_active || !queued.empty() || worker_busy ||
	    !completions.empty() || capture_failure_pending || durable_sequence == UINT64_MAX)
		return false;
	next_sequence = std::max(next_sequence, durable_sequence + 1);
	health.last_acknowledged_sequence =
		std::max(health.last_acknowledged_sequence, durable_sequence);
	return true;
}

void world_recovery_pipeline_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		stop_requested = true;
		generation_available.notify_all();
	}
	if (publisher_worker.joinable())
		publisher_worker.join();
	std::lock_guard<std::mutex> lock(recovery_mutex);
	queued.clear();
	completions.clear();
	capture_failure_pending = false;
	capture_failure_completion = {};
	active_capture = {};
	health.initialized = false;
	health.capture_active = false;
	health.worker_busy = false;
	publish_callback = nullptr;
	publish_context = nullptr;
}

void world_recovery_pipeline_cancel(void)
{
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		stop_requested = true;
		queued.clear();
		completions.clear();
		capture_failure_pending = false;
		capture_failure_completion = {};
		active_capture = {};
		health.capture_active = false;
		health.queued_generations = 0;
		generation_available.notify_all();
	}
	if (publisher_worker.joinable())
		publisher_worker.join();
	std::lock_guard<std::mutex> lock(recovery_mutex);
	health.initialized = false;
	health.worker_busy = false;
	publish_callback = nullptr;
	publish_context = nullptr;
}

bool world_recovery_pipeline_request(void)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	if (!health.initialized || stop_requested)
		return false;
	++health.requested;
	if (active_capture.stage != capture_stage::idle || !queued.empty() || worker_busy ||
	    !completions.empty() || capture_failure_pending)
	{
		++health.coalesced;
		return true;
	}
	active_capture = {};
	active_capture.stage = capture_stage::mobs;
	active_capture.generation.sequence = next_sequence++;
	active_capture.generation.timestamp = time(NULL);
	try
	{
		active_capture.generation.blob.resize(sizeof(world_recovery_header));
	}
	catch (const std::bad_alloc &)
	{
		active_capture = {};
		++health.capture_failures;
		return false;
	}
	active_capture.next_character = character_list;
	active_capture.started = std::chrono::steady_clock::now();
	health.capture_active = true;
	return true;
}

void world_recovery_pipeline_pulse(void)
{
	{
		std::lock_guard<std::mutex> lock(recovery_mutex);
		if (!health.capture_active)
			return;
	}
	if (world_recovery_capture_age_expired(elapsed_msec(active_capture.started)))
	{
		fail_capture(true);
		return;
	}
	const auto deadline = std::chrono::steady_clock::now() +
			      std::chrono::microseconds(WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC);
	for (size_t count = 0; count < WORLD_RECOVERY_CAPTURE_RECORD_BUDGET &&
			       std::chrono::steady_clock::now() < deadline;
	     ++count)
	{
		if (!capture_one_record())
		{
			fail_capture(false);
			return;
		}
		std::lock_guard<std::mutex> lock(recovery_mutex);
		++health.captured_records;
		if (!health.capture_active)
			return;
	}
}

bool world_recovery_pipeline_take_completion(world_recovery_completion *completion)
{
	if (!completion)
		return false;
	std::lock_guard<std::mutex> lock(recovery_mutex);
	if (capture_failure_pending)
	{
		*completion = capture_failure_completion;
		capture_failure_pending = false;
		capture_failure_completion = {};
		return true;
	}
	if (completions.empty())
		return false;
	*completion = completions.front();
	completions.pop_front();
	if (completion->published && completion->sequence == health.last_submitted_sequence &&
	    completion->sequence > health.last_acknowledged_sequence)
		health.last_acknowledged_sequence = completion->sequence;
	else if (completion->published)
		++health.stale_completions;
	return true;
}

bool world_recovery_pipeline_drain(uint64_t timeout_msec)
{
	if (!timeout_msec)
		return false;
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		world_recovery_pipeline_pulse();
		{
			std::lock_guard<std::mutex> lock(recovery_mutex);
			if (!health.capture_active && queued.empty() && !worker_busy &&
			    completions.empty() && !capture_failure_pending)
				return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return false;
}

bool world_recovery_pipeline_busy(void)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	return health.capture_active || !queued.empty() || worker_busy || !completions.empty() ||
	       capture_failure_pending;
}

bool world_recovery_capture_age_expired(uint64_t age_msec)
{
	return age_msec >= WORLD_RECOVERY_CAPTURE_MAX_AGE_MSEC;
}

world_recovery_health world_recovery_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	world_recovery_health snapshot = health;
	snapshot.queued_generations = queued.size();
	if (health.capture_active)
		snapshot.capture_age_msec = elapsed_msec(active_capture.started);
	return snapshot;
}

bool world_recovery_validate(const unsigned char *data, size_t size, int max_age_seconds,
			     uint64_t minimum_sequence, world_recovery_header *header_out)
{
	if (!data || size < WORLD_RECOVERY_WIRE_HEADER_BYTES || size > WORLD_RECOVERY_MAX_BYTES ||
	    max_age_seconds <= 0)
		return false;
	world_recovery_header header = {};
	if (!world_recovery_decode_header(data, size, &header) || memcmp(header.magic, "WRS9", 4) ||
	    header.schema_version != WORLD_RECOVERY_SCHEMA_VERSION ||
	    header.header_size != WORLD_RECOVERY_WIRE_HEADER_BYTES || !header.complete ||
	    header.sequence < minimum_sequence ||
	    header.payload_size != size - WORLD_RECOVERY_WIRE_HEADER_BYTES)
		return false;
	const time_t now = time(NULL);
	if (static_cast<int64_t>(static_cast<time_t>(header.timestamp)) != header.timestamp ||
	    header.timestamp <= 0 || header.timestamp > static_cast<int64_t>(now) + 60 ||
	    static_cast<int64_t>(now) - header.timestamp > max_age_seconds)
		return false;
	if (crc32(0, data + WORLD_RECOVERY_WIRE_HEADER_BYTES, header.payload_size) !=
	    header.checksum)
		return false;
	size_t offset = WORLD_RECOVERY_WIRE_HEADER_BYTES;
	uint32_t mobs = 0, objects = 0, doors = 0, zones = 0;
	while (offset < size)
	{
		if (size - offset < WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES)
			return false;
		world_recovery_record_type type = {};
		uint32_t record_size = 0;
		if (!world_recovery_decode_record_header(data + offset, size - offset, &type,
							 &record_size))
			return false;
		offset += WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES;
		if (record_size > size - offset)
			return false;
		size_t native_size = 0;
		if (!world_recovery_record_native_size(type, data + offset, record_size,
						       &native_size))
			return false;
		switch (type)
		{
		case world_recovery_record_type::mob:
			++mobs;
			break;
		case world_recovery_record_type::object:
			++objects;
			break;
		case world_recovery_record_type::door:
			++doors;
			break;
		case world_recovery_record_type::zone:
			++zones;
			break;
		}
		offset += record_size;
	}
	if (mobs != header.mob_count || objects != header.object_count ||
	    doors != header.door_count || zones != header.zone_count)
		return false;
	if (header_out)
		*header_out = header;
	return true;
}

namespace
{
bool terminated(const char *text, size_t size)
{
	return text && memchr(text, '\0', size);
}

P_obj find_object_uid(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return nullptr;
}

bool existing_tree_matches(const planned_object &planned)
{
	if (!planned.existing_root || planned.items.empty() || !OBJ_ROOM(planned.existing_root) ||
	    world[planned.existing_root->loc.room].number != planned.room_vnum)
		return false;
	for (const world_recovery_item_snapshot &item : planned.items)
	{
		P_obj object = find_object_uid(item.item_uid);
		if (!object || OBJ_VNUM(object) != item.vnum)
			return false;
		if (!item.parent_item_uid)
		{
			if (object != planned.existing_root)
				return false;
		}
		else if (!OBJ_INSIDE(object) || !object->loc.inside ||
			 object->loc.inside->obj_uid != item.parent_item_uid)
			return false;
	}
	size_t live_count = 0;
	std::array<P_obj, WORLD_RECOVERY_MAX_ITEM_TREE> pending = {};
	size_t pending_count = 1;
	pending[0] = planned.existing_root;
	while (pending_count)
	{
		P_obj object = pending[--pending_count];
		if (++live_count > planned.items.size())
			return false;
		for (P_obj child = object->contains; child; child = child->next_content)
		{
			if (pending_count >= pending.size())
				return false;
			pending[pending_count++] = child;
		}
	}
	return live_count == planned.items.size();
}

bool add_object_record(recovery_plan *plan, const unsigned char *data, size_t size)
{
	if (!plan || !data ||
	    size < sizeof(world_recovery_object_record) + sizeof(world_recovery_item_snapshot))
		return false;
	world_recovery_object_record record = {};
	memcpy(&record, data, sizeof(record));
	if (!record.item_count || record.item_count > WORLD_RECOVERY_MAX_ITEM_TREE ||
	    size != sizeof(record) + static_cast<size_t>(record.item_count) *
					     sizeof(world_recovery_item_snapshot))
		return false;
	planned_object planned;
	planned.room_vnum = record.room_vnum;
	planned.room_rnum = real_room(record.room_vnum);
	if (planned.room_rnum < 0 || planned.room_rnum > top_of_world)
		return false;
	try
	{
		planned.items.resize(record.item_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	memcpy(planned.items.data(), data + sizeof(record),
	       planned.items.size() * sizeof(world_recovery_item_snapshot));
	std::unordered_set<uint64_t> parents;
	try
	{
		parents.reserve(planned.items.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const uint64_t root_uid = planned.items[0].item_uid;
	for (size_t index = 0; index < planned.items.size(); ++index)
	{
		const world_recovery_item_snapshot &item = planned.items[index];
		for (int timer_index = 0; timer_index < 6; ++timer_index)
			if (static_cast<int64_t>(static_cast<time_t>(item.timers[timer_index])) !=
			    item.timers[timer_index])
				return false;
		if (!item.item_uid || item.item_uid > ULONG_MAX || !item.root_item_uid ||
		    item.root_item_uid != root_uid || item.vnum <= 0 || item.type < ITEM_LIGHT ||
		    item.type > ITEM_LAST || real_object(item.vnum) < 0 ||
		    !terminated(item.name, sizeof(item.name)) ||
		    !terminated(item.short_description, sizeof(item.short_description)) ||
		    !terminated(item.description, sizeof(item.description)) ||
		    (index == 0 && item.parent_item_uid) ||
		    (index != 0 && (!item.parent_item_uid ||
				    parents.find(item.parent_item_uid) == parents.end())))
			return false;
		try
		{
			if (!plan->item_uids.insert(item.item_uid).second ||
			    !parents.insert(item.item_uid).second)
				return false;
			plan->authority_items.push_back({ item.item_uid, item.root_item_uid,
							  item.parent_item_uid, planned.room_vnum,
							  item.vnum });
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	planned.existing_root = find_object_uid(root_uid);
	for (const world_recovery_item_snapshot &item : planned.items)
		if (find_object_uid(item.item_uid) &&
		    (!planned.existing_root || !existing_tree_matches(planned)))
			return false;
	try
	{
		plan->objects.push_back(std::move(planned));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool build_recovery_plan(const unsigned char *data, size_t size, int max_age_seconds,
			 uint64_t minimum_sequence, recovery_plan *plan)
{
	if (!plan ||
	    !world_recovery_validate(data, size, max_age_seconds, minimum_sequence, &plan->header))
		return false;
	size_t offset = WORLD_RECOVERY_WIRE_HEADER_BYTES;
	while (offset < size)
	{
		world_recovery_record_type type = {};
		uint32_t record_size = 0;
		if (!world_recovery_decode_record_header(data + offset, size - offset, &type,
							 &record_size))
			return false;
		offset += WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES;
		std::vector<unsigned char> native_record;
		if (record_size > size - offset ||
		    !world_recovery_decode_record(type, data + offset, record_size, &native_record))
			return false;
		const unsigned char *record_data = native_record.data();
		switch (type)
		{
		case world_recovery_record_type::mob:
		{
			copyover_mob entry = {};
			memcpy(&entry, record_data, sizeof(entry));
			planned_mob planned;
			planned.room_rnum = real_room(entry.room);
			if (real_mobile(entry.vnum) < 0 || planned.room_rnum < 0 ||
			    planned.room_rnum > top_of_world || entry.max_hit <= 0 ||
			    entry.max_mana < 0 || entry.max_vitality < 0 || entry.gold < 0)
				return false;
			const unsigned char *affect_data = record_data + sizeof(entry);
			for (int index = 0; index < entry.num_affects; ++index)
			{
				copyover_affect affect = {};
				memcpy(&affect,
				       affect_data + static_cast<size_t>(index) * sizeof(affect),
				       sizeof(affect));
				if (affect.location > APPLY_LAST)
					return false;
			}
			try
			{
				planned.record = std::move(native_record);
				plan->mobs.push_back(std::move(planned));
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
			break;
		}
		case world_recovery_record_type::object:
			if (!add_object_record(plan, record_data, native_record.size()))
				return false;
			break;
		case world_recovery_record_type::door:
		{
			copyover_room door = {};
			memcpy(&door, record_data, sizeof(door));
			const int room = real_room(door.vnum);
			constexpr int ALLOWED_DOOR_FLAGS =
				EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_RSCLOSED | EX_RSLOCKED |
				EX_PICKABLE | EX_SECRET | EX_BLOCKED | EX_PICKPROOF | EX_WALLED |
				EX_SPIKED | EX_ILLUSION | EX_BREAKABLE;
			if (room < 0 || room > top_of_world || door.dir < 0 ||
			    door.dir >= NUM_EXITS || !world[room].dir_option[door.dir] ||
			    !IS_SET(world[room].dir_option[door.dir]->exit_info, EX_ISDOOR) ||
			    !IS_SET(door.state, EX_ISDOOR) || (door.state & ~ALLOWED_DOOR_FLAGS))
				return false;
			try
			{
				plan->doors.push_back(door);
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
			break;
		}
		case world_recovery_record_type::zone:
		{
			zone_age_entry zone = {};
			memcpy(&zone, record_data, sizeof(zone));
			if (zone.zone_rnum < 0 || zone.zone_rnum > top_of_zone_table ||
			    zone.age < 0 || zone.lifespan < 0 || zone.fullreset_age < 0 ||
			    zone.fullreset_lifespan < 0)
				return false;
			try
			{
				plan->zones.push_back(zone);
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
			break;
		}
		}
		offset += record_size;
	}
	return plan->mobs.size() == plan->header.mob_count &&
	       plan->objects.size() == plan->header.object_count &&
	       plan->doors.size() == plan->header.door_count &&
	       plan->zones.size() == plan->header.zone_count;
}

void replace_object_text(P_obj object, const world_recovery_item_snapshot &item)
{
	if (item.name[0])
	{
		if ((object->str_mask & STRUNG_KEYS) && object->name)
			str_free(object->name);
		object->name = str_dup(item.name);
		object->str_mask |= STRUNG_KEYS;
	}
	if (item.short_description[0])
	{
		if ((object->str_mask & STRUNG_DESC2) && object->short_description)
			str_free(object->short_description);
		object->short_description = str_dup(item.short_description);
		object->str_mask |= STRUNG_DESC2;
	}
	if (item.description[0])
	{
		if ((object->str_mask & STRUNG_DESC1) && object->description)
			str_free(object->description);
		object->description = str_dup(item.description);
		object->str_mask |= STRUNG_DESC1;
	}
}

P_obj materialize_object(const planned_object &planned)
{
	if (planned.existing_root)
		return planned.existing_root;
	std::unordered_map<uint64_t, P_obj> objects;
	P_obj root = nullptr;
	try
	{
		objects.reserve(planned.items.size());
	}
	catch (const std::bad_alloc &)
	{
		return nullptr;
	}
	for (const world_recovery_item_snapshot &item : planned.items)
	{
		P_obj object = read_object(item.vnum, VIRTUAL);
		if (!object)
		{
			if (root)
				extract_obj(root, FALSE);
			return nullptr;
		}
		object->obj_uid = static_cast<unsigned long>(item.item_uid);
		object->type = item.type;
		for (int index = 0; index < NUMB_OBJ_VALS; ++index)
			object->value[index] = item.values[index];
		for (int index = 0; index < 6; ++index)
			object->timer[index] = static_cast<time_t>(item.timers[index]);
		replace_object_text(object, item);
		if (!item.parent_item_uid)
			root = object;
		else
		{
			const auto parent = objects.find(item.parent_item_uid);
			if (parent == objects.end())
			{
				extract_obj(object, FALSE);
				if (root)
					extract_obj(root, FALSE);
				return nullptr;
			}
			obj_to_obj(object, parent->second);
		}
		try
		{
			objects.emplace(item.item_uid, object);
		}
		catch (const std::bad_alloc &)
		{
			if (!item.parent_item_uid)
				extract_obj(object, FALSE);
			else if (root)
				extract_obj(root, FALSE);
			return nullptr;
		}
	}
	if (!root)
		return nullptr;
	obj_to_room(root, planned.room_rnum);
	return root;
}

void rollback_materialized(std::vector<P_char> *mobs, std::vector<P_obj> *objects)
{
	if (objects)
		for (auto item = objects->rbegin(); item != objects->rend(); ++item)
			extract_obj(*item, FALSE);
	if (mobs)
		for (auto mob = mobs->rbegin(); mob != mobs->rend(); ++mob)
			extract_char(*mob);
}

bool materialize_plan(const recovery_plan &plan,
		      const std::vector<item_ownership_runtime_entry> &authoritative)
{
	std::vector<P_char> created_mobs;
	std::vector<P_obj> created_objects;
	size_t applied_objects = 0;
	try
	{
		created_mobs.reserve(plan.mobs.size());
		created_objects.reserve(plan.objects.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (const planned_mob &planned : plan.mobs)
	{
		size_t consumed = 0;
		P_char mob = copyover_restore_mob_from_buffer(
			reinterpret_cast<const char *>(planned.record.data()),
			planned.record.size(), &consumed);
		if (!mob || consumed != planned.record.size())
		{
			rollback_materialized(&created_mobs, &created_objects);
			return false;
		}
		created_mobs.push_back(mob);
	}
	for (const planned_object &planned : plan.objects)
	{
		P_obj object = materialize_object(planned);
		if (!object)
		{
			rollback_materialized(&created_mobs, &created_objects);
			return false;
		}
		if (!planned.existing_root)
			created_objects.push_back(object);
		++applied_objects;
	}
	if (created_mobs.size() != plan.mobs.size() || applied_objects != plan.objects.size())
	{
		rollback_materialized(&created_mobs, &created_objects);
		return false;
	}
	if (!item_ownership_runtime_hydrate_many_atomic(authoritative.data(), authoritative.size()))
	{
		rollback_materialized(&created_mobs, &created_objects);
		return false;
	}
	for (const copyover_room &door : plan.doors)
	{
		const int room = real_room(door.vnum);
		world[room].dir_option[door.dir]->exit_info = door.state;
	}
	for (const zone_age_entry &zone : plan.zones)
	{
		zone_table[zone.zone_rnum].age = zone.age;
		zone_table[zone.zone_rnum].lifespan = zone.lifespan;
		zone_table[zone.zone_rnum].fullreset_age = zone.fullreset_age;
		zone_table[zone.zone_rnum].fullreset_lifespan = zone.fullreset_lifespan;
	}
	return true;
}
} // namespace

bool world_recovery_restore_with_floor(const unsigned char *data, size_t size, int max_age_seconds,
				       uint64_t minimum_sequence,
				       const unsigned char *const *floor_records,
				       const size_t *floor_record_sizes, size_t floor_record_count,
				       world_recovery_header *header_out)
{
	if ((!floor_records || !floor_record_sizes) && floor_record_count)
		return false;
	recovery_plan plan;
	if (!build_recovery_plan(data, size, max_age_seconds, minimum_sequence, &plan))
		return false;
	for (size_t index = 0; index < floor_record_count; ++index)
	{
		std::vector<unsigned char> native_record;
		if (!world_recovery_decode_record(world_recovery_record_type::object,
						  floor_records[index], floor_record_sizes[index],
						  &native_record) ||
		    !add_object_record(&plan, native_record.data(), native_record.size()))
			return false;
	}
	std::vector<item_ownership_runtime_entry> authoritative;
	try
	{
		authoritative.resize(plan.authority_items.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!sql_persistence_reconcile_world_recovery_items(
		    plan.authority_items.data(), plan.authority_items.size(), authoritative.data(),
		    authoritative.size()))
		return false;
	redis_world_recovery_set_materializing(true);
	const bool materialized = materialize_plan(plan, authoritative);
	redis_world_recovery_set_materializing(false);
	if (!materialized)
		return false;
	if (header_out)
		*header_out = plan.header;
	return true;
}

bool world_recovery_restore(const unsigned char *data, size_t size, int max_age_seconds,
			    uint64_t minimum_sequence, world_recovery_header *header_out)
{
	return world_recovery_restore_with_floor(data, size, max_age_seconds, minimum_sequence,
						 nullptr, nullptr, 0, header_out);
}

int world_recovery_write_object_to_buffer(P_obj obj, int room_vnum, char *buf, size_t max_len)
{
	return write_object_record(obj, room_vnum, buf, max_len);
}

void world_recovery_capture_forget_character(P_char ch)
{
	if (ch && active_capture.next_character == ch)
		active_capture.next_character = ch->next;
}

void world_recovery_capture_forget_object(P_obj obj)
{
	if (obj && active_capture.next_object == obj)
		active_capture.next_object = obj->next;
}

void world_recovery_pipeline_reset_for_tests(void)
{
	world_recovery_pipeline_shutdown();
	std::lock_guard<std::mutex> lock(recovery_mutex);
	health = {};
	next_sequence = 1;
	stop_requested = false;
	worker_busy = false;
	capture_failure_pending = false;
	capture_failure_completion = {};
}
