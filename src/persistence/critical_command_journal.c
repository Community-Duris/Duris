#include "persistence/critical_command_journal.h"

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
constexpr uint32_t JOURNAL_NATIVE_VERSION = 2;
constexpr size_t JOURNAL_NATIVE_PREFIX_SIZE = 20;
constexpr uint16_t JOURNAL_NATIVE_PAYLOAD_VERSION = 12;
constexpr size_t JOURNAL_HEADER_SIZE = 40;
constexpr const char *JOURNAL_FILE = "critical-command.journal";
constexpr const char *JOURNAL_TEMP = "critical-command.journal.tmp";

struct journal_frame
{
	bool native = false;
	uint64_t native_revision = 0;
	critical_native_recovery_phase native_phase =
		critical_native_recovery_phase::execution_pending;
	std::vector<uint8_t> native_attachment;
	critical_operation_id operation_id;
	critical_command command;
	std::vector<uint8_t> bytes;
};

std::mutex journal_mutex;
std::string journal_directory;
std::string journal_path;
size_t journal_quota = 0;
critical_command_journal_health health = {};

// Allocated before rename. An uncertain rewrite may only be confirmed against
// this exact attempted transition and the complete resulting journal image.
struct native_rewrite_attempt
{
	bool active = false;
	bool retirement = false;
	std::vector<uint8_t> expected, successor, postimage, retiring_child;
};
native_rewrite_attempt native_rewrite_uncertain;
bool journal_has_native = false;

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

bool native_phase_valid(critical_native_recovery_phase phase)
{
	return phase == critical_native_recovery_phase::execution_pending ||
	       phase == critical_native_recovery_phase::continuation_pending;
}

bool native_command_valid(const critical_command &command)
{
	// Domain/source/body authentication belongs to the typed recovery owner.
	// No item/SHOP provider dependency or legacy execution predicate is widened.
	const bool route = (command.type == critical_command_type::item_transfer &&
			    command.payload_version == JOURNAL_NATIVE_PAYLOAD_VERSION) ||
			   (command.type == critical_command_type::native_mobile_birth &&
			    (command.payload_version == 2 || command.payload_version == 3));
	return route && command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.publication_required && critical_command_envelope_valid(command);
}

uint32_t native_checksum(const uint8_t *data, size_t size)
{
	// Cover version, lengths, operation identity and all recovery bytes. The
	// stored checksum field is treated as zero; CCJ1/version1 CRC is unchanged.
	const uint8_t zero[4] = {};
	uLong checksum = crc32(0, data, 20);
	checksum = crc32(checksum, zero, sizeof(zero));
	return static_cast<uint32_t>(crc32(checksum, data + 24, size - 24));
}

