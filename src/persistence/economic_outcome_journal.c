#include "persistence/economic_outcome.h"
#include "persistence/critical_command_journal.h"

#include <array>
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <openssl/evp.h>
#include <set>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

namespace
{
constexpr size_t base_header_bytes = 60;
constexpr size_t frame_header_bytes = 16;
std::mutex outcome_mutex;
int outcome_fd = -1;
std::string outcome_path;
economic_outcome_journal_health outcome_health;
bool replay_in_progress = false;
critical_operation_id outcome_lineage{}, outcome_epoch{};
struct append_attempt
{
	off_t offset = 0;
	std::array<uint8_t, 32> prefix{};
	std::vector<uint8_t> frame;
	uint64_t sequence = 0;
};
append_attempt unresolved;

template <typename T> void put(std::vector<uint8_t> &out, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		out.push_back(static_cast<uint8_t>(value >> (i * 8)));
}
template <typename T> T get(const uint8_t *p)
{
	T value = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		value |= static_cast<T>(p[i]) << (i * 8);
	return value;
}
bool read_exact(void *data, size_t size, off_t at)
{
	auto *bytes = static_cast<uint8_t *>(data);
	while (size)
	{
		const ssize_t n = pread(outcome_fd, bytes, size, at);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return false;
		bytes += n;
		size -= static_cast<size_t>(n);
		at += n;
	}
	return true;
}
bool write_exact(std::span<const uint8_t> bytes, off_t at, size_t *written)
{
	*written = 0;
	while (*written < bytes.size())
	{
		const ssize_t n = pwrite(outcome_fd, bytes.data() + *written,
					 bytes.size() - *written, at + *written);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return false;
		*written += static_cast<size_t>(n);
	}
	return true;
}
bool named_file(off_t *size)
{
	struct stat held
	{
	}, named{};
	if (outcome_fd < 0 || fstat(outcome_fd, &held) || lstat(outcome_path.c_str(), &named) ||
	    !S_ISREG(held.st_mode) || !S_ISREG(named.st_mode) || held.st_dev != named.st_dev ||
	    held.st_ino != named.st_ino || held.st_uid != geteuid() || (held.st_mode & 0077) ||
	    held.st_size < 0)
		return false;
	*size = held.st_size;
	return true;
}
bool prefix_digest(off_t length, std::array<uint8_t, 32> *output)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	if (!ctx)
		return false;
	bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1;
	std::array<uint8_t, 8192> buffer{};
	for (off_t at = 0; ok && at < length;)
	{
		const size_t amount =
			static_cast<size_t>(std::min<off_t>(buffer.size(), length - at));
		ok = read_exact(buffer.data(), amount, at) &&
		     EVP_DigestUpdate(ctx, buffer.data(), amount) == 1;
		at += static_cast<off_t>(amount);
	}
	std::array<uint8_t, 32> result{};
	unsigned int size = 0;
	ok = ok && EVP_DigestFinal_ex(ctx, result.data(), &size) == 1 && size == result.size();
	EVP_MD_CTX_free(ctx);
	if (ok)
		*output = result;
	return ok;
}
bool exact_attempt(size_t length)
{
	off_t size = 0;
	std::array<uint8_t, 32> prefix{};
	if (length > unresolved.frame.size() || !named_file(&size) ||
	    size != unresolved.offset + static_cast<off_t>(length) ||
	    !prefix_digest(unresolved.offset, &prefix) || prefix != unresolved.prefix)
		return false;
	std::array<uint8_t, 8192> buffer{};
	for (size_t at = 0; at < length;)
	{
		const size_t amount = std::min(buffer.size(), length - at);
		if (!read_exact(buffer.data(), amount,
				unresolved.offset + static_cast<off_t>(at)) ||
		    !std::equal(buffer.begin(), buffer.begin() + amount,
				unresolved.frame.begin() + at))
			return false;
		at += amount;
	}
	return true;
}
using scan_fn = bool (*)(const economic_outcome_record &, std::span<const uint8_t>, void *);
bool scan(scan_fn visitor, void *context, economic_outcome_journal_health *health,
	  bool own_append = false)
{
	off_t size = 0;
	if (!named_file(&size) || static_cast<uint64_t>(size) > health->byte_limit ||
	    size < static_cast<off_t>(base_header_bytes))
		return false;
	std::array<uint8_t, base_header_bytes> base{};
	if (!read_exact(base.data(), base.size(), 0) ||
	    !std::equal(base.begin(), base.begin() + 4, "EOB1") ||
	    get<uint32_t>(base.data() + 4) != 1 || get<uint64_t>(base.data() + 48) != 0 ||
	    get<uint32_t>(base.data() + 56) != crc32(0, base.data(), 56))
		return false;
	critical_operation_id lineage{}, epoch{};
	std::copy_n(base.begin() + 8, 16, lineage.bytes.begin());
	std::copy_n(base.begin() + 24, 16, epoch.bytes.begin());
	if (critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch) ||
	    (!critical_operation_id_is_zero(outcome_lineage) &&
	     (!critical_operation_id_equal(lineage, outcome_lineage) ||
	      !critical_operation_id_equal(epoch, outcome_epoch))))
		return false;
	if (outcome_health.initialized &&
	    (get<uint64_t>(base.data() + 40) != outcome_health.compacted_through ||
	     size != static_cast<off_t>(own_append ? unresolved.offset + unresolved.frame.size() :
						     outcome_health.retained_bytes)))
		return false;
	health->compacted_through = get<uint64_t>(base.data() + 40);
	health->durable_sequence = health->compacted_through;
	health->retained_records = 0;
	std::set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>> ids;
	off_t at = base_header_bytes;
	while (at < size)
	{
		std::array<uint8_t, frame_header_bytes> header{};
		if (size - at < static_cast<off_t>(header.size()) ||
		    !read_exact(header.data(), header.size(), at) ||
		    !std::equal(header.begin(), header.begin() + 4, "EOJ1"))
			return false;
		const uint64_t length = get<uint64_t>(header.data() + 4);
		if (!length || length > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    length > static_cast<uint64_t>(size - at - static_cast<off_t>(header.size())) ||
		    health->retained_records >= health->record_limit ||
		    health->durable_sequence == UINT64_MAX)
			return false;
		std::vector<uint8_t> bytes(static_cast<size_t>(length));
		economic_outcome_record record;
		if (!read_exact(bytes.data(), bytes.size(), at + header.size()) ||
		    get<uint32_t>(header.data() + 12) != crc32(0, bytes.data(), bytes.size()) ||
		    !economic_outcome_decode(bytes, &record) ||
		    record.sequence != health->durable_sequence + 1 ||
		    !critical_operation_id_equal(record.accounting.metadata.lineage, lineage) ||
		    !critical_operation_id_equal(record.accounting.metadata.epoch, epoch) ||
		    !ids.insert(record.command.operation_id.bytes).second ||
		    (visitor && !visitor(record, bytes, context)))
			return false;
		health->durable_sequence = record.sequence;
		++health->retained_records;
		at += static_cast<off_t>(header.size() + length);
	}
	health->retained_bytes = static_cast<size_t>(size);
	if (at != size)
		return false;
	// Only initial successful commissioning may establish the namespace.
	// Runtime scans validate the immutable binding rather than replacing it.
	if (critical_operation_id_is_zero(outcome_lineage))
	{
		outcome_lineage = lineage;
		outcome_epoch = epoch;
	}
	return true;
}
} // namespace

