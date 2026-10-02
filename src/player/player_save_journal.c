#include "player/player_save_journal.h"

#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <new>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
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
constexpr const char *JOURNAL_ARCHIVE_NAME = "player-save.journal.quarantine.archive";
constexpr const char *JOURNAL_ARCHIVE_TEMP_NAME = "player-save.journal.quarantine.archive.tmp";
constexpr const char *JOURNAL_QUARANTINE_PIDS_NAME = "player-save.quarantine-pids";
constexpr std::array<uint8_t, 8> ARCHIVE_MAGIC = { 'D', 'P', 'Q', 'A', 'R', 'C', '1', 0 };
constexpr uint32_t ARCHIVE_VERSION = 1;
constexpr uint32_t ARCHIVE_RECOVERY_VERSION = 2;
constexpr std::array<uint8_t, 8> RECOVERY_MAGIC = { 'D', 'P', 'R', 'C', 'O', 'V', '1', 0 };
constexpr size_t ARCHIVE_HEADER_SIZE = 28;
constexpr size_t ARCHIVE_ENTRY_SIZE = 56;
constexpr uint64_t ARCHIVE_MAX_BYTES = 2ULL * PLAYER_SAVE_JOURNAL_MAX_BYTES;
constexpr uint32_t QUARANTINE_EXPLICIT_PID = 1;
constexpr uint32_t QUARANTINE_CORRUPT_FRAME = 2;
constexpr uint32_t QUARANTINE_UNSUPPORTED_FRAME = 3;
constexpr uint32_t QUARANTINE_LEGACY_BYTES = 4;
constexpr uint32_t QUARANTINE_RUNTIME_TERMINAL = 5;

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
	struct quarantine_record
	{
		int32_t pid = 0;
		uint32_t reason = 0;
		std::vector<uint8_t> bytes;
	};
	std::vector<quarantine_record> quarantine_records;
	player_save_journal_result result = player_save_journal_result::ok;
};

std::mutex journal_mutex;
std::string journal_directory;
std::string journal_path;
std::string quarantine_path;
std::string archive_path;
std::string archive_temporary_path;
std::string quarantine_pids_path;
size_t journal_quota = 0;
uint64_t record_sequence = 0;
uint64_t oldest_record_msec = 0;
player_save_journal_health health = {};
std::set<int32_t> quarantined_pids;
std::set<int32_t> archived_pids;
std::vector<scan_result::quarantine_record> archive_records;
struct stored_recovery
{
	player_save_recovery_record record;
	bool resolved = false;
	bool revoked = false;
};
std::vector<stored_recovery> recovery_records;
std::set<int32_t> policy_pids;
bool quarantine_state_ready = false;
bool quarantine_state_failed = false;

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

