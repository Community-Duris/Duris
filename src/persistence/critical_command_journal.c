#include <atomic>
#include "item/held_retirement_transport.h"
#include "item/native_quest_transport.h"
#include "persistence/critical_command_journal.h"
#include "economy/native_mobile_birth_recovery.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/zone_reset_item_command.h"

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
std::atomic<size_t> native_rewrite_storage{ 0 };
// Unavailable until a genuine journal-mutex owner publishes the complete census.
std::atomic<size_t> journal_persistent_storage{ SIZE_MAX };
void publish_native_rewrite_storage() noexcept;
void publish_journal_persistent_storage() noexcept;
bool journal_has_native = false;

// Ordinary initialization can throw during a string assignment. Publish the
// actual surviving persistent capacities before its real mutex owner exits.
struct journal_persistent_storage_snapshot
{
	~journal_persistent_storage_snapshot() noexcept { publish_journal_persistent_storage(); }
};

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
	const bool route =
		held_retirement_transport_command(command) ||
		native_quest_transport_command(command) ||
		(command.type == critical_command_type::native_mobile_birth &&
		 (command.payload_version == 2 || command.payload_version == 3 ||
		  command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)) ||
		(command.type == critical_command_type::auction && command.payload_version == 2) ||
		(command.type == critical_command_type::zone_reset_item_birth &&
		 (command.payload_version == ZONE_RESET_ITEM_PAYLOAD_VERSION ||
		  command.payload_version == ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION));
	return route && command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.publication_required && critical_command_envelope_valid(command);
}