bool build_native_frame(const critical_native_recovery_envelope &envelope, journal_frame *frame)
{
	if (!frame || !native_command_valid(envelope.command) || !envelope.revision ||
	    !native_phase_valid(envelope.phase) || envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	std::vector<uint8_t> command;
	if (critical_command_encode(envelope.command, &command) !=
		    critical_command_codec_result::ok ||
	    command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
		return false;
	try
	{
		journal_frame built;
		const size_t payload_size =
			JOURNAL_NATIVE_PREFIX_SIZE + command.size() + envelope.attachment.size();
		built.bytes.reserve(JOURNAL_HEADER_SIZE + payload_size);
		built.bytes.insert(built.bytes.end(), JOURNAL_MAGIC, JOURNAL_MAGIC + 4);
		append_le<uint32_t>(built.bytes, JOURNAL_NATIVE_VERSION);
		append_le<uint64_t>(built.bytes, JOURNAL_HEADER_SIZE + payload_size);
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(payload_size));
		append_le<uint32_t>(built.bytes, 0);
		built.bytes.insert(built.bytes.end(), envelope.command.operation_id.bytes.begin(),
				   envelope.command.operation_id.bytes.end());
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(command.size()));
		append_le<uint64_t>(built.bytes, envelope.revision);
		append_le<uint8_t>(built.bytes, static_cast<uint8_t>(envelope.phase));
		append_le<uint8_t>(built.bytes, 0);
		append_le<uint16_t>(built.bytes, 0);
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(envelope.attachment.size()));
		built.bytes.insert(built.bytes.end(), command.begin(), command.end());
		built.bytes.insert(built.bytes.end(), envelope.attachment.begin(),
				   envelope.attachment.end());
		const uint32_t checksum = native_checksum(built.bytes.data(), built.bytes.size());
		for (size_t index = 0; index < sizeof(checksum); ++index)
			built.bytes[20 + index] = static_cast<uint8_t>(checksum >> (index * 8));
		built.operation_id = envelope.command.operation_id;
		built.command = envelope.command;
		built.native = true;
		built.native_revision = envelope.revision;
		built.native_phase = envelope.phase;
		built.native_attachment = envelope.attachment;
		*frame = std::move(built);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool decode_native_payload(const uint8_t *payload, size_t size, journal_frame *frame)
{
	if (!frame || size < JOURNAL_NATIVE_PREFIX_SIZE)
		return false;
	size_t cursor = 0;
	uint32_t command_size = 0, attachment_size = 0;
	uint64_t revision = 0;
	uint8_t phase = 0, reserved8 = 0;
	uint16_t reserved16 = 0;
	if (!read_le(payload, size, &cursor, &command_size) ||
	    !read_le(payload, size, &cursor, &revision) ||
	    !read_le(payload, size, &cursor, &phase) ||
	    !read_le(payload, size, &cursor, &reserved8) ||
	    !read_le(payload, size, &cursor, &reserved16) ||
	    !read_le(payload, size, &cursor, &attachment_size) || reserved8 || reserved16 ||
	    !revision || !native_phase_valid(static_cast<critical_native_recovery_phase>(phase)) ||
	    !command_size || command_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    !attachment_size || attachment_size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
	    command_size > size - cursor || attachment_size != size - cursor - command_size)
		return false;
	critical_command command{};
	std::vector<uint8_t> canonical;
	if (critical_command_decode(payload + cursor, command_size, &command) !=
		    critical_command_codec_result::ok ||
	    !native_command_valid(command) ||
	    critical_command_encode(command, &canonical) != critical_command_codec_result::ok ||
	    canonical.size() != command_size ||
	    !std::equal(canonical.begin(), canonical.end(), payload + cursor))
		return false;
	cursor += command_size;
	frame->native_attachment.assign(payload + cursor, payload + size);
	frame->command = std::move(command);
	frame->native_revision = revision;
	frame->native_phase = static_cast<critical_native_recovery_phase>(phase);
	frame->native = true;
	return true;
}

critical_command_journal_result scan(std::vector<journal_frame> *frames,
				     size_t *physical_records_output = nullptr)
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
	std::unordered_map<std::string, bool> seen_native;
	size_t physical_records = 0;
	bool contains_native = false;
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
		    (version != JOURNAL_VERSION && version != JOURNAL_NATIVE_VERSION) ||
		    record_size != JOURNAL_HEADER_SIZE + payload_size ||
		    record_size > data.size() - offset)
			return critical_command_journal_result::corrupt_data;
		critical_operation_id operation_id = {};
		memcpy(operation_id.bytes.data(), data.data() + cursor, operation_id.bytes.size());
		cursor += operation_id.bytes.size();
		const uint8_t *payload = data.data() + cursor;
		journal_frame frame;
		if (version == JOURNAL_NATIVE_VERSION)
		{
			if (payload_size > JOURNAL_NATIVE_PREFIX_SIZE +
						   CRITICAL_COMMAND_MAX_ENCODED_BYTES +
						   CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
			    native_checksum(data.data() + offset,
					    static_cast<size_t>(record_size)) != checksum ||
			    !decode_native_payload(payload, payload_size, &frame))
				return critical_command_journal_result::corrupt_data;
		}
		else
		{
			if (crc32(0, payload, payload_size) != checksum)
				return critical_command_journal_result::corrupt_data;
			if (critical_command_decode(payload, payload_size, &frame.command) !=
			    critical_command_codec_result::ok)
				return critical_command_journal_result::corrupt_data;
		}
		if (!critical_operation_id_equal(operation_id, frame.command.operation_id))
			return critical_command_journal_result::corrupt_data;
		const std::string key(reinterpret_cast<const char *>(operation_id.bytes.data()),
				      operation_id.bytes.size());
		++physical_records;
		contains_native = contains_native || frame.native;
		if (contains_native && physical_records > CRITICAL_COMMAND_JOURNAL_MAX_RECORDS)
			return critical_command_journal_result::corrupt_data;
		std::vector<uint8_t> encoded(payload, payload + payload_size);
		auto prior = seen.find(key);
		if (prior != seen.end())
		{
			if (frame.native || seen_native.at(key) || prior->second != encoded)
				return critical_command_journal_result::corrupt_data;
			++health.duplicates;
			offset += static_cast<size_t>(record_size);
			continue;
		}
		seen.emplace(key, encoded);
		seen_native.emplace(key, frame.native);
		frame.operation_id = operation_id;
		frame.bytes.assign(data.begin() + offset, data.begin() + offset + record_size);
		frames->push_back(std::move(frame));
		offset += static_cast<size_t>(record_size);
	}
	if (physical_records_output)
		*physical_records_output = physical_records;
	return critical_command_journal_result::ok;
}

