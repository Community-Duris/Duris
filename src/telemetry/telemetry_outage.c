#include "telemetry/telemetry_outage_private.h"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

#include <fcntl.h>
#include <openssl/sha.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr const char *LEDGER = "outages.ledger";
constexpr const char *PENDING = "outages.pending";
constexpr const char *LOCK = "outages.owner";
constexpr unsigned char LEGACY_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '1' };
constexpr unsigned char BATTLE_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '2' };
constexpr unsigned char CONTRIBUTION_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '3' };
constexpr unsigned char BUILD_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '4' };
constexpr unsigned char CONTROL_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '5' };
constexpr unsigned char RESULT_MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '6' };
constexpr unsigned char MAGIC[] = { 'D', 'M', 'S', 'T', 'L', 'J', '0', '7' };
using words = std::array<std::uint64_t, TELEMETRY_OUTAGE_DISK_WORDS>;
constexpr std::uint64_t LEGACY_KINDS = (std::uint64_t{ 1U } << 10U) - 2U;
constexpr std::uint64_t BATTLE_KINDS = (std::uint64_t{ 1U } << 11U) - 2U;
constexpr std::uint64_t CONTRIBUTION_KINDS = (std::uint64_t{ 1U } << 12U) - 2U;
constexpr std::uint64_t BUILD_KINDS = (std::uint64_t{ 1U } << 13U) - 2U;
constexpr std::uint64_t CONTROL_KINDS = (std::uint64_t{ 1U } << 14U) - 2U;
constexpr std::uint64_t RESULT_KINDS = (std::uint64_t{ 1U } << 15U) - 2U;
constexpr std::uint64_t KNOWN_KINDS = (std::uint64_t{ 1U } << 17U) - 2U;
words encode(const telemetry_outage_observation &value);

struct file_guard
{
	int fd;
	~file_guard()
	{
		if (fd >= 0)
			close(fd);
	}
};

telemetry_outage_result failure(telemetry_outage_journal &journal, telemetry_outage_result result,
				int error)
{
	journal.error_code = static_cast<std::uint32_t>(error);
	return result;
}

bool same_producer(telemetry_producer_id left, telemetry_producer_id right)
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool clean(const telemetry_outage_observation &value)
{
	const auto &h = value.health;
	if (h.admitted_detail >= UINT64_MAX - h.admitted_control ||
	    h.applied_records >= UINT64_MAX - h.duplicate_records)
		return false;
	const auto accepted = h.admitted_detail + h.admitted_control;
	const auto acknowledged = h.applied_records + h.duplicate_records;
	return acknowledged < UINT64_MAX - h.stale_checkpoint_records &&
	       accepted == acknowledged + h.stale_checkpoint_records && h.queue_depth == 0U &&
	       value.inflight_records == 0U && h.invalid_records == 0U &&
	       h.conflict_records == 0U && h.quarantined_records == 0U;
}

bool valid(const telemetry_outage_observation &value)
{
	const auto phase = static_cast<std::uint8_t>(value.phase);
	const auto &h = value.health;
	return telemetry_producer_id_is_valid(value.producer) && value.environment_id != 0U &&
	       value.season_id != 0U &&
	       value.observed_monotonic_usec >= value.registered_monotonic_usec && phase >= 1U &&
	       phase <= 4U && (value.record_kind_mask & ~KNOWN_KINDS) == 0U &&
	       (h.last_failure_record_kind_mask & ~KNOWN_KINDS) == 0U &&
	       static_cast<std::uint8_t>(h.last_failure_class) <=
		       static_cast<std::uint8_t>(telemetry_failure_class::permanent_repository) &&
	       h.last_committed_record_seq <= h.last_admitted_record_seq &&
	       h.queue_depth <= TELEMETRY_QUEUE_CAPACITY_PROPOSAL &&
	       value.inflight_records <= TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL &&
	       value.unattempted_records <= h.queue_depth &&
	       value.inflight_records + value.unattempted_records == h.queue_depth &&
	       h.inflight_active <= 1U &&
	       (h.inflight_active != 0U) == (value.inflight_records != 0U) &&
	       (h.inflight_active == 0U ||
		(h.inflight_first_record_seq != 0U &&
		 h.inflight_last_record_seq >= h.inflight_first_record_seq &&
		 h.inflight_last_record_seq <= h.last_admitted_record_seq)) &&
	       (value.phase != telemetry_outage_phase::clean_drained || clean(value)) &&
	       (value.phase != telemetry_outage_phase::abandoned ||
		(value.inflight_records == 0U && value.unattempted_records != 0U));
}