// Version-4 framing carries the full original native recovery envelope.
// This is transport validation, never bare-command execution/admission support.
// Conservative SHOP selection prevents malformed shared bytes from falling
// through to the ordinary policy; both codecs authenticate canonical contents.
bool native_cash_role_envelope_valid(const critical_native_recovery_envelope &envelope) noexcept
{
	if (envelope.command.type != critical_command_type::native_mobile_birth ||
	    envelope.command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
		return true;
	const bool shared = std::any_of(envelope.command.keys.begin(), envelope.command.keys.end(),
					[](const auto &key)
					{ return key.type == critical_entity_type::shopkeeper; });
	return shared ? native_mobile_birth_shared_shop_recovery_valid(envelope) :
			native_mobile_birth_cash_role_recovery_valid(envelope);
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
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !native_phase_valid(envelope.phase) || envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
	    !native_cash_role_envelope_valid(envelope))
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
	    (held_retirement_transport_command(command) &&
	     static_cast<critical_native_recovery_phase>(phase) !=
		     critical_native_recovery_phase::execution_pending) ||
	    critical_command_encode(command, &canonical) != critical_command_codec_result::ok ||
	    canonical.size() != command_size ||
	    !std::equal(canonical.begin(), canonical.end(), payload + cursor))
		return false;
	cursor += command_size;
	if (command.type == critical_command_type::native_mobile_birth &&
	    command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
	{
		// Move the already decoded command and retain only the original bounded
		// attachment once; no second command/attachment copy is introduced.
		critical_native_recovery_envelope original;
		original.command = std::move(command);
		original.revision = revision;
		original.phase = static_cast<critical_native_recovery_phase>(phase);
		original.attachment.assign(payload + cursor, payload + size);
		if (!native_cash_role_envelope_valid(original))
			return false;
		frame->native_attachment = std::move(original.attachment);
		frame->command = std::move(original.command);
	}
	else
	{
		frame->native_attachment.assign(payload + cursor, payload + size);
		frame->command = std::move(command);
	}
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
	publish_native_rewrite_storage();
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
			 const critical_native_recovery_envelope *retiring_child = nullptr,
			 bool held_execution_retirement = false)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
	try
	{
		journal_frame old_frame, new_frame, child_frame;
		if ((held_execution_retirement &&
		     (successor || retiring_child ||
		      expected.phase != critical_native_recovery_phase::execution_pending ||
		      !held_retirement_transport_command(expected.command))) ||
		    !build_native_frame(expected, &old_frame) ||
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
		    (!successor && !held_execution_retirement &&
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
			publish_native_rewrite_storage();
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
	journal_persistent_storage_snapshot persistent_snapshot;
	if (health.initialized)
		return false;
	health = {};
	native_rewrite_uncertain = {};
	publish_native_rewrite_storage();
	journal_has_native = false;
	journal_directory = directory;
	publish_journal_persistent_storage();
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
	publish_journal_persistent_storage();
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
	publish_native_rewrite_storage();
	journal_has_native = false;
	journal_directory.clear();
	journal_path.clear();
	journal_quota = 0;
	publish_journal_persistent_storage();
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

critical_command_journal_result critical_held_retirement_journal_owner::retire_execution(
	const critical_native_recovery_envelope &expected)
{
	return transition_native_record(expected, nullptr, nullptr, true);
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
	publish_native_rewrite_storage();
	journal_has_native = false;
	publish_journal_persistent_storage();
}

#include "persistence/critical_command_coordinator.h"

namespace
{
bool journal_admit_add(size_t &bytes, size_t amount) noexcept
{
	if (bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
	    amount > CRITICAL_COORDINATOR_MAX_BYTES - bytes)
		return false;
	bytes += amount;
	return true;
}
bool journal_admit_array(size_t &bytes, size_t count, size_t unit) noexcept
{
	return (!unit || count <= CRITICAL_COORDINATOR_MAX_BYTES / unit) &&
	       journal_admit_add(bytes, count * unit);
}
bool journal_command_heap(const critical_command &command, bool fresh, size_t &bytes) noexcept
{
	return journal_admit_array(bytes, fresh ? command.keys.size() : command.keys.capacity(),
				   sizeof(critical_entity_key)) &&
	       journal_admit_array(bytes,
				   fresh ? command.expected_revisions.size() :
					   command.expected_revisions.capacity(),
				   sizeof(critical_expected_revision)) &&
	       journal_admit_add(bytes,
				 fresh ? command.payload.size() : command.payload.capacity()) &&
	       journal_admit_add(bytes, fresh ? command.accounting_intent.size() :
						command.accounting_intent.capacity());
}
bool journal_frame_heap(const journal_frame &frame, size_t &bytes) noexcept
{
	return journal_command_heap(frame.command, false, bytes) &&
	       journal_admit_add(bytes, frame.bytes.capacity()) &&
	       journal_admit_add(bytes, frame.native_attachment.capacity());
}
bool journal_persistent_storage_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}

// Only actual journal-mutex owners call this provider. Neither readers nor the
// shared budget dereference these mutable strings or vectors outside that lock.
void publish_journal_persistent_storage() noexcept
{
	size_t bytes = SIZE_MAX;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	// Actual installed GCC13 char-string SSO capacity is 15; allocated storage
	// is capacity+1 including the terminator. All attempt vectors use uint8_t.
	bytes = 0;
	if (!journal_persistent_storage_add(bytes, sizeof(journal_mutex)) ||
	    !journal_persistent_storage_add(bytes, sizeof(journal_directory)) ||
	    !journal_persistent_storage_add(bytes, sizeof(journal_path)) ||
	    !journal_persistent_storage_add(bytes, sizeof(journal_quota)) ||
	    !journal_persistent_storage_add(bytes, sizeof(health)) ||
	    !journal_persistent_storage_add(bytes, sizeof(journal_has_native)) ||
	    !journal_persistent_storage_add(bytes, sizeof(native_rewrite_uncertain)) ||
	    !journal_persistent_storage_add(bytes, sizeof(native_rewrite_storage)) ||
	    !journal_persistent_storage_add(bytes, sizeof(journal_persistent_storage)) ||
	    journal_directory.capacity() == SIZE_MAX || journal_path.capacity() == SIZE_MAX ||
	    (journal_directory.capacity() > 15 &&
	     !journal_persistent_storage_add(bytes, journal_directory.capacity() + 1)) ||
	    (journal_path.capacity() > 15 &&
	     !journal_persistent_storage_add(bytes, journal_path.capacity() + 1)) ||
	    !journal_persistent_storage_add(bytes, native_rewrite_uncertain.expected.capacity()) ||
	    !journal_persistent_storage_add(bytes, native_rewrite_uncertain.successor.capacity()) ||
	    !journal_persistent_storage_add(bytes, native_rewrite_uncertain.postimage.capacity()) ||
	    !journal_persistent_storage_add(bytes,
					    native_rewrite_uncertain.retiring_child.capacity()))
		bytes = SIZE_MAX;
#endif
	journal_persistent_storage.store(bytes, std::memory_order_release);
}

void publish_native_rewrite_storage() noexcept
{
	// Called only by the original journal-mutex owner. Readers observe this
	// scalar without taking journal/coordinator locks or dereferencing vectors.
	size_t bytes = native_rewrite_uncertain.active ? sizeof(native_rewrite_attempt) : 0;
	if (native_rewrite_uncertain.expected.capacity() > SIZE_MAX - bytes)
		bytes = SIZE_MAX;
	else
		bytes += native_rewrite_uncertain.expected.capacity();
	if (native_rewrite_uncertain.successor.capacity() > SIZE_MAX - bytes)
		bytes = SIZE_MAX;
	else
		bytes += native_rewrite_uncertain.successor.capacity();
	if (native_rewrite_uncertain.postimage.capacity() > SIZE_MAX - bytes)
		bytes = SIZE_MAX;
	else
		bytes += native_rewrite_uncertain.postimage.capacity();
	if (native_rewrite_uncertain.retiring_child.capacity() > SIZE_MAX - bytes)
		bytes = SIZE_MAX;
	else
		bytes += native_rewrite_uncertain.retiring_child.capacity();
	native_rewrite_storage.store(bytes, std::memory_order_release);
	// Preserve the original projection, then publish one complete persistent
	// term from this same genuine mutex-held state.
	publish_journal_persistent_storage();
}
struct journal_admission_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	bool rejected = false;
	static bool admit(size_t peak, void *opaque) noexcept
	{
		auto &self = *static_cast<journal_admission_budget *>(opaque);
		if (!self.reserve(peak, self.context))
		{
			self.rejected = true;
			return false;
		}
		return true;
	}
};
bool journal_cash_role_valid_admitted(const critical_native_recovery_envelope &envelope,
				      journal_admission_budget &budget, size_t live) noexcept
{
	if (envelope.command.type != critical_command_type::native_mobile_birth ||
	    envelope.command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
		return true;
	const bool shared = std::any_of(envelope.command.keys.begin(), envelope.command.keys.end(),
					[](const auto &key)
					{ return key.type == critical_entity_type::shopkeeper; });
	return shared ? native_mobile_birth_shared_shop_recovery_valid_bounded(
				envelope, journal_admission_budget::admit, &budget, live) :
			native_mobile_birth_cash_role_recovery_valid_bounded(
				envelope, journal_admission_budget::admit, &budget, live);
}
struct journal_native_build_workspace
{
	std::vector<uint8_t> command;
	journal_frame built;
};
bool journal_build_native_admitted(const critical_native_recovery_envelope &envelope,
				   journal_frame *output, journal_admission_budget &budget,
				   size_t outer) noexcept
{
	if (!output || !native_command_valid(envelope.command) || !envelope.revision ||
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !native_phase_valid(envelope.phase) || envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	try
	{
		size_t live = outer;
		if (!journal_admit_add(live, sizeof(journal_native_build_workspace)) ||
		    !budget.admit(live, &budget))
			return false;
		journal_native_build_workspace work;
		if (!journal_cash_role_valid_admitted(envelope, budget, live) ||
		    critical_command_encode_bounded(envelope.command, &work.command,
						    journal_admission_budget::admit, &budget,
						    live) != critical_command_codec_result::ok ||
		    work.command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
		    !journal_admit_add(live, work.command.capacity()))
			return false;
		const size_t payload = JOURNAL_NATIVE_PREFIX_SIZE + work.command.size() +
				       envelope.attachment.size();
		if (!journal_admit_add(live, JOURNAL_HEADER_SIZE + payload) ||
		    !journal_command_heap(envelope.command, true, live) ||
		    !journal_admit_add(live, envelope.attachment.size()) ||
		    !budget.admit(live, &budget))
			return false;
		auto &built = work.built;
		built.bytes.reserve(JOURNAL_HEADER_SIZE + payload);
		built.bytes.insert(built.bytes.end(), JOURNAL_MAGIC, JOURNAL_MAGIC + 4);
		append_le<uint32_t>(built.bytes, JOURNAL_NATIVE_VERSION);
		append_le<uint64_t>(built.bytes, JOURNAL_HEADER_SIZE + payload);
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(payload));
		append_le<uint32_t>(built.bytes, 0);
		built.bytes.insert(built.bytes.end(), envelope.command.operation_id.bytes.begin(),
				   envelope.command.operation_id.bytes.end());
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(work.command.size()));
		append_le<uint64_t>(built.bytes, envelope.revision);
		append_le<uint8_t>(built.bytes, static_cast<uint8_t>(envelope.phase));
		append_le<uint8_t>(built.bytes, 0);
		append_le<uint16_t>(built.bytes, 0);
		append_le<uint32_t>(built.bytes, static_cast<uint32_t>(envelope.attachment.size()));
		built.bytes.insert(built.bytes.end(), work.command.begin(), work.command.end());
		built.bytes.insert(built.bytes.end(), envelope.attachment.begin(),
				   envelope.attachment.end());
		size_t checksum_live = live;
		if (!journal_admit_add(checksum_live, 4) || !budget.admit(checksum_live, &budget))
			return false;
		const uint32_t checksum = native_checksum(built.bytes.data(), built.bytes.size());
		for (size_t index = 0; index < sizeof(checksum); ++index)
			built.bytes[20 + index] = static_cast<uint8_t>(checksum >> (index * 8));
		built.operation_id = envelope.command.operation_id;
		built.command = envelope.command;
		built.native = true;
		built.native_revision = envelope.revision;
		built.native_phase = envelope.phase;
		built.native_attachment = envelope.attachment;
		*output = std::move(built);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
struct journal_scan_workspace
{
	std::vector<uint8_t> data;
	std::vector<journal_frame> frames;
};
struct journal_decode_workspace
{
	journal_frame frame;
	std::vector<uint8_t> canonical;
	critical_native_recovery_envelope original;
};
critical_command_journal_result journal_scan_admitted(std::vector<journal_frame> *output,
						      journal_admission_budget &budget,
						      size_t outer,
						      size_t *physical_output = nullptr) noexcept
{
	if (!output)
		return critical_command_journal_result::unsafe_permissions;
	size_t guard_live = outer;
	if (!journal_admit_add(guard_live, sizeof(struct stat)) ||
	    !budget.admit(guard_live, &budget))
		return critical_command_journal_result::quota_exceeded;
	if (!safe_regular(journal_path, 0600))
		return critical_command_journal_result::unsafe_permissions;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	size_t base = outer;
	if (!journal_admit_add(base, sizeof(journal_scan_workspace)) ||
	    !journal_admit_add(base, sizeof(struct stat)) || !budget.admit(base, &budget))
		return critical_command_journal_result::quota_exceeded;
	journal_scan_workspace work;
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
	size_t file_live = base;
	if (!journal_admit_add(file_live, static_cast<size_t>(status.st_size)) ||
	    !budget.admit(file_live, &budget))
	{
		close(fd);
		return critical_command_journal_result::quota_exceeded;
	}
	try
	{
		work.data.resize(static_cast<size_t>(status.st_size));
	}
	catch (...)
	{
		close(fd);
		return critical_command_journal_result::quota_exceeded;
	}
	size_t read_offset = 0;
	while (read_offset < work.data.size())
	{
		const ssize_t count =
			read(fd, work.data.data() + read_offset, work.data.size() - read_offset);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			break;
		read_offset += static_cast<size_t>(count);
	}
	close(fd);
	if (read_offset != work.data.size())
		return critical_command_journal_result::io_failure;
	try
	{
		size_t offset = 0, physical = 0;
		bool contains_native = false;
		while (offset < work.data.size())
		{
			// Match the original pre-parse unique limit, including duplicates.
			if (work.frames.size() >= CRITICAL_COMMAND_JOURNAL_MAX_RECORDS ||
			    work.data.size() - offset < JOURNAL_HEADER_SIZE ||
			    memcmp(work.data.data() + offset, JOURNAL_MAGIC, 4))
				return critical_command_journal_result::corrupt_data;
			size_t cursor = offset + 4;
			uint32_t version = 0, payload_size = 0, checksum = 0;
			uint64_t record_size = 0;
			if (!read_le(work.data.data(), work.data.size(), &cursor, &version) ||
			    !read_le(work.data.data(), work.data.size(), &cursor, &record_size) ||
			    !read_le(work.data.data(), work.data.size(), &cursor, &payload_size) ||
			    !read_le(work.data.data(), work.data.size(), &cursor, &checksum) ||
			    (version != JOURNAL_VERSION && version != JOURNAL_NATIVE_VERSION) ||
			    record_size != JOURNAL_HEADER_SIZE + payload_size ||
			    record_size > work.data.size() - offset)
				return critical_command_journal_result::corrupt_data;
			size_t live = base;
			if (!journal_admit_add(live, work.data.capacity()) ||
			    !journal_admit_array(live, work.frames.capacity(),
						 sizeof(journal_frame)))
				return critical_command_journal_result::quota_exceeded;
			for (const auto &prior : work.frames)
				if (!journal_frame_heap(prior, live))
					return critical_command_journal_result::quota_exceeded;
			if (!journal_admit_add(live, sizeof(journal_decode_workspace)) ||
			    !journal_admit_add(live, sizeof(critical_operation_id)) ||
			    !budget.admit(live, &budget))
				return critical_command_journal_result::quota_exceeded;
			critical_operation_id operation{};
			memcpy(operation.bytes.data(), work.data.data() + cursor,
			       operation.bytes.size());
			cursor += operation.bytes.size();
			const uint8_t *payload = work.data.data() + cursor;
			journal_decode_workspace decoded;
			size_t command_heap = 0;
			if (version == JOURNAL_NATIVE_VERSION)
			{
				size_t checksum_live = live;
				if (!journal_admit_add(checksum_live, 4) ||
				    !budget.admit(checksum_live, &budget))
					return critical_command_journal_result::quota_exceeded;
				if (payload_size >
					    JOURNAL_NATIVE_PREFIX_SIZE +
						    CRITICAL_COMMAND_MAX_ENCODED_BYTES +
						    CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
				    native_checksum(work.data.data() + offset,
						    static_cast<size_t>(record_size)) != checksum ||
				    payload_size < JOURNAL_NATIVE_PREFIX_SIZE)
					return critical_command_journal_result::corrupt_data;
				size_t at = 0;
				uint32_t command_size = 0, attachment_size = 0;
				uint64_t revision = 0;
				uint8_t phase = 0, reserved8 = 0;
				uint16_t reserved16 = 0;
				if (!read_le(payload, payload_size, &at, &command_size) ||
				    !read_le(payload, payload_size, &at, &revision) ||
				    !read_le(payload, payload_size, &at, &phase) ||
				    !read_le(payload, payload_size, &at, &reserved8) ||
				    !read_le(payload, payload_size, &at, &reserved16) ||
				    !read_le(payload, payload_size, &at, &attachment_size) ||
				    reserved8 || reserved16 || !revision ||
				    !native_phase_valid(
					    static_cast<critical_native_recovery_phase>(phase)) ||
				    !command_size ||
				    command_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
				    !attachment_size ||
				    attachment_size >
					    CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
				    command_size > payload_size - at ||
				    attachment_size != payload_size - at - command_size)
					return critical_command_journal_result::corrupt_data;
				if (critical_command_decode_bounded(
					    payload + at, command_size, &decoded.frame.command,
					    journal_admission_budget::admit, &budget, live,
					    &command_heap) != critical_command_codec_result::ok)
					return budget.rejected ?
						       critical_command_journal_result::
							       quota_exceeded :
						       critical_command_journal_result::corrupt_data;
				if (!native_command_valid(decoded.frame.command) ||
				    (held_retirement_transport_command(decoded.frame.command) &&
				     static_cast<critical_native_recovery_phase>(phase) !=
					     critical_native_recovery_phase::execution_pending))
					return critical_command_journal_result::corrupt_data;
				if (!journal_admit_add(live, command_heap) ||
				    critical_command_encode_bounded(
					    decoded.frame.command, &decoded.canonical,
					    journal_admission_budget::admit, &budget,
					    live) != critical_command_codec_result::ok)
					return critical_command_journal_result::quota_exceeded;
				if (decoded.canonical.size() != command_size ||
				    !std::equal(decoded.canonical.begin(), decoded.canonical.end(),
						payload + at))
					return critical_command_journal_result::corrupt_data;
				at += command_size;
				if (!journal_admit_add(live, decoded.canonical.capacity()) ||
				    !journal_admit_add(live, attachment_size) ||
				    !budget.admit(live, &budget))
					return critical_command_journal_result::quota_exceeded;
				decoded.original.command = std::move(decoded.frame.command);
				decoded.original.revision = revision;
				decoded.original.phase =
					static_cast<critical_native_recovery_phase>(phase);
				decoded.original.attachment.assign(payload + at,
								   payload + payload_size);
				if (!journal_cash_role_valid_admitted(decoded.original, budget,
								      live))
					return budget.rejected ?
						       critical_command_journal_result::
							       quota_exceeded :
						       critical_command_journal_result::corrupt_data;
				decoded.frame.command = std::move(decoded.original.command);
				decoded.frame.native_attachment =
					std::move(decoded.original.attachment);
				decoded.frame.native_revision = revision;
				decoded.frame.native_phase =
					static_cast<critical_native_recovery_phase>(phase);
				decoded.frame.native = true;
			}
			else
			{
				if (crc32(0, payload, payload_size) != checksum)
					return critical_command_journal_result::corrupt_data;
				if (critical_command_decode_bounded(
					    payload, payload_size, &decoded.frame.command,
					    journal_admission_budget::admit, &budget, live,
					    &command_heap) != critical_command_codec_result::ok)
					return budget.rejected ?
						       critical_command_journal_result::
							       quota_exceeded :
						       critical_command_journal_result::corrupt_data;
				if (!journal_admit_add(live, command_heap))
					return critical_command_journal_result::quota_exceeded;
			}
			if (!critical_operation_id_equal(operation,
							 decoded.frame.command.operation_id))
				return critical_command_journal_result::corrupt_data;
			++physical;
			contains_native = contains_native || decoded.frame.native;
			if (contains_native && physical > CRITICAL_COMMAND_JOURNAL_MAX_RECORDS)
				return critical_command_journal_result::corrupt_data;
			const auto prior = std::find_if(
				work.frames.begin(), work.frames.end(), [&](const journal_frame &f)
				{ return critical_operation_id_equal(f.operation_id, operation); });
			if (prior != work.frames.end())
			{
				// Exact original payload bytes, not canonical command equality.
				if (decoded.frame.native || prior->native ||
				    prior->bytes.size() != JOURNAL_HEADER_SIZE + payload_size ||
				    !std::equal(prior->bytes.begin() + JOURNAL_HEADER_SIZE,
						prior->bytes.end(), payload))
					return critical_command_journal_result::corrupt_data;
				++health.duplicates;
				offset += static_cast<size_t>(record_size);
				continue;
			}
			if (!journal_admit_add(live, static_cast<size_t>(record_size)) ||
			    !budget.admit(live, &budget))
				return critical_command_journal_result::quota_exceeded;
			decoded.frame.bytes.assign(work.data.begin() + offset,
						   work.data.begin() + offset +
							   static_cast<size_t>(record_size));
			decoded.frame.operation_id = operation;
			if (work.frames.size() == work.frames.capacity())
			{
				const size_t request = work.frames.size() +
						       std::max(work.frames.size(), size_t{ 1 });
				if (!journal_admit_array(live, request, sizeof(journal_frame)) ||
				    !budget.admit(live, &budget))
					return critical_command_journal_result::quota_exceeded;
			}
			work.frames.push_back(std::move(decoded.frame));
			offset += static_cast<size_t>(record_size);
		}
		*output = std::move(work.frames);
		if (physical_output)
			*physical_output = physical;
		return critical_command_journal_result::ok;
	}
	catch (...)
	{
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}
struct journal_retire_workspace
{
	journal_frame expected;
	std::vector<journal_frame> frames;
	native_rewrite_attempt attempt;
	std::string temporary;
};
critical_command_journal_result journal_rewrite_admitted(const std::vector<journal_frame> &frames,
							 const std::string &temporary,
							 bool *renamed) noexcept
{
	*renamed = false;
	const int fd = open(temporary.c_str(),
			    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return critical_command_journal_result::io_failure;
	bool ok = fchmod(fd, 0600) == 0;
	for (const auto &frame : frames)
		if (!write_all(fd, frame.bytes.data(), frame.bytes.size()))
			ok = false;
	if (ok && fsync(fd))
		ok = false;
	if (close(fd))
		ok = false;
	if (!ok || rename(temporary.c_str(), journal_path.c_str()))
	{
		unlink(temporary.c_str());
		return critical_command_journal_result::io_failure;
	}
	*renamed = true;
	const int directory = open(journal_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (directory < 0 || fsync(directory))
	{
		if (directory >= 0)
			close(directory);
		return critical_command_journal_result::io_failure;
	}
	return close(directory) ? critical_command_journal_result::io_failure :
				  critical_command_journal_result::ok;
}
} // namespace

size_t critical_command_journal_native_rewrite_storage_bytes() noexcept
{
	return native_rewrite_storage.load(std::memory_order_acquire);
}

size_t critical_command_journal_persistent_storage_bytes() noexcept
{
	return journal_persistent_storage.load(std::memory_order_acquire);
}
critical_command_journal_result critical_command_journal_retire_native_recovery_bounded(
	const critical_native_recovery_envelope &expected, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer) noexcept
{
	if (!reserve)
		return critical_command_journal_result::quota_exceeded;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	try
	{
		size_t live = outer;
		if (!journal_admit_add(live, sizeof(journal_retire_workspace)) ||
		    !journal_admit_add(live, sizeof(journal_admission_budget)) ||
		    !reserve(live, context))
			return critical_command_journal_result::quota_exceeded;
		journal_admission_budget budget{ reserve, context };
		journal_retire_workspace work;
		if (expected.phase != critical_native_recovery_phase::continuation_pending ||
		    !journal_build_native_admitted(expected, &work.expected, budget, live))
			return budget.rejected ? critical_command_journal_result::quota_exceeded :
						 critical_command_journal_result::invalid;
		if (!journal_frame_heap(work.expected, live))
			return critical_command_journal_result::quota_exceeded;
		if (health.append_uncertain && !native_rewrite_uncertain.active)
			return critical_command_journal_result::append_uncertain;
		const auto scanned = journal_scan_admitted(&work.frames, budget, live);
		if (scanned != critical_command_journal_result::ok)
		{
			record_result(scanned);
			return scanned;
		}
		if (native_rewrite_uncertain.active)
		{
			size_t confirm_live = live;
			if (!journal_admit_array(confirm_live, work.frames.capacity(),
						 sizeof(journal_frame)))
				return critical_command_journal_result::quota_exceeded;
			for (const auto &frame : work.frames)
				if (!journal_frame_heap(frame, confirm_live))
					return critical_command_journal_result::quota_exceeded;
			if (!journal_admit_add(confirm_live, sizeof(struct stat)) ||
			    !journal_admit_add(confirm_live, sizeof(native_rewrite_attempt)) ||
			    !budget.admit(confirm_live, &budget))
				return critical_command_journal_result::quota_exceeded;
			const auto confirmed =
				confirm_native_rewrite(work.frames, work.expected, nullptr);
			record_result(confirmed);
			return confirmed;
		}
		const auto found = std::find_if(work.frames.begin(), work.frames.end(),
						[&](const journal_frame &frame) {
							return critical_operation_id_equal(
								frame.operation_id,
								work.expected.operation_id);
						});
		if (found == work.frames.end() || !found->native ||
		    found->bytes != work.expected.bytes)
			return critical_command_journal_result::invalid;
		work.frames.erase(found);
		if (!journal_admit_array(live, work.frames.capacity(), sizeof(journal_frame)))
			return critical_command_journal_result::quota_exceeded;
		size_t total = 0;
		for (const auto &frame : work.frames)
		{
			if (!journal_frame_heap(frame, live) || total > journal_quota ||
			    frame.bytes.size() > journal_quota - total)
				return critical_command_journal_result::quota_exceeded;
			total += frame.bytes.size();
		}
		const size_t temporary_size =
			journal_directory.size() + 1 + std::strlen(JOURNAL_TEMP);
		if (!journal_admit_add(live, sizeof(native_rewrite_attempt)) ||
		    !journal_admit_add(live, work.expected.bytes.size()) ||
		    !journal_admit_add(live, total) ||
		    (temporary_size > 15 &&
		     !journal_admit_add(live, std::max(temporary_size, size_t{ 30 }) + 1)) ||
		    !budget.admit(live, &budget))
			return critical_command_journal_result::quota_exceeded;
		work.attempt.expected = work.expected.bytes;
		work.attempt.retirement = true;
		work.attempt.postimage.reserve(total);
		for (const auto &frame : work.frames)
			work.attempt.postimage.insert(work.attempt.postimage.end(),
						      frame.bytes.begin(), frame.bytes.end());
		// Explicit reserve has the pinned SSO growth request admitted above.
		work.temporary.reserve(temporary_size);
		work.temporary.append(journal_directory);
		work.temporary.push_back('/');
		work.temporary.append(JOURNAL_TEMP);
		bool renamed = false;
		const auto result = journal_rewrite_admitted(work.frames, work.temporary, &renamed);
		if (result != critical_command_journal_result::ok && renamed)
		{
			work.attempt.active = true;
			native_rewrite_uncertain = std::move(work.attempt);
			publish_native_rewrite_storage();
			health.append_uncertain = true;
			record_result(critical_command_journal_result::append_uncertain);
			return critical_command_journal_result::append_uncertain;
		}
		if (result == critical_command_journal_result::ok)
		{
			++health.checkpoints;
			update_health(work.frames);
		}
		record_result(result);
		return result;
	}
	catch (...)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}

namespace
{
struct journal_replace_workspace
{
	journal_frame expected, successor;
	std::vector<journal_frame> frames;
	native_rewrite_attempt attempt;
	std::string temporary;
};
bool journal_command_equal_admitted(const critical_command &left, const critical_command &right,
				    journal_admission_budget &budget, size_t outer) noexcept
{
	size_t a = 0, b = 0;
	if (critical_command_encoder_working_bytes(left, &a) != critical_command_codec_result::ok ||
	    critical_command_encoder_working_bytes(right, &b) !=
		    critical_command_codec_result::ok ||
	    a < sizeof(std::vector<uint8_t>) || b < sizeof(std::vector<uint8_t>))
		return false;
	size_t first = outer, second = outer;
	if (!journal_admit_add(first, 2 * sizeof(std::vector<uint8_t>)) ||
	    !journal_admit_add(first, a) ||
	    !journal_admit_add(second, 2 * sizeof(std::vector<uint8_t>)) ||
	    !journal_admit_add(second, a - sizeof(std::vector<uint8_t>)) ||
	    !journal_admit_add(second, b) || !budget.admit(std::max(first, second), &budget))
		return false;
	try
	{
		return critical_command_equal(left, right);
	}
	catch (...)
	{
		return false;
	}
}
} // full original equality, not an alternate command identity

critical_command_journal_result critical_command_journal_replace_native_recovery_bounded(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!reserve)
		return critical_command_journal_result::quota_exceeded;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return critical_command_journal_result::not_initialized;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	try
	{
		size_t base = outer;
		if (!journal_admit_add(base, sizeof(journal_replace_workspace)) ||
		    !journal_admit_add(base, sizeof(journal_admission_budget)) ||
		    !reserve(base, context))
			return critical_command_journal_result::quota_exceeded;
		journal_admission_budget budget{ reserve, context };
		journal_replace_workspace work;
		size_t live = base;
		if (!journal_build_native_admitted(expected, &work.expected, budget, live))
			return budget.rejected ? critical_command_journal_result::quota_exceeded :
						 critical_command_journal_result::invalid;
		if (!journal_frame_heap(work.expected, live))
			return critical_command_journal_result::quota_exceeded;
		if (!journal_build_native_admitted(successor, &work.successor, budget, live))
			return budget.rejected ? critical_command_journal_result::quota_exceeded :
						 critical_command_journal_result::invalid;
		if (!journal_frame_heap(work.successor, live))
			return critical_command_journal_result::quota_exceeded;
		if (expected.revision == UINT64_MAX ||
		    successor.revision != expected.revision + 1 ||
		    !journal_command_equal_admitted(expected.command, successor.command, budget,
						    live) ||
		    (expected.phase == critical_native_recovery_phase::continuation_pending &&
		     successor.phase != expected.phase))
			return budget.rejected ? critical_command_journal_result::quota_exceeded :
						 critical_command_journal_result::invalid;
		if (health.append_uncertain && !native_rewrite_uncertain.active)
			return critical_command_journal_result::append_uncertain;
		const auto scanned = journal_scan_admitted(&work.frames, budget, live);
		if (scanned != critical_command_journal_result::ok)
		{
			record_result(scanned);
			return scanned;
		}
		if (native_rewrite_uncertain.active)
		{
			size_t confirm_live = live;
			if (!journal_admit_array(confirm_live, work.frames.capacity(),
						 sizeof(journal_frame)))
				return critical_command_journal_result::quota_exceeded;
			for (const auto &frame : work.frames)
				if (!journal_frame_heap(frame, confirm_live))
					return critical_command_journal_result::quota_exceeded;
			if (!journal_admit_add(confirm_live, sizeof(struct stat)) ||
			    !journal_admit_add(confirm_live, sizeof(native_rewrite_attempt)) ||
			    !budget.admit(confirm_live, &budget))
				return critical_command_journal_result::quota_exceeded;
			const auto confirmed =
				confirm_native_rewrite(work.frames, work.expected, &work.successor);
			record_result(confirmed);
			return confirmed;
		}
		const auto found = std::find_if(work.frames.begin(), work.frames.end(),
						[&](const journal_frame &frame) {
							return critical_operation_id_equal(
								frame.operation_id,
								work.expected.operation_id);
						});
		if (found == work.frames.end() || !found->native ||
		    found->bytes != work.expected.bytes)
			return critical_command_journal_result::invalid;
		*found = std::move(work.successor);
		// Recount actual survivors: successor ownership has moved into the frame
		// vector, while the replaced old frame has been destroyed. No double count.
		live = base;
		if (!journal_frame_heap(work.expected, live) ||
		    !journal_frame_heap(work.successor, live) ||
		    !journal_admit_array(live, work.frames.capacity(), sizeof(journal_frame)))
			return critical_command_journal_result::quota_exceeded;
		size_t total = 0;
		for (const auto &frame : work.frames)
		{
			if (!journal_frame_heap(frame, live) || total > journal_quota ||
			    frame.bytes.size() > journal_quota - total)
				return critical_command_journal_result::quota_exceeded;
			total += frame.bytes.size();
		}
		size_t temporary_size = journal_directory.size();
		if (!journal_admit_add(temporary_size, 1) ||
		    !journal_admit_add(temporary_size, std::strlen(JOURNAL_TEMP)) ||
		    !journal_admit_add(live, sizeof(native_rewrite_attempt)) ||
		    !journal_admit_add(live, work.expected.bytes.size()) ||
		    !journal_admit_add(live, found->bytes.size()) ||
		    !journal_admit_add(live, total) ||
		    (temporary_size > 15 &&
		     !journal_admit_add(live, std::max(temporary_size, size_t{ 30 }) + 1)) ||
		    !budget.admit(live, &budget))
			return critical_command_journal_result::quota_exceeded;
		work.attempt.expected = work.expected.bytes;
		work.attempt.successor = found->bytes;
		work.attempt.retirement = false;
		work.attempt.postimage.reserve(total);
		for (const auto &frame : work.frames)
			work.attempt.postimage.insert(work.attempt.postimage.end(),
						      frame.bytes.begin(), frame.bytes.end());
		work.temporary.reserve(temporary_size);
		work.temporary.append(journal_directory);
		work.temporary.push_back('/');
		work.temporary.append(JOURNAL_TEMP);
		bool renamed = false;
		const auto result = journal_rewrite_admitted(work.frames, work.temporary, &renamed);
		// Every allocation and budget callback precedes the irreversible namespace
		// change. The original exact attempt is promoted by nonallocating moves.
		if (result != critical_command_journal_result::ok && renamed)
		{
			work.attempt.active = true;
			native_rewrite_uncertain = std::move(work.attempt);
			publish_native_rewrite_storage();
			health.append_uncertain = true;
			record_result(critical_command_journal_result::append_uncertain);
			return critical_command_journal_result::append_uncertain;
		}
		if (result == critical_command_journal_result::ok)
		{
			++health.checkpoints;
			update_health(work.frames);
		}
		record_result(result);
		return result;
	}
	catch (...)
	{
		record_result(critical_command_journal_result::quota_exceeded);
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}

#include <type_traits>

namespace
{
// Called only under the actual journal owner. Keep the uncertainty carrier's
// published storage separate; its existing aggregate term owns active inline
// state and all retained attempt vector capacities once.
bool journal_startup_storage_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}

bool journal_startup_current_metadata_locked(size_t *output) noexcept
{
	if (!output)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t bytes = 0;
	if (!journal_startup_storage_add(bytes, sizeof(journal_mutex)) ||
	    !journal_startup_storage_add(bytes, sizeof(journal_directory)) ||
	    !journal_startup_storage_add(bytes, sizeof(journal_path)) ||
	    !journal_startup_storage_add(bytes, sizeof(journal_quota)) ||
	    !journal_startup_storage_add(bytes, sizeof(health)) ||
	    !journal_startup_storage_add(bytes, sizeof(native_rewrite_storage)) ||
	    !journal_startup_storage_add(bytes, sizeof(journal_persistent_storage)) ||
	    !journal_startup_storage_add(bytes, sizeof(journal_has_native)) ||
	    (!native_rewrite_uncertain.active &&
	     !journal_startup_storage_add(bytes, sizeof(native_rewrite_uncertain))) ||
	    journal_directory.capacity() == SIZE_MAX || journal_path.capacity() == SIZE_MAX ||
	    (journal_directory.capacity() > 15 &&
	     !journal_startup_storage_add(bytes, journal_directory.capacity() + 1)) ||
	    (journal_path.capacity() > 15 &&
	     !journal_startup_storage_add(bytes, journal_path.capacity() + 1)))
		return false;
	*output = bytes;
	return true;
#endif
}

bool journal_startup_metadata(size_t &bytes) noexcept
{
	size_t current = 0;
	return journal_startup_current_metadata_locked(&current) &&
	       journal_admit_add(bytes, current);
}

// Real scoped output carrier is destroyed before its actual journal guard.
// It performs no allocation, reserve callback, health update or promotion.
struct journal_startup_metadata_snapshot
{
	size_t *output;
	~journal_startup_metadata_snapshot() noexcept
	{
		publish_journal_persistent_storage();
		(void)journal_startup_current_metadata_locked(output);
	}
};

// All helper observations above require actual journal ownership; this private
// getter creates that ownership for the later root startup caller afresh.
} // metadata helpers

bool critical_startup_journal_budget_owner::current_metadata_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		return journal_startup_current_metadata_locked(output);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{

// Exact installed GCC13 basic_string::_M_create growth. The source-proven
// request is refused explicitly when outside the existing admission ceiling;
// no allocating length_error/bad_alloc is manufactured on refusal.
bool journal_startup_string_request(size_t wanted, size_t old_capacity, size_t *new_capacity,
				    size_t *request) noexcept
{
	if (!new_capacity || !request || wanted > CRITICAL_COORDINATOR_MAX_BYTES ||
	    old_capacity > CRITICAL_COORDINATOR_MAX_BYTES)
		return false;
	if (wanted <= old_capacity)
	{
		*new_capacity = old_capacity;
		*request = 0;
		return true;
	}
	size_t capacity = wanted;
	if (old_capacity > CRITICAL_COORDINATOR_MAX_BYTES / 2)
		return false;
	if (capacity < 2 * old_capacity)
		capacity = 2 * old_capacity;
	if (capacity >= CRITICAL_COORDINATOR_MAX_BYTES)
		return false;
	*new_capacity = capacity;
	*request = capacity + 1;
	return true;
}

struct journal_startup_init_workspace
{
	journal_admission_budget budget;
	std::vector<journal_frame> frames;
	size_t base = 0, live = 0, directory_size = 0, capacity = 0, request = 0;
	size_t first_size = 0, first_capacity = 0, first_request = 0;
	size_t final_size = 0, final_capacity = 0, final_request = 0;
};

struct journal_startup_replay_workspace
{
	journal_admission_budget budget;
	std::vector<journal_frame> frames;
	size_t base = 0, live = 0;
	bool accepted = false;
};

static_assert(std::is_nothrow_move_constructible_v<critical_command> &&
	      std::is_nothrow_move_assignable_v<critical_command> &&
	      std::is_nothrow_move_constructible_v<critical_native_recovery_envelope> &&
	      std::is_nothrow_move_assignable_v<critical_native_recovery_envelope>);

bool journal_startup_frames(const std::vector<journal_frame> &frames, size_t &live) noexcept
{
	if (!journal_admit_array(live, frames.capacity(), sizeof(journal_frame)))
		return false;
	for (const auto &frame : frames)
		if (!journal_frame_heap(frame, live))
			return false;
	return true;
}
} // actual startup ownership and requests

bool critical_command_journal_init_bounded(const char *directory, size_t quota_bytes,
					   bool (*reserve)(size_t, void *) noexcept,
					   void *budget_context, size_t outer_live,
					   size_t *current_journal_metadata_bytes) noexcept
{
	if (!directory || !*directory || !quota_bytes || !reserve)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	try
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
		if (health.initialized)
			return false;
		health = {};
		native_rewrite_uncertain = {};
		publish_native_rewrite_storage();
		journal_has_native = false;
		try
		{
			journal_startup_init_workspace work{ { reserve, budget_context }, {} };
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !journal_admit_add(work.base, sizeof(struct stat)) ||
			    !journal_startup_metadata(work.base) ||
			    !work.budget.admit(work.base, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.directory_size = std::strlen(directory);
			work.live = work.base;
			if (!journal_startup_string_request(work.directory_size,
							    journal_directory.capacity(),
							    &work.capacity, &work.request) ||
			    !journal_admit_add(work.live, work.request) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			journal_directory = directory; // Exact original assignment/request.
			publish_journal_persistent_storage();
			if (mkdir(directory, 0700) != 0 && errno != EEXIST)
			{
				health.last_result = critical_command_journal_result::io_failure;
				++health.io_failures;
				return false;
			}
			if (!safe_directory(journal_directory))
			{
				health.last_result =
					critical_command_journal_result::unsafe_permissions;
				return false;
			}
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !journal_admit_add(work.base, sizeof(struct stat)) ||
			    !journal_startup_metadata(work.base))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.first_size = journal_directory.size();
			work.final_size = work.first_size;
			// Installed lvalue operator+ uses a fresh __str_concat reserve; the
			// following rvalue operator+ appends to that actual result. Admit old
			// and new heaps during append mutation, before the exact expression.
			if (!journal_admit_add(work.first_size, 1) ||
			    !journal_admit_add(work.final_size, 1) ||
			    !journal_admit_add(work.final_size, std::strlen(JOURNAL_FILE)) ||
			    !journal_startup_string_request(work.first_size, 15,
							    &work.first_capacity,
							    &work.first_request) ||
			    !journal_startup_string_request(work.final_size, work.first_capacity,
							    &work.final_capacity,
							    &work.final_request))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.live = work.base;
			if (!journal_admit_add(work.live, 2 * sizeof(std::string)) ||
			    !journal_admit_add(work.live, 2 * sizeof(std::allocator<char>)) ||
			    !journal_admit_add(work.live, work.first_request) ||
			    !journal_admit_add(work.live, work.final_request) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			journal_path = journal_directory + "/" + JOURNAL_FILE;
			publish_journal_persistent_storage();
			journal_quota = quota_bytes;
			const int fd = open(journal_path.c_str(),
					    O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW,
					    0600);
			if (fd < 0)
			{
				health.last_result = critical_command_journal_result::io_failure;
				++health.io_failures;
				return false;
			}
			close(fd);
			if (!safe_regular(journal_path, 0600))
			{
				health.last_result =
					critical_command_journal_result::unsafe_permissions;
				return false;
			}
			health.initialized = true;
			// Concatenation carriers and replaced old path storage have died.
			// Reobserve the actual persistent strings before the complete first scan.
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !journal_startup_metadata(work.base))
			{
				health.initialized = false;
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			const auto result =
				journal_scan_admitted(&work.frames, work.budget, work.base);
			if (result != critical_command_journal_result::ok)
			{
				health.initialized = false;
				record_result(result);
				return false;
			}
			health.last_result = critical_command_journal_result::ok;
			update_health(work.frames);
			return true;
		}
		catch (...)
		{
			health.initialized = false;
			record_result(critical_command_journal_result::quota_exceeded);
			return false;
		}
	}
	catch (...)
	{
		// Real mutex acquisition failed; no unlocked journal mutation.
		return false;
	}
#endif
}

critical_command_journal_result critical_command_journal_replay_with_native_bounded(
	critical_command_replay_bounded_fn legacy_replay,
	critical_native_recovery_replay_bounded_fn native_replay, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes) noexcept
{
	if (!legacy_replay || !native_replay || !reserve)
		return critical_command_journal_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	try
	{
		journal_startup_replay_workspace work{ { reserve, budget_context }, {} };
		{
			std::lock_guard<std::mutex> lock(journal_mutex);
			journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
			if (!health.initialized)
				return critical_command_journal_result::not_initialized;
			// Full original uncertainty refusal precedes every registration.
			if (health.append_uncertain || native_rewrite_uncertain.active)
				return critical_command_journal_result::append_uncertain;
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_startup_metadata(work.base))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			work.live = work.base;
			if (!journal_admit_add(work.live, sizeof(lock)) ||
			    !journal_admit_add(work.live, sizeof(snapshot)) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			const auto result =
				journal_scan_admitted(&work.frames, work.budget, work.live);
			if (result != critical_command_journal_result::ok)
			{
				record_result(result);
				return result;
			}
			++health.replays;
		}
		// Original startup caller keeps coordinator ownership. The journal lock
		// is released exactly before callbacks; no outside coor getter is called.
		for (auto &frame : work.frames)
		{
			if (frame.native)
			{
				work.live = work.base;
				// Local envelope and original by-value parameter coexist. Full
				// current frame heap remains owned once before their nonallocating moves.
				if (!journal_admit_add(
					    work.live,
					    2 * sizeof(critical_native_recovery_envelope)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				critical_native_recovery_envelope envelope;
				envelope.command = std::move(frame.command);
				envelope.revision = frame.native_revision;
				envelope.phase = frame.native_phase;
				envelope.attachment = std::move(frame.native_attachment);
				// Recount actual moved-from frames and authentic current envelope.
				// Its next move transfers this heap to the by-value parameter once.
				work.live = work.base;
				if (!journal_admit_add(
					    work.live,
					    2 * sizeof(critical_native_recovery_envelope)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !journal_command_heap(envelope.command, false, work.live) ||
				    !journal_admit_add(work.live, envelope.attachment.capacity()) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				work.accepted = native_replay(std::move(envelope), original_context,
							      reserve, budget_context, work.live);
			}
			else
			{
				work.live = work.base;
				if (!journal_admit_add(work.live, sizeof(critical_command)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				// Original by-value parameter steals only this frame's command heap.
				// Parameter inline and all other retained frame capacities are admitted.
				work.accepted = legacy_replay(std::move(frame.command),
							      original_context, reserve,
							      budget_context, work.live);
			}
			if (!work.accepted)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				journal_startup_metadata_snapshot snapshot{
					current_journal_metadata_bytes
				};
				health.last_result =
					critical_command_journal_result::replay_blocked;
				return critical_command_journal_result::replay_blocked;
			}
		}
		std::lock_guard<std::mutex> lock(journal_mutex);
		journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
		health.last_result = critical_command_journal_result::ok;
		return critical_command_journal_result::ok;
	}
	catch (...)
	{
		// No callback throws through its noexcept interface. An actual mutex or
		// construction failure grants no registration and performs no unlocked write.
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}

// Additive physical startup journal owner. Original entry points remain exact.
namespace
{
struct journal_physical_metadata_scope
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t metadata;
	const journal_physical_metadata_scope *previous;
};
thread_local const journal_physical_metadata_scope *journal_physical_active_scope = nullptr;

struct journal_physical_scope_restore
{
	const journal_physical_metadata_scope *previous;
	~journal_physical_scope_restore() noexcept { journal_physical_active_scope = previous; }
};

// Source-declared scopes of the unchanged complete persistent census:
// publish local, checked-add inputs/results; vector capacity's this/result;
// string capacity's this/result and its genuine const _M_is_local/_M_data/
// _M_local_data/pointer_to/addressof/__addressof descendants. Calls are sequential.
constexpr size_t journal_physical_current_source_frames =
	sizeof(size_t) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	11 * sizeof(void *) + sizeof(bool) +
	// Atomic store/load: this/value/order/__b and declared returned value;
	// each __b initialization reaches operator&(order,modifier): both enum
	// parameters and returned enum. Store/load builtin ptr/value/order/result.
	4 * sizeof(void *) + 4 * sizeof(size_t) + 8 * sizeof(std::memory_order) +
	2 * sizeof(std::__memory_order_modifier) + 2 * sizeof(int) +
	// Public persistent getter's genuine returned size_t, in addition to the
	// selected atomic load's own returned value.
	sizeof(size_t);
constexpr size_t journal_physical_lock_source_frames =
	// Actual lock_guard ctor this/mutex, destructor this; mutex lock owns
	// this + __e while unlock has only this. Two gthread wrappers each own
	// the mutex pointer/int return and call active_p's int return. The genuine
	// weak glibc active_p alternative declares __gthread_active_ptr once.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(int) +
	2 * (sizeof(void *) + 2 * sizeof(int)) + sizeof(void *) +
	// __throw_system_error(int) and declared pthread lock/unlock boundaries.
	// No fabricated catch reference: both original catches are catch(...).
	sizeof(int) + 2 * (sizeof(void *) + sizeof(int));
constexpr size_t journal_physical_projection_source_frames =
	journal_physical_current_source_frames + journal_physical_lock_source_frames +
	// Public two callback/context pairs, output; outer/frame/total/current;
	// pure getter output/result and checked arithmetic/admission results.
	6 * sizeof(void *) + 4 * sizeof(size_t) + 5 * sizeof(bool) +
	sizeof(std::lock_guard<std::mutex>);
constexpr size_t journal_physical_scope_source_frames =
	sizeof(journal_physical_metadata_scope) + sizeof(journal_physical_scope_restore) +
	sizeof(journal_physical_active_scope) +
	// call: reserve/context,amount/metadata,return. Lookup: exact pair/output,
	// scope pointer,return. Relay: opaque/owner reference,amount,return.
	// Restore destructor this. observe_metadata: this/bytes,observed,return.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + 4 * sizeof(void *) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(bool);

bool journal_physical_call(bool (*reserve)(size_t, void *) noexcept, void *context, size_t amount,
			   size_t metadata) noexcept
{
	if (!reserve)
		return false;
	const journal_physical_metadata_scope scope{ reserve, context, metadata,
						     journal_physical_active_scope };
	const journal_physical_scope_restore restore{ scope.previous };
	journal_physical_active_scope = &scope;
	return reserve(amount, context);
}

struct journal_physical_startup_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t metadata = 0;
	bool metadata_ready = false;
	static bool relay(size_t amount, void *opaque) noexcept
	{
		auto &owner = *static_cast<journal_physical_startup_budget *>(opaque);
		return owner.metadata_ready &&
		       journal_physical_call(owner.reserve, owner.context, amount, owner.metadata);
	}
	bool observe_metadata(size_t &bytes) noexcept
	{
		size_t observed = 0;
		if (!journal_startup_current_metadata_locked(&observed) ||
		    !journal_admit_add(bytes, observed))
			return false;
		metadata = observed;
		metadata_ready = true;
		return true;
	}
};
}

bool critical_command_journal_startup_owned_metadata(bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t *output) noexcept
{
	const auto *scope = journal_physical_active_scope;
	if (!output || !reserve || !scope || scope->reserve != reserve || scope->context != context)
		return false;
	*output = scope->metadata;
	return true;
}

bool critical_command_journal_startup_projection_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(int) != 4 ||
		      sizeof(std::ptrdiff_t) != 8 || sizeof(std::allocator<uint8_t>) != 1 ||
		      sizeof(std::vector<uint8_t>::const_iterator) != sizeof(void *) ||
		      sizeof(std::memory_order) != 4 || sizeof(std::__memory_order_modifier) != 4)
		return false;
	*output = journal_physical_projection_source_frames + journal_physical_scope_source_frames;
	return true;
#else
	return false;
#endif
}

bool critical_command_journal_startup_persistent_projection_bounded(
	bool (*source_reserve)(size_t, void *) noexcept, void *source_context,
	bool (*physical_reserve)(size_t, void *) noexcept, void *physical_context, size_t outer,
	size_t *output) noexcept
{
	if (!source_reserve || !physical_reserve || !output)
		return false;
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(int) != 4 ||
		      sizeof(std::ptrdiff_t) != 8 || sizeof(std::allocator<uint8_t>) != 1 ||
		      sizeof(std::vector<uint8_t>::const_iterator) != sizeof(void *) ||
		      sizeof(std::memory_order) != 4 || sizeof(std::__memory_order_modifier) != 4)
		return false;
	size_t total = outer;
	if (!journal_admit_add(total, journal_physical_projection_source_frames) ||
	    !journal_admit_add(total, journal_physical_scope_source_frames) ||
	    !source_reserve(total, source_context))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		// Publication derives solely from this actual journal owner. No health,
		// generation, file or readiness state changes. Initial SIZE_MAX is not zero.
		publish_journal_persistent_storage();
		const size_t current = critical_command_journal_persistent_storage_bytes();
		if (current == SIZE_MAX ||
		    !journal_physical_call(physical_reserve, physical_context, total, 0))
			return false;
		*output = current;
		return true;
	}
	catch (...)
	{
		return false;
	}
#else
	(void)source_context;
	(void)physical_context;
	(void)outer;
	return false;
#endif
}

namespace
{
constexpr size_t journal_physical_library_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t journal_physical_library_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Actual forward assign body reaches advance(__mid,size()) at vector.tcc340.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t journal_physical_library_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t journal_physical_library_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t journal_physical_library_vector_frames =
	journal_physical_library_allocator_frames + journal_physical_library_copy_frames +
	journal_physical_library_relocate_frames + journal_physical_library_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
template <class T> constexpr size_t journal_physical_library_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<T>) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + journal_physical_library_allocator_frames;
// Genuine empty-vector source for the actual selected T: vector ->
// _Vector_base -> _Vector_impl -> allocator -> new_allocator ->
// _Vector_impl_data. Each owns its actual this pointer; no T element is built.
template <class T> constexpr size_t journal_physical_library_empty_vector_frames =
	6 * sizeof(void *);
// Actual vector(vector&&)=default -> _Vector_base(move)=default ->
// _Vector_impl(move) -> allocator(const&) -> new_allocator(const&) and
// _Vector_impl_data(move), captured stl_vector620/338/154/105 and allocator167.
// Each selected constructor has this/source. _Vector_impl uses two genuine
// std::move calls, each reference argument/returned reference; data reset has
// its actual pointer() null value. This path performs no allocation.
template <class T> constexpr size_t journal_physical_library_move_constructor_frames =
	6 * 2 * sizeof(void *) + 2 * 2 * sizeof(void *) + sizeof(T *);
constexpr size_t journal_physical_library_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t journal_physical_library_string_frames =
	journal_physical_library_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	journal_physical_library_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);

}

// Complete official zlib1.3 and authenticated Ubuntu declared amd64 source.
// No CRC implementation, table, input, result or original call is changed.
// Source-reference carrier bounds do not qualify installed/emitted code.
namespace
{
constexpr size_t journal_physical_crc_wrapper_frames =
	sizeof(uLong) + sizeof(void *) + sizeof(uInt) + sizeof(uLong);
constexpr size_t journal_physical_crc_root_frames =
	sizeof(uLong) + sizeof(void *) + sizeof(z_size_t) + sizeof(uLong);
constexpr size_t journal_physical_crc_word_frames =
	sizeof(uint64_t) + sizeof(int) + sizeof(z_crc_t);
constexpr size_t journal_physical_crc_word_big_frames = 2 * sizeof(uint64_t) + sizeof(int);
constexpr size_t journal_physical_crc_swap_frames = 2 * sizeof(uint64_t);
constexpr size_t journal_physical_crc_braid_common_frames =
	sizeof(size_t) + sizeof(void *) + sizeof(unsigned) + sizeof(int);
constexpr size_t journal_physical_crc_little_five_frames =
	5 * sizeof(z_crc_t) + 5 * sizeof(uint64_t) + journal_physical_crc_word_frames;
constexpr size_t journal_physical_crc_big_five_frames =
	11 * sizeof(uint64_t) +
	(journal_physical_crc_word_big_frames > journal_physical_crc_swap_frames ?
		 journal_physical_crc_word_big_frames :
		 journal_physical_crc_swap_frames);
constexpr size_t journal_physical_crc_default_frames =
	journal_physical_crc_wrapper_frames + journal_physical_crc_root_frames +
	journal_physical_crc_braid_common_frames +
	(journal_physical_crc_little_five_frames > journal_physical_crc_big_five_frames ?
		 journal_physical_crc_little_five_frames :
		 journal_physical_crc_big_five_frames);
// Optional DYNAMIC_CRC_TABLE, without standalone MAKECRCH generation:
// make owns i,j,n,p; braid owns table pointers,n,w,k,i,p,q; x2nmodp's
// n,k,p/return coexists with multmodp's a,b,m,p/return. All these descendants
// finish before the braided CRC block starts. No recursion or private heap.
constexpr size_t journal_physical_crc_mult_frames = 5 * sizeof(z_crc_t);
constexpr size_t journal_physical_crc_x2n_frames =
	sizeof(z_off64_t) + sizeof(unsigned) + 2 * sizeof(z_crc_t);
constexpr size_t journal_physical_crc_braid_table_frames =
	2 * sizeof(void *) + 3 * sizeof(int) + 3 * sizeof(z_crc_t) +
	journal_physical_crc_x2n_frames + journal_physical_crc_mult_frames;
constexpr size_t journal_physical_crc_make_frames =
	3 * sizeof(unsigned) + sizeof(z_crc_t) + journal_physical_crc_braid_table_frames;
constexpr size_t journal_physical_crc_test_frames = sizeof(void *) + 2 * sizeof(int);
// Authentic GNU13 C stdatomic statement expressions, not C++ atomic_base.
// Each macro ends before init(); store follows its return. Include the genuine
// generic builtin pointer/output-pointer/order boundary without inventing an
// external library body or hidden compiler temporary.
constexpr size_t journal_physical_crc_atomic_load_frames =
	sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(void *) + sizeof(int);
constexpr size_t journal_physical_crc_atomic_store_frames =
	sizeof(void *) + sizeof(int) + 2 * sizeof(void *) + sizeof(int);
constexpr size_t journal_physical_crc_atomic_flag_frames =
	sizeof(void *) + sizeof(int) + sizeof(bool);
static_assert(journal_physical_crc_atomic_load_frames <= journal_physical_crc_make_frames &&
	      journal_physical_crc_atomic_store_frames <= journal_physical_crc_make_frames &&
	      journal_physical_crc_atomic_flag_frames <= journal_physical_crc_make_frames);
constexpr size_t journal_physical_crc_once_frames =
	sizeof(void *) + sizeof(void (*)(void)) +
	(journal_physical_crc_make_frames > journal_physical_crc_test_frames ?
		 journal_physical_crc_make_frames :
		 journal_physical_crc_test_frames);
constexpr size_t journal_physical_crc_dynamic_frames = journal_physical_crc_wrapper_frames +
						       journal_physical_crc_root_frames +
						       journal_physical_crc_once_frames;
constexpr size_t journal_physical_crc32_reference_source_frames =
	journal_physical_crc_default_frames > journal_physical_crc_dynamic_frames ?
		journal_physical_crc_default_frames :
		journal_physical_crc_dynamic_frames;
}
bool journal_physical_crc32_source_frame_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(ZLIB_VERNUM) && ZLIB_VERNUM == 0x1300
	if (!output || sizeof(uLong) != 8 || sizeof(void *) != 8 || sizeof(uInt) != 4 ||
	    sizeof(z_crc_t) != 4 || sizeof(size_t) != 8 || sizeof(z_size_t) != 8 ||
	    sizeof(z_off64_t) != 8 || sizeof(void (*)(void)) != 8 || sizeof(bool) != 1)
		return false;
	*output = journal_physical_crc32_reference_source_frames;
	return true;
#else
	(void)output;
	return false;
#endif
}

// Genuine lower source dependencies, deliberately named instead of assigning
// an unrelated copy/SSO/hash allowance. Definitions/source controls must join
// before this prospective journal lane can be selected.
bool critical_command_startup_codec_source_frame_bytes(size_t *) noexcept;
bool journal_physical_crc32_source_frame_bytes(size_t *) noexcept;

namespace
{
// Genuine initial operation objects, independent of SOURCE. Their later
// actual original per-operation admissions own them once; ROOT may query this
// exact prospective peak for its transient pre-mutation entry reservation.
constexpr size_t journal_physical_initial_inline_bytes =
	(sizeof(journal_startup_init_workspace) > sizeof(journal_startup_replay_workspace) ?
		 sizeof(journal_startup_init_workspace) :
		 sizeof(journal_startup_replay_workspace)) +
	sizeof(std::lock_guard<std::mutex>) + sizeof(journal_startup_metadata_snapshot);
// This hidden owner and actual entry/query carriers belong to the journal lane,
// not ROOT outer. Include them in the public complete SOURCE result so the real
// parent can make its transient pre-mutation entry reservation authentically.
constexpr size_t journal_physical_entry_source_frames =
	sizeof(journal_physical_startup_budget) +
	critical_command_journal_startup_source_query_frame_bytes() + sizeof(size_t) +
	2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool);
// Public/init/replay signatures, scalar locals and real helper observations.
// Workspace/vector/envelope objects and their heap remain in the original
// per-operation admissions, rather than being duplicated in this profile.
constexpr size_t journal_physical_startup_declared_source_frames =
	// Init's exact signature: directory/reserve/context/metadata-output,
	// quota/outer, bool return; original fd and scan result. Its actual guard,
	// snapshot/workspace inline are per-operation owned, not source duplicates.
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(int) +
	sizeof(critical_command_journal_result) +
	// Actual journal_startup_metadata_snapshot destructor this parameter.
	sizeof(void *) +
	// Complete original health={} and rewrite={} assignment temporaries are
	// distinct from persistent J. Their four vector default/move/cleanup
	// descendants are genuine byte-vector source controls below.
	sizeof(critical_command_journal_health) + sizeof(native_rewrite_attempt) +
	// Replay's exact signature: legacy/native/original/reserve/context/output,
	// outer and result return; original scan result, actual range/frame
	// references plus hidden begin/end normal iterators. work.accepted is
	// already inside the original replay workspace; no duplicate bool local.
	6 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(critical_command_journal_result) +
	2 * sizeof(void *) + 2 * sizeof(std::vector<journal_frame>::iterator) +
	// scan: output/budget/outer/physical output and genuine guard/base/file
	// prefixes, fd/status/read_offset/count, parsed scalar fields, byte ptrs,
	// unique operation/cursor/prior/find/equality query locals.
	7 * sizeof(void *) + 14 * sizeof(size_t) + sizeof(int) + sizeof(ssize_t) +
	3 * sizeof(uint32_t) + sizeof(uint64_t) + sizeof(bool) + 3 * sizeof(uint32_t) +
	sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint16_t) +
	sizeof(std::vector<journal_frame>::iterator) +
	// scan's nested native validation: envelope reference, shared bool,
	// actual any_of range predicate/capture, route/type/phase helpers.
	6 * sizeof(void *) + 3 * sizeof(bool) + sizeof(critical_native_recovery_phase) +
	// CURRENT frame/command loops and four-vector/attachment/bytes queries;
	// exact add/array helper inputs/results, initializer-list range carriers.
	9 * sizeof(void *) + 6 * sizeof(size_t) + 5 * sizeof(bool) +
	sizeof(std::vector<journal_frame>::const_iterator) +
	// string request: five parameters, genuine capacity local and result;
	// actual locked metadata getter/output/bytes/return and its storage-add
	// reference/amount/return. Private observe_metadata is scope-owned above.
	// update_health loop/current age; rewrite publisher's real bytes local.
	4 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) +
	2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) + sizeof(size_t) + 4 * sizeof(void *) +
	3 * sizeof(uint64_t) + 3 * sizeof(bool) +
	// native_checksum: data,size,real zero array owned by scan admission,
	// uLong checksum and uint32 return; crc32 leaf lives in named lower profile.
	sizeof(void *) + sizeof(size_t) + sizeof(uLong) + sizeof(uint32_t) +
	// Exact held transport function bytes-reference, integer capture/this,
	// offset/count/result/i, item_section/blob_size/tail/terms and returned bool;
	// native quest transport command parameter and returned bool. No decoding.
	4 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(uint64_t) + 2 * sizeof(bool) +
	// Typed read_le input/size/offset/value, decoded/index/return; record_result
	// enum formal and bool comparisons; operation equality/array== leaves.
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool) +
	sizeof(critical_command_journal_result) + sizeof(bool) + 4 * sizeof(void *) +
	2 * sizeof(bool) +
	// safe_directory/safe_regular signatures and declared filesystem boundary
	// arguments. Their actual status objects and scan's own status already
	// belong to the original sizeof(struct stat) per-operation admissions.
	3 * sizeof(void *) + sizeof(mode_t) + 2 * sizeof(bool) + 7 * sizeof(void *) +
	6 * sizeof(int) + 3 * sizeof(mode_t) + 2 * sizeof(uid_t) +
	// Actual now_msec chain: returned time_point, time_since_epoch's this/
	// duration, duration_cast reference/milliseconds and the selected
	// __duration_cast_impl<...,true,false>::__cast reference/milliseconds.
	// The real duration(rep) has this/rep reference and a temporary rep;
	// each of the two selected duration::count calls has this/rep return.
	sizeof(std::chrono::system_clock::time_point) +
	sizeof(std::chrono::system_clock::duration) + sizeof(std::chrono::milliseconds) +
	sizeof(uint64_t) + sizeof(std::chrono::system_clock::time_point) + sizeof(void *) +
	sizeof(std::chrono::system_clock::duration) +
	2 * (sizeof(void *) + sizeof(std::chrono::milliseconds)) + 2 * sizeof(void *) +
	sizeof(std::chrono::milliseconds::rep) + sizeof(void *) +
	sizeof(std::chrono::system_clock::duration::rep) + sizeof(void *) +
	sizeof(std::chrono::milliseconds::rep) + journal_physical_lock_source_frames +
	journal_physical_current_source_frames + journal_physical_scope_source_frames +
	// Actual journal_admission_budget::admit forwarding layer is separate
	// from physical::relay: peak,opaque,self reference and bool return.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual outer byte-vector resize/assign and frame-vector push/growth/
	// relocation/cleanup paths, with their genuine selected T instantiation.
	journal_physical_library_vector_frames + journal_physical_library_move_frames<uint8_t> +
	journal_physical_library_move_frames<journal_frame> +
	journal_physical_library_empty_vector_frames<uint8_t> +
	journal_physical_library_empty_vector_frames<journal_frame> +
	journal_physical_library_empty_vector_frames<critical_entity_key> +
	journal_physical_library_empty_vector_frames<critical_expected_revision> +
	// Actual containing generated bodies, separate from their already-owned
	// object storage and empty-vector/source-member closures: frame, command
	// and replay/decode envelope default ctor and destructor each have this;
	// frame/command/by-value-envelope move ctors have this/source;
	// command/rewrite move assignment has this/source/reference-return;
	// aggregate rewrite temporary cleanup has its real destructor this.
	3 * sizeof(void *) + 3 * sizeof(void *) + 3 * 2 * sizeof(void *) + 2 * 3 * sizeof(void *) +
	sizeof(void *) +
	// Exact frame member moves: four byte vectors plus actual key/revision.
	4 * journal_physical_library_move_frames<uint8_t> +
	journal_physical_library_move_frames<critical_entity_key> +
	journal_physical_library_move_frames<critical_expected_revision> +
	// Actual generated frame/command/envelope move construction selects the
	// separate vector move-CONSTRUCTOR chain, not _M_move_assign. Six actual
	// members: four byte vectors, key vector and expected-revision vector.
	4 * journal_physical_library_move_constructor_frames<uint8_t> +
	journal_physical_library_move_constructor_frames<critical_entity_key> +
	journal_physical_library_move_constructor_frames<critical_expected_revision> +
	// Actual nontrivial journal_frame relocation, in addition to the genuine
	// shared _S_relocate/__relocate_a selected above: __relocate_a_1 owns four
	// formal pointers, a returned pointer and its __cur local. Each genuine
	// __relocate_object_a has dest/orig/allocator, two addressof closures,
	// allocator_traits::destroy and destroy_at. Real member moves are above.
	6 * sizeof(void *) + 3 * sizeof(void *) + 4 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(void *) +
	// Nontrivial _Destroy(range,allocator), _Destroy(range), aux<false>,
	// _Destroy(pointer), destroy_at and __addressof; journal_frame's actual
	// generated destructor plus six typed member vector destructors/allocator
	// descendants live in the genuine selected move/constructor controls.
	3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	// Actual char assignment/replacement/append and fresh __str_concat:
	// two operator+ alternatives, lhs/rhs/allocator/result refs, lengths;
	// default allocator/hider/use-local-data/set-length/constructor cleanup.
	journal_physical_library_string_frames + 10 * sizeof(void *) + 6 * sizeof(size_t) +
	3 * sizeof(std::allocator<char>) + 6 * sizeof(void *) + sizeof(size_t) + sizeof(char) +
	// reserve this/resarg/oldcap/tmp, rvalue append this/ptr/count/result,
	// generated string move this/source and allocator/data/capacity scopes.
	5 * sizeof(void *) + 4 * sizeof(size_t) + 12 * (sizeof(void *) + sizeof(size_t)) +
	2 * sizeof(bool) + sizeof(char) +
	// Actual find_if/__find_if random-access journal_frame iterator and
	// captured operation predicate; any_of/__find_if key range predicate;
	// equal/__equal_aux/__memcmp byte range and typed iterator query leaves.
	14 * sizeof(void *) + 4 * sizeof(std::ptrdiff_t) + 6 * sizeof(bool) +
	2 * sizeof(std::random_access_iterator_tag) + 2 * sizeof(char) +
	5 * (3 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(int);
}

