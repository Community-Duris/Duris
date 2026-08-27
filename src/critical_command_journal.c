#include "critical_command_journal.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <zlib.h>

namespace
{
constexpr unsigned char JOURNAL_MAGIC[4] = { 'C', 'C', 'J', '1' };
constexpr uint32_t JOURNAL_VERSION = 1;
constexpr size_t JOURNAL_HEADER_SIZE = 40;
constexpr const char *JOURNAL_FILE = "critical-command.journal";
constexpr const char *JOURNAL_TEMP = "critical-command.journal.tmp";

struct journal_frame
{
	critical_operation_id operation_id;
	critical_command command;
	std::vector<uint8_t> bytes;
};

std::mutex journal_mutex;
std::string journal_directory;
std::string journal_path;
size_t journal_quota = 0;
critical_command_journal_health health = {};

uint64_t now_msec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
					     std::chrono::system_clock::now().time_since_epoch())
					     .count());
}

template <typename T> void append_le(std::vector<uint8_t> &output, T value)
{
	for (size_t index = 0; index < sizeof(T); ++index)
		output.push_back(static_cast<uint8_t>(static_cast<uint64_t>(value) >> (index * 8)));
}

template <typename T> bool read_le(const uint8_t *input, size_t size, size_t *offset, T *value)
{
	if (*offset > size || sizeof(T) > size - *offset)
		return false;
	uint64_t decoded = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		decoded |= static_cast<uint64_t>(input[*offset + index]) << (index * 8);
	*offset += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

bool write_all(int fd, const uint8_t *data, size_t size)
{
	size_t offset = 0;
	while (offset < size)
	{
		const ssize_t written = write(fd, data + offset, size - offset);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return false;
		offset += static_cast<size_t>(written);
	}
	return true;
}

bool safe_regular(const std::string &path, mode_t maximum_mode)
{
	struct stat status = {};
	return lstat(path.c_str(), &status) == 0 && S_ISREG(status.st_mode) &&
	       status.st_uid == geteuid() && status.st_nlink == 1 &&
	       !((status.st_mode & 0777) & ~maximum_mode);
}

bool safe_directory(const std::string &path)
{
	struct stat status = {};
	return lstat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode) &&
	       status.st_uid == geteuid() && !(status.st_mode & 0077);
}

