#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <new>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
using status = flatfile_accounting_status;
constexpr size_t header_bytes = 48;
constexpr size_t index_max_bytes = header_bytes + 32 + FLATFILE_ACCOUNTING_BUCKET_RECORDS * 64;
constexpr std::array<uint8_t, 8> record_magic = { 'D', 'U', 'R', 'E', 'C', 'R', '2', 0 };
constexpr std::array<uint8_t, 8> index_magic = { 'D', 'U', 'R', 'E', 'C', 'I', '1', 0 };
constexpr std::array<uint8_t, 8> segment_magic = { 'D', 'U', 'R', 'E', 'C', 'S', '1', 0 };
struct failure
{
	status code;
};
void require(bool condition, status code = status::invalid)
{
	if (!condition)
		throw failure{ code };
}
void checked(economic_accounting_error value)
{
	require(value == economic_accounting_error::ok,
		value == economic_accounting_error::capacity ? status::capacity : status::invalid);
}
void command_checked(critical_command_codec_result value)
{
	require(value == critical_command_codec_result::ok,
		value == critical_command_codec_result::overflow ? status::capacity :
								   status::invalid);
}
economic_digest digest(std::span<const uint8_t> value)
{
	economic_digest result = {};
	SHA256(value.data(), value.size(), result.data());
	return result;
}
void number(std::vector<uint8_t> &bytes, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
void raw(std::vector<uint8_t> &bytes, std::span<const uint8_t> value)
{
	bytes.insert(bytes.end(), value.begin(), value.end());
}
struct reader
{
	std::span<const uint8_t> bytes;
	size_t offset = 0;
	std::span<const uint8_t> take(size_t size)
	{
		require(offset <= bytes.size() && size <= bytes.size() - offset);
		auto result = bytes.subspan(offset, size);
		offset += size;
		return result;
	}
	uint64_t number(size_t width)
	{
		auto value = take(width);
		uint64_t result = 0;
		for (size_t i = 0; i < width; ++i)
			result |= uint64_t(value[i]) << (8 * i);
		return result;
	}
	void done() { require(offset == bytes.size()); }
};
std::vector<uint8_t> envelope(const std::array<uint8_t, 8> &magic, std::span<const uint8_t> payload)
{
	require(payload.size() <= UINT32_MAX);
	std::vector<uint8_t> result;
	result.reserve(header_bytes + payload.size());
	raw(result, magic);
	number(result, 1, 4);
	number(result, payload.size(), 4);
	raw(result, digest(payload));
	raw(result, payload);
	return result;
}
std::span<const uint8_t> unwrap(std::span<const uint8_t> bytes, const std::array<uint8_t, 8> &magic,
				size_t maximum)
{
	require(bytes.size() >= header_bytes && bytes.size() <= maximum);
	reader input{ bytes };
	auto encoded_magic = input.take(8);
	require(std::equal(magic.begin(), magic.end(), encoded_magic.begin()));
	require(input.number(4) == 1 && input.number(4) == bytes.size() - header_bytes);
	auto checksum = input.take(32);
	auto payload = bytes.subspan(header_bytes);
	auto actual = digest(payload);
	require(std::equal(actual.begin(), actual.end(), checksum.begin()));
	return payload;
}
std::vector<uint8_t> validate_record(const flatfile_accounting_record &record)
{
	require(record.command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		critical_command_envelope_valid(record.command));
	require(critical_failure_stage_valid(record.failure_stage) &&
		(record.result_code || record.failure_stage == critical_failure_stage::none));
	require(record.result.size() <= CRITICAL_COMPLETION_RESULT_MAX_BYTES &&
		record.plan.size() <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES);
	economic_frozen_intent intent;
	checked(economic_intent_decode(record.command.accounting_intent, &intent));
	economic_plan_metadata metadata;
	checked(economic_intent_plan_metadata(record.command, intent, &metadata));
	if (record.result_code)
		require(record.plan.empty());
	else
	{
		economic_accounting_plan plan;
		checked(economic_plan_decode(record.plan, &plan));
		// Child IDs need reservations that this bounded storage phase does not own.
		require(plan.children.empty());
		plan.metadata = metadata;
		std::vector<uint8_t> canonical;
		checked(economic_plan_encode(plan, &canonical));
		require(canonical == record.plan);
	}
	std::vector<uint8_t> command;
	command_checked(critical_command_encode(record.command, &command));
	return command;
}
std::vector<uint8_t> encode_record(const flatfile_accounting_record &record)
{
	auto command = validate_record(record);
	std::vector<uint8_t> payload;
	payload.reserve(26 + command.size() + record.plan.size() + record.result.size());
	number(payload, command.size(), 4);
	number(payload, record.plan.size(), 4);
	number(payload, record.result.size(), 4);
	number(payload, record.result_code, 4);
	number(payload, record.durable_revision, 8);
	number(payload, static_cast<uint16_t>(record.failure_stage), 2);
	raw(payload, command);
	raw(payload, record.plan);
	raw(payload, record.result);
	return envelope(record_magic, payload);
}
flatfile_accounting_record decode_record(std::span<const uint8_t> bytes)
{
	reader input{ unwrap(bytes, record_magic, FLATFILE_ACCOUNTING_RECORD_MAX_BYTES) };
	const auto command_size = input.number(4), plan_size = input.number(4),
		   result_size = input.number(4);
	require(command_size <= CRITICAL_COMMAND_MAX_ENCODED_BYTES &&
		plan_size <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES &&
		result_size <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
	flatfile_accounting_record result;
	result.result_code = static_cast<uint32_t>(input.number(4));
	result.durable_revision = input.number(8);
	result.failure_stage = static_cast<critical_failure_stage>(input.number(2));
	auto command = input.take(command_size);
	command_checked(critical_command_decode(command.data(), command.size(), &result.command));
	auto plan = input.take(plan_size), payload = input.take(result_size);
	input.done();
	result.plan.assign(plan.begin(), plan.end());
	result.result.assign(payload.begin(), payload.end());
	auto canonical = validate_record(result);
	require(canonical.size() == command.size() &&
		std::equal(canonical.begin(), canonical.end(), command.begin()));
	return result;
}
size_t bucket_for(const critical_operation_id &id)
{
	return id.bytes[0];
}
std::string bucket_prefix(size_t bucket)
{
	require(bucket < FLATFILE_ACCOUNTING_BUCKETS);
	constexpr char hex[] = "0123456789abcdef";
	std::string name = "bucket-";
	name += hex[bucket >> 4];
	name += hex[bucket & 15];
	return name;
}
std::string index_name(size_t bucket)
{
	return bucket_prefix(bucket) + ".eai";
}
std::string segment_name(size_t bucket, size_t segment)
{
	require(segment <= FLATFILE_ACCOUNTING_BUCKET_SEGMENTS);
	return bucket_prefix(bucket) + "-" + std::to_string(segment) + ".eas";
}
struct entry
{
	critical_operation_id id = {};
	economic_digest digest = {};
	uint32_t segment = 0, offset = 0, bytes = 0;
};
struct bucket_index
{
	critical_operation_id lineage = {};
	uint32_t bucket = 0;
	uint64_t bytes = 0;
	std::vector<entry> entries;
};
uint32_t last_segment(const bucket_index &index)
{
	uint32_t result = 0;
	for (const auto &item : index.entries)
		result = std::max(result, item.segment);
	return result;
}
std::vector<const entry *> segment_entries(const bucket_index &index, uint32_t segment)
{
	std::vector<const entry *> entries;
	for (const auto &item : index.entries)
		if (item.segment == segment)
			entries.push_back(&item);
	std::sort(entries.begin(), entries.end(),
		  [](const auto *a, const auto *b) { return a->offset < b->offset; });
	return entries;
}
void validate_index(const bucket_index &index)
{
	require(!critical_operation_id_is_zero(index.lineage) &&
		index.bucket < FLATFILE_ACCOUNTING_BUCKETS &&
		index.entries.size() <= FLATFILE_ACCOUNTING_BUCKET_RECORDS);
	uint64_t total = 0;
	for (size_t i = 0; i < index.entries.size(); ++i)
	{
		const auto &item = index.entries[i];
		require(!critical_operation_id_is_zero(item.id) &&
			bucket_for(item.id) == index.bucket && item.digest != economic_digest{} &&
			item.bytes >= header_bytes &&
			item.bytes <= FLATFILE_ACCOUNTING_RECORD_MAX_BYTES &&
			item.segment < index.entries.size() &&
			item.segment < FLATFILE_ACCOUNTING_BUCKET_SEGMENTS);
		require(!i || index.entries[i - 1].id.bytes < item.id.bytes);
		total += item.bytes;
	}
	require(total == index.bytes && total <= FLATFILE_ACCOUNTING_BUCKET_MAX_BYTES);
	if (index.entries.empty())
		return;
	for (uint32_t segment = 0; segment <= last_segment(index); ++segment)
	{
		auto entries = segment_entries(index, segment);
		require(!entries.empty());
		uint64_t offset = 0;
		for (const auto *item : entries)
		{
			require(item->offset == offset);
			offset += item->bytes;
		}
		require(offset + header_bytes + 32 <= FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES);
	}
}
std::vector<uint8_t> encode_index(const bucket_index &index)
{
	validate_index(index);
	std::vector<uint8_t> payload;
	raw(payload, index.lineage.bytes);
	number(payload, index.bucket, 4);
	number(payload, index.entries.size(), 4);
	number(payload, index.bytes, 8);
	for (const auto &item : index.entries)
	{
		raw(payload, item.id.bytes);
		raw(payload, item.digest);
		number(payload, item.segment, 4);
		number(payload, item.offset, 4);
		number(payload, item.bytes, 4);
		number(payload, 0, 4);
	}
	return envelope(index_magic, payload);
}
bucket_index decode_index(size_t bucket, std::span<const uint8_t> bytes)
{
	reader input{ unwrap(bytes, index_magic, index_max_bytes) };
	bucket_index value;
	auto lineage = input.take(16);
	std::copy(lineage.begin(), lineage.end(), value.lineage.bytes.begin());
	value.bucket = static_cast<uint32_t>(input.number(4));
	const auto count = input.number(4);
	value.bytes = input.number(8);
	require(value.bucket == bucket && count <= FLATFILE_ACCOUNTING_BUCKET_RECORDS &&
		bytes.size() == header_bytes + 32 + count * 64);
	value.entries.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		entry item;
		auto id = input.take(16), hash = input.take(32);
		std::copy(id.begin(), id.end(), item.id.bytes.begin());
		std::copy(hash.begin(), hash.end(), item.digest.begin());
		item.segment = static_cast<uint32_t>(input.number(4));
		item.offset = static_cast<uint32_t>(input.number(4));
		item.bytes = static_cast<uint32_t>(input.number(4));
		require(input.number(4) == 0);
		value.entries.push_back(item);
	}
	input.done();
	validate_index(value);
	return value;
}
std::string directory(const std::string &root)
{
	return root + "/economic-evidence";
}
flatfile_read_result read(const std::string &root, const std::string &name, size_t limit,
			  std::vector<uint8_t> *bytes, std::string *error)
{
	const auto result = flatfile_read(directory(root), name, limit, bytes, error);
	require(result == flatfile_read_result::ok || result == flatfile_read_result::not_found,
		result == flatfile_read_result::io_error ? status::io_error : status::invalid);
	return result;
}
void recover(const std::string &root, const flatfile_authority_lock &lock, std::string *error)
{
	require(lock.matches(root));
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	require(result == flatfile_authority_transaction_result::ok,
		result == flatfile_authority_transaction_result::io_error ? status::io_error :
									    status::invalid);
}
bucket_index load_index(const std::string &root, size_t bucket, std::string *error)
{
	std::vector<uint8_t> bytes;
	require(read(root, index_name(bucket), index_max_bytes, &bytes, error) ==
		flatfile_read_result::ok);
	return decode_index(bucket, bytes);
}
std::vector<uint8_t> encode_segment(const bucket_index &index, uint32_t segment,
				    std::span<const uint8_t> records)
{
	std::vector<uint8_t> payload;
	payload.reserve(32 + records.size());
	raw(payload, index.lineage.bytes);
	number(payload, index.bucket, 4);
	number(payload, segment, 4);
	number(payload, segment_entries(index, segment).size(), 4);
	number(payload, 0, 4);
	raw(payload, records);
	require(header_bytes + payload.size() <= FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES);
	return envelope(segment_magic, payload);
}
std::vector<uint8_t> load_segment(const std::string &root, const bucket_index &index,
				  uint32_t segment, std::string *error)
{
	std::vector<uint8_t> bytes;
	require(read(root, segment_name(index.bucket, segment),
		     FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES, &bytes,
		     error) == flatfile_read_result::ok);
	reader input{ unwrap(bytes, segment_magic, FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES) };
	auto lineage = input.take(16);
	require(std::equal(lineage.begin(), lineage.end(), index.lineage.bytes.begin()));
	require(input.number(4) == index.bucket && input.number(4) == segment);
	auto entries = segment_entries(index, segment);
	require(input.number(4) == entries.size() && input.number(4) == 0);
	for (const auto *item : entries)
		require(digest(input.take(item->bytes)) == item->digest);
	input.done();
	return bytes;
}
struct context
{
	bucket_index index;
	std::vector<uint8_t> active;
};
context load_context(const std::string &root, size_t bucket, std::string *error)
{
	context value;
	value.index = load_index(root, bucket, error);
	uint32_t next = 0;
	if (!value.index.entries.empty())
	{
		const auto active = last_segment(value.index);
		value.active = load_segment(root, value.index, active, error);
		next = active + 1;
	}
	std::vector<uint8_t> extra;
	require(read(root, segment_name(bucket, next), FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES,
		     &extra, error) == flatfile_read_result::not_found);
	return value;
}
flatfile_accounting_record lookup_in(const std::string &root, const context &value,
				     const critical_command &command, std::string *error)
{
	auto found = std::lower_bound(value.index.entries.begin(), value.index.entries.end(),
				      command.operation_id.bytes,
				      [](const entry &item, const auto &id)
				      { return item.id.bytes < id; });
	require(found != value.index.entries.end() && found->id.bytes == command.operation_id.bytes,
		status::not_found);
	std::vector<uint8_t> older;
	const auto *bytes = &value.active;
	if (found->segment != last_segment(value.index))
	{
		older = load_segment(root, value.index, found->segment, error);
		bytes = &older;
	}
	auto record = decode_record(std::span<const uint8_t>(*bytes).subspan(
		header_bytes + 32 + found->offset, found->bytes));
	economic_frozen_intent intent;
	checked(economic_intent_decode(record.command.accounting_intent, &intent));
	require(intent.admission.metadata.lineage.bytes == value.index.lineage.bytes &&
		record.command.operation_id.bytes == command.operation_id.bytes);
	std::vector<uint8_t> expected, actual;
	command_checked(critical_command_encode(command, &expected));
	command_checked(critical_command_encode(record.command, &actual));
	require(expected == actual, status::conflict);
	return record;
}
void validate_append(const std::vector<flatfile_authority_operation> *operations, size_t count)
{
	require(operations && count <= flatfile_authority_transaction_maximum_operations &&
			operations->size() <=
				flatfile_authority_transaction_maximum_operations - count,
		status::capacity);
	for (const auto &operation : *operations)
		require(operation.store != flatfile_authority_store::economic_evidence);
}
size_t journal_bytes(const std::vector<flatfile_authority_operation> &operations)
{
	size_t size = 50;
	for (const auto &operation : operations)
	{
		require(operation.filename.size() <= 192 &&
				operation.bytes.size() <=
					flatfile_authority_transaction_maximum_bytes,
			status::capacity);
		const size_t extra = 8 + operation.filename.size() + operation.bytes.size();
		require(extra <= flatfile_authority_transaction_maximum_bytes - size,
			status::capacity);
		size += extra;
	}
	return size;
}
void require_room(const std::vector<flatfile_authority_operation> &operations, size_t extra)
{
	require(extra <= flatfile_authority_transaction_maximum_bytes - journal_bytes(operations),
		status::capacity);
}
void append(std::vector<flatfile_authority_operation> &operations, std::string name,
	    std::vector<uint8_t> bytes)
{
	operations.push_back({ flatfile_authority_store::economic_evidence,
			       flatfile_authority_operation_kind::write, std::move(name),
			       std::move(bytes) });
}
void require_empty_bucket(const std::string &root, size_t bucket)
{
	const auto prefix = bucket_prefix(bucket);
	const int fd =
		open(directory(root).c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	require(fd >= 0, status::io_error);
	struct stat info = {};
	const bool safe = fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
			  info.st_uid == geteuid() && !(info.st_mode & 0077);
	if (!safe)
	{
		close(fd);
		throw failure{ status::invalid };
	}
	DIR *dir = fdopendir(fd);
	if (!dir)
	{
		close(fd);
		throw failure{ status::io_error };
	}
	bool empty = true;
	errno = 0;
	while (auto *item = readdir(dir))
	{
		if (!strncmp(item->d_name, prefix.c_str(), prefix.size()))
		{
			empty = false;
			break;
		}
	}
	const int error = errno;
	closedir(dir);
	require(!error, status::io_error);
	require(empty, status::already_exists);
}
template <typename Work> status guarded(Work work, std::string *error = nullptr)
{
	try
	{
		work();
		return status::ok;
	}
	catch (const failure &failure)
	{
		try
		{
			if (error && error->empty())
				*error = "flatfile accounting storage refused";
		}
		catch (const std::bad_alloc &)
		{
		}
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
}
}

flatfile_accounting_status
flatfile_accounting_record_encode(const flatfile_accounting_record &record,
				  std::vector<uint8_t> *bytes)
{
	return guarded(
		[&]
		{
			require(bytes);
			auto encoded = encode_record(record);
			*bytes = std::move(encoded);
		});
}
flatfile_accounting_status flatfile_accounting_record_decode(std::span<const uint8_t> bytes,
							     flatfile_accounting_record *record)
{
	return guarded(
		[&]
		{
			require(record);
			auto decoded = decode_record(bytes);
			*record = std::move(decoded);
		});
}
flatfile_accounting_status flatfile_accounting_storage::initialize_bucket(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, size_t bucket,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
{
	return guarded(
		[&]
		{
			validate_append(operations, 1);
			require(bucket < FLATFILE_ACCOUNTING_BUCKETS);
			recover(root, lock, error);
			require_empty_bucket(root, bucket);
			bucket_index value;
			value.lineage = lineage;
			value.bucket = static_cast<uint32_t>(bucket);
			auto bytes = encode_index(value);
			require_room(*operations, 8 + index_name(bucket).size() + bytes.size());
			auto result = *operations;
			append(result, index_name(bucket), std::move(bytes));
			*operations = std::move(result);
		},
		error);
}
flatfile_accounting_status flatfile_accounting_lookup(const std::string &root,
						      const flatfile_authority_lock &lock,
						      const critical_command &command,
						      flatfile_accounting_record *record,
						      std::string *error)
{
	return guarded(
		[&]
		{
			require(record && critical_command_envelope_valid(command));
			recover(root, lock, error);
			auto value = load_context(root, bucket_for(command.operation_id), error);
			auto retained = lookup_in(root, value, command, error);
			*record = std::move(retained);
		},
		error);
}
flatfile_accounting_status
flatfile_accounting_storage::stage(const std::string &root, const flatfile_authority_lock &lock,
				   const flatfile_accounting_record &record,
				   std::vector<flatfile_authority_operation> *operations,
				   std::string *error)
{
	return guarded(
		[&]
		{
			validate_append(operations, 2);
			require(critical_command_envelope_valid(record.command));
			recover(root, lock, error);
			auto value =
				load_context(root, bucket_for(record.command.operation_id), error);
			try
			{
				(void)lookup_in(root, value, record.command, error);
				throw failure{ status::already_exists };
			}
			catch (const failure &lookup)
			{
				if (lookup.code != status::not_found)
					throw;
			}
			auto bytes = encode_record(record);
			auto &index = value.index;
			economic_frozen_intent intent;
			checked(economic_intent_decode(record.command.accounting_intent, &intent));
			require(intent.admission.metadata.lineage.bytes == index.lineage.bytes);
			require(index.entries.size() < FLATFILE_ACCOUNTING_BUCKET_RECORDS &&
					bytes.size() <=
						FLATFILE_ACCOUNTING_BUCKET_MAX_BYTES - index.bytes,
				status::capacity);
			uint32_t segment = last_segment(index);
			std::vector<uint8_t> records;
			if (!value.active.empty() && value.active.size() + bytes.size() <=
							     FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES)
				records.assign(value.active.begin() + header_bytes + 32,
					       value.active.end());
			else if (!value.active.empty())
				++segment;
			require(segment < FLATFILE_ACCOUNTING_BUCKET_SEGMENTS, status::capacity);
			entry item{ record.command.operation_id, digest(bytes), segment,
				    static_cast<uint32_t>(records.size()),
				    static_cast<uint32_t>(bytes.size()) };
			raw(records, bytes);
			index.entries.push_back(item);
			index.bytes += bytes.size();
			std::sort(index.entries.begin(), index.entries.end(),
				  [](const auto &a, const auto &b)
				  { return a.id.bytes < b.id.bytes; });
			auto encoded_index = encode_index(index);
			auto encoded_segment = encode_segment(index, segment, records);
			require_room(*operations, 16 + segment_name(index.bucket, segment).size() +
							  index_name(index.bucket).size() +
							  encoded_segment.size() +
							  encoded_index.size());
			auto result = *operations;
			append(result, segment_name(index.bucket, segment),
			       std::move(encoded_segment));
			append(result, index_name(index.bucket), std::move(encoded_index));
			*operations = std::move(result);
		},
		error);
}

flatfile_accounting_status flatfile_accounting_check_bucket(const std::string &root,
							    const flatfile_authority_lock &lock,
							    const critical_operation_id &lineage,
							    size_t bucket, std::string *error)
{
	return guarded(
		[&]
		{
			require(bucket < FLATFILE_ACCOUNTING_BUCKETS &&
				!critical_operation_id_is_zero(lineage));
			recover(root, lock, error);
			auto value = load_context(root, bucket, error);
			require(value.index.lineage.bytes == lineage.bytes);
		},
		error);
}