bool critical_command_journal_startup_source_frame_bytes(size_t *output) noexcept
{
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 ||
	    sizeof(std::vector<journal_frame>::iterator) != sizeof(void *) ||
	    sizeof(std::vector<journal_frame>::const_iterator) != sizeof(void *) ||
	    sizeof(std::vector<uint8_t>) != 3 * sizeof(void *) ||
	    sizeof(std::vector<critical_entity_key>) != 3 * sizeof(void *) ||
	    sizeof(std::vector<critical_expected_revision>) != 3 * sizeof(void *) ||
	    !std::is_nothrow_move_constructible_v<journal_frame> ||
	    !std::is_nothrow_move_assignable_v<journal_frame>)
		return false;
	size_t bytes = journal_physical_startup_declared_source_frames +
		       journal_physical_entry_source_frames;
	size_t codec = 0, crc = 0;
	// Required complete original codec and CRC source closures are strong
	// query outputs; no runtime/installed-library qualification is inferred.
	if (!critical_command_startup_codec_source_frame_bytes(&codec) ||
	    !journal_physical_crc32_source_frame_bytes(&crc) || !journal_admit_add(bytes, codec) ||
	    !journal_admit_add(bytes, crc))
		return false;
	*output = bytes;
	return true;
}

bool critical_command_journal_startup_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(int) != 4 ||
		      sizeof(std::ptrdiff_t) != 8 || sizeof(std::allocator<uint8_t>) != 1 ||
		      sizeof(std::vector<uint8_t>::const_iterator) != sizeof(void *) ||
		      sizeof(std::memory_order) != 4 || sizeof(std::__memory_order_modifier) != 4)
		return false;
	*output = journal_physical_initial_inline_bytes;
	return true;
