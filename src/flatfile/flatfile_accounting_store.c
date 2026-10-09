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
constexpr std::array<uint8_t, 8> source_claim_magic = { 'D', 'U', 'R', 'S', 'C', 'L', '1', 0 };
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
flatfile_accounting_record retained_in(const std::string &root, const context &value,
				       const critical_operation_id &operation, std::string *error)
{
	auto found = std::lower_bound(value.index.entries.begin(), value.index.entries.end(),
				      operation.bytes, [](const entry &item, const auto &id)
				      { return item.id.bytes < id; });
	require(found != value.index.entries.end() && found->id.bytes == operation.bytes,
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
		record.command.operation_id.bytes == operation.bytes);
	return record;
}
flatfile_accounting_record lookup_in(const std::string &root, const context &value,
				     const critical_command &command, std::string *error)
{
	auto record = retained_in(root, value, command.operation_id, error);
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
std::string source_claim_name(const economic_operation_metadata &metadata)
{
	require(metadata.source_event.has_value());
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> event = {};
	checked(economic_source_event_encode(*metadata.source_event, &event));
	std::vector<uint8_t> key;
	raw(key, metadata.lineage.bytes);
	raw(key, event);
	const auto key_digest = digest(key);
	static constexpr char digits[] = "0123456789abcdef";
	std::string name = "source-claim-";
	name.reserve(name.size() + key_digest.size() * 2 + 4);
	for (uint8_t byte : key_digest)
	{
		name.push_back(digits[byte >> 4]);
		name.push_back(digits[byte & 0x0f]);
	}
	name += ".bin";
	return name;
}
std::vector<uint8_t> source_claim_bytes(const flatfile_accounting_record &record)
{
	economic_frozen_intent intent;
	checked(economic_intent_decode(record.command.accounting_intent, &intent));
	require(intent.admission.metadata.source_event.has_value());
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> event = {};
	checked(economic_source_event_encode(*intent.admission.metadata.source_event, &event));
	std::vector<uint8_t> payload;
	raw(payload, intent.admission.metadata.lineage.bytes);
	raw(payload, event);
	raw(payload, record.command.operation_id.bytes);
	number(payload, 1, 1);
	number(payload, 0, 7);
	return envelope(source_claim_magic, payload);
}
flatfile_read_result read_source_claim(const std::string &root,
				       const economic_operation_metadata &metadata,
				       std::vector<uint8_t> *bytes, std::string *error)
{
	return read(root, source_claim_name(metadata), 256, bytes, error);
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
flatfile_accounting_status flatfile_accounting_storage::lookup_retained_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, flatfile_accounting_record *record,
	std::string *error)
{
	return guarded(
		[&]
		{
			require(record && !critical_operation_id_is_zero(operation));
			recover(root, lock, error);
			auto value = load_context(root, bucket_for(operation), error);
			auto retained = retained_in(root, value, operation, error);
			*record = std::move(retained);
		},
		error);
}
flatfile_accounting_status flatfile_accounting_storage::list_retained_bucket_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, size_t bucket,
	std::vector<flatfile_accounting_record> *records, std::string *error)
{
	return guarded(
		[&]
		{
			require(records && !critical_operation_id_is_zero(lineage) &&
				bucket < FLATFILE_ACCOUNTING_BUCKETS);
			recover(root, lock, error);
			auto value = load_context(root, bucket, error);
			require(value.index.lineage.bytes == lineage.bytes, status::conflict);
			std::vector<flatfile_accounting_record> candidate;
			candidate.reserve(value.index.entries.size());
			if (!value.index.entries.empty())
				for (uint32_t segment = 0; segment <= last_segment(value.index);
				     ++segment)
				{
					const auto bytes = segment == last_segment(value.index) ?
						std::move(value.active) :
						load_segment(root, value.index, segment, error);
					for (const auto *item : segment_entries(value.index, segment))
					{
						auto record = decode_record(std::span<const uint8_t>(bytes)
							.subspan(header_bytes + 32 + item->offset, item->bytes));
						economic_frozen_intent intent;
						checked(economic_intent_decode(record.command.accounting_intent,
							&intent));
						require(intent.admission.metadata.lineage.bytes == lineage.bytes &&
							record.command.operation_id.bytes == item->id.bytes);
						candidate.push_back(std::move(record));
					}
				}
			std::sort(candidate.begin(), candidate.end(), [](const auto &a, const auto &b)
				{ return a.command.operation_id.bytes < b.command.operation_id.bytes; });
			*records = std::move(candidate);
		}, error);
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
flatfile_accounting_status flatfile_accounting_storage::stage_source_claim(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
{
	return guarded(
		[&]
		{
			require(lock.owns(root) && operations);
			economic_frozen_intent intent;
			checked(economic_intent_decode(record.command.accounting_intent, &intent));
			if (record.result_code || !intent.admission.metadata.source_event)
				return;
			require(operations->size() <
					flatfile_authority_transaction_maximum_operations,
				status::capacity);
			const std::string name = source_claim_name(intent.admission.metadata);
			for (const auto &operation : *operations)
				require(operation.store !=
						flatfile_authority_store::economic_evidence ||
					operation.filename != name);
			std::vector<uint8_t> existing;
			const auto read_result = read_source_claim(root, intent.admission.metadata,
								   &existing, error);
			require(read_result == flatfile_read_result::not_found,
				read_result == flatfile_read_result::io_error ?
					status::io_error :
				read_result == flatfile_read_result::invalid ?
					status::invalid :
					status::already_exists);
			auto encoded = source_claim_bytes(record);
			require_room(*operations, 8 + name.size() + encoded.size());
			append(*operations, name, std::move(encoded));
		},
		error);
}
flatfile_accounting_status flatfile_accounting_storage::verify_source_claim(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record, std::string *error)
{
	return guarded(
		[&]
		{
			require(lock.owns(root));
			economic_frozen_intent intent;
			checked(economic_intent_decode(record.command.accounting_intent, &intent));
			if (!intent.admission.metadata.source_event)
				return;
			std::vector<uint8_t> retained;
			const auto read_result = read_source_claim(root, intent.admission.metadata,
								   &retained, error);
			if (record.result_code)
			{
				require(read_result == flatfile_read_result::not_found,
					read_result == flatfile_read_result::io_error ?
						status::io_error :
					read_result == flatfile_read_result::invalid ?
						status::invalid :
						status::conflict);
				return;
			}
			require(read_result == flatfile_read_result::ok,
				read_result == flatfile_read_result::io_error ? status::io_error :
				read_result == flatfile_read_result::invalid  ? status::invalid :
										status::not_found);
			require(retained == source_claim_bytes(record), status::conflict);
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool receipt_bounded_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
void receipt_bounded_admit(size_t outer, size_t amount,
			   flatfile_scratch_reserve_fn reserve_scratch_peak, void *context)
{
	require(receipt_bounded_add(outer, amount) && reserve_scratch_peak(outer, context),
		status::capacity);
}
constexpr size_t receipt_unwrap_working =
	sizeof(reader) + sizeof(std::span<const uint8_t>) * 5 + sizeof(economic_digest);
struct receipt_read_workspace
{
	receipt_read_workspace(size_t directory_size, size_t name_size)
		: directory(directory_size, '\0')
		, name(name_size, '\0')
	{
	}
	std::string directory, name;
};
flatfile_read_result receipt_read_bounded(const std::string &root, size_t bucket, bool segment_file,
					  size_t segment, size_t limit, std::vector<uint8_t> *bytes,
					  flatfile_scratch_reserve_fn reserve_scratch_peak,
					  void *context, size_t outer)
{
	require(bucket < FLATFILE_ACCOUNTING_BUCKETS);
	if (segment_file)
		require(segment <= FLATFILE_ACCOUNTING_BUCKET_SEGMENTS);
	size_t digits = 1;
	for (size_t value = segment; value >= 10; value /= 10)
		++digits;
	const size_t name_size = segment_file ? 9 + 1 + digits + 4 : 9 + 4;
	size_t directory_size = root.size(),
	       request = sizeof(receipt_read_workspace) + sizeof(char[17]);
	require(receipt_bounded_add(directory_size, sizeof("/economic-evidence") - 1),
		status::capacity);
	if (directory_size > 15)
		require(directory_size != SIZE_MAX &&
				receipt_bounded_add(request, directory_size + 1),
			status::capacity);
	if (name_size > 15)
		require(receipt_bounded_add(request, name_size + 1), status::capacity);
	receipt_bounded_admit(outer, request, reserve_scratch_peak, context);
	receipt_read_workspace work(directory_size, name_size);
	std::copy(root.begin(), root.end(), work.directory.begin());
	std::copy_n("/economic-evidence", sizeof("/economic-evidence") - 1,
		    work.directory.begin() + root.size());
	std::copy_n("bucket-", 7, work.name.begin());
	constexpr char hex[] = "0123456789abcdef";
	work.name[7] = hex[bucket >> 4];
	work.name[8] = hex[bucket & 15];
	if (segment_file)
	{
		work.name[9] = '-';
		size_t value = segment;
		for (size_t position = 10 + digits; position > 10; value /= 10)
			work.name[--position] = static_cast<char>('0' + value % 10);
		std::copy_n(".eas", 4, work.name.begin() + 10 + digits);
	}
	else
		std::copy_n(".eai", 4, work.name.begin() + 9);
	const auto loaded = flatfile_read_bounded(work.directory, work.name, limit, bytes,
						  reserve_scratch_peak, context, outer + request);
	require(loaded == flatfile_read_result::ok || loaded == flatfile_read_result::not_found,
		loaded == flatfile_read_result::io_error ? status::io_error : status::invalid);
	return loaded;
}

// Every original segment_entries call grows a fresh vector by one-pointer
// push_back. Admit each old/new request overlap before the original call.
size_t receipt_segment_entries_working(const bucket_index &index, uint32_t segment)
{
	size_t count = 0;
	for (const auto &item : index.entries)
		if (item.segment == segment)
			++count;
	size_t capacity = 0, peak = 0;
	for (size_t size = 0; size < count; ++size)
		if (size == capacity)
		{
			const size_t next = size ? size * 2 : 1;
			require(next <= SIZE_MAX / sizeof(const entry *) &&
					capacity <= SIZE_MAX / sizeof(const entry *),
				status::capacity);
			size_t simultaneous = capacity * sizeof(const entry *);
			require(receipt_bounded_add(simultaneous, next * sizeof(const entry *)),
				status::capacity);
			peak = std::max(peak, simultaneous);
			capacity = next;
		}
	require(receipt_bounded_add(peak, sizeof(std::vector<const entry *>) * 2),
		status::capacity);
	return peak;
}

void receipt_validate_index_bounded(const bucket_index &index,
				    flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
				    size_t outer)
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
		receipt_bounded_admit(outer, receipt_segment_entries_working(index, segment),
				      reserve_scratch_peak, context);
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
bucket_index receipt_decode_index_bounded(size_t bucket, const std::span<const uint8_t> &bytes,
					  flatfile_scratch_reserve_fn reserve_scratch_peak,
					  void *context, size_t outer)
{
	const size_t frame = sizeof(reader) + sizeof(bucket_index) + sizeof(entry) +
			     sizeof(std::span<const uint8_t>) * 3 + receipt_unwrap_working;
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context);
	reader input{ unwrap(bytes, index_magic, index_max_bytes) };
	bucket_index value;
	auto lineage = input.take(16);
	std::copy(lineage.begin(), lineage.end(), value.lineage.bytes.begin());
	value.bucket = static_cast<uint32_t>(input.number(4));
	const auto count = input.number(4);
	value.bytes = input.number(8);
	require(value.bucket == bucket && count <= FLATFILE_ACCOUNTING_BUCKET_RECORDS &&
		bytes.size() == header_bytes + 32 + count * 64);
	require(count <= SIZE_MAX / sizeof(entry), status::capacity);
	const size_t heap = count * sizeof(entry);
	receipt_bounded_admit(outer + frame, heap, reserve_scratch_peak, context);
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
	receipt_validate_index_bounded(value, reserve_scratch_peak, context, outer + frame + heap);
	return value;
}

bucket_index receipt_load_index_bounded(const std::string &root, size_t bucket,
					flatfile_scratch_reserve_fn reserve_scratch_peak,
					void *context, size_t outer)
{
	const size_t frame = sizeof(std::vector<uint8_t>) + sizeof(std::span<const uint8_t>);
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context);
	std::vector<uint8_t> bytes;
	require(receipt_read_bounded(root, bucket, false, 0, index_max_bytes, &bytes,
				     reserve_scratch_peak, context,
				     outer + frame) == flatfile_read_result::ok);
	const std::span<const uint8_t> wire(bytes);
	size_t live = outer + frame;
	require(receipt_bounded_add(live, bytes.capacity()), status::capacity);
	return receipt_decode_index_bounded(bucket, wire, reserve_scratch_peak, context, live);
}

std::vector<uint8_t> receipt_load_segment_bounded(const std::string &root,
						  const bucket_index &index, uint32_t segment,
						  flatfile_scratch_reserve_fn reserve_scratch_peak,
						  void *context, size_t outer)
{
	const size_t frame = sizeof(std::vector<uint8_t>) + sizeof(reader) +
			     sizeof(std::span<const uint8_t>) + sizeof(std::vector<const entry *>) +
			     receipt_unwrap_working + sizeof(economic_digest);
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context);
	std::vector<uint8_t> bytes;
	require(receipt_read_bounded(root, index.bucket, true, segment,
				     FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES, &bytes,
				     reserve_scratch_peak, context,
				     outer + frame) == flatfile_read_result::ok);
	size_t live = outer + frame;
	require(receipt_bounded_add(live, bytes.capacity()), status::capacity);
	receipt_bounded_admit(live, 0, reserve_scratch_peak, context);
	reader input{ unwrap(bytes, segment_magic, FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES) };
	auto lineage = input.take(16);
	require(std::equal(lineage.begin(), lineage.end(), index.lineage.bytes.begin()));
	require(input.number(4) == index.bucket && input.number(4) == segment);
	receipt_bounded_admit(live, receipt_segment_entries_working(index, segment),
			      reserve_scratch_peak, context);
	auto entries = segment_entries(index, segment);
	require(input.number(4) == entries.size() && input.number(4) == 0);
	for (const auto *item : entries)
		require(digest(input.take(item->bytes)) == item->digest);
	input.done();
	return bytes;
}

context receipt_load_context_bounded(const std::string &root, size_t bucket,
				     flatfile_scratch_reserve_fn reserve_scratch_peak,
				     void *context_pointer, size_t outer)
{
	const size_t frame = sizeof(context) + sizeof(std::vector<uint8_t>);
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context_pointer);
	context value;
	value.index = receipt_load_index_bounded(root, bucket, reserve_scratch_peak,
						 context_pointer, outer + frame);
	size_t live = outer + frame;
	require(value.index.entries.capacity() <= SIZE_MAX / sizeof(entry) &&
			receipt_bounded_add(live, value.index.entries.capacity() * sizeof(entry)),
		status::capacity);
	uint32_t next = 0;
	if (!value.index.entries.empty())
	{
		const auto active = last_segment(value.index);
		value.active = receipt_load_segment_bounded(
			root, value.index, active, reserve_scratch_peak, context_pointer, live);
		next = active + 1;
	}
	require(receipt_bounded_add(live, value.active.capacity()), status::capacity);
	std::vector<uint8_t> extra;
	require(receipt_read_bounded(root, bucket, true, next,
				     FLATFILE_ACCOUNTING_SEGMENT_MAX_BYTES, &extra,
				     reserve_scratch_peak, context_pointer,
				     live) == flatfile_read_result::not_found);
	return value;
}

size_t receipt_command_heap(const critical_command &command)
{
	size_t total = 0;
	require(command.keys.capacity() <= SIZE_MAX / sizeof(critical_entity_key) &&
			command.expected_revisions.capacity() <=
				SIZE_MAX / sizeof(critical_expected_revision) &&
			receipt_bounded_add(total, command.keys.capacity() *
							   sizeof(critical_entity_key)) &&
			receipt_bounded_add(total, command.expected_revisions.capacity() *
							   sizeof(critical_expected_revision)) &&
			receipt_bounded_add(total, command.payload.capacity()) &&
			receipt_bounded_add(total, command.accounting_intent.capacity()),
		status::capacity);
	return total;
}
size_t receipt_record_heap(const flatfile_accounting_record &record)
{
	size_t total = receipt_command_heap(record.command);
	require(receipt_bounded_add(total, record.plan.capacity()) &&
			receipt_bounded_add(total, record.result.capacity()),
		status::capacity);
	return total;
}

std::vector<uint8_t>
receipt_validate_record_bounded(const flatfile_accounting_record &record,
				flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
				size_t outer)
{
	require(record.command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		critical_command_envelope_valid(record.command));
	require(critical_failure_stage_valid(record.failure_stage) &&
		(record.result_code || record.failure_stage == critical_failure_stage::none));
	require(record.result.size() <= CRITICAL_COMPLETION_RESULT_MAX_BYTES &&
		record.plan.size() <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES);
	const size_t frame = sizeof(economic_frozen_intent) + sizeof(economic_plan_metadata) +
			     sizeof(std::span<const uint8_t>);
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context);
	economic_frozen_intent intent;
	const std::span<const uint8_t> intent_wire(record.command.accounting_intent);
	checked(economic_intent_decode_bounded(intent_wire, &intent, reserve_scratch_peak, context,
					       outer + frame));
	size_t live = outer + frame;
	require(receipt_bounded_add(live, intent.admission.facts.capacity()), status::capacity);
	economic_plan_metadata metadata;
	checked(economic_intent_plan_metadata_bounded(record.command, intent, &metadata,
						      reserve_scratch_peak, context, live));
	if (record.result_code)
		require(record.plan.empty());
	else
	{
		const size_t plan_frame = sizeof(economic_accounting_plan) +
					  sizeof(std::vector<uint8_t>) +
					  sizeof(economic_accounting_plan_allocation_profile) +
					  sizeof(std::span<const uint8_t>);
		receipt_bounded_admit(live, plan_frame, reserve_scratch_peak, context);
		economic_accounting_plan plan;
		const std::span<const uint8_t> plan_wire(record.plan);
		size_t plan_heap = 0;
		checked(economic_plan_decode_bounded(plan_wire, &plan, reserve_scratch_peak,
						     context, live + plan_frame, &plan_heap));
		// Preserve the original child-reservation prohibition and metadata proof.
		require(plan.children.empty());
		plan.metadata = metadata;
		std::vector<uint8_t> canonical;
		economic_accounting_plan_allocation_profile profile;
		size_t plan_live = live + plan_frame;
		require(receipt_bounded_add(plan_live, plan_heap), status::capacity);
		receipt_bounded_admit(plan_live, economic_plan_allocation_preflight_working_bytes(),
				      reserve_scratch_peak, context);
		checked(economic_plan_allocation_preflight(plan, &profile));
		require(profile.storage_policy_supported, status::capacity);
		receipt_bounded_admit(plan_live, profile.encode_working_bytes, reserve_scratch_peak,
				      context);
		checked(economic_plan_encode(plan, &canonical));
		require(canonical == record.plan);
	}
	// Plan/canonical/profile die before the command transport comparison vector.
	receipt_bounded_admit(live, sizeof(std::vector<uint8_t>), reserve_scratch_peak, context);
	std::vector<uint8_t> command;
	command_checked(critical_command_encode_bounded(record.command, &command,
							reserve_scratch_peak, context,
							live + sizeof(std::vector<uint8_t>)));
	return command;
}