critical_command_journal_result scan_bounded(std::vector<journal_frame> *frames,
					     size_t *physical_records_output = nullptr)
{
	try
	{
		return scan(frames, physical_records_output);
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
	else if (result == critical_command_journal_result::io_failure ||
		 result == critical_command_journal_result::append_uncertain)
		++health.io_failures;
	else if (result == critical_command_journal_result::quota_exceeded)
		health.quota_exceeded = true;
}

critical_command_journal_result rewrite(const std::vector<journal_frame> &frames,
					bool *renamed = nullptr)
{
	if (renamed)
		*renamed = false;
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
	if (renamed)
		*renamed = true;
	const int directory_fd =
		open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (directory_fd < 0 || fsync(directory_fd) != 0)
	{
		if (directory_fd >= 0)
			close(directory_fd);
		return critical_command_journal_result::io_failure;
	}
	const bool directory_closed = close(directory_fd) == 0;
	if (renamed && !directory_closed)
		return critical_command_journal_result::io_failure;
	return critical_command_journal_result::ok;
}

void update_health(const std::vector<journal_frame> &frames)
{
	health.records = frames.size();
	health.bytes = 0;
	journal_has_native = false;
	uint64_t oldest = 0;
	for (const journal_frame &frame : frames)
	{
		health.bytes += frame.bytes.size();
		journal_has_native = journal_has_native || frame.native;
		if (!oldest || frame.command.accepted_at_usec / 1000 < oldest)
			oldest = frame.command.accepted_at_usec / 1000;
	}
	health.oldest_age_msec = oldest && now_msec() > oldest ? now_msec() - oldest : 0;
	health.quota_exceeded = health.bytes >= journal_quota;
}

critical_command_journal_result
confirm_native_rewrite(const std::vector<journal_frame> &frames, const journal_frame &expected,
		       const journal_frame *successor,
		       const journal_frame *retiring_child = nullptr)
{
	if (!native_rewrite_uncertain.active ||
	    native_rewrite_uncertain.expected != expected.bytes ||
	    native_rewrite_uncertain.retirement != (successor == nullptr) ||
	    (retiring_child ? native_rewrite_uncertain.retiring_child != retiring_child->bytes :
			      !native_rewrite_uncertain.retiring_child.empty()) ||
	    (successor && native_rewrite_uncertain.successor != successor->bytes))
		return critical_command_journal_result::invalid;
	struct stat status
	{
	};
	if (stat(journal_path.c_str(), &status) != 0 || status.st_size < 0 ||
	    static_cast<uint64_t>(status.st_size) != native_rewrite_uncertain.postimage.size())
		return critical_command_journal_result::append_uncertain;
	size_t offset = 0;
	for (const auto &frame : frames)
	{
		if (offset > native_rewrite_uncertain.postimage.size() ||
		    frame.bytes.size() > native_rewrite_uncertain.postimage.size() - offset ||
		    !std::equal(frame.bytes.begin(), frame.bytes.end(),
				native_rewrite_uncertain.postimage.begin() + offset))
			return critical_command_journal_result::append_uncertain;
		offset += frame.bytes.size();
	}
	if (offset != native_rewrite_uncertain.postimage.size())
		return critical_command_journal_result::append_uncertain;
	const int fd = open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd < 0)
		return critical_command_journal_result::append_uncertain;
	const bool synced = fsync(fd) == 0;
	const bool closed = close(fd) == 0;
	if (!synced || !closed)
		return critical_command_journal_result::append_uncertain;
	native_rewrite_uncertain = {};
	health.append_uncertain = false;
	++health.checkpoints;
	update_health(frames);
	health.last_result = critical_command_journal_result::ok;
	return critical_command_journal_result::ok;
}