bool economic_outcome_journal_init_namespace(const char *directory, size_t bytes, size_t records,
					     const critical_operation_id &lineage,
					     const critical_operation_id &epoch,
					     uint64_t base) noexcept
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	try
	{
		if (outcome_fd >= 0 || !directory || !*directory || !records ||
		    critical_operation_id_is_zero(lineage) !=
			    critical_operation_id_is_zero(epoch) ||
		    records > CRITICAL_COMMAND_JOURNAL_MAX_RECORDS || bytes < base_header_bytes ||
		    bytes > CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA)
			return false;
		struct stat parent
		{
		};
		if (lstat(directory, &parent) || !S_ISDIR(parent.st_mode) ||
		    parent.st_uid != geteuid() || (parent.st_mode & 0022))
			return false;
		outcome_path = std::string(directory) + "/economic-outcomes.journal";
		const int create = critical_operation_id_is_zero(lineage) ? 0 : O_CREAT;
		outcome_fd =
			open(outcome_path.c_str(), O_RDWR | create | O_CLOEXEC | O_NOFOLLOW, 0600);
		if (outcome_fd < 0 || flock(outcome_fd, LOCK_EX | LOCK_NB))
		{
			if (outcome_fd >= 0)
				close(outcome_fd);
			outcome_fd = -1;
			return false;
		}
		outcome_health = {};
		outcome_lineage = lineage;
		outcome_epoch = epoch;
		outcome_health.byte_limit = bytes;
		outcome_health.record_limit = records;
		off_t size = 0;
		bool ok = named_file(&size);
		if (ok && !size)
		{
			ok = !critical_operation_id_is_zero(lineage) &&
			     !critical_operation_id_is_zero(epoch);
			if (ok)
			{
				std::vector<uint8_t> header{ 'E', 'O', 'B', '1' };
				put<uint32_t>(header, 1);
				header.insert(header.end(), lineage.bytes.begin(),
					      lineage.bytes.end());
				header.insert(header.end(), epoch.bytes.begin(), epoch.bytes.end());
				put<uint64_t>(header, base);
				put<uint64_t>(header, 0);
				put<uint32_t>(header, crc32(0, header.data(), header.size()));
				size_t written = 0;
				ok = write_exact(header, 0, &written) && fsync(outcome_fd) == 0;
				const int dir_fd = open(directory, O_RDONLY | O_DIRECTORY |
									   O_CLOEXEC | O_NOFOLLOW);
				ok = ok && dir_fd >= 0 && fsync(dir_fd) == 0;
				if (dir_fd >= 0)
					ok = close(dir_fd) == 0 && ok;
			}
		}
		std::array<uint8_t, 32> before_digest{}, after_digest{};
		off_t verified_size = 0;
		ok = ok && named_file(&verified_size) &&
		     prefix_digest(verified_size, &before_digest) &&
		     scan(nullptr, nullptr, &outcome_health);
		if (ok && !critical_operation_id_is_zero(lineage))
			ok = critical_operation_id_equal(lineage, outcome_lineage) &&
			     critical_operation_id_equal(epoch, outcome_epoch);
		// A readable complete tail may follow an unknown prior fsync. Reconfirm
		// durability and the exact accepted file before any recovery/ACK exposure.
		ok = ok && fsync(outcome_fd) == 0;
		const int directory_fd =
			open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
		ok = ok && directory_fd >= 0 && fsync(directory_fd) == 0;
		if (directory_fd >= 0)
			ok = close(directory_fd) == 0 && ok;
		off_t after_size = 0;
		ok = ok && named_file(&after_size) && after_size == verified_size &&
		     prefix_digest(after_size, &after_digest) && before_digest == after_digest;
		if (!ok)
		{
			close(outcome_fd);
			outcome_fd = -1;
			outcome_health = {};
			return false;
		}
		outcome_health.initialized = true;
		return true;
	}
	catch (...)
	{
		if (outcome_fd >= 0)
			close(outcome_fd);
		outcome_fd = -1;
		outcome_health = {};
		outcome_lineage = {};
		outcome_epoch = {};
		return false;
	}
}