flatfile_accounting_record
receipt_decode_record_bounded(const std::span<const uint8_t> &bytes,
			      flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
			      size_t outer)
{
	const size_t frame = sizeof(reader) + sizeof(flatfile_accounting_record) +
			     sizeof(std::span<const uint8_t>) * 3 + sizeof(std::vector<uint8_t>);
	receipt_bounded_admit(outer, frame + receipt_unwrap_working, reserve_scratch_peak, context);
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
	size_t command_heap = 0;
	command_checked(critical_command_decode_bounded(command.data(), command.size(),
							&result.command, reserve_scratch_peak,
							context, outer + frame, &command_heap));
	auto plan = input.take(plan_size), payload = input.take(result_size);
	input.done();
	size_t live = outer + frame;
	require(receipt_bounded_add(live, command_heap) && receipt_bounded_add(live, plan_size) &&
			receipt_bounded_add(live, result_size),
		status::capacity);
	receipt_bounded_admit(live, 0, reserve_scratch_peak, context);
	result.plan.assign(plan.begin(), plan.end());
	result.result.assign(payload.begin(), payload.end());
	auto canonical =
		receipt_validate_record_bounded(result, reserve_scratch_peak, context, live);
	require(canonical.size() == command.size() &&
		std::equal(canonical.begin(), canonical.end(), command.begin()));
	return result;
}