bool rollback_append(off_t original_size)
{
	const int fd = open(journal_path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
		return false;
	bool ok = ftruncate(fd, original_size) == 0;
	if (ok && fsync(fd) != 0)
		ok = false;
	if (close(fd) != 0)
		ok = false;
	return ok;
}
bool native_directory_sync()
{
	const int fd = open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd < 0)
		return false;
	const bool synced = fsync(fd) == 0;
	const bool closed = close(fd) == 0;
	return synced && closed;
}

critical_command_journal_result append_native_frame(const journal_frame &frame)
{
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
	const off_t original_size = status.st_size;
	if (frame.bytes.size() > journal_quota ||
	    static_cast<uint64_t>(original_size) > journal_quota - frame.bytes.size())
	{
		close(fd);
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
	const bool wrote = write_all(fd, frame.bytes.data(), frame.bytes.size());
	const bool synced = wrote && fsync(fd) == 0;
	const bool closed = close(fd) == 0;
	// File durability alone does not establish a freshly created or renamed
	// journal name. Confirm the current directory entry before admission succeeds.
	const bool directory_synced = wrote && synced && closed && native_directory_sync();
	if (!wrote || !synced || !closed || !directory_synced)
	{
		// A complete frame may already exist. Definite refusal requires both
		// the original file truncation and the current namespace confirmed durable.
		// Any unconfirmed cleanup retains uncertain admission and its original owner.
		const bool rolled_back = rollback_append(original_size) && native_directory_sync();
		const auto result = rolled_back ? critical_command_journal_result::io_failure :
						  critical_command_journal_result::append_uncertain;
		if (result == critical_command_journal_result::append_uncertain)
			health.append_uncertain = true;
		record_result(result);
		return result;
	}
	++health.appends;
	++health.records;
	health.bytes += frame.bytes.size();
	health.last_result = critical_command_journal_result::ok;
	return critical_command_journal_result::ok;
}

critical_command_journal_result
transition_native_record(const critical_native_recovery_envelope &expected,
			 const critical_native_recovery_envelope *successor,
			 const critical_native_recovery_envelope *retiring_child = nullptr)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	try
	{
		journal_frame old_frame, new_frame, child_frame;
		if (!build_native_frame(expected, &old_frame) ||
		    (retiring_child &&
		     (!build_native_frame(*retiring_child, &child_frame) ||
		      retiring_child->phase !=
			      critical_native_recovery_phase::continuation_pending ||
		      expected.phase != critical_native_recovery_phase::continuation_pending ||
		      critical_operation_id_equal(expected.command.operation_id,
						  retiring_child->command.operation_id))) ||
		    (successor &&
		     (!build_native_frame(*successor, &new_frame) ||
		      expected.revision == UINT64_MAX ||
		      successor->revision != expected.revision + 1 ||
		      !critical_command_equal(expected.command, successor->command) ||
		      (expected.phase == critical_native_recovery_phase::continuation_pending &&
		       successor->phase != expected.phase))) ||
		    (!successor &&
		     expected.phase != critical_native_recovery_phase::continuation_pending))
			return critical_command_journal_result::invalid;
		if (health.append_uncertain && !native_rewrite_uncertain.active)
			return critical_command_journal_result::append_uncertain;
		std::vector<journal_frame> frames;
		auto scanned = scan_bounded(&frames);
		if (scanned != critical_command_journal_result::ok)
		{
			record_result(scanned);
			return scanned;
		}
		if (native_rewrite_uncertain.active)
		{
			const auto result = confirm_native_rewrite(
				frames, old_frame, successor ? &new_frame : nullptr,
				retiring_child ? &child_frame : nullptr);
			record_result(result);
			return result;
		}
		auto found =
			std::find_if(frames.begin(), frames.end(),
				     [&](const journal_frame &frame) {
					     return critical_operation_id_equal(
						     frame.operation_id, old_frame.operation_id);
				     });
		if (found == frames.end() || !found->native || found->bytes != old_frame.bytes)
			return critical_command_journal_result::invalid;
		if (retiring_child)
		{
			const auto child = std::find_if(frames.begin(), frames.end(),
							[&](const journal_frame &frame) {
								return critical_operation_id_equal(
									frame.operation_id,
									child_frame.operation_id);
							});
			if (child == frames.end() || !child->native ||
			    child->bytes != child_frame.bytes)
				return critical_command_journal_result::invalid;
		}
		if (successor)
			*found = std::move(new_frame);
		else
			frames.erase(found);
		if (retiring_child)
		{
			// Re-find after vector replacement/erase; the exact child was checked
			// before either mutation, under this same original journal mutex.
			const auto child = std::find_if(frames.begin(), frames.end(),
							[&](const journal_frame &frame) {
								return critical_operation_id_equal(
									frame.operation_id,
									child_frame.operation_id);
							});
			frames.erase(child);
		}
		native_rewrite_attempt attempt;
		attempt.expected = old_frame.bytes;
		if (retiring_child)
			attempt.retiring_child = child_frame.bytes;
		if (successor)
		{
			auto replacement = std::find_if(frames.begin(), frames.end(),
							[&](const journal_frame &frame) {
								return critical_operation_id_equal(
									frame.operation_id,
									old_frame.operation_id);
							});
			attempt.successor = replacement->bytes;
		}
		attempt.retirement = !successor;
		size_t total = 0;
		for (const auto &frame : frames)
		{
			if (total > journal_quota || frame.bytes.size() > journal_quota - total)
				return critical_command_journal_result::quota_exceeded;
			total += frame.bytes.size();
		}
		attempt.postimage.reserve(total);
		for (const auto &frame : frames)
			attempt.postimage.insert(attempt.postimage.end(), frame.bytes.begin(),
						 frame.bytes.end());
		bool renamed = false;
		const auto result = rewrite(frames, &renamed);
		if (result != critical_command_journal_result::ok && renamed)
		{
			attempt.active = true;
			native_rewrite_uncertain = std::move(attempt);
			health.append_uncertain = true;
			record_result(critical_command_journal_result::append_uncertain);
			return critical_command_journal_result::append_uncertain;
		}
		if (result == critical_command_journal_result::ok)
		{
			++health.checkpoints;
			update_health(frames);
		}
		record_result(result);
		return result;
	}
	catch (const std::bad_alloc &)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
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
	native_rewrite_uncertain = {};
	journal_has_native = false;
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
	native_rewrite_uncertain = {};
	journal_has_native = false;
	journal_directory.clear();
	journal_path.clear();
	journal_quota = 0;
}