bool registration_is_empty(const telemetry_outage_observation &value)
{
	if (!valid(value) || value.phase != telemetry_outage_phase::running ||
	    value.observed_monotonic_usec != value.registered_monotonic_usec ||
	    value.observed_utc_usec != value.registered_utc_usec || value.record_kind_mask != 0U)
		return false;
	const auto fields = encode(value);
	return std::all_of(fields.begin() + 10U, fields.end(),
			   [](auto field) { return field == 0U; });
}

bool extends(const telemetry_outage_observation &previous, const telemetry_outage_observation &next)
{
	const auto left = encode(previous);
	const auto right = encode(next);
	if (previous.phase != telemetry_outage_phase::running)
		return left == right;
	if (!std::equal(left.begin(), left.begin() + 6U, right.begin()) || right[6] < left[6] ||
	    (right[9] & left[9]) != left[9])
		return false;
	for (const auto field : { 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U, 21U, 28U,
				  29U, 30U, 31U, 39U })
		if (right[field] < left[field])
			return false;
	return true;
}

words encode(const telemetry_outage_observation &v)
{
	const auto &h = v.health;
	return { v.producer.boot_id,
		 v.producer.process_id,
		 v.environment_id,
		 v.season_id,
		 v.registered_monotonic_usec,
		 std::bit_cast<std::uint64_t>(v.registered_utc_usec),
		 v.observed_monotonic_usec,
		 std::bit_cast<std::uint64_t>(v.observed_utc_usec),
		 static_cast<std::uint64_t>(v.phase),
		 v.record_kind_mask,
		 h.admitted_detail,
		 h.admitted_control,
		 h.last_admitted_record_seq,
		 h.applied_records,
		 h.duplicate_records,
		 h.stale_checkpoint_records,
		 h.quarantined_records,
		 h.invalid_records,
		 h.conflict_records,
		 h.dropped_detail,
		 h.dropped_control,
		 h.last_committed_record_seq,
		 h.inflight_first_record_seq,
		 h.inflight_last_record_seq,
		 v.inflight_records,
		 v.unattempted_records,
		 h.queue_depth,
		 h.inflight_active,
		 h.retryable_failures,
		 h.ambiguous_commits,
		 h.sequence_gap_count,
		 h.unclosed_tail_count,
		 static_cast<std::uint64_t>(h.last_failure_class),
		 h.last_error_code,
		 h.last_failure_first_record_seq,
		 h.last_failure_last_record_seq,
		 h.last_failure_record_kind_mask,
		 h.last_success_monotonic_usec,
		 h.last_failure_monotonic_usec,
		 h.circuit_open_count };
}

bool decode(const words &w, telemetry_outage_observation &v)
{
	if (w[8] > UINT8_MAX || w[24] > UINT32_MAX || w[25] > UINT32_MAX || w[27] > 1U ||
	    w[32] > static_cast<std::uint64_t>(telemetry_failure_class::permanent_repository) ||
	    w[33] > UINT32_MAX)
		return false;
	v = {};
	v.producer = { w[0], w[1] };
	v.environment_id = w[2];
	v.season_id = w[3];
	v.registered_monotonic_usec = w[4];
	v.registered_utc_usec = std::bit_cast<std::int64_t>(w[5]);
	v.observed_monotonic_usec = w[6];
	v.observed_utc_usec = std::bit_cast<std::int64_t>(w[7]);
	v.phase = static_cast<telemetry_outage_phase>(w[8]);
	v.record_kind_mask = w[9];
	auto &h = v.health;
	h.producer = v.producer;
	h.schema_version = TELEMETRY_SCHEMA_VERSION;
	h.backend = telemetry_storage_backend::sql;
	h.admitted_detail = w[10];
	h.admitted_control = w[11];
	h.last_admitted_record_seq = w[12];
	h.applied_records = w[13];
	h.duplicate_records = w[14];
	h.stale_checkpoint_records = w[15];
	h.quarantined_records = w[16];
	h.invalid_records = w[17];
	h.conflict_records = w[18];
	h.dropped_detail = w[19];
	h.dropped_control = w[20];
	h.last_committed_record_seq = w[21];
	h.inflight_first_record_seq = w[22];
	h.inflight_last_record_seq = w[23];
	v.inflight_records = static_cast<std::uint32_t>(w[24]);
	v.unattempted_records = static_cast<std::uint32_t>(w[25]);
	h.queue_depth = w[26];
	h.inflight_active = static_cast<std::uint8_t>(w[27]);
	h.retryable_failures = w[28];
	h.ambiguous_commits = w[29];
	h.sequence_gap_count = w[30];
	h.unclosed_tail_count = w[31];
	h.last_failure_class = static_cast<telemetry_failure_class>(w[32]);
	h.last_error_code = static_cast<std::uint32_t>(w[33]);
	h.last_failure_first_record_seq = w[34];
	h.last_failure_last_record_seq = w[35];
	h.last_failure_record_kind_mask = w[36];
	h.last_success_monotonic_usec = w[37];
	h.last_failure_monotonic_usec = w[38];
	h.circuit_open_count = w[39];
	return valid(v);
}