flatfile_accounting_record receipt_retained_in_bounded(
	const std::string &root, const context &value, const critical_operation_id &operation,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context_pointer, size_t outer)
{
	auto found = std::lower_bound(value.index.entries.begin(), value.index.entries.end(),
				      operation.bytes, [](const entry &item, const auto &id)
				      { return item.id.bytes < id; });
	require(found != value.index.entries.end() && found->id.bytes == operation.bytes,
		status::not_found);
	const size_t frame = sizeof(std::vector<uint8_t>) + sizeof(flatfile_accounting_record) +
			     sizeof(economic_frozen_intent) + sizeof(std::span<const uint8_t>) * 3;
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context_pointer);
	std::vector<uint8_t> older;
	const auto *bytes = &value.active;
	if (found->segment != last_segment(value.index))
	{
		older = receipt_load_segment_bounded(root, value.index, found->segment,
						     reserve_scratch_peak, context_pointer,
						     outer + frame);
		bytes = &older;
	}
	size_t live = outer + frame;
	require(receipt_bounded_add(live, older.capacity()), status::capacity);
	const auto record_wire = std::span<const uint8_t>(*bytes).subspan(
		header_bytes + 32 + found->offset, found->bytes);
	auto record = receipt_decode_record_bounded(record_wire, reserve_scratch_peak,
						    context_pointer, live);
	require(receipt_bounded_add(live, receipt_record_heap(record)), status::capacity);
	economic_frozen_intent intent;
	const std::span<const uint8_t> intent_wire(record.command.accounting_intent);
	checked(economic_intent_decode_bounded(intent_wire, &intent, reserve_scratch_peak,
					       context_pointer, live));
	require(intent.admission.metadata.lineage.bytes == value.index.lineage.bytes &&
		record.command.operation_id.bytes == operation.bytes);
	return record;
}