critical_command_journal_result critical_command_journal_append(const critical_command &command)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	if (health.append_uncertain)
	{
		health.last_result = critical_command_journal_result::append_uncertain;
		return critical_command_journal_result::append_uncertain;
	}
	if (journal_has_native)
	{
		std::vector<journal_frame> originals;
		size_t physical_records = 0;
		const auto result = scan_bounded(&originals, &physical_records);
		if (result != critical_command_journal_result::ok)
		{
			record_result(result);
			return result;
		}
		if (physical_records >= CRITICAL_COMMAND_JOURNAL_MAX_RECORDS)
			return critical_command_journal_result::quota_exceeded;
		for (const auto &original : originals)
			if (original.native && critical_operation_id_equal(original.operation_id,
									   command.operation_id))
				return critical_command_journal_result::invalid;
	}
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
	const off_t original_size = status.st_size;
	if (frame.bytes.size() > journal_quota ||
	    static_cast<uint64_t>(original_size) > journal_quota - frame.bytes.size())
	{
		close(fd);
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
	const bool wrote = write_all(fd, frame.bytes.data(), frame.bytes.size());
	const bool synced = wrote && fsync(fd) == 0;
	const bool closed = close(fd) == 0;
	if (!wrote || !synced || !closed)
	{
		// A failed append may already have written a complete frame. Roll it back
		// while the journal mutex is held; only an unsuccessful rollback is
		// admission-uncertain and must not be treated as a clean rejection.
		const bool rolled_back = rollback_append(original_size);
		const auto result = rolled_back ? critical_command_journal_result::io_failure :
						  critical_command_journal_result::append_uncertain;
		if (result == critical_command_journal_result::append_uncertain)
			health.append_uncertain = true;
		record_result(result);
		return result;
	}
	++health.appends;
	++health.records;
	health.bytes += frame.bytes.size();
	health.last_result = critical_command_journal_result::ok;
	return critical_command_journal_result::ok;
}

