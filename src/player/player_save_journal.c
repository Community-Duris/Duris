#include "player/player_save_journal.h"

#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> JOURNAL_MAGIC = { 'D', 'P', 'S', 'J', 'N', 'L', '1', 0 };
constexpr uint32_t JOURNAL_FORMAT_VERSION = 1;
constexpr size_t JOURNAL_HEADER_SIZE = 72;
constexpr size_t JOURNAL_CHECKSUM_OFFSET = 68;
constexpr size_t JOURNAL_MAX_RECORD_SIZE = JOURNAL_HEADER_SIZE + PLAYER_SNAPSHOT_MAX_BYTES;
constexpr const char *JOURNAL_FILE_NAME = "player-save.journal";
constexpr const char *JOURNAL_TEMP_NAME = "player-save.journal.tmp";
constexpr const char *JOURNAL_QUARANTINE_NAME = "player-save.journal.quarantine";

struct journal_frame
{
	std::vector<uint8_t> bytes;
	player_snapshot snapshot;
	uint64_t record_id;
	uint64_t created_msec;
	uint32_t payload_checksum;
};

struct scan_result
{
	std::vector<journal_frame> frames;
	player_save_journal_result result = player_save_journal_result::ok;
	bool quarantine_ok = true;
};

std::mutex journal_mutex;
std::string journal_directory;
std::string journal_path;
std::string quarantine_path;
size_t journal_quota = 0;
uint64_t record_sequence = 0;
uint64_t oldest_record_msec = 0;
player_save_journal_health health = {};

uint64_t realtime_msec()
{
	struct timespec value = {};
	if (clock_gettime(CLOCK_REALTIME, &value) != 0)
		return 0;
	return static_cast<uint64_t>(value.tv_sec) * 1000 + value.tv_nsec / 1000000;
}

void put_u32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
	for (size_t index = 0; index < sizeof(value); ++index)
		bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}

void put_u64(std::vector<uint8_t> &bytes, size_t offset, uint64_t value)
{
	for (size_t index = 0; index < sizeof(value); ++index)
		bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}

uint32_t get_u32(const uint8_t *bytes, size_t offset)
{
	uint32_t value = 0;
	for (size_t index = 0; index < sizeof(value); ++index)
		value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8);
	return value;
}

uint64_t get_u64(const uint8_t *bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < sizeof(value); ++index)
		value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return value;
}

uint32_t crc32_update(uint32_t crc, const uint8_t *bytes, size_t size)
{
	for (size_t index = 0; index < size; ++index)
	{
		crc ^= bytes[index];
		for (unsigned int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0U - (crc & 1U)));
	}
	return crc;
}

uint32_t frame_checksum(const uint8_t *bytes, size_t size)
{
	uint32_t crc = UINT32_MAX;
	crc = crc32_update(crc, bytes, JOURNAL_CHECKSUM_OFFSET);
	if (size > JOURNAL_HEADER_SIZE)
		crc = crc32_update(crc, bytes + JOURNAL_HEADER_SIZE, size - JOURNAL_HEADER_SIZE);
	return ~crc;
}

uint32_t payload_checksum(const uint8_t *bytes, size_t size)
{
	return ~crc32_update(UINT32_MAX, bytes, size);
}

bool write_all(int fd, const uint8_t *bytes, size_t size)
{
	while (size)
	{
		const ssize_t written = write(fd, bytes, size);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return false;
		bytes += written;
		size -= static_cast<size_t>(written);
	}
	return true;
}

bool safe_owned_mode(const struct stat &status, mode_t allowed, bool directory)
{
	return (directory ? S_ISDIR(status.st_mode) : S_ISREG(status.st_mode)) &&
	       status.st_uid == geteuid() && !(status.st_mode & ~allowed & 0777);
}

int open_safe_file(const std::string &path, int flags, mode_t mode)
{
	const int fd = open(path.c_str(), flags | O_CLOEXEC | O_NOFOLLOW, mode);
	if (fd < 0)
		return -1;
	struct stat status = {};
	if (fstat(fd, &status) != 0 || !safe_owned_mode(status, 0600, false) ||
	    fchmod(fd, 0600) != 0)
	{
		close(fd);
		errno = EPERM;
		return -1;
	}
	return fd;
}

bool sync_directory()
{
	const int fd =
		open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
		return false;
	const bool ok = fsync(fd) == 0;
	close(fd);
	return ok;
}