std::array<uint8_t, 32> sha256(const uint8_t *bytes, size_t size)
{
	static constexpr uint32_t constants[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
		0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
		0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
		0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
		0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
		0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
		0xc67178f2
	};
	uint32_t state[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	std::vector<uint8_t> padded;
	padded.reserve(size + 72);
	if (size)
		padded.insert(padded.end(), bytes, bytes + size);
	padded.push_back(0x80);
	while (padded.size() % 64 != 56)
		padded.push_back(0);
	const uint64_t bit_length = static_cast<uint64_t>(size) * 8;
	for (int shift = 56; shift >= 0; shift -= 8)
		padded.push_back(static_cast<uint8_t>(bit_length >> shift));
	auto rotate_right = [](uint32_t value, unsigned int count)
	{ return (value >> count) | (value << (32 - count)); };
	for (size_t block = 0; block < padded.size(); block += 64)
	{
		uint32_t words[64] = {};
		for (size_t index = 0; index < 16; ++index)
			words[index] =
				(static_cast<uint32_t>(padded[block + index * 4]) << 24) |
				(static_cast<uint32_t>(padded[block + index * 4 + 1]) << 16) |
				(static_cast<uint32_t>(padded[block + index * 4 + 2]) << 8) |
				static_cast<uint32_t>(padded[block + index * 4 + 3]);
		for (size_t index = 16; index < 64; ++index)
		{
			const uint32_t s0 = rotate_right(words[index - 15], 7) ^
					    rotate_right(words[index - 15], 18) ^
					    (words[index - 15] >> 3);
			const uint32_t s1 = rotate_right(words[index - 2], 17) ^
					    rotate_right(words[index - 2], 19) ^
					    (words[index - 2] >> 10);
			words[index] = words[index - 16] + s0 + words[index - 7] + s1;
		}
		uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
		uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
		for (size_t index = 0; index < 64; ++index)
		{
			const uint32_t s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^
					    rotate_right(e, 25);
			const uint32_t choice = (e & f) ^ (~e & g);
			const uint32_t temp1 = h + s1 + choice + constants[index] + words[index];
			const uint32_t s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^
					    rotate_right(a, 22);
			const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
			const uint32_t temp2 = s0 + majority;
			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}
		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}
	std::array<uint8_t, 32> digest = {};
	for (size_t index = 0; index < 8; ++index)
		for (size_t byte = 0; byte < 4; ++byte)
			digest[index * 4 + byte] =
				static_cast<uint8_t>(state[index] >> (24 - byte * 8));
	return digest;
}

bool read_safe_bytes(const std::string &path, uint64_t limit, std::vector<uint8_t> *bytes,
		     bool *exists)
{
	if (!bytes || !exists)
		return false;
	bytes->clear();
	*exists = false;
	const int fd = open_safe_file(path, O_RDONLY, 0600);
	if (fd < 0)
		return errno == ENOENT;
	struct stat status = {};
	if (fstat(fd, &status) != 0 || status.st_size < 0 ||
	    static_cast<uint64_t>(status.st_size) > limit)
	{
		close(fd);
		return false;
	}
	try
	{
		bytes->resize(static_cast<size_t>(status.st_size));
	}
	catch (const std::bad_alloc &)
	{
		close(fd);
		return false;
	}
	size_t offset = 0;
	while (offset < bytes->size())
	{
		const ssize_t count = read(fd, bytes->data() + offset, bytes->size() - offset);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
		{
			close(fd);
			bytes->clear();
			return false;
		}
		offset += static_cast<size_t>(count);
	}
	if (close(fd) != 0)
	{
		bytes->clear();
		return false;
	}
	*exists = true;
	return true;
}

bool ordinary_recovery_snapshot(const player_snapshot &snapshot)
{
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && snapshot.pid > 0 &&
	       snapshot.revision && !snapshot.death && snapshot.quest_xp_receipts.empty() &&
	       snapshot.spell_effect_receipts.empty() && snapshot.craft_receipts.empty();
}

bool archive_recovery_frames(int pid, std::vector<player_snapshot> *frames,
			     std::array<uint8_t, 32> *digest)
{
	if (!frames || !digest || pid <= 0)
		return false;
	frames->clear();
	std::vector<uint8_t> fingerprints;
	for (const auto &record : archive_records)
	{
		if (record.pid != pid)
			continue;
		if (record.reason != QUARANTINE_RUNTIME_TERMINAL ||
		    frames->size() >= PLAYER_SAVE_JOURNAL_MAX_RECORDS)
			return false;
		const auto &bytes = record.bytes;
		if (bytes.size() < JOURNAL_HEADER_SIZE || bytes.size() > JOURNAL_MAX_RECORD_SIZE ||
		    !std::equal(JOURNAL_MAGIC.begin(), JOURNAL_MAGIC.end(), bytes.begin()) ||
		    get_u32(bytes.data(), 8) != JOURNAL_FORMAT_VERSION ||
		    get_u32(bytes.data(), 12) != JOURNAL_HEADER_SIZE ||
		    get_u64(bytes.data(), 16) != bytes.size() ||
		    static_cast<int32_t>(get_u32(bytes.data(), 40)) != pid ||
		    get_u32(bytes.data(), 64) != bytes.size() - JOURNAL_HEADER_SIZE ||
		    get_u32(bytes.data(), 68) != frame_checksum(bytes.data(), bytes.size()))
			return false;
		player_snapshot snapshot;
		if (player_snapshot_decode(bytes.data() + JOURNAL_HEADER_SIZE,
					   bytes.size() - JOURNAL_HEADER_SIZE,
					   &snapshot) != player_snapshot_codec_result::ok ||
		    !ordinary_recovery_snapshot(snapshot) || snapshot.pid != pid ||
		    snapshot.schema_version != get_u32(bytes.data(), 44) ||
		    snapshot.revision != get_u64(bytes.data(), 48) ||
		    snapshot.components != get_u64(bytes.data(), 56))
			return false;
		frames->push_back(std::move(snapshot));
		const auto hash = sha256(bytes.data(), bytes.size());
		fingerprints.insert(fingerprints.end(), hash.begin(), hash.end());
	}
	if (frames->empty())
		return false;
	*digest = sha256(fingerprints.data(), fingerprints.size());
	return true;
}

bool encode_recovery(const stored_recovery &stored, std::vector<uint8_t> *encoded)
{
	const auto &record = stored.record;
	std::vector<uint8_t> baseline, replacement;
	if (!encoded || (record.backend != 1 && record.backend != 2) ||
	    record.backend_identity.empty() || record.backend_identity.size() > 256 ||
	    record.account_name.empty() || record.account_name.size() > 50 ||
	    std::all_of(record.identity.begin(), record.identity.end(),
			[](uint8_t b) { return !b; }) ||
	    !ordinary_recovery_snapshot(record.baseline) ||
	    !ordinary_recovery_snapshot(record.replacement) ||
	    record.baseline.pid != record.replacement.pid ||
	    record.baseline.components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    record.replacement.components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    record.replacement.revision <= record.baseline.revision ||
	    record.creation_commands.empty() || record.creation_commands.size() > 64 ||
	    record.authority_evidence.empty() || record.authority_evidence.size() > 1024 * 1024 ||
	    player_snapshot_encode(record.baseline, &baseline) !=
		    player_snapshot_codec_result::ok ||
	    player_snapshot_encode(record.replacement, &replacement) !=
		    player_snapshot_codec_result::ok)
		return false;
	encoded->assign(60, 0);
	put_u32(*encoded, 0, stored.revoked ? 2 : stored.resolved ? 1 : 0);
	put_u32(*encoded, 4, record.backend);
	std::copy(record.identity.begin(), record.identity.end(), encoded->begin() + 8);
	std::copy(record.archive_digest.begin(), record.archive_digest.end(),
		  encoded->begin() + 24);
	put_u32(*encoded, 56, record.creation_commands.size());
	auto block = [&](const uint8_t *bytes, size_t size)
	{
		const size_t offset = encoded->size();
		encoded->resize(offset + 4);
		put_u32(*encoded, offset, size);
		encoded->insert(encoded->end(), bytes, bytes + size);
	};
	block(reinterpret_cast<const uint8_t *>(record.backend_identity.data()),
	      record.backend_identity.size());
	block(reinterpret_cast<const uint8_t *>(record.account_name.data()),
	      record.account_name.size());
	block(baseline.data(), baseline.size());
	block(replacement.data(), replacement.size());
	block(record.authority_evidence.data(), record.authority_evidence.size());
	for (const auto &command : record.creation_commands)
	{
		if (command.empty() || command.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
			return false;
		block(command.data(), command.size());
	}
	return true;
}

bool decode_recovery(const uint8_t *bytes, size_t size, stored_recovery *stored)
{
	if (!stored || size < 60 || get_u32(bytes, 0) > 2 || get_u32(bytes, 56) > 64)
		return false;
	stored_recovery result;
	result.resolved = get_u32(bytes, 0) != 0;
	result.revoked = get_u32(bytes, 0) == 2;
	result.record.backend = get_u32(bytes, 4);
	std::copy(bytes + 8, bytes + 24, result.record.identity.begin());
	std::copy(bytes + 24, bytes + 56, result.record.archive_digest.begin());
	size_t cursor = 60;
	auto block = [&](std::vector<uint8_t> *value, size_t maximum)
	{
		if (cursor > size || size - cursor < 4)
			return false;
		const size_t length = get_u32(bytes, cursor);
		cursor += 4;
		if (!length || length > maximum || length > size - cursor)
			return false;
		value->assign(bytes + cursor, bytes + cursor + length);
		cursor += length;
		return true;
	};
	std::vector<uint8_t> identity, account, baseline, replacement;
	if (!block(&identity, 256) || !block(&account, 50) ||
	    !block(&baseline, PLAYER_SNAPSHOT_MAX_BYTES) ||
	    !block(&replacement, PLAYER_SNAPSHOT_MAX_BYTES) ||
	    !block(&result.record.authority_evidence, 1024 * 1024) ||
	    player_snapshot_decode(baseline.data(), baseline.size(), &result.record.baseline) !=
		    player_snapshot_codec_result::ok ||
	    player_snapshot_decode(replacement.data(), replacement.size(),
				   &result.record.replacement) != player_snapshot_codec_result::ok)
		return false;
	result.record.backend_identity.assign(identity.begin(), identity.end());
	result.record.account_name.assign(account.begin(), account.end());
	for (uint32_t index = 0; index < get_u32(bytes, 56); ++index)
	{
		std::vector<uint8_t> command;
		if (!block(&command, CRITICAL_COMMAND_MAX_ENCODED_BYTES))
			return false;
		result.record.creation_commands.push_back(std::move(command));
	}
	std::vector<uint8_t> canonical;
	if (cursor != size || !encode_recovery(result, &canonical) || canonical.size() != size ||
	    !std::equal(canonical.begin(), canonical.end(), bytes))
		return false;
	*stored = std::move(result);
	return true;
}

bool load_quarantine_archive()
{
	std::vector<uint8_t> bytes;
	bool exists = false;
	if (!read_safe_bytes(archive_path, ARCHIVE_MAX_BYTES, &bytes, &exists))
		return false;
	if (!exists)
		return true;
	if (bytes.size() < ARCHIVE_HEADER_SIZE ||
	    !std::equal(ARCHIVE_MAGIC.begin(), ARCHIVE_MAGIC.end(), bytes.begin()) ||
	    (get_u32(bytes.data(), 8) != ARCHIVE_VERSION &&
	     get_u32(bytes.data(), 8) != ARCHIVE_RECOVERY_VERSION) ||
	    get_u64(bytes.data(), 12) != bytes.size())
		return false;
	const uint32_t pid_count = get_u32(bytes.data(), 20);
	const uint32_t frame_count = get_u32(bytes.data(), 24);
	if (pid_count > 65536 || frame_count > 262144)
		return false;
	const uint64_t table_size = ARCHIVE_HEADER_SIZE + static_cast<uint64_t>(pid_count) * 4 +
				    static_cast<uint64_t>(frame_count) * ARCHIVE_ENTRY_SIZE;
	if (table_size > bytes.size())
		return false;
	size_t cursor = ARCHIVE_HEADER_SIZE;
	for (uint32_t index = 0; index < pid_count; ++index)
	{
		const int32_t pid = static_cast<int32_t>(get_u32(bytes.data(), cursor));
		cursor += 4;
		if (pid <= 0 || !archived_pids.insert(pid).second)
			return false;
		quarantined_pids.insert(pid);
	}
	uint64_t expected_data_offset = table_size;
	for (uint32_t index = 0; index < frame_count; ++index)
	{
		const int32_t pid = static_cast<int32_t>(get_u32(bytes.data(), cursor));
		const uint32_t reason = get_u32(bytes.data(), cursor + 4);
		const uint64_t offset = get_u64(bytes.data(), cursor + 8);
		const uint64_t size = get_u64(bytes.data(), cursor + 16);
		const uint8_t *expected_hash = bytes.data() + cursor + 24;
		cursor += ARCHIVE_ENTRY_SIZE;
		if ((pid < 0) || (pid > 0 && !archived_pids.count(pid)) ||
		    reason < QUARANTINE_EXPLICIT_PID || reason > QUARANTINE_RUNTIME_TERMINAL ||
		    !size || offset != expected_data_offset || size > bytes.size() - offset)
			return false;
		// Unassigned or unsupported evidence cannot authorize a per-PID release.
		// Preserve the archive and refuse all admissions until explicit reconciliation.
		if (pid == 0 || reason == QUARANTINE_CORRUPT_FRAME ||
		    reason == QUARANTINE_UNSUPPORTED_FRAME || reason == QUARANTINE_LEGACY_BYTES)
			return false;
		const auto digest = sha256(bytes.data() + offset, static_cast<size_t>(size));
		if (!std::equal(digest.begin(), digest.end(), expected_hash))
			return false;
		scan_result::quarantine_record record;
		record.pid = pid;
		record.reason = reason;
		try
		{
			record.bytes.assign(bytes.begin() + offset, bytes.begin() + offset + size);
			archive_records.push_back(std::move(record));
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		expected_data_offset += size;
	}
	if (get_u32(bytes.data(), 8) == ARCHIVE_VERSION)
		return expected_data_offset == bytes.size();
	cursor = expected_data_offset;
	if (bytes.size() - cursor < 12 ||
	    !std::equal(RECOVERY_MAGIC.begin(), RECOVERY_MAGIC.end(), bytes.begin() + cursor))
		return false;
	const uint32_t count = get_u32(bytes.data(), cursor + 8);
	cursor += 12;
	if (!count || count > 65536)
		return false;
	std::set<std::array<uint8_t, 16>> identities;
	for (uint32_t index = 0; index < count; ++index)
	{
		if (bytes.size() - cursor < 40)
			return false;
		const uint64_t size = get_u64(bytes.data(), cursor);
		const uint8_t *hash = bytes.data() + cursor + 8;
		cursor += 40;
		if (size > bytes.size() - cursor)
			return false;
		const auto actual_hash = sha256(bytes.data() + cursor, size);
		stored_recovery stored;
		if (!std::equal(actual_hash.begin(), actual_hash.end(), hash) ||
		    !decode_recovery(bytes.data() + cursor, size, &stored) ||
		    !archived_pids.count(stored.record.replacement.pid) ||
		    !identities.insert(stored.record.identity).second)
			return false;
		recovery_records.push_back(std::move(stored));
		cursor += size;
	}
	if (cursor != bytes.size())
		return false;
	// A retained resolution is not sufficient for a mixed-generation restore.
	// The native owner must verify its backend evidence again before admission.
	return true;
}

bool load_quarantine_pid_policy()
{
	std::vector<uint8_t> bytes;
	bool exists = false;
	if (!read_safe_bytes(quarantine_pids_path, 4096, &bytes, &exists))
		return false;
	if (!exists || bytes.empty())
		return true;
	size_t offset = 0;
	while (offset < bytes.size())
	{
		size_t end = offset;
		while (end < bytes.size() && bytes[end] != '\n')
			++end;
		if (end == offset)
			return false;
		int32_t pid = 0;
		const char *begin = reinterpret_cast<const char *>(bytes.data() + offset);
		const char *finish = reinterpret_cast<const char *>(bytes.data() + end);
		const auto parsed = std::from_chars(begin, finish, pid, 10);
		if (parsed.ec != std::errc() || parsed.ptr != finish || pid <= 0)
			return false;
		quarantined_pids.insert(pid);
		policy_pids.insert(pid);
		if (quarantined_pids.size() > 65536)
			return false;
		offset = end + (end < bytes.size() ? 1 : 0);
	}
	return true;
}

bool persist_quarantine_archive(const std::set<int32_t> &pids,
				const std::vector<scan_result::quarantine_record> &additions,
				const std::vector<stored_recovery> *recoveries = nullptr)
{
	const auto &recovery = recoveries ? *recoveries : recovery_records;
	std::vector<std::vector<uint8_t>> encoded_recovery;
	uint64_t recovery_bytes = recovery.empty() ? 0 : 12;
	if (recovery.size() > 65536)
		return false;
	for (const auto &stored : recovery)
	{
		std::vector<uint8_t> encoded;
		if (!encode_recovery(stored, &encoded) ||
		    encoded.size() + 40 > ARCHIVE_MAX_BYTES - recovery_bytes)
			return false;
		recovery_bytes += encoded.size() + 40;
		encoded_recovery.push_back(std::move(encoded));
	}
	const uint64_t entry_count = archive_records.size() + additions.size();
	if (pids.size() > 65536 || entry_count > 262144)
		return false;
	const uint64_t table_size =
		ARCHIVE_HEADER_SIZE + pids.size() * 4 + entry_count * ARCHIVE_ENTRY_SIZE;
	if (recovery_bytes > ARCHIVE_MAX_BYTES - table_size)
		return false;
	uint64_t total_size = table_size + recovery_bytes;
	const std::vector<scan_result::quarantine_record> *collections[] = { &archive_records,
									     &additions };
	for (const auto *collection : collections)
		for (const auto &record : *collection)
		{
			if (record.bytes.empty() ||
			    record.bytes.size() > ARCHIVE_MAX_BYTES - total_size)
				return false;
			total_size += record.bytes.size();
		}
	if (total_size > ARCHIVE_MAX_BYTES || total_size > std::numeric_limits<size_t>::max())
		return false;
	std::vector<uint8_t> bytes;
	try
	{
		bytes.resize(static_cast<size_t>(total_size));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::copy(ARCHIVE_MAGIC.begin(), ARCHIVE_MAGIC.end(), bytes.begin());
	put_u32(bytes, 8, recovery.empty() ? ARCHIVE_VERSION : ARCHIVE_RECOVERY_VERSION);
	put_u64(bytes, 12, total_size);
	put_u32(bytes, 20, static_cast<uint32_t>(pids.size()));
	put_u32(bytes, 24, static_cast<uint32_t>(entry_count));
	size_t cursor = ARCHIVE_HEADER_SIZE;
	for (int32_t pid : pids)
	{
		put_u32(bytes, cursor, static_cast<uint32_t>(pid));
		cursor += 4;
	}
	const size_t metadata_start = cursor;
	uint64_t raw_offset = table_size;
	size_t record_index = 0;
	for (const auto *collection : collections)
		for (const auto &record : *collection)
		{
			const size_t metadata = metadata_start + record_index * ARCHIVE_ENTRY_SIZE;
			put_u32(bytes, metadata, static_cast<uint32_t>(record.pid));
			put_u32(bytes, metadata + 4, record.reason);
			put_u64(bytes, metadata + 8, raw_offset);
			put_u64(bytes, metadata + 16, record.bytes.size());
			const auto digest = sha256(record.bytes.data(), record.bytes.size());
			std::copy(digest.begin(), digest.end(), bytes.begin() + metadata + 24);
			std::copy(record.bytes.begin(), record.bytes.end(),
				  bytes.begin() + raw_offset);
			raw_offset += record.bytes.size();
			++record_index;
		}
	if (!recovery.empty())
	{
		std::copy(RECOVERY_MAGIC.begin(), RECOVERY_MAGIC.end(), bytes.begin() + raw_offset);
		put_u32(bytes, raw_offset + 8, recovery.size());
		raw_offset += 12;
		for (const auto &encoded : encoded_recovery)
		{
			put_u64(bytes, raw_offset, encoded.size());
			const auto digest = sha256(encoded.data(), encoded.size());
			std::copy(digest.begin(), digest.end(), bytes.begin() + raw_offset + 8);
			std::copy(encoded.begin(), encoded.end(), bytes.begin() + raw_offset + 40);
			raw_offset += encoded.size() + 40;
		}
	}
	const int fd = open_safe_file(archive_temporary_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
		return false;
	bool ok = write_all(fd, bytes.data(), bytes.size()) && fdatasync(fd) == 0;
	if (close(fd) != 0)
		ok = false;
	if (ok)
		ok = rename(archive_temporary_path.c_str(), archive_path.c_str()) == 0;
	if (ok)
		ok = sync_directory();
	if (!ok)
		unlink(archive_temporary_path.c_str());
	return ok;
}

bool commit_quarantine_archive(const std::set<int32_t> &additional_pids,
			       std::vector<scan_result::quarantine_record> additions)
{
	std::set<int32_t> next_pids = archived_pids;
	next_pids.insert(quarantined_pids.begin(), quarantined_pids.end());
	for (int32_t pid : additional_pids)
	{
		if (pid <= 0)
			return false;
		next_pids.insert(pid);
	}
	auto next_recovery = recovery_records;
	bool revoked = false;
	for (auto &stored : next_recovery)
		if (additional_pids.count(stored.record.replacement.pid) && !stored.revoked)
		{
			stored.revoked = true;
			revoked = true;
		}
	if (next_pids == archived_pids && additions.empty() && !revoked)
		return true;
	if (!persist_quarantine_archive(next_pids, additions, &next_recovery))
		return false;
	recovery_records.swap(next_recovery);
	archived_pids.swap(next_pids);
	quarantined_pids.insert(additional_pids.begin(), additional_pids.end());
	try
	{
		archive_records.insert(archive_records.end(),
				       std::make_move_iterator(additions.begin()),
				       std::make_move_iterator(additions.end()));
	}
	catch (const std::bad_alloc &)
	{
		// The archive is already durable. Keep the process fail-closed and let a
		// later initialization reload the authoritative archive from disk.
		quarantine_state_ready = false;
		quarantine_state_failed = true;
		return false;
	}
	uint64_t total = 0;
	for (const auto &record : archive_records)
		total = record.bytes.size() > UINT64_MAX - total ? UINT64_MAX :
								   total + record.bytes.size();
	health.quarantined_bytes = total;
	return true;
}

bool load_legacy_quarantine_bytes()
{
	std::vector<uint8_t> bytes;
	bool exists = false;
	if (!read_safe_bytes(quarantine_path, journal_quota, &bytes, &exists))
		return false;
	if (!exists || bytes.empty())
		return true;
	for (const auto &record : archive_records)
		if (record.pid == 0 && record.reason == QUARANTINE_LEGACY_BYTES &&
		    record.bytes == bytes)
			return false;
	scan_result::quarantine_record legacy;
	legacy.reason = QUARANTINE_LEGACY_BYTES;
	legacy.bytes = std::move(bytes);
	(void)commit_quarantine_archive({}, { std::move(legacy) });
	// Legacy unassigned bytes are evidence, not permission to reopen any PID.
	return false;
}

bool write_compacted(const std::vector<journal_frame> &frames);

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
	auto quarantine_range = [&](size_t start, size_t size, uint32_t reason)
	{
		if (!size)
			return;
		scan_result::quarantine_record record;
		record.reason = reason;
		record.bytes.assign(bytes.begin() + start, bytes.begin() + start + size);
		scanned.quarantine_records.push_back(std::move(record));
	};
	size_t offset = 0;
	while (offset < bytes.size())
	{
		if (bytes.size() - offset < JOURNAL_HEADER_SIZE ||
		    !std::equal(JOURNAL_MAGIC.begin(), JOURNAL_MAGIC.end(), bytes.begin() + offset))
		{
			const size_t next = find_next_magic(bytes, offset + 1);
			quarantine_range(offset, next - offset, QUARANTINE_CORRUPT_FRAME);
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
			quarantine_range(offset, next - offset, QUARANTINE_CORRUPT_FRAME);
			++health.corrupt_records;
			offset = next;
			continue;
		}
		if (record_size > bytes.size() - offset)
		{
			quarantine_range(offset, bytes.size() - offset, QUARANTINE_CORRUPT_FRAME);
			++health.corrupt_records;
			offset = bytes.size();
			break;
		}
		if (version != JOURNAL_FORMAT_VERSION)
		{
			quarantine_range(offset, record_size, QUARANTINE_UNSUPPORTED_FRAME);
			++health.unsupported_records;
			offset += record_size;
			continue;
		}
		const uint32_t expected = get_u32(bytes.data() + offset, JOURNAL_CHECKSUM_OFFSET);
		if (expected != frame_checksum(bytes.data() + offset, record_size))
		{
			quarantine_range(offset, record_size, QUARANTINE_CORRUPT_FRAME);
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
		if (decoded == player_snapshot_codec_result::allocation_failure)
		{
			// Resource exhaustion cannot establish that durable bytes are corrupt.
			// Keep the active evidence and retry initialization after recovery.
			scanned.result = player_save_journal_result::io_failure;
			return scanned;
		}
		if (decoded != player_snapshot_codec_result::ok ||
		    static_cast<uint32_t>(frame.snapshot.pid) !=
			    get_u32(bytes.data() + offset, 40) ||
		    get_u32(bytes.data() + offset + header_size, 0) !=
			    get_u32(bytes.data() + offset, 44) ||
		    frame.snapshot.revision != get_u64(bytes.data() + offset, 48) ||
		    frame.snapshot.components != get_u64(bytes.data() + offset, 56))
		{
			quarantine_range(offset, record_size, QUARANTINE_CORRUPT_FRAME);
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
	return scanned;
}

scan_result scan_journal_safe()
{
	auto fail_closed = []()
	{
		health.initialized = false;
		quarantine_state_ready = false;
		quarantine_state_failed = true;
	};
	try
	{
		scan_result scanned = scan_journal();
		if (scanned.result != player_save_journal_result::ok)
		{
			fail_closed();
			return scanned;
		}
		if (!scanned.quarantine_records.empty())
		{
			// A corrupt frame's header is not an authoritative PID. Archive its exact
			// bytes, but never compact the active copy or allow any PID to load/save.
			const bool archived = commit_quarantine_archive(
				{}, std::move(scanned.quarantine_records));
			scanned.result = archived ? player_save_journal_result::corrupt_data :
						    player_save_journal_result::io_failure;
			fail_closed();
		}
		return scanned;
	}
	catch (const std::bad_alloc &)
	{
		fail_closed();
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
	health.record_limit_exceeded = health.records >= PLAYER_SAVE_JOURNAL_MAX_RECORDS;
}

bool quarantine_configured_frames()
{
	scan_result scanned = scan_journal_safe();
	if (scanned.result != player_save_journal_result::ok)
		return false;
	std::vector<scan_result::quarantine_record> additions;
	std::vector<journal_frame> retained;
	bool removed = false;
	try
	{
		retained.reserve(scanned.frames.size());
		for (journal_frame &frame : scanned.frames)
		{
			const bool pending_verification =
				std::any_of(recovery_records.begin(), recovery_records.end(),
					    [&](const stored_recovery &stored)
					    {
						    return stored.resolved && !stored.revoked &&
							   stored.record.replacement.pid ==
								   frame.snapshot.pid &&
							   !policy_pids.count(frame.snapshot.pid);
					    });
			if (!quarantined_pids.count(frame.snapshot.pid) || pending_verification)
			{
				retained.push_back(std::move(frame));
				continue;
			}
			scan_result::quarantine_record record;
			record.pid = frame.snapshot.pid;
			record.reason = QUARANTINE_EXPLICIT_PID;
			record.bytes = std::move(frame.bytes);
			additions.push_back(std::move(record));
			removed = true;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!commit_quarantine_archive({}, std::move(additions)))
		return false;
	if (removed && !write_compacted(retained))
		return false;
	refresh_health(retained);
	return true;
}

player_save_journal_result quarantine_runtime_pid(int pid)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	if (pid <= 0)
		return player_save_journal_result::corrupt_data;
	if (quarantined_pids.count(pid))
		return player_save_journal_result::quarantined_pid;
	auto fail_closed = []()
	{
		health.initialized = false;
		quarantine_state_ready = false;
		quarantine_state_failed = true;
		return player_save_journal_result::io_failure;
	};
	scan_result scanned = scan_journal_safe();
	if (scanned.result != player_save_journal_result::ok)
		return fail_closed();
	std::vector<scan_result::quarantine_record> additions;
	std::vector<journal_frame> retained;
	try
	{
		retained.reserve(scanned.frames.size());
		for (journal_frame &frame : scanned.frames)
		{
			if (frame.snapshot.pid != pid)
			{
				retained.push_back(std::move(frame));
				continue;
			}
			scan_result::quarantine_record record;
			record.pid = pid;
			record.reason = QUARANTINE_RUNTIME_TERMINAL;
			record.bytes = std::move(frame.bytes);
			additions.push_back(std::move(record));
		}
	}
	catch (const std::bad_alloc &)
	{
		return fail_closed();
	}
	try
	{
		if (!commit_quarantine_archive({ pid }, std::move(additions)) ||
		    !write_compacted(retained))
			return fail_closed();
	}
	catch (const std::bad_alloc &)
	{
		return fail_closed();
	}
	refresh_health(retained);
	return player_save_journal_result::ok;
}
} // namespace

bool player_save_journal_init(const char *directory, size_t quota_bytes)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (health.initialized)
		return false;
	try
	{
		quarantine_state_ready = false;
		quarantine_state_failed = true;
		health = {};
		quarantined_pids.clear();
		archived_pids.clear();
		archive_records.clear();
		recovery_records.clear();
		policy_pids.clear();
		if (!directory || !*directory || directory[0] != '/' ||
		    quota_bytes < JOURNAL_HEADER_SIZE + 64 ||
		    quota_bytes > PLAYER_SAVE_JOURNAL_MAX_BYTES)
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
		archive_path = journal_directory + "/" + JOURNAL_ARCHIVE_NAME;
		archive_temporary_path = journal_directory + "/" + JOURNAL_ARCHIVE_TEMP_NAME;
		quarantine_pids_path = journal_directory + "/" + JOURNAL_QUARANTINE_PIDS_NAME;
		journal_quota = quota_bytes;
		const std::string temporary = journal_directory + "/" + JOURNAL_TEMP_NAME;
		unlink(temporary.c_str());
		if (!load_quarantine_archive() || !load_quarantine_pid_policy() ||
		    !load_legacy_quarantine_bytes())
			return false;
		const int fd = open_safe_file(journal_path, O_WRONLY | O_APPEND | O_CREAT, 0600);
		if (fd < 0)
			return false;
		if (close(fd) != 0 || !sync_directory())
			return false;
		health.initialized = true;
		health.quarantined_bytes = 0;
		for (const auto &record : archive_records)
			health.quarantined_bytes += record.bytes.size();
		scan_result scanned = scan_journal_safe();
		if (scanned.result != player_save_journal_result::ok ||
		    !quarantine_configured_frames())
		{
			health.initialized = false;
			quarantine_state_failed = true;
			return false;
		}
		scan_result remaining = scan_journal_safe();
		if (remaining.result != player_save_journal_result::ok)
		{
			health.initialized = false;
			quarantine_state_failed = true;
			return false;
		}
		refresh_health(remaining.frames);
		struct stat file_status = {};
		if (stat(journal_path.c_str(), &file_status) != 0 || file_status.st_size < 0)
		{
			health.initialized = false;
			quarantine_state_failed = true;
			return false;
		}
		health.bytes = static_cast<uint64_t>(file_status.st_size);
		quarantine_state_ready = true;
		quarantine_state_failed = false;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		health.initialized = false;
		quarantine_state_ready = false;
		quarantine_state_failed = true;
		return false;
	}
}

void player_save_journal_shutdown(void)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	health.initialized = false;
	quarantine_state_ready = false;
	quarantine_state_failed = true;
	journal_directory.clear();
	journal_path.clear();
	quarantine_path.clear();
	archive_path.clear();
	archive_temporary_path.clear();
	quarantine_pids_path.clear();
	quarantined_pids.clear();
	archived_pids.clear();
	archive_records.clear();
	recovery_records.clear();
	policy_pids.clear();
	journal_quota = 0;
	oldest_record_msec = 0;
}

bool player_save_journal_pid_quarantined(int pid)
{
	if (pid <= 0)
		return false;
	std::lock_guard<std::mutex> lock(journal_mutex);
	return quarantine_state_failed || (quarantine_state_ready && quarantined_pids.count(pid));
}

player_save_journal_diagnostic player_save_journal_diagnostic_copy(int pid)
{
	player_save_journal_diagnostic result;
	std::unique_lock<std::mutex> lock(journal_mutex, std::try_to_lock);
	if (!lock.owns_lock())
		return result;
	result.available = true;
	result.health = health;
	result.global_fence = quarantine_state_failed;
	result.pid_fence = quarantined_pids.count(pid);
	result.policy_fence = policy_pids.count(pid);
	for (const auto &record : archive_records)
		if (record.pid == pid)
		{
			++result.archived_frames;
			result.archived_bytes += record.bytes.size();
		}
	for (const auto &stored : recovery_records)
		if (stored.record.replacement.pid == pid)
		{
			result.recovery_prepared = true;
			result.recovery_resolved = stored.resolved;
			result.recovery_revoked = stored.revoked;
			result.replacement_revision = stored.record.replacement.revision;
		}
	return result;
}

player_save_journal_result player_save_journal_append(const player_snapshot &snapshot)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	if (quarantined_pids.count(snapshot.pid))
		return player_save_journal_result::quarantined_pid;
	if (health.records >= PLAYER_SAVE_JOURNAL_MAX_RECORDS)
	{
		++health.append_failures;
		++health.backpressure;
		health.record_limit_exceeded = true;
		return player_save_journal_result::quota_exceeded;
	}
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
	health.record_limit_exceeded = health.records >= PLAYER_SAVE_JOURNAL_MAX_RECORDS;
	if (!oldest_record_msec || frame.created_msec < oldest_record_msec)
		oldest_record_msec = frame.created_msec;
	return player_save_journal_result::ok;
}

player_save_journal_result player_save_journal_archive_quarantined(const player_snapshot &snapshot)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	if (!quarantined_pids.count(snapshot.pid))
		return player_save_journal_result::replay_blocked;
	try
	{
		journal_frame frame;
		if (!build_frame(snapshot, frame))
			return player_save_journal_result::encode_failure;
		scan_result::quarantine_record record;
		record.pid = snapshot.pid;
		record.reason = QUARANTINE_RUNTIME_TERMINAL;
		record.bytes = std::move(frame.bytes);
		std::vector<scan_result::quarantine_record> additions;
		additions.push_back(std::move(record));
		if (!commit_quarantine_archive({}, std::move(additions)))
			return player_save_journal_result::io_failure;
	}
	catch (const std::bad_alloc &)
	{
		return player_save_journal_result::io_failure;
	}
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
	if (quarantined_pids.count(pid))
		return player_save_journal_result::quarantined_pid;
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
			if (frame.snapshot.pid != pid ||
			    frame.snapshot.revision > durable_revision || frame.snapshot.death ||
			    !frame.snapshot.quest_xp_receipts.empty() ||
			    !frame.snapshot.spell_effect_receipts.empty() ||
			    !frame.snapshot.craft_receipts.empty())
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

namespace
{
struct operation_record_proof
{
	int pid;
	player_revision_t revision;
	std::vector<uint8_t> payload;
};

bool is_proven_operation(const journal_frame &frame,
			 const std::vector<operation_record_proof> &proofs)
{
	if (!frame.snapshot.death && frame.snapshot.quest_xp_receipts.empty() &&
	    frame.snapshot.spell_effect_receipts.empty() && frame.snapshot.craft_receipts.empty())
		return false;
	for (const operation_record_proof &proof : proofs)
		if (frame.snapshot.pid == proof.pid && frame.snapshot.revision == proof.revision &&
		    frame.bytes.size() == JOURNAL_HEADER_SIZE + proof.payload.size() &&
		    std::equal(proof.payload.begin(), proof.payload.end(),
			       frame.bytes.begin() + JOURNAL_HEADER_SIZE))
			return true;
	return false;
}

player_save_journal_result checkpoint_proven(const std::map<int, player_revision_t> &acknowledged,
					     const std::vector<operation_record_proof> &proofs,
					     bool require_exact_match = false)
{
	if (acknowledged.empty() && proofs.empty())
		return player_save_journal_result::ok;
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized)
		return player_save_journal_result::not_initialized;
	for (const auto &[pid, revision] : acknowledged)
	{
		(void)revision;
		if (quarantined_pids.count(pid))
			return player_save_journal_result::quarantined_pid;
	}
	for (const operation_record_proof &proof : proofs)
		if (quarantined_pids.count(proof.pid))
			return player_save_journal_result::quarantined_pid;
	scan_result scanned = scan_journal_safe();
	if (scanned.result != player_save_journal_result::ok)
	{
		++health.checkpoint_failures;
		return scanned.result;
	}
	std::vector<journal_frame> retained;
	bool removed = false;
	bool matched_operation = false;
	try
	{
		retained.reserve(scanned.frames.size());
		for (journal_frame &frame : scanned.frames)
		{
			const bool operation_match = is_proven_operation(frame, proofs);
			const auto found = acknowledged.find(frame.snapshot.pid);
			const bool ordinary_match = !frame.snapshot.death &&
						    frame.snapshot.quest_xp_receipts.empty() &&
						    frame.snapshot.spell_effect_receipts.empty() &&
						    frame.snapshot.craft_receipts.empty() &&
						    found != acknowledged.end() &&
						    frame.snapshot.revision <= found->second;
			if (operation_match || ordinary_match)
			{
				removed = true;
				matched_operation |= operation_match;
			}
			else
				retained.push_back(std::move(frame));
		}
	}
	catch (const std::bad_alloc &)
	{
		++health.checkpoint_failures;
		return player_save_journal_result::io_failure;
	}
	if (require_exact_match && !matched_operation)
	{
		// Absence after a completed rename is retryable; a different retained
		// payload at this exact identity is conflicting evidence, not absence.
		for (const auto &proof : proofs)
		{
			for (const auto &frame : retained)
			{
				if (frame.snapshot.pid == proof.pid &&
				    frame.snapshot.revision == proof.revision)
					return player_save_journal_result::replay_blocked;
			}
		}
		// A previous compaction can rename successfully and then fail to sync
		// the directory. Its ACK reports failure, but the exact record is now
		// absent. The worker has already verified this snapshot's DB receipt;
		// make a retry idempotent only after syncing the observed file state.
		if (!sync_directory())
		{
			++health.checkpoint_failures;
			return player_save_journal_result::io_failure;
		}
		++health.checkpoints;
		refresh_health(retained);
		return player_save_journal_result::ok;
	}
	if (!removed)
		return player_save_journal_result::ok;
	if (!write_compacted(retained))
	{
		++health.checkpoint_failures;
		return player_save_journal_result::io_failure;
	}
	++health.checkpoints;
	refresh_health(retained);
	return player_save_journal_result::ok;
}
} // namespace

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
		for (journal_frame &frame : scanned.frames)
		{
			if (quarantined_pids.count(frame.snapshot.pid))
			{
				const bool resolved_pending_verification = std::any_of(
					recovery_records.begin(), recovery_records.end(),
					[&](const stored_recovery &stored)
					{
						return stored.resolved && !stored.revoked &&
						       stored.record.replacement.pid ==
							       frame.snapshot.pid &&
						       !policy_pids.count(frame.snapshot.pid);
					});
				if (!resolved_pending_verification)
					return player_save_journal_result::replay_blocked;
				// Keep its original active frames in the journal. Only this PID
				// waits for native recovery verification; unrelated PIDs replay.
				continue;
			}
			frames.push_back(std::move(frame));
		}
	}
	std::map<int, player_revision_t> acknowledged;
	std::vector<operation_record_proof> proven_operations;
	std::set<int> runtime_quarantined_pids;
	auto quarantine_failed_pid = [&](int pid)
	{
		const player_save_journal_result quarantined = quarantine_runtime_pid(pid);
		if (quarantined != player_save_journal_result::ok)
			return quarantined;
		runtime_quarantined_pids.insert(pid);
		acknowledged.erase(pid);
		proven_operations.erase(std::remove_if(proven_operations.begin(),
						       proven_operations.end(),
						       [&](const operation_record_proof &proof)
						       { return proof.pid == pid; }),
					proven_operations.end());
		return player_save_journal_result::ok;
	};
	auto stop_replay = [&]()
	{
		const player_save_journal_result drained =
			checkpoint_proven(acknowledged, proven_operations);
		return drained == player_save_journal_result::ok ?
			       player_save_journal_result::replay_blocked :
			       drained;
	};
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
		std::unordered_map<std::string, std::vector<const journal_frame *>> identities;
		for (const journal_frame &frame : frames)
		{
			if (runtime_quarantined_pids.count(frame.snapshot.pid))
				continue;
			const std::string identity = std::to_string(frame.snapshot.pid) + ":" +
						     std::to_string(frame.snapshot.revision) + ":" +
						     std::to_string(frame.snapshot.components) +
						     ":" + std::to_string(frame.payload_checksum);
			auto &same_identity = identities[identity];
			const bool duplicate = std::any_of(
				same_identity.begin(), same_identity.end(),
				[&](const journal_frame *previous)
				{
					return std::equal(previous->bytes.begin() +
								  JOURNAL_HEADER_SIZE,
							  previous->bytes.end(),
							  frame.bytes.begin() + JOURNAL_HEADER_SIZE,
							  frame.bytes.end());
				});
			if (duplicate)
			{
				std::lock_guard<std::mutex> lock(journal_mutex);
				++health.duplicates;
				continue;
			}
			same_identity.push_back(&frame);
			player_save_apply_result applied = {};
			try
			{
				applied = apply(frame.snapshot, context);
			}
			catch (...)
			{
				const player_save_journal_result quarantined =
					quarantine_failed_pid(frame.snapshot.pid);
				if (quarantined != player_save_journal_result::ok)
					return quarantined;
				continue;
			}
			if (applied.outcome == player_save_apply_outcome::retryable_failure ||
			    applied.outcome == player_save_apply_outcome::ambiguous_commit)
			{
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					++health.backpressure;
				}
				return stop_replay();
			}
			if (applied.outcome != player_save_apply_outcome::applied &&
			    applied.outcome != player_save_apply_outcome::already_applied &&
			    applied.outcome != player_save_apply_outcome::stale_revision)
			{
				{
					std::lock_guard<std::mutex> lock(journal_mutex);
					++health.backpressure;
				}
				const player_save_journal_result quarantined =
					quarantine_failed_pid(frame.snapshot.pid);
				if (quarantined != player_save_journal_result::ok)
					return quarantined;
				continue;
			}
			// A newer save does not prove that an attached disposition or
			// operation receipt committed. Require exact success or explicit
			// verification of every operation in an obsolete non-death frame.
			if (frame.snapshot.death || !frame.snapshot.quest_xp_receipts.empty() ||
			    !frame.snapshot.spell_effect_receipts.empty() ||
			    !frame.snapshot.craft_receipts.empty())
			{
				const bool verified_obsolete_operations =
					!frame.snapshot.death &&
					applied.operation_receipts_verified &&
					applied.outcome ==
						player_save_apply_outcome::stale_revision &&
					applied.durable_revision > frame.snapshot.revision;
				if (!player_save_result_matches_exact_request(frame.snapshot,
									      applied) &&
				    !verified_obsolete_operations)
				{
					const player_save_journal_result quarantined =
						quarantine_failed_pid(frame.snapshot.pid);
					if (quarantined != player_save_journal_result::ok)
						return quarantined;
					continue;
				}
				proven_operations.push_back(
					{ frame.snapshot.pid, frame.snapshot.revision,
					  std::vector<uint8_t>(frame.bytes.begin() +
								       JOURNAL_HEADER_SIZE,
							       frame.bytes.end()) });
			}
			acknowledged[frame.snapshot.pid] = std::max(
				acknowledged[frame.snapshot.pid], applied.durable_revision);
			std::lock_guard<std::mutex> lock(journal_mutex);
			++health.replayed;
		}
		return checkpoint_proven(acknowledged, proven_operations);
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

void player_save_journal_worker_terminal(const player_snapshot &snapshot, void *context) noexcept
{
	(void)context;
	try
	{
		const auto result = quarantine_runtime_pid(snapshot.pid);
		if (result == player_save_journal_result::ok ||
		    result == player_save_journal_result::quarantined_pid)
			return;
	}
	catch (...)
	{
		// Allocation or unexpected archive errors must not release admission.
	}
	std::lock_guard<std::mutex> lock(journal_mutex);
	health.initialized = false;
	quarantine_state_ready = false;
	quarantine_state_failed = true;
}

bool player_save_journal_worker_append(const player_snapshot &snapshot, void *context)
{
	(void)context;
	return player_save_journal_append(snapshot) == player_save_journal_result::ok;
}

bool player_save_journal_worker_ack(const player_snapshot &snapshot,
				    player_revision_t durable_revision, void *context)
{
	(void)context;
	if (!snapshot.death && snapshot.quest_xp_receipts.empty() &&
	    snapshot.spell_effect_receipts.empty() && snapshot.craft_receipts.empty())
		return player_save_journal_checkpoint(snapshot.pid, durable_revision) ==
		       player_save_journal_result::ok;
	if (durable_revision != snapshot.revision)
		return false;
	std::vector<uint8_t> encoded;
	if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
		return false;
	return checkpoint_proven({}, { { snapshot.pid, snapshot.revision, std::move(encoded) } },
				 true) == player_save_journal_result::ok;
}

player_save_journal_result
player_save_journal_recovery_inspect(int pid, std::vector<player_snapshot> *frames,
				     std::array<uint8_t, 32> *digest)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized || quarantine_state_failed)
		return player_save_journal_result::not_initialized;
	if (!quarantined_pids.count(pid) || policy_pids.count(pid))
		return player_save_journal_result::replay_blocked;
	try
	{
		return archive_recovery_frames(pid, frames, digest) ?
			       player_save_journal_result::ok :
			       player_save_journal_result::corrupt_data;
	}
	catch (const std::bad_alloc &)
	{
		return player_save_journal_result::io_failure;
	}
}