#else
	return false;
#endif
}

namespace
{
bool journal_physical_entry_prepare(journal_physical_startup_budget &physical,
				    size_t &outer) noexcept
{
	size_t initial = outer, frames = 0;
	// Genuine entry, source-query, lower query return and checked-add carriers
	// precede all profile calls, ownership observation and journal mutation.
	constexpr size_t query = journal_physical_entry_source_frames;
	if (!physical.reserve || !journal_admit_add(initial, query) ||
	    !journal_admit_add(initial, journal_physical_scope_source_frames) ||
	    !journal_physical_call(physical.reserve, physical.context, initial, 0) ||
	    !critical_command_journal_startup_source_frame_bytes(&frames))
		return false;
	if (!journal_admit_add(outer, frames))
		return false;
	// The original workspace is constructed only after its authentic source
	// closure is admitted. Its owning inline storage joins per-original peaks.
	size_t construction = outer;
	const size_t workspace = journal_physical_initial_inline_bytes;
	return journal_admit_add(construction, workspace) &&
	       journal_physical_call(physical.reserve, physical.context, construction, 0);
}
}

bool critical_command_journal_init_physical_bounded(const char *directory, size_t quota_bytes,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *budget_context, size_t outer_live,
						    size_t *current_journal_metadata_bytes) noexcept
{
	if (!directory || !*directory || !quota_bytes || !reserve)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	journal_physical_startup_budget physical{ reserve, budget_context };
	if (!journal_physical_entry_prepare(physical, outer_live))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(journal_mutex);
		journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
		if (health.initialized)
			return false;
		health = {};
		native_rewrite_uncertain = {};
		publish_native_rewrite_storage();
		journal_has_native = false;
		try
		{
			journal_startup_init_workspace work{
				{ journal_physical_startup_budget::relay, &physical }, {}
			};
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !journal_admit_add(work.base, sizeof(struct stat)) ||
			    !physical.observe_metadata(work.base) ||
			    !work.budget.admit(work.base, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.directory_size = std::strlen(directory);
			work.live = work.base;
			if (!journal_startup_string_request(work.directory_size,
							    journal_directory.capacity(),
							    &work.capacity, &work.request) ||
			    !journal_admit_add(work.live, work.request) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			journal_directory = directory; // Exact original assignment/request.
			publish_journal_persistent_storage();
			if (mkdir(directory, 0700) != 0 && errno != EEXIST)
			{
				health.last_result = critical_command_journal_result::io_failure;
				++health.io_failures;
				return false;
			}
			if (!safe_directory(journal_directory))
			{
				health.last_result =
					critical_command_journal_result::unsafe_permissions;
				return false;
			}
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !journal_admit_add(work.base, sizeof(struct stat)) ||
			    !physical.observe_metadata(work.base))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.first_size = journal_directory.size();
			work.final_size = work.first_size;
			// Installed lvalue operator+ uses a fresh __str_concat reserve; the
			// following rvalue operator+ appends to that actual result. Admit old
			// and new heaps during append mutation, before the exact expression.
			if (!journal_admit_add(work.first_size, 1) ||
			    !journal_admit_add(work.final_size, 1) ||
			    !journal_admit_add(work.final_size, std::strlen(JOURNAL_FILE)) ||
			    !journal_startup_string_request(work.first_size, 15,
							    &work.first_capacity,
							    &work.first_request) ||
			    !journal_startup_string_request(work.final_size, work.first_capacity,
							    &work.final_capacity,
							    &work.final_request))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			work.live = work.base;
			if (!journal_admit_add(work.live, 2 * sizeof(std::string)) ||
			    !journal_admit_add(work.live, 2 * sizeof(std::allocator<char>)) ||
			    !journal_admit_add(work.live, work.first_request) ||
			    !journal_admit_add(work.live, work.final_request) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			journal_path = journal_directory + "/" + JOURNAL_FILE;
			publish_journal_persistent_storage();
			journal_quota = quota_bytes;
			const int fd = open(journal_path.c_str(),
					    O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW,
					    0600);
			if (fd < 0)
			{
				health.last_result = critical_command_journal_result::io_failure;
				++health.io_failures;
				return false;
			}
			close(fd);
			if (!safe_regular(journal_path, 0600))
			{
				health.last_result =
					critical_command_journal_result::unsafe_permissions;
				return false;
			}
			health.initialized = true;
			// Concatenation carriers and replaced old path storage have died.
			// Reobserve the actual persistent strings before the complete first scan.
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !journal_admit_add(work.base, sizeof(lock)) ||
			    !journal_admit_add(work.base, sizeof(snapshot)) ||
			    !physical.observe_metadata(work.base))
			{
				health.initialized = false;
				record_result(critical_command_journal_result::quota_exceeded);
				return false;
			}
			const auto result =
				journal_scan_admitted(&work.frames, work.budget, work.base);
			if (result != critical_command_journal_result::ok)
			{
				health.initialized = false;
				record_result(result);
				return false;
			}
			health.last_result = critical_command_journal_result::ok;
			update_health(work.frames);
			return true;
		}
		catch (...)
		{
			health.initialized = false;
			record_result(critical_command_journal_result::quota_exceeded);
			return false;
		}
	}
	catch (...)
	{
		// Real mutex acquisition failed; no unlocked journal mutation.
		return false;
	}
#endif
}