bool append_quarantine(const uint8_t *bytes, size_t size)
{
	if (!size)
		return true;
	const int fd = open_safe_file(quarantine_path, O_WRONLY | O_APPEND | O_CREAT, 0600);
	if (fd < 0)
		return false;
	struct stat status = {};
	if (fstat(fd, &status) != 0 || status.st_size < 0 || size > journal_quota ||
	    static_cast<uint64_t>(status.st_size) > journal_quota - size)
	{
		close(fd);
		return false;
	}
	const bool ok = write_all(fd, bytes, size) && fdatasync(fd) == 0;
	close(fd);
	if (ok)
		health.quarantined_bytes += size;
	return ok;
}

bool build_frame(const player_snapshot &snapshot, journal_frame &frame)
{
	std::vector<uint8_t> payload;
	if (player_snapshot_encode(snapshot, &payload) != player_snapshot_codec_result::ok)
		return false;
	frame.bytes.assign(JOURNAL_HEADER_SIZE + payload.size(), 0);
	std::copy(JOURNAL_MAGIC.begin(), JOURNAL_MAGIC.end(), frame.bytes.begin());
	put_u32(frame.bytes, 8, JOURNAL_FORMAT_VERSION);
	put_u32(frame.bytes, 12, JOURNAL_HEADER_SIZE);
	put_u64(frame.bytes, 16, frame.bytes.size());
	frame.created_msec = realtime_msec();
	frame.record_id = (frame.created_msec << 20) ^ (++record_sequence) ^
			  (static_cast<uint64_t>(snapshot.pid) << 32) ^ snapshot.revision;
	put_u64(frame.bytes, 24, frame.record_id);
	put_u64(frame.bytes, 32, frame.created_msec);
	put_u32(frame.bytes, 40, static_cast<uint32_t>(snapshot.pid));
	put_u32(frame.bytes, 44, snapshot.schema_version);
	put_u64(frame.bytes, 48, snapshot.revision);
	put_u64(frame.bytes, 56, snapshot.components);
	put_u32(frame.bytes, 64, payload.size());
	std::copy(payload.begin(), payload.end(), frame.bytes.begin() + JOURNAL_HEADER_SIZE);
	frame.payload_checksum = payload_checksum(payload.data(), payload.size());
	put_u32(frame.bytes, JOURNAL_CHECKSUM_OFFSET,
		frame_checksum(frame.bytes.data(), frame.bytes.size()));
	frame.snapshot = snapshot;
	return true;
}

std::vector<uint8_t> read_journal_file(player_save_journal_result &result)
{
	std::vector<uint8_t> bytes;
	const int fd = open_safe_file(journal_path, O_RDONLY, 0600);
	if (fd < 0)
	{
		if (errno == ENOENT)
			return bytes;
		result = errno == EPERM ? player_save_journal_result::unsafe_permissions :
					  player_save_journal_result::io_failure;
		return bytes;
	}
	struct stat status = {};
	if (fstat(fd, &status) != 0 || status.st_size < 0 ||
	    static_cast<uint64_t>(status.st_size) > journal_quota)
	{
		result = static_cast<uint64_t>(status.st_size) > journal_quota ?
				 player_save_journal_result::quota_exceeded :
				 player_save_journal_result::io_failure;
		close(fd);
		return bytes;
	}
	try
	{
		bytes.resize(status.st_size);
	}
	catch (const std::bad_alloc &)
	{
		result = player_save_journal_result::io_failure;
		close(fd);
		return {};
	}
	size_t offset = 0;
	while (offset < bytes.size())
	{
		const ssize_t count = read(fd, bytes.data() + offset, bytes.size() - offset);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
		{
			result = player_save_journal_result::io_failure;
			bytes.clear();
			break;
		}
		offset += count;
	}
	close(fd);
	return bytes;
}

size_t find_next_magic(const std::vector<uint8_t> &bytes, size_t start)
{
	for (size_t offset = start; offset + JOURNAL_MAGIC.size() <= bytes.size(); ++offset)
		if (std::equal(JOURNAL_MAGIC.begin(), JOURNAL_MAGIC.end(), bytes.begin() + offset))
			return offset;
	return bytes.size();
}