bool economic_outcome_journal_init(const char *directory, size_t bytes, size_t records) noexcept
{
	const critical_operation_id absent{};
	return economic_outcome_journal_init_namespace(directory, bytes, records, absent, absent,
						       0);
}

void economic_outcome_journal_shutdown() noexcept
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	if (replay_in_progress)
		return;
	if (outcome_fd >= 0)
		close(outcome_fd);
	outcome_fd = -1;
	outcome_health.initialized = false;
	outcome_lineage = {};
	outcome_epoch = {};
	// Never repair/truncate an unresolved append at shutdown.
	unresolved = {};
}

economic_outcome_journal_health economic_outcome_journal_health_copy() noexcept
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	return outcome_health;
}

uint64_t economic_outcome_journal_next_sequence() noexcept
{
	const auto health = economic_outcome_journal_health_copy();
	return health.initialized && !health.uncertain && health.durable_sequence != UINT64_MAX ?
		       health.durable_sequence + 1 :
		       0;
}

bool economic_outcome_journal_namespace(critical_operation_id *lineage,
					critical_operation_id *epoch) noexcept
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	if (!lineage || !epoch || !outcome_health.initialized || outcome_health.uncertain)
		return false;
	*lineage = outcome_lineage;
	*epoch = outcome_epoch;
	return true;
}