flatfile_accounting_record receipt_lookup_in_bounded(
	const std::string &root, const context &value, const critical_command &command,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context_pointer, size_t outer)
{
	const size_t frame = sizeof(flatfile_accounting_record) + sizeof(std::vector<uint8_t>) * 2;
	receipt_bounded_admit(outer, frame, reserve_scratch_peak, context_pointer);
	auto record = receipt_retained_in_bounded(root, value, command.operation_id,
						  reserve_scratch_peak, context_pointer,
						  outer + frame);
	size_t live = outer + frame;
	require(receipt_bounded_add(live, receipt_record_heap(record)), status::capacity);
	std::vector<uint8_t> expected, actual;
	command_checked(critical_command_encode_bounded(command, &expected, reserve_scratch_peak,
							context_pointer, live));
	require(receipt_bounded_add(live, expected.capacity()), status::capacity);
	command_checked(critical_command_encode_bounded(
		record.command, &actual, reserve_scratch_peak, context_pointer, live));
	require(expected == actual, status::conflict);
	return record;
}
#endif
}

flatfile_accounting_status flatfile_accounting_lookup_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, flatfile_accounting_record *record,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context_pointer, size_t outer,
	size_t *retained_payload) noexcept
{
	if (!record || !critical_command_envelope_valid(command) || !reserve_scratch_peak)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)context_pointer;
	(void)outer;
	(void)retained_payload;
	errno = ENOTSUP;
	return status::io_error;
#else
	if (!lock.matches(root))
		return status::invalid;
	const auto recovered = flatfile_authority_transaction_recover_bounded(
		root, lock, reserve_scratch_peak, context_pointer, outer);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       status::io_error :
			       status::invalid;
	try
	{
		const size_t frame = sizeof(context) + sizeof(flatfile_accounting_record);
		receipt_bounded_admit(outer, frame, reserve_scratch_peak, context_pointer);
		auto value = receipt_load_context_bounded(root, bucket_for(command.operation_id),
							  reserve_scratch_peak, context_pointer,
							  outer + frame);
		size_t live = outer + frame;
		require(value.index.entries.capacity() <= SIZE_MAX / sizeof(entry) &&
				receipt_bounded_add(live, value.index.entries.capacity() *
								  sizeof(entry)) &&
				receipt_bounded_add(live, value.active.capacity()),
			status::capacity);
		auto retained = receipt_lookup_in_bounded(
			root, value, command, reserve_scratch_peak, context_pointer, live);
		const size_t transferred = receipt_record_heap(retained);
		*record = std::move(retained);
		if (retained_payload)
			*retained_payload = transferred;
		return status::ok;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (...)
	{
		return status::io_error;
	}
#endif
}