scan_result scan_journal()
{
	scan_result scanned;
	player_save_journal_result read_result = player_save_journal_result::ok;
	const std::vector<uint8_t> bytes = read_journal_file(read_result);
	if (read_result != player_save_journal_result::ok)
	{
		scanned.result = read_result;
		return scanned;
	}
	size_t offset = 0;
	while (offset < bytes.size() && scanned.frames.size() < PLAYER_SAVE_JOURNAL_MAX_RECORDS)
	{
		if (bytes.size() - offset < JOURNAL_HEADER_SIZE ||
		    !std::equal(JOURNAL_MAGIC.begin(), JOURNAL_MAGIC.end(), bytes.begin() + offset))
		{
			const size_t next = find_next_magic(bytes, offset + 1);
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, next - offset);
			++health.corrupt_records;
			offset = next;
			continue;
		}
		const uint32_t version = get_u32(bytes.data() + offset, 8);
		const uint32_t header_size = get_u32(bytes.data() + offset, 12);
		const uint64_t record_size = get_u64(bytes.data() + offset, 16);
		const uint32_t payload_size = get_u32(bytes.data() + offset, 64);
		if (header_size != JOURNAL_HEADER_SIZE || record_size < JOURNAL_HEADER_SIZE ||
		    record_size > JOURNAL_MAX_RECORD_SIZE ||
		    payload_size != record_size - header_size)
		{
			const size_t next = find_next_magic(bytes, offset + 1);
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, next - offset);
			++health.corrupt_records;
			offset = next;
			continue;
		}
		if (record_size > bytes.size() - offset)
		{
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, bytes.size() - offset);
			++health.corrupt_records;
			offset = bytes.size();
			break;
		}
		if (version != JOURNAL_FORMAT_VERSION)
		{
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, record_size);
			++health.unsupported_records;
			offset += record_size;
			continue;
		}
		const uint32_t expected = get_u32(bytes.data() + offset, JOURNAL_CHECKSUM_OFFSET);
		if (expected != frame_checksum(bytes.data() + offset, record_size))
		{
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, record_size);
			++health.corrupt_records;
			offset += record_size;
			continue;
		}
		journal_frame frame;
		frame.bytes.assign(bytes.begin() + offset, bytes.begin() + offset + record_size);
		frame.record_id = get_u64(bytes.data() + offset, 24);
		frame.created_msec = get_u64(bytes.data() + offset, 32);
		frame.payload_checksum =
			payload_checksum(bytes.data() + offset + header_size, payload_size);
		const auto decoded = player_snapshot_decode(bytes.data() + offset + header_size,
							    payload_size, &frame.snapshot);
		if (decoded != player_snapshot_codec_result::ok ||
		    static_cast<uint32_t>(frame.snapshot.pid) !=
			    get_u32(bytes.data() + offset, 40) ||
		    frame.snapshot.schema_version != get_u32(bytes.data() + offset, 44) ||
		    frame.snapshot.revision != get_u64(bytes.data() + offset, 48) ||
		    frame.snapshot.components != get_u64(bytes.data() + offset, 56))
		{
			scanned.quarantine_ok &=
				append_quarantine(bytes.data() + offset, record_size);
			++health.corrupt_records;
		}
		else
			scanned.frames.push_back(std::move(frame));
		offset += record_size;
	}
	if (offset < bytes.size())
	{
		scanned.result = player_save_journal_result::replay_blocked;
		++health.backpressure;
	}
	if (!scanned.quarantine_ok)
		scanned.result = player_save_journal_result::io_failure;
	return scanned;
}

scan_result scan_journal_safe()
{
	try
	{
		return scan_journal();
	}
	catch (const std::bad_alloc &)
	{
		scan_result scanned;
		scanned.result = player_save_journal_result::io_failure;
		return scanned;
	}
}

bool write_compacted(const std::vector<journal_frame> &frames)
{
	const std::string temporary = journal_directory + "/" + JOURNAL_TEMP_NAME;
	const int fd = open_safe_file(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (fd < 0)
		return false;
	bool ok = true;
	for (const journal_frame &frame : frames)
		if (!write_all(fd, frame.bytes.data(), frame.bytes.size()))
		{
			ok = false;
			break;
		}
	if (ok)
		ok = fdatasync(fd) == 0;
	if (close(fd) != 0)
		ok = false;
	if (ok)
		ok = rename(temporary.c_str(), journal_path.c_str()) == 0;
	if (ok)
		ok = sync_directory();
	if (!ok)
		unlink(temporary.c_str());
	return ok;
}

void refresh_health(const std::vector<journal_frame> &frames)
{
	health.records = frames.size();
	health.bytes = 0;
	oldest_record_msec = 0;
	const uint64_t now = realtime_msec();
	for (const journal_frame &frame : frames)
	{
		health.bytes += frame.bytes.size();
		if (!oldest_record_msec || frame.created_msec < oldest_record_msec)
			oldest_record_msec = frame.created_msec;
	}
	health.oldest_age_msec =
		oldest_record_msec && now >= oldest_record_msec ? now - oldest_record_msec : 0;
	health.age_limit_exceeded = health.oldest_age_msec > PLAYER_SAVE_JOURNAL_MAX_AGE_MSEC;
	health.quota_exceeded = health.bytes >= journal_quota;
}
} // namespace