bool economic_outcome_journal_lookup(const critical_operation_id &id,
				     economic_outcome_record *output) noexcept
try
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	if (!output || !outcome_health.initialized || outcome_health.uncertain)
		return false;
	struct lookup
	{
		const critical_operation_id &id;
		economic_outcome_record result;
		bool found = false;
	} context{ id, {}, false };
	auto health = outcome_health;
	const auto visit =
		+[](const economic_outcome_record &record, std::span<const uint8_t>, void *opaque)
	{
		auto &value = *static_cast<lookup *>(opaque);
		if (critical_operation_id_equal(value.id, record.command.operation_id))
		{
			value.result = record;
			value.found = true;
		}
		return true;
	};
	if (!scan(visit, &context, &health) || !context.found)
		return false;
	*output = std::move(context.result);
	return true;
}
catch (...)
{
	return false;
}

economic_outcome_result
economic_outcome_journal_append(const economic_outcome_record &record) noexcept
try
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	if (!outcome_health.initialized)
		return { economic_outcome_status::unavailable, 0, ENODEV };
	if (replay_in_progress)
		return { economic_outcome_status::unavailable, 0, EBUSY };
	if (outcome_health.uncertain)
		return { economic_outcome_status::uncertain, unresolved.sequence, EBUSY };
	std::vector<uint8_t> bytes;
	if (!economic_outcome_encode(record, &bytes) ||
	    !critical_operation_id_equal(record.accounting.metadata.lineage, outcome_lineage) ||
	    !critical_operation_id_equal(record.accounting.metadata.epoch, outcome_epoch))
		return { economic_outcome_status::rejected, 0, EINVAL };
	struct match
	{
		const economic_outcome_record &record;
		std::span<const uint8_t> bytes;
		uint64_t sequence = 0;
		bool conflict = false;
	} context{ record, bytes, 0, false };
	auto refreshed = outcome_health;
	const auto visit = +[](const economic_outcome_record &stored, std::span<const uint8_t> body,
			       void *opaque)
	{
		auto &value = *static_cast<match *>(opaque);
		if (critical_operation_id_equal(value.record.command.operation_id,
						stored.command.operation_id))
		{
			value.sequence = stored.sequence;
			value.conflict = body.size() != value.bytes.size() ||
					 !std::equal(body.begin(), body.end(), value.bytes.begin());
		}
		return true;
	};
	if (!scan(visit, &context, &refreshed))
	{
		outcome_health.uncertain = true;
		return { economic_outcome_status::uncertain, record.sequence, EILSEQ };
	}
	outcome_health = refreshed;
	if (context.sequence)
		return { context.conflict ? economic_outcome_status::conflict :
					    economic_outcome_status::durable,
			 context.sequence, context.conflict ? EEXIST : 0U };
	if (outcome_health.durable_sequence == UINT64_MAX ||
	    record.sequence != outcome_health.durable_sequence + 1)
		return { economic_outcome_status::conflict, record.sequence, ESTALE };
	if (outcome_health.retained_records >= outcome_health.record_limit ||
	    bytes.size() + frame_header_bytes >
		    outcome_health.byte_limit - outcome_health.retained_bytes)
		return { economic_outcome_status::rejected, 0, ENOSPC };
	append_attempt attempt;
	attempt.offset = static_cast<off_t>(outcome_health.retained_bytes);
	attempt.sequence = record.sequence;
	if (!prefix_digest(attempt.offset, &attempt.prefix))
		return { economic_outcome_status::rejected, 0, EIO };
	attempt.frame = { 'E', 'O', 'J', '1' };
	put<uint64_t>(attempt.frame, bytes.size());
	put<uint32_t>(attempt.frame, crc32(0, bytes.data(), bytes.size()));
	attempt.frame.insert(attempt.frame.end(), bytes.begin(), bytes.end());
	unresolved = std::move(attempt); // Complete retained attempt before I/O.
	size_t written = 0;
	const bool complete = write_exact(unresolved.frame, unresolved.offset, &written);
	if (complete && fsync(outcome_fd) == 0 && exact_attempt(unresolved.frame.size()))
	{
		outcome_health.durable_sequence = record.sequence;
		++outcome_health.retained_records;
		outcome_health.retained_bytes += unresolved.frame.size();
		unresolved = {};
		return { economic_outcome_status::durable, record.sequence, 0 };
	}
	// Clean rejection is allowed only for the exact own partial tail, with
	// unchanged full prefix/name and confirmed truncate+fsync. Full writes
	// with failed durability remain uncertain, never silently rolled back.
	if (!complete && exact_attempt(written) && ftruncate(outcome_fd, unresolved.offset) == 0 &&
	    fsync(outcome_fd) == 0 && exact_attempt(0))
	{
		unresolved = {};
		return { economic_outcome_status::rejected, 0, EIO };
	}
	outcome_health.uncertain = true;
	return { economic_outcome_status::uncertain, record.sequence, EIO };
}
catch (...)
{
	return { economic_outcome_status::rejected, 0, ENOMEM };
}