critical_command_journal_result critical_command_journal_replay_with_native_physical_bounded(
	critical_command_replay_bounded_fn legacy_replay,
	critical_native_recovery_replay_bounded_fn native_replay, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes) noexcept
{
	if (!legacy_replay || !native_replay || !reserve)
		return critical_command_journal_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	journal_physical_startup_budget physical{ reserve, budget_context };
	if (!journal_physical_entry_prepare(physical, outer_live))
		return critical_command_journal_result::quota_exceeded;
	try
	{
		journal_startup_replay_workspace work{
			{ journal_physical_startup_budget::relay, &physical }, {}
		};
		{
			std::lock_guard<std::mutex> lock(journal_mutex);
			journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
			if (!health.initialized)
				return critical_command_journal_result::not_initialized;
			// Full original uncertainty refusal precedes every registration.
			if (health.append_uncertain || native_rewrite_uncertain.active)
				return critical_command_journal_result::append_uncertain;
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !physical.observe_metadata(work.base))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			work.live = work.base;
			if (!journal_admit_add(work.live, sizeof(lock)) ||
			    !journal_admit_add(work.live, sizeof(snapshot)) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			const auto result =
				journal_scan_admitted(&work.frames, work.budget, work.live);
			if (result != critical_command_journal_result::ok)
			{
				record_result(result);
				return result;
			}
			++health.replays;
		}
		// Original startup caller keeps coordinator ownership. The journal lock
		// is released exactly before callbacks; no outside coor getter is called.
		for (auto &frame : work.frames)
		{
			if (frame.native)
			{
				work.live = work.base;
				// Local envelope and original by-value parameter coexist. Full
				// current frame heap remains owned once before their nonallocating moves.
				if (!journal_admit_add(
					    work.live,
					    2 * sizeof(critical_native_recovery_envelope)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				critical_native_recovery_envelope envelope;
				envelope.command = std::move(frame.command);
				envelope.revision = frame.native_revision;
				envelope.phase = frame.native_phase;
				envelope.attachment = std::move(frame.native_attachment);
				// Recount actual moved-from frames and authentic current envelope.
				// Its next move transfers this heap to the by-value parameter once.
				work.live = work.base;
				if (!journal_admit_add(
					    work.live,
					    2 * sizeof(critical_native_recovery_envelope)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !journal_command_heap(envelope.command, false, work.live) ||
				    !journal_admit_add(work.live, envelope.attachment.capacity()) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				work.accepted =
					native_replay(std::move(envelope), original_context,
						      journal_physical_startup_budget::relay,
						      &physical, work.live);
			}
			else
			{
				work.live = work.base;
				if (!journal_admit_add(work.live, sizeof(critical_command)) ||
				    !journal_startup_frames(work.frames, work.live) ||
				    !work.budget.admit(work.live, &work.budget))
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					journal_startup_metadata_snapshot snapshot{
						current_journal_metadata_bytes
					};
					health.last_result =
						critical_command_journal_result::quota_exceeded;
					return critical_command_journal_result::quota_exceeded;
				}
				// Original by-value parameter steals only this frame's command heap.
				// Parameter inline and all other retained frame capacities are admitted.
				work.accepted =
					legacy_replay(std::move(frame.command), original_context,
						      journal_physical_startup_budget::relay,
						      &physical, work.live);
			}
			if (!work.accepted)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				journal_startup_metadata_snapshot snapshot{
					current_journal_metadata_bytes
				};
				health.last_result =
					critical_command_journal_result::replay_blocked;
				return critical_command_journal_result::replay_blocked;
			}
		}
		std::lock_guard<std::mutex> lock(journal_mutex);
		journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
		health.last_result = critical_command_journal_result::ok;
		return critical_command_journal_result::ok;
	}
	catch (...)
	{
		// No callback throws through its noexcept interface. An actual mutex or
		// construction failure grants no registration and performs no unlocked write.
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}

namespace
{
// The additional original legacy preflight has no native callback and no
// allocating iterator/DTO substitute. Its actual index owns N, vector::size
// owns this/result(P+N), and operator[] owns this/index/returned-reference(2P+N).
// Genuine GNU13 nonasserted vector members directly read their stored pointers.
constexpr size_t journal_legacy_physical_preflight_source_frames =
	3 * sizeof(void *) + 3 * sizeof(size_t);
}

bool critical_command_journal_legacy_replay_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t source = 0;
	// The existing complete physical startup family owns the identical scan,
	// locks, metadata, workspace, frame census, move and callback cleanup graph.
	// Only the complete pre-callback legacy native-presence loop is additional.
	if (!critical_command_journal_startup_source_frame_bytes(&source) ||
	    !journal_admit_add(source, journal_legacy_physical_preflight_source_frames))
		return false;
	*output = source;
	return true;
}

critical_command_journal_result critical_command_journal_replay_physical_bounded(
	critical_command_replay_bounded_fn replay, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes) noexcept
{
	if (!replay || !reserve)
		return critical_command_journal_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return critical_command_journal_result::quota_exceeded;
#else
	journal_physical_startup_budget physical{ reserve, budget_context };
	// Retain the genuinely additional preflight SOURCE through the same exact
	// physical entry and identity relay. First admission precedes workspace,
	// mutex acquisition and every scan/native-presence observation.
	if (!journal_admit_add(outer_live, journal_legacy_physical_preflight_source_frames) ||
	    !journal_physical_entry_prepare(physical, outer_live))
		return critical_command_journal_result::quota_exceeded;
	try
	{
		journal_startup_replay_workspace work{
			{ journal_physical_startup_budget::relay, &physical }, {}
		};
		{
			std::lock_guard<std::mutex> lock(journal_mutex);
			journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
			if (!health.initialized)
				return critical_command_journal_result::not_initialized;
			work.base = outer_live;
			if (!journal_admit_add(work.base, sizeof(work)) ||
			    !physical.observe_metadata(work.base))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			work.live = work.base;
			if (!journal_admit_add(work.live, sizeof(lock)) ||
			    !journal_admit_add(work.live, sizeof(snapshot)) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				record_result(critical_command_journal_result::quota_exceeded);
				return critical_command_journal_result::quota_exceeded;
			}
			const auto result =
				journal_scan_admitted(&work.frames, work.budget, work.live);
			if (result != critical_command_journal_result::ok)
			{
				record_result(result);
				return result;
			}
			// Original legacy error priority: completed scan before this sole
			// uncertainty refusal. Do not add the mixed path's append-health gate.
			if (native_rewrite_uncertain.active)
				return critical_command_journal_result::append_uncertain;
			// Inspect the ENTIRE retained journal under its actual lock before
			// any legacy callback or replay-counter update. A late native frame
			// blocks the whole operation; no attachment is dropped or fabricated.
			for (size_t index = 0; index < work.frames.size(); ++index)
				if (work.frames[index].native)
					return critical_command_journal_result::replay_blocked;
			++health.replays;
		}
		// Exact original release boundary: callback runs without journal_mutex,
		// while its actual coordinator owner remains with the startup caller.
		for (auto &frame : work.frames)
		{
			work.live = work.base;
			if (!journal_admit_add(work.live, sizeof(critical_command)) ||
			    !journal_startup_frames(work.frames, work.live) ||
			    !work.budget.admit(work.live, &work.budget))
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				journal_startup_metadata_snapshot snapshot{
					current_journal_metadata_bytes
				};
				health.last_result =
					critical_command_journal_result::quota_exceeded;
				return critical_command_journal_result::quota_exceeded;
			}
			// Real by-value command parameter steals only this frame's heap.
			// Parameter inline and every genuine frame capacity are owned once.
			work.accepted = replay(std::move(frame.command), original_context,
					       journal_physical_startup_budget::relay, &physical,
					       work.live);
			if (!work.accepted)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				journal_startup_metadata_snapshot snapshot{
					current_journal_metadata_bytes
				};
				health.last_result =
					critical_command_journal_result::replay_blocked;
				return critical_command_journal_result::replay_blocked;
			}
		}
		std::lock_guard<std::mutex> lock(journal_mutex);
		journal_startup_metadata_snapshot snapshot{ current_journal_metadata_bytes };
		health.last_result = critical_command_journal_result::ok;
		return critical_command_journal_result::ok;
	}
	catch (...)
	{
		// Preserve real lock/construction failure containment. No noexcept
		// callback throws through this interface; no unlocked health mutation.
		return critical_command_journal_result::quota_exceeded;
	}
#endif
}