critical_command_journal_result critical_command_journal_sync(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	// File fsync alone cannot settle an uncertain native directory rename.
	if (native_rewrite_uncertain.active)
		return critical_command_journal_result::append_uncertain;
	const int fd = open(journal_path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
	{
		record_result(critical_command_journal_result::io_failure);
		return critical_command_journal_result::io_failure;
	}
	bool ok = fsync(fd) == 0;
	if (close(fd) != 0)
		ok = false;
	if (!ok)
	{
		record_result(critical_command_journal_result::io_failure);
		return critical_command_journal_result::io_failure;
	}
	std::vector<journal_frame> frames;
	const auto scan_result = scan_bounded(&frames);
	if (scan_result != critical_command_journal_result::ok)
	{
		record_result(scan_result);
		return scan_result;
	}
	update_health(frames);
	// A mixed journal may contain a complete uncertain native admission on a
	// fresh/renamed inode. Generic sync remains durability-only, but cannot clear
	// that uncertainty before the current namespace is confirmed too.
	if (journal_has_native && !native_directory_sync())
	{
		record_result(critical_command_journal_result::io_failure);
		return critical_command_journal_result::io_failure;
	}
	health.append_uncertain = false;
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
	if (native_rewrite_uncertain.active)
		return critical_command_journal_result::append_uncertain;
	for (const auto &frame : frames)
		if (frame.native && critical_operation_id_equal(frame.operation_id, operation_id))
			return critical_command_journal_result::invalid;
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
		if (native_rewrite_uncertain.active)
			return critical_command_journal_result::append_uncertain;
		// Preflight before any legacy callback, never discard an attachment.
		for (const auto &frame : frames)
			if (frame.native)
				return critical_command_journal_result::replay_blocked;
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

critical_command_journal_result
critical_command_journal_append_native_recovery(const critical_native_recovery_envelope &envelope)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	if (health.append_uncertain || native_rewrite_uncertain.active)
		return critical_command_journal_result::replay_blocked;
	try
	{
		journal_frame frame;
		if (envelope.revision != 1 ||
		    envelope.phase != critical_native_recovery_phase::execution_pending ||
		    !build_native_frame(envelope, &frame))
			return critical_command_journal_result::invalid;
		std::vector<journal_frame> originals;
		size_t physical_records = 0;
		const auto scanned = scan_bounded(&originals, &physical_records);
		if (scanned != critical_command_journal_result::ok)
		{
			record_result(scanned);
			return scanned;
		}
		if (physical_records >= CRITICAL_COMMAND_JOURNAL_MAX_RECORDS)
			return critical_command_journal_result::quota_exceeded;
		for (const auto &original : originals)
			if (critical_operation_id_equal(original.operation_id, frame.operation_id))
				return critical_command_journal_result::invalid;
		update_health(originals);
		const auto result = append_native_frame(frame);
		// Even an uncertain append may have written a valid native frame.
		if (result == critical_command_journal_result::ok ||
		    result == critical_command_journal_result::append_uncertain)
			journal_has_native = true;
		return result;
	}
	catch (const std::bad_alloc &)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
}

critical_command_journal_result
critical_command_journal_sync_native_recovery(const critical_native_recovery_envelope &expected)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	if (native_rewrite_uncertain.active)
		return critical_command_journal_result::append_uncertain;
	try
	{
		journal_frame initial;
		if (expected.revision != 1 ||
		    expected.phase != critical_native_recovery_phase::execution_pending ||
		    !build_native_frame(expected, &initial))
			return critical_command_journal_result::invalid;
		std::vector<journal_frame> originals;
		const auto scanned = scan_bounded(&originals);
		if (scanned != critical_command_journal_result::ok)
		{
			record_result(scanned);
			return scanned;
		}
		const auto found =
			std::find_if(originals.begin(), originals.end(),
				     [&](const journal_frame &frame) {
					     return critical_operation_id_equal(
						     frame.operation_id, initial.operation_id);
				     });
		if (found == originals.end() || !found->native || found->bytes != initial.bytes)
			return critical_command_journal_result::invalid;
		const int fd = open(journal_path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
		if (fd < 0)
			return critical_command_journal_result::io_failure;
		const bool synced = fsync(fd) == 0;
		const bool closed = close(fd) == 0;
		if (!synced || !closed)
			return critical_command_journal_result::io_failure;
		const int directory_fd =
			open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
		if (directory_fd < 0)
			return critical_command_journal_result::io_failure;
		const bool directory_synced = fsync(directory_fd) == 0;
		const bool directory_closed = close(directory_fd) == 0;
		if (!directory_synced || !directory_closed)
			return critical_command_journal_result::io_failure;
		update_health(originals);
		health.append_uncertain = false;
		health.last_result = critical_command_journal_result::ok;
		return critical_command_journal_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
}

critical_command_journal_result
critical_command_journal_replace_native_recovery(const critical_native_recovery_envelope &expected,
						 const critical_native_recovery_envelope &successor)
{
	return transition_native_record(expected, &successor);
}

critical_command_journal_result
critical_command_journal_retire_native_recovery(const critical_native_recovery_envelope &expected)
{
	return transition_native_record(expected, nullptr);
}

critical_command_journal_result critical_command_journal_transition_native_pair(
	const critical_native_recovery_envelope &expected_parent,
	const critical_native_recovery_envelope &expected_child,
	const critical_native_recovery_envelope *parent_successor)
{
	return transition_native_record(expected_parent, parent_successor, &expected_child);
}

critical_command_journal_result
critical_command_journal_replay_with_native(critical_command_replay_fn legacy_replay,
					    critical_native_recovery_replay_fn native_replay,
					    void *context)
{
	if (!legacy_replay || !native_replay)
		return critical_command_journal_result::invalid;
	std::vector<journal_frame> frames;
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		if (!health.initialized)
			return critical_command_journal_result::not_initialized;
		// Complete bytes from an uncertain append are not confirmed admission.
		// Exact native sync must settle them before any owner registration callback.
		if (health.append_uncertain || native_rewrite_uncertain.active)
			return critical_command_journal_result::append_uncertain;
		const auto result = scan_bounded(&frames);
		if (result != critical_command_journal_result::ok)
		{
			record_result(result);
			return result;
		}
		++health.replays;
	}
	for (auto &frame : frames)
	{
		bool accepted;
		if (frame.native)
		{
			critical_native_recovery_envelope envelope;
			envelope.command = std::move(frame.command);
			envelope.revision = frame.native_revision;
			envelope.phase = frame.native_phase;
			envelope.attachment = std::move(frame.native_attachment);
			accepted = native_replay(std::move(envelope), context);
		}
		else
			accepted = legacy_replay(std::move(frame.command), context);
		if (!accepted)
		{
			std::lock_guard<std::mutex> lock(journal_mutex);
			health.last_result = critical_command_journal_result::replay_blocked;
			return critical_command_journal_result::replay_blocked;
		}
	}
	std::lock_guard<std::mutex> lock(journal_mutex);
	health.last_result = critical_command_journal_result::ok;
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
	case critical_command_journal_result::append_uncertain:
		return "append_uncertain";
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
	native_rewrite_uncertain = {};
	journal_has_native = false;
}