bool build_frame(const critical_command &command, journal_frame *frame)
{
	std::vector<uint8_t> payload;
	if (!frame ||
	    critical_command_encode(command, &payload) != critical_command_codec_result::ok)
		return false;
	try
	{
		frame->bytes.clear();
		frame->bytes.reserve(JOURNAL_HEADER_SIZE + payload.size());
		frame->bytes.insert(frame->bytes.end(), JOURNAL_MAGIC, JOURNAL_MAGIC + 4);
		append_le<uint32_t>(frame->bytes, JOURNAL_VERSION);
		append_le<uint64_t>(frame->bytes, JOURNAL_HEADER_SIZE + payload.size());
		append_le<uint32_t>(frame->bytes, static_cast<uint32_t>(payload.size()));
		append_le<uint32_t>(frame->bytes, crc32(0, payload.data(), payload.size()));
		frame->bytes.insert(frame->bytes.end(), command.operation_id.bytes.begin(),
				    command.operation_id.bytes.end());
		frame->bytes.insert(frame->bytes.end(), payload.begin(), payload.end());
		frame->operation_id = command.operation_id;
		frame->command = command;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

critical_command_journal_result scan(std::vector<journal_frame> *frames)
{
	if (!frames || !safe_regular(journal_path, 0600))
		return critical_command_journal_result::unsafe_permissions;
	const int fd = open(journal_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
		return critical_command_journal_result::io_failure;
	struct stat status = {};
	if (fstat(fd, &status) != 0 || status.st_size < 0 ||
	    static_cast<uint64_t>(status.st_size) > journal_quota)
	{
		close(fd);
		return critical_command_journal_result::quota_exceeded;
	}
	std::vector<uint8_t> data;
	try
	{
		data.resize(static_cast<size_t>(status.st_size));
	}
	catch (const std::bad_alloc &)
	{
		close(fd);
		return critical_command_journal_result::quota_exceeded;
	}
	size_t read_offset = 0;
	while (read_offset < data.size())
	{
		const ssize_t count =
			read(fd, data.data() + read_offset, data.size() - read_offset);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			break;
		read_offset += static_cast<size_t>(count);
	}
	close(fd);
	if (read_offset != data.size())
		return critical_command_journal_result::io_failure;
	size_t offset = 0;
	std::unordered_map<std::string, std::vector<uint8_t>> seen;
	while (offset < data.size())
	{
		if (frames->size() >= CRITICAL_COMMAND_JOURNAL_MAX_RECORDS ||
		    data.size() - offset < JOURNAL_HEADER_SIZE ||
		    memcmp(data.data() + offset, JOURNAL_MAGIC, 4) != 0)
			return critical_command_journal_result::corrupt_data;
		size_t cursor = offset + 4;
		uint32_t version = 0, payload_size = 0, checksum = 0;
		uint64_t record_size = 0;
		if (!read_le(data.data(), data.size(), &cursor, &version) ||
		    !read_le(data.data(), data.size(), &cursor, &record_size) ||
		    !read_le(data.data(), data.size(), &cursor, &payload_size) ||
		    !read_le(data.data(), data.size(), &cursor, &checksum) ||
		    version != JOURNAL_VERSION ||
		    record_size != JOURNAL_HEADER_SIZE + payload_size ||
		    record_size > data.size() - offset)
			return critical_command_journal_result::corrupt_data;
		critical_operation_id operation_id = {};
		memcpy(operation_id.bytes.data(), data.data() + cursor, operation_id.bytes.size());
		cursor += operation_id.bytes.size();
		const uint8_t *payload = data.data() + cursor;
		if (crc32(0, payload, payload_size) != checksum)
			return critical_command_journal_result::corrupt_data;
		critical_command command = {};
		if (critical_command_decode(payload, payload_size, &command) !=
			    critical_command_codec_result::ok ||
		    !critical_operation_id_equal(operation_id, command.operation_id))
			return critical_command_journal_result::corrupt_data;
		const std::string key(reinterpret_cast<const char *>(operation_id.bytes.data()),
				      operation_id.bytes.size());
		std::vector<uint8_t> encoded(payload, payload + payload_size);
		auto prior = seen.find(key);
		if (prior != seen.end())
		{
			if (prior->second != encoded)
				return critical_command_journal_result::corrupt_data;
			++health.duplicates;
			offset += static_cast<size_t>(record_size);
			continue;
		}
		seen.emplace(key, encoded);
		journal_frame frame;
		frame.operation_id = operation_id;
		frame.command = std::move(command);
		frame.bytes.assign(data.begin() + offset, data.begin() + offset + record_size);
		frames->push_back(std::move(frame));
		offset += static_cast<size_t>(record_size);
	}
	return critical_command_journal_result::ok;
}

critical_command_journal_result scan_bounded(std::vector<journal_frame> *frames)
{
	try
	{
		return scan(frames);
	}
	catch (const std::bad_alloc &)
	{
		return critical_command_journal_result::quota_exceeded;
	}
}

void record_result(critical_command_journal_result result)
{
	health.last_result = result;
	if (result == critical_command_journal_result::corrupt_data)
		++health.corrupt_records;
	else if (result == critical_command_journal_result::io_failure)
		++health.io_failures;
	else if (result == critical_command_journal_result::quota_exceeded)
		health.quota_exceeded = true;
}

critical_command_journal_result rewrite(const std::vector<journal_frame> &frames)
{
	const std::string temporary = journal_directory + "/" + JOURNAL_TEMP;
	const int fd = open(temporary.c_str(),
			    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return critical_command_journal_result::io_failure;
	bool ok = fchmod(fd, 0600) == 0;
	for (const journal_frame &frame : frames)
		if (!write_all(fd, frame.bytes.data(), frame.bytes.size()))
			ok = false;
	if (ok && fsync(fd) != 0)
		ok = false;
	if (close(fd) != 0)
		ok = false;
	if (!ok || rename(temporary.c_str(), journal_path.c_str()) != 0)
	{
		unlink(temporary.c_str());
		return critical_command_journal_result::io_failure;
	}
	const int directory_fd =
		open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (directory_fd < 0 || fsync(directory_fd) != 0)
	{
		if (directory_fd >= 0)
			close(directory_fd);
		return critical_command_journal_result::io_failure;
	}
	close(directory_fd);
	return critical_command_journal_result::ok;
}

void update_health(const std::vector<journal_frame> &frames)
{
	health.records = frames.size();
	health.bytes = 0;
	uint64_t oldest = 0;
	for (const journal_frame &frame : frames)
	{
		health.bytes += frame.bytes.size();
		if (!oldest || frame.command.accepted_at_usec / 1000 < oldest)
			oldest = frame.command.accepted_at_usec / 1000;
	}
	health.oldest_age_msec = oldest && now_msec() > oldest ? now_msec() - oldest : 0;
	health.quota_exceeded = health.bytes >= journal_quota;
}
} // namespace

bool critical_command_journal_init(const char *directory, size_t quota_bytes)
{
	if (!directory || !*directory || !quota_bytes)
		return false;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (health.initialized)
		return false;
	health = {};
	journal_directory = directory;
	if (mkdir(directory, 0700) != 0 && errno != EEXIST)
	{
		health.last_result = critical_command_journal_result::io_failure;
		++health.io_failures;
		return false;
	}
	if (!safe_directory(journal_directory))
	{
		health.last_result = critical_command_journal_result::unsafe_permissions;
		return false;
	}
	journal_path = journal_directory + "/" + JOURNAL_FILE;
	journal_quota = quota_bytes;
	const int fd = open(journal_path.c_str(),
			    O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
	{
		health.last_result = critical_command_journal_result::io_failure;
		++health.io_failures;
		return false;
	}
	close(fd);
	if (!safe_regular(journal_path, 0600))
	{
		health.last_result = critical_command_journal_result::unsafe_permissions;
		return false;
	}
	health.initialized = true;
	std::vector<journal_frame> frames;
	const auto result = scan_bounded(&frames);
	if (result != critical_command_journal_result::ok)
	{
		health.initialized = false;
		record_result(result);
		return false;
	}
	health.last_result = critical_command_journal_result::ok;
	update_health(frames);
	return true;
}

void critical_command_journal_shutdown(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	health.initialized = false;
	journal_directory.clear();
	journal_path.clear();
	journal_quota = 0;
}

critical_command_journal_result critical_command_journal_append(const critical_command &command)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	journal_frame frame;
	if (!build_frame(command, &frame))
	{
		record_result(critical_command_journal_result::invalid);
		return critical_command_journal_result::invalid;
	}
	if (health.records >= CRITICAL_COMMAND_JOURNAL_MAX_RECORDS)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
	struct stat status = {};
	const int fd = open(journal_path.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0 || fstat(fd, &status) != 0 || status.st_size < 0)
	{
		if (fd >= 0)
			close(fd);
		record_result(critical_command_journal_result::io_failure);
		return critical_command_journal_result::io_failure;
	}
	if (frame.bytes.size() > journal_quota ||
	    static_cast<uint64_t>(status.st_size) > journal_quota - frame.bytes.size())
	{
		close(fd);
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
	bool ok = write_all(fd, frame.bytes.data(), frame.bytes.size());
	if (ok && fsync(fd) != 0)
		ok = false;
	if (close(fd) != 0)
		ok = false;
	if (!ok)
	{
		record_result(critical_command_journal_result::io_failure);
		return critical_command_journal_result::io_failure;
	}
	++health.appends;
	++health.records;
	health.bytes += frame.bytes.size();
	health.last_result = critical_command_journal_result::ok;
	return critical_command_journal_result::ok;
}

critical_command_journal_result
critical_command_journal_checkpoint(const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	if (critical_operation_id_is_zero(operation_id))
		return critical_command_journal_result::invalid;
	std::vector<journal_frame> frames;
	auto result = scan_bounded(&frames);
	if (result != critical_command_journal_result::ok)
	{
		record_result(result);
		return result;
	}
	frames.erase(std::remove_if(frames.begin(), frames.end(),
				    [&](const journal_frame &frame) {
					    return critical_operation_id_equal(frame.operation_id,
									       operation_id);
				    }),
		     frames.end());
	result = rewrite(frames);
	if (result == critical_command_journal_result::ok)
	{
		++health.checkpoints;
		update_health(frames);
		health.last_result = critical_command_journal_result::ok;
	}
	else
		record_result(result);
	return result;
}

critical_command_journal_result critical_command_journal_replay(critical_command_replay_fn replay,
								void *context)
{
	if (!replay)
		return critical_command_journal_result::invalid;
	std::vector<journal_frame> frames;
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		if (!health.initialized)
			return critical_command_journal_result::not_initialized;
		const auto result = scan_bounded(&frames);
		if (result != critical_command_journal_result::ok)
		{
			record_result(result);
			return result;
		}
		++health.replays;
	}
	for (journal_frame &frame : frames)
		if (!replay(std::move(frame.command), context))
		{
			std::lock_guard<std::mutex> lock(journal_mutex);
			health.last_result = critical_command_journal_result::replay_blocked;
			return critical_command_journal_result::replay_blocked;
		}
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		health.last_result = critical_command_journal_result::ok;
	}
	return critical_command_journal_result::ok;
}

critical_command_journal_health critical_command_journal_health_copy(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	return health;
}

const char *critical_command_journal_result_name(critical_command_journal_result result)
{
	switch (result)
	{
	case critical_command_journal_result::ok:
		return "ready";
	case critical_command_journal_result::not_initialized:
		return "stopped";
	case critical_command_journal_result::invalid:
		return "invalid";
	case critical_command_journal_result::io_failure:
		return "io_failure";
	case critical_command_journal_result::unsafe_permissions:
		return "unsafe_permissions";
	case critical_command_journal_result::quota_exceeded:
		return "full";
	case critical_command_journal_result::corrupt_data:
		return "corrupt";
	case critical_command_journal_result::replay_blocked:
		return "replay_blocked";
	}
	return "unknown";
}

void critical_command_journal_reset_for_tests(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	journal_directory.clear();
	journal_path.clear();
	journal_quota = 0;
	health = {};
}