bool economic_outcome_journal_reconcile() noexcept
try
{
	std::lock_guard<std::mutex> lock(outcome_mutex);
	if (!outcome_health.initialized)
		return false;
	if (!outcome_health.uncertain)
		return true;
	if (unresolved.frame.empty() || !exact_attempt(unresolved.frame.size()) ||
	    fsync(outcome_fd) || !exact_attempt(unresolved.frame.size()))
		return false;
	auto checked = outcome_health;
	if (!scan(nullptr, nullptr, &checked, true) ||
	    checked.durable_sequence != unresolved.sequence)
		return false;
	checked.uncertain = false;
	outcome_health = checked;
	unresolved = {};
	return true;
}
catch (...)
{
	return false;
}

bool economic_outcome_journal_replay(uint64_t base, economic_outcome_replay_fn apply,
				     void *opaque) noexcept
try
{
	std::unique_lock<std::mutex> lock(outcome_mutex);
	if (!apply || !outcome_health.initialized || outcome_health.uncertain ||
	    replay_in_progress || base < outcome_health.compacted_through ||
	    base > outcome_health.durable_sequence)
		return false;
	auto verified = outcome_health;
	std::array<uint8_t, 32> prefix{}, after_prefix{};
	if (!prefix_digest(static_cast<off_t>(verified.retained_bytes), &prefix) ||
	    !scan(nullptr, nullptr, &verified))
		return false;
	replay_in_progress = true;
	struct replay_guard
	{
		~replay_guard() { replay_in_progress = false; }
	} guard;
	// Hold the ordered stream against append/shutdown, but do not hold the
	// mutex while a real materializer reads namespace/health. Only one record
	// is owned at a time; replay never copies the entire retained backlog.
	off_t at = base_header_bytes;
	while (at < static_cast<off_t>(verified.retained_bytes))
	{
		std::array<uint8_t, frame_header_bytes> header{};
		if (verified.retained_bytes - static_cast<size_t>(at) < header.size() ||
		    !read_exact(header.data(), header.size(), at) ||
		    !std::equal(header.begin(), header.begin() + 4, "EOJ1"))
			return false;
		const uint64_t length = get<uint64_t>(header.data() + 4);
		if (!length || length > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    length > verified.retained_bytes - static_cast<size_t>(at) - header.size())
			return false;
		std::vector<uint8_t> bytes(static_cast<size_t>(length));
		economic_outcome_record record;
		if (!read_exact(bytes.data(), bytes.size(), at + header.size()) ||
		    get<uint32_t>(header.data() + 12) != crc32(0, bytes.data(), bytes.size()) ||
		    !economic_outcome_decode(bytes, &record))
			return false;
		if (record.sequence > base)
		{
			lock.unlock();
			const bool applied = apply(record, opaque);
			lock.lock();
			if (!applied)
				return false;
		}
		at += static_cast<off_t>(header.size() + length);
	}
	off_t final_size = 0;
	return named_file(&final_size) &&
	       final_size == static_cast<off_t>(verified.retained_bytes) &&
	       prefix_digest(final_size, &after_prefix) && prefix == after_prefix;
}
catch (...)
{
	return false;
}

bool economic_outcome_journal_retire_prefix(uint64_t, economic_outcome_base_verify_fn,
					    void *) noexcept
{
	// Retaining the complete bounded tail is safe. Prefix rewrite/cursor
	// retirement stays closed until the genuine coupled-base owner is joined.
	return false;
}