namespace
{
bool recovery_equal(const player_save_recovery_record &left,
		    const player_save_recovery_record &right)
{
	std::vector<uint8_t> a, b;
	return encode_recovery({ left, false }, &a) && encode_recovery({ right, false }, &b) &&
	       a == b;
}

bool recovery_generation_matches(const player_save_recovery_record &record)
{
	std::vector<player_snapshot> frames;
	std::array<uint8_t, 32> digest;
	if (!archive_recovery_frames(record.replacement.pid, &frames, &digest) ||
	    digest != record.archive_digest || policy_pids.count(record.replacement.pid))
		return false;
	return std::all_of(frames.begin(), frames.end(),
			   [&](const player_snapshot &frame)
			   { return frame.revision < record.replacement.revision; });
}
} // namespace

player_save_journal_result
player_save_journal_recovery_prepare(const player_save_recovery_record &record)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized || quarantine_state_failed)
		return player_save_journal_result::not_initialized;
	try
	{
		std::vector<uint8_t> encoded;
		if (!quarantined_pids.count(record.replacement.pid) ||
		    !encode_recovery({ record, false }, &encoded) ||
		    !recovery_generation_matches(record))
			return player_save_journal_result::replay_blocked;
		for (const auto &stored : recovery_records)
		{
			if (stored.record.identity == record.identity)
				return !stored.revoked && recovery_equal(stored.record, record) ?
					       player_save_journal_result::ok :
					       player_save_journal_result::replay_blocked;
			if (stored.record.replacement.pid == record.replacement.pid &&
			    stored.record.archive_digest == record.archive_digest)
				return player_save_journal_result::replay_blocked;
		}
		auto next = recovery_records;
		next.push_back({ record, false });
		if (!persist_quarantine_archive(archived_pids, {}, &next))
		{
			// Rename may already have succeeded. Prevent later writes from
			// forgetting that uncertain prepared record until reinitialization.
			quarantine_state_failed = true;
			return player_save_journal_result::io_failure;
		}
		recovery_records.swap(next);
		return player_save_journal_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_save_journal_result::io_failure;
	}
}