bool player_save_journal_init(const char *directory, size_t quota_bytes)
{
	if (!directory || !*directory || directory[0] != '/' ||
	    quota_bytes < JOURNAL_HEADER_SIZE + 64 || quota_bytes > PLAYER_SAVE_JOURNAL_MAX_BYTES)
		return false;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (health.initialized)
		return false;
	struct stat status = {};
	if (lstat(directory, &status) != 0)
	{
		if (errno != ENOENT || (mkdir(directory, 0700) != 0 && errno != EEXIST))
			return false;
	}
	const int directory_fd = open(directory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0 || fstat(directory_fd, &status) != 0 ||
	    !safe_owned_mode(status, 0700, true) || fchmod(directory_fd, 0700) != 0)
	{
		if (directory_fd >= 0)
			close(directory_fd);
		return false;
	}
	close(directory_fd);
	journal_directory = directory;
	journal_path = journal_directory + "/" + JOURNAL_FILE_NAME;
	quarantine_path = journal_directory + "/" + JOURNAL_QUARANTINE_NAME;
	journal_quota = quota_bytes;
	const std::string temporary = journal_directory + "/" + JOURNAL_TEMP_NAME;
	unlink(temporary.c_str());
	const int fd = open_safe_file(journal_path, O_WRONLY | O_APPEND | O_CREAT, 0600);
	if (fd < 0)
		return false;
	close(fd);
	if (!sync_directory())
		return false;
	health = {};
	health.initialized = true;
	const uint64_t invalid_before = health.corrupt_records + health.unsupported_records;
	scan_result scanned = scan_journal_safe();
	if (scanned.result != player_save_journal_result::ok &&
	    scanned.result != player_save_journal_result::corrupt_data)
	{
		health.initialized = false;
		return false;
	}
	if (health.corrupt_records + health.unsupported_records > invalid_before &&
	    !write_compacted(scanned.frames))
	{
		health.initialized = false;
		return false;
	}
	refresh_health(scanned.frames);
	struct stat file_status = {};
	if (stat(journal_path.c_str(), &file_status) == 0 && file_status.st_size >= 0)
		health.bytes = file_status.st_size;
	return true;
}

void player_save_journal_shutdown(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	health.initialized = false;
	journal_directory.clear();
	journal_path.clear();
	quarantine_path.clear();
	journal_quota = 0;
	oldest_record_msec = 0;
}

player_save_journal_result player_save_journal_append(const player_snapshot &snapshot)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	journal_frame frame;
	try
	{
		if (!build_frame(snapshot, frame))
		{
			++health.append_failures;
			return player_save_journal_result::encode_failure;
		}
	}
	catch (const std::bad_alloc &)
	{
		++health.append_failures;
		return player_save_journal_result::encode_failure;
	}
	const int fd = open_safe_file(journal_path, O_WRONLY | O_APPEND, 0600);
	if (fd < 0)
	{
		++health.append_failures;
		return errno == EPERM ? player_save_journal_result::unsafe_permissions :
					player_save_journal_result::io_failure;
	}
	struct stat status = {};
	if (frame.bytes.size() > journal_quota || fstat(fd, &status) != 0 || status.st_size < 0 ||
	    static_cast<uint64_t>(status.st_size) > journal_quota - frame.bytes.size())
	{
		close(fd);
		++health.append_failures;
		++health.backpressure;
		health.quota_exceeded = true;
		return player_save_journal_result::quota_exceeded;
	}
	const bool ok = write_all(fd, frame.bytes.data(), frame.bytes.size()) && fdatasync(fd) == 0;
	close(fd);
	if (!ok)
	{
		++health.append_failures;
		return player_save_journal_result::io_failure;
	}
	++health.appended;
	++health.records;
	health.bytes += frame.bytes.size();
	if (!oldest_record_msec || frame.created_msec < oldest_record_msec)
		oldest_record_msec = frame.created_msec;
	return player_save_journal_result::ok;
}