void append_word(std::vector<unsigned char> &data, std::uint64_t value)
{
	for (unsigned int shift = 64U; shift != 0U; shift -= 8U)
		data.push_back(static_cast<unsigned char>(value >> (shift - 8U)));
}

std::uint64_t read_word(const unsigned char *data)
{
	std::uint64_t value = 0U;
	for (unsigned int index = 0U; index < 8U; ++index)
		value = (value << 8U) | data[index];
	return value;
}

bool safe_file(int fd, struct stat &status)
{
	return fstat(fd, &status) == 0 && S_ISREG(status.st_mode) && status.st_uid == geteuid() &&
	       (status.st_mode & 07777U) == 0600U && status.st_nlink == 1U;
}

telemetry_outage_result load(telemetry_outage_journal &j, const char *name, bool &present)
{
	present = false;
	const int fd = openat(j.directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
		return errno == ENOENT ? telemetry_outage_result::ready :
					 failure(j, telemetry_outage_result::io_failure, errno);
	present = true;
	file_guard guard{ fd };
	struct stat status
	{
	};
	if (!safe_file(fd, status))
	{
		return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
	}
	if (status.st_size < 64 ||
	    static_cast<std::uint64_t>(status.st_size) > TELEMETRY_OUTAGE_MAX_BYTES)
	{
		return failure(j, telemetry_outage_result::corrupt, EBADMSG);
	}
	std::vector<unsigned char> data(static_cast<std::size_t>(status.st_size));
	std::size_t offset = 0U;
	while (offset < data.size())
	{
		const auto received = read(fd, data.data() + offset, data.size() - offset);
		if (received < 0 && errno == EINTR)
			continue;
		if (received <= 0)
		{
			const int error = received == 0 ? EBADMSG : errno;
			return failure(j, telemetry_outage_result::io_failure, error);
		}
		offset += static_cast<std::size_t>(received);
	}
	unsigned char digest[SHA256_DIGEST_LENGTH]{};
	if (SHA256(data.data(), data.size() - sizeof(digest), digest) == nullptr)
		return failure(j, telemetry_outage_result::io_failure, EIO);
	const bool legacy = std::memcmp(data.data(), LEGACY_MAGIC, sizeof(LEGACY_MAGIC)) == 0;
	const bool battle_version = std::memcmp(data.data(), BATTLE_MAGIC, sizeof(BATTLE_MAGIC)) ==
				    0;
	const bool contribution_version =
		std::memcmp(data.data(), CONTRIBUTION_MAGIC, sizeof(CONTRIBUTION_MAGIC)) == 0;
	const bool build_version = std::memcmp(data.data(), BUILD_MAGIC, sizeof(BUILD_MAGIC)) == 0;
	const bool control_version =
		std::memcmp(data.data(), CONTROL_MAGIC, sizeof(CONTROL_MAGIC)) == 0;
	const bool result_version = std::memcmp(data.data(), RESULT_MAGIC, sizeof(RESULT_MAGIC)) ==
				    0;
	const auto known_kinds = legacy		      ? LEGACY_KINDS :
				 battle_version	      ? BATTLE_KINDS :
				 contribution_version ? CONTRIBUTION_KINDS :
				 build_version	      ? BUILD_KINDS :
				 control_version      ? CONTROL_KINDS :
				 result_version	      ? RESULT_KINDS :
							KNOWN_KINDS;
	if ((!legacy && !battle_version && !contribution_version && !build_version &&
	     !control_version && !result_version &&
	     std::memcmp(data.data(), MAGIC, sizeof(MAGIC)) != 0) ||
	    std::memcmp(data.data() + data.size() - sizeof(digest), digest, sizeof(digest)) != 0)
		return failure(j, telemetry_outage_result::corrupt, EBADMSG);
	const auto generation = read_word(data.data() + 8U);
	const auto count = read_word(data.data() + 16U);
	if (generation == 0U || count == 0U || count > TELEMETRY_OUTAGE_MAX_PRODUCERS ||
	    read_word(data.data() + 24U) != 0U ||
	    data.size() != 64U + count * TELEMETRY_OUTAGE_DISK_WORDS * 8U)
		return failure(j, telemetry_outage_result::corrupt, EBADMSG);
	for (std::size_t index = 0U; index < count; ++index)
	{
		words fields{};
		for (std::size_t field = 0U; field < fields.size(); ++field)
			fields[field] =
				read_word(data.data() + 32U + (index * fields.size() + field) * 8U);
		if ((((fields[9] | fields[36]) & ~known_kinds) != 0U) ||
		    !decode(fields, j.observations[index]) ||
		    (index + 1U < count &&
		     j.observations[index].phase == telemetry_outage_phase::running))
			return failure(j, telemetry_outage_result::corrupt, EBADMSG);
		for (std::size_t previous = 0U; previous < index; ++previous)
			if (same_producer(j.observations[index].producer,
					  j.observations[previous].producer))
				return failure(j, telemetry_outage_result::corrupt, EBADMSG);
	}
	j.generation = generation;
	j.count = static_cast<std::size_t>(count);
	return telemetry_outage_result::ready;
}

telemetry_outage_result persist(telemetry_outage_journal &j)
{
	struct stat directory = {};
	struct stat owner = {};
	struct stat linked_owner = {};
	struct stat ledger = {};
	if (fstat(j.directory_fd, &directory) != 0 || directory.st_uid != geteuid() ||
	    (directory.st_mode & 07777U) != 0700U || !safe_file(j.ownership_fd, owner) ||
	    owner.st_size != 0 ||
	    fstatat(j.directory_fd, LOCK, &linked_owner, AT_SYMLINK_NOFOLLOW) != 0 ||
	    linked_owner.st_dev != owner.st_dev || linked_owner.st_ino != owner.st_ino)
		return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
	if (fstatat(j.directory_fd, LEDGER, &ledger, AT_SYMLINK_NOFOLLOW) == 0)
	{
		if (!S_ISREG(ledger.st_mode) || ledger.st_uid != geteuid() ||
		    (ledger.st_mode & 07777U) != 0600U || ledger.st_nlink != 1U)
			return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
	}
	else if (errno != ENOENT || j.generation != 0U)
		return failure(j, telemetry_outage_result::unsafe_storage, errno);
	if (j.generation == UINT64_MAX)
		return failure(j, telemetry_outage_result::quota, EOVERFLOW);
	std::vector<unsigned char> data(std::begin(MAGIC), std::end(MAGIC));
	data.reserve(64U + j.count * TELEMETRY_OUTAGE_DISK_WORDS * 8U);
	append_word(data, j.generation + 1U);
	append_word(data, j.count);
	append_word(data, 0U);
	for (std::size_t index = 0U; index < j.count; ++index)
		for (const auto value : encode(j.observations[index]))
			append_word(data, value);
	unsigned char digest[SHA256_DIGEST_LENGTH]{};
	if (SHA256(data.data(), data.size(), digest) == nullptr)
		return failure(j, telemetry_outage_result::io_failure, EIO);
	data.insert(data.end(), std::begin(digest), std::end(digest));
	const int fd = openat(j.directory_fd, PENDING,
			      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return failure(j, telemetry_outage_result::io_failure, errno);
	std::size_t offset = 0U;
	while (offset < data.size())
	{
		const auto written = write(fd, data.data() + offset, data.size() - offset);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
		{
			const int error = written == 0 ? EIO : errno;
			close(fd);
			return failure(j, telemetry_outage_result::io_failure, error);
		}
		offset += static_cast<std::size_t>(written);
	}
	if (fsync(fd) != 0)
	{
		const int error = errno;
		close(fd);
		return failure(j, telemetry_outage_result::io_failure, error);
	}
	if (close(fd) != 0 || renameat(j.directory_fd, PENDING, j.directory_fd, LEDGER) != 0 ||
	    fsync(j.directory_fd) != 0)
		return failure(j, telemetry_outage_result::io_failure, errno);
	++j.generation;
	j.error_code = 0U;
	return telemetry_outage_result::ready;
}

telemetry_outage_result acquire(telemetry_outage_journal &j, const char *directory)
{
	if (!directory || directory[0] != '/' || j.directory_fd >= 0 || j.ownership_fd >= 0)
		return failure(j, telemetry_outage_result::invalid, EINVAL);
	std::array<char, 4096U> path{};
	auto length = strnlen(directory, path.size());
	if (length == path.size())
		return failure(j, telemetry_outage_result::invalid, ENAMETOOLONG);
	std::memcpy(path.data(), directory, length);
	// A trailing slash otherwise makes O_NOFOLLOW follow a directory symlink.
	while (length > 1U && path[length - 1U] == '/')
		path[--length] = '\0';
	j.directory_fd = open(path.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (j.directory_fd < 0)
		return failure(j, telemetry_outage_result::io_failure, errno);
	struct stat status
	{
	};
	if (fstat(j.directory_fd, &status) != 0 || !S_ISDIR(status.st_mode) ||
	    status.st_uid != geteuid() || (status.st_mode & 07777U) != 0700U)
		return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
	j.ownership_fd =
		openat(j.directory_fd, LOCK, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (j.ownership_fd < 0)
		return failure(j, telemetry_outage_result::io_failure, errno);
	if (!safe_file(j.ownership_fd, status) || status.st_size != 0)
		return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
	if (flock(j.ownership_fd, LOCK_EX | LOCK_NB) != 0)
		return failure(j, telemetry_outage_result::owned_elsewhere, errno);
	j.count = 0U;
	j.generation = 0U;
	j.current = TELEMETRY_OUTAGE_MAX_PRODUCERS;
	bool present = false;
	auto result = load(j, LEDGER, present);
	if (result != telemetry_outage_result::ready)
		return result;
	telemetry_outage_journal staged{};
	staged.directory_fd = j.directory_fd;
	bool pending = false;
	result = load(staged, PENDING, pending);
	if (result != telemetry_outage_result::ready)
		return failure(j, result, static_cast<int>(staged.error_code));
	if (pending)
	{
		// A complete interrupted publication may only extend this exact chain.
		// Corrupt/truncated pending evidence is retained and refuses admission.
		if (j.generation == UINT64_MAX || staged.generation != j.generation + 1U ||
		    staged.count < j.count || staged.count > j.count + 1U)
			return failure(j, telemetry_outage_result::corrupt, EBADMSG);
		for (std::size_t index = 0U; index < j.count; ++index)
		{
			if (!extends(j.observations[index], staged.observations[index]))
				return failure(j, telemetry_outage_result::corrupt, EBADMSG);
			if (staged.count > j.count &&
			    j.observations[index].phase == telemetry_outage_phase::running)
			{
				auto expected = j.observations[index];
				expected.phase = telemetry_outage_phase::unknown_tail;
				if (encode(expected) != encode(staged.observations[index]))
					return failure(j, telemetry_outage_result::corrupt,
						       EBADMSG);
			}
		}
		if (staged.count > j.count && !registration_is_empty(staged.observations[j.count]))
			return failure(j, telemetry_outage_result::corrupt, EBADMSG);
		const int pending_fd =
			openat(j.directory_fd, PENDING, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
		if (pending_fd < 0)
			return failure(j, telemetry_outage_result::io_failure, errno);
		file_guard pending_guard{ pending_fd };
		if (!safe_file(pending_fd, status))
			return failure(j, telemetry_outage_result::unsafe_storage, EPERM);
		if (fsync(pending_fd) != 0 ||
		    renameat(j.directory_fd, PENDING, j.directory_fd, LEDGER) != 0 ||
		    fsync(j.directory_fd) != 0)
			return failure(j, telemetry_outage_result::io_failure, errno);
		j.generation = staged.generation;
		j.count = staged.count;
		j.observations = staged.observations;
	}
	j.error_code = 0U;
	return telemetry_outage_result::ready;
}
} // namespace

telemetry_outage_phase
telemetry_outage_terminal_phase(const telemetry_outage_observation &observation) noexcept
{
	if (clean(observation))
		return telemetry_outage_phase::clean_drained;
	if (observation.inflight_records == 0U && observation.unattempted_records != 0U)
		return telemetry_outage_phase::abandoned;
	return telemetry_outage_phase::unknown_tail;
}

void telemetry_outage_close(telemetry_outage_journal *journal) noexcept
{
	if (!journal)
		return;
	if (journal->ownership_fd >= 0)
		close(journal->ownership_fd);
	if (journal->directory_fd >= 0)
		close(journal->directory_fd);
	journal->ownership_fd = -1;
	journal->directory_fd = -1;
	journal->current = TELEMETRY_OUTAGE_MAX_PRODUCERS;
}

telemetry_outage_result telemetry_outage_read(telemetry_outage_journal *journal,
					      const char *directory) noexcept
{
	if (!journal || journal->directory_fd >= 0 || journal->ownership_fd >= 0)
		return telemetry_outage_result::invalid;
	telemetry_outage_result result;
	try
	{
		result = acquire(*journal, directory);
	}
	catch (...)
	{
		result = failure(*journal, telemetry_outage_result::io_failure, ENOMEM);
	}
	if (result != telemetry_outage_result::ready)
		telemetry_outage_close(journal);
	return result;
}

telemetry_outage_result
telemetry_outage_open(telemetry_outage_journal *journal, const char *directory,
		      const telemetry_outage_observation &registration) noexcept
{
	if (!journal || !registration_is_empty(registration))
		return telemetry_outage_result::invalid;
	auto result = telemetry_outage_read(journal, directory);
	if (result != telemetry_outage_result::ready)
		return result;
	try
	{
		for (std::size_t index = 0U; index < journal->count; ++index)
		{
			if (same_producer(registration.producer,
					  journal->observations[index].producer))
			{
				result =
					failure(*journal, telemetry_outage_result::invalid, EEXIST);
				telemetry_outage_close(journal);
				return result;
			}
			if (journal->observations[index].phase == telemetry_outage_phase::running)
				journal->observations[index].phase =
					telemetry_outage_phase::unknown_tail;
		}
		if (journal->count == TELEMETRY_OUTAGE_MAX_PRODUCERS)
			result = failure(*journal, telemetry_outage_result::quota, ENOSPC);
		else
		{
			journal->current = journal->count++;
			journal->observations[journal->current] = registration;
			result = persist(*journal);
		}
	}
	catch (...)
	{
		result = failure(*journal, telemetry_outage_result::io_failure, ENOMEM);
	}
	if (result != telemetry_outage_result::ready)
		telemetry_outage_close(journal);
	return result;
}

telemetry_outage_result
telemetry_outage_checkpoint(telemetry_outage_journal *journal,
			    const telemetry_outage_observation &observation) noexcept
{
	if (!journal || journal->directory_fd < 0 || journal->ownership_fd < 0 ||
	    journal->current >= journal->count || !valid(observation))
		return telemetry_outage_result::invalid;
	const auto previous = journal->observations[journal->current];
	if (!extends(previous, observation) || previous.phase != telemetry_outage_phase::running)
		return failure(*journal, telemetry_outage_result::invalid, EINVAL);
	telemetry_outage_result result;
	try
	{
		journal->observations[journal->current] = observation;
		result = persist(*journal);
	}
	catch (...)
	{
		result = failure(*journal, telemetry_outage_result::io_failure, ENOMEM);
	}
	if (result != telemetry_outage_result::ready)
	{
		journal->observations[journal->current] = previous;
		// Reopen and validate an ambiguously acknowledged publication before
		// any further mutation. A retry from this generation can overwrite it.
		journal->current = TELEMETRY_OUTAGE_MAX_PRODUCERS;
	}
	return result;
}
