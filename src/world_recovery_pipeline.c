#include "world_recovery_pipeline.h"

#include "copyover.h"
#include "db.h"
#include "prototypes.h"
#include "redis.h"
#include "ships/ships.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <utility>
#include <vector>
#include <zlib.h>

extern struct zone_data *zone_table;
extern struct room_data *world;
extern P_char character_list;
extern P_obj object_list;
extern int top_of_world;
extern int top_of_zone_table;
extern bool sql_persistence_item_owner_matches(unsigned long long item_uid, const char *owner_type,
					       const char *owner_ref, const char *context);

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

enum class record_type : uint8_t
{
	mob = 1,
	object = 2,
	door = 3,
	zone = 4,
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
	std::vector<unsigned char> payload;
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
std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> capture_buffer = {};

uint64_t elapsed_msec(std::chrono::steady_clock::time_point started)
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
					     std::chrono::steady_clock::now() - started)
					     .count());
}

bool append_record(recovery_generation &generation, record_type type, const void *data, size_t size)
{
	if (!data || !size || size > WORLD_RECOVERY_MAX_RECORD_BYTES)
		return false;
	const size_t added = sizeof(record_header) + size;
	if (generation.payload.size() > WORLD_RECOVERY_MAX_BYTES - added)
		return false;
	try
	{
		const size_t offset = generation.payload.size();
		generation.payload.resize(offset + added);
		record_header header = {};
		header.size = static_cast<uint32_t>(size);
		header.type = static_cast<uint8_t>(type);
		memcpy(generation.payload.data() + offset, &header, sizeof(header));
		memcpy(generation.payload.data() + offset + sizeof(header), data, size);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

void fail_capture()
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	++health.capture_failures;
	health.capture_active = false;
	active_capture = {};
}

bool submit_capture()
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	if (queued.size() >= WORLD_RECOVERY_QUEUE_CAPACITY)
		return false;
	const size_t bytes = active_capture.generation.payload.size();
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
	health.capture_active = false;
	active_capture = {};
	generation_available.notify_one();
	return true;
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
		memcpy(header.magic, "WRS7", 4);
		header.schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
		header.header_size = sizeof(header);
		header.sequence = generation.sequence;
		header.timestamp = generation.timestamp;
		header.payload_size = generation.payload.size();
		header.checksum = crc32(0, generation.payload.data(), generation.payload.size());
		header.mob_count = generation.mob_count;
		header.object_count = generation.object_count;
		header.door_count = generation.door_count;
		header.zone_count = generation.zone_count;
		header.complete = 1;
		std::vector<unsigned char> blob;
		bool published = false;
		unsigned int attempts = 0;
		const auto started = std::chrono::steady_clock::now();
		try
		{
			blob.resize(sizeof(header) + generation.payload.size());
			memcpy(blob.data(), &header, sizeof(header));
			memcpy(blob.data() + sizeof(header), generation.payload.data(),
			       generation.payload.size());
			for (; attempts < WORLD_RECOVERY_MAX_RETRIES && !published; ++attempts)
			{
				published = publish_callback &&
					    publish_callback(blob.data(), blob.size(), &header,
							     publish_context);
				if (!published)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		catch (const std::bad_alloc &)
		{
			published = false;
		}

		std::lock_guard<std::mutex> lock(recovery_mutex);
		health.worker_runtime_msec = elapsed_msec(started);
		health.last_published_bytes = blob.size();
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
			const int size = copyover_write_mob_to_buffer(
				ch, reinterpret_cast<char *>(capture_buffer.data()),
				capture_buffer.size());
			if (size <= 0 || !append_record(active_capture.generation, record_type::mob,
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
			const int size = copyover_write_obj_to_buffer(
				obj, reinterpret_cast<char *>(capture_buffer.data()),
				capture_buffer.size());
			if (size <= 0 ||
			    !append_record(active_capture.generation, record_type::object,
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
					return true;
				const int size = copyover_write_door_to_buffer(
					active_capture.room, direction,
					reinterpret_cast<char *>(capture_buffer.data()),
					capture_buffer.size());
				if (size <= 0 ||
				    !append_record(active_capture.generation, record_type::door,
						   capture_buffer.data(), size))
					return false;
				++active_capture.generation.door_count;
				return true;
			}
			++active_capture.room;
			active_capture.direction = 0;
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
			if (size <= 0 ||
			    !append_record(active_capture.generation, record_type::zone,
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
	    durable_sequence == UINT64_MAX)
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
	active_capture = {};
	health.initialized = false;
	health.capture_active = false;
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
	if (active_capture.stage != capture_stage::idle || !queued.empty() || worker_busy)
	{
		++health.coalesced;
		return true;
	}
	active_capture = {};
	active_capture.stage = capture_stage::mobs;
	active_capture.generation.sequence = next_sequence++;
	active_capture.generation.timestamp = time(NULL);
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
	const auto deadline = std::chrono::steady_clock::now() +
			      std::chrono::microseconds(WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC);
	for (size_t count = 0; count < WORLD_RECOVERY_CAPTURE_RECORD_BUDGET &&
			       std::chrono::steady_clock::now() < deadline;
	     ++count)
	{
		if (!capture_one_record())
		{
			fail_capture();
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
			if (!health.capture_active && queued.empty() && !worker_busy)
				return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return false;
}

bool world_recovery_pipeline_busy(void)
{
	std::lock_guard<std::mutex> lock(recovery_mutex);
	return health.capture_active || !queued.empty() || worker_busy || !completions.empty();
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
	if (!data || size < sizeof(world_recovery_header) || max_age_seconds <= 0)
		return false;
	world_recovery_header header = {};
	memcpy(&header, data, sizeof(header));
	if (memcmp(header.magic, "WRS7", 4) ||
	    header.schema_version != WORLD_RECOVERY_SCHEMA_VERSION ||
	    header.header_size != sizeof(header) || !header.complete ||
	    header.sequence < minimum_sequence || header.payload_size != size - sizeof(header))
		return false;
	const time_t now = time(NULL);
	if (header.timestamp <= 0 || header.timestamp > now + 60 ||
	    now - header.timestamp > max_age_seconds)
		return false;
	if (crc32(0, data + sizeof(header), header.payload_size) != header.checksum)
		return false;
	size_t offset = sizeof(header);
	uint32_t mobs = 0, objects = 0, doors = 0, zones = 0;
	while (offset < size)
	{
		if (size - offset < sizeof(record_header))
			return false;
		record_header record = {};
		memcpy(&record, data + offset, sizeof(record));
		offset += sizeof(record);
		if (!record.size || record.size > WORLD_RECOVERY_MAX_RECORD_BYTES ||
		    record.size > size - offset)
			return false;
		switch (static_cast<record_type>(record.type))
		{
		case record_type::mob:
			if (record.size < sizeof(copyover_mob))
				return false;
			{
				copyover_mob entry = {};
				memcpy(&entry, data + offset, sizeof(entry));
				if (entry.num_affects < 0 || entry.num_carrying < 0 ||
				    static_cast<size_t>(entry.num_affects) >
					    (WORLD_RECOVERY_MAX_RECORD_BYTES - sizeof(entry)) /
						    sizeof(copyover_affect) ||
				    static_cast<size_t>(entry.num_carrying) >
					    (WORLD_RECOVERY_MAX_RECORD_BYTES - sizeof(entry)) /
						    sizeof(copyover_carried_item))
					return false;
				const size_t affects = static_cast<size_t>(entry.num_affects) *
						       sizeof(copyover_affect);
				const size_t carrying = static_cast<size_t>(entry.num_carrying) *
							sizeof(copyover_carried_item);
				if (sizeof(entry) + affects + carrying != record.size)
					return false;
			}
			++mobs;
			break;
		case record_type::object:
			if (record.size < sizeof(copyover_obj))
				return false;
			{
				copyover_obj entry = {};
				memcpy(&entry, data + offset, sizeof(entry));
				if (entry.num_contents < 0 ||
				    static_cast<size_t>(entry.num_contents) >
					    (WORLD_RECOVERY_MAX_RECORD_BYTES - sizeof(entry)) /
						    sizeof(copyover_obj_content) ||
				    sizeof(entry) + static_cast<size_t>(entry.num_contents) *
							    sizeof(copyover_obj_content) !=
					    record.size)
					return false;
			}
			++objects;
			break;
		case record_type::door:
			if (record.size != sizeof(copyover_room))
				return false;
			++doors;
			break;
		case record_type::zone:
			if (record.size != sizeof(zone_age_entry))
				return false;
			++zones;
			break;
		default:
			return false;
		}
		offset += record.size;
	}
	if (mobs != header.mob_count || objects != header.object_count ||
	    doors != header.door_count || zones != header.zone_count)
		return false;
	if (header_out)
		*header_out = header;
	return true;
}

bool world_recovery_restore(const unsigned char *data, size_t size, int max_age_seconds,
			    uint64_t minimum_sequence, world_recovery_header *header_out)
{
	world_recovery_header header = {};
	if (!world_recovery_validate(data, size, max_age_seconds, minimum_sequence, &header))
		return false;
	size_t offset = sizeof(header);
	uint32_t mobs = 0, objects = 0, doors = 0, zones = 0;
	while (offset < size)
	{
		if (size - offset < sizeof(record_header))
			return false;
		record_header record = {};
		memcpy(&record, data + offset, sizeof(record));
		offset += sizeof(record);
		if (!record.size || record.size > WORLD_RECOVERY_MAX_RECORD_BYTES ||
		    record.size > size - offset)
			return false;
		size_t consumed = 0;
		switch (static_cast<record_type>(record.type))
		{
		case record_type::mob:
			copyover_restore_mob_from_buffer(
				reinterpret_cast<const char *>(data + offset), record.size,
				&consumed);
			++mobs;
			break;
		case record_type::object:
		{
			if (record.size < sizeof(copyover_obj))
				return false;
			copyover_obj entry = {};
			memcpy(&entry, data + offset, sizeof(entry));
			char room_ref[32];
			snprintf(room_ref, sizeof(room_ref), "%d", entry.room);
			if (entry.obj_uid &&
			    !sql_persistence_item_owner_matches(entry.obj_uid, "room", room_ref,
								"world_recovery_restore"))
			{
				consumed = record.size;
				++objects;
				break;
			}
			copyover_restore_obj_from_buffer(
				reinterpret_cast<const char *>(data + offset), record.size,
				&consumed);
			++objects;
			break;
		}
		case record_type::door:
			if (copyover_restore_door_from_buffer(
				    reinterpret_cast<const char *>(data + offset), record.size,
				    &consumed) < 0)
				return false;
			++doors;
			break;
		case record_type::zone:
			if (copyover_restore_zone_age_from_buffer(
				    reinterpret_cast<const char *>(data + offset), record.size,
				    &consumed) < 0)
				return false;
			++zones;
			break;
		default:
			return false;
		}
		if (consumed != record.size)
			return false;
		offset += record.size;
	}
	if (mobs != header.mob_count || objects != header.object_count ||
	    doors != header.door_count || zones != header.zone_count)
		return false;
	if (header_out)
		*header_out = header;
	return true;
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
}