player_save_journal_result player_save_journal_checkpoint(int pid,
							  player_revision_t durable_revision)
{
	if (pid <= 0 || !durable_revision)
		return player_save_journal_result::corrupt_data;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	scan_result scanned = scan_journal_safe();
	if (scanned.result != player_save_journal_result::ok)
	{
		++health.checkpoint_failures;
		return scanned.result;
	}
	std::vector<journal_frame> retained;
	try
	{
		for (journal_frame &frame : scanned.frames)
			if (frame.snapshot.pid != pid || frame.snapshot.revision > durable_revision)
				retained.push_back(std::move(frame));
	}
	catch (const std::bad_alloc &)
	{
		++health.checkpoint_failures;
		return player_save_journal_result::io_failure;
	}
	if (!write_compacted(retained))
	{
		++health.checkpoint_failures;
		return player_save_journal_result::io_failure;
	}
	++health.checkpoints;
	refresh_health(retained);
	return player_save_journal_result::ok;
}

player_save_journal_result player_save_journal_replay(player_save_apply_fn apply, void *context)
{
	if (!apply)
		return player_save_journal_result::replay_blocked;
	std::vector<journal_frame> frames;
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		if (!health.initialized)
			return player_save_journal_result::not_initialized;
		scan_result scanned = scan_journal_safe();
		if (scanned.result != player_save_journal_result::ok)
			return scanned.result;
		frames = std::move(scanned.frames);
	}
	try
	{
		std::sort(frames.begin(), frames.end(),
			  [](const auto &left, const auto &right)
			  {
				  if (left.snapshot.pid != right.snapshot.pid)
					  return left.snapshot.pid < right.snapshot.pid;
				  if (left.snapshot.revision != right.snapshot.revision)
					  return left.snapshot.revision < right.snapshot.revision;
				  return left.record_id < right.record_id;
			  });
		std::unordered_set<std::string> identities;
		std::map<int, player_revision_t> acknowledged;
		for (const journal_frame &frame : frames)
		{
			const std::string identity = std::to_string(frame.snapshot.pid) + ":" +
						     std::to_string(frame.snapshot.revision) + ":" +
						     std::to_string(frame.snapshot.components) +
						     ":" + std::to_string(frame.payload_checksum);
			if (!identities.insert(identity).second)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				++health.duplicates;
				continue;
			}
			player_save_apply_result applied = {};
			try
			{
				applied = apply(frame.snapshot, context);
			}
			catch (...)
			{
				return player_save_journal_result::replay_blocked;
			}
			if (applied.outcome != player_save_apply_outcome::applied &&
			    applied.outcome != player_save_apply_outcome::already_applied &&
			    applied.outcome != player_save_apply_outcome::stale_revision)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				++health.backpressure;
				return player_save_journal_result::replay_blocked;
			}
			acknowledged[frame.snapshot.pid] = std::max(
				acknowledged[frame.snapshot.pid], applied.durable_revision);
			std::lock_guard<std::mutex> lock(journal_mutex);
			++health.replayed;
		}
		for (const auto &[pid, revision] : acknowledged)
			if (revision && player_save_journal_checkpoint(pid, revision) !=
						player_save_journal_result::ok)
				return player_save_journal_result::io_failure;
	}
	catch (const std::bad_alloc &)
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		++health.backpressure;
		return player_save_journal_result::replay_blocked;
	}
	return player_save_journal_result::ok;
}

player_save_journal_health player_save_journal_health_copy(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	player_save_journal_health snapshot = health;
	const uint64_t now = realtime_msec();
	snapshot.oldest_age_msec =
		oldest_record_msec && now >= oldest_record_msec ? now - oldest_record_msec : 0;
	snapshot.age_limit_exceeded = snapshot.oldest_age_msec > PLAYER_SAVE_JOURNAL_MAX_AGE_MSEC;
	return snapshot;
}

bool player_save_journal_worker_append(const player_snapshot &snapshot, void *context)
{
	(void)context;
	return player_save_journal_append(snapshot) == player_save_journal_result::ok;
}

bool player_save_journal_worker_ack(int pid, player_revision_t revision, void *context)
{
	(void)context;
	return player_save_journal_checkpoint(pid, revision) == player_save_journal_result::ok;
}