player_save_journal_result
player_save_journal_recovery_read(int pid, player_save_recovery_record *record, bool *resolved)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized || quarantine_state_failed)
		return player_save_journal_result::not_initialized;
	if (!record || !resolved)
		return player_save_journal_result::corrupt_data;
	try
	{
		for (auto found = recovery_records.rbegin(); found != recovery_records.rend();
		     ++found)
			if (!found->revoked && found->record.replacement.pid == pid &&
			    recovery_generation_matches(found->record))
			{
				*record = found->record;
				*resolved = found->resolved;
				return player_save_journal_result::ok;
			}
		return player_save_journal_result::replay_blocked;
	}
	catch (const std::bad_alloc &)
	{
		return player_save_journal_result::io_failure;
	}
}

bool player_save_journal_recovery_matches(const player_save_recovery_record &record)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized || quarantine_state_failed)
		return false;
	try
	{
		return recovery_generation_matches(record) &&
		       std::any_of(recovery_records.begin(), recovery_records.end(),
				   [&](const stored_recovery &stored) {
					   return !stored.revoked &&
						  recovery_equal(stored.record, record);
				   });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

player_save_journal_result
player_save_journal_recovery_resolve(const player_save_recovery_record &record,
				     player_save_recovery_verify_fn verify, void *context)
{
	// Backend proof may query the journal and must not run under its mutex.
	try
	{
		if (!verify || !player_save_journal_recovery_matches(record) ||
		    !verify(record, context))
			return player_save_journal_result::replay_blocked;
	}
	catch (...)
	{
		// A failed native verifier cannot grant admission or erase evidence.
		return player_save_journal_result::replay_blocked;
	}
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!health.initialized || quarantine_state_failed)
		return player_save_journal_result::not_initialized;
	try
	{
		if (!recovery_generation_matches(record))
			return player_save_journal_result::replay_blocked;
		auto next = recovery_records;
		for (auto &stored : next)
			if (!stored.revoked && recovery_equal(stored.record, record))
			{
				if (!stored.resolved)
				{
					stored.resolved = true;
					if (!persist_quarantine_archive(archived_pids, {}, &next))
					{
						quarantine_state_failed = true;
						return player_save_journal_result::io_failure;
					}
					recovery_records.swap(next);
				}
				quarantined_pids.erase(record.replacement.pid);
				return player_save_journal_result::ok;
			}
		return player_save_journal_result::replay_blocked;
	}
	catch (const std::bad_alloc &)
	{
		return player_save_journal_result::io_failure;
	}
}

bool player_save_journal_recovery_fingerprint(const player_save_recovery_record &record,
					      std::array<uint8_t, 32> *digest)
{
	if (!digest)
		return false;
	try
	{
		std::vector<uint8_t> encoded;
		if (!encode_recovery({ record, false }, &encoded))
			return false;
		*digest = sha256(encoded.data(), encoded.size());
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool player_save_journal_resolved_recoveries(std::vector<player_save_recovery_record> *records)
{
	std::lock_guard<std::mutex> lock(journal_mutex);
	if (!records || !health.initialized || quarantine_state_failed)
		return false;
	try
	{
		std::vector<player_save_recovery_record> next;
		for (const auto &stored : recovery_records)
			if (stored.resolved && !stored.revoked &&
			    recovery_generation_matches(stored.record))
				next.push_back(stored.record);
		records->swap(next);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
